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
#include "local.h"
#include "ai_weight.h"

#define MAX_INVENTORYVALUE          999999

#define MAX_WEIGHT_FILES            128

static weightconfig_t* weightFileList[MAX_WEIGHT_FILES];

static bool ReadValue(source_t* source, float* value)
{
	token_t token;

	if (!PC_ExpectAnyToken(source, &token))
	{
		return false;
	}
	if (!String::Cmp(token.string, "-"))
	{
		SourceWarning(source, "negative value set to zero\n");
		if (!PC_ExpectTokenType(source, TT_NUMBER, 0, &token))
		{
			return false;
		}
	}
	if (token.type != TT_NUMBER)
	{
		SourceError(source, "invalid return value %s\n", token.string);
		return false;
	}
	*value = token.floatvalue;
	return true;
}

static bool ReadFuzzyWeight(source_t* source, fuzzyseperator_t* fs)
{
	if (PC_CheckTokenString(source, "balance"))
	{
		fs->type = WT_BALANCE;
		if (!PC_ExpectTokenString(source, "("))
		{
			return false;
		}
		if (!ReadValue(source, &fs->weight))
		{
			return false;
		}
		if (!PC_ExpectTokenString(source, ","))
		{
			return false;
		}
		if (!ReadValue(source, &fs->minweight))
		{
			return false;
		}
		if (!PC_ExpectTokenString(source, ","))
		{
			return false;
		}
		if (!ReadValue(source, &fs->maxweight))
		{
			return false;
		}
		if (!PC_ExpectTokenString(source, ")"))
		{
			return false;
		}
	}
	else
	{
		fs->type = 0;
		if (!ReadValue(source, &fs->weight))
		{
			return false;
		}
		fs->minweight = fs->weight;
		fs->maxweight = fs->weight;
	}
	if (!PC_ExpectTokenString(source, ";"))
	{
		return false;
	}
	return true;
}

static void FreeFuzzySeperators_r(fuzzyseperator_t* fs)
{
	if (!fs)
	{
		return;
	}
	if (fs->child)
	{
		FreeFuzzySeperators_r(fs->child);
	}
	if (fs->next)
	{
		FreeFuzzySeperators_r(fs->next);
	}
	Mem_Free(fs);
}

static void FreeWeightConfig2(weightconfig_t* config)
{
	for (int i = 0; i < config->numweights; i++)
	{
		FreeFuzzySeperators_r(config->weights[i].firstseperator);
		if (config->weights[i].name)
		{
			Mem_Free(config->weights[i].name);
		}
	}
	Mem_Free(config);
}

void FreeWeightConfig(weightconfig_t* config)
{
	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		return;
	}
	FreeWeightConfig2(config);
}

static fuzzyseperator_t* ReadFuzzySeperators_r(source_t* source)
{
	if (!PC_ExpectTokenString(source, "("))
	{
		return NULL;
	}
	token_t token;
	if (!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token))
	{
		return NULL;
	}
	int index = token.intvalue;
	if (!PC_ExpectTokenString(source, ")"))
	{
		return NULL;
	}
	if (!PC_ExpectTokenString(source, "{"))
	{
		return NULL;
	}
	if (!PC_ExpectAnyToken(source, &token))
	{
		return NULL;
	}
	fuzzyseperator_t* firstfs = NULL;
	fuzzyseperator_t* lastfs = NULL;
	bool founddefault = false;
	do
	{
		bool def = !String::Cmp(token.string, "default");
		if (def || !String::Cmp(token.string, "case"))
		{
			fuzzyseperator_t* fs = (fuzzyseperator_t*)Mem_ClearedAlloc(sizeof(fuzzyseperator_t));
			fs->index = index;
			if (lastfs)
			{
				lastfs->next = fs;
			}
			else
			{
				firstfs = fs;
			}
			lastfs = fs;
			if (def)
			{
				if (founddefault)
				{
					SourceError(source, "switch already has a default\n");
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				}
				fs->value = MAX_INVENTORYVALUE;
				founddefault = true;
			}
			else
			{
				if (!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token))
				{
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				}
				fs->value = token.intvalue;
			}
			if (!PC_ExpectTokenString(source, ":") || !PC_ExpectAnyToken(source, &token))
			{
				FreeFuzzySeperators_r(firstfs);
				return NULL;
			}
			bool newindent = false;
			if (!String::Cmp(token.string, "{"))
			{
				newindent = true;
				if (!PC_ExpectAnyToken(source, &token))
				{
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				}
			}
			if (!String::Cmp(token.string, "return"))
			{
				if (!ReadFuzzyWeight(source, fs))
				{
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				}
			}
			else if (!String::Cmp(token.string, "switch"))
			{
				fs->child = ReadFuzzySeperators_r(source);
				if (!fs->child)
				{
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				}
			}
			else
			{
				SourceError(source, "invalid name %s\n", token.string);
				return NULL;
			}
			if (newindent)
			{
				if (!PC_ExpectTokenString(source, "}"))
				{
					FreeFuzzySeperators_r(firstfs);
					return NULL;
				}
			}
		}
		else
		{
			FreeFuzzySeperators_r(firstfs);
			SourceError(source, "invalid name %s\n", token.string);
			return NULL;
		}
		if (!PC_ExpectAnyToken(source, &token))
		{
			FreeFuzzySeperators_r(firstfs);
			return NULL;
		}
	}
	while (String::Cmp(token.string, "}"));

	if (!founddefault)
	{
		SourceWarning(source, "switch without default\n");
		fuzzyseperator_t* fs = (fuzzyseperator_t*)Mem_ClearedAlloc(sizeof(fuzzyseperator_t));
		fs->index = index;
		fs->value = MAX_INVENTORYVALUE;
		fs->weight = 0;
		fs->next = NULL;
		fs->child = NULL;
		if (lastfs)
		{
			lastfs->next = fs;
		}
		else
		{
			firstfs = fs;
		}
		lastfs = fs;
	}

	return firstfs;
}

weightconfig_t* ReadWeightConfig(const char* filename)
{
#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif

	int avail = 0;
	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		avail = -1;
		for (int n = 0; n < MAX_WEIGHT_FILES; n++)
		{
			weightconfig_t* config = weightFileList[n];
			if (!config)
			{
				if (avail == -1)
				{
					avail = n;
				}
				continue;
			}
			if (String::Cmp(filename, config->filename) == 0)
			{
				return config;
			}
		}

		if (avail == -1)
		{
			BotImport_Print(PRT_ERROR, "weightFileList was full trying to load %s\n", filename);
			return NULL;
		}
	}

	if (GGameType & GAME_Quake3)
	{
		PC_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	source_t* source = LoadSourceFile(filename);
	if (!source)
	{
		BotImport_Print(PRT_ERROR, "counldn't load %s\n", filename);
		return NULL;
	}

	weightconfig_t* config = (weightconfig_t*)Mem_ClearedAlloc(sizeof(weightconfig_t));
	config->numweights = 0;
	String::NCpyZ(config->filename, filename, sizeof(config->filename));
	//parse the item config file
	token_t token;
	while (PC_ReadToken(source, &token))
	{
		if (!String::Cmp(token.string, "weight"))
		{
			if (config->numweights >= MAX_WEIGHTS)
			{
				SourceWarning(source, "too many fuzzy weights\n");
				break;
			}
			if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
			{
				FreeWeightConfig(config);
				FreeSource(source);
				return NULL;
			}
			StripDoubleQuotes(token.string);
			config->weights[config->numweights].name = (char*)Mem_ClearedAlloc(String::Length(token.string) + 1);
			String::Cpy(config->weights[config->numweights].name, token.string);
			if (!PC_ExpectAnyToken(source, &token))
			{
				FreeWeightConfig(config);
				FreeSource(source);
				return NULL;
			}
			bool newindent = false;
			if (!String::Cmp(token.string, "{"))
			{
				newindent = true;
				if (!PC_ExpectAnyToken(source, &token))
				{
					FreeWeightConfig(config);
					FreeSource(source);
					return NULL;
				}
			}
			if (!String::Cmp(token.string, "switch"))
			{
				fuzzyseperator_t* fs = ReadFuzzySeperators_r(source);
				if (!fs)
				{
					FreeWeightConfig(config);
					FreeSource(source);
					return NULL;
				}
				config->weights[config->numweights].firstseperator = fs;
			}
			else if (!String::Cmp(token.string, "return"))
			{
				fuzzyseperator_t* fs = (fuzzyseperator_t*)Mem_ClearedAlloc(sizeof(fuzzyseperator_t));
				fs->index = 0;
				fs->value = MAX_INVENTORYVALUE;
				fs->next = NULL;
				fs->child = NULL;
				if (!ReadFuzzyWeight(source, fs))
				{
					Mem_Free(fs);
					FreeWeightConfig(config);
					FreeSource(source);
					return NULL;
				}
				config->weights[config->numweights].firstseperator = fs;
			}
			else
			{
				SourceError(source, "invalid name %s\n", token.string);
				FreeWeightConfig(config);
				FreeSource(source);
				return NULL;
			}
			if (newindent)
			{
				if (!PC_ExpectTokenString(source, "}"))
				{
					FreeWeightConfig(config);
					FreeSource(source);
					return NULL;
				}
			}
			config->numweights++;
		}
		else
		{
			SourceError(source, "invalid name %s\n", token.string);
			FreeWeightConfig(config);
			FreeSource(source);
			return NULL;
		}
	}
	//free the source at the end of a pass
	FreeSource(source);
	//if the file was located in a pak file
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", filename);
#ifdef DEBUG
	if (bot_developer)
	{
		BotImport_Print(PRT_MESSAGE, "weights loaded in %d msec\n", Sys_Milliseconds() - starttime);
	}
#endif

	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		weightFileList[avail] = config;
	}

	return config;
}

int FindFuzzyWeight(weightconfig_t* wc, const char* name)
{
	for (int i = 0; i < wc->numweights; i++)
	{
		if (!String::Cmp(wc->weights[i].name, name))
		{
			return i;
		}
	}
	return -1;
}

static float FuzzyWeight_r(const int* inventory, fuzzyseperator_t* fs)
{
	if (inventory[fs->index] < fs->value)
	{
		if (fs->child)
		{
			return FuzzyWeight_r(inventory, fs->child);
		}
		else
		{
			return fs->weight;
		}
	}
	else if (fs->next)
	{
		if (inventory[fs->index] < fs->next->value)
		{
			//first weight
			float w1;
			if (fs->child)
			{
				w1 = FuzzyWeight_r(inventory, fs->child);
			}
			else
			{
				w1 = fs->weight;
			}
			//second weight
			float w2;
			if (fs->next->child)
			{
				w2 = FuzzyWeight_r(inventory, fs->next->child);
			}
			else
			{
				w2 = fs->next->weight;
			}
			//the scale factor
			float scale = (inventory[fs->index] - fs->value) / (fs->next->value - fs->value);
			//scale between the two weights
			return scale * w1 + (1 - scale) * w2;
		}
		return FuzzyWeight_r(inventory, fs->next);
	}
	return fs->weight;
}

static float FuzzyWeightUndecided_r(const int* inventory, fuzzyseperator_t* fs)
{
	if (inventory[fs->index] < fs->value)
	{
		if (fs->child)
		{
			return FuzzyWeightUndecided_r(inventory, fs->child);
		}
		else
		{
			return fs->minweight + random() * (fs->maxweight - fs->minweight);
		}
	}
	else if (fs->next)
	{
		if (inventory[fs->index] < fs->next->value)
		{
			//first weight
			float w1;
			if (fs->child)
			{
				w1 = FuzzyWeightUndecided_r(inventory, fs->child);
			}
			else
			{
				w1 = fs->minweight + random() * (fs->maxweight - fs->minweight);
			}
			//second weight
			float w2;
			if (fs->next->child)
			{
				w2 = FuzzyWeight_r(inventory, fs->next->child);
			}
			else
			{
				w2 = fs->next->minweight + random() * (fs->next->maxweight - fs->next->minweight);
			}
			//the scale factor
			float scale = (inventory[fs->index] - fs->value) / (fs->next->value - fs->value);
			//scale between the two weights
			return scale * w1 + (1 - scale) * w2;
		}
		return FuzzyWeightUndecided_r(inventory, fs->next);
	}
	return fs->weight;
}

float FuzzyWeight(const int* inventory, const weightconfig_t* wc, int weightnum)
{
	return FuzzyWeight_r(inventory, wc->weights[weightnum].firstseperator);
}

float FuzzyWeightUndecided(const int* inventory, const weightconfig_t* wc, int weightnum)
{
	return FuzzyWeightUndecided_r(inventory, wc->weights[weightnum].firstseperator);
}

static void EvolveFuzzySeperator_r(fuzzyseperator_t* fs)
{
	if (fs->child)
	{
		EvolveFuzzySeperator_r(fs->child);
	}
	else if (fs->type == WT_BALANCE)
	{
		//every once in a while an evolution leap occurs, mutation
		if (random() < 0.01)
		{
			fs->weight += crandom() * (fs->maxweight - fs->minweight);
		}
		else
		{
			fs->weight += crandom() * (fs->maxweight - fs->minweight) * 0.5;
		}
		//modify bounds if necesary because of mutation
		if (fs->weight < fs->minweight)
		{
			fs->minweight = fs->weight;
		}
		else if (fs->weight > fs->maxweight)
		{
			fs->maxweight = fs->weight;
		}
	}
	if (fs->next)
	{
		EvolveFuzzySeperator_r(fs->next);
	}
}

void EvolveWeightConfig(weightconfig_t* config)
{
	for (int i = 0; i < config->numweights; i++)
	{
		EvolveFuzzySeperator_r(config->weights[i].firstseperator);
	}
}

static int InterbreedFuzzySeperator_r(fuzzyseperator_t* fs1, fuzzyseperator_t* fs2,
	fuzzyseperator_t* fsout)
{
	if (fs1->child)
	{
		if (!fs2->child || !fsout->child)
		{
			BotImport_Print(PRT_ERROR, "cannot interbreed weight configs, unequal child\n");
			return false;
		}
		if (!InterbreedFuzzySeperator_r(fs2->child, fs2->child, fsout->child))
		{
			return false;
		}
	}
	else if (fs1->type == WT_BALANCE)
	{
		if (fs2->type != WT_BALANCE || fsout->type != WT_BALANCE)
		{
			BotImport_Print(PRT_ERROR, "cannot interbreed weight configs, unequal balance\n");
			return false;
		}
		fsout->weight = (fs1->weight + fs2->weight) / 2;
		if (fsout->weight > fsout->maxweight)
		{
			fsout->maxweight = fsout->weight;
		}
		if (fsout->weight > fsout->minweight)
		{
			fsout->minweight = fsout->weight;
		}
	}
	if (fs1->next)
	{
		if (!fs2->next || !fsout->next)
		{
			BotImport_Print(PRT_ERROR, "cannot interbreed weight configs, unequal next\n");
			return false;
		}
		if (!InterbreedFuzzySeperator_r(fs1->next, fs2->next, fsout->next))
		{
			return false;
		}
	}
	return true;
}

void InterbreedWeightConfigs(weightconfig_t* config1, weightconfig_t* config2,
	weightconfig_t* configout)
{
	if (config1->numweights != config2->numweights ||
		config1->numweights != configout->numweights)
	{
		BotImport_Print(PRT_ERROR, "cannot interbreed weight configs, unequal numweights\n");
		return;
	}
	for (int i = 0; i < config1->numweights; i++)
	{
		InterbreedFuzzySeperator_r(config1->weights[i].firstseperator,
			config2->weights[i].firstseperator,
			configout->weights[i].firstseperator);
	}
}

void BotShutdownWeights()
{
	for (int i = 0; i < MAX_WEIGHT_FILES; i++)
	{
		if (weightFileList[i])
		{
			FreeWeightConfig2(weightFileList[i]);
			weightFileList[i] = NULL;
		}
	}
}
