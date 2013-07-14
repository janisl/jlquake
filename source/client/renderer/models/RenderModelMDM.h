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

#ifndef __idRenderModelMDM__
#define __idRenderModelMDM__

#include "RenderModel.h"

class idRenderModelMDM : public idRenderModel {
public:
	idRenderModelMDM();
	virtual ~idRenderModelMDM();

	virtual bool Load( idList<byte>& buffer, idSkinTranslation* skinTranslation );
};

#endif
