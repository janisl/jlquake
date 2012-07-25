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
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"

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
// or crashing -- SV_FinalMessage() will handle that
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
	SVQ3_GameClientBegin(clientNum);
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
	SVWS_GameClientBegin(clientNum);
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
	SVWM_GameClientBegin(clientNum);
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
	SVET_GameClientBegin(clientNum);
}

//	Abort a download if in progress
void SVT3_StopDownload_f(client_t* cl)
{
	if (*cl->downloadName)
	{
		common->DPrintf("clientDownload: %d : file \"%s\" aborted\n", static_cast<int>(cl - svs.clients), cl->downloadName);
	}

	SVT3_CloseDownload(cl);
}

//	The argument will be the last acknowledged block from the client, it should be
// the same as cl->downloadClientBlock
void SVT3_NextDownload_f(client_t* cl)
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

void SVT3_BeginDownload_f(client_t* cl)
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
void SVT3_Disconnect_f(client_t* cl)
{
	SVT3_DropClient(cl, "disconnected");
}

void SVT3_ResetPureClient_f(client_t* cl)
{
	cl->q3_pureAuthentic = 0;
	cl->q3_gotCP = false;
}

//	Pull specific info from a newly changed userinfo string
// into a more C friendly form.
void SVT3_UserinfoChanged(client_t* cl)
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
	// this is set in SV_DirectConnect (directly on the server, not transmitted), may be lost when client updates it's userinfo
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

void SVT3_UpdateUserinfo_f(client_t* cl)
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
