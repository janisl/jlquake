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

#define DEFAULT_CONSOLE_WIDTH   78

console_t con;
field_t g_consoleField;
field_t historyEditLines[COMMAND_HISTORY];
int nextHistoryLine;		// the last line in the history buffer, not masked
int historyLine;			// the line being displayed from history buffer
							// will be <= nextHistoryLine

image_t* conback;

Cvar* cl_noprint;
Cvar* cl_conXOffset;
Cvar* con_notifytime;
Cvar* con_drawnotify;

static vec4_t console_highlightcolor = {0.5, 0.5, 0.2, 0.45};

field_t chatField;
bool chat_team;
bool chat_buddy;

void Con_ClearNotify()
{
	for (int i = 0; i < NUM_CON_TIMES; i++)
	{
		con.times[i] = 0;
	}
}

static void Con_ClearText()
{
	for (int i = 0; i < CON_TEXTSIZE; i++)
	{
		con.text[i] = (ColorIndex(COLOR_WHITE) << 8) | ' ';
	}
}

//	If the line width has changed, reformat the buffer.
static void Con_CheckResize()
{
	int width;
	if (GGameType & GAME_Tech3)
	{
		width = (cls.glconfig.vidWidth / SMALLCHAR_WIDTH) - 2;
	}
	else
	{
		width = (viddef.width / SMALLCHAR_WIDTH) - 2;
	}

	if (width == con.linewidth)
	{
		return;
	}

	if (width < 1)			// video hasn't been initialized yet
	{
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		Con_ClearText();
	}
	else
	{
		int oldwidth = con.linewidth;
		con.linewidth = width;
		int oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		int numlines = oldtotallines;

		if (con.totallines < numlines)
		{
			numlines = con.totallines;
		}

		int numchars = oldwidth;

		if (con.linewidth < numchars)
		{
			numchars = con.linewidth;
		}

		short tbuf[CON_TEXTSIZE];
		Com_Memcpy(tbuf, con.text, CON_TEXTSIZE * sizeof(short));
		Con_ClearText();

		for (int i = 0; i < numlines; i++)
		{
			for (int j = 0; j < numchars; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
					tbuf[((con.current - i + oldtotallines) % oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify();
	}

	con.current = con.totallines - 1;
	con.display = con.current;

	g_consoleField.widthInChars = con.linewidth;
	for (int i = 0; i < COMMAND_HISTORY; i++)
	{
		historyEditLines[i].widthInChars = con.linewidth;
	}
}

static void Con_Linefeed(bool skipNotify)
{
	// mark time for transparent overlay
	if (con.current >= 0)
	{
		if (skipNotify)
		{
			con.times[con.current % NUM_CON_TIMES] = 0;
		}
		else
		{
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}

	con.x = 0;
	if (con.display == con.current)
	{
		con.display++;
	}
	con.current++;
	int startPos = (con.current % con.totallines) * con.linewidth;
	for (int i = 0; i < con.linewidth; i++)
	{
		con.text[startPos + i] = (ColorIndex(COLOR_WHITE) << 8) | ' ';
	}
}

void CL_ConsolePrintCommon(const char*& txt, int mask)
{
	// for some demos we don't want to ever show anything on the console
	if (cl_noprint && cl_noprint->integer)
	{
		return;
	}

	if (!con.initialized)
	{
		con.color[0] =
			con.color[1] =
			con.color[2] =
			con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize();
		con.initialized = true;
	}

	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	bool skipnotify = false;
	if (!String::NCmp(txt, "[skipnotify]", 12))
	{
		skipnotify = true;
		txt += 12;
	}

	int color = ColorIndex(COLOR_WHITE);

	char c;
	while ((c = *txt))
	{
		if (Q_IsColorString(txt))
		{
			if (*(txt + 1) == COLOR_NULL)
			{
				color = ColorIndex(COLOR_WHITE);
			}
			else
			{
				color = ColorIndex(*(txt + 1));
			}
			txt += 2;
			continue;
		}

		// count word length
		int l;
		for (l = 0; l < con.linewidth; l++)
		{
			if (txt[l] <= ' ')
			{
				break;
			}
		}

		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth))
		{
			Con_Linefeed(skipnotify);
		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed(skipnotify);
			break;

		case '\r':
			con.x = 0;
			break;

		default:	// display character and advance
			int y = con.current % con.totallines;
			// rain - sign extension caused the character to carry over
			// into the color info for high ascii chars; casting c to unsigned
			con.text[y * con.linewidth + con.x] = (color << 8) | (unsigned char)c | mask | con.ormask;
			con.x++;
			if (con.x >= con.linewidth)
			{
				Con_Linefeed(skipnotify);
			}
			break;
		}
	}

	// mark time for transparent overlay
	if (con.current >= 0)
	{
		// NERVE - SMF
		if (skipnotify)
		{
			int prev = con.current % NUM_CON_TIMES - 1;
			if (prev < 0)
			{
				prev = NUM_CON_TIMES - 1;
			}
			con.times[prev] = 0;
		}
		else
		{
			// -NERVE - SMF
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}
}

static void Con_DrawBackground(float frac, int lines)
{
	if (GGameType & GAME_QuakeHexen)
	{
		int y = (viddef.height * 3) >> 2;
		if (lines > y)
		{
			UI_DrawStretchPic(0, lines - viddef.height, viddef.width, viddef.height, conback);
		}
		else
		{
			UI_DrawStretchPic(0, lines - viddef.height, viddef.width, viddef.height, conback, (float)(1.2 * lines) / y);
		}

		if (!clc.download)
		{
			const char* version;
			if (GGameType & GAME_QuakeWorld)
			{
				version = S_COLOR_ORANGE "JLQuakeWorld " JLQUAKE_VERSION_STRING;
			}
			else if (GGameType & GAME_Quake)
			{
				version = S_COLOR_ORANGE "JLQuake " JLQUAKE_VERSION_STRING;
			}
			else if (GGameType & GAME_HexenWorld)
			{
				version = S_COLOR_RED "JLHexenWorld " JLQUAKE_VERSION_STRING;
			}
			else
			{
				version = S_COLOR_RED "JLHexen II " JLQUAKE_VERSION_STRING;
			}
			int y = lines - 14;
			int x = viddef.width - (String::LengthWithoutColours(version) * 8 + 11);
			UI_DrawString(x, y, version);
		}
	}
	else if (GGameType & GAME_Quake2)
	{
		UI_DrawStretchNamedPic(0, -viddef.height + lines, viddef.width, viddef.height, "conback");

		const char* version = S_COLOR_GREEN "JLQuake II " JLQUAKE_VERSION_STRING;
		UI_DrawString(viddef.width - 4 - String::LengthWithoutColours(version) * 8, lines - 12, version);
	}
	else
	{
		int y = frac * viddef.height - 2;
		if (y < 1)
		{
			y = 0;
		}
		else
		{
			SCR_DrawPic(0, 0, viddef.width, y, cls.consoleShader);

			// only draw when the console is down all the way (for now)
			if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET) && frac >= 0.5f)
			{
				// draw the logo
				SCR_DrawPic(192, 70, 256, 128, cls.consoleShader2);
			}
		}

		vec4_t color;
		if (GGameType & (GAME_WolfSP | GAME_WolfMP))
		{
			color[0] = 0;
			color[1] = 0;
			color[2] = 0;
			color[3] = 0.6f;
			SCR_FillRect(0, y, viddef.width, 2, color);
		}
		else if (GGameType & GAME_ET)
		{
			// ydnar: matching light text
			color[0] = 0.75;
			color[1] = 0.75;
			color[2] = 0.75;
			color[3] = 1.0f;
			if (frac < 1.0)
			{
				SCR_FillRect(0, y, viddef.width, 1.25, color);
			}
		}
		else
		{
			color[0] = 1;
			color[1] = 0;
			color[2] = 0;
			color[3] = 1;
			SCR_FillRect(0, y, viddef.width, 2, color);
		}

		// draw the version number
		const char* version;
		if (GGameType & GAME_Quake3)
		{
			version = S_COLOR_RED "JLQuake III " JLQUAKE_VERSION_STRING;
		}
		else if (GGameType & GAME_WolfSP)
		{
			version = S_COLOR_WHITE "JLWolfSP " JLQUAKE_VERSION_STRING;
		}
		else if (GGameType & GAME_WolfMP)
		{
			version = S_COLOR_WHITE "JLWolfMP " JLQUAKE_VERSION_STRING;
		}
		else
		{
			version = S_COLOR_WHITE "JLET " JLQUAKE_VERSION_STRING;
		}
		int i = String::LengthWithoutColours(version);
		SCR_DrawSmallString(cls.glconfig.vidWidth - i * SMALLCHAR_WIDTH,
			(lines - (SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 2)), version);
	}
}

void Con_DrawFullBackground()
{
	Con_DrawBackground(1, viddef.height);
}

static void Con_DrawText(int lines)
{
	int charHeight = GGameType & GAME_Tech3 ? SMALLCHAR_HEIGHT : 8;

	int rows = (lines - charHeight) / charHeight;		// rows of text to draw

	int y = lines - charHeight * 3;
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld | GAME_Quake2))
	{
		y -= 6;
	}

	// draw from the bottom up
	if (con.display != con.current)
	{
		// draw arrows to show the buffer is backscrolled
		if (GGameType & GAME_Tech3)
		{
			R_SetColor(g_color_table[ColorIndex(COLOR_WHITE)]);
		}
		for (int x = 0; x < con.linewidth; x += 4)
		{
			if (!(GGameType & GAME_Tech3))
			{
				UI_DrawChar((x + 1) * SMALLCHAR_WIDTH, y, '^', 1, 1, 1, 1);
			}
			else
			{
				SCR_DrawSmallChar(con.xadjust + (x + 1) * SMALLCHAR_WIDTH, y, '^');
			}
		}
		y -= charHeight;
		rows--;
	}

	int row = con.display;
	if (con.x == 0)
	{
		row--;
	}

	int currentColor = 7;
	if (GGameType & GAME_Tech3)
	{
		R_SetColor(g_color_table[currentColor]);
	}

	for (int i = 0; i < rows; i++, y -= charHeight, row--)
	{
		if (row < 0)
		{
			break;
		}
		if (con.current - row >= con.totallines)
		{
			// past scrollback wrap point
			continue;
		}

		short* text = con.text + (row % con.totallines) * con.linewidth;

		for (int x = 0; x < con.linewidth; x++)
		{
			if ((text[x] & 0xff) == ' ')
			{
				continue;
			}

			if (((text[x] >> 8) & COLOR_BITS) != currentColor)
			{
				currentColor = (text[x] >> 8) & COLOR_BITS;
				if (GGameType & GAME_Tech3)
				{
					R_SetColor(g_color_table[currentColor]);
				}
			}
			if (!(GGameType & GAME_Tech3))
			{
				float* c = g_color_table[currentColor];
				UI_DrawChar((x + 1) * SMALLCHAR_WIDTH, y, text[x] & 0xff, c[0], c[1], c[2], c[3]);
			}
			else
			{
				SCR_DrawSmallChar(con.xadjust + (x + 1) * SMALLCHAR_WIDTH, y, text[x] & 0xff);
			}
		}
	}
}

static void Con_DrawDownloadBar(int lines)
{
	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld | GAME_Quake2)))
	{
		return;
	}
	if (!clc.download)
	{
		return;
	}

	char dlbar[1024];
	// figure out width
	char* text = String::RChr(clc.downloadName, '/');
	if (text)
	{
		text++;
	}
	else
	{
		text = clc.downloadName;
	}

	int x = con.linewidth - ((con.linewidth * 7) / 40);
	int y = x - String::Length(text) - 8;
	int i = con.linewidth / 3;
	if (String::Length(text) > i)
	{
		y = x - i - 11;
		String::NCpy(dlbar, text, i);
		dlbar[i] = 0;
		String::Cat(dlbar, sizeof(dlbar), "...");
	}
	else
	{
		String::Cpy(dlbar, text);
	}
	String::Cat(dlbar, sizeof(dlbar), ": ");
	i = String::Length(dlbar);
	dlbar[i++] = '\x80';

	// where's the dot go?
	int n = y * clc.downloadPercent / 100;
	for (int j = 0; j < y; j++)
	{
		if (j == n)
		{
			dlbar[i++] = '\x83';
		}
		else
		{
			dlbar[i++] = '\x81';
		}
	}
	dlbar[i++] = '\x82';
	dlbar[i] = 0;

	sprintf(dlbar + String::Length(dlbar), " %02d%%", clc.downloadPercent);

	// draw it
	y = lines - 12;
	UI_DrawString(8, y, dlbar);
}

//	The input line scrolls horizontally if typing goes beyond the right edge
static void Con_DrawInput(int lines)
{
	if (!(in_keyCatchers & KEYCATCH_CONSOLE))
	{
		if (GGameType & GAME_Tech3)
		{
			if (cls.state != CA_DISCONNECTED)
			{
				return;
			}
		}
		else if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld | GAME_Quake2))
		{
			if (cls.state == CA_ACTIVE)
			{
				return;
			}
		}
		else
		{
			if (cls.state == CA_ACTIVE && clc.qh_signon == SIGNONS)
			{
				return;
			}
		}
	}

	int charHeight = GGameType & GAME_Tech3 ? SMALLCHAR_HEIGHT : 8;
	int y = lines - (charHeight * 2);
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld | GAME_Quake2))
	{
		y -= 6;
	}

	// hightlight the current autocompleted part
	if (con.acLength)
	{
		Cmd_TokenizeString(g_consoleField.buffer);

		if (String::Length(Cmd_Argv(0)) - con.acLength > 0)
		{
			if (GGameType & GAME_Tech3)
			{
				R_SetColor(console_highlightcolor);
				R_StretchPic(con.xadjust + (2 + con.acLength) * SMALLCHAR_WIDTH,
					y + 2,
					(String::Length(Cmd_Argv(0)) - con.acLength) * SMALLCHAR_WIDTH,
					charHeight - 2, 0, 0, 0, 0, cls.whiteShader);
			}
			else
			{
				UI_Fill(con.xadjust + (2 + con.acLength) * SMALLCHAR_WIDTH, y + 1,
					(String::Length(Cmd_Argv(0)) - con.acLength) * SMALLCHAR_WIDTH,
					charHeight - 1, console_highlightcolor[0],
					console_highlightcolor[1], console_highlightcolor[2], console_highlightcolor[3]);
			}
		}
	}

	if (GGameType & GAME_Tech3)
	{
		R_SetColor(con.color);
		SCR_DrawSmallChar(con.xadjust + 1 * SMALLCHAR_WIDTH, y, ']');
	}
	else
	{
		UI_DrawString(8, y, "]");
	}

	Field_Draw(&g_consoleField, con.xadjust + 2 * SMALLCHAR_WIDTH, y, true);
}

static void Con_DrawSolidConsole(float frac)
{
	int lines;
	if (GGameType & GAME_Tech3)
	{
		lines = cls.glconfig.vidHeight * frac;
		if (lines > cls.glconfig.vidHeight)
		{
			lines = cls.glconfig.vidHeight;
		}
	}
	else
	{
		lines = viddef.height * frac;
		if (lines > viddef.height)
		{
			lines = viddef.height;
		}
	}
	if (lines <= 0)
	{
		return;
	}

	// on wide screens, we will center the text
	con.xadjust = 0;
	UI_AdjustFromVirtualScreen(&con.xadjust, NULL, NULL, NULL);

	Con_DrawBackground(frac, lines);

	Con_DrawText(lines);

	Con_DrawDownloadBar(lines);

	Con_DrawInput(lines);

	if (GGameType & GAME_Tech3)
	{
		R_SetColor(NULL);
	}
}

//	Draws the last few lines of output transparently over the game top
static int Con_DrawNotify()
{
	int charHeight = GGameType & GAME_Tech3 ? SMALLCHAR_HEIGHT : 8;

	int currentColor = 7;
	if (GGameType & GAME_Tech3)
	{
		R_SetColor(g_color_table[currentColor]);
	}

	int y = 0;
	for (int i = con.current - NUM_CON_TIMES + 1; i <= con.current; i++)
	{
		if (i < 0)
		{
			continue;
		}
		int time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
		{
			continue;
		}
		time = cls.realtime - time;
		if (time > con_notifytime->value * 1000)
		{
			continue;
		}
		short* text = con.text + (i % con.totallines) * con.linewidth;

		for (int x = 0; x < con.linewidth; x++)
		{
			if ((text[x] & 0xff) == ' ')
			{
				continue;
			}
			if (((text[x] >> 8) & COLOR_BITS) != currentColor)
			{
				currentColor = (text[x] >> 8) & COLOR_BITS;
				if (GGameType & GAME_Tech3)
				{
					R_SetColor(g_color_table[currentColor]);
				}
			}
			if (GGameType & GAME_Tech3)
			{
				SCR_DrawSmallChar(cl_conXOffset->integer + con.xadjust + (x + 1) * SMALLCHAR_WIDTH, y, text[x] & 0xff);
			}
			else
			{
				float* c = g_color_table[currentColor];
				UI_DrawChar((x + 1) * SMALLCHAR_WIDTH, y, text[x] & 0xff, c[0], c[1], c[2], c[3]);
			}
		}

		y += charHeight;
	}

	if (GGameType & GAME_Tech3)
	{
		R_SetColor(NULL);
	}

	return y;
}

static void Con_DrawChat(int y)
{
	if (in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME))
	{
		return;
	}

	if (!(in_keyCatchers & KEYCATCH_MESSAGE))
	{
		return;
	}

	char buf[128];
	if (chat_team)
	{
		CL_TranslateString("say_team:", buf);
	}
	else if (chat_buddy)
	{
		CL_TranslateString("say_fireteam:", buf);
	}
	else
	{
		CL_TranslateString("say:", buf);
	}

	int skip = String::Length(buf) + 2;
	if (GGameType & GAME_Tech3)
	{
		SCR_DrawBigString(8, y, buf, 1.0f);
		Field_BigDraw(&chatField, skip * BIGCHAR_WIDTH, y, true);
	}
	else
	{
		UI_DrawString(8, y, buf);
		Field_Draw(&chatField, skip << 3, y, true);
	}
}

static void Con_DrawNotifyAndChat()
{
	// NERVE - SMF - we dont want draw notify in limbo mode
	if (GGameType & GAME_WolfMP && Cvar_VariableIntegerValue("ui_limboMode"))
	{
		return;
	}

	if (in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME))
	{
		if (!(GGameType & GAME_Tech3))
		{
			return;
		}
		if (GGameType & GAME_Quake3 && cl.q3_snap.ps.pm_type != Q3PM_INTERMISSION)
		{
			return;
		}
		if (GGameType & GAME_WolfSP && cl.ws_snap.ps.pm_type != Q3PM_INTERMISSION)
		{
			return;
		}
		if (GGameType & GAME_WolfMP && cl.wm_snap.ps.pm_type != Q3PM_INTERMISSION)
		{
			return;
		}
		if (GGameType & GAME_ET && cl.et_snap.ps.pm_type != Q3PM_INTERMISSION)
		{
			return;
		}
	}

	int y = Con_DrawNotify();

	Con_DrawChat(y);
}

void Con_DrawConsole()
{
	// check for console width changes from a vid mode change
	Con_CheckResize();

	if (GGameType & GAME_Tech3)
	{
		// if disconnected, render console full screen
		if (cls.state == CA_DISCONNECTED)
		{
			if (!(in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)))
			{
				Con_DrawSolidConsole(1.0);
				return;
			}
		}
	}
	else if (GGameType & GAME_Quake2)
	{
		if (cls.state == CA_DISCONNECTED || cls.state == CA_CONNECTING)
		{
			// forced full screen console
			Con_DrawSolidConsole(1.0);
			return;
		}
	}
	else if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		if (cls.state != CA_ACTIVE)
		{
			Con_DrawSolidConsole(1.0);
			return;
		}
	}
	else
	{
		if (cls.state != CA_ACTIVE || clc.qh_signon != SIGNONS)
		{
			Con_DrawSolidConsole(1.0);
			return;
		}
	}

	if (GGameType & GAME_Quake2 && (cls.state != CA_ACTIVE || !cl.q2_refresh_prepped))
	{
		// connected, but can't render
		Con_DrawSolidConsole(0.5);
		UI_Fill(0, viddef.height / 2, viddef.width, viddef.height / 2, 0, 0, 0, 1);
		return;
	}

	if (con.displayFrac)
	{
		Con_DrawSolidConsole(con.displayFrac);
	}
	else
	{
		// draw notify lines
		if (cls.state == CA_ACTIVE && (!(GGameType & GAME_ET) || con_drawnotify->integer))
		{
			Con_DrawNotifyAndChat();
		}
	}
}

static void Con_PageUp()
{
	con.display -= 2;
	if (con.current - con.display >= con.totallines)
	{
		con.display = con.current - con.totallines + 1;
	}
}

static void Con_PageDown()
{
	con.display += 2;
	if (con.display > con.current)
	{
		con.display = con.current;
	}
}

static void Con_Top()
{
	con.display = con.totallines;
	if (con.current - con.display >= con.totallines)
	{
		con.display = con.current - con.totallines + 1;
	}
}

static void Con_Bottom()
{
	con.display = con.current;
}

static bool CheckForCommand()
{
	char command[128];
	const char* cmd, * s;
	int i;

	s = g_consoleField.buffer;

	for (i = 0; i < 127; i++)
		if (s[i] <= ' ')
		{
			break;
		}
		else
		{
			command[i] = s[i];
		}
	command[i] = 0;

	cmd = Cmd_CompleteCommand(command);
	if (!cmd || String::Cmp(cmd, command))
	{
		cmd = Cvar_CompleteVariable(command);
	}
	if (!cmd  || String::Cmp(cmd, command))
	{
		return false;		// just a chat message
	}
	return true;
}

void Con_KeyEvent(int key)
{
	// ctrl-L clears screen
	if (key == 'l' && keys[K_CTRL].down)
	{
		Cbuf_AddText("clear\n");
		return;
	}

	// enter finishes the line
	if (key == K_ENTER || key == K_KP_ENTER)
	{
		con.acLength = 0;

		// if not in the game explicitly prepent a slash if needed
		if (cls.state != CA_ACTIVE && g_consoleField.buffer[0] != '\\' &&
			g_consoleField.buffer[0] != '/')
		{
			char temp[MAX_STRING_CHARS];

			String::NCpyZ(temp, g_consoleField.buffer, sizeof(temp));
			String::Sprintf(g_consoleField.buffer, sizeof(g_consoleField.buffer), "\\%s", temp);
			g_consoleField.cursor++;
		}

		common->Printf("]%s\n", g_consoleField.buffer);

		// leading slash is an explicit command
		if (g_consoleField.buffer[0] == '\\' || g_consoleField.buffer[0] == '/')
		{
			if (!g_consoleField.buffer[1])
			{
				// empty lines just scroll the console without adding to history
				return;
			}
			Cbuf_AddText(g_consoleField.buffer + 1);	// valid command
			Cbuf_AddText("\n");
		}
		else
		{
			if (!g_consoleField.buffer[0])
			{
				// empty lines just scroll the console without adding to history
				return;
			}
			if (GGameType & GAME_Tech3)
			{
				// other text will be chat messages
				Cbuf_AddText("cmd say ");
			}
			else if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
			{
				if (!CheckForCommand())
				{
					// convert to a chat message
					Cbuf_AddText("say ");
				}
			}
			Cbuf_AddText(g_consoleField.buffer);
			Cbuf_AddText("\n");
		}

		// copy line to history buffer
		historyEditLines[nextHistoryLine % COMMAND_HISTORY] = g_consoleField;
		nextHistoryLine++;
		historyLine = nextHistoryLine;

		Field_Clear(&g_consoleField);

		if (cls.state == CA_DISCONNECTED)
		{
			SCR_UpdateScreen();		// force an update, because the command
		}							// may take some time
		return;
	}

	// command completion
	if (key == K_TAB)
	{
		Field_CompleteCommand(&g_consoleField, con.acLength);
		return;
	}

	// clear autocompletion buffer on normal key input
	if ((key >= K_SPACE && key <= K_BACKSPACE) || (key == K_LEFTARROW) || (key == K_RIGHTARROW) ||
		(key >= K_KP_LEFTARROW && key <= K_KP_RIGHTARROW) ||
		(key >= K_KP_SLASH && key <= K_KP_PLUS) || (key >= K_KP_STAR && key <= K_KP_EQUALS))
	{
		con.acLength = 0;
	}

	// command history (ctrl-p ctrl-n for unix style)
	if ((key == K_MWHEELUP && keys[K_SHIFT].down) || (key == K_UPARROW) || (key == K_KP_UPARROW) ||
		((String::ToLower(key) == 'p') && keys[K_CTRL].down))
	{
		if (nextHistoryLine - historyLine < COMMAND_HISTORY &&
			historyLine > 0)
		{
			historyLine--;
		}
		g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];
		con.acLength = 0;
		return;
	}

	if ((key == K_MWHEELDOWN && keys[K_SHIFT].down) || (key == K_DOWNARROW) || (key == K_KP_DOWNARROW) ||
		((String::ToLower(key) == 'n') && keys[K_CTRL].down))
	{
		if (historyLine == nextHistoryLine)
		{
			return;
		}
		historyLine++;
		g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];
		con.acLength = 0;
		return;
	}

	// console scrolling
	if (key == K_PGUP || key == K_KP_PGUP)
	{
		Con_PageUp();
		return;
	}

	if (key == K_PGDN || key == K_KP_PGDN)
	{
		Con_PageDown();
		return;
	}

	if (key == K_MWHEELUP)
	{
		Con_PageUp();
		if (keys[K_CTRL].down)	// hold <ctrl> to accelerate scrolling
		{
			Con_PageUp();
			Con_PageUp();
		}
		return;
	}

	if (key == K_MWHEELDOWN)
	{
		Con_PageDown();
		if (keys[K_CTRL].down)	// hold <ctrl> to accelerate scrolling
		{
			Con_PageDown();
			Con_PageDown();
		}
		return;
	}

	// ctrl-home = top of console
	if ((key == K_HOME || key == K_KP_HOME) && keys[K_CTRL].down)
	{
		Con_Top();
		return;
	}

	// ctrl-end = bottom of console
	if ((key == K_END || key == K_KP_END) && keys[K_CTRL].down)
	{
		Con_Bottom();
		return;
	}

	// pass to the normal editline routine
	Field_KeyDownEvent(&g_consoleField, key);
}

void Con_MessageKeyEvent(int key)
{
	if (key == K_ESCAPE)
	{
		in_keyCatchers &= ~KEYCATCH_MESSAGE;
		Field_Clear(&chatField);
		return;
	}

	Field_KeyDownEvent(&chatField, key);
}

void Con_Clear_f()
{
	Con_ClearText();
	Con_Bottom();		// go to end
}

void Con_InitCommon()
{
}
