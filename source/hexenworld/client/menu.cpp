/*
 * $Header: /HexenWorld/Client/menu.c 28    5/11/98 2:35p Mgummelt $
 */

#include "quakedef.h"

extern Cvar*		r_gamma;
extern	Cvar*	crosshair;

enum menu_state_t {m_none, m_main, m_singleplayer, m_load, m_save, m_multiplayer, m_setup, m_net, m_options, m_video, 
		m_keys, m_help, m_quit, m_lanconfig, m_gameoptions, m_search, m_slist, 
		m_class, m_mconnect};
menu_state_t m_state;

void M_Menu_Main_f (void);
	void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
	void M_Menu_MultiPlayer_f (void);
		void M_Menu_Setup_f (void);
		void M_Menu_Net_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
		void M_Menu_Video_f (void);
	void M_Menu_Help_f (void);
	void M_Menu_Quit_f (void);
void M_Menu_LanConfig_f (void);
void M_Menu_GameOptions_f (void);
void M_Menu_Search_f (void);
void M_Menu_ServerList_f (void);

void M_Main_Draw (void);
	void M_SinglePlayer_Draw (void);
		void M_Load_Draw (void);
		void M_Save_Draw (void);
	void M_MultiPlayer_Draw (void);
		void M_Setup_Draw (void);
		void M_Net_Draw (void);
	void M_Options_Draw (void);
		void M_Keys_Draw (void);
		void M_Video_Draw (void);
	void M_Help_Draw (void);
	void M_Quit_Draw (void);
void M_SerialConfig_Draw (void);
	void M_ModemConfig_Draw (void);
void M_LanConfig_Draw (void);
void M_GameOptions_Draw (void);
void M_Search_Draw (void);
void M_ServerList_Draw (void);

void M_Main_Key (int key);
	void M_SinglePlayer_Key (int key);
		void M_Load_Key (int key);
		void M_Save_Key (int key);
	void M_MultiPlayer_Key (int key);
		void M_Menu_Connect_f (void);
		void M_Setup_Key (int key);
		void M_Net_Key (int key);
	void M_Options_Key (int key);
		void M_Keys_Key (int key);
		void M_Video_Key (int key);
	void M_Help_Key (int key);
	void M_Quit_Key (int key);
void M_SerialConfig_Key (int key);
	void M_ModemConfig_Key (int key);
void M_LanConfig_Key (int key);
void M_GameOptions_Key (int key);
void M_Search_Key (int key);
void M_ServerList_Key (int key);

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean	m_recursiveDraw;

int			m_return_state;
qboolean	m_return_onerror;
char		m_return_reason [32];

static float TitlePercent = 0;
static float TitleTargetPercent = 1;
static float LogoPercent = 0;
static float LogoTargetPercent = 1;

int		setup_class, which_class;

static char *message,*message2;
static double message_time;

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)
#define SerialConfig	(m_net_cursor == 0)
#define DirectConfig	(m_net_cursor == 1)
#define	IPXConfig		(m_net_cursor == 2)
#define	TCPIPConfig		(m_net_cursor == 3)

void M_ConfigureNetSubsystem(void);
void M_Menu_Class_f (void);

const char *ClassNames[MAX_PLAYER_CLASS] = 
{
	"Paladin",
	"Crusader",
	"Necromancer",
	"Assassin",
	"Succubus",
	"Dwarf"
};

const char *ClassNamesU[MAX_PLAYER_CLASS] = 
{
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN",
	"SUCCUBUS",
	"DWARF"
};

const char *DiffNames[MAX_PLAYER_CLASS][4] =
{
	{	// Paladin
		"APPRENTICE",
		"SQUIRE",
		"ADEPT",
		"LORD"
	},
	{	// Crusader
		"GALLANT",
		"HOLY AVENGER",
		"DIVINE HERO",
		"LEGEND"
	},
	{	// Necromancer
		"SORCERER",
		"DARK SERVANT",
		"WARLOCK",
		"LICH KING"
	},
	{	// Assassin
		"ROGUE",
		"CUTTHROAT",
		"EXECUTIONER",
		"WIDOW MAKER"
	},
	{	// Demoness
		"LARVA",
		"SPAWN",
		"FIEND",
		"SHE BITCH"
	},
	{	// Dwarf
		"NATE",
		"MUNCHKIN",
		"BERZERKER",
		"HANK"
	}
};

//=============================================================================
/* Support Routines */

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character ( cx + ((viddef.width - 320)>>1), line, num);
}

void M_Print (int cx, int cy, const char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, ((unsigned char)(*str))+256);
		str++;
		cx += 8;
	}
}

void M_DrawCharacter2 (int cx, int line, int num)
{
	Draw_Character ( cx + ((viddef.width - 320)>>1), line + ((viddef.height - 200)>>1), num);
}

void M_Print2 (int cx, int cy, const char *str)
{
	while (*str)
	{
		M_DrawCharacter2 (cx, cy, ((unsigned char)(*str))+256);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, const char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (unsigned char)*str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, image_t *pic)
{
	UI_DrawPic (x + ((viddef.width - 320)>>1), y, pic);
}

void M_DrawTransPic2 (int x, int y, image_t *pic)
{
	UI_DrawPic (x + ((viddef.width - 320)>>1), y + ((viddef.height - 200)>>1), pic);
}

void M_DrawPic (int x, int y, image_t *pic)
{
	UI_DrawPic (x + ((viddef.width - 320)>>1), y, pic);
}

void M_DrawTransPicCropped (int x, int y, image_t *pic)
{
	UI_DrawPic (x + ((viddef.width - 320)>>1), y, pic);
}

#define PLAYER_PIC_WIDTH 68
#define PLAYER_PIC_HEIGHT 114

byte translationTable[256];
static byte		menuplyr_pixels[MAX_PLAYER_CLASS][PLAYER_PIC_WIDTH*PLAYER_PIC_HEIGHT];
image_t*	translate_texture[MAX_PLAYER_CLASS];

void M_DrawTextBox (int x, int y, int width, int lines)
{
	image_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = R_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = R_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = R_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = R_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic (cx, cy, p);
		p = R_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = R_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		p = R_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = R_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = R_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = R_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}

void M_DrawTextBox2 (int x, int y, int width, int lines)
{
	image_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = R_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic2 (cx, cy, p);
	p = R_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic2 (cx, cy, p);
	}
	p = R_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic2 (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = R_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic2 (cx, cy, p);
		p = R_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = R_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic2 (cx, cy, p);
		}
		p = R_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic2 (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = R_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic2 (cx, cy, p);
	p = R_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic2 (cx, cy, p);
	}
	p = R_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic2 (cx, cy+8, p);
}

//=============================================================================

int m_save_demonum;
		
/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (void)
{
	m_entersound = true;

	if (in_keyCatchers & KEYCATCH_UI)
	{
		if (m_state != m_main)
		{
			LogoTargetPercent = TitleTargetPercent = 1;
			LogoPercent = TitlePercent = 0;
			M_Menu_Main_f ();
			return;
		}
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		return;
	}
	if ((in_keyCatchers & KEYCATCH_CONSOLE) && cls.state == CA_ACTIVE)
	{
		Con_ToggleConsole_f ();
	}
	else
	{
		LogoTargetPercent = TitleTargetPercent = 1;
		LogoPercent = TitlePercent = 0;
		M_Menu_Main_f ();
	}
}

char BigCharWidth[27][27];
static char unused_filler;  // cuz the COM_LoadStackFile puts a 0 at the end of the data

//#define BUILD_BIG_CHAR 1

void M_BuildBigCharWidth (void)
{
#ifdef BUILD_BIG_CHAR
	image_t	*p;
	int ypos,xpos;
	byte			*source;
	int biggestX,adjustment;
	char After[20], Before[20];
	int numA,numB;
	FILE *FH;
	char temp[MAX_OSPATH];

	p = R_CachePic ("gfx/menu/bigfont.lmp");

	for(numA = 0; numA < 27; numA++)
	{
		Com_Memset(After,20,sizeof(After));
		source = p->data + ((numA % 8) * 20) + (numA / 8 * p->width * 20);
		biggestX = 0;

		for(ypos=0;ypos < 19;ypos++)
		{
			for(xpos=0;xpos<19;xpos++,source++)
			{
				if (*source) 
				{
					After[ypos] = xpos;
					if (xpos > biggestX) biggestX = xpos;
				}
			}
			source += (p->width - 19);
		}
		biggestX++;

		for(numB = 0; numB < 27; numB++)
		{
			Com_Memset(Before,0,sizeof(Before));
			source = p->data + ((numB % 8) * 20) + (numB / 8 * p->width * 20);
			adjustment = 0;

			for(ypos=0;ypos < 19;ypos++)
			{
				for(xpos=0;xpos<19;xpos++,source++)
				{
					if (!(*source))
					{
						Before[ypos]++;
					}
					else break;
				}
				source += (p->width - xpos);
			}


			while(1)
			{
				for(ypos=0;ypos<19;ypos++)
				{
					if (After[ypos] - Before[ypos] >= 15) break;
					Before[ypos]--;
				}
				if (ypos < 19) break;
				adjustment--;
			}
			BigCharWidth[numA][numB] = adjustment+biggestX;
		}
	}

	sprintf(temp,"%s\\gfx\\menu\\fontsize.lmp",com_gamedir);
	FH = fopen(temp,"wb");
	fwrite(BigCharWidth,1,sizeof(BigCharWidth),FH);
	fclose(FH);
#else
	COM_LoadStackFile ("gfx/menu/fontsize.lmp",BigCharWidth,sizeof(BigCharWidth)+1);
#endif
}

/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/

void M_DrawBigString(int x, int y, const char *string)
{
	int c,length;

	x += ((viddef.width - 320)>>1);

	length = String::Length(string);
	for(c=0;c<length;c++)
	{
		x += M_DrawBigCharacter(x,y,string[c],string[c+1]);
	}
}






void ScrollTitle (const char *name)
{
	float delta;
	image_t	*p;
	static const char *LastName = "";
	int finaly;
	static qboolean CanSwitch = true;

	if (TitlePercent < TitleTargetPercent)
	{
		delta = ((TitleTargetPercent-TitlePercent)/0.5)*host_frametime;
		if (delta < 0.004)
		{
			delta = 0.004;
		}
		TitlePercent += delta;
		if (TitlePercent > TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
		}
	}
	else if (TitlePercent > TitleTargetPercent)
	{
		delta = ((TitlePercent-TitleTargetPercent)/0.15)*host_frametime;
		if (delta < 0.02)
		{
			delta = 0.02;
		}
		TitlePercent -= delta;
		if (TitlePercent <= TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
			CanSwitch = true;
		}
	}

	if (LogoPercent < LogoTargetPercent)
	{
/*		delta = ((LogoTargetPercent-LogoPercent)/1.1)*host_frametime;
		if (delta < 0.0015)
		{
			delta = 0.0015;
		}
*/
		delta = ((LogoTargetPercent-LogoPercent)/.15)*host_frametime;
		if (delta < 0.02)
		{
			delta = 0.02;
		}
		LogoPercent += delta;
		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	if (String::ICmp(LastName,name) != 0 && TitleTargetPercent != 0) 
		TitleTargetPercent = 0;

    if (CanSwitch) 
	{
		LastName = name;
		CanSwitch = false;
		TitleTargetPercent = 1;
	}

	p = R_CachePic(LastName);
	finaly = ((float)R_GetImageHeight(p) * TitlePercent) - R_GetImageHeight(p);
	M_DrawTransPicCropped( (320-R_GetImageWidth(p))/2, finaly , p);

	if (m_state != m_keys)
	{
		p = R_CachePic("gfx/menu/hplaque.lmp");
		finaly = ((float)R_GetImageHeight(p) * LogoPercent) - R_GetImageHeight(p);
		M_DrawTransPicCropped(10, finaly, p);
	}		
}

	
//=============================================================================
/* MAIN MENU */

int	m_main_cursor;
#define	MAIN_ITEMS	4


void M_Menu_Main_f (void)
{
	if (!(in_keyCatchers & KEYCATCH_UI))
	{
		m_save_demonum = cls.qh_demonum;
		cls.qh_demonum = -1;
	}
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_main;
	m_entersound = true;
}
				

void M_Main_Draw (void)
{
	int		f;

	ScrollTitle("gfx/menu/title0.lmp");
//	M_DrawTransPic (72, 32, R_CachePic ("gfx/mainmenu.lmp") );
//	M_DrawBigString (72,60+(0*20),"SINGLE PLAYER");
	M_DrawBigString (72,60+(0*20),"MULTIPLAYER");
	M_DrawBigString (72,60+(1*20),"OPTIONS");
	M_DrawBigString (72,60+(2*20),"HELP");
	M_DrawBigString (72,60+(3*20),"QUIT");

	f = (int)(realtime * 10)%8;
	M_DrawTransPic (43, 54 + m_main_cursor * 20,R_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}


void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		cls.qh_demonum = m_save_demonum;
		if (cls.qh_demonum != -1 && !clc.demoplaying && cls.state == CA_DISCONNECTED)
			CL_NextDemo ();
		break;
		
	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 4:
			M_Menu_SinglePlayer_f ();
			break;

		case 0:
			M_Menu_MultiPlayer_f ();
			break;

		case 1:
			M_Menu_Options_f ();
			break;

		case 2:
			M_Menu_Help_f ();
			break;

		case 3:
			M_Menu_Quit_f ();
			break;
		}
	}
}

const char	*plaquemessage = NULL;   // Pointer to current plaque message
char    *errormessage = NULL;


//=============================================================================
/* CLASS CHOICE MENU */
int class_flag;

void M_Menu_Class_f (void)
{
	class_flag=0;
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_class;
}

void M_Menu_Class2_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_class;
	class_flag=1;
}


int	m_class_cursor;
#define	CLASS_ITEMS	6

void M_Class_Draw (void)
{
	int		f;

	ScrollTitle("gfx/menu/title2.lmp");

	M_DrawBigString (72,60+(0*20),ClassNamesU[0]);
	M_DrawBigString (72,60+(1*20),ClassNamesU[1]);
	M_DrawBigString (72,60+(2*20),ClassNamesU[2]);
	M_DrawBigString (72,60+(3*20),ClassNamesU[3]);
	M_DrawBigString (72,60+(4*20),ClassNamesU[4]);
	M_DrawBigString (72,60+(5*20),ClassNamesU[5]);

	f = (int)(realtime * 10)%8;
	M_DrawTransPic (43, 54 + m_class_cursor * 20,R_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );

	M_DrawTransPic (251,54 + 21, R_CachePic (va("gfx/cport%d.lmp", m_class_cursor + 1)));
	M_DrawTransPic (242,54, R_CachePic ("gfx/menu/frame.lmp"));

}

void M_Class_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;
		
	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (++m_class_cursor >= CLASS_ITEMS)
			m_class_cursor = 0;
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (--m_class_cursor < 0)
			m_class_cursor = CLASS_ITEMS - 1;
		break;

	case K_ENTER:

//		sv_player->v.playerclass=m_class_cursor+1;
		Cbuf_AddText ( va ("playerclass %d\n", m_class_cursor+1) );
		m_entersound = true;
//		if (!class_flag)
//		{		
//			M_Menu_Difficulty_f();
//		}
//		else
		{
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
		}
		break;
	default:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		break;
	}

}


//=============================================================================
/* OPTIONS MENU */

enum 
{
	OPT_CUSTOMIZE = 0,
	OPT_CONSOLE,
	OPT_DEFAULTS,
	OPT_SCRSIZE,	//3
	OPT_GAMMA,		//4
	OPT_MOUSESPEED,	//5
	OPT_MUSICTYPE,	//6
	OPT_MUSICVOL,	//7
	OPT_SNDVOL,		//8
	OPT_ALWAYRUN,	//9
	OPT_INVMOUSE,	//10
	OPT_LOOKSPRING,	//11
	OPT_LOOKSTRAFE,	//12
	OPT_CROSSHAIR,	//13
	OPT_ALWAYSMLOOK,//14
	OPT_VIDEO,		//15
	OPTIONS_ITEMS
};

#define	SLIDER_RANGE	10

int		options_cursor;

void M_Menu_Options_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_options;
	m_entersound = true;
}


void M_AdjustSliders (int dir)
{
	S_StartLocalSound("raven/menu3.wav");

	switch (options_cursor)
	{
	case OPT_SCRSIZE:	// screen size
		scr_viewsize->value += dir * 10;
		if (scr_viewsize->value < 30)
			scr_viewsize->value = 30;
		if (scr_viewsize->value > 120)
			scr_viewsize->value = 120;
		Cvar_SetValue ("viewsize", scr_viewsize->value);
		SB_ViewSizeChanged();
		break;
	case OPT_GAMMA:	// gamma
		r_gamma->value += dir * 0.1;
		if (r_gamma->value < 1)
			r_gamma->value = 1;
		if (r_gamma->value > 2)
			r_gamma->value = 2;
		Cvar_SetValue ("r_gamma", r_gamma->value);
		break;
	case OPT_MOUSESPEED:	// mouse speed
		sensitivity->value += dir * 0.5;
		if (sensitivity->value < 1)
			sensitivity->value = 1;
		if (sensitivity->value > 11)
			sensitivity->value = 11;
		Cvar_SetValue ("sensitivity", sensitivity->value);
		break;
	case OPT_MUSICTYPE: // bgm type
		if (String::ICmp(bgmtype->string,"midi") == 0)
		{
			if (dir < 0)
				Cvar_Set("bgmtype","none");
			else
				Cvar_Set("bgmtype","cd");
		}
		else if (String::ICmp(bgmtype->string,"cd") == 0)
		{
			if (dir < 0)
				Cvar_Set("bgmtype","midi");
			else
				Cvar_Set("bgmtype","none");
		}
		else
		{
			if (dir < 0)
				Cvar_Set("bgmtype","cd");
			else
				Cvar_Set("bgmtype","midi");
		}
		break;

	case OPT_MUSICVOL:	// music volume
		bgmvolume->value += dir * 0.1;

		if (bgmvolume->value < 0)
			bgmvolume->value = 0;
		if (bgmvolume->value > 1)
			bgmvolume->value = 1;
		Cvar_SetValue ("bgmvolume", bgmvolume->value);
		break;
	case OPT_SNDVOL:	// sfx volume
		s_volume->value += dir * 0.1;
		if (s_volume->value < 0)
			s_volume->value = 0;
		if (s_volume->value > 1)
			s_volume->value = 1;
		Cvar_SetValue ("s_volume", s_volume->value);
		break;
	case OPT_ALWAYRUN:	// allways run
		if (cl_forwardspeed->value > 200)
		{
			Cvar_SetValue ("cl_forwardspeed", 200);
			Cvar_SetValue ("cl_backspeed", 200);
		}
		else
		{
			Cvar_SetValue ("cl_forwardspeed", 400);
			Cvar_SetValue ("cl_backspeed", 400);
		}
		break;
	
	case OPT_INVMOUSE:	// invert mouse
		Cvar_SetValue ("m_pitch", -m_pitch->value);
		break;
	
	case OPT_LOOKSPRING:	// lookspring
		Cvar_SetValue ("lookspring", !lookspring->value);
		break;
	
	case OPT_LOOKSTRAFE:	// lookstrafe
		Cvar_SetValue ("lookstrafe", !lookstrafe->value);
		break;

	case OPT_CROSSHAIR:	
		Cvar_SetValue ("crosshair", !crosshair->value);
		break;

	case OPT_ALWAYSMLOOK:
		Cvar_SetValue ("cl_freelook", !cl_freelook->value);
		break;

	case OPT_VIDEO:
		Cvar_SetValue ("cl_sbar", !cl_sbar->value);
		break;
	}
}


void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter (x-8, y, 256);
	for (i=0 ; i<SLIDER_RANGE ; i++)
		M_DrawCharacter (x + i*8, y, 257);
	M_DrawCharacter (x+i*8, y, 258);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 259);
}

void M_DrawCheckbox (int x, int y, int on)
{
#if 0
	if (on)
		M_DrawCharacter (x, y, 131);
	else
		M_DrawCharacter (x, y, 129);
#endif
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}

void M_Options_Draw (void)
{
	float		r;
	
	ScrollTitle("gfx/menu/title3.lmp");
	
	M_Print (16, 60+(0*8), "    Customize controls");
	M_Print (16, 60+(1*8), "         Go to console");
	M_Print (16, 60+(2*8), "     Reset to defaults");

	M_Print (16, 60+(3*8), "           Screen size");
	r = (scr_viewsize->value - 30) / (120 - 30);
	M_DrawSlider (220, 60+(3*8), r);

	M_Print (16, 60+(4*8), "            Brightness");
	r = (r_gamma->value - 1);
	M_DrawSlider (220, 60+(4*8), r);

	M_Print (16, 60+(5*8), "           Mouse Speed");
	r = (sensitivity->value - 1)/10;
	M_DrawSlider (220, 60+(5*8), r);

	M_Print (16, 60+(6*8), "            Music Type");
	if (String::ICmp(bgmtype->string,"midi") == 0)
		M_Print (220, 60+(6*8), "MIDI");
	else if (String::ICmp(bgmtype->string,"cd") == 0)
		M_Print (220, 60+(6*8), "CD");
	else
		M_Print (220, 60+(6*8), "None");

	M_Print (16, 60+(7*8), "          Music Volume");
	r = bgmvolume->value;
	M_DrawSlider (220, 60+(7*8), r);

	M_Print (16, 60+(8*8), "          Sound Volume");
	r = s_volume->value;
	M_DrawSlider (220, 60+(8*8), r);

	M_Print (16, 60+(9*8),				"            Always Run");
	M_DrawCheckbox (220, 60+(9*8), cl_forwardspeed->value > 200);

	M_Print (16, 60+(OPT_INVMOUSE*8),	"          Invert Mouse");
	M_DrawCheckbox (220, 60+(OPT_INVMOUSE*8), m_pitch->value < 0);

	M_Print (16, 60+(OPT_LOOKSPRING*8),	"            Lookspring");
	M_DrawCheckbox (220, 60+(OPT_LOOKSPRING*8), lookspring->value);

	M_Print (16, 60+(OPT_LOOKSTRAFE*8),	"            Lookstrafe");
	M_DrawCheckbox (220, 60+(OPT_LOOKSTRAFE*8), lookstrafe->value);

	M_Print (16, 60+(OPT_CROSSHAIR*8),	"        Show Crosshair");
	M_DrawCheckbox (220, 60+(OPT_CROSSHAIR*8), crosshair->value);

	M_Print (16,60+(OPT_ALWAYSMLOOK*8),	"            Mouse Look");
	M_DrawCheckbox (220, 60+(OPT_ALWAYSMLOOK*8), cl_freelook->value);

	M_Print (16, 60+(OPT_VIDEO*8),	"         Video Options");

// cursor
	M_DrawCharacter (200, 60 + options_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Options_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
		
	case K_ENTER:
		m_entersound = true;
		switch (options_cursor)
		{
		case OPT_CUSTOMIZE:
			M_Menu_Keys_f ();
			break;
		case OPT_CONSOLE:
			//m_state = m_none;
			//Con_ToggleConsole_f ();
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
			break;
		case OPT_DEFAULTS:
			Cbuf_AddText ("exec default.cfg\n");
			break;
		case OPT_VIDEO:
			M_Menu_Video_f ();
			break;
		default:
			M_AdjustSliders (1);
			break;
		}
		return;
	
	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		options_cursor--;
		if (options_cursor < 0)
			options_cursor = OPTIONS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		options_cursor++;
		if (options_cursor >= OPTIONS_ITEMS)
			options_cursor = 0;
		break;	

	case K_LEFTARROW:
		M_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;
	}
}


//=============================================================================
/* KEYS MENU */

const char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"impulse 10", 		"change weapon"},
{"+jump", 			"jump / swim up"},
{"+forward", 		"walk forward"},
{"+back", 			"backpedal"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+speed", 			"run"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+strafe", 		"sidestep"},
{"+crouch",			"crouch"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"centerview", 		"center view"},
{"+mlook", 			"mouse look"},
{"+klook", 			"keyboard look"},
{"+moveup",			"swim up"},
{"+movedown",		"swim down"},
{"impulse 13", 		"use object"},
{"invuse",			"use inv item"},
{"invdrop",			"drop inv item"},
{"+showinfo",		"full inventory"},
{"+showdm",			"info / frags"},
{"toggle_dm",		"toggle frags"},
{"+shownames",		"player names"},
{"invleft",			"inv move left"},
{"invright",		"inv move right"},
{"impulse 100",		"inv:torch"},
{"impulse 101",		"inv:qrtz flask"},
{"impulse 102",		"inv:mystic urn"},
{"impulse 103",		"inv:krater"},
{"impulse 104",		"inv:chaos devc"},
{"impulse 105",		"inv:tome power"},
{"impulse 106",		"inv:summon stn"},
{"impulse 107",		"inv:invisiblty"},
{"impulse 108",		"inv:glyph"},
{"impulse 109",		"inv:boots"},
{"impulse 110",		"inv:repulsion"},
{"impulse 111",		"inv:bo peep"},
{"impulse 112",		"inv:flight"},
{"impulse 113",		"inv:force cube"},
{"impulse 114",		"inv:icon defn"}
};

#define	NUMCOMMANDS	(sizeof(bindnames)/sizeof(bindnames[0]))

#define KEYS_SIZE 14

int		keys_cursor;
int		bind_grab;
int		keys_top = 0;

void M_Menu_Keys_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_keys;
	m_entersound = true;
}


void M_FindKeysForCommand (const char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l,l2;
	char	*b;

	twokeys[0] = twokeys[1] = -1; 
	l = String::Length(command); 
	count = 0;

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j]; 
		if (!b)
			continue; 
		if (!String::NCmp(b, command, l))
		{
			l2= String::Length(b); 
			if (l == l2)
			{		
				twokeys[count] = j; 
				count++; 
				if (count == 2)
					break;
			}		
		}
	}
}

void M_UnbindCommand (const char *command)
{
	int		j;
	int		l;
	char	*b;

	l = String::Length(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!String::NCmp(b, command, l) )
			Key_SetBinding (j, "");
	}
}


void M_Keys_Draw (void)
{
	int		i, l;
	int		keys[2];
	const char	*name;
	int		x, y;

	ScrollTitle("gfx/menu/title6.lmp");

//	M_DrawTextBox (6,56, 35,16);

//	p = R_CachePic("gfx/menu/hback.lmp");
//	M_DrawTransPicCropped(8, 62, p);

	if (bind_grab)
		M_Print (12, 64, "Press a key or button for this action");
	else
		M_Print (18, 64, "Enter to change, backspace to clear");
		
	if (keys_top) 
		M_DrawCharacter (6, 80, 128);
	if (keys_top + KEYS_SIZE < (int)NUMCOMMANDS)
		M_DrawCharacter (6, 80 + ((KEYS_SIZE-1)*8), 129);

// search for known bindings
	for (i=0 ; i<KEYS_SIZE ; i++)
	{
		y = 80 + 8*i;

		M_Print (16, y, bindnames[i+keys_top][1]);

		l = String::Length(bindnames[i+keys_top][0]);
		
		M_FindKeysForCommand (bindnames[i+keys_top][0], keys);
		
		if (keys[0] == -1)
		{
			M_Print (140, y, "???");
		}
		else
		{
			name = Key_KeynumToString (keys[0]);
			M_Print (140, y, name);
			x = String::Length(name) * 8;
			if (keys[1] != -1)
			{
				M_Print (140 + x + 8, y, "or");
				M_Print (140 + x + 32, y, Key_KeynumToString (keys[1]));
			}
		}
	}

	if (bind_grab)
		M_DrawCharacter (130, 80 + (keys_cursor-keys_top)*8, '=');
	else
		M_DrawCharacter (130, 80 + (keys_cursor-keys_top)*8, 12+((int)(realtime*4)&1));
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[2];
	
	if (bind_grab)
	{	// defining a key
		S_StartLocalSound("raven/menu1.wav");
		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf (cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString (k), bindnames[keys_cursor][0]);			
			Cbuf_InsertText (cmd);
		}
		
		bind_grab = false;
		return;
	}
	
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
			keys_cursor = NUMCOMMANDS-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("raven/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= (int)NUMCOMMANDS)
			keys_cursor = 0;
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand (bindnames[keys_cursor][0], keys);
		S_StartLocalSound("raven/menu2.wav");
		if (keys[1] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_StartLocalSound("raven/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}

	if (keys_cursor < keys_top) 
		keys_top = keys_cursor;
	else if (keys_cursor >= keys_top+KEYS_SIZE)
		keys_top = keys_cursor - KEYS_SIZE + 1;
}

//=============================================================================
/* VIDEO MENU */

#define MAX_COLUMN_SIZE		9
#define MODE_AREA_HEIGHT	(MAX_COLUMN_SIZE + 2)

void M_Menu_Video_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_video;
	m_entersound = true;
}


void M_Video_Draw (void)
{
	ScrollTitle("gfx/menu/title7.lmp");

	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*2,
			 "Video modes must be set from the");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*3,
			 "console with set r_mode <number>");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*4,
			 "and set r_colorbits <bits-per-pixel>");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*6,
			 "Select windowed mode with set r_fullscreen 0");
}


void M_Video_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		S_StartLocalSound("raven/menu1.wav");
		M_Menu_Options_f ();
		break;

	default:
		break;
	}
}

//=============================================================================
/* HELP MENU */

int		help_page;

#define	NUM_HELP_PAGES	5
#define	NUM_SG_HELP_PAGES	10//Siege has more help



void M_Menu_Help_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}



void M_Help_Draw (void)
{
	if(cl_siege)
		M_DrawPic (0, 0, R_CachePic ( va("gfx/menu/sghelp%02i.lmp", help_page+1)) );
	else
		M_DrawPic (0, 0, R_CachePic ( va("gfx/menu/help%02i.lmp", help_page+1)) );
}


void M_Help_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
		
	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if(cl_siege)
		{
			if (++help_page >= NUM_SG_HELP_PAGES)
				help_page = 0;
		}
		else if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
		{
			if(cl_siege)
				help_page = NUM_SG_HELP_PAGES-1;
			else
				help_page = NUM_HELP_PAGES-1;
		}
		break;
	}

}

//=============================================================================
/* QUIT MENU */

int		msgNumber;
menu_state_t	m_quit_prevstate;
qboolean	wasInMenus;

#ifndef	_WIN32
const char *quitMessage [] = 
{
/* .........1.........2.... */
  "   Look! Behind you!    ",
  "  There's a big nasty   ",
  "   thing - shoot it!    ",
  "                        ",
 
  "  You can't go now, I   ",
  "   was just getting     ",
  "    warmed up.          ",
  "                        ",

  "    One more game.      ",
  "      C'mon...          ",
  "   Who's gonna know?    ",
  "                        ",

  "   What's the matter?   ",
  "   Palms too sweaty to  ",
  "     keep playing?      ",
  "                        ",
 
  "  Watch your local store",
  "      for Hexen 2       ",
  "    plush toys and      ",
  "    greeting cards!     ",
 
  "  Hexen 2...            ",
  "                        ",
  "    Too much is never   ",
  "        enough.         ",
 
  "  Sure go ahead and     ",
  "  leave.  But I know    ",
  "  you'll be back.       ",
  "                        ",
 
  "                        ",
  "  Insert cute phrase    ",
  "        here            ",
  "                        "
};
#endif

static float LinePos;
static int LineTimes;
static int MaxLines;
const char **LineText;
static qboolean SoundPlayed;


#define MAX_LINES 145+25

const char *CreditText[MAX_LINES] =
{
   "HexenWorld",
   "",
   "Lead Programmer: Rick Johnson",
   "",
   "Programming:",
   "   Nathan Albury",
   "   Ron Midthun",
   "   Steve Sengele",
   "   Mike Gummelt",
   "   James Monroe",
   "",
   "Deathmatch Levels:",
   "   Kenn Hoekstra",
   "   Mike Renner",
   "   Jeremy Statz",
   "   Jon Zuk",
   "",
   "Special thanks to:",
   "   Dave Kirsch",
   "   William Mull",
   "   Jack Mathews",
   "",
   "",
   "Hexen2",
   "",
   "Project Director: Brian Raffel",
   "",
   "Lead Programmer: Rick Johnson",
   "",
   "Programming:",
   "   Ben Gokey",
   "   Bob Love",
   "   Mike Gummelt",
   "",
   "Additional Programming:",
   "   Josh Weier",
   "",
   "Lead Design: Eric Biessman",
   "",
   "Design:",
   "   Brian Raffel",
   "   Brian Frank",
   "   Tom Odell",
   "",
   "Art Director: Brian Pelletier",
   "",
   "Art:",
   "   Shane Gurno",
   "   Jim Sumwalt",
   "   Mark Morgan",
   "   Kim Lathrop",
   "   Ted Halsted",
   "   Rebecca Rettenmund",
   "   Les Dorscheid",
   "",
   "Animation:",
   "   Chaos (Mike Werckle)",
   "   Brian Shubat",
   "",
   "Cinematics:",
   "   Jeff Dewitt",
   "   Jeffrey P. Lampo",
   "",
   "Music:",
   "   Kevin Schilder",
   "",
   "Sound:",
   "   Kevin Schilder",
   "   Chia Chin Lee",
   "",
   "",
   "Activision",
   "",
   "Producer:",
   "   Steve Stringer",
   "",
   "Localization Producer:",
   "   Sandi Isaacs",
   "",
   "Marketing Product Manager:",
   "   Henk Hartong",
   "",
   "European Marketing",
   "Product Director:",
   "   Janine Johnson",
   "",
   "Marketing Associate:",
   "   Kevin Kraff",
   "",
   "Senior Quality",
   "Assurance Lead:",
   "   Tim Vanlaw",
   "",
   "Quality Assurance Lead:",
   "   John Tam",
   "",
   "Quality Assurance Team:",
   "   Steve Rosenthal, Mike Spann,",
   "   Steve Elwell, Kelly Wand,",
   "   Kip Stolberg, Igor Krinitskiy,",
   "   Ian Stevens, Marilena Wahmann,",
   "   David Baker, Winnie Lee",
   "",
   "Documentation:",
   "   Mike Rivera, Sylvia Orzel,",
   "   Belinda Vansickle",
   "",
   "Chronicle of Deeds written by:",
   "   Joe Grant Bell",
   "",
   "Localization:",
   "   Nathalie Dove, Lucy Morgan,",
   "   Alex Wylde, Nicky Kerth",
   "",
   "Installer by:",
   "   Steve Stringer, Adam Goldberg,",
   "   Tanya Martino, Eric Schmidt,",
   "   Ronnie Lane",
   "",
   "Art Assistance by:",
   "   Carey Chico and Franz Boehm",
   "",
   "BizDev Babe:",
   "   Jamie Bafus",
   "",
   "And...",
   "",
   "Deal Guru:",
   "   Mitch Lasky",
   "",
   "",
   "Thanks to Id software:",
   "   John Carmack",
   "   Adrian Carmack",
   "   Kevin Cloud",
   "   Barrett 'Bear'  Alexander",
   "   American McGee",
   "",
   "",
   "Published by Id Software, Inc.",
   "Distributed by Activision, Inc.",
   "",
   "The Id Software Technology used",
   "under license in Hexen II (tm)",
   "(c) 1996, 1997 Id Software, Inc.",
   "All Rights Reserved.",
   "",
   "Hexen(r) is a registered trademark",
   "of Raven Software Corp.",
   "Hexen II (tm) and the Raven logo",
   "are trademarks of Raven Software",
   "Corp.  The Id Software name and",
   "id logo are trademarks of",
   "Id Software, Inc.  Activision(r)",
   "is a registered trademark of",
   "Activision, Inc. All other",
   "trademarks are the property of",
   "their respective owners.",
   "",
   "",
   "",
   "Send bug descriptions to:",
   "   h2bugs@mail.ravensoft.com",
   "",
   "Special thanks to Gary McTaggart",
   "at 3dfx for his help with",
   "the gl version!",
   "",
   "No snails were harmed in the",
   "making of this game!"
};

#define MAX_LINES2 158+27

const char *Credit2Text[MAX_LINES2] =
{
   "HexenWorld",
   "",
   "Superior Groucher:"
   "   Rick 'Grrr' Johnson",
   "",
   "Bug Creators:",
   "   Nathan 'Glory Code' Albury",
   "   Ron 'Stealth' Midthun",
   "   Steve 'Tie Dye' Sengele",
   "   Mike 'Foos' Gummelt",
   "   James 'Happy' Monroe",
   "",
   "Sloppy Joe Makers:",
   "   Kenn 'I'm a broken man'",
   "      Hoekstra",
   "   Mike 'Outback' Renner",
   "   Jeremy 'Under-rated' Statz",
   "   Jon Zuk",
   "",
   "Avoid the Noid:",
   "   Dave 'Zoid' Kirsch",
   "   William 'Phoeb' Mull",
   "   Jack 'Morbid' Mathews",
   "",
   "",
   "Hexen2",
   "",
   "Map Master: ",
   "   'Caffeine Buzz' Raffel",
   "",
   "Code Warrior:",
   "   Rick 'Superfly' Johnson",
   "",
   "Grunt Boys:",
   "   'Judah' Ben Gokey",
   "   Bob 'Whipped' Love",
   "   Mike '9-Pointer' Gummelt",
   "",
   "Additional Grunting:",
   "   Josh 'Intern' Weier",
   "",
   "Whippin' Boy:",
   "   Eric 'Baby' Biessman",
   "",
   "Crazy Levelers:",
   "   'Big Daddy' Brian Raffel",
   "   Brian 'Red' Frank",
   "   Tom 'Texture Alignment' Odell",
   "",
   "Art Lord:",
   "   Brian 'Mr. Calm' Pelletier",
   "",
   "Pixel Monkies:",
   "   Shane 'Duh' Gurno",
   "   'Slim' Jim Sumwalt",
   "   Mark 'Dad Gummit' Morgan",
   "   Kim 'Toy Master' Lathrop",
   "   'Drop Dead' Ted Halsted",
   "   Rebecca 'Zombie' Rettenmund",
   "   Les 'is not more' Dorscheid",
   "",
   "Salad Shooters:",
   "   Mike 'Chaos' Werckle",
   "   Brian 'Mutton Chops' Shubat",
   "",
   "Last Minute Addition:",
   "   Jeff 'Bud Bundy' Dewitt",
   "   Jeffrey 'Misspalld' Lampo",
   "",
   "Random Notes:",
   "   Kevin 'I Already Paid' Schilder",
   "",
   "Grunts, Groans, and Moans:",
   "   Kevin 'I Already Paid' Schilder",
   "   Chia 'Pet' Chin Lee",
   "",
   "",
   "Activision",
   "",
   "Producer:",
   "   Steve 'Ferris' Stringer",
   "",
   "Localization Producer:",
   "   Sandi 'Boneduster' Isaacs",
   "",
   "Marketing Product Manager:",
   "   Henk 'A-10' Hartong",
   "",
   "European Marketing",
   "Product Director:",
   "   Janine Johnson",
   "",
   "Marketing Associate:",
   "   Kevin 'Savage' Kraff",
   "",
   "Senior Quality",
   "Assurance Lead:",
   "   Tim 'Outlaw' Vanlaw",
   "",
   "Quality Assurance Lead:",
   "   John 'Armadillo' Tam",
   "",
   "Quality Assurance Team:",
   "   Steve 'Rhinochoadicus'",
   "      Rosenthal,",
   "   Mike 'Dragonhawk' Spann,",
   "   Steve 'Zendog' Elwell,",
   "   Kelly 'Li'l Bastard' Wand,",
   "   Kip 'Angus' Stolberg,",
   "   Igor 'Russo' Krinitskiy,",
   "   Ian 'Cracker' Stevens,",
   "   Marilena 'Raveness-X' Wahmann,",
   "   David 'Spicegirl' Baker,",
   "   Winnie 'Mew' Lee",
   "",
   "Documentation:",
   "   Mike Rivera, Sylvia Orzel,",
   "   Belinda Vansickle",
   "",
   "Chronicle of Deeds written by:",
   "   Joe Grant Bell",
   "",
   "Localization:",
   "   Nathalie Dove, Lucy Morgan,",
   "   Alex Wylde, Nicky Kerth",
   "",
   "Installer by:",
   "   Steve 'Bahh' Stringer,",
   "   Adam Goldberg, Tanya Martino,",
   "   Eric Schmidt, Ronnie Lane",
   "",
   "Art Assistance by:",
   "   Carey 'Damien' Chico and",
   "   Franz Boehm",
   "",
   "BizDev Babe:",
   "   Jamie Bafus",
   "",
   "And...",
   "",
   "Deal Guru:",
   "   Mitch 'I'll buy that' Lasky",
   "",
   "",
   "Thanks to Id software:",
   "   John Carmack",
   "   Adrian Carmack",
   "   Kevin Cloud",
   "   Barrett 'Bear'  Alexander",
   "   American McGee",
   "",
   "",
   "Published by Id Software, Inc.",
   "Distributed by Activision, Inc.",
   "",
   "The Id Software Technology used",
   "under license in Hexen II (tm)",
   "(c) 1996, 1997 Id Software, Inc.",
   "All Rights Reserved.",
   "",
   "Hexen(r) is a registered trademark",
   "of Raven Software Corp.",
   "Hexen II (tm) and the Raven logo",
   "are trademarks of Raven Software",
   "Corp.  The Id Software name and",
   "id logo are trademarks of",
   "Id Software, Inc.  Activision(r)",
   "is a registered trademark of",
   "Activision, Inc. All other",
   "trademarks are the property of",
   "their respective owners.",
   "",
   "",
   "",
   "Send bug descriptions to:",
   "   h2bugs@mail.ravensoft.com",
   "",
   "Special thanks to Bob for",
   "remembering 'P' is for Polymorph",
   "",
   "",
   "See the next movie in the long",
   "awaited sequel, starring",
   "Bobby Love in,",
   "   Out of Traction, Back in Action!",
};

#define QUIT_SIZE 18

void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;
	wasInMenus = !!(in_keyCatchers & KEYCATCH_UI);
	in_keyCatchers |= KEYCATCH_UI;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	msgNumber = rand()&7;

	LinePos = 0;
	LineTimes = 0;
	LineText = CreditText;
	MaxLines = MAX_LINES;
	SoundPlayed = false;
}


void M_Quit_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
		in_keyCatchers |= KEYCATCH_CONSOLE;
		CL_Disconnect ();
		Sys_Quit ();
		break;

	default:
		break;
	}

}

#define VSTR(x) #x
#define VSTR2(x) VSTR(x)

void M_Quit_Draw (void)
{
	int i,x,y,place,topy;
	image_t	*p;

	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_quit;
	}

	LinePos += host_frametime*1.75;
	if (LinePos > MaxLines + QUIT_SIZE + 2)
	{
		LinePos = 0;
		SoundPlayed = false;
		LineTimes++;
		if (LineTimes >= 2)
		{
			MaxLines = MAX_LINES2;
			LineText = Credit2Text;
		}
	}

	y = 12;
	M_DrawTextBox (0, 0, 38, 23);
	M_PrintWhite (16, y,  "      Hexen2World version " VSTR2(VERSION) "      ");	y += 8;
	M_PrintWhite (16, y,  "         by Raven Software          ");	y += 16;

	if (LinePos > 55 && !SoundPlayed && LineText == Credit2Text)
	{
		S_StartLocalSound("rj/steve.wav");
		SoundPlayed = true;
	}
	topy = y;
	place = LinePos;
	y -= (LinePos - (int)LinePos) * 8;
	for(i=0;i<QUIT_SIZE;i++,y+=8)
	{
		if (i+place-QUIT_SIZE >= MaxLines)
			break;
		if (i+place < QUIT_SIZE) 
			continue;

		if (LineText[i+place-QUIT_SIZE][0] == ' ')
			M_PrintWhite(24,y,LineText[i+place-QUIT_SIZE]);
		else
			M_Print(24,y,LineText[i+place-QUIT_SIZE]);
	}

	p = R_CachePic ("gfx/box_mm2.lmp");
	x = 24;
	y = topy-8;
	for(i=4;i<36;i++,x+=8)
	{
		M_DrawTransPic (x, y, p);
	}

	p = R_CachePic ("gfx/box_mm2.lmp");
	x = 24;
	y = topy+(QUIT_SIZE*8)-8;
	for(i=4;i<36;i++,x+=8)
	{
		M_DrawTransPic (x, y, p);
	}

	y += 8;
	M_PrintWhite (16, y,  "          Press y to exit           ");

/*	y = 12;
	M_DrawTextBox (0, 0, 38, 23);
	M_PrintWhite (16, y,  "        Hexen II version 0.0        ");	y += 8;
	M_PrintWhite (16, y,  "         by Raven Software          ");	y += 16;
	M_PrintWhite (16, y,  "Programming        Art              ");	y += 8;
	M_Print (16, y,       " Ben Gokey          Shane Gurno     ");	y += 8;
	M_Print (16, y,       " Rick Johnson       Mike Werckle    ");	y += 8;
	M_Print (16, y,       " Bob Love           Mark Morgan     ");	y += 8;
	M_Print (16, y,       " Mike Gummelt       Brian Pelletier ");	y += 8;
	M_Print (16, y,       "                    Kim Lathrop     ");	y += 8;
	M_PrintWhite (16, y,  "Design                              ");	
	M_Print (16, y,       "                    Les Dorscheid   ");	y += 8;
	M_Print (16, y,       " Brian Raffel       Jim Sumwalt     ");	y += 8;
	M_Print (16, y,       " Eric Biessman      Brian Shubat    ");	y += 16;
	M_PrintWhite (16, y,  "Sound Effects      Intern           ");	y += 8;
	M_Print (16, y,       " Kevin Schilder     Josh Weier      ");	y += 16;
	M_PrintWhite (16, y,  "          Press y to exit           ");	y += 8;*/

}

void M_Menu_SinglePlayer_f (void) {
	m_state = m_singleplayer;
}

void M_SinglePlayer_Draw (void)
{
	ScrollTitle("gfx/menu/title1.lmp");

	M_DrawTextBox (60, 10*8, 23, 4);	
	M_PrintWhite (92, 12*8, "HexenWorld is for");
	M_PrintWhite (88, 13*8, "Internet play only");

}

void M_SinglePlayer_Key (int key) {
	if (key == K_ESCAPE || key==K_ENTER)
		m_state = m_main;
}

/*
void M_Menu_MultiPlayer_f (void) {
	m_state = m_multiplayer;
}

void M_MultiPlayer_Draw (void)
{
	ScrollTitle("gfx/menu/title1.lmp");

	M_DrawTextBox (46, 8*8, 27, 9);	
	M_PrintWhite (72, 10*8, "If you want to find HW  ");
	M_PrintWhite (72, 11*8, "games, head on over to: ");
	     M_Print (72, 12*8, "   www.hexenworld.net   ");
	M_PrintWhite (72, 13*8, "          or            ");
	     M_Print (72, 14*8, "   www.quakespy.com     ");
	M_PrintWhite (72, 15*8, "For pointers on getting ");
	M_PrintWhite (72, 16*8, "        started!        ");
}

void M_MultiPlayer_Key (key) {
	if (key == K_ESCAPE || key==K_ENTER)
		m_state = m_main;
}
*/

//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	2

void M_Menu_MultiPlayer_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_multiplayer;
	m_entersound = true;

	message = NULL;
}
				

void M_MultiPlayer_Draw (void)
{
	int		f;

	ScrollTitle("gfx/menu/title4.lmp");
//	M_DrawTransPic (72, 32, R_CachePic ("gfx/mp_menu.lmp") );

	M_DrawBigString (72,60+(0*20),"JOIN A GAME");
	M_DrawBigString (72,60+(1*20),"SETUP");

	f = (int)(realtime * 10)%8;
	M_DrawTransPic (43, 54 + m_multiplayer_cursor * 20,R_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );

	if (message)
	{
		M_PrintWhite ((320/2) - ((27*8)/2), 168, message);
		M_PrintWhite ((320/2) - ((27*8)/2), 176, message2);
		if (realtime - 5 > message_time)
			message = NULL;
	}
}


void M_MultiPlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
		
	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			M_Menu_Connect_f ();
			break;

		case 1:
			M_Menu_Setup_f ();
			break;
		}
	}
}

//=============================================================================
/* CONNECT MENU */

#define MAX_HOST_NAMES 10
#define MAX_HOST_SIZE 80
char save_names[MAX_HOST_NAMES][MAX_HOST_SIZE];

Cvar*	hostname1;
Cvar*	hostname2;
Cvar*	hostname3;
Cvar*	hostname4;
Cvar*	hostname5;
Cvar*	hostname6;
Cvar*	hostname7;
Cvar*	hostname8;
Cvar*	hostname9;
Cvar*	hostname10;

int connect_cursor = 0;
#define MAX_CONNECT_CMDS 11

int		connect_cursor_table[MAX_CONNECT_CMDS] =
{
	72+0*8,
	72+1*8,
	72+2*8,
	72+3*8,
	72+4*8,
	72+5*8,
	72+6*8,
	72+7*8,
	72+8*8,
	72+9*8,

	72+11*8,
};


void M_Menu_Connect_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_mconnect;
	m_entersound = true;

	message = NULL;

	String::Cpy(save_names[0],hostname1->string);
	String::Cpy(save_names[1],hostname2->string);
	String::Cpy(save_names[2],hostname3->string);
	String::Cpy(save_names[3],hostname4->string);
	String::Cpy(save_names[4],hostname5->string);
	String::Cpy(save_names[5],hostname6->string);
	String::Cpy(save_names[6],hostname7->string);
	String::Cpy(save_names[7],hostname8->string);
	String::Cpy(save_names[8],hostname9->string);
	String::Cpy(save_names[9],hostname10->string);
}

void M_Connect_Draw (void)
{
	int		length;
	int		i,y;
	char	temp[MAX_HOST_SIZE];

	ScrollTitle("gfx/menu/title4.lmp");

	if (connect_cursor < MAX_HOST_NAMES)
	{
		M_DrawTextBox (16, 48, 34, 1);

		String::Cpy(temp,save_names[connect_cursor]);
		length = String::Length(temp);
		if (length > 33)
		{
			i = length-33;
		}
		else
		{
			i = 0;
		}
		M_Print (24, 56, &temp[i]);
		M_DrawCharacter (24 + 8*(length-i), 56, 10+((int)(realtime*4)&1));
	}

	y = 72;
	for(i=0;i<MAX_HOST_NAMES;i++,y+=8)
	{
		sprintf(temp,"%d.",i+1);
		if (i == connect_cursor)
		{
			M_Print(24,y,temp);
		}
		else
		{
			M_PrintWhite(24,y,temp);
		}

		String::Cpy(temp,save_names[i]);
		temp[30] = 0;
		if (i == connect_cursor)
		{
			M_Print(56, y, temp);
		}
		else
		{
			M_PrintWhite(56, y, temp);
		}
	}

	M_Print (24, y+8, "Save Changes");

	M_DrawCharacter (8, connect_cursor_table[connect_cursor], 12+((int)(realtime*4)&1));
}

void M_Connect_Key (int k)
{
	int			l;

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		connect_cursor--;
		if (connect_cursor < 0)
			connect_cursor = MAX_CONNECT_CMDS-1;
		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		connect_cursor++;
		if (connect_cursor >= MAX_CONNECT_CMDS)
			connect_cursor = 0;
		break;

	case K_ENTER:
		Cvar_Set ("host1",save_names[0]);
		Cvar_Set ("host2",save_names[1]);
		Cvar_Set ("host3",save_names[2]);
		Cvar_Set ("host4",save_names[3]);
		Cvar_Set ("host5",save_names[4]);
		Cvar_Set ("host6",save_names[5]);
		Cvar_Set ("host7",save_names[6]);
		Cvar_Set ("host8",save_names[7]);
		Cvar_Set ("host9",save_names[8]);
		Cvar_Set ("host10",save_names[9]);

		if (connect_cursor < MAX_HOST_NAMES)
		{
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
			Cbuf_AddText ( va ("connect %s\n", save_names[connect_cursor]) );
		}
		else
		{
			m_entersound = true;
			M_Menu_MultiPlayer_f ();
		}
		break;
	
	case K_BACKSPACE:
		if (connect_cursor < MAX_HOST_NAMES)
		{
			l = String::Length(save_names[connect_cursor]);
			if (l)
			{
				save_names[connect_cursor][l-1] = 0;
			}
		}
		break;
		
	default:
		if (k < 32 || k > 127)
			break;
		if (connect_cursor < MAX_HOST_NAMES)
		{
			l = String::Length(save_names[connect_cursor]);
			if (l < MAX_HOST_SIZE-1)
			{
				save_names[connect_cursor][l+1] = 0;
				save_names[connect_cursor][l] = k;
			}
		}
	}
}


//=============================================================================
/* SETUP MENU */

int		setup_cursor = 6;
int		setup_cursor_table[] = {40, 56, 72, 88, 112, 136, 164};

char	setup_myname[16];
int		setup_oldtop;
int		setup_oldbottom;
int		setup_top;
int		setup_bottom;
int		class_limit;

#define	NUM_SETUP_CMDS	7

extern Cvar*	name;
extern Cvar*	topcolor;
extern Cvar*	bottomcolor;

void M_Menu_Setup_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_setup;
	m_entersound = true;
	String::Cpy(setup_myname, name->string);
	setup_top = setup_oldtop = (int)topcolor->value;
	setup_bottom = setup_oldbottom = (int)bottomcolor->value;


	if(!com_portals)
		if(playerclass->value==CLASS_DEMON)
			playerclass->value = 0;
	if(String::ICmp(fs_gamedir, "siege"))
		if(playerclass->value==CLASS_DWARF)
			playerclass->value = 0;

	setup_class = playerclass->value;

//	if (com_portals||cl_siege)//FIXME!!!
//	{
		class_limit = MAX_PLAYER_CLASS;
//	}
//	else
//	{
//		class_limit = MAX_PLAYER_CLASS-1;
//	}

	if (setup_class < 0 || setup_class > class_limit)
		setup_class = 1;
	which_class = setup_class;
}
				

void M_Setup_Draw (void)
{
	image_t			*p;
	int				i;
	static qboolean wait;

	ScrollTitle("gfx/menu/title4.lmp");
	
	M_Print (64, 56, "Your name");
	M_DrawTextBox (160, 48, 16, 1);
	M_PrintWhite (168, 56, setup_myname);

	M_Print (64, 72, "Spectator: ");
	if (spectator->value)
	{
		M_PrintWhite (64 + 12*8, 72, "YES");
	}
	else
	{
		M_PrintWhite (64 + 12*8, 72, "NO");
	}

	M_Print (64, 88, "Current Class: ");

	if(!com_portals)
		if(setup_class==CLASS_DEMON)
			setup_class = 0;
	if(String::ICmp(fs_gamedir, "siege"))
		if(setup_class==CLASS_DWARF)
			setup_class = 0;
	switch(setup_class)
	{
		case 0:M_PrintWhite (88, 96, "Random");
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:M_PrintWhite (88, 96, ClassNames[setup_class-1]);
		case 6:M_PrintWhite (88, 96, ClassNames[setup_class-1]);
			break;
	}

	M_Print (64, 112, "First color patch");
	M_Print (64, 136, "Second color patch");
	
	M_DrawTextBox (64, 164-8, 14, 1);
	M_Print (72, 164, "Accept Changes");

	if (setup_class == 0)
	{
		i = (int)(realtime * 10)%8;

		if ((i == 0 && !wait) || which_class == 0)
		{
			if(!com_portals)
			{//not succubus
				if(String::ICmp(fs_gamedir, "siege"))
					which_class = (rand() % CLASS_THEIF) + 1;
				else
				{
					which_class = (rand() % CLASS_DEMON) + 1;
					if(which_class==CLASS_DEMON)
						which_class=CLASS_DWARF;
				}
			}
			else
			{
				if(String::ICmp(fs_gamedir, "siege"))
					which_class = (rand() % CLASS_DEMON) + 1;
				else
					which_class = (rand() % class_limit) + 1;
			}
			wait = true;
		}
		else if (i)
		{
			wait = false;
		}
	}
	else
	{
		which_class = setup_class;
	}
	p = R_CachePicWithTransPixels(va("gfx/menu/netp%i.lmp", which_class), menuplyr_pixels[which_class - 1]);
	CL_CalcHexen2SkinTranslation(setup_top, setup_bottom, which_class, translationTable);
	R_CreateOrUpdateTranslatedImage(translate_texture[which_class-1], va("translate_pic%d", which_class), menuplyr_pixels[which_class-1], translationTable, PLAYER_PIC_WIDTH, PLAYER_PIC_HEIGHT);
	M_DrawPic(220, 72, translate_texture[which_class - 1]);

	M_DrawCharacter (56, setup_cursor_table [setup_cursor], 12+((int)(realtime*4)&1));

	if (setup_cursor == 1)
		M_DrawCharacter (168 + 8*String::Length(setup_myname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));
}


void M_Setup_Key (int k)
{
	int			l;

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 1)
			setup_cursor = NUM_SETUP_CMDS-1;
		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 1;
		break;

	case K_LEFTARROW:
		if (setup_cursor < 2)
			return;
		S_StartLocalSound("raven/menu3.wav");
		if (setup_cursor == 2)
		{
			if (spectator->value)
			{
				Cvar_Set("spectator","0");
//				spectator.value = 0;
			}
			else
			{
				Cvar_Set("spectator","1");
//				spectator.value = 1;
			}
			cl.qh_spectator = spectator->value;
		}
		if (setup_cursor == 3)
		{
			setup_class--;
			if (setup_class < 0) 
				setup_class = class_limit;
		}
		if (setup_cursor == 4)
			setup_top = setup_top - 1;
		if (setup_cursor == 5)
			setup_bottom = setup_bottom - 1;
		break;
	case K_RIGHTARROW:
		if (setup_cursor < 2)
			return;
forward:
		S_StartLocalSound("raven/menu3.wav");
		if (setup_cursor == 2)
		{
			if (spectator->value)
			{
				Cvar_Set("spectator","0");
//				spectator.value = 0;
			}
			else
			{
				Cvar_Set("spectator","1");
//				spectator.value = 1;
			}
			cl.qh_spectator = spectator->value;

		}
		if (setup_cursor == 3)
		{
			setup_class++;
			if (setup_class > class_limit) 
				setup_class = 0;
		}
		if (setup_cursor == 4)
			setup_top = setup_top + 1;
		if (setup_cursor == 5)
			setup_bottom = setup_bottom + 1;
		break;

	case K_ENTER:
		if (setup_cursor == 0 || setup_cursor == 1)
			return;

		if (setup_cursor == 2 || setup_cursor == 3 || setup_cursor == 4 || setup_cursor == 5)
			goto forward;

		if (String::Cmp(name->string, setup_myname) != 0)
			Cbuf_AddText ( va ("name \"%s\"\n", setup_myname) );
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddText( va ("color %i %i\n", setup_top, setup_bottom) );
		Cbuf_AddText ( va ("playerclass %d\n", setup_class) );
		m_entersound = true;
		M_Menu_MultiPlayer_f ();
		break;
	
	case K_BACKSPACE:
		if (setup_cursor == 1)
		{
			if (String::Length(setup_myname))
				setup_myname[String::Length(setup_myname)-1] = 0;
		}
		break;
		
	default:
		if (k < 32 || k > 127)
			break;
		if (setup_cursor == 0)
		{
		}
		if (setup_cursor == 1)
		{
			l = String::Length(setup_myname);
			if (l < 15)
			{
				setup_myname[l+1] = 0;
				setup_myname[l] = k;
			}
		}
	}

	if (setup_top > 10)
		setup_top = 0;
	if (setup_top < 0)
		setup_top = 10;
	if (setup_bottom > 10)
		setup_bottom = 0;
	if (setup_bottom < 0)
		setup_bottom = 10;
}


//=============================================================================
/* Menu Subsystem */


void M_Init (void)
{
	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("help", M_Menu_Help_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
	Cmd_AddCommand ("menu_class", M_Menu_Class2_f);
	Cmd_AddCommand ("menu_connect" , M_Menu_Connect_f);

	hostname1 = Cvar_Get("host1","equalizer.ravensoft.com", CVAR_ARCHIVE);
	hostname2 = Cvar_Get("host2","", CVAR_ARCHIVE);
	hostname3 = Cvar_Get("host3","", CVAR_ARCHIVE);
	hostname4 = Cvar_Get("host4","", CVAR_ARCHIVE);
	hostname5 = Cvar_Get("host5","", CVAR_ARCHIVE);
	hostname6 = Cvar_Get("host6","", CVAR_ARCHIVE);
	hostname7 = Cvar_Get("host7","", CVAR_ARCHIVE);
	hostname8 = Cvar_Get("host8","", CVAR_ARCHIVE);
	hostname9 = Cvar_Get("host9","", CVAR_ARCHIVE);
	hostname10 = Cvar_Get("host10","", CVAR_ARCHIVE);

	M_BuildBigCharWidth();
}


void M_Draw (void)
{
	if (m_state == m_none || !(in_keyCatchers & KEYCATCH_UI))
		return;

	if (!m_recursiveDraw)
	{
		if (scr_con_current)
		{
			Draw_ConsoleBackground (viddef.height);
			S_ExtraUpdate ();
		}
		else
			Draw_FadeScreen ();
	}
	else
	{
		m_recursiveDraw = false;
	}

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_class:
		M_Class_Draw ();
		break;

	case m_load:
//		M_Load_Draw ();
		break;

	case m_save:
//		M_Save_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_net:
//		M_Net_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_video:
		M_Video_Draw ();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_quit:
		M_Quit_Draw ();
		break;

	case m_lanconfig:
//		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
//		M_GameOptions_Draw ();
		break;

	case m_search:
//		M_Search_Draw ();
		break;

	case m_slist:
//		M_ServerList_Draw ();
		break;

	case m_mconnect:
		M_Connect_Draw ();
		break;
	}

	if (m_entersound)
	{
		S_StartLocalSound("raven/menu2.wav");
		m_entersound = false;
	}

	S_ExtraUpdate ();
}


void M_Keydown (int key)
{
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_class:
		M_Class_Key (key);
		return;

	case m_load:
//		M_Load_Key (key);
		return;

	case m_save:
//		M_Save_Key (key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_net:
//		M_Net_Key (key);
		return;

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;

	case m_video:
		M_Video_Key (key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_quit:
		M_Quit_Key (key);
		return;

	case m_lanconfig:
//		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
//		M_GameOptions_Key (key);
		return;

	case m_search:
//		M_Search_Key (key);
		break;

	case m_slist:
//		M_ServerList_Key (key);
		break;

	case m_mconnect:
		M_Connect_Key (key);
		break;
	}
}


