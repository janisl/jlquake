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

#ifndef __KEYCODES_H__
#define __KEYCODES_H__

//
// these are the key numbers that should be passed to KeyEvent
//

// normal keys should be passed as lowercased ascii

//
//	These values are passed to Quake 3 UI VM and should not be changed.
//

enum
{
	K_TAB = 9,
	K_ENTER = 13,
	K_ESCAPE = 27,
	K_SPACE = 32,

	K_BACKSPACE = 127,

	K_COMMAND = 128,
	K_CAPSLOCK,
	K_POWER,
	K_PAUSE,

	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,
	K_SHIFT,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_F13,
	K_F14,
	K_F15,

	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_MINUS,
	K_KP_PLUS,
	K_KP_NUMLOCK,
	K_KP_STAR,
	K_KP_EQUALS,

	//
	// mouse buttons generate virtual keys
	//
	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,

	K_MWHEELDOWN,
	K_MWHEELUP,

	//
	// joystick buttons
	//
	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4,
	K_JOY5,
	K_JOY6,
	K_JOY7,
	K_JOY8,
	K_JOY9,
	K_JOY10,
	K_JOY11,
	K_JOY12,
	K_JOY13,
	K_JOY14,
	K_JOY15,
	K_JOY16,
	K_JOY17,
	K_JOY18,
	K_JOY19,
	K_JOY20,
	K_JOY21,
	K_JOY22,
	K_JOY23,
	K_JOY24,
	K_JOY25,
	K_JOY26,
	K_JOY27,
	K_JOY28,
	K_JOY29,
	K_JOY30,
	K_JOY31,
	K_JOY32,

	//
	// aux keys are for multi-buttoned joysticks to generate so they can use
	// the normal binding process
	//
	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,
	K_AUX17,
	K_AUX18,
	K_AUX19,
	K_AUX20,
	K_AUX21,
	K_AUX22,
	K_AUX23,
	K_AUX24,
	K_AUX25,
	K_AUX26,
	K_AUX27,
	K_AUX28,
	K_AUX29,
	K_AUX30,
	K_AUX31,
	K_AUX32,

	K_LAST_KEY		// this had better be <256!
};

// The menu code needs to get both key and char events, but
// to avoid duplicating the paths, the char events are just
// distinguished by or'ing in K_CHAR_FLAG (ugly)
#define K_CHAR_FLAG     1024

#endif
