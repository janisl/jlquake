// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

void S_Init (void);
void S_Shutdown (void);

void S_InitPaintChannels (void);

// ====================================================================
// User-setable variables
// ====================================================================

extern	QCvar* bgmvolume;
extern	QCvar* bgmtype;

#endif
