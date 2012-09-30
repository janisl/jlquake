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

#include "../client.h"
#include "local.h"
#include "../sound/local.h"

#define LETTERBOX_OFFSET 105

#define MAX_VIDEO_HANDLES   16

static QCinematicPlayer* cinTable[MAX_VIDEO_HANDLES];
static int CL_handle = -1;

static void CIN_MakeFullName(const char* Name, char* FullName)
{
	const char* Dot = strstr(Name, ".");
	if (Dot && !String::ICmp(Dot, ".pcx"))
	{
		// static pcx image
		String::Sprintf(FullName, MAX_QPATH, "pics/%s", Name);
	}
	else if (strstr(Name, "/") == NULL && strstr(Name, "\\") == NULL)
	{
		String::Sprintf(FullName, MAX_QPATH, "video/%s", Name);
	}
	else
	{
		String::Sprintf(FullName, MAX_QPATH, "%s", Name);
	}
}

static QCinematic* CIN_Open(const char* Name)
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

static int CIN_HandleForVideo()
{
	for (int i = 0; i < MAX_VIDEO_HANDLES; i++)
	{
		if (!cinTable[i])
		{
			return i;
		}
	}
	common->Error("CIN_HandleForVideo: none free");
	return 0;
}

static void CIN_StartedPlayback()
{
	if (GGameType & GAME_Quake2)
	{
		SCR_EndLoadingPlaque();

		cls.state = CA_ACTIVE;
	}
	else
	{
		// close the menu
		UI_ForceMenuOff();

		cls.state = CA_CINEMATIC;

		Con_Close();

		s_rawend[CIN_STREAM] = s_soundtime;
	}
}

//	Called when either the cinematic completes, or it is aborted
static void CIN_FinishCinematic()
{
	if (GGameType & GAME_Quake2)
	{
		// tell the server to advance to the next map / cinematic
		CL_AddReliableCommand(va("nextserver %i\n", cl.servercount));
		SCRQ2_BeginLoadingPlaque(true);
	}
	else
	{
		cls.state = CA_DISCONNECTED;
		// we can't just do a vstr nextmap, because
		// if we are aborting the intro cinematic with
		// a devmap command, nextmap would be valid by
		// the time it was referenced
		const char* s = Cvar_VariableString("nextmap");
		if (s[0])
		{
			Cbuf_ExecuteText(EXEC_APPEND, va("%s\n", s));
			Cvar_Set("nextmap", "");
		}
	}
	CL_handle = -1;
}

static int CIN_PlayCinematic(const char* arg, int x, int y, int w, int h, int systemBits)
{
	char name[MAX_OSPATH];
	CIN_MakeFullName(arg, name);

	if (!(systemBits & CIN_system))
	{
		for (int i = 0; i < MAX_VIDEO_HANDLES; i++)
		{
			if (cinTable[i] && !String::ICmp(cinTable[i]->Cin->Name, name))
			{
				return i;
			}
		}
	}

	common->DPrintf("SCR_PlayCinematic( %s )\n", arg);

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

	common->DPrintf("trFMV::play(), playing %s\n", arg);

	return Handle;
}

//  For QVMs all drawing is done to a 640*480 virtual screen size,
// it means it will be stretched on widescreen displays.
static void CIN_AdjustToVirtualScreen(int& x, int& y, int& w, int& h)
{
	// scale for screen sizes
	float xscale = (float)viddef.width / 640;
	float yscale = (float)viddef.height / 480;
	x *= xscale;
	y *= yscale;
	w *= xscale;
	h *= yscale;
}

int CIN_PlayCinematicStretched(const char* arg, int x, int y, int w, int h, int systemBits)
{
	CIN_AdjustToVirtualScreen(x, y, w, h);
	return CIN_PlayCinematic(arg, x, y, w, h, systemBits);
}

void CIN_SetExtents(int handle, int x, int y, int w, int h)
{
	if (handle < 0 || handle >= MAX_VIDEO_HANDLES || !cinTable[handle])
	{
		return;
	}
	CIN_AdjustToVirtualScreen(x, y, w, h);
	cinTable[handle]->SetExtents(x, y, w, h);
}

e_status CIN_RunCinematic(int handle)
{
	if (handle < 0 || handle >= MAX_VIDEO_HANDLES || !cinTable[handle])
	{
		return FMV_EOF;
	}

	e_status Status = cinTable[handle]->Run();

	if (Status == FMV_EOF)
	{
		delete cinTable[handle];
		cinTable[handle] = NULL;
		Status = FMV_IDLE;
	}

	return Status;
}

void CIN_UploadCinematic(int handle)
{
	if (handle >= 0 && handle < MAX_VIDEO_HANDLES && cinTable[handle])
	{
		cinTable[handle]->Upload(handle);
	}
}

void CIN_DrawCinematic(int handle)
{
	if (handle < 0 || handle >= MAX_VIDEO_HANDLES || !cinTable[handle])
	{
		return;
	}
	cinTable[handle]->Draw(handle);
}

e_status CIN_StopCinematic(int handle)
{
	if (handle < 0 || handle >= MAX_VIDEO_HANDLES || !cinTable[handle])
	{
		return FMV_EOF;
	}

	common->DPrintf("trFMV::stop(), closing %s\n", cinTable[handle]->Cin->Name);

	if (GGameType & GAME_Tech3 && cinTable[handle]->AlterGameState)
	{
		if (cls.state != CA_CINEMATIC)
		{
			return cinTable[handle]->Status;
		}
		CIN_FinishCinematic();
	}

	delete cinTable[handle];
	cinTable[handle] = NULL;

	return FMV_EOF;
}

void SCR_PlayCinematic(const char* arg)
{
	// make sure CD isn't playing music
	CDAudio_Stop();

	CL_handle = CIN_PlayCinematic(arg, 0, 0, viddef.width, viddef.height, CIN_system);
}

void SCR_RunCinematic()
{
	if (CL_handle < 0 || CL_handle >= MAX_VIDEO_HANDLES)
	{
		return;
	}

	if (GGameType & GAME_Quake2 && in_keyCatchers != 0)
	{
		// pause if menu or console is up
		cinTable[CL_handle]->StartTime = cls.realtime - cinTable[CL_handle]->Cin->GetCinematicTime();
		return;
	}

	CIN_RunCinematic(CL_handle);
}

//	Returns true if a cinematic is active, meaning the view rendering
// should be skipped
bool SCR_DrawCinematic()
{
	if (CL_handle < 0 || CL_handle >= MAX_VIDEO_HANDLES || !cinTable[CL_handle])
	{
		return false;
	}

	if (GGameType & GAME_Quake2 && in_keyCatchers & KEYCATCH_UI)
	{
		// pause if menu is up
		return true;
	}
	CIN_DrawCinematic(CL_handle);
	return true;
}

void SCR_StopCinematic()
{
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES)
	{
		CIN_StopCinematic(CL_handle);
		S_StopAllSounds();
		CL_handle = -1;
	}
}

void CIN_SkipCinematic()
{
	if (CL_handle < 0 || CL_handle >= MAX_VIDEO_HANDLES || !cinTable[CL_handle])
	{
		return;
	}
	if (!cl.q2_attractloop && cls.realtime - cinTable[CL_handle]->StartTime > 1000)
	{
		// skip the rest of the cinematic
		CIN_StopCinematic(CL_handle);
		CIN_FinishCinematic();
	}
}

void CIN_CloseAllVideos()
{
	for (int i = 0; i < MAX_VIDEO_HANDLES; i++)
	{
		if (cinTable[i])
		{
			CIN_StopCinematic(i);
		}
	}
}

void CL_PlayCinematic_f()
{
	// Arnout: don't allow this while on server
	if (cls.state > CA_DISCONNECTED && cls.state <= CA_ACTIVE)
	{
		return;
	}

	common->DPrintf("CL_PlayCinematic_f\n");
	if (cls.state == CA_CINEMATIC)
	{
		SCR_StopCinematic();
	}

	const char* arg = Cmd_Argv(1);
	const char* s = Cmd_Argv(2);

	int bits = CIN_system;
	if ((s && s[0] == '1') || String::ICmp(arg,"demoend.roq") == 0 || String::ICmp(arg,"end.roq") == 0)
	{
		bits |= CIN_hold;
	}
	if (s && s[0] == '2')
	{
		bits |= CIN_loop;
	}
	if (s && s[0] == '3')
	{
		bits |= CIN_letterBox;
	}

	S_StopAllSounds();
	if (GGameType & GAME_WolfSP)
	{
		// make sure volume is up for cine
		S_FadeAllSounds(1, 0, false);
	}

	if (bits & CIN_letterBox)
	{
		CL_handle = CIN_PlayCinematic(arg, 0, LETTERBOX_OFFSET, viddef.width, viddef.height - (LETTERBOX_OFFSET * 2), bits);
	}
	else
	{
		CL_handle = CIN_PlayCinematic(arg, 0, 0, viddef.width, viddef.height, bits);
	}

	if (CL_handle >= 0)
	{
		do
		{
			SCR_RunCinematic();
		}
		while (cinTable[CL_handle]->Cin->OutputFrame == NULL && cinTable[CL_handle]->Status == FMV_PLAY);		// wait for first frame (load codebook and sound)
	}
}

QCinematicPlayer::~QCinematicPlayer()
{
	if (Cin)
	{
		delete Cin;
		Cin = NULL;
	}
}

void QCinematicPlayer::SetExtents(int x, int y, int w, int h)
{
	XPos = x;
	YPos = y;
	Width = w;
	Height = h;
	Cin->Dirty = true;
}

e_status QCinematicPlayer::Run()
{
	if (PlayOnWalls < -1)
	{
		return Status;
	}

	if (AlterGameState)
	{
		if (GGameType & GAME_Tech3 && cls.state != CA_CINEMATIC)
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
			common->DPrintf("finished cinematic\n");
			if (AlterGameState)
			{
				CIN_FinishCinematic();
			}
		}
	}
	return Status;
}

void QCinematicPlayer::Reset()
{
	Cin->Reset();

	StartTime = LastTime = CL_ScaledMilliseconds();

	Status = FMV_LOOPED;
}

void QCinematicPlayer::Upload(int handle)
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
	R_UploadCinematic(256, 256, Cin->OutputFrame, handle, Cin->Dirty);
	if (cl_inGameVideo->integer == 0 && PlayOnWalls == 1)
	{
		PlayOnWalls--;
	}
}

void QCinematicPlayer::Draw(int handle)
{
	byte* buf = Cin->OutputFrame;
	if (!buf)
	{
		return;
	}

	float x = XPos;
	float y = YPos;
	float w = Width;
	float h = Height;
	UI_AdjustFromVirtualScreen(&x, &y, &w, &h);

	if (letterBox)
	{
		float vh = (float)cls.glconfig.vidHeight;
		float barheight = ((float)LETTERBOX_OFFSET / 480.0f) * vh;

		vec4_t colorBlack  = {0, 0, 0, 1};
		R_SetColor(&colorBlack[0]);
		R_StretchPic(0, 0, w, barheight, 0, 0, 0, 0, cls.whiteShader);
		R_StretchPic(0, vh - barheight - 1, w, barheight + 1, 0, 0, 0, 0, cls.whiteShader);
	}

	R_StretchRaw(x, y, w, h, Cin->Width, Cin->Height, buf, handle, Cin->Dirty);
	Cin->Dirty = false;
}
