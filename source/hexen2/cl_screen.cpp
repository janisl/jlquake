// screen.c -- master for refresh, status bar, console, chat, notify, etc

/*
 * $Header: /H2 Mission Pack/gl_screen.c 20    3/18/98 11:34p Jmonroe $
 */

#include "quakedef.h"

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
Con_Printf ();

net 
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
	notify lines
	half
	full
	

*/


float		scr_con_current;
float		scr_conlines;		// lines of console to display

Cvar*		scr_viewsize;
Cvar*		scr_fov;
Cvar*		scr_conspeed;
Cvar*		scr_centertime;
Cvar*		scr_showturtle;
Cvar*		scr_showpause;
Cvar*		scr_printspeed;
Cvar*		show_fps;
extern	Cvar*	crosshair;

qboolean	scr_initialized;		// ready to draw

image_t		*scr_net;
image_t		*scr_turtle;

float		introTime = 0.0;

vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;
qboolean	scr_drawloading;
float		scr_disabled_time;

static qboolean scr_needfull = false;

int			total_loading_size, current_loading_size, loading_stage;

void Plaque_Draw (const char *message, qboolean AlwaysDraw);
void Info_Plaque_Draw (const char *message);
void Bottom_Plaque_Draw (const char *message);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;

static int lines;
#define MAXLINES 27
static int StartC[MAXLINES],EndC[MAXLINES];

#ifdef MISSIONPACK
#define MAX_INFO 1024
char infomessage[MAX_INFO];
qboolean info_up = false;

extern int	*pr_info_string_index;
extern char	*pr_global_info_strings;
#endif

#ifdef MISSIONPACK
void UpdateInfoMessage(void)
{
	unsigned int i;
	unsigned int check;
	char *newmessage;

	String::Cpy(infomessage, "Objectives:");

	if (!pr_global_info_strings)
		return;

	for (i = 0; i < 32; i++)
	{
		check = (1 << i);
		
		if (cl.info_mask & check)
		{
			newmessage = &pr_global_info_strings[pr_info_string_index[i]];
			String::Cat(infomessage, sizeof(infomessage), "@@");
			String::Cat(infomessage, sizeof(infomessage), newmessage);
		}
	}

	for (i = 0; i < 32; i++)
	{
		check = (1 << i);
		
		if (cl.info_mask2 & check)
		{
			newmessage = &pr_global_info_strings[pr_info_string_index[i+32]];
			String::Cat(infomessage, sizeof(infomessage), "@@");
			String::Cat(infomessage, sizeof(infomessage), newmessage);
		}
	}
}
#endif

void FindTextBreaks(const char *message, int Width)
{
	int length,pos,start,lastspace,oldlast;

	length = String::Length(message);
	lines = pos = start = 0;
	lastspace = -1;

	while(1)
	{
		if (pos-start >= Width || message[pos] == '@' ||
			message[pos] == 0)
		{
			oldlast = lastspace;
			if (message[pos] == '@' || lastspace == -1 || message[pos] == 0)
				lastspace = pos;

			StartC[lines] = start;
			EndC[lines] = lastspace;
			lines++;
			if (lines == MAXLINES)
				return;
			if (message[pos] == '@')
				start = pos + 1;
			else if (oldlast == -1)
				start = lastspace;
			else
				start = lastspace + 1;

			lastspace = -1;
		}

		if (message[pos] == 32) lastspace = pos;
		else if (message[pos] == 0)
		{
			break;
		}

		pos++;
	}
}

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	String::NCpy(scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = cl.serverTimeFloat;

	FindTextBreaks(scr_centerstring, 38);
	scr_center_lines = lines;
}


void SCR_DrawCenterString (void)
{
	int		i;
	int		bx, by;
	int		remaining;
	char	temp[80];

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed->value * (cl.serverTimeFloat - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;

	FindTextBreaks(scr_centerstring, 38);

	by = ((25-lines) * 8) / 2;
	for(i=0;i<lines;i++,by+=8)
	{
		String::NCpy(temp,&scr_centerstring[StartC[i]],EndC[i]-StartC[i]);
		temp[EndC[i]-StartC[i]] = 0;
		bx = ((40-String::Length(temp)) * 8) / 2;
	  	M_Print2 (bx, by, temp);
	}
}

void SCR_CheckDrawCenterString (void)
{
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= host_frametime;
	
	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (in_keyCatchers != 0)
		return;

	if(intro_playing)
		Bottom_Plaque_Draw(scr_centerstring);
	else
		SCR_DrawCenterString ();
}

//=============================================================================

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef (void)
{
	float		size;
	int		h;


// bound viewsize
	if (scr_viewsize->value < 30)
		Cvar_Set ("viewsize","30");
	if (scr_viewsize->value > 120)
		Cvar_Set ("viewsize","120");

// bound field of view
	if (scr_fov->value < 10)
		Cvar_Set ("fov","10");
	if (scr_fov->value > 170)
		Cvar_Set ("fov","170");

// intermission is always full screen	
	if (cl.intermission)
		size = 110;
	else
		size = scr_viewsize->value;

/*	if (size >= 120)
		sb_lines = 0;		// no status bar at all
	else if (size >= 110)
		sb_lines = 24;		// no inventory
	else
		sb_lines = 24+16+8;*/

	if(size >= 110)
	{ // No status bar
		sb_lines = 0;
	}
	else
	{
		sb_lines = 36;
	}

	size = scr_viewsize->value > 100 ? 100 : scr_viewsize->value;
	if (cl.intermission)
	{
		size = 100;
		sb_lines = 0;
	}
	size /= 100;

	h = viddef.height - sb_lines;
	scr_vrect.width = viddef.width * size;
	if (scr_vrect.width < 96)
	{
		size = 96.0 / viddef.width;
		scr_vrect.width = 96;	// min for icons
	}

	scr_vrect.height = viddef.height * size;
	if (scr_vrect.height > (int)viddef.height - sb_lines)
		scr_vrect.height = viddef.height - sb_lines;

	scr_vrect.x = (viddef.width - scr_vrect.width)/2;
	scr_vrect.y = (h - scr_vrect.height)/2;

	cl.refdef.x = scr_vrect.x * cls.glconfig.vidWidth / viddef.width;
	cl.refdef.y = scr_vrect.y * cls.glconfig.vidHeight / viddef.height;
	cl.refdef.width = scr_vrect.width * cls.glconfig.vidWidth / viddef.width;
	cl.refdef.height = scr_vrect.height * cls.glconfig.vidHeight / viddef.height;
	cl.refdef.fov_x = 90;
	cl.refdef.fov_y = 2 * atan((float)cl.refdef.height / cl.refdef.width) * 180 / M_PI;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValue ("viewsize",scr_viewsize->value+10);
	SB_ViewSizeChanged();
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValue ("viewsize",scr_viewsize->value-10);
	SB_ViewSizeChanged();
}

/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
static void R_TimeRefresh_f (void)
{
	int			i;
	float		start, stop, time;

	start = Sys_DoubleTime();
	vec3_t viewangles;
	viewangles[0] = 0;
	viewangles[1] = 0;
	viewangles[2] = 0;
	for (i=0 ; i<128 ; i++)
	{
		viewangles[1] = i/128.0*360.0;
		AnglesToAxis(viewangles, cl.refdef.viewaxis);
		R_BeginFrame(STEREO_CENTER);
		V_RenderScene ();
		R_EndFrame(NULL, NULL);
	}

	stop = Sys_DoubleTime ();
	time = stop-start;
	Con_Printf ("%f seconds (%f fps)\n", time, 128/time);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	scr_viewsize = Cvar_Get("viewsize", "100", CVAR_ARCHIVE);
	scr_fov = Cvar_Get("fov", "90", 0);	// 10 - 170
	scr_conspeed = Cvar_Get("scr_conspeed", "300", 0);
	scr_centertime = Cvar_Get("scr_centertime", "4", 0);
	scr_showturtle = Cvar_Get("showturtle", "0", 0);
	scr_showpause = Cvar_Get("showpause", "1", 0);
	scr_printspeed = Cvar_Get("scr_printspeed", "8", 0);
	show_fps = Cvar_Get("show_fps", "0", CVAR_ARCHIVE);			// set for running times

//
// register our commands
//
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);
	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	

	scr_net = R_PicFromWad ("net");
	scr_turtle = R_PicFromWad ("turtle");

	scr_initialized = true;
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static int	count;
	
	if (!scr_showturtle->value)
		return;

	if (host_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	UI_DrawPic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (realtime - cl.last_received_message < 0.3)
		return;
	if (cls.demoplayback)
		return;

	UI_DrawPic (scr_vrect.x+64, scr_vrect.y, scr_net);
}

void SCR_DrawFPS (void)
{
	static double lastframetime;
	double t;
	extern int fps_count;
	static int lastfps;
	int x, y;
	char st[80];

	if (!show_fps->value)
		return;

	t = Sys_DoubleTime();
	if ((t - lastframetime) >= 1.0) {
		lastfps = fps_count;
		fps_count = 0;
		lastframetime = t;
	}

	sprintf(st, "%3d FPS", lastfps);
	x = viddef.width - String::Length(st) * 8 - 8;
	y = viddef.height - sb_lines - 8;
	Draw_String(x, y, st);
}

/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	image_t	*pic;
	float delta;
	static qboolean newdraw = false;
	int finaly;
	static float LogoPercent,LogoTargetPercent;

	if (!scr_showpause->value)		// turn off for screenshots
		return;

//	if (!cl.paused)
//		return;

//	pic = R_CachePic ("gfx/pause.lmp");
//	UI_DrawPic ( (viddef.width - pic->width)/2, 
//		(viddef.height - 48 - pic->height)/2, pic);


	if (!cl.paused)
	{
		newdraw = false;
		return;
	}

	if (!newdraw)
	{
		newdraw = true;
		LogoTargetPercent= 1;
		LogoPercent = 0;
	}

	pic = R_CachePic ("gfx/menu/paused.lmp");
//	UI_DrawPic ( (viddef.width - pic->width)/2, 
//		(viddef.height - 48 - pic->height)/2, pic);

	if (LogoPercent < LogoTargetPercent)
	{
		delta = ((LogoTargetPercent-LogoPercent)/.5)*host_frametime;
		if (delta < 0.004)
		{
			delta = 0.004;
		}
		LogoPercent += delta;
		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	finaly = ((float)R_GetImageHeight(pic) * LogoPercent) - R_GetImageHeight(pic);
	UI_DrawPic( (viddef.width - R_GetImageWidth(pic)) / 2, finaly, pic);
}



/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	int		size, count, offset;
	image_t	*pic;

	if (!scr_drawloading && loading_stage == 0)
		return;
		
	pic = R_CachePic ("gfx/menu/loading.lmp");
	offset = (viddef.width - R_GetImageWidth(pic)) / 2;
	UI_DrawPic (offset , 0, pic);

	if (loading_stage == 0)
		return;

	if (total_loading_size)
		size = current_loading_size * 106 / total_loading_size;
	else
		size = 0;

	if (loading_stage == 1)
		count = size;
	else
		count = 106;

	UI_FillPal (offset+42, 87, count, 1, 136);
	UI_FillPal (offset+42, 87+1, count, 4, 138);
	UI_FillPal (offset+42, 87+5, count, 1, 136);

	if (loading_stage == 2)
		count = size;
	else
		count = 0;

	UI_FillPal (offset+42, 97, count, 1, 168);
	UI_FillPal (offset+42, 97+1, count, 4, 170);
	UI_FillPal (offset+42, 97+5, count, 1, 168);
}



//=============================================================================


/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	Con_CheckResize ();
	
	if (scr_drawloading)
		return;		// never a console with loading plaque
		
// decide on the height of the console
	con_forcedup = cls.state != ca_connected || clc.qh_signon != SIGNONS;

	if (con_forcedup)
	{
		scr_conlines = viddef.height;		// full screen
		scr_con_current = scr_conlines;
	}
	else if (in_keyCatchers & KEYCATCH_CONSOLE)
		scr_conlines = viddef.height/2;	// half screen
	else
		scr_conlines = 0;				// none visible
	
	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed->value*host_frametime;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed->value*host_frametime;
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
	if (scr_con_current)
	{
		Con_DrawConsole (scr_con_current, true);
	}
	else
	{
		if (in_keyCatchers == 0 || in_keyCatchers == KEYCATCH_MESSAGE)
			Con_DrawNotify ();	// only draw notify in game
	}
}

/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds();

	if (cls.state != ca_connected)
		return;
	if (clc.qh_signon != SIGNONS)
		return;
	
// redraw with no console and the loading plaque
	Con_ClearNotify ();
	scr_centertime_off = 0;
	scr_con_current = 0;

	scr_drawloading = true;
	SCR_UpdateScreen ();
	scr_drawloading = false;

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
	Con_ClearNotify ();
}

//=============================================================================

const char	*scr_notifystring;
qboolean	scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	Plaque_Draw(scr_notifystring,1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.  
==================
*/
int SCR_ModalMessage (const char *text)
{
	if (cls.state == ca_dedicated)
		return true;

	scr_notifystring = text;
 
	scr_drawdialog = true;
	SCR_UpdateScreen ();
	scr_drawdialog = false;
	
	S_ClearSoundBuffer ();		// so dma doesn't loop current sound

	do
	{
		key_count = -1;		// wait for a key down and up
		Sys_SendKeyEvents ();
		IN_ProcessEvents();
	} while (key_lastpress != 'y' && key_lastpress != 'n' && key_lastpress != K_ESCAPE);

	SCR_UpdateScreen ();

	return key_lastpress == 'y';
}


//=============================================================================

/*
===============
SCR_BringDownConsole

Brings the console down and fades the palettes back to normal
================
*/
void SCR_BringDownConsole (void)
{
	int		i;
	
	scr_centertime_off = 0;
	
	for (i=0 ; i<20 && scr_conlines != scr_con_current ; i++)
		SCR_UpdateScreen ();

	cl.qh_cshifts[0].percent = 0;		// no area contents palette on next frame
}

void SCR_TileClear (void)
{
	if (scr_vrect.x > 0)
	{
		UI_TileClear(0,0,scr_vrect.x,viddef.height, draw_backtile);
		UI_TileClear(scr_vrect.x + scr_vrect.width, 0
			, viddef.width - scr_vrect.x + scr_vrect.width,viddef.height, draw_backtile);
	}
//	if (scr_vrect.height < viddef.height-44)
	{
		UI_TileClear(scr_vrect.x, 0, scr_vrect.width, scr_vrect.y, draw_backtile);
		UI_TileClear(scr_vrect.x, scr_vrect.y + scr_vrect.height,
			scr_vrect.width,
			viddef.height - (scr_vrect.y + scr_vrect.height), draw_backtile);
	}
}

// This is also located in screen.c
#define PLAQUE_WIDTH 26

void Plaque_Draw (const char *message, qboolean AlwaysDraw)
{
	int i;
	char temp[80];
	int bx,by;

	if (scr_con_current == viddef.height && !AlwaysDraw)
		return;		// console is full screen

	if (!*message) 
		return;

	FindTextBreaks(message, PLAQUE_WIDTH);

	by = ((25-lines) * 8) / 2;
	M_DrawTextBox2 (32, by-16, 30, lines+2,false);

	for(i=0;i<lines;i++,by+=8)
	{
		String::NCpy(temp,&message[StartC[i]],EndC[i]-StartC[i]);
		temp[EndC[i]-StartC[i]] = 0;
		bx = ((40-String::Length(temp)) * 8) / 2;
	  	M_Print2 (bx, by, temp);
	}
}

void Info_Plaque_Draw (const char *message)
{
	int i;
	char temp[80];
	int bx,by;

	if (scr_con_current == viddef.height)
		return;		// console is full screen

	if (!*message) 
		return;

	scr_needfull = true;

	FindTextBreaks(message, PLAQUE_WIDTH+4);

	if (lines == MAXLINES) 
	{
		Con_DPrintf("Info_Plaque_Draw: line overflow error\n");
		lines = MAXLINES-1;
	}

	by = ((25-lines) * 8) / 2;
	M_DrawTextBox2 (15, by-16, PLAQUE_WIDTH+4+4, lines+2,false);

	for(i=0;i<lines;i++,by+=8)
	{
		String::NCpy(temp,&message[StartC[i]],EndC[i]-StartC[i]);
		temp[EndC[i]-StartC[i]] = 0;
		bx = ((40-String::Length(temp)) * 8) / 2;
	  	M_Print2 (bx, by, temp);
	}
}
void I_DrawCharacter (int cx, int line, int num)
{
	Draw_Character ( cx + ((viddef.width - 320)>>1), line + ((viddef.height - 200)>>1), num);
}

void I_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		I_DrawCharacter (cx, cy, ((unsigned char)(*str))+256);
		str++;
		cx += 8;
	}
}

void Bottom_Plaque_Draw (const char *message)
{
	int i;
	char temp[80];
	int bx,by;

	if (!*message) 
		return;

	scr_needfull = true;

	FindTextBreaks(message, PLAQUE_WIDTH);

	by = (((viddef.height)/8)-lines-2) * 8;

	M_DrawTextBox2 (32, by-16, 30, lines+2,true);

	for(i=0;i<lines;i++,by+=8)
	{
		String::NCpy(temp,&message[StartC[i]],EndC[i]-StartC[i]);
		temp[EndC[i]-StartC[i]] = 0;
		bx = ((40-String::Length(temp)) * 8) / 2;
	  	M_Print(bx, by, temp);
	}
}
//==========================================================================
//
// SB_IntermissionOverlay
//
//==========================================================================

void SB_IntermissionOverlay(void)
{
	image_t	*pic;
	int		elapsed, size, bx, by, i;
	const char	*message;
	char		temp[80];

	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}
	
	switch(cl.intermission)
	{
	case 1:
			pic = R_CachePic ("gfx/meso.lmp");
			break;
		case 2:
			pic = R_CachePic ("gfx/egypt.lmp");
			break;
		case 3:
			pic = R_CachePic ("gfx/roman.lmp");
			break;
		case 4:
			pic = R_CachePic ("gfx/castle.lmp");
			break;
		case 5:
			pic = R_CachePic ("gfx/castle.lmp");
			break;
		case 6:
			pic = R_CachePic ("gfx/end-1.lmp");
			break;
		case 7:
			pic = R_CachePic ("gfx/end-2.lmp");
			break;
		case 8:
			pic = R_CachePic ("gfx/end-3.lmp");
			break;
		case 9:
			pic = R_CachePic ("gfx/castle.lmp");
			break;
		case 10:
			pic = R_CachePic ("gfx/mpend.lmp");
			break;
		case 11:
			pic = R_CachePic ("gfx/mpmid.lmp");
			break;
		case 12:
			pic = R_CachePic ("gfx/end-3.lmp");
			break;

		default:
			Sys_Error ("SB_IntermissionOverlay: Bad episode");
			break;
	}

	UI_DrawPic (((viddef.width - 320)>>1),((viddef.height - 200)>>1), pic);

	if (cl.intermission >= 6 && cl.intermission <= 8)
	{
		elapsed = (cl.serverTimeFloat - cl.completed_time) * 20;
		elapsed -= 50;
		if (elapsed < 0) elapsed = 0;
	}
	else if (cl.intermission == 12)
	{
		elapsed = (introTime);
		if (introTime < 500)
			introTime+=0.25;
	}
	else
	{
		elapsed = (cl.serverTimeFloat - cl.completed_time) * 20;
	}

	if (cl.intermission <= 4 && cl.intermission + 394 <= pr_string_count)
		message = &pr_global_strings[pr_string_index[cl.intermission + 394]];
	else if (cl.intermission == 5)
		message = &pr_global_strings[pr_string_index[ABILITIES_STR_INDEX+NUM_CLASSES*2+1]];
	else if (cl.intermission >= 6 && cl.intermission <= 8 && cl.intermission + 386 <= pr_string_count)
		message = &pr_global_strings[pr_string_index[cl.intermission + 386]];
	else if (cl.intermission == 9)
		message = &pr_global_strings[pr_string_index[391]];
	else
		message = "";
	
	if (cl.intermission == 10)
		message = &pr_global_strings[pr_string_index[538]];
	else if (cl.intermission == 11)
		message = &pr_global_strings[pr_string_index[545]];
	else if (cl.intermission == 12)
		message = &pr_global_strings[pr_string_index[561]];

	FindTextBreaks(message, 38);

	if (cl.intermission == 8)
		by = 16;
	else
		by = ((25-lines) * 8) / 2;

	for(i=0;i<lines;i++,by+=8)
	{
		size = EndC[i]-StartC[i];
		String::NCpy(temp,&message[StartC[i]],size);

		if (size > elapsed) size = elapsed;
		temp[size] = 0;

		bx = ((40-String::Length(temp)) * 8) / 2;
	  	if (cl.intermission < 6 || cl.intermission > 9)
			I_Print (bx, by, temp);
		else
			M_PrintWhite (bx, by, temp);

		elapsed -= size;
		if (elapsed <= 0) break;
	}

	if (i == lines && elapsed >= 300 && cl.intermission >= 6 && cl.intermission <= 7)
	{
		cl.intermission++;
		cl.completed_time = cl.serverTimeFloat;
	}
//	Con_Printf("Time is %10.2f\n",elapsed);
}

//==========================================================================
//
// SB_FinaleOverlay
//
//==========================================================================

void SB_FinaleOverlay(void)
{
	image_t	*pic;

	pic = R_CachePic("gfx/finale.lmp");
	UI_DrawPic((viddef.width - R_GetImageWidth(pic)) / 2, 16, pic);
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			total_loading_size = 0;
			loading_stage = 0;
			Con_Printf ("load failed.\n");
		}
		else
			return;
	}

	if (!scr_initialized || !con.initialized)
		return;				// not initialized yet

	R_BeginFrame(STEREO_CENTER);

	//
	// determine size of refresh window
	//
	SCR_CalcRefdef ();

//
// do 3D refresh drawing, and then update the screen
//
	SCR_SetUpToDrawConsole ();
	
	if (cl.intermission > 1 || cl.intermission <= 12)
	{
		V_RenderView ();
	}

	//
	// draw any areas not covered by the refresh
	//
	SCR_TileClear ();

	if (scr_drawdialog)
	{
		SB_Draw();
		Draw_FadeScreen ();
		SCR_DrawNotifyString ();
	}
	else if (scr_drawloading)
	{
		SB_Draw();
		Draw_FadeScreen ();
		SCR_DrawLoading ();
	}
	else if (cl.intermission >= 1 && cl.intermission <= 12)
	{
		SB_IntermissionOverlay();
		if (cl.intermission < 12)
		{
			SCR_DrawConsole();
			M_Draw();
		}
	}
/*	else if (cl.intermission == 2 && in_keyCatchers == 0)
	{
		SB_FinaleOverlay();
		SCR_CheckDrawCenterString();
	}*/
	else
	{
		if (crosshair->value)
			Draw_Character (scr_vrect.x + scr_vrect.width/2, scr_vrect.y + scr_vrect.height/2, '+');
		
		SCR_DrawNet();
		SCR_DrawFPS ();
		SCR_DrawTurtle();
		SCR_DrawPause();
		SCR_CheckDrawCenterString();
		SB_Draw();
		Plaque_Draw(plaquemessage,0);
		SCR_DrawConsole();
		M_Draw();
		if (errormessage)
			Plaque_Draw(errormessage,1);

#ifdef MISSIONPACK
		if (info_up)
		{
			UpdateInfoMessage();
			Info_Plaque_Draw(infomessage);
		}
#endif
	}

	if (loading_stage)
		SCR_DrawLoading();

	V_UpdatePalette ();

	R_EndFrame(NULL, NULL);
}

void VID_Init()
{
	R_BeginRegistration(&cls.glconfig);

	CLH2_InitColourShadeTables();

	Sys_ShowConsole(0, false);

	if (COM_CheckParm("-scale2d"))
	{
		viddef.height = 200;
		viddef.width = 320;
	}
	else
	{
		viddef.height = cls.glconfig.vidHeight;
		viddef.width = cls.glconfig.vidWidth;
	}
}
