///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Screen effects: the scene is drawn into an offscreen image, then copied to
// the screen with the optional effects of the launcher's advanced options
// (antialiasing, bloom, film grain, SSAO) and the screen fade. The offscreen image is also
// the source of the pause menu preview and of the repainted frames (V14).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bgfx/bgfx.h>

class ScreenEffects
{
public:
    ScreenEffects();  // all handles invalid (a zero handle is a valid one)
    // Creates the offscreen images and the effects' programs for a scene of width x height
    void init(int width, int height);
    // Deletes the offscreen images and programs
    void shutdown();
    // Remakes the offscreen images for a new scene size
    void resize(int width, int height);

    // The game draws the scene (view 0) into the offscreen image
    void beginScene();

    // Applies the enabled effects and draws the result on the screen.
    // modelsDrawn: 3D models were drawn this frame (ambient occlusion from
    // their depth). frozenImageDrawn: the frame shows the last image again
    // (BUGS.md V14, menus): its ambient occlusion is reused. Neither: none.
    void endScene(bool modelsDrawn, bool frozenImageDrawn);

    // Where the final image goes in the window (the rest is black); the scene
    // image is scaled to it (graphics.resolution)
    void setOutputRect(int x, int y, int width, int height, int windowWidth, int windowHeight)
    {
        m_outputX = x; m_outputY = y; m_outputWidth = width; m_outputHeight = height;
        m_windowWidth = windowWidth; m_windowHeight = windowHeight;
    }

    // Screens that must look plain (title armadillo, intro screens, books)
    // suspend the effects; nested calls are counted. The player's options in
    // g_settings are never changed for this (they were, and a save or
    // the F1 window during those screens lost them, BUGS.md C15).
    void suspend(bool on) { m_suspended += on ? 1 : (m_suspended > 0 ? -1 : 0); }

    // Picture drawn in the black bars around the scene (a custom background
    // wider than 4:3; invalid = black); u0..u1 is its 4:3 centre
    void setSideFill(bgfx::TextureHandle picture, float u0, float u1)
    {
        m_sidePicture = picture; m_sideU0 = u0; m_sideU1 = u1;
    }

    // Brightness of the final image, 0 (black) to 1 (normal); used by fades
    void setScreenFade(float fade) { m_screenFade = fade; }

    // The offscreen image of the scene
    bgfx::TextureHandle getMainColorTexture() const { return m_sceneColor; }
    // The framebuffer the scene is drawn into
    bgfx::FrameBufferHandle getSceneFrameBuffer() const { return m_sceneFB; }
    // Copy of the scene the glass reads (rendererBGFX.cpp, osystem_drawDeferredGlass)
    bgfx::TextureHandle getSceneCopy() const { return m_sceneCopy; }
    // Width of the scene image
    int getWidth() const { return m_width; }
    // Height of the scene image
    int getHeight() const { return m_height; }

private:
    static const int kBloomLevels = 4;

    // Creates the offscreen images (scene, copies, bloom levels, SSAO)
    void createTargets();
    // Deletes the offscreen images
    void destroyTargets();
    bool renderAmbientOcclusion();  // false when there is no camera yet
    void renderAntialiasing();      // FXAA on the 3D models: m_sceneColor -> m_aaColor
    // Makes the bloom: bright parts of the painted backgrounds, blurred down and up
    void renderBloom();
    // Copies the scene to the screen with the chosen effects and the fade
    void renderComposite(bool bloom, bool grain, bool ambientOcclusion);

    int m_width = 0;
    int m_height = 0;
    int m_outputX = 0;
    int m_outputY = 0;
    int m_outputWidth = 0;
    int m_outputHeight = 0;
    int m_windowWidth = 0;
    int m_windowHeight = 0;
    float m_screenFade = 1.0f;
    int m_suspended = 0;
    bgfx::TextureHandle m_sidePicture = BGFX_INVALID_HANDLE;
    float m_sideU0 = 0.0f;
    float m_sideU1 = 1.0f;
    bgfx::ProgramHandle m_sidesProgram = BGFX_INVALID_HANDLE;
    bool m_ambientOcclusionValid = false;  // m_aoBlurred holds the AO of the last drawn scene

    // Scene (color + depth) drawn by the game
    bgfx::TextureHandle m_sceneColor = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_sceneDepth = BGFX_INVALID_HANDLE;
    bgfx::FrameBufferHandle m_sceneFB = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_sceneCopy = BGFX_INVALID_HANDLE;

    // The scene with its 3D models antialiased (FXAA); the effects read it
    // instead of m_sceneColor while the option is on
    bgfx::TextureHandle m_aaColor = BGFX_INVALID_HANDLE;
    bgfx::FrameBufferHandle m_aaFB = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_sceneSource = BGFX_INVALID_HANDLE;  // this frame's scene for bloom and composite

    // Ambient occlusion: raw and blurred
    bgfx::TextureHandle m_aoRaw = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_aoBlurred = BGFX_INVALID_HANDLE;
    bgfx::FrameBufferHandle m_aoRawFB = BGFX_INVALID_HANDLE;
    bgfx::FrameBufferHandle m_aoBlurredFB = BGFX_INVALID_HANDLE;

    // Bloom: halving levels (1/2 to 1/16) and the levels mixed on the way up
    bgfx::TextureHandle m_bloomDown[kBloomLevels];
    bgfx::FrameBufferHandle m_bloomDownFB[kBloomLevels];
    bgfx::TextureHandle m_bloomUp[kBloomLevels - 1];
    bgfx::FrameBufferHandle m_bloomUpFB[kBloomLevels - 1];
    int m_bloomWidth[kBloomLevels] = {};
    int m_bloomHeight[kBloomLevels] = {};

    // Stand-ins when an effect is off: white = no occlusion, black = no bloom
    bgfx::TextureHandle m_white = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_black = BGFX_INVALID_HANDLE;

    bgfx::ProgramHandle m_compositeProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_bloomDownProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_bloomUpProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_ssaoProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_ssaoBlurProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_antialiasProgram = BGFX_INVALID_HANDLE;

    bgfx::UniformHandle s_fxScene = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle s_fxBloom = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle s_fxAO = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle s_fxDepth = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_fxComposite = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_fxGrain = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_fxBloom = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_fxSsao = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_fxCamera = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_fxTexel = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_fxDepth = BGFX_INVALID_HANDLE;
};

extern ScreenEffects* g_screenEffects;
