//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../../client.h"
#include "../../windows_shared.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void CDAudio_Stop();
void CDAudio_Pause();

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static qboolean cdValid = false;
static qboolean playing = false;
static qboolean wasPlaying = false;
static qboolean initialized = false;
static qboolean enabled = false;
static qboolean playLooping = false;
static float cdvolume;
static byte remap[100];
static byte cdrom;
static byte playTrack;
static byte maxTrack;

static UINT wDeviceID;
static int loopcounter;

static Cvar* cd_nocd;
static Cvar* cd_loopcount;
static Cvar* cd_looptrack;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CDAudio_Eject
//
//==========================================================================

static void CDAudio_Eject()
{
	DWORD dwReturn;

	if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, (DWORD)NULL))
	{
		Log::develWrite("MCI_SET_DOOR_OPEN failed (%i)\n", dwReturn);
	}
}

//==========================================================================
//
//	CDAudio_CloseDoor
//
//==========================================================================

static void CDAudio_CloseDoor()
{
	DWORD dwReturn;

	if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, (DWORD)NULL))
	{
		Log::develWrite("MCI_SET_DOOR_CLOSED failed (%i)\n", dwReturn);
	}
}

//==========================================================================
//
//	CDAudio_GetAudioDiskInfo
//
//==========================================================================

static int CDAudio_GetAudioDiskInfo()
{
	DWORD dwReturn;
	MCI_STATUS_PARMS mciStatusParms;


	cdValid = false;

	mciStatusParms.dwItem = MCI_STATUS_READY;
	dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms);
	if (dwReturn)
	{
		Log::develWrite("CDAudio: drive ready test - get status failed\n");
		return -1;
	}
	if (!mciStatusParms.dwReturn)
	{
		Log::develWrite("CDAudio: drive not ready\n");
		return -1;
	}

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms);
	if (dwReturn)
	{
		Log::develWrite("CDAudio: get tracks - status failed\n");
		return -1;
	}
	if (mciStatusParms.dwReturn < 1)
	{
		Log::develWrite("CDAudio: no music tracks\n");
		return -1;
	}

	cdValid = true;
	maxTrack = mciStatusParms.dwReturn;

	return 0;
}

//==========================================================================
//
//	CDAudio_Play2
//
//==========================================================================

static void CDAudio_Play2(int track, qboolean looping)
{
	DWORD dwReturn;
	MCI_PLAY_PARMS mciPlayParms;
	MCI_STATUS_PARMS mciStatusParms;

	if (!enabled)
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
		if (GGameType & GAME_QuakeHexen)
		{
			Log::develWrite("CDAudio: Bad track number %u.\n", track);
		}
		else
		{
			CDAudio_Stop();
		}
		return;
	}

	// don't try to play a non-audio track
	mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatusParms.dwTrack = track;
	dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms);
	if (dwReturn)
	{
		Log::develWrite("MCI_STATUS failed (%i)\n", dwReturn);
		return;
	}
	if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO)
	{
		Log::write("CDAudio: track %i is not audio\n", track);
		return;
	}

	// get the length of the track to be played
	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
	dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms);
	if (dwReturn)
	{
		Log::develWrite("MCI_STATUS failed (%i)\n", dwReturn);
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

	mciPlayParms.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = (mciStatusParms.dwReturn << 8) | track;
	mciPlayParms.dwCallback = (DWORD)GMainWindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)(LPVOID)&mciPlayParms);
	if (dwReturn)
	{
		Log::develWrite("CDAudio: MCI_PLAY failed (%i)\n", dwReturn);
		return;
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
		if (Cvar_VariableValue("cd_nocd"))
		{
			CDAudio_Pause();
		}
	}
}

//==========================================================================
//
//	CDAudio_Play
//
//==========================================================================

void CDAudio_Play(int track, qboolean looping)
{
	// set a loop counter so that this track will change to the
	// looptrack later
	loopcounter = 0;
	CDAudio_Play2(track, looping);
}

//==========================================================================
//
//	CDAudio_Stop
//
//==========================================================================

void CDAudio_Stop()
{
	DWORD dwReturn;

	if (!enabled)
	{
		return;
	}

	if (!playing)
	{
		return;
	}

	if (dwReturn = mciSendCommand(wDeviceID, MCI_STOP, 0, (DWORD)NULL))
	{
		Log::develWrite("MCI_STOP failed (%i)", dwReturn);
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
	DWORD dwReturn;
	MCI_GENERIC_PARMS mciGenericParms;

	if (!enabled)
	{
		return;
	}

	if (!playing)
	{
		return;
	}

	mciGenericParms.dwCallback = (DWORD)GMainWindow;
	if (dwReturn = mciSendCommand(wDeviceID, MCI_PAUSE, 0, (DWORD)(LPVOID)&mciGenericParms))
	{
		Log::develWrite("MCI_PAUSE failed (%i)", dwReturn);
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
	DWORD dwReturn;
	MCI_PLAY_PARMS mciPlayParms;

	if (!enabled)
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

	mciPlayParms.dwFrom = MCI_MAKE_TMSF(playTrack, 0, 0, 0);
	mciPlayParms.dwTo = MCI_MAKE_TMSF(playTrack + 1, 0, 0, 0);
	mciPlayParms.dwCallback = (DWORD)GMainWindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD)(LPVOID)&mciPlayParms);
	if (dwReturn)
	{
		Log::develWrite("CDAudio: MCI_PLAY failed (%i)\n", dwReturn);
		return;
	}
	playing = true;
}

//==========================================================================
//
//	CD_f
//
//==========================================================================

static void CD_f()
{
	char* command;
	int ret;
	int n;

	if (Cmd_Argc() < 2)
	{
		Log::write("commands:");
		Log::write("on, off, reset, remap, \n");
		Log::write("play, stop, loop, pause, resume\n");
		Log::write("eject, close, info\n");
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
					Log::write("  %u -> %u\n", n, remap[n]);
				}
			}
			return;
		}
		for (n = 1; n <= ret; n++)
		{
			remap[n] = String::Atoi(Cmd_Argv(n + 1));
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
			Log::write("No CD in player.\n");
			return;
		}
	}

	if (String::ICmp(command, "play") == 0)
	{
		CDAudio_Play(String::Atoi(Cmd_Argv(2)), false);
		return;
	}

	if (String::ICmp(command, "loop") == 0)
	{
		CDAudio_Play(String::Atoi(Cmd_Argv(2)), true);
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
		Log::write("%u tracks\n", maxTrack);
		if (playing)
		{
			Log::write("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		}
		else if (wasPlaying)
		{
			Log::write("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		}
		Log::write("Volume is %f\n", cdvolume);
		return;
	}
}

//==========================================================================
//
//	CDAudio_MessageHandler
//
//==========================================================================

LONG CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (lParam != wDeviceID)
	{
		return 1;
	}

	switch (wParam)
	{
	case MCI_NOTIFY_SUCCESSFUL:
		if (playing)
		{
			playing = false;
			if (playLooping)
			{
				// if the track has played the given number of times,
				// go to the ambient track
				if ((GGameType & GAME_Quake2) && ++loopcounter >= cd_loopcount->value)
				{
					CDAudio_Play2(cd_looptrack->value, true);
				}
				else
				{
					CDAudio_Play2(playTrack, true);
				}
			}
		}
		break;

	case MCI_NOTIFY_ABORTED:
	case MCI_NOTIFY_SUPERSEDED:
		break;

	case MCI_NOTIFY_FAILURE:
		Log::develWrite("MCI_NOTIFY_FAILURE\n");
		CDAudio_Stop();
		cdValid = false;
		break;

	default:
		Log::develWrite("Unexpected MM_MCINOTIFY type (%i)\n", wParam);
		return 1;
	}

	return 0;
}

//==========================================================================
//
//	CDAudio_Update
//
//==========================================================================

void CDAudio_Update()
{
	if (GGameType & GAME_QuakeHexen)
	{
		if (!enabled)
		{
			return;
		}

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
		if (!!cd_nocd->value != !enabled)
		{
			if (cd_nocd->value)
			{
				CDAudio_Stop();
				enabled = false;
			}
			else
			{
				enabled = true;
				CDAudio_Resume();
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
	DWORD dwReturn;
	MCI_OPEN_PARMS mciOpenParms;
	MCI_SET_PARMS mciSetParms;
	int n;

	if (GGameType & GAME_QuakeHexen)
	{
		if (COM_CheckParm("-nocdaudio"))
		{
			return -1;
		}
	}
	else
	{
		cd_nocd = Cvar_Get("cd_nocd", "0", CVAR_ARCHIVE);
		cd_loopcount = Cvar_Get("cd_loopcount", "4", 0);
		cd_looptrack = Cvar_Get("cd_looptrack", "11", 0);
		if (cd_nocd->value)
		{
			return -1;
		}
	}

	mciOpenParms.lpstrDeviceType = "cdaudio";
	if (dwReturn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD)(LPVOID)&mciOpenParms))
	{
		Log::write("CDAudio_Init: MCI_OPEN failed (%i)\n", dwReturn);
		return -1;
	}
	wDeviceID = mciOpenParms.wDeviceID;

	// Set the time format to track/minute/second/frame (TMSF).
	mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
	if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID)&mciSetParms))
	{
		Log::write("MCI_SET_TIME_FORMAT failed (%i)\n", dwReturn);
		mciSendCommand(wDeviceID, MCI_CLOSE, 0, (DWORD)NULL);
		return -1;
	}

	for (n = 0; n < 100; n++)
	{
		remap[n] = n;
	}
	initialized = true;
	enabled = true;

	if (CDAudio_GetAudioDiskInfo())
	{
		Log::write("CDAudio_Init: No CD in player.\n");
		cdValid = false;
		enabled = false;
	}

	Cmd_AddCommand("cd", CD_f);

	Log::write("CD Audio Initialized\n");

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
	if (mciSendCommand(wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)NULL))
	{
		Log::develWrite("CDAudio_Shutdown: MCI_CLOSE failed\n");
	}
}

//==========================================================================
//
//	CDAudio_Activate
//
//	Called when the main window gains or loses focus. The window have been
// destroyed and recreated between a deactivate and an activate.
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
