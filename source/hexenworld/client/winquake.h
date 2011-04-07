// winquake.h: Win32-specific Quake header file

#ifdef _WIN32 
#pragma warning( disable : 4229 )  // mgraph gets this

#define WM_MOUSEWHEEL                   0x020A

#include "../../client/win_shared.h"

extern	int			global_nCmdShow;

typedef enum {MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_UNINIT} modestate_t;

extern qboolean		ActiveApp, Minimized;

extern qboolean	WinNT;

int VID_ForceUnlockedAndReturnState (void);
void VID_ForceLockState (int lk);

extern HWND		hwnd_dialog;

extern HANDLE	hinput, houtput;

void VID_SetDefaultMode (void);

#endif
