///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The audio window, shared by the game (F1 window) and the launcher
// (advanced options): the volume of the background music and of the voices
// and effects, 0-100 (audioVolume.h). Header-only, like controlsWindow.h,
// whose window style it uses.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "advancedOptionsText.h"
#include "audioVolume.h"
#include "controlsWindow.h"
#include "ui/menuBase.h"

#include <imgui.h>

#include <algorithm>

struct AudioWindowState
{
    bool open = false;
    bool justOpened = false;     // set with open = true
    bool requestClose = false;   // close at the next frame (B in the launcher)
};

// Draws the window while state.open (call every frame). Returns true when a
// volume changed. asPopup: a modal popup inside the caller's popup (the
// launcher); otherwise a window of its own (the game's F1 window, hidden
// meanwhile so they never overlap).
inline bool drawAudioWindow(AudioWindowState& state, int& musicVolume, int& soundVolume, int language, bool asPopup,
                            const ControlsWindowStyle& style = ControlsWindowStyle())
{
    auto text = [language](AdvancedText t) { return advancedText(t, language); };
    if (!state.open)
        return false;

    ImGuiIO& io = ImGui::GetIO();
    const char* popupId = "##audioWindow";
    if (state.justOpened && asPopup)
        ImGui::OpenPopup(popupId);

    // Centred, its height follows the content (two sliders: it always fits)
    const float width = (std::min)(460.0f * style.scale, io.DisplaySize.x * 0.94f);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);
    if (!asPopup && state.justOpened)
        ImGui::SetNextWindowFocus();
    state.justOpened = false;

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    if (asPopup)
    {
        if (!ImGui::BeginPopupModal(popupId, nullptr, flags))
        {
            state.open = false;
            return false;
        }
    }
    else if (!ImGui::Begin(popupId, nullptr, flags))
    {
        ImGui::End();
        return false;
    }
    ImGui::SetWindowFontScale(style.fontScale > 0.0f ? style.fontScale : style.scale);

    if (style.titleFont)
        ImGui::PushFont(style.titleFont);
    ImGui::TextColored(style.titleColor, "%s", text(AdvancedText::Audio));
    if (style.titleFont)
        ImGui::PopFont();
    ImGui::Separator();
    ImGui::Spacing();

    // Name and "(?)" on the left, the slider in a column on the right
    bool changed = false;
    const float column = ImGui::GetContentRegionAvail().x * 0.45f;
    auto volumeRow = [&](const char* id, AdvancedText label, AdvancedText help, int& volume)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(text(label));
        menu::helpMarker(text(help));
        ImGui::SameLine(column);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::SliderInt(id, &volume, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp))
            changed = true;
        menu::helpWhenFocused(text(help));
    };
    volumeRow("##musicVolume", AdvancedText::MusicVolume, AdvancedText::MusicVolumeHelp, musicVolume);
    volumeRow("##soundVolume", AdvancedText::SoundVolume, AdvancedText::SoundVolumeHelp, soundVolume);

    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button(text(AdvancedText::Close), ImVec2(-1.0f, 0.0f))
        || (!ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) || state.requestClose)
    {
        state.requestClose = false;
        state.open = false;
        if (asPopup)
            ImGui::CloseCurrentPopup();
    }

    if (asPopup)
        ImGui::EndPopup();
    else
        ImGui::End();
    return changed;
}
