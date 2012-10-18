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

//	Create a new usercmd_t structure for this frame
static void CLT3_CreateNewCommands()
{
	// no need to create usercmds until we have a gamestate
	if (cls.state < CA_PRIMED)
	{
		return;
	}

	// generate a command for this frame
	cl.q3_cmdNumber++;
	int cmdNum = cl.q3_cmdNumber & CMD_MASK_Q3;
	in_usercmd_t inCmd = CL_CreateCmd();
	if (GGameType & GAME_WolfSP)
	{
		wsusercmd_t cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.buttons = inCmd.buttons & 0xff;
		cmd.wbuttons = inCmd.buttons >> 8;
		cmd.forwardmove = inCmd.forwardmove;
		cmd.rightmove = inCmd.sidemove;
		cmd.upmove = inCmd.upmove;
		cmd.wolfkick = inCmd.kick;
		cmd.weapon = inCmd.weapon;
		for (int i = 0; i < 3; i++)
		{
			cmd.angles[i] = inCmd.angles[i];
		}
		cmd.serverTime = inCmd.serverTime;
		cmd.holdable = inCmd.holdable;
		cmd.cld = inCmd.cld;
		cl.ws_cmds[cmdNum] = cmd;
	}
	else if (GGameType & GAME_WolfMP)
	{
		wmusercmd_t cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.buttons = inCmd.buttons & 0xff;
		cmd.wbuttons = inCmd.buttons >> 8;
		cmd.forwardmove = inCmd.forwardmove;
		cmd.rightmove = inCmd.sidemove;
		cmd.upmove = inCmd.upmove;
		cmd.wolfkick = inCmd.kick;
		cmd.weapon = inCmd.weapon;
		for (int i = 0; i < 3; i++)
		{
			cmd.angles[i] = inCmd.angles[i];
		}
		cmd.serverTime = inCmd.serverTime;
		cmd.holdable = inCmd.holdable;
		cmd.mpSetup = inCmd.mpSetup;
		cmd.identClient = inCmd.identClient;
		cl.wm_cmds[cmdNum] = cmd;
	}
	else if (GGameType & GAME_ET)
	{
		etusercmd_t cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.buttons = inCmd.buttons & 0xff;
		cmd.wbuttons = inCmd.buttons >> 8;
		cmd.forwardmove = inCmd.forwardmove;
		cmd.rightmove = inCmd.sidemove;
		cmd.upmove = inCmd.upmove;
		cmd.doubleTap = inCmd.doubleTap;
		cmd.weapon = inCmd.weapon;
		for (int i = 0; i < 3; i++)
		{
			cmd.angles[i] = inCmd.angles[i];
		}
		cmd.serverTime = inCmd.serverTime;
		cmd.identClient = inCmd.identClient;
		cmd.flags = inCmd.flags;
		cl.et_cmds[cmdNum] = cmd;
	}
	else
	{
		q3usercmd_t cmd;
		Com_Memset(&cmd, 0, sizeof(cmd));
		cmd.forwardmove = inCmd.forwardmove;
		cmd.rightmove = inCmd.sidemove;
		cmd.upmove = inCmd.upmove;
		cmd.buttons = inCmd.buttons;
		cmd.weapon = inCmd.weapon;
		for (int i = 0; i < 3; i++)
		{
			cmd.angles[i] = inCmd.angles[i];
		}
		cmd.serverTime = inCmd.serverTime;
		cl.q3_cmds[cmdNum] = cmd;
	}
}

//	Returns false if we are over the maxpackets limit
// and should choke back the bandwidth a bit by not sending
// a packet this frame.  All the commands will still get
// delivered in the next packet, but saving a header and
// getting more delta compression will reduce total bandwidth.
static bool CLT3_ReadyToSendPacket()
{
	// don't send anything if playing back a demo
	if (clc.demoplaying || cls.state == CA_CINEMATIC)
	{
		return false;
	}

	// If we are downloading, we send no less than 50ms between packets
	if ((GGameType & GAME_ET ? *cls.et_downloadTempName : *clc.downloadTempName) &&
		cls.realtime - clc.q3_lastPacketSentTime < 50)
	{
		return false;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if (cls.state != CA_ACTIVE &&
		cls.state != CA_PRIMED &&
		!(GGameType & GAME_ET ? *cls.et_downloadTempName : *clc.downloadTempName) &&
		cls.realtime - clc.q3_lastPacketSentTime < 1000)
	{
		return false;
	}

	// send every frame for loopbacks
	if (clc.netchan.remoteAddress.type == NA_LOOPBACK)
	{
		return true;
	}

	// send every frame for LAN
	if (SOCK_IsLANAddress(clc.netchan.remoteAddress))
	{
		return true;
	}

	// check for exceeding cl_maxpackets
	if (clt3_maxpackets->integer < 15)
	{
		Cvar_Set("cl_maxpackets", "15");
	}
	else if (clt3_maxpackets->integer > (GGameType & GAME_Quake3 ? 125 : 100))
	{
		Cvar_Set("cl_maxpackets", GGameType & GAME_Quake3 ? "125" : "100");
	}
	int oldPacketNum = (clc.netchan.outgoingSequence - 1) & PACKET_MASK_Q3;
	int delta = cls.realtime -  cl.q3_outPackets[oldPacketNum].p_realtime;
	if (delta < 1000 / clt3_maxpackets->integer)
	{
		// the accumulated commands will go out in the next packet
		return false;
	}

	return true;
}

//	Create and send the command packet to the server
// Including both the reliable commands and the usercmds
//
//	During normal gameplay, a client packet will contain something like:
//
//	4	sequence number
//	2	qport
//	4	serverid
//	4	acknowledged sequence number
//	4	clc.serverCommandSequence
//	<optional reliable commands>
//	1	q3clc_move or q3clc_moveNoDelta
//	1	command count
//	<count * usercmds>
void CLT3_WritePacket()
{
	// don't send anything if playing back a demo
	if (clc.demoplaying || cls.state == CA_CINEMATIC)
	{
		return;
	}

	QMsg buf;
	byte data[MAX_MSGLEN];
	buf.Init(data, GGameType & GAME_Quake3 ? MAX_MSGLEN_Q3 : MAX_MSGLEN_WOLF);

	buf.Bitstream();
	// write the current serverId so the server
	// can tell if this is from the current gameState
	buf.WriteLong(cl.q3_serverId);

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	buf.WriteLong(clc.q3_serverMessageSequence);

	// write the last reliable message we received
	buf.WriteLong(clc.q3_serverCommandSequence);

	// write any unacknowledged clientCommands
	// NOTE TTimo: if you verbose this, you will see that there are quite a few duplicates
	// typically several unacknowledged cp or userinfo commands stacked up
	int maxReliableCommands = GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF;
	for (int i = clc.q3_reliableAcknowledge + 1; i <= clc.q3_reliableSequence; i++)
	{
		buf.WriteByte(q3clc_clientCommand);
		buf.WriteLong(i);
		buf.WriteString(clc.q3_reliableCommands[i & (maxReliableCommands - 1)]);
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if (clt3_packetdup->integer < 0)
	{
		Cvar_Set("cl_packetdup", "0");
	}
	else if (clt3_packetdup->integer > 5)
	{
		Cvar_Set("cl_packetdup", "5");
	}
	int oldPacketNum = (clc.netchan.outgoingSequence - 1 - clt3_packetdup->integer) & PACKET_MASK_Q3;
	int count = cl.q3_cmdNumber - cl.q3_outPackets[oldPacketNum].p_cmdNumber;
	if (count > Q3MAX_PACKET_USERCMDS)
	{
		count = Q3MAX_PACKET_USERCMDS;
		common->Printf("Q3MAX_PACKET_USERCMDS\n");
	}
	int oldcmd_serverTime = 0;
	if (count >= 1)
	{
		if (clt3_showSend->integer)
		{
			common->Printf("(%i)", count);
		}

		// begin a client move command
		bool snap_valid = GGameType & GAME_WolfSP ? cl.ws_snap.valid :
			GGameType & GAME_WolfMP ? cl.wm_snap.valid :
			GGameType & GAME_ET ? cl.et_snap.valid : cl.q3_snap.valid;
		int snap_messageNum = GGameType & GAME_WolfSP ? cl.ws_snap.messageNum :
			GGameType & GAME_WolfMP ? cl.wm_snap.messageNum :
			GGameType & GAME_ET ? cl.et_snap.messageNum : cl.q3_snap.messageNum;
		if (cl_nodelta->integer || !snap_valid || clc.q3_demowaiting ||
			clc.q3_serverMessageSequence != snap_messageNum)
		{
			buf.WriteByte(q3clc_moveNoDelta);
		}
		else
		{
			buf.WriteByte(q3clc_move);
		}

		// write the command count
		buf.WriteByte(count);

		// use the checksum feed in the key
		int key = clc.q3_checksumFeed;
		// also use the message acknowledge
		key ^= clc.q3_serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(clc.q3_serverCommands[clc.q3_serverCommandSequence & (maxReliableCommands - 1)], 32);

		if (GGameType & GAME_WolfSP)
		{
			wsusercmd_t nullcmd = {};
			wsusercmd_t* oldcmd = &nullcmd;

			// write all the commands, including the predicted command
			for (int i = 0; i < count; i++)
			{
				int j = (cl.q3_cmdNumber - count + i + 1) & CMD_MASK_Q3;
				wsusercmd_t* cmd = &cl.ws_cmds[j];
				MSGWS_WriteDeltaUsercmdKey(&buf, key, oldcmd, cmd);
				oldcmd = cmd;
			}
			oldcmd_serverTime = oldcmd->serverTime;
		}
		else if (GGameType & GAME_WolfMP)
		{
			wmusercmd_t nullcmd = {};
			wmusercmd_t* oldcmd = &nullcmd;

			// write all the commands, including the predicted command
			for (int i = 0; i < count; i++)
			{
				int j = (cl.q3_cmdNumber - count + i + 1) & CMD_MASK_Q3;
				wmusercmd_t* cmd = &cl.wm_cmds[j];
				MSGWM_WriteDeltaUsercmdKey(&buf, key, oldcmd, cmd);
				oldcmd = cmd;
			}
			oldcmd_serverTime = oldcmd->serverTime;
		}
		else if (GGameType & GAME_ET)
		{
			etusercmd_t nullcmd = {};
			etusercmd_t* oldcmd = &nullcmd;

			// write all the commands, including the predicted command
			for (int i = 0; i < count; i++)
			{
				int j = (cl.q3_cmdNumber - count + i + 1) & CMD_MASK_Q3;
				etusercmd_t* cmd = &cl.et_cmds[j];
				MSGET_WriteDeltaUsercmdKey(&buf, key, oldcmd, cmd);
				oldcmd = cmd;
			}
			oldcmd_serverTime = oldcmd->serverTime;
		}
		else
		{
			q3usercmd_t nullcmd = {};
			q3usercmd_t* oldcmd = &nullcmd;

			// write all the commands, including the predicted command
			for (int i = 0; i < count; i++)
			{
				int j = (cl.q3_cmdNumber - count + i + 1) & CMD_MASK_Q3;
				q3usercmd_t* cmd = &cl.q3_cmds[j];
				MSGQ3_WriteDeltaUsercmdKey(&buf, key, oldcmd, cmd);
				oldcmd = cmd;
			}
			oldcmd_serverTime = oldcmd->serverTime;
		}
	}

	//
	// deliver the message
	//
	int packetNum = clc.netchan.outgoingSequence & PACKET_MASK_Q3;
	cl.q3_outPackets[packetNum].p_realtime = cls.realtime;
	cl.q3_outPackets[packetNum].p_serverTime = oldcmd_serverTime;
	cl.q3_outPackets[packetNum].p_cmdNumber = cl.q3_cmdNumber;
	clc.q3_lastPacketSentTime = cls.realtime;

	if (clt3_showSend->integer)
	{
		common->Printf("%i ", buf.cursize);
	}

	CLT3_Netchan_Transmit(&clc.netchan, &buf);

	// clients never really should have messages large enough
	// to fragment, but in case they do, fire them all off
	// at once
	// TTimo: this causes a packet burst, which is bad karma for winsock
	// added a WARNING message, we'll see if there are legit situations where this happens
	while (clc.netchan.unsentFragments)
	{
		common->DPrintf("WARNING: #462 unsent fragments (not supposed to happen!)\n");
		CLT3_Netchan_TransmitNextFragment(&clc.netchan);
	}
}

//	Called every frame to builds and sends a command packet to the server.
void CLT3_SendCmd()
{
	// don't send any message if not connected
	if (cls.state < CA_CONNECTED)
	{
		return;
	}

	// don't send commands if paused
	if (com_sv_running->integer && sv_paused->integer && cl_paused->integer)
	{
		return;
	}

	// we create commands even if a demo is playing,
	CLT3_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if (!CLT3_ReadyToSendPacket())
	{
		if (clt3_showSend->integer)
		{
			common->Printf(". ");
		}
		return;
	}

	CLT3_WritePacket();
}

void CLT3_SendPureChecksums()
{
	// if we are pure we need to send back a command with our referenced pk3 checksums
	const char* pChecksums = FS_ReferencedPakPureChecksums();

	char cMsg[MAX_INFO_VALUE_Q3];
	String::Sprintf(cMsg, sizeof(cMsg), "cp ");
	String::Cat(cMsg, sizeof(cMsg), va("%d ", cl.q3_serverId));
	String::Cat(cMsg, sizeof(cMsg), pChecksums);
	CL_AddReliableCommand(cMsg);
}

//	Clear download information that we keep in cls (disconnected download support)
void CLET_ClearStaticDownload()
{
	qassert(!cls.et_bWWWDlDisconnected);		// reset before calling
	cls.et_downloadRestart = false;
	cls.et_downloadTempName[0] = '\0';
	cls.et_downloadName[0] = '\0';
	cls.et_originalDownloadName[0] = '\0';
}

//	Requests a file to download from the server.  Stores it in the current
// game directory.
static void CLT3_BeginDownload(const char* localName, const char* remoteName)
{
	common->DPrintf("***** CLT3_BeginDownload *****\n"
				"Localname: %s\n"
				"Remotename: %s\n"
				"****************************\n", localName, remoteName);

	if (GGameType & GAME_ET)
	{
		String::NCpyZ(cls.et_downloadName, localName, sizeof(cls.et_downloadName));
		String::Sprintf(cls.et_downloadTempName, sizeof(cls.et_downloadTempName), "%s.tmp", localName);
	}
	else
	{
		String::NCpyZ(clc.downloadName, localName, sizeof(clc.downloadName));
		String::Sprintf(clc.downloadTempName, sizeof(clc.downloadTempName), "%s.tmp", localName);
	}

	// Set so UI gets access to it
	Cvar_Set("cl_downloadName", remoteName);
	Cvar_Set("cl_downloadSize", "0");
	Cvar_Set("cl_downloadCount", "0");
	Cvar_SetValue("cl_downloadTime", cls.realtime);

	clc.downloadBlock = 0;	// Starting new file
	clc.downloadCount = 0;

	CL_AddReliableCommand(va("download %s", remoteName));
}

//	Called when all downloading has been completed
static void CLT3_DownloadsComplete()
{
	// if we downloaded files we need to restart the file system
	if (GGameType & GAME_ET ? cls.et_downloadRestart : clc.downloadRestart)
	{
		clc.downloadRestart = false;
		cls.et_downloadRestart = false;

		FS_Restart(clc.q3_checksumFeed);	// We possibly downloaded a pak, restart the file system to load it

		if (!(GGameType & GAME_ET) || !cls.et_bWWWDlDisconnected)
		{
			// inform the server so we get new gamestate info
			CL_AddReliableCommand("donedl");
		}

		if (GGameType & GAME_ET)
		{
			// we can reset that now
			cls.et_bWWWDlDisconnected = false;
			CLET_ClearStaticDownload();
		}

		// by sending the donedl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}

	if (GGameType & GAME_ET)
	{
		// TTimo: I wonder if that happens - it should not but I suspect it could happen if a download fails in the middle or is aborted
		qassert(!cls.et_bWWWDlDisconnected);
	}

	// let the client game init and load data
	cls.state = CA_LOADING;

	// Pump the loop, this may change gamestate!
	Com_EventLoop();

	// if the gamestate was changed by calling Com_EventLoop
	// then we loaded everything already and we don't want to do it again.
	if (cls.state != CA_LOADING)
	{
		return;
	}

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// flush client memory and start loading stuff
	// this will also (re)load the UI
	// if this is a local client then only the client part of the hunk
	// will be cleared, note that this is done after the hunk mark has been set
	CLT3_FlushMemory();

	// initialize the CGame
	cls.q3_cgameStarted = true;
	CLT3_InitCGame();

	// set pure checksums
	CLT3_SendPureChecksums();

	CLT3_WritePacket();
	CLT3_WritePacket();
	CLT3_WritePacket();
}

//	A download completed or failed
void CLT3_NextDownload()
{
	// We are looking to start a download here
	if (*clc.downloadList)
	{
		char* s = clc.downloadList;

		// format is:
		//  @remotename@localname@remotename@localname, etc.

		if (*s == '@')
		{
			s++;
		}
		char* remoteName = s;

		if ((s = strchr(s, '@')) == NULL)
		{
			CLT3_DownloadsComplete();
			return;
		}

		*s++ = 0;
		char* localName = s;
		if ((s = strchr(s, '@')) != NULL)
		{
			*s++ = 0;
		}
		else
		{
			s = localName + String::Length(localName);	// point at the nul byte

		}
		CLT3_BeginDownload(localName, remoteName);

		if (GGameType & GAME_ET)
		{
			cls.et_downloadRestart = true;
		}
		else
		{
			clc.downloadRestart = true;
		}

		// move over the rest
		memmove(clc.downloadList, s, String::Length(s) + 1);

		return;
	}

	CLT3_DownloadsComplete();
}

//	After receiving a valid game state, we valid the cgame and local zip files here
// and determine if we need to download them
void CLT3_InitDownloads()
{
	if (GGameType & GAME_ET)
	{
		// TTimo
		// init some of the www dl data
		clc.et_bWWWDl = false;
		clc.et_bWWWDlAborting = false;
		cls.et_bWWWDlDisconnected = false;
		CLET_ClearStaticDownload();
	}

	if (GGameType & GAME_Quake3 && !clt3_allowDownload->integer)
	{
		// autodownload is disabled on the client
		// but it's possible that some referenced files on the server are missing
		char missingfiles[1024];
		if (FS_ComparePaks(missingfiles, sizeof(missingfiles), false))
		{
			// NOTE TTimo I would rather have that printed as a modal message box
			//   but at this point while joining the game we don't know wether we will successfully join or not
			common->Printf("\nWARNING: You are missing some files referenced by the server:\n%s"
					   "You might not be able to join the game\n"
					   "Go to the setting menu to turn on autodownload, or get the file elsewhere\n\n", missingfiles);
		}
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		// whatever autodownlad configuration, store missing files in a cvar, use later in the ui maybe
		char missingfiles[1024];
		if (FS_ComparePaks(missingfiles, sizeof(missingfiles), false))
		{
			Cvar_Set("com_missingFiles", missingfiles);
		}
		else
		{
			Cvar_Set("com_missingFiles", "");
		}
	}

	if (GGameType & GAME_ET)
	{
		// reset the redirect checksum tracking
		clc.et_redirectedList[0] = '\0';
	}

	if (clt3_allowDownload->integer && FS_ComparePaks(clc.downloadList, sizeof(clc.downloadList), true))
	{
		if (GGameType & (GAME_WolfMP | GAME_ET))
		{
			// this gets printed to UI, i18n
			common->Printf(CL_TranslateStringBuf("Need paks: %s\n"), clc.downloadList);
		}
		else
		{
			common->Printf("Need paks: %s\n", clc.downloadList);
		}

		if (*clc.downloadList)
		{
			// if autodownloading is not enabled on the server
			cls.state = CA_CONNECTED;
			CLT3_NextDownload();
			return;
		}
	}

	CLT3_DownloadsComplete();
}

void CLT3_Disconnect(bool showMainMenu)
{
	if (!com_cl_running || !com_cl_running->integer)
	{
		return;
	}

	// shutting down the client so enter full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	if (clc.demorecording)
	{
		CLT3_StopRecord_f();
	}

	if (!(GGameType & GAME_ET) || !cls.et_bWWWDlDisconnected)
	{
		if (clc.download)
		{
			FS_FCloseFile(clc.download);
			clc.download = 0;
		}
		if (GGameType & GAME_ET)
		{
			*cls.et_downloadTempName = *cls.et_downloadName = 0;
		}
		else
		{
			*clc.downloadTempName = *clc.downloadName = 0;
		}
		Cvar_Set("cl_downloadName", "");
	}

	if (clc.demofile)
	{
		FS_FCloseFile(clc.demofile);
		clc.demofile = 0;
	}

	if (showMainMenu)
	{
		UI_ForceMenuOff();
	}

	SCR_StopCinematic();
	S_ClearSoundBuffer(true);

	// send a disconnect message to the server
	// send it a few times in case one is dropped
	if (cls.state >= CA_CONNECTED)
	{
		CL_AddReliableCommand("disconnect");
		CLT3_WritePacket();
		CLT3_WritePacket();
		CLT3_WritePacket();
	}

	CL_ClearState();

	// wipe the client connection
	Com_Memset(&clc, 0, sizeof(clc));

	if (GGameType & GAME_ET && !cls.et_bWWWDlDisconnected)
	{
		CLET_ClearStaticDownload();
	}

	// allow cheats locally
	Cvar_Set("sv_cheats", "1");

	// not connected to a pure server anymore
	cl_connectedToPureServer = false;

	// show_bug.cgi?id=589
	// don't try a restart if uivm is NULL, as we might be in the middle of a restart already
	if (GGameType & GAME_ET && uivm && cls.state > CA_DISCONNECTED)
	{
		// restart the UI
		cls.state = CA_DISCONNECTED;

		// shutdown the UI
		CLT3_ShutdownUI();

		// init the UI
		CLT3_InitUI();
	}
	else
	{
		cls.state = CA_DISCONNECTED;
	}
}
