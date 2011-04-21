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

#include "quakedef.h"
#include "glquake.h"

#include "../../client/unix_shared.h"

#include <X11/keysym.h>
#include <X11/cursorfont.h>

/*-----------------------------------------------------------------------*/
void VID_Shutdown(void)
{
	GLimp_Shutdown();

	// shutdown QGL subsystem
	QGL_Shutdown();
}

void GL_EndRendering (void)
{
	qglFlush();
	glXSwapBuffers(dpy, win);
}

void GL_Init();
void VID_Init(unsigned char *palette)
{
	R_SharedRegister();
	int i;

	vid.colormap = host_colormap;

	if (GLW_SetMode(r_mode->integer, r_colorbits->integer, !!r_fullscreen->value) != RSERR_OK)
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

	GL_Init();

	VID_SetPalette(palette);

	vid.recalc_refdef = 1;				// force a surface cache flush
}

void Sys_SendKeyEvents(void)
{
	HandleEvents();
}
