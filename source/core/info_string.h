//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

#define BIG_INFO_STRING		8192  // used for system info key only
#define BIG_INFO_KEY		8192
#define BIG_INFO_VALUE		8192

//
// key / value info strings
//
const char* Info_ValueForKey(const char* S, const char* Key);
void Info_RemoveKey(char* s, const char* key, int MaxSize);
void Info_RemovePrefixedKeys(char* S, char Prefix, int MaxSize);
void Info_SetValueForKey(char* S, const char* Key, const char* Value,
	int MaxSize, int MaxKeySize = BIG_INFO_KEY, int MaxValSize = BIG_INFO_VALUE,
	bool NoHighChars = false, bool LowerCaseVal = false);
bool Info_Validate(const char* S);
void Info_NextPair(const char** s, char* key, char* value);
void Info_Print(const char* s);
