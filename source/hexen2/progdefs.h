
/* generated by hcc, do not modify */

typedef struct
{	int	pad[28];
	int	self;
	int	other;
	int	world;
	float	time;
	float	frametime;
	float	force_retouch;
	string_t	mapname;
	string_t	startspot;
	float	deathmatch;
	float	randomclass;
	float	coop;
	float	teamplay;
#ifdef MISSIONPACK
	float	cl_playerclass;
#endif
	float	serverflags;
	float	total_secrets;
	float	total_monsters;
	float	found_secrets;
	float	killed_monsters;
	float	chunk_cnt;
	float	done_precache;
	float	parm1;
	float	parm2;
	float	parm4;
	float	parm5;
	float	parm6;
	float	parm7;
	float	parm8;
	float	parm9;
	float	parm10;
	float	parm11;
	float	parm12;
	float	parm13;
	float	parm14;
	float	parm15;
	float	parm16;
	string_t	parm3;
	vec3_t	v_forward;
	vec3_t	v_up;
	vec3_t	v_right;
	float	trace_allsolid;
	float	trace_startsolid;
	float	trace_fraction;
	vec3_t	trace_endpos;
	vec3_t	trace_plane_normal;
	float	trace_plane_dist;
	int	trace_ent;
	float	trace_inopen;
	float	trace_inwater;
	int	msg_entity;
	float	cycle_wrapped;
	float	crouch_cnt;
#ifndef MISSIONPACK
	float	modelindex_assassin;
	float	modelindex_crusader;
	float	modelindex_paladin;
	float	modelindex_necromancer;
#endif
	float	modelindex_sheep;
	float	num_players;
	float	exp_mult;
	func_t	main;
	func_t	StartFrame;
	func_t	PlayerPreThink;
	func_t	PlayerPostThink;
	func_t	ClientKill;
	func_t	ClientConnect;
	func_t	PutClientInServer;
	func_t	ClientReEnter;
	func_t	ClientDisconnect;
	func_t	ClassChangeWeapon;
} globalvars_t;

#ifdef MISSIONPACK
#define PROGHEADER_CRC 26905
#else
#define PROGHEADER_CRC 38488
#endif
