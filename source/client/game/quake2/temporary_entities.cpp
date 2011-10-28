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

sfxHandle_t cl_sfx_ric1;
sfxHandle_t cl_sfx_ric2;
sfxHandle_t cl_sfx_ric3;
sfxHandle_t cl_sfx_lashit;
sfxHandle_t cl_sfx_spark5;
sfxHandle_t cl_sfx_spark6;
sfxHandle_t cl_sfx_spark7;
sfxHandle_t cl_sfx_railg;
sfxHandle_t cl_sfx_rockexp;
sfxHandle_t cl_sfx_grenexp;
sfxHandle_t cl_sfx_watrexp;
sfxHandle_t cl_sfx_footsteps[4];

qhandle_t cl_mod_parasite_tip;
qhandle_t cl_mod_powerscreen;

sfxHandle_t cl_sfx_lightning;
sfxHandle_t cl_sfx_disrexp;

void CLQ2_RegisterTEntSounds()
{
	cl_sfx_ric1 = S_RegisterSound("world/ric1.wav");
	cl_sfx_ric2 = S_RegisterSound("world/ric2.wav");
	cl_sfx_ric3 = S_RegisterSound("world/ric3.wav");
	cl_sfx_lashit = S_RegisterSound("weapons/lashit.wav");
	cl_sfx_spark5 = S_RegisterSound("world/spark5.wav");
	cl_sfx_spark6 = S_RegisterSound("world/spark6.wav");
	cl_sfx_spark7 = S_RegisterSound("world/spark7.wav");
	cl_sfx_railg = S_RegisterSound("weapons/railgf1a.wav");
	cl_sfx_rockexp = S_RegisterSound("weapons/rocklx1a.wav");
	cl_sfx_grenexp = S_RegisterSound("weapons/grenlx1a.wav");
	cl_sfx_watrexp = S_RegisterSound("weapons/xpld_wat.wav");
	S_RegisterSound("player/land1.wav");

	S_RegisterSound("player/fall2.wav");
	S_RegisterSound("player/fall1.wav");

	for (int i = 0; i < 4; i++)
	{
		char name[MAX_QPATH];
		String::Sprintf(name, sizeof(name), "player/step%i.wav", i + 1);
		cl_sfx_footsteps[i] = S_RegisterSound(name);
	}

	cl_sfx_lightning = S_RegisterSound("weapons/tesla.wav");
	cl_sfx_disrexp = S_RegisterSound("weapons/disrupthit.wav");
}	

void CLQ2_RegisterTEntModels()
{
	CLQ2_RegisterExplosionModels();
	CLQ2_RegisterBeamModels();
	cl_mod_parasite_tip = R_RegisterModel("models/monsters/parasite/tip/tris.md2");
	cl_mod_powerscreen = R_RegisterModel("models/items/armor/effect/tris.md2");

	R_RegisterModel("models/objects/laser/tris.md2");
	R_RegisterModel("models/objects/grenade2/tris.md2");
	R_RegisterModel("models/weapons/v_machn/tris.md2");
	R_RegisterModel("models/weapons/v_handgr/tris.md2");
	R_RegisterModel("models/weapons/v_shotg2/tris.md2");
	R_RegisterModel("models/objects/gibs/bone/tris.md2");
	R_RegisterModel("models/objects/gibs/sm_meat/tris.md2");
	R_RegisterModel("models/objects/gibs/bone2/tris.md2");

	R_RegisterPic("w_machinegun");
	R_RegisterPic("a_bullets");
	R_RegisterPic("i_health");
	R_RegisterPic("a_grenades");
}

void CLQ2_ClearTEnts()
{
	CLQ2_ClearBeams();
	CLQ2_ClearExplosions();
	CLQ2_ClearLasers();
	CLQ2_ClearSustains();
}
