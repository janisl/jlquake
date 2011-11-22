//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../client.h"
#include "local.h"

static sfxHandle_t clh2_sfx_tink1;
static sfxHandle_t clh2_sfx_ric1;
static sfxHandle_t clh2_sfx_ric2;
static sfxHandle_t clh2_sfx_ric3;
static sfxHandle_t clh2_sfx_r_exp3;

void CLH2_InitTEntsCommon()
{
	clh2_sfx_tink1 = S_RegisterSound("weapons/tink1.wav");
	clh2_sfx_ric1 = S_RegisterSound("weapons/ric1.wav");
	clh2_sfx_ric2 = S_RegisterSound("weapons/ric2.wav");
	clh2_sfx_ric3 = S_RegisterSound("weapons/ric3.wav");

	if (GGameType & GAME_HexenWorld)
	{
		clh2_sfx_r_exp3 = S_RegisterSound("weapons/r_exp3.wav");

		CLHW_InitExplosionSounds();
	}
}

void CLH2_ClearTEnts()
{
	CLH2_ClearExplosions();
	CLH2_ClearStreams();
}

int CLH2_TempSoundChannel()
{
	static int last = -1;
	if (!(GGameType & GAME_HexenWorld))
	{
		return -1;
	}
	last--;
	if (last < -20)
	{
		last = -1;
	}
	return last;
}

void CLH2_ParseWizSpike(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_RunParticleEffect(pos, vec3_origin, 30);
}

void CLH2_ParseKnightSpike(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_RunParticleEffect(pos, vec3_origin, 20);
}

static void CLH2_ParseSpikeCommon(QMsg& message, int count)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_RunParticleEffect(pos, vec3_origin, count);
	if (rand() % 5)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_tink1, 1, 1);
	}
	else
	{
		int rnd = rand() & 3;
		if (rnd == 1)
		{
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric1, 1, 1);
		}
		else if (rnd == 2)
		{
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric2, 1, 1);
		}
		else
		{
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric3, 1, 1);
		}
	}
}

void CLH2_ParseSpike(QMsg& message)
{
	CLH2_ParseSpikeCommon(message, 10);
}

void CLH2_ParseSuperSpike(QMsg& message)
{
	CLH2_ParseSpikeCommon(message, 20);
}

void CLH2_ParseExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_ParticleExplosion(pos);
}

void CLHW_ParseExplosion(QMsg& message)
{
	// particles
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_ParticleExplosion(pos);

	// light
	CLH2_ExplosionLight(pos);

	// sound
	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_r_exp3, 1, 1);
}

void CLH2_ParseBeam(QMsg& message)
{
	message.ReadShort();
	message.ReadCoord();
	message.ReadCoord();
	message.ReadCoord();
	message.ReadCoord();
	message.ReadCoord();
	message.ReadCoord();
}

void CLHW_ParseTarExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_BlobExplosion(pos);

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_r_exp3, 1, 1);
}

void CLH2_ParseLavaSplash(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_LavaSplash(pos);
}

void CLH2_ParseTeleport(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_TeleportSplash(pos);
}

void CLHW_ParseGunShot(QMsg& message)
{
	int cnt = message.ReadByte();
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_RunParticleEffect(pos, vec3_origin, 20 * cnt);
}

void CLHW_ParseLightningBlood(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_RunParticleEffect(pos, vec3_origin, 50);
}

void CLHW_ParseBoneRic(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	int cnt = message.ReadByte();

	CLH2_RunParticleEffect4(pos, 3, 368 + rand() % 16, pt_h2grav, cnt);
	int rnd = rand() % 100;
	if (rnd > 95)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric1, 1, 1);
	}
	else if (rnd > 91)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric2, 1, 1);
	}
	else if (rnd > 87)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric3, 1, 1);
	}
}
