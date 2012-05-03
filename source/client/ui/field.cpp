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

#include "../client.h"

field_t chatField;

bool key_overstrikeMode;

void Field_CharEventCommon(field_t* edit, int ch, int len)
{
	//
	// ignore any other non printable chars
	//
	if (ch < 32)
	{
		return;
	}

	if (key_overstrikeMode)
	{
		if (edit->cursor == MAX_EDIT_LINE - 1)
		{
			return;
		}
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
	}
	else		// insert mode
	{
		if (len == MAX_EDIT_LINE - 1)
		{
			return;	// all full
		}
		memmove(edit->buffer + edit->cursor + 1,
			edit->buffer + edit->cursor, len + 1 - edit->cursor);
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
	}

	if (edit->cursor >= edit->widthInChars)
	{
		edit->scroll++;
	}

	if (edit->cursor == len + 1)
	{
		edit->buffer[edit->cursor] = 0;
	}
}
