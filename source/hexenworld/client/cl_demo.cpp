
#include "quakedef.h"
#include "../../client/game/quake_hexen2/demo.h"

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
	CL_SendConnectPacket();
}
