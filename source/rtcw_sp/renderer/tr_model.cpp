/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

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

// tr_models.c -- model loading and caching

#include "tr_local.h"

/*
** RE_BeginRegistration
*/
void RE_BeginRegistration( glconfig_t *glconfigOut ) {
	ri.Hunk_Clear();    // (SA) MEM NOTE: not in missionpack

	R_Init();
	*glconfigOut = glConfig;

	R_SyncRenderThread();

	tr.viewCluster = -1;        // force markleafs to regenerate
	R_ClearFlares();
	RE_ClearScene();

	tr.registered = qtrue;

	// NOTE: this sucks, for some reason the first stretch pic is never drawn
	// without this we'd see a white flash on a level load because the very
	// first time the level shot would not be drawn
	RE_StretchPic( 0, 0, 0, 0, 0, 0, 1, 1, 0 );
}

//---------------------------------------------------------------------------
// Virtual Memory, used for model caching, since we can't allocate them
// in the main Hunk (since it gets cleared on level changes), and they're
// too large to go into the Zone, we have a special memory chunk just for
// caching models in between levels.
//
// Optimized for Win32 systems, so that they'll grow the swapfile at startup
// if needed, but won't actually commit it until it's needed.
//
// GOAL: reserve a big chunk of virtual memory for the media cache, and only
// use it when we actually need it. This will make sure the swap file grows
// at startup if needed, rather than each allocation we make.
byte    *membase = NULL;
int hunkmaxsize;
int hunkcursize;        //DAJ renamed from cursize

#define R_HUNK_MEGS     24
#define R_HUNK_SIZE     ( R_HUNK_MEGS*1024*1024 )

void *R_Hunk_Begin( void ) {
	int maxsize = R_HUNK_SIZE;

	//Com_Printf("R_Hunk_Begin\n");

	if ( !r_cache->integer ) {
		return NULL;
	}

	// reserve a huge chunk of memory, but don't commit any yet
	hunkcursize = 0;
	hunkmaxsize = maxsize;

#ifdef _WIN32

	// this will "reserve" a chunk of memory for use by this application
	// it will not be "committed" just yet, but the swap file will grow
	// now if needed
	membase = (byte*)VirtualAlloc( NULL, maxsize, MEM_RESERVE, PAGE_NOACCESS );

#elif defined( __MACOS__ )

	return; //DAJ FIXME memory leak

#else   // just allocate it now

	// show_bug.cgi?id=440
	// it is possible that we have been allocated already, in case we don't do anything
	// NOTE: in SP we currently default to r_cache 0, meaning this code is not used at all
	//   I am merging the MP way of doing things though, just in case (since we have r_cache 1 on linux MP)
	if ( !membase ) {
		membase = (byte*)malloc( maxsize );
		// TTimo NOTE: initially, I was doing the memset even if we had an existing membase
		// but this breaks some shaders (i.e. /map mp_beach, then go back to the main menu .. some shaders are missing)
		// I assume the shader missing is because we don't clear memory either on win32
		// meaning even on win32 we are using memory that is still reserved but was uncommited .. it works out of pure luck
		memset( membase, 0, maxsize );
	}

#endif

	if ( !membase ) {
		ri.Error( ERR_DROP, "R_Hunk_Begin: reserve failed" );
	}

	return (void *)membase;
}

void *R_Hunk_Alloc( int size ) {
#ifdef _WIN32
	void    *buf;
#endif

	//Com_Printf("R_Hunk_Alloc(%d)\n", size);

	// round to cacheline
	size = ( size + 31 ) & ~31;

#ifdef _WIN32

	// commit pages as needed
	buf = VirtualAlloc( membase, hunkcursize + size, MEM_COMMIT, PAGE_READWRITE );

	if ( !buf ) {
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPTSTR) &buf, 0, NULL );
		ri.Error( ERR_DROP, "VirtualAlloc commit failed.\n%s", buf );
	}

#elif defined( __MACOS__ )

	return NULL;    //DAJ

#endif

	hunkcursize += size;
	if ( hunkcursize > hunkmaxsize ) {
		ri.Error( ERR_DROP, "R_Hunk_Alloc overflow" );
	}

	return ( void * )( membase + hunkcursize - size );
}

// this is only called when we shutdown GL
void R_Hunk_End( void ) {
	//Com_Printf("R_Hunk_End\n");

	if ( !r_cache->integer ) {
		return;
	}

	if ( membase ) {
#ifdef _WIN32
		VirtualFree( membase, 0, MEM_RELEASE );
#elif defined( __MACOS__ )
		//DAJ FIXME free (membase);
#else
		free( membase );
#endif
	}

	membase = NULL;
}

void R_Hunk_Reset( void ) {
	//Com_Printf("R_Hunk_Reset\n");

	if ( !r_cache->integer ) {
		return;
	}

#if !defined( __MACOS__ )
	if ( !membase ) {
		ri.Error( ERR_DROP, "R_Hunk_Reset called without a membase!" );
	}
#endif

#ifdef _WIN32
	// mark the existing committed pages as reserved, but not committed
	VirtualFree( membase, hunkcursize, MEM_DECOMMIT );
#endif
	// on non win32 OS, we keep the allocated chunk as is, just start again to curzise = 0

	// start again at the top
	hunkcursize = 0;
}

//=============================================================================
// Ridah, model caching

// TODO: convert the Hunk_Alloc's in the model loading to malloc's, so we don't have
// to move so much memory around during transitions

extern model_t backupModels[MAX_MOD_KNOWN];
extern int numBackupModels;

/*
===============
R_CacheModelAlloc
===============
*/
void *R_CacheModelAlloc( int size ) {
	if ( r_cache->integer && r_cacheModels->integer ) {
#if defined( __MACOS__ )
		return malloc( size );      //DAJ FIXME was co
#else
		return R_Hunk_Alloc( size );
#endif
	} else {
		// if r_cache 0, this function is not supposed to get called
		Com_Printf( "FIXME: unexpected R_CacheModelAlloc call with r_cache 0\n" );
		return ri.Hunk_Alloc( size, h_low );
	}
}

/*
===============
R_CacheModelFree
===============
*/
void R_CacheModelFree( void *ptr ) {
	if ( r_cache->integer && r_cacheModels->integer ) {
		// TTimo: it's in the hunk, leave it there, next R_Hunk_Begin will clear it all
#if defined( __MACOS__ )
		free( ptr );    //DAJ FIXME was co
#endif
	} else
	{
		// if r_cache 0, this function is not supposed to get called
		Com_Printf( "FIXME: unexpected R_CacheModelFree call with r_cache 0\n" );
	}
}

/*
===============
R_PurgeModels
===============
*/
void R_PurgeModels( int count ) {
	static int lastPurged = 0;

	//Com_Printf("R_PurgeModels\n");

	if ( !numBackupModels ) {
		return;
	}

	lastPurged = 0;
	numBackupModels = 0;

	// note: we can only do this since we only use the virtual memory for the model caching!
	R_Hunk_Reset();
}

/*
===============
R_BackupModels
===============
*/
void R_BackupModels( void ) {
	int i, j;
	model_t *mod, *modBack;

	//Com_Printf("R_BackupModels\n");

	if ( !r_cache->integer ) {
		return;
	}
	if ( !r_cacheModels->integer ) {
		return;
	}

	if ( numBackupModels ) {
		R_PurgeModels( numBackupModels + 1 ); // get rid of them all
	}

	// copy each model in memory across to the backupModels
	modBack = backupModels;
	for ( i = 0; i < tr.numModels; i++ ) {
		mod = tr.models[i];

		if ( mod->type && mod->type != MOD_BRUSH46 && mod->type != MOD_MDS ) {
			memcpy( modBack, mod, sizeof( *mod ) );
			switch ( mod->type ) {
			case MOD_MESH3:
				for ( j = MD3_MAX_LODS - 1; j >= 0; j-- ) {
					if ( j < mod->q3_numLods && mod->q3_md3[j] ) {
						if ( ( j == MD3_MAX_LODS - 1 ) || ( mod->q3_md3[j] != mod->q3_md3[j + 1] ) ) {
							modBack->q3_md3[j] = (md3Header_t*)R_CacheModelAlloc( mod->q3_md3[j]->ofsEnd );
							memcpy( modBack->q3_md3[j], mod->q3_md3[j], mod->q3_md3[j]->ofsEnd );
						} else {
							modBack->q3_md3[j] = modBack->q3_md3[j + 1];
						}
					}
				}
				break;
			case MOD_MDC:
				for ( j = MD3_MAX_LODS - 1; j >= 0; j-- ) {
					if ( j < mod->q3_numLods && mod->q3_mdc[j] ) {
						if ( ( j == MD3_MAX_LODS - 1 ) || ( mod->q3_mdc[j] != mod->q3_mdc[j + 1] ) ) {
							modBack->q3_mdc[j] = (mdcHeader_t*)R_CacheModelAlloc( mod->q3_mdc[j]->ofsEnd );
							memcpy( modBack->q3_mdc[j], mod->q3_mdc[j], mod->q3_mdc[j]->ofsEnd );
						} else {
							modBack->q3_mdc[j] = modBack->q3_mdc[j + 1];
						}
					}
				}
				break;
			default:
				break; // MOD_BAD, MOD_BRUSH46, MOD_MDS
			}
			modBack++;
			numBackupModels++;
		}
	}
}
