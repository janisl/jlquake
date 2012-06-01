/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_aas_main.c
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ContinueInit(float time)
{
	//if no AAS file loaded
	if (!aasworld->loaded)
	{
		return;
	}
	//if AAS is already initialized
	if (aasworld->initialized)
	{
		return;
	}
	//calculate reachability, if not finished return
	if (AAS_ContinueInitReachability(time))
	{
		return;
	}
	//initialize clustering for the new map
	AAS_InitClustering();
	//if reachability has been calculated and an AAS file should be written
	//or there is a forced data optimization
	if (aasworld->savefile || ((int)LibVarGetValue("forcewrite")))
	{
		//optimize the AAS data
		if (!((int)LibVarValue("nooptimize", "1")))
		{
			AAS_Optimize();
		}
		//save the AAS file
		if (AAS_WriteAASFile(aasworld->filename))
		{
			BotImport_Print(PRT_MESSAGE, "%s written succesfully\n", aasworld->filename);
		}	//end if
		else
		{
			BotImport_Print(PRT_ERROR, "couldn't write %s\n", aasworld->filename);
		}	//end else
	}	//end if
		//initialize the routing
	AAS_InitRouting();
	//at this point AAS is initialized
	AAS_SetInitialized();
}	//end of the function AAS_ContinueInit
//===========================================================================
// called at the start of every frame
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_StartFrame(float time)
{
	// Ridah, do each of the aasworlds
	int i;

	for (i = 0; i < MAX_AAS_WORLDS; i++)
	{
		AAS_SetCurrentWorld(i);

		aasworld->time = time;
		//invalidate the entities
		AAS_InvalidateEntities();
		//initialize AAS
		AAS_ContinueInit(time);
		//
		aasworld->frameroutingupdates = 0;
		//
		/* Ridah, disabled for speed
		if (LibVarGetValue("showcacheupdates"))
		{
		    AAS_RoutingInfo();
		    LibVarSet("showcacheupdates", "0");
		} //end if
		*/
	}	//end if
	aasworld->numframes++;
	return BLERR_NOERROR;
}	//end of the function AAS_StartFrame
//===========================================================================
// called everytime a map changes
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================

// Ridah, modified this for multiple AAS files

int AAS_LoadMap(const char* mapname)
{
	int errnum;
	int i;
	char this_mapname[256], intstr[4];
	qboolean loaded = qfalse;
	int missingErrNum = 0;		// TTimo: init

	for (i = 0; i < MAX_AAS_WORLDS; i++)
	{
		AAS_SetCurrentWorld(i);

		String::NCpy(this_mapname, mapname, 256);
		strncat(this_mapname, "_b", 256);
		sprintf(intstr, "%i", i);
		strncat(this_mapname, intstr, 256);

		//if no mapname is provided then the string indexes are updated
		if (!mapname)
		{
			return 0;
		}	//end if
			//
		aasworld->initialized = qfalse;
		//NOTE: free the routing caches before loading a new map because
		// to free the caches the old number of areas, number of clusters
		// and number of areas in a clusters must be available
		AAS_FreeRoutingCaches();
		//load the map
		errnum = AAS_LoadFiles(this_mapname);
		if (errnum != BLERR_NOERROR)
		{
			aasworld->loaded = qfalse;
			// RF, we are allowed to skip one of the files, but not both
			//return errnum;
			missingErrNum = errnum;
			continue;
		}	//end if
			//
		loaded = qtrue;
		//
		AAS_InitSettings();
		//initialize the AAS link heap for the new map
		AAS_InitAASLinkHeap();
		//initialize the AAS linked entities for the new map
		AAS_InitAASLinkedEntities();
		//initialize reachability for the new map
		AAS_InitReachability();
		//initialize the alternative routing
		AAS_InitAlternativeRouting();
	}

	if (!loaded)
	{
		return missingErrNum;
	}

	//everything went ok
	return 0;
}	//end of the function AAS_LoadMap
