/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/* file generated by qcc, do not modify */

typedef struct
{	int	pad[28];
	int	self;
	int	other;
	int	world;
	float	time;
	float	frametime;
	int	newmis;
	float	force_retouch;
	string_t	mapname;
	float	serverflags;
	float	total_secrets;
	float	total_monsters;
	float	found_secrets;
	float	killed_monsters;
	float	parm1;
	float	parm2;
	float	parm3;
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
	func_t	main;
	func_t	StartFrame;
	func_t	PlayerPreThink;
	func_t	PlayerPostThink;
	func_t	ClientKill;
	func_t	ClientConnect;
	func_t	PutClientInServer;
	func_t	ClientDisconnect;
	func_t	SetNewParms;
	func_t	SetChangeParms;
} globalvars_t;

typedef struct
{
	float	modelindex;
	vec3_t	absmin;
	vec3_t	absmax;
	float	ltime;
	float	lastruntime;
	float	movetype;
	float	solid;
	vec3_t	origin;
	vec3_t	oldorigin;
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t	avelocity;
	string_t	classname;
	string_t	model;
	float	frame;
	float	skin;
	float	effects;
	vec3_t	mins;
	vec3_t	maxs;
	vec3_t	size;
	func_t	touch;
	func_t	use;
	func_t	think;
	func_t	blocked;
	float	nextthink;
	int	groundentity;
	float	health;
	float	frags;
	float	weapon;
	string_t	weaponmodel;
	float	weaponframe;
	float	currentammo;
	float	ammo_shells;
	float	ammo_nails;
	float	ammo_rockets;
	float	ammo_cells;
	float	_items;
	float	_takedamage;
	int	_chain;
	float	_deadflag;
	vec3_t	_view_ofs;
	float	_button0;
	float	_button1;
	float	_button2;
	float	_impulse;
	float	_fixangle;
	vec3_t	_v_angle;
	string_t	_netname;
	int	_enemy;
	float	_flags;
	float	_colormap;
	float	_team;
	float	_max_health;
	float	_teleport_time;
	float	_armortype;
	float	_armorvalue;
	float	_waterlevel;
	float	_watertype;
	float	_ideal_yaw;
	float	_yaw_speed;
	int	_aiment;
	int	_goalentity;
	float	_spawnflags;
	string_t	_target;
	string_t	_targetname;
	float	_dmg_take;
	float	_dmg_save;
	int	_dmg_inflictor;
	int	_owner;
	vec3_t	_movedir;
	string_t	_message;
	float	_sounds;
	string_t	_noise;
	string_t	_noise1;
	string_t	_noise2;
	string_t	_noise3;
} entvars_t;

#define PROGHEADER_CRC 54730
