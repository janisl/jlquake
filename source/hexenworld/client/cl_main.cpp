// cl_main.c  -- client main loop

#include "quakedef.h"
#ifdef _WIN32
#include "../../client/windows_shared.h"
#endif
#include "../../server/public.h"
#include "../../common/hexen2strings.h"
#include "../../client/game/quake_hexen2/demo.h"
#include "../../client/game/quake_hexen2/connection.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

Cvar* rcon_password;

Cvar* rcon_address;

Cvar* cl_timeout;

Cvar* entlatency;

//
// info mirrors
//
Cvar* password;
Cvar* team;
Cvar* skin;
Cvar* rate;
Cvar* noaim;
Cvar* msg;

quakeparms_t host_parms;

qboolean nomaster;

double host_frametime;
double realtime;					// without any filtering or bounding
double oldrealtime;					// last frame run

jmp_buf host_abort;

void Master_Connect_f(void);

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void ServerDisconnected(const char* format, ...) id_attribute((format(printf, 2, 3)));
};

static idCommonLocal commonLocal;
idCommon* common = &commonLocal;

void idCommonLocal::Printf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Con_Printf("%s", string);
}

void idCommonLocal::DPrintf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Con_DPrintf("%s", string);
}

void idCommonLocal::Error(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw DropException(string);
}

void idCommonLocal::FatalError(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw Exception(string);
}

void idCommonLocal::EndGame(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw EndGameException(string);
}

void idCommonLocal::ServerDisconnected(const char* format, ...)
{
}

/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f(void)
{
	if (1 /* !(in_keyCatchers & KEYCATCH_CONSOLE) */ /* && cls.state != ca_dedicated */)
	{
		MQH_Menu_Quit_f();
		return;
	}
	CL_Disconnect(true);
	Sys_Quit();
}

/*
=======================
CL_Version_f
======================
*/
void CL_Version_f(void)
{
	common->Printf("Version %4.2f\n", VERSION);
	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
}

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f(void)
{
	char message[1024];
	int i;
	netadr_t to;

	if (!rcon_password->string)
	{
		common->Printf("You must set 'rcon_password' before\n"
				   "issuing an rcon command.\n");
		return;
	}

	message[0] = 255;
	message[1] = 255;
	message[2] = 255;
	message[3] = 255;
	message[4] = 0;

	String::Cat(message, sizeof(message), "rcon ");

	String::Cat(message, sizeof(message), rcon_password->string);
	String::Cat(message, sizeof(message), " ");

	for (i = 1; i < Cmd_Argc(); i++)
	{
		String::Cat(message, sizeof(message), Cmd_Argv(i));
		String::Cat(message, sizeof(message), " ");
	}

	if (cls.state == CA_CONNECTED || cls.state == CA_LOADING || cls.state == CA_ACTIVE)
	{
		to = clc.netchan.remoteAddress;
	}
	else
	{
		if (!String::Length(rcon_address->string))
		{
			common->Printf("You must either be connected,\n"
					   "or set the 'rcon_address' cvar\n"
					   "to issue rcon commands\n");

			return;
		}
		SOCK_StringToAdr(rcon_address->string, &to, HWPORT_SERVER);
	}

	NET_SendPacket(NS_CLIENT, String::Length(message) + 1, message, to);
}


void CL_Disconnect_f(void)
{
	CL_Disconnect(true);
}

/*
====================
CL_User_f

user <name or userid>

Dump userdata / masterdata for a user
====================
*/
void CL_User_f(void)
{
	int uid;
	int i;

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: user <username / userid>\n");
		return;
	}

	uid = String::Atoi(Cmd_Argv(1));

	for (i = 0; i < MAX_CLIENTS_QHW; i++)
	{
		if (!cl.h2_players[i].name[0])
		{
			continue;
		}
		if (cl.h2_players[i].userid == uid ||
			!String::Cmp(cl.h2_players[i].name, Cmd_Argv(1)))
		{
			Info_Print(cl.h2_players[i].userinfo);
			return;
		}
	}
	common->Printf("User not in server.\n");
}

/*
====================
CL_Users_f

Dump userids for all current players
====================
*/
void CL_Users_f(void)
{
	int i;
	int c;

	c = 0;
	common->Printf("userid frags name\n");
	common->Printf("------ ----- ----\n");
	for (i = 0; i < MAX_CLIENTS_QHW; i++)
	{
		if (cl.h2_players[i].name[0])
		{
			common->Printf("%6i %4i %s\n", cl.h2_players[i].userid, cl.h2_players[i].frags, cl.h2_players[i].name);
			c++;
		}
	}

	common->Printf("%i total users\n", c);
}

void CL_Color_f(void)
{
	// just for quake compatability...
	int top, bottom;
	char num[16];

	if (Cmd_Argc() == 1)
	{
		common->Printf("\"color\" is \"%s %s\"\n",
			Info_ValueForKey(cls.qh_userinfo, "topcolor"),
			Info_ValueForKey(cls.qh_userinfo, "bottomcolor"));
		common->Printf("color <0-13> [0-13]\n");
		return;
	}

	if (Cmd_Argc() == 2)
	{
		top = bottom = String::Atoi(Cmd_Argv(1));
	}
	else
	{
		top = String::Atoi(Cmd_Argv(1));
		bottom = String::Atoi(Cmd_Argv(2));
	}

	top &= 15;
	if (top > 13)
	{
		top = 13;
	}
	bottom &= 15;
	if (bottom > 13)
	{
		bottom = 13;
	}

	sprintf(num, "%i", top);
	Cvar_Set("topcolor", num);
	sprintf(num, "%i", bottom);
	Cvar_Set("bottomcolor", num);
}

/*
==================
CL_FullServerinfo_f

Sent by server when serverinfo changes
==================
*/
void CL_FullServerinfo_f(void)
{
	const char* p;
	float v;

	if (Cmd_Argc() != 2)
	{
		common->Printf("usage: fullserverinfo <complete info string>\n");
		return;
	}

	String::Cpy(cl.qh_serverinfo, Cmd_Argv(1));

	if ((p = Info_ValueForKey(cl.qh_serverinfo, "*version")) && *p)
	{
		v = String::Atof(p);
		if (v)
		{
			if (!clqh_server_version)
			{
				common->Printf("Version %1.2f Server\n", v);
			}
			clqh_server_version = v;
		}
	}
}

/*
==================
CL_FullInfo_f

Allow clients to change userinfo
==================
*/
void CL_FullInfo_f(void)
{
	char key[512];
	char value[512];
	char* o;
	char* s;

	if (Cmd_Argc() != 2)
	{
		common->Printf("fullinfo <complete info string>\n");
		return;
	}

	s = Cmd_Argv(1);
	if (*s == '\\')
	{
		s++;
	}
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (!*s)
		{
			common->Printf("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
		{
			s++;
		}

		if (key[0] == '*')
		{
			common->Printf("Can't set * keys\n");
			continue;
		}

		Info_SetValueForKey(cls.qh_userinfo, key, value, MAX_INFO_STRING_QW, 64, 64,
			String::ICmp(key, "name") != 0, String::ICmp(key, "team") == 0);
	}
}

/*
==================
CL_SetInfo_f

Allow clients to change userinfo
==================
*/
void CL_SetInfo_f(void)
{
	if (Cmd_Argc() == 1)
	{
		Info_Print(cls.qh_userinfo);
		return;
	}
	if (Cmd_Argc() != 3)
	{
		common->Printf("usage: setinfo [ <key> <value> ]\n");
		return;
	}
	if (!String::ICmp(Cmd_Argv(1), "pmodel") || !String::Cmp(Cmd_Argv(1), "emodel"))
	{
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		common->Printf("Can't set * keys\n");
		return;
	}

	Info_SetValueForKey(cls.qh_userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING_QW, 64, 64,
		String::ICmp(Cmd_Argv(1), "name") != 0, String::ICmp(Cmd_Argv(1), "team") == 0);
	if (cls.state == CA_CONNECTED || cls.state == CA_LOADING || cls.state == CA_ACTIVE)
	{
		CL_ForwardKnownCommandToServer();
	}
}

/*
====================
CL_Packet_f

packet <destination> <contents>

Contents allows \n escape character
====================
*/
void CL_Packet_f(void)
{
	char send[2048];
	int i, l;
	char* in, * out;
	netadr_t adr;

	if (Cmd_Argc() != 3)
	{
		common->Printf("packet <destination> <contents>\n");
		return;
	}

	if (!SOCK_StringToAdr(Cmd_Argv(1), &adr, 0))
	{
		common->Printf("Bad address\n");
		return;
	}

	in = Cmd_Argv(2);
	out = send + 4;
	send[0] = send[1] = send[2] = send[3] = 0xff;

	l = String::Length(in);
	for (i = 0; i < l; i++)
	{
		if (in[i] == '\\' && in[i + 1] == 'n')
		{
			*out++ = '\n';
			i++;
		}
		else
		{
			*out++ = in[i];
		}
	}
	*out = 0;

	NET_SendPacket(NS_CLIENT, out - send, send, adr);
}


/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo(void)
{
	char str[1024];

	if (cls.qh_demonum == -1)
	{
		return;		// don't play demos

	}
	if (!cls.qh_demos[cls.qh_demonum][0] || cls.qh_demonum == MAX_DEMOS)
	{
		cls.qh_demonum = 0;
		if (!cls.qh_demos[cls.qh_demonum][0])
		{
//			common->Printf ("No demos listed with startdemos\n");
			cls.qh_demonum = -1;
			return;
		}
	}

	sprintf(str,"playdemo %s\n", cls.qh_demos[cls.qh_demonum]);
	Cbuf_InsertText(str);
	cls.qh_demonum++;
}


/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f(void)
{
	S_StopAllSounds();
	cl.qh_intermission = 0;
	cls.state = CA_CONNECTED;	// not active anymore, but not disconnected
	common->Printf("\nChanging map...\n");
}

/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets(void)
{
	netadr_t net_from;
	QMsg net_message;
	static byte net_message_buffer[MAX_MSGLEN_HW + 9];	// one more than msg + header
	net_message.InitOOB(net_message_buffer, sizeof(net_message_buffer));

	while (CLQHW_GetMessage(net_message, net_from))
	{
		//
		// remote command packet
		//
		if (*(int*)net_message._data == -1)
		{
			CLQHW_ConnectionlessPacket(net_message, net_from);
			continue;
		}

		if (net_message.cursize < 8)
		{
			common->Printf("%s: Runt packet\n",SOCK_AdrToString(net_from));
			continue;
		}

		//
		// packet from server
		//
		if (!clc.demoplaying &&
			!SOCK_CompareAdr(net_from, clc.netchan.remoteAddress))
		{
			common->Printf("%s:sequenced packet without connection\n",
				SOCK_AdrToString(net_from));
			continue;
		}
		if (!Netchan_Process(&clc.netchan, &net_message))
		{
			continue;		// wasn't accepted for some reason
		}
		clc.netchan.lastReceived = realtime * 1000;
		CLHW_ParseServerMessage(net_message);

//		if (cls.demoplayback && cls.state >= ca_active && !CL_DemoBehind())
//			return;
	}

	//
	// check timeout
	//
	if ((cls.state == CA_CONNECTED || cls.state == CA_LOADING || cls.state == CA_ACTIVE) &&
		realtime - clc.netchan.lastReceived / 1000.0 > cl_timeout->value)
	{
		common->Printf("\nServer connection timed out.\n");
		CL_Disconnect(true);
		return;
	}

}

//=============================================================================

/*
=====================
CL_Download_f
=====================
*/
void CL_Download_f(void)
{
	if (cls.state == CA_DISCONNECTED)
	{
		common->Printf("Must be connected.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: download <datafile>\n");
		return;
	}

	String::Cpy(clc.downloadName, Cmd_Argv(1));
	clc.download = FS_FOpenFileWrite(clc.downloadName);
	clc.downloadType = dl_single;

	CL_AddReliableCommand(va("download %s\n",Cmd_Argv(1)));
}

void CL_Sensitivity_save_f(void)
{
	static float save_sensitivity = 3;

	if (Cmd_Argc() != 2)
	{
		common->Printf("sensitivity_save <save/restore>\n");
		return;
	}

	if (String::ICmp(Cmd_Argv(1),"save") == 0)
	{
		save_sensitivity = cl_sensitivity->value;
	}
	else if (String::ICmp(Cmd_Argv(1),"restore") == 0)
	{
		Cvar_SetValue("sensitivity", save_sensitivity);
	}
}

/*
==========
Skin_Skins_f

HexenWorld doesn't use custom skins but we need this because it's
used during connection to the server.
==========
*/
static void Skin_Skins_f()
{
	if (cls.state == CA_DISCONNECTED)
	{
		common->Printf("WARNING: cannot complete command because there is no connection to a server\n");
		return;
	}

	clc.downloadNumber = 0;
	clc.downloadType = dl_none;

	if (cls.state != CA_ACTIVE)
	{	// get next signon phase
		CL_AddReliableCommand(va("begin %i", cl.servercount));
	}
}

/*
=================
CL_Init
=================
*/
void CL_Init(void)
{
	CL_SharedInit();

	cls.state = CA_DISCONNECTED;

	Info_SetValueForKey(cls.qh_userinfo, "name", "unnamed", MAX_INFO_STRING_QW, 64, 64, false, false);
	Info_SetValueForKey(cls.qh_userinfo, "playerclass", "0", MAX_INFO_STRING_QW, 64, 64, true, false);
	Info_SetValueForKey(cls.qh_userinfo, "topcolor", "0", MAX_INFO_STRING_QW, 64, 64, true, false);
	Info_SetValueForKey(cls.qh_userinfo, "bottomcolor", "0", MAX_INFO_STRING_QW, 64, 64, true, false);
	Info_SetValueForKey(cls.qh_userinfo, "rate", "2500", MAX_INFO_STRING_QW, 64, 64, true, false);
	Info_SetValueForKey(cls.qh_userinfo, "msg", "1", MAX_INFO_STRING_QW, 64, 64, true, false);

	CL_InitInput();
	CLQHW_InitPrediction();
	CLHW_InitEffects();
	CL_InitCam();
	PMQH_Init();

//
// register our commands
//
	Cmd_AddCommand("writeconfig", Com_WriteConfig_f);

	com_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times

	cl_timeout = Cvar_Get("cl_timeout", "60", 0);

	rcon_password = Cvar_Get("rcon_password", "", 0);
	rcon_address = Cvar_Get("rcon_address", "", 0);

	entlatency = Cvar_Get("entlatency", "20", 0);

	//
	// info mirrors
	//
	clqh_name = Cvar_Get("name","unnamed", CVAR_ARCHIVE | CVAR_USERINFO);
	clh2_playerclass = Cvar_Get("playerclass", "0", CVAR_ARCHIVE | CVAR_USERINFO);
	password = Cvar_Get("password", "", CVAR_USERINFO);
	qhw_spectator = Cvar_Get("spectator", "", CVAR_USERINFO);
	skin = Cvar_Get("skin","", CVAR_ARCHIVE | CVAR_USERINFO);
	team = Cvar_Get("team","", CVAR_ARCHIVE | CVAR_USERINFO);
	qhw_topcolor = Cvar_Get("topcolor","0", CVAR_ARCHIVE | CVAR_USERINFO);
	qhw_bottomcolor = Cvar_Get("bottomcolor","0", CVAR_ARCHIVE | CVAR_USERINFO);
	rate = Cvar_Get("rate","2500", CVAR_ARCHIVE | CVAR_USERINFO);
	msg = Cvar_Get("msg","1", CVAR_ARCHIVE | CVAR_USERINFO);
	noaim = Cvar_Get("noaim","0", CVAR_ARCHIVE | CVAR_USERINFO);
	clhw_talksounds = Cvar_Get("talksounds", "1", CVAR_ARCHIVE);

	clhw_teamcolor = Cvar_Get("clhw_teamcolor", "187", CVAR_ARCHIVE);

	Cmd_AddCommand("version", CL_Version_f);

	Cmd_AddCommand("changing", CL_Changing_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CLHW_Record_f);
	Cmd_AddCommand("rerecord", CL_ReRecord_f);
	Cmd_AddCommand("stop", CLQH_Stop_f);
	Cmd_AddCommand("playdemo", CLQHW_PlayDemo_f);
	Cmd_AddCommand("timedemo", CLQH_TimeDemo_f);

	Cmd_AddCommand("skins", Skin_Skins_f);

	Cmd_AddCommand("quit", CL_Quit_f);

	Cmd_AddCommand("connect", CLQHW_Connect_f);
	Cmd_AddCommand("reconnect", CLQHW_Reconnect_f);

	Cmd_AddCommand("rcon", CL_Rcon_f);
	Cmd_AddCommand("packet", CL_Packet_f);
	Cmd_AddCommand("user", CL_User_f);
	Cmd_AddCommand("users", CL_Users_f);

	Cmd_AddCommand("setinfo", CL_SetInfo_f);
	Cmd_AddCommand("fullinfo", CL_FullInfo_f);
	Cmd_AddCommand("fullserverinfo", CL_FullServerinfo_f);

	Cmd_AddCommand("color", CL_Color_f);
	Cmd_AddCommand("download", CL_Download_f);

	Cmd_AddCommand("sensitivity_save", CL_Sensitivity_save_f);

//
// forward to server commands
//
	Cmd_AddCommand("kill", NULL);
	Cmd_AddCommand("say", NULL);
	Cmd_AddCommand("say_team", NULL);
	Cmd_AddCommand("serverinfo", NULL);
}


/*
================
Host_EndGame

Call this to drop to a console without exiting the qwcl
================
*/
void Host_EndGame(const char* message, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr,message);
	Q_vsnprintf(string, 1024, message, argptr);
	va_end(argptr);
	common->Printf("\n===========================\n");
	common->Printf("Host_EndGame: %s\n",string);
	common->Printf("===========================\n\n");

	CL_Disconnect(true);

	longjmp(host_abort, 1);
}

/*
================
Host_FatalError

This shuts down the client and exits qwcl
================
*/
void Host_FatalError(const char* error, ...)
{
	va_list argptr;
	char string[1024];

	if (com_errorEntered)
	{
		Sys_Error("Host_FatalError: recursively entered");
	}
	com_errorEntered = true;

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	common->Printf("Host_FatalError: %s\n",string);

	CL_Disconnect(true);
	cls.qh_demonum = -1;

	com_errorEntered = false;

// FIXME
	Sys_Error("Host_FatalError: %s\n",string);
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
int nopacketcount;
void Host_Frame(float time)
{
	try
	{
		static double time1 = 0;
		static double time2 = 0;
		static double time3 = 0;
		int pass1, pass2, pass3;
		float fps;
		if (setjmp(host_abort))
		{
			return;		// something bad happened, or the server disconnected

		}
		// decide the simulation time
		realtime += time;
		cls.realtime = realtime * 1000;
		if (oldrealtime > realtime)
		{
			oldrealtime = 0;
		}

#define max(a, b)   ((a) > (b) ? (a) : (b))
#define min(a, b)   ((a) < (b) ? (a) : (b))
		fps = max(30.0, min(rate->value / 80.0, 72.0));

		if (!cls.qh_timedemo && realtime - oldrealtime < 1.0 / fps)
		{
			return;		// framerate is too high
		}
		host_frametime = realtime - oldrealtime;
		oldrealtime = realtime;
		if (host_frametime > 0.2)
		{
			host_frametime = 0.2;
		}
		cls.frametime = (int)(host_frametime * 1000);
		cls.realFrametime = cls.frametime;

		// allow mice or other external controllers to add commands
		IN_Frame();

		Com_EventLoop();

		// process console commands
		Cbuf_Execute();

		// fetch results from server
		CL_ReadPackets();

		// send intentions now
		// resend a connection request if necessary
		if (cls.state == CA_DISCONNECTED)
		{
			CLQHW_CheckForResend();
		}
		else
		{
			CLHW_SendCmd();
		}

		// Set up prediction for other players
		CLHW_SetUpPlayerPrediction(false);

		// do client side motion prediction
		CLHW_PredictMove();

		// Set up prediction for other players
		CLHW_SetUpPlayerPrediction(true);

		// update video
		if (com_speeds->value)
		{
			time1 = Sys_DoubleTime();
		}

		Con_RunConsole();

		SCR_UpdateScreen();

		if (com_speeds->value)
		{
			time2 = Sys_DoubleTime();
		}

		// update audio
		if (cls.state == CA_ACTIVE)
		{
			S_Respatialize(cl.playernum + 1, cl.refdef.vieworg, cl.refdef.viewaxis, 0);
			CL_RunDLights();

			CL_UpdateParticles(movevars.gravity);
		}

		S_Update();

		CDAudio_Update();

		if (com_speeds->value)
		{
			pass1 = (time1 - time3) * 1000;
			time3 = Sys_DoubleTime();
			pass2 = (time2 - time1) * 1000;
			pass3 = (time3 - time2) * 1000;
			common->Printf("%3i tot %3i server %3i gfx %3i snd\n",
				pass1 + pass2 + pass3, pass1, pass2, pass3);
		}

		cls.framecount++;
	}
	catch (DropException& e)
	{
		Host_EndGame(e.What());
	}
	catch (Exception& e)
	{
		Host_FatalError("%s", e.What());
	}
}

/*
====================
Host_Init
====================
*/
void Host_Init(quakeparms_t* parms)
{
	try
	{
		GGameType = GAME_Hexen2 | GAME_HexenWorld;
		Sys_SetHomePathSuffix("jlhexen2");

		COM_InitArgv2(parms->argc, parms->argv);
//	COM_AddParm ("-game");
//	COM_AddParm ("hw");

		host_parms = *parms;

		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();
		V_Init();

		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);

		COM_Init(parms->basedir);

		NETQHW_Init(HWPORT_CLIENT);

		Netchan_Init(0);

		CL_InitKeyCommands();
		Com_InitDebugLog();
		Con_Init();
		UI_Init();
		ComH2_LoadStrings();

		cls.state = CA_DISCONNECTED;
		CL_Init();

		Cbuf_InsertText("exec hexen.rc\n");
		Cbuf_AddText("cl_warncmd 1\n");
		Cbuf_Execute();

		IN_Init();
		CL_InitRenderer();
		Sys_ShowConsole(0, false);
		SCR_Init();
		S_Init();
		CLH2_InitTEnts();
		CDAudio_Init();
		MIDI_Init();
		SbarH2_Init();

		com_fullyInitialized = true;

		common->Printf("������� HexenWorld Initialized �������\n");
	}
	catch (Exception& e)
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
		printf("recursive shutdown\n");
		return;
	}
	isdown = true;

	Com_WriteConfiguration();

	CDAudio_Shutdown();
	MIDI_Cleanup();
	NET_Shutdown();
	S_Shutdown();
	IN_Shutdown();
	R_Shutdown(true);
}

float* CL_GetSimOrg()
{
	return cl.qh_simorg;
}

void server_referencer_dummy()
{
	svh2_kingofhill = 0;
}
