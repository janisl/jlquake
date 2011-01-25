// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

void S_Init (void);
void S_Shutdown (void);
void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
void S_ExtraUpdate (void);

void S_InitPaintChannels (void);

// ====================================================================
// User-setable variables
// ====================================================================

extern	QCvar* bgmvolume;
extern	QCvar* bgmtype;

#endif
