
extern int DEFAULTnet_hostport;

extern char playername[];
extern int playercolor;

//============================================================================
//
// public network functions
//
//============================================================================

extern QMsg net_message;

void        NET_Init(void);
void        NET_Shutdown(void);

qsocket_t* NET_CheckNewConnections(netadr_t* outaddr);
// returns a new connection number if there is one pending, else -1

qsocket_t* NET_Connect(const char* host, netchan_t* chan);
// called by client to connect to a host.  Returns -1 if not able to

qboolean NET_CanSendMessage(qsocket_t* sock, netchan_t* chan);
// Returns true or false if the given qsocket can currently accept a
// message to be transmitted.

int         NET_GetMessage(qsocket_t* sock, netchan_t* chan);
// returns data in net_message sizebuf
// returns 0 if no data is waiting
// returns 1 if a message was received
// returns 2 if an unreliable message was received
// returns -1 if the connection died

int         NET_SendMessage(qsocket_t* sock, netchan_t* chan, QMsg* data);
int         NET_SendUnreliableMessage(qsocket_t* sock, netchan_t* chan, QMsg* data);
// returns 0 if the message connot be delivered reliably, but the connection
//		is still considered valid
// returns 1 if the message was sent properly
// returns -1 if the connection died

int         NET_SendToAll(QMsg* data, int blocktime);
// This is a reliable *blocking* send to all attached clients.


void NET_Poll(void);


typedef struct _PollProcedure
{
	struct _PollProcedure* next;
	double nextTime;
	void (* procedure)();
	void* arg;
} PollProcedure;

void SchedulePollProcedure(PollProcedure* pp, double timeOffset);

extern qboolean slistInProgress;
extern qboolean slistSilent;
extern qboolean slistLocal;

void NET_Slist_f(void);
