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

#include "../../client_main.h"
#include "../../ui/ui.h"
#include "../../ui/console.h"
#include "local.h"
#include "../../../common/hexen2strings.h"
#include "../quake_hexen2/menu.h"
#include "../../../server/public.h"

#define STAT_MINUS 10	// num frame for '-' stats digit

#define BAR_TOP_HEIGHT              46.0
#define BAR_BOTTOM_HEIGHT           98.0
#define BAR_TOTAL_HEIGHT            (BAR_TOP_HEIGHT + BAR_BOTTOM_HEIGHT)
#define BAR_BUMP_HEIGHT             23.0
#define INVENTORY_DISPLAY_TIME      4

#define RING_FLIGHT                 1
#define RING_WATER                  2
#define RING_REGENERATION           4
#define RING_TURNING                8

#define INV_MAX_CNT         15

#define INV_MAX_ICON        6		// Max number of inventory icons to display at once

#define SPEC_FRAGS -9999

static float BarHeight;

static Cvar* BarSpeed;
static Cvar* DMMode;
static Cvar* sbtrans;

static image_t* sbh2_nums[11];

static int sbh2_fragsort[BIGGEST_MAX_CLIENTS_QH];
static int sbh2_scoreboardlines;

static float sbh2_ChainPosition = 0;

static bool sbh2_ShowInfo;
static bool sbh2_ShowDM;
static bool sbh2_ShowNames;

static bool sbh2_inv_flg;					// true - show inventory interface

static float sbh2_InventoryHideTime;

static int ring_row;

static const int AmuletAC[MAX_PLAYER_CLASS] =
{
	8,		// Paladin
	4,		// Crusader
	2,		// Necromancer
	6,		// Assassin
	6,		// Demoness
	0		// Dwarf
};

static const int BracerAC[MAX_PLAYER_CLASS] =
{
	6,		// Paladin
	8,		// Crusader
	4,		// Necromancer
	2,		// Assassin
	2,		// Demoness
	10		// Demoness
};

static const int BreastplateAC[MAX_PLAYER_CLASS] =
{
	2,		// Paladin
	6,		// Crusader
	8,		// Necromancer
	4,		// Assassin
	4,		// Demoness
	12		// Dwarf
};

static const int HelmetAC[MAX_PLAYER_CLASS] =
{
	4,		// Paladin
	2,		// Crusader
	6,		// Necromancer
	8,		// Assassin
	8,		// Demoness
	10		// Dwarf
};

static const int AbilityLineIndex[MAX_PLAYER_CLASS] =
{
	400,		// Paladin
	402,		// Crusader
	404,		// Necromancer
	406,		// Assassin
	590,		// Demoness
	594			// Dwarf
};

static void ShowInfoDown_f()
{
	if (sbh2_ShowInfo || cl.qh_intermission)
	{
		return;
	}
	S_StartLocalSound("misc/barmovup.wav");
	sbh2_ShowInfo = true;
}

static void ShowInfoUp_f()
{
	S_StartLocalSound("misc/barmovdn.wav");
	sbh2_ShowInfo = false;
}

static void ShowDMDown_f()
{
	sbh2_ShowDM = true;
}

static void ShowDMUp_f()
{
	sbh2_ShowDM = false;
}

static void ShowNamesDown_f()
{
	sbh2_ShowNames = true;
}

static void ShowNamesUp_f()
{
	sbh2_ShowNames = false;
}

static void Inv_Update(bool force)
{
	if (sbh2_inv_flg || force)
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
			sbh2_inv_flg = false;	// Toggle menu off
		}

		if (!(GGameType & GAME_HexenWorld) || cls.state == CA_ACTIVE)
		{
			// This will cause the server to set the client's edict's inventory value
			clc.netchan.message.WriteByte(GGameType & GAME_HexenWorld ? hwclc_inv_select : h2clc_inv_select);
			clc.netchan.message.WriteByte(cl.h2_v.inventory);
		}
	}
}

static void InvLeft_f()
{
	if (!cl.h2_inv_count || cl.qh_intermission)
	{
		return;
	}

	if (sbh2_inv_flg)
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
		sbh2_inv_flg = true;
	}
	S_StartLocalSound("misc/invmove.wav");
	sbh2_InventoryHideTime = cl.qh_serverTimeFloat + INVENTORY_DISPLAY_TIME;
}

static void InvRight_f()
{
	if (!cl.h2_inv_count || cl.qh_intermission)
	{
		return;
	}

	if (sbh2_inv_flg)
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
		sbh2_inv_flg = true;
	}
	S_StartLocalSound("misc/invmove.wav");
	sbh2_InventoryHideTime = cl.qh_serverTimeFloat + INVENTORY_DISPLAY_TIME;
}

static void InvUse_f()
{
	if (!cl.h2_inv_count || cl.qh_intermission)
	{
		return;
	}
	S_StartLocalSound("misc/invuse.wav");
	Inv_Update(true);
	sbh2_inv_flg = false;
	in_impulse = 23;
}

static void InvDrop_f()
{
	if (!cl.h2_inv_count || cl.qh_intermission)
	{
		return;
	}
	S_StartLocalSound("misc/invuse.wav");
	Inv_Update(true);
	sbh2_inv_flg = false;
	in_impulse = 44;
}

static void InvOff_f()
{
	sbh2_inv_flg = false;
}

static void ToggleDM_f()
{
	DMMode->value += 1;
	if (DMMode->value > 2)
	{
		DMMode->value = 0;
	}
}

void SbarH2_Init()
{
	Cmd_AddCommand("+showinfo", ShowInfoDown_f);
	Cmd_AddCommand("-showinfo", ShowInfoUp_f);
	Cmd_AddCommand("+showdm", ShowDMDown_f);
	Cmd_AddCommand("-showdm", ShowDMUp_f);
	if (GGameType & GAME_HexenWorld)
	{
		Cmd_AddCommand("+shownames", ShowNamesDown_f);
		Cmd_AddCommand("-shownames", ShowNamesUp_f);
	}
	Cmd_AddCommand("invleft", InvLeft_f);
	Cmd_AddCommand("invright", InvRight_f);
	Cmd_AddCommand("invuse", InvUse_f);
	if (GGameType & GAME_HexenWorld)
	{
		Cmd_AddCommand("invdrop", InvDrop_f);
	}
	Cmd_AddCommand("invoff", InvOff_f);
	Cmd_AddCommand("toggle_dm", ToggleDM_f);

	BarSpeed = Cvar_Get("barspeed", "5", 0);
	DMMode = Cvar_Get("dm_mode", "1", CVAR_ARCHIVE);
	sbtrans = Cvar_Get("sbtrans", "0", CVAR_ARCHIVE);
	BarHeight = BAR_TOP_HEIGHT;
}

void SbarH2_InitImages()
{
	for (int i = 0; i < 10; i++)
	{
		sbh2_nums[i] = R_PicFromWad(va("num_%i",i));
	}
	sbh2_nums[10] = R_PicFromWad("num_minus");
}

// Relative to the current status bar location.
static void SbarH2_DrawPic(int x, int y, image_t* pic)
{
	UI_DrawPic(x + ((viddef.width - 320) >> 1),
		y + (viddef.height - (int)BarHeight), pic);
}

static void SbarH2_DrawSubPic(int x, int y, int h, image_t* pic)
{
	UI_DrawStretchPic(x + ((viddef.width - 320) >> 1),
		y + (viddef.height - (int)BarHeight), R_GetImageWidth(pic), h, pic);
}

static void SbarH2_DrawSmallString(int x, int y, const char* str)
{
	UI_DrawSmallString(x + ((viddef.width - 320) >> 1), y + viddef.height - (int)BarHeight, str);
}

static void DrawBarArtifactNumber(int x, int y, int number)
{
	static char artiNumName[18] = "gfx/artinum0.lmp";

	if (number >= 10)
	{
		artiNumName[11] = '0' + (number % 100) / 10;
		SbarH2_DrawPic(x, y, R_CachePic(artiNumName));
	}
	artiNumName[11] = '0' + number % 10;
	SbarH2_DrawPic(x + 4, y, R_CachePic(artiNumName));
}

static void DrawBarArtifactIcon(int x, int y, int artifact)
{
	if ((artifact < 0) || (artifact > 14))
	{
		return;
	}
	SbarH2_DrawPic(x, y, R_CachePic(va("gfx/arti%02d.lmp", artifact)));
	int count = (int)(&cl.h2_v.cnt_torch)[artifact];
	if (count > 0)
	{
		DrawBarArtifactNumber(x + 20, y + 21, count);
	}
}

static void SbarH2_DrawNum(int x, int y, int number, int digits)
{
	char str[12];
	int l = SbarQH_itoa(number, str);
	char* ptr = str;
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
		int frame;
		if (*ptr == '-')
		{
			frame = STAT_MINUS;
		}
		else
		{
			frame = *ptr - '0';
		}
		SbarH2_DrawPic(x, y, sbh2_nums[frame]);
		x += 13;
		ptr++;
	}
}

static void UpdateHeight()
{
	float BarTargetHeight;
	if (sbh2_ShowInfo && !cl.qh_intermission)
	{
		BarTargetHeight = BAR_TOTAL_HEIGHT;
	}
	else if (cl.qh_intermission || (scr_viewsize->value >= 110.0 && !sbtrans->value))
	{
		BarTargetHeight = 0.0 - BAR_BUMP_HEIGHT;
	}
	else
	{
		BarTargetHeight = BAR_TOP_HEIGHT;
	}

	if (BarHeight < BarTargetHeight)
	{
		float delta = ((BarTargetHeight - BarHeight) * BarSpeed->value)
				* cls.frametime * 0.001;
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
		float delta = ((BarHeight - BarTargetHeight) * BarSpeed->value)
				* cls.frametime * 0.001;
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
}

static void DrawFullScreenInfo()
{
	int y = BarHeight - 37;
	SbarH2_DrawPic(3, y, R_CachePic("gfx/bmmana.lmp"));
	SbarH2_DrawPic(3, y + 18, R_CachePic("gfx/gmmana.lmp"));

	int maxMana = (int)cl.h2_v.max_mana;
	// Blue mana
	int mana = (int)cl.h2_v.bluemana;
	if (mana < 0)
	{
		mana = 0;
	}
	if (mana > maxMana)
	{
		mana = maxMana;
	}
	char tempStr[80];
	sprintf(tempStr, "%03d", mana);
	SbarH2_DrawSmallString(10, y + 6, tempStr);

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
	SbarH2_DrawSmallString(10, y + 18 + 6, tempStr);

	// HP
	SbarH2_DrawNum(38, y + 18, cl.h2_v.health, 3);

	// Current inventory item
	if (cl.h2_inv_selected >= 0)
	{
		DrawBarArtifactIcon(288, y + 7, cl.h2_inv_order[cl.h2_inv_selected]);
	}
}

static void SetChainPosition(float health, float maxHealth)
{
	if (health < 0)
	{
		health = 0;
	}
	else if (health > maxHealth)
	{
		health = maxHealth;
	}
	float chainTargetPosition = (health * 195) / maxHealth;
	if (Q_fabs(sbh2_ChainPosition - chainTargetPosition) < 0.1)
	{
		return;
	}
	if (sbh2_ChainPosition < chainTargetPosition)
	{
		float delta = ((chainTargetPosition - sbh2_ChainPosition) * 5) * cls.frametime * 0.001;
		if (delta < 0.5)
		{
			delta = 0.5;
		}
		sbh2_ChainPosition += delta;
		if (sbh2_ChainPosition > chainTargetPosition)
		{
			sbh2_ChainPosition = chainTargetPosition;
		}
	}
	else if (sbh2_ChainPosition > chainTargetPosition)
	{
		float delta = ((sbh2_ChainPosition - chainTargetPosition) * 5) * cls.frametime * 0.001;
		if (delta < 0.5)
		{
			delta = 0.5;
		}
		sbh2_ChainPosition -= delta;
		if (sbh2_ChainPosition < chainTargetPosition)
		{
			sbh2_ChainPosition = chainTargetPosition;
		}
	}
}

static int CalcAC()
{
	//playerClass = cl.h2_v.playerclass;
	//BUG cl.h2_v.playerclass was never assigned and was always 0
	int playerClass = GGameType & GAME_HexenWorld ? 1 : clh2_playerclass->value - 1;
	if (playerClass < 0 || playerClass >= MAX_PLAYER_CLASS)
	{
		playerClass = MAX_PLAYER_CLASS - 1;
	}
	int a = 0;
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

static void DrawTopBar()
{
	SbarH2_DrawPic(0, 0, R_CachePic("gfx/topbar1.lmp"));
	SbarH2_DrawPic(160, 0, R_CachePic("gfx/topbar2.lmp"));
	SbarH2_DrawPic(0, -23, R_CachePic("gfx/topbumpl.lmp"));
	SbarH2_DrawPic(138, -8, R_CachePic("gfx/topbumpm.lmp"));
	SbarH2_DrawPic(269, -23, R_CachePic("gfx/topbumpr.lmp"));

	int maxMana = (int)cl.h2_v.max_mana;
	// Blue mana
	int mana = (int)cl.h2_v.bluemana;
	if (mana < 0)
	{
		mana = 0;
	}
	if (mana > maxMana)
	{
		mana = maxMana;
	}
	char tempStr[80];
	sprintf(tempStr, "%03d", mana);
	SbarH2_DrawSmallString(201, 22, tempStr);
	if (mana)
	{
		int y = (int)((mana * 18.0) / (float)maxMana + 0.5);
		SbarH2_DrawSubPic(190, 26 - y, y + 1, R_CachePic("gfx/bmana.lmp"));
		SbarH2_DrawPic(190, 27, R_CachePic("gfx/bmanacov.lmp"));
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
	SbarH2_DrawSmallString(243, 22, tempStr);
	if (mana)
	{
		int y = (int)((mana * 18.0) / (float)maxMana + 0.5);
		SbarH2_DrawSubPic(232, 26 - y, y + 1, R_CachePic("gfx/gmana.lmp"));
		SbarH2_DrawPic(232, 27, R_CachePic("gfx/gmanacov.lmp"));
	}

	// HP
	if (cl.h2_v.health < -99)
	{
		SbarH2_DrawNum(58, 14, -99, 3);
	}
	else
	{
		SbarH2_DrawNum(58, 14, cl.h2_v.health, 3);
	}
	SetChainPosition(cl.h2_v.health, cl.h2_v.max_health);
	SbarH2_DrawPic(45 + ((int)sbh2_ChainPosition & 7), 38,
		R_CachePic("gfx/hpchain.lmp"));
	SbarH2_DrawPic(45 + (int)sbh2_ChainPosition, 36,
		R_CachePic("gfx/hpgem.lmp"));
	SbarH2_DrawPic(43, 36, R_CachePic("gfx/chnlcov.lmp"));
	SbarH2_DrawPic(267, 36, R_CachePic("gfx/chnrcov.lmp"));

	// AC
	SbarH2_DrawNum(105, 14, CalcAC(), 2);

	// Current inventory item
	if (cl.h2_inv_selected >= 0)
	{
		DrawBarArtifactIcon(144, 3, cl.h2_inv_order[cl.h2_inv_selected]);
	}
}

static void DrawLowerBar()
{
	int playerClass = GGameType & GAME_HexenWorld ? cl.h2_players[cl.playernum].playerclass : clh2_playerclass->value;
	if (playerClass < 1 || playerClass > MAX_PLAYER_CLASS)
	{
		// Default to paladin
		playerClass = 1;
	}

	// Backdrop
	SbarH2_DrawPic(0, 46, R_CachePic("gfx/btmbar1.lmp"));
	SbarH2_DrawPic(160, 46, R_CachePic("gfx/btmbar2.lmp"));

	// Stats
	SbarH2_DrawSmallString(11, 48, GGameType & GAME_HexenWorld ? hw_ClassNames[playerClass - 1] : h2_ClassNames[playerClass - 1]);

	char tempStr[80];
	SbarH2_DrawSmallString(11, 58, "int");
	sprintf(tempStr, "%02d", (int)cl.h2_v.intelligence);
	SbarH2_DrawSmallString(33, 58, tempStr);

	SbarH2_DrawSmallString(11, 64, "wis");
	sprintf(tempStr, "%02d", (int)cl.h2_v.wisdom);
	SbarH2_DrawSmallString(33, 64, tempStr);

	SbarH2_DrawSmallString(11, 70, "dex");
	sprintf(tempStr, "%02d", (int)cl.h2_v.dexterity);
	SbarH2_DrawSmallString(33, 70, tempStr);

	SbarH2_DrawSmallString(58, 58, "str");
	sprintf(tempStr, "%02d", (int)cl.h2_v.strength);
	SbarH2_DrawSmallString(80, 58, tempStr);

	SbarH2_DrawSmallString(58, 64, "lvl");
	sprintf(tempStr, "%02d", (int)cl.h2_v.level);
	SbarH2_DrawSmallString(80, 64, tempStr);

	SbarH2_DrawSmallString(58, 70, "exp");
	sprintf(tempStr, "%06d", (int)cl.h2_v.experience);
	SbarH2_DrawSmallString(80, 70, tempStr);

	// Abilities
	SbarH2_DrawSmallString(11, 79, "abilities");
	int i = GGameType & GAME_HexenWorld ? AbilityLineIndex[(playerClass - 1)] : ABILITIES_STR_INDEX + (playerClass - 1) * 2;
	if (i + 1 < prh2_string_count)
	{
		if (((int)cl.h2_v.flags) & H2FL_SPECIAL_ABILITY1)
		{
			SbarH2_DrawSmallString(8, 89,
				&prh2_global_strings[prh2_string_index[i]]);
		}
		if (((int)cl.h2_v.flags) & H2FL_SPECIAL_ABILITY2)
		{
			SbarH2_DrawSmallString(8, 96,
				&prh2_global_strings[prh2_string_index[i + 1]]);
		}
	}

	// Portrait
	sprintf(tempStr, "gfx/cport%d.lmp", playerClass);
	SbarH2_DrawPic(134, 50, R_CachePic(tempStr));

	// Armor
	if (cl.h2_v.armor_helmet > 0)
	{
		SbarH2_DrawPic(164, 115, R_CachePic("gfx/armor1.lmp"));
		sprintf(tempStr, "+%d", (int)cl.h2_v.armor_helmet);
		SbarH2_DrawSmallString(168, 136, tempStr);
	}
	if (cl.h2_v.armor_amulet > 0)
	{
		SbarH2_DrawPic(205, 115, R_CachePic("gfx/armor2.lmp"));
		sprintf(tempStr, "+%d", (int)cl.h2_v.armor_amulet);
		SbarH2_DrawSmallString(208, 136, tempStr);
	}
	if (cl.h2_v.armor_breastplate > 0)
	{
		SbarH2_DrawPic(246, 115, R_CachePic("gfx/armor3.lmp"));
		sprintf(tempStr, "+%d", (int)cl.h2_v.armor_breastplate);
		SbarH2_DrawSmallString(249, 136, tempStr);
	}
	if (cl.h2_v.armor_bracer > 0)
	{
		SbarH2_DrawPic(285, 115, R_CachePic("gfx/armor4.lmp"));
		sprintf(tempStr, "+%d", (int)cl.h2_v.armor_bracer);
		SbarH2_DrawSmallString(288, 136, tempStr);
	}

	// Rings
	if (cl.h2_v.ring_flight > 0)
	{
		SbarH2_DrawPic(6, 119, R_CachePic("gfx/ring_f.lmp"));

		int ringhealth = (int)cl.h2_v.ring_flight;
		if (ringhealth > 100)
		{
			ringhealth = 100;
		}
		SbarH2_DrawPic(35 - (int)(26 * (ringhealth / (float)100)),142,R_CachePic("gfx/ringhlth.lmp"));
		SbarH2_DrawPic(35, 142, R_CachePic("gfx/rhlthcvr.lmp"));
	}

	if (cl.h2_v.ring_water > 0)
	{
		SbarH2_DrawPic(44, 119, R_CachePic("gfx/ring_w.lmp"));
		int ringhealth = (int)cl.h2_v.ring_water;
		if (ringhealth > 100)
		{
			ringhealth = 100;
		}
		SbarH2_DrawPic(73 - (int)(26 * (ringhealth / (float)100)),142,R_CachePic("gfx/ringhlth.lmp"));
		SbarH2_DrawPic(73, 142, R_CachePic("gfx/rhlthcvr.lmp"));
	}

	if (cl.h2_v.ring_turning > 0)
	{
		SbarH2_DrawPic(81, 119, R_CachePic("gfx/ring_t.lmp"));
		int ringhealth = (int)cl.h2_v.ring_turning;
		if (ringhealth > 100)
		{
			ringhealth = 100;
		}
		SbarH2_DrawPic(110 - (int)(26 * (ringhealth / (float)100)),142,R_CachePic("gfx/ringhlth.lmp"));
		SbarH2_DrawPic(110, 142, R_CachePic("gfx/rhlthcvr.lmp"));
	}

	if (cl.h2_v.ring_regeneration > 0)
	{
		SbarH2_DrawPic(119, 119, R_CachePic("gfx/ring_r.lmp"));
		int ringhealth = (int)cl.h2_v.ring_regeneration;
		if (ringhealth > 100)
		{
			ringhealth = 100;
		}
		SbarH2_DrawPic(148 - (int)(26 * (ringhealth / (float)100)),142,R_CachePic("gfx/ringhlth.lmp"));
		SbarH2_DrawPic(148, 142, R_CachePic("gfx/rhlthcv2.lmp"));
	}

	// Puzzle pieces
	int piece = 0;
	for (i = 0; i < 8; i++)
	{
		if (cl.h2_puzzle_pieces[i][0] == 0)
		{
			continue;
		}
		SbarH2_DrawPic(194 + (piece % 4) * 31, piece < 4 ? 51 : 82,
			R_CachePic(va("gfx/puzzle/%s.lmp", cl.h2_puzzle_pieces[i])));
		piece++;
	}
}

static void DrawArtifactInventory()
{
	int i;
	int x, y;

	if (sbh2_InventoryHideTime < cl.qh_serverTimeFloat)
	{
		Inv_Update(false);
		return;
	}
	if (!sbh2_inv_flg)
	{
		return;
	}
	if (!cl.h2_inv_count)
	{
		Inv_Update(false);
		return;
	}

	//SbarH2_InvChanged();

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
			SbarH2_DrawPic(x + 9, y - 12, R_CachePic("gfx/artisel.lmp"));
		}
		DrawBarArtifactIcon(x, y, cl.h2_inv_order[cl.h2_inv_startpos + i]);
	}
}

static void DrawActiveRings()
{
	ring_row = 1;

	int flag = (int)cl.h2_v.rings_active;

	if (flag & RING_TURNING)
	{
		int frame = 1 + ((int)(cl.qh_serverTimeFloat * 16) & 15);
		char tempStr[24];
		sprintf(tempStr, "gfx/rngtrn%d.lmp", frame);
		UI_DrawPic(viddef.width - 50, ring_row, R_CachePic(tempStr));
		ring_row += 33;
	}

	if (flag & RING_WATER)
	{
		int frame = 1 + ((int)(cl.qh_serverTimeFloat * 16) & 15);
		char tempStr[24];
		sprintf(tempStr, "gfx/rngwtr%d.lmp", frame);
		UI_DrawPic(viddef.width - 50, ring_row, R_CachePic(tempStr));
		ring_row += 33;
	}

	if (flag & RING_FLIGHT)
	{
		int frame = 1 + ((int)(cl.qh_serverTimeFloat * 16) & 15);
		char tempStr[24];
		sprintf(tempStr, "gfx/rngfly%d.lmp", frame);
		UI_DrawPic(viddef.width - 50, ring_row, R_CachePic(tempStr));
		ring_row += 33;
	}
}

static void DrawActiveArtifacts()
{
	int flag;
	static int oldflags = 0;
	int frame;
	char tempStr[24];

	int art_col = 50;

	if (ring_row != 1)
	{
		art_col += 50;
	}

	flag = (int)cl.h2_v.artifact_active;
	if (flag & H2ARTFLAG_TOMEOFPOWER)
	{
		frame = 1 + ((int)(cl.qh_serverTimeFloat * 16) & 15);
		sprintf(tempStr, "gfx/pwrbook%d.lmp", frame);
		UI_DrawPic(viddef.width - art_col, 1, R_CachePic(tempStr));
		art_col += 50;
	}

	if (flag & H2ARTFLAG_HASTE)
	{
		frame = 1 + ((int)(cl.qh_serverTimeFloat * 16) & 15);
		sprintf(tempStr, "gfx/durhst%d.lmp", frame);
		UI_DrawPic(viddef.width - art_col,1, R_CachePic(tempStr));
		art_col += 50;
	}

	if (flag & H2ARTFLAG_INVINCIBILITY)
	{
		frame = 1 + ((int)(cl.qh_serverTimeFloat * 16) & 15);
		sprintf(tempStr, "gfx/durshd%d.lmp", frame);
		UI_DrawPic(viddef.width - art_col, 1, R_CachePic(tempStr));
		art_col += 50;
	}

	oldflags = flag;
}

static void DrawNormalBar()
{
	UpdateHeight();

	if (BarHeight < 0)
	{
		DrawFullScreenInfo();
	}

	DrawTopBar();

	if (BarHeight > BAR_TOP_HEIGHT)
	{
		DrawLowerBar();
	}

	DrawArtifactInventory();

	DrawActiveRings();
	DrawActiveArtifacts();
}

static void FindName(const char* which, char* name)
{
	String::Cpy(name, "Unknown");
	int j = atol(h2_puzzle_strings);
	char* pos = strchr(h2_puzzle_strings, 13);
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

	for (; j; j--)
	{
		char* p2 = strchr(pos, ' ');
		if (!p2)
		{
			return;
		}

		char test[40];
		String::NCpy(test, pos, p2 - pos);
		test[p2 - pos] = 0;

		if (String::ICmp(which, test) == 0)
		{
			pos = p2;
			pos++;
			p2 = strchr(pos, 13);	//look for newline after text
			if (p2)
			{
				String::NCpy(name, pos, p2 - pos);
				name[p2 - pos] = 0;
				return;
			}
			return;
		}
		pos = strchr(pos, 13);
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

static void SbarH2_NormalOverlay()
{
	int piece = 0;
	int y = 40;
	for (int i = 0; i < 8; i++)
	{
		if (cl.h2_puzzle_pieces[i][0] == 0)
		{
			continue;
		}

		if (piece == 4)
		{
			y = 40;
		}

		char name[40];
		FindName(cl.h2_puzzle_pieces[i], name);

		if (piece < 4)
		{
			MQH_DrawPic(10, y, R_CachePic(va("gfx/puzzle/%s.lmp", cl.h2_puzzle_pieces[i])));
			MQH_PrintWhite(45, y, name);
		}
		else
		{
			MQH_DrawPic(310 - 32, y, R_CachePic(va("gfx/puzzle/%s.lmp", cl.h2_puzzle_pieces[i])));
			MQH_PrintWhite(310 - 32 - 3 - (String::Length(name) * 8), 18 + y, name);
		}

		y += 32;
		piece++;
	}
}

static void SbarH2_SortFrags(bool includespec)
{
	// sort by frags
	sbh2_scoreboardlines = 0;
	for (int i = 0; i < (GGameType & GAME_HexenWorld ? MAX_CLIENTS_QHW : cl.qh_maxclients); i++)
	{
		if (cl.h2_players[i].name[0] &&
			(!(GGameType & GAME_HexenWorld) || !cl.h2_players[i].spectator || includespec))
		{
			sbh2_fragsort[sbh2_scoreboardlines] = i;
			sbh2_scoreboardlines++;
			if (GGameType & GAME_HexenWorld && cl.h2_players[i].spectator)
			{
				cl.h2_players[i].frags = SPEC_FRAGS;
			}
		}
	}

	for (int i = 0; i < sbh2_scoreboardlines; i++)
	{
		for (int j = 0; j < sbh2_scoreboardlines - 1 - i; j++)
		{
			if (cl.h2_players[sbh2_fragsort[j]].frags < cl.h2_players[sbh2_fragsort[j + 1]].frags)
			{
				int k = sbh2_fragsort[j];
				sbh2_fragsort[j] = sbh2_fragsort[j + 1];
				sbh2_fragsort[j + 1] = k;
			}
		}
	}
}

static void FindColor(int slot, int* color1, int* color2)
{
	if (slot > (GGameType & GAME_HexenWorld ? MAX_CLIENTS_QHW : cl.qh_maxclients))
	{
		common->FatalError("CL_NewTranslation: slot > cl.maxclients");
	}

	if (cl.h2_players[slot].playerclass <= 0 || cl.h2_players[slot].playerclass > MAX_PLAYER_CLASS)
	{
		*color1 = *color2 = 0;
		return;
	}

	int top = cl.h2_players[slot].topColour;
	int bottom = cl.h2_players[slot].bottomColour;

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

	byte* colorA = playerTranslation + 256 + color_offsets[cl.h2_players[slot].playerclass - 1];
	byte* colorB = colorA + 256;
	byte* sourceA = colorB + 256 + (top * 256);
	byte* sourceB = colorB + 256 + (bottom * 256);
	for (int j = 0; j < 256; j++, colorA++, colorB++, sourceA++, sourceB++)
	{
		if (*colorA != 255)
		{
			if (top >= 0)
			{
				*color1 = *sourceA;
			}
			else
			{
				*color1 = *colorA;
			}
		}
		if (*colorB != 255)
		{
			if (bottom >= 0)
			{
				*color2 = *sourceB;
			}
			else
			{
				*color2 = *colorB;
			}
		}
	}
}

void SbarH2_DeathmatchOverlay()
{
	if (GGameType & GAME_HexenWorld && cls.realtime - cl.qh_last_ping_request * 1000 > 2000)
	{
		cl.qh_last_ping_request = cls.realtime * 0.001;
		CL_AddReliableCommand("pings");
	}

	image_t* pic = R_CachePic("gfx/menu/title8.lmp");
	MQH_DrawPic((320 - R_GetImageWidth(pic)) / 2, 0, pic);

	// scores
	SbarH2_SortFrags(true);

	// draw the text
	int l = sbh2_scoreboardlines;

	int x = ((viddef.width - 320) >> 1);
	int y = 62;

	if (GGameType & GAME_HexenWorld)
	{
		UI_DrawString(x + 4, y - 1, S_COLOR_RED "FRAG");
		UI_DrawString(x + 48, y - 1, S_COLOR_RED "PING");
		UI_DrawString(x + 88, y - 1, S_COLOR_RED "TIME");
		UI_DrawString(x + 128, y - 1, S_COLOR_RED "NAME");
		y += 12;
	}

	for (int i = 0; i < l; i++)
	{
		if (y + 10 >= (int)viddef.height)
		{
			break;
		}

		int k = sbh2_fragsort[i];
		h2player_info_t* s = &cl.h2_players[k];
		if (!s->name[0])
		{
			continue;
		}

		if (s->frags != SPEC_FRAGS)
		{
			if (GGameType & GAME_HexenWorld)
			{
				//Print playerclass next to name
				switch (s->playerclass)
				{
				case 0:
					UI_DrawString(x + 128, y, S_COLOR_RED "(r   )");
					break;
				case 1:
					UI_DrawString(x + 128, y, S_COLOR_RED "(P   )");
					break;
				case 2:
					UI_DrawString(x + 128, y, S_COLOR_RED "(C   )");
					break;
				case 3:
					UI_DrawString(x + 128, y, S_COLOR_RED "(N   )");
					break;
				case 4:
					UI_DrawString(x + 128, y, S_COLOR_RED "(A   )");
					break;
				case 5:
					UI_DrawString(x + 128, y, S_COLOR_RED "(S   )");
					break;
				case 6:
					UI_DrawString(x + 128, y, S_COLOR_RED "(D   )");
					break;
				default:
					UI_DrawString(x + 128, y, S_COLOR_RED "(?   )");
					break;
				}

				if (clhw_siege)
				{
					UI_DrawString(x + 160, y, S_COLOR_RED "6");
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
						UI_DrawString(x + 152, y, S_COLOR_RED "?");//no team

					}
					if (k == clhw_keyholder)
					{
						UI_DrawChar(x + 144, y, 145);//key
					}
					else
					{
						UI_DrawString(x + 144, y, S_COLOR_RED "-");
					}

					if (k == clhw_doc)
					{
						UI_DrawChar(x + 160, y, 130);//crown
					}
					else
					{
						UI_DrawString(x + 160, y, S_COLOR_RED "6");
					}

				}
				else
				{
					UI_DrawString(x + 144, y, S_COLOR_RED "-");
					if (!s->level)
					{
						UI_DrawString(x + 152, y, S_COLOR_RED "01");
					}
					else if ((int)s->level < 10)
					{
						UI_DrawString(x + 152, y, S_COLOR_RED "0");
						char num[40];
						sprintf(num, S_COLOR_RED "%1d",s->level);
						UI_DrawString(x + 160, y, num);
					}
					else
					{
						char num[40];
						sprintf(num, S_COLOR_RED "%2d",s->level);
						UI_DrawString(x + 152, y, num);
					}
				}
			}

			// draw background
			int top;
			int bottom;
			FindColor(k, &top, &bottom);

			if (GGameType & GAME_HexenWorld)
			{
				UI_FillPal(x + 8, y, 28, 4, top);
				UI_FillPal(x + 8, y + 4, 28, 4, bottom);
			}
			else
			{
				UI_FillPal(x + 80, y, 40, 4, top);
				UI_FillPal(x + 80, y + 4, 40, 4, bottom);
			}

			// draw number
			char num[40];
			sprintf(num, "%3i", s->frags);

			if (GGameType & GAME_HexenWorld)
			{
				UI_DrawString(x + 10, y - 1, num);

				if (k == cl.playernum)
				{
					UI_DrawChar(x, y - 1, 13);
				}

				sprintf(num, "%4d",s->ping);
				UI_DrawString(x + 48, y, num);

				float total;
				if (cl.qh_intermission)
				{
					total = cl.qh_completed_time - s->entertime;
				}
				else
				{
					total = cls.realtime * 0.001 - s->entertime;
				}
				int minutes = (int)total / 60;
				sprintf(num, "%4i", minutes);
				UI_DrawString(x + 88, y, num);

				// draw name
				if (clhw_siege && s->siege_team == 2)	//attacker
				{
					UI_DrawString(x + 178, y, s->name);
				}
				else
				{
					UI_DrawString(x + 178, y, va(S_COLOR_RED "%s", s->name));
				}
			}
			else
			{
				if (k == svh2_kingofhill)
				{
					UI_DrawChar(x + 68, y - 1, 130);
				}
				UI_DrawString(x + 88, y - 1, num);

				if (k == cl.viewentity - 1)
				{
					UI_DrawChar(x + 72, y - 1, 12);
				}

				// draw name
				UI_DrawString(x + 144, y, s->name);
			}
		}
		else
		{
			char num[40];
			sprintf(num, S_COLOR_RED "%4d", s->ping);
			UI_DrawString(x + 48, y, num);

			float total;
			if (cl.qh_intermission)
			{
				total = cl.qh_completed_time - s->entertime;
			}
			else
			{
				total = cls.realtime * 0.001 - s->entertime;
			}
			int minutes = (int)total / 60;
			sprintf(num, S_COLOR_RED "%4i", minutes);
			UI_DrawString(x + 88, y, num);

			// draw name
			UI_DrawString(x + 128, y, va(S_COLOR_RED "%s", s->name));
		}

		y += 10;
	}
}

static void DrawTime(int x, int y, int disp_time)
{
	int show_min = disp_time;
	int show_sec = show_min % 60;
	show_min = (show_min - show_sec) / 60;
	char num[40];
	sprintf(num, S_COLOR_RED "%3d",show_min);
	UI_DrawString(x + 8, y, num);
	sprintf(num, S_COLOR_RED "%2d",show_sec);
	if (show_sec >= 10)
	{
		UI_DrawString(x + 40, y, num);
	}
	else
	{
		UI_DrawString(x + 40, y, num);
		UI_DrawString(x + 40, y, S_COLOR_RED "0");
	}
}

static void SbarH2_SmallDeathmatchOverlay()
{
	char num[40];
	h2player_info_t* s;

	if (DMMode->value == 2 && BarHeight != BAR_TOP_HEIGHT)
	{
		return;
	}

	// scores
	SbarH2_SortFrags(false);

	// draw the text
	int l = sbh2_scoreboardlines;

	int x = 10;
	int y;
	if (DMMode->value == 1)
	{
		int i = GGameType & GAME_HexenWorld ? (viddef.height - 120) / 10 : 8;
		if (l > i)
		{
			l = i;
		}
		y = 46;
	}
	else if (DMMode->value == 2)
	{
		if (l > 4)
		{
			l = 4;
		}
		y = viddef.height - BAR_TOP_HEIGHT;
	}

	if (GGameType & GAME_HexenWorld && clhw_siege)
	{
		int i = (viddef.height - 120) / 10;
		if (l > i)
		{
			l = i;
		}
		y = viddef.height - BAR_TOP_HEIGHT - 24;
		x -= 4;
		UI_DrawChar(x, y, 142);	//sundial
		UI_DrawChar(x + 32, y, 58);	// ":"
		if (clhw_timelimit > cl.qh_serverTimeFloat + clhw_server_time_offset)
		{
			DrawTime(x,y,(int)(clhw_timelimit - (cl.qh_serverTimeFloat + clhw_server_time_offset)));
		}
		else if ((int)cl.qh_serverTimeFloat % 2)
		{	//odd number, draw 00:00, this will make it flash every second
			UI_DrawString(x + 16, y, S_COLOR_RED "00");
			UI_DrawString(x + 40, y, S_COLOR_RED "00");
		}
	}

	int def_frags = 0;
	int att_frags = 0;
	for (int i = 0; i < l; i++)
	{
		int k = sbh2_fragsort[i];
		s = &cl.h2_players[k];
		if (!s->name[0])
		{
			continue;
		}

		if (GGameType & GAME_HexenWorld && clhw_siege)
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
			// draw background
			int top, bottom;
			FindColor(k, &top, &bottom);

			UI_FillPal(x, y, 28, 4, top);
			UI_FillPal(x, y + 4, 28, 4, bottom);

			// draw number
			sprintf(num, "%3i", s->frags);

			if (k == cl.viewentity - 1)
			{
				UI_DrawChar(x - 8, y - 1, 13);
			}
			UI_DrawString(x + 2, y - 1, num);
			if (!(GGameType & GAME_HexenWorld) && k == svh2_kingofhill)
			{
				UI_DrawChar(x + 30, y - 1, 130);
			}
			y += 10;
		}
	}
	if (GGameType & GAME_HexenWorld && clhw_siege)
	{
		if (cl.playernum == clhw_keyholder)
		{
			UI_DrawChar(x, y - 10, 145);	//key
		}
		if (cl.playernum == clhw_doc)
		{
			UI_DrawChar(x + 8, y - 10, 130);	//crown
		}
		if ((int)cl.h2_v.artifact_active & HWARTFLAG_BOOTS)
		{
			UI_DrawChar(x + 16, y - 10, 146);//boots

		}
		//Print defender losses
		UI_DrawChar(x, y + 10, 143);	//shield
		sprintf(num, S_COLOR_RED "%3i",clhw_defLosses);
		UI_DrawString(x + 8, y + 10, num);
		UI_DrawChar(x + 32, y + 10, 47);	// "/"
		sprintf(num, S_COLOR_RED "%3i",clhw_fraglimit);
		UI_DrawString(x + 40, y + 10, num);

		//Print attacker losses
		UI_DrawChar(x, y + 20, 144);	//sword
		sprintf(num, S_COLOR_RED "%3i",clhw_attLosses);
		UI_DrawString(x + 8, y + 20, num);
		UI_DrawChar(x + 32, y + 20, 47);	// "/"
		sprintf(num, S_COLOR_RED "%3i",clhw_fraglimit * 2);
		UI_DrawString(x + 40, y + 20, num);
	}
}

static void R_DrawName(vec3_t origin, const char* Name, int Red)
{
	if (!Name)
	{
		return;
	}

	int u;
	int v;
	if (!R_GetScreenPosFromWorldPos(origin, u, v))
	{
		return;
	}

	u -= String::Length(Name) * 4;

	if (clhw_siege)
	{
		if (Red > 10)
		{
			Red -= 10;
			UI_DrawChar(u, v, 145);	//key
			u += 8;
		}
		if (Red > 0 && Red < 3)	//def
		{
			if (Red == true)
			{
				UI_DrawChar(u, v, 143);	//shield
			}
			else
			{
				UI_DrawChar(u, v, 130);	//crown
			}
			UI_DrawString(u + 8, v, Name, 256);
		}
		else if (!Red)
		{
			UI_DrawChar(u, v, 144);	//sword
			UI_DrawString(u + 8, v, Name);
		}
		else
		{
			UI_DrawString(u + 8, v, Name);
		}
	}
	else
	{
		UI_DrawString(u, v, Name);
	}
}

static void SB_PlacePlayerNames()
{
	for (int i = 0; i < MAX_CLIENTS_QHW; i++)
	{
		if ((cl.hw_PIV & (1 << i)))
		{
			if (!cl.h2_players[i].shownames_off)
			{
				if (clhw_siege)
				{
					//why the fuck does GL fuck this up??!!!
					if (cl.h2_players[i].siege_team == HWST_ATTACKER)	//attacker
					{
						if (i == clhw_keyholder)
						{
							R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,10);
						}
						else
						{
							R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,false);
						}
					}
					else if (cl.h2_players[i].siege_team == HWST_DEFENDER)//def
					{
						if (i == clhw_keyholder && i == clhw_doc)
						{
							R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,12);
						}
						else if (i == clhw_keyholder)
						{
							R_DrawName(cl.h2_players[i].origin, cl.h2_players[i].name,11);
						}
						else if (i == clhw_doc)
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

void SbarH2_Draw()
{
	if (h2intro_playing)
	{
		return;
	}

	if (GGameType & GAME_HexenWorld && sbh2_ShowNames)
	{
		SB_PlacePlayerNames();
	}

	if (cls.state != CA_ACTIVE || (!(GGameType & GAME_HexenWorld) && clc.qh_signon != SIGNONS) || con.displayFrac == 1)
	{
		// console is full screen
		return;
	}

	if (!(GGameType & GAME_HexenWorld) || !cl.qh_spectator)
	{
		DrawNormalBar();
	}

	if (sbh2_ShowDM)
	{
		if (GGameType & GAME_HexenWorld || cl.qh_gametype == QHGAME_DEATHMATCH)
		{
			SbarH2_DeathmatchOverlay();
		}
		else
		{
			SbarH2_NormalOverlay();
		}
	}
	else if ((GGameType & GAME_HexenWorld || cl.qh_gametype == QHGAME_DEATHMATCH) && DMMode->value)
	{
		SbarH2_SmallDeathmatchOverlay();
	}
}

void SbarH2_InvChanged()
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

void SbarH2_InvReset()
{
	cl.h2_inv_count = cl.h2_inv_startpos = 0;
	cl.h2_inv_selected = -1;
	sbh2_inv_flg = false;
}
