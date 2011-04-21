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
// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"
#include "glquake.h"
#include "winquake.h"
#include "resource.h"
#include <commctrl.h>

rserr_t GLW_SetMode(int mode, int colorbits, bool fullscreen);

#define MAX_MODE_LIST	30
#define VID_ROW_SIZE	3
#define WARP_WIDTH		320
#define WARP_HEIGHT		200
#define MAXWIDTH		10000
#define MAXHEIGHT		10000
#define BASEWIDTH		320
#define BASEHEIGHT		200

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)
#define MODE_FULLSCREEN_DEFAULT	(MODE_WINDOWED + 1)

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

qboolean		scr_skipupdate;

static qboolean	vid_initialized = false;
static qboolean	windowed;
static qboolean vid_canalttab = false;
static qboolean vid_wassuspended = false;

unsigned char	vid_curpal[256*3];

static float vid_gamma = 1.0;

glvert_t glv;

QCvar*	gl_ztrick;

HWND WINAPI InitializeWindow (HINSTANCE hInstance, int nCmdShow);

unsigned	d_8to24table[256];

float		gldepthmin, gldepthmax;

void VID_MenuDraw (void);
void VID_MenuKey (int key);

void AppActivate(BOOL fActive, BOOL minimize);
void ClearAllStates (void);
void VID_UpdateWindowStatus (void);
void GL_Init (void);

qboolean gl_mtexable = false;

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

	if (vid.conheight > glConfig.vidHeight)
		vid.conheight = glConfig.vidHeight;
	if (vid.conwidth > glConfig.vidWidth)
		vid.conwidth = glConfig.vidWidth;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

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

void CheckMultiTextureExtensions(void) 
{
	if (strstr(gl_extensions, "GL_SGIS_multitexture ") && !COM_CheckParm("-nomtex")) {
		Con_Printf("Multitexture extensions found.\n");
		qglMTexCoord2fSGIS = (void ( APIENTRY *)( GLenum, GLfloat, GLfloat )) GLimp_GetProcAddress("glMTexCoord2fSGIS");
		qglSelectTextureSGIS = (void ( APIENTRY *)( GLenum )) GLimp_GetProcAddress("glSelectTextureSGIS");
		gl_mtexable = true;
	}
}

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

//	Con_Printf ("%s %s\n", gl_renderer, gl_version);

	CheckMultiTextureExtensions ();

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
	if (!scr_skipupdate || block_drawing)
		SwapBuffers(maindc);
}

void	VID_SetPalette (unsigned char *palette)
{
	//
	// 8 8 8 encoding
	//
	byte* pal = palette;
	unsigned* table = d_8to24table;
	for (int i = 0; i < 256; i++)
	{
		unsigned r = pal[0];
		unsigned g = pal[1];
		unsigned b = pal[2];
		pal += 3;
		
		unsigned v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
		*table++ = v;
	}
	d_8to24table[255] &= 0xffffff;	// 255 is transparent
}

void	VID_ShiftPalette (unsigned char *palette)
{
	extern	byte ramps[3][256];
	
//	VID_SetPalette (palette);
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

static void Check_Gamma (unsigned char *pal)
{
	float	f, inf;
	unsigned char	palette[768];
	int		i;

	if ((i = COM_CheckParm("-gamma")) == 0)
	{
		vid_gamma = 0.7;
	}
	else
	{
		vid_gamma = QStr::Atof(COM_Argv(i+1));
	}

	for (i=0 ; i<768 ; i++)
	{
		f = pow ( (pal[i]+1)/256.0 , (double)vid_gamma );
		inf = f*255 + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		palette[i] = inf;
	}

	Com_Memcpy(pal, palette, sizeof(palette));
}

/*
===================
VID_Init
===================
*/
void	VID_Init (unsigned char *palette)
{
	R_SharedRegister();

    gl_ztrick = Cvar_Get("gl_ztrick", "1", 0);

	InitCommonControls();

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

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	Sys_ShowConsole(0, false);

	Check_Gamma(palette);
	VID_SetPalette (palette);

	VID_SetMode(palette);

	GL_Init ();

	vid_menudrawfn = VID_MenuDraw;
	vid_menukeyfn = VID_MenuKey;

	vid_canalttab = true;
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
		S_StartLocalSound("misc/menu1.wav");
		M_Menu_Options_f ();
		break;

	default:
		break;
	}
}
