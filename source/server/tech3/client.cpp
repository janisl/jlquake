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

#include "local.h"
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"
#include "../public.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/command_buffer.h"
#include "../../common/message_utils.h"

#define AUTHORIZE_TIMEOUT   5000

// sent by the server, printed on connection screen, works for all clients
// (restrictions: does not handle \n, no more than 256 chars)
#define WMPROTOCOL_MISMATCH_ERROR "ERROR: Protocol Mismatch Between Client and Server.\
The server you are attempting to join is running an incompatible version of the game."

//	clear/free any download vars
static void SVT3_CloseDownload(client_t* cl)
{
	// EOF
	if (cl->download)
	{
		FS_FCloseFile(cl->download);
	}
	cl->download = 0;
	*cl->downloadName = 0;

	// Free the temporary buffer space
	for (int i = 0; i < MAX_DOWNLOAD_WINDOW; i++)
	{
		if (cl->downloadBlocks[i])
		{
			Mem_Free(cl->downloadBlocks[i]);
			cl->downloadBlocks[i] = NULL;
		}
	}

}

//	Called when the player is totally leaving the server, either willingly
// or unwillingly.  This is NOT called if the entire server is quiting
// or crashing -- SVT3_FinalCommand() will handle that
void SVT3_DropClient(client_t* drop, const char* reason)
{
	int i;
	challenge_t* challenge;

	if (drop->state == CS_ZOMBIE)
	{
		return;		// already dropped
	}

	bool isBot = (drop->q3_entity && (drop->q3_entity->GetSvFlags() & Q3SVF_BOT)) ||
		drop->netchan.remoteAddress.type == NA_BOT;

	if (!isBot && !(GGameType & GAME_WolfSP))
	{
		// see if we already have a challenge for this ip
		challenge = &svs.challenges[0];

		for (i = 0; i < MAX_CHALLENGES; i++, challenge++)
		{
			if (SOCK_CompareAdr(drop->netchan.remoteAddress, challenge->adr))
			{
				challenge->connected = false;
				break;
			}
		}
	}

	// Kill any download
	SVT3_CloseDownload(drop);

	// tell everyone why they got dropped
	if (GGameType & GAME_WolfSP)
	{
		// Ridah, no need to tell the player if an AI drops
		if (!(drop->q3_entity && drop->q3_entity->GetSvFlagCastAI()))
		{
			SVT3_SendServerCommand(NULL, "print \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason);
		}
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVT3_SendServerCommand(NULL, "print \"[lof]%s" S_COLOR_WHITE " [lon]%s\n\"", drop->name, reason);
	}
	else if (GGameType & GAME_ET)
	{
		if ((!SVET_GameIsSinglePlayer()) || (!isBot))
		{
			// Gordon: we want this displayed elsewhere now
			SVT3_SendServerCommand(NULL, "cpm \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason);
		}
	}
	else
	{
		SVT3_SendServerCommand(NULL, "print \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason);
	}

	common->DPrintf("Going to CS_ZOMBIE for %s\n", drop->name);
	drop->state = CS_ZOMBIE;		// become free in a few seconds

	if (drop->download)
	{
		FS_FCloseFile(drop->download);
		drop->download = 0;
	}

	// call the prog function for removing a client
	// this will remove the body, among other things
	if (GGameType & GAME_WolfSP)
	{
		SVWS_GameClientDisconnect(drop);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_GameClientDisconnect(drop);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_GameClientDisconnect(drop);
	}
	else
	{
		SVQ3_GameClientDisconnect(drop);
	}

	if (!(GGameType & GAME_WolfSP) || !(drop->q3_entity && drop->q3_entity->GetSvFlagCastAI()))
	{
		// add the disconnect command
		SVT3_SendServerCommand(drop, "disconnect \"%s\"", reason);
	}

	if (drop->netchan.remoteAddress.type == NA_BOT)
	{
		SVT3_BotFreeClient(drop - svs.clients);
	}

	// nuke user info
	SVT3_SetUserinfo(drop - svs.clients, "");

	if (GGameType & GAME_WolfSP)
	{
		// RF, nuke reliable commands
		SVWS_FreeReliableCommandsForClient(drop);
	}

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this ip, reuse it
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			break;
		}
	}
	if (i == sv_maxclients->integer)
	{
		SVT3_Heartbeat_f();
	}
}

void SVQ3_ClientEnterWorld(client_t* client, q3usercmd_t* cmd)
{
	common->DPrintf("Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name);
	client->state = CS_ACTIVE;

	// set up the entity for the client
	int clientNum = client - svs.clients;
	q3sharedEntity_t* ent = SVQ3_GentityNum(clientNum);
	ent->s.number = clientNum;
	client->q3_gentity = ent;
	client->q3_entity = SVT3_EntityNum(clientNum);

	client->q3_deltaMessage = -1;
	client->q3_nextSnapshotTime = svs.q3_time;	// generate a snapshot immediately
	client->q3_lastUsercmd = *cmd;

	// call the game begin function
	SVT3_GameClientBegin(clientNum);
}

void SVWS_ClientEnterWorld(client_t* client, wsusercmd_t* cmd)
{
	common->DPrintf("Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name);
	client->state = CS_ACTIVE;

	// set up the entity for the client
	int clientNum = client - svs.clients;
	wssharedEntity_t* ent = SVWS_GentityNum(clientNum);
	ent->s.number = clientNum;
	client->ws_gentity = ent;
	client->q3_entity = SVT3_EntityNum(clientNum);

	client->q3_deltaMessage = -1;
	client->q3_nextSnapshotTime = svs.q3_time;	// generate a snapshot immediately
	client->ws_lastUsercmd = *cmd;

	// call the game begin function
	SVT3_GameClientBegin(clientNum);
}

void SVWM_ClientEnterWorld(client_t* client, wmusercmd_t* cmd)
{
	common->DPrintf("Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name);
	client->state = CS_ACTIVE;

	// set up the entity for the client
	int clientNum = client - svs.clients;
	wmsharedEntity_t* ent = SVWM_GentityNum(clientNum);
	ent->s.number = clientNum;
	client->wm_gentity = ent;
	client->q3_entity = SVT3_EntityNum(clientNum);

	client->q3_deltaMessage = -1;
	client->q3_nextSnapshotTime = svs.q3_time;	// generate a snapshot immediately
	client->wm_lastUsercmd = *cmd;

	// call the game begin function
	SVT3_GameClientBegin(clientNum);
}

void SVET_ClientEnterWorld(client_t* client, etusercmd_t* cmd)
{
	common->DPrintf("Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name);
	client->state = CS_ACTIVE;

	// set up the entity for the client
	int clientNum = client - svs.clients;
	etsharedEntity_t* ent = SVET_GentityNum(clientNum);
	ent->s.number = clientNum;
	client->et_gentity = ent;
	client->q3_entity = SVT3_EntityNum(clientNum);

	client->q3_deltaMessage = -1;
	client->q3_nextSnapshotTime = svs.q3_time;	// generate a snapshot immediately
	client->et_lastUsercmd = *cmd;

	// call the game begin function
	SVT3_GameClientBegin(clientNum);
}

//	Abort a download if in progress
static void SVT3_StopDownload_f(client_t* cl)
{
	if (*cl->downloadName)
	{
		common->DPrintf("clientDownload: %d : file \"%s\" aborted\n", static_cast<int>(cl - svs.clients), cl->downloadName);
	}

	SVT3_CloseDownload(cl);
}

//	The argument will be the last acknowledged block from the client, it should be
// the same as cl->downloadClientBlock
static void SVT3_NextDownload_f(client_t* cl)
{
	int block = String::Atoi(Cmd_Argv(1));

	if (block == cl->downloadClientBlock)
	{
		common->DPrintf("clientDownload: %d : client acknowledge of block %d\n", static_cast<int>(cl - svs.clients), block);

		// Find out if we are done.  A zero-length block indicates EOF
		if (cl->downloadBlockSize[cl->downloadClientBlock % MAX_DOWNLOAD_WINDOW] == 0)
		{
			common->Printf("clientDownload: %d : file \"%s\" completed\n", static_cast<int>(cl - svs.clients), cl->downloadName);
			SVT3_CloseDownload(cl);
			return;
		}

		cl->downloadSendTime = svs.q3_time;
		cl->downloadClientBlock++;
		return;
	}
	// We aren't getting an acknowledge for the correct block, drop the client
	// FIXME: this is bad... the client will never parse the disconnect message
	//			because the cgame isn't loaded yet
	SVT3_DropClient(cl, "broken download");
}

static void SVT3_BeginDownload_f(client_t* cl)
{
	// Kill any existing download
	SVT3_CloseDownload(cl);

	//bani - stop us from printing dupe messages
	if (GGameType & GAME_ET && String::Cmp(cl->downloadName, Cmd_Argv(1)))
	{
		cl->et_downloadnotify = DLNOTIFY_ALL;
	}

	// cl->downloadName is non-zero now, SVT3_WriteDownloadToClient will see this and open
	// the file itself
	String::NCpyZ(cl->downloadName, Cmd_Argv(1), sizeof(cl->downloadName));
}

// abort an attempted download
static void SVT3_BadDownload(client_t* cl, QMsg* msg)
{
	msg->WriteByte(q3svc_download);
	msg->WriteShort(0);		// client is expecting block zero
	msg->WriteLong(-1);		// illegal file size

	*cl->downloadName = 0;
}

//	svet_wwwFallbackURL can be used to redirect clients to a web URL in case direct ftp/http didn't work (or is disabled on client's end)
// return true when a redirect URL message was filled up
// when the cvar is set to something, the download server will effectively never use a legacy download strategy
static bool SVET_CheckFallbackURL(client_t* cl, QMsg* msg)
{
	if (!svet_wwwFallbackURL->string || String::Length(svet_wwwFallbackURL->string) == 0)
	{
		return false;
	}

	common->Printf("clientDownload: sending client '%s' to fallback URL '%s'\n", cl->name, svet_wwwFallbackURL->string);

	msg->WriteByte(q3svc_download);
	msg->WriteShort(-1);	// block -1 means ftp/http download
	msg->WriteString(svet_wwwFallbackURL->string);
	msg->WriteLong(0);
	msg->WriteLong(1 << DL_FLAG_URL);

	return true;
}

/*
==================
SVT3_WriteDownloadToClient

Check to see if the client wants a file, open it if needed and start pumping the client
Fill up msg with data
==================
*/
void SVT3_WriteDownloadToClient(client_t* cl, QMsg* msg)
{
	int curindex;
	int rate;
	int blockspersnap;
	int idPack, missionPack;
	char errorMessage[1024];
	int download_flag;

	bool bTellRate = false;// verbosity

	if (!*cl->downloadName)
	{
		return;	// Nothing being downloaded
	}
	if (GGameType & GAME_ET && cl->et_bWWWing)
	{
		return;	// The client acked and is downloading with ftp/http
	}

	// CVE-2006-2082
	// validate the download against the list of pak files
	if (GGameType & (GAME_WolfMP | GAME_ET) && !FS_VerifyPak(cl->downloadName))
	{
		// will drop the client and leave it hanging on the other side. good for him
		SVT3_DropClient(cl, "illegal download request");
		return;
	}

	if (!cl->download)
	{
		// We open the file here

		//bani - prevent duplicate download notifications
		if (!(GGameType & GAME_ET) || cl->et_downloadnotify & DLNOTIFY_BEGIN)
		{
			cl->et_downloadnotify &= ~DLNOTIFY_BEGIN;
			common->Printf("clientDownload: %d : beginning \"%s\"\n", static_cast<int>(cl - svs.clients), cl->downloadName);
		}

		missionPack = GGameType & GAME_Quake3 && FS_idPak(cl->downloadName, "missionpack");
		idPack = missionPack || FS_idPak(cl->downloadName, fs_PrimaryBaseGame);

		if (!svt3_allowDownload->integer || idPack)
		{
			// cannot auto-download file
			if (idPack)
			{
				common->Printf("clientDownload: %d : \"%s\" cannot download id pk3 files\n", static_cast<int>(cl - svs.clients), cl->downloadName);
				if (missionPack)
				{
					String::Sprintf(errorMessage, sizeof(errorMessage), "Cannot autodownload Team Arena file \"%s\"\n"
																		"The Team Arena mission pack can be found in your local game store.", cl->downloadName);
				}
				else
				{
					String::Sprintf(errorMessage, sizeof(errorMessage), "Cannot autodownload official pk3 file \"%s\"", cl->downloadName);
				}
			}
			else
			{
				common->Printf("clientDownload: %d : \"%s\" download disabled", static_cast<int>(cl - svs.clients), cl->downloadName);
				if (svt3_pure->integer)
				{
					String::Sprintf(errorMessage, sizeof(errorMessage), "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
																		"You will need to get this file elsewhere before you "
																		"can connect to this pure server.\n", cl->downloadName);
				}
				else
				{
					String::Sprintf(errorMessage, sizeof(errorMessage), "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
																		"The server you are connecting to is not a pure server, "
																		"set autodownload to No in your settings and you might be "
																		"able to join the game anyway.\n", cl->downloadName);
				}
			}
			SVT3_BadDownload(cl, msg);
			msg->WriteString(errorMessage);
			return;
		}

		// www download redirect protocol
		// NOTE: this is called repeatedly while a client connects. Maybe we should sort of cache the message or something
		// FIXME: we need to abstract this to an independant module for maximum configuration/usability by server admins
		// FIXME: I could rework that, it's crappy
		if (GGameType & GAME_ET && svet_wwwDownload->integer)
		{
			if (cl->et_bDlOK)
			{
				if (!cl->et_bFallback)
				{
					fileHandle_t handle;
					int downloadSize = FS_SV_FOpenFileRead(cl->downloadName, &handle);
					if (downloadSize)
					{
						FS_FCloseFile(handle);	// don't keep open, we only care about the size

						String::NCpyZ(cl->et_downloadURL, va("%s/%s", svet_wwwBaseURL->string, cl->downloadName), sizeof(cl->et_downloadURL));

						//bani - prevent multiple download notifications
						if (cl->et_downloadnotify & DLNOTIFY_REDIRECT)
						{
							cl->et_downloadnotify &= ~DLNOTIFY_REDIRECT;
							common->Printf("Redirecting client '%s' to %s\n", cl->name, cl->et_downloadURL);
						}
						// once cl->downloadName is set (and possibly we have our listening socket), let the client know
						cl->et_bWWWDl = true;
						msg->WriteByte(q3svc_download);
						msg->WriteShort(-1);	// block -1 means ftp/http download
						// compatible with legacy q3svc_download protocol: [size] [size bytes]
						// download URL, size of the download file, download flags
						msg->WriteString(cl->et_downloadURL);
						msg->WriteLong(downloadSize);
						download_flag = 0;
						if (svet_wwwDlDisconnected->integer)
						{
							download_flag |= (1 << DL_FLAG_DISCON);
						}
						msg->WriteLong(download_flag);	// flags
						return;
					}
					else
					{
						// that should NOT happen - even regular download would fail then anyway
						common->Printf("ERROR: Client '%s': couldn't extract file size for %s\n", cl->name, cl->downloadName);
					}
				}
				else
				{
					cl->et_bFallback = false;
					if (SVET_CheckFallbackURL(cl, msg))
					{
						return;
					}
					common->Printf("Client '%s': falling back to regular downloading for failed file %s\n", cl->name, cl->downloadName);
				}
			}
			else
			{
				if (SVET_CheckFallbackURL(cl, msg))
				{
					return;
				}
				common->Printf("Client '%s' is not configured for www download\n", cl->name);
			}
		}

		// find file
		cl->et_bWWWDl = false;
		cl->downloadSize = FS_SV_FOpenFileRead(cl->downloadName, &cl->download);
		if (cl->downloadSize <= 0)
		{
			// NOTE TTimo this is NOT supposed to happen unless bug in our filesystem scheme?
			//   if the pk3 is referenced, it must have been found somewhere in the filesystem
			common->Printf("clientDownload: %d : \"%s\" file not found on server\n", static_cast<int>(cl - svs.clients), cl->downloadName);
			String::Sprintf(errorMessage, sizeof(errorMessage), "File \"%s\" not found on server for autodownloading.\n", cl->downloadName);
			SVT3_BadDownload(cl, msg);
			msg->WriteString(errorMessage);
			return;
		}

		// Init
		cl->downloadCurrentBlock = cl->downloadClientBlock = cl->downloadXmitBlock = 0;
		cl->downloadCount = 0;
		cl->downloadEOF = false;

		bTellRate = true;
	}

	// Perform any reads that we need to
	while (cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW &&
		   cl->downloadSize != cl->downloadCount)
	{

		curindex = (cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW);

		if (!cl->downloadBlocks[curindex])
		{
			cl->downloadBlocks[curindex] = (byte*)Mem_Alloc(MAX_DOWNLOAD_BLKSIZE);
		}

		cl->downloadBlockSize[curindex] = FS_Read(cl->downloadBlocks[curindex], MAX_DOWNLOAD_BLKSIZE, cl->download);

		if (cl->downloadBlockSize[curindex] < 0)
		{
			// EOF right now
			cl->downloadCount = cl->downloadSize;
			break;
		}

		cl->downloadCount += cl->downloadBlockSize[curindex];

		// Load in next block
		cl->downloadCurrentBlock++;
	}

	// Check to see if we have eof condition and add the EOF block
	if (cl->downloadCount == cl->downloadSize &&
		!cl->downloadEOF &&
		cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW)
	{

		cl->downloadBlockSize[cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW] = 0;
		cl->downloadCurrentBlock++;

		cl->downloadEOF = true;	// We have added the EOF block
	}

	// Loop up to window size times based on how many blocks we can fit in the
	// client snapMsec and rate

	// based on the rate, how many bytes can we fit in the snapMsec time of the client
	// normal rate / snapshotMsec calculation
	rate = cl->rate;

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		// show_bug.cgi?id=509
		// for autodownload, we use a seperate max rate value
		// we do this everytime because the client might change it's rate during the download
		if (svt3_dl_maxRate->integer < rate)
		{
			rate = svt3_dl_maxRate->integer;
			if (bTellRate)
			{
				common->Printf("'%s' downloading at sv_dl_maxrate (%d)\n", cl->name, svt3_dl_maxRate->integer);
			}
		}
		else
		if (bTellRate)
		{
			common->Printf("'%s' downloading at rate %d\n", cl->name, rate);
		}
	}
	else
	{
		if (svt3_maxRate->integer)
		{
			if (svt3_maxRate->integer < 1000)
			{
				Cvar_Set("sv_MaxRate", "1000");
			}
			if (svt3_maxRate->integer < rate)
			{
				rate = svt3_maxRate->integer;
			}
		}
	}

	if (!rate)
	{
		blockspersnap = 1;
	}
	else
	{
		blockspersnap = ((rate * cl->q3_snapshotMsec) / 1000 + MAX_DOWNLOAD_BLKSIZE) /
						MAX_DOWNLOAD_BLKSIZE;
	}

	if (blockspersnap < 0)
	{
		blockspersnap = 1;
	}

	while (blockspersnap--)
	{

		// Write out the next section of the file, if we have already reached our window,
		// automatically start retransmitting

		if (cl->downloadClientBlock == cl->downloadCurrentBlock)
		{
			return;	// Nothing to transmit

		}
		if (cl->downloadXmitBlock == cl->downloadCurrentBlock)
		{
			// We have transmitted the complete window, should we start resending?

			//FIXME:  This uses a hardcoded one second timeout for lost blocks
			//the timeout should be based on client rate somehow
			if (svs.q3_time - cl->downloadSendTime > 1000)
			{
				cl->downloadXmitBlock = cl->downloadClientBlock;
			}
			else
			{
				return;
			}
		}

		// Send current block
		curindex = (cl->downloadXmitBlock % MAX_DOWNLOAD_WINDOW);

		msg->WriteByte(q3svc_download);
		msg->WriteShort(cl->downloadXmitBlock);

		// block zero is special, contains file size
		if (cl->downloadXmitBlock == 0)
		{
			msg->WriteLong(cl->downloadSize);
		}

		msg->WriteShort(cl->downloadBlockSize[curindex]);

		// Write the block
		if (cl->downloadBlockSize[curindex])
		{
			msg->WriteData(cl->downloadBlocks[curindex], cl->downloadBlockSize[curindex]);
		}

		common->DPrintf("clientDownload: %d : writing block %d\n", static_cast<int>(cl - svs.clients), cl->downloadXmitBlock);

		// Move on to the next block
		// It will get sent with next snap shot.  The rate will keep us in line.
		cl->downloadXmitBlock++;

		cl->downloadSendTime = svs.q3_time;
	}
}

//	The client is going to disconnect, so remove the connection immediately  FIXME: move to game?
static void SVT3_Disconnect_f(client_t* cl)
{
	SVT3_DropClient(cl, "disconnected");
}

static void SVT3_ResetPureClient_f(client_t* cl)
{
	cl->q3_pureAuthentic = 0;
	cl->q3_gotCP = false;
}

//	Pull specific info from a newly changed userinfo string
// into a more C friendly form.
static void SVT3_UserinfoChanged(client_t* cl)
{
	const char* val;
	int i;

	// name for C code
	String::NCpyZ(cl->name, Info_ValueForKey(cl->userinfo, "name"), sizeof(cl->name));

	// rate command

	// if the client is on the same subnet as the server and we aren't running an
	// internet public server, assume they don't need a rate choke
	if (SOCK_IsLANAddress(cl->netchan.remoteAddress) && com_dedicated->integer != 2 && svt3_lanForceRate->integer == 1)
	{
		cl->rate = 99999;	// lans should not rate limit
	}
	else
	{
		val = Info_ValueForKey(cl->userinfo, "rate");
		if (String::Length(val))
		{
			i = String::Atoi(val);
			cl->rate = i;
			if (cl->rate < 1000)
			{
				cl->rate = 1000;
			}
			else if (cl->rate > 90000)
			{
				cl->rate = 90000;
			}
		}
		else
		{
			cl->rate = GGameType & (GAME_WolfMP | GAME_ET) ? 5000 : 3000;
		}
	}
	val = Info_ValueForKey(cl->userinfo, "handicap");
	if (String::Length(val))
	{
		i = String::Atoi(val);
		if (i <= (GGameType & GAME_ET ? -100 : 0) || i > 100 || String::Length(val) > 4)
		{
			Info_SetValueForKey(cl->userinfo, "handicap", GGameType & GAME_ET ? "0" : "100", MAX_INFO_STRING_Q3);
		}
	}

	// snaps command
	val = Info_ValueForKey(cl->userinfo, "snaps");
	if (String::Length(val))
	{
		i = String::Atoi(val);
		if (i < 1)
		{
			i = 1;
		}
		else if (i > 30)
		{
			i = 30;
		}
		cl->q3_snapshotMsec = 1000 / i;
	}
	else
	{
		cl->q3_snapshotMsec = 50;
	}

	// TTimo
	// maintain the IP information
	// this is set in SVT3_DirectConnect (directly on the server, not transmitted), may be lost when client updates it's userinfo
	// the banning code relies on this being consistently present
	// zinx - modified to always keep this consistent, instead of only
	// when "ip" is 0-length, so users can't supply their own IP
	val = Info_ValueForKey(cl->userinfo, "ip");
	if (!val[0] || GGameType & GAME_ET)
	{
		//common->DPrintf("Maintain IP in userinfo for '%s'\n", cl->name);
		if (!SOCK_IsLocalAddress(cl->netchan.remoteAddress))
		{
			Info_SetValueForKey(cl->userinfo, "ip", SOCK_AdrToString(cl->netchan.remoteAddress), MAX_INFO_STRING_Q3);
		}
		else
		{
			// force the "ip" info key to "localhost" for local clients
			Info_SetValueForKey(cl->userinfo, "ip", "localhost", MAX_INFO_STRING_Q3);
		}
	}

	if (GGameType & GAME_ET)
	{
		// TTimo
		// download prefs of the client
		val = Info_ValueForKey(cl->userinfo, "cl_wwwDownload");
		cl->et_bDlOK = false;
		if (String::Length(val))
		{
			i = String::Atoi(val);
			if (i != 0)
			{
				cl->et_bDlOK = true;
			}
		}
	}
}

static void SVT3_UpdateUserinfo_f(client_t* cl)
{
	String::NCpyZ(cl->userinfo, Cmd_Argv(1), MAX_INFO_STRING_Q3);

	SVT3_UserinfoChanged(cl);
	// call prog code to allow overrides
	if (GGameType & GAME_WolfSP)
	{
		SVWS_GameClientUserInfoChanged(cl - svs.clients);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_GameClientUserInfoChanged(cl - svs.clients);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_GameClientUserInfoChanged(cl - svs.clients);
	}
	else
	{
		SVQ3_GameClientUserInfoChanged(cl - svs.clients);
	}
}

//	A "getchallenge" OOB command has been received
// Returns a challenge number that can be used
// in a subsequent connectResponse command.
// We do this to prevent denial of service attacks that
// flood the server with invalid connection IPs.  With a
// challenge, they must give a valid IP address.
//	If we are authorizing, a challenge request will cause a packet
// to be sent to the authorize server.
//	When an authorizeip is returned, a challenge response will be
// sent to that ip.
void SVT3_GetChallenge(netadr_t from)
{
	// ignore if we are in single player
	if (GGameType & GAME_ET)
	{
		if (SVET_GameIsSinglePlayer())
		{
			return;
		}
	}
	else
	{
		if (Cvar_VariableValue("g_gametype") == Q3GT_SINGLE_PLAYER || (GGameType & GAME_Quake3 && Cvar_VariableValue("ui_singlePlayerActive")))
		{
			return;
		}
	}

	if (GGameType & GAME_ET && SVET_TempBanIsBanned(from))
	{
		NET_OutOfBandPrint(NS_SERVER, from, "print\n%s\n", svet_tempbanmessage->string);
		return;
	}

	int oldest = 0;
	int oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	challenge_t* challenge = &svs.challenges[0];
	int i;
	for (i = 0; i < MAX_CHALLENGES; i++, challenge++)
	{
		if (!challenge->connected && SOCK_CompareAdr(from, challenge->adr))
		{
			break;
		}
		if (challenge->time < oldestTime)
		{
			oldestTime = challenge->time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// this is the first time this client has asked for a challenge
		challenge = &svs.challenges[oldest];

		challenge->challenge = ((rand() << 16) ^ rand()) ^ svs.q3_time;
		challenge->adr = from;
		challenge->firstTime = svs.q3_time;
		challenge->firstPing = 0;
		challenge->time = svs.q3_time;
		challenge->connected = false;
		i = oldest;
	}

	// if they are on a lan address, send the challengeResponse immediately
	if (GGameType & GAME_ET || SOCK_IsLANAddress(from))
	{
		challenge->pingTime = svs.q3_time;
		if (GGameType & (GAME_WolfMP | GAME_ET) && svwm_onlyVisibleClients->integer)
		{
			NET_OutOfBandPrint(NS_SERVER, from, "challengeResponse %i %i", challenge->challenge, svwm_onlyVisibleClients->integer);
		}
		else
		{
			NET_OutOfBandPrint(NS_SERVER, from, "challengeResponse %i", challenge->challenge);
		}
		return;
	}

	// look up the authorize server's IP
	if (!svs.q3_authorizeAddress.ip[0] && svs.q3_authorizeAddress.type != NA_BAD)
	{
		const char* suthorizeServerName = GGameType & GAME_WolfSP ? WSAUTHORIZE_SERVER_NAME :
			GGameType & GAME_WolfMP ? WMAUTHORIZE_SERVER_NAME : Q3AUTHORIZE_SERVER_NAME;
		common->Printf("Resolving %s\n", suthorizeServerName);
		if (!SOCK_StringToAdr(suthorizeServerName, &svs.q3_authorizeAddress, Q3PORT_AUTHORIZE))
		{
			common->Printf("Couldn't resolve address\n");
			return;
		}
		common->Printf("%s resolved to %s\n", suthorizeServerName, SOCK_AdrToString(svs.q3_authorizeAddress));
	}

	// if they have been challenging for a long time and we
	// haven't heard anything from the authorize server, go ahead and
	// let them in, assuming the id server is down
	if (svs.q3_time - challenge->firstTime > AUTHORIZE_TIMEOUT)
	{
		common->DPrintf("authorize server timed out\n");

		challenge->pingTime = svs.q3_time;
		if (GGameType & GAME_WolfMP && svwm_onlyVisibleClients->integer)
		{
			NET_OutOfBandPrint(NS_SERVER, challenge->adr,
				"challengeResponse %i %i", challenge->challenge, svwm_onlyVisibleClients->integer);
		}
		else
		{
			NET_OutOfBandPrint(NS_SERVER, challenge->adr,
				"challengeResponse %i", challenge->challenge);
		}
		return;
	}

	// otherwise send their ip to the authorize server
	if (svs.q3_authorizeAddress.type != NA_BAD)
	{
		Cvar* fs;
		char game[1024];

		common->DPrintf("sending getIpAuthorize for %s\n", SOCK_AdrToString(from));

		if (GGameType & GAME_Quake3)
		{
			String::Cpy(game, fs_PrimaryBaseGame);
		}
		else
		{
			game[0] = 0;
		}
		fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
		if (fs && fs->string[0] != 0)
		{
			String::Cpy(game, fs->string);
		}

		if (GGameType & GAME_Quake3)
		{
			// the 0 is for backwards compatibility with obsolete sv_allowanonymous flags
			// getIpAuthorize <challenge> <IP> <game> 0 <auth-flag>
			NET_OutOfBandPrint(NS_SERVER, svs.q3_authorizeAddress,
				"getIpAuthorize %i %s %s 0 %s",  svs.challenges[i].challenge,
				SOCK_BaseAdrToString(from), game, svq3_strictAuth->string);
		}
		else
		{
			fs = Cvar_Get("sv_allowAnonymous", "0", CVAR_SERVERINFO);
			NET_OutOfBandPrint(NS_SERVER, svs.q3_authorizeAddress,
				"getIpAuthorize %i %s %s %i",  svs.challenges[i].challenge,
				SOCK_BaseAdrToString(from), game, fs->integer);
		}
	}
}

//	A packet has been returned from the authorize server.
// If we have a challenge adr for that ip, send the
// challengeResponse to it
void SVT3_AuthorizeIpPacket(netadr_t from)
{
	if (!SOCK_CompareBaseAdr(from, svs.q3_authorizeAddress))
	{
		common->Printf("SVT3_AuthorizeIpPacket: not from authorize server\n");
		return;
	}

	int challenge = String::Atoi(Cmd_Argv(1));

	int i;
	for (i = 0; i < MAX_CHALLENGES; i++)
	{
		if (svs.challenges[i].challenge == challenge)
		{
			break;
		}
	}
	if (i == MAX_CHALLENGES)
	{
		common->Printf("SVT3_AuthorizeIpPacket: challenge not found\n");
		return;
	}

	// send a packet back to the original client
	svs.challenges[i].pingTime = svs.q3_time;
	const char* s = Cmd_Argv(2);
	const char* r = Cmd_Argv(3);			// reason

	if (!String::ICmp(s, "demo"))
	{
		// they are a demo client trying to connect to a real server
		NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "print\nServer is not a demo server\n");
		// clear the challenge record so it won't timeout and let them through
		Com_Memset(&svs.challenges[i], 0, sizeof(svs.challenges[i]));
		return;
	}
	if (!String::ICmp(s, "accept"))
	{
		if (GGameType & GAME_WolfMP && svwm_onlyVisibleClients->integer)
		{
			NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr,
				"challengeResponse %i %i", svs.challenges[i].challenge, svwm_onlyVisibleClients->integer);
		}
		else
		{
			NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr,
				"challengeResponse %i", svs.challenges[i].challenge);
		}
		return;
	}
	if (!String::ICmp(s, "unknown"))
	{
		if (!r)
		{
			NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "print\nAwaiting CD key authorization\n");
		}
		else
		{
			NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "print\n%s\n", r);
		}
		// clear the challenge record so it won't timeout and let them through
		Com_Memset(&svs.challenges[i], 0, sizeof(svs.challenges[i]));
		return;
	}

	// authorization failed
	if (!r)
	{
		NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "print\nSomeone is using this CD Key\n");
	}
	else
	{
		NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "print\n%s\n", r);
	}

	// clear the challenge record so it won't timeout and let them through
	Com_Memset(&svs.challenges[i], 0, sizeof(svs.challenges[i]));
}

//	A "connect" OOB command has been received
void SVT3_DirectConnect(netadr_t from)
{
	common->DPrintf("SVC_DirectConnect ()\n");

	char userinfo[MAX_INFO_STRING_Q3];
	String::NCpyZ(userinfo, Cmd_Argv(1), sizeof(userinfo));

	int version = String::Atoi(Info_ValueForKey(userinfo, "protocol"));
	int expectedVersion = GGameType & GAME_WolfSP ? WSPROTOCOL_VERSION :
		GGameType & GAME_WolfMP ? WMPROTOCOL_VERSION :
		GGameType & GAME_ET ? ETPROTOCOL_VERSION : Q3PROTOCOL_VERSION;
	if (version != expectedVersion)
	{
		if (GGameType & GAME_WolfMP && version <= 59)
		{
			// old clients, don't send them the [err_drop] tag
			NET_OutOfBandPrint(NS_SERVER, from, "print\n" WMPROTOCOL_MISMATCH_ERROR);
		}
		else if (GGameType & (GAME_WolfMP | GAME_ET))
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_prot]" WMPROTOCOL_MISMATCH_ERROR);
		}
		else
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\nServer uses protocol version %i.\n", expectedVersion);
		}
		common->DPrintf("    rejected connect from version %i\n", version);
		return;
	}

	int challenge = String::Atoi(Info_ValueForKey(userinfo, "challenge"));
	int qport = String::Atoi(Info_ValueForKey(userinfo, "qport"));

	if (GGameType & GAME_ET && SVET_TempBanIsBanned(from))
	{
		NET_OutOfBandPrint(NS_SERVER, from, "print\n%s\n", svet_tempbanmessage->string);
		return;
	}

	// quick reject
	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->integer; i++,cl++)
	{
		// DHM - Nerve :: This check was allowing clients to reconnect after zombietime(2 secs)
		if (!(GGameType & (GAME_WolfMP | GAME_ET)) && cl->state == CS_FREE)
		{
			continue;
		}
		if (SOCK_CompareBaseAdr(from, cl->netchan.remoteAddress) &&
			(cl->netchan.qport == qport ||
			 from.port == cl->netchan.remoteAddress.port))
		{
			if ((svs.q3_time - cl->q3_lastConnectTime)
				< (svt3_reconnectlimit->integer * 1000))
			{
				common->DPrintf("%s:reconnect rejected : too soon\n", SOCK_AdrToString(from));
				return;
			}
			break;
		}
	}

	// see if the challenge is valid (local clients don't need to challenge)
	int chalengeIndex = 0;
	if (!SOCK_IsLocalAddress(from))
	{
		for (chalengeIndex = 0; chalengeIndex < MAX_CHALLENGES; chalengeIndex++)
		{
			if (SOCK_CompareAdr(from, svs.challenges[chalengeIndex].adr))
			{
				if (challenge == svs.challenges[chalengeIndex].challenge)
				{
					break;		// good
				}
				if (GGameType & GAME_WolfSP)
				{
					NET_OutOfBandPrint(NS_SERVER, from, "print\nBad challenge.\n");
					return;
				}
			}
		}
		if (chalengeIndex == MAX_CHALLENGES)
		{
			if (GGameType & GAME_ET)
			{
				NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]No or bad challenge for address.\n");
			}
			else
			{
				NET_OutOfBandPrint(NS_SERVER, from, "print\nNo or bad challenge for address.\n");
			}
			return;
		}
		// force the IP key/value pair so the game can filter based on ip
		Info_SetValueForKey(userinfo, "ip", SOCK_AdrToString(from), MAX_INFO_STRING_Q3);

		int ping;
		if (!(GGameType & (GAME_WolfMP | GAME_ET)) || svs.challenges[chalengeIndex].firstPing == 0)
		{
			ping = svs.q3_time - svs.challenges[chalengeIndex].pingTime;
			svs.challenges[chalengeIndex].firstPing = ping;
		}
		else
		{
			ping = svs.challenges[chalengeIndex].firstPing;
		}

		common->Printf("Client %i connecting with %i challenge ping\n", chalengeIndex, ping);
		svs.challenges[chalengeIndex].connected = true;

		// never reject a LAN client based on ping
		if (!SOCK_IsLANAddress(from))
		{
			if (svt3_minPing->value && ping < svt3_minPing->value)
			{
				// don't let them keep trying until they get a big delay
				if (GGameType & GAME_ET)
				{
					NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]Server is for high pings only\n");
				}
				else
				{
					NET_OutOfBandPrint(NS_SERVER, from, "print\nServer is for high pings only\n");
				}
				common->DPrintf("Client %i rejected on a too low ping\n", chalengeIndex);
				// reset the address otherwise their ping will keep increasing
				// with each connect message and they'd eventually be able to connect
				svs.challenges[chalengeIndex].adr.port = 0;
				return;
			}
			if (svt3_maxPing->value && ping > svt3_maxPing->value)
			{
				if (GGameType & GAME_ET)
				{
					NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]Server is for low pings only\n");
				}
				else
				{
					NET_OutOfBandPrint(NS_SERVER, from, "print\nServer is for low pings only\n");
				}
				common->DPrintf("Client %i rejected on a too high ping: %i\n", chalengeIndex, ping);
				return;
			}
		}
	}
	else
	{
		// force the "ip" info key to "localhost"
		Info_SetValueForKey(userinfo, "ip", "localhost", MAX_INFO_STRING_Q3);
	}

	static client_t temp;
	client_t* newcl = &temp;
	Com_Memset(newcl, 0, sizeof(client_t));

	const char* password;
	// if there is already a slot for this ip, reuse it
	cl = svs.clients;
	for (int i = 0; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (SOCK_CompareBaseAdr(from, cl->netchan.remoteAddress) &&
			(cl->netchan.qport == qport ||
			 from.port == cl->netchan.remoteAddress.port))
		{
			common->Printf("%s:reconnect\n", SOCK_AdrToString(from));
			newcl = cl;
			goto gotnewcl;
		}
	}

	// find a client slot
	// if "sv_privateClients" is set > 0, then that number
	// of client slots will be reserved for connections that
	// have "password" set to the value of "sv_privatePassword"
	// Info requests will report the maxclients as if the private
	// slots didn't exist, to prevent people from trying to connect
	// to a full server.
	// This is to allow us to reserve a couple slots here on our
	// servers so we can play without having to kick people.

	// check for privateClient password
	password = Info_ValueForKey(userinfo, "password");
	int startIndex;
	if (!String::Cmp(password, svt3_privatePassword->string))
	{
		startIndex = 0;
	}
	else
	{
		// skip past the reserved slots
		startIndex = svt3_privateClients->integer;
	}

	newcl = NULL;
	for (int i = startIndex; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == CS_FREE)
		{
			newcl = cl;
			break;
		}
	}

	if (!newcl)
	{
		if (SOCK_IsLocalAddress(from))
		{
			int count = 0;
			for (int i = startIndex; i < sv_maxclients->integer; i++)
			{
				cl = &svs.clients[i];
				if (cl->netchan.remoteAddress.type == NA_BOT)
				{
					count++;
				}
			}
			// if they're all bots
			if (count >= sv_maxclients->integer - startIndex)
			{
				SVT3_DropClient(&svs.clients[sv_maxclients->integer - 1], "only bots on server");
				newcl = &svs.clients[sv_maxclients->integer - 1];
			}
			else
			{
				common->FatalError("server is full on local connect\n");
				return;
			}
		}
		else
		{
			if (GGameType & GAME_ET)
			{
				NET_OutOfBandPrint(NS_SERVER, from, "print\n%s\n", svet_fullmsg->string);
			}
			else
			{
				NET_OutOfBandPrint(NS_SERVER, from, "print\nServer is full.\n");
			}
			common->DPrintf("Rejected a connection.\n");
			return;
		}
	}

	// we got a newcl, so reset the reliableSequence and reliableAcknowledge
	cl->q3_reliableAcknowledge = 0;
	cl->q3_reliableSequence = 0;

gotnewcl:
	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;
	int clientNum = newcl - svs.clients;
	if (GGameType & GAME_WolfSP)
	{
		newcl->ws_gentity = SVWS_GentityNum(clientNum);
	}
	else if (GGameType & GAME_WolfMP)
	{
		newcl->wm_gentity = SVWM_GentityNum(clientNum);
	}
	else if (GGameType & GAME_ET)
	{
		newcl->et_gentity = SVET_GentityNum(clientNum);
	}
	else
	{
		newcl->q3_gentity = SVQ3_GentityNum(clientNum);
	}
	newcl->q3_entity = SVT3_EntityNum(clientNum);

	// save the challenge
	newcl->challenge = challenge;

	// save the address
	Netchan_Setup(NS_SERVER, &newcl->netchan, from, qport);
	if (GGameType & (GAME_Quake3 | GAME_WolfMP))
	{
		// init the netchan queue
		newcl->q3_netchan_end_queue = &newcl->q3_netchan_start_queue;
	}

	// save the userinfo
	String::NCpyZ(newcl->userinfo, userinfo, MAX_INFO_STRING_Q3);

	// get the game a chance to reject this connection or modify the userinfo
	const char* denied = SVT3_GameClientConnect(clientNum, true, false);	// firstTime = true
	if (denied)
	{
		if (GGameType & GAME_ET)
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]%s\n", denied);
		}
		else
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\n%s\n", denied);
		}
		common->DPrintf("Game rejected a connection: %s.\n", denied);
		return;
	}

	if (GGameType & GAME_WolfSP)
	{
		// RF, create the reliable commands
		if (newcl->netchan.remoteAddress.type != NA_BOT)
		{
			SVWS_InitReliableCommandsForClient(newcl, MAX_RELIABLE_COMMANDS_WOLF);
		}
		else
		{
			SVWS_InitReliableCommandsForClient(newcl, 0);
		}
	}

	SVT3_UserinfoChanged(newcl);

	// DHM - Nerve :: Clear out firstPing now that client is connected
	svs.challenges[chalengeIndex].firstPing = 0;

	// send the connect packet to the client
	NET_OutOfBandPrint(NS_SERVER, from, "connectResponse");

	common->DPrintf("Going from CS_FREE to CS_CONNECTED for %s\n", newcl->name);

	newcl->state = CS_CONNECTED;
	newcl->q3_nextSnapshotTime = svs.q3_time;
	newcl->q3_lastPacketTime = svs.q3_time;
	newcl->q3_lastConnectTime = svs.q3_time;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	newcl->q3_gamestateMessageNum = -1;

	// if this was the first client on the server, or the last client
	// the server can hold, send a heartbeat to the master.
	int count = 0;
	cl = svs.clients;
	for (int i = 0; i < sv_maxclients->integer; i++,cl++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			count++;
		}
	}
	if (count == 1 || count == sv_maxclients->integer)
	{
		SVT3_Heartbeat_f();
	}
}

//	Sends the first message from the server to a connected client.
// This will be sent on the initial connection and upon each new map load.
//	It will be resent if the client acknowledges a later message but has
// the wrong gamestate.
static void SVT3_SendClientGameState(client_t* client)
{
	int start;
	QMsg msg;
	byte msgBuffer[MAX_MSGLEN];

	common->DPrintf("SVT3_SendClientGameState() for %s\n", client->name);
	common->DPrintf("Going from CS_CONNECTED to CS_PRIMED for %s\n", client->name);
	client->state = CS_PRIMED;
	client->q3_pureAuthentic = 0;
	client->q3_gotCP = false;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	client->q3_gamestateMessageNum = client->netchan.outgoingSequence;

	msg.Init(msgBuffer, GGameType & GAME_Quake3 ? MAX_MSGLEN_Q3 : MAX_MSGLEN_WOLF);

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	msg.WriteLong(client->q3_lastClientCommand);

	// send any server commands waiting to be sent first.
	// we have to do this cause we send the client->reliableSequence
	// with a gamestate and it sets the clc.serverCommandSequence at
	// the client side
	SVT3_UpdateServerCommandsToClient(client, &msg);

	// send the gamestate
	msg.WriteByte(q3svc_gamestate);
	msg.WriteLong(client->q3_reliableSequence);

	// write the configstrings
	for (start = 0; start < (GGameType & GAME_WolfSP ? MAX_CONFIGSTRINGS_WS :
		GGameType & GAME_WolfMP ? MAX_CONFIGSTRINGS_WM :
		GGameType & GAME_ET ? MAX_CONFIGSTRINGS_ET : MAX_CONFIGSTRINGS_Q3); start++)
	{
		if (sv.q3_configstrings[start][0])
		{
			msg.WriteByte(q3svc_configstring);
			msg.WriteShort(start);
			msg.WriteBigString(sv.q3_configstrings[start]);
		}
	}

	// write the baselines
	if (GGameType & GAME_WolfSP)
	{
		wsentityState_t nullstate;
		memset(&nullstate, 0, sizeof(nullstate));
		for (start = 0; start < MAX_GENTITIES_Q3; start++)
		{
			wsentityState_t* base = &sv.q3_svEntities[start].ws_baseline;
			if (!base->number)
			{
				continue;
			}
			msg.WriteByte(q3svc_baseline);
			MSGWS_WriteDeltaEntity(&msg, &nullstate, base, true);
		}
	}
	else if (GGameType & GAME_WolfMP)
	{
		wmentityState_t nullstate;
		memset(&nullstate, 0, sizeof(nullstate));
		for (start = 0; start < MAX_GENTITIES_Q3; start++)
		{
			wmentityState_t* base = &sv.q3_svEntities[start].wm_baseline;
			if (!base->number)
			{
				continue;
			}
			msg.WriteByte(q3svc_baseline);
			MSGWM_WriteDeltaEntity(&msg, &nullstate, base, true);
		}
	}
	else if (GGameType & GAME_ET)
	{
		etentityState_t nullstate;
		memset(&nullstate, 0, sizeof(nullstate));
		for (start = 0; start < MAX_GENTITIES_Q3; start++)
		{
			etentityState_t* base = &sv.q3_svEntities[start].et_baseline;
			if (!base->number)
			{
				continue;
			}
			msg.WriteByte(q3svc_baseline);
			MSGET_WriteDeltaEntity(&msg, &nullstate, base, true);
		}
	}
	else
	{
		q3entityState_t nullstate;
		Com_Memset(&nullstate, 0, sizeof(nullstate));
		for (start = 0; start < MAX_GENTITIES_Q3; start++)
		{
			q3entityState_t* base = &sv.q3_svEntities[start].q3_baseline;
			if (!base->number)
			{
				continue;
			}
			msg.WriteByte(q3svc_baseline);
			MSGQ3_WriteDeltaEntity(&msg, &nullstate, base, true);
		}
	}

	msg.WriteByte(q3svc_EOF);

	msg.WriteLong(client - svs.clients);

	// write the checksum feed
	msg.WriteLong(sv.q3_checksumFeed);

	// deliver this to the client
	SVT3_SendMessageToClient(&msg, client);
}

//	Downloads are finished
static void SVT3_DoneDownload_f(client_t* cl)
{
	common->DPrintf("clientDownload: %s Done\n", cl->name);
	// resend the game state to update any clients that entered during the download
	SVT3_SendClientGameState(cl);
}

static void SVET_WWWDownload_f(client_t* cl)
{
	char* subcmd = Cmd_Argv(1);

	// only accept wwwdl commands for clients which we first flagged as wwwdl ourselves
	if (!cl->et_bWWWDl)
	{
		common->Printf("SV_WWWDownload: unexpected wwwdl '%s' for client '%s'\n", subcmd, cl->name);
		SVT3_DropClient(cl, va("SV_WWWDownload: unexpected wwwdl %s", subcmd));
		return;
	}

	if (!String::ICmp(subcmd, "ack"))
	{
		if (cl->et_bWWWing)
		{
			common->Printf("WARNING: dupe wwwdl ack from client '%s'\n", cl->name);
		}
		cl->et_bWWWing = true;
		return;
	}
	else if (!String::ICmp(subcmd, "bbl8r"))
	{
		SVT3_DropClient(cl, "acking disconnected download mode");
		return;
	}

	// below for messages that only happen during/after download
	if (!cl->et_bWWWing)
	{
		common->Printf("SV_WWWDownload: unexpected wwwdl '%s' for client '%s'\n", subcmd, cl->name);
		SVT3_DropClient(cl, va("SV_WWWDownload: unexpected wwwdl %s", subcmd));
		return;
	}

	if (!String::ICmp(subcmd, "done"))
	{
		cl->download = 0;
		*cl->downloadName = 0;
		cl->et_bWWWing = false;
		return;
	}
	else if (!String::ICmp(subcmd, "fail"))
	{
		cl->download = 0;
		*cl->downloadName = 0;
		cl->et_bWWWing = false;
		cl->et_bFallback = true;
		// send a reconnect
		SVT3_SendClientGameState(cl);
		return;
	}
	else if (!String::ICmp(subcmd, "chkfail"))
	{
		common->Printf("WARNING: client '%s' reports that the redirect download for '%s' had wrong checksum.\n", cl->name, cl->downloadName);
		common->Printf("         you should check your download redirect configuration.\n");
		cl->download = 0;
		*cl->downloadName = 0;
		cl->et_bWWWing = false;
		cl->et_bFallback = true;
		// send a reconnect
		SVT3_SendClientGameState(cl);
		return;
	}

	common->Printf("SV_WWWDownload: unknown wwwdl subcommand '%s' for client '%s'\n", subcmd, cl->name);
	SVT3_DropClient(cl, va("SV_WWWDownload: unknown wwwdl subcommand '%s'", subcmd));
}

//	If we are pure, disconnect the client if they do no meet the following conditions:
//	1. the first two checksums match our view of cgame and ui
//		Wolf specific: the checksum is the checksum of the pk3 we found the DLL in
//	2. there are no any additional checksums that we do not have
//	This routine would be a bit simpler with a goto but i abstained
static void SVT3_VerifyPaks_f(client_t* cl)
{
	// if we are pure, we "expect" the client to load certain things from
	// certain pk3 files, namely we want the client to have loaded the
	// ui and cgame that we think should be loaded based on the pure setting
	if (svt3_pure->integer != 0)
	{
		int nChkSum1 = 0;
		int nChkSum2 = 0;
		// we run the game, so determine which cgame and ui the client "should" be running
		bool bGood = (FS_FileIsInPAK(GGameType & (GAME_WolfMP | GAME_ET) ? "cgame_mp_x86.dll" : "vm/cgame.qvm", &nChkSum1) == 1);
		if (bGood)
		{
			bGood = (FS_FileIsInPAK(GGameType & (GAME_WolfMP | GAME_ET) ? "ui_mp_x86.dll" : "vm/ui.qvm", &nChkSum2) == 1);
		}

		int nClientPaks = Cmd_Argc();

		// start at arg 2 ( skip serverId cl_paks )
		int nCurArg = 1;

		const char* pArg = Cmd_Argv(nCurArg++);
		if (!pArg)
		{
			bGood = false;
		}
		else
		{
			// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=475
			// we may get incoming cp sequences from a previous checksumFeed, which we need to ignore
			// since serverId is a frame count, it always goes up
			if (String::Atoi(pArg) < sv.q3_checksumFeedServerId)
			{
				common->DPrintf("ignoring outdated cp command from client %s\n", cl->name);
				return;
			}
		}

		// we basically use this while loop to avoid using 'goto' :)
		while (bGood)
		{

			// must be at least 6: "cl_paks cgame ui @ firstref ... numChecksums"
			// numChecksums is encoded
			if (nClientPaks < 6)
			{
				bGood = false;
				break;
			}
			// verify first to be the cgame checksum
			pArg = Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || String::Atoi(pArg) != nChkSum1)
			{
				bGood = false;
				break;
			}
			// verify the second to be the ui checksum
			pArg = Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || String::Atoi(pArg) != nChkSum2)
			{
				bGood = false;
				break;
			}
			// should be sitting at the delimeter now
			pArg = Cmd_Argv(nCurArg++);
			if (*pArg != '@')
			{
				bGood = false;
				break;
			}
			// store checksums since tokenization is not re-entrant
			int i;
			int nClientChkSum[1024];
			for (i = 0; nCurArg < nClientPaks; i++)
			{
				nClientChkSum[i] = String::Atoi(Cmd_Argv(nCurArg++));
			}

			// store number to compare against (minus one cause the last is the number of checksums)
			nClientPaks = i - 1;

			// make sure none of the client check sums are the same
			// so the client can't send 5 the same checksums
			for (i = 0; i < nClientPaks; i++)
			{
				for (int j = 0; j < nClientPaks; j++)
				{
					if (i == j)
					{
						continue;
					}
					if (nClientChkSum[i] == nClientChkSum[j])
					{
						bGood = false;
						break;
					}
				}
				if (bGood == false)
				{
					break;
				}
			}
			if (bGood == false)
			{
				break;
			}

			// get the pure checksums of the pk3 files loaded by the server
			const char* pPaks = FS_LoadedPakPureChecksums();
			Cmd_TokenizeString(pPaks);
			int nServerPaks = Cmd_Argc();
			if (nServerPaks > 1024)
			{
				nServerPaks = 1024;
			}

			int nServerChkSum[1024];
			for (i = 0; i < nServerPaks; i++)
			{
				nServerChkSum[i] = String::Atoi(Cmd_Argv(i));
			}

			// check if the client has provided any pure checksums of pk3 files not loaded by the server
			for (i = 0; i < nClientPaks; i++)
			{
				int j;
				for (j = 0; j < nServerPaks; j++)
				{
					if (nClientChkSum[i] == nServerChkSum[j])
					{
						break;
					}
				}
				if (j >= nServerPaks)
				{
					bGood = false;
					break;
				}
			}
			if (bGood == false)
			{
				break;
			}

			// check if the number of checksums was correct
			nChkSum1 = sv.q3_checksumFeed;
			for (i = 0; i < nClientPaks; i++)
			{
				nChkSum1 ^= nClientChkSum[i];
			}
			nChkSum1 ^= nClientPaks;
			if (nChkSum1 != nClientChkSum[nClientPaks])
			{
				bGood = false;
				break;
			}

			// break out
			break;
		}

		cl->q3_gotCP = true;

		if (bGood)
		{
			cl->q3_pureAuthentic = 1;
		}
		else
		{
			cl->q3_pureAuthentic = 0;
			cl->q3_nextSnapshotTime = -1;
			cl->state = CS_ACTIVE;
			SVT3_SendClientSnapshot(cl);
			SVT3_DropClient(cl, "Unpure client detected. Invalid .PK3 files referenced!");
		}
	}
}

ucmd_t q3_ucmds[] =
{
	{"userinfo", SVT3_UpdateUserinfo_f},
	{"disconnect", SVT3_Disconnect_f},
	{"cp", SVT3_VerifyPaks_f},
	{"vdr", SVT3_ResetPureClient_f},
	{"download", SVT3_BeginDownload_f},
	{"nextdl", SVT3_NextDownload_f},
	{"stopdl", SVT3_StopDownload_f},
	{"donedl", SVT3_DoneDownload_f},
	{NULL, NULL}
};

ucmd_t et_ucmds[] =
{
	{"userinfo", SVT3_UpdateUserinfo_f, false },
	{"disconnect", SVT3_Disconnect_f, true },
	{"cp", SVT3_VerifyPaks_f, false },
	{"vdr", SVT3_ResetPureClient_f, false },
	{"download", SVT3_BeginDownload_f, false },
	{"nextdl", SVT3_NextDownload_f, false },
	{"stopdl", SVT3_StopDownload_f, false },
	{"donedl", SVT3_DoneDownload_f, false },
	{"wwwdl", SVET_WWWDownload_f, false },
	{NULL, NULL}
};

static bool SVT3_ClientCommand(client_t* cl, QMsg* msg, bool premaprestart)
{
	int seq = msg->ReadLong();
	const char* s = msg->ReadString();

	// see if we have already executed it
	if (cl->q3_lastClientCommand >= seq)
	{
		return true;
	}

	common->DPrintf("clientCommand: %s : %i : %s\n", cl->name, seq, s);

	// drop the connection if we have somehow lost commands
	if (seq > cl->q3_lastClientCommand + 1)
	{
		common->Printf("Client %s lost %i clientCommands\n", cl->name,
			seq - cl->q3_lastClientCommand + 1);
		SVT3_DropClient(cl, "Lost reliable commands");
		return false;
	}

	// NERVE - SMF - some server game-only commands we cannot have flood protect
	bool floodprotect = true;
	if (GGameType & (GAME_WolfMP | GAME_ET) && (!String::NCmp("team", s, 4) || !String::NCmp("setspawnpt", s, 10) || !String::NCmp("score", s, 5)))
	{
		floodprotect = false;
	}
	if (GGameType & GAME_ET && !String::ICmp("forcetapout", s))
	{
		floodprotect = false;
	}

	// malicious users may try using too many string commands
	// to lag other players.  If we decide that we want to stall
	// the command, we will stop processing the rest of the packet,
	// including the usercmd.  This causes flooders to lag themselves
	// but not other people
	// We don't do this when the client hasn't been active yet since its
	// normal to spam a lot of commands when downloading
	bool clientOk = true;
	if (!com_cl_running->integer &&
		cl->state >= CS_ACTIVE &&
		svt3_floodProtect->integer &&
		svs.q3_time < cl->q3_nextReliableTime &&
		floodprotect)
	{
		// ignore any other text messages from this client but let them keep playing
		// TTimo - moved the ignored verbose to the actual processing in SV_ExecuteClientCommand, only printing if the core doesn't intercept
		clientOk = false;
	}

	if (GGameType & GAME_ET)
	{
		// don't allow another command for 800 msec
		if (floodprotect &&
			svs.q3_time >= cl->q3_nextReliableTime)
		{
			cl->q3_nextReliableTime = svs.q3_time + 800;
		}
	}
	else if (GGameType & GAME_WolfMP)
	{
		// don't allow another command for one second
		if (floodprotect)
		{
			cl->q3_nextReliableTime = svs.q3_time + 800;
		}
	}
	else
	{
		// don't allow another command for one second
		cl->q3_nextReliableTime = svs.q3_time + 1000;
	}

	SV_ExecuteClientCommand(cl, s, clientOk, premaprestart);

	cl->q3_lastClientCommand = seq;
	String::Sprintf(cl->q3_lastClientCommandString, sizeof(cl->q3_lastClientCommandString), "%s", s);

	return true;		// continue procesing
}

//	The message usually contains all the movement commands
// that were in the last three packets, so that the information
// in dropped packets can be recovered.
//	On very fast clients, there may be multiple usercmd packed into
// each of the backup packets.
static void SVT3_UserMove(client_t* cl, QMsg* msg, bool delta)
{
	if (delta)
	{
		cl->q3_deltaMessage = cl->q3_messageAcknowledge;
	}
	else
	{
		cl->q3_deltaMessage = -1;
	}

	int cmdCount = msg->ReadByte();

	if (cmdCount < 1)
	{
		common->Printf("cmdCount < 1\n");
		return;
	}

	if (cmdCount > Q3MAX_PACKET_USERCMDS)
	{
		common->Printf("cmdCount > Q3MAX_PACKET_USERCMDS\n");
		return;
	}

	// use the checksum feed in the key
	int key = sv.q3_checksumFeed;
	// also use the message acknowledge
	key ^= cl->q3_messageAcknowledge;
	if (GGameType & GAME_Quake3)
	{
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(cl->q3_reliableCommands[cl->q3_reliableAcknowledge & (MAX_RELIABLE_COMMANDS_Q3 - 1)], 32);

		q3usercmd_t nullcmd;
		Com_Memset(&nullcmd, 0, sizeof(nullcmd));
		q3usercmd_t* oldcmd = &nullcmd;
		q3usercmd_t cmds[Q3MAX_PACKET_USERCMDS];
		for (int i = 0; i < cmdCount; i++)
		{
			q3usercmd_t* cmd = &cmds[i];
			MSGQ3_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
			oldcmd = cmd;
		}

		// save time for ping calculation
		cl->q3_frames[cl->q3_messageAcknowledge & PACKET_MASK_Q3].messageAcked = svs.q3_time;

		// TTimo
		// catch the no-cp-yet situation before SVQ3_ClientEnterWorld
		// if CS_ACTIVE, then it's time to trigger a new gamestate emission
		// if not, then we are getting remaining parasite usermove commands, which we should ignore
		if (svt3_pure->integer != 0 && cl->q3_pureAuthentic == 0 && !cl->q3_gotCP)
		{
			if (cl->state == CS_ACTIVE)
			{
				// we didn't get a cp yet, don't assume anything and just send the gamestate all over again
				common->DPrintf("%s: didn't get cp command, resending gamestate\n", cl->name);
				SVT3_SendClientGameState(cl);
			}
			return;
		}

		// if this is the first usercmd we have received
		// this gamestate, put the client into the world
		if (cl->state == CS_PRIMED)
		{
			SVQ3_ClientEnterWorld(cl, &cmds[0]);
			// the moves can be processed normaly
		}

		// a bad cp command was sent, drop the client
		if (svt3_pure->integer != 0 && cl->q3_pureAuthentic == 0)
		{
			SVT3_DropClient(cl, "Cannot validate pure client!");
			return;
		}

		if (cl->state != CS_ACTIVE)
		{
			cl->q3_deltaMessage = -1;
			return;
		}

		// usually, the first couple commands will be duplicates
		// of ones we have previously received, but the servertimes
		// in the commands will cause them to be immediately discarded
		for (int i =  0; i < cmdCount; i++)
		{
			// if this is a cmd from before a map_restart ignore it
			if (cmds[i].serverTime > cmds[cmdCount - 1].serverTime)
			{
				continue;
			}
			// extremely lagged or cmd from before a map_restart
			//if ( cmds[i].serverTime > svs.q3_time + 3000 ) {
			//	continue;
			//}
			// don't execute if this is an old cmd which is already executed
			// these old cmds are included when cl_packetdup > 0
			if (cmds[i].serverTime <= cl->q3_lastUsercmd.serverTime)
			{
				continue;
			}
			SVQ3_ClientThink(cl, &cmds[i]);
		}
	}
	else if (GGameType & GAME_WolfSP)
	{
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(SVT3_GetReliableCommand(cl, cl->q3_reliableAcknowledge & (MAX_RELIABLE_COMMANDS_WOLF - 1)), 32);

		wsusercmd_t nullcmd;
		memset(&nullcmd, 0, sizeof(nullcmd));
		wsusercmd_t* oldcmd = &nullcmd;
		wsusercmd_t cmds[Q3MAX_PACKET_USERCMDS];
		for (int i = 0; i < cmdCount; i++)
		{
			wsusercmd_t* cmd = &cmds[i];
			MSGWS_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
			oldcmd = cmd;
		}

		// save time for ping calculation
		cl->q3_frames[cl->q3_messageAcknowledge & PACKET_MASK_Q3].messageAcked = svs.q3_time;

		// if this is the first usercmd we have received
		// this gamestate, put the client into the world
		if (cl->state == CS_PRIMED)
		{
			SVWS_ClientEnterWorld(cl, &cmds[0]);
			// the moves can be processed normaly
		}
		//
		if (svt3_pure->integer != 0 && cl->q3_pureAuthentic == 0)
		{
			SVT3_DropClient(cl, "Cannot validate pure client!");
			return;
		}

		if (cl->state != CS_ACTIVE)
		{
			cl->q3_deltaMessage = -1;
			return;
		}

		// usually, the first couple commands will be duplicates
		// of ones we have previously received, but the servertimes
		// in the commands will cause them to be immediately discarded
		for (int i =  0; i < cmdCount; i++)
		{
			// if this is a cmd from before a map_restart ignore it
			if (cmds[i].serverTime > cmds[cmdCount - 1].serverTime)
			{
				continue;
			}
			// extremely lagged or cmd from before a map_restart
			//if ( cmds[i].serverTime > svs.q3_time + 3000 ) {
			//	continue;
			//}
			if (svt3_gametype->integer != Q3GT_SINGLE_PLAYER)		// RF, we need to allow this in single player, where loadgame's can cause the player to freeze after reloading if we do this check
			{	// don't execute if this is an old cmd which is already executed
				// these old cmds are included when cl_packetdup > 0
				if (cmds[i].serverTime <= cl->ws_lastUsercmd.serverTime)		// Q3_MISSIONPACK
				{	//			if ( cmds[i].serverTime > cmds[cmdCount-1].serverTime ) {
					continue;	// from just before a map_restart
				}
			}
			SVWS_ClientThink(cl, &cmds[i]);
		}
	}
	else if (GGameType & GAME_WolfMP)
	{
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(cl->q3_reliableCommands[cl->q3_reliableAcknowledge & (MAX_RELIABLE_COMMANDS_WOLF - 1)], 32);

		wmusercmd_t nullcmd;
		memset(&nullcmd, 0, sizeof(nullcmd));
		wmusercmd_t* oldcmd = &nullcmd;
		wmusercmd_t cmds[Q3MAX_PACKET_USERCMDS];
		for (int i = 0; i < cmdCount; i++)
		{
			wmusercmd_t* cmd = &cmds[i];
			MSGWM_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
			oldcmd = cmd;
		}

		// save time for ping calculation
		cl->q3_frames[cl->q3_messageAcknowledge & PACKET_MASK_Q3].messageAcked = svs.q3_time;

		// TTimo
		// catch the no-cp-yet situation before SVWM_ClientEnterWorld
		// if CS_ACTIVE, then it's time to trigger a new gamestate emission
		// if not, then we are getting remaining parasite usermove commands, which we should ignore
		if (svt3_pure->integer != 0 && cl->q3_pureAuthentic == 0 && !cl->q3_gotCP)
		{
			if (cl->state == CS_ACTIVE)
			{
				// we didn't get a cp yet, don't assume anything and just send the gamestate all over again
				common->DPrintf("%s: didn't get cp command, resending gamestate\n", cl->name);
				SVT3_SendClientGameState(cl);
			}
			return;
		}

		// if this is the first usercmd we have received
		// this gamestate, put the client into the world
		if (cl->state == CS_PRIMED)
		{
			SVWM_ClientEnterWorld(cl, &cmds[0]);
			// the moves can be processed normaly
		}

		// a bad cp command was sent, drop the client
		if (svt3_pure->integer != 0 && cl->q3_pureAuthentic == 0)
		{
			SVT3_DropClient(cl, "Cannot validate pure client!");
			return;
		}

		if (cl->state != CS_ACTIVE)
		{
			cl->q3_deltaMessage = -1;
			return;
		}

		// usually, the first couple commands will be duplicates
		// of ones we have previously received, but the servertimes
		// in the commands will cause them to be immediately discarded
		for (int i =  0; i < cmdCount; i++)
		{
			// if this is a cmd from before a map_restart ignore it
			if (cmds[i].serverTime > cmds[cmdCount - 1].serverTime)
			{
				continue;
			}
			// extremely lagged or cmd from before a map_restart
			//if ( cmds[i].serverTime > svs.q3_time + 3000 ) {
			//	continue;
			//}
			if (svt3_gametype->integer != Q3GT_SINGLE_PLAYER)		// RF, we need to allow this in single player, where loadgame's can cause the player to freeze after reloading if we do this check
			{	// don't execute if this is an old cmd which is already executed
				// these old cmds are included when cl_packetdup > 0
				if (cmds[i].serverTime <= cl->wm_lastUsercmd.serverTime)		// Q3_MISSIONPACK
				{	//			if ( cmds[i].serverTime > cmds[cmdCount-1].serverTime ) {
					continue;	// from just before a map_restart
				}
			}
			SVWM_ClientThink(cl, &cmds[i]);
		}
	}
	else
	{
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(cl->q3_reliableCommands[cl->q3_reliableAcknowledge & (MAX_RELIABLE_COMMANDS_WOLF - 1)], 32);

		etusercmd_t nullcmd;
		memset(&nullcmd, 0, sizeof(nullcmd));
		etusercmd_t* oldcmd = &nullcmd;
		etusercmd_t cmds[Q3MAX_PACKET_USERCMDS];
		for (int i = 0; i < cmdCount; i++)
		{
			etusercmd_t* cmd = &cmds[i];
			MSGET_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
			oldcmd = cmd;
		}

		// save time for ping calculation
		cl->q3_frames[cl->q3_messageAcknowledge & PACKET_MASK_Q3].messageAcked = svs.q3_time;

		// TTimo
		// catch the no-cp-yet situation before SVET_ClientEnterWorld
		// if CS_ACTIVE, then it's time to trigger a new gamestate emission
		// if not, then we are getting remaining parasite usermove commands, which we should ignore
		if (svt3_pure->integer != 0 && cl->q3_pureAuthentic == 0 && !cl->q3_gotCP)
		{
			if (cl->state == CS_ACTIVE)
			{
				// we didn't get a cp yet, don't assume anything and just send the gamestate all over again
				common->DPrintf("%s: didn't get cp command, resending gamestate\n", cl->name);
				SVT3_SendClientGameState(cl);
			}
			return;
		}

		// if this is the first usercmd we have received
		// this gamestate, put the client into the world
		if (cl->state == CS_PRIMED)
		{
			SVET_ClientEnterWorld(cl, &cmds[0]);
			// the moves can be processed normaly
		}

		// a bad cp command was sent, drop the client
		if (svt3_pure->integer != 0 && cl->q3_pureAuthentic == 0)
		{
			SVT3_DropClient(cl, "Cannot validate pure client!");
			return;
		}

		if (cl->state != CS_ACTIVE)
		{
			cl->q3_deltaMessage = -1;
			return;
		}

		// usually, the first couple commands will be duplicates
		// of ones we have previously received, but the servertimes
		// in the commands will cause them to be immediately discarded
		for (int i =  0; i < cmdCount; i++)
		{
			// if this is a cmd from before a map_restart ignore it
			if (cmds[i].serverTime > cmds[cmdCount - 1].serverTime)
			{
				continue;
			}
			// extremely lagged or cmd from before a map_restart
			//if ( cmds[i].serverTime > svs.q3_time + 3000 ) {
			//	continue;
			//}
			if (!SVET_GameIsSinglePlayer())		// We need to allow this in single player, where loadgame's can cause the player to freeze after reloading if we do this check
			{	// don't execute if this is an old cmd which is already executed
				// these old cmds are included when cl_packetdup > 0
				if (cmds[i].serverTime <= cl->et_lastUsercmd.serverTime)		// Q3_MISSIONPACK
				{	//			if ( cmds[i].serverTime > cmds[cmdCount-1].serverTime ) {
					continue;	// from just before a map_restart
				}
			}
			SVET_ClientThink(cl, &cmds[i]);
		}
	}
}

static void SVET_ParseBinaryMessage(client_t* cl, QMsg* msg)
{
	msg->Uncompressed();

	int size = msg->cursize - msg->readcount;
	if (size <= 0 || size > MAX_BINARY_MESSAGE_ET)
	{
		return;
	}

	SVET_GameBinaryMessageReceived(cl - svs.clients, (char*)&msg->_data[msg->readcount], size, cl->et_lastUsercmd.serverTime);
}

//	Parse a client packet
void SVT3_ExecuteClientMessage(client_t* cl, QMsg* msg)
{
	msg->Bitstream();

	int serverId = msg->ReadLong();
	cl->q3_messageAcknowledge = msg->ReadLong();

	if (cl->q3_messageAcknowledge < 0)
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifndef NDEBUG
		SVT3_DropClient(cl, "DEBUG: illegible client message");
#endif
		return;
	}

	cl->q3_reliableAcknowledge = msg->ReadLong();

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SVT3_UpdateServerCommandsToClient
	if (cl->q3_reliableAcknowledge < cl->q3_reliableSequence - (GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF))
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifndef NDEBUG
		SVT3_DropClient(cl, "DEBUG: illegible client message");
#endif
		cl->q3_reliableAcknowledge = cl->q3_reliableSequence;
		return;
	}
	// if this is a usercmd from a previous gamestate,
	// ignore it or retransmit the current gamestate
	//
	// if the client was downloading, let it stay at whatever serverId and
	// gamestate it was at.  This allows it to keep downloading even when
	// the gamestate changes.  After the download is finished, we'll
	// notice and send it a new game state
	//
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=536
	// don't drop as long as previous command was a nextdl, after a dl is done, downloadName is set back to ""
	// but we still need to read the next message to move to next download or send gamestate
	// I don't like this hack though, it must have been working fine at some point, suspecting the fix is somewhere else
	if (serverId != sv.q3_serverId && !*cl->downloadName && !strstr(cl->q3_lastClientCommandString, "nextdl"))
	{
		if (serverId >= sv.q3_restartedServerId && serverId < sv.q3_serverId)		// TTimo - use a comparison here to catch multiple map_restart
		{	// they just haven't caught the map_restart yet
			common->DPrintf("%s : ignoring pre map_restart / outdated client message\n", cl->name);
			return;
		}
		// if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		if (cl->q3_messageAcknowledge > cl->q3_gamestateMessageNum)
		{
			common->DPrintf("%s : dropped gamestate, resending\n", cl->name);
			SVT3_SendClientGameState(cl);
		}

		if (GGameType & GAME_ET)
		{
			// read optional clientCommand strings
			do
			{
				int c = msg->ReadByte();
				if (c == q3clc_EOF)
				{
					break;
				}
				if (c != q3clc_clientCommand)
				{
					break;
				}
				if (!SVT3_ClientCommand(cl, msg, true))
				{
					return;	// we couldn't execute it because of the flood protection
				}
				if (cl->state == CS_ZOMBIE)
				{
					return;	// disconnect command
				}
			}
			while (1);
		}

		return;
	}

	if (GGameType & GAME_WolfSP)
	{
		// RF, kill any reliableCommands that have been acknowledged
		SVWS_FreeAcknowledgedReliableCommands(cl);
	}

	// read optional clientCommand strings
	int c;
	do
	{
		c = msg->ReadByte();
		if (c == q3clc_EOF)
		{
			break;
		}
		if (c != q3clc_clientCommand)
		{
			break;
		}
		if (!SVT3_ClientCommand(cl, msg, false))
		{
			return;	// we couldn't execute it because of the flood protection
		}
		if (cl->state == CS_ZOMBIE)
		{
			return;	// disconnect command
		}
	}
	while (1);

	// read the usercmd_t
	if (c == q3clc_move)
	{
		SVT3_UserMove(cl, msg, true);
		c = msg->ReadByte();
	}
	else if (c == q3clc_moveNoDelta)
	{
		SVT3_UserMove(cl, msg, false);
		c = msg->ReadByte();
	}

	if (c != q3clc_EOF)
	{
		common->Printf("WARNING: bad command byte for client %i\n", static_cast<int>(cl - svs.clients));
	}

	if (GGameType & GAME_ET)
	{
		SVET_ParseBinaryMessage(cl, msg);
	}
}
