// vid_null.c -- null video driver to aid porting efforts
// this assumes that one of the refs is statically linked to the executable

#include "../client/client.h"
#include "../ref_gl/gl_local.h"

void	VID_Init (void)
{
	VID_LoadRefresh();

    viddef.width = 320;
    viddef.height = 240;
}

void	VID_CheckChanges (void)
{
}

void	VID_MenuInit (void)
{
}

void	VID_MenuDraw (void)
{
}

const char *VID_MenuKey( int k)
{
	return NULL;
}
