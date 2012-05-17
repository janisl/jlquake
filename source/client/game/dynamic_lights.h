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

struct cdlight_t
{
	int key;			// so entities can reuse same entry
	vec3_t color;
	vec3_t origin;
	float radius;
	int die;			// stop lighting after this time
	float decay;		// drop this each second
	float minlight;		// don't add when contributing less
	//	Hexen 2 only, but wasn't implemented in GL.
	bool dark;			// subtracts light instead of adding
};

extern cdlight_t cl_dlights[MAX_DLIGHTS];

void CL_ClearDlights();
cdlight_t* CL_AllocDlight(int key);
void CL_RunDLights();
void CL_AddDLights();

void CLQ1_MuzzleFlashLight(int key, vec3_t origin, vec3_t angles);
void CLQ1_BrightLight(int key, vec3_t origin);
void CLQ1_DimLight(int key, vec3_t origin, int type);
void CLQ1_RocketLight(int key, vec3_t origin);
void CLQ1_ExplosionLight(vec3_t origin);

void CLH2_MuzzleFlashLight(int key, vec3_t origin, const vec3_t angles, bool adjustZ);
void CLH2_BrightLight(int key, vec3_t origin);
void CLH2_DimLight(int key, vec3_t origin);
void CLH2_DarkLight(int key, vec3_t origin);
void CLH2_Light(int key, vec3_t origin);
void CLH2_FireBallLight(int key, vec3_t origin);
void CLH2_SpitLight(int key, vec3_t origin);
void CLH2_BrightFieldLight(int key, vec3_t origin);
void CLH2_ExplosionLight(vec3_t origin);

void CLQ2_MuzzleFlashLight(int key, vec3_t origin, vec3_t angles, float radius, int delay, vec3_t colour);
void CLQ2_MuzzleFlash2Light(int key, vec3_t origin, float radius, int delay, vec3_t colour);
void CLQ2_Flashlight(int key, vec3_t origin);
void CLQ2_ColorFlash(int key, vec3_t origin, int intensity, float r, float g, float b);
