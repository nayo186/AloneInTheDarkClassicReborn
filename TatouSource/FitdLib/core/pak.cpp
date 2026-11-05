///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// PAK archive file format reading and extraction
///////////////////////////////////////////////////////////////////////////////

// seg 55

#include "common.h"
#include "consoleLog.h"
#include "languagePack.h"

#ifdef WIN32
#include <direct.h>
#endif

#include <cstring>

struct pakInfoStruct // warning: alignment unsafe
{
    s32 discSize;
    s32 uncompressedSize;
    char compressionFlag;
    char info5;
    s16 offset;
};

//#define USE_UNPACKED_DATA

void readPakInfo(pakInfoStruct* pPakInfo, DataFile& fileHandle)
{
    fileHandle.read(&pPakInfo->discSize,4);
    fileHandle.read(&pPakInfo->uncompressedSize,4);
    fileHandle.read(&pPakInfo->compressionFlag,1);
    fileHandle.read(&pPakInfo->info5,1);
    fileHandle.read(&pPakInfo->offset,2);

    pPakInfo->discSize = READ_LE_U32(&pPakInfo->discSize);
    pPakInfo->uncompressedSize = READ_LE_U32(&pPakInfo->uncompressedSize);
    pPakInfo->offset = READ_LE_U16(&pPakInfo->offset);
}

unsigned int PAK_getNumFiles(const char* name)
{
    // Try disk file first
    {
        DataFile fileHandle;
        u32 fileOffset;

        fileHandle.open((std::string(name) + ".PAK").c_str());

        if(fileHandle.isOpen())
        {
            fileHandle.seek(4,SEEK_CUR);
            fileHandle.read(&fileOffset,4);
#ifdef MACOSX
            fileOffset = READ_LE_U32(&fileOffset);
#endif
            fileHandle.close();
            // The entry table starts at offset 4 and ends where the first
            // entry begins, so it holds (fileOffset/4)-1 entries. The old
            // "-2" missed the last entry in every AITD1 and JACK PAK.
            return((fileOffset/4)-1);
        }
    }

    return 0;
}

int LoadPak(const char* name, int index, char* ptr)
{
#ifdef USE_UNPACKED_DATA
    char buffer[256];
    FILE* fHandle;
    int size;

    sprintf_s(buffer, sizeof(buffer), "%s/%04X.OUT",name,index);

    fopen_s(&fHandle, buffer,"rb");

    if(!fHandle)
        return(0);

    fseek(fHandle,0L,SEEK_END);
    size = ftell(fHandle);
    fseek(fHandle,0L,SEEK_SET);

    fread(ptr,size,1,fHandle);
    fclose(fHandle);

    return(1);
#else
    char* lptr;

    lptr = loadPak(name,index);

    if(!lptr)
        return(0);

    memcpy(ptr,lptr,getPakSize(name,index));

    free(lptr);

    return(1);
#endif
}

int getPakSize(const char* name, int index)
{
    // Bounds check - log warning but continue to access data (DOS buffer skipover)
    int numFiles = PAK_getNumFiles(name);
    if(index < 0 || numFiles <= index)
    {
        printf("[PAK] Warning: DOS buffer skipover - getPakSize index %d for '%s' (numFiles=%d)\n", index, name, numFiles);
    }

#ifdef USE_UNPACKED_DATA
    char buffer[256];
    FILE* fHandle;
    int size;

    sprintf_s(buffer, sizeof(buffer), "%s/%04X.OUT",name,index);

    fopen_s(&fHandle, buffer,"rb");

    if(!fHandle)
        return(0);

    fseek(fHandle,0L,SEEK_END);
    size = ftell(fHandle);
    fseek(fHandle,0L,SEEK_SET);

    fclose(fHandle);

    return (size);
#else
    // Try disk file first
    {
        DataFile fileHandle;
        s32 fileOffset;
        s32 additionalDescriptorSize;
        pakInfoStruct pakInfo;
        s32 size=0;

        fileHandle.open((std::string(name) + ".PAK").c_str());

        if(fileHandle.isOpen())
        {
            fileHandle.seek((index+1)*4,SEEK_SET);

            fileHandle.read(&fileOffset,4);
            fileOffset = READ_LE_U32(&fileOffset);

            fileHandle.seek(fileOffset,SEEK_SET);

            fileHandle.read(&additionalDescriptorSize,4);
            additionalDescriptorSize = READ_LE_U32(&additionalDescriptorSize);

            if(additionalDescriptorSize)
            {
                fileHandle.seek(additionalDescriptorSize-4, SEEK_CUR);
            }

            readPakInfo(&pakInfo,fileHandle);

            fileHandle.seek(pakInfo.offset,SEEK_CUR);

            // Apply same safety caps as loadPak for DOS buffer skipover consistency
            const s32 MAX_PAK_SIZE = 16 * 1024 * 1024;
            if(pakInfo.discSize > MAX_PAK_SIZE || pakInfo.discSize <= 0) pakInfo.discSize = 4096;
            if(pakInfo.uncompressedSize > MAX_PAK_SIZE || pakInfo.uncompressedSize <= 0) pakInfo.uncompressedSize = 4096;

            if(pakInfo.compressionFlag == 0) // uncompressed
            {
                size = pakInfo.discSize;
            }
            else if(pakInfo.compressionFlag == 1) // compressed
            {
                size = pakInfo.uncompressedSize;
            }
            else if(pakInfo.compressionFlag == 4)
            {
                size = pakInfo.uncompressedSize;
            }

            fileHandle.close();
            return size;
        }
    }

    return 0;
#endif
}

char* loadPak(const char* name, int index)
{
	// Bounds check - log warning but continue to access data (DOS buffer skipover)
	int numFiles = PAK_getNumFiles(name);
	if(index < 0 || numFiles <= index)
	{
		printf("[PAK] Warning: DOS buffer skipover - loadPak index %d for '%s' (numFiles=%d)\n", index, name, numFiles);
	}

#ifdef USE_UNPACKED_DATA
	char buffer[256];
	FILE* fHandle;
	int size;
	char* ptr;

	sprintf_s(buffer, sizeof(buffer), "%s/%04X.OUT",name,index);

	fopen_s(&fHandle, buffer,"rb");

	if(!fHandle)
		return NULL;

	fseek(fHandle,0L,SEEK_END);
	size = ftell(fHandle);
	fseek(fHandle,0L,SEEK_SET);

	ptr = (char*)malloc(size);

	fread(ptr,size,1,fHandle);
	fclose(fHandle);

	return ptr;
#else
	// Try disk file first
	{
		DataFile fileHandle;
		u32 fileOffset;
		u32 additionalDescriptorSize;
		pakInfoStruct pakInfo;
		char* ptr=0;

		fileHandle.open((std::string(name) + ".PAK").c_str());

		if(fileHandle.isOpen())
		{
			char nameBuffer[256] = "";

			fileHandle.seek((index+1)*4,SEEK_SET);

			fileHandle.read(&fileOffset,4);
			fileOffset = READ_LE_U32(&fileOffset);

			fileHandle.seek(fileOffset,SEEK_SET);

			fileHandle.read(&additionalDescriptorSize,4);
			additionalDescriptorSize = READ_LE_U32(&additionalDescriptorSize);

			if(additionalDescriptorSize)
		{
			fileHandle.seek(additionalDescriptorSize-4, SEEK_CUR);
		}

		readPakInfo(&pakInfo,fileHandle);

		if(pakInfo.offset >= sizeof(nameBuffer))
		{
			// A garbage header (index past the end of the PAK) asked for a
			// name longer than the buffer: skip it, never overflow the stack
			fileHandle.seek(pakInfo.offset, SEEK_CUR);
		}
		else if(pakInfo.offset)
		{
			fileHandle.read(nameBuffer,pakInfo.offset);
#ifdef FITD_DEBUGGER
			printf(PAK_TAG "Loading %s/%s\n", name,nameBuffer+2);
#endif
		}
		else
		{
			fileHandle.seek(pakInfo.offset,SEEK_CUR);
		}

		// DOS buffer skipover safety: cap sizes to prevent crash from garbage pakInfo
		const u32 MAX_PAK_SIZE = 16 * 1024 * 1024; // 16MB max
		if(pakInfo.discSize > MAX_PAK_SIZE) pakInfo.discSize = 4096;
		if(pakInfo.uncompressedSize > MAX_PAK_SIZE) pakInfo.uncompressedSize = 4096;
		if(pakInfo.discSize == 0) pakInfo.discSize = 4096;
		if(pakInfo.uncompressedSize == 0) pakInfo.uncompressedSize = 4096;

		switch(pakInfo.compressionFlag)
		{
		case 0:
			{
				ptr = (char*)malloc(pakInfo.discSize);
				if(ptr) fileHandle.read(ptr,pakInfo.discSize);
				break;
			}
		case 1:
			{
				char * compressedDataPtr = (char *) malloc(pakInfo.discSize);
				if(compressedDataPtr)
				{
					fileHandle.read(compressedDataPtr, pakInfo.discSize);
					ptr = (char *) malloc(pakInfo.uncompressedSize);
					if(ptr)
					{
						PAK_explode((unsigned char*)compressedDataPtr, (unsigned char*)ptr, pakInfo.discSize, pakInfo.uncompressedSize, pakInfo.info5);
					}
					free(compressedDataPtr);
				}
				break;
			}
		case 4:
			{
				char * compressedDataPtr = (char *) malloc(pakInfo.discSize);
				if(compressedDataPtr)
				{
					fileHandle.read(compressedDataPtr, pakInfo.discSize);
					ptr = (char *) malloc(pakInfo.uncompressedSize);
					if(ptr)
					{
						PAK_deflate((unsigned char*)compressedDataPtr, (unsigned char*)ptr, pakInfo.discSize, pakInfo.uncompressedSize);
					}
					free(compressedDataPtr);
				}
				break;
			}
		default:
			// DOS buffer skipover: unknown compression, allocate dummy buffer
			printf("[PAK] Warning: DOS buffer skipover - unknown compressionFlag %d\n", pakInfo.compressionFlag);
			ptr = (char*)malloc(4096);
			break;
		}

		// DOS buffer skipover: never return NULL, allocate fallback if needed
		if(!ptr)
		{
			printf("[PAK] Warning: DOS buffer skipover - malloc failed, returning dummy buffer\n");
			ptr = (char*)malloc(4096);
			if(ptr) memset(ptr, 0, 4096);
		}

			fileHandle.close();
			return ptr;
		}
	}

	return NULL;
#endif
}

