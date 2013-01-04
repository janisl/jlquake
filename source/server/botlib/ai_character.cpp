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

#define MAX_CHARACTERISTICS     80

#define CT_INTEGER              1
#define CT_FLOAT                2
#define CT_STRING               3

#define DEFAULT_CHARACTER       "bots/default_c.c"

//characteristic value
union cvalue
{
	int integer;
	float _float;
	char* string;
};

//a characteristic
struct bot_characteristic_t
{
	char type;					//characteristic type
	cvalue value;				//characteristic value
};

//a bot character
struct bot_character_t
{
	char filename[MAX_QPATH];
	float skill;
	bot_characteristic_t c[1];		//variable sized
};

static bot_character_t* botcharacters[MAX_BOTLIB_CLIENTS_ARRAY + 1];

static bot_character_t* BotCharacterFromHandle(int handle)
{
	if (handle <= 0 || handle > MAX_BOTLIB_CLIENTS)
	{
		BotImport_Print(PRT_FATAL, "character handle %d out of range\n", handle);
		return NULL;
	}
	if (!botcharacters[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid character %d\n", handle);
		return NULL;
	}
	return botcharacters[handle];
}

static void BotDumpCharacter(bot_character_t* ch)
{
	Log_Write("%s", ch->filename);
	Log_Write("skill %f\n", ch->skill);
	Log_Write("{\n");
	for (int i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		switch (ch->c[i].type)
		{
		case CT_INTEGER:
			Log_Write(" %4d %d\n", i, ch->c[i].value.integer);
			break;
		case CT_FLOAT:
			Log_Write(" %4d %f\n", i, ch->c[i].value._float);
			break;
		case CT_STRING:
			Log_Write(" %4d %s\n", i, ch->c[i].value.string);
			break;
		}
	}
	Log_Write("}\n");
}

static void BotFreeCharacterStrings(bot_character_t* ch)
{
	for (int i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		if (ch->c[i].type == CT_STRING)
		{
			Mem_Free(ch->c[i].value.string);
		}
	}
}

static void BotFreeCharacter2(int handle)
{
	if (handle <= 0 || handle > MAX_CLIENTS_Q3)
	{
		BotImport_Print(PRT_FATAL, "character handle %d out of range\n", handle);
		return;
	}
	if (!botcharacters[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid character %d\n", handle);
		return;
	}
	BotFreeCharacterStrings(botcharacters[handle]);
	Mem_Free(botcharacters[handle]);
	botcharacters[handle] = NULL;
}

void BotFreeCharacter(int handle)
{
	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		return;
	}
	BotFreeCharacter2(handle);
}

static void BotDefaultCharacteristics(bot_character_t* ch, bot_character_t* defaultch)
{
	for (int i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		if (ch->c[i].type)
		{
			continue;
		}

		if (defaultch->c[i].type == CT_FLOAT)
		{
			ch->c[i].type = CT_FLOAT;
			ch->c[i].value._float = defaultch->c[i].value._float;
		}
		else if (defaultch->c[i].type == CT_INTEGER)
		{
			ch->c[i].type = CT_INTEGER;
			ch->c[i].value.integer = defaultch->c[i].value.integer;
		}
		else if (defaultch->c[i].type == CT_STRING)
		{
			ch->c[i].type = CT_STRING;
			ch->c[i].value.string = (char*)Mem_Alloc(String::Length(defaultch->c[i].value.string) + 1);
			String::Cpy(ch->c[i].value.string, defaultch->c[i].value.string);
		}
	}
}

static bot_character_t* BotLoadCharacterFromFile(const char* charfile, int skill)
{
	bool foundcharacter = false;
	//a bot character is parsed in two phases
	if (GGameType & (GAME_Quake3 | GAME_ET))
	{
		PC_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	source_t* source = LoadSourceFile(charfile);
	if (GGameType & GAME_ET)
	{
		PC_SetBaseFolder("");
	}
	if (!source)
	{
		BotImport_Print(PRT_ERROR, "counldn't load %s\n", charfile);
		return NULL;
	}	//end if
	bot_character_t* ch = (bot_character_t*)Mem_ClearedAlloc(sizeof(bot_character_t) +
		MAX_CHARACTERISTICS * sizeof(bot_characteristic_t));
	String::Cpy(ch->filename, charfile);
	token_t token;
	while (PC_ReadToken(source, &token))
	{
		if (!String::Cmp(token.string, "skill"))
		{
			if (!PC_ExpectTokenType(source, TT_NUMBER, 0, &token))
			{
				FreeSource(source);
				BotFreeCharacterStrings(ch);
				Mem_Free(ch);
				return NULL;
			}
			if (!PC_ExpectTokenString(source, "{"))
			{
				FreeSource(source);
				BotFreeCharacterStrings(ch);
				Mem_Free(ch);
				return NULL;
			}
			//if it's the correct skill
			if (skill < 0 || (int)token.intvalue == skill)
			{
				foundcharacter = true;
				ch->skill = token.intvalue;
				while (PC_ExpectAnyToken(source, &token))
				{
					if (!String::Cmp(token.string, "}"))
					{
						break;
					}
					if (token.type != TT_NUMBER || !(token.subtype & TT_INTEGER))
					{
						SourceError(source, "expected integer index, found %s\n", token.string);
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						Mem_Free(ch);
						return NULL;
					}
					int index = token.intvalue;
					if (index < 0 || index > MAX_CHARACTERISTICS)
					{
						SourceError(source, "characteristic index out of range [0, %d]\n", MAX_CHARACTERISTICS);
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						Mem_Free(ch);
						return NULL;
					}
					if (ch->c[index].type)
					{
						SourceError(source, "characteristic %d already initialized\n", index);
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						Mem_Free(ch);
						return NULL;
					}
					if (!PC_ExpectAnyToken(source, &token))
					{
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						Mem_Free(ch);
						return NULL;
					}
					if (token.type == TT_NUMBER)
					{
						if (token.subtype & TT_FLOAT)
						{
							ch->c[index].value._float = token.floatvalue;
							ch->c[index].type = CT_FLOAT;
						}
						else
						{
							ch->c[index].value.integer = token.intvalue;
							ch->c[index].type = CT_INTEGER;
						}
					}
					else if (token.type == TT_STRING)
					{
						StripDoubleQuotes(token.string);
						ch->c[index].value.string = (char*)Mem_Alloc(String::Length(token.string) + 1);
						String::Cpy(ch->c[index].value.string, token.string);
						ch->c[index].type = CT_STRING;
					}
					else
					{
						SourceError(source, "expected integer, float or string, found %s\n", token.string);
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						Mem_Free(ch);
						return NULL;
					}
				}
				break;
			}
			else
			{
				int indent = 1;
				while (indent)
				{
					if (!PC_ExpectAnyToken(source, &token))
					{
						FreeSource(source);
						BotFreeCharacterStrings(ch);
						Mem_Free(ch);
						return NULL;
					}
					if (!String::Cmp(token.string, "{"))
					{
						indent++;
					}
					else if (!String::Cmp(token.string, "}"))
					{
						indent--;
					}
				}
			}
		}
		else
		{
			SourceError(source, "unknown definition %s\n", token.string);
			FreeSource(source);
			BotFreeCharacterStrings(ch);
			Mem_Free(ch);
			return NULL;
		}
	}
	FreeSource(source);

	if (!foundcharacter)
	{
		BotFreeCharacterStrings(ch);
		Mem_Free(ch);
		return NULL;
	}
	return ch;
}

static int BotFindCachedCharacter(const char* charfile, float skill)
{
	for (int handle = 1; handle <= MAX_CLIENTS_Q3; handle++)
	{
		if (!botcharacters[handle])
		{
			continue;
		}
		if (String::Cmp(botcharacters[handle]->filename, charfile) == 0 &&
			(skill < 0 || fabs(botcharacters[handle]->skill - skill) < 0.01))
		{
			return handle;
		}
	}
	return 0;
}

static int BotLoadCachedCharacter(const char* charfile, float skill, bool reload)
{
#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif

	//find a free spot for a character
	int handle;
	for (handle = 1; handle <= MAX_BOTLIB_CLIENTS; handle++)
	{
		if (!botcharacters[handle])
		{
			break;
		}
	}
	if (handle > MAX_BOTLIB_CLIENTS)
	{
		return 0;
	}
	//try to load a cached character with the given skill
	if (!reload)
	{
		int cachedhandle = BotFindCachedCharacter(charfile, skill);
		if (cachedhandle)
		{
			BotImport_Print(PRT_MESSAGE, "loaded cached skill %f from %s\n", skill, charfile);
			return cachedhandle;
		}
	}

	int intskill = (int)(skill + 0.5);
	//try to load the character with the given skill
	bot_character_t* ch = BotLoadCharacterFromFile(charfile, intskill);
	if (ch)
	{
		botcharacters[handle] = ch;

		BotImport_Print(PRT_MESSAGE, "loaded skill %d from %s\n", intskill, charfile);
#ifdef DEBUG
		if (bot_developer)
		{
			BotImport_Print(PRT_MESSAGE, "skill %d loaded in %d msec from %s\n", intskill, Sys_Milliseconds() - starttime, charfile);
		}
#endif
		return handle;
	}

	BotImport_Print(PRT_WARNING, "couldn't find skill %d in %s\n", intskill, charfile);

	if (!reload)
	{
		//try to load a cached default character with the given skill
		int cachedhandle = BotFindCachedCharacter(DEFAULT_CHARACTER, skill);
		if (cachedhandle)
		{
			BotImport_Print(PRT_MESSAGE, "loaded cached default skill %d from %s\n", intskill, charfile);
			return cachedhandle;
		}
	}
	//try to load the default character with the given skill
	ch = BotLoadCharacterFromFile(DEFAULT_CHARACTER, intskill);
	if (ch)
	{
		botcharacters[handle] = ch;
		BotImport_Print(PRT_MESSAGE, "loaded default skill %d from %s\n", intskill, charfile);
		return handle;
	}

	if (!reload)
	{
		//try to load a cached character with any skill
		int cachedhandle = BotFindCachedCharacter(charfile, -1);
		if (cachedhandle)
		{
			BotImport_Print(PRT_MESSAGE, "loaded cached skill %f from %s\n", botcharacters[cachedhandle]->skill, charfile);
			return cachedhandle;
		}
	}
	//try to load a character with any skill
	ch = BotLoadCharacterFromFile(charfile, -1);
	if (ch)
	{
		botcharacters[handle] = ch;
		BotImport_Print(PRT_MESSAGE, "loaded skill %f from %s\n", ch->skill, charfile);
		return handle;
	}

	if (!reload)
	{
		//try to load a cached character with any skill
		int cachedhandle = BotFindCachedCharacter(DEFAULT_CHARACTER, -1);
		if (cachedhandle)
		{
			BotImport_Print(PRT_MESSAGE, "loaded cached default skill %f from %s\n", botcharacters[cachedhandle]->skill, charfile);
			return cachedhandle;
		}
	}
	//try to load a character with any skill
	ch = BotLoadCharacterFromFile(DEFAULT_CHARACTER, -1);
	if (ch)
	{
		botcharacters[handle] = ch;
		BotImport_Print(PRT_MESSAGE, "loaded default skill %f from %s\n", ch->skill, charfile);
		return handle;
	}

	BotImport_Print(PRT_WARNING, "couldn't load any skill from %s\n", charfile);
	//couldn't load any character
	return 0;
}

static int BotLoadCharacterSkill(const char* charfile, float skill)
{
	int defaultch = BotLoadCachedCharacter(DEFAULT_CHARACTER, skill, false);
	int ch = BotLoadCachedCharacter(charfile, skill, LibVarGetValue("bot_reloadcharacters"));

	if (defaultch && ch)
	{
		BotDefaultCharacteristics(botcharacters[ch], botcharacters[defaultch]);
	}

	return ch;
}

static int BotInterpolateCharacters(int handle1, int handle2, float desiredskill)
{
	bot_character_t* ch1 = BotCharacterFromHandle(handle1);
	bot_character_t* ch2 = BotCharacterFromHandle(handle2);
	if (!ch1 || !ch2)
	{
		return 0;
	}
	//find a free spot for a character
	int handle;
	for (handle = 1; handle <= MAX_BOTLIB_CLIENTS; handle++)
	{
		if (!botcharacters[handle])
		{
			break;
		}
	}
	if (handle > MAX_BOTLIB_CLIENTS)
	{
		return 0;
	}
	bot_character_t* out = (bot_character_t*)Mem_ClearedAlloc(sizeof(bot_character_t) +
		MAX_CHARACTERISTICS * sizeof(bot_characteristic_t));
	out->skill = desiredskill;
	String::Cpy(out->filename, ch1->filename);
	botcharacters[handle] = out;

	float scale = (float)(desiredskill - (GGameType & GAME_Quake3 ? ch1->skill : 1)) / (ch2->skill - ch1->skill);
	for (int i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		if (ch1->c[i].type == CT_FLOAT && ch2->c[i].type == CT_FLOAT)
		{
			out->c[i].type = CT_FLOAT;
			out->c[i].value._float = ch1->c[i].value._float +
									 (ch2->c[i].value._float - ch1->c[i].value._float) * scale;
		}
		else if (ch1->c[i].type == CT_INTEGER)
		{
			out->c[i].type = CT_INTEGER;
			out->c[i].value.integer = ch1->c[i].value.integer;
		}
		else if (ch1->c[i].type == CT_STRING)
		{
			out->c[i].type = CT_STRING;
			out->c[i].value.string = (char*)Mem_Alloc(String::Length(ch1->c[i].value.string) + 1);
			String::Cpy(out->c[i].value.string, ch1->c[i].value.string);
		}
	}
	return handle;
}

int BotLoadCharacter(const char* charfile, float skill)
{
	//make sure the skill is in the valid range
	if (skill < 1.0)
	{
		skill = 1.0;
	}
	else if (skill > 5.0)
	{
		skill = 5.0;
	}
	//skill 1, 4 and 5 should be available in the character files
	if (skill == 1.0 || skill == 4.0 || skill == 5.0)
	{
		return BotLoadCharacterSkill(charfile, skill);
	}	//end if
		//check if there's a cached skill
	int handle = BotFindCachedCharacter(charfile, skill);
	if (handle)
	{
		BotImport_Print(PRT_MESSAGE, "loaded cached skill %f from %s\n", skill, charfile);
		return handle;
	}	//end if
	int firstskill, secondskill;
	if (skill < 4.0)
	{
		//load skill 1 and 4
		firstskill = BotLoadCharacterSkill(charfile, 1);
		if (!firstskill)
		{
			return 0;
		}
		secondskill = BotLoadCharacterSkill(charfile, 4);
		if (!secondskill)
		{
			return firstskill;
		}
	}
	else
	{
		//load skill 4 and 5
		firstskill = BotLoadCharacterSkill(charfile, 4);
		if (!firstskill)
		{
			return 0;
		}
		secondskill = BotLoadCharacterSkill(charfile, 5);
		if (!secondskill)
		{
			return firstskill;
		}
	}
	//interpolate between the two skills
	handle = BotInterpolateCharacters(firstskill, secondskill, skill);
	if (!handle)
	{
		return 0;
	}
	//write the character to the log file
	BotDumpCharacter(botcharacters[handle]);

	return handle;
}

static bool CheckCharacteristicIndex(int character, int index)
{
	bot_character_t* ch = BotCharacterFromHandle(character);
	if (!ch)
	{
		return false;
	}
	if (index < 0 || index >= MAX_CHARACTERISTICS)
	{
		BotImport_Print(PRT_ERROR, "characteristic %d does not exist\n", index);
		return false;
	}
	if (!ch->c[index].type)
	{
		BotImport_Print(PRT_ERROR, "characteristic %d is not initialized\n", index);
		return false;
	}
	return true;
}

float Characteristic_Float(int character, int index)
{
	bot_character_t* ch = BotCharacterFromHandle(character);
	if (!ch)
	{
		return 0;
	}
	//check if the index is in range
	if (!CheckCharacteristicIndex(character, index))
	{
		return 0;
	}
	//an integer will be converted to a float
	if (ch->c[index].type == CT_INTEGER)
	{
		return (float)ch->c[index].value.integer;
	}
	//floats are just returned
	if (ch->c[index].type == CT_FLOAT)
	{
		return ch->c[index].value._float;
	}
	//cannot convert a string pointer to a float
	BotImport_Print(PRT_ERROR, "characteristic %d is not a float\n", index);
	return 0;
}

float Characteristic_BFloat(int character, int index, float min, float max)
{
	bot_character_t* ch = BotCharacterFromHandle(character);
	if (!ch)
	{
		return 0;
	}
	if (min > max)
	{
		BotImport_Print(PRT_ERROR, "cannot bound characteristic %d between %f and %f\n", index, min, max);
		return 0;
	}
	float value = Characteristic_Float(character, index);
	if (value < min)
	{
		return min;
	}
	if (value > max)
	{
		return max;
	}
	return value;
}

int Characteristic_Integer(int character, int index)
{
	bot_character_t* ch = BotCharacterFromHandle(character);
	if (!ch)
	{
		return 0;
	}
	//check if the index is in range
	if (!CheckCharacteristicIndex(character, index))
	{
		return 0;
	}
	//an integer will just be returned
	if (ch->c[index].type == CT_INTEGER)
	{
		return ch->c[index].value.integer;
	}
	//floats are casted to integers
	if (ch->c[index].type == CT_FLOAT)
	{
		return (int)ch->c[index].value._float;
	}
	BotImport_Print(PRT_ERROR, "characteristic %d is not a integer\n", index);
	return 0;
}

int Characteristic_BInteger(int character, int index, int min, int max)
{
	bot_character_t* ch = BotCharacterFromHandle(character);
	if (!ch)
	{
		return 0;
	}
	if (min > max)
	{
		BotImport_Print(PRT_ERROR, "cannot bound characteristic %d between %d and %d\n", index, min, max);
		return 0;
	}
	int value = Characteristic_Integer(character, index);
	if (value < min)
	{
		return min;
	}
	if (value > max)
	{
		return max;
	}
	return value;
}

void Characteristic_String(int character, int index, char* buf, int size)
{
	bot_character_t* ch = BotCharacterFromHandle(character);
	if (!ch)
	{
		return;
	}
	//check if the index is in range
	if (!CheckCharacteristicIndex(character, index))
	{
		return;
	}
	//an integer will be converted to a float
	if (ch->c[index].type == CT_STRING)
	{
		String::NCpyZ(buf, ch->c[index].value.string, size);
		return;
	}
	BotImport_Print(PRT_ERROR, "characteristic %d is not a string\n", index);
}

void BotShutdownCharacters()
{
	for (int handle = 1; handle <= MAX_BOTLIB_CLIENTS; handle++)
	{
		if (botcharacters[handle])
		{
			BotFreeCharacter2(handle);
		}
	}
}
