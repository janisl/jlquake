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

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

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

void GL_SelectTexture(int tmu)
{
	if ( !qglActiveTextureARB )
		return;

	if ( tmu == glState.currenttmu )
		return;

	glState.currenttmu = tmu;

	if ( tmu == 0 )
		qglActiveTextureARB( GL_TEXTURE0_ARB );
	else
		qglActiveTextureARB( GL_TEXTURE1_ARB );
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

void GL_Bind (int texnum)
{
	extern	image_t	*draw_chars;

	if (gl_nobind->value && draw_chars)		// performance evaluation option
		texnum = draw_chars->texnum;
	if ( glState.currenttextures[glState.currenttmu] == texnum)
		return;
	glState.currenttextures[glState.currenttmu] = texnum;
	qglBindTexture (GL_TEXTURE_2D, texnum);
}

void GL_MBind( int target, int texnum )
{
	GL_SelectTexture( target );
	if ( target == 0 )
	{
		if ( glState.currenttextures[0] == texnum )
			return;
	}
	else
	{
		if ( glState.currenttextures[1] == texnum )
			return;
	}
	GL_Bind( texnum );
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

typedef struct
{
	char *name;
	int mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = {
	{"default", 4},
	{"GL_RGBA", GL_RGBA},
	{"GL_RGBA8", GL_RGBA8},
	{"GL_RGB5_A1", GL_RGB5_A1},
	{"GL_RGBA4", GL_RGBA4},
	{"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t gl_solid_modes[] = {
	{"default", 3},
	{"GL_RGB", GL_RGB},
	{"GL_RGB8", GL_RGB8},
	{"GL_RGB5", GL_RGB5},
	{"GL_RGB4", GL_RGB4},
	{"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
	{"GL_RGB2", GL_RGB2_EXT},
#endif
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))

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
			GL_Bind (glt->texnum);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
GL_TextureAlphaMode
===============
*/
void GL_TextureAlphaMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_ALPHA_MODES ; i++)
	{
		if ( !QStr::ICmp( gl_alpha_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_ALPHA_MODES)
	{
		ri.Con_Printf (PRINT_ALL, "bad alpha texture mode name\n");
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
GL_TextureSolidMode
===============
*/
void GL_TextureSolidMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_SOLID_MODES ; i++)
	{
		if ( !QStr::ICmp( gl_solid_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_SOLID_MODES)
	{
		ri.Con_Printf (PRINT_ALL, "bad solid texture mode name\n");
		return;
	}

	gl_tex_solid_format = gl_solid_modes[i].mode;
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
	GL_Bind(scrap_image->texnum);
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
		int		i, j, k;

		if (!R_ScrapAllocBlock(image->width, image->height, &x, &y))
			goto nonscrap;
		scrap_dirty = true;

		// copy the texels into the scrap block
		k = 0;
		for (i=0 ; i<image->height ; i++)
			for (j=0 ; j<image->width * 4; j++, k++)
				scrap_texels[(y + i) * SCRAP_BLOCK_WIDTH * 4 + x * 4 + j] = pic[k];
		image->texnum = scrap_image->texnum;
		image->scrap = true;
		image->sl = (x+0.01)/(float)SCRAP_BLOCK_WIDTH;
		image->sh = (x+image->width-0.01)/(float)SCRAP_BLOCK_WIDTH;
		image->tl = (y+0.01)/(float)SCRAP_BLOCK_WIDTH;
		image->th = (y+image->height-0.01)/(float)SCRAP_BLOCK_WIDTH;
	}
	else
	{
nonscrap:
		image->scrap = false;
		image->texnum = TEXNUM_IMAGES + i;
		GL_Bind(image->texnum);
		int format;
		R_UploadImage(pic, width, height, (image->type != it_pic && image->type != it_sky),
			(image->type != it_pic && image->type != it_sky), false, &format, &image->uploadWidth, &image->uploadHeight);
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
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

