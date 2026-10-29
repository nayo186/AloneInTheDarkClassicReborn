///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// BGFX hardware-accelerated 3D rendering pipeline
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "settings.h"

/***************************************************************************
mainSDL.cpp  -  description
-------------------
begin                : Mon Jun 3 2002
copyright            : (C) 2002 by Yaz0r
email                : yaz0r@yaz0r.net
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "osystem.h"
#include <bgfx/bgfx.h>
#include <bx/platform.h>
#include "shaders/embeddedShaders.h"
#include "imguiBGFX.h"
#include "embedded/embeddedData.h"
#include "settings.h"
#include "input.h"
#include "screenEffects.h"
#include "platformHooks.h"
#include <array>
#include <SDL.h>
#include <string>

// stb_image_write for mask dumping (static to avoid linker conflicts)
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../ThirdParty/bgfx.cmake/bimg/3rdparty/stb/stb_image_write.h"

// stb_image for loading replacement mask PNGs (static to avoid linker conflicts)
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../ThirdParty/bgfx.cmake/bimg/3rdparty/stb/stb_image.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

// Access to current model atlas from renderer

#include "rendererBGFXInternal.h"

unsigned int gameViewId = 1;

bool g_playWorldActive = false;
bool g_3dFlushedThisFrame = false;
bool g_menuActive = false;
bgfx::TextureHandle g_backgroundTexture = BGFX_INVALID_HANDLE;
bgfx::TextureHandle g_uiLayerTexture = BGFX_INVALID_HANDLE;
bgfx::TextureHandle g_paletteTexture = BGFX_INVALID_HANDLE;

// Frozen scene texture for menu backgrounds (sampleable GPU texture)
bgfx::TextureHandle s_frozenSceneTex = BGFX_INVALID_HANDLE;
int s_frozenSceneWidth = 0;
int s_frozenSceneHeight = 0;

// HD sequence (FMV cutscene) frame override

extern int outputResolution[2];

#define _USE_MATH_DEFINES
#include <math.h>

// globals

unsigned int ditherTexture = 0;

unsigned int    debugFontTexture = 0;

struct maskStruct
{
    bgfx::TextureHandle maskTexture = BGFX_INVALID_HANDLE;
    bgfx::VertexBufferHandle vertexBuffer = BGFX_INVALID_HANDLE;
    int maskX1 = 0;
    int maskY1 = 0;
    int maskX2 = 0;
    int maskY2 = 0;
};

std::vector<std::vector<maskStruct>> maskTextures; // [room][mask]

//vertex buffers for rendering
struct polyVertex
{
    float X;
    float Y;
    float Z;

    float U;
    float V;
};

struct sphereVertex
{
    float X;
    float Y;
    float Z;

    float U;
    float V;

    float centerX;
    float centerY;
    float size;
    float material;
};

#define NUM_MAX_FLAT_VERTICES 5000*3
#define NUM_MAX_NOISE_VERTICES 2000*3
#define NUM_MAX_TRANSPARENT_VERTICES 1000*2
#define NUM_MAX_RAMP_VERTICES 3000*3
#define NUM_MAX_SPHERES_VERTICES 3000*3

std::array<polyVertex, NUM_MAX_FLAT_VERTICES> flatVertices;
std::array<polyVertex, NUM_MAX_NOISE_VERTICES> noiseVertices;
std::array<polyVertex, NUM_MAX_TRANSPARENT_VERTICES> transparentVertices;
std::array<polyVertex, NUM_MAX_RAMP_VERTICES> rampVertices;
std::array<sphereVertex, NUM_MAX_SPHERES_VERTICES> sphereVertices;
std::vector<polyVertex> g_lineVertices;

int numUsedFlatVertices = 0;
int numUsedNoiseVertices = 0;
int numUsedTransparentVertices = 0;

// Painter's order (Original mode). The original engine drew the objects from
// far to near (sortActorList) without comparing depths between them, so the
// nearer one was always in front. Here each object gets its own slice of the
// depth range in that order: nearer objects are always in front of farther
// ones, and inside an object the depth still sorts its own polygons. Without
// it, the player close to the attic window was cut by its frame (BUGS.md V21).
static float s_depthSlice[4] = { 0.0f, 1.0f, 0.0f, 0.0f };  // slice, slice size, active
static bool s_depthSlicedThisFrame = false;

// Glass polygons of this frame (Original mode), drawn after the scene
static std::vector<polyVertex> s_deferredGlass;

void osystem_setDepthSlice(int slice)
{
    if (slice < 0)
    {
        s_depthSlice[0] = 0.0f;
        s_depthSlice[1] = 1.0f;
        s_depthSlice[2] = 0.0f;
        return;
    }
    if (slice > OSYSTEM_DEPTH_SLICES - 1)
        slice = OSYSTEM_DEPTH_SLICES - 1;
    s_depthSlice[0] = (float)slice;
    s_depthSlice[1] = 1.0f / OSYSTEM_DEPTH_SLICES;
    s_depthSlice[2] = 1.0f;
    s_depthSlicedThisFrame = true;
}

bool osystem_depthSlicedThisFrame()
{
    return s_depthSlicedThisFrame;
}

static void applyDepthSlice()
{
    static bgfx::UniformHandle depthSliceUniform = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(depthSliceUniform))
        depthSliceUniform = bgfx::createUniform("u_depthSlice", bgfx::UniformType::Vec4);
    bgfx::setUniform(depthSliceUniform, s_depthSlice);
}

int numUsedRampVertices = 0;
int numUsedSpheres = 0;


//static unsigned long int zoom = 0;

float nearVal = 100;
float farVal = 100000;
float cameraZoom = 0;
float fov = 0;

char RGB_Pal[256 * 4];

unsigned int    backTexture;


static bool s_darkRoomActive = false;
static const float DARK_ROOM_BRIGHTNESS = 0.10f;

static void uploadPaletteWithDarkening()
{
    if (s_darkRoomActive)
    {
        u8 darkPal[256 * 3];
        for (int i = 0; i < 256 * 3; i++)
        {
            darkPal[i] = (u8)(RGB_Pal[i] * DARK_ROOM_BRIGHTNESS);
        }
        bgfx::updateTexture2D(g_paletteTexture, 0, 0, 0, 0, 3, 256, bgfx::copy(darkPal, 256 * 3));
    }
    else
    {
        bgfx::updateTexture2D(g_paletteTexture, 0, 0, 0, 0, 3, 256, bgfx::copy(RGB_Pal, 256 * 3));
    }
}

void osystem_setPalette(u8* palette)
{
    osystem_setScreenFade(1.0f);
    memcpy(RGB_Pal, palette, 256 * 3);
    uploadPaletteWithDarkening();
}

void osystem_setPalette(palette_t* palette)
{
    osystem_setScreenFade(1.0f);
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 3; j++) {
            RGB_Pal[i * 3 + j] = palette->at(i)[j];
        }
    }
    uploadPaletteWithDarkening();
}

void osystem_setDarkRoom(bool dark)
{
    s_darkRoomActive = dark;
    uploadPaletteWithDarkening();
}

struct s_vertexData
{
    float positions[3];
    float textures[2];
    float color[4];
};
s_vertexData gVertexArray[1024 * 1024];

bgfx::ProgramHandle getUIShader()
{
    static bgfx::ProgramHandle programHandle = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(programHandle))
    {
        programHandle = loadBgfxProgram("ui_vs", "ui_ps");
    }

    return programHandle;
}

bgfx::ProgramHandle getBackgroundShader()
{
    static bgfx::ProgramHandle programHandle = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(programHandle))
    {
        programHandle = loadBgfxProgram("background_vs", "background_ps");
    }

    return programHandle;
}

// RGBA picture (the frozen scene behind the menus) with the fade level
static bgfx::ProgramHandle getImageBackgroundShader()
{
    static bgfx::ProgramHandle programHandle = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(programHandle))
    {
        programHandle = loadBgfxProgram("background_vs", "imageBackground_ps");
    }

    return programHandle;
}

void submitImageBackground(bgfx::ViewId viewId)
{
    bgfx::submit(viewId, getImageBackgroundShader());
}

// Dark room: the scene of this frame is covered with black (BUGS.md V23)
static bool s_sceneDarkThisFrame = false;

void osystem_drawDarkness()
{
    static bgfx::TextureHandle black = BGFX_INVALID_HANDLE;
    static bgfx::UniformHandle textureUniform = BGFX_INVALID_HANDLE;
    static bgfx::UniformHandle fadeUniform = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(black))
    {
        const uint32_t pixel = 0xff000000;
        black = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, 0, bgfx::copy(&pixel, 4));
        textureUniform = bgfx::createUniform("s_backgroundTexture", bgfx::UniformType::Sampler);
        fadeUniform = bgfx::createUniform("u_fadeLevel", bgfx::UniformType::Vec4);
    }

    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    bgfx::TransientVertexBuffer buffer;
    bgfx::allocTransientVertexBuffer(&buffer, 6, layout);
    struct Vertex { float position[3]; float texcoord[2]; };
    Vertex* v = (Vertex*)buffer.data;
    const Vertex quad[6] = {
        { { 0.f, 0.f, 0.f }, { 0.f, 0.f } }, { { 320.f, 200.f, 0.f }, { 1.f, 1.f } }, { { 320.f, 0.f, 0.f }, { 1.f, 0.f } },
        { { 0.f, 0.f, 0.f }, { 0.f, 0.f } }, { { 0.f, 200.f, 0.f }, { 0.f, 1.f } }, { { 320.f, 200.f, 0.f }, { 1.f, 1.f } },
    };
    for (int i = 0; i < 6; i++)
        v[i] = quad[i];

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_ALWAYS);
    bgfx::setVertexBuffer(0, &buffer);
    bgfx::setTexture(0, textureUniform, black);
    float fade[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(fadeUniform, fade);
    submitImageBackground(gameViewId);
    s_sceneDarkThisFrame = true;
}

bgfx::ProgramHandle getMaskBackgroundShader()
{
    static bgfx::ProgramHandle programHandle = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(programHandle))
    {
        programHandle = loadBgfxProgram("maskBackground_vs", "maskBackground_ps");
    }

    return programHandle;
}

bgfx::ProgramHandle getFlatShader()
{
    static bgfx::ProgramHandle programHandle = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(programHandle))
    {
        programHandle = loadBgfxProgram("flat_vs", "flat_ps");
    }

    return programHandle;
}

bgfx::ProgramHandle getNoiseShader()
{
    static bgfx::ProgramHandle programHandle = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(programHandle))
    {
        programHandle = loadBgfxProgram("noise_vs", "noise_ps");
    }

    return programHandle;
}

bgfx::ProgramHandle getRampShader()
{
    static bgfx::ProgramHandle programHandle = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(programHandle))
    {
        programHandle = loadBgfxProgram("ramp_vs", "ramp_ps");
    }

    return programHandle;
}

bgfx::ProgramHandle getSphereShader()
{
    static bgfx::ProgramHandle programHandle = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(programHandle))
    {
        programHandle = loadBgfxProgram("sphere_vs", "sphere_ps");
    }

    return programHandle;
}

void osystem_drawUILayer()
{
    bgfx::updateTexture2D(g_uiLayerTexture, 0, 0, 0, 0, 320, 200, bgfx::copy(uiLayer.data(), 320 * 200));

    if (backgroundMode == backgroundModeEnum_2D)
    {
        bgfx::VertexLayout layout;
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        bgfx::TransientVertexBuffer transientBuffer;
        bgfx::allocTransientVertexBuffer(&transientBuffer, 6, layout);

        struct sVertice
        {
            float position[3];
            float texcoord[2];
        };

        sVertice* pVertices = (sVertice*)transientBuffer.data;

        // 0
        pVertices->position[0] = 0.f;
        pVertices->position[1] = 0.f;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 0.f;
        pVertices->texcoord[1] = 0.f;
        pVertices++;

        //2
        pVertices->position[0] = 320.f;
        pVertices->position[1] = 200.f;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 1.f;
        pVertices->texcoord[1] = 1.f;
        pVertices++;

        //1
        pVertices->position[0] = 320.f;
        pVertices->position[1] = 0.f;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 1.f;
        pVertices->texcoord[1] = 0.f;
        pVertices++;

        //------------------------
        //3
        pVertices->position[0] = 0.f;
        pVertices->position[1] = 0.f;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 0.f;
        pVertices->texcoord[1] = 0.f;
        pVertices++;

        //4
        pVertices->position[0] = 0.f;
        pVertices->position[1] = 200.f;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 0.f;
        pVertices->texcoord[1] = 1.f;
        pVertices++;

        //5
        pVertices->position[0] = 320.f;
        pVertices->position[1] = 200.f;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 1.f;
        pVertices->texcoord[1] = 1.f;
        pVertices++;

        static bgfx::UniformHandle backgroundTextureUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(backgroundTextureUniform))
        {
            backgroundTextureUniform = bgfx::createUniform("s_backgroundTexture", bgfx::UniformType::Sampler);
        }
        static bgfx::UniformHandle paletteTextureUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(paletteTextureUniform))
        {
            paletteTextureUniform = bgfx::createUniform("s_paletteTexture", bgfx::UniformType::Sampler);
        }


        // Enhanced text (advanced options): the letters are rebuilt smooth at
        // the screen's resolution with a thin soft outline (textSmooth_ps)
        const bool styled = g_settings.graphics.enhancedText;

        bgfx::setState(0 | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_MSAA
            | BGFX_STATE_DEPTH_TEST_ALWAYS
            | (styled ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA) : 0)
        );

        bgfx::setVertexBuffer(0, &transientBuffer);

        bgfx::setTexture(0, backgroundTextureUniform, g_uiLayerTexture);
        bgfx::setTexture(1, paletteTextureUniform, g_paletteTexture);
        if (styled)
        {
            static bgfx::ProgramHandle textProgram = BGFX_INVALID_HANDLE;
            static bgfx::UniformHandle textStyleUniform = BGFX_INVALID_HANDLE;
            if (!bgfx::isValid(textProgram))
            {
                textProgram = loadBgfxProgram("ui_vs", "textSmooth_ps");
                textStyleUniform = bgfx::createUniform("u_textStyle", bgfx::UniformType::Vec4);
            }
            // Smooth, outlined; stroke cut 0.38 (about a quarter thicker than
            // the original strokes), outline at 60 %
            float style[4] = { 1.0f, 1.0f, 0.38f, 0.6f };
            bgfx::setUniform(textStyleUniform, style);
            bgfx::submit(gameViewId, textProgram);
        }
        else
        {
            bgfx::submit(gameViewId, getUIShader());
        }
    }
}
void osystem_drawBackground()
{
    if (backgroundMode == backgroundModeEnum_2D)
    {
        bgfx::VertexLayout layout;
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        bgfx::TransientVertexBuffer transientBuffer;
        bgfx::allocTransientVertexBuffer(&transientBuffer, 6, layout);

        struct sVertice
        {
            float position[3];
            float texcoord[2];
        };

        sVertice* pVertices = (sVertice*)transientBuffer.data;

        float shakeX = g_shakeOffsetX;
        float shakeY = g_shakeOffsetY;

        // 0
        pVertices->position[0] = 0.f + shakeX;
        pVertices->position[1] = 0.f + shakeY;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 0.f;
        pVertices->texcoord[1] = 0.f;
        pVertices++;

        //2
        pVertices->position[0] = 320.f + shakeX;
        pVertices->position[1] = 200.f + shakeY;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 1.f;
        pVertices->texcoord[1] = 1.f;
        pVertices++;

        //1
        pVertices->position[0] = 320.f + shakeX;
        pVertices->position[1] = 0.f + shakeY;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 1.f;
        pVertices->texcoord[1] = 0.f;
        pVertices++;

        //------------------------
        //3
        pVertices->position[0] = 0.f + shakeX;
        pVertices->position[1] = 0.f + shakeY;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 0.f;
        pVertices->texcoord[1] = 0.f;
        pVertices++;

        //4
        pVertices->position[0] = 0.f + shakeX;
        pVertices->position[1] = 200.f + shakeY;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 0.f;
        pVertices->texcoord[1] = 1.f;
        pVertices++;

        //5
        pVertices->position[0] = 320.f + shakeX;
        pVertices->position[1] = 200.f + shakeY;
        pVertices->position[2] = 1000.f;
        pVertices->texcoord[0] = 1.f;
        pVertices->texcoord[1] = 1.f;
        pVertices++;

        static bgfx::UniformHandle backgroundTextureUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(backgroundTextureUniform))
        {
            backgroundTextureUniform = bgfx::createUniform("s_backgroundTexture", bgfx::UniformType::Sampler);
        }
        static bgfx::UniformHandle paletteTextureUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(paletteTextureUniform))
        {
            paletteTextureUniform = bgfx::createUniform("s_paletteTexture", bgfx::UniformType::Sampler);
        }
        static bgfx::UniformHandle fadeLevelUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(fadeLevelUniform))
        {
            fadeLevelUniform = bgfx::createUniform("u_fadeLevel", bgfx::UniformType::Vec4);
        }


        bgfx::setState(0 | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_MSAA
        );

        bgfx::setVertexBuffer(0, &transientBuffer);

        // The player's picture (platformHooks.h), keeping what the game drew
        // 2D over the camera picture
        BackgroundPicture picture;
        if (g_getBackgroundPicture && g_getBackgroundPicture(&picture))
        {
            static bgfx::ProgramHandle customProgram = BGFX_INVALID_HANDLE;
            static bgfx::UniformHandle customUniform = BGFX_INVALID_HANDLE;
            static bgfx::UniformHandle originalUniform = BGFX_INVALID_HANDLE;
            static bgfx::UniformHandle centreUniform = BGFX_INVALID_HANDLE;
            if (!bgfx::isValid(customProgram))
            {
                customProgram = loadBgfxProgram("background_vs", "customBackground_ps");
                customUniform = bgfx::createUniform("s_customBackground", bgfx::UniformType::Sampler);
                originalUniform = bgfx::createUniform("s_originalBackground", bgfx::UniformType::Sampler);
                centreUniform = bgfx::createUniform("u_customBackground", bgfx::UniformType::Vec4);
            }
            float centre[4] = { picture.u0, picture.u1, 0.0f, 0.0f };
            bgfx::setUniform(centreUniform, centre);
            bgfx::setTexture(0, customUniform, picture.picture);
            bgfx::setTexture(1, paletteTextureUniform, g_paletteTexture);
            bgfx::setTexture(2, backgroundTextureUniform, g_backgroundTexture);
            bgfx::setTexture(3, originalUniform, picture.original);
            bgfx::submit(gameViewId, customProgram);
            return;
        }

        // The 320x200 screen
        bgfx::setTexture(0, backgroundTextureUniform, g_backgroundTexture);
        bgfx::setTexture(1, paletteTextureUniform, g_paletteTexture);

        // Fade level (used by the RGBA picture shader)
        float fadeParams[4] = { g_fadeLevel, 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(fadeLevelUniform, fadeParams);

        bgfx::submit(gameViewId, getBackgroundShader());


    }
}

bool g_bgfxMainResourcesInitialized = false;
bgfx::FrameBufferHandle fieldModelInspector_FB = BGFX_INVALID_HANDLE;
bgfx::TextureHandle fieldModelInspector_Texture = BGFX_INVALID_HANDLE;
bgfx::TextureHandle fieldModelInspector_Depth = BGFX_INVALID_HANDLE;

void initBgfxMainResources()
{
    // create background texture
    g_backgroundTexture = bgfx::createTexture2D(320, 200, false, 1, bgfx::TextureFormat::R8U);
    g_uiLayerTexture = bgfx::createTexture2D(320, 200, false, 1, bgfx::TextureFormat::R8U);
    g_paletteTexture = bgfx::createTexture2D(3, 256, false, 1, bgfx::TextureFormat::R8U);
}

ImVec2 gameResolution = { 320, 200 };

void osystem_startFrame()
{
    if (!g_bgfxMainResourcesInitialized)
    {
        initBgfxMainResources();
        g_bgfxMainResourcesInitialized = true;
    }

    g_3dFlushedThisFrame = false;
    s_depthSlicedThisFrame = false;
    s_deferredGlass.clear();
    s_sceneDarkThisFrame = false;

    //uiLayer.fill(0x0);

    static ImVec2 oldWindowSize = { -1,-1 };

    if (debuggerVar_debugMenuDisplayed)
    {
        gameViewId = 1;
        if (ImGui::Begin("Game"))
        {
            ImVec2 currentWindowSize = ImGui::GetContentRegionAvail();

            currentWindowSize[0] = std::max<float>(currentWindowSize[0], 1.f);
            currentWindowSize[1] = std::max<float>(currentWindowSize[1], 1.f);

            gameResolution = currentWindowSize;
        }
        else
        {
            gameResolution = { 320, 200 };
        }
        ImGui::End();

        if ((gameResolution[0] != oldWindowSize[0]) || (gameResolution[1] != oldWindowSize[1]))
        {
            oldWindowSize = gameResolution;

            if (bgfx::isValid(fieldModelInspector_FB))
            {
                bgfx::destroy(fieldModelInspector_FB);
            }

            const uint64_t tsFlags = 0
                //| BGFX_SAMPLER_MIN_POINT
                //| BGFX_SAMPLER_MAG_POINT
                //| BGFX_SAMPLER_MIP_POINT
                | BGFX_SAMPLER_U_CLAMP
                | BGFX_SAMPLER_V_CLAMP
                ;

            fieldModelInspector_Texture = bgfx::createTexture2D(static_cast<uint16_t>(gameResolution[0]), static_cast<uint16_t>(gameResolution[1]), false, 0, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT | tsFlags);
            fieldModelInspector_Depth = bgfx::createTexture2D(static_cast<uint16_t>(gameResolution[0]), static_cast<uint16_t>(gameResolution[1]), false, 0, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT | tsFlags);
            std::array<bgfx::Attachment, 2> attachements;
            attachements[0].init(fieldModelInspector_Texture);
            attachements[1].init(fieldModelInspector_Depth);
            fieldModelInspector_FB = bgfx::createFrameBuffer(2, &attachements[0], true);
        }
        bgfx::setViewFrameBuffer(gameViewId, fieldModelInspector_FB);
        bgfx::setViewRect(gameViewId, 0, 0, static_cast<uint16_t>(gameResolution[0]), static_cast<uint16_t>(gameResolution[1]));
    }
    else
    {
        gameViewId = 0;
        gameResolution[0] = static_cast<float>(outputResolution[0]);
        gameResolution[1] = static_cast<float>(outputResolution[1]);
        // beginScene() always binds the offscreen FB to view 0 now
        // (no backbuffer fallback needed; PP pipeline is always active for scene capture)
    }

    {
        bgfx::setViewClear(gameViewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL, 255);

        bgfx::setViewName(gameViewId, "Game");
        bgfx::setViewMode(gameViewId, bgfx::ViewMode::Sequential);

        bgfx::touch(gameViewId);
    }

    osystem_drawBackground();
}

unsigned char frontBuffer[320 * 200];
unsigned char physicalScreen[320 * 200];
std::array<unsigned char, 320 * 200> uiLayer;

void osystem_CopyBlockPhys(unsigned char* videoBuffer, int left, int top, int right, int bottom)
{
    unsigned char* in = (unsigned char*)&videoBuffer[0] + left + top * 320;

    // Clamp BEFORE 4-pixel alignment so the while loops can't push past the
    // 320/200 bounds and re-upload stale edge pixels.
    if (right > 320) right = 320;
    if (bottom > 200) bottom = 200;

    while ((right - left) % 4)
    {
        if (right >= 320) break;
        right++;
    }

    while ((bottom - top) % 4)
    {
        if (bottom >= 200) break;
        bottom++;
    }

    const int rowWidth = right - left;
    if (rowWidth <= 0 || bottom <= top)
        return;

    for (int i = top; i < bottom; i++)
    {
        unsigned char* src = videoBuffer + left + i * 320;
        unsigned char* dst = physicalScreen + left + i * 320;
        memcpy(dst, src, rowWidth);
    }
    if (g_onScreenUploaded)
        g_onScreenUploaded(physicalScreen);

    // Upload only the dirty region instead of the full 320x200 every call.
    // bgfx::makeRef on a contiguous staging copy keeps the per-row stride correct
    // by using bgfx::copy on a tightly packed temp buffer.
    if (rowWidth == 320)
    {
        // Whole-row range: upload as a single contiguous block.
        const int rows = bottom - top;
        bgfx::updateTexture2D(g_backgroundTexture, 0, 0, 0, (uint16_t)top,
            320, (uint16_t)rows,
            bgfx::copy(physicalScreen + top * 320, 320 * rows));
    }
    else
    {
        const int rows = bottom - top;
        const bgfx::Memory* mem = bgfx::alloc(rowWidth * rows);
        for (int i = 0; i < rows; i++)
        {
            memcpy(mem->data + i * rowWidth,
                   physicalScreen + (top + i) * 320 + left,
                   rowWidth);
        }
        bgfx::updateTexture2D(g_backgroundTexture, 0, 0,
            (uint16_t)left, (uint16_t)top,
            (uint16_t)rowWidth, (uint16_t)rows, mem);
    }
}

void osystem_setClip(float left, float top, float right, float bottom)
{
    float currentScissor[4];
    currentScissor[0] = ((left - 1) / 320.f) * gameResolution[0];
    currentScissor[1] = ((top - 1) / 200.f) * gameResolution[1];
    currentScissor[2] = ((right - left + 2) / 320.f) * gameResolution[0];
    currentScissor[3] = ((bottom - top + 2) / 200.f) * gameResolution[1];

    currentScissor[0] = std::max<float>(currentScissor[0], 0);
    currentScissor[1] = std::max<float>(currentScissor[1], 0);

    bgfx::setScissor(
        static_cast<uint16_t>(currentScissor[0]),
        static_cast<uint16_t>(currentScissor[1]),
        static_cast<uint16_t>(currentScissor[2]),
        static_cast<uint16_t>(currentScissor[3]));
}

void osystem_clearClip()
{
    bgfx::setScissor(0, 0, static_cast<uint16_t>(gameResolution[0]), static_cast<uint16_t>(gameResolution[1]));
}

void osystem_stopFrame()
{}

// Background-incrust rule (Original mode). In the original engine static
// objects (AF_BOXIFY) were drawn into the background copy that masks repaint
// from, so a mask drawn over a moving actor never hid them (e.g. the lamp on
// the attic table). Here they are drawn as 3D each frame, so the stencil
// marks their pixels (3) and masks of moving actors leave those pixels alone.
// Moving actors write 0 so their own masks still cover them.
static int s_objectStencilMode = 0;  // 0 none, 1 moving actor, 2 incrusted
static const int kIncrustStencil = 3;

void osystem_setObjectStencilMode(int mode)
{
    s_objectStencilMode = mode;
}

static void applyObjectStencil()
{
    if (s_objectStencilMode == 0)
        return;
    bgfx::setStencil(BGFX_STENCIL_TEST_ALWAYS
        | BGFX_STENCIL_FUNC_REF(s_objectStencilMode == 2 ? kIncrustStencil : 0)
        | BGFX_STENCIL_FUNC_RMASK(0xFF)
        | BGFX_STENCIL_OP_FAIL_S_KEEP
        | BGFX_STENCIL_OP_FAIL_Z_KEEP
        | BGFX_STENCIL_OP_PASS_Z_REPLACE);
}

// Glass (BUGS.md V20): what is behind keeps its light and takes the colour of
// the glass, as in the original engine, but with real colours instead of its
// 256-colour palette rule (glass_ps), so HD backgrounds keep their detail.
// That needs what is behind, models included (an enemy breaking in through
// a window), so the glass waits until the scene is drawn: view 1 first
// copies the scene, then draws the glass into it reading the copy.
void osystem_drawDeferredGlass()
{
    // A dark room shows no glass over its black (BUGS.md V23)
    if (s_sceneDarkThisFrame)
        s_deferredGlass.clear();
    if (s_deferredGlass.empty() || !g_screenEffects)
        return;

    const bgfx::ViewId kViewGlass = 1;
    const int width = g_screenEffects->getWidth();
    const int height = g_screenEffects->getHeight();
    bgfx::FrameBufferHandle scene = g_screenEffects->getSceneFrameBuffer();
    bgfx::TextureHandle copy = g_screenEffects->getSceneCopy();
    if (!bgfx::isValid(scene) || !bgfx::isValid(copy))
    {
        s_deferredGlass.clear();
        return;
    }

    bgfx::VertexLayout layout;
    layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    const uint32_t count = (uint32_t)s_deferredGlass.size();
    if (bgfx::getAvailTransientVertexBuffer(count, layout) < count)
    {
        s_deferredGlass.clear();
        return;
    }
    bgfx::TransientVertexBuffer transientBuffer;
    bgfx::allocTransientVertexBuffer(&transientBuffer, count, layout);
    memcpy(transientBuffer.data, s_deferredGlass.data(), sizeof(polyVertex) * count);
    s_deferredGlass.clear();

    bgfx::setViewName(kViewGlass, "Glass");
    bgfx::setViewRect(kViewGlass, 0, 0, (uint16_t)width, (uint16_t)height);
    bgfx::setViewFrameBuffer(kViewGlass, scene);
    bgfx::setViewClear(kViewGlass, BGFX_CLEAR_NONE);
    bgfx::blit(kViewGlass, copy, 0, 0, g_screenEffects->getMainColorTexture());

    static bgfx::ProgramHandle glassProgram = BGFX_INVALID_HANDLE;
    static bgfx::UniformHandle sceneCopyUniform = BGFX_INVALID_HANDLE;
    static bgfx::UniformHandle paletteTextureUniform = BGFX_INVALID_HANDLE;
    static bgfx::UniformHandle glassParamsUniform = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(glassProgram))
    {
        glassProgram = loadBgfxProgram("flat_vs", "glass_ps");
        sceneCopyUniform = bgfx::createUniform("s_sceneCopy", bgfx::UniformType::Sampler);
        paletteTextureUniform = bgfx::createUniform("s_paletteTexture", bgfx::UniformType::Sampler);
        glassParamsUniform = bgfx::createUniform("u_glassParams", bgfx::UniformType::Vec4);
    }

    // The slice is already in Z: plain depth
    osystem_setDepthSlice(-1);
    float glassParams[4] = { 1.0f / width, 1.0f / height, 0.0f, 0.0f };
    bgfx::setUniform(glassParamsUniform, glassParams);
    bgfx::setState(0 | BGFX_STATE_WRITE_RGB
        | BGFX_STATE_DEPTH_TEST_LEQUAL
        | BGFX_STATE_CULL_CCW);
    bgfx::setTexture(0, sceneCopyUniform, copy);
    bgfx::setTexture(1, paletteTextureUniform, g_paletteTexture);
    bgfx::setVertexBuffer(0, &transientBuffer);
    applyDepthSlice();
    bgfx::submit(kViewGlass, glassProgram);
}

void osystem_flushPendingPrimitives()
{
    if (numUsedFlatVertices || numUsedNoiseVertices || numUsedRampVertices ||
        numUsedSpheres || numUsedTransparentVertices ||
        !g_lineVertices.empty())
        g_3dFlushedThisFrame = true;

    // Helper lambda to apply shake offset to polyVertex arrays in transient buffers
    auto applyShakeToVertices = [](uint8_t* data, int numVertices, int stride) {
        if (g_shakeOffsetX == 0.f && g_shakeOffsetY == 0.f) return;
        for (int i = 0; i < numVertices; i++) {
            float* pos = (float*)(data + i * stride);
            pos[0] += g_shakeOffsetX;
            pos[1] += g_shakeOffsetY;
        }
        };

    if (numUsedFlatVertices)
    {
        bgfx::VertexLayout layout;
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        bgfx::TransientVertexBuffer transientBuffer;
        bgfx::allocTransientVertexBuffer(&transientBuffer, numUsedFlatVertices, layout);

        memcpy(transientBuffer.data, &flatVertices[0], sizeof(polyVertex) * numUsedFlatVertices);
        applyShakeToVertices(transientBuffer.data, numUsedFlatVertices, sizeof(polyVertex));

        bgfx::setState(0 | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LEQUAL
            | BGFX_STATE_MSAA
            | BGFX_STATE_CULL_CCW
        );

        static bgfx::UniformHandle paletteTextureUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(paletteTextureUniform))
        {
            paletteTextureUniform = bgfx::createUniform("s_paletteTexture", bgfx::UniformType::Sampler);
        }

        bgfx::setTexture(1, paletteTextureUniform, g_paletteTexture);

        bgfx::setVertexBuffer(0, &transientBuffer);
        applyObjectStencil();
        applyDepthSlice();
        bgfx::submit(gameViewId, getFlatShader());
    }

    if (numUsedNoiseVertices)
    {
        bgfx::VertexLayout layout;
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        bgfx::TransientVertexBuffer transientBuffer;
        bgfx::allocTransientVertexBuffer(&transientBuffer, numUsedNoiseVertices, layout);

        memcpy(transientBuffer.data, &noiseVertices[0], sizeof(polyVertex) * numUsedNoiseVertices);
        applyShakeToVertices(transientBuffer.data, numUsedNoiseVertices, sizeof(polyVertex));

        bgfx::setState(0 | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LEQUAL
            | BGFX_STATE_MSAA
            | BGFX_STATE_CULL_CCW
        );

        static bgfx::UniformHandle paletteTextureUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(paletteTextureUniform))
        {
            paletteTextureUniform = bgfx::createUniform("s_paletteTexture", bgfx::UniformType::Sampler);
        }

        bgfx::setTexture(1, paletteTextureUniform, g_paletteTexture);

        // noise texture
        static bgfx::TextureHandle noiseTexture = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(noiseTexture))
        {
            const int noiseTextureDim = 256;
            std::array<u8, noiseTextureDim* noiseTextureDim> noiseTextureData;
            for (int i = 0; i < noiseTextureData.size(); i++) {
                noiseTextureData[i] = rand();
            }
            noiseTexture = bgfx::createTexture2D(noiseTextureDim, noiseTextureDim, false, 1, bgfx::TextureFormat::R8U, 0, bgfx::copy(noiseTextureData.data(), noiseTextureDim * noiseTextureDim));
        }
        static bgfx::UniformHandle noiseTextureUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(noiseTextureUniform)) {
            noiseTextureUniform = bgfx::createUniform("s_noiseTexture", bgfx::UniformType::Sampler);
        }

        bgfx::setTexture(0, noiseTextureUniform, noiseTexture);

        bgfx::setVertexBuffer(0, &transientBuffer);
        applyObjectStencil();
        applyDepthSlice();
        bgfx::submit(gameViewId, getNoiseShader());
    }

    if (numUsedRampVertices)
    {
        bgfx::VertexLayout layout;
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        bgfx::TransientVertexBuffer transientBuffer;
        bgfx::allocTransientVertexBuffer(&transientBuffer, numUsedRampVertices, layout);

        memcpy(transientBuffer.data, &rampVertices[0], sizeof(polyVertex) * numUsedRampVertices);
        applyShakeToVertices(transientBuffer.data, numUsedRampVertices, sizeof(polyVertex));

        bgfx::setState(0 | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LEQUAL
            | BGFX_STATE_MSAA
            | BGFX_STATE_CULL_CCW
        );

        // Ramp vertices encode palette indices (colorIdx/15, bank/15) in their
        // UVs: the palette at slot 1 for getColor() in palette.sh
        static bgfx::UniformHandle rampPaletteUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(rampPaletteUniform))
        {
            rampPaletteUniform = bgfx::createUniform("s_paletteTexture", bgfx::UniformType::Sampler);
        }
        bgfx::setTexture(1, rampPaletteUniform, g_paletteTexture);

        bgfx::setVertexBuffer(0, &transientBuffer);
        applyObjectStencil();
        applyDepthSlice();
        bgfx::submit(gameViewId, getRampShader());
    }

    if (numUsedSpheres)
    {
        bgfx::VertexLayout layout;
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord1, 4, bgfx::AttribType::Float)
            .end();

        bgfx::TransientVertexBuffer transientBuffer;
        bgfx::allocTransientVertexBuffer(&transientBuffer, numUsedSpheres, layout);

        memcpy(transientBuffer.data, &sphereVertices[0], sizeof(sphereVertex) * numUsedSpheres);
        applyShakeToVertices(transientBuffer.data, numUsedSpheres, sizeof(sphereVertex));

        // applyShakeToVertices only shifts position.xy; sphereVertex also
        // carries centerX/centerY used by sphere_ps.sc to compute the normal
        // (screenSpacePos - sphereCenter). Shift those too so the normal
        // stays consistent during screen shake, otherwise lighting skews.
        if (g_shakeOffsetX != 0.f || g_shakeOffsetY != 0.f)
        {
            for (int i = 0; i < numUsedSpheres; i++)
            {
                sphereVertex* pV = (sphereVertex*)(transientBuffer.data + i * sizeof(sphereVertex));
                pV->centerX += g_shakeOffsetX;
                pV->centerY += g_shakeOffsetY;
            }
        }

        bgfx::setState(0 | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LEQUAL
            | BGFX_STATE_MSAA
            | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
        );

        // Spheres and points take their colour from the palette: slot 1 for
        // getColor() in palette.sh
        static bgfx::UniformHandle spherePaletteUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(spherePaletteUniform))
        {
            spherePaletteUniform = bgfx::createUniform("s_paletteTexture", bgfx::UniformType::Sampler);
        }
        bgfx::setTexture(1, spherePaletteUniform, g_paletteTexture);

        bgfx::setVertexBuffer(0, &transientBuffer);
        applyObjectStencil();
        applyDepthSlice();
        bgfx::submit(gameViewId, getSphereShader());
    }

    if (g_lineVertices.size()) {
        bgfx::VertexLayout layout;
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        bgfx::TransientVertexBuffer transientBuffer;
        bgfx::allocTransientVertexBuffer(&transientBuffer, static_cast<uint32_t>(g_lineVertices.size()), layout);

        memcpy(transientBuffer.data, g_lineVertices.data(), sizeof(polyVertex) * g_lineVertices.size());
        applyShakeToVertices(transientBuffer.data, static_cast<int>(g_lineVertices.size()), sizeof(polyVertex));

        bgfx::setState(0 | BGFX_STATE_PT_LINES
            | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LEQUAL
            | BGFX_STATE_MSAA
            | BGFX_STATE_LINEAA
            | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
        );

        static bgfx::UniformHandle paletteTextureUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(paletteTextureUniform))
        {
            paletteTextureUniform = bgfx::createUniform("s_paletteTexture", bgfx::UniformType::Sampler);
        }

        bgfx::setTexture(1, paletteTextureUniform, g_paletteTexture);

        bgfx::setVertexBuffer(0, &transientBuffer);
        applyObjectStencil();
        applyDepthSlice();
        bgfx::submit(gameViewId, getFlatShader());
    }

    // Original mode: glass is drawn after the scene, over a copy of it
    // (osystem_drawDeferredGlass). The depth slice of
    // its object goes into Z so it stays behind nearer objects and masks.
    if (numUsedTransparentVertices && s_depthSlice[2] > 0.5f)
    {
        const float sliceZ = s_depthSlice[0] * 40960.0f;
        for (int i = 0; i < numUsedTransparentVertices; i++)
        {
            polyVertex vertex = transparentVertices[i];
            vertex.X += g_shakeOffsetX;
            vertex.Y += g_shakeOffsetY;
            vertex.Z = (sliceZ + vertex.Z) * s_depthSlice[1];
            s_deferredGlass.push_back(vertex);
        }
    }
    else if (numUsedTransparentVertices)
    {
        bgfx::VertexLayout layout;
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        bgfx::TransientVertexBuffer transientBuffer;
        bgfx::allocTransientVertexBuffer(&transientBuffer, numUsedTransparentVertices, layout);

        memcpy(transientBuffer.data, &transparentVertices[0], sizeof(polyVertex) * numUsedTransparentVertices);
        applyShakeToVertices(transientBuffer.data, numUsedTransparentVertices, sizeof(polyVertex));

        bgfx::setState(0 | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_DEPTH_TEST_LEQUAL
            | BGFX_STATE_MSAA
            | BGFX_STATE_CULL_CCW
            | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_FACTOR, BGFX_STATE_BLEND_INV_FACTOR)
            , 0x80808080);

        static bgfx::UniformHandle paletteTextureUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(paletteTextureUniform))
        {
            paletteTextureUniform = bgfx::createUniform("s_paletteTexture", bgfx::UniformType::Sampler);
        }

        bgfx::setTexture(1, paletteTextureUniform, g_paletteTexture);
        bgfx::setVertexBuffer(0, &transientBuffer);
        applyObjectStencil();
        applyDepthSlice();
        bgfx::submit(gameViewId, getFlatShader());
    }

    numUsedFlatVertices = 0;
    numUsedNoiseVertices = 0;
    numUsedRampVertices = 0;
    numUsedSpheres = 0;
    numUsedTransparentVertices = 0;
    g_lineVertices.clear();
}

void osystem_fillPoly(float* buffer, int numPoint, unsigned char color, u8 polyType)
{
#define MAX_POINTS_PER_POLY 50

    if (numPoint >= MAX_POINTS_PER_POLY || numPoint < 3)
        return;

    // compute the polygon bounding box
    float polyMinX = 320.f;
    float polyMaxX = 0.f;
    float polyMinY = 200.f;
    float polyMaxY = 0.f;

    for (int i = 0; i < numPoint; i++)
    {
        float X = buffer[3 * i + 0];
        float Y = buffer[3 * i + 1];

        if (X > polyMaxX)
            polyMaxX = X;
        if (X < polyMinX)
            polyMinX = X;

        if (Y > polyMaxY)
            polyMaxY = Y;
        if (Y < polyMinY)
            polyMinY = Y;
    }

    float polyWidth = polyMaxX - polyMinX;
    float polyHeight = polyMaxY - polyMinY;

    if (polyWidth <= 0.f)
        polyWidth = 1;
    if (polyHeight <= 0.f)
        polyHeight = 1;

    switch (polyType)
    {
    default:
    case 0: // flat (triste)
    {
        int needed = (numPoint - 2) * 3;
        if (numUsedFlatVertices + needed > NUM_MAX_FLAT_VERTICES)
            return;
        polyVertex* pVertex = &flatVertices[numUsedFlatVertices];
        numUsedFlatVertices += needed;

        for (int i = 0; i < numPoint; i++)
        {
            if (i >= 3)
            {
                memcpy(pVertex, &pVertex[-3], sizeof(polyVertex));
                pVertex++;
                memcpy(pVertex, &pVertex[-2], sizeof(polyVertex));
                pVertex++;
            }

            pVertex->X = buffer[i * 3 + 0];
            pVertex->Y = buffer[i * 3 + 1];
            pVertex->Z = buffer[i * 3 + 2];

            int bank = (color & 0xF0) >> 4;
            int startColor = color & 0xF;
            float colorf = static_cast<float>(startColor);
            pVertex->U = colorf / 15.f;
            pVertex->V = bank / 15.f;
            pVertex++;
        }
        break;
    }
    case 1: // dither (pierre/tele) - use flat shader instead of noise to avoid transparency
    {
        int needed = (numPoint - 2) * 3;
        if (numUsedFlatVertices + needed > NUM_MAX_FLAT_VERTICES)
            return;
        polyVertex* pVertex = &flatVertices[numUsedFlatVertices];
        numUsedFlatVertices += needed;

        for (int i = 0; i < numPoint; i++)
        {
            if (i >= 3)
            {
                memcpy(pVertex, &pVertex[-3], sizeof(polyVertex));
                pVertex++;
                memcpy(pVertex, &pVertex[-2], sizeof(polyVertex));
                pVertex++;
            }

            pVertex->X = buffer[i * 3 + 0];
            pVertex->Y = buffer[i * 3 + 1];
            pVertex->Z = buffer[i * 3 + 2];

            int bank = (color & 0xF0) >> 4;
            int startColor = color & 0xF;
            float colorf = static_cast<float>(startColor);
            pVertex->U = colorf / 15.f;
            pVertex->V = bank / 15.f;
            pVertex++;
        }
        break;
    }
    case 2: // trans
    {
        int needed = (numPoint - 2) * 3;
        if (numUsedTransparentVertices + needed > NUM_MAX_TRANSPARENT_VERTICES)
            return;
        polyVertex* pVertex = &transparentVertices[numUsedTransparentVertices];
        numUsedTransparentVertices += needed;

        for (int i = 0; i < numPoint; i++)
        {
            if (i >= 3)
            {
                memcpy(pVertex, &pVertex[-3], sizeof(polyVertex));
                pVertex++;
                memcpy(pVertex, &pVertex[-2], sizeof(polyVertex));
                pVertex++;
            }

            pVertex->X = buffer[i * 3 + 0];
            pVertex->Y = buffer[i * 3 + 1];
            pVertex->Z = buffer[i * 3 + 2];

            int bank = (color & 0xF0) >> 4;
            int startColor = color & 0xF;
            float colorf = static_cast<float>(startColor);
            pVertex->U = colorf / 15.f;
            pVertex->V = bank / 15.f;
            pVertex++;
        }
        break;
    }
    case 4: // copper (ramps top to bottom)
    case 5: // copper2 (ramps top to bottom, 2 scanline per color)
    {
        int needed = (numPoint - 2) * 3;
        if (numUsedRampVertices + needed > NUM_MAX_RAMP_VERTICES)
            return;
        polyVertex* pVertex = &rampVertices[numUsedRampVertices];
        numUsedRampVertices += needed;

        int bank = (color & 0xF0) >> 4;
        int startColor = color & 0xF;
        float colorStep = 1; // TODO: this should be the scanline ratio for the current resolution to original resolution
        if (polyType == 5)
        {
            colorStep *= 0.5; // to stretch the ramp by 2 for copper2
        }

        for (int i = 0; i < numPoint; i++)
        {
            if (i >= 3)
            {
                memcpy(pVertex, &pVertex[-3], sizeof(polyVertex));
                pVertex++;
                memcpy(pVertex, &pVertex[-2], sizeof(polyVertex));
                pVertex++;
            }

            pVertex->X = buffer[i * 3 + 0];
            pVertex->Y = buffer[i * 3 + 1];
            pVertex->Z = buffer[i * 3 + 2];

            float colorf = startColor + colorStep * (pVertex->Y - polyMinY);

            pVertex->U = colorf / 15.f;
            pVertex->V = bank / 15.f;
            pVertex++;
        }
        break;
    }
    case 3: // marbre (ramp left to right)
    {
        int needed = (numPoint - 2) * 3;
        if (numUsedRampVertices + needed > NUM_MAX_RAMP_VERTICES)
            return;
        polyVertex* pVertex = &rampVertices[numUsedRampVertices];
        numUsedRampVertices += needed;

        float colorStep = 15.f / polyWidth;

        int bank = (color & 0xF0) >> 4;
        int startColor = color & 0xF;

        assert(startColor == 0);

        for (int i = 0; i < numPoint; i++)
        {
            if (i >= 3)
            {
                memcpy(pVertex, &pVertex[-3], sizeof(polyVertex));
                pVertex++;
                memcpy(pVertex, &pVertex[-2], sizeof(polyVertex));
                pVertex++;
            }

            pVertex->X = buffer[i * 3 + 0];
            pVertex->Y = buffer[i * 3 + 1];
            pVertex->Z = buffer[i * 3 + 2];

            float colorf = startColor + colorStep * (pVertex->X - polyMinX);

            pVertex->U = colorf / 15.f;
            pVertex->V = bank / 15.f;
            pVertex++;
        }
        break;
    }
    case 6: // marbre2 (ramp right to left)
    {
        int needed = (numPoint - 2) * 3;
        if (numUsedRampVertices + needed > NUM_MAX_RAMP_VERTICES)
            return;
        polyVertex* pVertex = &rampVertices[numUsedRampVertices];
        numUsedRampVertices += needed;

        float colorStep = 15.f / polyWidth;

        int bank = (color & 0xF0) >> 4;
        int startColor = color & 0xF;

        assert(startColor == 0);

        for (int i = 0; i < numPoint; i++)
        {
            if (i >= 3)
            {
                memcpy(pVertex, &pVertex[-3], sizeof(polyVertex));
                pVertex++;
                memcpy(pVertex, &pVertex[-2], sizeof(polyVertex));
                pVertex++;
            }

            pVertex->X = buffer[i * 3 + 0];
            pVertex->Y = buffer[i * 3 + 1];
            pVertex->Z = buffer[i * 3 + 2];

            float colorf = startColor + colorStep * (pVertex->X - polyMinX);

            pVertex->U = 1.f - colorf / 15.f;
            pVertex->V = bank / 15.f;
            pVertex++;
        }
        break;
    }
    }
}

void osystem_draw3dLine(float x1, float y1, float z1, float x2, float y2, float z2, unsigned char color)
{
    polyVertex vertex1;
    polyVertex vertex2;

    vertex1.X = x1;
    vertex1.Y = y1;
    vertex1.Z = z1;

    vertex2.X = x2;
    vertex2.Y = y2;
    vertex2.Z = z2;

    int bank = (color & 0xF0) >> 4;
    int startColor = color & 0xF;
    float colorf = static_cast<float>(startColor);
    vertex1.U = vertex2.U = colorf / 15.f;
    vertex1.V = vertex2.V = bank / 15.f;

    g_lineVertices.push_back(vertex1);
    g_lineVertices.push_back(vertex2);
}

void osystem_draw3dQuad(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float x4, float y4, float z4, unsigned char color, int transparency)
{
    osystem_draw3dLine(x1, y1, z1, x2, y2, z2, color);
    osystem_draw3dLine(x2, y2, z2, x3, y3, z3, color);
    osystem_draw3dLine(x3, y3, z3, x4, y4, z4, color);
    osystem_draw3dLine(x4, y4, z4, x1, y1, z1, color);
}

// A sphere or a point: a square facing the camera, coloured from the palette
// (bank and colour of the original), drawn round by the sphere shader
static void addBillboard(float X, float Y, float Z, u8 color, u8 material, float size)
{
    // Guard against zero/negative radius (the shader divides by it)
    if (size <= 0.f)
        return;

    const float fScaleRatio = 6.f / 5.f;
    std::array<sphereVertex, 4> corners;
    corners[0].X = X + size * fScaleRatio;
    corners[0].Y = Y + size * fScaleRatio;
    corners[0].Z = Z;

    corners[1].X = X + size * fScaleRatio;
    corners[1].Y = Y - size * fScaleRatio;
    corners[1].Z = Z;

    corners[2].X = X - size * fScaleRatio;
    corners[2].Y = Y - size * fScaleRatio;
    corners[2].Z = Z;

    corners[3].X = X - size * fScaleRatio;
    corners[3].Y = Y + size * fScaleRatio;
    corners[3].Z = Z;

    const std::array<int, 2 * 3> mapping = { {
            0,1,2,
            0,2,3
    } };

    for (int i = 0; i < (int)mapping.size(); i++)
    {
        if (numUsedSpheres >= NUM_MAX_SPHERES_VERTICES)
        {
            assert(false && "numUsedSpheres >= NUM_MAX_SPHERES_VERTICES");
            return;
        }

        sphereVertex* pVertex = &sphereVertices[numUsedSpheres];
        numUsedSpheres++;

        pVertex->X = corners[mapping[i]].X;
        pVertex->Y = corners[mapping[i]].Y;
        pVertex->Z = corners[mapping[i]].Z;
        pVertex->U = (float)(color & 0xF) / 15.f;
        pVertex->V = (float)((color >> 4) & 0xF) / 15.f;
        pVertex->size = size;
        pVertex->centerX = X;
        pVertex->centerY = Y;
        pVertex->material = material;
    }
}

void osystem_drawSphere(float X, float Y, float Z, u8 color, u8 material, float size)
{
    addBillboard(X, Y, Z, color, material, size);
}

void osystem_drawPoint(float X, float Y, float Z, u8 color, u8 material, float size)
{
    addBillboard(X, Y, Z, color, material, size);
}

void osystem_flip(unsigned char* videoBuffer)
{
    osystem_flushPendingPrimitives();
}

void osystem_createMask(const std::array<u8, 320 * 200>& mask, int roomId, int maskId, int actualRoomNumber, int maskX1, int maskY1, int maskX2, int maskY2)
{
    if (maskTextures.size() < roomId + 1)
    {
        maskTextures.resize(roomId + 1);
    }
    if (maskTextures[roomId].size() < maskId + 1)
    {
        maskTextures[roomId].resize(maskId + 1);
    }

    if (bgfx::isValid(maskTextures[roomId][maskId].maskTexture))
    {
        bgfx::destroy(maskTextures[roomId][maskId].maskTexture);
        maskTextures[roomId][maskId].maskTexture = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(maskTextures[roomId][maskId].vertexBuffer))
    {
        bgfx::destroy(maskTextures[roomId][maskId].vertexBuffer);
        maskTextures[roomId][maskId].vertexBuffer = BGFX_INVALID_HANDLE;
    }

    int maskW = 320;
    int maskH = 200;
    const u8* maskData = mask.data();
    // Original masks: grow them by half a 320x200 pixel (texture at double size,
    // one texel wider). The original drew objects at 320x200, so an object that
    // covered less than half a pixel beyond a mask edge was not drawn; at monitor
    // resolution that strip showed as a thin line along the edge, e.g. a door
    // behind a wall or a chest of drawers (BUGS.md V15).
    std::vector<u8> grownMask;
    if (maskW == 320 && maskH == 200)
    {
        const int w2 = 640, h2 = 400;
        grownMask.assign(w2 * h2, 0);
        for (int y = 0; y < h2; y++)
        {
            for (int x = 0; x < w2; x++)
            {
                u8 value = 0;
                for (int dy = -1; dy <= 1 && !value; dy++)
                {
                    int sy = (y + dy) / 2;
                    if (y + dy < 0 || sy >= 200)
                        continue;
                    for (int dx = -1; dx <= 1; dx++)
                    {
                        int sx = (x + dx) / 2;
                        if (x + dx < 0 || sx >= 320)
                            continue;
                        if (maskData[sy * 320 + sx])
                        {
                            value = 0xFF;
                            break;
                        }
                    }
                }
                grownMask[y * w2 + x] = value;
            }
        }
        maskData = grownMask.data();
        maskW = w2;
        maskH = h2;

        // the quad must cover the grown border too
        maskX1 = (maskX1 > 0) ? maskX1 - 1 : 0;
        maskY1 = (maskY1 > 0) ? maskY1 - 1 : 0;
        maskX2 = (maskX2 < 319) ? maskX2 + 1 : 319;
        maskY2 = (maskY2 < 199) ? maskY2 + 1 : 199;
    }

    // R8 (normalized float) with linear filtering enables bilinear AA at mask edges.
    // Clamp sampler prevents wrap-around artifacts at the texture border.
    // Texture may be HD-resolution when a replacement is loaded; the shader
    // samples via UVs so any size works.
    maskTextures[roomId][maskId].maskTexture = bgfx::createTexture2D(maskW, maskH, false, 1, bgfx::TextureFormat::R8,
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
        bgfx::copy(maskData, maskW * maskH));

    // The player's pictures keep the shape (export) and load their own mask
    if (g_onMaskCreated)
        g_onMaskCreated(roomId, maskId, mask.data(), maskX1, maskY1, maskX2, maskY2);

    maskTextures[roomId][maskId].maskX1 = maskX1;
    maskTextures[roomId][maskId].maskX2 = maskX2 + 1;
    maskTextures[roomId][maskId].maskY1 = maskY1;
    maskTextures[roomId][maskId].maskY2 = maskY2 + 1;

    bgfx::VertexLayout layout;
    layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    float X1 = static_cast<float>(maskTextures[roomId][maskId].maskX1);
    float X2 = static_cast<float>(maskTextures[roomId][maskId].maskX2);
    float Y1 = static_cast<float>(maskTextures[roomId][maskId].maskY1);
    float Y2 = static_cast<float>(maskTextures[roomId][maskId].maskY2);

    float maskZ = 0.f;

    struct sVertice
    {
        float position[3];
        float texcoord[2];
    } vertexBuffer[4];

    sVertice* pVertices = vertexBuffer;
    pVertices->position[0] = X1;
    pVertices->position[1] = Y2;
    pVertices->position[2] = maskZ;
    pVertices->texcoord[0] = X1 / 320.f;
    pVertices->texcoord[1] = Y2 / 200.f;
    pVertices++;
    pVertices->position[0] = X1;
    pVertices->position[1] = Y1;
    pVertices->position[2] = maskZ;
    pVertices->texcoord[0] = X1 / 320.f;
    pVertices->texcoord[1] = Y1 / 200.f;
    pVertices++;
    pVertices->position[0] = X2;
    pVertices->position[1] = Y2;
    pVertices->position[2] = maskZ;
    pVertices->texcoord[0] = X2 / 320.f;
    pVertices->texcoord[1] = Y2 / 200.f;
    pVertices++;
    pVertices->position[0] = X2;
    pVertices->position[1] = Y1;
    pVertices->position[2] = maskZ;
    pVertices->texcoord[0] = X2 / 320.f;
    pVertices->texcoord[1] = Y1 / 200.f;
    pVertices++;

    maskTextures[roomId][maskId].vertexBuffer = bgfx::createVertexBuffer(bgfx::copy(vertexBuffer, sizeof(vertexBuffer)), layout);
}

void osystem_drawMask(int roomId, int maskId)
{
    if (g_gameId == TIMEGATE)
        return;

    if (!bgfx::isValid(maskTextures[roomId][maskId].maskTexture))
        return;

    if (!bgfx::isValid(maskTextures[roomId][maskId].vertexBuffer))
        return;

#ifdef FITD_DEBUGGER
    if (backgroundMode != backgroundModeEnum_2D)
        return;
#endif

    static bgfx::UniformHandle backgroundTextureUniform = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(backgroundTextureUniform))
    {
        backgroundTextureUniform = bgfx::createUniform("s_backgroundTexture", bgfx::UniformType::Sampler);
    }
    static bgfx::UniformHandle paletteTextureUniform = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(paletteTextureUniform))
    {
        paletteTextureUniform = bgfx::createUniform("s_paletteTexture", bgfx::UniformType::Sampler);
    }
    static bgfx::UniformHandle maskTextureUniform = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(maskTextureUniform))
    {
        maskTextureUniform = bgfx::createUniform("s_maskTexture", bgfx::UniformType::Sampler);
    }
    static bgfx::UniformHandle fadeLevelUniform = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(fadeLevelUniform))
    {
        fadeLevelUniform = bgfx::createUniform("u_fadeLevel", bgfx::UniformType::Vec4);
    }

    // The player's mask (custom backgrounds) covers the whole picture: its
    // shape may reach beyond the original rectangle. Its edge is antialiased
    // over one screen pixel (customMask_ps), blended with what is behind
    bool customMaskOriginalShape = false;
    const bgfx::TextureHandle customMask = g_getMaskPicture
        ? g_getMaskPicture(roomId, maskId, &customMaskOriginalShape) : bgfx::TextureHandle(BGFX_INVALID_HANDLE);

    // Write depth (far plane) so SSAO treats masked pixels as background (no occlusion)
    bgfx::setState(0 | BGFX_STATE_WRITE_RGB
        | BGFX_STATE_WRITE_Z
        | BGFX_STATE_DEPTH_TEST_ALWAYS
        | BGFX_STATE_MSAA
        | BGFX_STATE_PT_TRISTRIP
        | (bgfx::isValid(customMask) ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA) : 0)
    );

    // Build transient vertex buffer with shake offset applied
    bgfx::VertexLayout maskLayout;
    maskLayout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    bgfx::TransientVertexBuffer maskTransientBuffer;
    bgfx::allocTransientVertexBuffer(&maskTransientBuffer, 4, maskLayout);

    float rectX1 = (float)maskTextures[roomId][maskId].maskX1;
    float rectX2 = (float)maskTextures[roomId][maskId].maskX2;
    float rectY1 = (float)maskTextures[roomId][maskId].maskY1;
    float rectY2 = (float)maskTextures[roomId][maskId].maskY2;
    if (bgfx::isValid(customMask))
    {
        rectX1 = 0.f; rectX2 = 320.f; rectY1 = 0.f; rectY2 = 200.f;
    }

    float mX1 = rectX1 + g_shakeOffsetX;
    float mX2 = rectX2 + g_shakeOffsetX;
    float mY1 = rectY1 + g_shakeOffsetY;
    float mY2 = rectY2 + g_shakeOffsetY;
    float mZ = 0.f;

    struct sMaskVertice { float position[3]; float texcoord[2]; };
    sMaskVertice* pMV = (sMaskVertice*)maskTransientBuffer.data;
    pMV[0] = { {mX1, mY2, mZ}, {rectX1 / 320.f, rectY2 / 200.f} };
    pMV[1] = { {mX1, mY1, mZ}, {rectX1 / 320.f, rectY1 / 200.f} };
    pMV[2] = { {mX2, mY2, mZ}, {rectX2 / 320.f, rectY2 / 200.f} };
    pMV[3] = { {mX2, mY1, mZ}, {rectX2 / 320.f, rectY1 / 200.f} };

    bgfx::setVertexBuffer(0, &maskTransientBuffer);

    bgfx::setTexture(2, backgroundTextureUniform, g_backgroundTexture);
    bgfx::setTexture(1, paletteTextureUniform, g_paletteTexture);
    bgfx::setTexture(0, maskTextureUniform, maskTextures[roomId][maskId].maskTexture);

    // Set fade level uniform so mask matches main background brightness
    float fadeParams[4] = { g_fadeLevel, 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(fadeLevelUniform, fadeParams);

    if (s_objectStencilMode == 1)
    {
        // Mask over a moving actor: never repaint incrusted objects
        bgfx::setStencil(BGFX_STENCIL_TEST_NOTEQUAL
            | BGFX_STENCIL_FUNC_REF(kIncrustStencil)
            | BGFX_STENCIL_FUNC_RMASK(0xFF)
            | BGFX_STENCIL_OP_FAIL_S_KEEP
            | BGFX_STENCIL_OP_FAIL_Z_KEEP
            | BGFX_STENCIL_OP_PASS_Z_REPLACE);
    }
    else if (s_objectStencilMode == 2)
    {
        // Mask over an incrusted object: it hides the object like in the background copy
        bgfx::setStencil(BGFX_STENCIL_TEST_ALWAYS
            | BGFX_STENCIL_FUNC_REF(kIncrustStencil)
            | BGFX_STENCIL_FUNC_RMASK(0xFF)
            | BGFX_STENCIL_OP_FAIL_S_KEEP
            | BGFX_STENCIL_OP_FAIL_Z_KEEP
            | BGFX_STENCIL_OP_PASS_Z_REPLACE);
    }
    else
    {
        // Write stencil=1 where mask is active (fragment shader will discard transparent pixels)
        bgfx::setStencil(BGFX_STENCIL_TEST_ALWAYS
            | BGFX_STENCIL_FUNC_REF(1)
            | BGFX_STENCIL_FUNC_RMASK(0xFF)
            | BGFX_STENCIL_OP_FAIL_Z_KEEP
            | BGFX_STENCIL_OP_PASS_Z_REPLACE);
    }

    applyDepthSlice();
    BackgroundPicture picture;
    if (g_getBackgroundPicture && g_getBackgroundPicture(&picture))
    {
        // Custom background: the mask shape is the original one, its pixels
        // come from the player's picture (customMask_ps)
        static bgfx::ProgramHandle customMaskProgram = BGFX_INVALID_HANDLE;
        static bgfx::UniformHandle customUniform = BGFX_INVALID_HANDLE;
        static bgfx::UniformHandle originalUniform = BGFX_INVALID_HANDLE;
        static bgfx::UniformHandle centreUniform = BGFX_INVALID_HANDLE;
        if (!bgfx::isValid(customMaskProgram))
        {
            customMaskProgram = loadBgfxProgram("maskBackground_vs", "customMask_ps");
            customUniform = bgfx::createUniform("s_customBackground", bgfx::UniformType::Sampler);
            originalUniform = bgfx::createUniform("s_originalBackground", bgfx::UniformType::Sampler);
            centreUniform = bgfx::createUniform("u_customBackground", bgfx::UniformType::Vec4);
        }
        // z = 1: the mask is the player's, the size of the picture (sampled
        // like it, through the 4:3 centre)
        // A player's mask with the original 16:10 shape (e.g. enlarged by an AI)
        // is placed over the scene; one of the picture's shape, over the picture
        const bool maskOverPicture = bgfx::isValid(customMask) && !customMaskOriginalShape;
        float centre[4] = { picture.u0, picture.u1, maskOverPicture ? 1.0f : 0.0f, bgfx::isValid(customMask) ? 1.0f : 0.0f };
        bgfx::setUniform(centreUniform, centre);
        if (bgfx::isValid(customMask))
            bgfx::setTexture(0, maskTextureUniform, customMask);
        bgfx::setTexture(2, backgroundTextureUniform, g_backgroundTexture);
        bgfx::setTexture(3, originalUniform, picture.original);
        bgfx::setTexture(4, customUniform, picture.picture);
        bgfx::submit(gameViewId, customMaskProgram);
    }
    else
    {
        bgfx::submit(gameViewId, getMaskBackgroundShader());
    }
}

// ============================================================================
// Animated grass-mask overlay
// Drawn AFTER 3D actors so the grass occludes them.
// ============================================================================

