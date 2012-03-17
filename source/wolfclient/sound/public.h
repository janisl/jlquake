//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

typedef int		sfxHandle_t;

//	RegisterSound will allways return a valid sample, even if it
// has to create a placeholder.  This prevents continuous filesystem
// checks for missing files.
void S_BeginRegistration();
sfxHandle_t S_RegisterSound(const char* Name);
void S_EndRegistration();

//	Cinematics and voice-over-network will send raw samples
// 1.0 volume will be direct output of source samples
void S_ByteSwapRawSamples(int samples, int width, int channels, byte* data);
void S_RawSamples(int samples, int rate, int width, int channels, const byte* data, float lvol, float rvol, int streamingIndex);

void S_StartBackgroundTrack(const char* intro, const char* loop, int fadeupTime);
void S_StopBackgroundTrack();

float S_StartStreamingSound(const char* intro, const char* loop, int entnum, int channel, int attenuation);
void S_StopEntStreamingSound(int entNum);
void S_FadeStreamingSound(float targetvol, int time, int ssNum);

#if 0
// let the sound system know where an entity currently is
void S_UpdateEntityPosition(int EntityNumber, const vec3_t Origin);

// all continuous looping sounds must be added before calling S_Update
void S_ClearLoopingSounds(bool KillAll);
void S_AddLoopingSound(int EntityNumber, const vec3_t Origin, const vec3_t Velocity, sfxHandle_t SfxHandle);
void S_AddRealLoopingSound(int EntityNumber, const vec3_t Origin, const vec3_t Velocity, sfxHandle_t SfxHandle);
void S_StopLoopingSound(int EntityNumber);

// stop all sounds and the background track
void S_StopAllSounds();

void S_ClearSoundBuffer();

// if origin is NULL, the sound will be dynamically sourced from the entity
void S_StartSound(const vec3_t Origin, int EntityNumber, int EntityChannel, sfxHandle_t SfxHandle, float FVolume = 1, float Attenuation = 1, float TimeOffset = 0);
void S_StartLocalSound(const char* Sound);
void S_StartLocalSound(sfxHandle_t SfxHandle, int ChannelNumber);
void S_StopSound(int EntityNumber, int EntityChannel);
void S_UpdateSoundPos(int EntityNumber, int EntityChannel, vec3_t Origin);
void S_StaticSound(sfxHandle_t SfxHandle, vec3_t Origin, float Volume, float Attenuation);

// recompute the reletive volumes for all running sounds
// reletive to the given entityNum / orientation
void S_Respatialize(int EntityNumber, const vec3_t Origin, vec3_t Axis[3], int InWater);

void S_Update();
void S_ExtraUpdate();
void S_Shutdown();
void S_Init();
void S_DisableSounds();
#endif

int S_GetVoiceAmplitude(int entityNum);

qboolean MIDI_Init();
void MIDI_Cleanup();
void MIDI_Play(char *Name);
void MIDI_Stop();
void MIDI_Pause(int mode);
void MIDI_Loop(int NewValue);

int CDAudio_Init();
void CDAudio_Play(int track, qboolean looping);
void CDAudio_Stop();
void CDAudio_Pause();
void CDAudio_Resume();
void CDAudio_Shutdown();
void CDAudio_Update();
void CDAudio_Activate(qboolean active);

extern	Cvar* s_volume;
extern	Cvar* bgmvolume;
extern	Cvar* bgmtype;
