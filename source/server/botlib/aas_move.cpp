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

#include "../server.h"
#include "local.h"

aas_settings_t aassettings;

static void AAS_InitSettingsQ3()
{
	aassettings.phys_friction               = LibVarValue("phys_friction", "6");
	aassettings.phys_stopspeed              = LibVarValue("phys_stopspeed", "100");
	aassettings.phys_gravity                = LibVarValue("phys_gravity", "800");
	aassettings.phys_waterfriction          = LibVarValue("phys_waterfriction", "1");
	aassettings.phys_watergravity           = LibVarValue("phys_watergravity", "400");
	aassettings.phys_maxvelocity            = LibVarValue("phys_maxvelocity", "320");
	aassettings.phys_maxwalkvelocity        = LibVarValue("phys_maxwalkvelocity", "320");
	aassettings.phys_maxcrouchvelocity      = LibVarValue("phys_maxcrouchvelocity", "100");
	aassettings.phys_maxswimvelocity        = LibVarValue("phys_maxswimvelocity", "150");
	aassettings.phys_walkaccelerate         = LibVarValue("phys_walkaccelerate", "10");
	aassettings.phys_airaccelerate          = LibVarValue("phys_airaccelerate", "1");
	aassettings.phys_swimaccelerate         = LibVarValue("phys_swimaccelerate", "4");
	aassettings.phys_maxstep                = LibVarValue("phys_maxstep", "19");
	aassettings.phys_maxsteepness           = LibVarValue("phys_maxsteepness", "0.7");
	aassettings.phys_maxwaterjump           = LibVarValue("phys_maxwaterjump", "18");
	aassettings.phys_maxbarrier             = LibVarValue("phys_maxbarrier", "33");
	aassettings.phys_jumpvel                = LibVarValue("phys_jumpvel", "270");
	aassettings.phys_falldelta5             = LibVarValue("phys_falldelta5", "40");
	aassettings.phys_falldelta10            = LibVarValue("phys_falldelta10", "60");
	aassettings.rs_waterjump                = LibVarValue("rs_waterjump", "400");
	aassettings.rs_teleport                 = LibVarValue("rs_teleport", "50");
	aassettings.rs_barrierjump              = LibVarValue("rs_barrierjump", "100");
	aassettings.rs_startcrouch              = LibVarValue("rs_startcrouch", "300");
	aassettings.rs_startgrapple             = LibVarValue("rs_startgrapple", "500");
	aassettings.rs_startwalkoffledge        = LibVarValue("rs_startwalkoffledge", "70");
	aassettings.rs_startjump                = LibVarValue("rs_startjump", "300");
	aassettings.rs_rocketjump               = LibVarValue("rs_rocketjump", "500");
	aassettings.rs_bfgjump                  = LibVarValue("rs_bfgjump", "500");
	aassettings.rs_jumppad                  = LibVarValue("rs_jumppad", "250");
	aassettings.rs_aircontrolledjumppad     = LibVarValue("rs_aircontrolledjumppad", "300");
	aassettings.rs_funcbob                  = LibVarValue("rs_funcbob", "300");
	aassettings.rs_startelevator            = LibVarValue("rs_startelevator", "50");
	aassettings.rs_falldamage5              = LibVarValue("rs_falldamage5", "300");
	aassettings.rs_falldamage10             = LibVarValue("rs_falldamage10", "500");
	aassettings.rs_maxfallheight            = LibVarValue("rs_maxfallheight", "0");
	aassettings.rs_maxjumpfallheight        = LibVarValue("rs_maxjumpfallheight", "450");
}

static void AAS_InitSettingsWolf()
{
	aassettings.phys_friction = 6;
	aassettings.phys_stopspeed = 100;
	aassettings.phys_gravity = 800;
	aassettings.phys_waterfriction = 1;
	aassettings.phys_watergravity = 400;
	aassettings.phys_maxvelocity = 320;
	aassettings.phys_maxwalkvelocity = 300;
	aassettings.phys_maxcrouchvelocity = 100;
	aassettings.phys_maxswimvelocity = 150;
	aassettings.phys_walkaccelerate = 10;
	aassettings.phys_airaccelerate = 1;
	aassettings.phys_swimaccelerate = 4;
	aassettings.phys_maxstep = 18;
	aassettings.phys_maxsteepness = 0.7;
	aassettings.phys_maxwaterjump = 17;
	aassettings.phys_jumpvel = 270;
	if (GGameType & GAME_WolfMP)
	{
		// Ridah, calculate maxbarrier according to jumpvel and gravity
		aassettings.phys_maxbarrier = -0.8 + (0.5 * aassettings.phys_jumpvel * aassettings.phys_jumpvel / aassettings.phys_gravity);
	}
	else
	{
		aassettings.phys_maxbarrier = 49;
	}
	aassettings.rs_maxjumpfallheight = 450;
	aassettings.rs_startcrouch = STARTCROUCH_TIME;
}

void AAS_InitSettings()
{
	if (GGameType & GAME_Quake3)
	{
		AAS_InitSettingsQ3();
	}
	else
	{
		AAS_InitSettingsWolf();
	}
}

// returns true if the bot is against a ladder
bool AAS_AgainstLadder(const vec3_t origin, int ms_areanum)
{
	vec3_t org;
	VectorCopy(origin, org);
	int areanum = AAS_PointAreaNum(org);
	if (!areanum)
	{
		org[0] += 1;
		areanum = AAS_PointAreaNum(org);
		if (!areanum)
		{
			org[1] += 1;
			areanum = AAS_PointAreaNum(org);
			if (!areanum)
			{
				org[0] -= 2;
				areanum = AAS_PointAreaNum(org);
				if (!areanum)
				{
					org[1] -= 2;
					areanum = AAS_PointAreaNum(org);
				}
			}
		}
	}
	//if in solid... wrrr shouldn't happen
	if (!areanum)
	{
		if (GGameType & GAME_Quake3)
		{
			return false;
		}
		// RF, it does if they're in a monsterclip brush
		areanum = ms_areanum;
	}
	//if not in a ladder area
	if (!(aasworld->areasettings[areanum].areaflags & AREA_LADDER))
	{
		return false;
	}
	//if a crouch only area
	if (!(aasworld->areasettings[areanum].presencetype & PRESENCE_NORMAL))
	{
		return false;
	}

	aas_area_t* area = &aasworld->areas[areanum];
	for (int i = 0; i < area->numfaces; i++)
	{
		int facenum = aasworld->faceindex[area->firstface + i];
		int side = facenum < 0;
		aas_face_t* face = &aasworld->faces[abs(facenum)];
		//if the face isn't a ladder face
		if (!(face->faceflags & FACE_LADDER))
		{
			continue;
		}
		//get the plane the face is in
		aas_plane_t* plane = &aasworld->planes[face->planenum ^ side];
		//if the origin is pretty close to the plane
		if (abs(DotProduct(plane->normal, origin) - plane->dist) < (GGameType & GAME_ET ? 7 : 3))
		{
			// RF, if hanging on to the edge of a ladder, we have to account for bounding box touching
			if (AAS_PointInsideFace(abs(facenum), origin, GGameType & GAME_ET ? 2.0 : 0.1f))
			{
				return true;
			}
		}
	}
	return false;
}

// applies ground friction to the given velocity
void AAS_Accelerate(vec3_t velocity, float frametime, const vec3_t wishdir, float wishspeed, float accel)
{
	// q2 style
	int i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
	{
		return;
	}
	accelspeed = accel * frametime * wishspeed;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	for (i = 0; i < 3; i++)
	{
		velocity[i] += accelspeed * wishdir[i];
	}
}

// applies ground friction to the given velocity
void AAS_ApplyFriction(vec3_t vel, float friction, float stopspeed, float frametime)
{
	//horizontal speed
	float speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
	if (!speed)
	{
		return;
	}
	float control = speed < stopspeed ? stopspeed : speed;
	float newspeed = speed - frametime * control * friction;
	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;
	vel[0] *= newspeed;
	vel[1] *= newspeed;
}

bool AAS_ClipToBBox(aas_trace_t* trace, const vec3_t start, const vec3_t end,
	int presencetype, const vec3_t mins, const vec3_t maxs)
{
	vec3_t bboxmins, bboxmaxs, absmins, absmaxs;
	AAS_PresenceTypeBoundingBox(presencetype, bboxmins, bboxmaxs);
	VectorSubtract(mins, bboxmaxs, absmins);
	VectorSubtract(maxs, bboxmins, absmaxs);

	VectorCopy(end, trace->endpos);
	trace->fraction = 1;
	int i;
	for (i = 0; i < 3; i++)
	{
		if (start[i] < absmins[i] && end[i] < absmins[i])
		{
			return false;
		}
		if (start[i] > absmaxs[i] && end[i] > absmaxs[i])
		{
			return false;
		}
	}
	//check bounding box collision
	vec3_t dir;
	VectorSubtract(end, start, dir);
	float frac = 1;
	vec3_t mid;
	for (i = 0; i < 3; i++)
	{
		//get plane to test collision with for the current axis direction
		float planedist;
		if (dir[i] > 0)
		{
			planedist = absmins[i];
		}
		else
		{
			planedist = absmaxs[i];
		}
		//calculate collision fraction
		float front = start[i] - planedist;
		float back = end[i] - planedist;
		frac = front / (front - back);
		//check if between bounding planes of next axis
		int side = i + 1;
		if (side > 2)
		{
			side = 0;
		}
		mid[side] = start[side] + dir[side] * frac;
		if (mid[side] > absmins[side] && mid[side] < absmaxs[side])
		{
			//check if between bounding planes of next axis
			side++;
			if (side > 2)
			{
				side = 0;
			}
			mid[side] = start[side] + dir[side] * frac;
			if (mid[side] > absmins[side] && mid[side] < absmaxs[side])
			{
				mid[i] = planedist;
				break;
			}
		}
	}
	//if there was a collision
	if (i != 3)
	{
		trace->startsolid = false;
		trace->fraction = frac;
		trace->ent = 0;
		trace->planenum = 0;
		trace->area = 0;
		trace->lastarea = 0;
		//trace endpos
		for (int j = 0; j < 3; j++)
		{
			trace->endpos[j] = start[j] + dir[j] * frac;
		}
		return true;
	}
	return false;
}

// calculates the horizontal velocity needed to perform a jump from start
// to end
// Parameter:			zvel	: z velocity for jump
//						start	: start position of jump
//						end		: end position of jump
//						*speed	: returned speed for jump
// Returns:				false if too high or too far from start to end
bool AAS_HorizontalVelocityForJump(float zvel, const vec3_t start, const vec3_t end, float* velocity)
{
	float phys_gravity = aassettings.phys_gravity;
	float phys_maxvelocity = aassettings.phys_maxvelocity;

	//maximum height a player can jump with the given initial z velocity
	float maxjump = 0.5 * phys_gravity * (zvel / phys_gravity) * (zvel / phys_gravity);
	//top of the parabolic jump
	float top = start[2] + maxjump;
	//height the bot will fall from the top
	float height2fall = top - end[2];
	//if the goal is to high to jump to
	if (height2fall < 0)
	{
		*velocity = phys_maxvelocity;
		return 0;
	}
	//time a player takes to fall the height
	float t = sqrt(height2fall / (0.5 * phys_gravity));
	//direction from start to end
	vec3_t dir;
	VectorSubtract(end, start, dir);

	if ((t + zvel / phys_gravity) == 0.0f)
	{
		*velocity = phys_maxvelocity;
		return 0;
	}
	//calculate horizontal speed
	*velocity = sqrt(dir[0] * dir[0] + dir[1] * dir[1]) / (t + zvel / phys_gravity);
	//the horizontal speed must be lower than the max speed
	if (*velocity > phys_maxvelocity)
	{
		*velocity = phys_maxvelocity;
		return false;
	}
	return true;
}
