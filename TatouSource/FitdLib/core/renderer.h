///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Renderer interface declarations
///////////////////////////////////////////////////////////////////////////////

#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <string>

extern int BBox3D1;
extern int BBox3D2;
extern int BBox3D3;
extern int BBox3D4;

#define NUM_MAX_POINT_IN_POINT_BUFFER 800
#define NUM_MAX_BONES 50

extern std::array<point3dStruct, NUM_MAX_POINT_IN_POINT_BUFFER> pointBuffer;
extern int numOfPoints;

// Track which body's primitives are currently in primTable[]
// External code (like lantern glow detection) can check this to verify
// they're scanning the correct body's primitives, not some other body
extern int g_currentPrimTableBodyNum;

// Projected primitive queued for drawing (filled by AffObjet)
struct rendererPointStruct
{
    float X;
    float Y;
    float Z;
};

#define NUM_MAX_VERTEX_IN_PRIM 64

struct primEntryStruct
{
	u8 material;
	u8 color;
	u16 size;
	u16 numOfVertices;
	primTypeEnum type;
	int originalPrimIndex;
	bool isRampPrim;  // Track if this primitive is ramp-shaded (material 3-6)
	bool nearClipped;  // Flagged by floating polygon detection (suppressed at render time)
	rendererPointStruct vertices[NUM_MAX_VERTEX_IN_PRIM];
};

extern primEntryStruct primTable[];

// Turns and moves a point by the camera
void transformPoint(float* ax, float* bx, float* cx);

// Draws a 3D model at a position and angles: projects it and sends its polygons to the renderer
int AffObjet(int x, int y, int z, int alpha, int beta, int gamma, sBody* pBody);

// Size of an object along its own axes: scale (1 = as modelled) and which
// way each axis grows: 0 to both sides (around the model's pivot), +1 only
// towards + (its - end stays), -1 only towards - (its + end stays)
struct ObjectSize
{
    float scale[3] = { 1.0f, 1.0f, 1.0f };
    int grow[3] = { 0, 0, 0 };
};

// Size of the next object AffObjet draws; put back ObjectSize() afterwards
void setNextObjectSize(const ObjectSize& size);

// The body about to be drawn (for the rules of special bodies)
void setCurrentBodyNum(int bodyNum, sBody* pBody, const std::string& hqrName);

// Projects a model and computes the screen rectangle it covers (BBox3D)
void computeScreenBox(int x, int y, int z, int alpha, int beta, int gamma, sBody* bodyPtr);

#endif
