// net_dgrm.h


int			Datagram_Init (void);
void		Datagram_Listen (qboolean state);
void		Datagram_SearchForHosts (qboolean xmit);
qsocket_t	*Datagram_Connect (const char *host, netchan_t* chan);
qsocket_t 	*Datagram_CheckNewConnections (netadr_t* outaddr);
int			Datagram_GetMessage (qsocket_t *sock, netchan_t* chan);
int			Datagram_SendMessage (qsocket_t *sock, netchan_t* chan, QMsg *data);
int			Datagram_SendUnreliableMessage (qsocket_t *sock, netchan_t* chan, QMsg *data);
qboolean	Datagram_CanSendMessage (qsocket_t *sock, netchan_t* chan);
qboolean	Datagram_CanSendUnreliableMessage (qsocket_t *sock);
void		Datagram_Close (qsocket_t *sock, netchan_t* chan);
void		Datagram_Shutdown (void);
