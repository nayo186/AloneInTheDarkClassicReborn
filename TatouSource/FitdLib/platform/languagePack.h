///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The game's data files and a language from the player's CD (AITD3): with
// that language chosen, the files and CD tracks its folder Languages\<name>
// has (loose or in its CD image, Shared/languageFolder.h) are read before the
// game's own, in homePath. Only read.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdio>
#include <string>
#include <vector>

namespace SoLoud { class AudioSource; }

// Uses Languages/<name>/ of the working folder if it has <language>.PAK,
// loose or in its CD image; false if not
bool selectLanguagePack(const std::string& language);
// The language folder in use, with a trailing slash (empty: none)
const std::string& languagePackFolder();
// A CD track of the language's CD image (CD numbering: 2 = the first audio
// track); null if it has no image, no track list or not that track. The
// caller deletes it (after stopping it)
SoLoud::AudioSource* languagePackCreateTrack(int track);

// A data file opened for reading: loose in the language folder, else in its
// CD image, else in homePath
class DataFile
{
public:
    ~DataFile() { close(); }
    // Opens a data file by name (e.g. "LISTSAMP.PAK"); false if it is nowhere
    bool open(const char* name);
    // True while a file is open
    bool isOpen() const { return m_file || m_memory; }
    // Moves the read position, as fseek
    void seek(long offset, int origin);
    // Reads up to `size` bytes, as fread; the bytes read
    size_t read(void* buffer, size_t size);
    // The size of the whole file in bytes
    long size();
    // Closes the file (also done when it goes away)
    void close();

private:
    FILE* m_file = nullptr;
    const std::vector<unsigned char>* m_memory = nullptr;   // a file of the CD image
    long m_position = 0;
};
