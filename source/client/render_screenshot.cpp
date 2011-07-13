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

void R_ScreenshotFilename(int lastNumber, char* fileName, const char* Extension)
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

bool R_FindAvailableScreenshotFilename(int& lastNumber, char* fileName, const char* Extension)
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
//	RB_TakeScreenshot
//
//==========================================================================

void RB_TakeScreenshot(int x, int y, int width, int height, const char* fileName, bool IsJpeg)
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

void R_LevelShot()
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
