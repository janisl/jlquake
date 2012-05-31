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

// sv_bot.c

#include "server.h"
#include "../game/botlib.h"
#include "../../server/botlib/local.h"

extern botlib_export_t* botlib_export;
int bot_enable;

/*
==================
SV_BotAllocateClient
==================
*/
int SV_BotAllocateClient(void)
{
	int i;
	client_t* cl;

	// find a client slot
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
	{
		// Wolfenstein, never use the first slot, otherwise if a bot connects before the first client on a listen server, game won't start
		if (i < 1)
		{
			continue;
		}
		// done.
		if (cl->state == CS_FREE)
		{
			break;
		}
	}

	if (i == sv_maxclients->integer)
	{
		return -1;
	}

	cl->gentity = SV_GentityNum(i);
	cl->gentity->s.number = i;
	cl->state = CS_ACTIVE;
	cl->lastPacketTime = svs.time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate = 16384;

	return i;
}

/*
==================
SV_BotFreeClient
==================
*/
void SV_BotFreeClient(int clientNum)
{
	client_t* cl;

	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_BotFreeClient: bad clientNum: %i", clientNum);
	}
	cl = &svs.clients[clientNum];
	cl->state = CS_FREE;
	cl->name[0] = 0;
	if (cl->gentity)
	{
		cl->gentity->r.svFlags &= ~SVF_BOT;
	}
}

/*
==================
BotDrawDebugPolygons
==================
*/
void BotDrawDebugPolygons(void (* drawPoly)(int color, int numPoints, float* points), int value)
{
	static Cvar* bot_debug, * bot_groundonly, * bot_reachability, * bot_highlightarea;
	static Cvar* bot_testhidepos, * bot_testroutevispos;
	bot_debugpoly_t* poly;
	int i;

	if (!bot_enable)
	{
		return;
	}
	//bot debugging
	if (!bot_debug)
	{
		bot_debug = Cvar_Get("bot_debug", "0", 0);
	}
	//show reachabilities
	if (!bot_reachability)
	{
		bot_reachability = Cvar_Get("bot_reachability", "0", 0);
	}
	//show ground faces only
	if (!bot_groundonly)
	{
		bot_groundonly = Cvar_Get("bot_groundonly", "1", 0);
	}
	//get the hightlight area
	if (!bot_highlightarea)
	{
		bot_highlightarea = Cvar_Get("bot_highlightarea", "0", 0);
	}
	//
	if (!bot_testhidepos)
	{
		bot_testhidepos = Cvar_Get("bot_testhidepos", "0", 0);
	}
	//
	if (!bot_testroutevispos)
	{
		bot_testroutevispos = Cvar_Get("bot_testroutevispos", "0", 0);
	}
	//
	if (bot_debug->integer)
	{
		BotLibVarSet("bot_highlightarea", bot_highlightarea->string);
		BotLibVarSet("bot_testhidepos", bot_testhidepos->string);
		BotLibVarSet("bot_testroutevispos", bot_testroutevispos->string);
	}	//end if
	for (i = 0; i < bot_maxdebugpolys; i++)
	{
		poly = &debugpolygons[i];
		if (!poly->inuse)
		{
			continue;
		}
		drawPoly(poly->color, poly->numPoints, (float*)poly->points);
		//Com_Printf("poly %i, numpoints = %d\n", i, poly->numPoints);
	}
}

/*
==================
BotImport_Trace
==================
*/
void BotImport_Trace(bsp_trace_t* bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask)
{
	q3trace_t trace;

	// always use bounding box for bot stuff ?
	SV_Trace(&trace, start, mins, maxs, end, passent, contentmask, qfalse);
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

/*
==================
BotImport_EntityTrace
==================
*/
void BotImport_EntityTrace(bsp_trace_t* bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask)
{
	q3trace_t trace;

	// always use bounding box for bot stuff ?
	SV_ClipToEntity(&trace, start, mins, maxs, end, entnum, contentmask, qfalse);
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}


/*
==================
BotImport_PointContents
==================
*/
int BotImport_PointContents(vec3_t point)
{
	return SV_PointContents(point, -1);
}

/*
==================
BotImport_inPVS
==================
*/
int BotImport_inPVS(vec3_t p1, vec3_t p2)
{
	return SV_inPVS(p1, p2);
}

/*
==================
SV_BotClientCommand
==================
*/
void BotClientCommand(int client, const char* command)
{
	SV_ExecuteClientCommand(&svs.clients[client], command, qtrue);
}

/*
==================
SV_BotFrame
==================
*/
void SV_BotFrame(int time)
{
	if (!bot_enable)
	{
		return;
	}
	//NOTE: maybe the game is already shutdown
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, BOTAI_START_FRAME, time);
}

/*
===============
SV_BotLibSetup
===============
*/
int SV_BotLibSetup(void)
{
	if (!bot_enable)
	{
		return 0;
	}

	if (!botlib_export)
	{
		Com_Printf(S_COLOR_RED "Error: SV_BotLibSetup without SV_BotInitBotLib\n");
		return -1;
	}

	return botlib_export->BotLibSetup();
}

/*
===============
SV_ShutdownBotLib

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
int SV_BotLibShutdown(void)
{

	if (!botlib_export)
	{
		return -1;
	}

	return botlib_export->BotLibShutdown();
}

/*
==================
SV_BotInitCvars
==================
*/
void SV_BotInitCvars(void)
{

	Cvar_Get("bot_enable", "1", 0);					//enable the bot
	Cvar_Get("bot_developer", "0", 0);				//bot developer mode
	Cvar_Get("bot_debug", "0", 0);					//enable bot debugging
	Cvar_Get("bot_groundonly", "1", 0);				//only show ground faces of areas
	Cvar_Get("bot_reachability", "0", 0);		//show all reachabilities to other areas
	Cvar_Get("bot_thinktime", "100", 0);		//msec the bots thinks
	Cvar_Get("bot_reloadcharacters", "0", 0);	//reload the bot characters each time
	Cvar_Get("bot_testichat", "0", 0);				//test ichats
	Cvar_Get("bot_testrchat", "0", 0);				//test rchats
	Cvar_Get("bot_fastchat", "0", 0);			//fast chatting bots
	Cvar_Get("bot_nochat", "0", 0);					//disable chats
	Cvar_Get("bot_grapple", "0", 0);			//enable grapple
	Cvar_Get("bot_rocketjump", "1", 0);				//enable rocket jumping
	Cvar_Get("bot_miniplayers", "0", 0);		//minimum players in a team or the game
}

// Ridah, Cast AI
/*
===============
BotImport_AICast_VisibleFromPos
===============
*/
qboolean BotImport_AICast_VisibleFromPos(vec3_t srcpos, int srcnum,
	vec3_t destpos, int destnum, qboolean updateVisPos)
{
	return VM_Call(gvm, AICAST_VISIBLEFROMPOS, (qintptr)srcpos, srcnum, (qintptr)destpos, destnum, updateVisPos);
}

/*
===============
BotImport_AICast_CheckAttackAtPos
===============
*/
qboolean BotImport_AICast_CheckAttackAtPos(int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld)
{
	return VM_Call(gvm, AICAST_CHECKATTACKATPOS, entnum, enemy, (qintptr)pos, ducking, allowHitWorld);
}
// done.

/*
==================
SV_BotInitBotLib
==================
*/
void SV_BotInitBotLib(void)
{
	botlib_import_t botlib_import;

	/*
	if ( botlib_export ) {
	    SV_BotLibShutdown();
	}*/

	if (debugpolygons)
	{
		Z_Free(debugpolygons);
	}
	bot_maxdebugpolys = 128;
	debugpolygons = (bot_debugpoly_t*)Z_Malloc(sizeof(bot_debugpoly_t) * bot_maxdebugpolys);

	botlib_import.Trace = BotImport_Trace;
	botlib_import.EntityTrace = BotImport_EntityTrace;
	botlib_import.PointContents = BotImport_PointContents;
	botlib_import.inPVS = BotImport_inPVS;
	botlib_import.BotClientCommand = BotClientCommand;

	// Ridah, Cast AI
	botlib_import.AICast_VisibleFromPos = BotImport_AICast_VisibleFromPos;
	botlib_import.AICast_CheckAttackAtPos = BotImport_AICast_CheckAttackAtPos;
	// done.

	botlib_export = (botlib_export_t*)GetBotLibAPI(BOTLIB_API_VERSION, &botlib_import);
}


//
//  * * * BOT AI CODE IS BELOW THIS POINT * * *
//

/*
==================
SV_BotGetConsoleMessage
==================
*/
int SV_BotGetConsoleMessage(int client, char* buf, int size)
{
	client_t* cl;
	int index;
	const char* msg;

	cl = &svs.clients[client];
	cl->lastPacketTime = svs.time;

	if (cl->reliableAcknowledge == cl->reliableSequence)
	{
		return qfalse;
	}

	cl->reliableAcknowledge++;
	index = cl->reliableAcknowledge & (MAX_RELIABLE_COMMANDS_WS - 1);

	//if ( !cl->reliableCommands[index][0] ) {
	if (!(msg = SV_GetReliableCommand(cl, index)) || !msg[0])
	{
		return qfalse;
	}

	//String::NCpyZ( buf, cl->reliableCommands[index], size );
	String::NCpyZ(buf, msg, size);
	return qtrue;
}

#if 0
/*
==================
EntityInPVS
==================
*/
int EntityInPVS(int client, int entityNum)
{
	client_t* cl;
	clientSnapshot_t* frame;
	int i;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK_Q3];
	for (i = 0; i < frame->num_entities; i++)
	{
		if (svs.snapshotEntities[(frame->first_entity + i) % svs.numSnapshotEntities].number == entityNum)
		{
			return qtrue;
		}
	}
	return qfalse;
}
#endif

/*
==================
SV_BotGetSnapshotEntity
==================
*/
int SV_BotGetSnapshotEntity(int client, int sequence)
{
	client_t* cl;
	clientSnapshot_t* frame;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK_Q3];
	if (sequence < 0 || sequence >= frame->num_entities)
	{
		return -1;
	}
	return svs.snapshotEntities[(frame->first_entity + sequence) % svs.numSnapshotEntities].number;
}
