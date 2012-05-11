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

void PC_PushIndent(source_t* source, int type, int skip)
{
	indent_t* indent = (indent_t*)Mem_Alloc(sizeof(indent_t));
	indent->type = type;
	indent->script = source->scriptstack;
	indent->skip = (skip != 0);
	source->skip += indent->skip;
	indent->next = source->indentstack;
	source->indentstack = indent;
}

void PC_PopIndent(source_t* source, int* type, int* skip)
{
	*type = 0;
	*skip = 0;

	indent_t* indent = source->indentstack;
	if (!indent)
	{
		return;
	}

	//must be an indent from the current script
	if (source->indentstack->script != source->scriptstack)
	{
		return;
	}

	*type = indent->type;
	*skip = indent->skip;
	source->indentstack = source->indentstack->next;
	source->skip -= indent->skip;
	Mem_Free(indent);
}

bool PC_ReadSourceToken(source_t* source, token_t* token)
{
	//if there's no token already available
	while (!source->tokens)
	{
		//if there's a token to read from the script
		if (PS_ReadToken(source->scriptstack, token))
		{
			return true;
		}
		//if at the end of the script
		if (EndOfScript(source->scriptstack))
		{
			//remove all indents of the script
			while (source->indentstack &&
				   source->indentstack->script == source->scriptstack)
			{
				SourceWarning(source, "missing #endif");
				int type, skip;
				PC_PopIndent(source, &type, &skip);
			}
		}
		//if this was the initial script
		if (!source->scriptstack->next)
		{
			return false;
		}
		//remove the script and return to the last one
		script_t* script = source->scriptstack;
		source->scriptstack = source->scriptstack->next;
		FreeScript(script);
	}
	//copy the already available token
	Com_Memcpy(token, source->tokens, sizeof(token_t));
	//free the read token
	token_t* t = source->tokens;
	source->tokens = source->tokens->next;
	PC_FreeToken(t);
	return true;
}

void PC_UnreadSourceToken(source_t* source, token_t* token)
{
	token_t* t = PC_CopyToken(token);
	t->next = source->tokens;
	source->tokens = t;
}
