// server.h

#define QW_SERVER

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

#define FL2_CROUCHED            4096

//============================================================================

extern Cvar* spawn;
extern Cvar* skill;
extern Cvar* randomclass;
extern Cvar* damageScale;
extern Cvar* meleeDamScale;
extern Cvar* shyRespawn;
extern Cvar* manaScale;
extern Cvar* tomeMode;
extern Cvar* tomeRespawn;
extern Cvar* w2Respawn;
extern Cvar* altRespawn;
extern Cvar* fixedLevel;
extern Cvar* autoItems;
extern Cvar* easyFourth;
extern Cvar* patternRunner;
extern Cvar* noexit;

extern client_t* host_client;

extern qhedict_t* sv_player;

extern char localmodels[MAX_MODELS_H2][5];			// inline model names for precache

extern int host_hunklevel;
extern fileHandle_t sv_logfile;

extern int sv_net_port;

//===========================================================

//
// sv_main.c
//
void SV_Shutdown(void);
void SV_Frame(float time);
void SV_FinalMessage(const char* message);
void SVQHW_DropClient(client_t* drop);

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

void SV_WriteInventory(client_t* host_client, qhedict_t* ent, QMsg* msg);
