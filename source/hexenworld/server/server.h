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

void SVQHW_Master_Heartbeat(void);
void Master_Packet(void);

//
// sv_ccmds.c
//
void SV_Status_f(void);
