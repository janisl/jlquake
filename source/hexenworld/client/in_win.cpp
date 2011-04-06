#include "quakedef.h"
#include "winquake.h"

/*
===========
Force_CenterView_f
===========
*/
void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}

void IN_Init()
{
	IN_SharedInit();

	Cmd_AddCommand ("force_centerview", Force_CenterView_f);
}

void IN_Move()
{
}

void IN_Commands()
{
	IN_SharedFrame();
}
