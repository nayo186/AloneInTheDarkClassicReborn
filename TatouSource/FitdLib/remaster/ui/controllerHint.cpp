///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Controller help on the start menu
///////////////////////////////////////////////////////////////////////////////

#include "controllerHint.h"
#include <bgfx/bgfx.h>
#include "common.h"  // after bgfx: windows.h min/max macros
#include "consoleLog.h"
#include "coreHooks.h"
#include "input.h"
#include "embedded/embeddedData.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../ThirdParty/bgfx.cmake/bimg/3rdparty/stb/stb_image.h"

#include "imguiBGFX.h"  // ImGui::Image with bgfx textures

static bgfx::TextureHandle s_texture = BGFX_INVALID_HANDLE;
static int s_width = 0;
static int s_height = 0;
static bool s_loaded = false;

// The picture of the controller buttons, embedded in the program
static void load()
{
    s_loaded = true;
    const unsigned char* file = nullptr;
    size_t fileSize = 0;
    if (!getEmbeddedFile("ControllerHint.png", &file, &fileSize))
    {
        printf(MAIN_WARN "Controller help: ControllerHint.png is not embedded\n");
        return;
    }
    int channels;
    unsigned char* pixels = stbi_load_from_memory(file, (int)fileSize, &s_width, &s_height, &channels, 4);
    if (!pixels)
    {
        printf(MAIN_WARN "Controller help: ControllerHint.png could not be read (%s)\n", stbi_failure_reason());
        return;
    }
    s_texture = bgfx::createTexture2D((uint16_t)s_width, (uint16_t)s_height, false, 1, bgfx::TextureFormat::RGBA8,
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP, bgfx::copy(pixels, (uint32_t)s_width * s_height * 4));
    stbi_image_free(pixels);
}

// Start menu frame (core/startupMenu.cpp): the help at the bottom of the play
// area, only with a controller connected
static void onStartMenuDrawn()
{
    if (!g_controllerState.connected)
        return;
    if (!s_loaded)
        load();
    if (!bgfx::isValid(s_texture) || s_width <= 0 || s_height <= 0)
        return;

    int rect[4];
    osystem_getSceneRect(rect);
    if (rect[2] <= 0 || rect[3] <= 0)
        return;

    const float width = rect[2] * 0.28f;
    const float height = width * (float)s_height / (float)s_width;
    const float x = rect[0] + (rect[2] - width) * 0.5f;
    const float y = rect[1] + rect[3] - height - rect[3] * 0.02f;

    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    if (ImGui::Begin("##controllerHelp", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground))
    {
        ImGui::Image(s_texture, ImVec2(width, height));
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void controllerHintRegister()
{
    g_onStartMenuDrawn = onStartMenuDrawn;
}
