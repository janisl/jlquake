#include "../../common/qcommon.h"
#include "../../apps/main.h"

void Com_SharedInit()
{
	GGameType = GAME_Hexen2 | GAME_HexenWorld;
	Sys_SetHomePathSuffix("jlhexen2");
}
