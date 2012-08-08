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

//============================================================================

extern Cvar* spawn;
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

void SV_InitOperatorCommands(void);

void SV_SendServerinfo(client_t* client);


void Master_Heartbeat(void);
void Master_Packet(void);

//
// sv_init.c
//
void SV_SpawnServer(char* server, char* startspot);

//
// sv_ccmds.c
//
void SV_Status_f(void);
