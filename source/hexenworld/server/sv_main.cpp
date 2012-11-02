#include "../../common/qcommon.h"
#include "../../common/hexen2strings.h"
#include "../../server/public.h"
#include "../../client/public.h"
#include "../../apps/main.h"

static double oldtime;

void Com_SharedFrame()
{
	// select on the net socket and stdin
	// the only reason we have a timeout at all is so that if the last
	// connected client times out, the message would not otherwise
	// be printed until the next event.
	//JL: Originally timeout was 0.1 ms
	if (!SOCK_Sleep(ip_sockets[0], 1))
	{
		return;
	}

	// find time passed since last cycle
	double newtime = Sys_DoubleTime();
	double time = newtime - oldtime;
	oldtime = newtime;

	// keep the random time dependent
	rand();

	Com_EventLoop();

	// process console commands
	Cbuf_Execute();

	SV_Frame(time * 1000);
}

void Com_SharedInit(int argc, char* argv[], char* cmdline)
{
	GGameType = GAME_Hexen2 | GAME_HexenWorld;
	Sys_SetHomePathSuffix("jlhexen2");

	COM_InitArgv2(argc, argv);

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);

	Com_InitByteOrder();

	COM_InitCommonCvars();

	qh_registered = Cvar_Get("registered", "0", 0);
	com_maxfps = Cvar_Get("com_maxfps", "0", CVAR_ARCHIVE);

	FS_InitFilesystem();
	COMQH_CheckRegistered();

	ComH2_LoadStrings();

	COM_InitCommonCommands();

	Cvar_Set("cl_warncmd", "1");

	SV_Init();
	Sys_Init();
	PMQH_Init();

	Cbuf_InsertText("exec server.cfg\n");

	com_fullyInitialized = true;

	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
	common->Printf("======== HexenWorld Initialized ========\n");

	// process command line arguments
	Cbuf_AddLateCommands();
	Cbuf_Execute();

	// if a map wasn't specified on the command line, spawn start.map

	if (!SV_IsServerActive())
	{
		Cmd_ExecuteString("map demo1");
	}
	if (!SV_IsServerActive())
	{
		common->Error("Couldn't spawn a server");
	}

	oldtime = Sys_DoubleTime() - HX_FRAME_TIME;
}
