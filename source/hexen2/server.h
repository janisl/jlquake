/*
 * $Header: /H2 Mission Pack/Server.h 6     2/27/98 11:53p Jweier $
 */

// server.h

// edict->deadflag values
#define DEAD_NO                 0
#define DEAD_DYING              1
#define DEAD_DEAD               2

#define DAMAGE_NO               0		// Cannot be damaged
#define DAMAGE_YES              1		// Can be damaged
#define DAMAGE_NO_GRENADE       2		// Will not set off grenades

#define FL2_CROUCHED            4096

// Built-in Spawn Flags
#define SPAWNFLAG_NOT_PALADIN       0x00000100
#define SPAWNFLAG_NOT_CLERIC        0x00000200
#define SPAWNFLAG_NOT_NECROMANCER   0x00000400
#define SPAWNFLAG_NOT_THEIF         0x00000800
#define SPAWNFLAG_NOT_EASY          0x00001000
#define SPAWNFLAG_NOT_MEDIUM        0x00002000
#define SPAWNFLAG_NOT_HARD          0x00004000
#define SPAWNFLAG_NOT_DEATHMATCH    0x00008000
#define SPAWNFLAG_NOT_COOP          0x00010000
#define SPAWNFLAG_NOT_SINGLE        0x00020000
#define SPAWNFLAG_NOT_DEMON         0x00040000


// server flags
#define SFL_EPISODE_1       1
#define SFL_EPISODE_2       2
#define SFL_EPISODE_3       4
#define SFL_EPISODE_4       8
#define SFL_NEW_UNIT        16
#define SFL_NEW_EPISODE     32
#define SFL_CROSS_TRIGGERS  65280

//============================================================================

extern Cvar* teamplay;
extern Cvar* skill;
extern Cvar* deathmatch;
extern Cvar* randomclass;
extern Cvar* coop;
extern Cvar* fraglimit;
extern Cvar* timelimit;

extern client_t* host_client;

extern jmp_buf host_abortserver;

extern double host_time;

extern qhedict_t* sv_player;

//===========================================================

void SV_Init(void);

void SV_StartParticle(vec3_t org, vec3_t dir, int color, int count);
void SV_StartParticle2(vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count);
void SV_StartParticle3(vec3_t org, vec3_t box, int color, int effect, int count);
void SV_StartParticle4(vec3_t org, float radius, int color, int effect, int count);
void SV_StartSound(qhedict_t* entity, int channel, const char* sample, int volume,
	float attenuation);

void SV_DropClient(qboolean crash);

void SV_SendClientMessages(void);
void SV_ClearDatagram(void);

int SV_ModelIndex(const char* name);

void SV_SetIdealPitch(void);

void SV_AddUpdates(void);

void SV_ClientThink(void);
void SV_AddClientToServer(qsocket_t* ret);

void SV_ClientPrintf(const char* fmt, ...);
void SV_BroadcastPrintf(const char* fmt, ...);

void SV_Physics(void);

void SV_WriteClientdataToMessage(client_t* client, qhedict_t* ent, QMsg* msg);

void SV_CheckForNewClients(void);
void SV_RunClients(void);
void SV_SaveSpawnparms();
void SV_SpawnServer(char* server, char* startspot);

void SV_StopSound(qhedict_t* entity, int channel);
void SV_UpdateSoundPos(qhedict_t* entity, int channel);
void SV_ParseEffect(QMsg* sb);
void SV_Edicts(const char* Name);
void SaveGamestate(qboolean ClientsOnly);

void SV_SaveEffects(fileHandle_t FH);
char* SV_LoadEffects(char* Data);
