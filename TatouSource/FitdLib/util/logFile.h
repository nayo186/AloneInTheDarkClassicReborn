///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Based on FITD by yaz0r, Re-haunted is released under GPL
//
// Log file: everything written to stdout/stderr goes to tatou.log
///////////////////////////////////////////////////////////////////////////////

#pragma once

// Redirects stdout/stderr (unbuffered) to tatou.log in the working directory.
// The previous log is kept as tatou.prev.log. Call once, as early as possible.
void logFileInit();

// Flushes stdout/stderr. Safe to call from crash handlers and at exit.
void logFileFlush();
