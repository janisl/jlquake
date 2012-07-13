// net_loop.h

int         Loop_Init(void);
void        Loop_Listen(qboolean state);
void        Loop_SearchForHosts(qboolean xmit);
qsocket_t* Loop_Connect(const char* host, netchan_t* chan);
qsocket_t* Loop_CheckNewConnections(netadr_t* outaddr);
int         Loop_GetMessage(qsocket_t* sock, netchan_t* chan);
int         Loop_SendMessage(qsocket_t* sock, netchan_t* chan, QMsg* data);
int         Loop_SendUnreliableMessage(qsocket_t* sock, netchan_t* chan, QMsg* data);
qboolean    Loop_CanSendMessage(qsocket_t* sock, netchan_t* chan);
void        Loop_Close(qsocket_t* sock, netchan_t* chan);
void        Loop_Shutdown(void);
