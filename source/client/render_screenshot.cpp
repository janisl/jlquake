//**************************************************************************
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

/* 
============================================================================== 
 
						SCREEN SHOTS 

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

============================================================================== 
*/ 

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_ScreenshotFilename
//
//==========================================================================

static void R_ScreenshotFilename(int lastNumber, char* fileName, const char* Extension)
{
	if (lastNumber < 0 || lastNumber > 9999)
	{
		QStr::Sprintf(fileName, MAX_OSPATH, "screenshots/shot9999.%s", Extension);
		return;
	}
	QStr::Sprintf(fileName, MAX_OSPATH, "screenshots/shot%04i.%s", lastNumber, Extension);
}

//==========================================================================
//
//	R_FindAvailableScreenshotFilename
//
//==========================================================================

static bool R_FindAvailableScreenshotFilename(int& lastNumber, char* fileName, const char* Extension)
{
	// scan for a free number
	for (; lastNumber <= 9999; lastNumber++)
	{
		R_ScreenshotFilename(lastNumber, fileName, Extension);

		if (!FS_FileExists(fileName))
		{
			//	Next time start scanning here.
			lastNumber++;

			return true;	// file doesn't exist
		}
	}

	GLog.Write("ScreenShot: Couldn't create a file\n");
	return false;
}

//==========================================================================
//
//	R_TakeScreenshot
//
//==========================================================================

static void R_TakeScreenshot(int x, int y, int width, int height, const char* name, bool jpeg)
{
	static char fileName[MAX_OSPATH]; // bad things if two screenshots per frame?

	screenshotCommand_t* cmd = (screenshotCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	QStr::NCpyZ(fileName, name, sizeof(fileName));
	cmd->fileName = fileName;
	cmd->jpeg = jpeg;
}

//==========================================================================
//
//	RB_TakeScreenshot
//
//==========================================================================

static void RB_TakeScreenshot(int x, int y, int width, int height, const char* fileName, bool IsJpeg)
{
	byte* buffer = new byte[width * height * 3];

	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	// gamma correct
	if (tr.overbrightBits > 0 && glConfig.deviceSupportsGamma)
	{
		R_GammaCorrect(buffer, width * height * 3);
	}

	if (IsJpeg)
	{
		FS_WriteFile(fileName, buffer, 1);		// create path
		R_SaveJPG(fileName, 95, width, height, buffer);
	}
	else
	{
		R_SaveTGA(fileName, buffer, width, height, false);
	}

	delete[] buffer;
}

//==========================================================================
//
//	RB_TakeScreenshotCmd
//
//==========================================================================

const void* RB_TakeScreenshotCmd(const void* data)
{
	const screenshotCommand_t* cmd = (const screenshotCommand_t*)data;

	RB_TakeScreenshot(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName, cmd->jpeg);

	return (const void*)(cmd + 1);	
}

//==========================================================================
//
//	R_LevelShot
//
//	levelshots are specialized 128*128 thumbnails for the menu system, sampled
// down from full screen distorted images
//
//==========================================================================

static void R_LevelShot()
{
	char checkname[MAX_OSPATH];
	QStr::Sprintf(checkname, MAX_OSPATH, "levelshots/%s.tga", tr.world->baseName);

	byte* source = new byte[glConfig.vidWidth * glConfig.vidHeight * 3];

	byte* buffer = new byte[128 * 128 * 3];

	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, source);

	// resample from source
	float xScale = glConfig.vidWidth / 512.0f;
	float yScale = glConfig.vidHeight / 384.0f;
	for (int y = 0; y < 128; y++)
	{
		for (int x = 0; x < 128; x++)
		{
			int r = 0;
			int g = 0;
			int b = 0;
			for (int yy = 0; yy < 3; yy++)
			{
				for (int xx = 0; xx < 4; xx++)
				{
					byte* src = source + 3 * (glConfig.vidWidth * (int)((y * 3 + yy) * yScale) + (int)((x * 4 + xx) * xScale));
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			byte* dst = buffer + 3 * (y * 128 + x);
			dst[0] = r / 12;
			dst[1] = g / 12;
			dst[2] = b / 12;
		}
	}

	// gamma correct
	if (tr.overbrightBits > 0 && glConfig.deviceSupportsGamma)
	{
		R_GammaCorrect(buffer, 128 * 128 * 3);
	}

	R_SaveTGA(checkname, buffer, 128, 128, false);

	delete[] buffer;
	delete[] source;

	GLog.Write("Wrote %s\n", checkname);
}

//==========================================================================
//
//	R_ScreenShot_f
//
//	screenshot
//	screenshot [silent]
//	screenshot [levelshot]
//	screenshot [filename]
//
//	Doesn't print the pacifier message if specified silent
//
//==========================================================================

void R_ScreenShot_f()
{
	// if we have saved a previous screenshot, don't scan
	// again, because recording demo avis can involve
	// thousands of shots
	static int lastNumber = 0;

	if (!QStr::Cmp(Cmd_Argv(1), "levelshot"))
	{
		R_LevelShot();
		return;
	}

	bool silent = !QStr::Cmp(Cmd_Argv(1), "silent");

	char checkname[MAX_OSPATH];
	if (Cmd_Argc() == 2 && !silent)
	{
		// explicit filename
		QStr::Sprintf(checkname, MAX_OSPATH, "screenshots/%s.tga", Cmd_Argv(1));
	}
	else
	{
		// scan for a free filename
		if (!R_FindAvailableScreenshotFilename(lastNumber, checkname, "tga"))
		{
			return;
		}
	}

	if (GGameType & GAME_Quake3)
	{
		R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, false);
	}
	else
	{
		RB_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, false);
	}

	if (!silent)
	{
		GLog.Write("Wrote %s\n", checkname);
	}
} 

//==========================================================================
//
//	R_ScreenShotJPEG_f
//
//==========================================================================

void R_ScreenShotJPEG_f()
{
	// if we have saved a previous screenshot, don't scan
	// again, because recording demo avis can involve
	// thousands of shots
	static int lastNumber = 0;

	if (!QStr::Cmp(Cmd_Argv(1), "levelshot"))
	{
		R_LevelShot();
		return;
	}

	bool silent = !QStr::Cmp(Cmd_Argv(1), "silent");

	char checkname[MAX_OSPATH];
	if (Cmd_Argc() == 2 && !silent)
	{
		// explicit filename
		QStr::Sprintf(checkname, MAX_OSPATH, "screenshots/%s.jpg", Cmd_Argv(1));
	}
	else
	{
		// scan for a free filename
		if (!R_FindAvailableScreenshotFilename(lastNumber, checkname, "jpg"))
		{
			return;
 		}
	}

	if (GGameType & GAME_Quake3)
	{
		R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, true);
	}
	else
	{
		R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, true);
	}

	if (!silent)
	{
		GLog.Write("Wrote %s\n", checkname);
	}
} 
