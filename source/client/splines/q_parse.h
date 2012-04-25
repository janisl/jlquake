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

#ifndef __Q_PARSE_H
#define __Q_PARSE_H

// this just controls the comment printing, it doesn't actually load a file
void Com_BeginParseSession(const char* filename);
void Com_EndParseSession();

// Will never return NULL, just empty strings.
// An empty string will only be returned at end of file.
// ParseOnLine will return empty if there isn't another token on this line

// this funny typedef just means a moving pointer into a const char * buffer
const char* Com_Parse(const char** data_p);
const char* Com_ParseOnLine(const char** data_p);

void Com_UngetToken();

void Com_MatchToken(const char** buf_p, const char* match, bool warning = false);

void Com_ScriptError(const char* msg, ...);
void Com_ScriptWarning(const char* msg, ...);

float Com_ParseFloat(const char** buf_p);

void Com_Parse1DMatrix(const char** buf_p, int x, float* m);

#endif
