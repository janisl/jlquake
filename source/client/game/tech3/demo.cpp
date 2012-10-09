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

#include "../../client.h"
#include "local.h"

void CLT3_Record(const char* demoName, const char* name)
{
	int i;
	char* s;
	int len;

	// open the demo file
	common->Printf("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}
	clc.demorecording = true;
	if (GGameType & GAME_Quake3)
	{
		if (Cvar_VariableValue("ui_recordSPDemo"))
		{
			clc.q3_spDemoRecording = true;
		}
		else
		{
			clc.q3_spDemoRecording = false;
		}
	}

	String::NCpyZ(clc.q3_demoName, demoName, sizeof(clc.q3_demoName));
	if (GGameType & GAME_ET)
	{
		Cvar_Set("cl_demorecording", "1");	// fretn
		Cvar_Set("cl_demofilename", clc.q3_demoName);	// bani
		Cvar_Set("cl_demooffset", "0");		// bani
	}

	// don't start saving messages until a non-delta compressed message is received
	clc.q3_demowaiting = true;

	// write out the gamestate message
	QMsg buf;
	byte bufData[MAX_MSGLEN];
	buf.Init(bufData, GGameType & GAME_Quake3 ? MAX_MSGLEN_Q3 : MAX_MSGLEN_WOLF);
	buf.Bitstream();

	// NOTE, MRE: all server->client messages now acknowledge
	buf.WriteLong(clc.q3_reliableSequence);

	buf.WriteByte(q3svc_gamestate);
	buf.WriteLong(clc.q3_serverCommandSequence);

	if (GGameType & GAME_Quake3)
	{
		// configstrings
		for (i = 0; i < MAX_CONFIGSTRINGS_Q3; i++)
		{
			if (!cl.q3_gameState.stringOffsets[i])
			{
				continue;
			}
			s = cl.q3_gameState.stringData + cl.q3_gameState.stringOffsets[i];
			buf.WriteByte(q3svc_configstring);
			buf.WriteShort(i);
			buf.WriteBigString(s);
		}

		// baselines
		q3entityState_t nullstate = {};
		for (i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			q3entityState_t* ent = &cl.q3_entityBaselines[i];
			if (!ent->number)
			{
				continue;
			}
			buf.WriteByte(q3svc_baseline);
			MSGQ3_WriteDeltaEntity(&buf, &nullstate, ent, true);
		}
	}
	else if (GGameType & GAME_WolfSP)
	{
		// configstrings
		for (i = 0; i < MAX_CONFIGSTRINGS_WS; i++)
		{
			if (!cl.ws_gameState.stringOffsets[i])
			{
				continue;
			}
			s = cl.ws_gameState.stringData + cl.ws_gameState.stringOffsets[i];
			buf.WriteByte(q3svc_configstring);
			buf.WriteShort(i);
			buf.WriteBigString(s);
		}

		// baselines
		wsentityState_t nullstate = {};
		for (i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			wsentityState_t* ent = &cl.ws_entityBaselines[i];
			if (!ent->number)
			{
				continue;
			}
			buf.WriteByte(q3svc_baseline);
			MSGWS_WriteDeltaEntity(&buf, &nullstate, ent, true);
		}
	}
	else if (GGameType & GAME_WolfMP)
	{
		// configstrings
		for (i = 0; i < MAX_CONFIGSTRINGS_WM; i++)
		{
			if (!cl.wm_gameState.stringOffsets[i])
			{
				continue;
			}
			s = cl.wm_gameState.stringData + cl.wm_gameState.stringOffsets[i];
			buf.WriteByte(q3svc_configstring);
			buf.WriteShort(i);
			buf.WriteBigString(s);
		}

		// baselines
		wmentityState_t nullstate = {};
		for (i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			wmentityState_t* ent = &cl.wm_entityBaselines[i];
			if (!ent->number)
			{
				continue;
			}
			buf.WriteByte(q3svc_baseline);
			MSGWM_WriteDeltaEntity(&buf, &nullstate, ent, true);
		}
	}
	else
	{
		// configstrings
		for (i = 0; i < MAX_CONFIGSTRINGS_ET; i++)
		{
			if (!cl.et_gameState.stringOffsets[i])
			{
				continue;
			}
			s = cl.et_gameState.stringData + cl.et_gameState.stringOffsets[i];
			buf.WriteByte(q3svc_configstring);
			buf.WriteShort(i);
			buf.WriteBigString(s);
		}

		// baselines
		etentityState_t nullstate = {};
		for (i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			etentityState_t* ent = &cl.et_entityBaselines[i];
			if (!ent->number)
			{
				continue;
			}
			buf.WriteByte(q3svc_baseline);
			MSGET_WriteDeltaEntity(&buf, &nullstate, ent, true);
		}
	}

	buf.WriteByte(q3svc_EOF);

	// finished writing the gamestate stuff

	// write the client num
	buf.WriteLong(clc.q3_clientNum);
	// write the checksum feed
	buf.WriteLong(clc.q3_checksumFeed);

	// finished writing the client packet
	buf.WriteByte(q3svc_EOF);

	// write it to the demo file
	len = LittleLong(clc.q3_serverMessageSequence - 1);
	FS_Write(&len, 4, clc.demofile);

	len = LittleLong(buf.cursize);
	FS_Write(&len, 4, clc.demofile);
	FS_Write(buf._data, buf.cursize, clc.demofile);

	// the rest of the demo file will be copied from net messages
}
