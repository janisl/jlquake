#include "quakedef.h"
#include "winquake.h"

void IN_Init()
{
	IN_SharedInit();
}

void IN_Move()
{
}

void IN_Commands()
{
	IN_SharedFrame();
}
