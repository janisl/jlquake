/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_udp.h

int  UDP_Init (void);
void UDP_Shutdown (void);
void UDP_Listen (qboolean state);
int  UDP_OpenSocket (int port);
int  UDP_CloseSocket (int socket);
int  UDP_Read (int socket, byte *buf, int len, netadr_t* addr);
int  UDP_Write (int socket, byte *buf, int len, netadr_t* addr);
int  UDP_Broadcast (int socket, byte *buf, int len);
char *UDP_AddrToString (netadr_t* addr);
int  UDP_GetSocketAddr (int socket, netadr_t* addr);
int  UDP_GetNameFromAddr (netadr_t* addr, char *name);
int  UDP_GetAddrFromName (const char *name, netadr_t* addr);
int  UDP_AddrCompare (netadr_t* addr1, netadr_t* addr2);
int  UDP_GetSocketPort (netadr_t* addr);
int  UDP_SetSocketPort (netadr_t* addr, int port);
