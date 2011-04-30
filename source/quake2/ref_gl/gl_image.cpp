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
		if (glt && glt->type != it_pic && glt->type != it_sky )
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
		switch (image->type)
		{
		case it_skin:
			ri.Con_Printf (PRINT_ALL, "M");
			break;
		case it_sprite:
			ri.Con_Printf (PRINT_ALL, "S");
			break;
		case it_wall:
			ri.Con_Printf (PRINT_ALL, "W");
			break;
		case it_pic:
			ri.Con_Printf (PRINT_ALL, "P");
			break;
		default:
			ri.Con_Printf (PRINT_ALL, " ");
			break;
		}

		ri.Con_Printf (PRINT_ALL,  " %3i %3i: %s\n",
			image->uploadWidth, image->uploadHeight, image->imgName);
	}
	ri.Con_Printf (PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}

image_t* scrap_image;
int	scrap_uploads;

void Scrap_Upload (void)
{
	scrap_uploads++;
	GL_Bind(scrap_image);
	int format;
	int upload_width, upload_height;
	R_UploadImage(scrap_texels, SCRAP_BLOCK_WIDTH, SCRAP_BLOCK_HEIGHT, false, false, false, &format, &upload_width, &upload_height);
	scrap_dirty = false;
}

//=======================================================

/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type)
{
	image_t		*image;
	int			i;

	// find a free image_t
	for (i=0; i<tr.numImages; i++)
	{
		if (!tr.images[i])
			break;
	}
	if (i == tr.numImages)
	{
		if (tr.numImages == MAX_DRAWIMAGES)
			ri.Sys_Error (ERR_DROP, "MAX_GLTEXTURES");
		tr.numImages++;
	}
	image = new image_t;
	Com_Memset(image, 0, sizeof(image_t));
	tr.images[i] = image;

	if (QStr::Length(name) >= sizeof(image->imgName))
		ri.Sys_Error (ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
	QStr::Cpy(image->imgName, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	// load little pics into the scrap
	if (image->type == it_pic && image->width < 64 && image->height < 64)
	{
		int		x, y;

		if (!R_ScrapAllocBlock(image->width, image->height, &x, &y))
			goto nonscrap;
		R_CommonCreateImage(image, pic, width, height, (type != it_pic && type != it_sky),
			(type != it_pic && type != it_sky), type == it_wall ? GL_REPEAT : GL_CLAMP, false, true, x, y);
		image->texnum = scrap_image->texnum;
	}
	else
	{
nonscrap:
		image->texnum = TEXNUM_IMAGES + i;
		GL_Bind(image);
		R_CommonCreateImage(image, pic, width, height, (image->type != it_pic && image->type != it_sky),
			(image->type != it_pic && image->type != it_sky), type == it_wall ? GL_REPEAT : GL_CLAMP, false, false, 0, 0);
	}

	return image;
}

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t	*GL_FindImage (char *name, imagetype_t type)
{
	image_t	*image;
	int		i;
	int		width, height;

	if (!name)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name");

	// look for it
	for (i=0; i<tr.numImages; i++)
	{
		if (tr.images[i] && !QStr::Cmp(name, tr.images[i]->imgName))
		{
			tr.images[i]->registration_sequence = registration_sequence;
			return tr.images[i];
		}
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

	image = GL_LoadPic (name, pic, width, height, type);

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
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages (void)
{
	int		i;

	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	for (i=0; i<tr.numImages; i++)
	{
		if (!tr.images[i])
			continue;		// free image_t slot
		if (tr.images[i]->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (tr.images[i]->type == it_pic)
			continue;		// don't free pics
		// free it
		qglDeleteTextures (1, (GLuint*)&tr.images[i]->texnum);
		delete tr.images[i];
		tr.images[i] = NULL;
	}
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

	scrap_image = new image_t;
	Com_Memset(scrap_image, 0, sizeof(image_t));
	scrap_image->texnum = TEXNUM_SCRAPS;
	GL_Bind(scrap_image);
	R_CommonCreateImage(scrap_image, scrap_texels, SCRAP_BLOCK_WIDTH, SCRAP_BLOCK_HEIGHT, false, false, GL_CLAMP, false, false, 0, 0);
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
		if (!tr.images[i])
			continue;		// free image_t slot
		// free it
		qglDeleteTextures (1, (GLuint*)&tr.images[i]->texnum);
		delete tr.images[i];
		tr.images[i] = NULL;
	}
}

