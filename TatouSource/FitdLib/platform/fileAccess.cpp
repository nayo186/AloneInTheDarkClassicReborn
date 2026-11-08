///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// File I/O utilities and path management
///////////////////////////////////////////////////////////////////////////////

#include "exitProgram.h"
#include "common.h"
#include "consoleLog.h"
#include "languagePack.h"
#include <SDL.h>
#include <cstdlib>
#include <cstring>

void fatalError(int type, const char* name)
{
    // It went on as if nothing happened and the game closed at the next file
    // it read, without a word: a message and a clean exit (BUGS.md C55)
    printf(FACC_ERR "Error: %s" CON_RESET "\n", name);
    char message[512];
    snprintf(message, sizeof(message), "A file of the game is missing or cannot be read: %s\n\n"
        "Check the game folder chosen in the launcher.", name);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Alone in the Dark: Classic Reborn", message, nullptr);
    freeAll();
    exitProgram(1);
}

char* loadFromItd(const char* name)
{
    // Try disk file first
    {
        DataFile file;
        char* ptr;

        if(file.open(name))
        {
            fileSize = file.size();
            ptr = (char*)malloc(fileSize);

            if(!ptr)
            {
                fatalError(1,name);
                return NULL;
            }
            file.read(ptr,fileSize);
            return(ptr);
        }
    }

    fatalError(0,name);
    return NULL;
}

char* CheckLoadMallocPak(const char* name, int index)
{
    char* ptr;
    ptr = loadPak(name, index);
    if(!ptr)
    {
        fatalError(0,name);
    }
    return ptr;
}
