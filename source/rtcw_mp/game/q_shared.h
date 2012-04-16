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

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#include "../../core/core.h"

#define Q3_VERSION      "Wolf 1.41b-MP"
// 1.41b-MP: fix autodl sploit
// 1.4-MP : (== 1.34)
// 1.3-MP : final for release
// 1.1b - TTimo SP linux release (+ MP updates)
// 1.1b5 - Mac update merge in

#define NEW_ANIMS
#define MAX_TEAMNAME    32

// DHM - Nerve
//#define PRE_RELEASE_DEMO

#if defined( ppc ) || defined( __ppc ) || defined( __ppc__ ) || defined( __POWERPC__ )
#define idppc 1
#endif

/**********************************************************************
  VM Considerations

  The VM can not use the standard system headers because we aren't really
  using the compiler they were meant for.  We use bg_lib.h which contains
  prototypes for the functions we define for our own use in bg_lib.c.

  When writing mods, please add needed headers HERE, do not start including
  stuff like <stdio.h> in the various .c files that make up each of the VMs
  since you will be including system headers files can will have issues.

  Remember, if you use a C library function that is not defined in bg_lib.c,
  you will have to add your own version for support in the VM.

 **********************************************************************/

#ifdef Q3_VM

#include "bg_lib.h"

#else

#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

#endif

#ifdef _WIN32

//#pragma intrinsic( memset, memcpy )

#endif


// for windows fastcall option

#define QDECL

//======================= WIN32 DEFINES =================================

#ifdef WIN32

#define MAC_STATIC

#undef QDECL
#define QDECL   __cdecl

// buildstring will be incorporated into the version string
#ifdef NDEBUG
#ifdef _M_IX86
#define CPUSTRING   "win-x86"
#elif defined _M_X86_64
#define CPUSTRING   "win-x86_64"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP"
#endif
#else
#ifdef _M_IX86
#define CPUSTRING   "win-x86-debug"
#elif defined _M_X64
#define CPUSTRING   "win-x86_64-debug"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP-debug"
#endif
#endif


#endif

//======================= MAC OS X SERVER DEFINES =====================

#if defined( MACOS_X )

#define MAC_STATIC

#define CPUSTRING   "MacOS_X"

// Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
// runs *much* faster than calling sqrt(). We'll use two Newton-Raphson
// refinement steps to get bunch more precision in the 1/sqrt() value for very little cost.
// We'll then multiply 1/sqrt times the original value to get the sqrt.
// This is about 12.4 times faster than sqrt() and according to my testing (not exhaustive)
// it returns fairly accurate results (error below 1.0e-5 up to 100000.0 in 0.1 increments).

static inline float idSqrt( float x ) {
	const float half = 0.5;
	const float one = 1.0;
	float B, y0, y1;

	// This'll NaN if it hits frsqrte. Handle both +0.0 and -0.0
	if ( fabs( x ) == 0.0 ) {
		return x;
	}
	B = x;

#ifdef __GNUC__
	asm ( "frsqrte %0,%1" : "=f" ( y0 ) : "f" ( B ) );
#else
	y0 = __frsqrte( B );
#endif
	/* First refinement step */

	y1 = y0 + half * y0 * ( one - B * y0 * y0 );

	/* Second refinement step -- copy the output of the last step to the input of this step */

	y0 = y1;
	y1 = y0 + half * y0 * ( one - B * y0 * y0 );

	/* Get sqrt(x) from x * 1/sqrt(x) */
	return x * y1;
}
#define sqrt idSqrt


#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

#include <MacTypes.h>
#define MAC_STATIC

#define CPUSTRING   "MacOS-PPC"

void Sys_PumpEvents( void );

static inline float idSqrt( float x ) {
	const float half = 0.5;
	const float one = 1.0;
	float B, y0, y1;

	// This'll NaN if it hits frsqrte. Handle both +0.0 and -0.0
	if ( fabs( x ) == 0.0 ) {
		return x;
	}
	B = x;

#ifdef __GNUC__
	asm ( "frsqrte %0,%1" : "=f" ( y0 ) : "f" ( B ) );
#else
	y0 = __frsqrte( B );
#endif
	/* First refinement step */

	y1 = y0 + half * y0 * ( one - B * y0 * y0 );

	/* Second refinement step -- copy the output of the last step to the input of this step */

	y0 = y1;
	y1 = y0 + half * y0 * ( one - B * y0 * y0 );

	/* Get sqrt(x) from x * 1/sqrt(x) */
	return x * y1;
}
#define sqrt idSqrt

#endif

//======================= LINUX DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __linux__

#define MAC_STATIC

#ifdef __i386__
#define CPUSTRING   "linux-i386"
#elif defined __axp__
#define CPUSTRING   "linux-alpha"
#else
#define CPUSTRING   "linux-other"
#endif

#endif

//=============================================================


enum {qfalse, qtrue};

#ifndef NULL
#define NULL ( (void *)0 )
#endif

#define MAX_QINT            0x7fffffff
#define MIN_QINT            ( -MAX_QINT - 1 )

// TTimo gcc: was missing, added from Q3 source
#ifndef max
#define max( x, y ) ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )
#define min( x, y ) ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )
#endif

// RF, this is just here so different elements of the engine can be aware of this setting as it changes
#define MAX_SP_CLIENTS      64      // increasing this will increase memory usage significantly

#define MAX_SAY_TEXT        150

// print levels from renderer (FIXME: set up for game / cgame?)
typedef enum {
	PRINT_ALL,
	PRINT_DEVELOPER,        // only print when "developer 1"
	PRINT_WARNING,
	PRINT_ERROR
} printParm_t;

#ifdef  ERR_FATAL
#undef  ERR_FATAL               // this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,                  // exit the entire game with a popup window
	ERR_DROP,                   // print to console and disconnect from game
	ERR_SERVERDISCONNECT,       // don't kill server
	ERR_DISCONNECT,             // client disconnected from the server
	ERR_NEED_CD                 // pop up the need-cd dialog
} errorParm_t;


// font rendering values used by ui and cgame

#define PROP_GAP_WIDTH          3
#define PROP_SPACE_WIDTH        8
#define PROP_HEIGHT             27
#define PROP_SMALL_SIZE_SCALE   0.75

#define BLINK_DIVISOR           200
#define PULSE_DIVISOR           75

#define UI_LEFT         0x00000000  // default
#define UI_CENTER       0x00000001
#define UI_RIGHT        0x00000002
#define UI_FORMATMASK   0x00000007
#define UI_SMALLFONT    0x00000010
#define UI_BIGFONT      0x00000020  // default
#define UI_GIANTFONT    0x00000040
#define UI_DROPSHADOW   0x00000800
#define UI_BLINK        0x00001000
#define UI_INVERSE      0x00002000
#define UI_PULSE        0x00004000
// JOSEPH 10-24-99
#define UI_MENULEFT     0x00008000
#define UI_MENURIGHT    0x00010000
#define UI_EXSMALLFONT  0x00020000
#define UI_MENUFULL     0x00080000
// END JOSEPH

#define UI_SMALLFONT75  0x00100000

#if defined( _DEBUG ) && !defined( BSPC )
	#define HUNK_DEBUG
#endif

typedef enum {
	h_high,
	h_low,
	h_dontcare
} ha_pref;

#ifdef HUNK_DEBUG
#define Hunk_Alloc( size, preference )              Hunk_AllocDebug( size, preference, # size, __FILE__, __LINE__ )
void *Hunk_AllocDebug( int size, ha_pref preference, char *label, char *file, int line );
#else
void *Hunk_Alloc( int size, ha_pref preference );
#endif

/*
==============================================================

MATHLIB

==============================================================
*/


// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

#define TINYCHAR_WIDTH      ( SMALLCHAR_WIDTH )
#define TINYCHAR_HEIGHT     ( SMALLCHAR_HEIGHT / 2 )

#define SMALLCHAR_WIDTH     8
#define SMALLCHAR_HEIGHT    16

#define BIGCHAR_WIDTH       16
#define BIGCHAR_HEIGHT      16

#define GIANTCHAR_WIDTH     32
#define GIANTCHAR_HEIGHT    48

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;

extern vec4_t g_color_table[8];

#define MAKERGB( v, r, g, b ) v[0] = r; v[1] = g; v[2] = b
#define MAKERGBA( v, r, g, b, a ) v[0] = r; v[1] = g; v[2] = b; v[3] = a

unsigned ColorBytes3( float r, float g, float b );

float NormalizeColor( const vec3_t in, vec3_t out );

int     Q_rand( int *seed );
float   Q_random( int *seed );
float   Q_crandom( int *seed );

void vectoangles( const vec3_t value1, vec3_t angles );
// TTimo: const vec_t ** would require explicit casts for ANSI C conformance
// see unix/const-arg.c
void AxisToAngles( /*const*/ vec3_t axis[3], vec3_t angles );
float VectorDistance( vec3_t v1, vec3_t v2 );

// Ridah
void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj );
// done.

//=============================================

float Com_Clamp( float min, float max, float value );

qboolean COM_BitCheck( const int array[], int bitNum );
void COM_BitSet( int array[], int bitNum );
void COM_BitClear( int array[], int bitNum );

#define MAX_TOKENLENGTH     1024

#ifndef TT_STRING
//token types
#define TT_STRING                   1           // string
#define TT_LITERAL                  2           // literal
#define TT_NUMBER                   3           // number
#define TT_NAME                     4           // name
#define TT_PUNCTUATION              5           // punctuation
#endif

typedef struct pc_token_s
{
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
} pc_token_t;

//=============================================

#ifdef _WIN32
#define Q_putenv _putenv
#else
#define Q_putenv putenv
#endif

// strlen that discounts Quake color sequences
int Q_PrintStrlen( const char *string );
// removes color sequences from string
char *Q_CleanStr( char *string );

//=============================================

// 64-bit integers for global rankings interface
// implemented as a struct for qvm compatibility
typedef struct
{
	byte b0;
	byte b1;
	byte b2;
	byte b3;
	byte b4;
	byte b5;
	byte b6;
	byte b7;
} qint64;

//=============================================

float   *tv( float x, float y, float z );

//=============================================

// this is only here so the functions in q_shared.c and bg_*.c can link
void QDECL Com_Error( int level, const char *error, ... );
void QDECL Com_Printf( const char *msg, ... );

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/
#define ANIM_BITS       10

#define SNAPFLAG_RATE_DELAYED   1
#define SNAPFLAG_NOT_ACTIVE     2   // snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT    4   // toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define MAX_LOCATIONS       64

#define MAX_SOUNDS          256     // so they cannot be blindly increased


#define MAX_PARTICLES_AREAS     128

#define MAX_MULTI_SPAWNTARGETS  16 // JPW NERVE

#define MAX_DLIGHT_CONFIGSTRINGS    128
#define MAX_CLIPBOARD_CONFIGSTRINGS 64
#define MAX_SPLINE_CONFIGSTRINGS    64

#define PARTICLE_SNOW128    1
#define PARTICLE_SNOW64     2
#define PARTICLE_SNOW32     3
#define PARTICLE_SNOW256    0

#define PARTICLE_BUBBLE8    4
#define PARTICLE_BUBBLE16   5
#define PARTICLE_BUBBLE32   6
#define PARTICLE_BUBBLE64   7

#define RESERVED_CONFIGSTRINGS  2   // game can't modify below this, only the system can

//=========================================================
// shared by AI and animation scripting
//
typedef enum
{
	// TTimo gcc: enums don't go <=0 unless you force a value
	AISTATE_NULL = -1,
	AISTATE_RELAXED,
	AISTATE_QUERY,
	AISTATE_ALERT,
	AISTATE_COMBAT,

	MAX_AISTATES
} aistateEnum_t;

//=========================================================


// weapon grouping
#define MAX_WEAP_BANKS      12
#define MAX_WEAPS_IN_BANK   3
// JPW NERVE
#define MAX_WEAPS_IN_BANK_MP    8
#define MAX_WEAP_BANKS_MP   7
// jpw
#define MAX_WEAP_ALTS       WP_DYNAMITE2


//====================================================================


//
// wmusercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define BUTTON_ACTIVATE     64
//----(SA)	end



//----(SA) wolf buttons
#define WBUTTON_ATTACK2     1
#define WBUTTON_ZOOM        2
#define WBUTTON_QUICKGREN   4
#define WBUTTON_RELOAD      8
#define WBUTTON_LEANLEFT    16
#define WBUTTON_LEANRIGHT   32
#define WBUTTON_DROP        64 // JPW NERVE

// unused
#define WBUTTON_EXTRA7      128
//----(SA) end

#define MOVE_RUN            120         // if forwardmove or rightmove are >= MOVE_RUN,
										// then Q3BUTTON_WALKING should be set

#define MP_TEAM_OFFSET      6
#define MP_CLASS_OFFSET     4
#define MP_WEAPON_OFFSET    0

#define MP_TEAM_BITS        2
#define MP_CLASS_BITS       2
#define MP_WEAPON_BITS      4

#define MP_TEAM_MASK        0xC0
#define MP_CLASS_MASK       0x30
#define MP_WEAPON_MASK      0x0F

//===================================================================

// RF, put this here so we have a central means of defining a Zombie (kind of a hack, but this is to minimize bandwidth usage)
#define SET_FLAMING_ZOMBIE( x,y ) ( x.frame = y )
#define IS_FLAMING_ZOMBIE( x )    ( x.frame == 1 )

// real time
//=============================================


typedef struct qtime_s {
	int tm_sec;     /* seconds after the minute - [0,59] */
	int tm_min;     /* minutes after the hour - [0,59] */
	int tm_hour;    /* hours since midnight - [0,23] */
	int tm_mday;    /* day of the month - [1,31] */
	int tm_mon;     /* months since January - [0,11] */
	int tm_year;    /* years since 1900 */
	int tm_wday;    /* days since Sunday - [0,6] */
	int tm_yday;    /* days since January 1 - [0,365] */
	int tm_isdst;   /* daylight savings time flag */
} qtime_t;


// server browser sources
#define AS_LOCAL        0
#define AS_GLOBAL       1           // NERVE - SMF - modified
#define AS_FAVORITES    2
#define AS_MPLAYER      3

typedef enum _flag_status {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,         // CTF
	FLAG_TAKEN_RED,     // One Flag CTF
	FLAG_TAKEN_BLUE,    // One Flag CTF
	FLAG_DROPPED
} flagStatus_t;



#define MAX_PINGREQUESTS            16
#define MAX_SERVERSTATUSREQUESTS    16

#define SAY_ALL     0
#define SAY_TEAM    1
#define SAY_TELL    2

#define CDKEY_LEN 16
#define CDCHKSUM_LEN 2

// NERVE - SMF - wolf server/game states
typedef enum {
	GS_INITIALIZE = -1,
	GS_PLAYING,
	GS_WARMUP_COUNTDOWN,
	GS_WARMUP,
	GS_INTERMISSION,
	GS_WAITING_FOR_PLAYERS,
	GS_RESET
} gamestate_t;

// TTimo - voting config flags
#define VOTEFLAGS_RESTART           ( 1 << 0 )
#define VOTEFLAGS_RESETMATCH    ( 1 << 1 )
#define VOTEFLAGS_STARTMATCH    ( 1 << 2 )
#define VOTEFLAGS_NEXTMAP           ( 1 << 3 )
#define VOTEFLAGS_SWAP              ( 1 << 4 )
#define VOTEFLAGS_TYPE              ( 1 << 5 )
#define VOTEFLAGS_KICK              ( 1 << 6 )
#define VOTEFLAGS_MAP                   ( 1 << 7 )

#endif  // __Q_SHARED_H
