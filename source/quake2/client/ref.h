/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#ifndef _ref_h
#define _ref_h

#include "../qcommon/qcommon.h"

#define	MAX_PARTICLES	4096
#define	MAX_LIGHTSTYLES_Q2	256

//
// these are the functions exported by the refresh module
//
typedef struct
{
	// called before the library is unloaded
	void	(*Shutdown) (void);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// EndRegistration will free any remaining data that wasn't registered.
	// Any qhandle_t or skin_s pointers from before the BeginRegistration
	// are no longer valid after EndRegistration.
	//
	// Skins and images need to be differentiated, because skins
	// are flood filled to eliminate mip map edge errors, and pics have
	// an implicit "pics/" prepended to the name. (a pic name that starts with a
	// slash will not use the "pics/" prefix or the ".pcx" postfix)
	void	(*BeginRegistration) (const char *map);
	struct image_t *(*RegisterSkin) (const char *name);
	struct image_t *(*RegisterPic) (const char *name);
	void	(*EndRegistration) (void);

	void	(*RenderFrame) (refdef_t *fd);

	void	(*DrawGetPicSize) (int *w, int *h, const char *name);	// will return 0 0 if not found
	void	(*DrawPic) (int x, int y, const char *name);
	void	(*DrawStretchPic) (int x, int y, int w, int h, const char *name);
	void	(*DrawChar) (int x, int y, int c);
	void	(*DrawTileClear) (int x, int y, int w, int h, const char *name);
	void	(*DrawFill) (int x, int y, int w, int h, int c);
	void	(*DrawFadeScreen) (void);

	/*
	** video mode and refresh state management entry points
	*/
	void	(*BeginFrame)( float camera_separation );
	void	(*EndFrame) (void);

} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct
{
	void	(*Sys_Error) (int err_level, char *str, ...);

	void	(*Con_Printf) (int print_level, char *str, ...);
} refimport_t;


// this is the only function actually exported at the linker level
typedef	refexport_t	(*GetRefAPI_t) (refimport_t);

void R_ClearScreen();
void Draw_FillRgb(int x, int y, int w, int h, int r, int g, int b);

extern float		v_blend[4];

#endif
