///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The audio window, shared by the game (F1 window) and the launcher
// (advanced options): the volume of the background music and of the voices
// and effects, 0-100 (audioVolume.h), and the narrator: the game's own
// (English) or a folder of Voices (Shared/narrators.h), with any text
// language. Header-only, like controlsWindow.h, whose window style it uses.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "advancedOptionsText.h"
#include "audioVolume.h"
#include "controlsWindow.h"
#include "ui/menuBase.h"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

// A narrator's name in the menus: the game's own (empty) is "English"; the
// folders named after a language (French, Spanish...) in the menu's
// language; any other folder (a fans' dubbing) by its name
inline std::string narratorLabel(const std::string& folder, int language)
{
    if (folder.empty())
        return advancedText(AdvancedText::VoicesEnglish, language);
    std::string lower = folder;
    for (char& c : lower)
        c = (char)tolower((unsigned char)c);
    static const struct { const char* folder; AdvancedText text; } known[] = {
        { "english", AdvancedText::VoicesEnglish }, { "french", AdvancedText::VoicesFrench },
        { "spanish", AdvancedText::VoicesSpanish }, { "italian", AdvancedText::VoicesItalian },
        { "german", AdvancedText::VoicesGerman },
    };
    for (const auto& entry : known)
        if (lower == entry.folder)
            return advancedText(entry.text, language);
    return folder;
}

struct AudioWindowState
{
    bool open = false;
    bool justOpened = false;     // set with open = true
    bool requestClose = false;   // close at the next frame (B in the launcher)
};

// Draws the window while state.open (call every frame). Returns true when a
// volume or the narrator changed. voices: the chosen narrator's folder (empty:
// the game's), narrators: the folders with voices (the list is greyed out
// without any). asPopup: a modal popup inside the caller's popup (the
// launcher); otherwise a window of its own (the game's F1 window, hidden
// meanwhile so they never overlap).
inline bool drawAudioWindow(AudioWindowState& state, int& musicVolume, int& soundVolume, std::string& voices,
                            const std::vector<std::string>& narrators, int language, bool asPopup,
                            const ControlsWindowStyle& style = ControlsWindowStyle())
{
    auto text = [language](AdvancedText t) { return advancedText(t, language); };
    if (!state.open)
        return false;

    ImGuiIO& io = ImGui::GetIO();
    const char* popupId = "##audioWindow";
    if (state.justOpened && asPopup)
        ImGui::OpenPopup(popupId);

    // Centred, its height follows the content (three rows: it always fits)
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

    // Narrator: the game's own (English, empty name) and each folder found;
    // a chosen folder that is gone goes back to the game's
    if (!voices.empty() && std::find(narrators.begin(), narrators.end(), voices) == narrators.end())
    {
        voices.clear();
        changed = true;
    }
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(text(AdvancedText::Voices));
    menu::helpMarker(text(AdvancedText::VoicesHelp));
    ImGui::SameLine(column);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::BeginDisabled(narrators.empty());
    if (ImGui::BeginCombo("##voices", narratorLabel(voices, language).c_str()))
    {
        if (ImGui::Selectable(text(AdvancedText::VoicesEnglish), voices.empty()) && !voices.empty())
        {
            voices.clear();
            changed = true;
        }
        for (const std::string& narrator : narrators)
        {
            if (ImGui::Selectable(narratorLabel(narrator, language).c_str(), voices == narrator) && voices != narrator)
            {
                voices = narrator;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    menu::helpWhenFocused(text(AdvancedText::VoicesHelp));

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
