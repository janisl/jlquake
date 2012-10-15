// host.c -- coordinates spawning and killing of local servers

/*
 * $Header: /H2 Mission Pack/HOST.C 6     3/12/98 6:31p Mgummelt $
 */

#include "quakedef.h"
#include "../common/hexen2strings.h"
#include "../server/public.h"

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

quakeparms_t host_parms;

double host_frametime;
double host_time;
double realtime;					// without any filtering or bounding
double oldrealtime;					// last frame run

jmp_buf host_abortserver;

Cvar* host_framerate;	// set for slow motion

Cvar* sys_ticrate;
Cvar* serverprofile;

Cvar* sys_adaptive;

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
================
Host_EndGame
================
*/
void Host_EndGame(const char* message, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr,message);
	Q_vsnprintf(string, 1024, message, argptr);
	va_end(argptr);
	common->DPrintf("Host_EndGame: %s\n",string);

	SV_Shutdown("");

	if (com_dedicated->integer)
	{
		Sys_Error("Host_EndGame: %s\n",string);		// dedicated servers exit

	}
#ifndef DEDICATED
	if (cls.qh_demonum != -1)
	{
		CL_NextDemo();
	}
	else
	{
		CL_Disconnect();
	}

	longjmp(host_abortserver, 1);
#endif
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error(const char* error, ...)
{
	va_list argptr;
	char string[1024];

	if (com_errorEntered)
	{
		Sys_Error("Host_Error: recursively entered");
	}
	com_errorEntered = true;

#ifndef DEDICATED
	SCR_EndLoadingPlaque();			// reenable screen updates
#endif

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	common->Printf("Host_Error: %s\n",string);

	SV_Shutdown("");

	if (com_dedicated->integer)
	{
		Sys_Error("Host_Error: %s\n",string);	// dedicated servers exit
	}

#ifndef DEDICATED
	CL_Disconnect();
	cls.qh_demonum = -1;

	com_errorEntered = false;

	longjmp(host_abortserver, 1);
#endif
}

/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal(void)
{
#ifndef DEDICATED
	Cmd_AddCommand("writeconfig", Com_WriteConfig_f);
#endif

	COM_InitCommonCvars();

	host_framerate = Cvar_Get("host_framerate", "0", 0);	// set for slow motion
	com_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times

	sys_ticrate = Cvar_Get("sys_ticrate", "0.05", 0);
	serverprofile = Cvar_Get("serverprofile", "0", 0);

	sys_adaptive = Cvar_Get("sys_adaptive","1", CVAR_ARCHIVE);

	if (COM_CheckParm("-dedicated"))
	{
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
	}
	else
	{
#ifdef DEDICATED
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
#else
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);

		cls.state = CA_DISCONNECTED;
#endif
	}

	Host_InitCommands();

	host_time = 1.0;		// so a think at time 0 won't get called
}

/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
qboolean Host_FilterTime(float time)
{
	realtime += time;
	SVQH_SetRealTime(realtime * 1000);
#ifdef DEDICATED
	if (realtime - oldrealtime < 1.0 / 72.0)
#else
	cls.realtime = realtime * 1000;

	if (!cls.qh_timedemo && realtime - oldrealtime < 1.0 / 72.0)
#endif
	{
		return false;		// framerate is too high

	}
	host_frametime = realtime - oldrealtime;
	oldrealtime = realtime;

	if (host_framerate->value > 0)
	{
		host_frametime = host_framerate->value;
	}
	else
	{	// don't allow really long or short frames
		if (host_frametime > 0.05 && !sys_adaptive->value)
		{
			host_frametime = 0.05;
		}
		if (host_frametime < 0.001)
		{
			host_frametime = 0.001;
		}
	}

#ifndef DEDICATED
	cls.frametime = (int)(host_frametime * 1000);
	cls.realFrametime = cls.frametime;
#endif
	return true;
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame(float time)
{
	static double time1 = 0;
	static double time2 = 0;
	static double time3 = 0;
	int pass1, pass2, pass3;
	double save_host_frametime,total_host_frametime;

	if (setjmp(host_abortserver))
	{
		return;			// something bad happened, or the server disconnected

	}
	try
	{
// keep the random time dependent
		rand();

// decide the simulation time
		if (!Host_FilterTime(time))
		{
			return;		// don't run too fast, or packets will flood out
		}

#ifndef DEDICATED
// allow mice or other external controllers to add commands
		IN_Frame();
#endif

		Com_EventLoop();

// process console commands
		Cbuf_Execute();

#ifndef DEDICATED
		NET_Poll();

// if running the server locally, make intentions now
		if (SV_IsServerActive())
		{
			CL_SendCmd();
		}
#endif

//-------------------
//
// server operations
//
//-------------------

		save_host_frametime = total_host_frametime = host_frametime;
		if (sys_adaptive->value)
		{
			if (host_frametime > 0.05)
			{
				host_frametime = 0.05;
			}
		}

		if (total_host_frametime > 1.0)
		{
			total_host_frametime = 0.05;
		}

		do
		{
			SV_Frame(host_frametime * 1000);

			//-------------------
			//
			// client operations
			//
			//-------------------

#ifndef DEDICATED
			// if running the server remotely, send intentions now after
			// the incoming messages have been read
			if (!SV_IsServerActive())
			{
				CL_SendCmd();
			}

			host_time += host_frametime;

			// fetch results from server
			if (cls.state == CA_ACTIVE)
			{
				CL_ReadFromServer();

				if (cl.qh_serverTimeFloat != cl.qh_oldtime)
				{
					CL_UpdateParticles(Cvar_VariableValue("sv_gravity"));
				}
				CLH2_UpdateEffects();
			}
#endif

			if (!sys_adaptive->value)
			{
				break;
			}

			total_host_frametime -= 0.05;
			if (total_host_frametime > 0 && total_host_frametime < 0.05)
			{
				save_host_frametime -= total_host_frametime;
				oldrealtime -= total_host_frametime;
				break;
			}

		}
		while (total_host_frametime > 0);


		host_frametime = save_host_frametime;

#ifndef DEDICATED
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
		if (clc.qh_signon == SIGNONS)
		{
			S_Respatialize(cl.viewentity, cl.refdef.vieworg, cl.refdef.viewaxis, 0);
			CL_RunDLights();
		}

		S_Update();

		CDAudio_Update();
#endif

		if (com_speeds->value)
		{
			pass1 = (time1 - time3) * 1000;
			time3 = Sys_DoubleTime();
			pass2 = (time2 - time1) * 1000;
			pass3 = (time3 - time2) * 1000;
			common->Printf("%3i tot %3i server %3i gfx %3i snd\n",
				pass1 + pass2 + pass3, pass1, pass2, pass3);
		}

#ifndef DEDICATED
		cls.framecount++;
#endif
	}
	catch (DropException& e)
	{
		Host_Error(e.What());
	}
	catch (EndGameException& e)
	{
		Host_EndGame(e.What());
	}
}

void Host_Frame(float time)
{
	try
	{
		double time1, time2;
		static double timetotal;
		static int timecount;
		int m;

		if (!serverprofile->value)
		{
			_Host_Frame(time);
			return;
		}

		time1 = Sys_DoubleTime();
		_Host_Frame(time);
		time2 = Sys_DoubleTime();

		timetotal += time2 - time1;
		timecount++;

		if (timecount < 1000)
		{
			return;
		}

		m = timetotal * 1000 / timecount;
		timecount = 0;
		timetotal = 0;

		common->Printf("serverprofile: %2i clients %2i msec\n",  SVQH_GetNumConnectedClients(),  m);
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
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
		GGameType = GAME_Hexen2;
#ifdef MISSIONPACK
		GGameType |= GAME_H2Portals;
#endif
		Sys_SetHomePathSuffix("jlhexen2");

		host_parms = *parms;

		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();
#ifndef DEDICATED
		V_Init();
		Chase_Init();
#endif
		COM_Init(parms->basedir);
		Host_InitLocal();
#ifndef DEDICATED
		CL_InitKeyCommands();
#endif
		ComH2_LoadStrings();
		Com_InitDebugLog();
#ifndef DEDICATED
		Con_Init();
		UI_Init();
#endif
		SV_Init();

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");

#ifndef DEDICATED
		if (!com_dedicated->integer)
		{
			CL_Init();
		}
#endif

		Cbuf_InsertText("exec hexen.rc\n");
		Cbuf_Execute();

		NETQH_Init();

#ifndef DEDICATED
		if (!com_dedicated->integer)
		{
			IN_Init();
			CL_InitRenderer();
			Sys_ShowConsole(0, false);
			SCR_Init();
			S_Init();
			CLH2_InitTEnts();
			CDAudio_Init();
			MIDI_Init();
			SbarH2_Init();
		}
#endif

		com_fullyInitialized = true;

		common->Printf("======== Hexen II Initialized =========\n");
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

#ifndef DEDICATED
// keep common->Printf from trying to update the screen
	cls.disable_screen = true;

	Com_WriteConfiguration();

	CDAudio_Shutdown();
	MIDI_Cleanup();
#endif
	SVQH_ShutdownNetwork();
#ifndef DEDICATED
	CLQH_ShutdownNetwork();
#endif
	NETQH_Shutdown();
#ifndef DEDICATED
	S_Shutdown();
	IN_Shutdown();

	if (!com_dedicated->integer)
	{
		R_Shutdown(true);
	}
#endif
}

#ifdef DEDICATED
void CL_Disconnect()
{
}
void CL_EstablishConnection(const char* name)
{
}
void Host_Reconnect_f()
{
}
#endif

void CL_MapLoading(void)
{
}
void CL_ShutdownAll(void)
{
}
void CL_Disconnect(qboolean showMainMenu)
{
}
void CL_Drop()
{
}
void CLT3_PacketEvent(netadr_t from, QMsg* msg)
{
}
