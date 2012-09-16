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

#include "../../client.h"
#include "local.h"
#include "cg_public.h"

bool CLET_GetTag(int clientNum, const char* tagname, orientation_t* _or)
{
	return VM_Call(cgvm, ETCG_GET_TAG, clientNum, tagname, _or);
}

bool CLET_CGameCheckKeyExec(int key)
{
	if (!cgvm)
	{
		return false;
	}
	return VM_Call(cgvm, ETCG_CHECKEXECKEY, key);
}

bool CLET_WantsBindKeys()
{
	if (!cgvm)
	{
		return false;
	}
	return VM_Call(cgvm, ETCG_WANTSBINDKEYS);
}

void CLET_CGameBinaryMessageReceived(const char* buf, int buflen, int serverTime)
{
	VM_Call(cgvm, ETCG_MESSAGERECEIVED, buf, buflen, serverTime);
}
