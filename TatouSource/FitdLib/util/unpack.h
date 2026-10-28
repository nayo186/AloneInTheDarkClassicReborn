///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Data decompression declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _UNPACK_DAMIEN_H_
#define _UNPACK_DAMIEN_H_

// Decompresses a PAK entry stored with deflate (zlib)
int PAK_deflate(unsigned char * srcBuffer, unsigned char * dstBuffer, unsigned int compressedSize, unsigned int uncompressedSize);
// Decompresses a PAK entry stored with PKWARE's explode
int PAK_explode(unsigned char * srcBuffer, unsigned char * dstBuffer, unsigned int compressedSize, unsigned int uncompressedSize, unsigned short flags);

#endif
