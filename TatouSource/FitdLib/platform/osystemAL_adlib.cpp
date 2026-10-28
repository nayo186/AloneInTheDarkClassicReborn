///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// AdLib music output (Jack in the Dark; AITD1 plays its CD tracks instead).
// The core's OPL emulation (core/music.cpp, musicUpdate) writes 16-bit mono
// samples at the mixer rate; this SoLoud source asks it for them whenever the
// mixer needs sound. Rebuilt for SoLoud: the old OpenAL version was disabled
// and AdLib music never played.
///////////////////////////////////////////////////////////////////////////////

#include "soloud.h"

#include "common.h"
#include "osystemAL.h"
#include "consoleLog.h"

#include <vector>

extern SoLoud::Soloud* gSoloud;
SoLoud::handle osystemAL_playMusicSource(SoLoud::AudioSource& source);   // osystemAL.cpp

// The emulation writes quietly: Jack's music peaks at about 3800 of 32767
// (11%, measured 2026-10-27). This brings the peaks to about 70%, the level
// of a CD track
static const float kAdlibGain = 6.0f;

class AdlibInstance : public SoLoud::AudioSourceInstance
{
public:
    unsigned int getAudio(float* buffer, unsigned int samplesToRead, unsigned int /*bufferSize*/) override
    {
        m_samples.resize(samplesToRead);
        // The emulation always fills the buffer (silence when no music plays)
        musicUpdate(nullptr, (uint8*)m_samples.data(), (int)(samplesToRead * sizeof(int16)));
        for (unsigned int i = 0; i < samplesToRead; i++)
            buffer[i] = m_samples[i] * (kAdlibGain / 32768.0f);
        return samplesToRead;
    }

    bool hasEnded() override
    {
        return false;   // a stream that lasts the whole session
    }

private:
    std::vector<int16> m_samples;
};

class AdlibSource : public SoLoud::AudioSource
{
public:
    AdlibSource()
    {
        mChannels = 1;
        mBaseSamplerate = (float)osystemAL_getSampleRate();   // the rate the emulation was set up with
    }

    SoLoud::AudioSourceInstance* createInstance() override
    {
        return new AdlibInstance();
    }
};

static AdlibSource* s_adlibSource = nullptr;
static SoLoud::handle s_adlibHandle = 0;

// Called when a piece of AdLib music starts (core/music.cpp): the stream is
// started once and then keeps playing what the emulation produces
void osystem_playAdlib()
{
    if (!gSoloud)
        return;
    if (s_adlibSource && gSoloud->isValidVoiceHandle(s_adlibHandle))
        return;
    if (!s_adlibSource)
        s_adlibSource = new AdlibSource();
    s_adlibHandle = osystemAL_playMusicSource(*s_adlibSource);
    printf(MUSIC_OK "AdLib output started (%d Hz)\n", osystemAL_getSampleRate());
}

// Before the mixer shuts down (osystemAL.cpp)
void osystemAL_adlibStop()
{
    if (!s_adlibSource)
        return;
    if (gSoloud)
        gSoloud->stop(s_adlibHandle);
    delete s_adlibSource;
    s_adlibSource = nullptr;
    s_adlibHandle = 0;
}
