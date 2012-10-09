// net.h -- quake's interface to the networking layer

extern netadr_t net_from;			// address of who sent the packet
extern QMsg net_message;

void        NET_Init();

//============================================================================

#define OLD_AVG     0.99		// total = oldtotal*OLD_AVG + new*(1-OLD_AVG)

#define MAX_LATENT  32
