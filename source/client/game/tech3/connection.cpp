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
#include "../../../server/public.h"
#include "../et/dl_public.h"

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

//	Authorization server protocol
//	-----------------------------
//
//	All commands are text in Q3 out of band packets (leading 0xff 0xff 0xff 0xff).
//
//	Whenever the client tries to get a challenge from the server it wants to
// connect to, it also blindly fires off a packet to the authorize server:
//
//	getKeyAuthorize <challenge> <cdkey>
//
//	cdkey may be "demo"
//
//#OLD The authorize server returns a:
//#OLD
//#OLD keyAthorize <challenge> <accept | deny>
//#OLD
//#OLD A client will be accepted if the cdkey is valid and it has not been used by any other IP
//#OLD address in the last 15 minutes.
//
//	The server sends a:
//
//	getIpAuthorize <challenge> <ip>
//
//	The authorize server returns a:
//
//	ipAuthorize <challenge> <accept | deny | demo | unknown >
//
//	A client will be accepted if a valid cdkey was sent by that ip (only) in the last 15 minutes.
// If no response is received from the authorize server after two tries, the client will be let
// in anyway.
static void CLT3_RequestAuthorization()
{
	if (!cls.q3_authorizeServer.port)
	{
		const char* authorizeServerName = GGameType & GAME_WolfSP ? WSAUTHORIZE_SERVER_NAME :
			GGameType & GAME_WolfMP ? WMAUTHORIZE_SERVER_NAME : Q3AUTHORIZE_SERVER_NAME;
		common->Printf("Resolving %s\n", authorizeServerName);
		if (!SOCK_StringToAdr(authorizeServerName, &cls.q3_authorizeServer, Q3PORT_AUTHORIZE))
		{
			common->Printf("Couldn't resolve address\n");
			return;
		}

		common->Printf("%s resolved to %s\n", authorizeServerName, SOCK_AdrToString(cls.q3_authorizeServer));
	}
	if (cls.q3_authorizeServer.type == NA_BAD)
	{
		return;
	}

	char nums[64];
	CLT3_CDKeyForAuthorize(nums);

	Cvar* fs = Cvar_Get("cl_anonymous", "0", CVAR_INIT | CVAR_SYSTEMINFO);

	NET_OutOfBandPrint(NS_CLIENT, cls.q3_authorizeServer, "getKeyAuthorize %i %s", fs->integer, nums);
}

//	Resend a connect message if the last one has timed out
void CLT3_CheckForResend()
{
	// don't send anything if playing back a demo
	if (clc.demoplaying)
	{
		return;
	}

	// resend if we haven't gotten a reply yet
	if (cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING)
	{
		return;
	}

	if (cls.realtime - clc.q3_connectTime < RETRANSMIT_TIMEOUT)
	{
		return;
	}

	clc.q3_connectTime = cls.realtime;	// for retransmit requests
	clc.q3_connectPacketCount++;

	int port, i;
	char info[MAX_INFO_STRING_Q3];
	char data[MAX_INFO_STRING_Q3];
	switch (cls.state)
	{
	case CA_CONNECTING:
		// requesting a challenge
		if (!(GGameType & GAME_ET) && !SOCK_IsLANAddress(clc.q3_serverAddress))
		{
			CLT3_RequestAuthorization();
		}
		NET_OutOfBandPrint(NS_CLIENT, clc.q3_serverAddress, "getchallenge");
		break;

	case CA_CHALLENGING:
		// sending back the challenge
		port = Cvar_VariableValue("net_qport");

		String::NCpyZ(info, Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING_Q3), sizeof(info));
		if (GGameType & GAME_WolfSP)
		{
			Info_SetValueForKey(info, "protocol", va("%i", WSPROTOCOL_VERSION), MAX_INFO_STRING_Q3);
		}
		else if (GGameType & GAME_WolfMP)
		{
			Info_SetValueForKey(info, "protocol", va("%i", WMPROTOCOL_VERSION), MAX_INFO_STRING_Q3);
		}
		else if (GGameType & GAME_ET)
		{
			Info_SetValueForKey(info, "protocol", va("%i", ETPROTOCOL_VERSION), MAX_INFO_STRING_Q3);
		}
		else
		{
			Info_SetValueForKey(info, "protocol", va("%i", Q3PROTOCOL_VERSION), MAX_INFO_STRING_Q3);
		}
		Info_SetValueForKey(info, "qport", va("%i", port), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "challenge", va("%i", clc.q3_challenge), MAX_INFO_STRING_Q3);

		if (GGameType & GAME_WolfSP)
		{
			NET_OutOfBandPrint(NS_CLIENT, clc.q3_serverAddress, "connect \"%s\"", info);
		}
		else
		{
			String::Cpy(data, "connect ");
			// TTimo adding " " around the userinfo string to avoid truncated userinfo on the server
			//   (Com_TokenizeString tokenizes around spaces)
			data[8] = '\"';

			for (i = 0; i < String::Length(info); i++)
			{
				data[9 + i] = info[i];		// + (clc.q3_challenge)&0x3;
			}
			data[9 + i] = '\"';
			data[10 + i] = 0;

			// NOTE TTimo don't forget to set the right data length!
			NET_OutOfBandData(NS_CLIENT, clc.q3_serverAddress, (byte*)&data[0], i + 10);
		}
		// the most current userinfo has been sent, so watch for any
		// newer changes to userinfo variables
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;

	default:
		common->FatalError("CLT3_CheckForResend: bad cls.state");
	}
}

//	A local server is starting to load a map, so update the
// screen to let the user know about it, then dump all client
// memory on the hunk from cgame, ui, and renderer
void CLT3_MapLoading()
{
	if (!com_cl_running->integer)
	{
		return;
	}

	Con_Close();
	in_keyCatchers = 0;

	// if we are already connected to the local host, stay connected
	if (cls.state >= CA_CONNECTED && !String::ICmp(cls.servername, "localhost"))
	{
		cls.state = CA_CONNECTED;		// so the connect screen is drawn
		Com_Memset(cls.q3_updateInfoString, 0, sizeof(cls.q3_updateInfoString));
		Com_Memset(clc.q3_serverMessage, 0, sizeof(clc.q3_serverMessage));
		Com_Memset(&cl.q3_gameState, 0, sizeof(cl.q3_gameState));
		Com_Memset(&cl.ws_gameState, 0, sizeof(cl.ws_gameState));
		Com_Memset(&cl.wm_gameState, 0, sizeof(cl.wm_gameState));
		Com_Memset(&cl.et_gameState, 0, sizeof(cl.et_gameState));
		clc.q3_lastPacketSentTime = -9999;
		SCR_UpdateScreen();
	}
	else
	{
		// clear nextmap so the cinematic shutdown doesn't execute it
		Cvar_Set("nextmap", "");
		CL_Disconnect(true);
		String::NCpyZ(cls.servername, "localhost", sizeof(cls.servername));
		cls.state = CA_CHALLENGING;		// so the connect screen is drawn
		in_keyCatchers = 0;
		SCR_UpdateScreen();
		clc.q3_connectTime = -RETRANSMIT_TIMEOUT;
		SOCK_StringToAdr(cls.servername, &clc.q3_serverAddress, Q3PORT_SERVER);
		// we don't need a challenge on the localhost

		CLT3_CheckForResend();
	}

	if (GGameType & GAME_WolfSP)
	{
		// make sure sound is quiet
		S_FadeAllSounds(0, 0, false);
	}
}

static void CLT3_RequestMotd()
{
	if (!clt3_motd->integer)
	{
		return;
	}
	const char* motdServerName = GGameType & GAME_WolfSP ? WSMOTD_SERVER_NAME :
		GGameType & GAME_WolfMP ? WMMOTD_SERVER_NAME :
		GGameType & GAME_ET ? ETMOTD_SERVER_NAME : Q3MOTD_SERVER_NAME;
	common->Printf("Resolving %s\n", motdServerName);
	if (!SOCK_StringToAdr(motdServerName, &cls.q3_updateServer, Q3PORT_MOTD))
	{
		common->Printf("Couldn't resolve address\n");
		return;
	}
	common->Printf("%s resolved to %s\n", motdServerName, SOCK_AdrToString(cls.q3_updateServer));

	if (GGameType & GAME_Quake3)
	{
		// NOTE TTimo xoring against Com_Milliseconds, otherwise we may not have a true randomization
		// only srand I could catch before here is tr_noise.c l:26 srand(1001)
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=382
		// NOTE: the Com_Milliseconds xoring only affects the lower 16-bit word,
		//   but I decided it was enough randomization
		String::Sprintf(cls.q3_updateChallenge, sizeof(cls.q3_updateChallenge), "%i", ((rand() << 16) ^ rand()) ^ Com_Milliseconds());
	}
	else
	{
		String::Sprintf(cls.q3_updateChallenge, sizeof(cls.q3_updateChallenge), "%i", rand());
	}

	char info[MAX_INFO_STRING_Q3];
	info[0] = 0;
	Info_SetValueForKey(info, "challenge", cls.q3_updateChallenge, MAX_INFO_STRING_Q3);
	Info_SetValueForKey(info, "renderer", cls.glconfig.renderer_string, MAX_INFO_STRING_Q3);
	Info_SetValueForKey(info, "version", comt3_version->string, MAX_INFO_STRING_Q3);

	NET_OutOfBandPrint(NS_CLIENT, cls.q3_updateServer, "getmotd \"%s\"\n", info);
}

void CLT3_Connect_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("usage: connect [server]\n");
		return;
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		S_StopAllSounds();		// NERVE - SMF
	}

	if (GGameType & GAME_Quake3)
	{
		Cvar_Set("ui_singlePlayerActive", "0");
	}
	else
	{
		// starting to load a map so we get out of full screen ui mode
		Cvar_Set("r_uiFullScreen", "0");
	}
	if (GGameType & GAME_ET)
	{
		Cvar_Set("ui_connecting", "1");
	}

	// fire a message off to the motd server
	CLT3_RequestMotd();

	// clear any previous "server full" type messages
	clc.q3_serverMessage[0] = 0;

	const char* server = Cmd_Argv(1);

	if (com_sv_running->integer && !String::Cmp(server, "localhost"))
	{
		// if running a local server, kill it
		SV_Shutdown("Server quit\n");
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");
	SV_Frame(0);

	CL_Disconnect(true);
	Con_Close();

	String::NCpyZ(cls.servername, server, sizeof(cls.servername));

	if (!SOCK_StringToAdr(cls.servername, &clc.q3_serverAddress, Q3PORT_SERVER))
	{
		common->Printf("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		if (GGameType & GAME_ET)
		{
			Cvar_Set("ui_connecting", "0");
		}
		return;
	}
	common->Printf("%s resolved to %s\n", cls.servername, SOCK_AdrToString(clc.q3_serverAddress));

	// if we aren't playing on a lan, we need to authenticate
	// with the cd key
	if (SOCK_IsLocalAddress(clc.q3_serverAddress))
	{
		cls.state = CA_CHALLENGING;
	}
	else
	{
		cls.state = CA_CONNECTING;
	}

	in_keyCatchers = 0;
	clc.q3_connectTime = -99999;	// CLT3_CheckForResend() will fire immediately
	clc.q3_connectPacketCount = 0;

	// server connection string
	Cvar_Set("cl_currentServerAddress", server);

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		// show_bug.cgi?id=507
		// prepare to catch a connection process that would turn bad
		Cvar_Set("com_errorDiagnoseIP", SOCK_AdrToString(clc.q3_serverAddress));
		// ATVI Wolfenstein Misc #439
		// we need to setup a correct default for this, otherwise the first val we set might reappear
		Cvar_Set("com_errorMessage", "");

		// Gordon: um, couldnt this be handled
		// NERVE - SMF - reset some cvars
		Cvar_Set("mp_playerType", "0");
		Cvar_Set("mp_currentPlayerType", "0");
		Cvar_Set("mp_weapon", "0");
		Cvar_Set("mp_team", "0");
		Cvar_Set("mp_currentTeam", "0");

		Cvar_Set("ui_limboOptions", "0");
		Cvar_Set("ui_limboPrevOptions", "0");
		Cvar_Set("ui_limboObjective", "0");
		// -NERVE - SMF
	}

	if (GGameType & GAME_ET)
	{
		Cvar_Set("cl_currentServerIP", SOCK_AdrToString(clc.q3_serverAddress));

		Cvar_Set("clt3_avidemo", "0");
	}
}

void CLT3_Reconnect_f()
{
	if (!String::Length(cls.servername) || !String::Cmp(cls.servername, "localhost"))
	{
		common->Printf("Can't reconnect to localhost.\n");
		return;
	}
	if (GGameType & GAME_Quake3)
	{
		Cvar_Set("ui_singlePlayerActive", "0");
	}
	Cbuf_AddText(va("connect %s\n", cls.servername));
}

//	Sometimes the server can drop the client and the netchan based
// disconnect can be lost.  If the client continues to send packets
// to the server, the server will send out of band disconnect packets
// to the client so it doesn't have to wait for the full timeout period.
static void CLT3_DisconnectPacket(netadr_t from)
{
	if (cls.state < CA_AUTHORIZING)
	{
		return;
	}

	// if not from our server, ignore it
	if (!SOCK_CompareAdr(from, clc.netchan.remoteAddress))
	{
		return;
	}

	// if we have received packets within three seconds, ignore (it might be a malicious spoof)
	// NOTE TTimo:
	// there used to be a  clc.q3_lastPacketTime = cls.realtime; line in CLT3_PacketEvent before calling CL_ConnectionLessPacket
	// therefore .. packets never got through this check, clients never disconnected
	// switched the clc.q3_lastPacketTime = cls.realtime to happen after the connectionless packets have been processed
	// you still can't spoof disconnects, cause legal netchan packets will maintain realtime - lastPacketTime below the threshold
	if (cls.realtime - clc.q3_lastPacketTime < 3000)
	{
		return;
	}

	// if we are doing a disconnected download, leave the 'connecting' screen on with the progress information
	if (!(GGameType & GAME_ET) || !cls.et_bWWWDlDisconnected)
	{
		// drop the connection
		const char* message = GGameType & GAME_WolfMP ? CL_TranslateStringBuf("Server disconnected for unknown reason\n") :
			"Server disconnected for unknown reason\n";
		common->Printf("%s", message);
		Cvar_Set("com_errorMessage", message);
		CL_Disconnect(true);
	}
	else
	{
		CL_Disconnect(false);
		Cvar_Set("ui_connecting", "1");
		Cvar_Set("ui_dl_running", "1");
	}
}

static void CLT3_MotdPacket(netadr_t from)
{
	// if not from our server, ignore it
	if (!SOCK_CompareAdr(from, cls.q3_updateServer))
	{
		return;
	}

	const char* info = Cmd_Argv(1);

	// check challenge
	const char* challenge = Info_ValueForKey(info, "challenge");
	if (String::Cmp(challenge, cls.q3_updateChallenge))
	{
		return;
	}

	challenge = Info_ValueForKey(info, "motd");

	String::NCpyZ(cls.q3_updateInfoString, info, sizeof(cls.q3_updateInfoString));
	Cvar_Set("cl_motdString", challenge);
}

//	an OOB message from server, with potential markups
//	print OOB are the only messages we handle markups in
//	[err_dialog]: used to indicate that the connection should be aborted
//	  no further information, just do an error diagnostic screen afterwards
//	[err_prot]: HACK. This is a protocol error. The client uses a custom
//	  protocol error message (client sided) in the diagnostic window.
//	  The space for the error message on the connection screen is limited
//	  to 256 chars.
static void CLT3_PrintPacket(netadr_t from, QMsg* msg)
{
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		const char* s = msg->ReadBigString();
		if (!String::NICmp(s, "[err_dialog]", 12))
		{
			String::NCpyZ(clc.q3_serverMessage, s + 12, sizeof(clc.q3_serverMessage));
			if (GGameType & GAME_ET)
			{
				common->Error("%s", clc.q3_serverMessage);
			}
			else
			{
				Cvar_Set("com_errorMessage", clc.q3_serverMessage);
			}
		}
		else if (!String::NICmp(s, "[err_prot]", 10))
		{
			String::NCpyZ(clc.q3_serverMessage, s + 10, sizeof(clc.q3_serverMessage));
			const char* msg = "ERROR: Protocol Mismatch Between Client and Server.\n\n"
				"The server you attempted to join is running an incompatible version of the game.\n"
				"You or the server may be running older versions of the game. Press the auto-update"
				" button if it appears on the Main Menu screen.";
			if (GGameType & GAME_ET)
			{
				common->Error("%s", CL_TranslateStringBuf(msg));
			}
			else
			{
				Cvar_Set("com_errorMessage", CL_TranslateStringBuf(msg));
			}
		}
		else if (GGameType & GAME_ET && !String::NICmp(s, "[err_update]", 12))
		{
			String::NCpyZ(clc.q3_serverMessage, s + 12, sizeof(clc.q3_serverMessage));
			common->Error("%s", clc.q3_serverMessage);
		}
		else if (GGameType & GAME_ET && !String::NICmp(s, "ET://", 5))					// fretn
		{
			String::NCpyZ(clc.q3_serverMessage, s, sizeof(clc.q3_serverMessage));
			Cvar_Set("com_errorMessage", clc.q3_serverMessage);
			common->Error("%s", clc.q3_serverMessage);
		}
		else
		{
			String::NCpyZ(clc.q3_serverMessage, s, sizeof(clc.q3_serverMessage));
		}
		common->Printf("%s", clc.q3_serverMessage);
	}
	else
	{
		const char* s = msg->ReadString();
		String::NCpyZ(clc.q3_serverMessage, s, sizeof(clc.q3_serverMessage));
		common->Printf("%s", s);
	}
}

//	Responses to broadcasts, etc
static void CLT3_ConnectionlessPacket(netadr_t from, QMsg* msg)
{
	msg->BeginReadingOOB();
	msg->ReadLong();	// skip the -1

	const char* s = msg->ReadStringLine();

	Cmd_TokenizeString(s);

	const char* c = Cmd_Argv(0);

	common->DPrintf("CL packet %s: %s\n", SOCK_AdrToString(from), c);

	// challenge from the server we are connecting to
	if (!String::ICmp(c, "challengeResponse"))
	{
		if (cls.state != CA_CONNECTING)
		{
			common->Printf("Unwanted challenge response received.  Ignored.\n");
		}
		else
		{
			// start sending challenge repsonse instead of challenge request packets
			clc.q3_challenge = String::Atoi(Cmd_Argv(1));
			if (GGameType & (GAME_WolfMP | GAME_ET))
			{
				if (Cmd_Argc() > 2)
				{
					clc.wm_onlyVisibleClients = String::Atoi(Cmd_Argv(2));				// DHM - Nerve
				}
				else
				{
					clc.wm_onlyVisibleClients = 0;
				}
			}
			cls.state = CA_CHALLENGING;
			clc.q3_connectPacketCount = 0;
			clc.q3_connectTime = -99999;

			// take this address as the new server address.  This allows
			// a server proxy to hand off connections to multiple servers
			clc.q3_serverAddress = from;
			common->DPrintf("challengeResponse: %d\n", clc.q3_challenge);
		}
		return;
	}

	// server connection
	if (!String::ICmp(c, "connectResponse"))
	{
		if (cls.state >= CA_CONNECTED)
		{
			common->Printf("Dup connect received.  Ignored.\n");
			return;
		}
		if (cls.state != CA_CHALLENGING)
		{
			common->Printf("connectResponse packet while not connecting.  Ignored.\n");
			return;
		}
		if (!SOCK_CompareBaseAdr(from, clc.q3_serverAddress))
		{
			common->Printf("connectResponse from a different address.  Ignored.\n");
			common->Printf("%s should have been %s\n", SOCK_AdrToString(from),
				SOCK_AdrToString(clc.q3_serverAddress));
			return;
		}
		Netchan_Setup(NS_CLIENT, &clc.netchan, from, Cvar_VariableValue("net_qport"));
		cls.state = CA_CONNECTED;
		clc.q3_lastPacketSentTime = -9999;		// send first packet immediately
		return;
	}

	// server responding to an info broadcast
	if (!String::ICmp(c, "infoResponse"))
	{
		CLT3_ServerInfoPacket(from, msg);
		return;
	}

	// server responding to a get playerlist
	if (!String::ICmp(c, "statusResponse"))
	{
		CLT3_ServerStatusResponse(from, msg);
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if (!String::ICmp(c, "disconnect"))
	{
		CLT3_DisconnectPacket(from);
		return;
	}

	// echo request from server
	if (!String::ICmp(c, "echo"))
	{
		NET_OutOfBandPrint(NS_CLIENT, from, "%s", Cmd_Argv(1));
		return;
	}

	// cd check
	if (!String::ICmp(c, "keyAuthorize"))
	{
		// we don't use these now, so dump them on the floor
		return;
	}

	// global MOTD from id
	if (!String::ICmp(c, "motd"))
	{
		CLT3_MotdPacket(from);
		return;
	}

	// echo request from server
	if (!String::ICmp(c, "print"))
	{
		CLT3_PrintPacket(from, msg);
		return;
	}

	// echo request from server
	if (!String::NCmp(c, "getserversResponse", 18))
	{
		CLT3_ServersResponsePacket(from, msg);
		return;
	}

	common->DPrintf("Unknown connectionless packet command.\n");
}

//	A packet has arrived from the main event loop
void CLT3_PacketEvent(netadr_t from, QMsg* msg)
{
	if (GGameType & (GAME_Quake3 | GAME_WolfSP))
	{
		clc.q3_lastPacketTime = cls.realtime;
	}

	if (msg->cursize >= 4 && *(int*)msg->_data == -1)
	{
		CLT3_ConnectionlessPacket(from, msg);
		return;
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		clc.q3_lastPacketTime = cls.realtime;
	}

	if (cls.state < CA_CONNECTED)
	{
		return;		// can't be a valid sequenced packet
	}

	if (msg->cursize < 4)
	{
		common->Printf("%s: Runt packet\n",SOCK_AdrToString(from));
		return;
	}

	//
	// packet from server
	//
	if (!SOCK_CompareAdr(from, clc.netchan.remoteAddress))
	{
		common->DPrintf("%s:sequenced packet without connection\n",
			SOCK_AdrToString(from));
		// FIXME: send a client disconnect?
		return;
	}

	if (!CLT3_Netchan_Process(&clc.netchan, msg))
	{
		return;		// out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	int headerBytes = msg->readcount;

	// track the last message received so it can be returned in
	// client messages, allowing the server to detect a dropped
	// gamestate
	clc.q3_serverMessageSequence = LittleLong(*(int*)msg->_data);

	clc.q3_lastPacketTime = cls.realtime;
	CLT3_ParseServerMessage(msg);

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (clc.demorecording && !clc.q3_demowaiting)
	{
		CLT3_WriteDemoMessage(msg, headerBytes);
	}
}

void CLT3_CheckTimeout()
{
	//
	// check timeout
	//
	if ((!cl_paused->integer || !sv_paused->integer) &&
		cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC &&
		cls.realtime - clc.q3_lastPacketTime > cl_timeout->value * 1000)
	{
		if (++cl.timeoutcount > 5)			// timeoutcount saves debugger
		{
			if (GGameType & GAME_WolfMP)
			{
				Cvar_Set("com_errorMessage", CL_TranslateStringBuf("Server connection timed out."));
			}
			else if (GGameType & GAME_ET)
			{
				Cvar_Set("com_errorMessage", "Server connection timed out.");
			}
			else
			{
				common->Printf("\nServer connection timed out.\n");
			}
			CL_Disconnect(true);
			return;
		}
	}
	else
	{
		cl.timeoutcount = 0;
	}
}

void CLET_WWWDownload()
{
	char* to_ospath;
	dlStatus_t ret;
	static qboolean bAbort = false;

	if (clc.et_bWWWDlAborting)
	{
		if (!bAbort)
		{
			common->DPrintf("CLET_WWWDownload: WWWDlAborting\n");
			bAbort = true;
		}
		return;
	}
	if (bAbort)
	{
		common->DPrintf("CLET_WWWDownload: WWWDlAborting done\n");
		bAbort = false;
	}

	ret = DL_DownloadLoop();

	if (ret == DL_CONTINUE)
	{
		return;
	}

	if (ret == DL_DONE)
	{
		// taken from CLT3_ParseDownload
		// we work with OS paths
		clc.download = 0;
		to_ospath = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), cls.et_originalDownloadName, "");
		to_ospath[String::Length(to_ospath) - 1] = '\0';
		if (rename(cls.et_downloadTempName, to_ospath))
		{
			FS_CopyFile(cls.et_downloadTempName, to_ospath);
			remove(cls.et_downloadTempName);
		}
		*cls.et_downloadTempName = *cls.et_downloadName = 0;
		Cvar_Set("cl_downloadName", "");
		if (cls.et_bWWWDlDisconnected)
		{
			// reconnect to the server, which might send us to a new disconnected download
			Cbuf_ExecuteText(EXEC_APPEND, "reconnect\n");
		}
		else
		{
			CL_AddReliableCommand("wwwdl done");
			// tracking potential web redirects leading us to wrong checksum - only works in connected mode
			if (String::Length(clc.et_redirectedList) + String::Length(cls.et_originalDownloadName) + 1 >= (int)sizeof(clc.et_redirectedList))
			{
				// just to be safe
				common->Printf("ERROR: redirectedList overflow (%s)\n", clc.et_redirectedList);
			}
			else
			{
				strcat(clc.et_redirectedList, "@");
				strcat(clc.et_redirectedList, cls.et_originalDownloadName);
			}
		}
	}
	else
	{
		if (cls.et_bWWWDlDisconnected)
		{
			// in a connected download, we'd tell the server about failure and wait for a reply
			// but in this case we can't get anything from server
			// if we just reconnect it's likely we'll get the same disconnected download message, and error out again
			// this may happen for a regular dl or an auto update
			const char* error = va("Download failure while getting '%s'\n", cls.et_downloadName);	// get the msg before clearing structs
			cls.et_bWWWDlDisconnected = false;	// need clearing structs before ERR_DROP, or it goes into endless reload
			CLET_ClearStaticDownload();
			common->Error("%s", error);
		}
		else
		{
			// see CLT3_ParseDownload, same abort strategy
			common->Printf("Download failure while getting '%s'\n", cls.et_downloadName);
			CL_AddReliableCommand("wwwdl fail");
			clc.et_bWWWDlAborting = true;
		}
		return;
	}

	clc.et_bWWWDl = false;
	CLT3_NextDownload();
}
