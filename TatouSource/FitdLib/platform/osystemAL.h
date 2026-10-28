///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// OpenAL audio system declarations
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif
  
// Starts the sound engine (SoLoud) and its two volume groups
void osystemAL_init();
// Applies the volumes and frees finished sounds (each frame)
void osystemAL_udpate();
// Sample rate of the sound output
int  osystemAL_getSampleRate();


#ifdef __cplusplus
}
#endif
