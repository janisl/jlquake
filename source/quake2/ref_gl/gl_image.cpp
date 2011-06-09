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

#include "gl_local.h"

void GL_EnableMultitexture( qboolean enable )
{
	if ( !qglActiveTextureARB )
		return;

	if ( enable )
	{
		GL_SelectTexture(1);
		qglEnable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	else
	{
		GL_SelectTexture(1);
		qglDisable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	GL_SelectTexture(0);
	GL_TexEnv( GL_REPLACE );
}

void GL_MBind( int target, image_t* image)
{
	if (!qglActiveTextureARB)
		return;
	GL_SelectTexture( target );
	GL_Bind(image);
}

//=======================================================

/*
===============
R_RegisterSkinQ2
===============
*/
image_t *R_RegisterSkinQ2 (char *name)
{
	return R_FindImageFile(name, true, true, GL_CLAMP, false, IMG8MODE_Skin);
}
