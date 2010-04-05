// winquake.h: Win32-specific Quake header file

/*
 * $Header: /H3/game/WINQUAKE.H 5     7/17/97 2:00p Rjohnson $
 */

#pragma warning( disable : 4229 )  // mgraph gets this

#include <windows.h>

#ifndef SERVERONLY
#include <ddraw.h>
#include <dsound.h>
#endif

extern	HINSTANCE	global_hInstance;
extern	int			global_nCmdShow;

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

void	VID_LockBuffer (void);
void	VID_UnlockBuffer (void);

void Snd_AcquireBuffer (void);
void Snd_ReleaseBuffer (void);

#endif

typedef enum {MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_FULLDIRECT, MS_UNINIT} modestate_t;

extern modestate_t	modestate;

extern HWND			mainwindow;
extern qboolean		ActiveApp, Minimized;

extern qboolean	Win32AtLeastV4, WinNT;

int VID_ForceUnlockedAndReturnState (void);
void VID_ForceLockState (int lk);

void IN_ShowMouse (void);
void IN_DeactivateMouse (void);
void IN_HideMouse (void);
void IN_ActivateMouse (void);
void IN_RestoreOriginalMouseState (void);
void IN_SetQuakeMouseState (void);
void IN_MouseEvent (int mstate);

extern cvar_t		_windowed_mouse;

extern int		window_center_x, window_center_y;
extern RECT		window_rect;

extern qboolean	mouseinitialized;
extern HWND		hwnd_dialog;

extern HANDLE	hinput, houtput;

void IN_UpdateClipCursor (void);
void CenterWindow(HWND hWndCenter, int width, int height, BOOL lefttopjustify);

void S_BlockSound (void);
void S_UnblockSound (void);

void VID_SetDefaultMode (void);

LONG CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void IN_Accumulate (void);
