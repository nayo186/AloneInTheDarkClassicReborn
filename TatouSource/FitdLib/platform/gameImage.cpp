///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The game's CD image (see gameImage.h); the image itself is read by
// Shared/cdImage.h
///////////////////////////////////////////////////////////////////////////////

#include "gameImage.h"
#include "gameFolder.h"
#include "consoleLog.h"
#include "cdImage.h"

#include "soloud.h"

#include <fstream>
#include <mutex>

static std::once_flag s_openOnce;
static bool s_available = false;
static CdImage s_image;

static void openImage()
{
    if (gameImagePath().empty())
        return;
    if (!s_image.open(std::filesystem::u8path(gameImagePath()), std::filesystem::u8path(gameCuePath())))
    {
        printf(AL_WARN "CD image: no CD file system in %s" CON_RESET "\n", gameImagePath().c_str());
        return;
    }
    // GOG's always has its track list; the player's CD may have none (an
    // .iso): the game then plays without music
    if (!gameCuePath().empty() && (s_image.tracks().empty() || s_image.tracks()[0].audio))
        printf(AL_WARN "CD image: cannot read the track list %s" CON_RESET "\n", gameCuePath().c_str());
    int musicTracks = 0;
    for (const CdTrack& track : s_image.tracks())
        musicTracks += track.audio ? 1 : 0;
    s_available = true;
    printf(AL_OK "CD image: %d files and %d music tracks in %s\n", s_image.fileCount(), musicTracks,
        gameImagePath().c_str());
}

bool gameImageAvailable()
{
    std::call_once(s_openOnce, openImage);
    return s_available;
}

bool gameImageHasFile(const char* folder, const char* name)
{
    return gameImageAvailable() && s_image.hasFile(folder, name);
}

bool gameImageReadFile(const char* folder, const char* name, std::vector<unsigned char>& data)
{
    return gameImageAvailable() && s_image.readFile(folder, name, data);
}

// ---- CD tracks -------------------------------------------------------------

class CdTrackInstance : public SoLoud::AudioSourceInstance
{
public:
    CdTrackInstance(const std::filesystem::path& file, long long start, long long end) : m_position(start), m_end(end)
    {
        m_image.open(file, std::ios::binary);
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

class CdTrackSource : public SoLoud::AudioSource
{
public:
    CdTrackSource(const std::filesystem::path& file, long long start, long long end) : m_file(file), m_start(start), m_end(end)
    {
        mChannels = 2;
        mBaseSamplerate = 44100.0f;
    }

    ~CdTrackSource() override
    {
        stop();
    }

    SoLoud::AudioSourceInstance* createInstance() override
    {
        return new CdTrackInstance(m_file, m_start, m_end);
    }

private:
    std::filesystem::path m_file;   // the image, or the track's own .bin
    long long m_start;
    long long m_end;
};

SoLoud::AudioSource* createCdTrack(const CdImage& image, int track)
{
    for (const CdTrack& cdTrack : image.tracks())
    {
        if (cdTrack.number == track && cdTrack.audio && !cdTrack.file.empty() && cdTrack.end > cdTrack.start)
            return new CdTrackSource(cdTrack.file, cdTrack.start, cdTrack.end);
    }
    return nullptr;
}

SoLoud::AudioSource* gameImageCreateTrack(int track)
{
    return gameImageAvailable() ? createCdTrack(s_image, track) : nullptr;
}
