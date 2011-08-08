#include "quakedef.h"
#include "../../client/sound/local.h"

int S_GetClientFrameCount()
{
	return host_framecount;
}

float S_GetClientFrameTime()
{
	return host_frametime;
}

sfx_t *S_RegisterSexedSound(int entnum, char *base)
{
	return NULL;
}

int S_GetClFrameServertime()
{
	return 0;
}

bool S_GetDisableScreen()
{
	return false;
}
