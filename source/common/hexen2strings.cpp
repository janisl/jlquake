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

#include "hexen2strings.h"
#include "common_defs.h"
#include "Common.h"

// For international stuff
int* prh2_string_index = NULL;
char* prh2_global_strings = NULL;
int prh2_string_count = 0;

int* prh2_info_string_index = NULL;
char* prh2_global_info_strings = NULL;
int prh2_info_string_count = 0;

char* h2_puzzle_strings;

static void ComH2_LoadPuzzleStrings()
{
	FS_ReadFile("puzzles.txt", (void**)&h2_puzzle_strings);
}

static void ComH2_LoadGlobalStrings()
{
	if (!FS_ReadFile("strings.txt", (void**)&prh2_global_strings))
	{
		common->FatalError("ComH2_LoadGlobalStrings: couldn't load strings.txt");
	}

	char NewLineChar = -1;

	int count = 0;
	for (int i = 0; prh2_global_strings[i] != 0; i++)
	{
		if (prh2_global_strings[i] == 13 || prh2_global_strings[i] == 10)
		{
			if (NewLineChar == prh2_global_strings[i] || NewLineChar == -1)
			{
				NewLineChar = prh2_global_strings[i];
				count++;
			}
		}
	}

	if (!count)
	{
		common->FatalError("ComH2_LoadGlobalStrings: no string lines found");
	}

	prh2_string_index = (int*)Mem_Alloc((count + 1) * 4);

	count = 0;
	int start = 0;
	for (int i = 0; prh2_global_strings[i] != 0; i++)
	{
		if (prh2_global_strings[i] == 13 || prh2_global_strings[i] == 10)
		{
			if (NewLineChar == prh2_global_strings[i])
			{
				prh2_string_index[count] = start;
				start = i + 1;
				count++;
			}
			else
			{
				start++;
			}

			prh2_global_strings[i] = 0;
		}
		else if (GGameType & GAME_HexenWorld)
		{
			//for indexed prints, translate '^' to a newline
			if (prh2_global_strings[i] == '^')
			{
				prh2_global_strings[i] = '\n';
			}
		}
	}

	prh2_string_count = count;
	common->Printf("Read in %d string lines\n", count);
}

static void ComH2_LoadInfoStrings()
{
	if (!FS_ReadFile("infolist.txt", (void**)&prh2_global_info_strings))
	{
		common->FatalError("ComH2_LoadInfoStrings: couldn't load infolist.txt");
	}

	char NewLineChar = -1;

	int count= 0;
	for (int i = 0; prh2_global_info_strings[i] != 0; i++)
	{
		if (prh2_global_info_strings[i] == 13 || prh2_global_info_strings[i] == 10)
		{
			if (NewLineChar == prh2_global_info_strings[i] || NewLineChar == -1)
			{
				NewLineChar = prh2_global_info_strings[i];
				count++;
			}
		}
	}

	if (!count)
	{
		common->FatalError("ComH2_LoadInfoStrings: no string lines found");
	}

	prh2_info_string_index = (int*)Mem_Alloc((count + 1) * 4);

	int start = 0;
	count = 0;
	for (int i = 0; prh2_global_info_strings[i] != 0; i++)
	{
		if (prh2_global_info_strings[i] == 13 || prh2_global_info_strings[i] == 10)
		{
			if (NewLineChar == prh2_global_info_strings[i])
			{
				prh2_info_string_index[count] = start;
				start = i + 1;
				count++;
			}
			else
			{
				start++;
			}

			prh2_global_info_strings[i] = 0;
		}
	}

	prh2_info_string_count = count;
	common->Printf("Read in %d objectives\n",count);
}

void ComH2_LoadStrings()
{
	ComH2_LoadGlobalStrings();
	ComH2_LoadPuzzleStrings();
	if (!(GGameType & GAME_HexenWorld) && GGameType & GAME_H2Portals)
	{
		ComH2_LoadInfoStrings();
	}
}
