// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"
#include "glquake.h"
#include "winquake.h"
#include "resource.h"
#include <commctrl.h>

#define MAX_MODE_LIST	30
#define VID_ROW_SIZE	3
#define BASEWIDTH		320
#define BASEHEIGHT		200

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)
#define MODE_FULLSCREEN_DEFAULT	(MODE_WINDOWED + 1)

qboolean		scr_skipupdate;

static qboolean	vid_initialized = false;
static qboolean	windowed;
static qboolean vid_canalttab = false;
static qboolean vid_wassuspended = false;

unsigned char	vid_curpal[256*3];

glvert_t glv;

HWND WINAPI InitializeWindow (HINSTANCE hInstance, int nCmdShow);

void AppActivate(BOOL fActive, BOOL minimize);
void ClearAllStates (void);

//====================================

int VID_SetMode(unsigned char *palette)
{
// so Con_Printfs don't mess us up by forcing vid and snd updates
	qboolean temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	CDAudio_Pause ();

	bool fullscreen = !!r_fullscreen->integer;

	IN_Activate(false);

	if (GLW_SetMode(r_mode->integer, r_colorbits->integer, fullscreen) != RSERR_OK)
		Sys_Error ("Couldn't set fullscreen DIB mode");

	if (vid.conheight > glConfig.vidHeight)
		vid.conheight = glConfig.vidHeight;
	if (vid.conwidth > glConfig.vidWidth)
		vid.conwidth = glConfig.vidWidth;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.numpages = 2;

	IN_Activate(true);

	CDAudio_Resume ();
	scr_disabled_for_loading = temp;

	VID_SetPalette (palette);

	// fix the leftover Alt from any Alt-Tab or the like that switched us away
	ClearAllStates ();

	VID_SetPalette (palette);

	vid.recalc_refdef = 1;

	return true;
}

void GL_EndRendering (void)
{
	if (!scr_skipupdate || block_drawing)
		SwapBuffers(maindc);
}

void VID_SetDefaultMode (void)
{
	IN_Activate(false);
}


void	VID_Shutdown (void)
{
	if (vid_initialized)
	{
		vid_canalttab = false;

		GLimp_Shutdown();

		QGL_Shutdown();

		AppActivate(false, false);
	}
}


/*
===================================================================

MAIN WINDOW

===================================================================
*/

/*
================
ClearAllStates
================
*/
void ClearAllStates (void)
{
	int		i;
	
// send an up event for each key, to make sure the server clears them all
	for (i=0 ; i<256 ; i++)
	{
		Key_Event (i, false);
	}

	Key_ClearStates ();
}

void AppActivate(BOOL fActive, BOOL minimize)
/****************************************************************************
*
* Function:     AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
{
	MSG msg;
    HDC			hdc;
    int			i, t;

	ActiveApp = fActive;
	Minimized = minimize;

	if (fActive)
	{
		IN_Activate(true);
		if (cdsFullscreen)
		{
			if (vid_canalttab && vid_wassuspended) {
				vid_wassuspended = false;
				ShowWindow(GMainWindow, SW_SHOWNORMAL);
			}
		}
	}

	if (!fActive)
	{
		IN_Activate(false);
		if (cdsFullscreen)
		{
			if (vid_canalttab) { 
				ChangeDisplaySettings (NULL, 0);
				vid_wassuspended = true;
			}
		}
	}
}

static int MWheelAccumulator;
extern QCvar* mwheelthreshold;


/* main window procedure */
LONG WINAPI MainWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    LONG    lRet = 1;
	int		fwKeys, xPos, yPos, fActive, fMinimized, temp;

	if (IN_HandleInputMessage(uMsg, wParam, lParam))
	{
		return 0;
	}

    switch (uMsg)
    {
		case WM_KILLFOCUS:
			if (cdsFullscreen)
				ShowWindow(GMainWindow, SW_SHOWMINNOACTIVE);
			break;

		case WM_CREATE:
			break;

		case WM_MOVE:
			break;

		case WM_SYSCHAR:
		// keep Alt-Space from happening
			break;

    	case WM_SIZE:
            break;

   	    case WM_CLOSE:
			if (MessageBox (GMainWindow, "Are you sure you want to quit?", "Confirm Exit",
						MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES)
			{
				Sys_Quit ();
			}

	        break;

		case WM_ACTIVATE:
			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);
			AppActivate(!(fActive == WA_INACTIVE), fMinimized);

		// fix the leftover Alt from any Alt-Tab or the like that switched us away
			ClearAllStates ();

			break;

   	    case WM_DESTROY:
            PostQuitMessage (0);
			break;

		case MM_MCINOTIFY:
            lRet = CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
			break;

    	default:
            /* pass all unhandled messages to DefWindowProc */
            lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);
        break;
    }

    /* return 1 if handled message, 0 if not */
    return lRet;
}

void GL_Init();
/*
===================
VID_Init
===================
*/
void	VID_Init (unsigned char *palette)
{
	R_SharedRegister();

	InitCommonControls();
	VID_SetPalette (palette);

	vid_initialized = true;

	int i;
	if ((i = COM_CheckParm("-conwidth")) != 0)
		vid.conwidth = QStr::Atoi(COM_Argv(i+1));
	else
		vid.conwidth = 640;

	vid.conwidth &= 0xfff8; // make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth*3 / 4;

	if ((i = COM_CheckParm("-conheight")) != 0)
		vid.conheight = QStr::Atoi(COM_Argv(i+1));
	if (vid.conheight < 200)
		vid.conheight = 200;

	vid.colormap = host_colormap;

	Sys_ShowConsole(0, false);

	VID_SetMode(palette);

	GL_Init ();

	VID_SetPalette (palette);
	
	vid_canalttab = true;
}
