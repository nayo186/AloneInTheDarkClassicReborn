///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The game's data files and a language from the player's CD (see
// languagePack.h)
///////////////////////////////////////////////////////////////////////////////

#include "languagePack.h"
#include "consoleLog.h"
#include "fileNameCase.h"
#include "gameImage.h"
#include "userFolder.h"
#include "languageFolder.h"
#include "gameFolder.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <map>
#include <mutex>

namespace fs = std::filesystem;

extern "C" { extern char homePath[512]; }

static std::string s_folder;
static CdImage s_image;
static bool s_hasImage = false;
static std::mutex s_cacheMutex;
static std::map<std::string, std::vector<unsigned char>> s_cache;   // files read from the language's image
static std::map<std::string, std::vector<unsigned char>> s_discCache;   // files read from the game's CD image

const std::string& languagePackFolder() { return s_folder; }

static bool pathExists(const std::string& path)
{
    std::error_code error;
    return fs::exists(fs::u8path(path), error);
}

bool selectLanguagePack(const std::string& language)
{
    s_folder.clear();
    s_hasImage = false;
    const char* name = languagePackName(language.c_str());
    if (!*name)
        return false;
    const std::string folder = matchFileCase(std::string(kLanguagesFolder) + "/" + name) + "/";
    const std::string pak = language + ".PAK";
    if (pathExists(matchFileCase(folder + pak)))
    {
        // Loose files: only if none of the ones it needs is missing, else
        // the game would mix its scripts with other objects (BUGS.md T48)
        const std::string missing = missingLanguageFiles(fs::u8path(folder), language, fs::u8path(homePath[0] ? homePath : "."));
        if (!missing.empty())
        {
            printf(MAIN_WARN "Language folder %s not used: it lacks %s\n", folder.c_str(), missing.c_str());
            return false;
        }
        s_folder = folder;
        return true;
    }
    if (!openLanguageImage(fs::u8path(folder), pak, s_image))
        return false;
    s_folder = folder;
    s_hasImage = true;
    int audioTracks = 0;
    for (const CdTrack& track : s_image.tracks())
        audioTracks += track.audio ? 1 : 0;
    printf(MAIN_OK "Language CD image: %s (%d files, %d audio tracks)\n", s_image.path().string().c_str(),
        s_image.fileCount(), audioTracks);
    return true;
}

// A file of the language's CD image, read once and kept; null if not there
static const std::vector<unsigned char>* imageFile(const char* name)
{
    if (!s_hasImage || !s_image.hasFile(kLanguageImageFolder, name))
        return nullptr;
    std::lock_guard<std::mutex> lock(s_cacheMutex);
    auto found = s_cache.find(name);
    if (found == s_cache.end())
    {
        std::vector<unsigned char> data;
        if (!s_image.readFile(kLanguageImageFolder, name, data))
            return nullptr;
        found = s_cache.emplace(name, std::move(data)).first;
    }
    return &found->second;
}

// A file of the game's CD image (the player's CD: gameFolder.h), read once
// and kept; null if not there
static const std::vector<unsigned char>* discFile(const char* name)
{
    const std::string& folder = gameImageFolder();
    if (folder.empty() || !gameImageHasFile(folder.c_str(), name))
        return nullptr;
    std::lock_guard<std::mutex> lock(s_cacheMutex);
    auto found = s_discCache.find(name);
    if (found == s_discCache.end())
    {
        std::vector<unsigned char> data;
        if (!gameImageReadFile(folder.c_str(), name, data))
            return nullptr;
        found = s_discCache.emplace(name, std::move(data)).first;
    }
    return &found->second;
}

bool dataFileExists(const char* name)
{
    if (!s_folder.empty() && (pathExists(matchFileCase(s_folder + name)) || imageFile(name)))
        return true;
    if (pathExists(matchFileCase(std::string(homePath) + name)))
        return true;
    return !gameImageFolder().empty() && gameImageHasFile(gameImageFolder().c_str(), name);
}

SoLoud::AudioSource* languagePackCreateTrack(int track)
{
    return s_hasImage ? createCdTrack(s_image, track) : nullptr;
}

bool DataFile::open(const char* name)
{
    close();
    // Loose in the language folder, else in its image, else in homePath, else
    // in the game's CD image
    if (!s_folder.empty())
    {
        const std::string loose = matchFileCase(s_folder + name);
        if (pathExists(loose))
        {
            m_file = fopen(loose.c_str(), "rb");
            return m_file != nullptr;
        }
        if ((m_memory = imageFile(name)))
            return true;
    }
    m_file = fopen(matchFileCase(std::string(homePath) + name).c_str(), "rb");
    if (m_file)
        return true;
    m_memory = discFile(name);
    return m_memory != nullptr;
}

void DataFile::seek(long offset, int origin)
{
    if (m_file)
    {
        fseek(m_file, offset, origin);
        return;
    }
    if (!m_memory)
        return;
    const long base = origin == SEEK_SET ? 0 : origin == SEEK_CUR ? m_position : (long)m_memory->size();
    m_position = base + offset;
}

size_t DataFile::read(void* buffer, size_t size)
{
    if (m_file)
        return fread(buffer, 1, size, m_file);
    if (!m_memory || m_position < 0 || (size_t)m_position >= m_memory->size())
        return 0;
    const size_t count = (std::min)(size, m_memory->size() - (size_t)m_position);
    memcpy(buffer, m_memory->data() + m_position, count);
    m_position += (long)count;
    return count;
}

long DataFile::size()
{
    if (m_memory)
        return (long)m_memory->size();
    if (!m_file)
        return 0;
    const long position = ftell(m_file);
    fseek(m_file, 0, SEEK_END);
    const long end = ftell(m_file);
    fseek(m_file, position, SEEK_SET);
    return end;
}

void DataFile::close()
{
    if (m_file)
        fclose(m_file);
    m_file = nullptr;
    m_memory = nullptr;
    m_position = 0;
}
