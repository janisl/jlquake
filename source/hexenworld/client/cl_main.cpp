// cl_main.c  -- client main loop

#include "quakedef.h"
#include "winquake.h"
#ifdef _WIN32
#include "winsock.h"
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#endif

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

qboolean	noclip_anglehack;		// remnant from old quake


QCvar*	rcon_password;

QCvar*	rcon_address;

QCvar*	cl_timeout;

QCvar*	cl_shownet;

QCvar*	cl_sbar;
QCvar*	cl_hudswap;

QCvar*	lookspring;
QCvar*	lookstrafe;
QCvar*	sensitivity;
QCvar*	mwheelthreshold;

QCvar*	m_pitch;
QCvar*	m_yaw;
QCvar*	m_forward;
QCvar*	m_side;

QCvar*	entlatency;
QCvar*	cl_predict_players;
QCvar*	cl_predict_players2;
QCvar*	cl_solid_players;

//
// info mirrors
//
QCvar*	password;
QCvar*	spectator;
QCvar*	name;
QCvar*	playerclass;
QCvar*	team;
QCvar*	skin;
QCvar*	topcolor;
QCvar*	bottomcolor;
QCvar*	rate;
QCvar*	noaim;
QCvar*  talksounds;
QCvar*	msg;


client_static_t	cls;
client_state_t	cl;

entity_state_t	cl_baselines[MAX_EDICTS];
efrag_t			cl_efrags[MAX_EFRAGS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];

// refresh list
// this is double buffered so the last frame
// can be scanned for oldorigins of trailing objects
int				cl_numvisedicts, cl_oldnumvisedicts;
entity_t		*cl_visedicts, *cl_oldvisedicts;
entity_t		cl_visedicts_list[2][MAX_VISEDICTS];

double			connect_time = -1;		// for connection retransmits

quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution
qboolean	nomaster;

double		host_frametime;
double		realtime;				// without any filtering or bounding
double		oldrealtime;			// last frame run
int			host_framecount;

int			host_hunklevel;

byte		*host_basepal;
byte		*host_colormap;

netadr_t	master_adr;				// address of the master server

QCvar*	host_speeds;
QCvar*	developer;

int			fps_count;

jmp_buf 	host_abort;

void Master_Connect_f (void);

float	server_version = 0;	// version of server we connected to


class QMainLog : public QLogListener
{
public:
	void Serialise(const char* Text, bool Devel)
	{
		if (Devel)
		{
			Con_DPrintf("%s", Text);
		}
		else
		{
			Con_Printf("%s", Text);
		}
	}
} MainLog;

/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f (void)
{
	if (1 /* key_dest != key_console */ /* && cls.state != ca_dedicated */)
	{
		M_Menu_Quit_f ();
		return;
	}
	CL_Disconnect ();
	Sys_Quit ();
}

/*
=======================
CL_Version_f
======================
*/
void CL_Version_f (void)
{
	Con_Printf ("Version %4.2f\n", VERSION);
	Con_Printf ("Exe: "__TIME__" "__DATE__"\n");
}


/*
=======================
CL_SendConnectPacket

called by CL_Connect_f and CL_CheckResend
======================
*/
void CL_SendConnectPacket (void)
{
	netadr_t	adr;
	char	data[2048];
	double t1, t2;
// JACK: Fixed bug where DNS lookups would cause two connects real fast
//       Now, adds lookup time to the connect time.
//		 Should I add it to realtime instead?!?!

	t1 = Sys_DoubleTime ();

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Con_Printf ("Bad server address\n");
		connect_time = -1;
		return;
	}
	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);
	t2 = Sys_DoubleTime ();

	connect_time = realtime+t2-t1;	// for retransmit requests

	Con_Printf ("Connecting to %s...\n", cls.servername);
	sprintf (data, "%c%c%c%cconnect %d \"%s\"\n",
		255, 255, 255, 255,	com_portals, cls.userinfo);
	NET_SendPacket (QStr::Length(data), data, adr);
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out

=================
*/
void CL_CheckForResend (void)
{
	if (connect_time == -1)
		return;
	if (cls.state != ca_disconnected)
		return;
	if (realtime - connect_time > 5.0)
		CL_SendConnectPacket ();
}


/*
================
CL_Connect_f

================
*/
void CL_Connect_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("usage: connect <server>\n");
		return;	
	}
	
	server = Cmd_Argv (1);

	CL_Disconnect ();

	QStr::NCpy(cls.servername, server, sizeof(cls.servername)-1);
	CL_SendConnectPacket ();
}


/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f (void)
{
	char	message[1024];
	int		i;
	netadr_t	to;

	if (!rcon_password->string)
	{
		Con_Printf ("You must set 'rcon_password' before\n"
					"issuing an rcon command.\n");
		return;
	}

	message[0] = 255;
	message[1] = 255;
	message[2] = 255;
	message[3] = 255;
	message[4] = 0;

	QStr::Cat(message, sizeof(message), "rcon ");

	QStr::Cat(message, sizeof(message), rcon_password->string);
	QStr::Cat(message, sizeof(message), " ");

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		QStr::Cat(message, sizeof(message), Cmd_Argv(i));
		QStr::Cat(message, sizeof(message), " ");
	}

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!QStr::Length(rcon_address->string))
		{
			Con_Printf ("You must either be connected,\n"
						"or set the 'rcon_address' cvar\n"
						"to issue rcon commands\n");

			return;
		}
		NET_StringToAdr (rcon_address->string, &to);
		if (to.port == 0)
		{
			to.port = BigShort (PORT_SERVER);
		}
	}
	
	NET_SendPacket (QStr::Length(message)+1, message
		, to);
}


/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	int			i;

	S_StopAllSounds (true);

	Con_DPrintf ("Clearing memory\n");
	D_FlushCaches ();
	Mod_ClearAll ();
	if (host_hunklevel)	// FIXME: check this...
		Hunk_FreeToLowMark (host_hunklevel);

	CL_ClearTEnts ();
	CL_ClearEffects();

// wipe the entire cl structure
	Com_Memset(&cl, 0, sizeof(cl));

	cls.netchan.message.Clear();

// clear other arrays	
	Com_Memset(cl_efrags, 0, sizeof(cl_efrags));
	Com_Memset(cl_dlights, 0, sizeof(cl_dlights));
	Com_Memset(cl_lightstyle, 0, sizeof(cl_lightstyle));

//
// allocate the efrags and chain together into a free list
//
	cl.free_efrags = cl_efrags;
	for (i=0 ; i<MAX_EFRAGS-1 ; i++)
		cl.free_efrags[i].entnext = &cl.free_efrags[i+1];
	cl.free_efrags[i].entnext = NULL;

	plaquemessage = "";

	SB_InvReset();
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
	byte	final[10];

	connect_time = -1;

#ifdef _WIN32
	SetWindowText (GMainWindow, "HexenWorld: disconnected");
#endif

// stop sounds (especially looping!)
	cl_siege=false;//no more siege display, etc.
	S_StopAllSounds (true);
	
// if running a local server, shut it down
	if (cls.demoplayback)
		CL_StopPlayback ();
	else if (cls.state != ca_disconnected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		final[0] = clc_stringcmd;
		QStr::Cpy((char*)final+1, "drop");
		Netchan_Transmit (&cls.netchan, 6, final);
		Netchan_Transmit (&cls.netchan, 6, final);
		Netchan_Transmit (&cls.netchan, 6, final);

		cls.state = ca_disconnected;

		cls.demoplayback = cls.demorecording = cls.timedemo = false;
	}
	Cam_Reset();

}

void CL_Disconnect_f (void)
{
	CL_Disconnect ();
}

/*
====================
CL_User_f

user <name or userid>

Dump userdata / masterdata for a user
====================
*/
void CL_User_f (void)
{
	int		uid;
	int		i;
	char	packet[1024];

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: user <username / userid>\n");
		return;
	}

	uid = QStr::Atoi(Cmd_Argv(1));

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.players[i].name[0])
			continue;
		if (cl.players[i].userid == uid
		|| !QStr::Cmp(cl.players[i].name, Cmd_Argv(1)) )
		{
			Info_Print (cl.players[i].userinfo);
			return;
		}
	}
	Con_Printf ("User not in server.\n");
}

/*
====================
CL_Users_f

Dump userids for all current players
====================
*/
void CL_Users_f (void)
{
	int		i;
	int		c;

	c = 0;
	Con_Printf ("userid frags name\n");
	Con_Printf ("------ ----- ----\n");
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (cl.players[i].name[0])
		{
			Con_Printf ("%6i %4i %s\n", cl.players[i].userid, cl.players[i].frags, cl.players[i].name);
			c++;
		}
	}

	Con_Printf ("%i total users\n", c);
}

void CL_Color_f (void)
{
	// just for quake compatability...
	int		top, bottom;
	char	num[16];

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"color\" is \"%s %s\"\n",
			Info_ValueForKey (cls.userinfo, "topcolor"),
			Info_ValueForKey (cls.userinfo, "bottomcolor") );
		Con_Printf ("color <0-13> [0-13]\n");
		return;
	}

	if (Cmd_Argc() == 2)
		top = bottom = QStr::Atoi(Cmd_Argv(1));
	else
	{
		top = QStr::Atoi(Cmd_Argv(1));
		bottom = QStr::Atoi(Cmd_Argv(2));
	}
	
	top &= 15;
	if (top > 13)
		top = 13;
	bottom &= 15;
	if (bottom > 13)
		bottom = 13;
	
	sprintf (num, "%i", top);
	Cvar_Set ("topcolor", num);
	sprintf (num, "%i", bottom);
	Cvar_Set ("bottomcolor", num);
}

/*
==================
CL_FullServerinfo_f

Sent by server when serverinfo changes
==================
*/
void CL_FullServerinfo_f (void)
{
	const char *p;
	float v;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("usage: fullserverinfo <complete info string>\n");
		return;
	}

	QStr::Cpy(cl.serverinfo, Cmd_Argv(1));

	if ((p = Info_ValueForKey(cl.serverinfo, "*version")) && *p) {
		v = QStr::Atof(p);
		if (v) 
		{
			if (!server_version)
				Con_Printf("Version %1.2f Server\n", v);
			server_version = v;
			if((int)(server_version*100)>(int)(VERSION*100))
			{
				Con_Printf("The server is running v%4.2f, you have v%4.2f, please go to www.hexenworld.com and update your client to join\n",server_version,VERSION);
				CL_Disconnect_f ();
			}
			if((int)(server_version*100)<(int)(VERSION*100))
			{
				Con_Printf("The server is running an old version (v%4.2f), you have v%4.2f, please ask server admin to update to latest version\n",server_version,VERSION);
				CL_Disconnect_f ();
			}
		}
	}
}

/*
==================
CL_FullInfo_f

Allow clients to change userinfo
==================
*/
void CL_FullInfo_f (void)
{
	char	key[512];
	char	value[512];
	char	*o;
	char	*s;
	int		l;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("fullinfo <complete info string>\n");
		return;
	}

	s = Cmd_Argv(1);
	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (!*s)
		{
			Con_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;

		if (key[0] == '*')
		{
			Con_Printf ("Can't set * keys\n");
			continue;
		}

		Info_SetValueForKey(cls.userinfo, key, value, MAX_INFO_STRING, 64, 64,
			QStr::ICmp(key, "name") != 0, QStr::ICmp(key, "team") == 0);
	}
}

/*
==================
CL_SetInfo_f

Allow clients to change userinfo
==================
*/
void CL_SetInfo_f (void)
{
	char	packet[1024];

	if (Cmd_Argc() == 1)
	{
		Info_Print (cls.userinfo);
		return;
	}
	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: setinfo [ <key> <value> ]\n");
		return;
	}
	if (!QStr::ICmp(Cmd_Argv(1), "pmodel") || !QStr::Cmp(Cmd_Argv(1), "emodel"))
		return;

	if (Cmd_Argv(1)[0] == '*')
	{
		Con_Printf ("Can't set * keys\n");
		return;
	}

	Info_SetValueForKey(cls.userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING, 64, 64,
		QStr::ICmp(Cmd_Argv(1), "name") != 0, QStr::ICmp(Cmd_Argv(1), "team") == 0);
	if (cls.state >= ca_connected)
		Cmd_ForwardToServer ();
}

/*
====================
CL_Packet_f

packet <destination> <contents>

Contents allows \n escape character
====================
*/
void CL_Packet_f (void)
{
	char	send[2048];
	int		i, l;
	char	*in, *out;
	netadr_t	adr;

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("packet <destination> <contents>\n");
		return;
	}

	if (!NET_StringToAdr (Cmd_Argv(1), &adr))
	{
		Con_Printf ("Bad address\n");
		return;
	}

	in = Cmd_Argv(2);
	out = send+4;
	send[0] = send[1] = send[2] = send[3] = 0xff;

	l = QStr::Length(in);
	for (i=0 ; i<l ; i++)
	{
		if (in[i] == '\\' && in[i+1] == 'n')
		{
			*out++ = '\n';
			i++;
		}
		else
			*out++ = in[i];
	}
	*out = 0;

	NET_SendPacket (out-send, send, adr);
}


/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
//			Con_Printf ("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	sprintf (str,"playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	cls.demonum++;
}


/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f (void)
{
	S_StopAllSounds (true);
	cl.intermission = 0;
	cls.state = ca_connected;	// not active anymore, but not disconnected
	Con_Printf ("\nChanging map...\n");
}


/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f (void)
{
	S_StopAllSounds (true);

	if (cls.state == ca_connected) {
		Con_Printf ("reconnecting...\n");
		cls.netchan.message.WriteChar(clc_stringcmd);
		cls.netchan.message.WriteString2("new");
		return;
	}

	if (!*cls.servername) {
		Con_Printf("No server to reconnect to...\n");
		return;
	}

	CL_Disconnect();
	CL_SendConnectPacket ();
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket (void)
{
	char	*s;
	int		c;

    net_message.BeginReadingOOB();
    net_message.ReadLong();        // skip the -1

	c = net_message.ReadByte ();
	if (!cls.demoplayback)
		Con_Printf ("%s:\n", NET_AdrToString (net_from));
	Con_DPrintf ("%s", net_message._data + 5);
	if (c == S2C_CONNECTION)
	{
		if (cls.state == ca_connected)
		{
			if (!cls.demoplayback)
				Con_Printf ("Dup connect received.  Ignored.\n");
			return;
		}
		Netchan_Setup (&cls.netchan, net_from);
		cls.netchan.message.WriteChar(clc_stringcmd);
		cls.netchan.message.WriteString2("new");	
		cls.state = ca_connected;
		Con_Printf ("Connected.\n");
		return;
	}
	// remote command from gui front end
	if (c == A2C_CLIENT_COMMAND)
	{
		if ((*(unsigned *)net_from.ip != *(unsigned *)net_local_adr.ip
			&& *(unsigned *)net_from.ip != htonl(INADDR_LOOPBACK)) )
		{
			Con_Printf ("Command packet from remote host.  Ignored.\n");
			return;
		}
#ifdef _WIN32
		ShowWindow (GMainWindow, SW_RESTORE);
		SetForegroundWindow (GMainWindow);
#endif
		s = const_cast<char*>(net_message.ReadString2());
		Cbuf_AddText (s);
		return;
	}
	// print command from somewhere
	if (c == A2C_PRINT)
	{
		s = const_cast<char*>(net_message.ReadString2());
		Con_Printf (s);
		return;
	}

	// ping from somewhere
	if (c == A2A_PING)
	{
		char	data[6];

		data[0] = 0xff;
		data[1] = 0xff;
		data[2] = 0xff;
		data[3] = 0xff;
		data[4] = A2A_ACK;
		data[5] = 0;
		
		NET_SendPacket (6, &data, net_from);
		return;
	}

	if (c == svc_disconnect) {
		Host_EndGame ("Server disconnected\n");
		return;
	}

	Con_Printf ("Unknown command:\n%c\n", c);
}


/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets (void)
{
//	while (NET_GetPacket ())
	while (CL_GetMessage())
	{
		//
		// remote command packet
		//
		if (*(int *)net_message._data == -1)
		{
			CL_ConnectionlessPacket ();
			continue;
		}

		if (net_message.cursize < 8)
		{
			Con_Printf ("%s: Runt packet\n",NET_AdrToString(net_from));
			continue;
		}

		//
		// packet from server
		//
		if (!cls.demoplayback && 
			!NET_CompareAdr (net_from, cls.netchan.remote_address))
		{
			Con_Printf ("%s:sequenced packet without connection\n"
				,NET_AdrToString(net_from));
			continue;
		}
		if (!Netchan_Process(&cls.netchan))
			continue;		// wasn't accepted for some reason
		CL_ParseServerMessage ();

//		if (cls.demoplayback && cls.state >= ca_active && !CL_DemoBehind())
//			return;
	}

	//
	// check timeout
	//
	if (cls.state >= ca_connected
	 && realtime - cls.netchan.last_received > cl_timeout->value)
	{
		Con_Printf ("\nServer connection timed out.\n");
		CL_Disconnect ();
		return;
	}
	
}

//=============================================================================

/*
=====================
CL_Download_f
=====================
*/
void CL_Download_f (void)
{
	if (cls.state == ca_disconnected)
	{
		Con_Printf ("Must be connected.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: download <datafile>\n");
		return;
	}

	QStr::Cpy(cls.downloadname, Cmd_Argv(1));
	cls.download = FS_FOpenFileWrite (cls.downloadname);
	cls.downloadtype = dl_single;

	cls.netchan.message.WriteByte(clc_stringcmd);
	cls.netchan.message.WriteString2(va("download %s\n",Cmd_Argv(1)));
}

#ifdef _WINDOWS
#include <windows.h>
/*
=================
CL_Minimize_f
=================
*/
void CL_Windows_f (void) {
//	if (modestate == MS_WINDOWED)
//		ShowWindow(GMainWindow, SW_MINIMIZE);
//	else
		SendMessage(GMainWindow, WM_SYSKEYUP, VK_TAB, 1 | (0x0F << 16) | (1<<29));
}
#endif

void CL_Sensitivity_save_f (void)
{
	static float save_sensitivity = 3;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("sensitivity_save <save/restore>\n");
		return;
	}

	if (QStr::ICmp(Cmd_Argv(1),"save") == 0)
	{
		save_sensitivity = sensitivity->value;
	}
	else if (QStr::ICmp(Cmd_Argv(1),"restore") == 0)
	{
		Cvar_SetValue ("sensitivity", save_sensitivity);
	}
}

/*
=================
CL_Init
=================
*/
void Host_SaveConfig_f (void);
void CL_Init (void)
{
	extern	QCvar*		baseskin;
	extern	QCvar*		noskins;

	cls.state = ca_disconnected;

	Info_SetValueForKey(cls.userinfo, "name", "unnamed", MAX_INFO_STRING, 64, 64, false, false);
	Info_SetValueForKey(cls.userinfo, "playerclass", "0", MAX_INFO_STRING, 64, 64, true, false);
	Info_SetValueForKey(cls.userinfo, "topcolor", "0", MAX_INFO_STRING, 64, 64, true, false);
	Info_SetValueForKey(cls.userinfo, "bottomcolor", "0", MAX_INFO_STRING, 64, 64, true, false);
	Info_SetValueForKey(cls.userinfo, "rate", "2500", MAX_INFO_STRING, 64, 64, true, false);
	Info_SetValueForKey(cls.userinfo, "msg", "1", MAX_INFO_STRING, 64, 64, true, false);

	CL_InitInput ();
	CL_InitTEnts ();
	CL_InitPrediction ();
	CL_InitEffects ();
	CL_InitCam ();
	Pmove_Init ();
	
//
// register our commands
//
	Cmd_AddCommand ("saveconfig", Host_SaveConfig_f);

	host_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times
	developer = Cvar_Get("developer", "0", 0);

	cl_upspeed = Cvar_Get("cl_upspeed", "200", 0);
	cl_forwardspeed = Cvar_Get("cl_forwardspeed", "200", CVAR_ARCHIVE);
	cl_backspeed = Cvar_Get("cl_backspeed", "200", CVAR_ARCHIVE);
	cl_sidespeed = Cvar_Get("cl_sidespeed","225", 0);
	cl_movespeedkey = Cvar_Get("cl_movespeedkey", "2.0", 0);
	cl_yawspeed = Cvar_Get("cl_yawspeed", "140", 0);
	cl_pitchspeed = Cvar_Get("cl_pitchspeed", "150", 0);
	cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0);
	cl_shownet = Cvar_Get("cl_shownet", "0", 0);	// can be 0, 1, or 2
	cl_hudswap	= Cvar_Get("cl_hudswap", "0", CVAR_ARCHIVE);
	cl_timeout = Cvar_Get("cl_timeout", "60", 0);
	lookspring = Cvar_Get("lookspring", "0", CVAR_ARCHIVE);
	lookstrafe = Cvar_Get("lookstrafe", "0", CVAR_ARCHIVE);
	sensitivity = Cvar_Get("sensitivity", "3", CVAR_ARCHIVE);
	mwheelthreshold = Cvar_Get("mwheelthreshold", "120", CVAR_ARCHIVE);

	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw = Cvar_Get("m_yaw", "0.022", 0);
	m_forward = Cvar_Get("m_forward", "1", 0);
	m_side = Cvar_Get("m_side", "0.8", 0);

	rcon_password = Cvar_Get("rcon_password", "", 0);
	rcon_address = Cvar_Get("rcon_address", "", 0);

	entlatency = Cvar_Get("entlatency", "20", 0);
	cl_predict_players = Cvar_Get("cl_predict_players", "1", 0);
	cl_predict_players2 = Cvar_Get("cl_predict_players2", "1", 0);
	cl_solid_players = Cvar_Get("cl_solid_players", "1", 0);

	baseskin = Cvar_Get("baseskin", "base", 0);
	noskins = Cvar_Get("noskins", "1", 0);

	//
	// info mirrors
	//
	name = Cvar_Get("name","unnamed", CVAR_ARCHIVE | CVAR_USERINFO);
	playerclass = Cvar_Get("playerclass", "0", CVAR_ARCHIVE | CVAR_USERINFO);
	password = Cvar_Get("password", "", CVAR_USERINFO);
	spectator = Cvar_Get("spectator", "", CVAR_USERINFO);
	skin = Cvar_Get("skin","", CVAR_ARCHIVE | CVAR_USERINFO);
	team = Cvar_Get("team","", CVAR_ARCHIVE | CVAR_USERINFO);
	topcolor = Cvar_Get("topcolor","0", CVAR_ARCHIVE | CVAR_USERINFO);
	bottomcolor = Cvar_Get("bottomcolor","0", CVAR_ARCHIVE | CVAR_USERINFO);
	rate = Cvar_Get("rate","2500", CVAR_ARCHIVE | CVAR_USERINFO);
	msg = Cvar_Get("msg","1", CVAR_ARCHIVE | CVAR_USERINFO);
	noaim = Cvar_Get("noaim","0", CVAR_ARCHIVE | CVAR_USERINFO);
	talksounds = Cvar_Get("talksounds", "1", CVAR_ARCHIVE);

	Cmd_AddCommand ("version", CL_Version_f);

	Cmd_AddCommand ("changing", CL_Changing_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("rerecord", CL_ReRecord_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);

	Cmd_AddCommand ("skins", Skin_Skins_f);
	Cmd_AddCommand ("allskins", Skin_AllSkins_f);

	Cmd_AddCommand ("quit", CL_Quit_f);

	Cmd_AddCommand ("connect", CL_Connect_f);
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);

	Cmd_AddCommand ("rcon", CL_Rcon_f);
	Cmd_AddCommand ("packet", CL_Packet_f);
	Cmd_AddCommand ("user", CL_User_f);
	Cmd_AddCommand ("users", CL_Users_f);

	Cmd_AddCommand ("setinfo", CL_SetInfo_f);
	Cmd_AddCommand ("fullinfo", CL_FullInfo_f);
	Cmd_AddCommand ("fullserverinfo", CL_FullServerinfo_f);

	Cmd_AddCommand ("color", CL_Color_f);
	Cmd_AddCommand ("download", CL_Download_f);

	Cmd_AddCommand ("sensitivity_save", CL_Sensitivity_save_f);

//
// forward to server commands
//
	Cmd_AddCommand ("kill", NULL);
	Cmd_AddCommand ("say", NULL);
	Cmd_AddCommand ("say_team", NULL);
	Cmd_AddCommand ("serverinfo", NULL);

//
//  Windows commands
//
#ifdef _WINDOWS
	Cmd_AddCommand ("windows", CL_Windows_f);
#endif
}


/*
================
Host_EndGame

Call this to drop to a console without exiting the qwcl
================
*/
void Host_EndGame (char *message, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,message);
	vsprintf (string,message,argptr);
	va_end (argptr);
	Con_Printf ("\n===========================\n");
	Con_Printf ("Host_EndGame: %s\n",string);
	Con_Printf ("===========================\n\n");
	
	CL_Disconnect ();

	longjmp (host_abort, 1);
}

/*
================
Host_Error

This shuts down the client and exits qwcl
================
*/
void Host_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = false;
	
	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;
	
	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);
	Con_Printf ("Host_Error: %s\n",string);
	
	CL_Disconnect ();
	cls.demonum = -1;

	inerror = false;

// FIXME
	Sys_Error ("Host_Error: %s\n",string);
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration (char *fname)
{
	if (host_initialized)
	{
		fileHandle_t f = FS_FOpenFileWrite(fname);
		if (!f)
		{
			Con_Printf ("Couldn't write %s.\n",fname);
			return;
		}
		
		Key_WriteBindings (f);
		Cvar_WriteVariables (f);

		if (in_mlook.state & 1)		//if mlook was down, keep it that way
			FS_Printf(f, "+mlook\n");

		FS_FCloseFile(f);
	}
}

void Host_SaveConfig_f (void)
{

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("saveconfig <savename> : save a config file\n");
			return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf ("Relative pathnames are not allowed.\n");
			return;
	}

	Host_WriteConfiguration (Cmd_Argv(1));
}

//============================================================================

/*
==================
Host_Frame

Runs all active servers
==================
*/
int		nopacketcount;
void Host_Frame (float time)
{
	try
	{
	static double		time1 = 0;
	static double		time2 = 0;
	static double		time3 = 0;
	int			pass1, pass2, pass3;
	float fps;
	if (setjmp (host_abort) )
		return;			// something bad happened, or the server disconnected

	// decide the simulation time
	realtime += time;
	if (oldrealtime > realtime)
		oldrealtime = 0;

#define max(a, b)	((a) > (b) ? (a) : (b))
#define min(a, b)	((a) < (b) ? (a) : (b))
	fps = max(30.0, min(rate->value/80.0, 72.0));

	if (!cls.timedemo && realtime - oldrealtime < 1.0/fps)
		return;			// framerate is too high
	host_frametime = realtime - oldrealtime;
	oldrealtime = realtime;
	if (host_frametime > 0.2)
		host_frametime = 0.2;
		
	// get new key events
	Sys_SendKeyEvents ();

	// allow mice or other external controllers to add commands
	IN_Commands ();

	// process console commands
	Cbuf_Execute ();

	// fetch results from server
	CL_ReadPackets ();

	// send intentions now
	// resend a connection request if necessary
	if (cls.state == ca_disconnected) 
	{
		CL_CheckForResend ();
	}
	else
		CL_SendCmd ();

	// Set up prediction for other players
	CL_SetUpPlayerPrediction(false);

	// do client side motion prediction
	CL_PredictMove ();

	// Set up prediction for other players
	CL_SetUpPlayerPrediction(true);

	// build a refresh entity list
	CL_EmitEntities ();

	// update video
	if (host_speeds->value)
		time1 = Sys_DoubleTime ();

	SCR_UpdateScreen ();

	if (host_speeds->value)
		time2 = Sys_DoubleTime ();
		
	// update audio
	if (cls.state == ca_active)
	{
		S_Update (r_origin, vpn, vright, vup);
		CL_DecayLights ();
	}
	else
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);
	
	CDAudio_Update();

	if (host_speeds->value)
	{
		pass1 = (time1 - time3)*1000;
		time3 = Sys_DoubleTime ();
		pass2 = (time2 - time1)*1000;
		pass3 = (time3 - time2)*1000;
		Con_Printf ("%3i tot %3i server %3i gfx %3i snd\n",
					pass1+pass2+pass3, pass1, pass2, pass3);
	}

	host_framecount++;
	fps_count++;
	}
	catch (QException& e)
	{
		Sys_Error("%s", e.What());
	}
}


//============================================================================

/*
====================
Host_Init
====================
*/
void Host_Init (quakeparms_t *parms)
{
	try
	{
	GGameType = GAME_Hexen2 | GAME_HexenWorld;
	Sys_SetHomePathSuffix("vhexen2");
	GLog.AddListener(&MainLog);

	COM_InitArgv2(parms->argc, parms->argv);
//	COM_AddParm ("-game");
//	COM_AddParm ("hw");

	if (COM_CheckParm ("-minmemory"))
		parms->memsize = MINIMUM_MEMORY;

	host_parms = *parms;

	if (parms->memsize < MINIMUM_MEMORY)
		Sys_Error ("Only %4.1f megs of memory reported, can't execute game", parms->memsize / (float)0x100000);

	Memory_Init (parms->membase, parms->memsize);
	Cbuf_Init ();
	Cmd_Init ();
	Cvar_Init();
	V_Init ();

	COM_Init (parms->basedir);
	
	NET_Init (PORT_CLIENT);
	Netchan_Init ();

	W_LoadWadFile ("gfx.wad");
	Key_Init ();
	Con_Init ();	
	M_Init ();	
	Mod_Init ();
	CM_Init ();

	
//	Con_Printf ("Exe: "__TIME__" "__DATE__"\n");
	Con_Printf ("%4.1f megs RAM used.\n",parms->memsize/ (1024*1024.0));
	
	R_InitTextures ();
 
	host_basepal = (byte *)COM_LoadHunkFile ("gfx/palette.lmp");
	if (!host_basepal)
		Sys_Error ("Couldn't load gfx/palette.lmp");
	host_colormap = (byte *)COM_LoadHunkFile ("gfx/colormap.lmp");
	if (!host_colormap)
		Sys_Error ("Couldn't load gfx/colormap.lmp");
#ifdef __linux__
	IN_Init ();
	CDAudio_Init ();
	VID_Init (host_basepal);
	Draw_Init ();
	SCR_Init ();
	R_Init ();

//	S_Init ();		// S_Init is now done as part of VID. Sigh.
	
	cls.state = ca_disconnected;
	Sbar_Init ();
	CL_Init ();
#else
	VID_Init (host_basepal);
	Draw_Init ();
	SCR_Init ();
	R_Init ();
	S_Init();

	cls.state = ca_disconnected;
	CDAudio_Init ();
	MIDI_Init ();
	Sbar_Init ();
	CL_Init ();
	IN_Init ();
#endif

	Cbuf_InsertText ("exec hexen.rc\n");
	Cbuf_AddText ("cl_warncmd 1\n");

	Hunk_AllocName (0, "-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark ();

	host_initialized = true;
	
	Con_Printf ("������� HexenWorld Initialized �������\n");	
	}
	catch (QException& e)
	{
		Sys_Error("%s", e.What());
	}
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown(void)
{
	static qboolean isdown = false;
	
	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

	Host_WriteConfiguration ("config.cfg"); 
		
	CDAudio_Shutdown ();
	MIDI_Cleanup ();
	NET_Shutdown ();
	S_Shutdown();
	IN_Shutdown ();
	if (host_basepal)
		VID_Shutdown();
}

