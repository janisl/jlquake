/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cl_parse.c  -- parse a message received from the server

#include "client.h"

const char* svc_strings[256] = {
	"q3svc_bad",

	"q3svc_nop",
	"q3svc_gamestate",
	"q3svc_configstring",
	"q3svc_baseline",
	"q3svc_serverCommand",
	"q3svc_download",
	"q3svc_snapshot"
};

/*
=========================================================================

MESSAGE PARSING

=========================================================================
*/

void CLT3_ParseGamestate(QMsg* msg)
{
	Con_Close();

	clc.q3_connectPacketCount = 0;

	// wipe local client state
	CL_ClearState();

	// a gamestate always marks a server command sequence
	clc.q3_serverCommandSequence = msg->ReadLong();

	// parse all the configstrings and baselines
	if (GGameType & GAME_Quake3)
	{
		cl.q3_gameState.dataCount = 1;	// leave a 0 at the beginning for uninitialized configstrings
	}
	else if (GGameType & GAME_WolfSP)
	{
		cl.ws_gameState.dataCount = 1;	// leave a 0 at the beginning for uninitialized configstrings
	}
	else if (GGameType & GAME_WolfMP)
	{
		cl.wm_gameState.dataCount = 1;	// leave a 0 at the beginning for uninitialized configstrings
	}
	else
	{
		cl.et_gameState.dataCount = 1;	// leave a 0 at the beginning for uninitialized configstrings
	}
	while (1)
	{
		int cmd = msg->ReadByte();

		if (cmd == q3svc_EOF)
		{
			break;
		}

		if (cmd == q3svc_configstring)
		{
			int i = msg->ReadShort();
			int maxConfigStrings = GGameType & GAME_WolfSP ? MAX_CONFIGSTRINGS_WS :
				GGameType & GAME_WolfMP ? MAX_CONFIGSTRINGS_WM :
				GGameType & GAME_ET ? MAX_CONFIGSTRINGS_ET : MAX_CONFIGSTRINGS_Q3;
			if (i < 0 || i >= maxConfigStrings)
			{
				common->Error("configstring > MAX_CONFIGSTRINGS");
			}
			const char* s = msg->ReadBigString();
			int len = String::Length(s);

			if (GGameType & GAME_Quake3)
			{
				if (len + 1 + cl.q3_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
				{
					common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
				}

				// append it to the gameState string buffer
				cl.q3_gameState.stringOffsets[i] = cl.q3_gameState.dataCount;
				Com_Memcpy(cl.q3_gameState.stringData + cl.q3_gameState.dataCount, s, len + 1);
				cl.q3_gameState.dataCount += len + 1;
			}
			else if (GGameType & GAME_WolfSP)
			{
				if (len + 1 + cl.ws_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
				{
					common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
				}

				// append it to the gameState string buffer
				cl.ws_gameState.stringOffsets[i] = cl.ws_gameState.dataCount;
				Com_Memcpy(cl.ws_gameState.stringData + cl.ws_gameState.dataCount, s, len + 1);
				cl.ws_gameState.dataCount += len + 1;
			}
			else if (GGameType & GAME_WolfMP)
			{
				if (len + 1 + cl.wm_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
				{
					common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
				}

				// append it to the gameState string buffer
				cl.wm_gameState.stringOffsets[i] = cl.wm_gameState.dataCount;
				Com_Memcpy(cl.wm_gameState.stringData + cl.wm_gameState.dataCount, s, len + 1);
				cl.wm_gameState.dataCount += len + 1;
			}
			else
			{
				if (len + 1 + cl.et_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
				{
					common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
				}

				// append it to the gameState string buffer
				cl.et_gameState.stringOffsets[i] = cl.et_gameState.dataCount;
				Com_Memcpy(cl.et_gameState.stringData + cl.et_gameState.dataCount, s, len + 1);
				cl.et_gameState.dataCount += len + 1;
			}
		}
		else if (cmd == q3svc_baseline)
		{
			int newnum = msg->ReadBits(GENTITYNUM_BITS_Q3);
			if (newnum < 0 || newnum >= MAX_GENTITIES_Q3)
			{
				common->Error("Baseline number out of range: %i", newnum);
			}
			if (GGameType & GAME_Quake3)
			{
				q3entityState_t nullstate = {};
				q3entityState_t* es = &cl.q3_entityBaselines[newnum];
				MSGQ3_ReadDeltaEntity(msg, &nullstate, es, newnum);
			}
			else if (GGameType & GAME_WolfSP)
			{
				wsentityState_t nullstate = {};
				wsentityState_t* es = &cl.ws_entityBaselines[newnum];
				MSGWS_ReadDeltaEntity(msg, &nullstate, es, newnum);
			}
			else if (GGameType & GAME_WolfMP)
			{
				wmentityState_t nullstate = {};
				wmentityState_t* es = &cl.wm_entityBaselines[newnum];
				MSGWM_ReadDeltaEntity(msg, &nullstate, es, newnum);
			}
			else
			{
				etentityState_t nullstate = {};
				etentityState_t* es = &cl.et_entityBaselines[newnum];
				MSGET_ReadDeltaEntity(msg, &nullstate, es, newnum);
			}
		}
		else
		{
			common->Error("CLT3_ParseGamestate: bad command byte");
		}
	}

	clc.q3_clientNum = msg->ReadLong();
	// read the checksum feed
	clc.q3_checksumFeed = msg->ReadLong();

	// parse serverId and other cvars
	CLT3_SystemInfoChanged();

	// Arnout: verify if we have all official pakfiles. As we won't
	// be downloading them, we should be kicked for not having them.
	if (GGameType & GAME_ET && cl_connectedToPureServer && !FS_VerifyOfficialPaks())
	{
		common->Error("Couldn't load an official pak file; verify your installation and make sure it has been updated to the latest version.");
	}

	// reinitialize the filesystem if the game directory has changed
	FS_ConditionalRestart(clc.q3_checksumFeed);

	// This used to call CL_StartHunkUsers, but now we enter the download state before loading the
	// cgame
	CL_InitDownloads();

	// make sure the game starts
	Cvar_Set("cl_paused", "0");
}

//=====================================================================

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload(QMsg* msg)
{
	int size;
	unsigned char data[MAX_MSGLEN_WOLF];
	int block;

	// read the data
	block = msg->ReadShort();

	if (!block)
	{
		// block zero is special, contains file size
		clc.downloadSize = msg->ReadLong();

		Cvar_SetValue("cl_downloadSize", clc.downloadSize);

		if (clc.downloadSize < 0)
		{
			common->Error(msg->ReadString());
			return;
		}
	}

	size = msg->ReadShort();
	if (size > 0)
	{
		msg->ReadData(data, size);
	}

	if (clc.downloadBlock != block)
	{
		common->DPrintf("CL_ParseDownload: Expected block %d, got %d\n", clc.downloadBlock, block);
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		if (!*clc.downloadTempName)
		{
			common->Printf("Server sending download, but no download was requested\n");
			CL_AddReliableCommand("stopdl");
			return;
		}

		clc.download = FS_SV_FOpenFileWrite(clc.downloadTempName);

		if (!clc.download)
		{
			common->Printf("Could not create %s\n", clc.downloadTempName);
			CL_AddReliableCommand("stopdl");
			CL_NextDownload();
			return;
		}
	}

	if (size)
	{
		FS_Write(data, size, clc.download);
	}

	CL_AddReliableCommand(va("nextdl %d", clc.downloadBlock));
	clc.downloadBlock++;

	clc.downloadCount += size;

	// So UI gets access to it
	Cvar_SetValue("cl_downloadCount", clc.downloadCount);

	if (!size)		// A zero length block means EOF
	{
		if (clc.download)
		{
			FS_FCloseFile(clc.download);
			clc.download = 0;

			// rename the file
			FS_SV_Rename(clc.downloadTempName, clc.downloadName);
		}
		*clc.downloadTempName = *clc.downloadName = 0;
		Cvar_Set("cl_downloadName", "");

		// send intentions now
		// We need this because without it, we would hold the last nextdl and then start
		// loading right away.  If we take a while to load, the server is happily trying
		// to send us that last block over and over.
		// Write it twice to help make sure we acknowledge the download
		CLT3_WritePacket();
		CLT3_WritePacket();

		// get another file if needed
		CL_NextDownload();
	}
}

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage(QMsg* msg)
{
	int cmd;
	QMsg msgback;

	msgback = *msg;

	if (cl_shownet->integer == 1)
	{
		common->Printf("%i ",msg->cursize);
	}
	else if (cl_shownet->integer >= 2)
	{
		common->Printf("------------------\n");
	}

	msg->Bitstream();

	// get the reliable sequence acknowledge number
	clc.q3_reliableAcknowledge = msg->ReadLong();
	//
	if (clc.q3_reliableAcknowledge < clc.q3_reliableSequence - MAX_RELIABLE_COMMANDS_WOLF)
	{
		clc.q3_reliableAcknowledge = clc.q3_reliableSequence;
	}

	//
	// parse the message
	//
	while (1)
	{
		if (msg->readcount > msg->cursize)
		{
			Com_Error(ERR_DROP,"CL_ParseServerMessage: read past end of server message");
			break;
		}

		cmd = msg->ReadByte();

		if (cmd == q3svc_EOF)
		{
			SHOWNET(*msg, "END OF MESSAGE");
			break;
		}

		if (cl_shownet->integer >= 2)
		{
			if (!svc_strings[cmd])
			{
				common->Printf("%3i:BAD CMD %i\n", msg->readcount - 1, cmd);
			}
			else
			{
				SHOWNET(*msg, svc_strings[cmd]);
			}
		}

		// other commands
		switch (cmd)
		{
		default:
			Com_Error(ERR_DROP,"CL_ParseServerMessage: Illegible server message\n");
			break;
		case q3svc_nop:
			break;
		case q3svc_serverCommand:
			CLT3_ParseCommandString(msg);
			break;
		case q3svc_gamestate:
			CLT3_ParseGamestate(msg);
			break;
		case q3svc_snapshot:
			CLWS_ParseSnapshot(msg);
			break;
		case q3svc_download:
			CL_ParseDownload(msg);
			break;
		}
	}
}
