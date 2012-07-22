// net.h -- quake's interface to the networking layer

extern netadr_t net_from;			// address of who sent the packet
extern QMsg net_message;

void        NET_Init(int port);
void        NET_Shutdown(void);

//============================================================================

#define OLD_AVG     0.99		// total = oldtotal*OLD_AVG + new*(1-OLD_AVG)

#define MAX_LATENT  32

void Netchan_Init(void);
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr);
