/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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

void SHOWNET(QMsg* msg, const char* s)
{
	if (cl_shownet->integer >= 2)
	{
		Com_Printf("%3i:%s\n", msg->readcount - 1, s);
	}
}


/*
=========================================================================

MESSAGE PARSING

=========================================================================
*/

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity(QMsg* msg, q3clSnapshot_t* frame, int newnum, q3entityState_t* old,
	qboolean unchanged)
{
	q3entityState_t* state;

	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	state = &cl.q3_parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES_Q3 - 1)];

	if (unchanged)
	{
		*state = *old;
	}
	else
	{
		MSG_ReadDeltaEntity(msg, old, state, newnum);
	}

	if (state->number == (MAX_GENTITIES_Q3 - 1))
	{
		return;		// entity was delta removed
	}
	cl.parseEntitiesNum++;
	frame->numEntities++;
}

/*
==================
CL_ParsePacketEntities

==================
*/
void CL_ParsePacketEntities(QMsg* msg, q3clSnapshot_t* oldframe, q3clSnapshot_t* newframe)
{
	int newnum;
	q3entityState_t* oldstate;
	int oldindex, oldnum;

	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	oldstate = NULL;
	if (!oldframe)
	{
		oldnum = 99999;
	}
	else
	{
		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.q3_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		// read the entity index number
		newnum = msg->ReadBits(GENTITYNUM_BITS_Q3);

		if (newnum == (MAX_GENTITIES_Q3 - 1))
		{
			break;
		}

		if (msg->readcount > msg->cursize)
		{
			Com_Error(ERR_DROP,"CL_ParsePacketEntities: end of message");
		}

		while (oldnum < newnum)
		{
			// one or more entities from the old packet are unchanged
			if (cl_shownet->integer == 3)
			{
				Com_Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			}
			CL_DeltaEntity(msg, newframe, oldnum, oldstate, qtrue);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.q3_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
		}
		if (oldnum == newnum)
		{
			// delta from previous state
			if (cl_shownet->integer == 3)
			{
				Com_Printf("%3i:  delta: %i\n", msg->readcount, newnum);
			}
			CL_DeltaEntity(msg, newframe, newnum, oldstate, qfalse);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.q3_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{
			// delta from baseline
			if (cl_shownet->integer == 3)
			{
				Com_Printf("%3i:  baseline: %i\n", msg->readcount, newnum);
			}
			CL_DeltaEntity(msg, newframe, newnum, &cl.q3_entityBaselines[newnum], qfalse);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{
		// one or more entities from the old packet are unchanged
		if (cl_shownet->integer == 3)
		{
			Com_Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		}
		CL_DeltaEntity(msg, newframe, oldnum, oldstate, qtrue);

		oldindex++;

		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.q3_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}
}


/*
================
CL_ParseSnapshot

If the snapshot is parsed properly, it will be copied to
cl.snap and saved in cl.snapshots[].  If the snapshot is invalid
for any reason, no changes to the state will be made at all.
================
*/
void CL_ParseSnapshot(QMsg* msg)
{
	int len;
	q3clSnapshot_t* old;
	q3clSnapshot_t newSnap;
	int deltaNum;
	int oldMessageNum;
	int i, packetNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.q3_reliableAcknowledge = msg->ReadLong();

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.snap if it is valid
	Com_Memset(&newSnap, 0, sizeof(newSnap));

	// we will have read any new server commands in this
	// message before we got to q3svc_snapshot
	newSnap.serverCommandNum = clc.q3_serverCommandSequence;

	newSnap.serverTime = msg->ReadLong();

	newSnap.messageNum = clc.q3_serverMessageSequence;

	deltaNum = msg->ReadByte();
	if (!deltaNum)
	{
		newSnap.deltaNum = -1;
	}
	else
	{
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = msg->ReadByte();

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if (newSnap.deltaNum <= 0)
	{
		newSnap.valid = qtrue;		// uncompressed frame
		old = NULL;
		clc.q3_demowaiting = qfalse;	// we can start recording now
	}
	else
	{
		old = &cl.q3_snapshots[newSnap.deltaNum & PACKET_MASK_Q3];
		if (!old->valid)
		{
			// should never happen
			Com_Printf("Delta from invalid frame (not supposed to happen!).\n");
		}
		else if (old->messageNum != newSnap.deltaNum)
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_Printf("Delta frame too old.\n");
		}
		else if (cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES_Q3 - 128)
		{
			Com_Printf("Delta parseEntitiesNum too old.\n");
		}
		else
		{
			newSnap.valid = qtrue;	// valid delta parse
		}
	}

	// read areamask
	len = msg->ReadByte();
	msg->ReadData(&newSnap.areamask, len);

	// read playerinfo
	SHOWNET(msg, "playerstate");
	if (old)
	{
		MSG_ReadDeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	}
	else
	{
		MSG_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);
	}

	// read packet entities
	SHOWNET(msg, "packet entities");
	CL_ParsePacketEntities(msg, old, &newSnap);

	// if not valid, dump the entire thing now that it has
	// been properly read
	if (!newSnap.valid)
	{
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.q3_snap.messageNum + 1;

	if (newSnap.messageNum - oldMessageNum >= PACKET_BACKUP_Q3)
	{
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP_Q3 - 1);
	}
	for (; oldMessageNum < newSnap.messageNum; oldMessageNum++)
	{
		cl.q3_snapshots[oldMessageNum & PACKET_MASK_Q3].valid = qfalse;
	}

	// copy to the current good spot
	cl.q3_snap = newSnap;
	cl.q3_snap.ping = 999;
	// calculate ping time
	for (i = 0; i < PACKET_BACKUP_Q3; i++)
	{
		packetNum = (clc.netchan.outgoingSequence - 1 - i) & PACKET_MASK_Q3;
		if (cl.q3_snap.ps.commandTime >= cl.q3_outPackets[packetNum].p_serverTime)
		{
			cl.q3_snap.ping = cls.realtime - cl.q3_outPackets[packetNum].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	cl.q3_snapshots[cl.q3_snap.messageNum & PACKET_MASK_Q3] = cl.q3_snap;

	if (cl_shownet->integer == 3)
	{
		Com_Printf("   snapshot:%i  delta:%i  ping:%i\n", cl.q3_snap.messageNum,
			cl.q3_snap.deltaNum, cl.q3_snap.ping);
	}

	cl.q3_newSnapshots = qtrue;
}


//=====================================================================

int cl_connectedToPureServer;

/*
==================
CL_SystemInfoChanged

The systeminfo configstring has been changed, so parse
new information out of it.  This will happen at every
gamestate, and possibly during gameplay.
==================
*/
void CL_SystemInfoChanged(void)
{
	char* systemInfo;
	const char* s, * t;
	char key[BIG_INFO_KEY];
	char value[BIG_INFO_VALUE];
	qboolean gameSet;

	systemInfo = cl.q3_gameState.stringData + cl.q3_gameState.stringOffsets[Q3CS_SYSTEMINFO];
	// NOTE TTimo:
	// when the serverId changes, any further messages we send to the server will use this new serverId
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=475
	// in some cases, outdated cp commands might get sent with this news serverId
	cl.q3_serverId = String::Atoi(Info_ValueForKey(systemInfo, "sv_serverid"));

	// don't set any vars when playing a demo
	if (clc.demoplaying)
	{
		return;
	}

	s = Info_ValueForKey(systemInfo, "sv_cheats");
	if (String::Atoi(s) == 0)
	{
		Cvar_SetCheatState();
	}

	// check pure server string
	s = Info_ValueForKey(systemInfo, "sv_paks");
	t = Info_ValueForKey(systemInfo, "sv_pakNames");
	FS_PureServerSetLoadedPaks(s, t);

	s = Info_ValueForKey(systemInfo, "sv_referencedPaks");
	t = Info_ValueForKey(systemInfo, "sv_referencedPakNames");
	FS_PureServerSetReferencedPaks(s, t);

	gameSet = qfalse;
	// scan through all the variables in the systeminfo and locally set cvars to match
	s = systemInfo;
	while (s)
	{
		Info_NextPair(&s, key, value);
		if (!key[0])
		{
			break;
		}
		// ehw!
		if (!String::ICmp(key, "fs_game"))
		{
			gameSet = qtrue;
		}

		Cvar_Set(key, value);
	}
	// if game folder should not be set and it is set at the client side
	if (!gameSet && *Cvar_VariableString("fs_game"))
	{
		Cvar_Set("fs_game", "");
	}
	cl_connectedToPureServer = Cvar_VariableValue("sv_pure");
}

/*
==================
CL_ParseGamestate
==================
*/
void CL_ParseGamestate(QMsg* msg)
{
	int i;
	q3entityState_t* es;
	int newnum;
	q3entityState_t nullstate;
	int cmd;
	const char* s;

	Con_Close();

	clc.q3_connectPacketCount = 0;

	// wipe local client state
	CL_ClearState();

	// a gamestate always marks a server command sequence
	clc.q3_serverCommandSequence = msg->ReadLong();

	// parse all the configstrings and baselines
	cl.q3_gameState.dataCount = 1;	// leave a 0 at the beginning for uninitialized configstrings
	while (1)
	{
		cmd = msg->ReadByte();

		if (cmd == q3svc_EOF)
		{
			break;
		}

		if (cmd == q3svc_configstring)
		{
			int len;

			i = msg->ReadShort();
			if (i < 0 || i >= MAX_CONFIGSTRINGS_Q3)
			{
				Com_Error(ERR_DROP, "configstring > MAX_CONFIGSTRINGS_Q3");
			}
			s = msg->ReadBigString();
			len = String::Length(s);

			if (len + 1 + cl.q3_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
			{
				Com_Error(ERR_DROP, "MAX_GAMESTATE_CHARS_Q3 exceeded");
			}

			// append it to the gameState string buffer
			cl.q3_gameState.stringOffsets[i] = cl.q3_gameState.dataCount;
			Com_Memcpy(cl.q3_gameState.stringData + cl.q3_gameState.dataCount, s, len + 1);
			cl.q3_gameState.dataCount += len + 1;
		}
		else if (cmd == q3svc_baseline)
		{
			newnum = msg->ReadBits(GENTITYNUM_BITS_Q3);
			if (newnum < 0 || newnum >= MAX_GENTITIES_Q3)
			{
				Com_Error(ERR_DROP, "Baseline number out of range: %i", newnum);
			}
			Com_Memset(&nullstate, 0, sizeof(nullstate));
			es = &cl.q3_entityBaselines[newnum];
			MSG_ReadDeltaEntity(msg, &nullstate, es, newnum);
		}
		else
		{
			Com_Error(ERR_DROP, "CL_ParseGamestate: bad command byte");
		}
	}

	clc.q3_clientNum = msg->ReadLong();
	// read the checksum feed
	clc.q3_checksumFeed = msg->ReadLong();

	// parse serverId and other cvars
	CL_SystemInfoChanged();

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
	unsigned char data[MAX_MSGLEN_Q3];
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
			Com_Error(ERR_DROP, msg->ReadString());
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
		Com_DPrintf("CL_ParseDownload: Expected block %d, got %d\n", clc.downloadBlock, block);
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		if (!*clc.downloadTempName)
		{
			Com_Printf("Server sending download, but no download was requested\n");
			CL_AddReliableCommand("stopdl");
			return;
		}

		clc.download = FS_SV_FOpenFileWrite(clc.downloadTempName);

		if (!clc.download)
		{
			Com_Printf("Could not create %s\n", clc.downloadTempName);
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

	if (!size)	// A zero length block means EOF
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
		CL_WritePacket();
		CL_WritePacket();

		// get another file if needed
		CL_NextDownload();
	}
}

/*
=====================
CL_ParseCommandString

Command strings are just saved off until cgame asks for them
when it transitions a snapshot
=====================
*/
void CL_ParseCommandString(QMsg* msg)
{
	const char* s;
	int seq;
	int index;

	seq = msg->ReadLong();
	s = msg->ReadString();

	// see if we have already executed stored it off
	if (clc.q3_serverCommandSequence >= seq)
	{
		return;
	}
	clc.q3_serverCommandSequence = seq;

	index = seq & (MAX_RELIABLE_COMMANDS_Q3 - 1);
	String::NCpyZ(clc.q3_serverCommands[index], s, sizeof(clc.q3_serverCommands[index]));
}


/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage(QMsg* msg)
{
	int cmd;

	if (cl_shownet->integer == 1)
	{
		Com_Printf("%i ",msg->cursize);
	}
	else if (cl_shownet->integer >= 2)
	{
		Com_Printf("------------------\n");
	}

	msg->Bitstream();

	// get the reliable sequence acknowledge number
	clc.q3_reliableAcknowledge = msg->ReadLong();
	//
	if (clc.q3_reliableAcknowledge < clc.q3_reliableSequence - MAX_RELIABLE_COMMANDS_Q3)
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
			SHOWNET(msg, "END OF MESSAGE");
			break;
		}

		if (cl_shownet->integer >= 2)
		{
			if (!svc_strings[cmd])
			{
				Com_Printf("%3i:BAD CMD %i\n", msg->readcount - 1, cmd);
			}
			else
			{
				SHOWNET(msg, svc_strings[cmd]);
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
			CL_ParseCommandString(msg);
			break;
		case q3svc_gamestate:
			CL_ParseGamestate(msg);
			break;
		case q3svc_snapshot:
			CL_ParseSnapshot(msg);
			break;
		case q3svc_download:
			CL_ParseDownload(msg);
			break;
		}
	}
}
