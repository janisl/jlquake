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

Cvar* q2_hand;
Cvar* clq2_footsteps;
Cvar* clq2_name;
Cvar* clq2_skin;
Cvar* clq2_vwep;
Cvar* clq2_predict;
Cvar* clq2_noskins;
Cvar* clq2_showmiss;
Cvar* clq2_gender;
Cvar* clq2_gender_auto;

q2centity_t clq2_entities[MAX_EDICTS_Q2];

char clq2_weaponmodels[MAX_CLIENTWEAPONMODELS_Q2][MAX_QPATH];
int clq2_num_weaponmodels;

void CLQ2_PingServers_f()
{
	NET_Config(true);		// allow remote

	// send a broadcast packet
	common->Printf("pinging broadcast...\n");

	Cvar* noudp = Cvar_Get("noudp", "0", CVAR_INIT);
	if (!noudp->value)
	{
		netadr_t adr;
		adr.type = NA_BROADCAST;
		adr.port = BigShort(Q2PORT_SERVER);
		NET_OutOfBandPrint(NS_CLIENT, adr, "info %i", Q2PROTOCOL_VERSION);
	}

	// send a packet to each address book entry
	for (int i = 0; i < 16; i++)
	{
		char name[32];
		String::Sprintf(name, sizeof(name), "adr%i", i);
		const char* adrstring = Cvar_VariableString(name);
		if (!adrstring || !adrstring[0])
		{
			continue;
		}

		common->Printf("pinging %s...\n", adrstring);
		netadr_t adr;
		if (!SOCK_StringToAdr(adrstring, &adr, Q2PORT_SERVER))
		{
			common->Printf("Bad address: %s\n", adrstring);
			continue;
		}
		NET_OutOfBandPrint(NS_CLIENT, adr, "info %i", Q2PROTOCOL_VERSION);
	}
}

void CLQ2_ClearState()
{
	Com_Memset(&clq2_entities, 0, sizeof(clq2_entities));
	CLQ2_ClearTEnts();
}

//	Specifies the model that will be used as the world
static void R_BeginRegistrationAndLoadWorld(const char* model)
{
	char fullname[MAX_QPATH];
	String::Sprintf(fullname, sizeof(fullname), "maps/%s.bsp", model);

	R_Shutdown(false);
	CL_InitRenderer();

	R_LoadWorld(fullname);
}

//	Call before entering a new level, or after changing dlls
void CLQ2_PrepRefresh()
{
	if (!cl.q2_configstrings[Q2CS_MODELS + 1][0])
	{
		return;		// no map loaded

	}
	// let the render dll load the map
	char mapname[32];
	String::Cpy(mapname, cl.q2_configstrings[Q2CS_MODELS + 1] + 5);		// skip "maps/"
	mapname[String::Length(mapname) - 4] = 0;		// cut off ".bsp"

	// register models, pics, and skins
	common->Printf("Map: %s\r", mapname);
	SCR_UpdateScreen();
	R_BeginRegistrationAndLoadWorld(mapname);
	common->Printf("                                     \r");

	// precache status bar pics
	common->Printf("pics\r");
	SCR_UpdateScreen();
	SCR_TouchPics();
	common->Printf("                                     \r");

	CLQ2_RegisterTEntModels();

	clq2_num_weaponmodels = 1;
	String::Cpy(clq2_weaponmodels[0], "weapon.md2");

	for (int i = 1; i < MAX_MODELS_Q2 && cl.q2_configstrings[Q2CS_MODELS + i][0]; i++)
	{
		char name[MAX_QPATH];
		String::Cpy(name, cl.q2_configstrings[Q2CS_MODELS + i]);
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
		{
			common->Printf("%s\r", name);
		}
		SCR_UpdateScreen();
		if (name[0] == '#')
		{
			// special player weapon model
			if (clq2_num_weaponmodels < MAX_CLIENTWEAPONMODELS_Q2)
			{
				String::NCpy(clq2_weaponmodels[clq2_num_weaponmodels], cl.q2_configstrings[Q2CS_MODELS + i] + 1,
					sizeof(clq2_weaponmodels[clq2_num_weaponmodels]) - 1);
				clq2_num_weaponmodels++;
			}
		}
		else
		{
			cl.model_draw[i] = R_RegisterModel(cl.q2_configstrings[Q2CS_MODELS + i]);
			if (name[0] == '*')
			{
				cl.model_clip[i] = CM_InlineModel(String::Atoi(cl.q2_configstrings[Q2CS_MODELS + i] + 1));
			}
			else
			{
				cl.model_clip[i] = 0;
			}
		}
		if (name[0] != '*')
		{
			common->Printf("                                     \r");
		}
	}

	common->Printf("images\r");
	SCR_UpdateScreen();
	for (int i = 1; i < MAX_IMAGES_Q2 && cl.q2_configstrings[Q2CS_IMAGES + i][0]; i++)
	{
		cl.q2_image_precache[i] = R_RegisterPic(cl.q2_configstrings[Q2CS_IMAGES + i]);
	}

	common->Printf("                                     \r");
	for (int i = 0; i < MAX_CLIENTS_Q2; i++)
	{
		if (!cl.q2_configstrings[Q2CS_PLAYERSKINS + i][0])
		{
			continue;
		}
		common->Printf("client %i\r", i);
		SCR_UpdateScreen();
		CLQ2_ParseClientinfo(i);
		common->Printf("                                     \r");
	}

	CLQ2_LoadClientinfo(&cl.q2_baseclientinfo, "unnamed\\male/grunt");

	// set sky textures and speed
	common->Printf("sky\r");
	SCR_UpdateScreen();
	float rotate = String::Atof(cl.q2_configstrings[Q2CS_SKYROTATE]);
	vec3_t axis;
	sscanf(cl.q2_configstrings[Q2CS_SKYAXIS], "%f %f %f",
		&axis[0], &axis[1], &axis[2]);
	R_SetSky(cl.q2_configstrings[Q2CS_SKY], rotate, axis);
	common->Printf("                                     \r");

	R_EndRegistration();

	// clear any lines of console text
	Con_ClearNotify();

	SCR_UpdateScreen();
	cl.q2_refresh_prepped = true;

	// start the cd track
	CDAudio_Play(String::Atoi(cl.q2_configstrings[Q2CS_CDTRACK]), true);
}

void CLQ2_FixUpGender()
{
	char* p;
	char sk[80];

	if (clq2_gender_auto->value)
	{

		if (clq2_gender->modified)
		{
			// was set directly, don't override the user
			clq2_gender->modified = false;
			return;
		}

		String::NCpy(sk, clq2_skin->string, sizeof(sk) - 1);
		if ((p = strchr(sk, '/')) != NULL)
		{
			*p = 0;
		}
		if (String::ICmp(sk, "male") == 0 || String::ICmp(sk, "cyborg") == 0)
		{
			Cvar_SetLatched("gender", "male");
		}
		else if (String::ICmp(sk, "female") == 0 || String::ICmp(sk, "crackhor") == 0)
		{
			Cvar_SetLatched("gender", "female");
		}
		else
		{
			Cvar_SetLatched("gender", "none");
		}
		clq2_gender->modified = false;
	}
}

void CLQ2_Init()
{
}
