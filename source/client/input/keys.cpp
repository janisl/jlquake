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
#include "../game/tech3/local.h"
#include "../game/et/local.h"

struct keyname_t
{
	const char* name;
	int keynum;
};

qkey_t keys[MAX_KEYS];

bool key_overstrikeMode;

static bool consolekeys[256];	// if true, can't be rebound while in console
static bool menubound[256];	// if true, can't be rebound while in menu

static bool consoleButtonWasPressed = false;

Cvar* clwm_missionStats;
Cvar* clwm_waitForFire;

// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
static keyname_t keynames[] =
{
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},
	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"SHIFT", K_SHIFT},

	{"COMMAND", K_COMMAND},

	{"CAPSLOCK", K_CAPSLOCK},

	{"POWER", K_POWER},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},
	{"F13", K_F13},
	{"F14", K_F14},
	{"F15", K_F15},

	{"INS", K_INS},
	{"DEL", K_DEL},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},

	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},

	{"MWHEELUP", K_MWHEELUP },
	{"MWHEELDOWN", K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"KP_HOME", K_KP_HOME },
	{"KP_UPARROW", K_KP_UPARROW },
	{"KP_PGUP", K_KP_PGUP },
	{"KP_LEFTARROW", K_KP_LEFTARROW },
	{"KP_5", K_KP_5 },
	{"KP_RIGHTARROW", K_KP_RIGHTARROW },
	{"KP_END", K_KP_END },
	{"KP_DOWNARROW", K_KP_DOWNARROW },
	{"KP_PGDN", K_KP_PGDN },
	{"KP_ENTER", K_KP_ENTER },
	{"KP_INS", K_KP_INS },
	{"KP_DEL", K_KP_DEL },
	{"KP_SLASH", K_KP_SLASH },
	{"KP_MINUS", K_KP_MINUS },
	{"KP_PLUS", K_KP_PLUS },
	{"KP_NUMLOCK", K_KP_NUMLOCK },
	{"KP_STAR", K_KP_STAR },
	{"KP_EQUALS", K_KP_EQUALS },

	{"PAUSE", K_PAUSE},

	{"SEMICOLON", ';'},	// because a raw semicolon seperates commands

	{NULL,0}
};

static keyname_t keynames_d[] =	//deutsch
{
	{"TAB", K_TAB},
	{"EINGABETASTE", K_ENTER},
	{"ESC", K_ESCAPE},
	{"LEERTASTE", K_SPACE},
	{"R�CKTASTE", K_BACKSPACE},
	{"PFEILT.AUF", K_UPARROW},
	{"PFEILT.UNTEN", K_DOWNARROW},
	{"PFEILT.LINKS", K_LEFTARROW},
	{"PFEILT.RECHTS", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"STRG", K_CTRL},
	{"UMSCHALT", K_SHIFT},

	{"COMMAND", K_COMMAND},	//mac

	{"FESTSTELLT", K_CAPSLOCK},

	{"POWER", K_POWER},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},
	{"F13", K_F13},
	{"F14", K_F14},
	{"F15", K_F15},

	{"EINFG", K_INS},
	{"ENTF", K_DEL},
	{"BILD-AB", K_PGDN},
	{"BILD-AUF", K_PGUP},
	{"POS1", K_HOME},
	{"ENDE", K_END},

	{"MAUS1", K_MOUSE1},
	{"MAUS2", K_MOUSE2},
	{"MAUS3", K_MOUSE3},
	{"MAUS4", K_MOUSE4},
	{"MAUS5", K_MOUSE5},

	{"MRADOBEN", K_MWHEELUP },
	{"MRADUNTEN", K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"ZB_POS1", K_KP_HOME },
	{"ZB_PFEILT.AUF", K_KP_UPARROW },
	{"ZB_BILD-AUF", K_KP_PGUP },
	{"ZB_PFEILT.LINKS", K_KP_LEFTARROW },
	{"ZB_5", K_KP_5 },
	{"ZB_PFEILT.RECHTS", K_KP_RIGHTARROW },
	{"ZB_ENDE", K_KP_END },
	{"ZB_PFEILT.UNTEN", K_KP_DOWNARROW },
	{"ZB_BILD-AB", K_KP_PGDN },
	{"ZB_ENTER", K_KP_ENTER },
	{"ZB_EINFG", K_KP_INS },
	{"ZB_ENTF", K_KP_DEL },
	{"ZB_SLASH", K_KP_SLASH },
	{"ZB_MINUS", K_KP_MINUS },
	{"ZB_PLUS", K_KP_PLUS },
	{"ZB_NUM", K_KP_NUMLOCK },
	{"ZB_*", K_KP_STAR },
	{"ZB_EQUALS", K_KP_EQUALS },

	{"PAUSE", K_PAUSE},

	{"SEMICOLON", ';'},	// because a raw semicolon seperates commands

	{NULL,0}
};	//end german

static keyname_t keynames_f[] =	//french
{
	{"TAB", K_TAB},
	{"ENTREE", K_ENTER},
	{"ECHAP", K_ESCAPE},
	{"ESPACE", K_SPACE},
	{"RETOUR", K_BACKSPACE},
	{"HAUT", K_UPARROW},
	{"BAS", K_DOWNARROW},
	{"GAUCHE", K_LEFTARROW},
	{"DROITE", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"MAJ", K_SHIFT},

	{"COMMAND", K_COMMAND},	//mac

	{"VERRMAJ", K_CAPSLOCK},

	{"POWER", K_POWER},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},
	{"F13", K_F13},
	{"F14", K_F14},
	{"F15", K_F15},

	{"INSER", K_INS},
	{"SUPPR", K_DEL},
	{"PGBAS", K_PGDN},
	{"PGHAUT", K_PGUP},
	{"ORIGINE", K_HOME},
	{"FIN", K_END},

	{"SOURIS1", K_MOUSE1},
	{"SOURIS2", K_MOUSE2},
	{"SOURIS3", K_MOUSE3},
	{"SOURIS4", K_MOUSE4},
	{"SOURIS5", K_MOUSE5},

	{"MOLETTEHT.", K_MWHEELUP },
	{"MOLETTEBAS", K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"PN_ORIGINE", K_KP_HOME },
	{"PN_HAUT", K_KP_UPARROW },
	{"PN_PGBAS", K_KP_PGUP },
	{"PN_GAUCHE", K_KP_LEFTARROW },
	{"PN_5", K_KP_5 },
	{"PN_DROITE", K_KP_RIGHTARROW },
	{"PN_FIN", K_KP_END },
	{"PN_BAS", K_KP_DOWNARROW },
	{"PN_PGBAS", K_KP_PGDN },
	{"PN_ENTR", K_KP_ENTER },
	{"PN_INSER", K_KP_INS },
	{"PN_SUPPR", K_KP_DEL },
	{"PN_SLASH", K_KP_SLASH },
	{"PN_MOINS", K_KP_MINUS },
	{"PN_PLUS", K_KP_PLUS },
	{"PN_VERRNUM", K_KP_NUMLOCK },
	{"PN_*", K_KP_STAR },
	{"PN_EQUALS", K_KP_EQUALS },

	{"PAUSE", K_PAUSE},

	{"SEMICOLON", ';'},	// because a raw semicolon seperates commands

	{NULL,0}
};	//end french

static keyname_t keynames_s[] =	//Spanish
{
	{"TABULADOR", K_TAB},
	{"INTRO", K_ENTER},
	{"ESC", K_ESCAPE},
	{"BARRA_ESPACIAD", K_SPACE},
	{"RETROCESO", K_BACKSPACE},
	{"CURSOR_ARRIBA", K_UPARROW},
	{"CURSOR_ABAJO", K_DOWNARROW},
	{"CURSOR_IZQDA", K_LEFTARROW},
	{"CURSOR_DERECHA", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"MAY�S", K_SHIFT},

	{"COMANDO", K_COMMAND},	//mac

	{"BLOQ_MAY�S", K_CAPSLOCK},

	{"POWER", K_POWER},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},
	{"F13", K_F13},
	{"F14", K_F14},
	{"F15", K_F15},

	{"INSERT", K_INS},
	{"SUPR", K_DEL},
	{"AV_P�G", K_PGDN},
	{"RE_P�G", K_PGUP},
	{"INICIO", K_HOME},
	{"FIN", K_END},

	{"RAT�N1", K_MOUSE1},
	{"RAT�N2", K_MOUSE2},
	{"RAT�N3", K_MOUSE3},
	{"RAT�N4", K_MOUSE4},
	{"RAT�N5", K_MOUSE5},

	{"RUEDA_HACIA_ARRIBA", K_MWHEELUP },
	{"RUEDA_HACIA_ABAJO", K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"INICIO(NUM)", K_KP_HOME },
	{"ARRIBA(NUM)", K_KP_UPARROW },
	{"RE_P�G(NUM)", K_KP_PGUP },
	{"IZQUIERDA(NUM)", K_KP_LEFTARROW },
	{"5(NUM)", K_KP_5 },
	{"DERECHA(NUM)", K_KP_RIGHTARROW },
	{"FIN(NUM)", K_KP_END },
	{"ABAJO(NUM)", K_KP_DOWNARROW },
	{"AV_P�G(NUM)", K_KP_PGDN },
	{"INTRO(NUM)", K_KP_ENTER },
	{"INS(NUM)", K_KP_INS },
	{"SUPR(NUM)", K_KP_DEL },
	{"/(NUM)", K_KP_SLASH },
	{"-(NUM)", K_KP_MINUS },
	{"+(NUM)", K_KP_PLUS },
	{"BLOQ_NUM", K_KP_NUMLOCK },
	{"*(NUM)", K_KP_STAR },
	{"INTRO(NUM)", K_KP_EQUALS },

	{"PAUSA", K_PAUSE},

	{"PUNTO_Y_COMA", ';'},		// because a raw semicolon seperates commands

	{NULL,0}
};

static keyname_t keynames_i[] =	//Italian
{
	{"TAB", K_TAB},
	{"INVIO", K_ENTER},
	{"ESC", K_ESCAPE},
	{"SPAZIO", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},
	{"FRECCIASU", K_UPARROW},
	{"FRECCIAGI�", K_DOWNARROW},
	{"FRECCIASX", K_LEFTARROW},
	{"FRECCIADX", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"MAIUSC", K_SHIFT},

	{"COMMAND", K_COMMAND},	//mac

	{"BLOCMAIUSC", K_CAPSLOCK},

	{"POWER", K_POWER},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},
	{"F13", K_F13},
	{"F14", K_F14},
	{"F15", K_F15},

	{"INS", K_INS},
	{"CANC", K_DEL},
	{"PAGGI�", K_PGDN},
	{"PAGGSU", K_PGUP},
	{"HOME", K_HOME},
	{"FINE", K_END},

	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},

	{"ROTELLASU", K_MWHEELUP },
	{"ROTELLAGI�", K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"TN_HOME", K_KP_HOME },
	{"TN_FRECCIASU", K_KP_UPARROW },
	{"TN_PAGGSU", K_KP_PGUP },
	{"TN_FRECCIASX", K_KP_LEFTARROW },
	{"TN_5", K_KP_5 },
	{"TN_FRECCIA_DX", K_KP_RIGHTARROW },
	{"TN_FINE", K_KP_END },
	{"TN_FRECCIAGI�", K_KP_DOWNARROW },
	{"TN_PAGGI�", K_KP_PGDN },
	{"TN_INVIO", K_KP_ENTER },
	{"TN_INS", K_KP_INS },
	{"TN_CANC", K_KP_DEL },
	{"TN_/", K_KP_SLASH },
	{"TN_-", K_KP_MINUS },
	{"TN_+", K_KP_PLUS },
	{"TN_BLOCNUM", K_KP_NUMLOCK },
	{"TN_*", K_KP_STAR },
	{"TN_=", K_KP_EQUALS },

	{"PAUSA", K_PAUSE},

	{"�", ';'},		// because a raw semicolon seperates commands

	{NULL,0}
};

bool Key_GetOverstrikeMode()
{
	return key_overstrikeMode;
}

void Key_SetOverstrikeMode(bool state)
{
	key_overstrikeMode = state;
}

bool Key_IsDown(int keynum)
{
	if (keynum == -1)
	{
		return false;
	}

	return keys[keynum].down;
}

//	Returns a key number to be used to index keys[] by looking at
// the given string.  Single ascii characters return themselves, while
// the K_* names are matched up.
//	0x11 will be interpreted as raw hex, which will allow new controlers
// to be configured even if they don't have defined names.
static int Key_StringToKeynum(const char* str)
{
	if (!str || !str[0])
	{
		return -1;
	}
	if (!str[1])
	{
		// Always lowercase
		return String::ToLower(str[0]);
	}

	// check for hex code
	if (str[0] == '0' && str[1] == 'x' && String::Length(str) == 4)
	{
		int n1 = str[2];
		if (String::IsDigit(n1))
		{
			n1 -= '0';
		}
		else if (n1 >= 'a' && n1 <= 'f')
		{
			n1 = n1 - 'a' + 10;
		}
		else
		{
			n1 = 0;
		}

		int n2 = str[3];
		if (String::IsDigit(n2))
		{
			n2 -= '0';
		}
		else if (n2 >= 'a' && n2 <= 'f')
		{
			n2 = n2 - 'a' + 10;
		}
		else
		{
			n2 = 0;
		}

		return n1 * 16 + n2;
	}

	for (keyname_t* kn = keynames; kn->name; kn++)
	{
		if (!String::ICmp(str,kn->name))
		{
			return kn->keynum;
		}
	}
	return -1;
}

//	Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
// given keynum.
const char* Key_KeynumToString(int keynum, bool translate)
{
	static char tinystr[5];

	if (keynum == -1)
	{
		return "<KEY NOT FOUND>";
	}

	if (keynum < 0 || keynum > 255)
	{
		return "<OUT OF RANGE>";
	}

	// check for printable ascii (don't use quote)
	if (keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';')
	{
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	keyname_t* kn = keynames;		//init to english
	if (translate && GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		if (cl_language->integer - 1 == LANGUAGE_FRENCH)
		{
			kn = keynames_f;	//use french
		}
		else if (cl_language->integer - 1 == LANGUAGE_GERMAN)
		{
			kn = keynames_d;	//use german
		}
		else if (cl_language->integer - 1 == LANGUAGE_ITALIAN)
		{
			kn = keynames_i;	//use italian
		}
		else if (cl_language->integer - 1 == LANGUAGE_SPANISH)
		{
			kn = keynames_s;	//use spanish
		}
	}

	// check for a key string
	for (; kn->name; kn++)
	{
		if (keynum == kn->keynum)
		{
			return kn->name;
		}
	}

	// make a hex string
	int i = keynum >> 4;
	int j = keynum & 15;

	tinystr[0] = '0';
	tinystr[1] = 'x';
	tinystr[2] = i > 9 ? i - 10 + 'a' : i + '0';
	tinystr[3] = j > 9 ? j - 10 + 'a' : j + '0';
	tinystr[4] = 0;

	return tinystr;
}

void Key_SetBinding(int keynum, const char* binding)
{
	if (keynum == -1)
	{
		return;
	}

	// free old bindings
	if (keys[keynum].binding)
	{
		Mem_Free(keys[keynum].binding);
	}

	// allocate memory for new binding
	keys[keynum].binding = __CopyString(binding);

	// consider this like modifying an archived cvar, so the
	// file write will be triggered at the next oportunity
	cvar_modifiedFlags |= CVAR_ARCHIVE;
}

const char* Key_GetBinding(int keynum)
{
	if (keynum == -1)
	{
		return "";
	}

	return keys[keynum].binding;
}

int Key_GetKey(const char* binding)
{
	if (binding)
	{
		for (int i = 0; i < MAX_KEYS; i++)
		{
			if (keys[i].binding && String::ICmp(binding, keys[i].binding) == 0)
			{
				return i;
			}
		}
	}
	return -1;
}

void Key_GetKeysForBinding(const char* binding, int* key1, int* key2)
{
	*key1 = -1;
	*key2 = -1;

	for (int i = 0; i < MAX_KEYS; i++)
	{
		if (!keys[i].binding)
		{
			continue;
		}
		if (!String::ICmp(keys[i].binding, binding))
		{
			if (*key1 == -1)
			{
				*key1 = i;
			}
			else
			{
				*key2 = i;
				return;
			}
		}
	}
}

void Key_UnbindCommand(const char* command)
{
	for (int i = 0; i < MAX_KEYS; i++)
	{
		if (!keys[i].binding)
		{
			continue;
		}
		if (!String::ICmp(keys[i].binding, command))
		{
			Key_SetBinding(i, "");
		}
	}
}

static void Key_Unbind_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("unbind <key> : remove commands from a key\n");
		return;
	}

	int b = Key_StringToKeynum(Cmd_Argv(1));
	if (b == -1)
	{
		common->Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding(b, "");
}

static void Key_Unbindall_f()
{
	for (int i = 0; i < MAX_KEYS; i++)
	{
		if (keys[i].binding)
		{
			Key_SetBinding(i, "");
		}
	}
}

static void Key_Bind_f()
{
	int c = Cmd_Argc();

	if (c < 2)
	{
		common->Printf("bind <key> [command] : attach a command to a key\n");
		return;
	}
	int b = Key_StringToKeynum(Cmd_Argv(1));
	if (b == -1)
	{
		common->Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keys[b].binding)
		{
			common->Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), keys[b].binding);
		}
		else
		{
			common->Printf("\"%s\" is not bound\n", Cmd_Argv(1));
		}
		return;
	}

	// copy the rest of the command line
	char cmd[1024];
	cmd[0] = 0;		// start out with a null string
	for (int i = 2; i < c; i++)
	{
		String::Cat(cmd, sizeof(cmd), Cmd_Argv(i));
		if (i != (c - 1))
		{
			String::Cat(cmd, sizeof(cmd), " ");
		}
	}

	Key_SetBinding(b, cmd);
}

static void Key_Bindlist_f()
{
	for (int i = 0; i < MAX_KEYS; i++)
	{
		if (keys[i].binding && keys[i].binding[0])
		{
			common->Printf("%s \"%s\"\n", Key_KeynumToString(i, false), keys[i].binding);
		}
	}
}

void CL_InitKeyCommands()
{
	//
	// init ascii characters in console mode
	//
	for (int i = 32; i < 128; i++)
	{
		consolekeys[i] = true;
	}
	consolekeys[K_ENTER] = true;
	consolekeys[K_KP_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_KP_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_KP_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_KP_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_KP_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_HOME] = true;
	consolekeys[K_KP_HOME] = true;
	consolekeys[K_END] = true;
	consolekeys[K_KP_END] = true;
	consolekeys[K_PGUP] = true;
	consolekeys[K_KP_PGUP] = true;
	consolekeys[K_PGDN] = true;
	consolekeys[K_KP_PGDN] = true;
	consolekeys[K_INS] = true;
	consolekeys[K_KP_INS] = true;
	consolekeys[K_DEL] = true;
	consolekeys[K_KP_DEL] = true;
	consolekeys[K_KP_SLASH] = true;
	consolekeys[K_KP_PLUS] = true;
	consolekeys[K_KP_MINUS] = true;
	consolekeys[K_KP_5] = true;
	consolekeys[K_SHIFT] = true;
	consolekeys[K_MWHEELUP] = true;
	consolekeys[K_MWHEELDOWN] = true;
	consolekeys['`'] = false;
	consolekeys['~'] = false;

	menubound[K_ESCAPE] = true;
	for (int i = 0; i < 12; i++)
	{
		menubound[K_F1 + i] = true;
	}

	// register our functions
	Cmd_AddCommand("bind", Key_Bind_f);
	Cmd_AddCommand("unbind", Key_Unbind_f);
	Cmd_AddCommand("unbindall", Key_Unbindall_f);
	Cmd_AddCommand("bindlist", Key_Bindlist_f);
}

//	Writes lines containing "bind key value"
void Key_WriteBindings(fileHandle_t f)
{
	FS_Printf(f, "unbindall\n");

	for (int i = 0; i < MAX_KEYS; i++)
	{
		if (keys[i].binding && keys[i].binding[0])
		{
			FS_Printf(f, "bind %s \"%s\"\n", Key_KeynumToString(i, false), keys[i].binding);
		}
	}
}

static void CL_AddKeyDownCommands(int key, unsigned time)
{
	char* kb = keys[key].binding;
	if (!kb)
	{
		if (key >= 200)
		{
			common->Printf("%s is unbound, use controls menu to set.\n",
				Key_KeynumToString(key, true));
		}
		return;
	}

	if (kb[0] == '+')
	{
		char button[1024];
		char* buttonPtr = button;
		for (int i = 0;; i++)
		{
			if (kb[i] == ';' || !kb[i])
			{
				*buttonPtr = '\0';
				if (button[0] == '+')
				{
					// button commands add keynum and time as parms so that multiple
					// sources can be discriminated and subframe corrected
					char cmd[1024];
					String::Sprintf(cmd, sizeof(cmd), "%s %i %i\n", button, key, time);
					Cbuf_AddText(cmd);
				}
				else
				{
					// down-only command
					Cbuf_AddText(button);
					Cbuf_AddText("\n");
				}
				buttonPtr = button;
				while ((kb[i] <= ' ' || kb[i] == ';') && kb[i] != 0)
				{
					i++;
				}
			}
			*buttonPtr++ = kb[i];
			if (!kb[i])
			{
				break;
			}
		}
	}
	else
	{
		// down-only command
		Cbuf_AddText(kb);
		Cbuf_AddText("\n");
	}
}

static void CL_AddKeyUpCommands(int key, unsigned time)
{
	char* kb = keys[key].binding;

	if (!kb)
	{
		return;
	}
	bool keyevent = false;
	char button[1024];
	char* buttonPtr = button;
	for (int i = 0;; i++)
	{
		if (kb[i] == ';' || !kb[i])
		{
			*buttonPtr = '\0';
			if (button[0] == '+')
			{
				// button commands add keynum and time as parms so that multiple
				// sources can be discriminated and subframe corrected
				char cmd[1024];
				String::Sprintf(cmd, sizeof(cmd), "-%s %i %i\n", button + 1, key, time);
				Cbuf_AddText(cmd);
				keyevent = true;
			}
			else if (keyevent)
			{
				// down-only command
				Cbuf_AddText(button);
				Cbuf_AddText("\n");
			}
			buttonPtr = button;
			while ((kb[i] <= ' ' || kb[i] == ';') && kb[i] != 0)
			{
				i++;
			}
		}
		*buttonPtr++ = kb[i];
		if (!kb[i])
		{
			break;
		}
	}
}

//	Called by the system for both key up and key down events
void CL_KeyEvent(int key, bool down, unsigned time)
{
	if (!key)
	{
		return;
	}

	// update auto-repeat status and BUTTON_ANY status
	keys[key].down = down;

	if (down)
	{
		keys[key].repeats++;
		if (keys[key].repeats == 1)
		{
			anykeydown++;
		}
	}
	else
	{
		keys[key].repeats = 0;
		anykeydown--;
		if (anykeydown < 0)
		{
			anykeydown = 0;
		}
	}

#ifdef __linux__
	if (key == K_ENTER)
	{
		if (down)
		{
			if (keys[K_ALT].down)
			{
				Key_ClearStates();
				if (Cvar_VariableValue("r_fullscreen") == 0)
				{
					common->Printf("Switching to fullscreen rendering\n");
					Cvar_Set("r_fullscreen", "1");
				}
				else
				{
					common->Printf("Switching to windowed rendering\n");
					Cvar_Set("r_fullscreen", "0");
				}
				Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
				return;
			}
		}
	}
#endif

	if (GGameType & GAME_WolfMP)
	{
		// are we waiting to clear stats and move to briefing screen
		if (down && clwm_waitForFire && clwm_waitForFire->integer)			//DAJ BUG in dedicated clwm_waitForFire don't exist
		{
			if (in_keyCatchers & KEYCATCH_CONSOLE)		// get rid of the consol
			{
				Con_ToggleConsole_f();
			}
			// clear all input controls
			CL_ClearKeys();
			// allow only attack command input
			char* kb = keys[key].binding;
			if (!String::ICmp(kb, "+attack"))
			{
				// clear the stats out, ignore the keypress
				Cvar_Set("g_missionStats", "xx");			// just remove the stats, but still wait until we're done loading, and player has hit fire to begin playing
				Cvar_Set("clwm_waitForFire", "0");
			}
			return;		// no buttons while waiting
		}

		// are we waiting to begin the level
		if (down && clwm_missionStats && clwm_missionStats->string[0] && clwm_missionStats->string[1])		//DAJ BUG in dedicated clwm_missionStats don't exist
		{
			if (in_keyCatchers & KEYCATCH_CONSOLE)		// get rid of the consol
			{
				Con_ToggleConsole_f();
			}
			// clear all input controls
			CL_ClearKeys();
			// allow only attack command input
			char* kb = keys[key].binding;
			if (!String::ICmp(kb, "+attack"))
			{
				// clear the stats out, ignore the keypress
				Cvar_Set("com_expectedhunkusage", "-1");
				Cvar_Set("g_missionStats", "0");
			}
			return;		// no buttons while waiting
		}
	}

	// console key is hardcoded, so the user can never unbind it
	if (key == '`' || key == '~')
	{
		if (!down)
		{
			return;
		}
		Con_ToggleConsole_f();

		// the console key should never be used as a char
		consoleButtonWasPressed = true;
		return;
	}
	else
	{
		consoleButtonWasPressed = false;
	}

	if (GGameType & (GAME_WolfSP | GAME_ET) && cl.wa_cameraMode)
	{
		if (!(in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CONSOLE)))			// let menu/console handle keys if necessary
		{
			// in cutscenes we need to handle keys specially (pausing not allowed in camera mode)
			if ((key == K_ESCAPE ||
				 key == K_SPACE ||
				 key == K_ENTER) && down)
			{
				CL_AddReliableCommand("cameraInterrupt");
				return;
			}

			// eat all keys
			if (down)
			{
				return;
			}
		}

		if ((in_keyCatchers & KEYCATCH_CONSOLE) && key == K_ESCAPE)
		{
			// don't allow menu starting when console is down and camera running
			return;
		}
	}

	// any key during the attract mode will bring up the menu
	if (GGameType & GAME_Quake2 && cl.q2_attractloop && !(in_keyCatchers & KEYCATCH_UI))
	{
		key = K_ESCAPE;
	}

	// keys can still be used for bound actions
	if (GGameType & GAME_Tech3 && down && (key < 128 || key == K_MOUSE1) &&
		(clc.demoplaying || cls.state == CA_CINEMATIC) && !in_keyCatchers)
	{
		if (!(GGameType & GAME_Quake3) || Cvar_VariableValue("com_cameraMode") == 0)
		{
			Cvar_Set("nextdemo","");
			key = K_ESCAPE;
		}
	}

	// escape is always handled special
	if (key == K_ESCAPE && down)
	{
		if (in_keyCatchers & KEYCATCH_MESSAGE)
		{
			// clear message mode
			Con_MessageKeyEvent(key);
			return;
		}

		// escape always gets out of CGAME stuff
		if (in_keyCatchers & KEYCATCH_CGAME)
		{
			in_keyCatchers &= ~KEYCATCH_CGAME;
			if (GGameType & GAME_Tech3)
			{
				CLT3_EventHandling();
			}
			return;
		}

		if (in_keyCatchers & KEYCATCH_UI)
		{
			UI_KeyDownEvent(key);
			return;
		}

		if (GGameType & GAME_Quake2 && cl.q2_frame.playerstate.stats[Q2STAT_LAYOUTS] && in_keyCatchers == 0)
		{
			// put away help computer / inventory
			Cbuf_AddText("cmd putaway\n");
			return;
		}

		if (cls.state == CA_ACTIVE && !clc.demoplaying)
		{
			if (in_keyCatchers & KEYCATCH_CONSOLE)		// get rid of the console
			{
				Con_ToggleConsole_f();
			}
			else
			{
				UI_SetInGameMenu();
			}
		}
		else
		{
			if (GGameType & GAME_QuakeHexen && in_keyCatchers & KEYCATCH_CONSOLE && (!(GGameType & GAME_HexenWorld) || cls.state == CA_ACTIVE))
			{
				Con_ToggleConsole_f();
				return;
			}

			if (GGameType & GAME_Tech3)
			{
				CL_Disconnect_f();
				S_StopAllSounds();
			}
			UI_SetMainMenu();
		}
		return;
	}

	bool onlybinds = false;
	if (GGameType & GAME_ET)
	{
		switch (key)
		{
		case K_KP_PGUP:
		case K_KP_EQUALS:
		case K_KP_5:
		case K_KP_LEFTARROW:
		case K_KP_UPARROW:
		case K_KP_RIGHTARROW:
		case K_KP_DOWNARROW:
		case K_KP_END:
		case K_KP_PGDN:
		case K_KP_INS:
		case K_KP_DEL:
		case K_KP_HOME:
			if (Sys_IsNumLockDown())
			{
				onlybinds = true;
			}
			break;
		}
	}

	//
	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing
	// an action started before a mode switch.
	//
	if (!down)
	{
		CL_AddKeyUpCommands(key, time);

		if (in_keyCatchers & KEYCATCH_UI)
		{
			if (!(GGameType & GAME_ET) || !onlybinds || UIET_WantsBindKeys())
			{
				UI_KeyEvent(key, down);
			}
		}
		else if (GGameType & GAME_Tech3 && in_keyCatchers & KEYCATCH_CGAME)
		{
			if (!(GGameType & GAME_ET) || !onlybinds || CLET_WantsBindKeys())
			{
				CLT3_KeyEvent(key, down);
			}
		}

		return;
	}

	//
	// during demo playback, most keys bring up the main menu
	//
	if (GGameType & GAME_QuakeHexen && clc.demoplaying && consolekeys[key] && in_keyCatchers == 0)
	{
		UI_SetMainMenu();
		return;
	}

	if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld) && cl.qh_intermission == 12)
	{
		Cbuf_AddText(GGameType & GAME_H2Portals ? "map keep1\n" : "map demo1\n");
	}

	// NERVE - SMF - if we just want to pass it along to game
	bool bypassMenu = false;
	if (GGameType & (GAME_WolfMP | GAME_ET) && cl_bypassMouseInput && cl_bypassMouseInput->integer)
	{
		if ((key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 || key == K_MOUSE4 || key == K_MOUSE5))
		{
			if (cl_bypassMouseInput->integer == 1)
			{
				bypassMenu = true;
			}
		}
		else if ((in_keyCatchers & KEYCATCH_UI && !UIT3_CheckKeyExec(key)) ||
			(GGameType & GAME_ET && in_keyCatchers & KEYCATCH_CGAME && !CLET_CGameCheckKeyExec(key)))
		{
			bypassMenu = true;
		}
	}

	if (!(GGameType & GAME_Tech3))
	{
		//
		// if not a consolekey, send to the interpreter no matter what mode is
		//
		if (((in_keyCatchers & KEYCATCH_UI) && menubound[key]) ||
			((in_keyCatchers & KEYCATCH_CONSOLE) && !consolekeys[key]) ||
			(in_keyCatchers == 0 && ((cls.state == CA_ACTIVE && (GGameType & (GAME_QuakeWorld | GAME_HexenWorld | GAME_Quake2) || clc.qh_signon == SIGNONS)) || !consolekeys[key])))
		{
			CL_AddKeyDownCommands(key, time);
			return;
		}

		if (in_keyCatchers & KEYCATCH_MESSAGE)
		{
			Con_MessageKeyEvent(key);
		}
		else if (in_keyCatchers & KEYCATCH_UI)
		{
			UI_KeyDownEvent(key);
		}
		else
		{
			Con_KeyEvent(key);
		}
	}
	else
	{
		// distribute the key down event to the apropriate handler
		if (in_keyCatchers & KEYCATCH_CONSOLE)
		{
			if (!onlybinds)
			{
				Con_KeyEvent(key);
			}
		}
		else if (in_keyCatchers & KEYCATCH_UI && !bypassMenu)
		{
			if (!(GGameType & GAME_ET) || !onlybinds || UIET_WantsBindKeys())
			{
				UI_KeyDownEvent(key);
			}
		}
		else if (in_keyCatchers & KEYCATCH_CGAME && !bypassMenu)
		{
			if (!(GGameType & GAME_ET) || !onlybinds || CLET_WantsBindKeys())
			{
				CLT3_KeyEvent(key, down);
			}
		}
		else if (in_keyCatchers & KEYCATCH_MESSAGE)
		{
			if (!onlybinds)
			{
				Con_MessageKeyEvent(key);
			}
		}
		else if (cls.state == CA_DISCONNECTED)
		{
			if (!onlybinds)
			{
				Con_KeyEvent(key);
			}
		}
		else
		{
			// send the bound action
			CL_AddKeyDownCommands(key, time);
		}
	}
}

//	Normal keyboard characters, already shifted / capslocked / etc
void CL_CharEvent(int key)
{
	// fretn
	// we just pressed the console button,
	// so ignore this event
	// this prevents chars appearing at console input
	// when you just opened it
	if (GGameType & GAME_ET && consoleButtonWasPressed)
	{
		consoleButtonWasPressed = false;
		return;
	}

	// the console key should never be used as a char
	if (key == '`' || key == '~' || key == 0xac)
	{
		return;
	}

	// distribute the key down event to the apropriate handler
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		Con_CharEvent(key);
	}
	else if (in_keyCatchers & KEYCATCH_UI)
	{
		UI_CharEvent(key);
	}
	else if (GGameType & GAME_ET && in_keyCatchers & KEYCATCH_CGAME)
	{
		CLT3_KeyEvent(key | K_CHAR_FLAG, true);
	}
	else if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		Con_MessageCharEvent(key);
	}
	else if (cls.state == CA_DISCONNECTED)
	{
		Con_CharEvent(key);
	}
}

void Key_ClearStates()
{
	anykeydown = 0;

	for (int i = 0; i < MAX_KEYS; i++)
	{
		if (keys[i].down)
		{
			CL_KeyEvent(i, false, 0);
		}
		keys[i].down = false;
		keys[i].repeats = 0;
	}
}
