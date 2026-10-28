///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Volumes of the music and of the voices and effects (aitd_remaster.cfg
// audio.musicVolume / audio.soundVolume, 0-100), shared by the game and the
// launcher.
///////////////////////////////////////////////////////////////////////////////

#pragma once

static const int kDefaultAudioVolume = 100;   // the GOG level (BUGS.md C22)

// Slider position (0-100) to mixer gain: squared, so the middle of the
// slider sounds like half the loudness rather than barely quieter
inline float audioVolumeGain(int percent)
{
    if (percent <= 0)
        return 0.0f;
    if (percent >= 100)
        return 1.0f;
    const float x = percent / 100.0f;
    return x * x;
}
