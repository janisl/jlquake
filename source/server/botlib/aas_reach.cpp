//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../server.h"
#include "local.h"

//NOTE: all travel times are in hundreth of a second
//number of reachabilities of each type
int reach_swim;			//swim
int reach_equalfloor;	//walk on floors with equal height
int reach_step;			//step up
int reach_walk;			//walk of step
int reach_barrier;		//jump up to a barrier
int reach_waterjump;	//jump out of water
int reach_walkoffledge;	//walk of a ledge
int reach_jump;			//jump
int reach_ladder;		//climb or descent a ladder
int reach_teleport;		//teleport
int reach_elevator;		//use an elevator
int reach_funcbob;		//use a func bob
int reach_grapple;		//grapple hook
int reach_doublejump;	//double jump
int reach_rampjump;		//ramp jump
int reach_strafejump;	//strafe jump (just normal jump but further)
int reach_rocketjump;	//rocket jump
int reach_bfgjump;		//bfg jump
int reach_jumppad;		//jump pads
//if true grapple reachabilities are skipped
int calcgrapplereach;
//temporary reachabilities
aas_lreachability_t* reachabilityheap;	//heap with reachabilities
aas_lreachability_t* nextreachability;	//next free reachability from the heap
aas_lreachability_t** areareachability;	//reachability links for every area
int numlreachabilities;

aas_jumplink_t* jumplinks;
