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

// qcommon.h -- definitions common between client and server, but not game.or ref modules
#ifndef _QCOMMON_H_
#define _QCOMMON_H_

//#define	PRE_RELEASE_DEMO

//============================================================================

void MSG_Init( QMsg *buf, byte *data, int length );
void MSG_InitOOB( QMsg *buf, byte *data, int length );
void *MSG_GetSpace( QMsg *buf, int length );

struct wmentityState_t;
struct wmplayerState_t;

void MSG_WriteDeltaUsercmd( QMsg *msg, struct wmusercmd_t *from, struct wmusercmd_t *to );
void MSG_ReadDeltaUsercmd( QMsg *msg, struct wmusercmd_t *from, struct wmusercmd_t *to );

void MSG_WriteDeltaUsercmdKey( QMsg *msg, int key, wmusercmd_t *from, wmusercmd_t *to );
void MSG_ReadDeltaUsercmdKey( QMsg *msg, int key, wmusercmd_t *from, wmusercmd_t *to );

void MSG_WriteDeltaEntity( QMsg *msg, struct wmentityState_t *from, struct wmentityState_t *to
						   , qboolean force );
void MSG_ReadDeltaEntity( QMsg *msg, wmentityState_t *from, wmentityState_t *to,
						  int number );

void MSG_WriteDeltaPlayerstate( QMsg *msg, struct wmplayerState_t *from, struct wmplayerState_t *to );
void MSG_ReadDeltaPlayerstate( QMsg *msg, struct wmplayerState_t *from, struct wmplayerState_t *to );


void MSG_ReportChangeVectors_f( void );

/*
==============================================================

NET

==============================================================
*/

#define MAX_PACKET_USERCMDS     32      // max number of wmusercmd_t in a packet

void        NET_Init( void );
void        NET_Shutdown( void );
void        NET_Restart( void );

void        NET_SendPacket( netsrc_t sock, int length, const void *data, netadr_t to );
void QDECL NET_OutOfBandPrint( netsrc_t net_socket, netadr_t adr, const char *format, ... );
void QDECL NET_OutOfBandData( netsrc_t sock, netadr_t adr, byte *format, int len );

qboolean    NET_GetLoopPacket( netsrc_t sock, netadr_t *net_from, QMsg *net_message );
void        NET_Sleep( int msec );

qboolean    Sys_GetPacket( netadr_t *net_from, QMsg *net_message );

// be fragmented into multiple packets
#define MAX_DOWNLOAD_WINDOW         8       // max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE        2048    // 2048 byte block chunks


/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

void Netchan_Init( int qport );
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport );

void Netchan_Transmit( netchan_t *chan, int length, const byte *data );
void Netchan_TransmitNextFragment( netchan_t *chan );

qboolean Netchan_Process( netchan_t *chan, QMsg *msg );


/*
==============================================================

PROTOCOL

==============================================================
*/

// sent by the server, printed on connection screen, works for all clients
// (restrictions: does not handle \n, no more than 256 chars)
#define PROTOCOL_MISMATCH_ERROR "ERROR: Protocol Mismatch Between Client and Server.\
The server you are attempting to join is running an incompatible version of the game."

// long version used by the client in diagnostic window
#define PROTOCOL_MISMATCH_ERROR_LONG "ERROR: Protocol Mismatch Between Client and Server.\n\n\
The server you attempted to join is running an incompatible version of the game.\n\
You or the server may be running older versions of the game. Press the auto-update\
 button if it appears on the Main Menu screen."

#ifndef PRE_RELEASE_DEMO
// 1.33 - protocol 59
// 1.4 - protocol 60
#define PROTOCOL_VERSION 60
#define GAMENAME_STRING     "wolfmp"
#else
// the demo uses a different protocol version for independant browsing
  #define   PROTOCOL_VERSION    50  // NERVE - SMF - wolfMP protocol version
#endif

// NERVE - SMF - wolf multiplayer master servers
#define UPDATE_SERVER_NAME      "wolfmotd.idsoftware.com"            // 192.246.40.65
#define MASTER_SERVER_NAME      "wolfmaster.idsoftware.com"
#define AUTHORIZE_SERVER_NAME   "wolfauthorize.idsoftware.com"

// TTimo: allow override for easy dev/testing..
// see cons -- update_server=myhost
#if !defined( AUTOUPDATE_SERVER_NAME )
  #define AUTOUPDATE_SERVER1_NAME   "au2rtcw1.activision.com"            // DHM - Nerve
  #define AUTOUPDATE_SERVER2_NAME   "au2rtcw2.activision.com"            // DHM - Nerve
  #define AUTOUPDATE_SERVER3_NAME   "au2rtcw3.activision.com"            // DHM - Nerve
  #define AUTOUPDATE_SERVER4_NAME   "au2rtcw4.activision.com"            // DHM - Nerve
  #define AUTOUPDATE_SERVER5_NAME   "au2rtcw5.activision.com"            // DHM - Nerve
#else
  #define AUTOUPDATE_SERVER1_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER2_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER3_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER4_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER5_NAME   AUTOUPDATE_SERVER_NAME
#endif

#define PORT_MASTER         27950
#define PORT_UPDATE         27951
#define PORT_AUTHORIZE      27952
#define PORT_SERVER         27960
#define NUM_SERVER_PORTS    4       // broadcast scan this many ports after
									// PORT_SERVER so a single machine can
									// run multiple servers


/*
==============================================================

VIRTUAL MACHINE

==============================================================
*/

typedef enum {
	VMI_NATIVE,
	VMI_BYTECODE,
	VMI_COMPILED
} vmInterpret_t;

typedef enum {
	TRAP_MEMSET = 100,
	TRAP_MEMCPY,
	TRAP_STRNCPY,
	TRAP_SIN,
	TRAP_COS,
	TRAP_ATAN2,
	TRAP_SQRT,
	TRAP_MATRIXMULTIPLY,
	TRAP_ANGLEVECTORS,
	TRAP_PERPENDICULARVECTOR,
	TRAP_FLOOR,
	TRAP_CEIL,

	TRAP_TESTPRINTINT,
	TRAP_TESTPRINTFLOAT
} sharedTraps_t;

void    VM_Init( void );
vm_t    *VM_Create( const char *module, qintptr ( *systemCalls )( qintptr * ),
					vmInterpret_t interpret );
// module should be bare: "cgame", not "cgame.dll" or "vm/cgame.qvm"

void    VM_Free( vm_t *vm );
void    VM_Clear( void );
vm_t    *VM_Restart( vm_t *vm );

qintptr QDECL VM_Call( vm_t *vm, int callNum, ... );

/*
==============================================================

CMD

Command text buffering and command execution

==============================================================
*/

void    Cmd_Init( void );

/*
==============================================================

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator char
issues.
==============================================================
*/

#define BASEGAME "main"

void    FS_InitFilesystem( void );

qboolean    FS_ConditionalRestart( int checksumFeed );
void    FS_Restart( int checksumFeed );
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

int     FS_LoadStack();

#if !defined( DEDICATED )
extern int cl_connectedToPureServer;
qboolean FS_CL_ExtractFromPakFile( const char *path, const char *gamedir, const char *filename, const char *cvar_lastVersion );
#endif

#if defined( DO_LIGHT_DEDICATED )
int FS_RandChecksumFeed();
#endif

char *FS_ShiftStr( const char *string, int shift );

/*
==============================================================

MISC

==============================================================
*/

// centralizing the declarations for cl_cdkey
// (old code causing buffer overflows)
extern char cl_cdkey[34];
void Com_AppendCDKey( const char *filename );
void Com_ReadCDKey( const char *filename );

// returnbed by Sys_GetProcessorId
#define CPUID_GENERIC           0           // any unrecognized processor

#define CPUID_AXP               0x10

#define CPUID_INTEL_UNSUPPORTED 0x20            // Intel 386/486
#define CPUID_INTEL_PENTIUM     0x21            // Intel Pentium or PPro
#define CPUID_INTEL_MMX         0x22            // Intel Pentium/MMX or P2/MMX
#define CPUID_INTEL_KATMAI      0x23            // Intel Katmai

#define CPUID_AMD_3DNOW         0x30            // AMD K6 3DNOW!

char        *CopyString( const char *in );

void Com_BeginRedirect( char *buffer, int buffersize, void ( *flush )( char * ) );
void        Com_EndRedirect( void );
void QDECL Com_Printf( const char *fmt, ... );
void QDECL Com_DPrintf( const char *fmt, ... );
void QDECL Com_Error( int code, const char *fmt, ... );
void        Com_Quit_f( void );
int         Com_EventLoop( void );
int         Com_Milliseconds( void );   // will be journaled properly
int         Com_HashKey( char *string, int maxlen );
int         Com_RealTime( qtime_t *qtime );
qboolean    Com_SafeMode( void );

void        Com_StartupVariable( const char *match );
void        Com_SetRecommended();
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.


extern Cvar  *com_speeds;
extern Cvar  *com_sv_running;
extern Cvar  *com_cl_running;
extern Cvar  *com_version;
extern Cvar  *com_blood;
extern Cvar  *com_buildScript;        // for building release pak files
extern Cvar  *com_cameraMode;

// both client and server must agree to pause
extern Cvar  *cl_paused;
extern Cvar  *sv_paused;

// com_speeds times
extern int time_game;
extern int time_frontend;
extern int time_backend;            // renderer backend time

extern int com_frameMsec;

extern qboolean com_errorEntered;

typedef enum {
	TAG_FREE,
	TAG_GENERAL,
	TAG_BOTLIB,
	TAG_RENDERER,
	TAG_SMALL,
	TAG_STATIC
} memtag_t;

/*

--- low memory ----
server vm
server clipmap
---mark---
renderer initialization (shaders, etc)
UI vm
cgame vm
renderer map
renderer models

---free---

temp file loading
--- high memory ---

*/

#if defined( _DEBUG ) && !defined( BSPC )
#define ZONE_DEBUG
#endif

#ifdef ZONE_DEBUG
#define Z_TagMalloc( size, tag )          Z_TagMallocDebug( size, tag, # size, __FILE__, __LINE__ )
#define Z_Malloc( size )                  Z_MallocDebug( size, # size, __FILE__, __LINE__ )
#define S_Malloc( size )                  S_MallocDebug( size, # size, __FILE__, __LINE__ )
void *Z_TagMallocDebug( int size, int tag, char *label, char *file, int line ); // NOT 0 filled memory
void *Z_MallocDebug( int size, char *label, char *file, int line );         // returns 0 filled memory
void *S_MallocDebug( int size, char *label, char *file, int line );         // returns 0 filled memory
#else
void *Z_TagMalloc( int size, int tag ); // NOT 0 filled memory
void *Z_Malloc( int size );         // returns 0 filled memory
void *S_Malloc( int size );         // NOT 0 filled memory only for small allocations
#endif
void Z_Free( void *ptr );
void Z_FreeTags( int tag );
void Z_LogHeap( void );

void Hunk_Clear( void );
void Hunk_ClearToMark( void );
void Hunk_SetMark( void );
qboolean Hunk_CheckMark( void );
//void *Hunk_Alloc( int size );
// void *Hunk_Alloc( int size, ha_pref preference );
void Hunk_ClearTempMemory( void );
void *Hunk_AllocateTempMemory( int size );
void Hunk_FreeTempMemory( void *buf );
int Hunk_MemoryRemaining( void );
void Hunk_SmallLog( void );
void Hunk_Log( void );

void Com_TouchMemory( void );

// commandLine should not include the executable name (argv[0])
void Com_Init( char *commandLine );
void Com_Frame( void );
void Com_Shutdown( void );


/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

//
// client interface
//
void CL_InitKeyCommands( void );
// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later

void CL_Init( void );
void CL_Disconnect( qboolean showMainMenu );
void CL_Shutdown( void );
void CL_Frame( int msec );
qboolean CL_GameCommand( void );
void CL_KeyEvent( int key, qboolean down, unsigned time );

void CL_CharEvent( int key );
// char events are for field typing, not game control

void CL_MouseEvent( int dx, int dy, int time );

void CL_JoystickEvent( int axis, int value, int time );

void CL_PacketEvent( netadr_t from, QMsg *msg );

void CL_ConsolePrint( char *text );

void CL_MapLoading( void );
// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void    CL_ForwardCommandToServer( const char *string );
// adds the current command line as a q3clc_clientCommand to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

void CL_CDDialog( void );
// bring up the "need a cd to play" dialog

void CL_ShutdownAll( void );
// shutdown all the client stuff

void CL_FlushMemory( void );
// dump all memory on an error

void CL_StartHunkUsers( void );
// start all the client stuff using the hunk


#ifndef UPDATE_SERVER
void CL_CheckAutoUpdate( void );
// Send a message to auto-update server
#endif

void Key_WriteBindings( fileHandle_t f );
// for writing the config files

void S_ClearSoundBuffer( void );
// call before filesystem access

void SCR_DebugGraph( float value, int color );   // FIXME: move logging to common?


//
// server interface
//
void SV_Init( void );
void SV_Shutdown( char *finalmsg );
void SV_Frame( int msec );
void SV_PacketEvent( netadr_t from, QMsg *msg );
qboolean SV_GameCommand( void );


//
// UI interface
//
qboolean UI_GameCommand( void );
qboolean UI_usesUniqueCDKey();

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

sysEvent_t  Sys_GetEvent( void );

void    Sys_Init( void );

void *Sys_InitializeCriticalSection();
void Sys_EnterCriticalSection( void *ptr );
void Sys_LeaveCriticalSection( void *ptr );

// FIXME: wants win32 implementation
char* Sys_GetDLLName( const char *name );
// fqpath param added 2/15/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
void    * QDECL Sys_LoadDll( const char *name, char *fqpath, qintptr( QDECL * *entryPoint ) ( int, ... ),
							 qintptr ( QDECL * systemcalls )( int, ... ) );
void    Sys_UnloadDll( void *dllHandle );

void    Sys_UnloadGame( void );
void    *Sys_GetGameAPI( void *parms );

void    Sys_UnloadCGame( void );
void    *Sys_GetCGameAPI( void );

void    Sys_UnloadUI( void );
void    *Sys_GetUIAPI( void );

//bot libraries
void    Sys_UnloadBotLib( void );
void    *Sys_GetBotLibAPI( void *parms );

char    *Sys_GetCurrentUser( void );

void QDECL Sys_Error( const char *error, ... );
void    Sys_Quit( void );

void    Sys_SnapVector( float *v );

// the system console is shown when a dedicated server is running
void    Sys_DisplaySystemConsole( qboolean show );

int     Sys_GetProcessorId( void );

void    Sys_BeginStreamedFile( fileHandle_t f, int readahead );
void    Sys_EndStreamedFile( fileHandle_t f );
int     Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f );
void    Sys_StreamSeek( fileHandle_t f, int offset, int origin );

void    Sys_SetErrorText( const char *text );

qboolean    Sys_CheckCD( void );

void    Sys_BeginProfiling( void );
void    Sys_EndProfiling( void );

qboolean Sys_LowPhysicalMemory();
unsigned int Sys_ProcessorCount();

// NOTE TTimo - on win32 the cwd is prepended .. non portable behaviour
void Sys_StartProcess( char *exeName, qboolean doexit );            // NERVE - SMF
void Sys_OpenURL( const char *url, qboolean doexit );                       // NERVE - SMF
int Sys_GetHighQualityCPU();

#ifdef __linux__
// TTimo only on linux .. maybe on Mac too?
// will OR with the existing mode (chmod ..+..)
void Sys_Chmod( char *file, int mode );
#endif

extern huffman_t clientHuffTables;

#define SV_ENCODE_START     4
#define SV_DECODE_START     12
#define CL_ENCODE_START     12
#define CL_DECODE_START     4

// TTimo
// dll checksuming stuff, centralizing OS-dependent parts
// *_SHIFT is the shifting we applied to the reference string

#if defined( _WIN32 )

// qagame_mp_x86.dll
#define SYS_DLLNAME_QAGAME_SHIFT 6
#define SYS_DLLNAME_QAGAME "wgmgskesve~><4jrr"

// cgame_mp_x86.dll
#define SYS_DLLNAME_CGAME_SHIFT 2
#define SYS_DLLNAME_CGAME "eicogaoraz:80fnn"

// ui_mp_x86.dll
#define SYS_DLLNAME_UI_SHIFT 5
#define SYS_DLLNAME_UI "zndrud}=;3iqq"

#elif defined( __linux__ )

// qagame.mp.i386.so
#define SYS_DLLNAME_QAGAME_SHIFT 6
#define SYS_DLLNAME_QAGAME "wgmgsk4sv4o9><4yu"

// cgame.mp.i386.so
#define SYS_DLLNAME_CGAME_SHIFT 2
#define SYS_DLLNAME_CGAME "eicog0or0k5:80uq"

// ui.mp.i386.so
#define SYS_DLLNAME_UI_SHIFT 5
#define SYS_DLLNAME_UI "zn3ru3n8=;3xt"

#elif defined( __MACOS__ )

#if 1 //DAJ
// qagame.mp.i386.so
#define SYS_DLLNAME_QAGAME_SHIFT 6
#define SYS_DLLNAME_QAGAME "wgmgsk4sv4o9><4yu"

// cgame.mp.i386.so
#define SYS_DLLNAME_CGAME_SHIFT 2
#define SYS_DLLNAME_CGAME "eicog0or0k5:80uq"

// ui.mp.i386.so
#define SYS_DLLNAME_UI_SHIFT 5
#define SYS_DLLNAME_UI "zn3ru3n8=;3xt"

#else   //DAJ
// qagame.mp.i386.so
#define SYS_DLLNAME_QAGAME_SHIFT 0
#define SYS_DLLNAME_QAGAME "qgame_MP.dll"

// cgame.mp.i386.so
#define SYS_DLLNAME_CGAME_SHIFT 0
#define SYS_DLLNAME_CGAME "cgame_MP.dll"

// ui.mp.i386.so
#define SYS_DLLNAME_UI_SHIFT 0
#define SYS_DLLNAME_UI "ui_MP.dll"
#endif  //DAJ

#else
#error unknown OS
#endif

#endif // _QCOMMON_H_

