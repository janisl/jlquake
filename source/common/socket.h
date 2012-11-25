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

enum netadrtype_t
{
	NA_BOT,
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
};

#define PORT_ANY            -1

#define SOCKRECV_ERROR      -1
#define SOCKRECV_NO_DATA    -2

#define SOCKSEND_ERROR      -1
#define SOCKSEND_WOULDBLOCK -2

struct netadr_t
{
	netadrtype_t type;
	quint8 ip[4];
	quint16 port;
};

bool SOCK_StringToAdr(const char* string, netadr_t* Address, int DefaultPort);
const char* SOCK_AdrToString(const netadr_t& Address);
const char* SOCK_BaseAdrToString(const netadr_t& Address);
bool SOCK_CompareAdr(const netadr_t& AddressA, const netadr_t& AddressB);
bool SOCK_CompareBaseAdr(const netadr_t& AddressA, const netadr_t& AddressB);
bool SOCK_IsLocalAddress(const netadr_t& Address);
bool SOCK_IsLocalIP(const netadr_t& Address);
bool SOCK_IsLANAddress(const netadr_t& Address);
void SOCK_ShowIP();
int SOCK_GetPort(netadr_t* Address);
void SOCK_SetPort(netadr_t* Address, int Port);
void SOCK_CheckAddr(netadr_t* addr);

bool SOCK_Init();
void SOCK_Shutdown();
void SOCK_GetLocalAddress();

bool SOCK_GetSocksCvars();
void SOCK_OpenSocks(int Port);
void SOCK_CloseSocks();

int SOCK_Open(const char* NetInterface, int Port);
void SOCK_Close(int Socket);
int SOCK_Recv(int Socket, void* Buffer, int Length, netadr_t* From);
int SOCK_Send(int Socket, const void* Data, int Length, const netadr_t& To);
bool SOCK_Sleep(int Socket, int MiliSeconds);
bool SOCK_GetAddr(int Socket, netadr_t* Address);

extern Cvar* net_socksEnabled;
extern Cvar* net_socksServer;
extern Cvar* net_socksPort;
extern Cvar* net_socksUsername;
extern Cvar* net_socksPassword;
