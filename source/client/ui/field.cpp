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

void Field_Paste(field_t* edit)
{
	char* cbd;
	int pasteLen, i;

	cbd = Sys_GetClipboardData();

	if (!cbd)
	{
		return;
	}

	// send as if typed, so insert / overstrike works properly
	pasteLen = String::Length(cbd);
	for (i = 0; i < pasteLen; i++)
	{
		Field_CharEvent(edit, cbd[i]);
	}

	delete[] cbd;
}

//	Performs the basic line editing functions for the console,
// in-game talk, and menu fields
//	Key events are used for non-printable characters, others are gotten from char events.
bool Field_KeyDownEventCommon(field_t* edit, int key, int len)
{
	return false;
}

void Field_CharEvent(field_t* edit, int ch)
{
	if (ch == 'v' - 'a' + 1)		// ctrl-v is paste
	{
		Field_Paste(edit);
		return;
	}

	if (ch == 'c' - 'a' + 1)		// ctrl-c clears the field
	{
		Field_Clear(edit);
		return;
	}

	int len = String::Length(edit->buffer);

	if (ch == 'h' - 'a' + 1)		// ctrl-h is backspace
	{
		if (edit->cursor > 0)
		{
			memmove(edit->buffer + edit->cursor - 1,
				edit->buffer + edit->cursor, len + 1 - edit->cursor);
			edit->cursor--;
			if (edit->cursor < edit->scroll)
			{
				edit->scroll--;
			}
		}
		return;
	}

	if (ch == 'a' - 'a' + 1)		// ctrl-a is home
	{
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if (ch == 'e' - 'a' + 1)		// ctrl-e is end
	{
		edit->cursor = len;
		edit->scroll = edit->cursor - edit->widthInChars;
		return;
	}

	//
	// ignore any other non printable chars
	//
	if (ch < 32)
	{
		return;
	}

	int maxLen = edit->maxLength ? edit->maxLength : MAX_EDIT_LINE - 1;
	if (key_overstrikeMode)
	{
		if (edit->cursor == maxLen)
		{
			return;
		}
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
	}
	else		// insert mode
	{
		if (len == maxLen)
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
