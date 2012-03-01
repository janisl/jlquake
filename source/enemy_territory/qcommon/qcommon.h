/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// qcommon.h -- definitions common between client and server, but not game.or ref modules
#ifndef _QCOMMON_H_
#define _QCOMMON_H_

//#define PRE_RELEASE_DEMO
#ifdef PRE_RELEASE_DEMO
#define PRE_RELEASE_DEMO_NODEVMAP
#endif // PRE_RELEASE_DEMO

//============================================================================

//
// msg.c
//
typedef struct {
	qboolean allowoverflow;     // if false, do a Com_Error
	qboolean overflowed;        // set to true if the buffer size failed (with allowoverflow set)
	qboolean oob;               // set to true if the buffer size failed (with allowoverflow set)
	byte    *data;
	int maxsize;
	int cursize;
	int uncompsize;             // NERVE - SMF - net debugging
	int readcount;
	int bit;                    // for bitwise reads and writes
} msg_t;

void MSG_Init( msg_t *buf, byte *data, int length );
void MSG_InitOOB( msg_t *buf, byte *data, int length );
void MSG_Clear( msg_t *buf );
void *MSG_GetSpace( msg_t *buf, int length );
void MSG_WriteData( msg_t *buf, const void *data, int length );
void MSG_Bitstream( msg_t *buf );
void MSG_Uncompressed( msg_t *buf );

// TTimo
// copy a msg_t in case we need to store it as is for a bit
// (as I needed this to keep an msg_t from a static var for later use)
// sets data buffer as MSG_Init does prior to do the copy
void MSG_Copy( msg_t *buf, byte *data, int length, msg_t *src );

struct usercmd_s;
struct entityState_s;
struct playerState_s;

void MSG_WriteBits( msg_t *msg, int value, int bits );

void MSG_WriteChar( msg_t *sb, int c );
void MSG_WriteByte( msg_t *sb, int c );
void MSG_WriteShort( msg_t *sb, int c );
void MSG_WriteLong( msg_t *sb, int c );
void MSG_WriteFloat( msg_t *sb, float f );
void MSG_WriteString( msg_t *sb, const char *s );
void MSG_WriteBigString( msg_t *sb, const char *s );
void MSG_WriteAngle16( msg_t *sb, float f );

void    MSG_BeginReading( msg_t *sb );
void    MSG_BeginReadingOOB( msg_t *sb );
void    MSG_BeginReadingUncompressed( msg_t *msg );

int     MSG_ReadBits( msg_t *msg, int bits );

int     MSG_ReadChar( msg_t *sb );
int     MSG_ReadByte( msg_t *sb );
int     MSG_ReadShort( msg_t *sb );
int     MSG_ReadLong( msg_t *sb );
float   MSG_ReadFloat( msg_t *sb );
char    *MSG_ReadString( msg_t *sb );
char    *MSG_ReadBigString( msg_t *sb );
char    *MSG_ReadStringLine( msg_t *sb );
float   MSG_ReadAngle16( msg_t *sb );
void    MSG_ReadData( msg_t *sb, void *buffer, int size );


void MSG_WriteDeltaUsercmd( msg_t *msg, struct usercmd_s *from, struct usercmd_s *to );
void MSG_ReadDeltaUsercmd( msg_t *msg, struct usercmd_s *from, struct usercmd_s *to );

void MSG_WriteDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to );
void MSG_ReadDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to );

void MSG_WriteDeltaEntity( msg_t *msg, struct entityState_s *from, struct entityState_s *to
						   , qboolean force );
void MSG_ReadDeltaEntity( msg_t *msg, entityState_t *from, entityState_t *to,
						  int number );

void MSG_WriteDeltaPlayerstate( msg_t *msg, struct playerState_s *from, struct playerState_s *to );
void MSG_ReadDeltaPlayerstate( msg_t *msg, struct playerState_s *from, struct playerState_s *to );


void MSG_ReportChangeVectors_f( void );

/*
==============================================================

NET

==============================================================
*/

#define PACKET_BACKUP   32  // number of old messages that must be kept on client and
							// server for delta comrpession and ping estimation
#define PACKET_MASK     ( PACKET_BACKUP - 1 )

#define MAX_PACKET_USERCMDS     32      // max number of usercmd_t in a packet

#define PORT_ANY            -1

// RF, increased this, seems to keep causing problems when set to 64, especially when loading
// a savegame, which is hard to fix on that side, since we can't really spread out a loadgame
// among several frames
//#define	MAX_RELIABLE_COMMANDS	64			// max string commands buffered for restransmit
//#define	MAX_RELIABLE_COMMANDS	128			// max string commands buffered for restransmit
#define MAX_RELIABLE_COMMANDS   256 // bigger!

typedef enum {
	NA_BOT,
	NA_BAD,                 // an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX
} netadrtype_t;

typedef enum {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

typedef struct {
	netadrtype_t type;

	byte ip[4];
	byte ipx[10];

	unsigned short port;
} netadr_t;

void        NET_Init( void );
void        NET_Shutdown( void );
void        NET_Restart( void );
void        NET_Config( qboolean enableNetworking );

void        NET_SendPacket( netsrc_t sock, int length, const void *data, netadr_t to );
void QDECL NET_OutOfBandPrint( netsrc_t net_socket, netadr_t adr, const char *format, ... );
void QDECL NET_OutOfBandData( netsrc_t sock, netadr_t adr, byte *format, int len );

qboolean    NET_CompareAdr( netadr_t a, netadr_t b );
qboolean    NET_CompareBaseAdr( netadr_t a, netadr_t b );
qboolean    NET_IsLocalAddress( netadr_t adr );
qboolean    NET_IsIPXAddress( const char *buf );
const char  *NET_AdrToString( netadr_t a );
qboolean    NET_StringToAdr( const char *s, netadr_t *a );
qboolean    NET_GetLoopPacket( netsrc_t sock, netadr_t *net_from, msg_t *net_message );
void        NET_Sleep( int msec );


//----(SA)	increased for larger submodel entity counts
#define MAX_MSGLEN              32768       // max length of a message, which may
//#define	MAX_MSGLEN				16384		// max length of a message, which may
// be fragmented into multiple packets
#define MAX_DOWNLOAD_WINDOW         8       // max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE        2048    // 2048 byte block chunks


/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

typedef struct {
	netsrc_t sock;

	int dropped;                    // between last packet and previous

	netadr_t remoteAddress;
	int qport;                      // qport value to write when transmitting

	// sequencing variables
	int incomingSequence;
	int outgoingSequence;

	// incoming fragment assembly buffer
	int fragmentSequence;
	int fragmentLength;
	byte fragmentBuffer[MAX_MSGLEN];

	// outgoing fragment buffer
	// we need to space out the sending of large fragmented messages
	qboolean unsentFragments;
	int unsentFragmentStart;
	int unsentLength;
	byte unsentBuffer[MAX_MSGLEN];

} netchan_t;

void Netchan_Init( int qport );
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport );

void Netchan_Transmit( netchan_t *chan, int length, const byte *data );
void Netchan_TransmitNextFragment( netchan_t *chan );

qboolean Netchan_Process( netchan_t *chan, msg_t *msg );


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

#define GAMENAME_STRING "et"
#ifndef PRE_RELEASE_DEMO
// 2.56 - protocol 83
// 2.4 - protocol 80
// 1.33 - protocol 59
// 1.4 - protocol 60
#define PROTOCOL_VERSION    84
#else
// the demo uses a different protocol version for independant browsing
#define PROTOCOL_VERSION    72
#endif

// NERVE - SMF - wolf multiplayer master servers
#ifndef MASTER_SERVER_NAME
	#define MASTER_SERVER_NAME      "etmaster.idsoftware.com"
#endif
#define MOTD_SERVER_NAME        "etmaster.idsoftware.com"    //"etmotd.idsoftware.com"			// ?.?.?.?

#ifdef AUTHORIZE_SUPPORT
	#define AUTHORIZE_SERVER_NAME   "wolfauthorize.idsoftware.com"
#endif // AUTHORIZE_SUPPORT

// TTimo: override autoupdate server for testing
#ifndef AUTOUPDATE_SERVER_NAME
//	#define AUTOUPDATE_SERVER_NAME "127.0.0.1"
	#define AUTOUPDATE_SERVER_NAME "au2rtcw2.activision.com"
#endif

// TTimo: allow override for easy dev/testing..
// FIXME: not planning to support more than 1 auto update server
// see cons -- update_server=myhost
#define MAX_AUTOUPDATE_SERVERS  5
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
#define PORT_MOTD           27951
#ifdef AUTHORIZE_SUPPORT
#define PORT_AUTHORIZE      27952
#endif // AUTHORIZE_SUPPORT
#define PORT_SERVER         27960
#define NUM_SERVER_PORTS    4       // broadcast scan this many ports after
									// PORT_SERVER so a single machine can
									// run multiple servers


// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum svc_ops_e {
	svc_bad,
	svc_nop,
	svc_gamestate,
	svc_configstring,           // [short] [string] only in gamestate messages
	svc_baseline,               // only in gamestate messages
	svc_serverCommand,          // [string] to be executed by client game module
	svc_download,               // [short] size [size bytes]
	svc_snapshot,
	svc_EOF
};


//
// client to server
//
enum clc_ops_e {
	clc_bad,
	clc_nop,
	clc_move,               // [[usercmd_t]
	clc_moveNoDelta,        // [[usercmd_t]
	clc_clientCommand,      // [string] message
	clc_EOF
};

/*
==============================================================

VIRTUAL MACHINE

==============================================================
*/

typedef struct vm_s vm_t;

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
vm_t    *VM_Create( const char *module, intptr_t ( *systemCalls )( intptr_t * ),
					vmInterpret_t interpret );
// module should be bare: "cgame", not "cgame.dll" or "vm/cgame.qvm"

void    VM_Free( vm_t *vm );
void    VM_Clear( void );
vm_t    *VM_Restart( vm_t *vm );

intptr_t QDECL VM_Call( vm_t *vm, int callNum, ... );

void    VM_Debug( int level );

void    *VM_ArgPtr( intptr_t intValue );
void    *VM_ExplicitArgPtr( vm_t *vm, intptr_t intValue );

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

#ifndef PRE_RELEASE_DEMO
//#define BASEGAME "main"
#define BASEGAME "etmain"
#else
#define BASEGAME "ettest"
#endif

void    FS_InitFilesystem( void );

qboolean    FS_ConditionalRestart( int checksumFeed );
void    FS_Restart( int checksumFeed );
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

qboolean FS_OS_FileExists( const char *file ); // TTimo - test file existence given OS path

int     FS_LoadStack();

#if !defined( DEDICATED )
extern int cl_connectedToPureServer;
qboolean FS_CL_ExtractFromPakFile( const char *path, const char *gamedir, const char *filename, const char *cvar_lastVersion );
#endif

#if defined( DO_LIGHT_DEDICATED )
int FS_RandChecksumFeed();
#endif

char *FS_ShiftStr( const char *string, int shift );

unsigned int FS_ChecksumOSPath( char *OSPath );

/*
==============================================================

DOWNLOAD

==============================================================
*/

#include "dl_public.h"

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

typedef struct gameInfo_s {
	qboolean spEnabled;
	int spGameTypes;
	int defaultSPGameType;
	int coopGameTypes;
	int defaultCoopGameType;
	int defaultGameType;
	qboolean usesProfiles;
} gameInfo_t;

extern gameInfo_t com_gameInfo;

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
int QDECL Com_VPrintf( const char *fmt, va_list argptr ) id_attribute( ( format( printf,1,0 ) ) ); // conforms to vprintf prototype for print callback passing
void QDECL Com_Printf( const char *fmt, ... ) id_attribute( ( format( printf,1,2 ) ) ); // this one calls to Com_VPrintf now
void QDECL Com_DPrintf( const char *fmt, ... ) id_attribute( ( format( printf,1,2 ) ) );
void QDECL Com_Error( int code, const char *fmt, ... ) id_attribute( ( format( printf,2,3 ) ) );
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

//bani - profile functions
void Com_TrackProfile( char *profile_path );
qboolean Com_CheckProfile( char *profile_path );
qboolean Com_WriteProfile( char *profile_path );

extern Cvar  *com_ignorecrash;    //bani

extern Cvar  *com_pid;    //bani

extern Cvar  *com_speeds;
extern Cvar  *com_sv_running;
extern Cvar  *com_cl_running;
extern Cvar  *com_version;
//extern	Cvar	*com_blood;
extern Cvar  *com_buildScript;        // for building release pak files
extern Cvar  *com_cameraMode;
extern Cvar  *com_logosPlaying;

// watchdog
extern Cvar  *com_watchdog;
extern Cvar  *com_watchdog_cmd;

// both client and server must agree to pause
extern Cvar  *cl_paused;
extern Cvar  *sv_paused;

// com_speeds times
extern int time_game;
extern int time_frontend;
extern int time_backend;            // renderer backend time

extern int com_frameMsec;
extern int com_expectedhunkusage;
extern int com_hunkusedvalue;

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
void Com_Shutdown( qboolean badProfile );


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
void CL_ClearStaticDownload( void );
void CL_Disconnect( qboolean showMainMenu );
void CL_Shutdown( void );
void CL_Frame( int msec );
qboolean CL_GameCommand( void );
void CL_KeyEvent( int key, qboolean down, unsigned time );

void CL_CharEvent( int key );
// char events are for field typing, not game control

void CL_MouseEvent( int dx, int dy, int time );

void CL_JoystickEvent( int axis, int value, int time );

void CL_PacketEvent( netadr_t from, msg_t *msg );

void CL_ConsolePrint( char *text );

void CL_MapLoading( void );
// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void    CL_ForwardCommandToServer( const char *string );
// adds the current command line as a clc_clientCommand to the client message.
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

void CL_CheckAutoUpdate( void );
qboolean CL_NextUpdateServer( void );
void CL_GetAutoUpdate( void );

void Key_WriteBindings( fileHandle_t f );
// for writing the config files

void S_ClearSoundBuffer( qboolean killStreaming );  //----(SA)	modified
// call before filesystem access

void SCR_DebugGraph( float value, int color );   // FIXME: move logging to common?


//
// server interface
//
void SV_Init( void );
void SV_Shutdown( char *finalmsg );
void SV_Frame( int msec );
void SV_PacketEvent( netadr_t from, msg_t *msg );
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

typedef enum {
	AXIS_SIDE,
	AXIS_FORWARD,
	AXIS_UP,
	AXIS_ROLL,
	AXIS_YAW,
	AXIS_PITCH,
	MAX_JOYSTICK_AXIS
} joystickAxis_t;

sysEvent_t  Sys_GetEvent( void );

void    Sys_Init( void );
qboolean Sys_IsNumLockDown( void );

void *Sys_InitializeCriticalSection();
void Sys_EnterCriticalSection( void *ptr );
void Sys_LeaveCriticalSection( void *ptr );

char* Sys_GetDLLName( const char *name );
// fqpath param added 2/15/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
void    * QDECL Sys_LoadDll( const char *name, char *fqpath, intptr_t( QDECL * *entryPoint ) ( int, ... ),
							 intptr_t ( QDECL * systemcalls )( int, ... ) );
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
char    *Sys_GetClipboardData( void );  // note that this isn't journaled...

void    Sys_SnapVector( float *v );

// the system console is shown when a dedicated server is running
void    Sys_DisplaySystemConsole( qboolean show );

int     Sys_GetProcessorId( void );

void    Sys_BeginStreamedFile( fileHandle_t f, int readahead );
void    Sys_EndStreamedFile( fileHandle_t f );
int     Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f );
void    Sys_StreamSeek( fileHandle_t f, int offset, int origin );

void    Sys_SetErrorText( const char *text );

void    Sys_SendPacket( int length, const void *data, netadr_t to );

qboolean    Sys_StringToAdr( const char *s, netadr_t *a );
//Does NOT parse port numbers, only base addresses.

qboolean    Sys_IsLANAddress( netadr_t adr );
void        Sys_ShowIP( void );

qboolean    Sys_CheckCD( void );

void    Sys_BeginProfiling( void );
void    Sys_EndProfiling( void );

qboolean Sys_LowPhysicalMemory();
unsigned int Sys_ProcessorCount();

// NOTE TTimo - on win32 the cwd is prepended .. non portable behaviour
void Sys_StartProcess( char *exeName, qboolean doexit );            // NERVE - SMF
void Sys_OpenURL( const char *url, qboolean doexit );                       // NERVE - SMF
int Sys_GetHighQualityCPU();
float Sys_GetCPUSpeed( void );

#ifdef __linux__
// TTimo only on linux .. maybe on Mac too?
// will OR with the existing mode (chmod ..+..)
void Sys_Chmod( char *file, int mode );
#endif

void    Huff_Compress( msg_t *buf, int offset );
void    Huff_Decompress( msg_t *buf, int offset );

extern huffman_t clientHuffTables;

#define SV_ENCODE_START     4
#define SV_DECODE_START     12
#define CL_ENCODE_START     12
#define CL_DECODE_START     4

void Com_GetHunkInfo( int* hunkused, int* hunkexpected );

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

#elif __MACOS__

#ifdef _DEBUG
// qagame_d_mac
	#define SYS_DLLNAME_QAGAME_SHIFT 6
	#define SYS_DLLNAME_QAGAME "wgmgskejesgi"

// cgame_d_mac
	#define SYS_DLLNAME_CGAME_SHIFT 2
	#define SYS_DLLNAME_CGAME "eicogafaoce"

// ui_d_mac
	#define SYS_DLLNAME_UI_SHIFT 5
	#define SYS_DLLNAME_UI "zndidrfh"
#else
// qagame_mac
	#define SYS_DLLNAME_QAGAME_SHIFT 6
	#define SYS_DLLNAME_QAGAME "wgmgskesgi"

// cgame_mac
	#define SYS_DLLNAME_CGAME_SHIFT 2
	#define SYS_DLLNAME_CGAME "eicogaoce"

// ui_mac
	#define SYS_DLLNAME_UI_SHIFT 5
	#define SYS_DLLNAME_UI "zndrfh"
#endif

#else

#error unknown OS

#endif

#endif // _QCOMMON_H_
