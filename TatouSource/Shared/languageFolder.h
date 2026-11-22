///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Other languages of AITD3 (GOG and Steam sell one at a time; Spanish only
// came on the original CD): a folder Languages\<name> in the working folder
// (Shared/userFolder.h), e.g. Languages\Spanish, holds either
// - loose files: the language PAK (ESPAGNOL.PAK) and the data files that go
//   with it (LISTSAMP.PAK, LISTLIFE.PAK and others, missingLanguageFiles; the
//   whole installed folder also works), and the CD tracks as
//   NN.ogg (track - 1, as in a copy of the data) or TrackNN.ogg (as GOG
//   names them), .wav or .mp3;
// - or the CD image: one .bin (or .img) with its .cue of the same name, for
//   the tracks; an .iso or a .bin without .cue has the files but no tracks.
// A language can also be a single file, a language pack: Languages\
// Spanish.language, the same loose files gathered one after the other
// without compression (zipArchive.h; tools/makeLanguagePack.py); a folder
// of the same name comes first.
// Used by the game and the launcher. Header-only.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "cdImage.h"
#include "zipArchive.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

// The folder of AITD3's files on its CD
static const char* const kLanguageImageFolder = "INDARK3";

// The folder in Languages of a language, by the name of its PAK (ESPAGNOL:
// Spanish); empty if it is not one of the game's languages
inline const char* languagePackName(const char* languagePak)
{
    static const char* const names[][2] = {
        { "ENGLISH", "English" }, { "FRANCAIS", "French" }, { "ITALIANO", "Italian" },
        { "ESPAGNOL", "Spanish" }, { "DEUTSCH", "German" }, { "RUSSIAN", "Russian" },
    };
    for (const auto& name : names)
        if (strcmp(languagePak, name[0]) == 0)
            return name[1];
    return "";
}

// The PAK of a language's texts (without .PAK): its own name, but Alone 3's
// Russian CD of Old-Games.ru keeps its Russian texts in ENGLISH.PAK
// (Shared/knownEditions.h, kRussianAitd3Texts)
inline std::string languageTextPak(const std::string& language)
{
    return language == "RUSSIAN" ? std::string("ENGLISH") : language;
}

// The CD image in a language folder whose INDARK3 folder has the language
// PAK (e.g. "ESPAGNOL.PAK"), opened with its .cue if there is one; false if
// the folder has none
inline bool openLanguageImage(const std::filesystem::path& folder, const std::string& languagePak, CdImage& image)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    for (const fs::directory_entry& entry : fs::directory_iterator(folder, ec))
    {
        std::string ext = entry.path().extension().string();
        for (char& c : ext)
            c = (char)tolower((unsigned char)c);
        if (ext != ".bin" && ext != ".img" && ext != ".iso")
            continue;
        fs::path cue = entry.path();
        cue.replace_extension(".cue");
        if (!fs::exists(cue, ec))
        {
            cue.replace_extension(".CUE");
            if (!fs::exists(cue, ec))
                cue.clear();
        }
        if (image.open(entry.path(), cue) && image.hasFile(kLanguageImageFolder, languagePak))
            return true;
    }
    return false;
}

// A file in a folder, whatever the case of its name (Linux)
inline bool languageFileExists(const std::filesystem::path& folder, const char* name)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    for (const fs::directory_entry& entry : fs::directory_iterator(folder, ec))
    {
        const std::string file = entry.path().filename().string();
        if (file.size() != strlen(name))
            continue;
        bool same = true;
        for (size_t i = 0; i < file.size() && same; i++)
            same = toupper((unsigned char)file[i]) == toupper((unsigned char)name[i]);
        if (same)
            return true;
    }
    return false;
}

// AITD3: the language installed in a data folder, from its INDARK3.CFG
// (byte 3, in the order of AITD3.EXE's table of PAKs: 0 French, 1 English,
// 2 Spanish, 3 German, 4 Italian); empty if it cannot be told
inline std::string installedLanguageAITD3(const std::filesystem::path& gameData)
{
    static const char* const languages[] = { "FRANCAIS", "ENGLISH", "ESPAGNOL", "DEUTSCH", "ITALIANO" };
    FILE* file = nullptr;
    std::error_code ec;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(gameData, ec))
    {
        std::string name = entry.path().filename().string();
        for (char& c : name)
            c = (char)toupper((unsigned char)c);
        if (name == "INDARK3.CFG")
        {
#ifdef _WIN32
            file = _wfopen(entry.path().c_str(), L"rb");
#else
            file = fopen(entry.path().c_str(), "rb");
#endif
            break;
        }
    }
    if (!file)
        return std::string();
    unsigned char header[4] = {};
    const size_t read = fread(header, 1, sizeof(header), file);
    fclose(file);
    if (read != sizeof(header) || header[3] >= sizeof(languages) / sizeof(languages[0]))
        return std::string();
    return languages[header[3]];
}

// AITD3: what a folder of loose files of a language (its PAK's name, e.g.
// "FRANCAIS") lacks to be used over the data in gameData; empty if nothing.
// It needs its text PAK, the voices and sounds (LISTSAMP.PAK) and the
// scripts (LISTLIFE.PAK). The French version numbers its scripts otherwise
// than the others (its objects call them by number): between French and
// another language, the objects (OBJETS.ITD) go with the scripts (BUGS.md T48)
template <typename HasFile>
inline std::string missingLanguageFilesBy(HasFile hasFile, const std::string& languagePak,
    const std::filesystem::path& gameData)
{
    std::string installed = installedLanguageAITD3(gameData);
    if (installed.empty())
        installed = "ENGLISH";
    std::vector<std::string> needed = { languagePak + ".PAK", "LISTSAMP.PAK", "LISTLIFE.PAK" };
    if ((installed == "FRANCAIS") != (languagePak == "FRANCAIS"))
        needed.push_back("OBJETS.ITD");
    std::string missing;
    for (const std::string& name : needed)
    {
        if (hasFile(name))
            continue;
        if (!missing.empty())
            missing += ", ";
        missing += name;
    }
    return missing;
}

// The language pack of a language folder (Languages\Spanish ->
// Languages\Spanish.language, any case of the extension)
inline std::filesystem::path languagePackFile(const std::filesystem::path& folder)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    const std::string wanted = folder.filename().string();
    for (const fs::directory_entry& entry : fs::directory_iterator(folder.parent_path(), ec))
    {
        std::string ext = entry.path().extension().string();
        for (char& c : ext)
            c = (char)tolower((unsigned char)c);
        if (ext == ".language" && entry.path().stem().string() == wanted && entry.is_regular_file(ec))
            return entry.path();
    }
    return fs::path();
}

// The language pack of a folder, opened, if it holds the language's PAK
// (e.g. "ESPAGNOL.PAK"); false if there is none
inline bool openLanguagePack(const std::filesystem::path& folder, const std::string& languagePak, ZipArchive& pack)
{
    const std::filesystem::path file = languagePackFile(folder);
    return !file.empty() && pack.open(file) && pack.find(languagePak) != nullptr;
}

// The same for a language pack
inline std::string missingLanguagePackFiles(const ZipArchive& pack, const std::string& languagePak,
    const std::filesystem::path& gameData)
{
    return missingLanguageFilesBy([&](const std::string& name) {
        const ZipArchive::Entry* entry = pack.find(name);
        return entry && entry->method == 0;
    }, languagePak, gameData);
}

inline std::string missingLanguageFiles(const std::filesystem::path& folder, const std::string& languagePak,
    const std::filesystem::path& gameData)
{
    return missingLanguageFilesBy([&](const std::string& name) { return languageFileExists(folder, name.c_str()); },
        languagePak, gameData);
}

