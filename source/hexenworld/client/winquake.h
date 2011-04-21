// winquake.h: Win32-specific Quake header file

#ifdef _WIN32 
#pragma warning( disable : 4229 )  // mgraph gets this

#define WM_MOUSEWHEEL                   0x020A

#include "../../client/windows_shared.h"

extern	int			global_nCmdShow;

typedef enum {MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_UNINIT} modestate_t;

extern qboolean		ActiveApp, Minimized;

void VID_SetDefaultMode (void);

#endif
