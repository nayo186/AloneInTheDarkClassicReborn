///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// GOG's CD image (see gogImage.h)
///////////////////////////////////////////////////////////////////////////////

#include "gogImage.h"
#include "gameFolder.h"
#include "consoleLog.h"

#include "soloud.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

static const int kSectorSize = 2352;       // raw CD sector
static const int kSectorDataOffset = 16;   // MODE1: sync and header first
static const int kSectorDataSize = 2048;

struct CdTrack
{
    int number = 0;
    bool audio = false;
    long long startSector = 0;
    long long endSector = 0;   // first sector after the track
};

struct CdFile
{
    unsigned int sector = 0;
    unsigned int size = 0;
};

static std::once_flag s_openOnce;
static bool s_available = false;
static std::vector<CdTrack> s_tracks;
static std::map<std::string, CdFile> s_files;   // "INDARK/000123.VOC"

static std::string upper(std::string text)
{
    for (char& c : text)
        c = (char)toupper((unsigned char)c);
    return text;
}

// GAME.INS: "TRACK 02 AUDIO" then "INDEX 01 05:31:19" (minutes, seconds, frames)
static bool readTrackList(long long imageSectors)
{
    std::ifstream cue(gogCuePath());
    if (!cue)
        return false;
    std::string line;
    CdTrack* current = nullptr;
    while (std::getline(cue, line))
    {
        std::istringstream words(line);
        std::string word;
        words >> word;
        if (word == "TRACK")
        {
            CdTrack track;
            std::string type;
            words >> track.number >> type;
            track.audio = (upper(type) == "AUDIO");
            s_tracks.push_back(track);
            current = &s_tracks.back();
        }
        else if (word == "INDEX" && current)
        {
            int index = 0, minutes = 0, seconds = 0, frames = 0;
            std::string time;
            words >> index >> time;
            if (index == 1 && sscanf(time.c_str(), "%d:%d:%d", &minutes, &seconds, &frames) == 3)
                current->startSector = ((long long)minutes * 60 + seconds) * 75 + frames;
        }
    }
    for (size_t i = 0; i < s_tracks.size(); i++)
        s_tracks[i].endSector = (i + 1 < s_tracks.size()) ? s_tracks[i + 1].startSector : imageSectors;
    return !s_tracks.empty();
}

static bool readSector(std::ifstream& image, long long sector, unsigned char* out)
{
    image.seekg(sector * kSectorSize + kSectorDataOffset);
    image.read((char*)out, kSectorDataSize);
    return image.gcount() == kSectorDataSize;
}

// The files of one folder of the CD's root (ISO 9660 directory records)
static void readFolder(std::ifstream& image, const std::string& folder, unsigned int sector, unsigned int size)
{
    std::vector<unsigned char> data(((size + kSectorDataSize - 1) / kSectorDataSize) * kSectorDataSize);
    for (size_t i = 0; i * kSectorDataSize < data.size(); i++)
        if (!readSector(image, sector + i, &data[i * kSectorDataSize]))
            return;
    size_t pos = 0;
    while (pos < size)
    {
        const unsigned char length = data[pos];
        if (length == 0)
        {
            pos = (pos / kSectorDataSize + 1) * kSectorDataSize;   // records do not cross sectors
            continue;
        }
        const unsigned char* record = &data[pos];
        pos += length;
        const unsigned char nameLength = record[32];
        if ((record[25] & 2) || nameLength == 0 || (nameLength == 1 && record[33] <= 1))
            continue;   // a folder, or "." and ".."
        std::string name((const char*)record + 33, nameLength);
        name = upper(name.substr(0, name.find(';')));
        CdFile file;
        memcpy(&file.sector, record + 2, 4);
        memcpy(&file.size, record + 10, 4);
        s_files[folder + "/" + name] = file;
    }
}

static void openImage()
{
    if (!isGogInstall())
        return;
    std::ifstream image(gogImagePath(), std::ios::binary);
    if (!image)
    {
        printf(AL_WARN "GOG image: cannot open %s" CON_RESET "\n", gogImagePath().c_str());
        return;
    }
    image.seekg(0, std::ios::end);
    const long long imageSectors = (long long)image.tellg() / kSectorSize;
    if (!readTrackList(imageSectors) || s_tracks[0].audio)
    {
        printf(AL_WARN "GOG image: cannot read the track list %s" CON_RESET "\n", gogCuePath().c_str());
        return;
    }

    // The CD's files: primary volume descriptor at sector 16, then the root
    unsigned char descriptor[kSectorDataSize];
    if (!readSector(image, 16, descriptor) || memcmp(descriptor + 1, "CD001", 5) != 0)
    {
        printf(AL_WARN "GOG image: no CD file system in %s" CON_RESET "\n", gogImagePath().c_str());
        return;
    }
    unsigned int rootSector = 0, rootSize = 0;
    memcpy(&rootSector, descriptor + 156 + 2, 4);
    memcpy(&rootSize, descriptor + 156 + 10, 4);
    std::vector<unsigned char> root(((rootSize + kSectorDataSize - 1) / kSectorDataSize) * kSectorDataSize);
    for (size_t i = 0; i * kSectorDataSize < root.size(); i++)
        if (!readSector(image, rootSector + i, &root[i * kSectorDataSize]))
            return;
    size_t pos = 0;
    while (pos < rootSize)
    {
        const unsigned char length = root[pos];
        if (length == 0)
        {
            pos = (pos / kSectorDataSize + 1) * kSectorDataSize;
            continue;
        }
        const unsigned char* record = &root[pos];
        pos += length;
        if (!(record[25] & 2) || record[32] <= 1)
            continue;
        const std::string folder = upper(std::string((const char*)record + 33, record[32]));
        if (folder != "INDARK" && folder != "JACK")
            continue;
        unsigned int sector = 0, size = 0;
        memcpy(&sector, record + 2, 4);
        memcpy(&size, record + 10, 4);
        readFolder(image, folder, sector, size);
    }

    int musicTracks = 0;
    for (const CdTrack& track : s_tracks)
        musicTracks += track.audio ? 1 : 0;
    s_available = true;
    printf(AL_OK "GOG image: %d files and %d music tracks in %s\n", (int)s_files.size(), musicTracks,
        gogImagePath().c_str());
}

bool gogImageAvailable()
{
    std::call_once(s_openOnce, openImage);
    return s_available;
}

bool gogImageReadFile(const char* folder, const char* name, std::vector<unsigned char>& data)
{
    if (!gogImageAvailable())
        return false;
    auto found = s_files.find(upper(folder) + "/" + upper(name));
    if (found == s_files.end())
        return false;
    std::ifstream image(gogImagePath(), std::ios::binary);
    if (!image)
        return false;
    data.resize(found->second.size);
    std::vector<unsigned char> sector(kSectorDataSize);
    for (size_t done = 0, i = 0; done < data.size(); i++)
    {
        if (!readSector(image, found->second.sector + i, sector.data()))
            return false;
        const size_t part = (std::min)(data.size() - done, (size_t)kSectorDataSize);
        memcpy(&data[done], sector.data(), part);
        done += part;
    }
    return true;
}

// ---- Music tracks -------------------------------------------------------------

class GogTrackInstance : public SoLoud::AudioSourceInstance
{
public:
    GogTrackInstance(long long start, long long end) : m_position(start), m_end(end)
    {
        m_image.open(gogImagePath(), std::ios::binary);
        m_image.seekg(start);
    }

    // SoLoud wants the channels one after the other: left samples, then right
    unsigned int getAudio(float* buffer, unsigned int samplesToRead, unsigned int bufferSize) override
    {
        const long long left = (m_end - m_position) / 4;
        const unsigned int frames = (unsigned int)(std::min<long long>)(samplesToRead, left > 0 ? left : 0);
        m_pcm.resize((size_t)frames * 2);
        if (frames > 0 && m_image)
        {
            m_image.read((char*)m_pcm.data(), (std::streamsize)frames * 4);
            const unsigned int got = (unsigned int)(m_image.gcount() / 4);
            for (unsigned int i = 0; i < got; i++)
            {
                buffer[i] = m_pcm[i * 2] / 32768.0f;
                buffer[bufferSize + i] = m_pcm[i * 2 + 1] / 32768.0f;
            }
            m_position += (long long)got * 4;
            if (got < frames)
                m_position = m_end;
            return got;
        }
        m_position = m_end;
        return 0;
    }

    bool hasEnded() override
    {
        return m_position >= m_end;
    }

private:
    std::ifstream m_image;
    long long m_position;
    long long m_end;
    std::vector<short> m_pcm;
};

class GogTrackSource : public SoLoud::AudioSource
{
public:
    GogTrackSource(long long start, long long end) : m_start(start), m_end(end)
    {
        mChannels = 2;
        mBaseSamplerate = 44100.0f;
    }

    ~GogTrackSource() override
    {
        stop();
    }

    SoLoud::AudioSourceInstance* createInstance() override
    {
        return new GogTrackInstance(m_start, m_end);
    }

private:
    long long m_start;
    long long m_end;
};

SoLoud::AudioSource* gogImageCreateTrack(int track)
{
    if (!gogImageAvailable())
        return nullptr;
    for (const CdTrack& cdTrack : s_tracks)
    {
        if (cdTrack.number == track && cdTrack.audio && cdTrack.endSector > cdTrack.startSector)
            return new GogTrackSource(cdTrack.startSector * kSectorSize, cdTrack.endSector * kSectorSize);
    }
    return nullptr;
}
