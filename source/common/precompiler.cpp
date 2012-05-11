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

#include "qcommon.h"

token_t* PC_CopyToken(token_t* token)
{
	token_t* t = (token_t*)Mem_Alloc(sizeof(token_t));
	Com_Memcpy(t, token, sizeof(token_t));
	t->next = NULL;
	return t;
}

void PC_FreeToken(token_t* token)
{
	Mem_Free(token);
}

void SourceError(source_t* source, const char* str, ...)
{
	va_list ap;
	char text[1024];
	va_start(ap, str);
	Q_vsnprintf(text, sizeof(text), str, ap);
	va_end(ap);
	common->Printf(S_COLOR_RED "Error: file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text);
}

void SourceWarning(source_t* source, const char* str, ...)
{
	va_list ap;
	char text[1024];
	va_start(ap, str);
	Q_vsnprintf(text, sizeof(text), str, ap);
	va_end(ap);
	common->Printf(S_COLOR_YELLOW "Warning: file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text);
}
