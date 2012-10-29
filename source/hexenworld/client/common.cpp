// common.c -- misc functions used in client and server

#ifdef DEDICATED
#include "qwsvdef.h"
#else
#include "quakedef.h"
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include "../../client/public.h"
#include "../../server/public.h"

/*
================
COM_Init
================
*/
void COM_Init()
{
	Com_InitByteOrder();

	COM_InitCommonCvars();

	qh_registered = Cvar_Get("registered", "0", 0);

	FS_InitFilesystem();
	COMQH_CheckRegistered();
}
