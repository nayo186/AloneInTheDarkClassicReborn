///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Log file: everything written to stdout/stderr goes to tatou.log
///////////////////////////////////////////////////////////////////////////////

#include "logFile.h"

#ifdef _WIN32

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>

static const char* kLogName = "tatou.log";
static const char* kPrevLogName = "tatou.prev.log";

static bool s_initialized = false;

// stdout/stderr are pointed straight at the file (no pipe, no extra thread),
// so nothing can block or deadlock when the game exits or crashes.
void logFileInit()
{
    if (s_initialized)
        return;

    // Keep the previous session's log
    MoveFileExA(kLogName, kPrevLogName, MOVEFILE_REPLACE_EXISTING);

    // Shared for reading so the log can be inspected while the game runs
    int fd = -1;
    if (_sopen_s(&fd, kLogName, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_TEXT, _SH_DENYWR, _S_IREAD | _S_IWRITE) != 0)
        return;

    // A GUI-subsystem process may start without valid stdio descriptors
    FILE* reopened = nullptr;
    if (_fileno(stdout) < 0)
        freopen_s(&reopened, "NUL", "w", stdout);
    if (_fileno(stderr) < 0)
        freopen_s(&reopened, "NUL", "w", stderr);

    fflush(stdout);
    fflush(stderr);
    _dup2(fd, _fileno(stdout));
    _dup2(fd, _fileno(stderr));
    _close(fd);

    HANDLE fileHandle = (HANDLE)_get_osfhandle(_fileno(stdout));
    SetStdHandle(STD_OUTPUT_HANDLE, fileHandle);
    SetStdHandle(STD_ERROR_HANDLE, fileHandle);

    // Unbuffered so nothing is lost if the game crashes
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    s_initialized = true;

    time_t now = time(nullptr);
    char timeStr[64];
    ctime_s(timeStr, sizeof(timeStr), &now);
    printf("=== Tatou log started %s", timeStr);
}

void logFileFlush()
{
    if (!s_initialized)
        return;

    fflush(stdout);
    fflush(stderr);
}

#else

#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static const char* kLogName = "tatou.log";
static const char* kPrevLogName = "tatou.prev.log";

static bool s_initialized = false;

// As on Windows: stdout/stderr pointed straight at the file
void logFileInit()
{
    if (s_initialized)
        return;

    // Keep the previous session's log
    rename(kLogName, kPrevLogName);

    const int fd = open(kLogName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return;

    fflush(stdout);
    fflush(stderr);
    dup2(fd, fileno(stdout));
    dup2(fd, fileno(stderr));
    close(fd);

    // Unbuffered so nothing is lost if the game crashes
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    s_initialized = true;

    time_t now = time(nullptr);
    printf("=== Tatou log started %s", ctime(&now));
}

void logFileFlush()
{
    if (!s_initialized)
        return;

    fflush(stdout);
    fflush(stderr);
}

#endif
