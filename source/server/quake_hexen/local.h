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

#define MULTICAST_ALL           0
#define MULTICAST_PHS           1
#define MULTICAST_PVS           2
#define MULTICAST_ALL_R         3
#define MULTICAST_PHS_R         4
#define MULTICAST_PVS_R         5

//
//	CCmds
//
extern int qhw_fp_messages, qhw_fp_persecond, qhw_fp_secondsdead;
extern char qhw_fp_msg[];

//
//	Client
//
void SVQH_DropClient(client_t* host_client, bool crash);
void SVQHW_DropClient(client_t* drop);
void SVQH_SetIdealPitch(qhedict_t* player);
void SVQ1_PreSpawn_f(client_t* client);
void SVQH_Spawn_f(client_t* client);
void SVQH_Begin_f(client_t* client);
void SVQHW_New_f(client_t* client);
void SVQW_Soundlist_f(client_t* client);
void SVHW_Soundlist_f(client_t* client);
void SVQW_Modellist_f(client_t* client);
void SVHW_Modellist_f(client_t* client);
void SVQHW_PreSpawn_f(client_t* client);
void SVQHW_Spawn_f(client_t* client);
void SVQHW_Begin_f(client_t* client);
void SVQHW_NextDownload_f(client_t* client);
void SVQHW_BeginDownload_f(client_t* client);
void SVQH_Say_f(client_t* host_client);
void SVQH_Say_Team_f(client_t* host_client);
void SVQH_Tell_f(client_t* host_client);
void SVQHW_Say_f(client_t* host_client);
void SVQHW_Say_Team_f(client_t* host_client);
void SVQH_God_f(client_t* client);
void SVQH_Notarget_f(client_t* client);
void SVQH_Noclip_f(client_t* client);
void SVQ1_Fly_f(client_t* client);
void SVQ1_Give_f(client_t* host_client);
void SVH2_Give_f(client_t* host_client);
void SVQH_Name_f(client_t* client);
void SVH2_Class_f(client_t* client);
void SVQH_Color_f(client_t* client);
void SVQHW_Msg_f(client_t* client);
void SVQHW_SetInfo_f(client_t* client);
void SVQH_Kill_f(client_t* client);
void SVQH_TogglePause(const char* msg);
void SVQH_Pause_f(client_t* client);
void SVQH_Status_f(client_t* client);
void SVQH_Kick_f(client_t* host_client);
void SVQH_Ping_f(client_t* host_client);
void SVQH_Ban_f(client_t* client);
void SVQHW_Pings_f(client_t* host_client);
void SVQHW_Drop_f(client_t* client);
void SVQHW_PTrack_f(client_t* client);
void SVQHW_ShowServerinfo_f(client_t* client);
void SVQW_NoSnap_f(client_t* client);

//
//	Game
//
#define RETURN_EDICT(e) (((int*)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(e))
#define RETURN_STRING(s) (((int*)pr_globals)[OFS_RETURN] = PR_SetString(s))

extern Cvar* svqh_aim;

void PF_objerror();
void PF_changeyaw();
void PF_setorigin();
void SetMinMaxSize(qhedict_t* e, const float* min, const float* max);
void PF_setsize();
void PFQ1_setmodel();
void PFQW_setmodel();
void PF_ambientsound();
void PF_sound();
void PF_traceline();
void PF_checkclient();
void PF_findradius();
void PF_Spawn();
void PFQ1_Remove();
void PFH2_Remove();
void PFHW_Remove();
void PF_Find();
void PR_CheckEmptyString(const char* s);
void PF_precache_file();
void PF_precache_sound();
void PF_precache_model();
void PFQ1_walkmove();
void PFH2_walkmove();
void PF_droptofloor();
void PFQ1_lightstyle();
void PFQW_lightstyle();
void PF_checkbottom();
void PF_pointcontents();
void PF_nextent();
void PFQ1_aim();
void PFQW_aim();
void PFH2_aim();
QMsg* Q1WriteDest();
QMsg* QWWriteDest();
client_t* Write_GetClient();
void PFQ1_WriteByte();
void PFQW_WriteByte();
void PFQ1_WriteChar();
void PFQW_WriteChar();
void PFQ1_WriteShort();
void PFQW_WriteShort();
void PFQ1_WriteLong();
void PFQW_WriteLong();
void PFQ1_WriteAngle();
void PFQW_WriteAngle();
void PFQ1_WriteCoord();
void PFQW_WriteCoord();
void PFQ1_WriteString();
void PFQW_WriteString();
void PFQ1_WriteEntity();
void PFQW_WriteEntity();
void PF_setspawnparms();
void PF_logfrag();
void PF_multicast();
void PFQ1_bprint();
void PFQW_bprint();
void PFQ1_sprint();
void PFQW_sprint();
void PF_centerprint();
void PF_particle();
void PF_stuffcmd();

//
//	Init
//
extern char qhw_localinfo[QHMAX_LOCALINFO_STRING + 1];
extern func_t qhw_SpectatorConnect;
extern func_t qhw_SpectatorThink;
extern func_t qhw_SpectatorDisconnect;

void SVQH_FlushSignon();
int SVQH_ModelIndex(const char* name);

//
//	Main
//
extern Cvar* svqh_deathmatch;
extern Cvar* svqh_coop;
extern Cvar* svqh_teamplay;
extern Cvar* svqh_highchars;
extern Cvar* hw_spartanPrint;
extern Cvar* hw_dmMode;
extern Cvar* qh_timelimit;
extern Cvar* qh_fraglimit;
extern Cvar* svqw_mapcheck;
extern Cvar* qhw_allow_download;
extern Cvar* qhw_allow_download_skins;
extern Cvar* qhw_allow_download_models;
extern Cvar* qhw_allow_download_sounds;
extern Cvar* qhw_allow_download_maps;
extern Cvar* svhw_allowtaunts;
extern Cvar* svqhw_spectalk;
extern Cvar* qh_skill;
extern Cvar* qh_pausable;
extern int svqh_current_skill;			// skill level for currently loaded level (in case
										//  the user changes the cvar while the level is
										//  running, this reflects the level actually in use)
extern fileHandle_t svqhw_fraglogfile;
extern Cvar* svqh_idealpitchscale;

int SVQH_CalcPing(client_t* cl);
void SVQHW_FullClientUpdate(client_t* client, QMsg* buf);
void SVQHW_FullClientUpdateToClient(client_t* client, client_t* cl);
void SVQHW_ExtractFromUserinfo(client_t* cl);

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
// returns a new connection number if there is one pending, else -1
qsocket_t* NET_CheckNewConnections(netadr_t* outaddr);
// This is a reliable *blocking* send to all attached clients.
int NET_SendToAll(QMsg* data, int blocktime);
void MaxPlayers_f();
void SVQH_Shutdown();

//
//	Physics
//
extern Cvar* svqh_friction;
extern Cvar* svqh_stopspeed;
extern Cvar* svqh_gravity;
extern Cvar* svqh_maxspeed;
extern Cvar* svqh_accelerate;

void SVQH_RegisterPhysicsCvars();
void SVQH_SetMoveVars();
bool SVQH_RunThink(qhedict_t* ent, float frametime);
void SVQH_ProgStartFrame();
void SVQH_RunNewmis(float realtime);
void SVQH_RunPhysicsAndUpdateTime(float frametime, float realtime);
void SVQH_RunPhysicsForTime(float realtime);

//
//	Send
//
extern unsigned clients_multicast;
extern Cvar* sv_phs;

void SVQH_Multicast(const vec3_t origin, int to);
void SVHW_MulticastSpecific(unsigned clients, bool reliable);
void SVH2_StopSound(qhedict_t* entity, int channel);
void SVQH_StartSound(qhedict_t* entity, int channel, const char* sample, int volume,
	float attenuation);
void SVH2_UpdateSoundPos(qhedict_t* entity, int channel);
void SVQH_PrintToClient(client_t* cl, int level, char* string);
void SVQH_ClientPrintf(client_t* cl, int level, const char* fmt, ...) id_attribute((format(printf, 3, 4)));
void SVQH_BroadcastPrintf(int level, const char* fmt, ...) id_attribute((format(printf, 2, 3)));
void SVQH_SendClientCommand(client_t* cl, const char* fmt, ...) id_attribute((format(printf, 2, 3)));
void SVQH_BroadcastCommand(const char* fmt, ...) id_attribute((format(printf, 1, 2)));
void SVQH_StartParticle(const vec3_t org, const vec3_t dir, int color, int count);
void SVH2_StartParticle2(const vec3_t org, const vec3_t dmin, const vec3_t dmax, int color, int effect, int count);
void SVH2_StartParticle3(const vec3_t org, const vec3_t box, int color, int effect, int count);
void SVH2_StartParticle4(const vec3_t org, float radius, int color, int effect, int count);
void SVH2_StartRainEffect(const vec3_t org, const vec3_t e_size, int x_dir, int y_dir, int color, int count);
void SVQH_WriteClientdataToMessage(client_t* client, QMsg* msg);

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
q1trace_t SVQH_MoveHull0(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	int type, qhedict_t* passedict);
qhedict_t* SVQH_TestEntityPosition(qhedict_t* ent);

#endif
