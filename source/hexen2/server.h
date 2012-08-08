/*
 * $Header: /H2 Mission Pack/Server.h 6     2/27/98 11:53p Jweier $
 */

// server.h

// edict->deadflag values
#define DEAD_NO                 0
#define DEAD_DYING              1
#define DEAD_DEAD               2

//============================================================================

extern Cvar* randomclass;

extern client_t* host_client;

extern jmp_buf host_abortserver;

extern double host_time;

//===========================================================

void SV_Init(void);

void SV_AddUpdates(void);

void SV_AddClientToServer(qsocket_t* ret);

void SV_CheckForNewClients(void);
void SV_SpawnServer(char* server, char* startspot);

void SV_Edicts(const char* Name);
