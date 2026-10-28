///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Zone volume (bounding box) declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _ZV_H_
#define _ZV_H_
// Collision box of a body as a cube (the largest side on every axis)
void getZvCube(sBody* bodyPtr, ZVStruct* zvPtr);
// Collision box of a body as stored in its data
void GiveZVObjet(sBody* bodyPtr, ZVStruct* zvPtr);
// Collision box of a body big enough for any turn
void getZvMax(sBody* bodyPtr, ZVStruct* zvPtr);
// Collision box of a body turned by its angles
void getZvRot(sBody* bodyPtr, ZVStruct* zvPtr, int alpha, int beta, int gamma);
// A default collision box (200 x 200 x 200 around the origin)
void makeDefaultZV(ZVStruct* zvPtr);
#endif
