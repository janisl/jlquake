// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"
#include "glquake.h"
#include "winquake.h"
#include "resource.h"

#define MAX_MODE_LIST	30
#define VID_ROW_SIZE	3
#define BASEWIDTH		320
#define BASEHEIGHT		200

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)
#define MODE_FULLSCREEN_DEFAULT	(MODE_WINDOWED + 1)

byte globalcolormap[VID_GRADES*256];

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

qboolean		scr_skipupdate;

qboolean	vid_initialized = false;
static qboolean	windowed;

unsigned char	vid_curpal[256*3];
float RTint[256],GTint[256],BTint[256];

glvert_t glv;

QCvar*	gl_ztrick;

HWND WINAPI InitializeWindow (HINSTANCE hInstance, int nCmdShow);

unsigned	d_8to24table[256];
unsigned	d_8to24TranslucentTable[256];

float		gldepthmin, gldepthmax;

void VID_MenuDraw (void);
void VID_MenuKey (int key);

void AppActivate(BOOL fActive, BOOL minimize);
void ClearAllStates (void);
void VID_UpdateWindowStatus (void);
void GL_Init (void);

//====================================

// direct draw software compatability stuff

void VID_HandlePause (qboolean pause)
{
}

void VID_ForceLockState (int lk)
{
}

void VID_LockBuffer (void)
{
}

void VID_UnlockBuffer (void)
{
}

int VID_ForceUnlockedAndReturnState (void)
{
	return 0;
}

void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}


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

	if (COM_CheckParm ("-scale2d")) {
		vid.height = vid.conheight = BASEHEIGHT ;//modelist[modenum].height; // BASEHEIGHT;
		vid.width = vid.conwidth =   BASEWIDTH  ;//modelist[modenum].width; //  BASEWIDTH ;
	} else {
		vid.height = vid.conheight = glConfig.vidHeight; // BASEHEIGHT;
		vid.width = vid.conwidth =   glConfig.vidWidth; //  BASEWIDTH ;
	}
	vid.numpages = 2;

	IN_Activate(true);

	VID_UpdateWindowStatus ();

	CDAudio_Resume ();
	scr_disabled_for_loading = temp;

	VID_SetPalette (palette);

	// fix the leftover Alt from any Alt-Tab or the like that switched us away
	ClearAllStates ();

	VID_SetPalette (palette);

	vid.recalc_refdef = 1;

	return true;
}


/*
================
VID_UpdateWindowStatus
================
*/
void VID_UpdateWindowStatus (void)
{
}


//====================================

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
	QGL_Init();

	gl_vendor = (char*)qglGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = (char*)qglGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = (char*)qglGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = (char*)qglGetString (GL_EXTENSIONS);
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

	qglClearColor (1,0,0,0);
	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.666);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel (GL_FLAT);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = *y = 0;
	*width = glConfig.vidWidth;
	*height = glConfig.vidHeight;

//    if (!wglMakeCurrent( maindc, baseRC ))
//		Sys_Error ("wglMakeCurrent failed");

//	qglViewport (*x, *y, *width, *height);
}


void GL_EndRendering (void)
{
	if (!scr_skipupdate)
		SwapBuffers(maindc);
}


int ColorIndex[16] =
{
	0, 31, 47, 63, 79, 95, 111, 127, 143, 159, 175, 191, 199, 207, 223, 231
};

unsigned ColorPercent[16] =
{
	25, 51, 76, 102, 114, 127, 140, 153, 165, 178, 191, 204, 216, 229, 237, 247
};

void VID_SetPalette (unsigned char *palette)
{
	byte	*pal;
	int		r,g,b,v;
	int		i,c,p;
	unsigned	*table;
	
	
//
// 8 8 8 encoding
//
	pal = palette;
	table = d_8to24table;
	
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		
//		v = (255<<24) + (r<<16) + (g<<8) + (b<<0);
//		v = (255<<0) + (r<<8) + (g<<16) + (b<<24);
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		*table++ = v;
	}

	d_8to24table[255] &= 0xffffff;	// 255 is transparent

	pal = palette;
	table = d_8to24TranslucentTable;

	for (i=0; i<16;i++)
	{
		c = ColorIndex[i]*3;

		r = pal[c];
		g = pal[c+1];
		b = pal[c+2];

		for(p=0;p<16;p++)
		{
			v = (ColorPercent[15-p]<<24) + (r<<0) + (g<<8) + (b<<16);
			//v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
			*table++ = v;

			RTint[i*16+p] = ((float)r) / ((float)ColorPercent[15-p]) ;
			GTint[i*16+p] = ((float)g) / ((float)ColorPercent[15-p]);
			BTint[i*16+p] = ((float)b) / ((float)ColorPercent[15-p]);
		}
	}
}

void	VID_ShiftPalette (unsigned char *palette)
{
	extern	byte ramps[3][256];
}


void VID_SetDefaultMode (void)
{
	IN_Activate(false);
}


void	VID_Shutdown (void)
{
	if (vid_initialized)
	{
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
    HDC			hdc;
    int			i, t;

	ActiveApp = fActive;
	Minimized = minimize;

	if (fActive)
	{
		IN_Activate(true);
	}

	if (!fActive)
	{
		IN_Activate(false);
	}
}


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
		case WM_CREATE:
			break;

		case WM_MOVE:
			VID_UpdateWindowStatus ();
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

/*
===================
VID_Init
===================
*/
void	VID_Init (unsigned char *palette)
{
	R_SharedRegister();

    gl_ztrick = Cvar_Get("gl_ztrick", "1", CVAR_ARCHIVE);

	vid_initialized = true;

	vid.colormap = host_colormap;

	Sys_ShowConsole(0, false);

	VID_SetMode(palette);

	GL_Init ();

	VID_SetPalette (palette);

	vid_menudrawfn = VID_MenuDraw;
	vid_menukeyfn = VID_MenuKey;
}


//========================================================
// Video menu stuff
//========================================================

extern void M_Menu_Options_f (void);
extern void M_Print (int cx, int cy, char *str);
extern void M_PrintWhite (int cx, int cy, char *str);
extern void M_DrawCharacter (int cx, int line, int num);
extern void M_DrawTransPic (int x, int y, qpic_t *pic);
extern void M_DrawPic (int x, int y, qpic_t *pic);

#define MAX_COLUMN_SIZE		9
#define MODE_AREA_HEIGHT	(MAX_COLUMN_SIZE + 2)

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	qpic_t* p = Draw_CachePic ("gfx/vidmodes.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*2,
			 "Video modes must be set from the");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*3,
			 "console with set r_mode <number>");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*4,
			 "and set r_colorbits <bits-per-pixel>");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*6,
			 "Select windowed mode with set r_fullscreen 0");
}


/*
================
VID_MenuKey
================
*/
void VID_MenuKey (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		S_StartLocalSound("raven/menu1.wav");
		M_Menu_Options_f ();
		break;

	default:
		break;
	}
}

void D_ShowLoadingSize(void)
{
	if (!vid_initialized)
		return;

	qglDrawBuffer  (GL_FRONT);

	SCR_DrawLoading();

	qglDrawBuffer  (GL_BACK);
}
