/*
 * $Header: /H2 Mission Pack/Menu.c 41    3/20/98 2:03p Jmonroe $
 */

#include "quakedef.h"
#include "glquake.h"

extern	float introTime;
extern	QCvar*	crosshair;
QCvar* m_oldmission;

void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);

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

menu_state_t	m_return_state;
qboolean	m_return_onerror;
char		m_return_reason [32];

static float TitlePercent = 0;
static float TitleTargetPercent = 1;
static float LogoPercent = 0;
static float LogoTargetPercent = 1;

int		setup_class;

static char *message,*message2;
static double message_time;

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)
#define NUM_DIFFLEVELS	4

void M_ConfigureNetSubsystem(void);
void M_Menu_Class_f (void);

extern qboolean introPlaying;

extern float introTime;

char *ClassNames[NUM_CLASSES] = 
{
	"Paladin",
	"Crusader",
	"Necromancer",
	"Assassin",
	"Demoness"
};

char *ClassNamesU[NUM_CLASSES] = 
{
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN",
	"DEMONESS"
};

char *DiffNames[NUM_CLASSES][4] =
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
	}
};


//=============================================================================
/* Support Routines */

/*
================
M_DrawCharacter

Draws one solid graphics character, centered, on line
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character ( cx + ((vid.width - 320)>>1), line, num);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, ((unsigned char)(*str))+256);
		str++;
		cx += 8;
	}
}

/*
================
M_DrawCharacter2

Draws one solid graphics character, centered H and V
================
*/
void M_DrawCharacter2 (int cx, int line, int num)
{
	Draw_Character ( cx + ((vid.width - 320)>>1), line + ((vid.height - 200)>>1), num);
}

void M_Print2 (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter2 (cx, cy, ((unsigned char)(*str))+256);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (unsigned char)*str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320)>>1), y, pic);
}

void M_DrawTransPic2 (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320)>>1), y + ((vid.height - 200)>>1), pic);
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x + ((vid.width - 320)>>1), y, pic);
}

void M_DrawTransPicCropped (int x, int y, qpic_t *pic)
{
	Draw_TransPicCropped (x + ((vid.width - 320)>>1), y, pic);
}

byte identityTable[256];
byte translationTable[256];
extern int color_offsets[NUM_CLASSES];
extern byte *playerTranslation;
extern int setup_class;

void M_BuildTranslationTable(int top, int bottom)
{
	int		j;
	byte	*dest, *source, *sourceA, *sourceB, *colorA, *colorB;

	for (j = 0; j < 256; j++)
		identityTable[j] = j;
	dest = translationTable;
	source = identityTable;
	Com_Memcpy(dest, source, 256);

	if (top > 10) top = 0;
	if (bottom > 10) bottom = 0;

	top -= 1;
	bottom -= 1;

	colorA = playerTranslation + 256 + color_offsets[(int)setup_class-1];
	colorB = colorA + 256;
	sourceA = colorB + 256 + (top * 256);
	sourceB = colorB + 256 + (bottom * 256);
	for(j=0;j<256;j++,colorA++,colorB++,sourceA++,sourceB++)
	{
		if (top >= 0 && (*colorA != 255)) 
			dest[j] = source[*sourceA];
		if (bottom >= 0 && (*colorB != 255)) 
			dest[j] = source[*sourceB];
	}

}


void M_DrawTransPicTranslate (int x, int y, qpic_t *pic)
{
	Draw_TransPicTranslate (x + ((vid.width - 320)>>1), y, pic, translationTable);
}


void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p,*tm,*bm;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	tm = Draw_CachePic ("gfx/box_tm.lmp");
	bm = Draw_CachePic ("gfx/box_bm.lmp");
	while (width > 0)
	{
		cy = y;
		M_DrawTransPic (cx, cy, tm);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		M_DrawTransPic (cx, cy+8, bm);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}


void M_DrawTextBox2 (int x, int y, int width, int lines, qboolean bottom)
{
	qpic_t	*p,*tm,*bm;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy, p);
	else
		M_DrawTransPic2 (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		if(bottom)
			M_DrawTransPic (cx, cy, p);
		else
			M_DrawTransPic2 (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy+8, p);
	else
		M_DrawTransPic2 (cx, cy+8, p);

	// draw middle
	cx += 8;
	tm = Draw_CachePic ("gfx/box_tm.lmp");
	bm = Draw_CachePic ("gfx/box_bm.lmp");
	while (width > 0)
	{
		cy = y;
		
		if(bottom)
			M_DrawTransPic (cx, cy, tm);
		else
			M_DrawTransPic2 (cx, cy, tm);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			if(bottom)
				M_DrawTransPic (cx, cy, p);
			else
				M_DrawTransPic2 (cx, cy, p);
		}
		if(bottom)
			M_DrawTransPic (cx, cy+8, bm);
		else
			M_DrawTransPic2 (cx, cy+8, bm);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy, p);
	else
		M_DrawTransPic2 (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		if(bottom)
			M_DrawTransPic (cx, cy, p);
		else
			M_DrawTransPic2 (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy+8, p);
	else
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
	if (in_keyCatchers & KEYCATCH_CONSOLE)
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
	qpic_t	*p;
	int ypos,xpos;
	byte			*source;
	int biggestX,adjustment;
	char After[20], Before[20];
	int numA,numB;
	FILE *FH;
	char temp[MAX_OSPATH];

	p = Draw_CachePic ("gfx/menu/bigfont.lmp");

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

void M_DrawBigString(int x, int y, char *string)
{
	int c,length;

	x += ((vid.width - 320)>>1);

	length = QStr::Length(string);
	for(c=0;c<length;c++)
	{
		x += M_DrawBigCharacter(x,y,string[c],string[c+1]);
	}
}






void ScrollTitle (char *name)
{
	float delta;
	qpic_t	*p;
	static char *LastName = "";
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

	if (QStr::ICmp(LastName,name) != 0 && TitleTargetPercent != 0) 
		TitleTargetPercent = 0;

    if (CanSwitch) 
	{
		LastName = name;
		CanSwitch = false;
		TitleTargetPercent = 1;
	}

	p = Draw_CachePic(LastName);
	finaly = ((float)p->height * TitlePercent) - p->height;
	M_DrawTransPicCropped( (320-p->width)/2, finaly , p);

	if (m_state != m_keys)
	{
		p = Draw_CachePic("gfx/menu/hplaque.lmp");
		finaly = ((float)p->height * LogoPercent) - p->height;
		M_DrawTransPicCropped(10, finaly, p);
	}		
}

//=============================================================================
/* MAIN MENU */

int	m_main_cursor;
#define	MAIN_ITEMS	5


void M_Menu_Main_f (void)
{
	if (!(in_keyCatchers & KEYCATCH_UI))
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_main;
	m_entersound = true;
}
				

void M_Main_Draw (void)
{
	int		f;
	qpic_t	*p;

	ScrollTitle("gfx/menu/title0.lmp");
//	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mainmenu.lmp") );
	M_DrawBigString (72,60+(0*20),"SINGLE PLAYER");
	M_DrawBigString (72,60+(1*20),"MULTIPLAYER");
	M_DrawBigString (72,60+(2*20),"OPTIONS");
	M_DrawBigString (72,60+(3*20),"HELP");
	M_DrawBigString (72,60+(4*20),"QUIT");


	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 54 + m_main_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}


void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		cls.demonum = m_save_demonum;
		if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
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
		case 0:
			M_Menu_SinglePlayer_f ();
			break;

		case 1:
			M_Menu_MultiPlayer_f ();
			break;

		case 2:
			M_Menu_Options_f ();
			break;

		case 3:
			M_Menu_Help_f ();
			break;

		case 4:
			M_Menu_Quit_f ();
			break;
		}
	}
}

char	*plaquemessage = NULL;   // Pointer to current plaque message
char    *errormessage = NULL;

//=============================================================================
/* DIFFICULTY MENU */
void M_Menu_Difficulty_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_difficulty;
}

int	m_diff_cursor;
int m_enter_portals;
#define	DIFF_ITEMS	NUM_DIFFLEVELS

void M_Difficulty_Draw (void)
{
	int		f, i;
	qpic_t	*p;

	ScrollTitle("gfx/menu/title5.lmp");

	setup_class = cl_playerclass->value;

	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;
	setup_class--;
	
	for(i = 0; i < NUM_DIFFLEVELS; ++i)
		M_DrawBigString (72,60+(i*20),DiffNames[setup_class][i]);

	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 54 + m_diff_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}

void M_Difficulty_Key (int key)
{
	switch (key)
	{
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;
	case K_ESCAPE:
		M_Menu_Class_f ();
		break;
		
	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (++m_diff_cursor >= DIFF_ITEMS)
			m_diff_cursor = 0;
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (--m_diff_cursor < 0)
			m_diff_cursor = DIFF_ITEMS - 1;
		break;
	case K_ENTER:
		Cvar_SetValue ("skill", m_diff_cursor);
		m_entersound = true;
		if (m_enter_portals)
		{
			introTime = 0.0;
			cl.intermission = 12;
			cl.completed_time = cl.time;
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
			cls.demonum = m_save_demonum;

			//Cbuf_AddText ("map keep1\n");
		}
		else
			Cbuf_AddText ("map demo1\n");
		break;
	default:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		break;
	}
}

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
#define	CLASS_ITEMS	NUM_CLASSES

void M_Class_Draw (void)
{
	int		f, i;
	qpic_t	*p;

	ScrollTitle("gfx/menu/title2.lmp");

	for(i = 0; i < NUM_CLASSES; ++i)
		M_DrawBigString (72,60+(i*20),ClassNamesU[i]);

	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 54 + m_class_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );

	M_DrawPic (251,54 + 21, Draw_CachePic (va("gfx/cport%d.lmp", m_class_cursor + 1)));
	M_DrawTransPic (242,54, Draw_CachePic ("gfx/menu/frame.lmp"));
}

void M_Class_Key (int key)
{
	switch (key)
	{
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;
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
		if (!class_flag)
		{		
			M_Menu_Difficulty_f();
		}
		else
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
/* SINGLE PLAYER MENU */

int	m_singleplayer_cursor;
#define	SINGLEPLAYER_ITEMS	5


void M_Menu_SinglePlayer_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_singleplayer;
	m_entersound = true;
	Cvar_SetValue ("timelimit", 0);		//put this here to help play single after dm
}
				
void M_SinglePlayer_Draw (void)
{
	int		f;
	qpic_t	*p;

	ScrollTitle("gfx/menu/title1.lmp");
	
	M_DrawBigString (72,60+(0*20),"NEW MISSION");
	M_DrawBigString (72,60+(1*20),"LOAD");
	M_DrawBigString (72,60+(2*20),"SAVE");
	if (m_oldmission->value)
		M_DrawBigString (72,60+(3*20),"OLD MISSION");
	M_DrawBigString (72,60+(4*20),"VIEW INTRO");
	
	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 54 + m_singleplayer_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}


void M_SinglePlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
		
	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		if (!m_oldmission->value)
		{
			if (m_singleplayer_cursor ==3)
				m_singleplayer_cursor =4;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		if (!m_oldmission->value)
		{
			if (m_singleplayer_cursor ==3)
				m_singleplayer_cursor =2;
		}		break;

	case K_ENTER:
		m_entersound = true;

		m_enter_portals = 0;
		switch (m_singleplayer_cursor)
		{
		case 0:
			m_enter_portals = 1;
			
		case 3:
			if (sv.active)
				if (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n"))
					break;
			in_keyCatchers &= ~KEYCATCH_UI;
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			CL_RemoveGIPFiles(NULL);
			Cbuf_AddText ("maxplayers 1\n");
			M_Menu_Class_f ();
			break;

		case 1:
			M_Menu_Load_f ();
			break;

		case 2:
			M_Menu_Save_f ();
			break;

		case 4:
			in_keyCatchers &= ~KEYCATCH_UI;
			Cbuf_AddText("playdemo t9\n");
			break;
		}
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		12
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (void)
{
	int				i, j;
	char			name[MAX_OSPATH];
	fileHandle_t	f;
	int				version;

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		QStr::Cpy(m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "s%i/info.dat", i);
		if (!FS_FileExists(name))
			continue;
		FS_FOpenFileRead(name, &f, true);
		if (!f)
			continue;
		FS_Read(name, 80, f);
		name[80] = 0;
		//	Skip version
		char* Ptr = name;
		while (*Ptr && *Ptr != '\n')
			Ptr++;
		if (*Ptr == '\n')
		{
			*Ptr = 0;
			Ptr++;
		}
		char* SaveName = Ptr;
		while (*Ptr && *Ptr != '\n')
			Ptr++;
		*Ptr = 0;
		QStr::NCpy(m_filenames[i], SaveName, sizeof(m_filenames[i])-1);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		FS_FCloseFile(f);
	}
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	m_state = m_load;
	in_keyCatchers |= KEYCATCH_UI;
	M_ScanSaves ();
}
				

void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
	if (cl.intermission)
		return;
	if (svs.maxclients != 1)
		return;
	m_entersound = true;
	m_state = m_save;
	in_keyCatchers |= KEYCATCH_UI;
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int		i;
	
	ScrollTitle("gfx/menu/load.lmp");
	
	for (i=0 ; i< MAX_SAVEGAMES; i++)
		M_Print (16, 60 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 60 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Save_Draw (void)
{
	int		i;

	ScrollTitle("gfx/menu/save.lmp");

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
		M_Print (16, 60 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 60 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Load_Key (int k)
{	
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		S_StartLocalSound("raven/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;

	// Host_Loadgame_f can't bring up the loading plaque because too much
	// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();

	// issue the load command
		Cbuf_AddText (va ("load s%i\n", load_cursor) );
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;
		Cbuf_AddText (va("save s%i\n", load_cursor));
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;	
	}
}








//=============================================================================
/* MULTIPLAYER LOAD/SAVE MENU */

void M_ScanMSaves (void)
{
	int		i, j;
	char	name[MAX_OSPATH];
	fileHandle_t	f;
	int		version;

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		QStr::Cpy(m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "ms%i/info.dat", i);
		if (!FS_FileExists(name))
			continue;
		FS_FOpenFileRead(name, &f, true);
		if (!f)
			continue;
		FS_Read(name, 80, f);
		name[80] = 0;
		//	Skip version
		char* Ptr = name;
		while (*Ptr && *Ptr != '\n')
			Ptr++;
		if (*Ptr == '\n')
		{
			*Ptr = 0;
			Ptr++;
		}
		char* SaveName = Ptr;
		while (*Ptr && *Ptr != '\n')
			Ptr++;
		*Ptr = 0;
		QStr::NCpy(m_filenames[i], SaveName, sizeof(m_filenames[i])-1);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		FS_FCloseFile(f);
	}
}

void M_Menu_MLoad_f (void)
{
	m_entersound = true;
	m_state = m_mload;
	in_keyCatchers |= KEYCATCH_UI;
	M_ScanMSaves ();
}
				

void M_Menu_MSave_f (void)
{
	if (!sv.active || cl.intermission || svs.maxclients == 1)
	{
		message = "Only a network server";
		message2 = "can save a multiplayer game";
		message_time = host_time;
		return;
	}
	m_entersound = true;
	m_state = m_msave;
	in_keyCatchers |= KEYCATCH_UI;
	M_ScanMSaves ();
}


void M_MLoad_Key (int k)
{	
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_ENTER:
		S_StartLocalSound("raven/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;

		if (sv.active)
			Cbuf_AddText ("disconnect\n");
		Cbuf_AddText ("listen 1\n");	// so host_netport will be re-examined

	// Host_Loadgame_f can't bring up the loading plaque because too much
	// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();

	// issue the load command
		Cbuf_AddText (va ("load ms%i\n", load_cursor) );
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_MSave_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;
		Cbuf_AddText (va("save ms%i\n", load_cursor));
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;	
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	5

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
	qpic_t	*p;

	ScrollTitle("gfx/menu/title4.lmp");
//	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mp_menu.lmp") );

	M_DrawBigString (72,60+(0*20),"JOIN A GAME");
	M_DrawBigString (72,60+(1*20),"NEW GAME");
	M_DrawBigString (72,60+(2*20),"SETUP");
	M_DrawBigString (72,60+(3*20),"LOAD");
	M_DrawBigString (72,60+(4*20),"SAVE");

	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 54 + m_multiplayer_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );

	if (message)
	{
		M_PrintWhite ((320/2) - ((27*8)/2), 168, message);
		M_PrintWhite ((320/2) - ((27*8)/2), 176, message2);
		if (host_time - 5 > message_time)
			message = NULL;
	}

	if (tcpipAvailable)
		return;
	M_PrintWhite ((320/2) - ((27*8)/2), 160, "No Communications Available");
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
			if (tcpipAvailable)
				M_Menu_Net_f ();
			break;

		case 1:
			if (tcpipAvailable)
				M_Menu_Net_f ();
			break;

		case 2:
			M_Menu_Setup_f ();
			break;

		case 3:
			M_Menu_MLoad_f ();
			break;

		case 4:
			M_Menu_MSave_f ();
			break;
		}
	}
}

//=============================================================================
/* SETUP MENU */

int		setup_cursor = 5;
int		setup_cursor_table[] = {40, 56, 80, 104, 128, 156};

char	setup_hostname[16];
char	setup_myname[16];
int		setup_oldtop;
int		setup_oldbottom;
int		setup_top;
int		setup_bottom;

#define	NUM_SETUP_CMDS	6

void M_Menu_Setup_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_setup;
	m_entersound = true;
	QStr::Cpy(setup_myname, cl_name->string);
	QStr::Cpy(setup_hostname, hostname->string);
	setup_top = setup_oldtop = ((int)cl_color->value) >> 4;
	setup_bottom = setup_oldbottom = ((int)cl_color->value) & 15;
	setup_class = cl_playerclass->value;
	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;
}
				

void M_Setup_Draw (void)
{
	qpic_t	*p;

	ScrollTitle("gfx/menu/title4.lmp");
	
	M_Print (64, 40, "Hostname");
	M_DrawTextBox (160, 32, 16, 1);
	M_Print (168, 40, setup_hostname);

	M_Print (64, 56, "Your name");
	M_DrawTextBox (160, 48, 16, 1);
	M_Print (168, 56, setup_myname);

	M_Print (64, 80, "Current Class: ");
	M_Print (88, 88, ClassNames[setup_class-1]);

	M_Print (64, 104, "First color patch");
	M_Print (64, 128, "Second color patch");
	
	M_DrawTextBox (64, 156-8, 14, 1);
	M_Print (72, 156, "Accept Changes");

	p = Draw_CachePic (va("gfx/menu/netp%i.lmp",setup_class));
	M_BuildTranslationTable(setup_top, setup_bottom);

	/* garymct */
	M_DrawTransPicTranslate (220, 72, p);

	M_DrawCharacter (56, setup_cursor_table [setup_cursor], 12+((int)(realtime*4)&1));

	if (setup_cursor == 0)
		M_DrawCharacter (168 + 8*QStr::Length(setup_hostname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));

	if (setup_cursor == 1)
		M_DrawCharacter (168 + 8*QStr::Length(setup_myname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));
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
		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS-1;
		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;
		break;

	case K_LEFTARROW:
		if (setup_cursor < 2)
			return;
		S_StartLocalSound("raven/menu3.wav");
		if (setup_cursor == 2)
		{
			setup_class--;
			if (setup_class < 1) 
				setup_class = NUM_CLASSES;
		}
		if (setup_cursor == 3)
			setup_top = setup_top - 1;
		if (setup_cursor == 4)
			setup_bottom = setup_bottom - 1;
		break;
	case K_RIGHTARROW:
		if (setup_cursor < 2)
			return;
forward:
		S_StartLocalSound("raven/menu3.wav");
		if (setup_cursor == 2)
		{
			setup_class++;
			if (setup_class > NUM_CLASSES) 
				setup_class = 1;
		}
		if (setup_cursor == 3)
			setup_top = setup_top + 1;
		if (setup_cursor == 4)
			setup_bottom = setup_bottom + 1;
		break;

	case K_ENTER:
		if (setup_cursor == 0 || setup_cursor == 1)
			return;

		if (setup_cursor == 2 || setup_cursor == 3 || setup_cursor == 4)
			goto forward;

		if (QStr::Cmp(cl_name->string, setup_myname) != 0)
			Cbuf_AddText ( va ("name \"%s\"\n", setup_myname) );
		if (QStr::Cmp(hostname->string, setup_hostname) != 0)
			Cvar_Set("hostname", setup_hostname);
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddText( va ("color %i %i\n", setup_top, setup_bottom) );
		Cbuf_AddText ( va ("playerclass %d\n", setup_class) );
		m_entersound = true;
		M_Menu_MultiPlayer_f ();
		break;
	
	case K_BACKSPACE:
		if (setup_cursor == 0)
		{
			if (QStr::Length(setup_hostname))
				setup_hostname[QStr::Length(setup_hostname)-1] = 0;
		}

		if (setup_cursor == 1)
		{
			if (QStr::Length(setup_myname))
				setup_myname[QStr::Length(setup_myname)-1] = 0;
		}
		break;
		
	default:
		if (k < 32 || k > 127)
			break;
		if (setup_cursor == 0)
		{
			l = QStr::Length(setup_hostname);
			if (l < 15)
			{
				setup_hostname[l+1] = 0;
				setup_hostname[l] = k;
			}
		}
		if (setup_cursor == 1)
		{
			l = QStr::Length(setup_myname);
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
/* NET MENU */

int	m_net_cursor = 0;
int m_net_items;
int m_net_saveHeight;

char *net_helpMessage [] = 
{
/* .........1.........2.... */
  "                        ",
  " Two computers connected",
  "   through two modems.  ",
  "                        ",
 
  "                        ",
  " Two computers connected",
  " by a null-modem cable. ",
  "                        ",

  " Novell network LANs    ",
  " or Windows 95 DOS-box. ",
  "                        ",
  "(LAN=Local Area Network)",

  " Commonly used to play  ",
  " over the Internet, but ",
  " also used on a Local   ",
  " Area Network.          "
};

void M_Menu_Net_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_net;
	m_entersound = true;
	m_net_items = 4;

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;
	m_net_cursor--;
	M_Net_Key (K_DOWNARROW);
}


void M_Net_Draw (void)
{
	int		f;
	qpic_t	*p;

	ScrollTitle("gfx/menu/title4.lmp");

	f = 89;
/* rjr
	if (tcpipAvailable)
		p = Draw_CachePic ("gfx/netmen4.lmp");
	else
		p = Draw_CachePic ("gfx/dim_tcp.lmp");
*/
//	M_DrawTransPic (72, f, p);
	M_DrawBigString (72,f,"TCP/IP");

	f = (320-26*8)/2;
	M_DrawTextBox (f, 134, 24, 4);
	f += 8;
	M_Print (f, 142, net_helpMessage[m_net_cursor*4+0]);
	M_Print (f, 150, net_helpMessage[m_net_cursor*4+1]);
	M_Print (f, 158, net_helpMessage[m_net_cursor*4+2]);
	M_Print (f, 166, net_helpMessage[m_net_cursor*4+3]);

	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 24 + m_net_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}

void M_Net_Key (int k)
{
again:
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;
		
	case K_DOWNARROW:
// Tries to re-draw the menu here, and m_net_cursor could be set to -1
//		S_StartLocalSound("raven/menu1.wav");
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;
		break;

	case K_UPARROW:
//		S_StartLocalSound("raven/menu1.wav");
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_net_cursor)
		{
		case 2:
			M_Menu_LanConfig_f ();
			break;

		case 3:
			M_Menu_LanConfig_f ();
			break;

		case 4:
// multiprotocol
			break;
		}
	}

	if (m_net_cursor == 0)
		goto again;
	if (m_net_cursor == 1)
		goto again;
	if (m_net_cursor == 2)
		goto again;
	if (m_net_cursor == 3 && !tcpipAvailable)
		goto again;

	switch (k)
	{
		case K_DOWNARROW:
		case K_UPARROW:
			S_StartLocalSound("raven/menu1.wav");
			break;
	}
}

//=============================================================================
/* OPTIONS MENU */

#define	SLIDER_RANGE	10

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
		vid.recalc_refdef = 1;
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
		if (QStr::ICmp(bgmtype->string,"midi") == 0)
		{
			if (dir < 0)
				Cvar_Set("bgmtype","none");
			else
				Cvar_Set("bgmtype","cd");
		}
		else if (QStr::ICmp(bgmtype->string,"cd") == 0)
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
		Cvar_SetValue ("volume", s_volume->value);
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
		if (in_mlook.state & 1)
			//IN_MLookUp();
			Cbuf_AddText("-mlook");
		else
			//IN_MLookDown();
			Cbuf_AddText("+mlook");
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
	qpic_t	*p;
	
	ScrollTitle("gfx/menu/title3.lmp");
	
	M_Print (16, 60+(0*8), "    Customize controls");
	M_Print (16, 60+(1*8), "         Go to console");
	M_Print (16, 60+(2*8), "     Reset to defaults");

	M_Print (16, 60+(3*8), "           Screen size");
	r = (scr_viewsize->value - 30) / (120 - 30);
	M_DrawSlider (220, 60+(3*8), r);

	M_Print (16, 60+(5*8), "           Mouse Speed");
	r = (sensitivity->value - 1)/10;
	M_DrawSlider (220, 60+(5*8), r);

	M_Print (16, 60+(6*8), "            Music Type");
	if (QStr::ICmp(bgmtype->string,"midi") == 0)
		M_Print (220, 60+(6*8), "MIDI");
	else if (QStr::ICmp(bgmtype->string,"cd") == 0)
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
	M_DrawCheckbox (220, 60+(OPT_ALWAYSMLOOK*8), in_mlook.state & 1);

	if (vid_menudrawfn)
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
			m_state = m_none;
			Con_ToggleConsole_f ();
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

		if ((options_cursor == OPT_GAMMA)) options_cursor--;

		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		options_cursor++;
		if (options_cursor >= OPTIONS_ITEMS)
			options_cursor = 0;

		if ((options_cursor == OPT_GAMMA)) options_cursor++;

		break;	

	case K_LEFTARROW:
		M_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;
	}

	if (options_cursor == 12 && vid_menudrawfn == NULL)
		if (k == K_UPARROW)
			options_cursor = 11;
		else
			options_cursor = 0;
}

//=============================================================================
/* KEYS MENU */

char *bindnames[][2] =
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
{"impulse 13", 		"lift object"},
{"invuse",			"use inv item"},
{"impulse 44",		"drop inv item"},
{"+showinfo",		"full inventory"},
{"+showdm",			"info / frags"},
{"toggle_dm",		"toggle frags"},
{"+infoplaque",		"objectives"},
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


void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l,l2;
	char	*b;

	twokeys[0] = twokeys[1] = -1; 
	l = QStr::Length(command); 
	count = 0;
	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j]; 
		if (!b)
			continue; 
		if (!QStr::NCmp(b, command, l))
		{
			l2= QStr::Length(b); 
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

void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = QStr::Length(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!QStr::NCmp(b, command, l) )
			Key_SetBinding (j, "");
	}
}


void M_Keys_Draw (void)
{
	int		i, l;
	int		keys[2];
	char	*name;
	int		x, y;
	qpic_t	*p;

	ScrollTitle("gfx/menu/title6.lmp");

//	M_DrawTextBox (6,56, 35,16);

//	p = Draw_CachePic("gfx/menu/hback.lmp");
//	M_DrawTransPicCropped(8, 62, p);

	if (bind_grab)
		M_Print (12, 64, "Press a key or button for this action");
	else
		M_Print (18, 64, "Enter to change, backspace to clear");
		
	if (keys_top) 
		M_DrawCharacter (6, 80, 128);
	if (keys_top + KEYS_SIZE < NUMCOMMANDS)
		M_DrawCharacter (6, 80 + ((KEYS_SIZE-1)*8), 129);

// search for known bindings
	for (i=0 ; i<KEYS_SIZE ; i++)
	{
		y = 80 + 8*i;

		M_Print (16, y, bindnames[i+keys_top][1]);

		l = QStr::Length(bindnames[i+keys_top][0]);
		
		M_FindKeysForCommand (bindnames[i+keys_top][0], keys);
		
		if (keys[0] == -1)
		{
			M_Print (140, y, "???");
		}
		else
		{
			name = Key_KeynumToString (keys[0]);
			M_Print (140, y, name);
			x = QStr::Length(name) * 8;
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
		if (keys_cursor >= NUMCOMMANDS)
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

void M_Menu_Video_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_video;
	m_entersound = true;
}


void M_Video_Draw (void)
{
	(*vid_menudrawfn) ();
}


void M_Video_Key (int key)
{
	(*vid_menukeyfn) (key);
}

//=============================================================================
/* HELP MENU */

int		help_page;
#define	NUM_HELP_PAGES	5


void M_Menu_Help_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}



void M_Help_Draw (void)
{
	M_DrawPic (0, 0, Draw_CachePic ( va("gfx/menu/help%02i.lmp", help_page+1)) );
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
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}

//=============================================================================
/* QUIT MENU */

//int		msgNumber;
menu_state_t	m_quit_prevstate;
qboolean	wasInMenus;

#ifndef	_WIN32
char *quitMessage [] = 
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
char **LineText;
static qboolean SoundPlayed;


#define MAX_LINES 138

char *CreditText[MAX_LINES] =
{
   "Project Director: James Monroe",
   "Creative Director: Brian Raffel",
   "Project Coordinator: Kevin Schilder",
   "",
   "Lead Programmer: James Monroe",
   "",
   "Programming:",
   "   Mike Gummelt",
   "   Josh Weier",
   "",
   "Additional Programming:",
   "   Josh Heitzman",
   "   Nathan Albury",
   "   Rick Johnson",
   "",
   "Assembly Consultant:",
   "   Mr. John Scott",
   "",
   "Lead Design: Jon Zuk",
   "",
   "Design:",
   "   Tom Odell",
   "   Jeremy Statz",
   "   Mike Renner",
   "   Eric Biessman",
   "   Kenn Hoekstra",
   "   Matt Pinkston",
   "   Bobby Duncanson",
   "   Brian Raffel",
   "",
   "Art Director: Les Dorscheid",
   "",
   "Art:",
   "   Kim Lathrop",
   "   Gina Garren",
   "   Joe Koberstein",
   "   Kevin Long",
   "   Jeff Butler",
   "   Scott Rice",
   "   John Payne",
   "   Steve Raffel",
   "",
   "Animation:",
   "   Eric Turman",
   "   Chaos (Mike Werckle)",
   "",
   "Music:",
   "   Kevin Schilder",
   "",
   "Sound:",
   "   Chia Chin Lee",
   "",
   "Activision",
   "",
   "Producer:",
   "   Steve Stringer",
   "",
   "Marketing Product Manager:",
   "   Henk Hartong",
   "",
   "Marketing Associate:",
   "   Kevin Kraff",
   "",
   "Senior Quality",
   "Assurance Lead:",
   "   Tim Vanlaw",
   "",
   "Quality Assurance Lead:",
   "   Doug Jacobs",
   "",
   "Quality Assurance Team:",
   "   Steve Rosenthal, Steve Elwell,",
   "   Chad Bordwell, David Baker,",
   "   Aaron Casillas, Damien Fischer,",
   "   Winnie Lee, Igor Krinitskiy,",
   "   Samantha Lee, John Park",
   "   Ian Stevens, Chris Toft",
   "",
   "Production Testers:",
   "   Steve Rosenthal and",
   "   Chad Bordwell",
   "",
   "Additional QA and Support:",
   "    Tony Villalobos",
   "    Jason Sullivan",
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
   "Our Big Toe:",
   "   Mitch Lasky",
   "",
   "",
   "Special Thanks to:",
   "  Id software",
   "  The original Hexen2 crew",
   "   We couldn't have done it",
   "   without you guys!",
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
   "",
   "No yaks were harmed in the",
   "making of this game!"
};

#define MAX_LINES2 150

char *Credit2Text[MAX_LINES2] =
{
   "PowerTrip: James (emorog) Monroe",
   "Cartoons: Brian Raffel",
   "         (use more puzzles)",
   "Doc Keeper: Kevin Schilder",
   "",
   "Whip cracker: James Monroe",
   "",
   "Whipees:",
   "   Mike (i didn't break it) Gummelt",
   "   Josh (extern) Weier",
   "",
   "We don't deserve whipping:",
   "   Josh (I'm not on this project)",
   "         Heitzman",
   "   Nathan (deer hunter) Albury",
   "   Rick (model crusher) Johnson",
   "",
   "Bit Packer:",
   "   Mr. John (Slaine) Scott",
   "",
   "Lead Slacker: Jon (devil boy) Zuk",
   "",
   "Other Slackers:",
   "   Tom (can i have an office) Odell",
   "   Jeremy (nt crashed again) Statz",
   "   Mike (i should be doing my ",
   "         homework) Renner",
   "   Eric (the nose) Biessman",
   "   Kenn (.plan) Hoekstra",
   "   Matt (big elbow) Pinkston",
   "   Bobby (needs haircut) Duncanson",
   "   Brian (they're in my town) Raffel",
   "",
   "Use the mouse: Les Dorscheid",
   "",
   "What's a mouse?:",
   "   Kim (where's my desk) Lathrop",
   "   Gina (i can do your laundry)",
   "        Garren",
   "   Joe (broken axle) Koberstein",
   "   Kevin (titanic) Long",
   "   Jeff (norbert) Butler",
   "   Scott (what's the DEL key for?)",
   "          Rice",
   "   John (Shpluuurt!) Payne",
   "   Steve (crash) Raffel",
   "",
   "Boners:",
   "   Eric (terminator) Turman",
   "   Chaos Device",
   "",
   "Drum beater:",
   "   Kevin Schilder",
   "",
   "Whistle blower:",
   "   Chia Chin (bruce) Lee",
   "",
   "",
   "Activision",
   "",
   "Producer:",
   "   Steve 'Ferris' Stringer",
   "",
   "Marketing Product Manager:",
   "   Henk 'GODMODE' Hartong",
   "",
   "Marketing Associate:",
   "   Kevin 'Kraffinator' Kraff",
   "",
   "Senior Quality",
   "Assurance Lead:",
   "   Tim 'Outlaw' Vanlaw",
   "",
   "Quality Assurance Lead:",
   "   Doug Jacobs",
   "",
   "Shadow Finders:",
   "   Steve Rosenthal, Steve Elwell,",
   "   Chad Bordwell,",
   "   David 'Spice Girl' Baker,",
   "   Error Casillas, Damien Fischer,",
   "   Winnie Lee,"
   "   Ygor Krynytyskyy,",
   "   Samantha (Crusher) Lee, John Park",
   "   Ian Stevens, Chris Toft",
   "",
   "Production Testers:",
   "   Steve 'Damn It's Cold!'",
   "       Rosenthal and",
   "   Chad 'What Hotel Receipt?'",
   "        Bordwell",
   "",
   "Additional QA and Support:",
   "    Tony Villalobos",
   "    Jason Sullivan",
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
   "Our Big Toe:",
   "   Mitch Lasky",
   "",
   "",
   "Special Thanks to:",
   "  Id software",
   "  Anyone who ever worked for Raven,",
   "  (except for Alex)",
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
   "Special Thanks To:",
   "   E.H.S., The Osmonds,",
   "   B.B.V.D., Daisy The Lovin' Lamb,",
   "  'You Killed' Kenny,",
   "   and Baby Biessman.",
   "",
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
//	msgNumber = rand()&7;

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
		Host_Quit_f ();
		break;

	default:
		break;
	}

}

void M_Quit_Draw (void)
{
	int i,x,y,place,topy;
	qpic_t	*p;

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
			CDAudio_Play (12, false);
		}
	}

	y = 12;
	M_DrawTextBox (0, 0, 38, 23);
	M_PrintWhite (16, y,  "        Hexen II version 1.12       ");	y += 8;
	M_PrintWhite (16, y,  "         by Raven Software          ");	y += 16;

	if (LinePos > 55 && !SoundPlayed && LineText == Credit2Text)
	{
		S_StartLocalSound("rj/steve.wav");
		SoundPlayed = true;
	}
	topy = y;
	place = floor(LinePos);
	y -= floor((LinePos - place) * 8);
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

	p = Draw_CachePic ("gfx/box_mm2.lmp");
	x = 24;
	y = topy-8;
	for(i=4;i<38;i++,x+=8)
	{
		M_DrawPic(x, y, p);	//background at top for smooth scroll out
		M_DrawPic(x, y+(QUIT_SIZE*8), p);	//draw at bottom for smooth scroll in
	}

	y += (QUIT_SIZE*8)+8;
	M_PrintWhite (16, y,  "          Press y to exit           ");
}

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
int		lanConfig_cursor_table [] = {100, 120, 140, 172};
#define NUM_LANCONFIG_CMDS	4

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[30];

void M_Menu_LanConfig_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}
	if (StartingGame && lanConfig_cursor >= 2)
		lanConfig_cursor = 1;
	lanConfig_port = DEFAULTnet_hostport;
	sprintf(lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;

	setup_class = cl_playerclass->value;
	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;
	setup_class--;
}
				

void M_LanConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	char	*startJoin;
	char	*protocol;

	ScrollTitle("gfx/menu/title4.lmp");
	basex = 48;

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	protocol = "TCP/IP";
	M_Print (basex, 60, va ("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print (basex, 80, "Address:");
	M_Print (basex+9*8, 80, my_tcpip_address);

	M_Print (basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, lanConfig_cursor_table[0]-8, 6, 1);
	M_Print (basex+9*8, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print (basex, lanConfig_cursor_table[1], "Class:");
		M_Print (basex+8*7, lanConfig_cursor_table[1], ClassNames[setup_class]);

		M_Print (basex, lanConfig_cursor_table[2], "Search for local games...");
		M_Print (basex, 156, "Join game at:");
		M_DrawTextBox (basex, lanConfig_cursor_table[3]-8, 30, 1);
		M_Print (basex+8, lanConfig_cursor_table[3], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1]-8, 2, 1);
		M_Print (basex+8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex-8, lanConfig_cursor_table [lanConfig_cursor], 12+((int)(realtime*4)&1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex+9*8 + 8*QStr::Length(lanConfig_portname), lanConfig_cursor_table [0], 10+((int)(realtime*4)&1));

	if (lanConfig_cursor == 3)
		M_DrawCharacter (basex+8 + 8*QStr::Length(lanConfig_joinname), lanConfig_cursor_table [3], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 192, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		lanConfig_cursor--;

		if (JoiningGame)
		{
			if (lanConfig_cursor < 0)
				lanConfig_cursor = NUM_LANCONFIG_CMDS-1;
		}
		else
		{
			if (lanConfig_cursor < 0)
				lanConfig_cursor = NUM_LANCONFIG_CMDS-2;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;

	case K_ENTER:
		if ((JoiningGame && lanConfig_cursor <= 1) ||
			(!JoiningGame && lanConfig_cursor == 0))
			break;

		m_entersound = true;
		if (JoiningGame)
			Cbuf_AddText ( va ("playerclass %d\n", setup_class+1) );

		M_ConfigureNetSubsystem ();

		if ((JoiningGame && lanConfig_cursor == 2) ||
			(!JoiningGame && lanConfig_cursor == 1))
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}
			M_Menu_Search_f();
			break;
		}

		if (lanConfig_cursor == 3)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
			Cbuf_AddText ( va ("connect \"%s\"\n", lanConfig_joinname) );
			break;
		}

		break;
	
	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (QStr::Length(lanConfig_portname))
				lanConfig_portname[QStr::Length(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 3)
		{
			if (QStr::Length(lanConfig_joinname))
				lanConfig_joinname[QStr::Length(lanConfig_joinname)-1] = 0;
		}
		break;
		
	case K_LEFTARROW:
		if (lanConfig_cursor != 1 || !JoiningGame)
			break;

		S_StartLocalSound("raven/menu3.wav");
		setup_class--;
		if (setup_class < 0) 
			setup_class = NUM_CLASSES -1;
		break;

	case K_RIGHTARROW:
		if (lanConfig_cursor != 1 || !JoiningGame)
			break;

		S_StartLocalSound("raven/menu3.wav");
		setup_class++;
		if (setup_class > NUM_CLASSES - 1) 
			setup_class = 0;
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (lanConfig_cursor == 3)
		{
			l = QStr::Length(lanConfig_joinname);
			if (l < 29)
			{
				lanConfig_joinname[l+1] = 0;
				lanConfig_joinname[l] = key;
			}
		}

		if (key < '0' || key > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			l = QStr::Length(lanConfig_portname);
			if (l < 5)
			{
				lanConfig_portname[l+1] = 0;
				lanConfig_portname[l] = key;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;

	l =  QStr::Atoi(lanConfig_portname);
	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;
	sprintf(lanConfig_portname, "%u", lanConfig_port);
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	char	*name;
	char	*description;
} level_t;

level_t		levels[] =
{
	{"demo1", "Blackmarsh"},							// 0
	{"demo2", "Barbican"},								// 1

	{"ravdm1", "Deathmatch 1"},							// 2

	{"demo1","Blackmarsh"},								// 3
	{"demo2","Barbican"},								// 4
	{"demo3","The Mill"},								// 5
	{"village1","King's Court"},						// 6
	{"village2","Inner Courtyard"},						// 7
	{"village3","Stables"},								// 8
	{"village4","Palace Entrance"},						// 9
	{"village5","The Forgotten Chapel"},				// 10
	{"rider1a","Famine's Domain"},						// 11

	{"meso2","Plaza of the Sun"},						// 12
	{"meso1","The Palace of Columns"},					// 13
	{"meso3","Square of the Stream"},					// 14
	{"meso4","Tomb of the High Priest"},				// 15
	{"meso5","Obelisk of the Moon"},					// 16
	{"meso6","Court of 1000 Warriors"},					// 17
	{"meso8","Bridge of Stars"},						// 18
	{"meso9","Well of Souls"},							// 19

	{"egypt1","Temple of Horus"},						// 20
	{"egypt2","Ancient Temple of Nefertum"},			// 21
	{"egypt3","Temple of Nefertum"},					// 22
	{"egypt4","Palace of the Pharaoh"},					// 23
	{"egypt5","Pyramid of Anubis"},						// 24
	{"egypt6","Temple of Light"},						// 25
	{"egypt7","Shrine of Naos"},						// 26
	{"rider2c","Pestilence's Lair"},					// 27

	{"romeric1","The Hall of Heroes"},					// 28
	{"romeric2","Gardens of Athena"},					// 29
	{"romeric3","Forum of Zeus"},						// 30
	{"romeric4","Baths of Demetrius"},					// 31
	{"romeric5","Temple of Mars"},						// 32
	{"romeric6","Coliseum of War"},						// 33
	{"romeric7","Reflecting Pool"},						// 34

	{"cath","Cathedral"},								// 35
	{"tower","Tower of the Dark Mage"},					// 36
	{"castle4","The Underhalls"},						// 37
	{"castle5","Eidolon's Ordeal"},						// 38
	{"eidolon","Eidolon's Lair"},						// 39
  
	{"ravdm1","Atrium of Immolation"},					// 40
	{"ravdm2","Total Carnage"},							// 41
	{"ravdm3","Reckless Abandon"},						// 42
	{"ravdm4","Temple of RA"},							// 43
	{"ravdm5","Tom Foolery"},							// 44

	{"ravdm1", "Deathmatch 1"},							// 45

//OEM
	{"demo1","Blackmarsh"},								// 46
	{"demo2","Barbican"},								// 47
	{"demo3","The Mill"},								// 48
	{"village1","King's Court"},						// 49
	{"village2","Inner Courtyard"},						// 50
	{"village3","Stables"},								// 51
	{"village4","Palace Entrance"},						// 52
	{"village5","The Forgotten Chapel"},				// 53
	{"rider1a","Famine's Domain"},						// 54

//Mission Pack
	{"keep1",	"Eidolon's Lair"},						// 55
	{"keep2",	"Village of Turnabel"},					// 56
	{"keep3",	"Duke's Keep"},							// 57
	{"keep4",	"The Catacombs"},						// 58
	{"keep5",	"Hall of the Dead"},					// 59

	{"tibet1",	"Tulku"},								// 60
	{"tibet2",	"Ice Caverns"},							// 61
	{"tibet3",	"The False Temple"},					// 62
	{"tibet4",	"Courtyards of Tsok"},					// 63
	{"tibet5",	"Temple of Kalachakra"},				// 64
	{"tibet6",	"Temple of Bardo"},						// 65
	{"tibet7",	"Temple of Phurbu"},					// 66
	{"tibet8",	"Palace of Emperor Egg Chen"},			// 67
	{"tibet9",	"Palace Inner Chambers"},				// 68
	{"tibet10",	"The Inner Sanctum of Praevus"},		// 69
};

typedef struct
{
	char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

episode_t	episodes[] =
{
	// Demo
	{"Demo", 0, 2},
	{"Demo Deathmatch", 2, 1},

	// Registered
	{"Village", 3, 9},
	{"Meso", 12, 8},
	{"Egypt", 20, 8},
	{"Romeric", 28, 7},
	{"Cathedral", 35, 5},
	{"MISSION PACK", 55, 15},
	{"Deathmatch", 40, 5},

	// OEM
	{"Village", 46, 9},
	{"Deathmatch", 45, 1},
};

#define OEM_START 9
#define REG_START 2
#define MP_START 7
#define DM_START 8

int	startepisode;
int	startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 104, 112, 128, 136};
#define	NUM_GAMEOPTIONS	11
int		gameoptions_cursor;

void M_Menu_GameOptions_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients;
	if (maxplayers < 2)
		maxplayers = svs.maxclientslimit;

	setup_class = cl_playerclass->value;
	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;
	setup_class--;

	if (registered->value && (startepisode < REG_START || startepisode >= OEM_START))
		startepisode = REG_START;

	if (coop->value)
	{
		startlevel = 0;
		if (startepisode == 1)
			startepisode = 0;
		else if (startepisode == DM_START)
			startepisode = REG_START;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS-1)
			gameoptions_cursor = 0;
	}
	if (!m_oldmission->value)
	{
		startepisode = MP_START;
	}
}

void M_GameOptions_Draw (void)
{
	qpic_t	*p;
	int		x;

	ScrollTitle("gfx/menu/title4.lmp");

	M_DrawTextBox (152+8, 60, 10, 1);
	M_Print (160+8, 68, "begin game");
	
	M_Print (0+8, 84, "      Max players");
	M_Print (160+8, 84, va("%i", maxplayers) );

	M_Print (0+8, 92, "        Game Type");
	if (coop->value)
		M_Print (160+8, 92, "Cooperative");
	else
		M_Print (160+8, 92, "Deathmatch");

	M_Print (0+8, 100, "        Teamplay");
/*	if (rogue)
	{
		char *msg;

		switch((int)teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			case 3: msg = "Tag"; break;
			case 4: msg = "Capture the Flag"; break;
			case 5: msg = "One Flag CTF"; break;
			case 6: msg = "Three Team CTF"; break;
			default: msg = "Off"; break;
		}
		M_Print (160+8, 100, msg);
	}
	else
*/	{
		char *msg;

		switch((int)teamplay->value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			default: msg = "Off"; break;
		}
		M_Print (160+8, 100, msg);
	}

	M_Print (0+8, 108, "            Class");
	M_Print (160+8, 108, ClassNames[setup_class]);

	M_Print (0+8, 116, "       Difficulty");

	M_Print (160+8, 116, DiffNames[setup_class][(int)skill->value]);

	M_Print (0+8, 124, "       Frag Limit");
	if (fraglimit->value == 0)
		M_Print (160+8, 124, "none");
	else
		M_Print (160+8, 124, va("%i frags", (int)fraglimit->value));

	M_Print (0+8, 132, "       Time Limit");
	if (timelimit->value == 0)
		M_Print (160+8, 132, "none");
	else
		M_Print (160+8, 132, va("%i minutes", (int)timelimit->value));

	M_Print (0+8, 140, "     Random Class");
	if (randomclass->value)
		M_Print (160+8, 140, "on");
	else
		M_Print (160+8, 140, "off");

	M_Print (0+8, 156, "         Episode");
	M_Print (160+8, 156, episodes[startepisode].description);

	M_Print (0+8, 164, "           Level");
	M_Print (160+8, 164, levels[episodes[startepisode].firstLevel + startlevel].name);
	M_Print (96, 180, levels[episodes[startepisode].firstLevel + startlevel].description);

// line cursor
	M_DrawCharacter (172-16, gameoptions_cursor_table[gameoptions_cursor]+28, 12+((int)(realtime*4)&1));

/*	rjr
	if (m_serverInfoMessage)
	{
		if ((realtime - m_serverInfoMessageTime) < 5.0)
		{
			x = (320-26*8)/2;
			M_DrawTextBox (x, 138, 24, 4);
			x += 8;
			M_Print (x, 146, "  More than 4 players   ");
			M_Print (x, 154, " requires using command ");
			M_Print (x, 162, "line parameters; please ");
			M_Print (x, 170, "   see techinfo.txt.    ");
		}
		else
		{
			m_serverInfoMessage = false;
		}
	}*/
}


void M_NetStart_Change (int dir)
{
	int count;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
		Cvar_SetValue ("coop", coop->value ? 0 : 1);
		if (coop->value)
		{
			startlevel = 0;
			if (startepisode == 1)
				startepisode = 0;
			else if (startepisode == DM_START)
				startepisode = REG_START;
			if (!m_oldmission->value)
			{
				startepisode = MP_START;
			}
		}
		break;

	case 3:
//		if (rogue)
//			count = 6;
//		else
			count = 2;

		Cvar_SetValue ("teamplay", teamplay->value + dir);
		if (teamplay->value > count)
			Cvar_SetValue ("teamplay", 0);
		else if (teamplay->value < 0)
			Cvar_SetValue ("teamplay", count);
		break;

	case 4:
		setup_class += dir;
		if (setup_class < 0) 
			setup_class = NUM_CLASSES - 1;
		if (setup_class > NUM_CLASSES - 1) 
			setup_class = 0;
		break;

	case 5:
		Cvar_SetValue ("skill", skill->value + dir);
		if (skill->value > 3)
			Cvar_SetValue ("skill", 0);
		if (skill->value < 0)
			Cvar_SetValue ("skill", 3);
		break;

	case 6:
		Cvar_SetValue ("fraglimit", fraglimit->value + dir*10);
		if (fraglimit->value > 100)
			Cvar_SetValue ("fraglimit", 0);
		if (fraglimit->value < 0)
			Cvar_SetValue ("fraglimit", 100);
		break;

	case 7:
		Cvar_SetValue ("timelimit", timelimit->value + dir*5);
		if (timelimit->value > 60)
			Cvar_SetValue ("timelimit", 0);
		if (timelimit->value < 0)
			Cvar_SetValue ("timelimit", 60);
		break;

	case 8:
		if (randomclass->value)
			Cvar_SetValue ("randomclass", 0);
		else
			Cvar_SetValue ("randomclass", 1);
		break;

	case 9:
		startepisode += dir;

		if (registered->value)
		{
			count = DM_START;
			if (!coop->value)
				count++;
			else
			{
				if (!m_oldmission->value)
				{
					startepisode = MP_START;
				}
			}
			if (startepisode < REG_START)
				startepisode = count - 1;

			if (startepisode >= count)
				startepisode = REG_START;

			startlevel = 0;
		}
		else
		{
			count = 2;

			if (startepisode < 0)
				startepisode = count - 1;

			if (startepisode >= count)
				startepisode = 0;

			startlevel = 0;
		}
		break;

	case 10:
		if (coop->value)
		{
			startlevel = 0;
			break;
		}
		startlevel += dir;
		count = episodes[startepisode].levels;

		if (startlevel < 0)
			startlevel = count - 1;

		if (startlevel >= count)
			startlevel = 0;
		break;
	}
}

void M_GameOptions_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
		{
			gameoptions_cursor = NUM_GAMEOPTIONS-1;
			if (coop->value)
				gameoptions_cursor--;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		gameoptions_cursor++;
		if (coop->value)
		{
			if (gameoptions_cursor >= NUM_GAMEOPTIONS-1)
				gameoptions_cursor = 0;
		}
		else
		{
			if (gameoptions_cursor >= NUM_GAMEOPTIONS)
				gameoptions_cursor = 0;
		}
		break;

	case K_LEFTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_StartLocalSound("raven/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_StartLocalSound("raven/menu3.wav");
		M_NetStart_Change (1);
		break;

	case K_ENTER:
		S_StartLocalSound("raven/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ( va ("playerclass %d\n", setup_class+1) );
			Cbuf_AddText ("listen 0\n");	// so host_netport will be re-examined
			Cbuf_AddText ( va ("maxplayers %u\n", maxplayers) );
			SCR_BeginLoadingPlaque ();

			Cbuf_AddText ( va ("map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name) );

			return;
		}
		
		M_NetStart_Change (1);
		break;	
	}
}

//=============================================================================
/* SEARCH MENU */

qboolean	searchComplete = false;
double		searchCompleteTime;

void M_Menu_Search_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f();

}


void M_Search_Draw (void)
{
	qpic_t	*p;
	int x;

	ScrollTitle("gfx/menu/title4.lmp");

	x = (320/2) - ((12*8)/2) + 4;
	M_DrawTextBox (x-8, 60, 12, 1);
	M_Print (x, 68, "Searching...");

	if(slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (! searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f ();
		return;
	}

	M_PrintWhite ((320/2) - ((22*8)/2), 92, "No Hexen II servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

//=============================================================================
/* SLIST MENU */

int		slist_cursor;
qboolean slist_sorted;

void M_Menu_ServerList_f (void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	int		n;
	char	string [64],*name;
	qpic_t	*p;

	if (!slist_sorted)
	{
		if (hostCacheCount > 1)
		{
			int	i,j;
			hostcache_t temp;
			for (i = 0; i < hostCacheCount; i++)
				for (j = i+1; j < hostCacheCount; j++)
					if (QStr::Cmp(hostcache[j].name, hostcache[i].name) < 0)
					{
						Com_Memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
						Com_Memcpy(&hostcache[j], &hostcache[i], sizeof(hostcache_t));
						Com_Memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
					}
		}
		slist_sorted = true;
	}

	ScrollTitle("gfx/menu/title4.lmp");
	for (n = 0; n < hostCacheCount; n++)
	{
		if (hostcache[n].driver == 0)
			name = net_drivers[hostcache[n].driver].name;
		else
			name = net_landrivers[hostcache[n].ldriver].name;

		if (hostcache[n].maxusers)
			sprintf(string, "%-11.11s %-8.8s %-10.10s %2u/%2u\n", hostcache[n].name, name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		else
			sprintf(string, "%-11.11s %-8.8s %-10.10s\n", hostcache[n].name, name, hostcache[n].map);
		M_Print (16, 60 + 8*n, string);
	}
	M_DrawCharacter (0, 60 + slist_cursor*8, 12+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (16, 176, m_return_reason);
}


void M_ServerList_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_LanConfig_f ();
		break;

	case K_SPACE:
		M_Menu_Search_f ();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound("raven/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
			slist_cursor = hostCacheCount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("raven/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;
		break;

	case K_ENTER:
		S_StartLocalSound("raven/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		Cbuf_AddText ( va ("connect \"%s\"\n", hostcache[slist_cursor].cname) );
		break;

	default:
		break;
	}

}

//=============================================================================
/* Menu Subsystem */


void M_Init (void)
{
	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand ("menu_load", M_Menu_Load_f);
	Cmd_AddCommand ("menu_save", M_Menu_Save_f);
	Cmd_AddCommand ("menu_multiplayer", M_Menu_MultiPlayer_f);
	Cmd_AddCommand ("menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("help", M_Menu_Help_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
	Cmd_AddCommand ("menu_class", M_Menu_Class2_f);

	M_BuildBigCharWidth();
	m_oldmission = Cvar_Get("m_oldmission", "0", CVAR_ARCHIVE);
}


void M_Draw (void)
{
	if (m_state == m_none || !(in_keyCatchers & KEYCATCH_UI))
		return;

	if (!m_recursiveDraw)
	{
		scr_copyeverything = 1;

		if (scr_con_current)
		{
			Draw_ConsoleBackground (vid.height);
			VID_UnlockBuffer ();
			S_ExtraUpdate ();
			VID_LockBuffer ();
		}
		else
			Draw_FadeScreen ();

		scr_fullupdate = 0;
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

	case m_difficulty:
		M_Difficulty_Draw ();
		break;

	case m_class:
		M_Class_Draw ();
		break;

	case m_load:
	case m_mload:
		M_Load_Draw ();
		break;

	case m_save:
	case m_msave:
		M_Save_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_net:
		M_Net_Draw ();
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
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;
	}

	if (m_entersound)
	{
		S_StartLocalSound("raven/menu2.wav");
		m_entersound = false;
	}

	VID_UnlockBuffer ();
	S_ExtraUpdate ();
	VID_LockBuffer ();
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

	case m_difficulty:
		M_Difficulty_Key (key);
		return;

	case m_class:
		M_Class_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;

	case m_mload:
		M_MLoad_Key (key);
		return;

	case m_msave:
		M_MSave_Key (key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_net:
		M_Net_Key (key);
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
		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;

	case m_search:
		M_Search_Key (key);
		break;

	case m_slist:
		M_ServerList_Key (key);
		return;
	}
}


void M_ConfigureNetSubsystem(void)
{
// enable/disable net systems to match desired config

	Cbuf_AddText ("stopdemo\n");

	net_hostport = lanConfig_port;
}
