///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Debug cheats (see debugCheats.h)
///////////////////////////////////////////////////////////////////////////////

#include "debugCheats.h"

#if CLASSIC_REBORN_DEV_TOOLS

#include "gameFolder.h"
#include "common.h"
#include "settings.h"
#include "consoleLog.h"
#include "coreHooks.h"

#include <imgui.h>
#include <atomic>
#include <cctype>
#include <mutex>
#include <string>
#include <vector>

static std::atomic_bool s_open(false);
static std::mutex s_pendingMutex;
static std::vector<int> s_pending;   // items chosen, given when the window closes

void debugCheatsToggle()
{
    if (!isDebugMode())   // test tool: only in debug mode (launcher)
        return;
    if (g_gameId != AITD1 && g_gameId != JACK)
        return;
    s_open.store(!s_open.load());
}

bool debugCheatsWindowOpen()
{
    return s_open.load();
}

static bool carried(int objIdx)
{
    for (int i = 0; i < numObjInInventoryTable[currentInventory]; i++)
        if (inventoryTable[currentInventory][i] == objIdx)
            return true;
    return false;
}

static bool pending(int objIdx)
{
    std::lock_guard<std::mutex> lock(s_pendingMutex);
    for (int idx : s_pending)
        if (idx == objIdx)
            return true;
    return false;
}

// An item that can be picked up: it has a "found" view and name
static bool isItem(const tWorldObject& object)
{
    return object.foundBody != -1 && object.foundName != -1;
}

static std::string itemName(const tWorldObject& object)
{
    textEntryStruct* name = getTextFromIdx(object.foundName);
    return (name && name->textPtr) ? (const char*)name->textPtr : "?";
}

// The game's text uses the DOS code page 437: the Spanish, French, Italian
// and German letters it needs, as UTF-8 (for ImGui) and without accent (for
// the search)
struct DosLetter { unsigned char dos; const char* utf8; char plain; };
static const DosLetter kDosLetters[] = {
    { 0x80, "\xC3\x87", 'c' }, { 0x81, "\xC3\xBC", 'u' }, { 0x82, "\xC3\xA9", 'e' }, { 0x83, "\xC3\xA2", 'a' },
    { 0x84, "\xC3\xA4", 'a' }, { 0x85, "\xC3\xA0", 'a' }, { 0x87, "\xC3\xA7", 'c' }, { 0x88, "\xC3\xAA", 'e' },
    { 0x89, "\xC3\xAB", 'e' }, { 0x8A, "\xC3\xA8", 'e' }, { 0x8B, "\xC3\xAF", 'i' }, { 0x8C, "\xC3\xAE", 'i' },
    { 0x8D, "\xC3\xAC", 'i' }, { 0x8E, "\xC3\x84", 'a' }, { 0x90, "\xC3\x89", 'e' }, { 0x93, "\xC3\xB4", 'o' },
    { 0x94, "\xC3\xB6", 'o' }, { 0x95, "\xC3\xB2", 'o' }, { 0x96, "\xC3\xBB", 'u' }, { 0x97, "\xC3\xB9", 'u' },
    { 0x99, "\xC3\x96", 'o' }, { 0x9A, "\xC3\x9C", 'u' }, { 0xA0, "\xC3\xA1", 'a' }, { 0xA1, "\xC3\xAD", 'i' },
    { 0xA2, "\xC3\xB3", 'o' }, { 0xA3, "\xC3\xBA", 'u' }, { 0xA4, "\xC3\xB1", 'n' }, { 0xA5, "\xC3\x91", 'n' },
    { 0xA8, "\xC2\xBF", '?' }, { 0xAD, "\xC2\xA1", '!' }, { 0xE1, "\xC3\x9F", 's' },
};

static const DosLetter* dosLetter(unsigned char c)
{
    for (const DosLetter& letter : kDosLetters)
        if (letter.dos == c)
            return &letter;
    return nullptr;
}

static std::string toUtf8(const std::string& text)
{
    std::string out;
    for (unsigned char c : text)
    {
        if (c < 0x80)
            out += (char)c;
        else if (const DosLetter* letter = dosLetter(c))
            out += letter->utf8;
        else
            out += '?';
    }
    return out;
}

// Lower case, without accents: "fosforo" finds "fósforo". The search box gives
// UTF-8, the game's names are DOS text
static std::string searchKey(const std::string& text, bool dosText)
{
    std::string out;
    for (size_t i = 0; i < text.size(); i++)
    {
        const unsigned char c = (unsigned char)text[i];
        if (c < 0x80)
            out += (char)tolower(c);
        else if (dosText)
        {
            const DosLetter* letter = dosLetter(c);
            out += letter ? letter->plain : '?';
        }
        else if ((c == 0xC3 || c == 0xC2) && i + 1 < text.size())
        {
            // A UTF-8 letter: compare it by its DOS twin
            const unsigned char next = (unsigned char)text[++i];
            char plain = '?';
            for (const DosLetter& letter : kDosLetters)
                if ((unsigned char)letter.utf8[0] == c && (unsigned char)letter.utf8[1] == next)
                    plain = letter.plain;
            out += plain;
        }
    }
    return out;
}

void debugCheatsDraw()
{
    if (!s_open.load())
        return;

    // take() moves one slot too many when inserting: two slots stay free
    const int room = (INVENTORY_SIZE - 2) - numObjInInventoryTable[currentInventory];
    int chosen;
    {
        std::lock_guard<std::mutex> lock(s_pendingMutex);
        chosen = (int)s_pending.size();
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(460.0f, io.DisplaySize.y * 0.7f), ImGuiCond_Appearing);
    bool open = true;
    if (ImGui::Begin("Objetos (prueba, F9)", &open, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("Huecos libres en el inventario: %d", room - chosen);
        ImGui::TextDisabled("Se entregan al cerrar esta ventana.");
        static char filter[64] = "";
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##filter", "Buscar por nombre", filter, sizeof(filter));
        const std::string wanted = searchKey(filter, false);
        ImGui::Separator();

        ImGui::BeginChild("##items");
        for (int i = 0; i < (int)ListWorldObjets.size(); i++)
        {
            const tWorldObject& object = ListWorldObjets[i];
            if (!isItem(object))
                continue;
            const std::string name = itemName(object);
            if (!wanted.empty() && searchKey(name, true).find(wanted) == std::string::npos)
                continue;

            ImGui::PushID(i);
            const bool have = (object.foundFlag & 0x8000) || carried(i);
            const bool chosenItem = pending(i);
            ImGui::BeginDisabled(have || chosenItem || room - chosen <= 0);
            if (ImGui::SmallButton(have ? "Llevado" : (chosenItem ? "Elegido" : "Dar")))
            {
                std::lock_guard<std::mutex> lock(s_pendingMutex);
                s_pending.push_back(i);
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::Text("%s  (objeto %d)", toUtf8(name).c_str(), i);
            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    ImGui::End();
    if (!open)
        s_open.store(false);
}

// Game thread, once per loop of the game (core/mainLoop.cpp): the chosen
// items are picked up as the game does (take: found script, leaves the room)
static void onGameLoopTick()
{
    std::vector<int> items;
    {
        std::lock_guard<std::mutex> lock(s_pendingMutex);
        items.swap(s_pending);
    }
    for (int objIdx : items)
    {
        if (objIdx < 0 || objIdx >= (int)ListWorldObjets.size() || carried(objIdx))
            continue;
        if (numObjInInventoryTable[currentInventory] >= INVENTORY_SIZE - 2)
        {
            printf(MAIN_WARN "Debug cheat (F9): the inventory is full\n");
            break;
        }
        take(objIdx);
        printf(MAIN_TAG "Debug cheat (F9): %s (object %d)\n", itemName(ListWorldObjets[objIdx]).c_str(), objIdx);
    }
}

void debugCheatsRegister()
{
    g_onGameLoopTick = onGameLoopTick;
}

#else   // players' builds: no item window

void debugCheatsToggle() {}
bool debugCheatsWindowOpen() { return false; }
void debugCheatsDraw() {}
void debugCheatsRegister() {}

#endif
