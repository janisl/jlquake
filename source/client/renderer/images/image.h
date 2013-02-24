//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

//	Hexen 2 model texture modes, plus special type for skins.
enum
{
	//	standard
	IMG8MODE_Normal,
	//	color 0 transparent
	IMG8MODE_Holey,
	//	All skin modes will fllo fill image
	IMG8MODE_Skin,
	//	color 0 transparent, odd - translucent, even - full value
	IMG8MODE_SkinTransparent,
	//	color 0 transparent
	IMG8MODE_SkinHoley,
	//	special (particle translucency table)
	IMG8MODE_SkinSpecialTrans,
};

struct image_t {
	char imgName[ MAX_QPATH ];					// game path, including extension
	int width, height;						// source image
	int uploadWidth, uploadHeight;			// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	GLuint texnum;							// gl texture binding

	int frameUsed;							// for texture usage in frame statistics

	int internalFormat;

	bool mipmap;
	bool allowPicmip;
	bool characterMIP;						// independant 'character' mip scale
	GLenum wrapClampMode;					// GL_CLAMP or GL_REPEAT

	int hash;								// for fast building of the backupHash
	image_t* next;

	float sl, tl, sh, th;					// 0,0 - 1,1 unless part of the scrap
	bool scrap;
};

void R_InitQ1Palette();
void R_InitQ2Palette();
byte* R_ConvertImage8To32( byte* DataIn, int Width, int Height, int Mode );
byte* R_GetFullBrightImage( byte* data8, byte* data32, int width, int height );
void R_LoadImage( const char* FileName, byte** Pic, int* Width, int* Height, int Mode = IMG8MODE_Normal, byte* TransPixels = NULL );
image_t* R_CreateImage( const char* Name, byte* Data, int Width, int Height, bool MipMap, bool AllowPicMip, GLenum WrapClampMode, bool AllowScrap, bool characterMip = false );
void R_ReUploadImage( image_t* image, byte* data );
bool R_TouchImage( image_t* inImage );
image_t* R_FindImage( const char* name );
image_t* R_FindImageFile( const char* name, bool mipmap, bool allowPicmip, GLenum glWrapClampMode, bool AllowScrap = false, int Mode = IMG8MODE_Normal, byte* TransPixels = NULL, bool characterMIP = false, bool lightmap = false );
void R_SetColorMappings();
void R_GammaCorrect( byte* Buffer, int BufferSize );
void R_InitFogTable();
float R_FogFactor( float S, float T );
void R_InitImages();
void R_DeleteTextures();
void GL_TextureMode( const char* string );
void GL_TextureAnisotropy( float anisotropy );
void R_ScrapUpload();
void R_ImageList_f();
int R_SumOfUsedImages();
void R_BackupImages();
void R_PurgeBackupImages( int purgeCount );

void R_LoadBMP( const char* FileName, byte** Pic, int* Width, int* Height );

void R_LoadJPG( const char* FileName, byte** Pic, int* Width, int* Height );
void R_SaveJPG( const char* FileName, int Quality, int Width, int Height, byte* Buffer );

void R_LoadPCX( const char* FileName, byte** Pic, byte** Palette, int* Width, int* Height );
void R_LoadPCX32( const char* filename, byte** pic, int* width, int* height, int Mode );
void R_SavePCXMem( idList<byte>& buffer, byte* data, int width, int height, byte* palette );

void R_LoadPICMem( byte* Data, byte** Pic, int* Width, int* Height, byte* TransPixels = NULL, int Mode = IMG8MODE_Normal );
void R_LoadPIC( const char* FileName, byte** Pic, int* Width, int* Height, byte* TransPixels = NULL, int Mode = IMG8MODE_Normal );

void R_LoadTGA( const char* FileName, byte** Pic, int* Width, int* Height );
void R_SaveTGA( const char* FileName, byte* Data, int Width, int Height, bool HaveAlpha );

void R_LoadWAL( const char* FileName, byte** Pic, int* Width, int* Height );

extern byte host_basepal[ 768 ];
extern unsigned* d_8to24table;

extern bool scrap_dirty;
