///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Animation data (frames and bone groups of LISTANIM) and the animation
// functions of anim.cpp
///////////////////////////////////////////////////////////////////////////////

#ifndef _ANIM_H_
#define _ANIM_H_

#define INFO_TRI 1
#define INFO_ANIM 2
#define INFO_TORTUE 4
#define INFO_OPTIMISE 8


struct sFrame
{
    u16 m_timestamp;
    point3dStruct m_animStep;
    std::vector<sGroupState> m_groups;
};

struct sAnimation
{
    void* m_raw;

    u16 m_numFrames;
    u16 m_numGroups;
    std::vector<sFrame> m_frames;
};

extern hqrEntryStruct<sAnimation>* HQ_Anims;

#define NB_BUFFER_ANIM 25 // AITD1 was  20
#define SIZE_BUFFER_ANIM (8*41) // AITD1 was 4*31

extern std::vector<sFrame> BufferAnim;

// Starts an animation on the current object (once, repeated or all once; animInfo: the next one); false if it cannot
int InitAnim(int animNum,int animType, int animInfo);
// Puts a body in the pose of one frame of an animation
int SetAnimObjet(int frame, sAnimation* anim, sBody* body);
// Puts a body in the pose between two frames by the time passed; returns 1 when the frame ends
s16 SetInterAnimObjet(int frame, sAnimation* animPtr, sBody* bodyPtr);
// Number of frames of an animation
s16 GetNbFramesAnim(sAnimation* animPtr);
// Keeps the current pose of a body as the start of the next interpolation
void StockInterAnim(sFrame& animBuffer, sBody* bodyPtr);
// Restarts the interpolation timer of a body
void ResetStartAnim(sBody* bodyPtr);
// Advances the animation of the current object and moves it by its animation step
void GereAnim(void);
// Sets the screens used to copy rectangles (the original's screen pointers)
void InitCopyBox(char* var0, char* var1);


#endif // _ANIM_H_
