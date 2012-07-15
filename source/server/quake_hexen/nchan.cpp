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

#include "../server.h"
#include "local.h"

// check to see if client block will fit, if not, rotate buffers
void SVQH_ClientReliableCheckBlock(client_t* cl, int maxsize)
{
	//	Only in QuakeWorld.
	if (GGameType & GAME_Hexen2)
	{
		return;
	}

	if (cl->qw_num_backbuf ||
		cl->netchan.message.cursize >
		cl->netchan.message.maxsize - maxsize - 1)
	{
		// we would probably overflow the buffer, save it for next
		if (!cl->qw_num_backbuf)
		{
			cl->qw_backbuf.InitOOB(cl->qw_backbuf_data[0], sizeof(cl->qw_backbuf_data[0]));
			cl->qw_backbuf.allowoverflow = true;
			cl->qw_backbuf_size[0] = 0;
			cl->qw_num_backbuf++;
		}

		if (cl->qw_backbuf.cursize > cl->qw_backbuf.maxsize - maxsize - 1)
		{
			if (cl->qw_num_backbuf == MAX_BACK_BUFFERS)
			{
				common->Printf("WARNING: MAX_BACK_BUFFERS for %s\n", cl->name);
				cl->qw_backbuf.cursize = 0;// don't overflow without allowoverflow set
				cl->netchan.message.overflowed = true;	// this will drop the client
				return;
			}
			cl->qw_backbuf.InitOOB(cl->qw_backbuf_data[cl->qw_num_backbuf], sizeof(cl->qw_backbuf_data[cl->qw_num_backbuf]));
			cl->qw_backbuf.allowoverflow = true;
			cl->qw_backbuf_size[cl->qw_num_backbuf] = 0;
			cl->qw_num_backbuf++;
		}
	}
}

// begin a client block, estimated maximum size
void SVQH_ClientReliableWrite_Begin(client_t* cl, int c, int maxsize)
{
	SVQH_ClientReliableCheckBlock(cl, maxsize);
	SVQH_ClientReliableWrite_Byte(cl, c);
}

void SVQH_ClientReliable_FinishWrite(client_t* cl)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf_size[cl->qw_num_backbuf - 1] = cl->qw_backbuf.cursize;

		if (cl->qw_backbuf.overflowed)
		{
			common->Printf("WARNING: backbuf [%d] reliable overflow for %s\n",cl->qw_num_backbuf,cl->name);
			cl->netchan.message.overflowed = true;	// this will drop the client
		}
	}
}

void SVQH_ClientReliableWrite_Angle(client_t* cl, float f)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteAngle(f);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteAngle(f);
	}
}

void SVQH_ClientReliableWrite_Angle16(client_t* cl, float f)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteAngle16(f);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteAngle16(f);
	}
}

void SVQH_ClientReliableWrite_Byte(client_t* cl, int c)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteByte(c);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteByte(c);
	}
}

void SVQH_ClientReliableWrite_Char(client_t* cl, int c)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteChar(c);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteChar(c);
	}
}

void SVQH_ClientReliableWrite_Float(client_t* cl, float f)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteFloat(f);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteFloat(f);
	}
}

void SVQH_ClientReliableWrite_Coord(client_t* cl, float f)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteCoord(f);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteCoord(f);
	}
}

void SVQH_ClientReliableWrite_Long(client_t* cl, int c)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteLong(c);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteLong(c);
	}
}

void SVQH_ClientReliableWrite_Short(client_t* cl, int c)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteShort(c);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteShort(c);
	}
}

void SVQH_ClientReliableWrite_String(client_t* cl, const char* s)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteString2(s);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteString2(s);
	}
}

void SVQH_ClientReliableWrite_SZ(client_t* cl, const void* data, int len)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteData(data, len);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteData(data, len);
	}
}

void Loop_SearchForHosts(bool xmit)
{
	if (sv.state == SS_DEAD)
	{
		return;
	}

	hostCacheCount = 1;
	if (String::Cmp(sv_hostname->string, "UNNAMED") == 0)
	{
		String::Cpy(hostcache[0].name, "local");
	}
	else
	{
		String::Cpy(hostcache[0].name, sv_hostname->string);
	}
	String::Cpy(hostcache[0].map, sv.name);
	hostcache[0].users = net_activeconnections;
	hostcache[0].maxusers = svs.qh_maxclients;
	hostcache[0].driver = 0;
	String::Cpy(hostcache[0].cname, "local");
}
