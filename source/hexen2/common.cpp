// common.c -- misc functions used in client and server

/*
 * $Header: /H2 Mission Pack/COMMON.C 15    3/19/98 4:28p Jmonroe $
 */

#include "quakedef.h"
#include "../client/public.h"
#include "../apps/main.h"

/*
================
COM_Init
================
*/
void COM_Init()
{
	Com_InitByteOrder();

	qh_registered = Cvar_Get("registered", "0", 0);

	FS_InitFilesystem();
	COMQH_CheckRegistered();
}
