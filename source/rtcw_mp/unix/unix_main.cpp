/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#ifdef __linux__ // rb010123
  #include <mntent.h>
#endif
#include <dlfcn.h>

#ifdef __linux__
  #include <fpu_control.h> // bk001213 - force dumps on divide by zero
#endif

// FIXME TTimo should we gard this? most *nix system should comply?
#include <termios.h>

#include "../client/client.h"
#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderer/tr_public.h"
#include "../../core/system_unix.h"

#include "linux_local.h" // bk001204

unsigned sys_frame_time;

uid_t saved_euid;

// =======================================================================
// General routines
// =======================================================================

// bk001207
#define MEM_THRESHOLD 96 * 1024 * 1024

/*
==================
Sys_LowPhysicalMemory()
==================
*/
qboolean Sys_LowPhysicalMemory() {
	//MEMORYSTATUS stat;
	//GlobalMemoryStatus (&stat);
	//return (stat.dwTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
	return qfalse; // bk001207 - FIXME
}

void Sys_BeginProfiling( void ) {
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void ) {
	IN_Shutdown();
	IN_Init();
}

// =============================================================
// general sys routines
// =============================================================

#define MAX_CMD 1024
static char exit_cmdline[MAX_CMD] = "";
void Sys_DoStartProcess( char *cmdline );

// single exit point (regular exit or in case of signal fault)
void Sys_Exit( int ex ) {
	Sys_ConsoleInputShutdown();

	// we may be exiting to spawn another process
	if ( exit_cmdline[0] != '\0' ) {
		// possible race conditions?
		// buggy kernels / buggy GL driver, I don't know for sure
		// but it's safer to wait an eternity before and after the fork
		sleep( 1 );
		Sys_DoStartProcess( exit_cmdline );
		sleep( 1 );
	}

#ifdef NDEBUG // regular behavior

	// We can't do this
	//  as long as GL DLL's keep installing with atexit...
	//exit(ex);
	_exit( ex );
#else

	// Give me a backtrace on error exits.
	assert( ex == 0 );
	exit( ex );
#endif
}


void Sys_Quit( void ) {
	CL_Shutdown();
	fcntl( 0, F_SETFL, fcntl( 0, F_GETFL, 0 ) & ~FNDELAY );
	Sys_Exit( 0 );
}

void Sys_Init( void ) {
	Cmd_AddCommand( "in_restart", Sys_In_Restart_f );

#if defined __linux__
#if defined __i386__
	Cvar_Set( "arch", "linux i386" );
#elif defined __x86_64__
	Cvar_Set( "arch", "linux x86_64" );
#elif defined __alpha__
	Cvar_Set( "arch", "linux alpha" );
#elif defined __sparc__
	Cvar_Set( "arch", "linux sparc" );
#elif defined __FreeBSD__

#if defined __i386__ // FreeBSD
	Cvar_Set( "arch", "freebsd i386" );
#elif defined __alpha__
	Cvar_Set( "arch", "freebsd alpha" );
#else
	Cvar_Set( "arch", "freebsd unknown" );
#endif // FreeBSD

#else
	Cvar_Set( "arch", "linux unknown" );
#endif
#elif defined __sun__
#if defined __i386__
	Cvar_Set( "arch", "solaris x86" );
#elif defined __sparc__
	Cvar_Set( "arch", "solaris sparc" );
#else
	Cvar_Set( "arch", "solaris unknown" );
#endif
#elif defined __sgi__
#if defined __mips__
	Cvar_Set( "arch", "sgi mips" );
#else
	Cvar_Set( "arch", "sgi unknown" );
#endif
#else
	Cvar_Set( "arch", "unknown" );
#endif

	Cvar_Set( "username", Sys_GetCurrentUser() );

	IN_Init();

}

void Sys_Error( const char *error, ... ) {
	va_list argptr;
	char string[1024];

	// change stdin to non blocking
	// NOTE TTimo not sure how well that goes with tty console mode
	fcntl( 0, F_SETFL, fcntl( 0, F_GETFL, 0 ) & ~FNDELAY );

	// don't bother do a show on this one heh
	if ( ttycon_on ) {
		tty_Hide();
	}

	CL_Shutdown();

	va_start( argptr,error );
	vsprintf( string,error,argptr );
	va_end( argptr );
	fprintf( stderr, "Sys_Error: %s\n", string );

	Sys_Exit( 1 ); // bk010104 - use single exit point.
}

void Sys_Warn( char *warning, ... ) {
	va_list argptr;
	char string[1024];

	va_start( argptr,warning );
	vsprintf( string,warning,argptr );
	va_end( argptr );

	if ( ttycon_on ) {
		tty_Hide();
	}

	fprintf( stderr, "Warning: %s", string );

	if ( ttycon_on ) {
		tty_Show();
	}
}

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime( char *path ) {
	struct  stat buf;

	if ( stat( path,&buf ) == -1 ) {
		return -1;
	}

	return buf.st_mtime;
}

// NOTE TTimo: huh?
void floating_point_exception_handler( int whatever ) {
	signal( SIGFPE, floating_point_exception_handler );
}

/*****************************************************************************/

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
	// bk001206 - verbose error reporting
	const char* err; // rb010123 - now const
	if ( !dllHandle ) {
		Com_Printf( "Sys_UnloadDll(NULL)\n" );
		return;
	}
	dlclose( dllHandle );
	err = dlerror();
	if ( err != NULL ) {
		Com_Printf( "Sys_UnloadGame failed on dlclose: \"%s\"!\n", err );
	}
}


/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine

NOTE: DLL checksuming / DLL pk3 and search paths

if we are connecting to an sv_pure server, the .so will be extracted
from a pk3 and placed in the homepath (i.e. the write area)

since DLLs are looked for in several places, it is important that
we are sure we are going to load the DLL that was just extracted

for convenience, debug builds are searching in current directory
prior to scanning homepath and basepath. this allows tu run stuff
'out of the box' with a +set fs_basepath after compile.

in release we start searching directly in fs_homepath then fs_basepath
(that makes sure we are going to load the DLL we extracted from pk3)
this should not be a problem for mod makers, since they always copy their
DLL to the main installation prior to run (sv_pure 1 would have a tendency
to kill their compiled DLL with extracted ones though :-( )

NOTE T.Ray:
fqpath param added 2/15/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
fqpath will be empty if dll not loaded, otherwise will hold fully qualified path of dll module loaded
fqpath buffersize must be at least MAX_QPATH+1 bytes long
=================
*/

/*
=================
Try_CopyDLLForMod
little utility function for media-only mods
tries to copy a reference DLL to the mod's fs_homepath
return false if failed, or if we are not in a mod situation
returns true if successful, *p_fn is set to the correct path
this is used when we are loading a mod and realize we don't have the DLL in the standard path
=================
*/
qboolean CopyDLLForMod( char **p_fn, const char* gamedir, const char *pwdpath, const char  *homepath, const char *basepath, const char *fname ) {
	char *fn = *p_fn;

	// this may be a media only mod, so next we need to search in the basegame
	if ( String::Length( gamedir ) && String::ICmp( gamedir, BASEGAME ) ) {
		// walk for a base file
		// NOTE TTimo: we don't attempt to validate version-wise, it could be a problem
		// (acceptable tradeoff I say, should not cause major issues)
#ifndef NDEBUG
		fn = FS_BuildOSPath( pwdpath, BASEGAME, fname );
		if ( access( fn, R_OK ) == -1 ) {
#endif
		fn = FS_BuildOSPath( homepath, BASEGAME, fname );
		if ( access( fn, R_OK ) == -1 ) {
			fn = FS_BuildOSPath( basepath, BASEGAME, fname );
			if ( access( fn, R_OK ) == -1 ) {
				return qfalse; // this one is hopeless
			}
		}
#ifndef NDEBUG
	}
#endif
		// basefile found, we copy to homepath in all cases
		// fortunately FS_BuildOSPath does a flip flop, we have 'from path' in fn and 'to path' in *p_fn
		*p_fn = FS_BuildOSPath( homepath, gamedir, fname );
		// copy to destination
		FS_CopyFile( fn, *p_fn );
		if ( access( *p_fn, R_OK ) == -1 ) { // could do with FS_CopyFile returning a boolean actually
			Com_DPrintf( "Copy operation failed\n" );
			return qfalse;
		}
		return qtrue;
	} else
	{
		return qfalse;
	}
}

// TTimo - Wolf MP specific, adding .mp. to shared objects
char* Sys_GetDLLName( const char *name ) {
#if defined __i386__
	return va( "%s.mp.i386.so", name );
#elif defined __x86_64__
	return va( "%s.mp.x86_64.so", name );
#elif defined __powerpc__
	return va( "%s.mp.ppc.so", name );
#elif defined __axp__
	return va( "%s.mp.axp.so", name );
#elif defined __mips__
	return va( "%s.mp.mips.so", name );
#else
#error Unknown arch
#endif
}

void *Sys_LoadDll( const char *name, char *fqpath,
				   qintptr ( **entryPoint ) ( int, ... ),
				   qintptr ( *systemcalls )( int, ... ) ) {
	void *libHandle;
	void ( *dllEntry )( qintptr ( *syscallptr )( int, ... ) );
	char fname[MAX_OSPATH];
	const char  *pwdpath;
	const char  *homepath;
	const char  *basepath;
	const char  *gamedir;
	char  *fn;
	const char*  err = NULL; // bk001206 // rb0101023 - now const
#if !defined( DEDICATED )
	char *cvar_name = NULL;
#endif

	*fqpath = 0 ;       // added 2/15/02 by T.Ray

	// bk001206 - let's have some paranoia
	assert( name );

	String::NCpyZ( fname, Sys_GetDLLName( name ), sizeof( fname ) );

// bk001129 - was RTLD_LAZY
#define Q_RTLD    RTLD_NOW

	pwdpath = Sys_Cwd();
	homepath = Cvar_VariableString( "fs_homepath" );
	basepath = Cvar_VariableString( "fs_basepath" );
	gamedir = Cvar_VariableString( "fs_game" );

	// this is relevant to client only
	// this code is in for full client hosting a game, but it's not affected by it
#if !defined( DEDICATED )
	// do a first scan to identify what we are going to dlopen
	// we need to pass this to FS_ExtractFromPakFile so that it checksums the right file
	// NOTE: if something fails (not found, or file operation failed), we will ERR_FATAL (in the checksum itself, we only ERR_DROP)
#ifndef NDEBUG
	fn = FS_BuildOSPath( pwdpath, gamedir, fname );
	if ( access( fn, R_OK ) == -1 ) {
#endif
	fn = FS_BuildOSPath( homepath, gamedir, fname );
	if ( access( fn, R_OK ) == 0 ) {
		// there is a .so in fs_homepath, but is it a valid one version-wise?
		// we use a persistent variable in config.cfg to make sure
		// this is set in FS_CL_ExtractFromPakFile when the file is extracted
		Cvar *lastVersion;
		cvar_name = va( "cl_lastVersion%s", name );
		lastVersion = Cvar_Get( cvar_name, "(uninitialized)", CVAR_ARCHIVE );
		if ( String::ICmp( Cvar_VariableString( "version" ), lastVersion->string ) ) {
			Com_DPrintf( "clearing non matching version of %s .so: %s\n", name, fn );
			if ( remove( fn ) == -1 ) {
				Com_Error( ERR_FATAL, "failed to remove outdated '%s' file:\n\"%s\"\n", fn, strerror( errno ) );
			}
			// we cancelled fs_homepath, go work on basepath now
			fn = FS_BuildOSPath( basepath, gamedir, fname );
			if ( access( fn, R_OK ) == -1 ) {
				// we may be dealing with a media-only mod, check wether we can find 'reference' DLLs and copy them over
				if ( !CopyDLLForMod( &fn, gamedir, pwdpath, homepath, basepath, fname ) ) {
					Com_Error( ERR_FATAL, "Sys_LoadDll(%s) failed, no corresponding .so file found in fs_homepath or fs_basepath\n", name );
				}
			}
		}
		// the .so in fs_homepath is valid version-wise .. FS_CL_ExtractFromPakFile will have to decide wether it's valid pk3-wise later
	} else {
		fn = FS_BuildOSPath( basepath, gamedir, fname );
		if ( access( fn, R_OK ) == -1 ) {
			// we may be dealing with a media-only mod, check wether we can find 'reference' DLLs and copy them over
			if ( !CopyDLLForMod( &fn, gamedir, pwdpath, homepath, basepath, fname ) ) {
				Com_Error( ERR_FATAL, "Sys_LoadDll(%s) failed, no corresponding .so file found in fs_homepath or fs_basepath\n", name );
			}
		}
	}
#ifndef NDEBUG
}
#endif

	// NERVE - SMF - extract dlls from pak file for security
	// we have to handle the game dll a little differently
	// NOTE #2: we may have found a file in fs_basepath, and if the checksum is wrong, FS_Extract will write in fs_homepath
	//   won't be a problem since we start a brand new scan next
	if ( cl_connectedToPureServer && String::NCmp( name, "qagame", 6 ) ) {
		if ( !FS_CL_ExtractFromPakFile( fn, gamedir, fname, cvar_name ) ) {
			Com_Error( ERR_DROP, "Game code(%s) failed Pure Server check", fname );
		}
	}
#endif

#ifndef NDEBUG
	// current directory
	// NOTE: only for debug build, see Sys_LoadDll discussion
	fn = FS_BuildOSPath( pwdpath, gamedir, fname );
	Com_Printf( "Sys_LoadDll(%s)... ", fn );
	libHandle = dlopen( fn, Q_RTLD );

	if ( !libHandle ) {
		Com_Printf( "\nSys_LoadDll(%s) failed:\n\"%s\"\n", fn, dlerror() );
#endif

	// homepath
	fn = FS_BuildOSPath( homepath, gamedir, fname );
	Com_Printf( "Sys_LoadDll(%s)... ", fn );
	libHandle = dlopen( fn, Q_RTLD );

	if ( !libHandle ) {
		Com_Printf( "\nSys_LoadDll(%s) failed:\n\"%s\"\n", fn, dlerror() );

		// basepath
		fn = FS_BuildOSPath( basepath, gamedir, fname );
		Com_Printf( "Sys_LoadDll(%s)... ", fn );
		libHandle = dlopen( fn, Q_RTLD );
		if ( !libHandle ) {
			// report any problem
			Com_Printf( "\nSys_LoadDll(%s) failed:\n\"%s\"\n", fn, dlerror() );
		} else {
			Com_Printf( "ok\n" );
		}

		// not found, bail
		if ( !libHandle ) {
#ifndef NDEBUG // in debug abort on failure
			Com_Error( ERR_FATAL, "Sys_LoadDll(%s) failed dlopen() completely!\n", name  );
#else
			Com_Printf( "Sys_LoadDll(%s) failed dlopen() completely!\n", name );
#endif
			return NULL;
		}

	} else {
		Com_Printf( "ok\n" );
	}

#ifndef NDEBUG
} else {
	Com_Printf( "ok\n" );
}
#endif

	String::NCpyZ( fqpath, fn, MAX_QPATH ) ;           // added 2/15/02 by T.Ray

	dllEntry = (void (*)(qintptr(*)(int, ...)))dlsym( libHandle, "dllEntry" );
	*entryPoint = (qintptr(*)(int, ... ))dlsym( libHandle, "vmMain" );
	if ( !*entryPoint || !dllEntry ) {
		err = dlerror();
#ifndef NDEBUG // in debug abort on failure
		Com_Error( ERR_FATAL, "Sys_LoadDll(%s) failed dlsym(vmMain):\n\"%s\" !\n", name, err );
#else
		Com_Printf( "Sys_LoadDll(%s) failed dlsym(vmMain):\n\"%s\" !\n", name, err );
#endif
		dlclose( libHandle );
		err = dlerror();
		if ( err != NULL ) {
			Com_Printf( "Sys_LoadDll(%s) failed dlcose:\n\"%s\"\n", name, err );
		}
		return NULL;
	}
	Com_Printf( "Sys_LoadDll(%s) found **vmMain** at  %p  \n", name, *entryPoint );
	dllEntry( systemcalls );
	Com_Printf( "Sys_LoadDll(%s) succeeded!\n", name );
	return libHandle;
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

byte sys_packetReceived[MAX_MSGLEN_WOLF];

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
	char    *s;
	QMsg netmsg;
	netadr_t adr;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// pump the message loop
	// in vga this calls KBD_Update, under X, it calls GetEvent
	Sys_SendKeyEvents();

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char  *b;
		int len;

		len = String::Length( s ) + 1;
		b = (char*)Mem_Alloc( len );
		String::Cpy( b, s );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for other input devices
	IN_Frame();

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket( &adr, &netmsg ) ) {
		netadr_t    *buf;
		int len;

		// copy out to a seperate buffer for qeueing
		len = sizeof( netadr_t ) + netmsg.cursize;
		buf = (netadr_t*)Mem_Alloc( len );
		*buf = adr;
		memcpy( buf + 1, netmsg._data, netmsg.cursize );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	return Sys_SharedGetEvent();
}

/*****************************************************************************/

qboolean Sys_CheckCD( void ) {
	return qtrue;
}

void Sys_AppActivate( void ) {
}

void    Sys_ConfigureFPU() { // bk001213 - divide by zero
#ifdef __linux__
#ifdef __i386
#ifndef NDEBUG

	// bk0101022 - enable FPE's in debug mode
	static int fpu_word = _FPU_DEFAULT & ~( _FPU_MASK_ZM | _FPU_MASK_IM );
	int current = 0;
	_FPU_GETCW( current );
	if ( current != fpu_word ) {
#if 0
		Com_Printf( "FPU Control 0x%x (was 0x%x)\n", fpu_word, current );
		_FPU_SETCW( fpu_word );
		_FPU_GETCW( current );
		assert( fpu_word == current );
#endif
	}
#else // NDEBUG
	static int fpu_word = _FPU_DEFAULT;
	_FPU_SETCW( fpu_word );
#endif // NDEBUG
#endif // __i386
#endif // __linux
}

void Sys_PrintBinVersion( const char* name ) {
	char* date = __DATE__;
	char* time = __TIME__;
	char* sep = "==============================================================";
	fprintf( stdout, "\n\n%s\n", sep );
#ifdef DEDICATED
	fprintf( stdout, "Linux Quake3 Dedicated Server [%s %s]\n", date, time );
#else
	fprintf( stdout, "Linux Quake3 Full Executable  [%s %s]\n", date, time );
#endif
	fprintf( stdout, " local install: %s\n", name );
	fprintf( stdout, "%s\n\n", sep );
}

/*
==================
chmod OR on a file
==================
*/
void Sys_Chmod( char *file, int mode ) {
	struct stat s_buf;
	int perm;
	if ( stat( file, &s_buf ) != 0 ) {
		Com_Printf( "stat('%s')  failed: errno %d\n", file, errno );
		return;
	}
	perm = s_buf.st_mode | mode;
	if ( chmod( file, perm ) != 0 ) {
		Com_Printf( "chmod('%s', %d) failed: errno %d\n", file, perm, errno );
	}
	Com_DPrintf( "chmod +%d '%s'\n", mode, file );
}

/*
==================
Sys_DoStartProcess
actually forks and starts a process

UGLY HACK:
  Sys_StartProcess works with a command line only
  if this command line is actually a binary with command line parameters,
  the only way to do this on unix is to use a system() call
  but system doesn't replace the current process, which leads to a situation like:
  wolf.x86--spawned_process.x86
  in the case of auto-update for instance, this will cause write access denied on wolf.x86:
  wolf-beta/2002-March/000079.html
  we hack the implementation here, if there are no space in the command line, assume it's a straight process and use execl
  otherwise, use system ..
  The clean solution would be Sys_StartProcess and Sys_StartProcess_Args..
==================
*/
void Sys_DoStartProcess( char *cmdline ) {
	switch ( fork() )
	{
	case - 1:
		// main thread
		break;
	case 0:
		if ( strchr( cmdline, ' ' ) ) {
			system( cmdline );
		} else
		{
			execl( cmdline, cmdline, NULL );
			printf( "execl failed: %s\n", strerror( errno ) );
		}
		_exit( 0 );
		break;
	}
}

/*
==================
Sys_StartProcess
if !doexit, start the process asap
otherwise, push it for execution at exit
(i.e. let complete shutdown of the game and freeing of resources happen)
NOTE: might even want to add a small delay?
==================
*/
void Sys_StartProcess( char *cmdline, qboolean doexit ) {

	if ( doexit ) {
		Com_DPrintf( "Sys_StartProcess %s (delaying to final exit)\n", cmdline );
		String::NCpyZ( exit_cmdline, cmdline, MAX_CMD );
		Cbuf_ExecuteText( EXEC_APPEND, "quit\n" );
		return;
	}

	Com_DPrintf( "Sys_StartProcess %s\n", cmdline );
	Sys_DoStartProcess( cmdline );
}

/*
=================
Sys_OpenURL
=================
*/
void Sys_OpenURL( const char *url, qboolean doexit ) {
	const char *basepath, *homepath, *pwdpath;
	char fname[20];
	char fn[MAX_OSPATH];
	char cmdline[MAX_CMD];

	Com_Printf( "Sys_OpenURL %s\n", url );
	// opening an URL on *nix can mean a lot of things ..
	// just spawn a script instead of deciding for the user :-)

	// do the setup before we fork
	// search for an openurl.sh script
	// search procedure taken from Sys_LoadDll
	String::NCpyZ( fname, "openurl.sh", 20 );

	pwdpath = Sys_Cwd();
	String::Sprintf( fn, MAX_OSPATH, "%s/%s", pwdpath, fname );
	if ( access( fn, X_OK ) == -1 ) {
		Com_DPrintf( "%s not found\n", fn );
		// try in home path
		homepath = Cvar_VariableString( "fs_homepath" );
		String::Sprintf( fn, MAX_OSPATH, "%s/%s", homepath, fname );
		if ( access( fn, X_OK ) == -1 ) {
			Com_DPrintf( "%s not found\n", fn );
			// basepath, last resort
			basepath = Cvar_VariableString( "fs_basepath" );
			String::Sprintf( fn, MAX_OSPATH, "%s/%s", basepath, fname );
			if ( access( fn, X_OK ) == -1 ) {
				Com_DPrintf( "%s not found\n", fn );
				Com_Printf( "Can't find script '%s' to open requested URL (use +set developer 1 for more verbosity)\n", fname );
				// we won't quit
				return;
			}
		}
	}

	Com_DPrintf( "URL script: %s\n", fn );

	// build the command line
	String::Sprintf( cmdline, MAX_CMD, "%s '%s' &", fn, url );

	Sys_StartProcess( cmdline, doexit );

}

void Sys_ParseArgs( int argc, char* argv[] ) {

	if ( argc == 2 ) {
		if ( ( !String::Cmp( argv[1], "--version" ) )
			 || ( !String::Cmp( argv[1], "-v" ) ) ) {
			Sys_PrintBinVersion( argv[0] );
			Sys_Exit( 0 );
		}
	}
}

int main( int argc, char* argv[] ) {
	// int  oldtime, newtime; // bk001204 - unused
	int len, i;
	char  *cmdline;

	// go back to real user for config loads
	saved_euid = geteuid();
	seteuid( getuid() );

	Sys_ParseArgs( argc, argv ); // bk010104 - added this for support

	// merge the command line, this is kinda silly
	for ( len = 1, i = 1; i < argc; i++ )
		len += String::Length( argv[i] ) + 1;
	cmdline = (char*)malloc( len );
	*cmdline = 0;
	for ( i = 1; i < argc; i++ )
	{
		if ( i > 1 ) {
			strcat( cmdline, " " );
		}
		strcat( cmdline, argv[i] );
	}

	// bk000306 - clear queues
	memset( &eventQue[0], 0, MAX_QUED_EVENTS * sizeof( sysEvent_t ) );
	memset( &sys_packetReceived[0], 0, MAX_MSGLEN_WOLF * sizeof( byte ) );

	Com_Init( cmdline );
	NET_Init();

	Sys_ConsoleInputInit();

	fcntl( 0, F_SETFL, fcntl( 0, F_GETFL, 0 ) | FNDELAY );

#ifdef DEDICATED
	// init here for dedicated, as we don't have GLimp_Init
	InitSig();
#endif

	while ( 1 )
	{
#ifdef __linux__
		Sys_ConfigureFPU();
#endif
		Com_Frame();
	}
}
