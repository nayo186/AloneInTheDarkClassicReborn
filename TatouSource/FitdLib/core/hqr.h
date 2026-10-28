///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// HQR resource system declarations and templates
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>
#include "config.h"

template <typename T>
struct hqrSubEntryStruct
{
    s16 key;
    s16 size;
    unsigned int lastTimeUsed;
    T* ptr;
};

template <typename T>
struct hqrEntryStruct
{
    std::string string;
    u16 maxFreeData;
    u16 sizeFreeData;
    u16 numMaxEntry;
    u16 numUsedEntry;
    std::vector<hqrSubEntryStruct<T>> entries;
};

template <typename T>
// Entry `index` of a resource list, loaded from its PAK if it is not in memory yet
T* HQR_Get(hqrEntryStruct<T>* hqrPtr, int index);

// Creates an empty list of memory blocks (size in bytes, number of entries)
hqrEntryStruct<char>* HQR_Init(int size, int numEntry);
// Reserves a block of `size` bytes in a list; returns its number
int HQ_Malloc(hqrEntryStruct<char>* hqrPtr,int size);
// The memory of block number `index`
char* HQ_PtrMalloc(hqrEntryStruct<char>* hqrPtr, int index);
// Frees block number `index`
void HQ_Free_Malloc(hqrEntryStruct<char>* hqrPtr, int index);

template <typename T>
// Empties and deletes a resource list
void HQR_Free(hqrEntryStruct<T>* hqrPtr);

template <typename T>
// Creates a resource list read from a PAK (bodies, animations...)
hqrEntryStruct<T>* HQR_InitRessource(const char* name, int size, int numEntries);

template <typename T>
// Empties a resource list (everything is loaded again when asked for)
void HQR_Reset(hqrEntryStruct<T>* hqrPtr);

template <typename T>
// Changes the PAK a resource list reads from (the hero's bodies)
void configureHqrHero(hqrEntryStruct<T>* hqrPtr, const char* name);

// Builds a body (3D model) from its data in a PAK entry
struct sBody* createBodyFromPtr(void* ptr);


