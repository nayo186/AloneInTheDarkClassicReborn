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
#include "userFolder.h"
#include "narrators.h"
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

// The music track that is playing (file or GAME.GOG), to know when it ends
static SoLoud::handle s_trackHandle = 0;

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
    s_trackHandle = 0;

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
        s_trackHandle = osystemAL_playMusicSource(*s_gogTrack);
        printf(AL_TAG "Music: CD track %d from the GOG image\n", trackId);
        return 1;
    }

    pFile = new SoLoud::DiskFile(fHandle);
    pWavStream = new SoLoud::WavStream();
    SoLoud::result res = pWavStream->loadFile(pFile);
    if (res == SoLoud::SO_NO_ERROR)
    {
        s_trackHandle = osystemAL_playMusicSource(*pWavStream);
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

int osystem_isTrackPlaying()
{
#ifdef _WIN32
    if (s_cdPlaying)
    {
        char mode[32] = {};
        return mciSendStringA("status cdaudio mode", mode, sizeof(mode), NULL) == 0 && strcmp(mode, "playing") == 0;
    }
#endif
    return gSoloud && s_trackHandle && gSoloud->isValidVoiceHandle(s_trackHandle);
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

static std::shared_ptr<const NarratorVoices> currentNarrator();
static unsigned char* buildNarration(const NarratorVoices* narrator, int first, int count, size_t* outSize);
static bool playVoiceWav(unsigned char* wav, size_t wavSize, bool owned, const char* label);

void osystem_playVocByIndex(int vocIndex)
{
    if (!gSoloud || vocIndex < 0)
        return;

    char vocName[64];
    sprintf_s(vocName, sizeof(vocName), "%06d.VOC", vocIndex);
    printf(VO_TAG "Playing VOC by index: %d -> %s\n", vocIndex, vocName);

    // The chosen narrator when it has this reading
    const std::shared_ptr<const NarratorVoices> narrator = currentNarrator();
    if (narrator && narrator->hasPiece(vocIndex))
    {
        osystem_stopVO();
        size_t wavSize = 0;
        if (unsigned char* wav = buildNarration(narrator.get(), vocIndex, 1, &wavSize))
        {
            const std::string label = narrator->name() + " " + vocName;
            if (playVoiceWav(wav, wavSize, true, label.c_str()))
                return;
        }
    }
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

// ---- Narrators (Shared/narrators.h) ---------------------------------------------

// The chosen narrator (audio.voices: a folder of Voices in the working
// folder), opened again when the choice changes; null for the game's own
// (English) or when the folder has no voices. Main thread only; the
// background decoding keeps its own reference.
static std::shared_ptr<const NarratorVoices> currentNarrator()
{
    static std::shared_ptr<const NarratorVoices> s_narrator;
    static std::string s_openedName;
    static bool s_opened = false;
    const std::string wanted = g_settings.audio.voices;
    if (s_opened && wanted == s_openedName)
        return s_narrator;
    s_opened = true;
    s_openedName = wanted;
    s_narrator.reset();
    if (wanted.empty())
        return s_narrator;
    auto voices = std::make_shared<NarratorVoices>();
    if (voices->open(std::filesystem::u8path(std::string(kVoicesFolder) + "/" + wanted)))
    {
        const std::string image = narratorPathText(voices->imagePath().filename());
        printf(VO_OK "Narrator: %s (%s)" CON_RESET "\n", wanted.c_str(),
               image.empty() ? "voice files" : ("CD image " + image).c_str());
        s_narrator = voices;
    }
    else
    {
        printf(VO_WARN "Narrator %s: no voices in %s/%s, the game's narrator is used" CON_RESET "\n",
               wanted.c_str(), kVoicesFolder, wanted.c_str());
    }
    return s_narrator;
}

std::vector<std::string> osystem_findNarrators()
{
    return findNarrators(std::filesystem::u8path(kVoicesFolder));
}

// A voice file (VOC, WAV or OGG) as mono samples; false if it cannot be decoded
static bool decodeVoicePiece(const std::vector<unsigned char>& data, const std::string& ext,
                             std::vector<float>& samples, float& rate)
{
    const unsigned char* memory = data.data();
    size_t size = data.size();
    unsigned char* vocWav = nullptr;
    if (ext == ".voc")
    {
        if (!vocDecodeToWav(memory, size, &vocWav, &size))
            return false;
        memory = vocWav;
    }
    SoLoud::Wav wav;
    const bool loaded = wav.loadMem(memory, (unsigned int)size, true, false) == SoLoud::SO_NO_ERROR;
    delete[] vocWav;
    if (!loaded || wav.mSampleCount == 0)
        return false;

    // SoLoud keeps the channels one after the other
    const unsigned int channels = wav.mChannels > 0 ? wav.mChannels : 1;
    samples.resize(wav.mSampleCount);
    for (unsigned int i = 0; i < wav.mSampleCount; i++)
    {
        float sum = 0.0f;
        for (unsigned int c = 0; c < channels; c++)
            sum += wav.mData[c * wav.mSampleCount + i];
        samples[i] = sum / channels;
    }
    rate = wav.mBaseSamplerate;
    return true;
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

// One voice: the chosen narrator's, or else the game's own (English)
static bool readVoice(const NarratorVoices* narrator, int number, std::vector<unsigned char>& data, std::string& ext)
{
    if (narrator)
        return narrator->readPiece(number, data, ext);
    char vocName[16];
    snprintf(vocName, sizeof(vocName), "%06d.VOC", number);
    size_t size = 0;
    unsigned char* raw = loadVocFileData(vocName, &size);
    if (!raw)
        return false;
    data.assign(raw, raw + size);
    delete[] raw;
    ext = ".voc";
    return true;
}

// The voices first, first + 1... (at most count, until the first missing one)
// as one 16-bit mono WAV in memory (the buffer is the caller's: delete[]), or
// nullptr. A book page is BBSS00 onwards: pieces of about 2.7 s of the
// page's narration (BBSSNN: book, page, piece), which do not follow the lines
// of the text, so a page plays them all (BUGS.md C35). Pieces with another
// sample rate (WAV, OGG) are converted to the first one's.
static unsigned char* buildNarration(const NarratorVoices* narrator, int first, int count, size_t* outSize)
{
    std::vector<float> narration;
    float rate = 0.0f;
    for (int piece = 0; piece < count; piece++)
    {
        std::vector<unsigned char> data;
        std::string ext;
        std::vector<float> samples;
        float pieceRate = 0.0f;
        if (!readVoice(narrator, first + piece, data, ext) || !decodeVoicePiece(data, ext, samples, pieceRate))
            break;
        if (rate == 0.0f)
            rate = pieceRate;
        if (pieceRate == rate)
        {
            narration.insert(narration.end(), samples.begin(), samples.end());
            continue;
        }
        const double step = (double)pieceRate / rate;
        for (double pos = 0.0; pos < samples.size() - 1; pos += step)
        {
            const size_t at = (size_t)pos;
            const float t = (float)(pos - at);
            narration.push_back(samples[at] + (samples[at + 1] - samples[at]) * t);
        }
    }
    if (narration.empty())
        return nullptr;

    const uint32_t dataSize = (uint32_t)narration.size() * 2;
    const uint32_t wavSize = 44 + dataSize;
    unsigned char* wav = new (std::nothrow) unsigned char[wavSize];
    if (!wav)
        return nullptr;
    auto put16 = [wav](int at, uint32_t value) { wav[at] = (unsigned char)(value & 0xFF); wav[at + 1] = (unsigned char)((value >> 8) & 0xFF); };
    auto put32 = [&put16](int at, uint32_t value) { put16(at, value & 0xFFFF); put16(at + 2, value >> 16); };
    const uint32_t sampleRate = (uint32_t)(rate + 0.5f);
    memcpy(wav, "RIFF", 4);
    put32(4, wavSize - 8);
    memcpy(wav + 8, "WAVE", 4);
    memcpy(wav + 12, "fmt ", 4);
    put32(16, 16);              // fmt chunk size
    put16(20, 1);               // PCM
    put16(22, 1);               // mono
    put32(24, sampleRate);
    put32(28, sampleRate * 2);  // bytes per second
    put16(32, 2);               // bytes per sample
    put16(34, 16);              // bits per sample
    memcpy(wav + 36, "data", 4);
    put32(40, dataSize);
    for (size_t i = 0; i < narration.size(); i++)
    {
        const float clamped = (std::max)(-1.0f, (std::min)(1.0f, narration[i]));
        put16(44 + (int)i * 2, (uint16_t)(int16_t)(clamped * 32767.0f));
    }
    *outSize = wavSize;
    return wav;
}

// Plays a voice WAV in memory; owned: deleted with the voice (else the cache keeps it)
static bool playVoiceWav(unsigned char* wav, size_t wavSize, bool owned, const char* label)
{
    s_voStream = new SoLoud::WavStream();
    if (s_voStream->loadMem(wav, static_cast<unsigned int>(wavSize), false, false) != SoLoud::SO_NO_ERROR)
    {
        if (owned)
            delete[] wav;
        delete s_voStream;
        s_voStream = nullptr;
        printf(VO_WARN "Failed to play the voice %s" CON_RESET "\n", label);
        return false;
    }
    if (owned)
        s_voBuf = wav;
    s_voHandle = playAudioSource(*s_voStream);
    if (g_voPitchMultiplier != 1.0f)
        gSoloud->setRelativePlaySpeed(s_voHandle, g_voPitchMultiplier);
    return true;
}

static std::string vocCacheKey(const NarratorVoices* narrator, int vocIndex, int page);
static unsigned char* tryGetPreDecodedVocPage(const std::string& key, size_t* outSize);

// The narrator of a page: the chosen one when it has that page (a page never
// mixes narrators), else null (the game's own)
static const NarratorVoices* pageNarrator(const std::shared_ptr<const NarratorVoices>& narrator, int vocIndex, int page)
{
    return (narrator && narrator->hasPiece(vocIndex * 10000 + page * 100)) ? narrator.get() : nullptr;
}

void osystem_playVocPage(int vocIndex, int page)
{
    if (!gSoloud || vocIndex < 0)
        return;

    osystem_stopVO();

    const std::shared_ptr<const NarratorVoices> chosen = currentNarrator();
    const NarratorVoices* narrator = pageNarrator(chosen, vocIndex, page);
    const std::string narratorName = narrator ? narrator->name() : std::string("the game's");

    // Already decoded in the background (the cache keeps the buffer)
    size_t wavSize = 0;
    unsigned char* wav = tryGetPreDecodedVocPage(vocCacheKey(narrator, vocIndex, page), &wavSize);
    const bool cached = wav && wavSize > 0;
    if (!cached)
        wav = buildNarration(narrator, vocIndex * 10000 + page * 100, 100, &wavSize);
    if (!wav)
    {
        printf(VO_WARN "No voice for book %d page %d" CON_RESET "\n", vocIndex, page);
        return;
    }
    char label[64];
    snprintf(label, sizeof(label), "of book %d page %d", vocIndex, page);
    if (playVoiceWav(wav, wavSize, !cached, label))
        printf(VO_OK "Voice of book %d page %d: %s narrator, %u bytes%s" CON_RESET "\n", vocIndex, page,
               narratorName.c_str(), (unsigned)wavSize, cached ? " (decoded in the background)" : "");
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

static std::unordered_map<std::string, std::shared_ptr<PreDecodedVocPage>> s_vocPageCache;
static std::mutex s_vocCacheMutex;

// One entry per narrator, book and page
static std::string vocCacheKey(const NarratorVoices* narrator, int vocIndex, int page)
{
    return (narrator ? narrator->name() : std::string()) + "/" + std::to_string(vocIndex * 10000 + page);
}

void osystem_preDecodeVocPage(int vocIndex, int page)
{
    if (!JobSystem::instance().isInitialized())
        return;

    // The job keeps the narrator alive even if the choice changes meanwhile
    const std::shared_ptr<const NarratorVoices> chosen = currentNarrator();
    const NarratorVoices* narrator = pageNarrator(chosen, vocIndex, page);
    const std::string cacheKey = vocCacheKey(narrator, vocIndex, page);

    auto cacheEntry = std::make_shared<PreDecodedVocPage>();
    {
        std::lock_guard<std::mutex> lock(s_vocCacheMutex);
        if (s_vocPageCache.count(cacheKey))
            return;
        s_vocPageCache[cacheKey] = cacheEntry;
    }

    const std::shared_ptr<const NarratorVoices> keep = narrator ? chosen : nullptr;
    JobSystem::instance().scheduleJob(
        JobSystem::JobType::DECODE_VOC,
        JobSystem::Priority::HIGH,
        [vocIndex, page, keep, cacheEntry]() {
            size_t wavSize = 0;
            unsigned char* wav = buildNarration(keep.get(), vocIndex * 10000 + page * 100, 100, &wavSize);
            if (!wav)
                return;
            {
                std::lock_guard<std::mutex> lock(cacheEntry->mtx);
                cacheEntry->wavData = wav;
                cacheEntry->wavSize = wavSize;
                cacheEntry->ready = true;
            }
            cacheEntry->cv.notify_all();
        });
}

// The voice of a book page already decoded in the background, or nullptr
static unsigned char* tryGetPreDecodedVocPage(const std::string& key, size_t* outSize)
{
    std::lock_guard<std::mutex> lock(s_vocCacheMutex);
    auto it = s_vocPageCache.find(key);
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
