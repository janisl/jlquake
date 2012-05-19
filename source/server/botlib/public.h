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

#ifndef _BOTLIB_PUBLIC_H
#define _BOTLIB_PUBLIC_H

//botlib error codes
#define BLERR_NOERROR                   0	//no error
#define BLERR_LIBRARYNOTSETUP           1	//library not setup
#define Q3BLERR_INVALIDENTITYNUMBER       2	//invalid entity number
#define Q3BLERR_NOAASFILE                 3	//no AAS file available
#define Q3BLERR_CANNOTOPENAASFILE         4	//cannot open AAS file
#define Q3BLERR_WRONGAASFILEID            5	//incorrect AAS file id
#define Q3BLERR_WRONGAASFILEVERSION       6	//incorrect AAS file version
#define Q3BLERR_CANNOTREADAASLUMP         7	//cannot read AAS file lump
#define Q3BLERR_CANNOTLOADICHAT           8	//cannot load initial chats
#define Q3BLERR_CANNOTLOADITEMWEIGHTS     9	//cannot load item weights
#define Q3BLERR_CANNOTLOADITEMCONFIG      10	//cannot load item config
#define Q3BLERR_CANNOTLOADWEAPONWEIGHTS   11	//cannot load weapon weights
#define Q3BLERR_CANNOTLOADWEAPONCONFIG    12	//cannot load weapon config
#define WOLFBLERR_INVALIDENTITYNUMBER       4	//invalid entity number
#define WOLFBLERR_NOAASFILE                 5	//BotLoadMap: no AAS file available
#define WOLFBLERR_CANNOTOPENAASFILE         6	//BotLoadMap: cannot open AAS file
#define WOLFBLERR_WRONGAASFILEID            9	//BotLoadMap: incorrect AAS file id
#define WOLFBLERR_WRONGAASFILEVERSION       10	//BotLoadMap: incorrect AAS file version
#define WOLFBLERR_CANNOTREADAASLUMP         11	//BotLoadMap: cannot read AAS file lump
#define WOLFBLERR_CANNOTLOADICHAT           27	//BotSetupClient: cannot load initial chats
#define WOLFBLERR_CANNOTLOADITEMWEIGHTS     28	//BotSetupClient: cannot load item weights
#define WOLFBLERR_CANNOTLOADITEMCONFIG      29	//BotSetupLibrary: cannot load item config
#define WOLFBLERR_CANNOTLOADWEAPONWEIGHTS   30	//BotSetupClient: cannot load weapon weights
#define WOLFBLERR_CANNOTLOADWEAPONCONFIG    31	//BotSetupLibrary: cannot load weapon config

int BotImport_DebugPolygonCreate(int color, int numPoints, const vec3_t* points);
void BotImport_DebugPolygonDelete(int id);

int BotLibVarSet(const char* var_name, const char* value);
int BotLibVarGet(char* var_name, char* value, int size);

#endif
