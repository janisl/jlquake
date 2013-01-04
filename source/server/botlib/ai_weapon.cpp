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

#include "../../common/qcommon.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "local.h"
#include "ai_weight.h"

//weapon configuration: set of weapons with projectiles
struct weaponconfig_t
{
	int numweapons;
	int numprojectiles;
	projectileinfo_t* projectileinfo;
	weaponinfo_t* weaponinfo;
};

//the bot weapon state
struct bot_weaponstate_t
{
	weightconfig_t* weaponweightconfig;		//weapon weight configuration
	int* weaponweightindex;							//weapon weight index
};

//structure field offsets
#define WEAPON_OFS(x) (qintptr) & (((weaponinfo_t*)0)->x)
#define PROJECTILE_OFS(x) (qintptr) & (((projectileinfo_t*)0)->x)

//weapon definition
static fielddef_t weaponinfo_fields[] =
{
	{"number", WEAPON_OFS(number), FT_INT},					//weapon number
	{"name", WEAPON_OFS(name), FT_STRING},						//name of the weapon
	{"level", WEAPON_OFS(level), FT_INT},
	{"model", WEAPON_OFS(model), FT_STRING},				//model of the weapon
	{"weaponindex", WEAPON_OFS(weaponindex), FT_INT},		//index of weapon in inventory
	{"flags", WEAPON_OFS(flags), FT_INT},						//special flags
	{"projectile", WEAPON_OFS(projectile), FT_STRING},		//projectile used by the weapon
	{"numprojectiles", WEAPON_OFS(numprojectiles), FT_INT},	//number of projectiles
	{"hspread", WEAPON_OFS(hspread), FT_FLOAT},				//horizontal spread of projectiles (degrees from middle)
	{"vspread", WEAPON_OFS(vspread), FT_FLOAT},				//vertical spread of projectiles (degrees from middle)
	{"speed", WEAPON_OFS(speed), FT_FLOAT},					//speed of the projectile (0 = instant hit)
	{"acceleration", WEAPON_OFS(acceleration), FT_FLOAT},	//"acceleration" * time (in seconds) + "speed" = projectile speed
	{"recoil", WEAPON_OFS(recoil), FT_FLOAT | FT_ARRAY, 3},	//amount of recoil the player gets from the weapon
	{"offset", WEAPON_OFS(offset), FT_FLOAT | FT_ARRAY, 3},	//projectile start offset relative to eye and view angles
	{"angleoffset", WEAPON_OFS(angleoffset), FT_FLOAT | FT_ARRAY, 3},	//offset of the shoot angles relative to the view angles
	{"extrazvelocity", WEAPON_OFS(extrazvelocity), FT_FLOAT},	//extra z velocity the projectile gets
	{"ammoamount", WEAPON_OFS(ammoamount), FT_INT},			//ammo amount used per shot
	{"ammoindex", WEAPON_OFS(ammoindex), FT_INT},			//index of ammo in inventory
	{"activate", WEAPON_OFS(activate), FT_FLOAT},			//time it takes to select the weapon
	{"reload", WEAPON_OFS(reload), FT_FLOAT},					//time it takes to reload the weapon
	{"spinup", WEAPON_OFS(spinup), FT_FLOAT},					//time it takes before first shot
	{"spindown", WEAPON_OFS(spindown), FT_FLOAT},			//time it takes before weapon stops firing
	{NULL, 0, 0, 0}
};

//projectile definition
static fielddef_t projectileinfo_fields[] =
{
	{"name", PROJECTILE_OFS(name), FT_STRING},				//name of the projectile
	{"model", WEAPON_OFS(model), FT_STRING},				//model of the projectile
	{"flags", PROJECTILE_OFS(flags), FT_INT},					//special flags
	{"gravity", PROJECTILE_OFS(gravity), FT_FLOAT},			//amount of gravity applied to the projectile [0,1]
	{"damage", PROJECTILE_OFS(damage), FT_INT},				//damage of the projectile
	{"radius", PROJECTILE_OFS(radius), FT_FLOAT},			//radius of damage
	{"visdamage", PROJECTILE_OFS(visdamage), FT_INT},		//damage of the projectile to visible entities
	{"damagetype", PROJECTILE_OFS(damagetype), FT_INT},		//type of damage (combination of the DAMAGETYPE_? flags)
	{"healthinc", PROJECTILE_OFS(healthinc), FT_INT},		//health increase the owner gets
	{"push", PROJECTILE_OFS(push), FT_FLOAT},					//amount a player is pushed away from the projectile impact
	{"detonation", PROJECTILE_OFS(detonation), FT_FLOAT},	//time before projectile explodes after fire pressed
	{"bounce", PROJECTILE_OFS(bounce), FT_FLOAT},			//amount the projectile bounces
	{"bouncefric", PROJECTILE_OFS(bouncefric), FT_FLOAT},	//amount the bounce decreases per bounce
	{"bouncestop", PROJECTILE_OFS(bouncestop), FT_FLOAT},	//minimum bounce value before bouncing stops
//recurive projectile definition??
	{NULL, 0, 0, 0}
};

static structdef_t weaponinfo_struct =
{
	sizeof(weaponinfo_t), weaponinfo_fields
};
static structdef_t projectileinfo_struct =
{
	sizeof(projectileinfo_t), projectileinfo_fields
};

static bot_weaponstate_t* botweaponstates[MAX_BOTLIB_CLIENTS_ARRAY + 1];
static weaponconfig_t* weaponconfig;

static bool BotValidWeaponNumber(int weaponnum)
{
	if ((!(GGameType & GAME_ET) && weaponnum <= 0) ||
		(GGameType & GAME_ET && weaponnum < 0) ||
		weaponnum > weaponconfig->numweapons)
	{
		BotImport_Print(PRT_ERROR, "weapon number out of range\n");
		return false;
	}
	return true;
}

static bot_weaponstate_t* BotWeaponStateFromHandle(int handle)
{
	if (handle <= 0 || handle > MAX_BOTLIB_CLIENTS)
	{
		BotImport_Print(PRT_FATAL, "weapon state handle %d out of range\n", handle);
		return NULL;
	}
	if (!botweaponstates[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid weapon state %d\n", handle);
		return NULL;
	}
	return botweaponstates[handle];
}

static weaponconfig_t* LoadWeaponConfig(const char* filename)
{
	int max_weaponinfo = (int)LibVarValue("max_weaponinfo", "64");
	if (max_weaponinfo < 0)
	{
		BotImport_Print(PRT_ERROR, "max_weaponinfo = %d\n", max_weaponinfo);
		max_weaponinfo = 64;
		LibVarSet("max_weaponinfo", "64");
	}
	int max_projectileinfo = (int)LibVarValue("max_projectileinfo", "64");
	if (max_projectileinfo < 0)
	{
		BotImport_Print(PRT_ERROR, "max_projectileinfo = %d\n", max_projectileinfo);
		max_projectileinfo = 64;
		LibVarSet("max_projectileinfo", "64");
	}
	char path[MAX_QPATH];
	String::NCpy(path, filename, MAX_QPATH);
	if (GGameType & GAME_Quake3)
	{
		PC_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	source_t* source = LoadSourceFile(path);
	if (!source)
	{
		BotImport_Print(PRT_ERROR, "counldn't load %s\n", path);
		return NULL;
	}
	//initialize weapon config
	weaponconfig_t* wc = (weaponconfig_t*)Mem_ClearedAlloc(sizeof(weaponconfig_t) +
		max_weaponinfo * sizeof(weaponinfo_t) +
		max_projectileinfo * sizeof(projectileinfo_t));
	wc->weaponinfo = (weaponinfo_t*)((char*)wc + sizeof(weaponconfig_t));
	wc->projectileinfo = (projectileinfo_t*)((char*)wc->weaponinfo +
		max_weaponinfo * sizeof(weaponinfo_t));
	wc->numweapons = max_weaponinfo;
	wc->numprojectiles = 0;
	//parse the source file
	token_t token;
	while (PC_ReadToken(source, &token))
	{
		if (!String::Cmp(token.string, "weaponinfo"))
		{
			weaponinfo_t weaponinfo;
			Com_Memset(&weaponinfo, 0, sizeof(weaponinfo_t));
			if (!ReadStructure(source, &weaponinfo_struct, (char*)&weaponinfo))
			{
				Mem_Free(wc);
				FreeSource(source);
				return NULL;
			}
			if (weaponinfo.number < 0 || weaponinfo.number >= max_weaponinfo)
			{
				BotImport_Print(PRT_ERROR, "weapon info number %d out of range in %s\n", weaponinfo.number, path);
				Mem_Free(wc);
				FreeSource(source);
				return NULL;
			}
			Com_Memcpy(&wc->weaponinfo[weaponinfo.number], &weaponinfo, sizeof(weaponinfo_t));
			wc->weaponinfo[weaponinfo.number].valid = true;
		}
		else if (!String::Cmp(token.string, "projectileinfo"))
		{
			if (wc->numprojectiles >= max_projectileinfo)
			{
				BotImport_Print(PRT_ERROR, "more than %d projectiles defined in %s\n", max_projectileinfo, path);
				Mem_Free(wc);
				FreeSource(source);
				return NULL;
			}
			Com_Memset(&wc->projectileinfo[wc->numprojectiles], 0, sizeof(projectileinfo_t));
			if (!ReadStructure(source, &projectileinfo_struct, (char*)&wc->projectileinfo[wc->numprojectiles]))
			{
				Mem_Free(wc);
				FreeSource(source);
				return NULL;
			}
			wc->numprojectiles++;
		}
		else
		{
			BotImport_Print(PRT_ERROR, "unknown definition %s in %s\n", token.string, path);
			Mem_Free(wc);
			FreeSource(source);
			return NULL;
		}
	}
	FreeSource(source);
	//fix up weapons
	for (int i = 0; i < wc->numweapons; i++)
	{
		if (!wc->weaponinfo[i].valid)
		{
			continue;
		}
		if (!wc->weaponinfo[i].name[0])
		{
			BotImport_Print(PRT_ERROR, "weapon %d has no name in %s\n", i, path);
			Mem_Free(wc);
			return NULL;
		}
		if (!wc->weaponinfo[i].projectile[0])
		{
			BotImport_Print(PRT_ERROR, "weapon %s has no projectile in %s\n", wc->weaponinfo[i].name, path);
			Mem_Free(wc);
			return NULL;
		}
		//find the projectile info and copy it to the weapon info
		int j;
		for (j = 0; j < wc->numprojectiles; j++)
		{
			if (!String::Cmp(wc->projectileinfo[j].name, wc->weaponinfo[i].projectile))
			{
				Com_Memcpy(&wc->weaponinfo[i].proj, &wc->projectileinfo[j], sizeof(projectileinfo_t));
				break;
			}
		}
		if (j == wc->numprojectiles)
		{
			BotImport_Print(PRT_ERROR, "weapon %s uses undefined projectile in %s\n", wc->weaponinfo[i].name, path);
			Mem_Free(wc);
			return NULL;
		}
	}
	if (!wc->numweapons)
	{
		BotImport_Print(PRT_WARNING, "no weapon info loaded\n");
	}
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", path);
	return wc;
}

static int* WeaponWeightIndex(weightconfig_t* wwc, weaponconfig_t* wc)
{
	//initialize item weight index
	int* index = (int*)Mem_ClearedAlloc(sizeof(int) * wc->numweapons);

	for (int i = 0; i < wc->numweapons; i++)
	{
		index[i] = FindFuzzyWeight(wwc, wc->weaponinfo[i].name);
	}
	return index;
}

static void BotFreeWeaponWeights(int weaponstate)
{
	bot_weaponstate_t* ws = BotWeaponStateFromHandle(weaponstate);
	if (!ws)
	{
		return;
	}
	if (ws->weaponweightconfig)
	{
		FreeWeightConfig(ws->weaponweightconfig);
	}
	if (ws->weaponweightindex)
	{
		Mem_Free(ws->weaponweightindex);
	}
}

int BotLoadWeaponWeights(int weaponstate, const char* filename)
{
	bot_weaponstate_t* ws = BotWeaponStateFromHandle(weaponstate);
	if (!ws)
	{
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADWEAPONWEIGHTS : WOLFBLERR_CANNOTLOADWEAPONWEIGHTS;
	}
	BotFreeWeaponWeights(weaponstate);

	if (GGameType & GAME_ET)
	{
		PC_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	ws->weaponweightconfig = ReadWeightConfig(filename);
	if (GGameType & GAME_ET)
	{
		PC_SetBaseFolder("");
	}
	if (!ws->weaponweightconfig)
	{
		BotImport_Print(PRT_FATAL, "couldn't load weapon config %s\n", filename);
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADWEAPONWEIGHTS : WOLFBLERR_CANNOTLOADWEAPONWEIGHTS;
	}
	if (!weaponconfig)
	{
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADWEAPONCONFIG : WOLFBLERR_CANNOTLOADWEAPONCONFIG;
	}
	ws->weaponweightindex = WeaponWeightIndex(ws->weaponweightconfig, weaponconfig);
	return BLERR_NOERROR;
}

void BotGetWeaponInfo(int weaponstate, int weapon, weaponinfo_t* weaponinfo)
{
	if (!BotValidWeaponNumber(weapon))
	{
		return;
	}
	bot_weaponstate_t* ws = BotWeaponStateFromHandle(weaponstate);
	if (!ws)
	{
		return;
	}
	if (!weaponconfig)
	{
		return;
	}
	Com_Memcpy(weaponinfo, &weaponconfig->weaponinfo[weapon], sizeof(weaponinfo_t));
}

int BotChooseBestFightWeapon(int weaponstate, int* inventory)
{
	bot_weaponstate_t* ws = BotWeaponStateFromHandle(weaponstate);
	if (!ws)
	{
		return 0;
	}
	weaponconfig_t* wc = weaponconfig;
	if (!weaponconfig)
	{
		return 0;
	}

	//if the bot has no weapon weight configuration
	if (!ws->weaponweightconfig)
	{
		return 0;
	}

	float bestweight = 0;
	int bestweapon = 0;
	for (int i = 0; i < wc->numweapons; i++)
	{
		if (!wc->weaponinfo[i].valid)
		{
			continue;
		}
		int index = ws->weaponweightindex[i];
		if (index < 0)
		{
			continue;
		}
		float weight = FuzzyWeight(inventory, ws->weaponweightconfig, index);
		if (weight > bestweight)
		{
			bestweight = weight;
			bestweapon = i;
		}
	}
	return bestweapon;
}

void BotResetWeaponState(int weaponstate)
{
	bot_weaponstate_t* ws = BotWeaponStateFromHandle(weaponstate);
	if (!ws)
	{
		return;
	}
	weightconfig_t* weaponweightconfig = ws->weaponweightconfig;
	int* weaponweightindex = ws->weaponweightindex;

	ws->weaponweightconfig = weaponweightconfig;
	ws->weaponweightindex = weaponweightindex;
}

int BotAllocWeaponState()
{
	for (int i = 1; i <= MAX_BOTLIB_CLIENTS; i++)
	{
		if (!botweaponstates[i])
		{
			botweaponstates[i] = (bot_weaponstate_t*)Mem_ClearedAlloc(sizeof(bot_weaponstate_t));
			return i;
		}
	}
	return 0;
}

void BotFreeWeaponState(int handle)
{
	if (handle <= 0 || handle > MAX_BOTLIB_CLIENTS)
	{
		BotImport_Print(PRT_FATAL, "move state handle %d out of range\n", handle);
		return;
	}
	if (!botweaponstates[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid move state %d\n", handle);
		return;
	}
	BotFreeWeaponWeights(handle);
	Mem_Free(botweaponstates[handle]);
	botweaponstates[handle] = NULL;
}

int BotSetupWeaponAI()
{
	if (GGameType & GAME_ET)
	{
		PC_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	const char* file = LibVarString("weaponconfig", "weapons.c");
	weaponconfig = LoadWeaponConfig(file);
	if (GGameType & GAME_ET)
	{
		PC_SetBaseFolder("");
	}
	if (!weaponconfig)
	{
		BotImport_Print(PRT_FATAL, "couldn't load the weapon config\n");
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADWEAPONCONFIG : WOLFBLERR_CANNOTLOADWEAPONCONFIG;
	}

	return BLERR_NOERROR;
}

void BotShutdownWeaponAI()
{
	if (weaponconfig)
	{
		Mem_Free(weaponconfig);
	}
	weaponconfig = NULL;

	for (int i = 1; i <= MAX_CLIENTS_Q3; i++)
	{
		if (botweaponstates[i])
		{
			BotFreeWeaponState(i);
		}
	}
}
