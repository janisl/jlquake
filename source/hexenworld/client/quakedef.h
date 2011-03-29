// quakedef.h -- primary header for client

#include "../../client/client.h"

//define	PARANOID			// speed sapping error checking

#include <setjmp.h>

#include "bothdefs.h"

#include "common.h"
#include "../../core/bsp29file.h"
#include "vid.h"
#include "sys.h"
#include "zone.h"
#include "wad.h"
#include "draw.h"
#include "screen.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "strings.h"
#include "sbar.h"
#include "render.h"
#include "cl_effect.h"
#include "client.h"
#include "gl_model.h"
#include "input.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"
#include "pmove.h"

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	char	*basedir;
	char	*cachedir;		// for development over ISDN lines
	int		argc;
	char	**argv;
	void	*membase;
	int		memsize;
} quakeparms_t;


//=============================================================================


extern qboolean noclip_anglehack;


//
// host
//
extern	quakeparms_t host_parms;

extern	QCvar*		developer;

extern	QCvar*	password;
extern	QCvar*  talksounds;

extern	qboolean	host_initialized;		// true if into command execution
extern	double		host_frametime;
extern	byte		*host_basepal;
extern	byte		*host_colormap;
extern	int			host_framecount;	// incremented every frame, never reset
extern	double		realtime;			// not bounded in any way, changed at
										// start of every frame, never reset

void Host_ServerFrame (void);
void Host_InitCommands (void);
void Host_Init (quakeparms_t *parms);
void Host_Shutdown(void);
void Host_Error (char *error, ...);
void Host_EndGame (char *message, ...);
void Host_Frame (float time);
void Host_Quit_f (void);
void Host_ClientCommands (char *fmt, ...);
void Host_ShutdownServer (qboolean crash);

extern qboolean		msg_suppress_1;		// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss

extern	unsigned int			defLosses;	// Defenders losses in Siege
extern	unsigned int			attLosses;	// Attackers Losses in Siege

extern int			cl_keyholder;
extern int			cl_doc;

void PR_LoadStrings(void);
