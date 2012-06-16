// server.h

#define QW_SERVER

#define MAX_MASTERS 8				// max recipients for heartbeat packets

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================


#define STATFRAMES  100

// edict->deadflag values
#define DEAD_NO                 0
#define DEAD_DYING              1
#define DEAD_DEAD               2

#define DAMAGE_NO               0
#define DAMAGE_YES              1
#define DAMAGE_AIM              2

#define FL2_CROUCHED            4096

// Built-in Spawn Flags
#define SPAWNFLAG_NOT_PALADIN       256
#define SPAWNFLAG_NOT_CLERIC        512
#define SPAWNFLAG_NOT_NECROMANCER   1024
#define SPAWNFLAG_NOT_THEIF         2048
#define SPAWNFLAG_NOT_EASY          4096
#define SPAWNFLAG_NOT_MEDIUM        8192
#define SPAWNFLAG_NOT_HARD          16384
#define SPAWNFLAG_NOT_DEATHMATCH    32768
#define SPAWNFLAG_NOT_COOP          65536
#define SPAWNFLAG_NOT_SINGLE        131072

// server flags
#define SFL_EPISODE_1       1
#define SFL_EPISODE_2       2
#define SFL_EPISODE_3       4
#define SFL_EPISODE_4       8
#define SFL_NEW_UNIT        16
#define SFL_NEW_EPISODE     32
#define SFL_CROSS_TRIGGERS  65280

#define MULTICAST_ALL           0
#define MULTICAST_PHS           1
#define MULTICAST_PVS           2

#define MULTICAST_ALL_R         3
#define MULTICAST_PHS_R         4
#define MULTICAST_PVS_R         5

//============================================================================

extern Cvar* sv_mintic;
extern Cvar* sv_maxtic;
extern Cvar* sv_maxspeed;
extern Cvar* sv_highchars;

extern netadr_t master_adr[MAX_MASTERS];		// address of the master server

extern Cvar* spawn;
extern Cvar* teamplay;
extern Cvar* skill;
extern Cvar* deathmatch;
extern Cvar* coop;
extern Cvar* maxclients;
extern Cvar* randomclass;
extern Cvar* damageScale;
extern Cvar* meleeDamScale;
extern Cvar* shyRespawn;
extern Cvar* spartanPrint;
extern Cvar* manaScale;
extern Cvar* tomeMode;
extern Cvar* tomeRespawn;
extern Cvar* w2Respawn;
extern Cvar* altRespawn;
extern Cvar* fixedLevel;
extern Cvar* autoItems;
extern Cvar* dmMode;
extern Cvar* easyFourth;
extern Cvar* patternRunner;
extern Cvar* fraglimit;
extern Cvar* timelimit;
extern Cvar* noexit;

extern client_t* host_client;

extern qhedict_t* sv_player;

extern char localmodels[MAX_MODELS_H2][5];			// inline model names for precache

extern char localinfo[MAX_LOCALINFO_STRING + 1];

extern int host_hunklevel;
extern fileHandle_t sv_logfile;
extern fileHandle_t sv_fraglogfile;

extern int sv_net_port;

//===========================================================

//
// sv_main.c
//
void SV_Shutdown(void);
void SV_Frame(float time);
void SV_FinalMessage(const char* message);
void SV_DropClient(client_t* drop);

int SV_CalcPing(client_t* cl);
void SV_FullClientUpdate(client_t* client, QMsg* buf);

int SV_ModelIndex(const char* name);

qboolean SV_CheckBottom(qhedict_t* ent);
qboolean SV_movestep(qhedict_t* ent, vec3_t move, qboolean relink, qboolean noenemy, qboolean set_trace);

void SV_WriteClientdataToMessage(client_t* client, QMsg* msg);

void SV_MoveToGoal(void);

void SV_SaveSpawnparms(void);

void SV_Physics_Client(qhedict_t* ent);

void SV_InitOperatorCommands(void);

void SV_SendServerinfo(client_t* client);
void SV_ExtractFromUserinfo(client_t* cl);


void Master_Heartbeat(void);
void Master_Packet(void);

//
// sv_init.c
//
void SV_SpawnServer(char* server, char* startspot);


//
// sv_phys.c
//
void SV_ProgStartFrame(void);
void SV_Physics(void);
void SV_CheckVelocity(qhedict_t* ent);
void SV_AddGravity(qhedict_t* ent, float scale);
qboolean SV_RunThink(qhedict_t* ent);
void SV_Physics_Toss(qhedict_t* ent);
void SV_RunNewmis(void);
void SV_Impact(qhedict_t* e1, qhedict_t* e2);

//
// sv_send.c
//
extern unsigned clients_multicast;

void SV_SendClientMessages(void);

void SV_Multicast(vec3_t origin, int to);
void SV_MulticastSpecific(unsigned clients, qboolean reliable);
void SV_StartSound(qhedict_t* entity, int channel, const char* sample, int volume,
	float attenuation);
void SV_StartParticle(vec3_t org, vec3_t dir, int color, int count);
void SV_StartParticle2(vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count);
void SV_StartParticle3(vec3_t org, vec3_t box, int color, int effect, int count);
void SV_StartParticle4(vec3_t org, float radius, int color, int effect, int count);
void SV_StartRainEffect(vec3_t org, vec3_t e_size, int x_dir, int y_dir, int color, int count);
void SV_ClientPrintf(client_t* cl, int level, const char* fmt, ...);
void SV_BroadcastPrintf(int level, const char* fmt, ...);
void SV_BroadcastCommand(const char* fmt, ...);
void SV_SendMessagesToAll(void);
void SV_FindModelNumbers(void);

//
// sv_user.c
//
void SV_ExecuteClientMessage(client_t* cl);
void SV_UserInit(void);


//
// svonly.c
//
typedef enum {RD_NONE, RD_CLIENT, RD_PACKET} redirect_t;
void SV_BeginRedirect(redirect_t rd);
void SV_EndRedirect(void);

//
// sv_ccmds.c
//
void SV_Status_f(void);

//
// sv_ents.c
//
void SV_WriteEntitiesToClient(client_t* client, QMsg* msg);

void SV_UpdateSoundPos(qhedict_t* entity, int channel);
void SV_StopSound(qhedict_t* entity, int channel);
void SV_ParseEffect(QMsg* sb);
void SV_FlushSignon(void);
void SV_SetMoveVars(void);
void SV_WriteInventory(client_t* host_client, qhedict_t* ent, QMsg* msg);
void SV_SendEffect(QMsg* sb, int index);
