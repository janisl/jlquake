/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

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
	"q3svc_snapshot",
	"q3svc_EOF"
};

void SHOWNET(QMsg* msg, const char* s)
{
	if (cl_shownet->integer >= 2)
	{
		common->Printf("%3i:%s\n", msg->readcount - 1, s);
	}
}


/*
=========================================================================

MESSAGE PARSING

=========================================================================
*/
#if 1

qboolean isEntVisible(etentityState_t* ent)
{
	q3trace_t tr;
	vec3_t start, end, temp;
	vec3_t forward, up, right, right2;
	float view_height;

	VectorCopy(cl.wm_cgameClientLerpOrigin, start);
	start[2] += (cl.et_snap.ps.viewheight - 1);
	if (cl.et_snap.ps.leanf != 0)
	{
		vec3_t lright, v3ViewAngles;
		VectorCopy(cl.et_snap.ps.viewangles, v3ViewAngles);
		v3ViewAngles[2] += cl.et_snap.ps.leanf / 2.0f;
		AngleVectors(v3ViewAngles, NULL, lright, NULL);
		VectorMA(start, cl.et_snap.ps.leanf, lright, start);
	}

	VectorCopy(ent->pos.trBase, end);

	// Compute vector perpindicular to view to ent
	VectorSubtract(end, start, forward);
	VectorNormalizeFast(forward);
	VectorSet(up, 0, 0, 1);
	CrossProduct(forward, up, right);
	VectorNormalizeFast(right);
	VectorScale(right, 10, right2);
	VectorScale(right, 18, right);

	// Set viewheight
	if (ent->animMovetype)
	{
		view_height = 16;
	}
	else
	{
		view_height = 40;
	}

	// First, viewpoint to viewpoint
	end[2] += view_height;
	CM_BoxTraceQ3(&tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// First-b, viewpoint to top of head
	end[2] += 16;
	CM_BoxTraceQ3(&tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}
	end[2] -= 16;

	// Second, viewpoint to ent's origin
	end[2] -= view_height;
	CM_BoxTraceQ3(&tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Third, to ent's right knee
	VectorAdd(end, right, temp);
	temp[2] += 8;
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Fourth, to ent's right shoulder
	VectorAdd(end, right2, temp);
	if (ent->animMovetype)
	{
		temp[2] += 28;
	}
	else
	{
		temp[2] += 52;
	}
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Fifth, to ent's left knee
	VectorScale(right, -1, right);
	VectorScale(right2, -1, right2);
	VectorAdd(end, right2, temp);
	temp[2] += 2;
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Sixth, to ent's left shoulder
	VectorAdd(end, right, temp);
	if (ent->animMovetype)
	{
		temp[2] += 16;
	}
	else
	{
		temp[2] += 36;
	}
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	return false;
}

#endif

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity(QMsg* msg, etclSnapshot_t* frame, int newnum, etentityState_t* old,
	qboolean unchanged)
{
	etentityState_t* state;

	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	state = &cl.et_parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES_Q3 - 1)];

	if (unchanged)
	{
		*state = *old;
	}
	else
	{
		MSGET_ReadDeltaEntity(msg, old, state, newnum);
	}

	if (state->number == (MAX_GENTITIES_Q3 - 1))
	{
		return;		// entity was delta removed
	}

#if 1
	// DHM - Nerve :: Only draw clients if visible
	if (clc.wm_onlyVisibleClients)
	{
		if (state->number < MAX_CLIENTS_ET)
		{
			if (isEntVisible(state))
			{
				entLastVisible[state->number] = frame->serverTime;
				state->eFlags &= ~ETEF_NODRAW;
			}
			else
			{
				if (entLastVisible[state->number] < (frame->serverTime - 600))
				{
					state->eFlags |= ETEF_NODRAW;
				}
			}
		}
	}
#endif

	cl.parseEntitiesNum++;
	frame->numEntities++;
}

/*
==================
CL_ParsePacketEntities

==================
*/
void CL_ParsePacketEntities(QMsg* msg, etclSnapshot_t* oldframe, etclSnapshot_t* newframe)
{
	int newnum;
	etentityState_t* oldstate;
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
			oldstate = &cl.et_parseEntities[
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
				common->Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			}
			CL_DeltaEntity(msg, newframe, oldnum, oldstate, true);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.et_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
		}
		if (oldnum == newnum)
		{
			// delta from previous state
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  delta: %i\n", msg->readcount, newnum);
			}
			CL_DeltaEntity(msg, newframe, newnum, oldstate, false);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.et_parseEntities[
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
				common->Printf("%3i:  baseline: %i\n", msg->readcount, newnum);
			}
			CL_DeltaEntity(msg, newframe, newnum, &cl.et_entityBaselines[newnum], false);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{
		// one or more entities from the old packet are unchanged
		if (cl_shownet->integer == 3)
		{
			common->Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		}
		CL_DeltaEntity(msg, newframe, oldnum, oldstate, true);

		oldindex++;

		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.et_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}

	if (cl_shownuments->integer)
	{
		common->Printf("Entities in packet: %i\n", newframe->numEntities);
	}
}


/*
================
CL_ParseSnapshot

If the snapshot is parsed properly, it will be copied to
cl.et_snap and saved in cl.et_snapshots[].  If the snapshot is invalid
for any reason, no changes to the state will be made at all.
================
*/
void CL_ParseSnapshot(QMsg* msg)
{
	int len;
	etclSnapshot_t* old;
	etclSnapshot_t newSnap;
	int deltaNum;
	int oldMessageNum;
	int i, packetNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.q3_reliableAcknowledge = msg->ReadLong();

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.et_snap if it is valid
	memset(&newSnap, 0, sizeof(newSnap));

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
		newSnap.valid = true;		// uncompressed frame
		old = NULL;
		if (clc.demorecording)
		{
			clc.q3_demowaiting = false;	// we can start recording now
//			if(cl_autorecord->integer) {
//				Cvar_Set( "g_synchronousClients", "0" );
//			}
		}
		else
		{
			if (cl_autorecord->integer /*&& Cvar_VariableValue( "g_synchronousClients")*/)
			{
				char name[256];
				char mapname[MAX_QPATH];
				char* period;
				qtime_t time;

				Com_RealTime(&time);

				String::NCpyZ(mapname, cl.q3_mapname, MAX_QPATH);
				for (period = mapname; *period; period++)
				{
					if (*period == '.')
					{
						*period = '\0';
						break;
					}
				}

				for (period = mapname; *period; period++)
				{
					if (*period == '/')
					{
						break;
					}
				}
				if (*period)
				{
					period++;
				}

				String::Sprintf(name, sizeof(name), "demos/%s_%i_%i.dm_%d", period, time.tm_mday, time.tm_mon + 1, ETPROTOCOL_VERSION);

				CL_Record(name);
			}
		}
	}
	else
	{
		old = &cl.et_snapshots[newSnap.deltaNum & PACKET_MASK_Q3];
		if (!old->valid)
		{
			// should never happen
			common->Printf("Delta from invalid frame (not supposed to happen!).\n");
		}
		else if (old->messageNum != newSnap.deltaNum)
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			common->DPrintf("Delta frame too old.\n");
		}
		else if (cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES_Q3 - 128)
		{
			common->DPrintf("Delta parseEntitiesNum too old.\n");
		}
		else
		{
			newSnap.valid = true;	// valid delta parse
		}
	}

	// read areamask
	len = msg->ReadByte();

	if (len > (int)sizeof(newSnap.areamask))
	{
		Com_Error(ERR_DROP,"CL_ParseSnapshot: Invalid size %d for areamask.", len);
		return;
	}

	msg->ReadData(&newSnap.areamask, len);

	// read playerinfo
	SHOWNET(msg, "playerstate");
	if (old)
	{
		MSGET_ReadDeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	}
	else
	{
		MSGET_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);
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
	oldMessageNum = cl.et_snap.messageNum + 1;

	if (newSnap.messageNum - oldMessageNum >= PACKET_BACKUP_Q3)
	{
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP_Q3 - 1);
	}
	for (; oldMessageNum < newSnap.messageNum; oldMessageNum++)
	{
		cl.et_snapshots[oldMessageNum & PACKET_MASK_Q3].valid = false;
	}

	// copy to the current good spot
	cl.et_snap = newSnap;
	cl.et_snap.ping = 999;
	// calculate ping time
	for (i = 0; i < PACKET_BACKUP_Q3; i++)
	{
		packetNum = (clc.netchan.outgoingSequence - 1 - i) & PACKET_MASK_Q3;
		if (cl.et_snap.ps.commandTime >= cl.q3_outPackets[packetNum].p_serverTime)
		{
			cl.et_snap.ping = cls.realtime - cl.q3_outPackets[packetNum].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	cl.et_snapshots[cl.et_snap.messageNum & PACKET_MASK_Q3] = cl.et_snap;

	if (cl_shownet->integer == 3)
	{
		common->Printf("   snapshot:%i  delta:%i  ping:%i\n", cl.et_snap.messageNum,
			cl.et_snap.deltaNum, cl.et_snap.ping);
	}

	cl.q3_newSnapshots = true;
}


//=====================================================================

/*
==================
CL_ParseGamestate
==================
*/
void CL_ParseGamestate(QMsg* msg)
{
	int i;
	etentityState_t* es;
	int newnum;
	etentityState_t nullstate;
	int cmd;
	const char* s;

	Con_Close();

	clc.q3_connectPacketCount = 0;

	// wipe local client state
	CL_ClearState();

	// a gamestate always marks a server command sequence
	clc.q3_serverCommandSequence = msg->ReadLong();

	// parse all the configstrings and baselines
	cl.et_gameState.dataCount = 1;	// leave a 0 at the beginning for uninitialized configstrings
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
			if (i < 0 || i >= MAX_CONFIGSTRINGS_ET)
			{
				common->Error("configstring > MAX_CONFIGSTRINGS_ET");
			}
			s = msg->ReadBigString();
			len = String::Length(s);

			if (len + 1 + cl.et_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
			{
				common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
			}

			// append it to the gameState string buffer
			cl.et_gameState.stringOffsets[i] = cl.et_gameState.dataCount;
			memcpy(cl.et_gameState.stringData + cl.et_gameState.dataCount, s, len + 1);
			cl.et_gameState.dataCount += len + 1;
		}
		else if (cmd == q3svc_baseline)
		{
			newnum = msg->ReadBits(GENTITYNUM_BITS_Q3);
			if (newnum < 0 || newnum >= MAX_GENTITIES_Q3)
			{
				common->Error("Baseline number out of range: %i", newnum);
			}
			memset(&nullstate, 0, sizeof(nullstate));
			es = &cl.et_entityBaselines[newnum];
			MSGET_ReadDeltaEntity(msg, &nullstate, es, newnum);
		}
		else
		{
			common->Error("CL_ParseGamestate: bad command byte");
		}
	}

	clc.q3_clientNum = msg->ReadLong();
	// read the checksum feed
	clc.q3_checksumFeed = msg->ReadLong();

	// parse serverId and other cvars
	CLT3_SystemInfoChanged();

	// Arnout: verify if we have all official pakfiles. As we won't
	// be downloading them, we should be kicked for not having them.
	if (cl_connectedToPureServer && !FS_VerifyOfficialPaks())
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

	if (!*cls.et_downloadTempName)
	{
		common->Printf("Server sending download, but no download was requested\n");
		CL_AddReliableCommand("stopdl");
		return;
	}

	// read the data
	block = msg->ReadShort();

	// TTimo - www dl
	// if we haven't acked the download redirect yet
	if (block == -1)
	{
		if (!clc.et_bWWWDl)
		{
			// server is sending us a www download
			String::NCpyZ(cls.et_originalDownloadName, cls.et_downloadName, sizeof(cls.et_originalDownloadName));
			String::NCpyZ(cls.et_downloadName, msg->ReadString(), sizeof(cls.et_downloadName));
			clc.downloadSize = msg->ReadLong();
			clc.downloadFlags = msg->ReadLong();
			if (clc.downloadFlags & (1 << DL_FLAG_URL))
			{
				Sys_OpenURL(cls.et_downloadName, true);
				Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
				CL_AddReliableCommand("wwwdl bbl8r");	// not sure if that's the right msg
				clc.et_bWWWDlAborting = true;
				return;
			}
			Cvar_SetValue("cl_downloadSize", clc.downloadSize);
			common->DPrintf("Server redirected download: %s\n", cls.et_downloadName);
			clc.et_bWWWDl = true;	// activate wwwdl client loop
			CL_AddReliableCommand("wwwdl ack");
			// make sure the server is not trying to redirect us again on a bad checksum
			if (strstr(clc.et_badChecksumList, va("@%s", cls.et_originalDownloadName)))
			{
				common->Printf("refusing redirect to %s by server (bad checksum)\n", cls.et_downloadName);
				CL_AddReliableCommand("wwwdl fail");
				clc.et_bWWWDlAborting = true;
				return;
			}
			// make downloadTempName an OS path
			String::NCpyZ(cls.et_downloadTempName, FS_BuildOSPath(Cvar_VariableString("fs_homepath"), cls.et_downloadTempName, ""), sizeof(cls.et_downloadTempName));
			cls.et_downloadTempName[String::Length(cls.et_downloadTempName) - 1] = '\0';
			if (!DL_BeginDownload(cls.et_downloadTempName, cls.et_downloadName, com_developer->integer))
			{
				// setting bWWWDl to false after sending the wwwdl fail doesn't work
				// not sure why, but I suspect we have to eat all remaining block -1 that the server has sent us
				// still leave a flag so that CL_WWWDownload is inactive
				// we count on server sending us a gamestate to start up clean again
				CL_AddReliableCommand("wwwdl fail");
				clc.et_bWWWDlAborting = true;
				common->Printf("Failed to initialize download for '%s'\n", cls.et_downloadName);
			}
			// Check for a disconnected download
			// we'll let the server disconnect us when it gets the bbl8r message
			if (clc.downloadFlags & (1 << DL_FLAG_DISCON))
			{
				CL_AddReliableCommand("wwwdl bbl8r");
				cls.et_bWWWDlDisconnected = true;
			}
			return;
		}
		else
		{
			// server keeps sending that message till we ack it, eat and ignore
			//msg->ReadLong();
			msg->ReadString();
			msg->ReadLong();
			msg->ReadLong();
			return;
		}
	}

	if (!block)
	{
		// block zero is special, contains file size
		clc.downloadSize = msg->ReadLong();

		Cvar_SetValue("cl_downloadSize", clc.downloadSize);

		if (clc.downloadSize < 0)
		{
			common->Error("%s", msg->ReadString());
			return;
		}
	}

	size = msg->ReadShort();
	if (size < 0 || size > (int)sizeof(data))
	{
		common->Error("CL_ParseDownload: Invalid size %d for download chunk.", size);
		return;
	}

	msg->ReadData(data, size);

	if (clc.downloadBlock != block)
	{
		common->DPrintf("CL_ParseDownload: Expected block %d, got %d\n", clc.downloadBlock, block);
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		clc.download = FS_SV_FOpenFileWrite(cls.et_downloadTempName);

		if (!clc.download)
		{
			common->Printf("Could not create %s\n", cls.et_downloadTempName);
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
			FS_SV_Rename(cls.et_downloadTempName, cls.et_downloadName);
		}
		*cls.et_downloadTempName = *cls.et_downloadName = 0;
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

	index = seq & (MAX_RELIABLE_COMMANDS_WOLF - 1);
	String::NCpyZ(clc.q3_serverCommands[index], s, sizeof(clc.q3_serverCommands[index]));
}

/*
=====================
CL_ParseBinaryMessage
=====================
*/
void CL_ParseBinaryMessage(QMsg* msg)
{
	int size;

	msg->Uncompressed();

	size = msg->cursize - msg->readcount;
	if (size <= 0 || size > MAX_BINARY_MESSAGE_ET)
	{
		return;
	}

	CLET_CGameBinaryMessageReceived((char*)&msg->_data[msg->readcount], size, cl.et_snap.serverTime);
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
			SHOWNET(msg, "END OF MESSAGE");
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
				SHOWNET(msg, svc_strings[cmd]);
			}
		}

		// other commands
		switch (cmd)
		{
		default:
			Com_Error(ERR_DROP,"CL_ParseServerMessage: Illegible server message %d\n", cmd);
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

	CL_ParseBinaryMessage(msg);
}
