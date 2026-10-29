///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// GOG's CD image (see gogImage.h); the image itself is read by
// Shared/cdImage.h
///////////////////////////////////////////////////////////////////////////////

#include "gogImage.h"
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
    if (!isGogInstall())
        return;
    if (!s_image.open(std::filesystem::u8path(gogImagePath()), std::filesystem::u8path(gogCuePath())))
    {
        printf(AL_WARN "GOG image: no CD file system in %s" CON_RESET "\n", gogImagePath().c_str());
        return;
    }
    if (s_image.tracks().empty() || s_image.tracks()[0].audio)
    {
        printf(AL_WARN "GOG image: cannot read the track list %s" CON_RESET "\n", gogCuePath().c_str());
        return;
    }
    int musicTracks = 0;
    for (const CdTrack& track : s_image.tracks())
        musicTracks += track.audio ? 1 : 0;
    s_available = true;
    printf(AL_OK "GOG image: %d files and %d music tracks in %s\n", s_image.fileCount(), musicTracks,
        gogImagePath().c_str());
}

bool gogImageAvailable()
{
    std::call_once(s_openOnce, openImage);
    return s_available;
}

bool gogImageReadFile(const char* folder, const char* name, std::vector<unsigned char>& data)
{
    return gogImageAvailable() && s_image.readFile(folder, name, data);
}

// ---- Music tracks -------------------------------------------------------------

class GogTrackInstance : public SoLoud::AudioSourceInstance
{
public:
    GogTrackInstance(long long start, long long end) : m_position(start), m_end(end)
    {
        m_image.open(s_image.path(), std::ios::binary);
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
    const long long sectorSize = s_image.sectorSize();
    for (const CdTrack& cdTrack : s_image.tracks())
    {
        if (cdTrack.number == track && cdTrack.audio && cdTrack.endSector > cdTrack.startSector)
            return new GogTrackSource(cdTrack.startSector * sectorSize, cdTrack.endSector * sectorSize);
    }
    return nullptr;
}
