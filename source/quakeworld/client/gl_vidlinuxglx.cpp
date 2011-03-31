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
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>

#include "quakedef.h"
#include "glquake.h"

#include "../../client/unix_shared.h"

#include <X11/keysym.h>
#include <X11/cursorfont.h>

#define WARP_WIDTH              320
#define WARP_HEIGHT             200

static float old_windowed_mouse = 0;

unsigned short	d_8to16table[256];
unsigned		d_8to24table[256];
unsigned char	d_15to8table[65536];

QCvar*	_windowed_mouse;
QCvar*	vid_mode;
 
static float	old_mx, old_my;

QCvar*	m_filter;

static int scr_width, scr_height;

/*-----------------------------------------------------------------------*/

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

float		gldepthmin, gldepthmax;

QCvar*	gl_ztrick;

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

qboolean gl_mtexable = false;
qboolean dgamouse = false;

/*-----------------------------------------------------------------------*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}

static void install_grabs(void)
{
	XGrabPointer(dpy, win,
				 True,
				 0,
				 GrabModeAsync, GrabModeAsync,
				 win,
				 None,
				 CurrentTime);

	XF86DGADirectVideo(dpy, DefaultScreen(dpy), XF86DGADirectMouse);
	dgamouse = 1;

	XGrabKeyboard(dpy, win,
				  False,
				  GrabModeAsync, GrabModeAsync,
				  CurrentTime);

//	XSync(dpy, True);
}

static void uninstall_grabs(void)
{
	XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
	dgamouse = 0;

	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);

//	XSync(dpy, True);
}

static void GetEvent(void)
{
	XEvent event;
	int b;
	int key;

	if (!dpy)
		return;

	XNextEvent(dpy, &event);

	switch (event.type) {
	case KeyPress:
	case KeyRelease:
		XLateKey(&event.xkey, key);
		Key_Event(key, event.type == KeyPress);
		break;

	case MotionNotify:
		if (dgamouse && _windowed_mouse->value) {
			mx = event.xmotion.x_root;
			my = event.xmotion.y_root;
		} else
		{
			if (_windowed_mouse->value) {
				mx = (float) ((int)event.xmotion.x - (int)(vid.width/2));
				my = (float) ((int)event.xmotion.y - (int)(vid.height/2));

				/* move the mouse to the window center again */
				XSelectInput(dpy, win, X_MASK & ~PointerMotionMask);
				XWarpPointer(dpy, None, win, 0, 0, 0, 0, 
					(vid.width/2), (vid.height/2));
				XSelectInput(dpy, win, X_MASK);
			}
		}
		break;

	case ButtonPress:
		b=-1;
		if (event.xbutton.button == 1)
			b = 0;
		else if (event.xbutton.button == 2)
			b = 2;
		else if (event.xbutton.button == 3)
			b = 1;
		if (b>=0)
			Key_Event(K_MOUSE1 + b, true);
		break;

	case ButtonRelease:
		b=-1;
		if (event.xbutton.button == 1)
			b = 0;
		else if (event.xbutton.button == 2)
			b = 2;
		else if (event.xbutton.button == 3)
			b = 1;
		if (b>=0)
			Key_Event(K_MOUSE1 + b, false);
		break;
	}

	if (old_windowed_mouse != _windowed_mouse->value) {
		old_windowed_mouse = _windowed_mouse->value;

		if (!_windowed_mouse->value) {
			/* ungrab the pointer */
			uninstall_grabs();
		} else {
			/* grab the pointer */
			install_grabs();
		}
	}
}


void VID_Shutdown(void)
{
	if (!ctx)
		return;

	glXDestroyContext(dpy, ctx);
}

void signal_handler(int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	Sys_Quit();
	exit(0);
}

void InitSig(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}

void VID_ShiftPalette(unsigned char *p)
{
//	VID_SetPalette(p);
}

void	VID_SetPalette (unsigned char *palette)
{
	byte	*pal;
	unsigned r,g,b;
	unsigned v;
	int     r1,g1,b1;
	int		k;
	unsigned short i;
	unsigned	*table;
	fileHandle_t	f;
	float dist, bestdist;
	static qboolean palflag = false;

//
// 8 8 8 encoding
//
	Con_Printf("Converting 8to24\n");

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

	// JACK: 3D distance calcs - k is last closest, l is the distance.
	// FIXME: Precalculate this and cache to disk.
	if (palflag)
		return;
	palflag = true;

	FS_FOpenFileRead("glquake/15to8.pal", &f, true);
	if (f) {
		FS_Read(d_15to8table, 1<<15, f);
		FS_FCloseFile(f);
	} else {
		for (i=0; i < (1<<15); i++) {
			/* Maps
 			000000000000000
 			000000000011111 = Red  = 0x1F
 			000001111100000 = Blue = 0x03E0
 			111110000000000 = Grn  = 0x7C00
 			*/
 			r = ((i & 0x1F) << 3)+4;
 			g = ((i & 0x03E0) >> 2)+4;
 			b = ((i & 0x7C00) >> 7)+4;
			pal = (unsigned char *)d_8to24table;
			for (v=0,k=0,bestdist=10000.0; v<256; v++,pal+=4) {
 				r1 = (int)r - (int)pal[0];
 				g1 = (int)g - (int)pal[1];
 				b1 = (int)b - (int)pal[2];
				dist = sqrt(((r1*r1)+(g1*g1)+(b1*b1)));
				if (dist < bestdist) {
					k=v;
					bestdist = dist;
				}
			}
			d_15to8table[i]=k;
		}
		if ((f = FS_FOpenFileWrite("glquake/15to8.pal")) != 0) {
			FS_Write(d_15to8table, 1<<15, f);
			FS_FCloseFile(f);
		}
	}
}

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
	QGL_SharedInit();

	gl_vendor = (char*)qglGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = (char*)qglGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = (char*)qglGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = (char*)qglGetString (GL_EXTENSIONS);
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

//	Con_Printf ("%s %s\n", gl_renderer, gl_version);

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
	*width = scr_width;
	*height = scr_height;

//    if (!wglMakeCurrent( maindc, baseRC ))
//		Sys_Error ("wglMakeCurrent failed");

//	qglViewport (*x, *y, *width, *height);
}


void GL_EndRendering (void)
{
	qglFlush();
	glXSwapBuffers(dpy, win);
}

void VID_Init(unsigned char *palette)
{
	R_SharedRegister();
	int i;
	int width = 640, height = 480;

	S_Init();

	vid_mode = Cvar_Get("vid_mode", "0", 0);
	gl_ztrick = Cvar_Get("gl_ztrick", "1", 0);
	_windowed_mouse = Cvar_Get("_windowed_mouse", "0", CVAR_ARCHIVE);
	m_filter = Cvar_Get("m_filter", "0", 0);
	in_dgamouse = Cvar_Get("in_dgamouse", "1", 0);

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

// interpret command-line params

// set vid parameters
	if ((i = COM_CheckParm("-width")) != 0)
		width = QStr::Atoi(COM_Argv(i+1));
	if ((i = COM_CheckParm("-height")) != 0)
		height = QStr::Atoi(COM_Argv(i+1));

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

	if (GLimp_GLXSharedInit(width, height, true) != RSERR_OK)
	{
		exit(1);
	}

	scr_width = width;
	scr_height = height;

	if (vid.conheight > height)
		vid.conheight = height;
	if (vid.conwidth > width)
		vid.conwidth = width;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
	vid.numpages = 2;

	InitSig(); // trap evil signals

	GL_Init();

	VID_SetPalette(palette);

	Con_SafePrintf ("Video mode %dx%d initialized.\n", width, height);

	vid.recalc_refdef = 1;				// force a surface cache flush
}

void Sys_SendKeyEvents(void)
{
	if (dpy) {
		while (XPending(dpy)) 
			GetEvent();
	}
}

void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}

void IN_Init(void)
{
}

void IN_Shutdown(void)
{
}

/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
}

/*
===========
IN_Move
===========
*/
void IN_MouseMove (usercmd_t *cmd)
{
	if (m_filter->value)
	{
		mx = (mx + old_mx) * 0.5;
		my = (my + old_my) * 0.5;
	}
	old_mx = mx;
	old_my = my;

	mx *= sensitivity->value;
	my *= sensitivity->value;

// add mouse X/Y movement to cmd
	if ( (in_strafe.state & 1) || (lookstrafe->value && (in_mlook.state & 1) ))
		cmd->sidemove += m_side->value * mx;
	else
		cl.viewangles[YAW] -= m_yaw->value * mx;
	
	if (in_mlook.state & 1)
		V_StopPitchDrift ();
		
	if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch->value * my;
		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	}
	else
	{
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward->value * my;
		else
			cmd->forwardmove -= m_forward->value * my;
	}
	mx = my = 0.0;
}

void IN_Move (usercmd_t *cmd)
{
	IN_MouseMove(cmd);
}


void VID_UnlockBuffer() {}
void VID_LockBuffer() {}

