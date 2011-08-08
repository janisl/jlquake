//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <linux/cdrom.h>

#include "client.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void CDAudio_Stop();
void CDAudio_Pause();

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

qboolean cdValid = false;
qboolean	playing = false;
qboolean	wasPlaying = false;
qboolean	initialized = false;
qboolean	enabled = true;
qboolean playLooping = false;
float	cdvolume;
byte 	remap[100];
byte		playTrack;
byte		maxTrack;

int cdfile = -1;
static char cd_dev_old[64] = "/dev/cdrom";

QCvar	*cd_volume;
QCvar *cd_nocd;
QCvar *cd_dev;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CDAudio_Eject
//
//==========================================================================

void CDAudio_Eject()
{
	if (cdfile == -1 || !enabled)
	{
		return; // no cd init'd
	}

	if (ioctl(cdfile, CDROMEJECT) == -1)
	{
		GLog.DWrite("ioctl cdromeject failed\n");
	}
}


//==========================================================================
//
//	CDAudio_CloseDoor
//
//==========================================================================

void CDAudio_CloseDoor()
{
	if (cdfile == -1 || !enabled)
	{
		return; // no cd init'd
	}

	if (ioctl(cdfile, CDROMCLOSETRAY) == -1)
	{
		GLog.DWrite("ioctl cdromclosetray failed\n");
	}
}

//==========================================================================
//
//	CDAudio_GetAudioDiskInfo
//
//==========================================================================

int CDAudio_GetAudioDiskInfo()
{
	struct cdrom_tochdr tochdr;

	cdValid = false;

	if (ioctl(cdfile, CDROMREADTOCHDR, &tochdr) == -1)
	{
		GLog.DWrite("ioctl cdromreadtochdr failed\n");
		return -1;
	}

	if (tochdr.cdth_trk0 < 1)
	{
		GLog.DWrite("CDAudio: no music tracks\n");
		return -1;
	}

	cdValid = true;
	maxTrack = tochdr.cdth_trk1;

	return 0;
}

//==========================================================================
//
//	CDAudio_Play
//
//==========================================================================

void CDAudio_Play(int track, qboolean looping)
{
	struct cdrom_tocentry entry;
	struct cdrom_ti ti;

	if (cdfile == -1 || !enabled)
	{
		return;
	}

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
		{
			return;
		}
	}

	track = remap[track];

	if (track < 1 || track > maxTrack)
	{
		GLog.DWrite("CDAudio: Bad track number %u.\n", track);
		return;
	}

	// don't try to play a non-audio track
	entry.cdte_track = track;
	entry.cdte_format = CDROM_MSF;
    if (ioctl(cdfile, CDROMREADTOCENTRY, &entry) == -1)
	{
		GLog.DWrite("ioctl cdromreadtocentry failed\n");
		return;
	}
	if (entry.cdte_ctrl == CDROM_DATA_TRACK)
	{
		GLog.Write("CDAudio: track %i is not audio\n", track);
		return;
	}

	if (playing)
	{
		if (playTrack == track)
		{
			return;
		}
		CDAudio_Stop();
	}

	ti.cdti_trk0 = track;
	ti.cdti_trk1 = track;
	ti.cdti_ind0 = 1;
	ti.cdti_ind1 = 99;

	if (ioctl(cdfile, CDROMPLAYTRKIND, &ti) == -1)
    {
		GLog.DWrite("ioctl cdromplaytrkind failed\n");
		return;
    }

	if (ioctl(cdfile, CDROMRESUME) == -1)
	{
		GLog.DWrite("ioctl cdromresume failed\n");
	}

	playLooping = looping;
	playTrack = track;
	playing = true;

	if (GGameType & GAME_QuakeHexen)
	{
		if (cdvolume == 0.0)
		{
			CDAudio_Pause();
		}
	}
	else
	{
		if (cd_volume->value == 0.0)
		{
			CDAudio_Pause();
		}
	}
}

//==========================================================================
//
//	CDAudio_Stop
//
//==========================================================================

void CDAudio_Stop()
{
	if (cdfile == -1 || !enabled)
	{
		return;
	}

	if (!playing)
	{
		return;
	}

	if (ioctl(cdfile, CDROMSTOP) == -1)
	{
		GLog.DWrite("ioctl cdromstop failed (%d)\n", errno);
	}

	wasPlaying = false;
	playing = false;
}

//==========================================================================
//
//	CDAudio_Pause
//
//==========================================================================

void CDAudio_Pause()
{
	if (cdfile == -1 || !enabled)
	{
		return;
	}

	if (!playing)
	{
		return;
	}

	if (ioctl(cdfile, CDROMPAUSE) == -1)
	{
		GLog.DWrite("ioctl cdrompause failed\n");
	}

	wasPlaying = playing;
	playing = false;
}

//==========================================================================
//
//	CDAudio_Resume
//
//==========================================================================

void CDAudio_Resume()
{
	if (cdfile == -1 || !enabled)
	{
		return;
	}

	if (!cdValid)
	{
		return;
	}

	if (!wasPlaying)
	{
		return;
	}

	if (ioctl(cdfile, CDROMRESUME) == -1)
	{
		GLog.DWrite("ioctl cdromresume failed\n");
	}
	playing = true;
}

//==========================================================================
//
//	CD_f
//
//==========================================================================

void CD_f()
{
	char	*command;
	int		ret;
	int		n;

	if (Cmd_Argc() < 2)
	{
		return;
	}

	command = Cmd_Argv(1);

	if (String::ICmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (String::ICmp(command, "off") == 0)
	{
		if (playing)
		{
			CDAudio_Stop();
		}
		enabled = false;
		return;
	}

	if (String::ICmp(command, "reset") == 0)
	{
		enabled = true;
		if (playing)
		{
			CDAudio_Stop();
		}
		for (n = 0; n < 100; n++)
		{
			remap[n] = n;
		}
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (String::ICmp(command, "remap") == 0)
	{
		ret = Cmd_Argc() - 2;
		if (ret <= 0)
		{
			for (n = 1; n < 100; n++)
			{
				if (remap[n] != n)
				{
					GLog.Write("  %u -> %u\n", n, remap[n]);
				}
			}
			return;
		}
		for (n = 1; n <= ret; n++)
		{
			remap[n] = String::Atoi(Cmd_Argv (n+1));
		}
		return;
	}

	if (String::ICmp(command, "close") == 0)
	{
		CDAudio_CloseDoor();
		return;
	}

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
		{
			GLog.Write("No CD in player.\n");
			return;
		}
	}

	if (String::ICmp(command, "play") == 0)
	{
		CDAudio_Play(String::Atoi(Cmd_Argv (2)), false);
		return;
	}

	if (String::ICmp(command, "loop") == 0)
	{
		CDAudio_Play(String::Atoi(Cmd_Argv (2)), true);
		return;
	}

	if (String::ICmp(command, "stop") == 0)
	{
		CDAudio_Stop();
		return;
	}

	if (String::ICmp(command, "pause") == 0)
	{
		CDAudio_Pause();
		return;
	}

	if (String::ICmp(command, "resume") == 0)
	{
		CDAudio_Resume();
		return;
	}

	if (String::ICmp(command, "eject") == 0)
	{
		if (playing)
		{
			CDAudio_Stop();
		}
		CDAudio_Eject();
		cdValid = false;
		return;
	}

	if (String::ICmp(command, "info") == 0)
	{
		GLog.Write("%u tracks\n", maxTrack);
		if (playing)
		{
			GLog.Write("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		}
		else if (wasPlaying)
		{
			GLog.Write("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		}
		GLog.Write("Volume is %f\n", cdvolume);
		return;
	}
}

//==========================================================================
//
//	CDAudio_Update
//
//==========================================================================

void CDAudio_Update()
{
	struct cdrom_subchnl subchnl;
	static time_t lastchk;

	if (cdfile == -1 || !enabled)
	{
		return;
	}

	if (GGameType & GAME_QuakeHexen)
	{
		if (bgmvolume->value != cdvolume)
		{
			if (cdvolume)
			{
				Cvar_SetValue("bgmvolume", 0.0);
				cdvolume = bgmvolume->value;
				CDAudio_Pause();
			}
			else
			{
				Cvar_SetValue("bgmvolume", 1.0);
				cdvolume = bgmvolume->value;
				CDAudio_Resume();
			}
		}
	}
	else
	{
		if (cd_volume && cd_volume->value != cdvolume)
		{
			if (cdvolume)
			{
				Cvar_SetValueLatched("cd_volume", 0.0);
				cdvolume = cd_volume->value;
				CDAudio_Pause();
			}
			else
			{
				Cvar_SetValueLatched("cd_volume", 1.0);
				cdvolume = cd_volume->value;
				CDAudio_Resume();
			}
		}
	}

	if (playing && lastchk < time(NULL))
	{
		lastchk = time(NULL) + 2; //two seconds between chks
		subchnl.cdsc_format = CDROM_MSF;
		if (ioctl(cdfile, CDROMSUBCHNL, &subchnl) == -1 )
		{
			GLog.DWrite("ioctl cdromsubchnl failed\n");
			playing = false;
			return;
		}
		if (subchnl.cdsc_audiostatus != CDROM_AUDIO_PLAY &&
			subchnl.cdsc_audiostatus != CDROM_AUDIO_PAUSED)
		{
			playing = false;
			if (playLooping)
			{
				CDAudio_Play(playTrack, true);
			}
		}
	}
}

//==========================================================================
//
//	CDAudio_Init
//
//==========================================================================

int CDAudio_Init()
{
	int i;

	if (GGameType & GAME_QuakeHexen)
	{
		if (COM_CheckParm("-nocdaudio"))
		{
			return -1;
		}

		if ((i = COM_CheckParm("-cddev")) != 0 && i < COM_Argc() - 1)
		{
			String::NCpy(cd_dev_old, COM_Argv(i + 1), sizeof(cd_dev_old));
			cd_dev_old[sizeof(cd_dev_old) - 1] = 0;
		}

		cdfile = open(cd_dev_old, O_RDONLY);
		if (cdfile == -1)
		{
			GLog.Write("CDAudio_Init: open of \"%s\" failed (%i)\n", cd_dev_old, errno);
			cdfile = -1;
			return -1;
		}
	}
	else
	{
		QCvar* cv = Cvar_Get("nocdaudio", "0", CVAR_INIT);
		if (cv->value)
		{
			return -1;
		}

		cd_nocd = Cvar_Get("cd_nocd", "0", CVAR_ARCHIVE);
		if (cd_nocd->value)
		{
			return -1;
		}

		cd_volume = Cvar_Get("cd_volume", "1", CVAR_ARCHIVE);

		cd_dev = Cvar_Get("cd_dev", "/dev/cdrom", CVAR_ARCHIVE);

		cdfile = open(cd_dev->string, O_RDONLY);

		if (cdfile == -1)
		{
			GLog.Write("CDAudio_Init: open of \"%s\" failed (%i)\n", cd_dev->string, errno);
			cdfile = -1;
			return -1;
		}
	}

	for (i = 0; i < 100; i++)
	{
		remap[i] = i;
	}
	initialized = true;
	enabled = true;

	if (CDAudio_GetAudioDiskInfo())
	{
		GLog.Write("CDAudio_Init: No CD in player.\n");
		cdValid = false;
	}

	Cmd_AddCommand("cd", CD_f);

	GLog.Write("CD Audio Initialized\n");

	return 0;
}

//==========================================================================
//
//	CDAudio_Shutdown
//
//==========================================================================

void CDAudio_Shutdown()
{
	if (!initialized)
	{
		return;
	}
	CDAudio_Stop();
	close(cdfile);
	cdfile = -1;
}

//==========================================================================
//
//	CDAudio_Activate
//
//==========================================================================

void CDAudio_Activate(qboolean active)
{
	if (active)
	{
		CDAudio_Resume();
	}
	else
	{
		CDAudio_Pause();
	}
}
