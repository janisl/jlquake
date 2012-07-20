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

extern bool slistInProgress;
extern bool slistSilent;
extern bool slistLocal;

void NET_Slist_f();
void NET_Poll();
// called by client to connect to a host.  Returns -1 if not able to
qsocket_t* NET_Connect(const char* host, netchan_t* chan);
void CLQH_ShutdownNetwork();
