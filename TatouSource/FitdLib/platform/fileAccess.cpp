///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// File I/O utilities and path management
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "languagePack.h"
#include <cstring>
// seg 20
void fatalError(int type, const char* name)
{
    //  freeScene();
    freeAll();
    printf(FACC_ERR "Error: %s" CON_RESET "\n", name);
    assert(0);
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
