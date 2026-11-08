///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Ends the program. exit() also destroys the program's static objects
// (mutexes among them) while other threads (sound, the window, the job
// threads) are still running; Android checks it and aborts ("mutex
// destroyed", BUGS.md C60). There _Exit ends at once without destroying
// anything: the options are saved and the log written as they happen.
// Elsewhere exit() as always.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdlib>

// Ends the program with this exit code
[[noreturn]] inline void exitProgram(int code)
{
#ifdef __ANDROID__
    std::_Exit(code);
#else
    std::exit(code);
#endif
}
