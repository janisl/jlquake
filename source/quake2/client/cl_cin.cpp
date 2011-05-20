/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "client.h"
#include "../../client/cinematic_local.h"

typedef struct
{
	QCinematicCin*	Cin;
	QCinematicPcx*	Pcx;
} cinematics_t;

cinematics_t	cin;

//=============================================================

/*
==================
SCR_StopCinematic
==================
*/
void SCR_StopCinematic (void)
{
	cl.cinematictime = 0;	// done
	if (cin.Cin)
	{
		delete cin.Cin;
		cin.Cin = NULL;
	}
	if (cin.Pcx)
	{
		delete cin.Pcx;
		cin.Pcx = NULL;
	}
}

/*
====================
SCR_FinishCinematic

Called when either the cinematic completes, or it is aborted
====================
*/
void SCR_FinishCinematic (void)
{
	// tell the server to advance to the next map / cinematic
	cls.netchan.message.WriteByte(clc_stringcmd);
	cls.netchan.message.WriteString2(va("nextserver %i\n", cl.servercount));
}

//==========================================================================


/*
==================
SCR_RunCinematic

==================
*/
void SCR_RunCinematic (void)
{
	if (cl.cinematictime <= 0)
	{
		SCR_StopCinematic ();
		return;
	}

	if (cin.Pcx)
		return;		// static image

	if (in_keyCatchers != 0)
	{	// pause if menu or console is up
		cl.cinematictime = cls.realtime - cin.Cin->GetCinematicTime();
		return;
	}

	if (!cin.Cin->Update(cls.realtime - cl.cinematictime))
	{
		SCR_StopCinematic ();
		SCR_FinishCinematic ();
		cl.cinematictime = 1;	// hack to get the black screen behind loading
		SCR_BeginLoadingPlaque ();
		cl.cinematictime = 0;
		return;
	}
}

/*
==================
SCR_DrawCinematic

Returns true if a cinematic is active, meaning the view rendering
should be skipped
==================
*/
qboolean SCR_DrawCinematic (void)
{
	if (cl.cinematictime <= 0)
	{
		return false;
	}

	if (in_keyCatchers & KEYCATCH_UI)
	{
		// pause if menu is up
		return true;
	}

	if (cin.Cin)
	{
		if (!cin.Cin->buf)
			return true;

		re.DrawStretchRaw (0, 0, viddef.width, viddef.height,
			cin.Cin->width, cin.Cin->height, cin.Cin->buf);
	}
	else
	{
		if (!cin.Pcx->buf)
			return true;

		re.DrawStretchRaw (0, 0, viddef.width, viddef.height,
			cin.Pcx->width, cin.Pcx->height, cin.Pcx->buf);
	}

	return true;
}

/*
==================
SCR_PlayCinematic

==================
*/
void SCR_PlayCinematic (char *arg)
{
	char	name[MAX_OSPATH], *dot;

	// make sure CD isn't playing music
	CDAudio_Stop();

	dot = strstr (arg, ".");
	if (dot && !QStr::Cmp(dot, ".pcx"))
	{	// static pcx image
		cin.Pcx = new QCinematicPcx();
		QStr::Sprintf (name, sizeof(name), "pics/%s", arg);
		cl.cinematictime = 1;
		SCR_EndLoadingPlaque ();
		cls.state = ca_active;
		if (!cin.Pcx->Open(name))
		{
			Com_Printf ("%s not found.\n", name);
			cl.cinematictime = 0;
		}
		return;
	}

	cin.Cin = new QCinematicCin();
	QStr::Sprintf (name, sizeof(name), "video/%s", arg);
	if (!cin.Cin->Open(name))
	{
		delete cin.Cin;
//		Com_Error (ERR_DROP, "Cinematic %s not found.\n", name);
		SCR_FinishCinematic ();
		cl.cinematictime = 0;	// done
		return;
	}

	SCR_EndLoadingPlaque ();

	cls.state = ca_active;

	cl.cinematictime = Sys_Milliseconds_ ();
}
