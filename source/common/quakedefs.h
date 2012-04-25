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

#define MAX_EDICTS_Q1       768			// FIXME: ouch! ouch! ouch!

// entity effects
enum
{
	Q1EF_BRIGHTFIELD    = 1,
	Q1EF_MUZZLEFLASH    = 2,
	Q1EF_BRIGHTLIGHT    = 4,
	Q1EF_DIMLIGHT       = 8,
	QWEF_FLAG1          = 16,
	QWEF_FLAG2          = 32,
	QWEF_BLUE           = 64,
	QWEF_RED            = 128,
};

//
// temp entity events
//
#define Q1TE_SPIKE          0
#define Q1TE_SUPERSPIKE     1
#define Q1TE_GUNSHOT        2
#define Q1TE_EXPLOSION      3
#define Q1TE_TAREXPLOSION   4
#define Q1TE_LIGHTNING1     5
#define Q1TE_LIGHTNING2     6
#define Q1TE_WIZSPIKE       7
#define Q1TE_KNIGHTSPIKE    8
#define Q1TE_LIGHTNING3     9
#define Q1TE_LAVASPLASH     10
#define Q1TE_TELEPORT       11
//	Quake only
#define Q1TE_EXPLOSION2     12
#define Q1TE_BEAM           13
//	QuakeWorld only
#define QWTE_BLOOD          12
#define QWTE_LIGHTNINGBLOOD 13

// q1entity_state_t is the information conveyed from the server
// in an update message
struct q1entity_state_t
{
	int number;			// edict index

	vec3_t origin;
	vec3_t angles;
	int modelindex;
	int frame;
	int colormap;
	int skinnum;
	int effects;
	int flags;			// nolerp, etc
};

struct q1usercmd_t
{
	vec3_t viewangles;

	// intended velocities
	float forwardmove;
	float sidemove;
	float upmove;
};

struct qwusercmd_t
{
	byte msec;
	vec3_t angles;
	short forwardmove, sidemove, upmove;
	byte buttons;
	byte impulse;
};

#define QWMAX_PACKET_ENTITIES   64	// doesn't count nails

struct qwpacket_entities_t
{
	int num_entities;
	q1entity_state_t entities[QWMAX_PACKET_ENTITIES];
};

#define MAX_CLIENTS_Q1          16
#define MAX_CLIENTS_QW          32
#define BIGGEST_MAX_CLIENTS_Q1  32	//	For common arrays

#define UPDATE_BACKUP_QW    64	// copies of entity_state_t to keep buffered
// must be power of two
#define UPDATE_MASK_QW      (UPDATE_BACKUP_QW - 1)

// if the high bit of the servercmd is set, the low bits are fast update flags:
#define Q1U_MOREBITS    (1 << 0)
#define Q1U_ORIGIN1     (1 << 1)
#define Q1U_ORIGIN2     (1 << 2)
#define Q1U_ORIGIN3     (1 << 3)
#define Q1U_ANGLE2      (1 << 4)
#define Q1U_NOLERP      (1 << 5)		// don't interpolate movement
#define Q1U_FRAME       (1 << 6)
#define Q1U_SIGNAL      (1 << 7)		// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define Q1U_ANGLE1      (1 << 8)
#define Q1U_ANGLE3      (1 << 9)
#define Q1U_MODEL       (1 << 10)
#define Q1U_COLORMAP    (1 << 11)
#define Q1U_SKIN        (1 << 12)
#define Q1U_EFFECTS     (1 << 13)
#define Q1U_LONGENTITY  (1 << 14)

// the first 16 bits of a packetentities update holds 9 bits
// of entity number and 7 bits of flags
#define QWU_ORIGIN1     (1 << 9)
#define QWU_ORIGIN2     (1 << 10)
#define QWU_ORIGIN3     (1 << 11)
#define QWU_ANGLE2      (1 << 12)
#define QWU_FRAME       (1 << 13)
#define QWU_REMOVE      (1 << 14)		// REMOVE this entity, don't add it
#define QWU_MOREBITS    (1 << 15)

// if MOREBITS is set, these additional flags are read in next
#define QWU_ANGLE1      (1 << 0)
#define QWU_ANGLE3      (1 << 1)
#define QWU_MODEL       (1 << 2)
#define QWU_COLORMAP    (1 << 3)
#define QWU_SKIN        (1 << 4)
#define QWU_EFFECTS     (1 << 5)
#define QWU_SOLID       (1 << 6)		// the entity should be solid for prediction

// playerinfo flags from server
// playerinfo allways sends: playernum, flags, origin[] and framenumber

#define QWPF_MSEC           (1 << 0)
#define QWPF_COMMAND        (1 << 1)
#define QWPF_VELOCITY1ND    (1 << 2)
#define QWPF_VELOCITY2      (1 << 3)
#define QWPF_VELOCITY3      (1 << 4)
#define QWPF_MODEL          (1 << 5)
#define QWPF_SKINNUM        (1 << 6)
#define QWPF_EFFECTS        (1 << 7)
#define QWPF_WEAPONFRAME    (1 << 8)		// only sent for view player
#define QWPF_DEAD           (1 << 9)		// don't block movement any more
#define QWPF_GIB            (1 << 10)		// offset the view height differently

// if the high bit of the client to server byte is set, the low bits are
// client move cmd bits
// ms and angle2 are allways sent, the others are optional
#define QWCM_ANGLE1     (1 << 0)
#define QWCM_ANGLE3     (1 << 1)
#define QWCM_FORWARD    (1 << 2)
#define QWCM_SIDE       (1 << 3)
#define QWCM_UP         (1 << 4)
#define QWCM_BUTTONS    (1 << 5)
#define QWCM_IMPULSE    (1 << 6)
#define QWCM_ANGLE2     (1 << 7)

//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
#define q1svc_bad                   0
#define q1svc_nop                   1
#define q1svc_disconnect            2
#define q1svc_updatestat            3	// [byte] Q1:[long] QW:[byte]
#define q1svc_version               4	// [long] server version, not in QW
#define q1svc_setview               5	// [short] entity number
#define q1svc_sound                 6	// <see code>
#define q1svc_time                  7	// [float] server time, not in QW
#define q1svc_print                 8	// QW:[byte] id [string] null terminated string
#define q1svc_stufftext             9	// [string] stuffed into client's console buffer
										// the string should be \n terminated
#define q1svc_setangle              10	// [angle3] set the view angle to this absolute value

#define q1svc_serverinfo            11	// [long] version
										// [string] signon string
										// [string]..[0]model cache
										// [string]...[0]sounds cache
#define qwsvc_serverdata            11	// [long] protocol ...
#define q1svc_lightstyle            12	// [byte] [string]
#define q1svc_updatename            13	// [byte] [string], not in QW
#define q1svc_updatefrags           14	// [byte] [short]
#define q1svc_clientdata            15	// <shortbits + data>, not in QW
#define q1svc_stopsound             16	// <see code>
#define q1svc_updatecolors          17	// [byte] [byte], not in QW
#define q1svc_particle              18	// [vec3] <variable>, not in QW
#define q1svc_damage                19

#define q1svc_spawnstatic           20
//	svc_spawnbinary		21
#define q1svc_spawnbaseline         22

#define q1svc_temp_entity           23	// variable

#define q1svc_setpause              24	// [byte] on / off
#define q1svc_signonnum             25	// [byte]  used for the signon sequence, not in QW

#define q1svc_centerprint           26	// [string] to put in center of the screen

#define q1svc_killedmonster         27
#define q1svc_foundsecret           28

#define q1svc_spawnstaticsound      29	// [coord3] [byte] samp [byte] vol [byte] aten

#define q1svc_intermission          30	// Q1:[string] music QW:[vec3_t] origin [vec3_t] angle
#define q1svc_finale                31	// Q1:[string] music [string] text

#define q1svc_cdtrack               32	// [byte] track Q1:[byte] looptrack
#define q1svc_sellscreen            33

#define q1svc_cutscene              34

#define qwsvc_smallkick             34	// set client punchangle to 2
#define qwsvc_bigkick               35	// set client punchangle to 4

#define qwsvc_updateping            36	// [byte] [short]
#define qwsvc_updateentertime       37	// [byte] [float]

#define qwsvc_updatestatlong        38	// [byte] [long]

#define qwsvc_muzzleflash           39	// [short] entity

#define qwsvc_updateuserinfo        40	// [byte] slot [long] uid
										// [string] userinfo

#define qwsvc_download              41	// [short] size [size bytes]
#define qwsvc_playerinfo            42	// variable
#define qwsvc_nails                 43	// [byte] num [48 bits] xyzpy 12 12 12 4 8
#define qwsvc_chokecount            44	// [byte] packets choked
#define qwsvc_modellist             45	// [strings]
#define qwsvc_soundlist             46	// [strings]
#define qwsvc_packetentities        47	// [...]
#define qwsvc_deltapacketentities   48	// [...]
#define qwsvc_maxspeed              49	// maxspeed change, for prediction
#define qwsvc_entgravity            50	// gravity change, for prediction
#define qwsvc_setinfo               51	// setinfo on a client
#define qwsvc_serverinfo            52	// serverinfo
#define qwsvc_updatepl              53	// [byte] [byte]

//
// client to server
//
#define q1clc_bad           0
#define q1clc_nop           1
#define q1clc_disconnect    2	// Not in QW
#define q1clc_move          3	// [usercmd_t]
#define q1clc_stringcmd     4	// [string] message
#define qwclc_delta         5	// [byte] sequence number, requests delta compression of message
#define qwclc_tmove         6	// teleport request, spectator only
#define qwclc_upload        7	// teleport request, spectator only

#define MAX_INFO_STRING_QW  196

#define MAX_LIGHTSTYLES_Q1  64
#define MAX_MODELS_Q1       256			// these are sent over the net as bytes
#define MAX_SOUNDS_Q1       256			// so they cannot be blindly increased
