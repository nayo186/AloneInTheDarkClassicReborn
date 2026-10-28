///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Software 3D renderer and projection pipeline
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "settings.h"
#include "consoleLog.h"
#include "coreHooks.h"
#include "launchOptions.h"

extern int numSpheresPrimitives;

/* Projection:

 Z += cameraPerspective;
 float transformedX = ((X * cameraFovX) / Z) + cameraCenterX;
 float transformedY = ((Y * cameraFovY) / Z) + cameraCenterY;
 
 {X, Y, Z, 1}
 
 {
    ((X * cameraFovX) / Z) + cameraCenterX
    ((Y * cameraFovY) / Z) + cameraCenterY
    Z + cameraPerspective
    ?
 }
 
 ((X * CameraFovX) + (Z+CameraPersp) * cameraCenterX) / (Z+cameraPersp)
 
 
 
 */


#define NUM_MAX_PRIM_ENTRY 500

primEntryStruct primTable[NUM_MAX_PRIM_ENTRY];

u32 positionInPrimEntry = 0;

// Track which body's primitives are currently in primTable[]
// This allows external code (like lantern glow detection) to verify they're
// scanning the correct body's primitives, not some other body rendered later
int g_currentPrimTableBodyNum = -1;

static int s_currentPrimIndex = 0;
static int s_currentBodyNum = -1;  // Track current rendering body number for special handling

// Near-camera floating polygon clipping thresholds.
// When a model is close to the camera, small detail polygons (eyes, nostrils)
// that are geometrically offset from the face surface toward the camera can
// appear as floating colored patches. These constants control detection.
static const float NEAR_POLY_CLIP_Z          = 300.f;   // Enhanced near-clip zone (camera-space Z)
static const float NEAR_POLY_MAX_SCREEN_FRAC = 0.50f;   // Max screen-space extent as fraction of viewport
static const float NEAR_POLY_MAX_Z_RANGE     = 200.f;   // Max Z spread across a single polygon's vertices

// Post-processing: detect small detail polygons (eyes, nostrils) overlapping
// larger polygons (face). A tiny polygon whose screen bbox fits inside a
// significantly larger polygon's screen bbox is suppressed.
static const float FLOATING_POLY_AREA_MAX    = 500.f;   // Max rest-pose triangle area to consider "detail"
static const float FLOATING_POLY_AREA_RATIO  = 4.0f;    // Containing polygon must be this many times larger
static const float FLOATING_POLY_MAX_EXTENT  = 30.f;    // Max rest-pose bbox extent (any axis) for "detail"

int BBox3D1=0;
int BBox3D2=0;
int BBox3D3=0;
int BBox3D4=0;

int renderVar1=0;

int numOfPrimitiveToRender=0;

char renderBuffer[3261];

char* renderVar2=NULL;

int modelFlags = 0;

int modelCosAlpha;
int modelSinAlpha;
int modelCosBeta;
int modelSinBeta;
int modelCosGamma;
int modelSinGamma;

bool noModelRotation;

int renderX;
int renderY;
int renderZ;

int numOfPoints;
int numOfBones;

std::array<point3dStruct, NUM_MAX_POINT_IN_POINT_BUFFER> pointBuffer;
s16 cameraSpaceBuffer[NUM_MAX_POINT_IN_POINT_BUFFER*3];
s16 bonesBuffer[NUM_MAX_BONES];

bool boneRotateX;
bool boneRotateY;
bool boneRotateZ;

int boneRotateXCos;
int boneRotateXSin;
int boneRotateYCos;
int boneRotateYSin;
int boneRotateZCos;
int boneRotateZSin;

char primBuffer[30000];

int renderVar3;

#ifndef AITD_UE4
void fillpoly(s16 * datas, int n, char c);
#endif

/*

 
 
*/

void transformPoint(float* ax, float* bx, float* cx)
{
    int X = (int)*ax;
    int Y = (int)*bx;
    int Z = (int)*cx;
    {
        int* ax = &X;
        int* bx = &Y;
        int* cx = &Z;

        {
            int x;
            int y;
            int z;

            if(transformUseY)
            {
                x = (((((*ax) * transformYSin) - ((*cx) * transformYCos))) / 0x10000)<<1;
                z = (((((*ax) * transformYCos) + ((*cx) * transformYSin))) / 0x10000)<<1;
            }
            else
            {
                x = (*ax);
                z = (*cx);
            }

            //si = x
            //ax = z

            if(transformUseX)
            {
                int tempY = (*bx);
                int tempZ = z;
                y = ((((tempY * transformXSin ) - (tempZ * transformXCos))) / 0x10000)<<1;
                z = ((((tempY * transformXCos ) + (tempZ * transformXSin))) / 0x10000)<<1;
            }
            else
            {
                y = (*bx);
            }

            // cx = y
            // bx = z

            if(transformUseZ)
            {
                int tempX = x;
                int tempY = y;
                x = ((((tempX * transformZSin) - ( tempY * transformZCos))) / 0x10000)<<1;
                y = ((((tempX * transformZCos) + ( tempY * transformZSin))) / 0x10000)<<1;
            }

            *ax = x;
            *bx = y;
            *cx = z;
        }
    }

    *ax = (float)X;
    *bx = (float)Y;
    *cx = (float)Z;
}

void InitGroupeRot(int transX,int transY,int transZ)
{
    if(transX)
    {
        boneRotateXCos = cosTable[transX&0x3FF];
        boneRotateXSin = cosTable[(transX+0x100)&0x3FF];

        boneRotateX = true;
    }
    else
    {
        boneRotateX = false;
    }

    if(transY)
    {
        boneRotateYCos = cosTable[transY&0x3FF];
        boneRotateYSin = cosTable[(transY+0x100)&0x3FF];

        boneRotateY = true;
    }
    else
    {
        boneRotateY = false;
    }

    if(transZ)
    {
        boneRotateZCos = cosTable[transZ&0x3FF];
        boneRotateZSin = cosTable[(transZ+0x100)&0x3FF];

        boneRotateZ = true;
    }
    else
    {
        boneRotateZ = false;
    }
}

void RotateList(point3dStruct* pointPtr, int numOfPoint)
{
    for(int i=0;i<numOfPoint;i++)
    {
        point3dStruct& point = pointPtr[i];
        int x = point.x;
        int y = point.y;
        int z = point.z;

        if(boneRotateY)
        {
            int tempX = x;
            int tempZ = z;
            x = ((((tempX * boneRotateYSin) - (tempZ * boneRotateYCos)))>>16)<<1;
            z = ((((tempX * boneRotateYCos) + (tempZ * boneRotateYSin)))>>16)<<1;
        }

        if(boneRotateX)
        {
            int tempY = y;
            int tempZ = z;
            y = ((((tempY * boneRotateXSin ) - (tempZ * boneRotateXCos)))>>16)<<1;
            z = ((((tempY * boneRotateXCos ) + (tempZ * boneRotateXSin)))>>16)<<1;
        }

		if(boneRotateZ)
		{
			int tempX = x;
			int tempY = y;
			x = ((((tempX * boneRotateZSin) - ( tempY * boneRotateZCos)))>>16)<<1;
			y = ((((tempX * boneRotateZCos) + ( tempY * boneRotateZSin)))>>16)<<1;
		}

        point.x = x;
        point.y = y;
        point.z = z;
    }
}

void RotateGroupeOptimise(sGroup* ptr)
{
    if (ptr->m_numGroup) // if group number is 0
    {
        int baseBone = ptr->m_start;
        int numPoints = ptr->m_numVertices;

        RotateList(pointBuffer.data() + baseBone, numPoints);
    }
}

void RotateGroupe(sGroup* ptr)
{
    int baseBone = ptr->m_start;
    int numPoints = ptr->m_numVertices;
    int temp;
    int temp2;

    RotateList(pointBuffer.data() + baseBone, numPoints);

    temp = ptr->m_numGroup; // group number

    temp2 = numOfBones - temp;

    do
    {
        if (ptr->m_orgGroup == temp) // is it on of this group child
        {
            RotateGroupe(ptr); // yes, so apply the transformation to him
        }

        ptr++;
    } while (--temp2);
}

void TranslateGroupe(int transX, int transY, int transZ, sGroup* ptr)
{
    for (int i = 0; i < ptr->m_numVertices; i++)
    {
        point3dStruct& point = pointBuffer[ptr->m_start + i];
        point.x += transX;
        point.y += transY;
        point.z += transZ;
    }
}

void ZoomGroupe(int zoomX, int zoomY, int zoomZ, sGroup* ptr)
{
    for (int i = 0; i < ptr->m_numVertices; i++)
    {
        point3dStruct& point = pointBuffer[ptr->m_start + i];
        point.x = (point.x * (zoomX + 256)) / 256;
        point.y = (point.y * (zoomY + 256)) / 256;
        point.z = (point.z * (zoomZ + 256)) / 256;
    }
}

// Size of the next object drawn (renderer.h). Not in the 1992 renderer: the
// object editor's visual fixes set it before AffObjet and put it back after
// (core/sceneDraw.cpp).
static ObjectSize s_objectSize;
static float s_scaleAnchor[3] = {};   // the point of each axis that does not move

void setNextObjectSize(const ObjectSize& size)
{
    s_objectSize = size;
}

static bool objectScaled()
{
    return s_objectSize.scale[0] != 1.0f || s_objectSize.scale[1] != 1.0f || s_objectSize.scale[2] != 1.0f;
}

// Where each axis is fixed: the pivot (grows to both sides) or the end of the
// model opposite to where it grows
template <typename Point>
static void computeScaleAnchors(const Point* points, int count)
{
    for (int axis = 0; axis < 3; axis++)
    {
        s_scaleAnchor[axis] = 0.0f;
        if (s_objectSize.grow[axis] == 0 || count <= 0)
            continue;
        float low = 1e9f, high = -1e9f;
        for (int i = 0; i < count; i++)
        {
            const float value = (axis == 0) ? points[i].x : (axis == 1) ? points[i].y : points[i].z;
            low = (std::min)(low, value);
            high = (std::max)(high, value);
        }
        s_scaleAnchor[axis] = (s_objectSize.grow[axis] > 0) ? low : high;
    }
}

static float scaledCoordinate(float value, int axis)
{
    return s_scaleAnchor[axis] + (value - s_scaleAnchor[axis]) * s_objectSize.scale[axis];
}

int AnimNuage(int x,int y,int z,int alpha,int beta,int gamma, sBody* pBody)
{
    renderX = x - translateX;
    renderY = y;
    renderZ = z - translateZ;

    ASSERT(pBody->m_vertices.size()<NUM_MAX_POINT_IN_POINT_BUFFER);

    for (int i = 0; i < pBody->m_vertices.size(); i++)
    {
        pointBuffer[i] = pBody->m_vertices[i];
    }

    numOfPoints = pBody->m_vertices.size();
    numOfBones = pBody->m_groupOrder.size();
    ASSERT(numOfBones<NUM_MAX_BONES);

    if(pBody->m_flags & INFO_OPTIMISE)
    {
        for(int i=0;i<pBody->m_groupOrder.size();i++)
        {
            int boneDataOffset = pBody->m_groupOrder[i];
            sGroup* pGroup = &pBody->m_groups[pBody->m_groupOrder[i]];

            switch(pGroup->m_state.m_type)
            {
            case 1:
                if(pGroup->m_state.m_delta.x || pGroup->m_state.m_delta.y || pGroup->m_state.m_delta.z)
                {
                    TranslateGroupe(pGroup->m_state.m_delta.x, pGroup->m_state.m_delta.y, pGroup->m_state.m_delta.z, pGroup);
                }
                break;
            case 2:
                if (pGroup->m_state.m_delta.x || pGroup->m_state.m_delta.y || pGroup->m_state.m_delta.z)
                {
                    ZoomGroupe(pGroup->m_state.m_delta.x, pGroup->m_state.m_delta.y, pGroup->m_state.m_delta.z, pGroup);
                }
                break;
            }

            InitGroupeRot(pGroup->m_state.m_rotateDelta.value().x, pGroup->m_state.m_rotateDelta.value().y, pGroup->m_state.m_rotateDelta.value().z);
            RotateGroupeOptimise(pGroup);
        }
    }
    else
    {
        pBody->m_groups[0].m_state.m_delta.x = alpha;
        pBody->m_groups[0].m_state.m_delta.y = beta;
        pBody->m_groups[0].m_state.m_delta.z = gamma;

        for(int i=0;i<pBody->m_groups.size();i++)
        {
            int boneDataOffset = pBody->m_groupOrder[i];
            sGroup* pGroup = &pBody->m_groups[pBody->m_groupOrder[i]];

            int transX = pGroup->m_state.m_delta.x;
            int transY = pGroup->m_state.m_delta.y;
            int transZ = pGroup->m_state.m_delta.z;

            if(transX || transY || transZ)
            {
                switch(pGroup->m_state.m_type)
                {
                case 0:
                    { 
                        InitGroupeRot(transX,transY,transZ);
                        RotateGroupe(pGroup);
                        break;
                    }
                case 1:
                    {
                        TranslateGroupe(transX,transY,transZ, pGroup);
                        break;
                    }
                case 2:
                    {
                        ZoomGroupe(transX,transY,transZ, pGroup);
                        break;
                    }
                }
            }
        }
    }

    for(int i=0;i<pBody->m_groups.size();i++)
    {
        sGroup* pGroup = &pBody->m_groups[i];
        for(int j=0;j< pGroup->m_numVertices;j++)
        {
            pointBuffer[pGroup->m_start + j].x += pointBuffer[pGroup->m_baseVertices].x;
            pointBuffer[pGroup->m_start + j].y += pointBuffer[pGroup->m_baseVertices].y;
            pointBuffer[pGroup->m_start + j].z += pointBuffer[pGroup->m_baseVertices].z;
        }
    }

    // Scaled after the animation, before the whole model is turned
    if (objectScaled())
    {
        computeScaleAnchors(pointBuffer.data(), numOfPoints);
        for (int i = 0; i < numOfPoints; i++)
        {
            pointBuffer[i].x = (s16)scaledCoordinate(pointBuffer[i].x, 0);
            pointBuffer[i].y = (s16)scaledCoordinate(pointBuffer[i].y, 1);
            pointBuffer[i].z = (s16)scaledCoordinate(pointBuffer[i].z, 2);
        }
    }

    if(modelFlags & INFO_OPTIMISE)
    {
        InitGroupeRot(alpha,beta,gamma);
        RotateList(pointBuffer.data(),numOfPoints);
    }

    {
        s16* outPtr = cameraSpaceBuffer;
        for(int i=0;i<numOfPoints;i++)
        {
            float X = pointBuffer[i].x;
            float Y = pointBuffer[i].y;
            float Z = pointBuffer[i].z;


            X += renderX;
            Y += renderY;
            Z += renderZ;

#if defined(AITD_UE4)
            *(outPtr++) = (s16)X;
            *(outPtr++) = (s16)Y;
            *(outPtr++) = (s16)Z;
#else
            if(Y>10000) // height clamp
            {
                *(outPtr++) = -10000;
                *(outPtr++) = -10000;
                *(outPtr++) = -10000;
            }
            else
            {
                Y -= translateY;

                transformPoint(&X,&Y,&Z);

                *(outPtr++) = (s16)X;
                *(outPtr++) = (s16)Y;
                *(outPtr++) = (s16)Z;
            }
#endif
        }

        s16* ptr = cameraSpaceBuffer;
        float* outPtr2 = renderPointList;

        int k = numOfPoints;
        do
        {
            float X;
            float Y;
            float Z;

            X = ptr[0];
            Y = ptr[1];
            Z = ptr[2];
            ptr+=3;

#if defined(AITD_UE4)
            *(outPtr2++) = X;
            *(outPtr2++) = Y;
            *(outPtr2++) = Z;
#else
            Z += cameraPerspective;

            if( Z<=50 ) // clipping
            {
                *(outPtr2++) = -10000;
                *(outPtr2++) = -10000;
                *(outPtr2++) = -10000;
            }
            else
            {
                float transformedX = ((X * cameraFovX) / Z) + cameraCenterX;
                float transformedY;

                *(outPtr2++) = transformedX;

                if(transformedX < BBox3D1)
                    BBox3D1 = (int)transformedX;

                if(transformedX > BBox3D3)
                    BBox3D3 = (int)transformedX;

                transformedY = ((Y * cameraFovY) / Z) + cameraCenterY;

                *(outPtr2++) = transformedY;

                if(transformedY < BBox3D2)
                    BBox3D2 = (int)transformedY;

                if(transformedY > BBox3D4)
                    BBox3D4 = (int)transformedY;

                *(outPtr2++) = Z; 
            }
#endif

            k--;
            if(k==0)
            {
                return(1);
            }

        }while(renderVar1 == 0);
    }

    return(0);
}

/*
x - cameraX
y
z - cameraZ
 
*/

int RotateNuage(int x,int y,int z,int alpha,int beta,int gamma, sBody* pBody)
{
    float* outPtr;

    renderX = x - translateX;
    renderY = y;
    renderZ = z - translateZ;

    if(!alpha && !beta && !gamma)
    {
        noModelRotation = true;
    }
    else
    {
        noModelRotation = false;

        modelCosAlpha = cosTable[alpha&0x3FF];
        modelSinAlpha = cosTable[(alpha+0x100)&0x3FF];

        modelCosBeta = cosTable[beta&0x3FF];
        modelSinBeta = cosTable[(beta+0x100)&0x3FF];

        modelCosGamma = cosTable[gamma&0x3FF];
        modelSinGamma = cosTable[(gamma+0x100)&0x3FF];
    }

    outPtr = renderPointList;

    const bool scaled = objectScaled();
    if (scaled)
        computeScaleAnchors(pBody->m_vertices.data(), (int)pBody->m_vertices.size());

    for(int i=0; i<pBody->m_vertices.size(); i++)
    {
        // Scaled before the model is turned
        float X = pBody->m_vertices[i].x;
        float Y = pBody->m_vertices[i].y;
        float Z = pBody->m_vertices[i].z;
        if (scaled)
        {
            X = scaledCoordinate(X, 0);
            Y = scaledCoordinate(Y, 1);
            Z = scaledCoordinate(Z, 2);
        }

        if(!noModelRotation)
        {
			// Y rotation
			{
				float tempX = X;
				float tempZ = Z;

				X = (((modelSinBeta * tempX) - (modelCosBeta * tempZ))/65536.f)*2.f;
				Z = (((modelCosBeta * tempX) + (modelSinBeta * tempZ))/65536.f)*2.f;
			}

			// Z rotation
            {
                float tempX = X;
                float tempY = Y;

                X = (((modelSinGamma * tempX) - (modelCosGamma * tempY))/65536.f)*2.f;
                Y = (((modelCosGamma * tempX) + (modelSinGamma * tempY))/65536.f)*2.f;
            }

			// X rotation
            {
                float tempY = Y;
                float tempZ = Z;

                Y = (((modelSinAlpha * tempY) - (modelCosAlpha * tempZ))/65536.f)*2.f;
                Z = (((modelCosAlpha * tempY) + (modelSinAlpha * tempZ))/65536.f)*2.f;
            }
        }

        X += renderX;
        Y += renderY;
        Z += renderZ;

#if defined(AITD_UE4)
        *(outPtr++) = X;
        *(outPtr++) = Y;
        *(outPtr++) = Z;
#else
        if(Y>10000) // height clamp
        {
            *(outPtr++) = -10000;
            *(outPtr++) = -10000;
            *(outPtr++) = -10000;
        }
        else
        {
            float transformedX;
            float transformedY;

            Y -= translateY;

            transformPoint(&X,&Y,&Z);

            Z += cameraPerspective;

            transformedX = ((X * cameraFovX) / Z) + cameraCenterX;

            *(outPtr++) = transformedX;

            if(transformedX < BBox3D1)
                BBox3D1 = (int)transformedX;

            if(transformedX > BBox3D3)
                BBox3D3 = (int)transformedX;

            transformedY = ((Y * cameraFovY) / Z) + cameraCenterY;

            *(outPtr++) = transformedY;

            if(transformedY < BBox3D2)
                BBox3D2 = (int)transformedY;

            if(transformedY > BBox3D4)
                BBox3D4 = (int)transformedY;

            *(outPtr++) = Z; 

            /*  *(outPtr++) = X;
            *(outPtr++) = Y;
            *(outPtr++) = Z; */

        }
#endif
    }
    return(1);
}

char* primVar1;
char* primVar2;

void processPrim_Line(int primType, sPrimitive* ptr, char** out)
{
    primEntryStruct* pCurrentPrimEntry = &primTable[positionInPrimEntry];

    ASSERT(positionInPrimEntry < NUM_MAX_PRIM_ENTRY);

    pCurrentPrimEntry->type = primTypeEnum_Line;
    pCurrentPrimEntry->numOfVertices = 2;
    pCurrentPrimEntry->color = ptr->m_color;
    pCurrentPrimEntry->material = ptr->m_material;

    float depth = 32000.f;
    ASSERT(pCurrentPrimEntry->numOfVertices < NUM_MAX_VERTEX_IN_PRIM);
    for (int i = 0; i < pCurrentPrimEntry->numOfVertices; i++)
    {
        u16 pointIndex;

        pointIndex = ptr->m_points[i] * 6;

        ASSERT((pointIndex % 2) == 0);

        pCurrentPrimEntry->vertices[i].X = renderPointList[pointIndex / 2];
        pCurrentPrimEntry->vertices[i].Y = renderPointList[(pointIndex / 2) + 1];
        pCurrentPrimEntry->vertices[i].Z = renderPointList[(pointIndex / 2) + 2];

        if (pCurrentPrimEntry->vertices[i].Z < depth)
            depth = pCurrentPrimEntry->vertices[i].Z;

    }

#if !defined(AITD_UE4)
    // Exception: Lamp body (body 11) must never be depth-culled.
    // JACK mode uses body 11 for Grace (the player), not a lamp, so the
    // exception must not apply there.
    const int LAMP_BODY_NUM = 11;
    bool isLampBody = (g_gameId != JACK && s_currentBodyNum == LAMP_BODY_NUM);

    if (depth > 100 || isLampBody)
#endif
    {
        positionInPrimEntry++;

        numOfPrimitiveToRender++;
        ASSERT(positionInPrimEntry < NUM_MAX_PRIM_ENTRY);
    }
}

void processPrim_Poly(int primType, sPrimitive* ptr, char** out, int originalPrimIndex)
{
    primEntryStruct* pCurrentPrimEntry = &primTable[positionInPrimEntry];

    ASSERT(positionInPrimEntry < NUM_MAX_PRIM_ENTRY);

    // Preserve original primitive type (important for distinguishing flat vs textured polys)
    pCurrentPrimEntry->type = (primTypeEnum)primType;
    pCurrentPrimEntry->numOfVertices = ptr->m_points.size();
    pCurrentPrimEntry->color = ptr->m_color;
    pCurrentPrimEntry->material = ptr->m_material;
    pCurrentPrimEntry->originalPrimIndex = originalPrimIndex;  // Set directly to avoid indexing issues with depth culling

    // Track if this is a ramp-shaded primitive (material 3-6)
    pCurrentPrimEntry->isRampPrim = (primType == primTypeEnum_Poly && ptr->m_material >= 3 && ptr->m_material <= 6);

    float depth = 32000.f;
    ASSERT(pCurrentPrimEntry->numOfVertices < NUM_MAX_VERTEX_IN_PRIM);
    for (int i = 0; i < pCurrentPrimEntry->numOfVertices; i++)
    {
        u16 pointIndex;

        pointIndex = ptr->m_points[i] * 6;

        ASSERT((pointIndex % 2) == 0);

        pCurrentPrimEntry->vertices[i].X = renderPointList[pointIndex / 2];
        pCurrentPrimEntry->vertices[i].Y = renderPointList[(pointIndex / 2) + 1];
        pCurrentPrimEntry->vertices[i].Z = renderPointList[(pointIndex / 2) + 2];

        if (pCurrentPrimEntry->vertices[i].Z < depth)
            depth = pCurrentPrimEntry->vertices[i].Z;

    }

#if !defined(AITD_UE4)
    // Exception: Lamp body (body 11) must never be depth-culled or clipped.
    // JACK mode uses body 11 for Grace (the player), not a lamp, so the
    // exception must not apply there.
    const int LAMP_BODY_NUM = 11;
    bool isLampBody = (g_gameId != JACK && s_currentBodyNum == LAMP_BODY_NUM);

    if (depth > 100 || isLampBody)
#endif
    {
        bool clipPoly = false;

        // Enhanced near-camera polygon clipping:
        // Detect polygons close to the camera that are severely distorted
        // by perspective or span the near-plane region.
        // Exception: Lamp body (body 11) needs relaxed clipping so glow detection works.
        // JACK mode uses body 11 for Grace (the player), not a lamp, so the
        // exception must not apply there.
        bool isLampBody = (g_gameId != JACK && s_currentBodyNum == LAMP_BODY_NUM);

        if (!clipPoly)
        {
            positionInPrimEntry++;

            numOfPrimitiveToRender++;
            ASSERT(positionInPrimEntry < NUM_MAX_PRIM_ENTRY);
        }
    }
}

void processPrim_Point(primTypeEnum primType, sPrimitive* ptr, char** out)
{
    primEntryStruct* pCurrentPrimEntry = &primTable[positionInPrimEntry];

    ASSERT(positionInPrimEntry < NUM_MAX_PRIM_ENTRY);

    pCurrentPrimEntry->type = primType;
    pCurrentPrimEntry->numOfVertices = 1;
    pCurrentPrimEntry->color = ptr->m_color;
    pCurrentPrimEntry->material = ptr->m_material;

    float depth = 32000.f;
    {
        u16 pointIndex;
        pointIndex = ptr->m_points[0] * 6;
        ASSERT((pointIndex % 2) == 0);
        pCurrentPrimEntry->vertices[0].X = renderPointList[pointIndex / 2];
        pCurrentPrimEntry->vertices[0].Y = renderPointList[(pointIndex / 2) + 1];
        pCurrentPrimEntry->vertices[0].Z = renderPointList[(pointIndex / 2) + 2];

        depth = pCurrentPrimEntry->vertices[0].Z;
    }

#if !defined(AITD_UE4)
    // Exception: Lamp body (body 11) must never be depth-culled
    const int LAMP_BODY_NUM = 11;
    bool isLampBody = (s_currentBodyNum == LAMP_BODY_NUM);

    if (depth > 100 || isLampBody)
#endif
    {
        positionInPrimEntry++;

        numOfPrimitiveToRender++;
        ASSERT(positionInPrimEntry < NUM_MAX_PRIM_ENTRY);
    }
}

void processPrim_Sphere(int primType, sPrimitive* ptr, char** out)
{
    primEntryStruct* pCurrentPrimEntry = &primTable[positionInPrimEntry];

    ASSERT(positionInPrimEntry < NUM_MAX_PRIM_ENTRY);

    pCurrentPrimEntry->type = primTypeEnum_Sphere;
    pCurrentPrimEntry->numOfVertices = 1;
    pCurrentPrimEntry->color = ptr->m_color;
    pCurrentPrimEntry->material = ptr->m_material;
    pCurrentPrimEntry->size = ptr->m_size;

    float depth = 32000.f;
    {
        u16 pointIndex;
        pointIndex = ptr->m_points[0] * 6;
        ASSERT((pointIndex % 2) == 0);
        pCurrentPrimEntry->vertices[0].X = renderPointList[pointIndex / 2];
        pCurrentPrimEntry->vertices[0].Y = renderPointList[(pointIndex / 2) + 1];
        pCurrentPrimEntry->vertices[0].Z = renderPointList[(pointIndex / 2) + 2];

        depth = pCurrentPrimEntry->vertices[0].Z;
    }

#if !defined(AITD_UE4)
    // Sphere primitives (e.g. character eyes) are always depth-culled normally.
    // The lamp-body exception does NOT apply here — sphere prims in body 11 are
    // character eye spheres, not the lamp itself (which is polygon-based).
    if (depth > 100)
#endif
    {
        positionInPrimEntry++;

        numOfPrimitiveToRender++;
        ASSERT(positionInPrimEntry < NUM_MAX_PRIM_ENTRY);
    }
}

void primType5(int primType, char** ptr, char** out) // draw out of hardClip
{
	printf(RNDR_WARN "ignoring prim type 5" CON_RESET "\n");
	return;

    int pointNumber;
    s16 ax2;

    primVar1 = *out;

    *(s16*)(*out) = *(s16*)(*ptr);
    *out+=2;
    *ptr+=3;

    pointNumber = *(s16*)(*ptr);
    *ptr+=2;

    // here, should check for clip on X Y Z

    *(float*)(*out) = renderPointList[pointNumber/2]; // X
    *out+=sizeof(float);
    *(float*)(*out) = renderPointList[pointNumber/2+1]; // Y
    *out+=sizeof(float);
    *(float*)(*out) = renderPointList[pointNumber/2+2]; // Z
    ax2 = (s16)(*(float*)(*out));
    *out+=sizeof(float);

    primVar2 = *out;

    {
        numOfPrimitiveToRender++;

        *out = renderVar2;

        *(s16*)(*out) = ax2;
        *out+=2;
        *(s16*)(*out) = ax2;
        *out+=2;
        *(s16*)(*out) = primType;
        *out+=2;

        *(char**)(*out) = primVar1;
        *out+=4;

        renderVar2 = *out;
        *out = primVar2;
    }
}

void line(int x1, int y1, int x2, int y2, char c);

void renderLine(primEntryStruct* pEntry) // line
{
    osystem_draw3dLine( pEntry->vertices[0].X,pEntry->vertices[0].Y,pEntry->vertices[0].Z,
						pEntry->vertices[1].X,pEntry->vertices[1].Y,pEntry->vertices[1].Z,
						pEntry->color);
}

void renderPoly(primEntryStruct* pEntry) // poly
{
    // Multi-layered rendering: always render the flat base color first, then overlay
    // the atlas texture on top. Transparent atlas pixels naturally let the base show through.

    // Always render the base material layer first
    osystem_fillPoly((float*)pEntry->vertices, pEntry->numOfVertices, pEntry->color, pEntry->material);

}

void renderZixel(primEntryStruct* pEntry) // point
{
    static float pointSize = 20.f;
    float transformedSize = ((pointSize * (float)cameraFovX) / (float)(pEntry->vertices[0].Z+cameraPerspective));

    osystem_drawPoint(pEntry->vertices[0].X,pEntry->vertices[0].Y,pEntry->vertices[0].Z,pEntry->color,pEntry->material, transformedSize);
}

void renderPoint(primEntryStruct* pEntry) // point
{
    static float pointSize = 0.3f; // TODO: better way to compute that?
    osystem_drawPoint(pEntry->vertices[0].X,pEntry->vertices[0].Y,pEntry->vertices[0].Z,pEntry->color, pEntry->material, pointSize);
}

void renderBigPoint(primEntryStruct* pEntry) // point
{
    static float bigPointSize = 1.f; // TODO: better way to compute that?
    osystem_drawPoint(pEntry->vertices[0].X,pEntry->vertices[0].Y,pEntry->vertices[0].Z,pEntry->color, pEntry->material, bigPointSize);
}

void renderSphere(primEntryStruct* pEntry) // sphere
{
    float transformedSize;

    transformedSize = (((float)pEntry->size * (float)cameraFovX) / (float)(pEntry->vertices[0].Z+cameraPerspective));

    // NOTE: The atlas cell index counter is advanced inside the BGFX sphere
    // backend, so we must NOT increment it here too.

    osystem_drawSphere(pEntry->vertices[0].X,pEntry->vertices[0].Y,pEntry->vertices[0].Z,pEntry->color, pEntry->material, transformedSize);
}


typedef void (*renderFunction)(primEntryStruct* buffer);

renderFunction renderFunctions[]={
	renderLine, // line (0)
	renderPoly, // poly (1)
	renderPoint, // point (2)
	renderSphere, // sphere (3)
	nullptr, // disk (4)
	nullptr, // cylinder (5)
	renderBigPoint, // bigpoint (6)
	renderZixel, // zixel (7)
	nullptr, // polyTexture8 (8) - unsupported
	renderPoly, // polyTexture9 (9) - textured polygon
	renderPoly, // polyTexture10 (10) - textured polygon
};

void setCurrentBodyNum(int bodyNum, sBody* pBody, const std::string& hqrName)
{
    s_currentBodyNum = bodyNum;  // Track current body for special rendering rules
}

int AffObjet(int x,int y,int z,int alpha,int beta,int gamma, sBody* pBody)
{
    if (!pBody)
        return 2;

    int numPrim;
    int i;
    char* out;

    // reinit the 2 static tables
    positionInPrimEntry = 0;
    // Clear primitive table to ensure fields are initialized
    memset(primTable, 0, sizeof(primTable));
    // Reset tracking variable when clearing table - the old body's primitives are gone
    g_currentPrimTableBodyNum = -1;
    //

    BBox3D1 = 0x7FFF;
    BBox3D2 = 0x7FFF;

    BBox3D3 = -0x7FFF;
    BBox3D4 = -0x7FFF;

    renderVar1 = 0;

    numOfPrimitiveToRender = 0;

    renderVar2 = renderBuffer;

    modelFlags = pBody->m_flags;

    if(modelFlags&INFO_ANIM)
    {
        if(!AnimNuage(x,y,z,alpha,beta,gamma, pBody))
        {
            BBox3D3 = -32000;
            BBox3D4 = -32000;
            BBox3D1 = 32000;
            BBox3D2 = 32000;
            g_currentPrimTableBodyNum = -1;  // Reset primTable body tracking
            s_currentBodyNum = -1;  // Reset body tracking before returning
            return(2);
        }
    }
    else
        if(!(modelFlags&INFO_TORTUE))
        {
            if(!RotateNuage(x,y,z,alpha,beta,gamma, pBody))
            {
                BBox3D3 = -32000;
                BBox3D4 = -32000;
                BBox3D1 = 32000;
                BBox3D2 = 32000;
                g_currentPrimTableBodyNum = -1;  // Reset primTable body tracking
                s_currentBodyNum = -1;  // Reset body tracking before returning
                return(2);
            }
        }
        else
        {
            printf(RNDR_WARN "unsupported model type prerenderFlag4 in renderer !" CON_RESET "\n");

            BBox3D3 = -32000;
            BBox3D4 = -32000;
            BBox3D1 = 32000;
            BBox3D2 = 32000;
            g_currentPrimTableBodyNum = -1;  // Reset primTable body tracking
            s_currentBodyNum = -1;  // Reset body tracking before returning
            return(2);
        }

        numPrim = pBody->m_primitives.size();

        if(!numPrim)
        {
            BBox3D3 = -32000;
            BBox3D4 = -32000;
            BBox3D1 = 32000;
            BBox3D2 = 32000;
            g_currentPrimTableBodyNum = -1;  // Reset primTable body tracking
            s_currentBodyNum = -1;  // Reset body tracking before returning
            return(2);
        }

		out = primBuffer;

		// create the list of all primitives to render
		for(i=0;i<numPrim;i++)
		{
			sPrimitive* pPrimitive = &pBody->m_primitives[i];
			primTypeEnum primType = pPrimitive->m_type;

			u32 prevPos = positionInPrimEntry;

			switch(primType)
			{
			case primTypeEnum_Line:
				processPrim_Line(primType, pPrimitive,&out);
				break;
			case primTypeEnum_Poly:
				processPrim_Poly(primType, pPrimitive, &out, i);
				break;
			case primTypeEnum_Point:
			case primTypeEnum_BigPoint:
			case primTypeEnum_Zixel:
				processPrim_Point(primType, pPrimitive,&out);
				break;
			case primTypeEnum_Sphere:
				processPrim_Sphere(primType, pPrimitive,&out);
				break;
			case processPrim_PolyTexture9:
			case processPrim_PolyTexture10:
				processPrim_Poly(primType, pPrimitive, &out, i);
				break;
			default:
				g_currentPrimTableBodyNum = -1;  // Reset primTable body tracking
				s_currentBodyNum = -1;  // Reset body tracking before returning
				return 0;
				assert(0);
			}

			// Track which original primitive this entry came from (for atlas UV lookup)
			// NOTE: originalPrimIndex is now set directly in processPrim_Poly to avoid
			// index mismatches when depth culling filters out primitives
			if (positionInPrimEntry > prevPos)
			{
				// Verify the index was set correctly (should be set for poly types)
				ASSERT(primTable[prevPos].originalPrimIndex == i || 
					   (primType != primTypeEnum_Poly && primType != processPrim_PolyTexture9 && primType != processPrim_PolyTexture10));
			}
		}

		if(!numOfPrimitiveToRender)
		{
			BBox3D3 = -32000;
			BBox3D4 = -32000;
			BBox3D1 = 32000;
			BBox3D2 = 32000;
			g_currentPrimTableBodyNum = -1;  // Reset primTable body tracking
			s_currentBodyNum = -1;  // Reset body tracking before returning
			return(1); // model ok, but out of screen
		}

		// --- Floating detail polygon detection ---
		// Detect small detail polygons (eyes, nostrils) that overlap larger
		// polygons (face). Uses rest-pose model-space area to identify tiny
		// polygons, then checks screen-space bbox containment to confirm they
		// sit within a larger polygon. Works at any distance and camera angle.
		//
		// Key discriminator: vertex isolation. Floating geometry (eyes) uses
		// vertices that no other polygon in the model references. Connected
		// mesh polygons (lips, chin, cheek) share vertices with neighbors.
		//
		// Only apply in HD mode; in non-HD mode keep the original model
		// intact so eyes remain visible.
		//
		// Exception: NEVER clip lamp body (body 11) - glow detection needs all primitives!
		// JACK mode uses body 11 for Grace (the player), not a lamp, so the
		// exception must not apply there.
		const int LAMP_BODY_NUM = 11;
		bool isLampBody = (g_gameId != JACK && s_currentBodyNum == LAMP_BODY_NUM);

		// Track which body's primitives are now in primTable[] (for external code like lantern glow detection)
		g_currentPrimTableBodyNum = s_currentBodyNum;

				// Render primitives in order
		for(i=0;i<numOfPrimitiveToRender;i++)
		{
			if (primTable[i].nearClipped)
				continue;
			int type = primTable[i].type;
			if(type >= 0 && type < (int)(sizeof(renderFunctions)/sizeof(renderFunctions[0])) && renderFunctions[type])
			{
				renderFunctions[type](&primTable[i]);
			}
		}

		osystem_flushPendingPrimitives();
		// DON'T reset g_currentPrimTableBodyNum here! It needs to stay set so external
		// code (like lantern glow) can check which body's primitives are in primTable[].
		// We reset it when clearing primTable[] at the start of the next AffObjet() call.
		s_currentBodyNum = -1;  // Reset body tracking after successful render
		return(0);
}

void computeScreenBox(int x, int y, int z, int alpha, int beta, int gamma, sBody* bodyPtr)
{
    BBox3D1 = 0x7FFF;
    BBox3D2 = 0x7FFF;

    BBox3D3 = -0x7FFF;
    BBox3D4 = -0x7FFF;

    renderVar1 = 0;

    numOfPrimitiveToRender = 0;

    renderVar2 = renderBuffer;

    modelFlags = bodyPtr->m_flags;

    if(modelFlags&INFO_ANIM)
    {
        AnimNuage(x,y,z,alpha,beta,gamma, bodyPtr);
    }
}
