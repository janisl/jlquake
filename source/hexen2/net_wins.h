// net_wins.h

int  UDP_Init (void);
void UDP_Shutdown (void);
void UDP_Listen (qboolean state);
int  UDP_OpenSocket (int port);
int  UDP_CloseSocket (int socket);
int  UDP_Connect (int socket, struct qsockaddr *addr);
int  UDP_CheckNewConnections (void);
int  UDP_Read (int socket, byte *buf, int len, struct qsockaddr *addr);
int  UDP_Write (int socket, byte *buf, int len, struct qsockaddr *addr);
int  UDP_Broadcast (int socket, byte *buf, int len);
char *UDP_AddrToString (struct qsockaddr *addr);
int  UDP_StringToAddr (char *string, struct qsockaddr *addr);
int  UDP_GetSocketAddr (int socket, struct qsockaddr *addr);
int  UDP_GetNameFromAddr (struct qsockaddr *addr, char *name);
int  UDP_GetAddrFromName (char *name, struct qsockaddr *addr);
int  UDP_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2);
int  UDP_GetSocketPort (struct qsockaddr *addr);
int  UDP_SetSocketPort (struct qsockaddr *addr, int port);
