/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// common.c -- misc functions used in client and server

#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../client/client.h"
#include "../../server/server.h"
#include "../../server/tech3/local.h"
#include <setjmp.h>

int demo_protocols[] =
{ 66, 67, 68, 0 };

#define DEF_COMZONEMEGS "16"

jmp_buf abortframe;		// an ERR_DROP occured, exit the entire frame


FILE* debuglogfile;
static fileHandle_t logfile;

Cvar* com_speeds;
Cvar* com_fixedtime;
Cvar* com_dropsim;			// 0.0 to 1.0, simulated packet drops
Cvar* com_maxfps;
Cvar* com_timedemo;
Cvar* com_logfile;			// 1 = buffer log, 2 = flush after each print
Cvar* com_showtrace;
Cvar* com_version;
Cvar* com_blood;
Cvar* com_buildScript;		// for automated data building scripts
Cvar* com_introPlayed;
Cvar* com_cameraMode;
#if defined(_WIN32) && defined(_DEBUG)
Cvar* com_noErrorInterrupt;
#endif

// com_speeds times
int time_game;
int time_frontend;			// renderer frontend time
int time_backend;			// renderer backend time

int com_frameNumber;

qboolean com_fullyInitialized;

char com_errorMessage[MAXPRINTMSG];

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3)));
};

static idCommonLocal commonLocal;
idCommon* common = &commonLocal;

void idCommonLocal::Printf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_Printf("%s", string);
}

void idCommonLocal::DPrintf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_DPrintf("%s", string);
}

void idCommonLocal::Error(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw DropException(string);
}

void idCommonLocal::FatalError(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw Exception(string);
}

void idCommonLocal::EndGame(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw EndGameException(string);
}

void Com_WriteConfig_f(void);
void CIN_CloseAllVideos();

//============================================================================

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void Com_Printf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];
	static qboolean opening_qconsole = false;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	if (rd_buffer)
	{
		if ((String::Length(msg) + String::Length(rd_buffer)) > (rd_buffersize - 1))
		{
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		String::Cat(rd_buffer, rd_buffersize, msg);
		// TTimo nooo .. that would defeat the purpose
		//rd_flush(rd_buffer);
		//*rd_buffer = 0;
		return;
	}

	// echo to console if we're not a dedicated server
	if (com_dedicated && !com_dedicated->integer)
	{
		Con_ConsolePrint(msg);
	}

	// echo to dedicated console and early console
	Sys_Print(msg);

	// logfile
	if (com_logfile && com_logfile->integer)
	{
		// TTimo: only open the qconsole.log if the filesystem is in an initialized state
		//   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
		if (!logfile && FS_Initialized() && !opening_qconsole)
		{
			struct tm* newtime;
			time_t aclock;

			opening_qconsole = true;

			time(&aclock);
			newtime = localtime(&aclock);

			logfile = FS_FOpenFileWrite("qconsole.log");
			common->Printf("logfile opened on %s\n", asctime(newtime));
			if (com_logfile->integer > 1)
			{
				// force it to not buffer so we get valid
				// data even if we are crashing
				FS_ForceFlush(logfile);
			}

			opening_qconsole = false;
		}
		if (logfile && FS_Initialized())
		{
			FS_Write(msg, String::Length(msg), logfile);
		}
	}
}


/*
================
Com_DPrintf

A common->Printf that only shows up if the "developer" cvar is set
================
*/
void Com_DPrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	if (!com_developer || !com_developer->integer)
	{
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start(argptr,fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	common->Printf("%s", msg);
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Error(int code, const char* fmt, ...)
{
	va_list argptr;
	static int lastErrorTime;
	static int errorCount;
	int currentTime;

#if defined(_WIN32) && defined(_DEBUG) && !defined(_WIN64)
	if (code != ERR_DISCONNECT)
	{
		if (!com_noErrorInterrupt->integer)
		{
			__asm {
				int 0x03
			}
		}
	}
#endif

	// when we are running automated scripts, make sure we
	// know if anything failed
	if (com_buildScript && com_buildScript->integer)
	{
		code = ERR_FATAL;
	}

	// make sure we can get at our local stuff
	FS_PureServerSetLoadedPaks("", "");

	// if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
	currentTime = Sys_Milliseconds();
	if (currentTime - lastErrorTime < 100)
	{
		if (++errorCount > 3)
		{
			code = ERR_FATAL;
		}
	}
	else
	{
		errorCount = 0;
	}
	lastErrorTime = currentTime;

	if (com_errorEntered)
	{
		Sys_Error("recursive error after: %s", com_errorMessage);
	}
	com_errorEntered = true;

	va_start(argptr,fmt);
	Q_vsnprintf(com_errorMessage, MAXPRINTMSG, fmt,argptr);
	va_end(argptr);

	if (code != ERR_DISCONNECT)
	{
		Cvar_Set("com_errorMessage", com_errorMessage);
	}

	if (code == ERR_SERVERDISCONNECT)
	{
		CL_Disconnect(true);
		CL_FlushMemory();
		com_errorEntered = false;
		longjmp(abortframe, -1);
	}
	else if (code == ERR_DROP || code == ERR_DISCONNECT)
	{
		common->Printf("********************\nERROR: %s\n********************\n", com_errorMessage);
		SVT3_Shutdown(va("Server crashed: %s\n",  com_errorMessage));
		CL_Disconnect(true);
		CL_FlushMemory();
		com_errorEntered = false;
		longjmp(abortframe, -1);
	}
	else
	{
		CL_Shutdown();
		SVT3_Shutdown(va("Server fatal crashed: %s\n", com_errorMessage));
	}

	Com_Shutdown();

	Sys_Error("%s", com_errorMessage);
}


/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit_f(void)
{
	// don't try to shutdown if we are in a recursive error
	if (!com_errorEntered)
	{
		SVT3_Shutdown("Server quit\n");
		CL_Shutdown();
		Com_Shutdown();
		FS_Shutdown();
	}
	Sys_Quit();
}



/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define MAX_CONSOLE_LINES   32
int com_numConsoleLines;
char* com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine(char* commandLine)
{
	int inq = 0;
	com_consoleLines[0] = commandLine;
	com_numConsoleLines = 1;

	while (*commandLine)
	{
		if (*commandLine == '"')
		{
			inq = !inq;
		}
		// look for a + seperating character
		// if commandLine came from a file, we might have real line seperators
		if ((*commandLine == '+' && !inq) || *commandLine == '\n'  || *commandLine == '\r')
		{
			if (com_numConsoleLines == MAX_CONSOLE_LINES)
			{
				return;
			}
			com_consoleLines[com_numConsoleLines] = commandLine + 1;
			com_numConsoleLines++;
			*commandLine = 0;
		}
		commandLine++;
	}
}


/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of q3config.cfg
===================
*/
qboolean Com_SafeMode(void)
{
	int i;

	for (i = 0; i < com_numConsoleLines; i++)
	{
		Cmd_TokenizeString(com_consoleLines[i]);
		if (!String::ICmp(Cmd_Argv(0), "safe") ||
			!String::ICmp(Cmd_Argv(0), "cvar_restart"))
		{
			com_consoleLines[i][0] = 0;
			return true;
		}
	}
	return false;
}


/*
===============
Com_StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets shouls
be after execing the config and default.
===============
*/
void Com_StartupVariable(const char* match)
{
	int i;
	char* s;
	Cvar* cv;

	for (i = 0; i < com_numConsoleLines; i++)
	{
		Cmd_TokenizeString(com_consoleLines[i]);
		if (String::Cmp(Cmd_Argv(0), "set"))
		{
			continue;
		}

		s = Cmd_Argv(1);
		if (!match || !String::Cmp(s, match))
		{
			Cvar_Set(s, Cmd_Argv(2));
			cv = Cvar_Get(s, "", 0);
			cv->flags |= CVAR_USER_CREATED;
//			com_consoleLines[i] = 0;
		}
	}
}


/*
=================
Com_AddStartupCommands

Adds command line parameters as script statements
Commands are seperated by + signs

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Com_AddStartupCommands(void)
{
	int i;
	qboolean added;

	added = false;
	// quote every token, so args with semicolons can work
	for (i = 0; i < com_numConsoleLines; i++)
	{
		if (!com_consoleLines[i] || !com_consoleLines[i][0])
		{
			continue;
		}

		// set commands won't override menu startup
		if (String::NICmp(com_consoleLines[i], "set", 3))
		{
			added = true;
		}
		Cbuf_AddText(com_consoleLines[i]);
		Cbuf_AddText("\n");
	}

	return added;
}

/*
==============================================================================

                        ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

#define ZONEID  0x1d4a11
#define MINFRAGMENT 64

typedef struct zonedebug_s
{
	char* label;
	char* file;
	int line;
	int allocSize;
} zonedebug_t;

typedef struct memblock_s
{
	int size;				// including the header and possibly tiny fragments
	int tag;				// a tag of 0 is a free block
	struct memblock_s* next, * prev;
	int id;					// should be ZONEID
} memblock_t;

typedef struct
{
	int size;				// total bytes malloced, including header
	int used;				// total bytes used
	memblock_t blocklist;	// start / end cap for linked list
	memblock_t* rover;
} memzone_t;

// main zone for all "dynamic" memory allocation
memzone_t* mainzone;
// we also have a small zone for small allocations that would only
// fragment the main zone (think of cvar and cmd strings)
memzone_t* smallzone;

void Z_CheckHeap(void);

/*
========================
Z_ClearZone
========================
*/
void Z_ClearZone(memzone_t* zone, int size)
{
	memblock_t* block;

	// set the entire zone to one free block

	zone->blocklist.next = zone->blocklist.prev = block =
													  (memblock_t*)((byte*)zone + sizeof(memzone_t));
	zone->blocklist.tag = 1;	// in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->rover = block;
	zone->size = size;
	zone->used = 0;

	block->prev = block->next = &zone->blocklist;
	block->tag = 0;			// free block
	block->id = ZONEID;
	block->size = size - sizeof(memzone_t);
}

/*
========================
Z_Free
========================
*/
void Z_Free(void* ptr)
{
	memblock_t* block, * other;
	memzone_t* zone;

	if (!ptr)
	{
		common->Error("Z_Free: NULL pointer");
	}

	block = (memblock_t*)((byte*)ptr - sizeof(memblock_t));
	if (block->id != ZONEID)
	{
		common->FatalError("Z_Free: freed a pointer without ZONEID");
	}
	if (block->tag == 0)
	{
		common->FatalError("Z_Free: freed a freed pointer");
	}
	// if static memory
	if (block->tag == TAG_STATIC)
	{
		return;
	}

	// check the memory trash tester
	if (*(int*)((byte*)block + block->size - 4) != ZONEID)
	{
		common->FatalError("Z_Free: memory block wrote past end");
	}

	if (block->tag == TAG_SMALL)
	{
		zone = smallzone;
	}
	else
	{
		zone = mainzone;
	}

	zone->used -= block->size;
	// set the block to something that should cause problems
	// if it is referenced...
	Com_Memset(ptr, 0xaa, block->size - sizeof(*block));

	block->tag = 0;		// mark as free

	other = block->prev;
	if (!other->tag)
	{
		// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if (block == zone->rover)
		{
			zone->rover = other;
		}
		block = other;
	}

	zone->rover = block;

	other = block->next;
	if (!other->tag)
	{
		// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if (other == zone->rover)
		{
			zone->rover = block;
		}
	}
}

/*
================
Z_TagMalloc
================
*/
void* Z_TagMalloc(int size, int tag)
{
	int extra, allocSize;
	memblock_t* start, * rover, * newb, * base;
	memzone_t* zone;

	if (!tag)
	{
		common->FatalError("Z_TagMalloc: tried to use a 0 tag");
	}

	if (tag == TAG_SMALL)
	{
		zone = smallzone;
	}
	else
	{
		zone = mainzone;
	}

	allocSize = size;
	//
	// scan through the block list looking for the first free block
	// of sufficient size
	//
	size += sizeof(memblock_t);	// account for size of block header
	size += 4;					// space for memory trash tester
	size = (size + 3) & ~3;		// align to 32 bit boundary

	base = rover = zone->rover;
	start = base->prev;

	do
	{
		if (rover == start)
		{
			// scaned all the way around the list
			common->FatalError("Z_Malloc: failed on allocation of %i bytes from the %s zone",
				size, zone == smallzone ? "small" : "main");
			return NULL;
		}
		if (rover->tag)
		{
			base = rover = rover->next;
		}
		else
		{
			rover = rover->next;
		}
	}
	while (base->tag || base->size < size);

	//
	// found a block big enough
	//
	extra = base->size - size;
	if (extra > MINFRAGMENT)
	{
		// there will be a free fragment after the allocated block
		newb = (memblock_t*)((byte*)base + size);
		newb->size = extra;
		newb->tag = 0;			// free block
		newb->prev = base;
		newb->id = ZONEID;
		newb->next = base->next;
		newb->next->prev = newb;
		base->next = newb;
		base->size = size;
	}

	base->tag = tag;			// no longer a free block

	zone->rover = base->next;	// next allocation will start looking here
	zone->used += base->size;	//

	base->id = ZONEID;

	// marker for memory trash testing
	*(int*)((byte*)base + base->size - 4) = ZONEID;

	return (void*)((byte*)base + sizeof(memblock_t));
}

/*
========================
Z_Malloc
========================
*/
void* Z_Malloc(int size)
{
	void* buf;

	//Z_CheckHeap ();	// DEBUG

	buf = Z_TagMalloc(size, TAG_GENERAL);
	Com_Memset(buf, 0, size);

	return buf;
}

void* S_Malloc(int size)
{
	return Z_TagMalloc(size, TAG_SMALL);
}

/*
========================
Z_CheckHeap
========================
*/
void Z_CheckHeap(void)
{
	memblock_t* block;

	for (block = mainzone->blocklist.next;; block = block->next)
	{
		if (block->next == &mainzone->blocklist)
		{
			break;			// all blocks have been hit
		}
		if ((byte*)block + block->size != (byte*)block->next)
		{
			common->FatalError("Z_CheckHeap: block size does not touch the next block\n");
		}
		if (block->next->prev != block)
		{
			common->FatalError("Z_CheckHeap: next block doesn't have proper back link\n");
		}
		if (!block->tag && !block->next->tag)
		{
			common->FatalError("Z_CheckHeap: two consecutive free blocks\n");
		}
	}
}

/*
========================
Z_LogZoneHeap
========================
*/
void Z_LogZoneHeap(memzone_t* zone, const char* name)
{
	memblock_t* block;
	char buf[4096];
	int size, allocSize, numBlocks;

	if (!logfile || !FS_Initialized())
	{
		return;
	}
	size = allocSize = numBlocks = 0;
	String::Sprintf(buf, sizeof(buf), "\r\n================\r\n%s log\r\n================\r\n", name);
	FS_Write(buf, String::Length(buf), logfile);
	for (block = zone->blocklist.next; block->next != &zone->blocklist; block = block->next)
	{
		if (block->tag)
		{
			size += block->size;
			numBlocks++;
		}
	}
	allocSize = numBlocks * sizeof(memblock_t);	// + 32 bit alignment
	String::Sprintf(buf, sizeof(buf), "%d %s memory in %d blocks\r\n", size, name, numBlocks);
	FS_Write(buf, String::Length(buf), logfile);
	String::Sprintf(buf, sizeof(buf), "%d %s memory overhead\r\n", size - allocSize, name);
	FS_Write(buf, String::Length(buf), logfile);
}

/*
========================
Z_LogHeap
========================
*/
void Z_LogHeap(void)
{
	Z_LogZoneHeap(mainzone, "MAIN");
	Z_LogZoneHeap(smallzone, "SMALL");
}

// static mem blocks to reduce a lot of small zone overhead
typedef struct memstatic_s
{
	memblock_t b;
	byte mem[2];
} memstatic_t;

// bk001204 - initializer brackets
memstatic_t emptystring =
{ {(sizeof(memblock_t) + 2 + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'\0', '\0'} };
memstatic_t numberstring[] = {
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'0', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'1', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'2', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'3', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'4', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'5', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'6', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'7', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'8', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'9', '\0'} }
};

/*
========================
CopyString

 NOTE:	never write over the memory CopyString returns because
        memory from a memstatic_t might be returned
========================
*/
char* CopyString(const char* in)
{
	char* out;

	if (!in[0])
	{
		return ((char*)&emptystring) + sizeof(memblock_t);
	}
	else if (!in[1])
	{
		if (in[0] >= '0' && in[0] <= '9')
		{
			return ((char*)&numberstring[in[0] - '0']) + sizeof(memblock_t);
		}
	}
	out = (char*)S_Malloc(String::Length(in) + 1);
	String::Cpy(out, in);
	return out;
}

static int s_zoneTotal;
static int s_smallZoneTotal;


/*
=================
Com_Meminfo_f
=================
*/
void Com_Meminfo_f(void)
{
	memblock_t* block;
	int zoneBytes, zoneBlocks;
	int smallZoneBytes, smallZoneBlocks;

	zoneBytes = 0;
	zoneBlocks = 0;
	for (block = mainzone->blocklist.next;; block = block->next)
	{
		if (Cmd_Argc() != 1)
		{
			common->Printf("block:%p    size:%7i    tag:%3i\n",
				block, block->size, block->tag);
		}
		if (block->tag)
		{
			zoneBytes += block->size;
			zoneBlocks++;
		}

		if (block->next == &mainzone->blocklist)
		{
			break;			// all blocks have been hit
		}
		if ((byte*)block + block->size != (byte*)block->next)
		{
			common->Printf("ERROR: block size does not touch the next block\n");
		}
		if (block->next->prev != block)
		{
			common->Printf("ERROR: next block doesn't have proper back link\n");
		}
		if (!block->tag && !block->next->tag)
		{
			common->Printf("ERROR: two consecutive free blocks\n");
		}
	}

	smallZoneBytes = 0;
	smallZoneBlocks = 0;
	for (block = smallzone->blocklist.next;; block = block->next)
	{
		if (block->tag)
		{
			smallZoneBytes += block->size;
			smallZoneBlocks++;
		}

		if (block->next == &smallzone->blocklist)
		{
			break;			// all blocks have been hit
		}
	}

	common->Printf("%8i bytes total zone\n", s_zoneTotal);
	common->Printf("\n");
	common->Printf("%8i bytes in %i zone blocks\n", zoneBytes, zoneBlocks);
	common->Printf("        %8i bytes in dynamic other\n", zoneBytes);
	common->Printf("        %8i bytes in small Zone memory\n", smallZoneBytes);
}

/*
===============
Com_TouchMemory

Touch all known used data to make sure it is paged in
===============
*/
void Com_TouchMemory(void)
{
	int start, end;
	int i, j;
	int sum;
	memblock_t* block;

	Z_CheckHeap();

	start = Sys_Milliseconds();

	sum = 0;

	for (block = mainzone->blocklist.next;; block = block->next)
	{
		if (block->tag)
		{
			j = block->size >> 2;
			for (i = 0; i < j; i += 64)					// only need to touch each page
			{
				sum += ((int*)block)[i];
			}
		}
		if (block->next == &mainzone->blocklist)
		{
			break;			// all blocks have been hit
		}
	}

	end = Sys_Milliseconds();

	common->Printf("Com_TouchMemory: %i msec\n", end - start);
}



/*
=================
Com_InitZoneMemory
=================
*/
void Com_InitSmallZoneMemory(void)
{
	s_smallZoneTotal = 512 * 1024;
	// bk001205 - was malloc
	smallzone = (memzone_t*)calloc(s_smallZoneTotal, 1);
	if (!smallzone)
	{
		common->FatalError("Small zone data failed to allocate %1.1f megs", (float)s_smallZoneTotal / (1024 * 1024));
	}
	Z_ClearZone(smallzone, s_smallZoneTotal);

	return;
}

void Com_InitZoneMemory(void)
{
	Cvar* cv;
	// allocate the random block zone
	cv = Cvar_Get("com_zoneMegs", DEF_COMZONEMEGS, CVAR_LATCH2 | CVAR_ARCHIVE);

	if (cv->integer < 20)
	{
		s_zoneTotal = 1024 * 1024 * 16;
	}
	else
	{
		s_zoneTotal = cv->integer * 1024 * 1024;
	}

	// bk001205 - was malloc
	mainzone = (memzone_t*)calloc(s_zoneTotal, 1);
	if (!mainzone)
	{
		common->FatalError("Zone data failed to allocate %i megs", s_zoneTotal / (1024 * 1024));
	}
	Z_ClearZone(mainzone, s_zoneTotal);

}

/*
=================
Com_InitZoneMemory
=================
*/
void Com_InitHunkMemory(void)
{
	Cmd_AddCommand("meminfo", Com_Meminfo_f);
}

/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

// bk001129 - here we go again: upped from 64
// FIXME TTimo blunt upping from 256 to 1024
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=5
#define MAX_PUSHED_EVENTS               1024
// bk001129 - init, also static
static int com_pushedEventsHead = 0;
static int com_pushedEventsTail = 0;
// bk001129 - static
static sysEvent_t com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling(void)
{
	Com_StartupVariable("journal");
	com_journal = Cvar_Get("journal", "0", CVAR_INIT);
	if (!com_journal->integer)
	{
		return;
	}

	if (com_journal->integer == 1)
	{
		common->Printf("Journaling events\n");
		com_journalFile = FS_FOpenFileWrite("journal.dat");
		com_journalDataFile = FS_FOpenFileWrite("journaldata.dat");
	}
	else if (com_journal->integer == 2)
	{
		common->Printf("Replaying journaled events\n");
		FS_FOpenFileRead("journal.dat", &com_journalFile, true);
		FS_FOpenFileRead("journaldata.dat", &com_journalDataFile, true);
	}

	if (!com_journalFile || !com_journalDataFile)
	{
		Cvar_Set("com_journal", "0");
		com_journalFile = 0;
		com_journalDataFile = 0;
		common->Printf("Couldn't open journal files\n");
	}
}

/*
=================
Com_GetRealEvent
=================
*/
sysEvent_t  Com_GetRealEvent(void)
{
	int r;
	sysEvent_t ev;

	// either get an event from the system or the journal file
	if (com_journal->integer == 2)
	{
		r = FS_Read(&ev, sizeof(ev), com_journalFile);
		if (r != sizeof(ev))
		{
			common->FatalError("Error reading from journal file");
		}
		if (ev.evPtrLength)
		{
			ev.evPtr = Mem_Alloc(ev.evPtrLength);
			r = FS_Read(ev.evPtr, ev.evPtrLength, com_journalFile);
			if (r != ev.evPtrLength)
			{
				common->FatalError("Error reading from journal file");
			}
		}
	}
	else
	{
		ev = Sys_GetEvent();

		// write the journal value out if needed
		if (com_journal->integer == 1)
		{
			r = FS_Write(&ev, sizeof(ev), com_journalFile);
			if (r != sizeof(ev))
			{
				common->FatalError("Error writing to journal file");
			}
			if (ev.evPtrLength)
			{
				r = FS_Write(ev.evPtr, ev.evPtrLength, com_journalFile);
				if (r != ev.evPtrLength)
				{
					common->FatalError("Error writing to journal file");
				}
			}
		}
	}

	return ev;
}


/*
=================
Com_InitPushEvent
=================
*/
// bk001129 - added
void Com_InitPushEvent(void)
{
	// clear the static buffer array
	// this requires SE_NONE to be accepted as a valid but NOP event
	Com_Memset(com_pushedEvents, 0, sizeof(com_pushedEvents));
	// reset counters while we are at it
	// beware: GetEvent might still return an SE_NONE from the buffer
	com_pushedEventsHead = 0;
	com_pushedEventsTail = 0;
}


/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent(sysEvent_t* event)
{
	sysEvent_t* ev;
	static int printedWarning = 0;	// bk001129 - init, bk001204 - explicit int

	ev = &com_pushedEvents[com_pushedEventsHead & (MAX_PUSHED_EVENTS - 1)];

	if (com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS)
	{

		// don't print the warning constantly, or it can give time for more...
		if (!printedWarning)
		{
			printedWarning = true;
			common->Printf("WARNING: Com_PushEvent overflow\n");
		}

		if (ev->evPtr)
		{
			Mem_Free(ev->evPtr);
		}
		com_pushedEventsTail++;
	}
	else
	{
		printedWarning = false;
	}

	*ev = *event;
	com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t  Com_GetEvent(void)
{
	if (com_pushedEventsHead > com_pushedEventsTail)
	{
		com_pushedEventsTail++;
		return com_pushedEvents[(com_pushedEventsTail - 1) & (MAX_PUSHED_EVENTS - 1)];
	}
	return Com_GetRealEvent();
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket(netadr_t* evFrom, QMsg* buf)
{
	int t1, t2, msec;

	t1 = 0;

	if (com_speeds->integer)
	{
		t1 = Sys_Milliseconds();
	}

	SV_PacketEvent(*evFrom, buf);

	if (com_speeds->integer)
	{
		t2 = Sys_Milliseconds();
		msec = t2 - t1;
		if (com_speeds->integer == 3)
		{
			common->Printf("SV_PacketEvent time: %i\n", msec);
		}
	}
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/
int Com_EventLoop(void)
{
	sysEvent_t ev;
	netadr_t evFrom;
	byte bufData[MAX_MSGLEN_Q3];
	QMsg buf;

	buf.Init(bufData, sizeof(bufData));

	while (1)
	{
		ev = Com_GetEvent();

		// if no more events are available
		if (ev.evType == SE_NONE)
		{
			// manually send packet events for the loopback channel
			while (NET_GetLoopPacket(NS_CLIENT, &evFrom, &buf))
			{
				CL_PacketEvent(evFrom, &buf);
			}

			while (NET_GetLoopPacket(NS_SERVER, &evFrom, &buf))
			{
				// if the server just shut down, flush the events
				if (com_sv_running->integer)
				{
					Com_RunAndTimeServerPacket(&evFrom, &buf);
				}
			}

			return ev.evTime;
		}


		switch (ev.evType)
		{
		default:
			// bk001129 - was ev.evTime
			common->FatalError("Com_EventLoop: bad event type %i", ev.evType);
			break;
		case SE_NONE:
			break;
		case SE_KEY:
			CL_KeyEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CHAR:
			CL_CharEvent(ev.evValue);
			break;
		case SE_MOUSE:
			CL_MouseEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_JOYSTICK_AXIS:
			CL_JoystickEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CONSOLE:
			Cbuf_AddText((char*)ev.evPtr);
			Cbuf_AddText("\n");
			break;
		case SE_PACKET:
			// this cvar allows simulation of connections that
			// drop a lot of packets.  Note that loopback connections
			// don't go through here at all.
			if (com_dropsim->value > 0)
			{
				static int seed;

				if (Q_random(&seed) < com_dropsim->value)
				{
					break;		// drop this packet
				}
			}

			evFrom = *(netadr_t*)ev.evPtr;
			buf.cursize = ev.evPtrLength - sizeof(evFrom);

			// we must copy the contents of the message out, because
			// the event buffers are only large enough to hold the
			// exact payload, but channel messages need to be large
			// enough to hold fragment reassembly
			if ((unsigned)buf.cursize > (unsigned)buf.maxsize)
			{
				common->Printf("Com_EventLoop: oversize packet\n");
				continue;
			}
			Com_Memcpy(buf._data, (byte*)((netadr_t*)ev.evPtr + 1), buf.cursize);
			if (com_sv_running->integer)
			{
				Com_RunAndTimeServerPacket(&evFrom, &buf);
			}
			else
			{
				CL_PacketEvent(evFrom, &buf);
			}
			break;
		}

		// free any block data
		if (ev.evPtr)
		{
			Mem_Free(ev.evPtr);
		}
	}

	return 0;	// never reached
}

/*
================
Com_Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int Com_Milliseconds(void)
{
	sysEvent_t ev;

	// get events and push them until we get a null event with the current time
	do
	{

		ev = Com_GetRealEvent();
		if (ev.evType != SE_NONE)
		{
			Com_PushEvent(&ev);
		}
	}
	while (ev.evType != SE_NONE);

	return ev.evTime;
}

//============================================================================

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Com_Error_f(void)
{
	if (Cmd_Argc() > 1)
	{
		common->Error("Testing drop error");
	}
	else
	{
		common->FatalError("Testing fatal error");
	}
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f(void)
{
	float s;
	int start, now;

	if (Cmd_Argc() != 2)
	{
		common->Printf("freeze <seconds>\n");
		return;
	}
	s = String::Atof(Cmd_Argv(1));

	start = Com_Milliseconds();

	while (1)
	{
		now = Com_Milliseconds();
		if ((now - start) * 0.001 > s)
		{
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void Com_Crash_f(void)
{
	*(int*)0 = 0x12345678;
}

// TTimo: centralizing the cl_cdkey stuff after I discovered a buffer overflow problem with the dedicated server version
//   not sure it's necessary to have different defaults for regular and dedicated, but I don't want to risk it
//   https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=470
#ifndef DEDICATED
char cl_cdkey[34] = "                                ";
#else
char cl_cdkey[34] = "123456789";
#endif

/*
=================
Com_ReadCDKey
=================
*/
qboolean CL_CDKeyValidate(const char* key, const char* checksum);
void Com_ReadCDKey(const char* filename)
{
	fileHandle_t f;
	char buffer[33];
	char fbuffer[MAX_OSPATH];

	sprintf(fbuffer, "%s/q3key", filename);

	FS_SV_FOpenFileRead(fbuffer, &f);
	if (!f)
	{
		String::NCpyZ(cl_cdkey, "                ", 17);
		return;
	}

	Com_Memset(buffer, 0, sizeof(buffer));

	FS_Read(buffer, 16, f);
	FS_FCloseFile(f);

	if (CL_CDKeyValidate(buffer, NULL))
	{
		String::NCpyZ(cl_cdkey, buffer, 17);
	}
	else
	{
		String::NCpyZ(cl_cdkey, "                ", 17);
	}
}

/*
=================
Com_AppendCDKey
=================
*/
void Com_AppendCDKey(const char* filename)
{
	fileHandle_t f;
	char buffer[33];
	char fbuffer[MAX_OSPATH];

	sprintf(fbuffer, "%s/q3key", filename);

	FS_SV_FOpenFileRead(fbuffer, &f);
	if (!f)
	{
		String::NCpyZ(&cl_cdkey[16], "                ", 17);
		return;
	}

	Com_Memset(buffer, 0, sizeof(buffer));

	FS_Read(buffer, 16, f);
	FS_FCloseFile(f);

	if (CL_CDKeyValidate(buffer, NULL))
	{
		String::Cat(&cl_cdkey[16], sizeof(cl_cdkey) - 16, buffer);
	}
	else
	{
		String::NCpyZ(&cl_cdkey[16], "                ", 17);
	}
}

#ifndef DEDICATED	// bk001204
/*
=================
Com_WriteCDKey
=================
*/
static void Com_WriteCDKey(const char* filename, const char* ikey)
{
	fileHandle_t f;
	char fbuffer[MAX_OSPATH];
	char key[17];


	sprintf(fbuffer, "%s/q3key", filename);


	String::NCpyZ(key, ikey, 17);

	if (!CL_CDKeyValidate(key, NULL))
	{
		return;
	}

	f = FS_SV_FOpenFileWrite(fbuffer);
	if (!f)
	{
		common->Printf("Couldn't write %s.\n", filename);
		return;
	}

	FS_Write(key, 16, f);

	FS_Printf(f, "\n// generated by quake, do not modify\r\n");
	FS_Printf(f, "// Do not give this file to ANYONE.\r\n");
	FS_Printf(f, "// id Software and Activision will NOT ask you to send this file to them.\r\n");

	FS_FCloseFile(f);
}
#endif


/*
=================
Com_Init
=================
*/
void Com_Init(char* commandLine)
{
	try
	{
		char* s;

		common->Printf("%s %s %s\n", Q3_VERSION, CPUSTRING, __DATE__);

		if (setjmp(abortframe))
		{
			Sys_Error("Error during initialization");
		}

		GGameType = GAME_Quake3;
		Sys_SetHomePathSuffix("jlquake3");

		Com_InitByteOrder();

		// bk001129 - do this before anything else decides to push events
		Com_InitPushEvent();

		Com_InitSmallZoneMemory();
		Cvar_Init();

		// prepare enough of the subsystems to handle
		// cvar and command buffer management
		Com_ParseCommandLine(commandLine);

		Cbuf_Init();

		Com_InitZoneMemory();
		Cmd_Init();

		// override anything from the config files with command line args
		Com_StartupVariable(NULL);

		// get the developer cvar set as early as possible
		Com_StartupVariable("developer");

		// done early so bind command exists
		CL_InitKeyCommands();

		FS_InitFilesystem();

		Com_InitJournaling();

		Cbuf_AddText("exec default.cfg\n");

		// skip the q3config.cfg if "safe" is on the command line
		if (!Com_SafeMode())
		{
			Cbuf_AddText("exec q3config.cfg\n");
		}

		Cbuf_AddText("exec autoexec.cfg\n");

		Cbuf_Execute();

		// override anything from the config files with command line args
		Com_StartupVariable(NULL);

		// get dedicated here for proper hunk megs initialization
#ifdef DEDICATED
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
#else
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_LATCH2);
#endif
		// allocate the stack based hunk allocator
		Com_InitHunkMemory();

		// if any archived cvars are modified after this, we will trigger a writing
		// of the config file
		cvar_modifiedFlags &= ~CVAR_ARCHIVE;

		//
		// init commands and vars
		//
		COM_InitCommonCvars();

		com_maxfps = Cvar_Get("com_maxfps", "85", CVAR_ARCHIVE);
		com_blood = Cvar_Get("com_blood", "1", CVAR_ARCHIVE);

		com_logfile = Cvar_Get("logfile", "0", CVAR_TEMP);

		com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
		com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);
		com_dropsim = Cvar_Get("com_dropsim", "0", CVAR_CHEAT);
		com_speeds = Cvar_Get("com_speeds", "0", 0);
		com_timedemo = Cvar_Get("timedemo", "0", CVAR_CHEAT);
		com_cameraMode = Cvar_Get("com_cameraMode", "0", CVAR_CHEAT);

		cl_paused = Cvar_Get("cl_paused", "0", CVAR_ROM);
		sv_paused = Cvar_Get("sv_paused", "0", CVAR_ROM);
		com_sv_running = Cvar_Get("sv_running", "0", CVAR_ROM);
		com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);
		com_buildScript = Cvar_Get("com_buildScript", "0", 0);

		com_introPlayed = Cvar_Get("com_introplayed", "0", CVAR_ARCHIVE);

#if defined(_WIN32) && defined(_DEBUG)
		com_noErrorInterrupt = Cvar_Get("com_noErrorInterrupt", "0", 0);
#endif

		if (com_dedicated->integer)
		{
			if (!com_viewlog->integer)
			{
				Cvar_Set("viewlog", "1");
			}
		}

		if (com_developer && com_developer->integer)
		{
			Cmd_AddCommand("error", Com_Error_f);
			Cmd_AddCommand("crash", Com_Crash_f);
			Cmd_AddCommand("freeze", Com_Freeze_f);
		}
		Cmd_AddCommand("quit", Com_Quit_f);
		Cmd_AddCommand("writeconfig", Com_WriteConfig_f);

		s = va("%s %s %s", Q3_VERSION, CPUSTRING, __DATE__);
		com_version = Cvar_Get("version", s, CVAR_ROM | CVAR_SERVERINFO);

		Sys_Init();
		Netchan_Init(Com_Milliseconds() & 0xffff);	// pick a port value that should be nice and random
		VM_Init();
		SV_Init();

		com_dedicated->modified = false;
		if (!com_dedicated->integer)
		{
			CL_Init();
			Sys_ShowConsole(com_viewlog->integer, false);
		}

		// set com_frameTime so that if a map is started on the
		// command line it will still be able to count on com_frameTime
		// being random enough for a serverid
		com_frameTime = Com_Milliseconds();

		// add + commands from command line
		if (!Com_AddStartupCommands())
		{
			// if the user didn't give any commands, run default action
			if (!com_dedicated->integer)
			{
				Cbuf_AddText("cinematic idlogo.RoQ\n");
				if (!com_introPlayed->integer)
				{
					Cvar_Set(com_introPlayed->name, "1");
					Cvar_Set("nextmap", "cinematic intro.RoQ");
				}
			}
		}

		// start in full screen ui mode
		Cvar_Set("r_uiFullScreen", "1");

		CL_StartHunkUsers();

		// make sure single player is off by default
		Cvar_Set("ui_singlePlayerActive", "0");

		fs_ProtectKeyFile = true;
		com_fullyInitialized = true;
		common->Printf("--- Common Initialization Complete ---\n");
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

//==================================================================

void Com_WriteConfigToFile(const char* filename)
{
	fileHandle_t f;

	f = FS_FOpenFileWrite(filename);
	if (!f)
	{
		common->Printf("Couldn't write %s.\n", filename);
		return;
	}

	FS_Printf(f, "// generated by quake, do not modify\n");
	Key_WriteBindings(f);
	Cvar_WriteVariables(f);
	FS_FCloseFile(f);
}


/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration(void)
{
#ifndef DEDICATED	// bk001204
	Cvar* fs;
#endif
	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if (!com_fullyInitialized)
	{
		return;
	}

	if (!(cvar_modifiedFlags & CVAR_ARCHIVE))
	{
		return;
	}
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	Com_WriteConfigToFile("q3config.cfg");

	// bk001119 - tentative "not needed for dedicated"
#ifndef DEDICATED
	fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (UI_usesUniqueCDKey() && fs && fs->string[0] != 0)
	{
		Com_WriteCDKey(fs->string, &cl_cdkey[16]);
	}
	else
	{
		Com_WriteCDKey("baseq3", cl_cdkey);
	}
#endif
}


/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f(void)
{
	char filename[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: writeconfig <filename>\n");
		return;
	}

	String::NCpyZ(filename, Cmd_Argv(1), sizeof(filename));
	String::DefaultExtension(filename, sizeof(filename), ".cfg");
	common->Printf("Writing %s.\n", filename);
	Com_WriteConfigToFile(filename);
}

/*
================
Com_ModifyMsec
================
*/
int Com_ModifyMsec(int msec)
{
	int clampTime;

	//
	// modify time for debugging values
	//
	if (com_fixedtime->integer)
	{
		msec = com_fixedtime->integer;
	}
	else if (com_timescale->value)
	{
		msec *= com_timescale->value;
	}
	else if (com_cameraMode->integer)
	{
		msec *= com_timescale->value;
	}

	// don't let it scale below 1 msec
	if (msec < 1 && com_timescale->value)
	{
		msec = 1;
	}

	if (com_dedicated->integer)
	{
		// dedicated servers don't want to clamp for a much longer
		// period, because it would mess up all the client's views
		// of time.
		if (msec > 500)
		{
			common->Printf("Hitch warning: %i msec frame time\n", msec);
		}
		clampTime = 5000;
	}
	else
	if (!com_sv_running->integer)
	{
		// clients of remote servers do not want to clamp time, because
		// it would skew their view of the server's time temporarily
		clampTime = 5000;
	}
	else
	{
		// for local single player gaming
		// we may want to clamp the time to prevent players from
		// flying off edges when something hitches.
		clampTime = 200;
	}

	if (msec > clampTime)
	{
		msec = clampTime;
	}

	return msec;
}

/*
=================
Com_Frame
=================
*/
void Com_Frame(void)
{
	try
	{

		int msec, minMsec;
		static int lastTime;
		int key;

		int timeBeforeFirstEvents;
		int timeBeforeServer;
		int timeBeforeEvents;
		int timeBeforeClient;
		int timeAfter;





		if (setjmp(abortframe))
		{
			return;		// an ERR_DROP was thrown
		}

		// bk001204 - init to zero.
		//  also:  might be clobbered by `longjmp' or `vfork'
		timeBeforeFirstEvents = 0;
		timeBeforeServer = 0;
		timeBeforeEvents = 0;
		timeBeforeClient = 0;
		timeAfter = 0;


		// old net chan encryption key
		key = 0x87243987;

		// write config file if anything changed
		Com_WriteConfiguration();

		// if "viewlog" has been modified, show or hide the log console
		if (com_viewlog->modified)
		{
			if (!com_dedicated->value)
			{
				Sys_ShowConsole(com_viewlog->integer, false);
			}
			com_viewlog->modified = false;
		}

		//
		// main event loop
		//
		if (com_speeds->integer)
		{
			timeBeforeFirstEvents = Sys_Milliseconds();
		}

		// we may want to spin here if things are going too fast
		if (!com_dedicated->integer && com_maxfps->integer > 0 && !com_timedemo->integer)
		{
			minMsec = 1000 / com_maxfps->integer;
		}
		else
		{
			minMsec = 1;
		}
		do
		{
			com_frameTime = Com_EventLoop();
			if (lastTime > com_frameTime)
			{
				lastTime = com_frameTime;	// possible on first frame
			}
			msec = com_frameTime - lastTime;
		}
		while (msec < minMsec);
		Cbuf_Execute();

		lastTime = com_frameTime;

		// mess with msec if needed
		msec = Com_ModifyMsec(msec);

		//
		// server side
		//
		if (com_speeds->integer)
		{
			timeBeforeServer = Sys_Milliseconds();
		}

		SV_Frame(msec);

		// if "dedicated" has been modified, start up
		// or shut down the client system.
		// Do this after the server may have started,
		// but before the client tries to auto-connect
		if (com_dedicated->modified)
		{
			// get the latched value
			Cvar_Get("dedicated", "0", 0);
			com_dedicated->modified = false;
			if (!com_dedicated->integer)
			{
				CL_Init();
				Sys_ShowConsole(com_viewlog->integer, false);
			}
			else
			{
				CL_Shutdown();
				Sys_ShowConsole(1, true);
			}
		}

		//
		// client system
		//
		if (!com_dedicated->integer)
		{
			//
			// run event loop a second time to get server to client packets
			// without a frame of latency
			//
			if (com_speeds->integer)
			{
				timeBeforeEvents = Sys_Milliseconds();
			}
			Com_EventLoop();
			Cbuf_Execute();


			//
			// client side
			//
			if (com_speeds->integer)
			{
				timeBeforeClient = Sys_Milliseconds();
			}

			CL_Frame(msec);

			if (com_speeds->integer)
			{
				timeAfter = Sys_Milliseconds();
			}
		}

		//
		// report timing information
		//
		if (com_speeds->integer)
		{
			int all, sv, ev, cl;

			all = timeAfter - timeBeforeServer;
			sv = timeBeforeEvents - timeBeforeServer;
			ev = timeBeforeServer - timeBeforeFirstEvents + timeBeforeClient - timeBeforeEvents;
			cl = timeAfter - timeBeforeClient;
			sv -= time_game;
			cl -= time_frontend + time_backend;

			common->Printf("frame:%i all:%3i sv:%3i ev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n",
				com_frameNumber, all, sv, ev, cl, time_game, time_frontend, time_backend);
		}

		//
		// trace optimization tracking
		//
		if (com_showtrace->integer)
		{
			common->Printf("%4i traces  (%ib %ip) %4i points\n", c_traces,
				c_brush_traces, c_patch_traces, c_pointcontents);
			c_traces = 0;
			c_brush_traces = 0;
			c_patch_traces = 0;
			c_pointcontents = 0;
		}

		// old net chan encryption key
		key = lastTime * 0x87243987;

		com_frameNumber++;
	}
	catch (DropException& e)
	{
		Com_Error(ERR_DROP, "%s", e.What());
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown(void)
{
	if (logfile)
	{
		FS_FCloseFile(logfile);
		logfile = 0;
	}

	if (com_journalFile)
	{
		FS_FCloseFile(com_journalFile);
		com_journalFile = 0;
	}

}
