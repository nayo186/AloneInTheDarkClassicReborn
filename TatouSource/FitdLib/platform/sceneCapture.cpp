///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Scene capture: the snapshot of the 3D scene for the pause menu preview and
// the frozen background behind the menus (split from rendererBGFX.cpp)
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


///////////////////////////////////////////////////////////////////////////////
// Scene preview capture
// A persistent GPU snapshot texture is blitted every game frame (inside
// EndFrame, after endScene, before bgfx::frame).  When the pause menu opens
// we stop updating the snapshot, request a CPU readback of the last game
// frame, and use that RGBA data as the menu preview thumbnail.
///////////////////////////////////////////////////////////////////////////////

static const bgfx::ViewId kViewSnapshot = 206;

static bgfx::TextureHandle s_snapshotTex = BGFX_INVALID_HANDLE;
static unsigned char* s_previewData = nullptr;
static int s_snapshotWidth = 0;
static int s_snapshotHeight = 0;
static bool s_snapshotEnabled = true;   // true during gameplay, false while menu is open
static bool s_previewReady = false;

// Sampleable texture for rendering the frozen scene as a background during menus
// Called every frame from EndFrame() (after endScene, before bgfx::frame).
// Blits the composited scene from m_mainColorTex into a persistent snapshot
// texture so it survives the clear at the start of the next frame.
void osystem_updateSceneSnapshot()
{
    if (!s_snapshotEnabled)
        return;

    if (!g_screenEffects || !bgfx::isValid(g_screenEffects->getMainColorTexture()))
        return;

    int w = g_screenEffects->getWidth();
    int h = g_screenEffects->getHeight();
    if (w <= 0 || h <= 0)
        return;

    // Recreate snapshot texture if resolution changed
    if (w != s_snapshotWidth || h != s_snapshotHeight)
    {
        if (bgfx::isValid(s_snapshotTex))
            bgfx::destroy(s_snapshotTex);
        if (s_previewData)
        {
            delete[] s_previewData;
            s_previewData = nullptr;
        }

        s_snapshotTex = bgfx::createTexture2D(
            (uint16_t)w, (uint16_t)h, false, 1,
            bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK
        );

        s_previewData = new unsigned char[(size_t)w * h * 4];
        s_snapshotWidth = w;
        s_snapshotHeight = h;
    }

    // Recreate sampleable frozen scene texture if resolution changed
    if (w != s_frozenSceneWidth || h != s_frozenSceneHeight)
    {
        if (bgfx::isValid(s_frozenSceneTex))
            bgfx::destroy(s_frozenSceneTex);
        s_frozenSceneTex = bgfx::createTexture2D(
            (uint16_t)w, (uint16_t)h, false, 1,
            bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_BLIT_DST | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
        );
        s_frozenSceneWidth = w;
        s_frozenSceneHeight = h;
    }

    // Set up snapshot view (after composite view 205) so the blit happens
    // after the post-processing chain has written the final image.
    bgfx::setViewName(kViewSnapshot, "Snapshot");
    bgfx::setViewRect(kViewSnapshot, 0, 0, (uint16_t)w, (uint16_t)h);
    bgfx::touch(kViewSnapshot);

    // GPU blit: m_mainColorTex -> s_snapshotTex (for CPU readback)
    bgfx::blit(kViewSnapshot, s_snapshotTex, 0, 0, g_screenEffects->getMainColorTexture());

    // GPU blit: m_mainColorTex -> s_frozenSceneTex (sampleable, for menu background)
    if (bgfx::isValid(s_frozenSceneTex))
        bgfx::blit(kViewSnapshot, s_frozenSceneTex, 0, 0, g_screenEffects->getMainColorTexture());
}

// Called when the pause menu opens.  Freezes snapshot updates and kicks off
// a CPU readback of the last captured frame.
void osystem_captureScenePreview()
{
    s_previewReady = false;
    s_snapshotEnabled = false;  // freeze   keep the last game frame in the snapshot

    if (!bgfx::isValid(s_snapshotTex) || !s_previewData || s_snapshotWidth <= 0 || s_snapshotHeight <= 0)
        return;

    // Request async readback   data is available after 2 bgfx::frame() calls.
    bgfx::readTexture(s_snapshotTex, s_previewData);
}

// Call after pumping 2 bgfx::frame() calls so the readback has completed.
void osystem_finalizeScenePreview()
{
    if (s_previewData && s_snapshotWidth > 0 && s_snapshotHeight > 0)
    {
        s_previewReady = true;
    }
}

unsigned char* osystem_getScenePreviewData()
{
    return s_previewReady ? s_previewData : nullptr;
}

int osystem_getScenePreviewWidth()
{
    return s_previewReady ? s_snapshotWidth : 0;
}

int osystem_getScenePreviewHeight()
{
    return s_previewReady ? s_snapshotHeight : 0;
}

// Freeze/unfreeze the scene snapshot for menu backgrounds
void osystem_freezeSceneForMenu()
{
    s_snapshotEnabled = false;
}

bool osystem_isSceneSnapshotLive()
{
    return s_snapshotEnabled;
}

void osystem_unfreezeSceneForMenu()
{
    s_snapshotEnabled = true;
}

// Render the frozen scene snapshot as a full-screen background at Z=1000.
// Used during inventory/found-object menus to preserve 3D objects in the scene.
static bool s_frozenSceneDrawn = false;

bool osystem_takeFrozenSceneDrawn()
{
    bool drawn = s_frozenSceneDrawn;
    s_frozenSceneDrawn = false;
    return drawn;
}

void osystem_drawFrozenSceneBackground()
{
    if (backgroundMode != backgroundModeEnum_2D)
        return;

    if (!bgfx::isValid(s_frozenSceneTex))
        return;

    s_frozenSceneDrawn = true;

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

    // Full-screen quad at Z=1000 (same depth as normal background)
    pVertices->position[0] = 0.f;   pVertices->position[1] = 0.f;   pVertices->position[2] = 1000.f;
    pVertices->texcoord[0] = 0.f;   pVertices->texcoord[1] = 0.f;
    pVertices++;

    pVertices->position[0] = 320.f; pVertices->position[1] = 200.f; pVertices->position[2] = 1000.f;
    pVertices->texcoord[0] = 1.f;   pVertices->texcoord[1] = 1.f;
    pVertices++;

    pVertices->position[0] = 320.f; pVertices->position[1] = 0.f;   pVertices->position[2] = 1000.f;
    pVertices->texcoord[0] = 1.f;   pVertices->texcoord[1] = 0.f;
    pVertices++;

    pVertices->position[0] = 0.f;   pVertices->position[1] = 0.f;   pVertices->position[2] = 1000.f;
    pVertices->texcoord[0] = 0.f;   pVertices->texcoord[1] = 0.f;
    pVertices++;

    pVertices->position[0] = 0.f;   pVertices->position[1] = 200.f; pVertices->position[2] = 1000.f;
    pVertices->texcoord[0] = 0.f;   pVertices->texcoord[1] = 1.f;
    pVertices++;

    pVertices->position[0] = 320.f; pVertices->position[1] = 200.f; pVertices->position[2] = 1000.f;
    pVertices->texcoord[0] = 1.f;   pVertices->texcoord[1] = 1.f;
    pVertices++;

    static bgfx::UniformHandle frozenSceneTexUniform = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(frozenSceneTexUniform))
    {
        frozenSceneTexUniform = bgfx::createUniform("s_backgroundTexture", bgfx::UniformType::Sampler);
    }
    static bgfx::UniformHandle frozenSceneFadeUniform = BGFX_INVALID_HANDLE;
    if (!bgfx::isValid(frozenSceneFadeUniform))
    {
        frozenSceneFadeUniform = bgfx::createUniform("u_fadeLevel", bgfx::UniformType::Vec4);
    }

    bgfx::setState(0 | BGFX_STATE_WRITE_RGB
        | BGFX_STATE_MSAA
    );

    bgfx::setVertexBuffer(0, &transientBuffer);
    bgfx::setTexture(0, frozenSceneTexUniform, s_frozenSceneTex);

    float fadeParams[4] = { g_fadeLevel, 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(frozenSceneFadeUniform, fadeParams);

    submitImageBackground(gameViewId);
}



