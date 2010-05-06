// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

struct sfx_t;
struct channel_t;
struct wavinfo_t;

void S_Init (void);
void S_Startup (void);
void S_Shutdown (void);
void S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol,  float attenuation);
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound (int entnum, int entchannel);
void S_StopAllSounds(qboolean clear);
void S_ClearSoundBuffer (void);
void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
void S_ExtraUpdate (void);

sfx_t *S_PrecacheSound (char *sample);
void S_TouchSound (char *sample);
void S_ClearPrecache (void);
void S_BeginPrecaching (void);
void S_EndPrecaching (void);
void S_InitPaintChannels (void);

// picks a channel based on priorities, empty slots, number of channels
channel_t *SND_PickChannel(int entnum, int entchannel);

// spatializes a channel
void SND_Spatialize(channel_t *ch);

// ====================================================================
// User-setable variables
// ====================================================================

//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

extern qboolean 		fakedma;
extern int 			fakedma_updates;
extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
extern vec_t sound_nominal_clip_dist;

extern	QCvar* bgmvolume;
extern	QCvar* bgmtype;

extern qboolean	snd_initialized;

extern int		snd_blocked;

void S_LocalSound (char *s);

void S_AmbientOff (void);
void S_AmbientOn (void);
void S_UpdateSoundPos (int entnum, int entchannel, vec3_t origin);

#endif
