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

#ifndef _QUAKE_HEXEN_LOCAL_H
#define _QUAKE_HEXEN_LOCAL_H

#define QHEDICT_FROM_AREA(l) STRUCT_FROM_LINK(l, qhedict_t, area)

//
//	Game
//
void PF_changeyaw();

//
//	Move
//
bool SVQH_CheckBottom(qhedict_t* ent);
void SVQH_SetMoveTrace(const q1trace_t& trace);
bool SVQH_movestep(qhedict_t* ent, const vec3_t move, bool relink, bool noenemy, bool set_trace);
void SVQH_MoveToGoal();

//
//	NChan
//
void SVQH_ClientReliableCheckBlock(client_t* cl, int maxsize);
void SVQH_ClientReliableWrite_Begin(client_t* cl, int c, int maxsize);
void SVQH_ClientReliable_FinishWrite(client_t* cl);
void SVQH_ClientReliableWrite_Angle(client_t* cl, float f);
void SVQH_ClientReliableWrite_Angle16(client_t* cl, float f);
void SVQH_ClientReliableWrite_Byte(client_t* cl, int c);
void SVQH_ClientReliableWrite_Char(client_t* cl, int c);
void SVQH_ClientReliableWrite_Float(client_t* cl, float f);
void SVQH_ClientReliableWrite_Coord(client_t* cl, float f);
void SVQH_ClientReliableWrite_Long(client_t* cl, int c);
void SVQH_ClientReliableWrite_Short(client_t* cl, int c);
void SVQH_ClientReliableWrite_String(client_t* cl, const char* s);
void SVQH_ClientReliableWrite_SZ(client_t* cl, const void* data, int len);

//
//	Physics
//
extern Cvar* svqh_friction;
extern Cvar* svqh_stopspeed;
extern Cvar* svqh_gravity;
extern Cvar* svqh_maxvelocity;
extern Cvar* svqh_nostep;
extern Cvar* svqh_maxspeed;
extern Cvar* svqh_spectatormaxspeed;
extern Cvar* svqh_accelerate;
extern Cvar* svqh_airaccelerate;
extern Cvar* svqh_wateraccelerate;
extern Cvar* svqh_waterfriction;

void SVQH_SetMoveVars();
void SVQH_CheckVelocity(qhedict_t* ent);
bool SVQH_RunThink(qhedict_t* ent, float frametime);
void SVQH_Impact(qhedict_t* e1, qhedict_t* e2);
int SVQH_FlyMove(qhedict_t* ent, float time, q1trace_t* steptrace);
void SVQH_FlyExtras(qhedict_t* ent);
void SVQH_AddGravity(qhedict_t* ent, float frametime);
q1trace_t SVQH_PushEntity(qhedict_t* ent, const vec3_t push);
void SVQH_Physics_Pusher(qhedict_t* ent, float frametime);
void SVQH_Physics_None(qhedict_t* ent, float frametime);
void SVQH_Physics_Noclip(qhedict_t* ent, float frametime);

//
//	World
//
extern Cvar* sys_quake2;

// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
// flags ent->v.modified
void SVQH_UnlinkEdict(qhedict_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs, or solid
// flags ent->v.modified
// sets ent->v.absmin and ent->v.absmax
// if touchtriggers, calls prog functions for the intersected triggers
void SVQH_LinkEdict(qhedict_t* ent, bool touch_triggers);
// returns the CONTENTS_* value from the world at the given point.
// does not check any entities at all
// the non-true version remaps the water current contents to content_water
int SVQH_PointContents(vec3_t p);
//	mins and maxs are reletive
//	if the entire move stays in a solid volume, trace.allsolid will be set
//	if the starting point is in a solid, it will be allowed to move out
// to an open area
//	nomonsters is used for line of sight or edge testing, where mosnters
// shouldn't be considered solid objects
//	passedict is explicitly excluded from clipping checks (normally NULL)
q1trace_t SVQH_Move(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	int type, qhedict_t* passedict);
qhedict_t* SVQH_TestEntityPosition(qhedict_t* ent);

#endif
