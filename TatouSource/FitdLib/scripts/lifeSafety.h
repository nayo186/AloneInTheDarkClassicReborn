///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Safety checks after each life script instruction (life.cpp, processLife):
// an object left with an animation or body that does not exist, a frame past
// the end of its animation or an impossible speed is put back to a valid
// value, and the case is written to tatou.log, instead of letting the engine
// read outside its data.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "common.h"

// Puts back a valid animation, body, frame or speed after a script instruction
void checkObjectAfterLifeOpcode(int lifeNum, int opcode, tObject* actor);
