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

extern Cvar* clqh_sbar;
extern Cvar* qhw_topcolor;
extern Cvar* qhw_bottomcolor;
extern Cvar* qhw_spectator;

#define NET_TIMINGS_QH 256
#define NET_TIMINGSMASK_QH 255
extern int clqh_packet_latency[NET_TIMINGS_QH];

void CLQH_Init();
