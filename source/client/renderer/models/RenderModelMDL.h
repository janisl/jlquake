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

#ifndef __idRenderModelMLD__
#define __idRenderModelMLD__

#include "RenderModel.h"

#define MAX_LBM_HEIGHT      480

#define ALIAS_BASE_SIZE_RATIO       ( 1.0 / 11.0 )
// normalizing factor so player model works out to about
//  1 pixel per triangle

#define MAXALIASFRAMES      256
#define MAXALIASVERTS       2000
#define MAXALIASTRIS        2048

struct mmesh1triangle_t {
	int facesfront;
	int vertindex[ 3 ];
	int stindex[ 3 ];
};

class idRenderModelMDL : public idRenderModel {
public:
	idRenderModelMDL();
	virtual ~idRenderModelMDL();

	virtual bool Load( idList<byte>& buffer, idSkinTranslation* skinTranslation );
};

extern vec3_t mdl_mins;
extern vec3_t mdl_maxs;
extern int mdl_posenum;
extern mesh1hdr_t* mdl_pheader;
extern dmdl_stvert_t mdl_stverts[ MAXALIASVERTS ];
extern mmesh1triangle_t mdl_triangles[ MAXALIASTRIS ];

const void* Mod_LoadAliasFrame( const void* pin, mmesh1framedesc_t* frame );
const void* Mod_LoadAliasGroup( const void* pin, mmesh1framedesc_t* frame );
void* Mod_LoadAllSkins( int numskins, dmdl_skintype_t* pskintype, int mdl_flags, idSkinTranslation* skinTranslation );
void GL_MakeAliasModelDisplayLists( idRenderModel* m, mesh1hdr_t* hdr );

#endif
