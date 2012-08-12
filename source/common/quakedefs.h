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

#define MAX_EDICTS_QH       768			// FIXME: ouch! ouch! ouch!

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

#define MAX_CLIENTS_QH          16
#define MAX_CLIENTS_QHW         32
#define BIGGEST_MAX_CLIENTS_QH  32	//	For common arrays

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

//
// stats are integers communicated to the client by the server
//
#define Q1STAT_HEALTH           0
#define Q1STAT_FRAGS            1
#define Q1STAT_WEAPON           2
#define Q1STAT_AMMO             3
#define Q1STAT_ARMOR            4
#define Q1STAT_WEAPONFRAME      5
#define Q1STAT_SHELLS           6
#define Q1STAT_NAILS            7
#define Q1STAT_ROCKETS          8
#define Q1STAT_CELLS            9
#define Q1STAT_ACTIVEWEAPON     10
#define Q1STAT_TOTALSECRETS     11
#define Q1STAT_TOTALMONSTERS    12
#define Q1STAT_SECRETS          13		// bumped on client side by q1svc_foundsecret
#define Q1STAT_MONSTERS         14		// bumped by q1svc_killedmonster
#define QWSTAT_ITEMS            15

// edict->flags
#define QHFL_FLY                1
#define QHFL_SWIM               2
#define QHFL_CLIENT             8
#define QHFL_MONSTER            32
#define QHFL_GODMODE            64
#define QHFL_NOTARGET           128
#define QHFL_ITEM               256
#define QHFL_ONGROUND           512
#define QHFL_PARTIALGROUND      1024	// not all corners are valid
#define QHFL_WATERJUMP          2048	// player jumping out of water
#define H2FL_ARCHIVE_OVERRIDE   1048576
#define H2FL_MOVECHAIN_ANGLE    32768	// when in a move chain, will update the angle
#define H2FL_HUNTFACE           65536	//Makes monster go for enemy view_ofs thwn moving
#define H2FL_NOZ                131072	//Monster will not automove on Z if flying or swimming
#define H2FL_SET_TRACE          262144	//Trace will always be set for this monster (pentacles)
#define H2FL_CLASS_DEPENDENT    2097152	// model will appear different to each player
#define H2FL_SPECIAL_ABILITY1   4194304	// has 1st special ability
#define H2FL_SPECIAL_ABILITY2   8388608	// has 2nd special ability

#define QHDEFAULT_SOUND_PACKET_VOLUME 255
#define QHDEFAULT_SOUND_PACKET_ATTENUATION 1.0

// a sound with no channel is a local only sound
#define QHSND_VOLUME        BIT(0)		// a byte
#define QHSND_ATTENUATION   BIT(1)		// a byte
#define H2SND_OVERFLOW      BIT(2)		// add 255 to snd num
//gonna use the rest of the bits to pack the ent+channel

// the sound field has bits 0-2: channel, 3-12: entity
#define QHWSND_VOLUME       BIT(15)		// a byte
#define QHWSND_ATTENUATION  BIT(14)		// a byte

#define Q1SU_VIEWHEIGHT     (1 << 0)
#define Q1SU_IDEALPITCH     (1 << 1)
#define Q1SU_PUNCH1         (1 << 2)
#define Q1SU_PUNCH2         (1 << 3)
#define Q1SU_PUNCH3         (1 << 4)
#define Q1SU_VELOCITY1      (1 << 5)
#define Q1SU_VELOCITY2      (1 << 6)
#define Q1SU_VELOCITY3      (1 << 7)
#define Q1SU_ITEMS          (1 << 9)
#define Q1SU_ONGROUND       (1 << 10)		// no data follows, the bit is it
#define Q1SU_INWATER        (1 << 11)		// no data follows, the bit is it
#define Q1SU_WEAPONFRAME    (1 << 12)
#define Q1SU_ARMOR          (1 << 13)
#define Q1SU_WEAPON         (1 << 14)

#define H2SU_IDEALROLL      (1 << 8)	// I'll take that available bit
#define H2SU_SC1            (1 << 9)
#define H2SU_SC2            (1 << 15)

// defaults for clientinfo messages
#define Q1DEFAULT_VIEWHEIGHT  22

#define Q1PROTOCOL_VERSION    15
#define QWPROTOCOL_VERSION    28

#define Q1IT_SHOTGUN              1
#define Q1IT_SUPER_SHOTGUN        2
#define Q1IT_NAILGUN              4
#define Q1IT_SUPER_NAILGUN        8
#define Q1IT_GRENADE_LAUNCHER     16
#define Q1IT_ROCKET_LAUNCHER      32
#define Q1IT_LIGHTNING            64
#define Q1IT_SUPER_LIGHTNING      128
#define Q1IT_SHELLS               256
#define Q1IT_NAILS                512
#define Q1IT_ROCKETS              1024
#define Q1IT_CELLS                2048
#define Q1IT_AXE                  4096
#define Q1IT_ARMOR1               8192
#define Q1IT_ARMOR2               16384
#define Q1IT_ARMOR3               32768
#define Q1IT_SUPERHEALTH          65536
#define Q1IT_KEY1                 131072
#define Q1IT_KEY2                 262144
#define Q1IT_INVISIBILITY         524288
#define Q1IT_INVULNERABILITY      1048576
#define Q1IT_SUIT                 2097152
#define Q1IT_QUAD                 4194304
#define Q1IT_SIGIL1               (1 << 28)
#define Q1IT_SIGIL2               (1 << 29)
#define Q1IT_SIGIL3               (1 << 30)
#define Q1IT_SIGIL4               (1 << 31)

//rogue changed and added defines
#define Q1RIT_SHELLS              128
#define Q1RIT_NAILS               256
#define Q1RIT_ROCKETS             512
#define Q1RIT_CELLS               1024
#define Q1RIT_LAVA_NAILGUN        4096
#define Q1RIT_ARMOR1              8388608
#define Q1RIT_ARMOR2              16777216
#define Q1RIT_ARMOR3              33554432
#define Q1RIT_LAVA_NAILS          67108864
#define Q1RIT_PLASMA_AMMO         134217728
#define Q1RIT_MULTI_ROCKETS       268435456

//hipnotic added defines
#define Q1HIT_PROXIMITY_GUN_BIT 16
#define Q1HIT_MJOLNIR_BIT       7
#define Q1HIT_LASER_CANNON_BIT  23
#define Q1HIT_PROXIMITY_GUN     (1 << Q1HIT_PROXIMITY_GUN_BIT)
#define Q1HIT_MJOLNIR           (1 << Q1HIT_MJOLNIR_BIT)
#define Q1HIT_LASER_CANNON      (1 << Q1HIT_LASER_CANNON_BIT)

// out of band message id bytes

// M = master, S = server, C = client, A = any
// the second character will allways be \n if the message isn't a single
// byte long (?? not true anymore?)

#define S2C_CHALLENGE       'c'
#define S2C_CONNECTION      'j'
#define A2A_PING            'k'	// respond with an A2A_ACK
#define A2A_ACK             'l'	// general acknowledgement without info
#define A2A_NACK            'm'	// [+ comment] general failure
#define A2C_PRINT           'n'	// print a message on client
#define A2S_ECHO            'g'	// echo back a message

#define S2M_HEARTBEAT       'a'	// + serverinfo + userlist + fraglist
#define A2C_CLIENT_COMMAND  'B'	// + command line
#define S2M_SHUTDOWN        'C'

// game types sent by serverinfo
// these determine which intermission screen plays
#define QHGAME_COOP           0
#define QHGAME_DEATHMATCH     1

#define QWPORT_CLIENT 27001
#define QWPORT_MASTER 27000
#define QWPORT_SERVER 27500
