/*
 * $Header: /H2 Mission Pack/HOST.C 6     3/12/98 6:31p Mgummelt $
 */

#include "../common/qcommon.h"
#include "../apps/main.h"

void Com_SharedInit()
{
	GGameType = GAME_Hexen2;
	Sys_SetHomePathSuffix("jlhexen2");
}
