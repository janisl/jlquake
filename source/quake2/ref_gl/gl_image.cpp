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

int			base_textureid;		// gltextures[i] = base_textureid+i

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

void GL_TexEnv( GLenum mode )
{
	static int lastmodes[2] = { -1, -1 };

	if ( mode != lastmodes[glState.currenttmu] )
	{
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		lastmodes[glState.currenttmu] = mode;
	}
}

void GL_MBind( int target, image_t* image)
{
	if (!qglActiveTextureARB)
		return;
	GL_SelectTexture( target );
	GL_Bind(image);
}

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( char *string )
{
	int		i;
	image_t	*glt;

	for (i=0 ; i< NUM_GL_MODES ; i++)
	{
		if ( !QStr::ICmp( modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_MODES)
	{
		ri.Con_Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i=0; i<tr.numImages; i++)
	{
		glt = tr.images[i];
		if (glt && glt->mipmap)
		{
			GL_Bind (glt);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
GL_ImageList_f
===============
*/
void	GL_ImageList_f (void)
{
	int		i;
	image_t	*image;
	int		texels;

	ri.Con_Printf (PRINT_ALL, "------------------\n");
	texels = 0;

	for (i=0; i<tr.numImages; i++)
	{
		image = tr.images[i];
		if (!image)
			continue;
		texels += image->uploadWidth*image->uploadHeight;

		ri.Con_Printf (PRINT_ALL,  "%3i %3i: %s\n",
			image->uploadWidth, image->uploadHeight, image->imgName);
	}
	ri.Con_Printf (PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}

int	scrap_uploads;

void Scrap_Upload (void)
{
	scrap_uploads++;
	GL_Bind(tr.scrapImage);
	int format;
	int upload_width, upload_height;
	R_UploadImage(scrap_texels, SCRAP_BLOCK_WIDTH, SCRAP_BLOCK_HEIGHT, false, false, false, &format, &upload_width, &upload_height);
	scrap_dirty = false;
}

//=======================================================

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t	*GL_FindImage (char *name, imagetype_t type)
{
	image_t	*image;
	int		width, height;

	if (!name)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name");

	// look for it
	image = R_FindImage(name);
	if (image)
	{
		return image;
	}

	//
	// load the pic from disk
	//
	byte* pic;
	R_LoadImage(name, &pic, &width, &height, type == it_skin ? IMG8MODE_Skin : IMG8MODE_Normal);

	if (!pic)
	{
		return NULL;
	}

	image = R_CreateImage(name, pic, width, height, (type != it_pic && type != it_sky),
		(type != it_pic && type != it_sky), type == it_wall ? GL_REPEAT : GL_CLAMP, type == it_pic);

	if (pic)
	{
		delete[] pic;
	}

	return image;
}



/*
===============
R_RegisterSkin
===============
*/
image_t *R_RegisterSkin (char *name)
{
	return GL_FindImage (name, it_skin);
}

/*
===============
GL_InitImages
===============
*/
void	GL_InitImages (void)
{
	registration_sequence = 1;

	R_SetColorMappings();

	tr.scrapImage = R_CreateImage("*scrap", scrap_texels, SCRAP_BLOCK_WIDTH, SCRAP_BLOCK_HEIGHT, false, false, GL_CLAMP, false);
}

/*
===============
GL_ShutdownImages
===============
*/
void	GL_ShutdownImages (void)
{
	int		i;

	for (i=0; i<tr.numImages; i++)
	{
		// free it
		qglDeleteTextures (1, (GLuint*)&tr.images[i]->texnum);
		delete tr.images[i];
		tr.images[i] = NULL;
	}
}

