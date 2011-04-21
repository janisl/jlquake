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
#include <execinfo.h>
/* get REG_EIP from ucontext.h */
#include <ucontext.h>

#include <dlfcn.h>

#include "quakedef.h"
#include "glquake.h"

#include "../client/unix_shared.h"

#include <X11/keysym.h>
#include <X11/cursorfont.h>

#define WARP_WIDTH              320
#define WARP_HEIGHT             200

unsigned		d_8to24table[256];

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

static float vid_gamma = 1.0;

qboolean gl_mtexable = false;

/*-----------------------------------------------------------------------*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}

void VID_Shutdown(void)
{
	GLimp_Shutdown();

	// shutdown QGL subsystem
	QGL_Shutdown();
}

static void signal_handler(int sig, siginfo_t *info, void *secret)
{
	void *trace[64];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
	ucontext_t *uc = (ucontext_t *)secret;

	/* Do something useful with siginfo_t */
#if id386
	if (sig == SIGSEGV)
		printf("Received signal %d, faulty address is %p, "
			"from %p\n", sig, info->si_addr, 
			uc->uc_mcontext.gregs[REG_EIP]);
	else
#endif
		printf("Received signal %d, exiting...\n", sig);
		
	trace_size = backtrace(trace, 64);
#if id386
	/* overwrite sigaction with caller's address */
	trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];
#endif

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	printf("[bt] Execution path:\n");
	for (i=1; i<trace_size; ++i)
		printf("[bt] %s\n", messages[i]);

	Sys_Quit();
	exit(0);
}

void InitSig(void)
{
	struct sigaction sa;

	sa.sa_sigaction = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
	sigaction(SIGIOT, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

void VID_ShiftPalette(unsigned char *p)
{
//	VID_SetPalette(p);
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

void CheckMultiTextureExtensions(void) 
{
	void *prjobj;

	if (strstr(gl_extensions, "GL_SGIS_multitexture ") && !COM_CheckParm("-nomtex")) {
		Con_Printf("Found GL_SGIS_multitexture...\n");

		if ((prjobj = dlopen(NULL, RTLD_LAZY)) == NULL) {
			Con_Printf("Unable to open symbol list for main program.\n");
			return;
		}

		qglMTexCoord2fSGIS = (void(APIENTRY*)(GLenum, GLfloat, GLfloat)) dlsym(prjobj, "glMTexCoord2fSGIS");
		qglSelectTextureSGIS = (void(APIENTRY*)(GLenum)) dlsym(prjobj, "glSelectTextureSGIS");

		if (qglMTexCoord2fSGIS && qglSelectTextureSGIS) {
			Con_Printf("Multitexture extensions found.\n");
			gl_mtexable = true;
		} else
			Con_Printf("Symbol not found, disabled.\n");

		dlclose(prjobj);
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

	gl_vendor = (const char*)qglGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = (const char*)qglGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = (const char*)qglGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = (const char*)qglGetString (GL_EXTENSIONS);
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
	qglFlush();
	glXSwapBuffers(dpy, win);
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
		f = pow ( (pal[i]+1)/256.0 , vid_gamma );
		inf = f*255 + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		palette[i] = inf;
	}

	Com_Memcpy(pal, palette, sizeof(palette));
}

void VID_Init(unsigned char *palette)
{
	R_SharedRegister();
	int i;

	gl_ztrick = Cvar_Get("gl_ztrick", "1", 0);

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	if (GLW_SetMode(r_mode->integer, !!r_fullscreen->value) != RSERR_OK)
	{
		exit(1);
	}

	if ((i = COM_CheckParm("-conwidth")) != 0)
		vid.conwidth = QStr::Atoi(COM_Argv(i+1));
	else
		vid.conwidth = 640;

	vid.conwidth &= 0xfff8; // make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth / glConfig.windowAspect;

	if ((i = COM_CheckParm("-conheight")) != 0)
		vid.conheight = QStr::Atoi(COM_Argv(i+1));
	if (vid.conheight < 200)
		vid.conheight = 200;

	if (vid.conheight > glConfig.vidHeight)
		vid.conheight = glConfig.vidHeight;
	if (vid.conwidth > glConfig.vidWidth)
		vid.conwidth = glConfig.vidWidth;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
	vid.numpages = 2;

	InitSig(); // trap evil signals

	GL_Init();

	VID_SetPalette(palette);

	vid.recalc_refdef = 1;				// force a surface cache flush
}

void Sys_SendKeyEvents(void)
{
	HandleEvents();
}
