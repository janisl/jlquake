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

static void Field_Paste(field_t* edit)
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

static void Field_Home(field_t* edit)
{
	edit->cursor = 0;
	edit->scroll = 0;
}

static void Field_End(field_t* edit, int len)
{
	edit->cursor = len;
	edit->scroll = edit->cursor - edit->widthInChars;
	if (edit->scroll < 0)
	{
		edit->scroll = 0;
	}
}

//	Performs the basic line editing functions for the console,
// in-game talk, and menu fields
//	Key events are used for non-printable characters, others are gotten from char events.
bool Field_KeyDownEvent(field_t* edit, int key)
{
	// shift-insert is paste
	if (((key == K_INS) || (key == K_KP_INS)) && keys[K_SHIFT].down)
	{
		Field_Paste(edit);
		return true;
	}

	int len = String::Length(edit->buffer);

	if (key == K_DEL || key == K_KP_DEL)
	{
		if (edit->cursor < len)
		{
			memmove(edit->buffer + edit->cursor,
				edit->buffer + edit->cursor + 1, len - edit->cursor);
		}
		return true;
	}

	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
	{
		if (edit->cursor < len)
		{
			edit->cursor++;
		}

		if (edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= len)
		{
			edit->scroll++;
		}
		return true;
	}

	if (key == K_LEFTARROW || key == K_KP_LEFTARROW)
	{
		if (edit->cursor > 0)
		{
			edit->cursor--;
		}
		if (edit->cursor < edit->scroll)
		{
			edit->scroll--;
		}
		return true;
	}

	if (key == K_HOME || key == K_KP_HOME || (String::ToLower(key) == 'a' && keys[K_CTRL].down))
	{
		Field_Home(edit);
		return true;
	}

	if (key == K_END || key == K_KP_END || (String::ToLower(key) == 'e' && keys[K_CTRL].down))
	{
		Field_End(edit, len);
		return true;
	}

	if (key == K_INS)
	{
		key_overstrikeMode = !key_overstrikeMode;
		return true;
	}
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
		Field_Home(edit);
		return;
	}

	if (ch == 'e' - 'a' + 1)		// ctrl-e is end
	{
		Field_End(edit, len);
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
