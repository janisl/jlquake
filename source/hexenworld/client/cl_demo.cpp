
#include "quakedef.h"

void CL_FinishTimeDemo(void);

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void CL_StopPlayback(void)
{
	if (!clc.demoplaying)
	{
		return;
	}

	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;
	cls.state = CA_DISCONNECTED;
	clc.demoplaying = false;

	if (cls.qh_timedemo)
	{
		CL_FinishTimeDemo();
	}
}

#define dem_cmd     0
#define dem_read    1

/*
====================
CL_WriteDemoCmd

Writes the current user cmd
====================
*/
void CL_WriteDemoCmd(hwusercmd_t* pcmd)
{
	int i;
	float fl;
	byte c;
	hwusercmd_t cmd;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, realtime);

	fl = LittleFloat((float)realtime);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	c = dem_cmd;
	FS_Write(&c, sizeof(c), clc.demofile);

	// correct for byte order, bytes don't matter
	cmd = *pcmd;

	for (i = 0; i < 3; i++)
		cmd.angles[i] = LittleFloat(cmd.angles[i]);
	cmd.forwardmove = LittleShort(cmd.forwardmove);
	cmd.sidemove    = LittleShort(cmd.sidemove);
	cmd.upmove      = LittleShort(cmd.upmove);

	FS_Write(&cmd, sizeof(cmd), clc.demofile);

	for (i = 0; i < 3; i++)
	{
		fl = LittleFloat(cl.viewangles[i]);
		FS_Write(&fl, 4, clc.demofile);
	}

	FS_Flush(clc.demofile);
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
void CL_WriteDemoMessage(QMsg* msg)
{
	int len;
	float fl;
	byte c;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, realtime);

	if (!clc.demorecording)
	{
		return;
	}

	fl = LittleFloat((float)realtime);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	c = dem_read;
	FS_Write(&c, sizeof(c), clc.demofile);

	len = LittleLong(msg->cursize);
	FS_Write(&len, 4, clc.demofile);
	FS_Write(msg->_data, msg->cursize, clc.demofile);

	FS_Flush(clc.demofile);
}

/*
====================
CL_GetDemoMessage

  FIXME...
====================
*/
qboolean CL_GetDemoMessage(void)
{
	int r, i, j;
	float f;
	float demotime;
	byte c;
	hwusercmd_t* pcmd;


	// read the time from the packet
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
			FS_Seek(clc.demofile, FS_FTell(clc.demofile) - sizeof(demotime),
				FS_SEEK_SET);
			return 0;		// allready read this frame's message
		}
		if (!cls.qh_td_starttime && cls.state == CA_ACTIVE)
		{
			cls.qh_td_starttime = Sys_DoubleTime();
			cls.qh_td_startframe = host_framecount;
		}
		realtime = demotime;// warp
	}
	else if (cls.state == CA_LOADING || cls.state == CA_ACTIVE)		// allways grab until fully connected
	{
		if (realtime + 1.0 < demotime)
		{
			// too far back
			realtime = demotime - 1.0;
			// rewind back to time
			FS_Seek(clc.demofile, FS_FTell(clc.demofile) - sizeof(demotime),
				FS_SEEK_SET);
			return 0;
		}
		else if (realtime < demotime)
		{
			// rewind back to time
			FS_Seek(clc.demofile, FS_FTell(clc.demofile) - sizeof(demotime),
				FS_SEEK_SET);
			return 0;		// don't need another message yet
		}
	}
	else
	{
		realtime = demotime;// we're warping

	}
	if (cls.state == CA_DISCONNECTED)
	{
		Host_FatalError("CL_GetDemoMessage: cls.state != ca_active");
	}

	// get the msg type
	FS_Read(&c, sizeof(c), clc.demofile);

	switch (c)
	{
	case dem_cmd:
		// user sent input
		i = clc.netchan.outgoingSequence & UPDATE_MASK_HW;
		pcmd = &cl.hw_frames[i].cmd;
		r = FS_Read(pcmd, sizeof(*pcmd), clc.demofile);
		if (r != sizeof(*pcmd))
		{
			CL_StopPlayback();
			return 0;
		}
		// byte order stuff
		for (j = 0; j < 3; j++)
			pcmd->angles[j] = LittleFloat(pcmd->angles[j]);
		pcmd->forwardmove = LittleShort(pcmd->forwardmove);
		pcmd->sidemove    = LittleShort(pcmd->sidemove);
		pcmd->upmove      = LittleShort(pcmd->upmove);
		cl.hw_frames[i].senttime = demotime;
		cl.hw_frames[i].receivedtime = -1;		// we haven't gotten a reply yet
		clc.netchan.outgoingSequence++;
		for (i = 0; i < 3; i++)
		{
			r = FS_Read(&f, 4, clc.demofile);
			cl.viewangles[i] = LittleFloat(f);
		}
		break;

	case dem_read:
		// get the next message
		FS_Read(&net_message.cursize, 4, clc.demofile);
		net_message.cursize = LittleLong(net_message.cursize);
		//Con_Printf("read: %ld bytes\n", net_message.cursize);
		if (net_message.cursize > MAX_MSGLEN_HW)
		{
			Sys_Error("Demo message > MAX_MSGLEN_HW");
		}
		r = FS_Read(net_message._data, net_message.cursize, clc.demofile);
		if (r != net_message.cursize)
		{
			CL_StopPlayback();
			return 0;
		}
		break;

	default:
		Con_Printf("Corrupted demo.\n");
		CL_StopPlayback();
		return 0;
	}

	return 1;
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
qboolean CL_GetMessage(void)
{
	if  (clc.demoplaying)
	{
		return CL_GetDemoMessage();
	}

	if (!NET_GetUdpPacket(NS_SERVER, &net_from, &net_message))
	{
		return false;
	}

	CL_WriteDemoMessage(&net_message);

	return true;
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f(void)
{
	if (!clc.demorecording)
	{
		Con_Printf("Not recording a demo.\n");
		return;
	}

// write a disconnect message to the demo file
	net_message.Clear();
	net_message.WriteLong(-1);	// -1 sequence means out of band
	net_message.WriteByte(h2svc_disconnect);
	net_message.WriteString2("EndOfDemo");
	CL_WriteDemoMessage(&net_message);

// finish up
	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;
	clc.demorecording = false;
	Con_Printf("Completed demo\n");
}

/*
====================
CL_Record_f

record <demoname> <server>
====================
*/
void CL_Record_f(void)
{
	int c;
	char name[MAX_OSPATH];

	c = Cmd_Argc();
	if (c != 3)
	{
		Con_Printf("record <demoname> <server>\n");
		return;
	}

	if (clc.demorecording)
	{
		CL_Stop_f();
	}

	String::Cpy(name, Cmd_Argv(1));

//
// open the demo file
//
	String::DefaultExtension(name, sizeof(name), ".qwd");

	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}

	if (cls.state != CA_DISCONNECTED)
	{
		CL_Disconnect();
	}

	Con_Printf("recording to %s.\n", name);
	clc.demorecording = true;

//
// start the map up
//
	Cmd_ExecuteString(va("connect %s", Cmd_Argv(2)));
}

/*
====================
CL_ReRecord_f

record <demoname>
====================
*/
void CL_ReRecord_f(void)
{
	int c;
	char name[MAX_OSPATH];

	c = Cmd_Argc();
	if (c != 2)
	{
		Con_Printf("rerecord <demoname>\n");
		return;
	}

	if (!*cls.servername)
	{
		Con_Printf("No server to reconnect to...\n");
		return;
	}

	if (clc.demorecording)
	{
		CL_Stop_f();
	}

	String::Cpy(name, Cmd_Argv(1));

//
// open the demo file
//
	String::DefaultExtension(name, sizeof(name), ".qwd");

	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}

	Con_Printf("recording to %s.\n", name);
	clc.demorecording = true;

	CL_Disconnect();
	CL_SendConnectPacket();
}


/*
====================
CL_PlayDemo_f

play [demoname]
====================
*/
void CL_PlayDemo_f(void)
{
	char name[256];

	if (Cmd_Argc() != 2)
	{
		Con_Printf("play <demoname> : plays a demo\n");
		return;
	}

//
// disconnect from server
//
	CL_Disconnect();

//
// open the demo file
//
	String::Cpy(name, Cmd_Argv(1));
	String::DefaultExtension(name, sizeof(name), ".qwd");

	Con_Printf("Playing demo from %s.\n", name);
	FS_FOpenFileRead(name, &clc.demofile, true);
	if (!clc.demofile)
	{
		Con_Printf("ERROR: couldn't open.\n");
		cls.qh_demonum = -1;		// stop demo loop
		return;
	}

	clc.demoplaying = true;
	cls.state = CA_DEMOSTART;
	Netchan_Setup(NS_CLIENT, &clc.netchan, net_from, 0);
	realtime = 0;
}

/*
====================
CL_FinishTimeDemo

====================
*/
void CL_FinishTimeDemo(void)
{
	int frames;
	float time;

	cls.qh_timedemo = false;

// the first frame didn't count
	frames = (host_framecount - cls.qh_td_startframe) - 1;
	time = Sys_DoubleTime() - cls.qh_td_starttime;
	if (!time)
	{
		time = 1;
	}
	Con_Printf("%i frames %5.1f seconds %5.1f fps\n", frames, time, frames / time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("timedemo <demoname> : gets demo speeds\n");
		return;
	}

	CL_PlayDemo_f();

//	if (cls.state != ca_active)
//		return;

// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted

	cls.qh_timedemo = true;
	cls.qh_td_starttime = 0;
	cls.qh_td_startframe = host_framecount;
	cls.qh_td_lastframe = -1;		// get a new message this frame
}
