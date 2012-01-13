/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

void CL_FinishTimeDemo (void);

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
void CL_StopPlayback (void)
{
	if (!clc.demoplaying)
		return;

	FS_FCloseFile (clc.demofile);
	clc.demoplaying = false;
	clc.demofile = 0;
	cls.state = CA_DISCONNECTED;

	if (cls.timedemo)
		CL_FinishTimeDemo ();
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
void CL_WriteDemoMessage (void)
{
	int		len;
	int		i;
	float	f;

	len = LittleLong (net_message.cursize);
	FS_Write(&len, 4, clc.demofile);
	for (i=0 ; i<3 ; i++)
	{
		f = LittleFloat (cl.viewangles[i]);
		FS_Write(&f, 4, clc.demofile);
	}
	FS_Write(net_message._data, net_message.cursize, clc.demofile);
	FS_Flush(clc.demofile);
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
int CL_GetMessage (void)
{
	int		r, i;
	float	f;
	
	if	(clc.demoplaying)
	{
	// decide if it is time to grab the next message		
		if (clc.qh_signon == SIGNONS)	// allways grab until fully connected
		{
			if (cls.timedemo)
			{
				if (host_framecount == cls.td_lastframe)
					return 0;		// allready read this frame's message
				cls.td_lastframe = host_framecount;
			// if this is the second frame, grab the real td_starttime
			// so the bogus time on the first frame doesn't count
				if (host_framecount == cls.td_startframe + 1)
					cls.td_starttime = realtime;
			}
			else if ( /* cl.time > 0 && */ cl.serverTimeFloat <= cl.qh_mtime[0])
			{
					return 0;		// don't need another message yet
			}
		}
		
	// get the next message
		FS_Read(&net_message.cursize, 4, clc.demofile);
		VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);
		for (i=0 ; i<3 ; i++)
		{
			r = FS_Read(&f, 4, clc.demofile);
			cl.mviewangles[0][i] = LittleFloat (f);
		}
		
		net_message.cursize = LittleLong (net_message.cursize);
		if (net_message.cursize > MAX_MSGLEN_Q1)
			Sys_Error ("Demo message > MAX_MSGLEN_Q1");
		r = FS_Read(net_message._data, net_message.cursize, clc.demofile);
		if (r != net_message.cursize)
		{
			CL_StopPlayback ();
			return 0;
		}
	
		return 1;
	}

	while (1)
	{
		r = NET_GetMessage (cls.netcon, &clc.netchan);
		
		if (r != 1 && r != 2)
			return r;
	
	// discard nop keepalive message
		if (net_message.cursize == 1 && net_message._data[0] == q1svc_nop)
			Con_Printf ("<-- server to client keepalive\n");
		else
			break;
	}

	if (clc.demorecording)
		CL_WriteDemoMessage ();
	
	return r;
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f (void)
{
	if (cmd_source != src_command)
		return;

	if (!clc.demorecording)
	{
		Con_Printf ("Not recording a demo.\n");
		return;
	}

// write a disconnect message to the demo file
	net_message.Clear();
	net_message.WriteByte(q1svc_disconnect);
	CL_WriteDemoMessage ();

// finish up
	FS_FCloseFile (clc.demofile);
	clc.demofile = 0;
	clc.demorecording = false;
	Con_Printf ("Completed demo\n");
}

/*
====================
CL_Record_f

record <demoname> <map> [cd track]
====================
*/
void CL_Record_f (void)
{
	int		c;
	char	name[MAX_OSPATH];
	int		track;

	if (cmd_source != src_command)
		return;

	c = Cmd_Argc();
	if (c != 2 && c != 3 && c != 4)
	{
		Con_Printf ("record <demoname> [<map> [cd track]]\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf ("Relative pathnames are not allowed.\n");
		return;
	}

	if (c == 2 && cls.state == CA_CONNECTED)
	{
		Con_Printf("Can not record - already connected to server\nClient demo recording must be started before connecting\n");
		return;
	}

// write the forced cd track number, or -1
	if (c == 4)
	{
		track = String::Atoi(Cmd_Argv(3));
		Con_Printf ("Forcing CD track to %i\n", cls.forcetrack);
	}
	else
		track = -1;	

	String::Cpy(name, Cmd_Argv(1));
	
//
// start the map up
//
	if (c > 2)
		Cmd_ExecuteString ( va("map %s", Cmd_Argv(2)), src_command);
	
//
// open the demo file
//
	String::DefaultExtension(name, sizeof(name), ".dem");

	Con_Printf ("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}

	cls.forcetrack = track;
	FS_Printf(clc.demofile, "%i\n", cls.forcetrack);
	
	clc.demorecording = true;
}


/*
====================
CL_PlayDemo_f

play [demoname]
====================
*/
void CL_PlayDemo_f (void)
{
	char	name[256];
	char c;
	qboolean neg = false;

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("play <demoname> : plays a demo\n");
		return;
	}

//
// disconnect from server
//
	CL_Disconnect ();
	
//
// open the demo file
//
	String::Cpy(name, Cmd_Argv(1));
	String::DefaultExtension(name, sizeof(name), ".dem");

	Con_Printf ("Playing demo from %s.\n", name);
	FS_FOpenFileRead (name, &clc.demofile, true);
	if (!clc.demofile)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		cls.demonum = -1;		// stop demo loop
		return;
	}

	clc.demoplaying = true;
	clc.netchan.message.InitOOB(clc.netchan.messageBuffer, 1024);
	cls.state = CA_CONNECTED;
	cls.forcetrack = 0;

	FS_Read(&c, 1, clc.demofile);
	while (c != '\n')
	{
		if (c == '-')
			neg = true;
		else
			cls.forcetrack = cls.forcetrack * 10 + (c - '0');
		if (FS_Read(&c, 1, clc.demofile) != 1)
			break;
	}

	if (neg)
		cls.forcetrack = -cls.forcetrack;
// ZOID, fscanf is evil
//	fscanf (clc.demofile, "%i\n", &cls.forcetrack);
}

/*
====================
CL_FinishTimeDemo

====================
*/
void CL_FinishTimeDemo (void)
{
	int		frames;
	float	time;
	
	cls.timedemo = false;
	
// the first frame didn't count
	frames = (host_framecount - cls.td_startframe) - 1;
	time = realtime - cls.td_starttime;
	if (!time)
		time = 1;
	Con_Printf ("%i frames %5.1f seconds %5.1f fps\n", frames, time, frames/time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f (void)
{
	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("timedemo <demoname> : gets demo speeds\n");
		return;
	}

	CL_PlayDemo_f ();
	
// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted
	
	cls.timedemo = true;
	cls.td_startframe = host_framecount;
	cls.td_lastframe = -1;		// get a new message this frame
}

