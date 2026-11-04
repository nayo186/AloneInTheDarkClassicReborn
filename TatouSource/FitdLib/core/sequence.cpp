///////////////////////////////////////////////////////////////////////////////
// Alone In The Dark Re-Haunted
// Copyright (C) 2026 Infogrames / Spacefarer Retro Remasters LLC
// Based on FITD by yaz0r, Re-haunted is released under GPL
// Author: Jake Jackson (jake@spacefarergames.com)
//
// FMV cutscene sequence player
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "consoleLog.h"
#include "settings.h"
#include "coreHooks.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

const char* sequenceListAITD2[]=
{
    "BATL",
    "GRAP",
    "CLE1",
    "CLE2",
    "COOK",
    "EXPL",
    "FALA",
    "FAL2",
    "GLIS",
    "GREN",
    "JEND",
    "MANI",
    "MER",
    "TORD",
    "PANT",
    "VERE",
    "PL21",
    "PL22",
    "ENDX",
    "SORT",
    "EFER",
    "STAR",
    "MEDU",
    "PROL",
    "GRAS",
    "STRI",
    "ITRO",
    "BILL",
    "PIRA",
    "PIR2",
    "VENT",
    "FIN",
    "LAST"
};

const char* sequenceName(int index)
{
    static char name[16];
    if (index < 0)
        return nullptr;
    if (g_gameId == AITD2)
        return index < (int)(sizeof(sequenceListAITD2) / sizeof(sequenceListAITD2[0])) ? sequenceListAITD2[index] : nullptr;
    if (g_gameId == AITD3)
    {
        snprintf(name, sizeof(name), "AN%d", index);
        return name;
    }
    // Jack's only sequence: Santa ("le Père Noël") at the end (BUGS.md J2)
    if (g_gameId == JACK)
        return index == 0 ? "PERE" : nullptr;
    return nullptr;
}

void unapckSequenceFrame(unsigned char* source,unsigned char* dest)
{
    unsigned char byteCode;

    byteCode = *(source++);

    while(byteCode)
    {
        if(!(--byteCode)) // change pixel or skip pixel
        {
            unsigned char changeColor;

            changeColor = *(source++);

            if(changeColor)
            {
                *(dest++) = changeColor;
            }
            else
            {
                dest++;
            }
        }
        else
            if(!(--byteCode)) // change 2 pixels or skip 2 pixels
            {
                unsigned char changeColor;

                changeColor = *(source++);

                if(changeColor)
                {
                    *(dest++) = changeColor;
                    *(dest++) = changeColor;
                }
                else
                {
                    dest+=2;
                }
            }
            else
                if(!(--byteCode)) // fill or skip
                {
                    unsigned char size;
                    unsigned char fillColor;

                    size = *(source++);
                    fillColor = *(source++);

                    if(fillColor)
                    {
                        int i;

                        for(i=0;i<size;i++)
                        {
                            *(dest++) = fillColor;
                        }
                    }
                    else
                    {
                        dest+=size;
                    }
                }
                else // large fill of skip
                {
                    u16 size;
                    unsigned char fillColor;

                    size = READ_LE_U16(source);
                    source+=2;
                    fillColor = *(source++);

                    if(fillColor)
                    {
                        int i;

                        for(i=0;i<size;i++)
                        {
                            *(dest++) = fillColor;
                        }
                    }
                    else
                    {
                        dest+=size;
                    }
                }

                byteCode = *(source++);
    }
}

extern "C" { extern char homePath[512]; }

static const int kSequenceFrameTicks = 4;

void playSequence(int sequenceIdx, int fadeStart, int fadeOutVar)
{

    int frames=0;                   /* Number of frames displayed */

    int var_4 = 1;
    int quitPlayback = 0;
    int nextFrame = 1;
    palette_t seqPalette;           // Palette at outer scope for dump/load access

    char buffer[256];
    const char* name = sequenceName(sequenceIdx);
    if (!name)
    {
        printf(SEQ_TAG "No sequence %d in this game\n", sequenceIdx);
        return;
    }
    strcpy(buffer, name);

    printf(SEQ_TAG "Playing sequence: %s (fade %d, times %d)" CON_RESET "\n", buffer, fadeStart, fadeOutVar);

    int numMaxFrames = PAK_getNumFiles(buffer);

    // Fade flag 1: what is on screen goes to black first. Before composing
    // only pictures: AITD2's Infogrames logo is a camera, and without its
    // scene the fade had nothing to show (BUGS.md A2)
    if (fadeStart & 1)
        FadeOutPhys(0x10, 0);

    // Fade flags 2 and 4: the first frame comes up from black (2 is the
    // cliff after the logo, MER; 4 the rest of AITD2's intro)
    const bool fadeInFirstFrame = (fadeStart & (2 | 4)) != 0;

    // Compose every frame, there is no 3D; without it the last camera stayed
    // on screen and Jack's Santa was never seen (BUGS.md J2)
    const bool savedMenuActive = g_menuActive;
    g_menuActive = true;
    bool skipped = false;

    // The text on screen belongs to the scene, not to the sequence: a use
    // of the captain's staff plays one from the inventory, and its list
    // stayed over the sequence (BUGS.md A46)
    uiLayer.fill(0);

    while(!quitPlayback)
    {
        int currentFrameId = 0;
        int sequenceParamIdx;

        while(currentFrameId < nextFrame)
        {
            frames++;

            timer = timeGlobal;

            if (currentFrameId >= numMaxFrames)
            {
                quitPlayback = 1;
                break;
            }

            if(!LoadPak(buffer,currentFrameId,logicalScreen))
            {
                fatalError(0,buffer);
            }

            if(!currentFrameId) // first frame
            {
                copyPalette(logicalScreen, seqPalette);  // copy palette
                memcpy(aux,logicalScreen+0x300,64000);
                nextFrame = READ_LE_U16(logicalScreen+64768);

                convertPaletteIfRequired(seqPalette);

                if(var_4 != 0)
                {
                    setPalette(seqPalette);
                    copyPalette(seqPalette,currentGamePalette);
                }
            }
            else // not first frame
            {
                U32 frameSize;

                frameSize = READ_LE_U32(logicalScreen);

                if(frameSize < 64000) // key frame
                {
                    unapckSequenceFrame((unsigned char*)logicalScreen+4,(unsigned char*)aux);
                }
                else // delta frame
                {
                    FastCopyScreen(logicalScreen,aux);
                }
            }

            for(sequenceParamIdx = 0; sequenceParamIdx < numSequenceParam; sequenceParamIdx++)
            {
                if(sequenceParams[sequenceParamIdx].frame == currentFrameId)
                {
                    playSound(sequenceParams[sequenceParamIdx].sample);
                }
            }

			// A remade frame of the player replaces it (customBackgrounds.cpp)
			coreSequenceFrameShown(buffer, currentFrameId, (const unsigned char*)aux);
			osystem_CopyBlockPhys((unsigned char*)aux,0,0,320,200);

			// setPalette left the screen lit: not one bright frame before the fade
			if (fadeInFirstFrame && frames == 1)
				osystem_setScreenFade(0.0f);

			osystem_drawBackground();

			if (fadeInFirstFrame && frames == 1)
				FadeInPhys(0x10, 0);

            currentFrameId++;

			// The original waited 5 screen syncs (70 Hz) per frame: about 4
			// ticks of the game clock, whatever the FPS
			int frameTicks = 0;
			while (frameTicks < kSequenceFrameTicks && !key)
			{
				process_events();
				frameTicks += ticksThisFrame;
			}

			if(key)
			{
				//stopSample();
				skipped = true;
				quitPlayback = 1;
				break;
			}

        }

        fadeOutVar--;

        if(fadeOutVar==0)
        {
            quitPlayback=1;
        }
    }

    printf(SEQ_TAG "Sequence %s: %d of %d frames shown%s\n", buffer, frames, numMaxFrames, skipped ? ", skipped with a key" : "");
    g_menuActive = savedMenuActive;
	FlagInitView = 2;
}


// ---- AITD1 ending: the burning tree (END_SEQUENCE) -------------------------
// Never written in FITD (END_SEQUENCE did nothing), so the tree did not burn.
// ENDSEQ.PAK: entry 0 is "VP", a palette and the tree in fire colours; entries
// 1 to 31 are the flames, each packed like a sequence frame
// (unapckSequenceFrame) over the clean tree. Times taken from the GOG game
// (BUGS.md C50), in ticks of the game clock

static const int kEndTreeTicks = 60;       // the tree as the camera shows it, darkening
static const int kEndFireInTicks = 30;     // the fire colours come up
static const int kEndFlameTicks = 4;       // each flame frame
static const int kEndYellowTicks = 15;     // the flames fill the screen with yellow
static const int kEndRedTicks = 60;        // yellow, orange, red
static const int kEndRedHoldTicks = 20;

// Shows the screen for `ticks` ticks, calling step(progress 0..1) before
// each frame
template <typename Step>
static void showEndScreen(int ticks, Step step)
{
    int elapsed = 0;
    while (elapsed < ticks)
    {
        step((float)elapsed / (float)ticks);
        osystem_drawBackground();
        process_events();
        elapsed += ticksThisFrame;
    }
    step(1.0f);
}

// `from` moved towards one colour by `amount` (0..1)
static void paletteTowards(const palette_t& from, const unsigned char* colour, float amount, palette_t& out)
{
    for (int i = 0; i < 256; i++)
        for (int j = 0; j < 3; j++)
            out[i][j] = (unsigned char)(from[i][j] + (colour[j] - from[i][j]) * amount);
}

void playEndSequence()
{
    if (g_gameId != AITD1)
        return;

    const int numFiles = (int)PAK_getNumFiles("ENDSEQ");
    if (numFiles < 2 || getPakSize("ENDSEQ", 0) < 770 + 64000)
    {
        printf(SEQ_TAG "Ending: ENDSEQ.PAK missing\n");
        return;
    }
    char* tree = loadPak("ENDSEQ", 0);
    if (!tree)
        return;
    printf(SEQ_TAG "Ending: the tree burns (%d flame frames)\n", numFiles - 1);

    SaveTimerAnim();
    const bool savedMenuActive = g_menuActive;
    g_menuActive = true;  // compose every frame, there is no 3D

    // The tree as the camera shows it (CAMERA06 picture 7), getting darker
    setPalette(currentGamePalette);
    if (LoadPak("CAMERA06", 7, aux))
    {
        coreScreenPictureShown("CAMERA06", 7);
        osystem_CopyBlockPhys((unsigned char*)aux, 0, 0, 320, 200);
        showEndScreen(kEndTreeTicks, [](float t) { osystem_setScreenFade(1.0f - 0.7f * t); });
    }

    // The tree in fire colours, coming up from the dark
    palette_t firePalette;
    copyPalette(tree + 2, firePalette);
    unsigned char* treePicture = (unsigned char*)tree + 770;
    coreScreenPictureShown("ENDSEQ", 0);
    FastCopyScreen(treePicture, aux);
    osystem_CopyBlockPhys((unsigned char*)aux, 0, 0, 320, 200);
    const unsigned char black[3] = { 0, 0, 0 };
    palette_t palette;
    showEndScreen(kEndFireInTicks, [&](float t) {
        paletteTowards(firePalette, black, 0.7f * (1.0f - t), palette);
        setPalette(palette);
    });

    // The flames, each over the clean tree
    for (int frame = 1; frame < numFiles; frame++)
    {
        char* flames = loadPak("ENDSEQ", frame);
        if (!flames)
            break;
        FastCopyScreen(treePicture, aux);
        unapckSequenceFrame((unsigned char*)flames, (unsigned char*)aux);
        free(flames);
        osystem_CopyBlockPhys((unsigned char*)aux, 0, 0, 320, 200);
        showEndScreen(kEndFlameTicks, [](float) {});
    }

    // The fire fills the screen: yellow, then orange and red
    const unsigned char yellow[3] = { 255, 240, 0 };
    const unsigned char red[3] = { 250, 20, 0 };
    palette_t yellowPalette;
    paletteTowards(firePalette, yellow, 1.0f, yellowPalette);
    showEndScreen(kEndYellowTicks, [&](float t) {
        paletteTowards(firePalette, yellow, t, palette);
        setPalette(palette);
    });
    showEndScreen(kEndRedTicks, [&](float t) {
        paletteTowards(yellowPalette, red, t, palette);
        setPalette(palette);
    });
    showEndScreen(kEndRedHoldTicks, [](float) {});

    free(tree);
    setPalette(currentGamePalette);
    g_menuActive = savedMenuActive;
    RestoreTimerAnim();
    FlagInitView = 1;
}
