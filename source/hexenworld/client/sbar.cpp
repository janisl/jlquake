
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

#define STAT_MINUS 10	// num frame for '-' stats digit

#define BAR_TOP_HEIGHT              46.0
#define BAR_BOTTOM_HEIGHT           98.0
#define BAR_TOTAL_HEIGHT            (BAR_TOP_HEIGHT + BAR_BOTTOM_HEIGHT)
#define BAR_BUMP_HEIGHT             23.0
#define INVENTORY_DISPLAY_TIME      4
#define ABILITIES_STR_INDEX         400

#define RING_FLIGHT                 1
#define RING_WATER                  2
#define RING_REGENERATION           4
#define RING_TURNING                8

#define INV_MAX_CNT         15

#define INV_MAX_ICON        6		// Max number of inventory icons to display at once

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void Sbar_DeathmatchOverlay(void);
void Sbar_NormalOverlay(void);

void Sbar_SmallDeathmatchOverlay(void);
void M_DrawPic(int x, int y, image_t* pic);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void Sbar_DrawPic(int x, int y, image_t* pic);
static void Sbar_DrawSubPic(int x, int y, int h, image_t* pic);
static void Sbar_DrawTransPic(int x, int y, image_t* pic);
static int Sbar_itoa(int num, char* buf);
static void Sbar_DrawNum(int x, int y, int number, int digits);
void Sbar_SortFrags(qboolean includespec);
//static void Sbar_DrawCharacter(int x, int y, int num);
static void Sbar_DrawString(int x, int y, char* str);
static void Sbar_DrawRedString(int cx, int cy, const char* str);
static void Sbar_DrawSmallString(int x, int y, const char* str);
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

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int sb_lines;	// scan lines to draw

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static float BarHeight;
static float BarTargetHeight;
static Cvar* BarSpeed;
static Cvar* sbtemp;
static Cvar* DMMode;
static Cvar* sbtrans;

static image_t* sb_nums[11];
static image_t* sb_colon, * sb_slash;

static int fragsort[HWMAX_CLIENTS];

static int scoreboardlines;

static float ChainPosition = 0;

static qboolean sb_ShowInfo;
static qboolean sb_ShowDM;
static qboolean sb_ShowNames;

qboolean inv_flg;					// true - show inventory interface

static float InventoryHideTime;

static const char* PlayerClassNames[MAX_PLAYER_CLASS] =
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

static int trans_level = 0;

// CODE --------------------------------------------------------------------

//==========================================================================
//
// SB_Init
//
//==========================================================================

void Sbar_Init(void)
{
	int i;

	for (i = 0; i < 10; i++)
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
	for (int i = 0; i < HWMAX_CLIENTS; i++)
	{
		if ((cl.hw_PIV & (1 << i)))
		{
			if (!cl.h2_players[i].shownames_off)
			{
				if (cl_siege)
				{
					//why the fuck does GL fuck this up??!!!
					if (cl.h2_players[i].siege_team == ST_ATTACKER)	//attacker
					{
						if (i == cl_keyholder)
						{
							R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,10);
						}
						else
						{
							R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,false);
						}
					}
					else if (cl.h2_players[i].siege_team == ST_DEFENDER)//def
					{
						if (i == cl_keyholder && i == cl_doc)
						{
							R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,12);
						}
						else if (i == cl_keyholder)
						{
							R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,11);
						}
						else if (i == cl_doc)
						{
							R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,2);
						}
						else
						{
							R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,1);
						}
					}
					else
					{
						R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,3);
					}
				}
				else
				{
					R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,false);
				}
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

	if (con.displayFrac == 1)
	{	// console is full screen
		return;
	}

	if (cl.qh_spectator)
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

/*	if(BarHeight == BarTargetHeight)
    {
        return;
    }
*/

	if (BarHeight < BarTargetHeight)
	{
		delta = ((BarTargetHeight - BarHeight) * BarSpeed->value)
				* host_frametime;
		if (delta < 0.5)
		{
			delta = 0.5;
		}
		BarHeight += delta;
		if (BarHeight > BarTargetHeight)
		{
			BarHeight = BarTargetHeight;
		}
	}
	else if (BarHeight > BarTargetHeight)
	{
		delta = ((BarHeight - BarTargetHeight) * BarSpeed->value)
				* host_frametime;
		if (delta < 0.5)
		{
			delta = 0.5;
		}
		BarHeight -= delta;
		if (BarHeight < BarTargetHeight)
		{
			BarHeight = BarTargetHeight;
		}
	}

	if (BarHeight < 0)
	{
		DrawFullScreenInfo();
	}

	//Sbar_DrawPic(0, 0, R_CachePic("gfx/topbar.lmp"));
	Sbar_DrawPic(0, 0, R_CachePic("gfx/topbar1.lmp"));
	Sbar_DrawPic(160, 0, R_CachePic("gfx/topbar2.lmp"));
	Sbar_DrawTransPic(0, -23, R_CachePic("gfx/topbumpl.lmp"));
	Sbar_DrawTransPic(138, -8, R_CachePic("gfx/topbumpm.lmp"));
	Sbar_DrawTransPic(269, -23, R_CachePic("gfx/topbumpr.lmp"));

	maxMana = (int)cl.h2_v.max_mana;
	// Blue mana
	mana = (int)cl.h2_v.bluemana;
	if (mana < 0)
	{
		mana = 0;
	}
	if (mana > maxMana)
	{
		mana = maxMana;
	}
	sprintf(tempStr, "%03d", mana);
	Sbar_DrawSmallString(201, 22, tempStr);
	if (mana)
	{
		y = (int)((mana * 18.0) / (float)maxMana + 0.5);
		Sbar_DrawSubPic(190, 26 - y, y + 1,
			R_CachePic("gfx/bmana.lmp"));
		Sbar_DrawPic(190, 27, R_CachePic("gfx/bmanacov.lmp"));
	}

	// Green mana
	mana = (int)cl.h2_v.greenmana;
	if (mana < 0)
	{
		mana = 0;
	}
	if (mana > maxMana)
	{
		mana = maxMana;
	}
	sprintf(tempStr, "%03d", mana);
	Sbar_DrawSmallString(243, 22, tempStr);
	if (mana)
	{
		y = (int)((mana * 18.0) / (float)maxMana + 0.5);
		Sbar_DrawSubPic(232, 26 - y, y + 1,
			R_CachePic("gfx/gmana.lmp"));
		Sbar_DrawPic(232, 27, R_CachePic("gfx/gmanacov.lmp"));
	}

	// HP
	if (cl.h2_v.health < -99)
	{
		Sbar_DrawNum(58, 14, -99, 3);
	}
	else
	{
		Sbar_DrawNum(58, 14, cl.h2_v.health, 3);
	}
	SetChainPosition(cl.h2_v.health, cl.h2_v.max_health);
	Sbar_DrawTransPic(45 + ((int)ChainPosition & 7), 38,
		R_CachePic("gfx/hpchain.lmp"));
	Sbar_DrawTransPic(45 + (int)ChainPosition, 36,
		R_CachePic("gfx/hpgem.lmp"));
	Sbar_DrawPic(43, 36, R_CachePic("gfx/chnlcov.lmp"));
	Sbar_DrawPic(267, 36, R_CachePic("gfx/chnrcov.lmp"));

	// AC
	Sbar_DrawNum(105, 14, CalcAC(), 2);

	if (BarHeight > BAR_TOP_HEIGHT)
	{
		DrawLowerBar();
	}

	// Current inventory item
	if (cl.h2_inv_selected >= 0)
	{
		DrawBarArtifactIcon(144, 3, cl.h2_inv_order[cl.h2_inv_selected]);

		//Sbar_DrawTransPic(144, 3, R_CachePic(va("gfx/arti%02d.lmp",
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

	if (health < 0)
	{
		health = 0;
	}
	else if (health > maxHealth)
	{
		health = maxHealth;
	}
	chainTargetPosition = (health * 195) / maxHealth;
	if (fabs(ChainPosition - chainTargetPosition) < 0.1)
	{
		return false;
	}
	if (ChainPosition < chainTargetPosition)
	{
		delta = ((chainTargetPosition - ChainPosition) * 5) * host_frametime;
		if (delta < 0.5)
		{
			delta = 0.5;
		}
		ChainPosition += delta;
		if (ChainPosition > chainTargetPosition)
		{
			ChainPosition = chainTargetPosition;
		}
	}
	else if (ChainPosition > chainTargetPosition)
	{
		delta = ((ChainPosition - chainTargetPosition) * 5) * host_frametime;
		if (delta < 0.5)
		{
			delta = 0.5;
		}
		ChainPosition -= delta;
		if (ChainPosition < chainTargetPosition)
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

	y = BarHeight - 37;
	Sbar_DrawPic(3, y, R_CachePic("gfx/bmmana.lmp"));
	Sbar_DrawPic(3, y + 18, R_CachePic("gfx/gmmana.lmp"));

	maxMana = (int)cl.h2_v.max_mana;
	// Blue mana
	mana = (int)cl.h2_v.bluemana;
	if (mana < 0)
	{
		mana = 0;
	}
	if (mana > maxMana)
	{
		mana = maxMana;
	}
	sprintf(tempStr, "%03d", mana);
	Sbar_DrawSmallString(10, y + 6, tempStr);

	// Green mana
	mana = (int)cl.h2_v.greenmana;
	if (mana < 0)
	{
		mana = 0;
	}
	if (mana > maxMana)
	{
		mana = maxMana;
	}
	sprintf(tempStr, "%03d", mana);
	Sbar_DrawSmallString(10, y + 18 + 6, tempStr);

	// HP
	Sbar_DrawNum(38, y + 18, cl.h2_v.health, 3);

	// Current inventory item
	if (cl.h2_inv_selected >= 0)
	{
		DrawBarArtifactIcon(288, y + 7, cl.h2_inv_order[cl.h2_inv_selected]);
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
	char tempStr[80];
	int playerClass;
	int piece;
	int ringhealth;

	playerClass = cl.h2_players[cl.playernum].playerclass;
//	playerClass = playerclass->value;
	if (playerClass < 1 || playerClass > MAX_PLAYER_CLASS)
	{	// Default to paladin
		playerClass = 1;
	}

	// Backdrop
	//Sbar_DrawPic(0, 46, R_CachePic("gfx/btmbar.lmp"));
	Sbar_DrawPic(0, 46, R_CachePic("gfx/btmbar1.lmp"));
	Sbar_DrawPic(160, 46, R_CachePic("gfx/btmbar2.lmp"));

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
	Sbar_DrawSmallString(11, 48, PlayerClassNames[playerClass - 1]);

	Sbar_DrawSmallString(11, 58, "int");
	sprintf(tempStr, "%02d", (int)cl.h2_v.intelligence);
	Sbar_DrawSmallString(33, 58, tempStr);

	Sbar_DrawSmallString(11, 64, "wis");
	sprintf(tempStr, "%02d", (int)cl.h2_v.wisdom);
	Sbar_DrawSmallString(33, 64, tempStr);

	Sbar_DrawSmallString(11, 70, "dex");
	sprintf(tempStr, "%02d", (int)cl.h2_v.dexterity);
	Sbar_DrawSmallString(33, 70, tempStr);

	Sbar_DrawSmallString(58, 58, "str");
	sprintf(tempStr, "%02d", (int)cl.h2_v.strength);
	Sbar_DrawSmallString(80, 58, tempStr);

	Sbar_DrawSmallString(58, 64, "lvl");
	sprintf(tempStr, "%02d", (int)cl.h2_v.level);
	Sbar_DrawSmallString(80, 64, tempStr);

	Sbar_DrawSmallString(58, 70, "exp");
	sprintf(tempStr, "%06d", (int)cl.h2_v.experience);
	Sbar_DrawSmallString(80, 70, tempStr);

	// Abilities
	Sbar_DrawSmallString(11, 79, "abilities");
	i = AbilityLineIndex[(playerClass - 1)];
	if (i + 1 < pr_string_count)
	{
		if (((int)cl.h2_v.flags) & FL_SPECIAL_ABILITY1)
		{
			Sbar_DrawSmallString(8, 89,
				&pr_global_strings[pr_string_index[i]]);
		}
		if (((int)cl.h2_v.flags) & FL_SPECIAL_ABILITY2)
		{
			Sbar_DrawSmallString(8, 96,
				&pr_global_strings[pr_string_index[i + 1]]);
		}
	}

	// Portrait
	sprintf(tempStr, "gfx/cport%d.lmp", playerClass);
	Sbar_DrawPic(134, 50, R_CachePic(tempStr));

	// Armor
	if (cl.h2_v.armor_helmet > 0)
	{
		Sbar_DrawPic(164, 115, R_CachePic("gfx/armor1.lmp"));
		sprintf(tempStr, "+%d", (int)cl.h2_v.armor_helmet);
		Sbar_DrawSmallString(168, 136, tempStr);
	}
	if (cl.h2_v.armor_amulet > 0)
	{
		Sbar_DrawPic(205, 115, R_CachePic("gfx/armor2.lmp"));
		sprintf(tempStr, "+%d", (int)cl.h2_v.armor_amulet);
		Sbar_DrawSmallString(208, 136, tempStr);
	}
	if (cl.h2_v.armor_breastplate > 0)
	{
		Sbar_DrawPic(246, 115, R_CachePic("gfx/armor3.lmp"));
		sprintf(tempStr, "+%d", (int)cl.h2_v.armor_breastplate);
		Sbar_DrawSmallString(249, 136, tempStr);
	}
	if (cl.h2_v.armor_bracer > 0)
	{
		Sbar_DrawPic(285, 115, R_CachePic("gfx/armor4.lmp"));
		sprintf(tempStr, "+%d", (int)cl.h2_v.armor_bracer);
		Sbar_DrawSmallString(288, 136, tempStr);
	}

	// Rings
	if (cl.h2_v.ring_flight > 0)
	{
		Sbar_DrawTransPic(6, 119, R_CachePic("gfx/ring_f.lmp"));

		ringhealth = (int)cl.h2_v.ring_flight;
		if (ringhealth > 100)
		{
			ringhealth = 100;
		}
		Sbar_DrawPic(35 - (int)(26 * (ringhealth / (float)100)),142,R_CachePic("gfx/ringhlth.lmp"));
		Sbar_DrawPic(35, 142, R_CachePic("gfx/rhlthcvr.lmp"));
	}

	if (cl.h2_v.ring_water > 0)
	{
		Sbar_DrawTransPic(44, 119, R_CachePic("gfx/ring_w.lmp"));
		ringhealth = (int)cl.h2_v.ring_water;
		if (ringhealth > 100)
		{
			ringhealth = 100;
		}
		Sbar_DrawPic(73 - (int)(26 * (ringhealth / (float)100)),142,R_CachePic("gfx/ringhlth.lmp"));
		Sbar_DrawPic(73, 142, R_CachePic("gfx/rhlthcvr.lmp"));
	}

	if (cl.h2_v.ring_turning > 0)
	{
		Sbar_DrawTransPic(81, 119, R_CachePic("gfx/ring_t.lmp"));
		ringhealth = (int)cl.h2_v.ring_turning;
		if (ringhealth > 100)
		{
			ringhealth = 100;
		}
		Sbar_DrawPic(110 - (int)(26 * (ringhealth / (float)100)),142,R_CachePic("gfx/ringhlth.lmp"));
		Sbar_DrawPic(110, 142, R_CachePic("gfx/rhlthcvr.lmp"));
	}

	if (cl.h2_v.ring_regeneration > 0)
	{
		Sbar_DrawTransPic(119, 119, R_CachePic("gfx/ring_r.lmp"));
		ringhealth = (int)cl.h2_v.ring_regeneration;
		if (ringhealth > 100)
		{
			ringhealth = 100;
		}
		Sbar_DrawPic(148 - (int)(26 * (ringhealth / (float)100)),142,R_CachePic("gfx/ringhlth.lmp"));
		Sbar_DrawPic(148, 142, R_CachePic("gfx/rhlthcv2.lmp"));
	}

	// Puzzle pieces
	piece = 0;
	for (i = 0; i < 8; i++)
	{
		if (cl.h2_puzzle_pieces[i][0] == 0)
		{
			continue;
		}
		Sbar_DrawPic(194 + (piece % 4) * 31, piece < 4 ? 51 : 82,
			R_CachePic(va("gfx/puzzle/%s.lmp", cl.h2_puzzle_pieces[i])));
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

	//playerClass = cl.h2_v.playerclass;
	//BUG cl.h2_v.playerclass was never assigned and was always 0
	playerClass = 0;
	if (playerClass < 1 || playerClass > MAX_PLAYER_CLASS)
	{
		playerClass = 1;
	}
	a = 0;
	if (cl.h2_v.armor_amulet > 0)
	{
		a += AmuletAC[playerClass];
		a += cl.h2_v.armor_amulet / 5;
	}
	if (cl.h2_v.armor_bracer > 0)
	{
		a += BracerAC[playerClass];
		a += cl.h2_v.armor_bracer / 5;
	}
	if (cl.h2_v.armor_breastplate > 0)
	{
		a += BreastplateAC[playerClass];
		a += cl.h2_v.armor_breastplate / 5;
	}
	if (cl.h2_v.armor_helmet > 0)
	{
		a += HelmetAC[playerClass];
		a += cl.h2_v.armor_helmet / 5;
	}
	return a;
}

//==========================================================================
//
// Sbar_itoa
//
//==========================================================================

static int Sbar_itoa(int num, char* buf)
{
	char* str;
	int pow10;
	int dig;

	str = buf;
	if (num < 0)
	{
		*str++ = '-';
		num = -num;
	}

	for (pow10 = 10; num >= pow10; pow10 *= 10)
		;

	do
	{
		pow10 /= 10;
		dig = num / pow10;
		*str++ = '0' + dig;
		num -= dig * pow10;
	}
	while (pow10 != 1);

	*str = 0;

	return str - buf;
}

//==========================================================================
//
// Sbar_DrawNum
//
//==========================================================================

static void Sbar_DrawNum(int x, int y, int number, int digits)
{
	char str[12];
	char* ptr;
	int l, frame;

	l = Sbar_itoa(number, str);
	ptr = str;
	if (l > digits)
	{
		ptr += (l - digits);
	}
	if (l < digits)
	{
		x += ((digits - l) * 13) / 2;
	}

	while (*ptr)
	{
		if (*ptr == '-')
		{
			frame = STAT_MINUS;
		}
		else
		{
			frame = *ptr - '0';
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

void Sbar_SortFrags(qboolean includespec)
{
	int i, j, k;

// sort by frags
	scoreboardlines = 0;
	for (i = 0; i < HWMAX_CLIENTS; i++)
	{
		if (cl.h2_players[i].name[0] &&
			(!cl.h2_players[i].spectator || includespec))
		{
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
			if (cl.h2_players[i].spectator)
			{
				cl.h2_players[i].frags = SPEC_FRAGS;
			}
		}
	}

	for (i = 0; i < scoreboardlines; i++)
		for (j = 0; j < scoreboardlines - 1 - i; j++)
			if (cl.h2_players[fragsort[j]].frags < cl.h2_players[fragsort[j + 1]].frags)
			{
				k = fragsort[j];
				fragsort[j] = fragsort[j + 1];
				fragsort[j + 1] = k;
			}
}

int Sbar_ColorForMap(int m)
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
	char str[80];
	int minutes, seconds, tens, units;
	int l;

	sprintf(str,"Monsters:%3i /%3i", cl.qh_stats[STAT_MONSTERS], cl.qh_stats[STAT_TOTALMONSTERS]);
	Sbar_DrawString(8, 4, str);

	sprintf(str,"Secrets :%3i /%3i", cl.qh_stats[STAT_SECRETS], cl.qh_stats[STAT_TOTALSECRETS]);
	Sbar_DrawString(8, 12, str);

	// draw time
	minutes = cl.qh_serverTimeFloat / 60;
	seconds = cl.qh_serverTimeFloat - 60 * minutes;
	tens = seconds / 10;
	units = seconds - 10 * tens;
	sprintf(str,"Time :%3i:%i%i", minutes, tens, units);
	Sbar_DrawString(184, 4, str);

	// draw level name
	l = String::Length(cl.qh_levelname);
	Sbar_DrawString(232 - l * 4, 12, cl.qh_levelname);
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
    h2player_info_t	*s;

    Sbar_SortFrags ();

// draw the text
    l = scoreboardlines <= 4 ? scoreboardlines : 4;

    x = 23;
    xofs = (viddef.width - 320)>>1;
    y = viddef.height - BAR_TOP_HEIGHT - 23;

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

        UI_FillPal (xofs + x*8 + 10, y, 28, 4, top);
        UI_FillPal (xofs + x*8 + 10, y+4, 28, 3, bottom);

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

void Sbar_IntermissionNumber(int x, int y, int num, int digits, int color)
{
	char str[12];
	char* ptr;
	int l, frame;

	l = Sbar_itoa(num, str);
	ptr = str;
	if (l > digits)
	{
		ptr += (l - digits);
	}
	if (l < digits)
	{
		x += (digits - l) * 24;
	}

	while (*ptr)
	{
		if (*ptr == '-')
		{
			frame = STAT_MINUS;
		}
		else
		{
			frame = *ptr - '0';
		}

		UI_DrawPic(x,y,sb_nums[frame]);
		x += 24;
		ptr++;
	}
}

void FindColor(int slot, int* color1, int* color2)
{
	int j;
	int top, bottom, done;
	byte* sourceA, * sourceB, * colorA, * colorB;

	if (slot > HWMAX_CLIENTS)
	{
		Sys_Error("CL_NewTranslation: slot > cl.maxclients");
	}

	if (cl.h2_players[slot].playerclass <= 0 || cl.h2_players[slot].playerclass > MAX_PLAYER_CLASS)
	{
		*color1 = *color2 = 0;
		return;
	}

	top = cl.h2_players[slot].topColour;
	bottom = cl.h2_players[slot].bottomColour;

	if (top > 10)
	{
		top = 0;
	}
	if (bottom > 10)
	{
		bottom = 0;
	}

	top -= 1;
	bottom -= 1;

	colorA = playerTranslation + 256 + color_offsets[(int)cl.h2_players[slot].playerclass - 1];
	colorB = colorA + 256;
	sourceA = colorB + 256 + (top * 256);
	sourceB = colorB + 256 + (bottom * 256);
	done = 0;
	for (j = 0; j < 256; j++,colorA++,colorB++,sourceA++,sourceB++)
	{
		if ((*colorA != 255) && !(done & 1))
		{
			if (top >= 0)
			{
				*color1 = *sourceA;
			}
			else
			{
				*color1 = *colorA;
			}
			//	done |= 1;
		}
		if ((*colorB != 255) && !(done & 2))
		{
			if (bottom >= 0)
			{
				*color2 = *sourceB;
			}
			else
			{
				*color2 = *colorB;
			}
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
	image_t* pic;
	int i, k, l;
	int top, bottom;
	int x, y, f;
	char num[40];
	h2player_info_t* s;
	float total;
	int minutes;

	if (realtime - cl.qh_last_ping_request > 2)
	{
		cl.qh_last_ping_request = realtime;
		clc.netchan.message.WriteByte(h2clc_stringcmd);
		clc.netchan.message.WriteString2("pings");
	}

	pic = R_CachePic("gfx/menu/title8.lmp");
	M_DrawTransPic((320 - R_GetImageWidth(pic)) / 2, 0, pic);

// scores
	Sbar_SortFrags(true);

// draw the text
	l = scoreboardlines;

	x = ((viddef.width - 320) >> 1);//was 8 + ...
	y = 62;

	Sbar_DrawRedString(x + 4, y - 1, "FRAG");
	Sbar_DrawRedString(x + 48, y - 1, "PING");
	Sbar_DrawRedString(x + 88, y - 1, "TIME");
	Sbar_DrawRedString(x + 128, y - 1, "NAME");
	y += 12;

	for (i = 0; i < l; i++)
	{
		if (y + 10 >= (int)viddef.height)
		{
			break;
		}

		k = fragsort[i];
		s = &cl.h2_players[k];

		if (!s->name[0])
		{
			continue;
		}


		if (s->frags != SPEC_FRAGS)
		{
			switch (s->playerclass)
			{	//Print playerclass next to name
			case 0:
				Sbar_DrawRedString(x + 128, y, "(r   )");
				break;
			case 1:
				Sbar_DrawRedString(x + 128, y, "(P   )");
				break;
			case 2:
				Sbar_DrawRedString(x + 128, y, "(C   )");
				break;
			case 3:
				Sbar_DrawRedString(x + 128, y, "(N   )");
				break;
			case 4:
				Sbar_DrawRedString(x + 128, y, "(A   )");
				break;
			case 5:
				Sbar_DrawRedString(x + 128, y, "(S   )");
				break;
			case 6:
				Sbar_DrawRedString(x + 128, y, "(D   )");
				break;
			default:
				Sbar_DrawRedString(x + 128, y, "(?   )");
				break;
			}

			if (cl_siege)
			{
				Sbar_DrawRedString(x + 160, y, "6");
				if (s->siege_team == 1)
				{
					UI_DrawChar(x + 152, y, 143);//shield
				}
				else if (s->siege_team == 2)
				{
					UI_DrawChar(x + 152, y, 144);//sword
				}
				else
				{
					Sbar_DrawRedString(x + 152, y, "?");//no team

				}
				if (k == cl_keyholder)
				{
					UI_DrawChar(x + 144, y, 145);//key
				}
				else
				{
					Sbar_DrawRedString(x + 144, y, "-");
				}

				if (k == cl_doc)
				{
					UI_DrawChar(x + 160, y, 130);//crown
				}
				else
				{
					Sbar_DrawRedString(x + 160, y, "6");
				}

			}
			else
			{
				Sbar_DrawRedString(x + 144, y, "-");
				if (!s->level)
				{
					Sbar_DrawRedString(x + 152, y, "01");
				}
				else if ((int)s->level < 10)
				{
					Sbar_DrawRedString(x + 152, y, "0");
					sprintf(num, "%1d",s->level);
					Sbar_DrawRedString(x + 160, y, num);
				}
				else
				{
					sprintf(num, "%2d",s->level);
					Sbar_DrawRedString(x + 152, y, num);
				}
			}

			// draw background
			FindColor(k, &top, &bottom);

			UI_FillPal(x + 8, y, 28, 4, top);	//was x to 40
			UI_FillPal(x + 8, y + 4, 28, 4, bottom);//was x to 40

			// draw number
			f = s->frags;
			sprintf(num, "%3i",f);

			UI_DrawString(x + 10, y - 1, num);	//was 8

			if (k == cl.playernum)
			{
				UI_DrawChar(x, y - 1, 13);
			}

//rjr			if(k==sv_kingofhill)
//rjr				UI_DrawChar ( x+40 , y-1, 130);
			sprintf(num, "%4d",s->ping);
			UI_DrawString(x + 48, y, num);

			if (cl.qh_intermission)
			{
				total = cl.qh_completed_time - s->entertime;
			}
			else
			{
				total = realtime - s->entertime;
			}
			minutes = (int)total / 60;
			sprintf(num, "%4i", minutes);
			UI_DrawString(x + 88, y, num);

			// draw name
			if (cl_siege && s->siege_team == 2)	//attacker
			{
				UI_DrawString(x + 178, y, s->name);
			}
			else
			{
				Sbar_DrawRedString(x + 178, y, s->name);
			}
		}
		else
		{
			sprintf(num, "%4d",s->ping);
			Sbar_DrawRedString(x + 48, y, num);

			if (cl.qh_intermission)
			{
				total = cl.qh_completed_time - s->entertime;
			}
			else
			{
				total = realtime - s->entertime;
			}
			minutes = (int)total / 60;
			sprintf(num, "%4i", minutes);
			Sbar_DrawRedString(x + 88, y, num);

			// draw name
			Sbar_DrawRedString(x + 128, y, s->name);
		}

		y += 10;
	}
}

void FindName(char* which, char* name)
{
	int j,length;
	char* pos,* p2;
	char test[40];

	String::Cpy(name, "Unknown");
	j = atol(puzzle_strings);
	pos = strchr(puzzle_strings,13);
	if (!pos)
	{
		return;
	}

	if ((*pos) == 10 || (*pos) == 13)
	{
		pos++;
	}
	if ((*pos) == 10 || (*pos) == 13)
	{
		pos++;
	}

	length = String::Length(which);

	for (; j; j--)
	{
		p2 = strchr(pos,' ');
		if (!p2)
		{
			return;
		}

		String::NCpy(test,pos,p2 - pos);
		test[p2 - pos] = 0;

		if (String::ICmp(which,test) == 0)
		{
			pos = strchr(pos,' ');
			if (pos)
			{
				pos++;
				p2 = strchr(pos,13);
				if (p2)
				{
					String::NCpy(name,pos,p2 - pos);
					name[p2 - pos] = 0;
					return;
				}
			}
			return;
		}
		pos = strchr(pos,13);
		if (!pos)
		{
			return;
		}
		if ((*pos) == 10 || (*pos) == 13)
		{
			pos++;
		}
		if ((*pos) == 10 || (*pos) == 13)
		{
			pos++;
		}
	}
}

//==========================================================================
//
// Sbar_NormalOverlay
//
//==========================================================================

void Sbar_NormalOverlay(void)
{
	int i,y,piece;
	char Name[40];

	piece = 0;
	y = 40;
	for (i = 0; i < 8; i++)
	{
		if (cl.h2_puzzle_pieces[i][0] == 0)
		{
			continue;
		}

		if (piece == 4)
		{
			y = 40;
		}

		FindName(cl.h2_puzzle_pieces[i],Name);

		if (piece < 4)
		{
			M_DrawPic(10, y,
				R_CachePic(va("gfx/puzzle/%s.lmp", cl.h2_puzzle_pieces[i])));
			M_PrintWhite(45, y, Name);
		}
		else
		{
			M_DrawPic(310 - 32, y,
				R_CachePic(va("gfx/puzzle/%s.lmp", cl.h2_puzzle_pieces[i])));
			M_PrintWhite(310 - 32 - 3 - (String::Length(Name) * 8), 18 + y, Name);
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

void DrawTime(int x, int y, int disp_time)
{
	int show_min,show_sec;
	char num[40];
	show_min = disp_time;
	show_sec = show_min % 60;
	show_min = (show_min - show_sec) / 60;
	sprintf(num, "%3d",show_min);
	Sbar_DrawRedString(x + 8, y, num);
//	UI_DrawChar ( x+32 , y, 58);// ":"
	sprintf(num, "%2d",show_sec);
	if (show_sec >= 10)
	{
		Sbar_DrawRedString(x + 40, y, num);
	}
	else
	{
		Sbar_DrawRedString(x + 40, y, num);
		Sbar_DrawRedString(x + 40, y, "0");
	}
}

void Sbar_SmallDeathmatchOverlay(void)
{
	int i, k, l;
	int top, bottom;
	int x, y, f;
	int def_frags,att_frags;
	char num[40];
//	unsigned char	num[12];
	h2player_info_t* s;

	if (DMMode->value >= 2 && BarHeight != BAR_TOP_HEIGHT)
	{
		return;
	}

	trans_level = (int)((DMMode->value - floor(DMMode->value) + 1E-3) * 10);
	if (trans_level > 2)
	{
		trans_level = 0;
	}

// scores
	Sbar_SortFrags(false);

// draw the text
	l = scoreboardlines;

	x = 10;
	if ((int)DMMode->value == 1)
	{
		i = (viddef.height - 120) / 10;

		if (l > i)
		{
			l = i;
		}

		y = 46;
	}
	else if ((int)DMMode->value == 2)
	{
		if (l > 4)
		{
			l = 4;
		}
		y = viddef.height - BAR_TOP_HEIGHT;
	}
	if (cl_siege)
	{
		if (l > i)
		{
			l = i;
		}
		y = viddef.height - BAR_TOP_HEIGHT - 24;
		x -= 4;
		UI_DrawChar(x, y, 142);	//sundial
		UI_DrawChar(x + 32, y, 58);	// ":"
		if (cl_timelimit > cl.qh_serverTimeFloat + cl_server_time_offset)
		{
			DrawTime(x,y,(int)(cl_timelimit - (cl.qh_serverTimeFloat + cl_server_time_offset)));
		}
		else if ((int)cl.qh_serverTimeFloat % 2)
		{	//odd number, draw 00:00, this will make it flash every second
			Sbar_DrawRedString(x + 16, y, "00");
			Sbar_DrawRedString(x + 40, y, "00");
		}
	}

	def_frags = att_frags = 0;
	for (i = 0; i < l; i++)
	{
		k = fragsort[i];
		s = &cl.h2_players[k];
		if (!s->name[0])
		{
			continue;
		}

		if (cl_siege)
		{
			if (s->siege_team == 1)
			{	//defender
				if (s->frags > 0)
				{
					def_frags += s->frags;
				}
				else
				{
					att_frags -= s->frags;
				}
			}
			else if (s->siege_team == 2)
			{	//attacker
				if (s->frags > 0)
				{
					att_frags += s->frags;
				}
				else
				{
					def_frags -= s->frags;
				}
			}
		}
		else
		{
			if ((int)DMMode->value == 2)
			{
			}
			// draw background
			FindColor(k, &top, &bottom);

			UI_FillPal(x, y, 28, 4, top);
			UI_FillPal(x, y + 4, 28, 4, bottom);

			// draw number
			f = s->frags;
			sprintf(num, "%3i",f);

			if (k == cl.playernum)
			{
				UI_DrawChar(x - 8, y - 1, 13);
			}
			UI_DrawString(x + 2, y - 1, num);
			y += 10;
		}
	}
	if (cl_siege)
	{
		if (cl.playernum == cl_keyholder)
		{
			UI_DrawChar(x, y - 10, 145);	//key
		}
		if (cl.playernum == cl_doc)
		{
			UI_DrawChar(x + 8, y - 10, 130);	//crown
		}
		if ((int)cl.h2_v.artifact_active & HWARTFLAG_BOOTS)
		{
			UI_DrawChar(x + 16, y - 10, 146);//boots

		}
		//Print defender losses
		UI_DrawChar(x, y + 10, 143);	//shield
		sprintf(num, "%3i",defLosses);
//		sprintf (num, "%3i",att_frags);
		Sbar_DrawRedString(x + 8, y + 10, num);
		UI_DrawChar(x + 32, y + 10, 47);	// "/"
		sprintf(num, "%3i",cl_fraglimit);
		Sbar_DrawRedString(x + 40, y + 10, num);

		//Print attacker losses
		UI_DrawChar(x, y + 20, 144);	//sword
		sprintf(num, "%3i",attLosses);
//		sprintf (num, "%3i",def_frags);
		Sbar_DrawRedString(x + 8, y + 20, num);
		UI_DrawChar(x + 32, y + 20, 47);	// "/"
		sprintf(num, "%3i",cl_fraglimit * 2);
		Sbar_DrawRedString(x + 40, y + 20, num);
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

	if (con.displayFrac == 1)
	{
		return;		// console is full screen

	}
	ring_row = 1;

	flag = (int)cl.h2_v.rings_active;

	if (flag & RING_TURNING)
	{
		frame = 1 + (int)(cl.qh_serverTimeFloat * 16) % 16;
		sprintf(tempStr, "gfx/rngtrn%d.lmp", frame);
		UI_DrawPic(viddef.width - 50, ring_row, R_CachePic(tempStr));
		ring_row += 33;
	}

/*	if(flag&RING_REGENERATION)
    {
        frame = 1+(int)(cl.time*16)%16;
        sprintf(tempStr, "gfx/rngreg%d.lmp", frame);
        UI_DrawPic(viddef.width - 50, ring_row, R_CachePic(tempStr));
        ring_row += 33;
    }
*/

	if (flag & RING_WATER)
	{
		frame = 1 + (int)(cl.qh_serverTimeFloat * 16) % 16;
		sprintf(tempStr, "gfx/rngwtr%d.lmp", frame);
		UI_DrawPic(viddef.width - 50, ring_row, R_CachePic(tempStr));
		ring_row += 33;
	}

	if (flag & RING_FLIGHT)
	{
		frame = 1 + (int)(cl.qh_serverTimeFloat * 16) % 16;
		sprintf(tempStr, "gfx/rngfly%d.lmp", frame);
		UI_DrawPic(viddef.width - 50, ring_row, R_CachePic(tempStr));
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

	if (con.displayFrac == 1)
	{
		return;
	}

	art_col = 50;

	if (ring_row != 1)
	{
		art_col += 50;
	}

	flag = (int)cl.h2_v.artifact_active;
	if (flag & HWARTFLAG_TOMEOFPOWER)
	{
		frame = 1 + (int)(cl.qh_serverTimeFloat * 16) % 16;
		sprintf(tempStr, "gfx/pwrbook%d.lmp", frame);
		UI_DrawPic(viddef.width - art_col, 1, R_CachePic(tempStr));
		art_col += 50;
	}

	if (flag & HWARTFLAG_HASTE)
	{
		frame = 1 + (int)(cl.qh_serverTimeFloat * 16) % 16;
		sprintf(tempStr, "gfx/durhst%d.lmp", frame);
		UI_DrawPic(viddef.width - art_col,1, R_CachePic(tempStr));
		art_col += 50;
	}

	if (flag & HWARTFLAG_INVINCIBILITY)
	{
		frame = 1 + (int)(cl.qh_serverTimeFloat * 16) % 16;
		sprintf(tempStr, "gfx/durshd%d.lmp", frame);
		UI_DrawPic(viddef.width - art_col, 1, R_CachePic(tempStr));
		art_col += 50;
	}

	oldflags = flag;
}

//============================================================================


void Inv_Update(qboolean force)
{
	if (inv_flg || force)
	{
		// Just to be safe
		if (cl.h2_inv_selected >= 0 && cl.h2_inv_count > 0)
		{
			cl.h2_v.inventory = cl.h2_inv_order[cl.h2_inv_selected] + 1;
		}
		else
		{
			cl.h2_v.inventory = 0;
		}

		if (!force)
		{
			inv_flg = false;	// Toggle menu off
		}

		if (cls.state == CA_ACTIVE)
		{	// This will cause the server to set the client's edict's inventory value
			clc.netchan.message.WriteByte(hwclc_inv_select);
			clc.netchan.message.WriteByte(cl.h2_v.inventory);
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

	Sbar_DrawTransPic(x, y, R_CachePic(va("gfx/arti%02d.lmp",
				artifact)));
//	if((count = (int)(&cl.h2_v.cnt_torch)[artifact]) > 1)
	if ((count = (int)(&cl.h2_v.cnt_torch)[artifact]) > 0)
	{
		DrawBarArtifactNumber(x + 20, y + 21, count);
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

	if (InventoryHideTime < cl.qh_serverTimeFloat)
	{
		Inv_Update(false);
		return;
	}
	if (!inv_flg)
	{
		return;
	}
	if (!cl.h2_inv_count)
	{
		Inv_Update(false);
		return;
	}

	//SB_InvChanged();

	if (BarHeight < 0)
	{
		y = BarHeight - 34;
	}
	else
	{
		y = -37;
	}
	for (i = 0, x = 64; i < INV_MAX_ICON; i++, x += 33)
	{
		if (cl.h2_inv_startpos + i >= cl.h2_inv_count)
		{
			break;
		}
		if (cl.h2_inv_startpos + i == cl.h2_inv_selected)
		{	// Highlight icon
			Sbar_DrawTransPic(x + 9, y - 12, R_CachePic("gfx/artisel.lmp"));
		}
		DrawBarArtifactIcon(x, y, cl.h2_inv_order[cl.h2_inv_startpos + i]);
	}

	//Inv_DrawArrows (x, y);

/*	if (cl.inv_startpos)  // Left arrow showing there are icons to the left
        UI_FillPal ( x , y - 5, 3, 1, 30);

    if (cl.inv_startpos + INV_MAX_ICON < cl.inv_count) // Right arrow showing there are icons to the right
        UI_FillPal ( x + 200, y - 5 , 3, 1, 30);
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
	if (sb_ShowInfo || cl.qh_intermission)
	{
		return;
	}
	S_StartLocalSound("misc/barmovup.wav");
	BarTargetHeight = BAR_TOTAL_HEIGHT;
	sb_ShowInfo = true;
}

//==========================================================================
//
// ShowInfoUp_f
//
//==========================================================================

static void ShowInfoUp_f(void)
{
	if (cl.qh_intermission || (scr_viewsize->value >= 110.0 && !sbtrans->value))
	{
		BarTargetHeight = 0.0 - BAR_BUMP_HEIGHT;
	}
	else
	{
		BarTargetHeight = BAR_TOP_HEIGHT;
	}
	S_StartLocalSound("misc/barmovdn.wav");
	sb_ShowInfo = false;
}

//==========================================================================
//
// InvLeft_f
//
//==========================================================================

static void InvLeft_f(void)
{
	if (!cl.h2_inv_count || cl.qh_intermission)
	{
		return;
	}

	if (inv_flg)
	{
		if (cl.h2_inv_selected > 0)
		{
			cl.h2_inv_selected--;
			if (cl.h2_inv_selected < cl.h2_inv_startpos)
			{
				cl.h2_inv_startpos = cl.h2_inv_selected;
			}
		}
	}
	else
	{
		inv_flg = true;
	}
	S_StartLocalSound("misc/invmove.wav");
	InventoryHideTime = cl.qh_serverTimeFloat + INVENTORY_DISPLAY_TIME;
}

//==========================================================================
//
// InvRight_f
//
//==========================================================================

static void InvRight_f(void)
{
	if (!cl.h2_inv_count || cl.qh_intermission)
	{
		return;
	}

	if (inv_flg)
	{
		if (cl.h2_inv_selected < cl.h2_inv_count - 1)
		{
			cl.h2_inv_selected++;
			if (cl.h2_inv_selected - cl.h2_inv_startpos >= INV_MAX_ICON)
			{
				// could probably be just a cl.inv_startpos++, but just in case
				cl.h2_inv_startpos = cl.h2_inv_selected - INV_MAX_ICON + 1;
			}
		}
	}
	else
	{
		inv_flg = true;
	}
	S_StartLocalSound("misc/invmove.wav");
	InventoryHideTime = cl.qh_serverTimeFloat + INVENTORY_DISPLAY_TIME;
}

//==========================================================================
//
// InvUse_f
//
//==========================================================================

static void InvUse_f(void)
{
	if (!cl.h2_inv_count || cl.qh_intermission)
	{
		return;
	}
	S_StartLocalSound("misc/invuse.wav");
	//Inv_Update(false);
	Inv_Update(true);
	inv_flg = false;
	in_impulse = 23;
}

//==========================================================================
//
// InvDrop_f
//
//==========================================================================

static void InvDrop_f(void)
{
	if (!cl.h2_inv_count || cl.qh_intermission)
	{
		return;
	}
	S_StartLocalSound("misc/invuse.wav");
	//Inv_Update(false);
	Inv_Update(true);
	inv_flg = false;
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
	{
		DMMode->value = 0;
	}
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

	Com_Memset(examined,0,sizeof(examined));// examined[x] = false

	if (cl.h2_inv_selected >= 0 &&
		(&cl.h2_v.cnt_torch)[cl.h2_inv_order[cl.h2_inv_selected]] == 0)
	{
		ForceUpdate = true;
	}

	// removed items we no longer have from the order
	for (counter = position = 0; counter < cl.h2_inv_count; counter++)
	{
		//if (Inv_GetCount(cl.inv_order[counter]) >= 0)
		if ((&cl.h2_v.cnt_torch)[cl.h2_inv_order[counter]] > 0)
		{
			cl.h2_inv_order[position] = cl.h2_inv_order[counter];
			examined[cl.h2_inv_order[position]] = true;

			position++;
		}
	}

	// add in the new items
	for (counter = 0; counter < INV_MAX_CNT; counter++)
		if (!examined[counter])
		{
			//if (Inv_GetCount(counter) > 0)
			if ((&cl.h2_v.cnt_torch)[counter] > 0)
			{
				cl.h2_inv_order[position] = counter;
				position++;
			}
		}

	cl.h2_inv_count = position;
	if (cl.h2_inv_selected >= cl.h2_inv_count)
	{
		cl.h2_inv_selected = cl.h2_inv_count - 1;
		ForceUpdate = true;
	}
	if (cl.h2_inv_count && cl.h2_inv_selected < 0)
	{
		cl.h2_inv_selected = 0;
		ForceUpdate = true;
	}
	if (ForceUpdate)
	{
		Inv_Update(true);
	}

	if (cl.h2_inv_startpos + INV_MAX_ICON > cl.h2_inv_count)
	{
		cl.h2_inv_startpos = cl.h2_inv_count - INV_MAX_ICON;
		if (cl.h2_inv_startpos < 0)
		{
			cl.h2_inv_startpos = 0;
		}
	}
}

//==========================================================================
//
// SB_InvReset
//
//==========================================================================

void SB_InvReset(void)
{
	cl.h2_inv_count = cl.h2_inv_startpos = 0;
	cl.h2_inv_selected = -1;
	inv_flg = false;
}

//==========================================================================
//
// SB_ViewSizeChanged
//
//==========================================================================

void SB_ViewSizeChanged(void)
{
	if (cl.qh_intermission || (scr_viewsize->value >= 110.0 && !sbtrans->value))
	{
		BarTargetHeight = 0.0 - BAR_BUMP_HEIGHT;
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

static void Sbar_DrawPic(int x, int y, image_t* pic)
{
	UI_DrawPic(x + ((viddef.width - 320) >> 1),
		y + (viddef.height - (int)BarHeight), pic);
}

//==========================================================================
//
// Sbar_DrawSubPic
//
// Relative to the current status bar location.
//
//==========================================================================

static void Sbar_DrawSubPic(int x, int y, int h, image_t* pic)
{
	UI_DrawStretchPic(x + ((viddef.width - 320) >> 1),
		y + (viddef.height - (int)BarHeight), R_GetImageWidth(pic), h, pic);
}

//==========================================================================
//
// Sbar_DrawTransPic
//
// Relative to the current status bar location.
//
//==========================================================================

static void Sbar_DrawTransPic(int x, int y, image_t* pic)
{
	UI_DrawPic(x + ((viddef.width - 320) >> 1),
		y + (viddef.height - (int)BarHeight), pic);
}

//==========================================================================
//
// Sbar_DrawCharacter
//
//==========================================================================

/*static void Sbar_DrawCharacter(int x, int y, int num)
{
    UI_DrawChar(x+((viddef.width-320)>>1)+4,
        y+viddef.height-(int)BarHeight, num);
}*/

//==========================================================================
//
// Sbar_DrawString
//
//==========================================================================

static void Sbar_DrawString(int x, int y, char* str)
{
	UI_DrawString(x + ((viddef.width - 320) >> 1), y + viddef.height - (int)BarHeight, str);
}

void Sbar_DrawRedString(int cx, int cy, const char* str)
{
	UI_DrawString(cx, cy, str, 256);
}

//==========================================================================
//
// Sbar_DrawSmallString
//
//==========================================================================

static void Sbar_DrawSmallString(int x, int y, const char* str)
{
	Draw_SmallString(x + ((viddef.width - 320) >> 1),
		y + viddef.height - (int)BarHeight, str);
}

//==========================================================================
//
// DrawBarArtifactNumber
//
//==========================================================================

static void DrawBarArtifactNumber(int x, int y, int number)
{
	static char artiNumName[18] = "gfx/artinum0.lmp";

	if (number >= 10)
	{
		artiNumName[11] = '0' + (number % 100) / 10;
		Sbar_DrawTransPic(x, y, R_CachePic(artiNumName));
	}
	artiNumName[11] = '0' + number % 10;
	Sbar_DrawTransPic(x + 4, y, R_CachePic(artiNumName));
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
