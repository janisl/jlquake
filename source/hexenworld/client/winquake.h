// winquake.h: Win32-specific Quake header file

#ifdef _WIN32 
#pragma warning( disable : 4229 )  // mgraph gets this

#define WM_MOUSEWHEEL                   0x020A

#include "../../../libs/client/win_shared.h"

#ifndef SERVERONLY
#include <ddraw.h>
#include <dsound.h>
#endif

extern	HINSTANCE	global_hInstance;
extern	int			global_nCmdShow;

extern UINT	uMSG_MOUSEWHEEL;

#ifndef SERVERONLY

extern LPDIRECTDRAW		lpDD;
extern qboolean			DDActive;
extern LPDIRECTDRAWSURFACE	lpPrimary;
extern LPDIRECTDRAWSURFACE	lpFrontBuffer;
extern LPDIRECTDRAWSURFACE	lpBackBuffer;
extern LPDIRECTDRAWPALETTE	lpDDPal;
extern LPDIRECTSOUND pDS;
extern LPDIRECTSOUNDBUFFER pDSBuf;

extern DWORD gSndBufSize;
//#define SNDBUFSIZE 65536

#endif

typedef enum {MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_UNINIT} modestate_t;

extern modestate_t	modestate;

extern qboolean		ActiveApp, Minimized;

extern qboolean	WinNT;

int VID_ForceUnlockedAndReturnState (void);
void VID_ForceLockState (int lk);

void IN_ShowMouse (void);
void IN_DeactivateMouse (void);
void IN_HideMouse (void);
void IN_ActivateMouse (void);
void IN_RestoreOriginalMouseState (void);
void IN_SetQuakeMouseState (void);
void IN_MouseEvent (int mstate);

extern QCvar*		_windowed_mouse;

extern int		window_center_x, window_center_y;
extern RECT		window_rect;

extern qboolean	mouseinitialized;
extern HWND		hwnd_dialog;

extern HANDLE	hinput, houtput;

void IN_UpdateClipCursor (void);
void CenterWindow(HWND hWndCenter, int width, int height, BOOL lefttopjustify);

void VID_SetDefaultMode (void);

void IN_Accumulate (void);
void IN_ClearStates (void);
LONG CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
