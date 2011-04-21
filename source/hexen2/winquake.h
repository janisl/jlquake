// winquake.h: Win32-specific Quake header file

/*
 * $Header: /H3/game/WINQUAKE.H 5     7/17/97 2:00p Rjohnson $
 */

#pragma warning( disable : 4229 )  // mgraph gets this

#include "../client/windows_shared.h"

#ifndef SERVERONLY
#include <ddraw.h>
#include <dsound.h>
#endif

extern	int			global_nCmdShow;

typedef enum {MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_FULLDIRECT, MS_UNINIT} modestate_t;

extern qboolean		ActiveApp, Minimized;

void VID_SetDefaultMode (void);
