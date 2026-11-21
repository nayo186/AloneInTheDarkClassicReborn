///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Screen effects: offscreen scene, antialiasing, bloom, film grain, SSAO and screen fade
// (replaces the remaster's postProcessing.cpp)
///////////////////////////////////////////////////////////////////////////////

#include "screenEffects.h"
#include "widescreen.h"
#include "platformHooks.h"
#include "settings.h"
#include <bx/timer.h>
#include "common.h"  // after bx: windows.h min/max macros
#include <algorithm>
#include <cmath>
#include <string>

extern bgfx::ProgramHandle loadBgfxProgram(const std::string& VSFile, const std::string& PSFile);

// Camera of the current view (core/camera.cpp), to rebuild 3D positions for SSAO
extern int cameraFovX;
extern int cameraFovY;
extern int cameraCenterX;
extern int cameraCenterY;

ScreenEffects* g_screenEffects = nullptr;

// Views after the scene (view 0) and the glass (view 1, rendererBGFX.cpp);
// 206 is the pause menu snapshot (sceneEffects.cpp)
static const bgfx::ViewId kViewAntialias = 199;
static const bgfx::ViewId kViewSsao = 200;
static const bgfx::ViewId kViewSsaoBlur = 201;
static const bgfx::ViewId kViewBloomDown = 202;  // 202-205
static const bgfx::ViewId kViewBloomUp = 207;    // 207-209
static const bgfx::ViewId kViewComposite = 210;

// Effect settings
static const float kBloomThreshold = 0.5f;   // brightness where the glow starts
static const float kBloomKnee = 0.25f;       // soft start of the glow
static const float kBloomIntensity = 1.0f;
static const float kBloomWideWeight = 0.6f;  // each wider level weighs this much of the previous
static const float kGrainIntensity = 0.035f;
static const float kGrainChangesPerSecond = 24.0f;  // like film
// How dark the scanlines are at each level of the option (1 to 4)
static const float kScanlineDarkness[4] = { 0.2f, 0.35f, 0.5f, 0.65f };
static const float kScanlineCount = 400.0f;
static const float kSsaoRadius = 250.0f;     // world units
static const float kSsaoIntensity = 1.0f;

ScreenEffects::ScreenEffects()
{
    for (int i = 0; i < kBloomLevels; i++)
    {
        m_bloomDown[i] = BGFX_INVALID_HANDLE;
        m_bloomDownFB[i] = BGFX_INVALID_HANDLE;
    }
    for (int i = 0; i < kBloomLevels - 1; i++)
    {
        m_bloomUp[i] = BGFX_INVALID_HANDLE;
        m_bloomUpFB[i] = BGFX_INVALID_HANDLE;
    }
}

template <typename Handle>
static void destroyHandle(Handle& handle)
{
    if (bgfx::isValid(handle))
        bgfx::destroy(handle);
    handle = BGFX_INVALID_HANDLE;
}

static bgfx::TextureHandle createTarget(int width, int height, bgfx::TextureFormat::Enum format, uint64_t sampler)
{
    return bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, format,
        BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | sampler);
}

static bgfx::TextureHandle createPixel(bgfx::TextureFormat::Enum format, uint32_t value)
{
    const bgfx::Memory* memory = bgfx::copy(&value, sizeof(value));
    return bgfx::createTexture2D(1, 1, false, 1, format, BGFX_SAMPLER_POINT, memory);
}

void ScreenEffects::init(int width, int height)
{
    m_width = width > 0 ? width : 1;
    m_height = height > 0 ? height : 1;

    s_fxScene = bgfx::createUniform("s_fxScene", bgfx::UniformType::Sampler);
    s_fxBloom = bgfx::createUniform("s_fxBloom", bgfx::UniformType::Sampler);
    s_fxAO = bgfx::createUniform("s_fxAO", bgfx::UniformType::Sampler);
    s_fxDepth = bgfx::createUniform("s_fxDepth", bgfx::UniformType::Sampler);
    u_fxComposite = bgfx::createUniform("u_fxComposite", bgfx::UniformType::Vec4);
    u_fxGrain = bgfx::createUniform("u_fxGrain", bgfx::UniformType::Vec4);
    u_fxSmoke = bgfx::createUniform("u_fxSmoke", bgfx::UniformType::Vec4);
    u_fxSmokeTime = bgfx::createUniform("u_fxSmokeTime", bgfx::UniformType::Vec4);
    u_fxBloom = bgfx::createUniform("u_fxBloom", bgfx::UniformType::Vec4);
    u_fxSsao = bgfx::createUniform("u_fxSsao", bgfx::UniformType::Vec4);
    u_fxCamera = bgfx::createUniform("u_fxCamera", bgfx::UniformType::Vec4);
    u_fxTexel = bgfx::createUniform("u_fxTexel", bgfx::UniformType::Vec4);
    u_fxDepth = bgfx::createUniform("u_fxDepth", bgfx::UniformType::Vec4);

    m_compositeProgram = loadBgfxProgram("effects_vs", "effectsComposite_ps");
    m_bloomDownProgram = loadBgfxProgram("effects_vs", "effectsBloomDown_ps");
    m_bloomUpProgram = loadBgfxProgram("effects_vs", "effectsBloomUp_ps");
    m_ssaoProgram = loadBgfxProgram("effects_vs", "effectsSsao_ps");
    m_ssaoBlurProgram = loadBgfxProgram("effects_vs", "effectsSsaoBlur_ps");
    m_antialiasProgram = loadBgfxProgram("effects_vs", "effectsAntialias_ps");
    m_sidesProgram = loadBgfxProgram("effects_vs", "effectsSides_ps");

    m_white = createPixel(bgfx::TextureFormat::RGBA8, 0xffffffff);
    m_black = createPixel(bgfx::TextureFormat::RGBA8, 0xff000000);

    createTargets();
}

void ScreenEffects::shutdown()
{
    destroyTargets();

    destroyHandle(m_white);
    destroyHandle(m_black);

    destroyHandle(m_compositeProgram);
    destroyHandle(m_bloomDownProgram);
    destroyHandle(m_bloomUpProgram);
    destroyHandle(m_ssaoProgram);
    destroyHandle(m_ssaoBlurProgram);
    destroyHandle(m_antialiasProgram);
    destroyHandle(m_sidesProgram);

    destroyHandle(s_fxScene);
    destroyHandle(s_fxBloom);
    destroyHandle(s_fxAO);
    destroyHandle(s_fxDepth);
    destroyHandle(u_fxComposite);
    destroyHandle(u_fxGrain);
    destroyHandle(u_fxSmoke);
    destroyHandle(u_fxSmokeTime);
    destroyHandle(u_fxBloom);
    destroyHandle(u_fxSsao);
    destroyHandle(u_fxCamera);
    destroyHandle(u_fxTexel);
    destroyHandle(u_fxDepth);
}

void ScreenEffects::resize(int width, int height)
{
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;
    if (width == m_width && height == m_height)
        return;

    m_width = width;
    m_height = height;
    destroyTargets();
    createTargets();
}

void ScreenEffects::createTargets()
{
    m_sceneColor = createTarget(m_width, m_height, bgfx::TextureFormat::RGBA8, 0);
    m_sceneDepth = createTarget(m_width, m_height, bgfx::TextureFormat::D24S8, BGFX_SAMPLER_POINT);
    bgfx::TextureHandle sceneAttachments[] = { m_sceneColor, m_sceneDepth };
    m_sceneFB = bgfx::createFrameBuffer(2, sceneAttachments, false);
    m_sceneCopy = bgfx::createTexture2D((uint16_t)m_width, (uint16_t)m_height, false, 1, bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_BLIT_DST | BGFX_SAMPLER_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

    m_aaColor = createTarget(m_width, m_height, bgfx::TextureFormat::RGBA8, 0);
    m_aaFB = bgfx::createFrameBuffer(1, &m_aaColor, false);

    // Ambient occlusion at half size: a soft shade, enlarged smoothly by the
    // composite; a quarter of the work (BUGS.md C86)
    m_aoWidth = m_width > 1 ? m_width / 2 : 1;
    m_aoHeight = m_height > 1 ? m_height / 2 : 1;
    m_aoRaw = createTarget(m_aoWidth, m_aoHeight, bgfx::TextureFormat::R8, BGFX_SAMPLER_POINT);
    m_aoRawFB = bgfx::createFrameBuffer(1, &m_aoRaw, false);
    m_aoBlurred = createTarget(m_aoWidth, m_aoHeight, bgfx::TextureFormat::R8, 0);
    m_aoBlurredFB = bgfx::createFrameBuffer(1, &m_aoBlurred, false);
    m_ambientOcclusionValid = false;

    int width = m_width;
    int height = m_height;
    for (int i = 0; i < kBloomLevels; i++)
    {
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
        m_bloomWidth[i] = width;
        m_bloomHeight[i] = height;
        m_bloomDown[i] = createTarget(width, height, bgfx::TextureFormat::RGBA8, 0);
        m_bloomDownFB[i] = bgfx::createFrameBuffer(1, &m_bloomDown[i], false);
        if (i < kBloomLevels - 1)
        {
            m_bloomUp[i] = createTarget(width, height, bgfx::TextureFormat::RGBA8, 0);
            m_bloomUpFB[i] = bgfx::createFrameBuffer(1, &m_bloomUp[i], false);
        }
    }
}

void ScreenEffects::destroyTargets()
{
    // The frame buffers do not own their textures (created with false)
    destroyHandle(m_sceneFB);
    destroyHandle(m_sceneColor);
    destroyHandle(m_sceneDepth);
    destroyHandle(m_sceneCopy);
    destroyHandle(m_aaFB);
    destroyHandle(m_aaColor);
    destroyHandle(m_aoRawFB);
    destroyHandle(m_aoRaw);
    destroyHandle(m_aoBlurredFB);
    destroyHandle(m_aoBlurred);
    for (int i = 0; i < kBloomLevels; i++)
    {
        destroyHandle(m_bloomDownFB[i]);
        destroyHandle(m_bloomDown[i]);
    }
    for (int i = 0; i < kBloomLevels - 1; i++)
    {
        destroyHandle(m_bloomUpFB[i]);
        destroyHandle(m_bloomUp[i]);
    }
    m_ambientOcclusionValid = false;
}

static void drawFullScreenAt(bgfx::ViewId view, bgfx::FrameBufferHandle target, int x, int y, int width, int height,
    int targetWidth, int targetHeight, bgfx::ProgramHandle program, const char* name, bool clearBlack)
{
    if (!bgfx::isValid(program))
        return;

    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    if (bgfx::getAvailTransientVertexBuffer(6, layout) < 6)
        return;

    bgfx::TransientVertexBuffer buffer;
    bgfx::allocTransientVertexBuffer(&buffer, 6, layout);
    struct Vertex { float position[3]; float uv[2]; };
    Vertex* v = (Vertex*)buffer.data;

    // Render targets start at the bottom on OpenGL: flip V to read them upright
    const bool flip = bgfx::getCaps()->originBottomLeft;
    const float top = flip ? 1.0f : 0.0f;
    const float bottom = flip ? 0.0f : 1.0f;
    // The quad covers (x, y, width, height) of a target of targetWidth x targetHeight
    const float left = 2.0f * x / targetWidth - 1.0f;
    const float right = 2.0f * (x + width) / targetWidth - 1.0f;
    const float upper = 1.0f - 2.0f * y / targetHeight;
    const float lower = 1.0f - 2.0f * (y + height) / targetHeight;
    v[0] = { {left,  upper, 0.0f}, {0.0f, top} };
    v[1] = { {right, lower, 0.0f}, {1.0f, bottom} };
    v[2] = { {right, upper, 0.0f}, {1.0f, top} };
    v[3] = { {left,  upper, 0.0f}, {0.0f, top} };
    v[4] = { {left,  lower, 0.0f}, {0.0f, bottom} };
    v[5] = { {right, lower, 0.0f}, {1.0f, bottom} };

    bgfx::setViewName(view, name);
    bgfx::setViewRect(view, 0, 0, (uint16_t)targetWidth, (uint16_t)targetHeight);
    bgfx::setViewFrameBuffer(view, target);
    // The final image clears the window to black around the scene
    bgfx::setViewClear(view, clearBlack ? BGFX_CLEAR_COLOR : BGFX_CLEAR_NONE, 0x000000ff);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::setVertexBuffer(0, &buffer);
    bgfx::submit(view, program);
}

static void drawFullScreen(bgfx::ViewId view, bgfx::FrameBufferHandle target, int width, int height,
    bgfx::ProgramHandle program, const char* name)
{
    drawFullScreenAt(view, target, 0, 0, width, height, width, height, program, name, false);
}

void ScreenEffects::beginScene()
{
    // An invalid handle (no offscreen image) draws on the screen
    bgfx::setViewFrameBuffer(0, m_sceneFB);
}

void ScreenEffects::endScene(bool modelsDrawn, bool frozenImageDrawn)
{
    if (!bgfx::isValid(m_sceneColor) || !bgfx::isValid(m_compositeProgram))
        return;

    const bool active = (m_suspended == 0);
    const bool bloom = active && g_settings.postProcessing.enableBloom;
    const bool grain = active && g_settings.postProcessing.enableFilmGrain;
    const bool scanlines = active && g_settings.postProcessing.enableScanlines;
    bool ambientOcclusion = active && g_settings.postProcessing.enableSSAO;
    const bool antialias = active && g_settings.postProcessing.antialiasing == 1 && bgfx::isValid(m_antialiasProgram);

    placeScene();
    m_sceneSource = m_sceneColor;
    if (antialias)
    {
        renderAntialiasing();
        m_sceneSource = m_aaColor;
    }

    if (ambientOcclusion && modelsDrawn)
    {
        m_ambientOcclusionValid = renderAmbientOcclusion();
        ambientOcclusion = m_ambientOcclusionValid;
    }
    else if (ambientOcclusion && frozenImageDrawn)
    {
        ambientOcclusion = m_ambientOcclusionValid;
    }
    else
    {
        // No models (title screens, books...): nothing to shade, and the last
        // ambient occlusion belongs to another image
        ambientOcclusion = false;
        if (!g_settings.postProcessing.enableSSAO)
            m_ambientOcclusionValid = false;
    }

    if (bloom)
        renderBloom();

    renderComposite(bloom, grain, scanlines, ambientOcclusion);
}

// SSAO and FXAA look for the 3D models in every pixel they shade (reading the
// depth around it): over the whole image that halved the frame rate of a
// phone each (BUGS.md C86). They shade only the part of the image with models.
void ScreenEffects::placeScene()
{
    const int outputWidth = (m_outputWidth > 0) ? m_outputWidth : m_width;
    const int outputHeight = (m_outputHeight > 0) ? m_outputHeight : m_height;
    const int windowWidth = (m_windowWidth > 0) ? m_windowWidth : m_width;
    const int windowHeight = (m_windowHeight > 0) ? m_windowHeight : m_height;
    m_sceneQuad[0] = m_outputX;
    m_sceneQuad[1] = m_outputY;
    m_sceneQuad[2] = outputWidth;
    m_sceneQuad[3] = outputHeight;
    // Playing with a zoomed aspect ratio: the scene at the screen's width,
    // the view where it follows the character (it moves the view: once a
    // frame); otherwise the whole 4:3 scene in the middle
    if (widescreenZoomed())
    {
        const int sceneRect[4] = { m_outputX, m_outputY, outputWidth, outputHeight };
        widescreenSceneQuad(sceneRect, m_sceneQuad);
    }
    m_visible[0] = 0;
    m_visible[1] = 0;
    m_visible[2] = m_width;
    m_visible[3] = m_height;
    if (m_sceneQuad[2] <= 0 || m_sceneQuad[3] <= 0)
        return;
    const float sx = (float)m_width / (float)m_sceneQuad[2];
    const float sy = (float)m_height / (float)m_sceneQuad[3];
    const int x0 = (std::max)(0, (int)floorf((0 - m_sceneQuad[0]) * sx));
    const int y0 = (std::max)(0, (int)floorf((0 - m_sceneQuad[1]) * sy));
    const int x1 = (std::min)(m_width, (int)ceilf((windowWidth - m_sceneQuad[0]) * sx));
    const int y1 = (std::min)(m_height, (int)ceilf((windowHeight - m_sceneQuad[1]) * sy));
    if (x1 > x0 && y1 > y0)
    {
        m_visible[0] = x0;
        m_visible[1] = y0;
        m_visible[2] = x1 - x0;
        m_visible[3] = y1 - y0;
    }
}

void ScreenEffects::setVisibleScissor(int width, int height, int margin) const
{
    const float sx = (float)width / (float)m_width;
    const float sy = (float)height / (float)m_height;
    const int x0 = (std::max)(0, (int)floorf(m_visible[0] * sx) - margin);
    const int y0 = (std::max)(0, (int)floorf(m_visible[1] * sy) - margin);
    const int x1 = (std::min)(width, (int)ceilf((m_visible[0] + m_visible[2]) * sx) + margin);
    const int y1 = (std::min)(height, (int)ceilf((m_visible[1] + m_visible[3]) * sy) + margin);
    bgfx::setScissor((uint16_t)x0, (uint16_t)y0, (uint16_t)(x1 - x0), (uint16_t)(y1 - y0));
}

bool ScreenEffects::modelRect(int rect[4], int margin) const
{
    float area[4];
    if (!osystem_modelArea(area))
        return false;
    int x0 = (int)floorf(area[0] / 320.0f * (float)m_width) - margin;
    int y0 = (int)floorf(area[1] / 200.0f * (float)m_height) - margin;
    int x1 = (int)ceilf(area[2] / 320.0f * (float)m_width) + margin;
    int y1 = (int)ceilf(area[3] / 200.0f * (float)m_height) + margin;
    // Only what is seen (a zoomed aspect ratio leaves the top and bottom of
    // the scene off the screen)
    x0 = (std::max)(x0, m_visible[0]);
    y0 = (std::max)(y0, m_visible[1]);
    x1 = (std::min)(x1, m_visible[0] + m_visible[2]);
    y1 = (std::min)(y1, m_visible[1] + m_visible[3]);
    if (x1 <= x0 || y1 <= y0)
        return false;
    rect[0] = x0;
    rect[1] = y0;
    rect[2] = x1 - x0;
    rect[3] = y1 - y0;
    return true;
}

bool ScreenEffects::renderAmbientOcclusion()
{
    const bgfx::Caps* caps = bgfx::getCaps();
    float ssao[4] = { kSsaoRadius, kSsaoIntensity, caps->originBottomLeft ? 1.0f : 0.0f,
        caps->homogeneousDepth ? 1.0f : 0.0f };
    float camera[4] = { (float)cameraFovX, (float)cameraFovY, (float)cameraCenterX, (float)cameraCenterY };
    float texel[4] = { 1.0f / (float)m_width, 1.0f / (float)m_height, 0.0f, 0.0f };
    // Original mode draws each object in its own slice of the depth (V21)
    float depth[4] = { osystem_depthSlicedThisFrame() ? (float)OSYSTEM_DEPTH_SLICES : 0.0f, 0.0f, 0.0f, 0.0f };
    if (camera[0] <= 0.0f || camera[1] <= 0.0f)
        return false;  // no camera yet

    // White (no shade) where there are no models; shaded only around them
    // (the rectangle of the image, halved like the shade)
    int rect[4] = { 0, 0, m_width, m_height };
    modelRect(rect, 4);
    rect[2] = (rect[0] + rect[2] + 1) / 2 - rect[0] / 2;
    rect[3] = (rect[1] + rect[3] + 1) / 2 - rect[1] / 2;
    rect[0] /= 2;
    rect[1] /= 2;
    // The depth is read at full size (normals); the blur steps over the shade
    float aoTexel[4] = { 1.0f / (float)m_aoWidth, 1.0f / (float)m_aoHeight, 0.0f, 0.0f };

    bgfx::setUniform(u_fxSsao, ssao);
    bgfx::setUniform(u_fxCamera, camera);
    bgfx::setUniform(u_fxTexel, texel);
    bgfx::setUniform(u_fxDepth, depth);
    bgfx::setTexture(0, s_fxDepth, m_sceneDepth);
    bgfx::setScissor((uint16_t)rect[0], (uint16_t)rect[1], (uint16_t)rect[2], (uint16_t)rect[3]);
    drawFullScreen(kViewSsao, m_aoRawFB, m_aoWidth, m_aoHeight, m_ssaoProgram, "SSAO");
    bgfx::setViewClear(kViewSsao, BGFX_CLEAR_COLOR, 0xffffffff);

    bgfx::setUniform(u_fxTexel, aoTexel);
    bgfx::setUniform(u_fxSsao, ssao);
    bgfx::setUniform(u_fxDepth, depth);
    bgfx::setTexture(0, s_fxAO, m_aoRaw);
    bgfx::setTexture(1, s_fxDepth, m_sceneDepth);
    bgfx::setScissor((uint16_t)rect[0], (uint16_t)rect[1], (uint16_t)rect[2], (uint16_t)rect[3]);
    drawFullScreen(kViewSsaoBlur, m_aoBlurredFB, m_aoWidth, m_aoHeight, m_ssaoBlurProgram, "SSAO blur");
    bgfx::setViewClear(kViewSsaoBlur, BGFX_CLEAR_COLOR, 0xffffffff);
    return true;
}

void ScreenEffects::renderAntialiasing()
{
    const bgfx::Caps* caps = bgfx::getCaps();
    float texel[4] = { 1.0f / (float)m_width, 1.0f / (float)m_height, 0.0f, 0.0f };
    // Original mode draws each object in its own slice of the depth (V21)
    float depth[4] = { osystem_depthSlicedThisFrame() ? (float)OSYSTEM_DEPTH_SLICES : 0.0f,
        caps->homogeneousDepth ? 1.0f : 0.0f, 0.0f, 0.0f };
    // The image as it is, then FXAA only around the models (margin: the
    // pixels next to a model and the 8 texels FXAA reaches along an edge)
    int rect[4];
    const bool models = modelRect(rect, 10);
    bgfx::setViewMode(kViewAntialias, bgfx::ViewMode::Sequential);
    const float copy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };   // effectsSides_ps: w = no fade
    bgfx::setUniform(u_fxComposite, copy);
    bgfx::setTexture(0, s_fxScene, m_sceneColor, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_POINT);
    setVisibleScissor(m_width, m_height, 2);
    drawFullScreen(kViewAntialias, m_aaFB, m_width, m_height, m_sidesProgram, "Antialias");
    if (!models)
        return;   // nothing to smooth (title, books, menus)

    bgfx::setUniform(u_fxTexel, texel);
    bgfx::setUniform(u_fxDepth, depth);
    bgfx::setTexture(0, s_fxScene, m_sceneColor, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
    bgfx::setTexture(1, s_fxDepth, m_sceneDepth);
    bgfx::setScissor((uint16_t)rect[0], (uint16_t)rect[1], (uint16_t)rect[2], (uint16_t)rect[3]);
    drawFullScreen(kViewAntialias, m_aaFB, m_width, m_height, m_antialiasProgram, "Antialias");
}

void ScreenEffects::renderBloom()
{
    // Halve the image four times; the first pass keeps only the bright part of
    // the backgrounds (the depth tells the 3D models apart, as for FXAA)
    const bgfx::Caps* caps = bgfx::getCaps();
    float depth[4] = { osystem_depthSlicedThisFrame() ? (float)OSYSTEM_DEPTH_SLICES : 0.0f,
        caps->homogeneousDepth ? 1.0f : 0.0f, 0.0f, 0.0f };
    for (int i = 0; i < kBloomLevels; i++)
    {
        bgfx::TextureHandle source = (i == 0) ? m_sceneSource : m_bloomDown[i - 1];
        int sourceWidth = (i == 0) ? m_width : m_bloomWidth[i - 1];
        int sourceHeight = (i == 0) ? m_height : m_bloomHeight[i - 1];
        float params[4] = { 1.0f / sourceWidth, 1.0f / sourceHeight,
            (i == 0) ? kBloomThreshold : 0.0f, kBloomKnee };
        bgfx::setUniform(u_fxBloom, params);
        bgfx::setUniform(u_fxDepth, depth);
        bgfx::setTexture(0, s_fxScene, source);
        bgfx::setTexture(1, s_fxDepth, m_sceneDepth);
        // Only the part seen, and a few pixels around it the blur reaches
        setVisibleScissor(m_bloomWidth[i], m_bloomHeight[i], 4);
        drawFullScreen(kViewBloomDown + i, m_bloomDownFB[i], m_bloomWidth[i], m_bloomHeight[i],
            m_bloomDownProgram, "Bloom down");
    }

    // Back up to 1/2, adding each level: a glow of several widths
    for (int i = kBloomLevels - 2; i >= 0; i--)
    {
        bgfx::TextureHandle smaller = (i == kBloomLevels - 2) ? m_bloomDown[i + 1] : m_bloomUp[i + 1];
        float params[4] = { 1.0f / m_bloomWidth[i + 1], 1.0f / m_bloomHeight[i + 1], kBloomWideWeight, 0.0f };
        bgfx::setUniform(u_fxBloom, params);
        bgfx::setTexture(0, s_fxScene, smaller);
        bgfx::setTexture(1, s_fxBloom, m_bloomDown[i]);
        setVisibleScissor(m_bloomWidth[i], m_bloomHeight[i], 4);
        drawFullScreen(kViewBloomUp + (kBloomLevels - 2 - i), m_bloomUpFB[i], m_bloomWidth[i], m_bloomHeight[i],
            m_bloomUpProgram, "Bloom up");
    }
}

void ScreenEffects::renderComposite(bool bloom, bool grain, bool scanlines, bool ambientOcclusion)
{
    // Grain frame from real time: the same speed at any frame rate (BUGS.md V17)
    double seconds = (double)bx::getHPCounter() / (double)bx::getHPFrequency();
    float grainFrame = (float)fmod(floor(seconds * kGrainChangesPerSecond), 1000.0);

    // The levels add up to 1 + w + w^2 + w^3: divide so the intensity means the same
    float bloomWeights = 0.0f;
    for (int i = 0; i < kBloomLevels; i++)
        bloomWeights += powf(kBloomWideWeight, (float)i);
    float composite[4] = { bloom ? kBloomIntensity / bloomWeights : 0.0f, grain ? kGrainIntensity : 0.0f,
        grainFrame, m_screenFade };
    // Grain of about one pixel at 1080p, a little coarser above; scanlines:
    // how dark, and how many over the scene (two per line of the game, as a
    // 640x400 CRT)
    float grainParams[4] = { m_height > 1200 ? 2.0f : 1.0f, scanlines ? kScanlineDarkness[std::clamp(g_settings.postProcessing.scanlineLevel, 1, 4) - 1] : 0.0f, kScanlineCount, 0.0f };

    // A scene image smaller than its place on screen is enlarged with sharp
    // pixels (640x400 looks like pixel art, not blurred); a larger one is
    // reduced smoothly
    const int outputWidth = (m_outputWidth > 0) ? m_outputWidth : m_width;
    const int outputHeight = (m_outputHeight > 0) ? m_outputHeight : m_height;
    const bool enlarged = (m_width < outputWidth || m_height < outputHeight);
    const uint32_t sceneSampler = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | (enlarged ? BGFX_SAMPLER_POINT : 0);

    bgfx::setUniform(u_fxComposite, composite);
    bgfx::setUniform(u_fxGrain, grainParams);
    bgfx::setTexture(0, s_fxScene, m_sceneSource, sceneSampler);
    bgfx::setTexture(1, s_fxBloom, bloom ? m_bloomUp[0] : m_black);
    bgfx::setTexture(2, s_fxAO, ambientOcclusion ? m_aoBlurred : m_white);
    const int windowWidth = (m_windowWidth > 0) ? m_windowWidth : m_width;
    const int windowHeight = (m_windowHeight > 0) ? m_windowHeight : m_height;

    // The two quads below must keep their order: sides first, scene over them
    bgfx::setViewMode(kViewComposite, bgfx::ViewMode::Sequential);
    int sceneQuad[4] = { m_sceneQuad[0], m_sceneQuad[1], m_sceneQuad[2], m_sceneQuad[3] };
    if (widescreenZoomed())
    {
        // Zoomed aspect ratios (widescreen.cpp): a still, blurred picture of
        // the game fills the window behind, as large as needed to cover it
        float sidesAspect = 1.0f;
        const bgfx::TextureHandle sides = g_getWidescreenSides ? g_getWidescreenSides(&sidesAspect) : bgfx::TextureHandle(BGFX_INVALID_HANDLE);
        if (bgfx::isValid(sides) && sidesAspect > 0.0f)
        {
            int w = windowWidth, h = (int)(windowWidth / sidesAspect + 0.5f);
            if (h < windowHeight)
            {
                h = windowHeight;
                w = (int)(windowHeight * sidesAspect + 0.5f);
            }
            bgfx::setUniform(u_fxComposite, composite);
            bgfx::setTexture(0, s_fxScene, sides);
            drawFullScreenAt(kViewComposite, BGFX_INVALID_HANDLE, (windowWidth - w) / 2, (windowHeight - h) / 2, w, h,
                windowWidth, windowHeight, m_sidesProgram, "Composite", true);
        }
        // The scene where placeScene put it this frame
        bgfx::setUniform(u_fxComposite, composite);
        bgfx::setTexture(0, s_fxScene, m_sceneSource, sceneSampler);
        bgfx::setTexture(1, s_fxBloom, bloom ? m_bloomUp[0] : m_black);
        bgfx::setTexture(2, s_fxAO, ambientOcclusion ? m_aoBlurred : m_white);
    }
    else if (bgfx::isValid(m_sidePicture) && m_sideU1 > m_sideU0)
    {
        // The 4:3 centre of the picture lies on the scene; its sides go out
        // into the bars at the same scale
        const float fullWidth = (float)outputWidth / (m_sideU1 - m_sideU0);
        const int sideX = m_outputX - (int)(m_sideU0 * fullWidth + 0.5f);
        bgfx::setUniform(u_fxComposite, composite);
        bgfx::setTexture(0, s_fxScene, m_sidePicture);
        drawFullScreenAt(kViewComposite, BGFX_INVALID_HANDLE, sideX, m_outputY, (int)(fullWidth + 0.5f), outputHeight,
            windowWidth, windowHeight, m_sidesProgram, "Composite", true);
    }
    bgfx::setUniform(u_fxSmoke, m_smoke);
    const float smokeTime[4] = { (float)fmod(seconds, 10000.0), 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(u_fxSmokeTime, smokeTime);
    drawFullScreenAt(kViewComposite, BGFX_INVALID_HANDLE, sceneQuad[0], sceneQuad[1], sceneQuad[2], sceneQuad[3],
        windowWidth, windowHeight, m_compositeProgram, "Composite", true);
    // The smoke is set again by the frame that still shows it (AffSmoke)
    m_smoke[2] = m_smoke[3] = 0.0f;
}
