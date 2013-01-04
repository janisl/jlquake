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

#include "demo.h"
#include "../../client_main.h"
#include "../../public.h"
#include "../light_styles.h"
#include "../quake/local.h"
#include "../hexen2/local.h"
#include "menu.h"
#include "connection.h"
#include "../../../server/public.h"

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

#define dem_cmd     0
#define dem_read    1
#define dem_set     2

static void CLQH_FinishTimeDemo()
{
	cls.qh_timedemo = false;

	// the first frame didn't count
	int frames = (cls.framecount - cls.qh_td_startframe) - 1;
	float time = Sys_DoubleTime() - cls.qh_td_starttime;
	if (!time)
	{
		time = 1;
	}
	common->Printf("%i frames %5.1f seconds %5.1f fps\n", frames, time, frames / time);
}

//	Called when a demo file runs out, or the user starts a game
void CLQH_StopPlayback()
{
	if (!clc.demoplaying)
	{
		return;
	}

	if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld) && h2intro_playing)
	{
		MQH_ToggleMenu_f();
		h2intro_playing = false;
	}

	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;
	clc.demoplaying = false;
	cls.state = CA_DISCONNECTED;

	if (cls.qh_timedemo)
	{
		CLQH_FinishTimeDemo();
	}
}

//	Dumps the current net message, prefixed by the length and view angles
static void CLQH_WriteDemoMessage(QMsg* msg)
{
	// skip the packet sequencing information
	int len = LittleLong(msg->cursize);
	FS_Write(&len, 4, clc.demofile);
	for (int i = 0; i < 3; i++)
	{
		float f = LittleFloat(cl.viewangles[i]);
		FS_Write(&f, 4, clc.demofile);
	}
	FS_Write(msg->_data, msg->cursize, clc.demofile);
}

//	Dumps the current net message, prefixed by the length
static void CLQHW_WriteDemoMessage(QMsg* msg)
{
	float fl = LittleFloat(cls.realtime * 0.001f);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	byte c = dem_read;
	FS_Write(&c, sizeof(c), clc.demofile);

	// skip the packet sequencing information
	int len = LittleLong(msg->cursize);
	FS_Write(&len, 4, clc.demofile);
	FS_Write(msg->_data, msg->cursize, clc.demofile);
}

//	Writes the current user cmd
void CLQW_WriteDemoCmd(qwusercmd_t* pcmd)
{
	float fl = LittleFloat(cls.realtime * 0.001f);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	byte c = dem_cmd;
	FS_Write(&c, sizeof(c), clc.demofile);

	// correct for byte order, bytes don't matter
	qwusercmd_t cmd = *pcmd;

	for (int i = 0; i < 3; i++)
	{
		cmd.angles[i] = LittleFloat(cmd.angles[i]);
	}
	cmd.forwardmove = LittleShort(cmd.forwardmove);
	cmd.sidemove    = LittleShort(cmd.sidemove);
	cmd.upmove      = LittleShort(cmd.upmove);

	FS_Write(&cmd, sizeof(cmd), clc.demofile);

	for (int i = 0; i < 3; i++)
	{
		fl = LittleFloat(cl.viewangles[i]);
		FS_Write(&fl, 4, clc.demofile);
	}

	FS_Flush(clc.demofile);
}

//	Writes the current user cmd
void CLHW_WriteDemoCmd(hwusercmd_t* pcmd)
{
	float fl = LittleFloat(cls.realtime * 0.001f);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	byte c = dem_cmd;
	FS_Write(&c, sizeof(c), clc.demofile);

	// correct for byte order, bytes don't matter
	hwusercmd_t cmd = *pcmd;

	for (int i = 0; i < 3; i++)
	{
		cmd.angles[i] = LittleFloat(cmd.angles[i]);
	}
	cmd.forwardmove = LittleShort(cmd.forwardmove);
	cmd.sidemove = LittleShort(cmd.sidemove);
	cmd.upmove = LittleShort(cmd.upmove);

	FS_Write(&cmd, sizeof(cmd), clc.demofile);

	for (int i = 0; i < 3; i++)
	{
		fl = LittleFloat(cl.viewangles[i]);
		FS_Write(&fl, 4, clc.demofile);
	}

	FS_Flush(clc.demofile);
}

//	Dumps the current net message, prefixed by the length and view angles
static void CLQW_WriteRecordDemoMessage(QMsg* msg, int seq)
{
	float fl = LittleFloat(cls.realtime * 0.001f);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	byte c = dem_read;
	FS_Write(&c, sizeof(c), clc.demofile);

	int len = LittleLong(msg->cursize + 8);
	FS_Write(&len, 4, clc.demofile);

	int i = LittleLong(seq);
	FS_Write(&i, 4, clc.demofile);
	FS_Write(&i, 4, clc.demofile);

	FS_Write(msg->_data, msg->cursize, clc.demofile);

	FS_Flush(clc.demofile);
}

static void CLQW_WriteSetDemoMessage()
{
	float fl = LittleFloat(cls.realtime * 0.001f);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	byte c = dem_set;
	FS_Write(&c, sizeof(c), clc.demofile);

	int len = LittleLong(clc.netchan.outgoingSequence);
	FS_Write(&len, 4, clc.demofile);
	len = LittleLong(clc.netchan.incomingSequence);
	FS_Write(&len, 4, clc.demofile);

	FS_Flush(clc.demofile);
}

void CLWQ_WriteServerDataToDemo()
{
	// send the info about the new client to all connected clients
	QMsg buf;
	byte buf_data[MAX_MSGLEN_QW];
	buf.InitOOB(buf_data, sizeof(buf_data));

	// send the serverdata
	buf.WriteByte(qwsvc_serverdata);
	buf.WriteLong(QWPROTOCOL_VERSION);
	buf.WriteLong(cl.servercount);
	buf.WriteString2(fsqhw_gamedirfile);

	if (cl.qh_spectator)
	{
		buf.WriteByte(cl.playernum | 128);
	}
	else
	{
		buf.WriteByte(cl.playernum);
	}

	// send full levelname
	buf.WriteString2(cl.qh_levelname);

	// send the movevars
	buf.WriteFloat(movevars.gravity);
	buf.WriteFloat(movevars.stopspeed);
	buf.WriteFloat(movevars.maxspeed);
	buf.WriteFloat(movevars.spectatormaxspeed);
	buf.WriteFloat(movevars.accelerate);
	buf.WriteFloat(movevars.airaccelerate);
	buf.WriteFloat(movevars.wateraccelerate);
	buf.WriteFloat(movevars.friction);
	buf.WriteFloat(movevars.waterfriction);
	buf.WriteFloat(movevars.entgravity);

	// send music
	buf.WriteByte(q1svc_cdtrack);
	buf.WriteByte(0);	// none in demos

	// send server info string
	buf.WriteByte(q1svc_stufftext);
	buf.WriteString2(va("fullserverinfo \"%s\"\n", cl.qh_serverinfo));

	int seq = 1;

	// flush packet
	CLQW_WriteRecordDemoMessage(&buf, seq++);
	buf.Clear();

	// soundlist
	buf.WriteByte(qwsvc_soundlist);
	buf.WriteByte(0);

	int n = 0;
	const char* s = cl.qh_sound_name[n + 1];
	while (*s)
	{
		buf.WriteString2(s);
		if (buf.cursize > MAX_MSGLEN_QW / 2)
		{
			buf.WriteByte(0);
			buf.WriteByte(n);
			CLQW_WriteRecordDemoMessage(&buf, seq++);
			buf.Clear();
			buf.WriteByte(qwsvc_soundlist);
			buf.WriteByte(n + 1);
		}
		n++;
		s = cl.qh_sound_name[n + 1];
	}
	if (buf.cursize)
	{
		buf.WriteByte(0);
		buf.WriteByte(0);
		CLQW_WriteRecordDemoMessage(&buf, seq++);
		buf.Clear();
	}

	// modellist
	buf.WriteByte(qwsvc_modellist);
	buf.WriteByte(0);

	n = 0;
	s = cl.qh_model_name[n + 1];
	while (*s)
	{
		buf.WriteString2(s);
		if (buf.cursize > MAX_MSGLEN_QW / 2)
		{
			buf.WriteByte(0);
			buf.WriteByte(n);
			CLQW_WriteRecordDemoMessage(&buf, seq++);
			buf.Clear();
			buf.WriteByte(qwsvc_modellist);
			buf.WriteByte(n + 1);
		}
		n++;
		s = cl.qh_model_name[n + 1];
	}
	if (buf.cursize)
	{
		buf.WriteByte(0);
		buf.WriteByte(0);
		CLQW_WriteRecordDemoMessage(&buf, seq++);
		buf.Clear();
	}

	// spawnstatic
	for (int i = 0; i < cl.qh_num_statics; i++)
	{
		q1entity_t* ent = clq1_static_entities + i;

		buf.WriteByte(q1svc_spawnstatic);

		buf.WriteByte(ent->state.modelindex);
		buf.WriteByte(ent->state.frame);
		buf.WriteByte(0);
		buf.WriteByte(ent->state.skinnum);
		for (int j = 0; j < 3; j++)
		{
			buf.WriteCoord(ent->state.origin[j]);
			buf.WriteAngle(ent->state.angles[j]);
		}

		if (buf.cursize > MAX_MSGLEN_QW / 2)
		{
			CLQW_WriteRecordDemoMessage(&buf, seq++);
			buf.Clear();
		}
	}

	// spawnstaticsound
	// static sounds are skipped in demos, life is hard

	// baselines

	q1entity_state_t blankes = {};
	for (int i = 0; i < MAX_EDICTS_QH; i++)
	{
		q1entity_state_t* es = clq1_baselines + i;

		if (memcmp(es, &blankes, sizeof(blankes)))
		{
			buf.WriteByte(q1svc_spawnbaseline);
			buf.WriteShort(i);

			buf.WriteByte(es->modelindex);
			buf.WriteByte(es->frame);
			buf.WriteByte(es->colormap);
			buf.WriteByte(es->skinnum);
			for (int j = 0; j < 3; j++)
			{
				buf.WriteCoord(es->origin[j]);
				buf.WriteAngle(es->angles[j]);
			}

			if (buf.cursize > MAX_MSGLEN_QW / 2)
			{
				CLQW_WriteRecordDemoMessage(&buf, seq++);
				buf.Clear();
			}
		}
	}

	buf.WriteByte(q1svc_stufftext);
	buf.WriteString2(va("cmd spawn %i 0\n", cl.servercount));

	if (buf.cursize)
	{
		CLQW_WriteRecordDemoMessage(&buf, seq++);
		buf.Clear();
	}

	// send current status of all other players
	for (int i = 0; i < MAX_CLIENTS_QHW; i++)
	{
		q1player_info_t* player = cl.q1_players + i;

		buf.WriteByte(q1svc_updatefrags);
		buf.WriteByte(i);
		buf.WriteShort(player->frags);

		buf.WriteByte(qwsvc_updateping);
		buf.WriteByte(i);
		buf.WriteShort(player->ping);

		buf.WriteByte(qwsvc_updatepl);
		buf.WriteByte(i);
		buf.WriteByte(player->pl);

		buf.WriteByte(qwsvc_updateentertime);
		buf.WriteByte(i);
		buf.WriteFloat(player->entertime);

		buf.WriteByte(qwsvc_updateuserinfo);
		buf.WriteByte(i);
		buf.WriteLong(player->userid);
		buf.WriteString2(player->userinfo);

		if (buf.cursize > MAX_MSGLEN_QW / 2)
		{
			CLQW_WriteRecordDemoMessage(&buf, seq++);
			buf.Clear();
		}
	}

	// send all current light styles
	for (int i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		buf.WriteByte(q1svc_lightstyle);
		buf.WriteByte((char)i);
		buf.WriteString2(cl_lightstyle[i].mapStr);
	}

	for (int i = 0; i < MAX_CL_STATS; i++)
	{
		buf.WriteByte(qwsvc_updatestatlong);
		buf.WriteByte(i);
		buf.WriteLong(cl.qh_stats[i]);
		if (buf.cursize > MAX_MSGLEN_QW / 2)
		{
			CLQW_WriteRecordDemoMessage(&buf, seq++);
			buf.Clear();
		}
	}

#if 0
	buf.WriteByte(qwsvc_updatestatlong);
	buf.WriteByte(Q1STAT_TOTALMONSTERS);
	buf.WriteLong(cl.stats[Q1STAT_TOTALMONSTERS]);

	buf.WriteByte(qwsvc_updatestatlong);
	buf.WriteByte(Q1STAT_SECRETS);
	buf.WriteLong(cl.stats[Q1STAT_SECRETS]);

	buf.WriteByte(qwsvc_updatestatlong);
	buf.WriteByte(Q1STAT_MONSTERS);
	buf.WriteLong(cl.stats[Q1STAT_MONSTERS]);
#endif

	// get the client to check and download skins
	// when that is completed, a begin command will be issued
	buf.WriteByte(q1svc_stufftext);
	buf.WriteString2(va("skins\n"));

	CLQW_WriteRecordDemoMessage(&buf, seq++);

	CLQW_WriteSetDemoMessage();

	// done
}

static bool CLQH_GetDemoMessage(QMsg& message)
{
	// decide if it is time to grab the next message
	if (clc.qh_signon == SIGNONS)	// allways grab until fully connected
	{
		if (cls.qh_timedemo)
		{
			if (cls.framecount == cls.qh_td_lastframe)
			{
				return 0;		// allready read this frame's message
			}
			cls.qh_td_lastframe = cls.framecount;
			// if this is the second frame, grab the real td_starttime
			// so the bogus time on the first frame doesn't count
			if (cls.framecount == cls.qh_td_startframe + 1)
			{
				cls.qh_td_starttime = Sys_DoubleTime();
			}
		}
		else if (/* cl.time > 0 && */ cl.qh_serverTimeFloat <= cl.qh_mtime[0])
		{
			return 0;			// don't need another message yet
		}
	}

	// get the next message
	FS_Read(&message.cursize, 4, clc.demofile);
	VectorCopy(cl.qh_mviewangles[0], cl.qh_mviewangles[1]);
	for (int i = 0; i < 3; i++)
	{
		float f;
		FS_Read(&f, 4, clc.demofile);
		cl.qh_mviewangles[0][i] = LittleFloat(f);
	}

	message.cursize = LittleLong(message.cursize);
	if (message.cursize > message.maxsize)
	{
		common->FatalError("Demo message > MAX_MSGLEN");
	}
	int r = FS_Read(message._data, message.cursize, clc.demofile);
	if (r != message.cursize)
	{
		CLQH_StopPlayback();
		return 0;
	}

	return 1;
}

static bool CLQW_ReadDemoUserCommand(float demotime)
{
	int i = clc.netchan.outgoingSequence & UPDATE_MASK_QW;
	qwusercmd_t* pcmd = &cl.qw_frames[i].cmd;
	int r = FS_Read(pcmd, sizeof(*pcmd), clc.demofile);
	if (r != sizeof(*pcmd))
	{
		CLQH_StopPlayback();
		return false;
	}
	// byte order stuff
	for (int j = 0; j < 3; j++)
	{
		pcmd->angles[j] = LittleFloat(pcmd->angles[j]);
	}
	pcmd->forwardmove = LittleShort(pcmd->forwardmove);
	pcmd->sidemove    = LittleShort(pcmd->sidemove);
	pcmd->upmove      = LittleShort(pcmd->upmove);
	cl.qw_frames[i].senttime = demotime;
	cl.qw_frames[i].receivedtime = -1;		// we haven't gotten a reply yet
	clc.netchan.outgoingSequence++;
	for (i = 0; i < 3; i++)
	{
		float f;
		r = FS_Read(&f, 4, clc.demofile);
		cl.viewangles[i] = LittleFloat(f);
	}
	return true;
}

static bool CLHW_ReadDemoUserCommand(float demotime)
{
	int i = clc.netchan.outgoingSequence & UPDATE_MASK_HW;
	hwusercmd_t* pcmd = &cl.hw_frames[i].cmd;
	int r = FS_Read(pcmd, sizeof(*pcmd), clc.demofile);
	if (r != sizeof(*pcmd))
	{
		CLQH_StopPlayback();
		return false;
	}
	// byte order stuff
	for (int j = 0; j < 3; j++)
	{
		pcmd->angles[j] = LittleFloat(pcmd->angles[j]);
	}
	pcmd->forwardmove = LittleShort(pcmd->forwardmove);
	pcmd->sidemove    = LittleShort(pcmd->sidemove);
	pcmd->upmove      = LittleShort(pcmd->upmove);
	cl.hw_frames[i].senttime = demotime;
	cl.hw_frames[i].receivedtime = -1;		// we haven't gotten a reply yet
	clc.netchan.outgoingSequence++;
	for (i = 0; i < 3; i++)
	{
		float f;
		r = FS_Read(&f, 4, clc.demofile);
		cl.viewangles[i] = LittleFloat(f);
	}
	return true;
}

static bool CLQHW_GetDemoMessage(QMsg& message)
{
	// read the time from the packet
	float demotime;
	FS_Read(&demotime, sizeof(demotime), clc.demofile);
	demotime = LittleFloat(demotime);

	// decide if it is time to grab the next message
	if (cls.qh_timedemo)
	{
		if (cls.qh_td_lastframe < 0)
		{
			cls.qh_td_lastframe = demotime;
		}
		else if (demotime > cls.qh_td_lastframe)
		{
			cls.qh_td_lastframe = demotime;
			// rewind back to time
			FS_Seek(clc.demofile, FS_FTell(clc.demofile) - sizeof(demotime), FS_SEEK_SET);
			return false;		// allready read this frame's message
		}
		if (!cls.qh_td_starttime && cls.state == CA_ACTIVE)
		{
			cls.qh_td_starttime = Sys_DoubleTime();
			cls.qh_td_startframe = cls.framecount;
		}
		cls.realtime = demotime * 1000;// warp
	}
	else if ((GGameType & GAME_HexenWorld || !cl.qh_paused) && cls.state >= CA_LOADING)		// allways grab until fully connected
	{
		if (cls.realtime * 0.001 + 1.0 < demotime)
		{
			// too far back
			cls.realtime = demotime * 1000 - 1000;
			// rewind back to time
			FS_Seek(clc.demofile, FS_FTell(clc.demofile) - sizeof(demotime), FS_SEEK_SET);
			return false;
		}
		else if (cls.realtime * 0.001 < demotime)
		{
			// rewind back to time
			FS_Seek(clc.demofile, FS_FTell(clc.demofile) - sizeof(demotime), FS_SEEK_SET);
			return false;		// don't need another message yet
		}
	}
	else
	{
		cls.realtime = demotime * 1000;// we're warping

	}
	if (cls.state == CA_DISCONNECTED)
	{
		common->FatalError("CLQHW_GetDemoMessage: cls.state != ca_active");
	}

	// get the msg type
	byte c;
	FS_Read(&c, sizeof(c), clc.demofile);

	switch (c)
	{
	case dem_cmd:
		// user sent input
		if (GGameType & GAME_HexenWorld)
		{
			if (!CLHW_ReadDemoUserCommand(demotime))
			{
				return false;
			}
		}
		else
		{
			if (!CLQW_ReadDemoUserCommand(demotime))
			{
				return false;
			}
		}
		break;

	case dem_read:
	{
		// get the next message
		FS_Read(&message.cursize, 4, clc.demofile);
		message.cursize = LittleLong(message.cursize);
		//common->Printf("read: %ld bytes\n", net_message.cursize);
		if (message.cursize > message.maxsize)
		{
			common->FatalError("Demo message > MAX_MSGLEN");
		}
		int r = FS_Read(message._data, message.cursize, clc.demofile);
		if (r != message.cursize)
		{
			CLQH_StopPlayback();
			return false;
		}
	}
		break;

	case dem_set:
		if (GGameType & GAME_QuakeWorld)
		{
			int i;
			FS_Read(&i, 4, clc.demofile);
			clc.netchan.outgoingSequence = LittleLong(i);
			FS_Read(&i, 4, clc.demofile);
			clc.netchan.incomingSequence = LittleLong(i);
			break;
		}

	default:
		common->Printf("Corrupted demo.\n");
		CLQH_StopPlayback();
		return false;
	}

	return true;
}

//	Handles recording and playback of demos, on top of NET_ code
int CLQH_GetMessage(QMsg& message)
{
	if  (clc.demoplaying)
	{
		return CLQH_GetDemoMessage(message);
	}

	int r;
	while (1)
	{
		r = NET_GetMessage(&clc.netchan, &message);

		if (r != 1 && r != 2)
		{
			return r;
		}

		// discard nop keepalive message
		if (message.cursize == 1 && message._data[0] == (GGameType & GAME_Hexen2 ? h2svc_nop : q1svc_nop))
		{
			common->Printf("<-- server to client keepalive\n");
		}
		else
		{
			break;
		}
	}

	if (clc.demorecording)
	{
		CLQH_WriteDemoMessage(&message);
	}

	return r;
}

//	Handles recording and playback of demos, on top of NET_ code
bool CLQHW_GetMessage(QMsg& message, netadr_t& from)
{
	if  (clc.demoplaying)
	{
		return CLQHW_GetDemoMessage(message);
	}

	if (!NET_GetUdpPacket(NS_SERVER, &from, &message))
	{
		return false;
	}

	if (clc.demorecording)
	{
		CLQHW_WriteDemoMessage(&message);
	}

	return true;
}

//	stop recording a demo
void CLQH_Stop_f()
{
	if (!clc.demorecording)
	{
		common->Printf("Not recording a demo.\n");
		return;
	}

	// write a disconnect message to the demo file
	QMsg message;
	byte message_buffer[32];
	message.InitOOB(message_buffer, sizeof(message_buffer));
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		message.WriteLong(-1);	// -1 sequence means out of band
		message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_disconnect : q1svc_disconnect);
		message.WriteString2("EndOfDemo");
		CLQHW_WriteDemoMessage(&message);
	}
	else
	{
		message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_disconnect : q1svc_disconnect);
		CLQH_WriteDemoMessage(&message);
	}

	// finish up
	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;
	clc.demorecording = false;
	common->Printf("Completed demo\n");
}

void CLQH_Record_f()
{
	int c = Cmd_Argc();
	if (c != 2 && c != 3 && c != 4)
	{
		common->Printf("record <demoname> [<map> [cd track]]\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		common->Printf("Relative pathnames are not allowed.\n");
		return;
	}

	if (c == 2 && cls.state != CA_DISCONNECTED)
	{
		common->Printf("Can not record - already connected to server\nClient demo recording must be started before connecting\n");
		return;
	}

	// write the forced cd track number, or -1
	int track;
	if (c == 4)
	{
		track = String::Atoi(Cmd_Argv(3));
		common->Printf("Forcing CD track to %i\n", cls.qh_forcetrack);
	}
	else
	{
		track = -1;
	}

	char name[MAX_OSPATH];
	String::Cpy(name, Cmd_Argv(1));

	//
	// start the map up
	//
	if (c > 2)
	{
		Cmd_ExecuteString(va("map %s", Cmd_Argv(2)));
	}

	//
	// open the demo file
	//
	String::DefaultExtension(name, sizeof(name), ".dem");

	common->Printf("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	cls.qh_forcetrack = track;
	FS_Printf(clc.demofile, "%i\n", cls.qh_forcetrack);

	clc.demorecording = true;
}

void CLQW_Record_f()
{
	int c;
	char name[MAX_OSPATH];

	c = Cmd_Argc();
	if (c != 2)
	{
		common->Printf("record <demoname>\n");
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		common->Printf("You must be connected to record.\n");
		return;
	}

	if (clc.demorecording)
	{
		CLQH_Stop_f();
	}

	String::Cpy(name, Cmd_Argv(1));

	//
	// open the demo file
	//
	String::DefaultExtension(name, sizeof(name), ".qwd");

	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	common->Printf("recording to %s.\n", name);
	clc.demorecording = true;

	CLWQ_WriteServerDataToDemo();
}

void CLHW_Record_f()
{
	int c;
	char name[MAX_OSPATH];

	c = Cmd_Argc();
	if (c != 3)
	{
		common->Printf("record <demoname> <server>\n");
		return;
	}

	if (clc.demorecording)
	{
		CLQH_Stop_f();
	}

	String::Cpy(name, Cmd_Argv(1));

	//
	// open the demo file
	//
	String::DefaultExtension(name, sizeof(name), ".qwd");

	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	if (cls.state != CA_DISCONNECTED)
	{
		CL_Disconnect(true);
	}

	common->Printf("recording to %s.\n", name);
	clc.demorecording = true;

	//
	// start the map up
	//
	Cmd_ExecuteString(va("connect %s", Cmd_Argv(2)));
}

void CLQH_PlayDemo_f()
{
	char name[256];

	if (Cmd_Argc() != 2)
	{
		common->Printf("play <demoname> : plays a demo\n");
		return;
	}

//
// disconnect from server
//
	CL_Disconnect(true);

//
// open the demo file
//
	String::Cpy(name, Cmd_Argv(1));
	if (GGameType & GAME_Hexen2 && !String::ICmp(name,"t9"))
	{
		h2intro_playing = true;
	}
	else
	{
		h2intro_playing = false;
	}
	String::DefaultExtension(name, sizeof(name), ".dem");

	common->Printf("Playing demo from %s.\n", name);
	FS_FOpenFileRead(name, &clc.demofile, true);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		cls.qh_demonum = -1;		// stop demo loop
		return;
	}

	clc.demoplaying = true;
	clc.netchan.message.InitOOB(clc.netchan.messageBuffer, 1024);
	cls.state = CA_ACTIVE;
	cls.qh_forcetrack = 0;

	bool neg = false;
	char c;
	FS_Read(&c, 1, clc.demofile);
	while (c != '\n')
	{
		if (c == '-')
		{
			neg = true;
		}
		else
		{
			cls.qh_forcetrack = cls.qh_forcetrack * 10 + (c - '0');
		}
		if (FS_Read(&c, 1, clc.demofile) != 1)
		{
			break;
		}
	}

	if (neg)
	{
		cls.qh_forcetrack = -cls.qh_forcetrack;
	}
// ZOID, fscanf is evil
//	fscanf (clc.demofile, "%i\n", &cls.forcetrack);
}

void CLQHW_PlayDemo_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("play <demoname> : plays a demo\n");
		return;
	}

	//
	// disconnect from server
	//
	CL_Disconnect(true);

	//
	// open the demo file
	//
	char name[256];
	String::Cpy(name, Cmd_Argv(1));
	String::DefaultExtension(name, sizeof(name), ".qwd");

	common->Printf("Playing demo from %s.\n", name);
	FS_FOpenFileRead(name, &clc.demofile, true);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		cls.qh_demonum = -1;		// stop demo loop
		return;
	}

	clc.demoplaying = true;
	cls.state = CA_CONNECTING;
	netadr_t net_from = {};
	Netchan_Setup(NS_CLIENT, &clc.netchan, net_from, 0);
	//JL Huh?
	cls.realtime = 0;
}

void CLQH_TimeDemo_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("timedemo <demoname> : gets demo speeds\n");
		return;
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		CLQHW_PlayDemo_f();

		if (cls.state != CA_CONNECTING)
		{
			return;
		}
	}
	else
	{
		CLQH_PlayDemo_f();
	}

	// cls.td_starttime will be grabbed at the second frame of the demo, so
	// all the loading time doesn't get counted

	cls.qh_timedemo = true;
	cls.qh_td_starttime = 0;
	cls.qh_td_startframe = cls.framecount;
	cls.qh_td_lastframe = -1;		// get a new message this frame
}

void CLQHW_ReRecord_f()
{
	int c = Cmd_Argc();
	if (c != 2)
	{
		common->Printf("rerecord <demoname>\n");
		return;
	}

	if (!*cls.servername)
	{
		common->Printf("No server to reconnect to...\n");
		return;
	}

	if (clc.demorecording)
	{
		CLQH_Stop_f();
	}

	char name[MAX_OSPATH];
	String::Cpy(name, Cmd_Argv(1));

	//
	// open the demo file
	//
	String::DefaultExtension(name, sizeof(name), ".qwd");

	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	common->Printf("recording to %s.\n", name);
	clc.demorecording = true;

	CL_Disconnect(true);
	CLQHW_BeginServerConnect();
}

void CLQH_NextDemo()
{
	if (cls.qh_demonum == -1)
	{
		return;		// don't play demos
	}

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		SCRQH_BeginLoadingPlaque();
	}

	if (!cls.qh_demos[cls.qh_demonum][0] || cls.qh_demonum == MAX_DEMOS)
	{
		cls.qh_demonum = 0;
		if (!cls.qh_demos[cls.qh_demonum][0])
		{
			if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
			{
				common->Printf("No demos listed with startdemos\n");
			}
			cls.qh_demonum = -1;
			return;
		}
	}

	char str[1024];
	sprintf(str, "playdemo %s\n", cls.qh_demos[cls.qh_demonum]);
	Cbuf_InsertText(str);
	cls.qh_demonum++;
}

void CLQH_Startdemos_f()
{
	if (com_dedicated->integer)
	{
		if (!SV_IsServerActive())
		{
			Cbuf_AddText("map start\n");
		}
		return;
	}

	int c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		common->Printf("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	common->Printf("%i demo(s) in loop\n", c);

	for (int i = 1; i < c + 1; i++)
	{
		String::NCpy(cls.qh_demos[i - 1], Cmd_Argv(i), sizeof(cls.qh_demos[0]) - 1);
	}

	if (!SV_IsServerActive() && cls.qh_demonum != -1 && !clc.demoplaying)
	{
		cls.qh_demonum = 0;
		CL_NextDemo();
	}
	else
	{
		cls.qh_demonum = -1;
	}
}

//	Return to looping demos
void CLQH_Demos_f()
{
	if (com_dedicated->integer)
	{
		return;
	}
	if (cls.qh_demonum == -1)
	{
		cls.qh_demonum = 1;
	}
	CL_Disconnect_f();
	CL_NextDemo();
}

void CLQH_Stopdemo_f()
{
	if (com_dedicated->integer)
	{
		return;
	}
	if (!clc.demoplaying)
	{
		return;
	}
	CLQH_StopPlayback();
	CL_Disconnect(true);
}
