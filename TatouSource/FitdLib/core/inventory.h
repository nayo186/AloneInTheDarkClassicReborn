///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// Inventory system declarations
///////////////////////////////////////////////////////////////////////////////


// Room of each inventory: 50, as in the saves of AITD2's and AITD3's
// originals; each game takes up to its own limit (inventoryCapacity)
#define INVENTORY_SIZE      50
#define NUM_MAX_INVENTORY	2
extern s16 currentInventory;
extern s16 numObjInInventoryTable[NUM_MAX_INVENTORY];
extern s16 inHandTable[NUM_MAX_INVENTORY];
extern s16 inventoryTable[NUM_MAX_INVENTORY][INVENTORY_SIZE];

extern int statusLeft;
extern int statusTop;
extern int statusRight;
extern int statusBottom;

// The inventory screen: choose an object and an action
void processInventory(void);
// How many objects an inventory takes before "You cannot carry anything
// else": 30, and in AITD3 40 as its original (AITD3.EXE, 0xD567)
int inventoryCapacity();
// How many objects an inventory can hold at all, as the game's saves keep
// them: 30 in AITD1 and Jack, 50 in AITD2 and AITD3
int inventoryTableSize();
