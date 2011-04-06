// in_null.c -- for systems without a mouse

#include "../client/client.h"

QCvar	*in_mouse;

void IN_Init (void)
{
    in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
    in_joystick = Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE);
}

void IN_Commands (void)
{
}

void IN_Move ()
{
}

void IN_Activate (qboolean active)
{
}

