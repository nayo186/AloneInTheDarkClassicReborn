///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Hybrid 2D/3D object declarations
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "sprite.h"

struct sHybrid_EntityPart {
    uint8_t id;
    uint8_t nbpt;
    uint8_t cout;
    uint8_t cin;
    std::vector<std::array<int16_t, 2>> XY;
};

struct sHybrid_Entity {
    uint16_t flags;

    std::vector<sHybrid_EntityPart> parts;
};

struct sHybrid_Anim {
    uint16_t id;
    uint16_t flag;
    int16_t deltaX;
    int16_t deltaY;
};

struct sHybrid_Animation {
    uint8_t flag;
    uint8_t count;
    std::vector<sHybrid_Anim> anims;
};

struct sHybrid {
    sHybrid(uint8_t* buffer, int size);

    std::vector<sHybrid_Entity> entities;
    std::vector<sHybrid_Sprite> sprites;
    std::vector<sHybrid_Animation> animations;

private:
    // Reads the entities of a hybrid object (AITD2 and later)
    void readEntites(uint8_t* buffer, int size);
    // Reads the sprites of a hybrid object
    void readSprites(uint8_t* buffer, int size);
    // Reads the animations of a hybrid object
    void readAnimations(uint8_t* buffer, int size);
};

extern hqrEntryStruct<sHybrid>* HQ_Hybrides;

// Draws entity `index` of a hybrid object at (X, Y)
void AffHyb(int index, int X, int Y, sHybrid* pHybrid);