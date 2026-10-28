///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Embedded data registry. Original game data is no longer embedded: the
// engine always reads the player's own copy from disk. Only remaster
// helper assets remain here.
///////////////////////////////////////////////////////////////////////////////

#include "embeddedData.h"
#include <cstring>
#include <cctype>

extern const unsigned char embdata_ControllerHint_png[];
extern const unsigned long long embdata_ControllerHint_png_size;

struct EmbeddedFileEntry
{
    const char* name;
    const unsigned char* data;
    unsigned long long size;
};

static const EmbeddedFileEntry s_embeddedFiles[] = {
    { "ControllerHint.png", embdata_ControllerHint_png, embdata_ControllerHint_png_size },
};

static const int s_numEmbeddedFiles = sizeof(s_embeddedFiles) / sizeof(s_embeddedFiles[0]);

static const char* extractFilename(const char* path)
{
    const char* lastSlash = path;
    for (const char* p = path; *p; p++)
    {
        if (*p == '/' || *p == '\\')
            lastSlash = p + 1;
    }
    return lastSlash;
}

static bool strEqualNoCase(const char* a, const char* b)
{
    while (*a && *b)
    {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b))
            return false;
        a++; b++;
    }
    return *a == *b;
}

bool getEmbeddedFile(const char* filename, const unsigned char** outData, size_t* outSize)
{
    const char* name = extractFilename(filename);
    for (int i = 0; i < s_numEmbeddedFiles; i++)
    {
        if (strEqualNoCase(name, s_embeddedFiles[i].name))
        {
            if (outData) *outData = s_embeddedFiles[i].data;
            if (outSize) *outSize = (size_t)s_embeddedFiles[i].size;
            return true;
        }
    }
    return false;
}
