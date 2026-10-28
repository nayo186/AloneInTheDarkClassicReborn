///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Object editor (see objectEditor.h)
///////////////////////////////////////////////////////////////////////////////

#include "objectEditor.h"

#if CLASSIC_REBORN_DEV_TOOLS

#include "objectFixes.h"
#include "common.h"
#include "coreHooks.h"
#include "gameFolder.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <algorithm>
#include <atomic>
#include <cstdio>

static std::atomic_bool s_open(false);
static int s_selected = -1;      // index in ListObjets
static bool s_blink = true;      // the selected object blinks
static bool s_changed = false;   // fixes not saved yet

void objectEditorToggle()
{
    if (!isDebugMode())   // only in debug mode (launcher.cfg debugMode = 1)
        return;
    s_open.store(!s_open.load());
}

bool objectEditorIsOpen()
{
    return s_open.load();
}

void objectEditorRegister()
{
    g_isGameFrozenLive = objectEditorIsOpen;   // the game waits, the scene is drawn
}

// A value with a drag field and -/+ buttons (one step)
static void axisRow(const char* label, int& value, int minimum, int maximum, float speed)
{
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(ImGui::GetFontSize() * 7.0f);
    ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() * 2.0f - ImGui::GetStyle().ItemSpacing.x * 2.0f);
    if (ImGui::DragInt("##value", &value, speed, minimum, maximum, "%d", ImGuiSliderFlags_AlwaysClamp))
        s_changed = true;
    ImGui::SameLine();
    if (ImGui::Button("-", ImVec2(ImGui::GetFrameHeight(), 0.0f)) && value > minimum)
    {
        value--;
        s_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("+", ImVec2(ImGui::GetFrameHeight(), 0.0f)) && value < maximum)
    {
        value++;
        s_changed = true;
    }
    ImGui::PopID();
}

// Which way a size grows: both sides, only + or only -
static void growChoice(const char* id, int& grow)
{
    static const char* const labels[] = { "Ambos lados", "Solo +", "Solo -" };
    static const int values[] = { 0, 1, -1 };
    ImGui::PushID(id);
    ImGui::Indent();
    ImGui::TextDisabled("Crece hacia:");
    for (int i = 0; i < 3; i++)
    {
        ImGui::SameLine();
        if (ImGui::RadioButton(labels[i], grow == values[i]))
        {
            grow = values[i];
            s_changed = true;
        }
    }
    ImGui::Unindent();
    ImGui::PopID();
}

void objectEditorDraw()
{
    if (!s_open.load())
    {
        objectFixesSetBlinking(-1);
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    const float scale = (std::max)(1.0f, io.DisplaySize.y / 900.0f);
    ImGui::SetNextWindowSize(ImVec2(440.0f * scale, 0.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_Appearing);
    bool open = true;
    if (!ImGui::Begin("Editor de objetos (prueba, F10)", &open, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        if (!open)
            s_open.store(false);
        return;
    }

    std::lock_guard<std::mutex> lock(objectFixesMutex());
    const SeenObject* seenObjects = objectFixesSeen();
    const Uint64 now = SDL_GetTicks();

    ImGui::TextDisabled("Juego congelado; los ajustes se ven al momento.");
    ImGui::Checkbox("El elegido parpadea", &s_blink);
    ImGui::Separator();

    // The objects drawn in the last half second (this camera)
    ImGui::TextUnformatted("Objetos de esta cámara:");
    ImGui::BeginChild("##objects", ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 7.0f), ImGuiChildFlags_Borders);
    for (int i = 0; i < NUM_MAX_OBJECT; i++)
    {
        const SeenObject& seen = seenObjects[i];
        if (!seen.lastSeen || now - seen.lastSeen > 500)
            continue;
        const ObjectFix* existing = findObjectFix(seen.floor, seen.world, seen.room, seen.body);
        const bool fixed = existing && !objectFixIsEmpty(*existing);
        char label[96];
        if (seen.world >= 0)
            snprintf(label, sizeof(label), "Objeto %d  (modelo %d, sala %d)%s", seen.world, seen.body, seen.room, fixed ? "  *" : "");
        else
            snprintf(label, sizeof(label), "Actor %d  (modelo %d, sala %d)%s", i, seen.body, seen.room, fixed ? "  *" : "");
        if (ImGui::Selectable(label, s_selected == i))
            s_selected = i;
    }
    ImGui::EndChild();

    // The chosen object's fix
    const bool valid = s_selected >= 0 && s_selected < NUM_MAX_OBJECT && seenObjects[s_selected].lastSeen &&
        now - seenObjects[s_selected].lastSeen <= 500;
    objectFixesSetBlinking(valid && s_blink ? s_selected : -1);
    if (valid)
    {
        const SeenObject& seen = seenObjects[s_selected];
        ObjectFix* fix = findObjectFix(seen.floor, seen.world, seen.room, seen.body);
        if (!fix)
        {
            ObjectFix created;
            created.floor = seen.floor;
            created.world = seen.world;
            created.room = seen.room;
            created.body = seen.body;
            objectFixes().push_back(created);
            fix = &objectFixes().back();
        }
        ImGui::Spacing();
        ImGui::TextUnformatted("Posición (unidades del mundo):");
        axisRow("X", fix->offset[0], -4000, 4000, 1.0f);
        axisRow("Y (altura)", fix->offset[1], -4000, 4000, 1.0f);
        axisRow("Z", fix->offset[2], -4000, 4000, 1.0f);
        ImGui::TextUnformatted("Giro (1024 = vuelta completa):");
        axisRow("Alfa", fix->rotation[0], -1023, 1023, 1.0f);
        axisRow("Beta", fix->rotation[1], -1023, 1023, 1.0f);
        axisRow("Gamma", fix->rotation[2], -1023, 1023, 1.0f);
        ImGui::TextUnformatted("Tamaño (%, 100 = el original; en los ejes del modelo):");
        axisRow("Ancho (X)", fix->scale[0], 10, 500, 0.5f);
        growChoice("growX", fix->grow[0]);
        axisRow("Alto (Y)", fix->scale[1], 10, 500, 0.5f);
        growChoice("growY", fix->grow[1]);
        axisRow("Fondo (Z)", fix->scale[2], 10, 500, 0.5f);
        growChoice("growZ", fix->grow[2]);
        if (ImGui::Button("Volver a su sitio original"))
        {
            *fix = ObjectFix{ fix->floor, fix->world, fix->room, fix->body };
            s_changed = true;
        }
    }
    else
    {
        ImGui::TextDisabled("Elige un objeto de la lista.");
    }

    // Saved into the project: the next build gives them to every player
    ImGui::Separator();
    if (ImGui::Button(s_changed ? "Guardar ajustes *" : "Guardar ajustes") && objectFixesSave())
        s_changed = false;
    ImGui::SameLine();
    ImGui::TextDisabled("assets/objectFixes/%s", objectFixesFileName());
    ImGui::End();
    if (!open)
        s_open.store(false);
}

#else   // players' builds: no editor

void objectEditorToggle() {}
bool objectEditorIsOpen() { return false; }
void objectEditorDraw() {}
void objectEditorRegister() {}

#endif
