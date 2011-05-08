/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// quakedef.h -- primary header for server

#include "../../core/core.h"

//define	PARANOID			// speed sapping error checking

#include <setjmp.h>

#include "bothdefs.h"

#include "common.h"
#include "../../core/bsp29file.h"
#include "sys.h"
#include "zone.h"

#include "net.h"
#include "protocol.h"
#include "cmd.h"
typedef void efrag_t;
typedef int qhandle_t;
#include "gl_model.h"
#include "progs.h"

#include "server.h"
#include "world.h"
#include "pmove.h"

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	char	*basedir;
	int		argc;
	char	**argv;
	void	*membase;
	int		memsize;
} quakeparms_t;


//=============================================================================

//
// host
//
extern	quakeparms_t host_parms;

extern	QCvar*		developer;

extern	qboolean	host_initialized;		// true if into command execution
extern	double		host_frametime;
extern	double		realtime;			// not bounded in any way, changed at
										// start of every frame, never reset

void SV_Error (char *error, ...);
void SV_Init (quakeparms_t *parms);

void Con_Printf (char *fmt, ...);
void Con_DPrintf (char *fmt, ...);

