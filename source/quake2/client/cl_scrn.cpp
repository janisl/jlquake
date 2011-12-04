/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

/*

  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  cinematics
  full screen image for quit and victory

  end of unit intermissions

  */

#include "client.h"

float		scr_con_current;	// aproaches scr_conlines at scr_conspeed
float		scr_conlines;		// 0.0 to 1.0 lines of console to display

qboolean	scr_initialized;		// ready to draw

int			scr_draw_loading;

vrect_t		scr_vrect;		// position of render window on screen


Cvar		*scr_viewsize;
Cvar		*scr_conspeed;
Cvar		*scr_centertime;
Cvar		*scr_showturtle;
Cvar		*scr_showpause;
Cvar		*scr_printspeed;

Cvar		*scr_netgraph;
Cvar		*scr_timegraph;
Cvar		*scr_debuggraph;
Cvar		*scr_graphheight;
Cvar		*scr_graphscale;
Cvar		*scr_graphshift;

char		crosshair_pic[MAX_QPATH];
int			crosshair_width, crosshair_height;

static image_t		*draw_chars;

void SCR_TimeRefresh_f (void);
void SCR_Loading_f (void);


/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

/*
==============
CL_AddNetgraph

A new packet was just parsed
==============
*/
void CL_AddNetgraph (void)
{
	int		i;
	int		in;
	int		ping;

	// if using the debuggraph for something else, don't
	// add the net lines
	if (scr_debuggraph->value || scr_timegraph->value)
		return;

	for (i=0 ; i<cls.netchan.dropped ; i++)
		SCR_DebugGraph (30, 0x40);

	for (i=0 ; i<cl.surpressCount ; i++)
		SCR_DebugGraph (30, 0xdf);

	// see what the latency was on this packet
	in = cls.netchan.incoming_acknowledged & (CMD_BACKUP-1);
	ping = cls.realtime - cl.cmd_time[in];
	ping /= 30;
	if (ping > 30)
		ping = 30;
	SCR_DebugGraph (ping, 0xd0);
}


typedef struct
{
	float	value;
	int		color;
} graphsamp_t;

static	int			current;
static	graphsamp_t	values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color)
{
	values[current&1023].value = value;
	values[current&1023].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void)
{
	int		a, x, y, w, i, h;
	float	v;
	int		color;

	//
	// draw the graph
	//
	w = scr_vrect.width;

	x = scr_vrect.x;
	y = scr_vrect.y+scr_vrect.height;
	UI_FillPal (x, y-scr_graphheight->value,
		w, scr_graphheight->value, 8);

	for (a=0 ; a<w ; a++)
	{
		i = (current-1-a+1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v*scr_graphscale->value + scr_graphshift->value;
		
		if (v < 0)
			v += scr_graphheight->value * (1+(int)(-v/scr_graphheight->value));
		h = (int)v % (int)scr_graphheight->value;
		UI_FillPal (x+w-1-a, y - h, 1,	h, color);
	}
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	char	*s;
	char	line[64];
	int		i, j, l;

	String::NCpy(scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = cl.serverTime;

	// count the number of lines for centering
	scr_center_lines = 1;
	s = str;
	while (*s)
	{
		if (*s == '\n')
			scr_center_lines++;
		s++;
	}

	// echo it to the console
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

	s = str;
	do	
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (s[l] == '\n' || !s[l])
				break;
		for (i=0 ; i<(40-l)/2 ; i++)
			line[i] = ' ';

		for (j=0 ; j<l ; j++)
		{
			line[i++] = s[j];
		}

		line[i] = '\n';
		line[i+1] = 0;

		Com_Printf ("%s", line);

		while (*s && *s != '\n')
			s++;

		if (!*s)
			break;
		s++;		// skip the \n
	} while (1);
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_ClearNotify ();
}


void SCR_DrawCenterString (void)
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;

// the finale prints the characters one at a time
	remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = viddef.height*0.35;
	else
		y = 48;

	do	
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (viddef.width - l*8)/2;
		for (j=0 ; j<l ; j++, x+=8)
		{
			Draw_Char (x, y, start[j]);	
			if (!remaining--)
				return;
		}
			
		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	scr_centertime_off -= cls.frametimeFloat;
	
	if (scr_centertime_off <= 0)
		return;

	SCR_DrawCenterString ();
}

//=============================================================================

/*
=================
SCR_CalcVrect

Sets scr_vrect, the coordinates of the rendered window
=================
*/
static void SCR_CalcVrect (void)
{
	int		size;

	// bound viewsize
	if (scr_viewsize->value < 40)
		Cvar_SetLatched("viewsize","40");
	if (scr_viewsize->value > 100)
		Cvar_SetLatched("viewsize","100");

	size = scr_viewsize->value;

	scr_vrect.width = viddef.width*size/100;
	scr_vrect.width &= ~7;

	scr_vrect.height = viddef.height*size/100;
	scr_vrect.height &= ~1;

	scr_vrect.x = (viddef.width - scr_vrect.width)/2;
	scr_vrect.y = (viddef.height - scr_vrect.height)/2;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValueLatched("viewsize",scr_viewsize->value+10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValueLatched("viewsize",scr_viewsize->value-10);
}

/*
=================
SCR_Sky_f

Set a specific sky and rotation speed
=================
*/
void SCR_Sky_f (void)
{
	float	rotate;
	vec3_t	axis;

	if (Cmd_Argc() < 2)
	{
		Com_Printf ("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}
	if (Cmd_Argc() > 2)
		rotate = String::Atof(Cmd_Argv(2));
	else
		rotate = 0;
	if (Cmd_Argc() == 6)
	{
		axis[0] = String::Atof(Cmd_Argv(3));
		axis[1] = String::Atof(Cmd_Argv(4));
		axis[2] = String::Atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	R_SetSky (Cmd_Argv(1), rotate, axis);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);
	scr_conspeed = Cvar_Get ("scr_conspeed", "3", 0);
	scr_showturtle = Cvar_Get ("scr_showturtle", "0", 0);
	scr_showpause = Cvar_Get ("scr_showpause", "1", 0);
	scr_centertime = Cvar_Get ("scr_centertime", "2.5", 0);
	scr_printspeed = Cvar_Get ("scr_printspeed", "8", 0);
	scr_netgraph = Cvar_Get ("netgraph", "0", 0);
	scr_timegraph = Cvar_Get ("timegraph", "0", 0);
	scr_debuggraph = Cvar_Get ("debuggraph", "0", 0);
	scr_graphheight = Cvar_Get ("graphheight", "32", 0);
	scr_graphscale = Cvar_Get ("graphscale", "1", 0);
	scr_graphshift = Cvar_Get ("graphshift", "0", 0);

//
// register our commands
//
	Cmd_AddCommand ("timerefresh",SCR_TimeRefresh_f);
	Cmd_AddCommand ("loading",SCR_Loading_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);
	Cmd_AddCommand ("sky",SCR_Sky_f);

	scr_initialized = true;
}


/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (cls.netchan.outgoingSequence - cls.netchan.incoming_acknowledged 
		< CMD_BACKUP-1)
		return;

	UI_DrawNamedPic (scr_vrect.x+64, scr_vrect.y, "net");
}

/*
==============
SCR_DrawPause
==============
*/
void SCR_DrawPause (void)
{
	int		w, h;

	if (!scr_showpause->value)		// turn off for screenshots
		return;

	if (!cl_paused->value)
		return;

	R_GetPicSize (&w, &h, "pause");
	UI_DrawNamedPic ((viddef.width-w)/2, viddef.height/2 + 8, "pause");
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	int		w, h;
		
	if (!scr_draw_loading)
		return;

	scr_draw_loading = false;
	R_GetPicSize (&w, &h, "loading");
	UI_DrawNamedPic ((viddef.width-w)/2, (viddef.height-h)/2, "loading");
}

//=============================================================================

/*
==================
SCR_RunConsole

Scroll it up or down
==================
*/
void SCR_RunConsole (void)
{
// decide on the height of the console
	if (in_keyCatchers & KEYCATCH_CONSOLE)
		scr_conlines = 0.5;		// half screen
	else
		scr_conlines = 0;				// none visible
	
	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed->value*cls.frametimeFloat;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed->value*cls.frametimeFloat;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	Con_CheckResize ();
	
	if (cls.state == ca_disconnected || cls.state == ca_connecting)
	{	// forced full screen console
		Con_DrawConsole (1.0);
		return;
	}

	if (cls.state != ca_active || !cl.refresh_prepped)
	{	// connected, but can't render
		Con_DrawConsole (0.5);
		UI_FillPal (0, viddef.height/2, viddef.width, viddef.height/2, 0);
		return;
	}

	if (scr_con_current)
	{
		Con_DrawConsole (scr_con_current);
	}
	else
	{
		if (in_keyCatchers == 0 || in_keyCatchers == KEYCATCH_MESSAGE)
			Con_DrawNotify ();	// only draw notify in game
	}
}

//=============================================================================

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque(bool Clear)
{
	S_StopAllSounds ();
	cl.sound_prepped = false;		// don't play ambients
	CDAudio_Stop ();
	if (cls.disable_screen)
		return;
	if (developer->value)
		return;
	if (cls.state == ca_disconnected)
		return;	// if at console, don't bring up the plaque
	if (in_keyCatchers & KEYCATCH_CONSOLE)
		return;
	scr_draw_loading = Clear ? 2 : 1;
	SCR_UpdateScreen ();
	cls.disable_screen = Sys_Milliseconds_ ();
	cls.disable_servercount = cl.servercount;
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void)
{
	cls.disable_screen = 0;
	Con_ClearNotify ();
}

/*
================
SCR_Loading_f
================
*/
void SCR_Loading_f (void)
{
	SCR_BeginLoadingPlaque ();
}

void SCR_TimeRefresh_f (void)
{
	int		i;
	int		start, stop;
	float	time;

	if ( cls.state != ca_active )
		return;

	start = Sys_Milliseconds_ ();

	vec3_t viewangles;
	viewangles[0] = 0;
	viewangles[1] = 0;
	viewangles[2] = 0;
	if (Cmd_Argc() == 2)
	{	// run without page flipping
		R_BeginFrame(STEREO_CENTER);
		for (i=0 ; i<128 ; i++)
		{
			viewangles[1] = i/128.0*360.0;
			AnglesToAxis(viewangles, cl.refdef.viewaxis);
			R_RenderScene(&cl.refdef);
		}
		R_EndFrame(NULL, NULL);
	}
	else
	{
		for (i=0 ; i<128 ; i++)
		{
			viewangles[1] = i/128.0*360.0;
			AnglesToAxis(viewangles, cl.refdef.viewaxis);

			R_BeginFrame(STEREO_CENTER);
			R_RenderScene(&cl.refdef);
			R_EndFrame(NULL, NULL);
		}
	}

	stop = Sys_Milliseconds_ ();
	time = (stop-start)/1000.0;
	Com_Printf ("%f seconds (%f fps)\n", time, 128/time);
}

/*
==============
SCR_TileClear

Clear any parts of the tiled background that were drawn on last frame
==============
*/
void SCR_TileClear (void)
{
	if (scr_con_current == 1.0)
		return;		// full screen console
	if (scr_viewsize->value == 100)
		return;		// full screen rendering

	int top = scr_vrect.y;
	int bottom = top + scr_vrect.height;
	int left = scr_vrect.x;
	int right = left + scr_vrect.width;

	if (top > 0)
	{
		// clear above view screen
		UI_NamedTileClear(0, 0, cls.glconfig.vidWidth, top, "backtile");
	}
	if (cls.glconfig.vidHeight > bottom)
	{
		// clear below view screen
		UI_NamedTileClear(0, bottom, cls.glconfig.vidWidth, cls.glconfig.vidHeight - bottom, "backtile");
	}
	if (0 < left)
	{
		// clear left of view screen
		UI_NamedTileClear(0, top, left, bottom - top, "backtile");
	}
	if (cls.glconfig.vidWidth > right)
	{
		// clear left of view screen
		UI_NamedTileClear(right, top, cls.glconfig.vidWidth - right, bottom - top, "backtile");
	}
}


//===============================================================


#define STAT_MINUS		10	// num frame for '-' stats digit
const char		*sb_nums[2][11] = 
{
	{"num_0", "num_1", "num_2", "num_3", "num_4", "num_5",
	"num_6", "num_7", "num_8", "num_9", "num_minus"},
	{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5",
	"anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
};

#define	ICON_WIDTH	24
#define	ICON_HEIGHT	24
#define	CHAR_WIDTH	16
#define	ICON_SPACE	8



/*
================
SizeHUDString

Allow embedded \n in the string
================
*/
void SizeHUDString (char *string, int *w, int *h)
{
	int		lines, width, current;

	lines = 1;
	width = 0;

	current = 0;
	while (*string)
	{
		if (*string == '\n')
		{
			lines++;
			current = 0;
		}
		else
		{
			current++;
			if (current > width)
				width = current;
		}
		string++;
	}

	*w = width * 8;
	*h = lines * 8;
}

void DrawHUDString (char *string, int x, int y, int centerwidth, int _xor)
{
	int		margin;
	char	line[1024];
	int		width;
	int		i;

	margin = x;

	while (*string)
	{
		// scan out one line of text from the string
		width = 0;
		while (*string && *string != '\n')
			line[width++] = *string++;
		line[width] = 0;

		if (centerwidth)
			x = margin + (centerwidth - width*8)/2;
		else
			x = margin;
		for (i=0 ; i<width ; i++)
		{
			Draw_Char (x, y, line[i]^_xor);
			x += 8;
		}
		if (*string)
		{
			string++;	// skip the \n
			x = margin;
			y += 8;
		}
	}
}


/*
==============
SCR_DrawField
==============
*/
void SCR_DrawField (int x, int y, int color, int width, int value)
{
	char	num[16], *ptr;
	int		l;
	int		frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	String::Sprintf (num, sizeof(num), "%i", value);
	l = String::Length(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		UI_DrawNamedPic (x,y,sb_nums[color][frame]);
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}


/*
===============
SCR_TouchPics

Allows rendering code to cache all needed sbar graphics
===============
*/
void SCR_TouchPics (void)
{
	int		i, j;

	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<11 ; j++)
			R_RegisterPic (sb_nums[i][j]);

	if (crosshair->value)
	{
		if (crosshair->value > 3 || crosshair->value < 0)
			crosshair->value = 3;

		String::Sprintf (crosshair_pic, sizeof(crosshair_pic), "ch%i", (int)(crosshair->value));
		R_GetPicSize (&crosshair_width, &crosshair_height, crosshair_pic);
		if (!crosshair_width)
			crosshair_pic[0] = 0;
	}
}

/*
================
SCR_ExecuteLayoutString 

================
*/
void SCR_ExecuteLayoutString (const char *s)
{
	int		x, y;
	int		value;
	char	*token;
	int		width;
	int		index;
	clientinfo_t	*ci;

	if (cls.state != ca_active || !cl.refresh_prepped)
		return;

	if (!s[0])
		return;

	x = 0;
	y = 0;
	width = 3;

	while (s)
	{
		token = String::Parse2(&s);
		if (!String::Cmp(token, "xl"))
		{
			token = String::Parse2 (&s);
			x = String::Atoi(token);
			continue;
		}
		if (!String::Cmp(token, "xr"))
		{
			token = String::Parse2 (&s);
			x = viddef.width + String::Atoi(token);
			continue;
		}
		if (!String::Cmp(token, "xv"))
		{
			token = String::Parse2 (&s);
			x = viddef.width/2 - 160 + String::Atoi(token);
			continue;
		}

		if (!String::Cmp(token, "yt"))
		{
			token = String::Parse2 (&s);
			y = String::Atoi(token);
			continue;
		}
		if (!String::Cmp(token, "yb"))
		{
			token = String::Parse2 (&s);
			y = viddef.height + String::Atoi(token);
			continue;
		}
		if (!String::Cmp(token, "yv"))
		{
			token = String::Parse2 (&s);
			y = viddef.height/2 - 120 + String::Atoi(token);
			continue;
		}

		if (!String::Cmp(token, "pic"))
		{	// draw a pic from a stat number
			token = String::Parse2 (&s);
			value = cl.q2_frame.playerstate.stats[String::Atoi(token)];
			if (value >= MAX_IMAGES)
				Com_Error (ERR_DROP, "Pic >= MAX_IMAGES");
			if (cl.configstrings[CS_IMAGES+value])
			{
				UI_DrawNamedPic (x, y, cl.configstrings[CS_IMAGES+value]);
			}
			continue;
		}

		if (!String::Cmp(token, "client"))
		{	// draw a deathmatch client block
			int		score, ping, time;

			token = String::Parse2 (&s);
			x = viddef.width/2 - 160 + String::Atoi(token);
			token = String::Parse2 (&s);
			y = viddef.height/2 - 120 + String::Atoi(token);

			token = String::Parse2 (&s);
			value = String::Atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = String::Parse2 (&s);
			score = String::Atoi(token);

			token = String::Parse2 (&s);
			ping = String::Atoi(token);

			token = String::Parse2 (&s);
			time = String::Atoi(token);

			DrawAltString (x+32, y, ci->name);
			DrawString (x+32, y+8,  "Score: ");
			DrawAltString (x+32+7*8, y+8,  va("%i", score));
			DrawString (x+32, y+16, va("Ping:  %i", ping));
			DrawString (x+32, y+24, va("Time:  %i", time));

			if (!ci->icon)
				ci = &cl.baseclientinfo;
			UI_DrawNamedPic (x, y, ci->iconname);
			continue;
		}

		if (!String::Cmp(token, "ctf"))
		{	// draw a ctf client block
			int		score, ping;
			char	block[80];

			token = String::Parse2 (&s);
			x = viddef.width/2 - 160 + String::Atoi(token);
			token = String::Parse2 (&s);
			y = viddef.height/2 - 120 + String::Atoi(token);

			token = String::Parse2 (&s);
			value = String::Atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = String::Parse2 (&s);
			score = String::Atoi(token);

			token = String::Parse2 (&s);
			ping = String::Atoi(token);
			if (ping > 999)
				ping = 999;

			sprintf(block, "%3d %3d %-12.12s", score, ping, ci->name);

			if (value == cl.playernum)
				DrawAltString (x, y, block);
			else
				DrawString (x, y, block);
			continue;
		}

		if (!String::Cmp(token, "picn"))
		{	// draw a pic from a name
			token = String::Parse2 (&s);
			UI_DrawNamedPic (x, y, token);
			continue;
		}

		if (!String::Cmp(token, "num"))
		{	// draw a number
			token = String::Parse2 (&s);
			width = String::Atoi(token);
			token = String::Parse2 (&s);
			value = cl.q2_frame.playerstate.stats[String::Atoi(token)];
			SCR_DrawField (x, y, 0, width, value);
			continue;
		}

		if (!String::Cmp(token, "hnum"))
		{	// health number
			int		color;

			width = 3;
			value = cl.q2_frame.playerstate.stats[Q2STAT_HEALTH];
			if (value > 25)
				color = 0;	// green
			else if (value > 0)
				color = (cl.q2_frame.serverframe>>2) & 1;		// flash
			else
				color = 1;

			if (cl.q2_frame.playerstate.stats[Q2STAT_FLASHES] & 1)
				UI_DrawNamedPic (x, y, "field_3");

			SCR_DrawField (x, y, color, width, value);
			continue;
		}

		if (!String::Cmp(token, "anum"))
		{	// ammo number
			int		color;

			width = 3;
			value = cl.q2_frame.playerstate.stats[Q2STAT_AMMO];
			if (value > 5)
				color = 0;	// green
			else if (value >= 0)
				color = (cl.q2_frame.serverframe>>2) & 1;		// flash
			else
				continue;	// negative number = don't show

			if (cl.q2_frame.playerstate.stats[Q2STAT_FLASHES] & 4)
				UI_DrawNamedPic (x, y, "field_3");

			SCR_DrawField (x, y, color, width, value);
			continue;
		}

		if (!String::Cmp(token, "rnum"))
		{	// armor number
			int		color;

			width = 3;
			value = cl.q2_frame.playerstate.stats[Q2STAT_ARMOR];
			if (value < 1)
				continue;

			color = 0;	// green

			if (cl.q2_frame.playerstate.stats[Q2STAT_FLASHES] & 2)
				UI_DrawNamedPic (x, y, "field_3");

			SCR_DrawField (x, y, color, width, value);
			continue;
		}


		if (!String::Cmp(token, "stat_string"))
		{
			token = String::Parse2 (&s);
			index = String::Atoi(token);
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error (ERR_DROP, "Bad stat_string index");
			index = cl.q2_frame.playerstate.stats[index];
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error (ERR_DROP, "Bad stat_string index");
			DrawString (x, y, cl.configstrings[index]);
			continue;
		}

		if (!String::Cmp(token, "cstring"))
		{
			token = String::Parse2 (&s);
			DrawHUDString (token, x, y, 320, 0);
			continue;
		}

		if (!String::Cmp(token, "string"))
		{
			token = String::Parse2 (&s);
			DrawString (x, y, token);
			continue;
		}

		if (!String::Cmp(token, "cstring2"))
		{
			token = String::Parse2 (&s);
			DrawHUDString (token, x, y, 320,0x80);
			continue;
		}

		if (!String::Cmp(token, "string2"))
		{
			token = String::Parse2 (&s);
			DrawAltString (x, y, token);
			continue;
		}

		if (!String::Cmp(token, "if"))
		{	// draw a number
			token = String::Parse2 (&s);
			value = cl.q2_frame.playerstate.stats[String::Atoi(token)];
			if (!value)
			{	// skip to endif
				while (s && String::Cmp(token, "endif") )
				{
					token = String::Parse2 (&s);
				}
			}

			continue;
		}


	}
}


/*
================
SCR_DrawStats

The status bar is a small layout program that
is based on the stats array
================
*/
void SCR_DrawStats (void)
{
	SCR_ExecuteLayoutString (cl.configstrings[CS_STATUSBAR]);
}


/*
================
SCR_DrawLayout

================
*/
#define	Q2STAT_LAYOUTS		13

void SCR_DrawLayout (void)
{
	if (!cl.q2_frame.playerstate.stats[Q2STAT_LAYOUTS])
		return;
	SCR_ExecuteLayoutString (cl.layout);
}

//=======================================================

static void SCR_DrawScreen(stereoFrame_t stereoFrame, float separation)
{
	R_BeginFrame(stereoFrame);

	if (scr_draw_loading == 2)
	{	//  loading plaque over black screen
		int		w, h;

		UI_Fill(0, 0, viddef.width, viddef.height, 0, 0, 0, 1);
		scr_draw_loading = false;
		R_GetPicSize (&w, &h, "loading");
		UI_DrawNamedPic ((viddef.width-w)/2, (viddef.height-h)/2, "loading");
	} 
	// if a cinematic is supposed to be running, handle menus
	// and console specially
	else if (SCR_DrawCinematic())
	{
		if (in_keyCatchers & KEYCATCH_UI)
		{
			M_Draw ();
		}
		else if (in_keyCatchers & KEYCATCH_CONSOLE)
		{
			SCR_DrawConsole ();
		}
	}
	else 
	{
		// do 3D refresh drawing, and then update the screen
		SCR_CalcVrect ();

		// clear any dirty part of the background
		SCR_TileClear ();

		V_RenderView (separation);

		SCR_DrawStats ();
		if (cl.q2_frame.playerstate.stats[Q2STAT_LAYOUTS] & 1)
			SCR_DrawLayout ();
		if (cl.q2_frame.playerstate.stats[Q2STAT_LAYOUTS] & 2)
			CL_DrawInventory ();

		SCR_DrawNet ();
		SCR_CheckDrawCenterString ();

		if (scr_timegraph->value)
			SCR_DebugGraph (cls.frametimeFloat*300, 0);

		if (scr_debuggraph->value || scr_timegraph->value || scr_netgraph->value)
			SCR_DrawDebugGraph ();

		SCR_DrawPause ();

		SCR_DrawConsole ();

		M_Draw ();

		SCR_DrawLoading ();
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen (void)
{
	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if (cls.disable_screen)
	{
		if (Sys_Milliseconds_() - cls.disable_screen > 120000)
		{
			cls.disable_screen = 0;
			Com_Printf ("Loading plaque timed out.\n");
		}
		return;
	}

	if (!scr_initialized || !con.initialized)
		return;				// not initialized yet

	/*
	** range check cl_camera_separation so we don't inadvertently fry someone's
	** brain
	*/
	if ( cl_stereo_separation->value > 1.0 )
		Cvar_SetValueLatched( "cl_stereo_separation", 1.0 );
	else if ( cl_stereo_separation->value < 0 )
		Cvar_SetValueLatched( "cl_stereo_separation", 0.0 );

	if (cls.glconfig.stereoEnabled)
	{
		SCR_DrawScreen(STEREO_LEFT, -cl_stereo_separation->value / 2);
		SCR_DrawScreen(STEREO_RIGHT, cl_stereo_separation->value / 2);
	}		
	else
	{
		SCR_DrawScreen(STEREO_CENTER, 0);
	}

	R_EndFrame(NULL, NULL);

	if (cls.state == ca_active && cl.refresh_prepped && cl.q2_frame.valid)
	{
		CL_UpdateParticles(800);
	}
}

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
	draw_chars = R_LoadQuake2FontImage("pics/conchars.pcx");
}

/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char (int x, int y, int num)
{
	num &= 255;
	
	if ( (num&127) == 32 )
		return;		// space

	UI_DrawChar(x, y, num, 8, 8, draw_chars, 16, 16);
}
