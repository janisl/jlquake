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

// server flags
#define SFL_EPISODE_1       1
#define SFL_EPISODE_2       2
#define SFL_EPISODE_3       4
#define SFL_EPISODE_4       8
#define SFL_NEW_UNIT        16
#define SFL_NEW_EPISODE     32
#define SFL_CROSS_TRIGGERS  65280

//============================================================================

extern Cvar* sv_highchars;

extern netadr_t master_adr[MAX_MASTERS];		// address of the master server

extern Cvar* spawn;
extern Cvar* teamplay;
extern Cvar* skill;
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

void SV_WriteClientdataToMessage(client_t* client, QMsg* msg);

void SV_SaveSpawnparms(void);

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
// sv_send.c
//
void SV_SendClientMessages(void);

void SV_MulticastSpecific(unsigned clients, qboolean reliable);
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

void SV_ParseEffect(QMsg* sb);
void SV_WriteInventory(client_t* host_client, qhedict_t* ent, QMsg* msg);
void SV_SendEffect(QMsg* sb, int index);
