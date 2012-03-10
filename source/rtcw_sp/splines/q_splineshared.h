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

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_splineshared.h -- included first by ALL program modules.
// these are the definitions that have no dependance on
// central system services, and can be used by any part
// of the program without any state issues.

// A user mod should never modify this file

#include "../../wolfcore/core.h"

#define Q3_VERSION      "DOOM 0.01"

// alignment macros for SIMD
#define ALIGN_ON
#define ALIGN_OFF

#include <assert.h>
#include <time.h>
#include <ctype.h>
#ifdef WIN32                // mac doesn't have malloc.h
#include <malloc.h>          // for _alloca()
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
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP"
#endif
#else
#ifdef _M_IX86
#define CPUSTRING   "win-x86-debug"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP-debug"
#endif
#endif


#endif

//======================= MAC OS X SERVER DEFINES =====================

#if defined( __MACH__ ) && defined( __APPLE__ )

#define MAC_STATIC

#ifdef __ppc__
#define CPUSTRING   "MacOSXS-ppc"
#elif defined __i386__
#define CPUSTRING   "MacOSXS-i386"
#else
#define CPUSTRING   "MacOSXS-other"
#endif

#define GAME_HARD_LINKED
#define CGAME_HARD_LINKED
#define UI_HARD_LINKED
#define _alloca alloca

#undef ALIGN_ON
#undef ALIGN_OFF
#define ALIGN_ON        # pragma align( 16 )
#define ALIGN_OFF       # pragma align()

void *osxAllocateMemory( long size );
void osxFreeMemory( void *pointer );

#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

//DAJ #define	MAC_STATIC static
#define MAC_STATIC

#define CPUSTRING   "MacOS-PPC"

void Sys_PumpEvents( void );

#endif

#ifdef __MRC__

#define MAC_STATIC

#define CPUSTRING   "MacOS-PPC"

void Sys_PumpEvents( void );

#undef QDECL
#define QDECL   __cdecl

#define _alloca alloca
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



typedef enum {qfalse, qtrue};

#define EQUAL_EPSILON   0.001

typedef int qhandle_t;
typedef int sfxHandle_t;

typedef enum {
	INVALID_JOINT = -1
} jointHandle_t;

#ifndef NULL
#define NULL ( (void *)0 )
#endif

#define MAX_QINT            0x7fffffff
#define MIN_QINT            ( -MAX_QINT - 1 )

#ifndef max
#define max( x, y ) ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )
#define min( x, y ) ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )
#endif

#ifndef sign
#define sign( f )   ( ( f > 0 ) ? 1 : ( ( f < 0 ) ? -1 : 0 ) )
#endif

#define MAX_STRING_TOKENS   256     // max tokens resulting from Cmd_TokenizeString

#define MAX_NAME_LENGTH     32      // max length of a client name

//
// these aren't needed by any of the VMs.  put in another header?
//
#define MAX_MAP_AREA_BYTES      32      // bit vector of area visibility

#undef ERR_FATAL                        // malloc.h on unix

// parameters to the main Error routine
typedef enum {
	ERR_NONE,
	ERR_FATAL,                  // exit the entire game with a popup window
	ERR_DROP,                   // print to console and disconnect from game
	ERR_DISCONNECT,             // don't kill server
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


/*
==============================================================

MATHLIB

==============================================================
*/
#ifdef __cplusplus          // so we can include this in C code
#define Q_PI    3.14159265358979323846

#include "math_vector.h"
#include "math_angles.h"
#include "math_matrix.h"
#include "math_quaternion.h"

class idVec3;                       // for defining vectors
typedef idVec3 &vec3_p;             // for passing vectors as function arguments
typedef const idVec3 &vec3_c;       // for passing vectors as const function arguments

class angles_t;                     // for defining angle vectors
typedef angles_t &angles_p;         // for passing angles as function arguments
typedef const angles_t &angles_c;   // for passing angles as const function arguments

class mat3_t;                       // for defining matrices
typedef mat3_t &mat3_p;             // for passing matrices as function arguments
typedef const mat3_t &mat3_c;       // for passing matrices as const function arguments



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

extern idVec4 colorBlack;
extern idVec4 colorRed;
extern idVec4 colorGreen;
extern idVec4 colorBlue;
extern idVec4 colorYellow;
extern idVec4 colorMagenta;
extern idVec4 colorCyan;
extern idVec4 colorWhite;
extern idVec4 colorLtGrey;
extern idVec4 colorMdGrey;
extern idVec4 colorDkGrey;

extern idVec4 g_color_table[8];

#define MAKERGB( v, r, g, b ) v[0] = r; v[1] = g; v[2] = b
#define MAKERGBA( v, r, g, b, a ) v[0] = r; v[1] = g; v[2] = b; v[3] = a

extern idVec4 vec4_origin;

// TTimo
// handy stuff when tracking isnan problems
#ifndef NDEBUG
#define CHECK_NAN( x ) assert( !IS_NAN( x ) )
#define CHECK_NAN_VEC( v ) assert( !IS_NAN( v[0] ) && !IS_NAN( v[1] ) && !IS_NAN( v[2] ) )
#else
#define CHECK_NAN
#define CHECK_NAN_VEC
#endif

float NormalizeColor( vec3_c in, vec3_p out );

void VectorPolar( vec3_p v, float radius, float theta, float phi );
void VectorSnap( vec3_p v );
void Vector53Copy( const idVec5_t &in, vec3_p out );
void Vector5Scale( const idVec5_t &v, float scale, idVec5_t &out );
void Vector5Add( const idVec5_t &va, const idVec5_t &vb, idVec5_t &out );
void VectorRotate3( vec3_c vIn, vec3_c vRotation, vec3_p out );
void VectorRotate3Origin( vec3_c vIn, vec3_c vRotation, vec3_c vOrigin, vec3_p out );


int     Q_rand( int *seed );
float   Q_random( int *seed );
float   Q_crandom( int *seed );

float Q_rint( float in );

void vectoangles( vec3_c value1, angles_p angles );

qboolean AxisRotated( mat3_c in );          // assumes a non-degenerate axis

int SignbitsForNormal( vec3_c normal );

void MatrixInverseMultiply( mat3_c in1, mat3_c in2, mat3_p out );   // in2 is transposed during multiply
void MatrixTransformVector( vec3_c in, mat3_c matrix, vec3_p out );
void MatrixProjectVector( vec3_c in, mat3_c matrix, vec3_p out ); // Places the vector into a new coordinate system.

float TriangleArea( vec3_c a, vec3_c b, vec3_c c );
#endif                                      // __cplusplus

//=============================================

float Com_Clamp( float min, float max, float value );

#define FILE_HASH_SIZE      1024
int Com_HashString( const char *fname );

/*
=====================================================================================

SCRIPT PARSING

=====================================================================================
*/

// this just controls the comment printing, it doesn't actually load a file
void Com_BeginParseSession( const char *filename );
void Com_EndParseSession( void );

int Com_GetCurrentParseLine( void );

// Will never return NULL, just empty strings.
// An empty string will only be returned at end of file.
// ParseOnLine will return empty if there isn't another token on this line

// this funny typedef just means a moving pointer into a const char * buffer
const char *Com_Parse( const char *( *data_p ) );
const char *Com_ParseOnLine( const char *( *data_p ) );
const char *Com_ParseRestOfLine( const char *( *data_p ) );

void Com_UngetToken( void );

#ifdef __cplusplus
void Com_MatchToken( const char *( *buf_p ), const char *match, qboolean warning = qfalse );
#else
void Com_MatchToken( const char *( *buf_p ), const char *match, qboolean warning );
#endif

void Com_ScriptError( const char *msg, ... );
void Com_ScriptWarning( const char *msg, ... );

void Com_SkipBracedSection( const char *( *program ) );
void Com_SkipRestOfLine( const char *( *data ) );

float Com_ParseFloat( const char *( *buf_p ) );
int Com_ParseInt( const char *( *buf_p ) );

void Com_Parse1DMatrix( const char *( *buf_p ), int x, float *m );
void Com_Parse2DMatrix( const char *( *buf_p ), int y, int x, float *m );
void Com_Parse3DMatrix( const char *( *buf_p ), int z, int y, int x, float *m );

//=====================================================================================

// String::Length that discounts Quake color sequences
int Q_PrintStrlen( const char *string );
// removes color sequences from string
char *Q_CleanStr( char *string );

//=============================================
#ifdef __cplusplus
//
// mapfile parsing
//
typedef struct ePair_s {
	char    *key;
	char    *value;
} ePair_t;

typedef struct mapSide_s {
	char material[MAX_QPATH];
	idVec4 plane;
	idVec4 textureVectors[2];
} mapSide_t;

typedef struct {
	int numSides;
	mapSide_t   **sides;
} mapBrush_t;

typedef struct {
	idVec3 xyz;
	float st[2];
} patchVertex_t;

typedef struct {
	char material[MAX_QPATH];
	int width, height;
	patchVertex_t   *patchVerts;
} mapPatch_t;

typedef struct {
	char modelName[MAX_QPATH];
	float matrix[16];
} mapModel_t;

typedef struct mapPrimitive_s {
	int numEpairs;
	ePair_t         **ePairs;

	// only one of these will be non-NULL
	mapBrush_t      *brush;
	mapPatch_t      *patch;
	mapModel_t      *model;
} mapPrimitive_t;

typedef struct mapEntity_s {
	int numPrimitives;
	mapPrimitive_t  **primitives;

	int numEpairs;
	ePair_t         **ePairs;
} mapEntity_t;

typedef struct {
	int numEntities;
	mapEntity_t     **entities;
} mapFile_t;


// the order of entities, brushes, and sides will be maintained, the
// lists won't be swapped on each load or save
mapFile_t *ParseMapFile( const char *text );
void FreeMapFile( mapFile_t *mapFile );
void WriteMapFile( const mapFile_t *mapFile, FILE *f );

// key names are case-insensitive
const char  *ValueForMapEntityKey( const mapEntity_t *ent, const char *key );
float   FloatForMapEntityKey( const mapEntity_t *ent, const char *key );
qboolean    GetVectorForMapEntityKey( const mapEntity_t *ent, const char *key, idVec3 &vec );

typedef struct {
	idVec3 xyz;
	idVec2 st;
	idVec3 normal;
	idVec3 tangents[2];
	byte smoothing[4];              // colors for silhouette smoothing
} bsp46_drawVert_t;

typedef struct {
	int width, height;
	bsp46_drawVert_t  *verts;
} drawVertMesh_t;

// Tesselate a map patch into smoothed, drawable vertexes
// MaxError of around 4 is reasonable
drawVertMesh_t *SubdivideMapPatch( const mapPatch_t *patch, float maxError );
#endif          // __cplusplus

//=========================================

void QDECL Com_Error( int level, const char *error, ... );
void QDECL Com_Printf( const char *msg, ... );
void QDECL Com_DPrintf( const char *msg, ... );


typedef struct {
	qboolean frameMemory;
	int currentElements;
	int maxElements;            // will reallocate and move when exceeded
	void    **elements;
} growList_t;

// you don't need to init the growlist if you don't mind it growing and moving
// the list as it expands
void        Com_InitGrowList( growList_t *list, int maxElements );
int         Com_AddToGrowList( growList_t *list, void *data );
void        *Com_GrowListElement( const growList_t *list, int index );
int         Com_IndexForGrowListElement( const growList_t *list, const void *element );

#endif  // __Q_SHARED_H

