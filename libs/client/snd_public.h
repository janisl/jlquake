//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
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
void S_ByteSwapRawSamples(int Samples, int Width, int Channels, const byte* Data);
void S_RawSamples(int Samples, int Rate, int Width, int Channels, const byte* Data, float Volume);

void S_StartBackgroundTrack(const char* Intro, const char* Loop);
void S_StopBackgroundTrack();

// let the sound system know where an entity currently is
void S_UpdateEntityPosition(int EntityNum, const vec3_t Origin);

// all continuous looping sounds must be added before calling S_Update
void S_ClearLoopingSounds(bool KillAll);
void S_AddLoopingSound(int EntityNum, const vec3_t Origin, const vec3_t Velocity, sfxHandle_t SfxHandle);
void S_AddRealLoopingSound(int EntityNum, const vec3_t Origin, const vec3_t Velocity, sfxHandle_t SfxHandle);
void S_StopLoopingSound(int EntityNum);

// stop all sounds and the background track
void S_StopAllSounds();

void S_ClearSoundBuffer();

extern	QCvar* s_volume;
