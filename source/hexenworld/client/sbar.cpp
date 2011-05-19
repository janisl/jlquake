
//**************************************************************************
//**
//** sbar.c
//**
//** $Header: /HexenWorld/Client/sbar.c 37    5/31/98 2:58p Mgummelt $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

#define STAT_MINUS 10 // num frame for '-' stats digit

#define BAR_TOP_HEIGHT				46.0
#define BAR_BOTTOM_HEIGHT			98.0
#define BAR_TOTAL_HEIGHT			(BAR_TOP_HEIGHT+BAR_BOTTOM_HEIGHT)
#define BAR_BUMP_HEIGHT				23.0
#define INVENTORY_DISPLAY_TIME		4
#define ABILITIES_STR_INDEX			400

#define RING_FLIGHT					1
#define RING_WATER					2
#define RING_REGENERATION			4
#define RING_TURNING				8

#define	INV_MAX_CNT			15

#define INV_MAX_ICON		6		// Max number of inventory icons to display at once

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void Sbar_DeathmatchOverlay(void);
void Sbar_NormalOverlay(void);

void Sbar_SmallDeathmatchOverlay(void);
void M_DrawPic(int x, int y, image_t *pic);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void Sbar_DrawPic(int x, int y, image_t *pic);
static void Sbar_DrawSubPic(int x, int y, int h, image_t *pic);
static void Sbar_DrawTransPic(int x, int y, image_t *pic);
static int Sbar_itoa(int num, char *buf);
static void Sbar_DrawNum(int x, int y, int number, int digits);
void Sbar_SortFrags (qboolean includespec);
static void Sbar_DrawCharacter(int x, int y, int num);
static void Sbar_DrawString(int x, int y, char *str);
static void Sbar_DrawRedString (int cx, int cy, char *str);
static void Sbar_DrawSmallCharacter(int x, int y, int num);
static void Sbar_DrawSmallString(int x, int y, char *str);
static void DrawBarArtifactNumber(int x, int y, int number);

static void DrawFullScreenInfo(void);
static void DrawLowerBar(void);
static void DrawActiveRings(void);
static void DrawActiveArtifacts(void);
static int CalcAC(void);
static void DrawBarArtifactIcon(int x, int y, int artifact);

static qboolean SetChainPosition(float health, float maxHealth);

static void ShowInfoDown_f(void);
static void ShowInfoUp_f(void);
static void ShowDMDown_f(void);
static void ShowDMUp_f(void);
static void ShowNamesDown_f(void);
static void ShowNamesUp_f(void);
static void ToggleDM_f(void);
static void InvLeft_f(void);
static void InvRight_f(void);
static void InvUse_f(void);
static void InvDrop_f(void);
static void InvOff_f(void);

static void DrawArtifactInventory(void);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern int in_impulse;
// PUBLIC DATA DEFINITIONS -------------------------------------------------

int sb_lines; // scan lines to draw

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int sb_updates; // if >= vid.numpages, no update needed

static float BarHeight;
static float BarTargetHeight;
static QCvar* BarSpeed;
static QCvar* sbtemp;
static QCvar* DMMode;
static QCvar* sbtrans;

static image_t *sb_nums[11];
static image_t *sb_colon, *sb_slash;

static int fragsort[MAX_SCOREBOARD];

static char scoreboardtext[MAX_SCOREBOARD][20];
static int scoreboardtop[MAX_SCOREBOARD];
static int scoreboardbottom[MAX_SCOREBOARD];
static int scoreboardcount[MAX_SCOREBOARD];
static int scoreboardlines;

static float ChainPosition = 0;

static qboolean sb_ShowInfo;
static qboolean sb_ShowDM;
static qboolean sb_ShowNames;

qboolean inv_flg;					// true - show inventory interface

static float InventoryHideTime;

static char *PlayerClassNames[MAX_PLAYER_CLASS] =
{
	"Paladin",
	"Crusader",
	"Necromancer",
	"Assassin",
	"Succubus",
	"Dwarf"
};

static int AmuletAC[MAX_PLAYER_CLASS] =
{
	8,		// Paladin
	4,		// Crusader
	2,		// Necromancer
	6,		// Assassin
	6,		// Demoness
	0		// Dwarf
};

static int BracerAC[MAX_PLAYER_CLASS] =
{
	6,		// Paladin
	8,		// Crusader
	4,		// Necromancer
	2,		// Assassin
	2,		// Demoness
	10		// Demoness
};

static int BreastplateAC[MAX_PLAYER_CLASS] =
{
	2,		// Paladin
	6,		// Crusader
	8,		// Necromancer
	4,		// Assassin
	4,		// Demoness
	12		// Dwarf
};

static int HelmetAC[MAX_PLAYER_CLASS] =
{
	4,		// Paladin
	2,		// Crusader
	6,		// Necromancer
	8,		// Assassin
	8,		// Demoness
	10		// Dwarf
};

static int AbilityLineIndex[MAX_PLAYER_CLASS] =
{
	400,		// Paladin
	402,		// Crusader
	404,		// Necromancer
	406,		// Assassin
	590,		// Demoness
	594			// Dwarf
};
// CODE --------------------------------------------------------------------

//==========================================================================
//
// SB_Init
//
//==========================================================================

void Sbar_Init(void)
{
	int i;

	for(i = 0; i < 10; i++)
	{
		sb_nums[i] = R_PicFromWad(va("num_%i",i));
	}
	sb_nums[10] = R_PicFromWad("num_minus");
	sb_colon = R_PicFromWad("num_colon");
	sb_slash = R_PicFromWad("num_slash");

	Cmd_AddCommand("+showinfo", ShowInfoDown_f);
	Cmd_AddCommand("-showinfo", ShowInfoUp_f);
	Cmd_AddCommand("+showdm", ShowDMDown_f);
	Cmd_AddCommand("-showdm", ShowDMUp_f);
	Cmd_AddCommand("+shownames", ShowNamesDown_f);
	Cmd_AddCommand("-shownames", ShowNamesUp_f);
  	Cmd_AddCommand("invleft", InvLeft_f);
	Cmd_AddCommand("invright", InvRight_f);
	Cmd_AddCommand("invuse", InvUse_f);
	Cmd_AddCommand("invdrop", InvDrop_f);
	Cmd_AddCommand("invoff", InvOff_f);
	Cmd_AddCommand("toggle_dm", ToggleDM_f);

	BarSpeed = Cvar_Get("barspeed", "5", 0);
	sbtemp = Cvar_Get("sbtemp", "5", 0);
	DMMode = Cvar_Get("dm_mode", "1", CVAR_ARCHIVE);
	sbtrans = Cvar_Get("sbtrans", "0", CVAR_ARCHIVE);
	BarHeight = BarTargetHeight = BAR_TOP_HEIGHT;
}





void SB_PlacePlayerNames(void)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if ((cl.PIV & (1 << i)))
		{
			if (!cl.players[i].shownames_off)
			{
				if (cl_siege)
				{
					//why the fuck does GL fuck this up??!!!
					if (cl.players[i].siege_team==ST_ATTACKER)//attacker
					{
						if(i==cl_keyholder)
							R_DrawName(cl.players[i].origin, cl.players[i].name,10);
						else
							R_DrawName(cl.players[i].origin, cl.players[i].name,false);
					}
					else if (cl.players[i].siege_team==ST_DEFENDER)//def
					{
						if(i==cl_keyholder&&i==cl_doc)
							R_DrawName(cl.players[i].origin, cl.players[i].name,12);
						else if(i==cl_keyholder)
							R_DrawName(cl.players[i].origin, cl.players[i].name,11);
						else if(i==cl_doc)
							R_DrawName(cl.players[i].origin, cl.players[i].name,2);
						else
							R_DrawName(cl.players[i].origin, cl.players[i].name,1);
					}
					else
						R_DrawName(cl.players[i].origin, cl.players[i].name,3);
				}
				else
					R_DrawName(cl.players[i].origin, cl.players[i].name,false);
			}
		}
	}
}


//==========================================================================
//
// SB_Draw
//
//==========================================================================

void Sbar_Draw(void)
{
	float delta;
	char tempStr[80];
	int mana;
	int maxMana;
	int y;

	if (sb_ShowNames)
	{
		SB_PlacePlayerNames();
	}

	if (scr_con_current == vid.height)
	{ // console is full screen
		return;
	}

	if (cl.spectator)
	{
		if (sb_ShowDM)
		{
//rjr		if (cl.gametype == GAME_DEATHMATCH)
			Sbar_DeathmatchOverlay();
//rjr		else
//rjr			Sbar_NormalOverlay();
		}
		else if (DMMode->value)
		{
			Sbar_SmallDeathmatchOverlay();
		}

		return;
	}

	trans_level = (int)sbtrans->value;
	if (trans_level < 0 || trans_level > 2)
	{
		trans_level = 0;
	}

	// Draw always until we fix things
	//if (sb_updates >= vid.numpages)
	//	return;

/*	if(BarHeight == BarTargetHeight)
	{
		return;
	}
*/

	if(BarHeight < BarTargetHeight)
	{
		delta = ((BarTargetHeight-BarHeight)*BarSpeed->value)
			*host_frametime;
		if(delta < 0.5)
		{
			delta = 0.5;
		}
		BarHeight += delta;
		if(BarHeight > BarTargetHeight)
		{
			BarHeight = BarTargetHeight;
		}
		scr_fullupdate = 0;
	}
	else if(BarHeight > BarTargetHeight)
	{
		delta = ((BarHeight-BarTargetHeight)*BarSpeed->value)
			*host_frametime;
		if(delta < 0.5)
		{
			delta = 0.5;
		}
		BarHeight -= delta;
		if(BarHeight < BarTargetHeight)
		{
			BarHeight = BarTargetHeight;
		}
		scr_fullupdate = 0;
	}


	scr_copyeverything = 1;
	sb_updates++;


	if(BarHeight < 0)
	{
		DrawFullScreenInfo();
	}

	//Sbar_DrawPic(0, 0, Draw_CachePic("gfx/topbar.lmp"));
	Sbar_DrawPic(0, 0, Draw_CachePic("gfx/topbar1.lmp"));
	Sbar_DrawPic(160, 0, Draw_CachePic("gfx/topbar2.lmp"));
	Sbar_DrawTransPic(0, -23, Draw_CachePic("gfx/topbumpl.lmp"));
	Sbar_DrawTransPic(138, -8, Draw_CachePic("gfx/topbumpm.lmp"));
	Sbar_DrawTransPic(269, -23, Draw_CachePic("gfx/topbumpr.lmp"));

	maxMana = (int)cl.v.max_mana;
	// Blue mana
	mana = (int)cl.v.bluemana;
	if(mana < 0)
	{
		mana = 0;
	}
	if(mana > maxMana)
	{
		mana = maxMana;
	}
	sprintf(tempStr, "%03d", mana);
	Sbar_DrawSmallString(201, 22, tempStr);
	if(mana)
	{
		y = (int)((mana*18.0)/(float)maxMana+0.5);
		Sbar_DrawSubPic(190, 26-y, y+1,
			Draw_CachePic("gfx/bmana.lmp"));
		Sbar_DrawPic(190, 27, Draw_CachePic("gfx/bmanacov.lmp"));
	}

	// Green mana
	mana = (int)cl.v.greenmana;
	if(mana < 0)
	{
		mana = 0;
	}
	if(mana > maxMana)
	{
		mana = maxMana;
	}
	sprintf(tempStr, "%03d", mana);
	Sbar_DrawSmallString(243, 22, tempStr);
	if(mana)
	{
		y = (int)((mana*18.0)/(float)maxMana+0.5);
		Sbar_DrawSubPic(232, 26-y, y+1,
			Draw_CachePic("gfx/gmana.lmp"));
		Sbar_DrawPic(232, 27, Draw_CachePic("gfx/gmanacov.lmp"));
	}

	// HP
	if (cl.v.health < -99)
		Sbar_DrawNum(58, 14, -99, 3);
	else
		Sbar_DrawNum(58, 14, cl.v.health, 3);
	SetChainPosition(cl.v.health, cl.v.max_health);
	Sbar_DrawTransPic(45+((int)ChainPosition&7), 38,
		Draw_CachePic("gfx/hpchain.lmp"));
	Sbar_DrawTransPic(45+(int)ChainPosition, 36,
		Draw_CachePic("gfx/hpgem.lmp"));
	Sbar_DrawPic(43, 36, Draw_CachePic("gfx/chnlcov.lmp"));
	Sbar_DrawPic(267, 36, Draw_CachePic("gfx/chnrcov.lmp"));

	// AC
	Sbar_DrawNum(105, 14, CalcAC(), 2);

	if(BarHeight > BAR_TOP_HEIGHT)
	{
		DrawLowerBar();
	}

	// Current inventory item
	if(cl.inv_selected >= 0)
	{
		DrawBarArtifactIcon(144, 3, cl.inv_order[cl.inv_selected]);

		//Sbar_DrawTransPic(144, 3, Draw_CachePic(va("gfx/arti%02d.lmp",
		//	cl.inv_order[cl.inv_selected])));
	}

	// FIXME: Check for deathmatch and draw frags
	// if(cl.maxclients != 1) Sbar_Draw

	DrawArtifactInventory();

	DrawActiveRings();
	DrawActiveArtifacts();

	trans_level = 0;

	if (sb_ShowDM)
	{
//rjr		if (cl.gametype == GAME_DEATHMATCH)
			Sbar_DeathmatchOverlay();
//rjr		else
//rjr			Sbar_NormalOverlay();
	}
	else if (DMMode->value)
	{
		Sbar_SmallDeathmatchOverlay();
	}
}

//==========================================================================
//
// SetChainPosition
//
//==========================================================================

static qboolean SetChainPosition(float health, float maxHealth)
{
	float delta;
	float chainTargetPosition;

	if(health < 0)
	{
		health = 0;
	}
	else if(health > maxHealth)
	{
		health = maxHealth;
	}
	chainTargetPosition = (health*195)/maxHealth;
	if(fabs(ChainPosition-chainTargetPosition) < 0.1)
	{
		return false;
	}
	if(ChainPosition < chainTargetPosition)
	{
		delta = ((chainTargetPosition-ChainPosition)*5)*host_frametime;
		if(delta < 0.5)
		{
			delta = 0.5;
		}
		ChainPosition += delta;
		if(ChainPosition > chainTargetPosition)
		{
			ChainPosition = chainTargetPosition;
		}
	}
	else if(ChainPosition > chainTargetPosition)
	{
		delta = ((ChainPosition-chainTargetPosition)*5)*host_frametime;
		if(delta < 0.5)
		{
			delta = 0.5;
		}
		ChainPosition -= delta;
		if(ChainPosition < chainTargetPosition)
		{
			ChainPosition = chainTargetPosition;
		}
	}
	return true;
}

//==========================================================================
//
// DrawFullScreenInfo
//
//==========================================================================

static void DrawFullScreenInfo(void)
{
	int y;
	int mana;
	int maxMana;
	char tempStr[80];

	y = BarHeight-37;
	Sbar_DrawPic(3, y, Draw_CachePic("gfx/bmmana.lmp"));
	Sbar_DrawPic(3, y+18, Draw_CachePic("gfx/gmmana.lmp"));

	maxMana = (int)cl.v.max_mana;
	// Blue mana
	mana = (int)cl.v.bluemana;
	if(mana < 0)
	{
		mana = 0;
	}
	if(mana > maxMana)
	{
		mana = maxMana;
	}
	sprintf(tempStr, "%03d", mana);
	Sbar_DrawSmallString(10, y+6, tempStr);

	// Green mana
	mana = (int)cl.v.greenmana;
	if(mana < 0)
	{
		mana = 0;
	}
	if(mana > maxMana)
	{
		mana = maxMana;
	}
	sprintf(tempStr, "%03d", mana);
	Sbar_DrawSmallString(10, y+18+6, tempStr);

	// HP
	Sbar_DrawNum(38, y+18, cl.v.health, 3);

	// Current inventory item
	if(cl.inv_selected >= 0)
	{
		DrawBarArtifactIcon(288, y+7, cl.inv_order[cl.inv_selected]);
	}
}

//==========================================================================
//
// DrawLowerBar
//
//==========================================================================

static void DrawLowerBar(void)
{
	int i;
	int minutes, seconds, tens, units;
	char tempStr[80];
	int playerClass;
	int piece;
	int ringhealth;

	playerClass = cl.players[cl.playernum].playerclass;
//	playerClass = playerclass->value;
	if(playerClass < 1 || playerClass > MAX_PLAYER_CLASS)
	{ // Default to paladin
		playerClass = 1;
	}

	// Backdrop
	//Sbar_DrawPic(0, 46, Draw_CachePic("gfx/btmbar.lmp"));
	Sbar_DrawPic(0, 46, Draw_CachePic("gfx/btmbar1.lmp"));
	Sbar_DrawPic(160, 46, Draw_CachePic("gfx/btmbar2.lmp"));

	// Game time
	//minutes = cl.time / 60;
	//seconds = cl.time - 60*minutes;
	//tens = seconds / 10;
	//units = seconds - 10*tens;
	//sprintf(tempStr, "Time :%3i:%i%i", minutes, tens, units);
	//Sbar_DrawSmallString(116, 114, tempStr);

	// Map name
	//Sbar_DrawSmallString(10, 114, cl.levelname);

	// Stats
	Sbar_DrawSmallString(11, 48, PlayerClassNames[playerClass-1]);

	Sbar_DrawSmallString(11, 58, "int");
	sprintf(tempStr, "%02d", (int)cl.v.intelligence);
	Sbar_DrawSmallString(33, 58, tempStr);

	Sbar_DrawSmallString(11, 64, "wis");
	sprintf(tempStr, "%02d", (int)cl.v.wisdom);
	Sbar_DrawSmallString(33, 64, tempStr);

	Sbar_DrawSmallString(11, 70, "dex");
	sprintf(tempStr, "%02d", (int)cl.v.dexterity);
	Sbar_DrawSmallString(33, 70, tempStr);

	Sbar_DrawSmallString(58, 58, "str");
	sprintf(tempStr, "%02d", (int)cl.v.strength);
	Sbar_DrawSmallString(80, 58, tempStr);

	Sbar_DrawSmallString(58, 64, "lvl");
	sprintf(tempStr, "%02d", (int)cl.v.level);
	Sbar_DrawSmallString(80, 64, tempStr);

	Sbar_DrawSmallString(58, 70, "exp");
	sprintf(tempStr, "%06d", (int)cl.v.experience);
	Sbar_DrawSmallString(80, 70, tempStr);

	// Abilities
	Sbar_DrawSmallString(11, 79, "abilities");
	i = AbilityLineIndex[(playerClass-1)];
	if(i+1 < pr_string_count)
	{
		if(((int)cl.v.flags)&FL_SPECIAL_ABILITY1)
		{
			Sbar_DrawSmallString(8, 89,
				&pr_global_strings[pr_string_index[i]]);
		}
		if(((int)cl.v.flags)&FL_SPECIAL_ABILITY2)
		{
			Sbar_DrawSmallString(8, 96,
				&pr_global_strings[pr_string_index[i+1]]);
		}
	}

	// Portrait
	sprintf(tempStr, "gfx/cport%d.lmp", playerClass);
	Sbar_DrawPic(134, 50, Draw_CachePic(tempStr));

	// Armor
	if(cl.v.armor_helmet > 0)
	{
		Sbar_DrawPic(164, 115, Draw_CachePic("gfx/armor1.lmp"));
		sprintf(tempStr, "+%d", (int)cl.v.armor_helmet);
		Sbar_DrawSmallString(168, 136, tempStr);
	}
	if(cl.v.armor_amulet > 0)
	{
		Sbar_DrawPic(205, 115, Draw_CachePic("gfx/armor2.lmp"));
		sprintf(tempStr, "+%d", (int)cl.v.armor_amulet);
		Sbar_DrawSmallString(208, 136, tempStr);
	}
	if(cl.v.armor_breastplate > 0)
	{
		Sbar_DrawPic(246, 115, Draw_CachePic("gfx/armor3.lmp"));
		sprintf(tempStr, "+%d", (int)cl.v.armor_breastplate);
		Sbar_DrawSmallString(249, 136, tempStr);
	}
	if(cl.v.armor_bracer > 0)
	{
		Sbar_DrawPic(285, 115, Draw_CachePic("gfx/armor4.lmp"));
		sprintf(tempStr, "+%d", (int)cl.v.armor_bracer);
		Sbar_DrawSmallString(288, 136, tempStr);
	}

	// Rings 
	if(cl.v.ring_flight > 0)
	{
		Sbar_DrawTransPic(6, 119, Draw_CachePic("gfx/ring_f.lmp"));

		ringhealth = (int)cl.v.ring_flight;
		if(ringhealth > 100)
			ringhealth = 100;
		Sbar_DrawPic(  35 - (int)(26 * (ringhealth/(float)100)),142,Draw_CachePic("gfx/ringhlth.lmp"));
		Sbar_DrawPic( 35, 142, Draw_CachePic("gfx/rhlthcvr.lmp"));
	}

	if(cl.v.ring_water > 0)
	{
		Sbar_DrawTransPic(44, 119, Draw_CachePic("gfx/ring_w.lmp"));
		ringhealth = (int)cl.v.ring_water;
		if(ringhealth > 100)
			ringhealth = 100;
		Sbar_DrawPic(  73 - (int)(26 * (ringhealth/(float)100)),142,Draw_CachePic("gfx/ringhlth.lmp"));
		Sbar_DrawPic( 73, 142, Draw_CachePic("gfx/rhlthcvr.lmp"));
	}

	if(cl.v.ring_turning > 0)
	{
		Sbar_DrawTransPic(81, 119, Draw_CachePic("gfx/ring_t.lmp"));
		ringhealth = (int)cl.v.ring_turning;
		if(ringhealth > 100)
			ringhealth = 100;
		Sbar_DrawPic(  110 - (int)(26 * (ringhealth/(float)100)),142,Draw_CachePic("gfx/ringhlth.lmp"));
		Sbar_DrawPic( 110, 142, Draw_CachePic("gfx/rhlthcvr.lmp"));
	}

	if(cl.v.ring_regeneration > 0)
	{
		Sbar_DrawTransPic(119, 119, Draw_CachePic("gfx/ring_r.lmp"));
		ringhealth = (int)cl.v.ring_regeneration;
		if(ringhealth > 100)
			ringhealth = 100;
		Sbar_DrawPic( 148 -(int)(26 * (ringhealth/(float)100)),142,Draw_CachePic("gfx/ringhlth.lmp"));
		Sbar_DrawPic( 148, 142, Draw_CachePic("gfx/rhlthcv2.lmp"));
	}

	// Puzzle pieces
	piece = 0;
	for(i = 0; i < 8; i++)
	{
		if(cl.puzzle_pieces[i][0] == 0)
		{
			continue;
		}
		Sbar_DrawPic(194+(piece%4)*31, piece < 4 ? 51 : 82,
			Draw_CachePic(va("gfx/puzzle/%s.lmp", cl.puzzle_pieces[i])));
		piece++;
	}
}

//==========================================================================
//
// CalcAC
//
//==========================================================================

static int CalcAC(void)
{
	int a;
	int playerClass;

	playerClass = cl.v.playerclass;
	if(playerClass < 1 || playerClass > MAX_PLAYER_CLASS)
	{
		playerClass = 1;
	}
	a = 0;
	if(cl.v.armor_amulet > 0)
	{
		a += AmuletAC[playerClass];
		a += cl.v.armor_amulet/5;
	}
	if(cl.v.armor_bracer > 0)
	{
		a += BracerAC[playerClass];
		a += cl.v.armor_bracer/5;
	}
	if(cl.v.armor_breastplate > 0)
	{
		a += BreastplateAC[playerClass];
		a += cl.v.armor_breastplate/5;
	}
	if(cl.v.armor_helmet > 0)
	{
		a += HelmetAC[playerClass];
		a += cl.v.armor_helmet/5;
	}
	return a;
}

//==========================================================================
//
// SB_Changed
//
//==========================================================================

void SB_Changed(void)
{
	sb_updates = 0;	// Update next frame
}

//==========================================================================
//
// Sbar_itoa
//
//==========================================================================

static int Sbar_itoa(int num, char *buf)
{
	char *str;
	int pow10;
	int dig;

	str = buf;
	if(num < 0)
	{
		*str++ = '-';
		num = -num;
	}

	for(pow10 = 10; num >= pow10; pow10 *= 10)
	;

	do
	{
		pow10 /= 10;
		dig = num/pow10;
		*str++ = '0'+dig;
		num -= dig*pow10;
	} while(pow10 != 1);

	*str = 0;

	return str-buf;
}

//==========================================================================
//
// Sbar_DrawNum
//
//==========================================================================

static void Sbar_DrawNum(int x, int y, int number, int digits)
{
	char str[12];
	char *ptr;
	int l, frame;

	l = Sbar_itoa(number, str);
	ptr = str;
	if(l > digits)
	{
		ptr += (l-digits);
	}
	if(l < digits)
	{
		x += ((digits-l)*13)/2;
	}

	while(*ptr)
	{
		if(*ptr == '-')
		{
			frame = STAT_MINUS;
		}
		else
		{
			frame = *ptr -'0';
		}
		Sbar_DrawTransPic(x, y, sb_nums[frame]);
		x += 13;
		ptr++;
	}
}

//==========================================================================
//
// Sbar_SortFrags
//
//==========================================================================

#define SPEC_FRAGS -9999

void Sbar_SortFrags (qboolean includespec)
{
	int		i, j, k;
		
// sort by frags
	scoreboardlines = 0;
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (cl.players[i].name[0] &&
			(!cl.players[i].spectator || includespec))
		{
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
			if (cl.players[i].spectator)
				cl.players[i].frags = SPEC_FRAGS;
		}
	}
		
	for (i=0 ; i<scoreboardlines ; i++)
		for (j=0 ; j<scoreboardlines-1-i ; j++)
			if (cl.players[fragsort[j]].frags < cl.players[fragsort[j+1]].frags)
			{
				k = fragsort[j];
				fragsort[j] = fragsort[j+1];
				fragsort[j+1] = k;
			}
}

int	Sbar_ColorForMap (int m)
{
	return m < 128 ? m + 8 : m + 8;
}

//==========================================================================
//
// SoloScoreboard
//
//==========================================================================

static void SoloScoreboard(void)
{
	char	str[80];
	int		minutes, seconds, tens, units;
	int		l;

	sprintf (str,"Monsters:%3i /%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	Sbar_DrawString (8, 4, str);

	sprintf (str,"Secrets :%3i /%3i", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
	Sbar_DrawString (8, 12, str);

	// draw time
	minutes = cl.time / 60;
	seconds = cl.time - 60*minutes;
	tens = seconds / 10;
	units = seconds - 10*tens;
	sprintf (str,"Time :%3i:%i%i", minutes, tens, units);
	Sbar_DrawString (184, 4, str);
	
	// draw level name
	l = QStr::Length(cl.levelname);
	Sbar_DrawString (232 - l*4, 12, cl.levelname);
}

//==========================================================================
//
// Sbar_DrawScoreboard
//
//==========================================================================

void Sbar_DrawScoreboard(void)
{
	SoloScoreboard();
//rjr	if(cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay();
	}
}

//==========================================================================
//
// Sbar_DrawFrags
//
//==========================================================================
/*
void Sbar_DrawFrags (void)
{	
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	int				xofs;
	char			num[12];
	scoreboard_t	*s;
	
	Sbar_SortFrags ();

// draw the text
	l = scoreboardlines <= 4 ? scoreboardlines : 4;
	
	x = 23;
	xofs = (vid.width - 320)>>1;
	y = vid.height - BAR_TOP_HEIGHT - 23;

	for (i=0 ; i<l ; i++)
	{
		k = fragsort[i];
		s = &cl.scores[k];
		if (!s->name[0])
			continue;

	// draw background
		top = s->colors & 0xf0;
		bottom = (s->colors & 15)<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);
	
		Draw_Fill (xofs + x*8 + 10, y, 28, 4, top);
		Draw_Fill (xofs + x*8 + 10, y+4, 28, 3, bottom);

	// draw number
		f = s->frags;
		sprintf (num, "%3i",f);
		
		Sbar_DrawCharacter ( (x+1)*8 , -24, num[0]);
		Sbar_DrawCharacter ( (x+2)*8 , -24, num[1]);
		Sbar_DrawCharacter ( (x+3)*8 , -24, num[2]);

		if (k == cl.viewentity - 1)
		{
			Sbar_DrawCharacter (x*8+2, -24, 16);
			Sbar_DrawCharacter ( (x+4)*8-4, -24, 17);
		}
		x+=4;
	}
}
*/

//==========================================================================
//
// Sbar_IntermissionNumber
//
//==========================================================================

void Sbar_IntermissionNumber (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Draw_TransPic (x,y,sb_nums[frame]);
		x += 24;
		ptr++;
	}
}

extern int color_offsets[MAX_PLAYER_CLASS];
extern byte *playerTranslation;

void FindColor (int slot, int *color1, int *color2)
{
	int		j;
	int		top, bottom, done;
	byte	*sourceA, *sourceB, *colorA, *colorB;
	
	if (slot > MAX_CLIENTS)
		Sys_Error ("CL_NewTranslation: slot > cl.maxclients");

	if (cl.players[slot].playerclass <= 0 || cl.players[slot].playerclass > MAX_PLAYER_CLASS)
	{
		*color1 = *color2 = 0;
		return;
	}

	top = cl.players[slot].topcolor;
	bottom = cl.players[slot].bottomcolor;

	if (top > 10) top = 0;
	if (bottom > 10) bottom = 0;

	top -= 1;
	bottom -= 1;

	colorA = playerTranslation + 256 + color_offsets[(int)cl.players[slot].playerclass-1];
	colorB = colorA + 256;
	sourceA = colorB + 256 + (top * 256);
	sourceB = colorB + 256 + (bottom * 256);
	done = 0;
	for(j=0;j<256;j++,colorA++,colorB++,sourceA++,sourceB++)
	{
		if ((*colorA != 255) && !(done & 1)) 
		{
			if (top >= 0)
				*color1 = *sourceA;
			else
				*color1 = *colorA;
		//	done |= 1;
		}
		if ((*colorB != 255) && !(done & 2)) 
		{
			if (bottom >= 0)
				*color2 = *sourceB;
			else
				*color2 = *colorB;
		//	done |= 2;
		}
	}
}

//==========================================================================
//
// Sbar_DeathmatchOverlay
//
//==========================================================================

void Sbar_DeathmatchOverlay(void)
{
	image_t			*pic;
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	char			num[40];
	player_info_t	*s;
	float			total;
	int				minutes;

	if (realtime - cl.last_ping_request > 2)
	{
		cl.last_ping_request = realtime;
		cls.netchan.message.WriteByte(clc_stringcmd);
		cls.netchan.message.WriteString2("pings");
	}

	scr_copyeverything = 1;
	scr_fullupdate = 0;

	pic = Draw_CachePic ("gfx/menu/title8.lmp");
	M_DrawTransPic ((320-Draw_GetWidth(pic))/2, 0, pic);

// scores	
	Sbar_SortFrags (true);

// draw the text
	l = scoreboardlines;
	
	x = ((vid.width - 320)>>1);//was 8 + ...
	y = 62;

	Sbar_DrawRedString (x+4, y-1, "FRAG");
	Sbar_DrawRedString (x+48, y-1, "PING");
	Sbar_DrawRedString (x+88, y-1, "TIME");
	Sbar_DrawRedString (x+128, y-1, "NAME");
	y += 12;

	for (i=0 ; i<l ; i++)
	{
		if (y+10 >= vid.height)
			break;

		k = fragsort[i];
		s = &cl.players[k];

		if (!s->name[0])
			continue;

		
		if (s->frags != SPEC_FRAGS)
		{
			switch(s->playerclass)
			{//Print playerclass next to name
			case 0:
				Sbar_DrawRedString (x+128, y, "(r   )");
				break;
			case 1:
				Sbar_DrawRedString (x+128, y, "(P   )");
				break;
			case 2:
				Sbar_DrawRedString (x+128, y, "(C   )");
				break;
			case 3:
				Sbar_DrawRedString (x+128, y, "(N   )");
				break;
			case 4:
				Sbar_DrawRedString (x+128, y, "(A   )");
				break;
			case 5:
				Sbar_DrawRedString (x+128, y, "(S   )");
				break;
			case 6:
				Sbar_DrawRedString (x+128, y, "(D   )");
				break;
			default:
				Sbar_DrawRedString (x+128, y, "(?   )");
				break;
			}

			if(cl_siege)
			{
				Sbar_DrawRedString (x+160, y, "6");
				if(s->siege_team==1)
					Draw_Character ( x+152 , y, 143);//shield
				else if(s->siege_team==2)
					Draw_Character ( x+152 , y, 144);//sword
				else
					Sbar_DrawRedString ( x+152 , y, "?");//no team

				if(k==cl_keyholder)
					Draw_Character ( x+144 , y, 145);//key
				else
					Sbar_DrawRedString (x+144, y, "-");

				if(k==cl_doc)
					Draw_Character ( x+160 , y, 130);//crown
				else
					Sbar_DrawRedString (x+160, y, "6");

			}
			else
			{
				Sbar_DrawRedString (x+144, y, "-");
				if(!s->level)
					Sbar_DrawRedString (x+152, y, "01");
				else if((int)s->level<10)
				{
					Sbar_DrawRedString (x+152, y, "0");
					sprintf(num, "%1d",s->level);
					Sbar_DrawRedString (x+160, y, num);
				}
				else
				{
					sprintf(num, "%2d",s->level);
					Sbar_DrawRedString (x+152, y, num);
				}
			}

			// draw background
			FindColor (k, &top, &bottom);
		
			Draw_Fill ( x+8, y, 28, 4, top);//was x to 40
			Draw_Fill ( x+8, y+4, 28, 4, bottom);//was x to 40

			// draw number
			f = s->frags;
			sprintf (num, "%3i",f);
			
			Draw_Character ( x+10 , y-1, num[0]);//was 8
			Draw_Character ( x+18 , y-1, num[1]);//was 16
			Draw_Character ( x+26 , y-1, num[2]);//was 24

			if (k == cl.playernum)
				Draw_Character ( x, y-1, 13);

//rjr			if(k==sv_kingofhill)
//rjr				Draw_Character ( x+40 , y-1, 130);
			sprintf(num, "%4d",s->ping);
			Draw_String(x+48, y, num);

			if (cl.intermission)
				total = cl.completed_time - s->entertime;
			else
				total = realtime - s->entertime;
			minutes = (int)total/60;
			sprintf (num, "%4i", minutes);
			Draw_String ( x+88 , y, num);

			// draw name
			if(cl_siege&&s->siege_team==2)//attacker
				Draw_String (x+178, y, s->name);
			else
				Sbar_DrawRedString (x+178, y, s->name);
		}
		else
		{
			sprintf(num, "%4d",s->ping);
			Sbar_DrawRedString (x+48, y, num);

			if (cl.intermission)
				total = cl.completed_time - s->entertime;
			else
				total = realtime - s->entertime;
			minutes = (int)total/60;
			sprintf (num, "%4i", minutes);
			Sbar_DrawRedString ( x+88 , y, num);

			// draw name
			Sbar_DrawRedString (x+128, y, s->name);
		}

		y += 10;
	}
}

void FindName(char *which, char *name)
{
	int j,length;
	char *pos,*p2;
	char test[40];

	QStr::Cpy(name, "Unknown");
	j = atol(puzzle_strings);
	pos = strchr(puzzle_strings,13);
	if (!pos) return;

	if ((*pos) == 10 || (*pos) == 13)
		pos++;
	if ((*pos) == 10 || (*pos) == 13)
		pos++;

	length = QStr::Length(which);

	for(;j;j--)
	{
		p2 = strchr(pos,' ');
		if (!p2) 
			return;

		QStr::NCpy(test,pos,p2-pos);
		test[p2-pos] = 0;

		if (QStr::ICmp(which,test) == 0)
		{
			pos = strchr(pos,' ');
			if (pos)
			{
				pos++;
				p2 = strchr(pos,13);
				if (p2)
				{
					QStr::NCpy(name,pos,p2-pos);
					name[p2-pos] = 0;
					return;
				}
			}
			return;
		}
		pos = strchr(pos,13);
		if (!pos) return;
		if ((*pos) == 10 || (*pos) == 13)
			pos++;
		if ((*pos) == 10 || (*pos) == 13)
			pos++;
	}
}

//==========================================================================
//
// Sbar_NormalOverlay
//
//==========================================================================

void Sbar_NormalOverlay(void)
{
	image_t			*pic;
	int				i,y,piece;
	char			Name[40];

	scr_copyeverything = 1;
	scr_fullupdate = 0;

	piece = 0;
	y = 40;
	for(i = 0; i < 8; i++)
	{
		if(cl.puzzle_pieces[i][0] == 0)
		{
			continue;
		}

		if (piece == 4) y = 40;

		FindName(cl.puzzle_pieces[i],Name);

		if (piece < 4)
		{
			M_DrawPic(10, y,
				Draw_CachePic(va("gfx/puzzle/%s.lmp", cl.puzzle_pieces[i])));
			M_PrintWhite (45, y, Name);
		}
		else
		{
			M_DrawPic(310-32, y,
				Draw_CachePic(va("gfx/puzzle/%s.lmp", cl.puzzle_pieces[i])));
			M_PrintWhite (310-32-3-(QStr::Length(Name)*8), 18+y, Name);
		}

		y += 32;
		piece++;
	}
}

//==========================================================================
//
// Sbar_SmallDeathmatchOverlay
//
//==========================================================================

void DrawTime (int x, int y, int disp_time)
{
int	show_min,show_sec;
char num[40];
	show_min = disp_time;
	show_sec = show_min%60;
	show_min = (show_min - show_sec)/60;
	sprintf(num, "%3d",show_min);
	Sbar_DrawRedString ( x+8 , y, num);
//	Draw_Character ( x+32 , y, 58);// ":"
	sprintf(num, "%2d",show_sec);
	if(show_sec>=10)
		Sbar_DrawRedString ( x+40 , y, num);
	else
	{
		Sbar_DrawRedString ( x+40 , y, num);
		Sbar_DrawRedString ( x+40 , y, "0");
	}
}

void Sbar_SmallDeathmatchOverlay(void)
{
	image_t			*pic;
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	int				def_frags,att_frags;
	int				show_min,show_sec;
	char			num[40];
//	unsigned char	num[12];
	player_info_t	*s;

	if (DMMode->value >= 2 && BarHeight != BAR_TOP_HEIGHT)
		return;

	trans_level = (int)((DMMode->value-floor(DMMode->value)+1E-3)*10);
	if (trans_level > 2) 
	{
		trans_level = 0;
	}

	scr_copyeverything = 1;
	scr_fullupdate = 0;

// scores	
	Sbar_SortFrags (false);

// draw the text
	l = scoreboardlines;

	x = 10;
	if ((int)DMMode->value == 1)
	{
		i = (vid.height - 120) / 10;

		if (l > i) 
			l = i;

		y = 46;
	}
	else if ((int)DMMode->value == 2)
	{
		if (l > 4) 
			l = 4;
		y = vid.height - BAR_TOP_HEIGHT;
	}
	if (cl_siege)
	{
		if (l > i) 
			l = i;
		y = vid.height - BAR_TOP_HEIGHT - 24;
		x-=4;
		Draw_Character ( x , y, 142);//sundial
		Draw_Character ( x+32 , y, 58);// ":"
		if(cl_timelimit>cl.time+cl_server_time_offset)
		{
			DrawTime(x,y,(int)(cl_timelimit - (cl.time + cl_server_time_offset)));
		}
		else if((int)cl.time%2)
		{//odd number, draw 00:00, this will make it flash every second
			Sbar_DrawRedString ( x+16 , y, "00");
			Sbar_DrawRedString ( x+40 , y, "00");
		}
	}
	
	def_frags = att_frags = 0;
	for (i=0 ; i<l ; i++)
	{
		k = fragsort[i];
		s = &cl.players[k];
		if (!s->name[0])
			continue;

		if(cl_siege)
		{
			if(s->siege_team==1)
			{//defender
				if(s->frags>0)
					def_frags+=s->frags;
				else
					att_frags-=s->frags;
			}
			else if(s->siege_team==2)
			{//attacker
				if(s->frags>0)
					att_frags+=s->frags;
				else
					def_frags-=s->frags;
			}
		}
		else
		{
			if ((int)DMMode->value == 2)
			{
			}
			// draw background
			FindColor (k, &top, &bottom);
		
			Draw_Fill ( x, y, 28, 4, top);
			Draw_Fill ( x, y+4, 28, 4, bottom);

			// draw number
			f = s->frags;
			sprintf (num, "%3i",f);
			
	/*
			if (k != cl.playernum)
			{
				Draw_Character ( x+2 , y-1, num[0]);
				Draw_Character ( x+10 , y-1, num[1]);
				Draw_Character ( x+18 , y-1, num[2]);
			}
			else
			{
				Draw_Character ( x - 8, y-1, 13);
				Draw_Character ( x+2 , y-1, num[0] + 256);
				Draw_Character ( x+10 , y-1, num[1] + 256);
				Draw_Character ( x+18 , y-1, num[2] + 256);
			}
	*/

			if (k == cl.playernum)
			{
				Draw_Character ( x - 8, y-1, 13);
			}
			Draw_Character ( x+2 , y-1, num[0]);
			Draw_Character ( x+10 , y-1, num[1]);
			Draw_Character ( x+18 , y-1, num[2]);
	//rjr		if(k==sv_kingofhill)
	//rjr			Draw_Character ( x+28 , y-1, 130);
			y += 10;
		}
	}
	if(cl_siege)
	{
		if(cl.playernum == cl_keyholder)
			Draw_Character ( x , y-10, 145);//key
		if(cl.playernum == cl_doc)
			Draw_Character ( x+8 , y-10, 130);//crown
		if((int)cl.v.artifact_active&ARTFLAG_BOOTS)
			Draw_Character ( x+16 , y-10, 146);//boots
		
		//Print defender losses
		Draw_Character ( x , y+10, 143);//shield
		sprintf (num, "%3i",defLosses);
//		sprintf (num, "%3i",att_frags);
		Sbar_DrawRedString ( x+8 , y+10, num);
		Draw_Character ( x+32 , y+10, 47);// "/"
		sprintf (num, "%3i",cl_fraglimit);
		Sbar_DrawRedString ( x+40 , y+10, num);

		//Print attacker losses
		Draw_Character ( x , y+20, 144);//sword
		sprintf (num, "%3i",attLosses);
//		sprintf (num, "%3i",def_frags);
		Sbar_DrawRedString ( x+8 , y+20, num);
		Draw_Character ( x+32 , y+20, 47);// "/"
		sprintf (num, "%3i",cl_fraglimit*2);
		Sbar_DrawRedString ( x+40 , y+20, num);
	}

	trans_level = 0;
}


//==========================================================================
//
// DrawActiveRings
//
//==========================================================================
int art_col;
int ring_row;

static void DrawActiveRings(void)
{
	int flag;
	int frame;
	char tempStr[24];

	if (scr_con_current == vid.height)
		return;		// console is full screen

	ring_row = 1;

	flag = (int)cl.v.rings_active;

	if(flag&RING_TURNING)
	{
		frame = 1+(int)(cl.time*16)%16;
		sprintf(tempStr, "gfx/rngtrn%d.lmp", frame);
		Draw_TransPic(vid.width - 50, ring_row, Draw_CachePic(tempStr));
		ring_row += 33;
	}

/*	if(flag&RING_REGENERATION)
	{
		frame = 1+(int)(cl.time*16)%16;
		sprintf(tempStr, "gfx/rngreg%d.lmp", frame);
		Draw_TransPic(vid.width - 50, ring_row, Draw_CachePic(tempStr));
		ring_row += 33;
	}
*/

	if(flag&RING_WATER)
	{
		frame = 1+(int)(cl.time*16)%16;
		sprintf(tempStr, "gfx/rngwtr%d.lmp", frame);
		Draw_TransPic(vid.width - 50, ring_row, Draw_CachePic(tempStr));
		ring_row += 33;
	}

	if(flag&RING_FLIGHT)
	{
		frame = 1+(int)(cl.time*16)%16;
		sprintf(tempStr, "gfx/rngfly%d.lmp", frame);
		Draw_TransPic(vid.width - 50, ring_row, Draw_CachePic(tempStr));
		ring_row += 33;
	}
}

//==========================================================================
//
// DrawActiveArtifacts
//
//==========================================================================
static void DrawActiveArtifacts(void)
{
	int flag;
	static int oldflags = 0;
	int frame;
	char tempStr[24];

	if (scr_con_current == vid.height)
		return;

	art_col = 50;

	if (ring_row != 1)
		art_col += 50;

	flag = (int)cl.v.artifact_active;
	if (flag & ART_TOMEOFPOWER)
	{
		frame = 1+(int)(cl.time*16)%16;
		sprintf(tempStr, "gfx/pwrbook%d.lmp", frame);
		Draw_TransPic(vid.width-art_col, 1, Draw_CachePic(tempStr));
		art_col += 50;
		scr_topupdate = 0;
	}
	else if (oldflags & ART_TOMEOFPOWER)
	{
		scr_topupdate = 0;
	}

	if (flag & ART_HASTE)
	{
		frame = 1+(int)(cl.time*16)%16;
		sprintf(tempStr, "gfx/durhst%d.lmp", frame);
		Draw_TransPic(vid.width-art_col,1, Draw_CachePic(tempStr));
		art_col += 50;
		scr_topupdate = 0;
	}
	else if (oldflags & ART_HASTE)
	{
		scr_topupdate = 0;
	}

	if (flag & ART_INVINCIBILITY)
	{
		frame = 1+(int)(cl.time*16)%16;
		sprintf(tempStr, "gfx/durshd%d.lmp", frame);
		Draw_TransPic(vid.width-art_col, 1, Draw_CachePic(tempStr));
		art_col += 50;
		scr_topupdate = 0;
	}
	else if (oldflags & ART_INVINCIBILITY)
	{
		scr_topupdate = 0;
	}

	oldflags = flag;
}

//============================================================================


void Inv_Update(qboolean force)
{
	if(inv_flg || force)
	{
		// Just to be safe
		if (cl.inv_selected >= 0 && cl.inv_count > 0)
			cl.v.inventory = cl.inv_order[cl.inv_selected] + 1;
		else
			cl.v.inventory = 0;

		if (!force) 
		{
			scr_fullupdate = 0;
			inv_flg = false;  // Toggle menu off
		}

		if (cls.state == ca_active)
		{	// This will cause the server to set the client's edict's inventory value
			cls.netchan.message.WriteByte(clc_inv_select);
			cls.netchan.message.WriteByte(cl.v.inventory);
		}
	}
}

//==========================================================================
//
// DrawBarArtifactIcon
//
//==========================================================================

static void DrawBarArtifactIcon(int x, int y, int artifact)
{
	int count;

	Sbar_DrawTransPic(x, y, Draw_CachePic(va("gfx/arti%02d.lmp",
		artifact)));
//	if((count = (int)(&cl.v.cnt_torch)[artifact]) > 1)
	if((count = (int)(&cl.v.cnt_torch)[artifact]) > 0)
	{
		DrawBarArtifactNumber(x+20, y+21, count);
	}
}

//==========================================================================
//
// DrawArtifactInventory
//
//==========================================================================

static void DrawArtifactInventory(void)
{
	int i;
	int x, y;

	if(InventoryHideTime < cl.time)
	{
		Inv_Update(false);
		return;
	}
	if(!inv_flg)
	{
		return;
	}
	if(!cl.inv_count)
	{
		Inv_Update(false);
		return;
	}

	//SB_InvChanged();

	if(BarHeight < 0)
	{
		y = BarHeight-34;
	}
	else
	{
		y = -37;
	}
	for(i = 0, x = 64; i < INV_MAX_ICON; i++, x += 33)
	{
		if(cl.inv_startpos+i >= cl.inv_count)
		{
			break;
		}
		if(cl.inv_startpos+i == cl.inv_selected)
		{ // Highlight icon
			Sbar_DrawTransPic(x+9, y-12, Draw_CachePic("gfx/artisel.lmp"));
		}
		DrawBarArtifactIcon(x, y, cl.inv_order[cl.inv_startpos+i]);
	}

	//Inv_DrawArrows (x, y);

/*	if (cl.inv_startpos)  // Left arrow showing there are icons to the left 
		Draw_Fill ( x , y - 5, 3, 1, 30);

	if (cl.inv_startpos + INV_MAX_ICON < cl.inv_count) // Right arrow showing there are icons to the right
		Draw_Fill ( x + 200, y - 5 , 3, 1, 30);
*/
}

//==========================================================================
//
// ShowDMDown_f
//
//==========================================================================

static void ShowDMDown_f(void)
{
	if (sb_ShowDM)
	{
		return;
	}

	sb_ShowDM = true;
}

//==========================================================================
//
// ShowDMUp_f
//
//==========================================================================

static void ShowDMUp_f(void)
{
	sb_ShowDM = false;
}

//==========================================================================
//
// ShowNamesDown_f
//
//==========================================================================

static void ShowNamesDown_f(void)
{
	if (sb_ShowNames)
	{
		return;
	}

	sb_ShowNames = true;
}

//==========================================================================
//
// ShowNamesUp_f
//
//==========================================================================

static void ShowNamesUp_f(void)
{
	sb_ShowNames = false;
}

//==========================================================================
//
// ShowInfoDown_f
//
//==========================================================================

static void ShowInfoDown_f(void)
{
	if(sb_ShowInfo || cl.intermission)
	{
		return;
	}
	S_StartLocalSound("misc/barmovup.wav");
	BarTargetHeight = BAR_TOTAL_HEIGHT;
	sb_ShowInfo = true;
	sb_updates = 0;
}

//==========================================================================
//
// ShowInfoUp_f
//
//==========================================================================

static void ShowInfoUp_f(void)
{
	if(cl.intermission || (scr_viewsize->value >= 110.0 && !sbtrans->value))
	{
		BarTargetHeight = 0.0-BAR_BUMP_HEIGHT;
	}
	else
	{
		BarTargetHeight = BAR_TOP_HEIGHT;
	}
	S_StartLocalSound("misc/barmovdn.wav");
	sb_ShowInfo = false;
	sb_updates = 0;
}

//==========================================================================
//
// InvLeft_f
//
//==========================================================================

static void InvLeft_f(void)
{
	if(!cl.inv_count || cl.intermission)
	{
		return;
	}

	if(inv_flg)
	{
		if(cl.inv_selected > 0)
		{
			cl.inv_selected--;
			if(cl.inv_selected < cl.inv_startpos)
			{
				cl.inv_startpos = cl.inv_selected;
			}
			scr_fullupdate = 0;
		}
	}
	else
	{
		inv_flg = true;
	}
	S_StartLocalSound("misc/invmove.wav");
	InventoryHideTime = cl.time+INVENTORY_DISPLAY_TIME;
}

//==========================================================================
//
// InvRight_f
//
//==========================================================================

static void InvRight_f(void)
{
	if(!cl.inv_count || cl.intermission)
	{
		return;
	}

	if(inv_flg)
	{
		if(cl.inv_selected < cl.inv_count-1)
		{
			cl.inv_selected++;
			if (cl.inv_selected - cl.inv_startpos >= INV_MAX_ICON)
			{
				// could probably be just a cl.inv_startpos++, but just in case
				cl.inv_startpos = cl.inv_selected - INV_MAX_ICON + 1;
			}
			scr_fullupdate = 0;
		}
	}
	else
	{
		inv_flg = true;
	}
	S_StartLocalSound("misc/invmove.wav");
	InventoryHideTime = cl.time+INVENTORY_DISPLAY_TIME;
}

//==========================================================================
//
// InvUse_f
//
//==========================================================================

static void InvUse_f(void)
{
	if(!cl.inv_count || cl.intermission)
	{
		return;
	}
	S_StartLocalSound("misc/invuse.wav");
	//Inv_Update(false);
	Inv_Update(true);
	inv_flg = false;
	scr_fullupdate = 0;
	in_impulse = 23;
}

//==========================================================================
//
// InvDrop_f
//
//==========================================================================

static void InvDrop_f(void)
{
	if(!cl.inv_count || cl.intermission)
	{
		return;
	}
	S_StartLocalSound("misc/invuse.wav");
	//Inv_Update(false);
	Inv_Update(true);
	inv_flg = false;
	scr_fullupdate = 0;
	in_impulse = 44;
}

//==========================================================================
//
// InvOff_f
//
//==========================================================================

static void InvOff_f(void)
{
	inv_flg = false;
	scr_fullupdate = 0;
}

//==========================================================================
//
// ToggleDM_f
//
//==========================================================================

static void ToggleDM_f(void)
{
	DMMode->value += 1;
	if (DMMode->value > 2)
		DMMode->value = 0;
}

//==========================================================================
//
// SB_InvChanged
//
//==========================================================================

void SB_InvChanged(void)
{
	int counter, position;
	qboolean examined[INV_MAX_CNT];
	qboolean ForceUpdate = false;

	Com_Memset(examined,0,sizeof(examined)); // examined[x] = false

	if(cl.inv_selected >= 0 &&
	   (&cl.v.cnt_torch)[cl.inv_order[cl.inv_selected]] == 0)
		ForceUpdate = true;

	// removed items we no longer have from the order
	for(counter=position=0;counter<cl.inv_count;counter++)
	{
		//if (Inv_GetCount(cl.inv_order[counter]) >= 0)
		if((&cl.v.cnt_torch)[cl.inv_order[counter]] > 0)
		{
			cl.inv_order[position] = cl.inv_order[counter];
			examined[cl.inv_order[position]] = true;

			position++;
		}
	}

	// add in the new items
	for(counter=0;counter<INV_MAX_CNT;counter++)
		if (!examined[counter])
		{
			//if (Inv_GetCount(counter) > 0)
			if((&cl.v.cnt_torch)[counter] > 0)
			{
				cl.inv_order[position] = counter;
				position++;
			}
		}

	cl.inv_count = position;
	if (cl.inv_selected >= cl.inv_count) 
	{
		cl.inv_selected = cl.inv_count-1;
		ForceUpdate = true;
	}
	if (cl.inv_count && cl.inv_selected < 0) 
	{
		cl.inv_selected = 0;
		ForceUpdate = true;
	}
	if (ForceUpdate)
	{
		Inv_Update(true);
	}

	if (cl.inv_startpos+INV_MAX_ICON > cl.inv_count)
	{
		cl.inv_startpos = cl.inv_count - INV_MAX_ICON;
		if (cl.inv_startpos < 0) cl.inv_startpos = 0;
	}
}

//==========================================================================
//
// SB_InvReset
//
//==========================================================================

void SB_InvReset(void)
{
	cl.inv_count = cl.inv_startpos = 0;
	cl.inv_selected = -1;
	inv_flg = false;
	scr_fullupdate = 0;
}

//==========================================================================
//
// SB_ViewSizeChanged
//
//==========================================================================

void SB_ViewSizeChanged(void)
{
	if(cl.intermission || (scr_viewsize->value >= 110.0 && !sbtrans->value))
	{
		BarTargetHeight = 0.0-BAR_BUMP_HEIGHT;
	}
	else
	{
		BarTargetHeight = BAR_TOP_HEIGHT;
	}
}

// DRAWING FUNCTIONS *******************************************************

//==========================================================================
//
// Sbar_DrawPic
//
// Relative to the current status bar location.
//
//==========================================================================

static void Sbar_DrawPic(int x, int y, image_t *pic)
{
	Draw_PicCropped(x+((vid.width-320)>>1),
		y+(vid.height-(int)BarHeight), pic);
}

//==========================================================================
//
// Sbar_DrawSubPic
//
// Relative to the current status bar location.
//
//==========================================================================

static void Sbar_DrawSubPic(int x, int y, int h, image_t *pic)
{
	Draw_SubPicCropped(x+((vid.width-320)>>1),
		y+(vid.height-(int)BarHeight), h, pic);
}

//==========================================================================
//
// Sbar_DrawTransPic
//
// Relative to the current status bar location.
//
//==========================================================================

static void Sbar_DrawTransPic(int x, int y, image_t *pic)
{
	Draw_TransPicCropped(x+((vid.width-320)>>1),
		y+(vid.height-(int)BarHeight), pic);
}

//==========================================================================
//
// Sbar_DrawCharacter
//
//==========================================================================

static void Sbar_DrawCharacter(int x, int y, int num)
{
	Draw_Character(x+((vid.width-320)>>1)+4,
		y+vid.height-(int)BarHeight, num);
}

//==========================================================================
//
// Sbar_DrawString
//
//==========================================================================

static void Sbar_DrawString(int x, int y, char *str)
{
	Draw_String(x+((vid.width-320)>>1), y+vid.height-(int)BarHeight, str);
}

void Sbar_DrawRedString (int cx, int cy, char *str)
{
	while (*str)
	{
		Draw_Character (cx, cy, ((unsigned char)(*str))+256);
		str++;
		cx += 8;
	}
}

//==========================================================================
//
// Sbar_DrawSmallCharacter
//
//==========================================================================

static void Sbar_DrawSmallCharacter(int x, int y, int num)
{
	Draw_SmallCharacter(x+((vid.width-320)>>1)+4,
		y+vid.height-(int)BarHeight, num);
}

//==========================================================================
//
// Sbar_DrawSmallString
//
//==========================================================================

static void Sbar_DrawSmallString(int x, int y, char *str)
{
	Draw_SmallString(x+((vid.width-320)>>1),
		y+vid.height-(int)BarHeight, str);
}

//==========================================================================
//
// DrawBarArtifactNumber
//
//==========================================================================

static void DrawBarArtifactNumber(int x, int y, int number)
{
	static char artiNumName[18] = "gfx/artinum0.lmp";

	if(number >= 10)
	{
		artiNumName[11] = '0'+(number%100)/10;
		Sbar_DrawTransPic(x, y, Draw_CachePic(artiNumName));
	}
	artiNumName[11] = '0'+number%10;
	Sbar_DrawTransPic(x+4, y, Draw_CachePic(artiNumName));
}



// rjr

void Sbar_Changed(void)
{
	SB_InvChanged();
}

void Sbar_FinaleOverlay(void)
{
}

void Sbar_IntermissionOverlay(void)
{
}

