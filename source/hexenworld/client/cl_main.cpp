// cl_main.c  -- client main loop

#include "quakedef.h"
#ifdef _WIN32
#include "../../client/windows_shared.h"
#endif
#include "../../server/public.h"
#include "../../common/hexen2strings.h"
#include "../../client/game/quake_hexen2/demo.h"
#include "../../client/game/quake_hexen2/connection.h"

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
	virtual void Disconnect(const char* message);
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

void idCommonLocal::Disconnect(const char* message)
{
	CL_Disconnect(true);
	SV_Shutdown("");
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
=================
CL_Init
=================
*/
void CL_Init(void)
{
	CL_SharedInit();

	cls.state = CA_DISCONNECTED;

//
// register our commands
//
	Cmd_AddCommand("writeconfig", Com_WriteConfig_f);

	com_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times

	Cmd_AddCommand("version", CL_Version_f);

	Cmd_AddCommand("quit", CL_Quit_f);
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
		fps = max(30.0, min(clqhw_rate->value / 80.0, 72.0));

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
		CLQHW_ReadPackets();

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
		ComH2_LoadStrings();

		cls.state = CA_DISCONNECTED;
		CL_Init();

		Cbuf_InsertText("exec hexen.rc\n");
		Cbuf_AddText("cl_warncmd 1\n");
		Cbuf_Execute();

		IN_Init();
		CL_InitRenderer();
		Sys_ShowConsole(0, false);
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

void server_referencer_dummy()
{
	svh2_kingofhill = 0;
}
