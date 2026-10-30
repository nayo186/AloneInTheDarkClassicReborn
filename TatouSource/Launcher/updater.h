///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Updates from GitHub Releases (nayo186/AloneInTheDarkClassicReborn). The
// launcher is the only part of the project that connects to the internet, and
// only for this; the game never does. The work runs on a thread of its own:
// - check: asks GitHub for the latest release and compares its tag with this
//   build's version (CLASSIC_REBORN_VERSION, set by tools/makeRelease from the
//   tag; "dev" builds never look for updates);
// - install: downloads this system's package (windows-x64.zip or
//   linux-x64.tar.gz) to the Update folder next to the programs, unpacks it
//   and replaces only the files the package brings (programs and documents).
//   UserData and launcher.cfg are never touched. The running launcher cannot
//   be overwritten on Windows, so every old file is first moved to
//   Update/old (deleted the next time the launcher opens); if anything fails,
//   the old files go back.
// curl and tar do the downloading and unpacking: Windows 10 and 11 have both
// (System32; its tar also opens .zip), as Linux does.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

enum class UpdateState
{
    Idle,           // nothing asked yet
    Checking,
    UpToDate,
    Available,      // a newer release with this system's package
    CheckFailed,
    Installing,
    Installed,      // the launcher must be closed and opened again
    InstallFailed,  // nothing was changed
};

// This build's version ("1.0.0-beta.8"), or "dev"
const char* launcherVersion();
// False for development builds: no checks, no update button
bool updatesEnabled();

// Removes what the previous update left (Update folder); when the launcher opens
void updaterCleanup();
// Looks for a newer release on a thread of its own
void updaterStartCheck();
// Downloads and installs the release found by the check
void updaterStartInstall();
// What the updater is doing now
UpdateState updaterState();
// The newest version found ("1.0.0-beta.8"); valid from Available on
std::string updaterLatestVersion();
// Waits while files are being replaced (never leaves the programs half done)
void updaterShutdown();
