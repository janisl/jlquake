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

#include "../client.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

QCinematicPlayer*	cinTable[MAX_VIDEO_HANDLES];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

#if 0
//==========================================================================
//
//	CIN_MakeFullName
//
//==========================================================================

void CIN_MakeFullName(const char* Name, char* FullName)
{
	const char* Dot = strstr(Name, ".");
	if (Dot && !String::ICmp(Dot, ".pcx"))
	{
		// static pcx image
		String::Sprintf(FullName, MAX_QPATH, "pics/%s", Name);
	}
	if (strstr(Name, "/") == NULL && strstr(Name, "\\") == NULL)
	{
		String::Sprintf(FullName, MAX_QPATH, "video/%s", Name);
	}
	else
	{
		String::Sprintf(FullName, MAX_QPATH, "%s", Name);
	}
}

//==========================================================================
//
//	CIN_Open
//
//==========================================================================

QCinematic* CIN_Open(const char* Name)
{
	const char* dot = strstr(Name, ".");
	if (dot && !String::ICmp(dot, ".pcx"))
	{
		// static pcx image
		QCinematicPcx* Cin = new QCinematicPcx();
		if (!Cin->Open(Name))
		{
			delete Cin;
			return NULL;
		}
		return Cin;
	}

	if (dot && !String::ICmp(dot, ".cin"))
	{
		QCinematicCin* Cin = new QCinematicCin();
		if (!Cin->Open(Name))
		{
			delete Cin;
			return NULL;
		}
		return Cin;
	}

	QCinematicRoq* Cin = new QCinematicRoq();
	if (!Cin->Open(Name))
	{
		delete Cin;
		return NULL;
	}
	return Cin;
}

//==========================================================================
//
//	CIN_HandleForVideo
//
//==========================================================================

int CIN_HandleForVideo()
{
	for (int i = 0; i < MAX_VIDEO_HANDLES; i++)
	{
		if (!cinTable[i])
		{
			return i;
		}
	}
	throw DropException("CIN_HandleForVideo: none free");
}

//==========================================================================
//
//	CIN_PlayCinematic
//
//==========================================================================

int CIN_PlayCinematic(const char* arg, int x, int y, int w, int h, int systemBits)
{
	char name[MAX_OSPATH];
	CIN_MakeFullName(arg, name);

	if (!(systemBits & CIN_system))
	{
		for (int i = 0; i < MAX_VIDEO_HANDLES; i++ )
		{
			if (cinTable[i] && !String::Cmp(cinTable[i]->Cin->Name, name))
			{
				return i;
			}
		}
	}

	Log::develWrite("SCR_PlayCinematic( %s )\n", arg);

	int Handle = CIN_HandleForVideo();

	QCinematic* Cin = CIN_Open(name);

	if (!Cin)
	{
		if (systemBits & CIN_system)
		{
			CIN_FinishCinematic();
		}
		return -1;
	}

	cinTable[Handle] = new QCinematicPlayer(Cin, x, y, w, h, systemBits);

	if (cinTable[Handle]->AlterGameState)
	{
		CIN_StartedPlayback();
	}

	Log::develWrite("trFMV::play(), playing %s\n", arg);

	return Handle;
}

//==========================================================================
//
//	CIN_RunCinematic
//
//==========================================================================

e_status CIN_RunCinematic(int handle)
{
	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || !cinTable[handle])
	{
		return FMV_EOF;
	}

	e_status Status = cinTable[handle]->Run();

	if (Status == FMV_EOF)
	{
		delete cinTable[handle];
		cinTable[handle] = NULL;
	}

	return Status;
}

//==========================================================================
//
//	CIN_UploadCinematic
//
//==========================================================================

void CIN_UploadCinematic(int handle)
{
	if (handle >= 0 && handle < MAX_VIDEO_HANDLES && cinTable[handle])
	{
		cinTable[handle]->Upload(handle);
	}
}
#endif

//==========================================================================
//
//	QCinematicPlayer::~QCinematicPlayer
//
//==========================================================================

QCinematicPlayer::~QCinematicPlayer()
{
	if (Cin)
	{
		delete Cin;
		Cin = NULL;
	}
}

//==========================================================================
//
//	QCinematicPlayer::SetExtents
//
//==========================================================================

void QCinematicPlayer::SetExtents(int x, int y, int w, int h)
{
	XPos = x;
	YPos = y;
	Width = w;
	Height = h;
	Cin->Dirty = true;
}

//==========================================================================
//
//	QCinematicPlayer::Run
//
//==========================================================================

e_status QCinematicPlayer::Run()
{
	if (PlayOnWalls < -1)
	{
		return Status;
	}

	if (AlterGameState)
	{
		if (!CIN_IsInCinematicState())
		{
			return Status;
		}
	}

	if (Status == FMV_IDLE)
	{
		return Status;
	}

	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	int thisTime = CL_ScaledMilliseconds();
	if (Shader && (abs(thisTime - (int)LastTime)) > 100)
	{
		StartTime += thisTime - LastTime;
	}
	if (!Cin->Update(CL_ScaledMilliseconds() - StartTime))
	{ 
		if (!HoldAtEnd)
		{
			if (Looping)
			{
				Reset();
			}
			else
			{
				Status = FMV_EOF;
			}
		}
		else
		{
			Status = FMV_IDLE;
		}
	}

	LastTime = thisTime;

	if (Status == FMV_LOOPED)
	{
		Status = FMV_PLAY;
	}

	if (Status == FMV_EOF)
	{
		if (Looping)
		{
			Reset();
		}
		else
		{
			Log::develWrite("finished cinematic\n");
			if (AlterGameState)
			{
				CIN_FinishCinematic();
			}
		}
	}
	return Status;
}

//==========================================================================
//
//	QCinematicPlayer::Reset
//
//==========================================================================

void QCinematicPlayer::Reset()
{
	Cin->Reset();

	StartTime = LastTime = CL_ScaledMilliseconds();

	Status = FMV_LOOPED;
}

//==========================================================================
//
//	QCinematicPlayer::Upload
//
//==========================================================================

void QCinematicPlayer::Upload(int Handle)
{
	if (!Cin->OutputFrame)
	{
		return;
	}
	if (PlayOnWalls <= 0 && Cin->Dirty)
	{
		if (PlayOnWalls == 0)
		{
			PlayOnWalls = -1;
		}
		else if (PlayOnWalls == -1)
		{
			PlayOnWalls = -2;
		}
		else
		{
			Cin->Dirty = false;
		}
	}
	R_UploadCinematic(256, 256, Cin->OutputFrame, Handle, Cin->Dirty);
	if (cl_inGameVideo->integer == 0 && PlayOnWalls == 1)
	{
		PlayOnWalls--;
	}
}
