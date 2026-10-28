///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// OpenAL audio system for sound effects and music
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <assert.h>
#include <new>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include "consoleLog.h"

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#endif

//#include "OpenAL/al.h"
//#include "OpenAL/alc.h"

#include "soloud.h"
#include "soloud_file.h"
#include "soloud_wav.h"
#include "soloud_wavstream.h"
#include "soloud_bus.h"
#include <SDL.h>

#include "common.h"
#include "settings.h"
#include "audioVolume.h"
#include "gogImage.h"
#include "osystemAL.h"
#include "vocDecoder.h"
#include "jobSystem.h"

#include "zlib.h"

void osystemAL_adlibStop();

SoLoud::Soloud* gSoloud = NULL;

// Two groups with their own volume (aitd_remaster.cfg audio.*, F1 and the
// launcher): the background music, and the voices and sound effects
static SoLoud::Bus* s_musicBus = nullptr;
static SoLoud::Bus* s_audioBus = nullptr;
static SoLoud::handle s_musicBusHandle = 0;
static SoLoud::handle s_audioBusHandle = 0;

SoLoud::handle osystemAL_playMusicSource(SoLoud::AudioSource& source)
{
    return s_musicBus ? s_musicBus->play(source) : gSoloud->play(source);
}

static SoLoud::handle playAudioSource(SoLoud::AudioSource& source)
{
    return s_audioBus ? s_audioBus->play(source) : gSoloud->play(source);
}

// Music stream state (declared here so closeAudioArchive can clean them up)
SoLoud::DiskFile* pFile = NULL;
SoLoud::WavStream* pWavStream = NULL;

// A music track read from GOG's CD image (gogImage.cpp), when there are no
// track files
static SoLoud::AudioSource* s_gogTrack = nullptr;

static void stopGogTrack()
{
    if (s_gogTrack)
    {
        s_gogTrack->stop();
        delete s_gogTrack;
        s_gogTrack = nullptr;
    }
}

// Voice Over stream state
static SoLoud::WavStream* s_voStream = nullptr;
static SoLoud::DiskFile*  s_voFile   = nullptr;
static unsigned char*      s_voBuf    = nullptr;
static SoLoud::handle      s_voHandle = 0;

float g_voPitchMultiplier = 1.0f;

static void stopCDAudio();

void closeAudioArchive()
{
    // Stop CD audio if playing
    stopCDAudio();

    // Stop and clean up any active VO stream
    if (s_voStream)
    {
        if (gSoloud) gSoloud->stop(s_voHandle);
        s_voStream->stop();
        delete s_voStream;
        s_voStream = nullptr;
    }
    if (s_voFile)  { delete s_voFile;  s_voFile  = nullptr; }
    if (s_voBuf)   { delete[] s_voBuf; s_voBuf   = nullptr; }
    s_voHandle = 0;

    // Stop and clean up any active music stream before closing the archive.
    // The WavStream may be reading from s_musicArchiveBuf, so destroy it first.
    if (pWavStream)
    {
        pWavStream->stop();
        delete pWavStream;
        pWavStream = nullptr;
    }
    stopGogTrack();
    if (pFile)
    {
        delete pFile;
        pFile = nullptr;
    }
}

// 
#ifdef _WIN32
static bool  s_cdDetected    = false;
static bool  s_cdDetectDone  = false;
static char  s_cdDriveLetter = '\0';
static bool  s_cdPlaying     = false;

// Scan all CD/DVD drives for a volume labelled "ALONECD".
static void detectAloneCD()
{
    if (s_cdDetectDone) return;
    s_cdDetectDone = true;

    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++)
    {
        if (!(drives & (1u << i))) continue;

        char root[] = { (char)('A' + i), ':', '\\', '\0' };
        if (GetDriveTypeA(root) != DRIVE_CDROM) continue;

        char volName[MAX_PATH + 1] = {};
        if (GetVolumeInformationA(root, volName, sizeof(volName),
                                   NULL, NULL, NULL, NULL, 0))
        {
            if (_stricmp(volName, "ALONECD") == 0)
            {
                s_cdDriveLetter = (char)('A' + i);
                s_cdDetected = true;
                printf(AL_OK "Detected AITD1 CD (ALONECD) on drive %c:\\\n", s_cdDriveLetter);
                return;
            }
        }
    }
}

static void stopCDAudio()
{
    if (s_cdPlaying)
    {
        mciSendStringA("stop cdaudio", NULL, 0, NULL);
        mciSendStringA("close cdaudio", NULL, 0, NULL);
        s_cdPlaying = false;
    }
}

// Play a CD audio track via MCI.  Returns true on success.
static bool playCDAudioTrack(int trackId)
{
    detectAloneCD();
    if (!s_cdDetected) return false;

    stopCDAudio();

    // Open the CD-ROM drive
    char cmd[256];
    char ret[128];
    sprintf_s(cmd, sizeof(cmd), "open %c: type cdaudio alias cdaudio", s_cdDriveLetter);
    if (mciSendStringA(cmd, NULL, 0, NULL) != 0)
        return false;

    // Set time format to track/minute/second/frame
    if (mciSendStringA("set cdaudio time format tmsf", NULL, 0, NULL) != 0)
    {
        mciSendStringA("close cdaudio", NULL, 0, NULL);
        return false;
    }

    // Verify the track exists by querying the number of tracks
    if (mciSendStringA("status cdaudio number of tracks", ret, sizeof(ret), NULL) != 0)
    {
        mciSendStringA("close cdaudio", NULL, 0, NULL);
        return false;
    }
    int numTracks = atoi(ret);
    if (trackId < 1 || trackId > numTracks)
    {
        mciSendStringA("close cdaudio", NULL, 0, NULL);
        return false;
    }

    // Play the requested track (to the start of the next track)
    if (trackId < numTracks)
        sprintf_s(cmd, sizeof(cmd), "play cdaudio from %d to %d", trackId, trackId + 1);
    else
        sprintf_s(cmd, sizeof(cmd), "play cdaudio from %d", trackId);

    if (mciSendStringA(cmd, NULL, 0, NULL) != 0)
    {
        mciSendStringA("close cdaudio", NULL, 0, NULL);
        return false;
    }

    s_cdPlaying = true;
    printf(AL_TAG "Playing CD audio track %d from drive %c:\\\n", trackId, s_cdDriveLetter);
    return true;
}
#else
static void stopCDAudio() {}
static bool playCDAudioTrack(int) { return false; }
#endif

void osystem_stopTrack()
{
    if (gSoloud)
    {
        if (pWavStream)
        {
            pWavStream->stop();
            delete pWavStream;
            pWavStream = nullptr;
        }
        stopGogTrack();

        if (pFile)
        {
            delete pFile;
            pFile = nullptr;
        }

    }

    stopCDAudio();
}

void osystemAL_init()
{
    gSoloud = new SoLoud::Soloud();
    gSoloud->init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::AUTO, SoLoud::Soloud::AUTO, SoLoud::Soloud::AUTO, 2);
    s_musicBus = new SoLoud::Bus();
    s_audioBus = new SoLoud::Bus();
    s_musicBusHandle = gSoloud->play(*s_musicBus);
    s_audioBusHandle = gSoloud->play(*s_audioBus);

    // The mixer and the sound device may differ: SDL converts between them
    // (BUGS.md C17)
    SDL_AudioSpec device{};
    SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &device, nullptr);
    printf(AL_TAG "Audio output: %s, mixer %u Hz %u channels, device %d Hz %d channels\n",
           gSoloud->getBackendString() ? gSoloud->getBackendString() : "none",
           gSoloud->getBackendSamplerate(), gSoloud->getBackendChannels(),
           device.freq, device.channels);
}

void osystemAL_deinit()
{
    // Stop VO playback
    if (s_voStream)
    {
        if (gSoloud) gSoloud->stop(s_voHandle);
        s_voStream->stop();
        delete s_voStream;
        s_voStream = nullptr;
    }
    if (s_voFile)  { delete s_voFile;  s_voFile  = nullptr; }
    if (s_voBuf)   { delete[] s_voBuf; s_voBuf   = nullptr; }
    s_voHandle = 0;

    stopCDAudio();
    osystemAL_adlibStop();

    if (gSoloud)
    {
        gSoloud->stop(s_musicBusHandle);
        gSoloud->stop(s_audioBusHandle);
        delete s_musicBus;
        delete s_audioBus;
        s_musicBus = s_audioBus = nullptr;
        gSoloud->deinit();
        delete gSoloud;
        gSoloud = nullptr;
    }
}

class ITD_AudioSource : public SoLoud::AudioSource
{
public:
    char* m_samples;
    char* m_ownedBuf;
    int m_size;

    ITD_AudioSource(char* samplePtr, int size);
    virtual ~ITD_AudioSource();
    virtual SoLoud::AudioSourceInstance *createInstance();
};

class ITD_AudioInstance : public SoLoud::AudioSourceInstance
{
    ITD_AudioSource *mParent;
    unsigned int mOffset;
public:
    ITD_AudioInstance(ITD_AudioSource *aParent) : SoLoud::AudioSourceInstance()
    {
        mOffset = 0;
        mParent = aParent;
    }

    virtual unsigned int getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize) override
    {
        if (mParent->m_size < 0 || (int)mOffset > mParent->m_size)
        {
            return 0;
        }

        unsigned int samplesWritten = 0;
        for (unsigned int i = 0; i < aSamplesToRead; i++)
        {
            if ((int)mOffset > mParent->m_size)
            {
                break;
            }
            aBuffer[i] = (((float)((unsigned char*)mParent->m_samples)[mOffset]) - 128.f) / 128.f;
            mOffset++;
            samplesWritten++;
        }
        return samplesWritten;
    }

    virtual bool hasEnded()
    {
        if (mParent->m_size < 0 || (int)mOffset > mParent->m_size)
        {
            return true;
        }

        return false;
    }
};

ITD_AudioSource::ITD_AudioSource(char* samplePtr, int size) : SoLoud::AudioSource()
{
    assert(samplePtr[26] == 1); //assert first block is of sound data type
    int sampleSize = (READ_LE_U32(samplePtr + 26) >> 8) - 2;

    int frequencyDiv = *(unsigned char*)(samplePtr + 30);
    //    int codecId = samplePtr[31];

    char* sampleData = samplePtr + 32;

    int sampleRate = 1000000 / (256 - frequencyDiv);

    // Copy sample data into an owned buffer so we don't hold a dangling
    // pointer into HQR cache memory that may be evicted during playback.
    m_ownedBuf = new char[sampleSize];
    memcpy(m_ownedBuf, sampleData, sampleSize);
    m_samples = m_ownedBuf;
    m_size = sampleSize-1;

    mBaseSamplerate = static_cast<float>(sampleRate);
}

ITD_AudioSource::~ITD_AudioSource()
{
    delete[] m_ownedBuf;
}

SoLoud::AudioSourceInstance* ITD_AudioSource::createInstance()
{
   return new ITD_AudioInstance(this);
}

// Track active SFX handles and sources so multiple sounds can overlap
struct ActiveSfx
{
    SoLoud::handle handle;
    SoLoud::AudioSource* source;
    bool looping;
};
static std::vector<ActiveSfx> g_activeSfx;

// Remove finished sounds from the active list and free their AudioSource memory.
static void reapFinishedSfx()
{
    if (!gSoloud) return;
    for (size_t i = 0; i < g_activeSfx.size(); )
    {
        if (!gSoloud->isValidVoiceHandle(g_activeSfx[i].handle))
        {
            delete g_activeSfx[i].source;
            g_activeSfx.erase(g_activeSfx.begin() + i);
        }
        else
        {
            i++;
        }
    }
}

void osystem_playSample(char* samplePtr,int size)
{
    if (!gSoloud || !samplePtr || size <= 0)
        return;

    // Clean up finished sounds to avoid unbounded growth.
    // Unlike the original single-channel DOS behavior, we allow multiple
    // one-shot SFX to overlap so that e.g. combat hits and ambient sounds
    // don't cut each other out.
    reapFinishedSfx();

    // Check if the sample data is in VOC format (LISTSAMP entries for AITD-era games are VOC)
    bool isVoc = (size >= 20 && memcmp(samplePtr, "Creative Voice File\x1a", 20) == 0);

    SoLoud::AudioSource* pAudioSource = nullptr;
    SoLoud::handle h = 0;

    if (isVoc)
    {
        // Use the robust VOC decoder to convert to WAV, then play via SoLoud
        unsigned char* wavBuf = nullptr;
        size_t wavSize = 0;
        if (!vocDecodeToWav((const unsigned char*)samplePtr, (size_t)size, &wavBuf, &wavSize))
        {
            printf(VO_WARN "Failed to decode VOC sample (%d bytes)" CON_RESET "\n", size);
            return;
        }

        SoLoud::Wav* pWav = new SoLoud::Wav();
        SoLoud::result res = pWav->loadMem(wavBuf, (unsigned int)wavSize, true);
        delete[] wavBuf;
        if (res != SoLoud::SO_NO_ERROR)
        {
            delete pWav;
            return;
        }
        pWav->setLooping(false);
        h = playAudioSource(*pWav);
        pAudioSource = pWav;
    }
    else if (g_gameId >= TIMEGATE)
    {
        SoLoud::Wav* pWav = new SoLoud::Wav();
        SoLoud::result res = pWav->loadMem((u8*)samplePtr, size, true);
        if (res != SoLoud::SO_NO_ERROR)
        {
            delete pWav;
            return;
        }
        pWav->setLooping(false);
        h = playAudioSource(*pWav);
        pAudioSource = pWav;
    }
    else
    {
        // Legacy fallback for non-VOC sample data
        ITD_AudioSource* pItd = new ITD_AudioSource(samplePtr, size);
        pItd->setLooping(false);
        h = playAudioSource(*pItd);
        pAudioSource = pItd;
    }

    if (pAudioSource)
    {
        ActiveSfx sfx = { h, pAudioSource, false };
        g_activeSfx.push_back(sfx);

        // Apply random frequency modulation if set (rndFreqModulation is 0-100)
        extern s16 rndFreqModulation;
        if (rndFreqModulation > 0 && h != 0)
        {
            // Random variation: ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â±rndFreqModulation% of pitch
            // rndFreqModulation of 40 means ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â±40% variation
            float variation = (float)(rand() % (rndFreqModulation * 2 + 1) - rndFreqModulation) / 100.0f;
            float playSpeed = 1.0f + variation;

            // Clamp to reasonable range (0.5x to 2.0x)
            if (playSpeed < 0.5f) playSpeed = 0.5f;
            if (playSpeed > 2.0f) playSpeed = 2.0f;

            gSoloud->setRelativePlaySpeed(h, playSpeed);
        }
    }
}

void osystem_stopSample()
{
    if (!gSoloud) return;
    for (auto& sfx : g_activeSfx)
    {
        gSoloud->stop(sfx.handle);
        delete sfx.source;
    }
    g_activeSfx.clear();
}

// Stop only the looping samples (one-shot sounds keep playing)
void osystem_stopLoopingSamples()
{
    if (!gSoloud) return;
    for (size_t i = 0; i < g_activeSfx.size(); )
    {
        if (g_activeSfx[i].looping)
        {
            gSoloud->stop(g_activeSfx[i].handle);
            delete g_activeSfx[i].source;
            g_activeSfx.erase(g_activeSfx.begin() + i);
        }
        else
        {
            i++;
        }
    }
}

bool osystem_isSamplePlaying()
{
    if (!gSoloud) return false;
    for (auto& sfx : g_activeSfx)
    {
        if (gSoloud->isValidVoiceHandle(sfx.handle))
            return true;
    }
    return false;
}

void osystem_playLoopingSample(char* samplePtr, int size)
{
    if (!gSoloud || !samplePtr || size <= 0)
        return;

    // Stop previous looping sounds (only one ambient loop at a time)
    // but let one-shot SFX keep playing.
    for (size_t i = 0; i < g_activeSfx.size(); )
    {
        if (g_activeSfx[i].looping)
        {
            gSoloud->stop(g_activeSfx[i].handle);
            delete g_activeSfx[i].source;
            g_activeSfx.erase(g_activeSfx.begin() + i);
        }
        else
        {
            i++;
        }
    }
    reapFinishedSfx();

    // Check if the sample data is in VOC format
    bool isVoc = (size >= 20 && memcmp(samplePtr, "Creative Voice File\x1a", 20) == 0);

    SoLoud::Wav* pWav = new SoLoud::Wav();
    SoLoud::handle h = 0;

    if (isVoc)
    {
        unsigned char* wavBuf = nullptr;
        size_t wavSize = 0;
        if (!vocDecodeToWav((const unsigned char*)samplePtr, (size_t)size, &wavBuf, &wavSize))
        {
            printf(VO_WARN "Failed to decode VOC sample for looping (%d bytes)" CON_RESET "\n", size);
            delete pWav;
            return;
        }

        SoLoud::result res = pWav->loadMem(wavBuf, (unsigned int)wavSize, true);
        delete[] wavBuf;
        if (res != SoLoud::SO_NO_ERROR)
        {
            delete pWav;
            return;
        }
    }
    else
    {
        SoLoud::result res = pWav->loadMem((u8*)samplePtr, size, true);
        if (res != SoLoud::SO_NO_ERROR)
        {
            delete pWav;
            return;
        }
    }

    pWav->setLooping(true);
    h = playAudioSource(*pWav);

    ActiveSfx sfx = { h, pWav, true };
    g_activeSfx.push_back(sfx);
}
extern float gVolume;

// SoLoud places every sound in the centre at about 71% per speaker (-3 dB);
// DOSBox sends it whole to both. This brings back the GOG level for music,
// effects and voices (the round-off clipping keeps loud peaks soft)
static const float kCentreCompensation = 1.41421356f;

void osystemAL_udpate()
{
    if (gSoloud)
    {
        gSoloud->setGlobalVolume(gVolume * kCentreCompensation);
        gSoloud->setVolume(s_musicBusHandle, audioVolumeGain(g_settings.audio.musicVolume));
        gSoloud->setVolume(s_audioBusHandle, audioVolumeGain(g_settings.audio.soundVolume));
    }
}

int osystemAL_getSampleRate()
{
    if (gSoloud)
        return (int)gSoloud->getBackendSamplerate();
    return 44100;
}


int osystem_playTrack(int trackId)
{
    if (!gSoloud)
        return 0;

    // Delete the previous WavStream BEFORE freeing the archive buffer.
    // WavStream::loadMem with aCopy=false means SoLoud's audio mixing thread
    // reads directly from s_musicArchiveBuf. We must fully destroy the stream
    // (which stops the audio thread's access) before freeing the buffer,
    // otherwise we get a use-after-free access violation.
    if (pWavStream)
    {
        pWavStream->stop();
        delete pWavStream;
        pWavStream = nullptr;
    }
    stopGogTrack();

    // Clean up previous DiskFile (used by filesystem fallback path)
    if (pFile)
    {
        delete pFile;
        pFile = nullptr;
    }

    // Now safe to free the archive buffer - no audio thread references remain
    // Stop any active CD audio before switching tracks
    stopCDAudio();

    // Try CD audio first (ALONECD in drive)
    if (playCDAudioTrack(trackId))
        return 1;

    // MP3/audio files are numbered from 01 for the first audio track, but trackId
    // uses CD track numbers where track 1 is the data track and audio starts at 2.
    // Subtract 1 so CD track 2 (first audio) maps to "01.mp3", etc.
    int fileTrackId = trackId - 1;
    if (fileTrackId < 1)
        return 0;

    // Filesystem fallback
    char filename[256];
    FILE* fHandle = NULL;

    // Try OGG first  
    sprintf_s(filename, sizeof(filename), "%02d.ogg", fileTrackId);
    fopen_s(&fHandle, filename, "rb");

    // Fall back to WAV if OGG not found
    if (fHandle == NULL)
    {
        sprintf_s(filename, sizeof(filename), "%02d.wav", fileTrackId);
        fopen_s(&fHandle, filename, "rb");
    }

    // Fall back to MP3 if WAV not found
    if (fHandle == NULL)
    {
        sprintf_s(filename, sizeof(filename), "%02d.mp3", fileTrackId);
        fopen_s(&fHandle, filename, "rb");
    }
    
    if (fHandle == NULL)
    {
        // GOG installation: the track straight from GAME.GOG
        s_gogTrack = gogImageCreateTrack(trackId);
        if (!s_gogTrack)
            return 0;
        osystemAL_playMusicSource(*s_gogTrack);
        printf(AL_TAG "Music: CD track %d from the GOG image\n", trackId);
        return 1;
    }

    pFile = new SoLoud::DiskFile(fHandle);
    pWavStream = new SoLoud::WavStream();
    SoLoud::result res = pWavStream->loadFile(pFile);
    if (res == SoLoud::SO_NO_ERROR)
    {
        osystemAL_playMusicSource(*pWavStream);
        return 1;
    }
    else
    {
        delete pWavStream;
        pWavStream = nullptr;
        delete pFile;
        pFile = nullptr;
    }

    return 0;
}

// ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ Voice Over (VO) playback ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚ÂÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬

void osystem_stopVO()
{
    if (!gSoloud) return;

    if (s_voHandle != 0)
    {
        gSoloud->stop(s_voHandle);
        s_voHandle = 0;
    }
    if (s_voStream) { s_voStream->stop(); delete s_voStream; s_voStream = nullptr; }
    if (s_voFile)   { delete s_voFile;   s_voFile   = nullptr; }
    if (s_voBuf)    { delete[] s_voBuf;  s_voBuf    = nullptr; }
}

// Try to decode a VOC buffer and play it.  Returns true on success.
static bool tryPlayVocFromMemory(const unsigned char* data, size_t dataSize, const char* label)
{
    unsigned char* wavBuf = nullptr;
    size_t wavSize = 0;
    if (!vocDecodeToWav(data, dataSize, &wavBuf, &wavSize))
        return false;

    s_voStream = new SoLoud::WavStream();
    SoLoud::result res = s_voStream->loadMem(wavBuf, (unsigned int)wavSize, false, false);
    if (res == SoLoud::SO_NO_ERROR)
    {
        s_voBuf = wavBuf;
        s_voHandle = playAudioSource(*s_voStream);
        printf(VO_OK "Playing VO: %s (VOC decoded)" CON_RESET "\n", label);
        return true;
    }

    delete[] wavBuf;
    delete s_voStream;
    s_voStream = nullptr;
    return false;
}

// Check whether a path ends with a given extension (case-insensitive).
static bool hasExtCI(const char* path, const char* ext)
{
    size_t pl = strlen(path);
    size_t el = strlen(ext);
    if (el == 0 || pl < el) return false;
    return _stricmp(path + pl - el, ext) == 0;
}

// Try to open a file by path, decode VOC if needed, and play it.
// Returns true on success.
static bool tryPlayVoFile(const char* path, const char* label)
{
    FILE* fh = nullptr;
    fopen_s(&fh, path, "rb");
    if (!fh) return false;

    // VOC files need special decoding ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â read fully into memory first
    if (hasExtCI(path, ".voc"))
    {
        fseek(fh, 0, SEEK_END);
        long fileLen = ftell(fh);
        fseek(fh, 0, SEEK_SET);
        if (fileLen <= 0) { fclose(fh); return false; }

        unsigned char* raw = new (std::nothrow) unsigned char[(size_t)fileLen];
        if (!raw) { fclose(fh); return false; }
        if (fread(raw, 1, (size_t)fileLen, fh) != (size_t)fileLen)
        {
            delete[] raw;
            fclose(fh);
            return false;
        }
        fclose(fh);

        bool ok = tryPlayVocFromMemory(raw, (size_t)fileLen, label);
        delete[] raw;
        return ok;
    }

    // Standard audio formats ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â let SoLoud handle directly
    s_voFile   = new SoLoud::DiskFile(fh);
    s_voStream = new SoLoud::WavStream();
    SoLoud::result res = s_voStream->loadFile(s_voFile);
    if (res == SoLoud::SO_NO_ERROR)
    {
        s_voHandle = playAudioSource(*s_voStream);
        printf(VO_OK "Playing VO: %s" CON_RESET "\n", label);
        return true;
    }

    delete s_voStream; s_voStream = nullptr;
    delete s_voFile;   s_voFile   = nullptr;
    return false;
}

void osystem_playVocByIndex(int vocIndex)
{
    if (!gSoloud || vocIndex < 0)
        return;

    char vocName[64];
    sprintf_s(vocName, sizeof(vocName), "%06d.VOC", vocIndex);
    printf(VO_TAG "Playing VOC by index: %d -> %s\n", vocIndex, vocName);
    osystem_playVO(vocName);
}

// A voice from GOG's CD image, with or without its .VOC extension; the buffer
// is the caller's (delete[])
static unsigned char* loadVocFromGogImage(const char* vocName, size_t* outSize)
{
    std::vector<unsigned char> data;
    std::string name = vocName;
    if (!gogImageReadFile("INDARK", name.c_str(), data))
    {
        name += ".VOC";
        if (!gogImageReadFile("INDARK", name.c_str(), data))
            return nullptr;
    }
    unsigned char* raw = new (std::nothrow) unsigned char[data.size()];
    if (!raw)
        return nullptr;
    memcpy(raw, data.data(), data.size());
    *outSize = data.size();
    return raw;
}

// Helper: try to load raw file data for a VOC name from archive, CD, or filesystem.
// Returns allocated buffer (caller must delete[]) and size, or nullptr on failure.
static unsigned char* loadVocFileData(const char* vocName, size_t* outSize)
{
    *outSize = 0;

    // Try CD path
#ifdef _WIN32
    detectAloneCD();
    if (s_cdDetected)
    {
        const char* exts[] = { "", ".voc" };
        for (int i = 0; i < 2; i++)
        {
            char cdPath[512];
            sprintf_s(cdPath, sizeof(cdPath), "%c:\\INDARK\\%s%s", s_cdDriveLetter, vocName, exts[i]);
            FILE* fh = nullptr;
            fopen_s(&fh, cdPath, "rb");
            if (!fh) continue;

            fseek(fh, 0, SEEK_END);
            long fileLen = ftell(fh);
            fseek(fh, 0, SEEK_SET);
            if (fileLen <= 0) { fclose(fh); continue; }

            unsigned char* raw = new (std::nothrow) unsigned char[(size_t)fileLen];
            if (!raw) { fclose(fh); continue; }
            if (fread(raw, 1, (size_t)fileLen, fh) != (size_t)fileLen)
            {
                delete[] raw;
                fclose(fh);
                continue;
            }
            fclose(fh);
            *outSize = (size_t)fileLen;
            return raw;
        }
    }
#endif

    // Filesystem fallback
    {
        const char* exts[] = { "", ".voc" };
        for (int i = 0; i < 2; i++)
        {
            char path[512];
            sprintf_s(path, sizeof(path), "%s%s", vocName, exts[i]);
            FILE* fh = nullptr;
            fopen_s(&fh, path, "rb");
            if (!fh) continue;

            fseek(fh, 0, SEEK_END);
            long fileLen = ftell(fh);
            fseek(fh, 0, SEEK_SET);
            if (fileLen <= 0) { fclose(fh); continue; }

            unsigned char* raw = new (std::nothrow) unsigned char[(size_t)fileLen];
            if (!raw) { fclose(fh); continue; }
            if (fread(raw, 1, (size_t)fileLen, fh) != (size_t)fileLen)
            {
                delete[] raw;
                fclose(fh);
                continue;
            }
            fclose(fh);
            *outSize = (size_t)fileLen;
            return raw;
        }
    }

    // GOG installation: the voices are in GAME.GOG (INDARK folder of the CD)
    if (unsigned char* raw = loadVocFromGogImage(vocName, outSize))
        return raw;

    return nullptr;
}

// VOC page naming: BBSSLL.VOC where BB=vocIndex, SS=page, LL=line
// Numeric value = vocIndex * 10000 + page * 100 + line
void osystem_playVocPageLines(int vocIndex, int page, int numLines)
{
    if (!gSoloud || vocIndex < 0 || numLines <= 0)
        return;

    osystem_stopVO();

    // CACHE CHECK: Try pre-decoded page from cache first
    size_t cachedWavSize = 0;
    unsigned char* cachedWav = osystem_tryGetPreDecodedVocPage(vocIndex, page, &cachedWavSize);
    if (cachedWav && cachedWavSize > 0)
    {
        s_voStream = new SoLoud::WavStream();
        SoLoud::result res = s_voStream->loadMem(cachedWav, static_cast<unsigned int>(cachedWavSize), false, false);
        if (res == SoLoud::SO_NO_ERROR)
        {
            s_voHandle = playAudioSource(*s_voStream);
            if (g_voPitchMultiplier != 1.0f)
                gSoloud->setRelativePlaySpeed(s_voHandle, g_voPitchMultiplier);
            printf(VO_OK "Playing VOC page from cache: book %d page %d" CON_RESET "\n",
                   vocIndex, page);
            return;
        }
        else
        {
            delete s_voStream;
            s_voStream = nullptr;
        }
    }

    // CACHE MISS: Decode from scratch
    // Collect decoded PCM data from all line VOCs for this page
    std::vector<unsigned char> combinedPcm;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint16_t numChannels = 0;
    bool gotFormat = false;

    for (int line = 0; line < numLines; line++)
    {
        int vocNum = vocIndex * 10000 + page * 100 + line;
        char vocName[64];
        sprintf_s(vocName, sizeof(vocName), "%06d.VOC", vocNum);

        size_t rawSize = 0;
        unsigned char* rawData = loadVocFileData(vocName, &rawSize);
        if (!rawData)
            break; // stop at first missing line

        // Decode VOC to WAV
        unsigned char* wavBuf = nullptr;
        size_t wavSize = 0;
        if (!vocDecodeToWav(rawData, rawSize, &wavBuf, &wavSize))
        {
            delete[] rawData;
            break;
        }
        delete[] rawData;

        // Extract format info from first decoded WAV header (44 bytes)
        if (!gotFormat && wavSize >= 44)
        {
            sampleRate    = (uint32_t)wavBuf[24] | ((uint32_t)wavBuf[25] << 8) |
                            ((uint32_t)wavBuf[26] << 16) | ((uint32_t)wavBuf[27] << 24);
            numChannels   = (uint16_t)wavBuf[22] | ((uint16_t)wavBuf[23] << 8);
            bitsPerSample = (uint16_t)wavBuf[34] | ((uint16_t)wavBuf[35] << 8);
            gotFormat = true;
        }

        // Append PCM data (skip 44-byte WAV header)
        if (wavSize > 44)
            combinedPcm.insert(combinedPcm.end(), wavBuf + 44, wavBuf + wavSize);

        delete[] wavBuf;
    }

    if (!gotFormat || combinedPcm.empty())
    {
        printf(VO_WARN "No VOC lines found for book %d page %d" CON_RESET "\n", vocIndex, page);
        return;
    }

    // Build combined WAV in memory
    uint32_t dataSize   = (uint32_t)combinedPcm.size();
    uint32_t wavSize    = 44 + dataSize;
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    uint32_t byteRate   = sampleRate * blockAlign;

    unsigned char* wav = new (std::nothrow) unsigned char[wavSize];
    if (!wav)
        return;

    // RIFF header
    memcpy(wav, "RIFF", 4);
    wav[4] = (unsigned char)((wavSize - 8) & 0xFF);
    wav[5] = (unsigned char)(((wavSize - 8) >> 8) & 0xFF);
    wav[6] = (unsigned char)(((wavSize - 8) >> 16) & 0xFF);
    wav[7] = (unsigned char)(((wavSize - 8) >> 24) & 0xFF);
    memcpy(wav + 8, "WAVE", 4);

    // fmt chunk
    memcpy(wav + 12, "fmt ", 4);
    wav[16] = 16; wav[17] = 0; wav[18] = 0; wav[19] = 0; // chunk size = 16
    wav[20] = 1;  wav[21] = 0; // PCM format
    wav[22] = (unsigned char)(numChannels & 0xFF);
    wav[23] = (unsigned char)((numChannels >> 8) & 0xFF);
    wav[24] = (unsigned char)(sampleRate & 0xFF);
    wav[25] = (unsigned char)((sampleRate >> 8) & 0xFF);
    wav[26] = (unsigned char)((sampleRate >> 16) & 0xFF);
    wav[27] = (unsigned char)((sampleRate >> 24) & 0xFF);
    wav[28] = (unsigned char)(byteRate & 0xFF);
    wav[29] = (unsigned char)((byteRate >> 8) & 0xFF);
    wav[30] = (unsigned char)((byteRate >> 16) & 0xFF);
    wav[31] = (unsigned char)((byteRate >> 24) & 0xFF);
    wav[32] = (unsigned char)(blockAlign & 0xFF);
    wav[33] = (unsigned char)((blockAlign >> 8) & 0xFF);
    wav[34] = (unsigned char)(bitsPerSample & 0xFF);
    wav[35] = (unsigned char)((bitsPerSample >> 8) & 0xFF);

    // data chunk
    memcpy(wav + 36, "data", 4);
    wav[40] = (unsigned char)(dataSize & 0xFF);
    wav[41] = (unsigned char)((dataSize >> 8) & 0xFF);
    wav[42] = (unsigned char)((dataSize >> 16) & 0xFF);
    wav[43] = (unsigned char)((dataSize >> 24) & 0xFF);
    memcpy(wav + 44, combinedPcm.data(), dataSize);

    // Play the combined WAV
    s_voStream = new SoLoud::WavStream();
    SoLoud::result res = s_voStream->loadMem(wav, wavSize, false, false);
    if (res == SoLoud::SO_NO_ERROR)
    {
        s_voBuf = wav;
        s_voHandle = playAudioSource(*s_voStream);
        if (g_voPitchMultiplier != 1.0f)
            gSoloud->setRelativePlaySpeed(s_voHandle, g_voPitchMultiplier);
        printf(VO_OK "Playing VOC page: book %d page %d (%d lines, %u bytes)" CON_RESET "\n",
               vocIndex, page, numLines, dataSize);
    }
    else
    {
        delete[] wav;
        delete s_voStream;
        s_voStream = nullptr;
        printf(VO_WARN "Failed to play combined VOC for book %d page %d" CON_RESET "\n",
               vocIndex, page);
    }
}

void osystem_playVO(const char* voFileName)
{
    if (!gSoloud || !voFileName || !voFileName[0])
        return;

    // Stop any currently playing VO
    osystem_stopVO();

    // Extension list for filesystem searches (includes .voc)
    const char* exts[] = { "", ".voc", ".ogg", ".wav", ".mp3" };

    // Try ALONECD INDARK subfolder (e.g. D:\INDARK\voFileName)
#ifdef _WIN32
    detectAloneCD();
    if (s_cdDetected)
    {
        for (int i = 0; i < 5; i++)
        {
            char cdPath[512];
            sprintf_s(cdPath, sizeof(cdPath), "%c:\\INDARK\\%s%s", s_cdDriveLetter, voFileName, exts[i]);
            if (tryPlayVoFile(cdPath, cdPath))
                return;
        }
    }
#endif

    // Filesystem fallback
    for (int i = 0; i < 5; i++)
    {
        char path[512];
        sprintf_s(path, sizeof(path), "%s%s", voFileName, exts[i]);
        if (tryPlayVoFile(path, path))
            return;
    }

    // GOG installation: the voices are in GAME.GOG (INDARK folder of the CD)
    size_t gogSize = 0;
    if (unsigned char* raw = loadVocFromGogImage(voFileName, &gogSize))
    {
        const bool ok = tryPlayVocFromMemory(raw, gogSize, voFileName);
        delete[] raw;
        if (ok)
        {
            printf(VO_OK "Playing VO: %s (from the GOG image)" CON_RESET "\n", voFileName);
            return;
        }
    }

    printf(VO_WARN "VO file not found: %s" CON_RESET "\n", voFileName);
}

static SoLoud::Wav* g_lastNamedSfx = nullptr;
static SoLoud::handle g_lastNamedSfxHandle = 0;

void osystem_playSampleFromName(char* sampleName)
{
    if (!gSoloud || !soundEnabled)
        return;

    // Stop and free previous named sample
    if (g_lastNamedSfxHandle != 0)
    {
        gSoloud->stop(g_lastNamedSfxHandle);
        g_lastNamedSfxHandle = 0;
    }
    if (g_lastNamedSfx)
    {
        delete g_lastNamedSfx;
        g_lastNamedSfx = nullptr;
    }

    // Filesystem fallback
    SoLoud::Wav* pWav = new SoLoud::Wav();
    SoLoud::result result = pWav->load(sampleName);

    if (result == SoLoud::SO_NO_ERROR)
    {
        g_lastNamedSfxHandle = playAudioSource(*pWav);
        g_lastNamedSfx = pWav;
    }
    else
    {
        delete pWav;
    }
}
///////////////////////////////////////////////////////////////////////////////
// VOC Pre-Decode Cache for Book Reading (Phase 3)
///////////////////////////////////////////////////////////////////////////////

struct PreDecodedVocPage
{
    unsigned char* wavData = nullptr;
    size_t wavSize = 0;
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;

    ~PreDecodedVocPage()
    {
        if (wavData)
        {
            delete[] wavData;
            wavData = nullptr;
        }
    }
};

static std::unordered_map<uint32_t, std::shared_ptr<PreDecodedVocPage>> s_vocPageCache;
static std::mutex s_vocCacheMutex;

static uint32_t makeVocCacheKey(int vocIndex, int page)
{
    return (uint32_t)(vocIndex * 10000 + page);
}

void osystem_preDecodeVocPage(int vocIndex, int page)
{
    if (!JobSystem::instance().isInitialized())
        return;

    uint32_t cacheKey = makeVocCacheKey(vocIndex, page);

    {
        std::lock_guard<std::mutex> lock(s_vocCacheMutex);
        if (s_vocPageCache.count(cacheKey))
            return;
    }

    auto cacheEntry = std::make_shared<PreDecodedVocPage>();

    {
        std::lock_guard<std::mutex> lock(s_vocCacheMutex);
        s_vocPageCache[cacheKey] = cacheEntry;
    }

    int numMaxLines = 20;
    JobSystem::instance().scheduleJob(
        JobSystem::JobType::DECODE_VOC,
        JobSystem::Priority::HIGH,
        [vocIndex, page, cacheEntry, numMaxLines]() {
            std::vector<unsigned char> combinedPcm;
            uint32_t sampleRate = 0;
            uint16_t bitsPerSample = 0;
            uint16_t numChannels = 0;
            bool gotFormat = false;

            for (int line = 0; line < numMaxLines; line++)
            {
                int vocNum = vocIndex * 10000 + page * 100 + line;
                char vocName[64];
                sprintf_s(vocName, sizeof(vocName), "%06d.VOC", vocNum);

                        size_t rawSize = 0;
                        unsigned char* rawData = loadVocFileData(vocName, &rawSize);
                        if (!rawData)
                            break;

                unsigned char* wavBuf = nullptr;
                size_t wavSz = 0;
                if (!vocDecodeToWav(rawData, rawSize, &wavBuf, &wavSz))
                {
                    delete[] rawData;
                    break;
                }
                delete[] rawData;

                if (!gotFormat && wavSz >= 44)
                {
                    sampleRate    = (uint32_t)wavBuf[24] | ((uint32_t)wavBuf[25] << 8) |
                                    ((uint32_t)wavBuf[26] << 16) | ((uint32_t)wavBuf[27] << 24);
                    numChannels   = (uint16_t)wavBuf[22] | ((uint16_t)wavBuf[23] << 8);
                    bitsPerSample = (uint16_t)wavBuf[34] | ((uint16_t)wavBuf[35] << 8);
                    gotFormat = true;
                }

                if (wavSz > 44)
                    combinedPcm.insert(combinedPcm.end(), wavBuf + 44, wavBuf + wavSz);

                delete[] wavBuf;
            }

            if (!gotFormat || combinedPcm.empty())
                return;

            uint32_t dataSize   = (uint32_t)combinedPcm.size();
            uint32_t totalWavSz = 44 + dataSize;
            uint16_t blockAlign = numChannels * (bitsPerSample / 8);
            uint32_t byteRate   = sampleRate * blockAlign;

            unsigned char* wav = new (std::nothrow) unsigned char[totalWavSz];
            if (!wav)
                return;

            memcpy(wav, "RIFF", 4);
            wav[4] = (unsigned char)((totalWavSz - 8) & 0xFF);
            wav[5] = (unsigned char)(((totalWavSz - 8) >> 8) & 0xFF);
            wav[6] = (unsigned char)(((totalWavSz - 8) >> 16) & 0xFF);
            wav[7] = (unsigned char)(((totalWavSz - 8) >> 24) & 0xFF);
            memcpy(wav + 8, "WAVE", 4);
            memcpy(wav + 12, "fmt ", 4);
            wav[16] = 16; wav[17] = 0; wav[18] = 0; wav[19] = 0;
            wav[20] = 1;  wav[21] = 0;
            wav[22] = (unsigned char)(numChannels & 0xFF);
            wav[23] = (unsigned char)((numChannels >> 8) & 0xFF);
            wav[24] = (unsigned char)(sampleRate & 0xFF);
            wav[25] = (unsigned char)((sampleRate >> 8) & 0xFF);
            wav[26] = (unsigned char)((sampleRate >> 16) & 0xFF);
            wav[27] = (unsigned char)((sampleRate >> 24) & 0xFF);
            wav[28] = (unsigned char)(byteRate & 0xFF);
            wav[29] = (unsigned char)((byteRate >> 8) & 0xFF);
            wav[30] = (unsigned char)((byteRate >> 16) & 0xFF);
            wav[31] = (unsigned char)((byteRate >> 24) & 0xFF);
            wav[32] = (unsigned char)(blockAlign & 0xFF);
            wav[33] = (unsigned char)((blockAlign >> 8) & 0xFF);
            wav[34] = (unsigned char)(bitsPerSample & 0xFF);
            wav[35] = (unsigned char)((bitsPerSample >> 8) & 0xFF);
            memcpy(wav + 36, "data", 4);
            wav[40] = (unsigned char)(dataSize & 0xFF);
            wav[41] = (unsigned char)((dataSize >> 8) & 0xFF);
            wav[42] = (unsigned char)((dataSize >> 16) & 0xFF);
            wav[43] = (unsigned char)((dataSize >> 24) & 0xFF);
            memcpy(wav + 44, combinedPcm.data(), dataSize);

            {
                std::lock_guard<std::mutex> lock(cacheEntry->mtx);
                cacheEntry->wavData = wav;
                cacheEntry->wavSize = totalWavSz;
                cacheEntry->ready = true;
            }
            cacheEntry->cv.notify_all();
        });
}

unsigned char* osystem_tryGetPreDecodedVocPage(int vocIndex, int page, size_t* outSize)
{
    uint32_t cacheKey = makeVocCacheKey(vocIndex, page);

    std::lock_guard<std::mutex> lock(s_vocCacheMutex);
    auto it = s_vocPageCache.find(cacheKey);
    if (it == s_vocPageCache.end())
        return nullptr;

    auto& entry = it->second;
    std::lock_guard<std::mutex> entryLock(entry->mtx);

    if (!entry->ready)
        return nullptr;

    if (outSize)
        *outSize = entry->wavSize;

    return entry->wavData;
}

void osystem_clearVocCache()
{
    std::lock_guard<std::mutex> lock(s_vocCacheMutex);
    s_vocPageCache.clear();
}
