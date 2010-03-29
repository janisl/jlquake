/*
 * $Header: /H2 Mission Pack/HCode/Client.hc 72    3/23/98 1:08p Mgummelt $
 */

// prototypes
void () W_WeaponFrame;
void() W_SetCurrentAmmo;
void(entity attacker,float total_damage) player_pain;
void (vector org, entity death_owner) spawn_tdeath;
void() DecrementSuperHealth;
void CheckRings (void);


void FreezeAllEntities(void)
{
	entity search;

	search = nextent(world);
	while(search != world)
	{
		if (search.classname != "player")
		{
			thinktime search : 99999;
		}
		search = nextent(search);
	}
}

/*
=============================================================================

				LEVEL CHANGING / INTERMISSION

=============================================================================
*/

float	intermission_running;
float	intermission_exittime;

/*QUAKED info_intermission (1 0.5 0.5) (-16 -16 -16) (16 16 16)
This is the camera point for the intermission. Use mangle instead of angle, so you can set pitch or roll as well as yaw.  'pitch roll yaw'
-----------------------FIELDS-------------------------

--------------------------------------------------------
*/
void info_intermission(void)
{
}

/*
============
FindIntermission

Returns the entity to view from
============
*/
/*
entity FindIntermission(void)
{
entity spot;
float cyc;

// look for info_intermission first
	spot = find (world, classname, "info_intermission");
	if (spot)
	{	// pick a random one
		cyc = random(4);
		while (cyc > 1)
		{
			spot = find (spot, classname, "info_intermission");
			if (!spot)
				spot = find (spot, classname, "info_intermission");
			cyc = cyc - 1;
		}
		return spot;
	}

// then look for the start position
	spot = find (world, classname, "info_player_start");
	if (spot)
		return spot;
}
*/

string nextmap;
string nextstartspot;

void GotoNextMap(void)
{
	if (cvar("samelevel"))	// if samelevel is set, stay on same level
	{
		changelevel (mapname, startspot);
	}
	else
	{
		changelevel (nextmap, nextstartspot);
	}
}


void ExitIntermission(void)
{
	// skip any text in deathmatch
	if (deathmatch)
	{
		intermission_exittime = 
		intermission_running = 0;
	}

	other = find (world, classname, "player");
	while (other != world)
	{
		stuffcmd(other, "-showdm\n");
		other.frags=0;//reset frags
		other.takedamage = DAMAGE_YES;
		other.solid = SOLID_BBOX;
		other.movetype = MOVETYPE_WALK;
		other.flags(-)FL_NOTARGET;
		other.effects=FALSE;
		other.weaponmodel=other.lastweapon;
		other = find (other, classname, "player");
	}	

	if (deathmatch)
	{
		gameover = FALSE;
		GotoNextMap ();
		return;
	}
	
	intermission_exittime = time + 1;
	intermission_running = intermission_running + 1;

	//
	// run some text if at the end of an episode
	//

	if (intermission_running == 2)
		GotoNextMap();
}

/*
============
IntermissionThink

When the player presses attack or jump, change to the next level
============
*/
void IntermissionThink(void)
{
	if (time < intermission_exittime)
		return;

	if (!self.button0 && !self.button1 && !self.button2)
		return;
	
	ExitIntermission ();
}

void() execute_changelevel =
{
	intermission_running = 1;
	
// enforce a wait time before allowing changelevel

	if (deathmatch)
		intermission_exittime = time + 5;
	else
		intermission_exittime = time + 2;

	other = find (world, classname, "player");
	while (other != world)
	{
//		other.sv_flags=serverflags;
		
		dprint("Other model = ");
		dprint(other.model);
		dprint("\n");

		if(other.model=="models/sheep.mdl")
			unsheep(other);
		
		thinktime other : 0.5;
		other.takedamage = DAMAGE_NO;
		other.solid = SOLID_NOT;
		other.movetype = MOVETYPE_NONE;
		other.flags(+)FL_NOTARGET;
		other.effects=EF_NODRAW|EF_LIGHT;
		other.lastweapon=other.weaponmodel;
		stuffcmd(other,"+showdm\n");
		other = find (other, classname, "player");
	}	
};

void FindDMLevel(void)
{
	serverflags (+) SFL_NEW_UNIT;

	nextmap = string_null;

	if (cvar("registered") != 0 || cvar("oem") != 0)
	{
		if (mapname == "demo1")
			nextmap = "demo2";
		else if (mapname == "demo2")
			nextmap = "demo3";
		else if (mapname == "demo3")
			nextmap = "village1";
		else if (mapname == "village1")
			nextmap = "village2";
		else if (mapname == "village2")
			nextmap = "village3";
		else if (mapname == "village3")
			nextmap = "village4";
		else if (mapname == "village4")
			nextmap = "village5";
		else if (mapname == "village5")
			nextmap = "rider1a";
		else if (mapname == "rider1a")
			nextmap = "demo1";

		else if (mapname == "meso1")
			nextmap = "meso2";
		else if (mapname == "meso2")
			nextmap = "meso3";
		else if (mapname == "meso3")
			nextmap = "meso4";
		else if (mapname == "meso4")
			nextmap = "meso5";
		else if (mapname == "meso5")
			nextmap = "meso6";
		else if (mapname == "meso6")
			nextmap = "meso8";
		else if (mapname == "meso8")
			nextmap = "meso9";
		else if (mapname == "meso9")
			nextmap = "meso1";

		else if (mapname == "egypt1")
			nextmap = "egypt2";
		else if (mapname == "egypt2")
			nextmap = "egypt3";
		else if (mapname == "egypt3")
			nextmap = "egypt4";
		else if (mapname == "egypt4")
			nextmap = "egypt5";
		else if (mapname == "egypt5")
			nextmap = "egypt6";
		else if (mapname == "egypt6")
			nextmap = "egypt7";
		else if (mapname == "egypt7")
			nextmap = "rider2c";
		else if (mapname == "rider2c")
			nextmap = "egypt1";

		else if (mapname == "romeric1")
			nextmap = "romeric2";
		else if (mapname == "romeric2")
			nextmap = "romeric3";
		else if (mapname == "romeric3")
			nextmap = "romeric4";
		else if (mapname == "romeric4")
			nextmap = "romeric5";
		else if (mapname == "romeric5")
			nextmap = "romeric6";
		else if (mapname == "romeric6")
			nextmap = "romeric7";
		else if (mapname == "romeric7")
			nextmap = "romeric1";

		else if (mapname == "cath")
			nextmap = "tower";
		else if (mapname == "tower")
			nextmap = "castle4";
		else if (mapname == "castle4")
			nextmap = "castle5";
		else if (mapname == "castle5")
			nextmap = "eidolon";
		else if (mapname == "eidolon")
			nextmap = "cath";

		else if (mapname == "ravdm1")
			nextmap = "ravdm2";
		else if (mapname == "ravdm2")
			nextmap = "ravdm3";
		else if (mapname == "ravdm3")
			nextmap = "ravdm4";
		else if (mapname == "ravdm4")
			nextmap = "ravdm5";
		else if (mapname == "ravdm5")
			nextmap = "ravdm1";

		else if (mapname == "keep1")
			nextmap = "keep2";
		else if (mapname == "keep2")
			nextmap = "keep3";
		else if (mapname == "keep3")
			nextmap = "keep4";
		else if (mapname == "keep4")
			nextmap = "keep5";
		else if (mapname == "keep5")
			nextmap = "keep1";

		else if (mapname == "tibet1")
			nextmap = "tibet2";
		else if (mapname == "tibet2")
			nextmap = "tibet3";
		else if (mapname == "tibet3")
			nextmap = "tibet4";
		else if (mapname == "tibet4")
			nextmap = "tibet5";
		else if (mapname == "tibet5")
			nextmap = "tibet6";
		else if (mapname == "tibet6")
			nextmap = "tibet7";
		else if (mapname == "tibet7")
			nextmap = "tibet8";
		else if (mapname == "tibet8")
			nextmap = "tibet9";
		else if (mapname == "tibet9")
			nextmap = "tibet10";
		else if (mapname == "tibet10")
			nextmap = "tibet1";
	}
	else
	{
		if (mapname == "demo1")
			nextmap = "demo2";
		else if (mapname == "demo2")
			nextmap = "ravdm1";
		else if (mapname == "ravdm1")
			nextmap = "demo1";
	}
}

void() changelevel_touch =
{
entity found;
	if (other.classname != "player")//||(!infront_of_ent(self,other)))
		return;

	if ((cvar("noexit") == 1) || ((cvar("noexit") == 2) && (mapname != "start")))
	{
// rjr quake2 change		T_Damage (other, self, self, 50000, 1000, TRUE);
		T_Damage (other, self, self, 50000);
		return;
	}

	if (self.movedir != '0 0 0')
	{
		makevectors (other.angles);
		if (v_forward * self.movedir < 0)
			return;		// not facing the right way
	}

	found = find(world,classname,"cube_of_force");
	while(found)
	{
		stopSound(found,0);
		remove(found);
		found = find(found,classname,"cube_of_force");
	}

	found = find (world, classname, "player");
	while (found != world)
	{
		if(found.model=="models/sheep.mdl")
			unsheep(found);
		found = find (found, classname, "player");
	}	

	//FIXME: temp server flags fix
//	other.sv_flags=serverflags;
	if (coop || deathmatch)
	{
		bprint (other.netname);
		bprint (" exited the level\n");
	}
	
	if (deathmatch)
		FindDMLevel();
	else
	{
		nextmap = self.map;
		nextstartspot = self.target;
	}

	SUB_UseTargets ();

	if (cvar("registered") == 0 && cvar("oem") == 0 && nextmap == "village1")
	{
		remove(self);
		intermission_running = 2;
		intermission_exittime = time + 20;
		WriteByte (MSG_ALL, SVC_INTERMISSION);
		WriteByte (MSG_ALL, 5);
		FreezeAllEntities();
		return;
	}

/*	if (self.spawnflags & 2)
	{
		serverflags (+) SFL_NEW_UNIT;
		serverflags (-) SFL_CROSS_TRIGGERS;
	}
	else
		serverflags (-) SFL_NEW_UNIT;
	if (self.spawnflags & 4)
	{
		serverflags (+) SFL_NEW_EPISODE;
		serverflags (-) SFL_CROSS_TRIGGERS;
	}
	else
		serverflags (-) SFL_NEW_EPISODE; */

//	rjr   spawnflag 1 use to be "no intermission" - removed the option completely
//	if ( (self.spawnflags & 1) && (deathmatch == 0) )

	if ( (deathmatch == 0) )
	{	// NO_INTERMISSION
		GotoNextMap();
		return;
	}
	
	self.touch = SUB_Null;

// we can't move people right now, because touch functions are called
// in the middle of C movement code, so set a think time to do it
	self.think = execute_changelevel;
	thinktime self : 0.1;
};

void() changelevel_use =
{
dprint("Changing level\n");
	local	entity	saveOther;

	saveOther = other;
	other = activator;
	changelevel_touch ();
	other = saveOther;
};

/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? x END_OF_UNIT END_OF_EPISODE
When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
Those spawnflags are from Quake- do nothing.
*/
void() trigger_changelevel =
{
	if (!self.map)
		objerror ("changelevel trigger doesn't have map");
	
	InitTrigger ();
	self.touch = changelevel_touch;
	self.use = changelevel_use;
};


/*
=============================================================================

				PLAYER GAME EDGE FUNCTIONS

=============================================================================
*/

//void() set_suicide_frame;
void()GibPlayer;
// called by ClientKill and DeadThink
void() respawn =
{
	if (coop)
	{
		// make a copy of the dead body for appearances sake
		SolidPlayer();
		// get the spawn parms as they were at level start
		setspawnparms (self);
		// respawn		
	
		PutClientInServer ();
	}
	else if (deathmatch)
	{
		// make a copy of the dead body for appearances sake
		SolidPlayer();
		PutClientInServer ();
	}
	else
	{	// restart the entire server
		if(parm7)
			changelevel (mapname, startspot);
		else
			localcmd ("restart restore\n");
	}
};


/*
============
ClientKill

Player entered the suicide command
============
*/
void() ClientKill =
{
entity lastleader,newking;
	bprint (self.netname);
	bprint (" suicides\n");
	self.model=self.init_model;
	GibPlayer();
	self.frags -= 2;	// extra penalty
	lastleader=FindExpLeader();
	drop_level(self,2);
	newking=FindExpLeader();
	if(newking!=lastleader)
	{//Tell everyone if the king of the hill has changed
		sound (world, CHAN_BODY, "misc/comm.wav", 1, ATTN_NONE);
		bprint(newking.netname);
		bprint(" is the NEW King of the Hill!\n");
		WriteByte(MSG_ALL, SVC_UPDATE_KINGOFHILL);
		WriteEntity (MSG_ALL, newking);
	}
	respawn ();
};


/*
============
SelectSpawnPoint

Returns the entity to spawn at
============
*/
//@@ TODO: not fixed order!!!
entity() SelectSpawnPoint =
{//FIXME: if start on 2nd - 5th hubs, fill in correct startspot string
	entity spot;
	entity thing;
	float  pcount;
	float ok;
	
	
// choose a info_player_deathmatch point
	if(self.newclass)
	{
	dprint("newclass start\n");
		spot = find(world, classname, "classchangespot");
		if(spot)
		{
			spot.think=SUB_Remove;
			thinktime spot : 1;
			return spot;
		}
	}

	if (coop)
	{
	dprint("Coop start\n");
		spot = lastspawn;
		pcount = 1;
		while (pcount < 3)
		{
			spot = find(spot, classname, "info_player_coop");

			/*			if(spot.playerclass)
				dprintf("Spot has a playerclass %s\n",spot.playerclass);
			if(spot.playerclass>0&&spot.playerclass!=self.playerclass)
				dprint("Skipping spawn spot\n");
			else*/ 
			
			if (spot != world && ((spot.targetname == startspot) ||
				(startspot == string_null && spot.spawnflags & 1)))
			{
/*				self.scale=1.5;
				self.proj_ofs_z*=1.5;
				self.drawflags(+)SCALE_ORIGIN_BOTTOM;*/
//		dprint(spot.targetname);
//		dprint(" checking Spot at");
//		dprintv("%s\n",spot.origin);
				thing = findradius(spot.origin, 64);
				ok = TRUE;
				while (thing)
				{
					if (thing.classname == "player")
					{
						thing = world;
						ok = FALSE;
					}
					else
						thing = thing.chain;
				}
				if (ok)
				{
//		dprint("found spot at");
//		dprintv("%s\n",spot.origin);
					lastspawn = spot;
					return lastspawn;
				}
			}
			if (spot == world)
				pcount += 1;
		}
/*		dprint("Resorting to info_player_start\n");
		lastspawn = find (lastspawn, classname, "info_player_start");
		if (lastspawn != world)
			return lastspawn;*/
	}
	else if (deathmatch)
	{
	dprint("dmatch start\n");
		spot = lastspawn;
		while (1)
		{
			spot = find(spot, classname, "info_player_deathmatch");
			if (spot != world&&random()<0.5)//add some randomness
			{
				if (spot == lastspawn)
					return lastspawn;
				pcount = 0;
				thing = findradius(spot.origin, 64);
				while(thing)
				{
					if (thing.classname == "player")
						pcount = pcount + 1;
					thing = thing.chain;
				}
				if (pcount == 0)
				{
					lastspawn = spot;
					return spot;
				}
			}
		}
	}

	if (startspot)
	{
	dprint("Searching for info_player_start with targetname\n");
		spot = world;
		pcount = 1;
		while(pcount)
		{
			spot = find (spot, classname, "info_player_start");
			if (!spot)
				pcount = 0;
			else if (spot.targetname == startspot)
				pcount = 0;
		}
	}
	
	if (!spot)
	{
	dprint("Resorting to info_player_start\n");
		spot = find (world, classname, "info_player_start");
		if (!spot)
			error ("PutClientInServer: no info_player_start on level");
	}

	return spot;
};

/*
===========
PutClientInServer

called each time a player is spawned
============
*/
void() PlayerDie;

void() PutClientInServer =
{
entity spot;
float pclass;

	spot = SelectSpawnPoint ();

	if(deathmatch)
	{
		self.items(-)IT_WEAPON4|IT_WEAPON3|IT_WEAPON4_1|IT_WEAPON4_2|IT_WEAPON2;
		self.skin=0;
	}
//	else if(self.sv_flags)
//		serverflags=self.sv_flags;


	self.classname = "player";
	self.takedamage = DAMAGE_YES;
	self.solid = SOLID_SLIDEBOX;
	self.movetype = MOVETYPE_WALK;
	self.deathtype="";
	self.viewentity=self;
	self.wallspot='0 0 0';
//	self.scale=1.5;
	self.scale=1;
	self.skin=0;
//	self.drawflags=	self.abslight=self.effects=0;
	self.abslight=self.effects=0;
	self.flags(+)FL_CLIENT;
	self.flags2(+)FL_ALIVE;
	self.air_finished = time + 12;
	self.dmg = 2;   		// initial water damage
	self.thingtype=THINGTYPE_FLESH;
	self.adjust_velocity = '-999 -999 -999';
//Reset all time-based fields
	self.dflags=
	self.act_state =
	self.show_hostile = 
	self.onfire=
	self.invisible_time=
	self.camptime=
	self.last_attack=
	self.torchtime=
	self.healthtime=
	self.catapult_time=
	self.safe_time=
	self.absorb_time=
	self.last_impact=
	self.sheep_sound_time=
	self.still_time=
	self.last_onground=
	self.invisible_finished=
	self.invincible_time=
	self.splash_time=
	self.ring_regen_time=
	self.rings_low=
	self.pausetime = 
	self.teleport_time = 
	self.sheep_time =
	self.attack_finished = 
	self.super_damage_time=
	self.haste_time =
	self.tome_time =
	self.camera_time=
	self.ring_regen_time=
	self.ring_flight_time=
	self.ring_water_time=
	self.ring_turning_time=
	self.super_damage=
	self.super_damage_low=
	self.hasted=
	self.decap=
	self.frozen=
	self.plaqueflg = 0;
	self.raven_cnt = 0;
	self.friction=self.gravity=self.standard_grav = 1;
	self.artifact_active(-)ARTFLAG_FROZEN|ARTFLAG_STONED;

/*	if(spot.playerclass)
	{
		self.playerclass=spot.playerclass;
		setclass(self,self.playerclass);
		stats_NewClass(self);
		self.newclass=FALSE;
	}
	else
	{*/
		if(self.newclass)
		{
			bprint(self.netname);
			bprint(" becomes a ");
			switch(self.newclass)
			{
			case CLASS_PALADIN:
				bprint("Paladin!\n");
				break;
			case CLASS_CRUSADER:
				bprint("Crusader!\n");
				break;
			case CLASS_NECROMANCER:
				bprint("Necromancer!\n");
				break;
			case CLASS_SUCCUBUS:
				bprint("Demoness!\n");
				break;
			default:
				bprint("Assassin!\n");
				break;
			}
			self.playerclass=self.newclass;
			setclass(self,self.playerclass);
			stats_NewClass(self);
			self.newclass=FALSE;
		}

		if(deathmatch&&randomclass)
			self.playerclass=CLASS_NONE;

		if (self.playerclass == CLASS_NONE)
		{ // Default it to the paladin if not selected
			if (cvar("registered") != 0 || cvar("oem") != 0)
			{//not sure what rint was doing to the results, threw
				//in this bad temp hack for now
				pclass=random(0,5);
				switch(pclass)
				{
					case 0..1:
						setclass(self,1);
						break;
					case 1..2:
						setclass(self,2);
						break;
					case 2..3:
						setclass(self,3);
						break;
					case 3..4:
						setclass(self,4);
						break;
					default:
						setclass(self,5);
						break;
				}
			}
			else
			{
				if (random() < 0.5)
					setclass(self,CLASS_PALADIN);
				else
					setclass(self,CLASS_ASSASSIN);
			}
		}
//	}

	if(self.max_health<=0)
		stats_NewPlayer(self);
	else
		self.health = self.max_health;
	
	if(self.max_health<=0||self.health<=0)
	{
		dprint("ERROR: Respawned Dead!\n");
		dprintf("Class: %s\n",self.playerclass);
		dprint("Map: ");
		dprint(mapname);
		dprint("\n");
		dprintf("Max: %s\n",self.max_health);
		dprintf("Health: %s\n",self.health);
		dprint("Autofix: health default to 100\n");
		self.health=self.max_health=100;
	}

	self.deadflag = DEAD_NO;

	setorigin(self, spot.origin + '0 0 1');
	self.angles = spot.angles;
	self.fixangle = TRUE;		// turn this way immediately

	if(!self.weapon)
	{
		self.items=IT_WEAPON1;
		self.weapon=IT_WEAPON1;
		self.oldweapon=IT_WEAPON1;
	}
	if(deathmatch)
		self.weapon=IT_WEAPON1;

	if(coop)
	{//Need more mana in coop, especially if you die
		if(self.bluemana<25)
			self.bluemana=25;
		if(self.greenmana<25)
			self.greenmana=25;
	}
	
	W_SetCurrentAmmo ();

	SetModelAndThinks();

	PlayerSpeed_Calc();

	if(deathmatch)
	{
		self.effects=0;
		self.artifact_active=ART_INVINCIBILITY;
		self.invincible_time = time + 3;
		self.artifact_low(+)ART_INVINCIBILITY;

		switch(self.playerclass)
		{
		case CLASS_PALADIN:
			self.effects(+)EF_BRIGHTLIGHT; 
			break;
		case CLASS_CRUSADER:
			self.skin = GLOBAL_SKIN_STONE;
			break;
		case CLASS_NECROMANCER:
			self.effects(+)EF_DARKLIGHT;
			break;
		case CLASS_SUCCUBUS:
			self.drawflags(+)MLS_ABSLIGHT|DRF_TRANSLUCENT;
			self.effects(+)EF_BRIGHTFIELD;
			self.abslight=1;
			break;
		default:	//assassin
			self.colormap=140;
			break;
		}
	}
	self.ring_regen_time = 0;
	self.ring_flight_time=0;	
	self.ring_water_time=0;	
	self.ring_turning_time=0;

	self.ring_flight=0;			// Health of rings 0 - 100
	self.ring_water=0;			// 
	self.ring_turning=0;		//
	self.ring_regeneration=0;	//
	self.rings = 0;

	self.view_ofs = '0 0 50';
	self.proj_ofs=' 0 0 44';
	self.hull=HULL_PLAYER;
	self.idealpitch = cvar("sv_walkpitch");
	

	self.button0 = self.button1 = self.button2 = 0;
	self.attack_finished=time+0.5;//so no respawn fire

//	self.th_stand();
	player_frames();
	
	if (deathmatch || coop)
	{
		makevectors(self.angles);
		GenerateTeleportEffect(self.origin,0);
	}

	spawn_tdeath (self.origin, self);
	if (self.artifact_flags & AFL_CUBE_LEFT)
	{
		self.cnt_cubeofforce+=1;
		self.artifact_flags(-)AFL_CUBE_LEFT;
		UseCubeOfForce(TRUE);
	}
	if (self.artifact_flags & AFL_CUBE_RIGHT)
	{
		self.cnt_cubeofforce+=1;
		self.artifact_flags(-)AFL_CUBE_RIGHT;
		UseCubeOfForce(TRUE);
	}
};


void ClientReEnter(float TimeDiff)
{
/*
	Called for living players entering a level
	(except for first starting a game)
	or when	you die any time other than on the
	first level	you started playing on.
*/
entity spot;
//string tempmodel;

	if(!self.flags2&FL_ALIVE||self.health<1||(self.newclass&&!deathmatch&&!coop))
	{//If dead, put them in the right spot.
		self.weapon=IT_WEAPON1;
		PutClientInServer();
		return;
	}

	// Need to reset these because they could still point to entities in the previous map
	self.enemy = self.groundentity = self.chain = self.goalentity = self.dmg_inflictor = 
		self.owner = world;

//RESET TIMERS:
	if(deathmatch)
	{
		self.items(-)IT_WEAPON4|IT_WEAPON2|IT_WEAPON3|IT_WEAPON4_1|IT_WEAPON4_2;
		self.skin=0;
	}
//	else if(self.sv_flags)
//		serverflags=self.sv_flags;

	self.movetype=MOVETYPE_WALK;
	self.viewentity=self;
	self.wallspot='0 0 0';
	self.deathtype="";
	self.act_state =
	self.onfire=
	self.healthtime=
	self.splash_time=
	self.decap=
	self.frozen= 
	self.plaqueflg = 0;
	self.raven_cnt = 0;
	self.friction=self.gravity=self.standard_grav = 1;
	self.artifact_active(-)ARTFLAG_FROZEN|ARTFLAG_STONED;

	self.ring_flight_time = 0;
	self.ring_flight = 0;
	self.rings (-) RING_FLIGHT;
	self.rings_active (-) RING_FLIGHT;


	self.air_finished   = time + 6;//+= TimeDiff;
	self.pain_finished  = time + 6;//+= TimeDiff;

	self.ring_regen_time += TimeDiff; 
	self.ring_water_time += TimeDiff; 
	self.ring_turning_time += TimeDiff; 
	self.dflags+=TimeDiff; 

	self.super_damage_time += TimeDiff; 
	self.haste_time  += TimeDiff; 
	self.tome_time  += TimeDiff; 
	self.camera_time  += TimeDiff; 
	self.torchtime += TimeDiff; 

	self.pausetime += TimeDiff; 
	self.teleport_time += TimeDiff; 
	self.sheep_time += TimeDiff; 
	self.attack_finished += TimeDiff;
	self.catapult_time+= TimeDiff;
	self.safe_time+= TimeDiff;
	self.absorb_time+= TimeDiff;
	self.last_impact+= TimeDiff;
	self.sheep_sound_time+= TimeDiff;
	self.still_time+= TimeDiff;
	self.last_onground+= TimeDiff;
	self.invincible_time+= TimeDiff;
	self.show_hostile+= TimeDiff;
	self.invisible_time+= TimeDiff;
	self.camptime+= TimeDiff;
	self.last_attack= self.attack_finished=0;

	self.light_level = 128;		// So the assassin doesn't go invisible coming out of the teleporter

	self.dmg = 2;   		// initial water damage

	setsize (self, '-16 -16 0', '16 16 56');	
	self.hull=HULL_PLAYER;
	self.view_ofs = '0 0 50';
	self.proj_ofs='0 0 44';

	spot = SelectSpawnPoint ();
	setorigin(self, spot.origin + '0 0 1');
	self.angles = spot.angles;
	self.fixangle = TRUE;		// turn this way immediately

	self.velocity = '0 0 0';
	self.avelocity = '0 0 0';
	self.adjust_velocity = '-999 -999 -999';

	if (deathmatch || coop)
	{
		makevectors(self.angles);
		GenerateTeleportEffect(self.origin,0);
	}

	spawn_tdeath (self.origin, self);

	SetModelAndThinks();
	PlayerSpeed_Calc();
	W_SetCurrentAmmo ();

	force_retouch = 2;		// make sure even still objects get hit

	self.think=player_frames;
	thinktime self : 0;

	if (self.artifact_flags & AFL_CUBE_LEFT)
	{
		self.cnt_cubeofforce+=1;
		self.artifact_flags(-)AFL_CUBE_LEFT;
		UseCubeOfForce(TRUE);
	}
	if (self.artifact_flags & AFL_CUBE_RIGHT)
	{
		self.cnt_cubeofforce+=1;
		self.artifact_flags(-)AFL_CUBE_RIGHT;
		UseCubeOfForce(TRUE);
	}
}

/*
=============================================================================

				QUAKED FUNCTIONS

=============================================================================
*/


/*QUAKED info_player_start (1 0 0) (-16 -16 0) (16 16 56)
The normal starting point for a level.
-----------------------FIELDS-------------------------

--------------------------------------------------------
*/
void() info_player_start =
{
};


/*QUAKED info_player_start2 (1 0 0) (-16 -16 0) (16 16 56)
Only used on start map for the return point from an episode.
-----------------------FIELDS-------------------------

--------------------------------------------------------
*/
void() info_player_start2 =
{
};


/*
saved out by quak ed in region mode
*/
/* nope, it sends out a normal player_start
void() testplayerstart =
{
};
*/
/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 0) (16 16 56)
potential spawning position for deathmatch games
-----------------------FIELDS-------------------------

--------------------------------------------------------
*/
void() info_player_deathmatch =
{
	if(!deathmatch)
		remove(self);
};

/*QUAKED info_player_coop (1 0 1) (-16 -16 0) (16 16 56) DEFAULT
potential spawning position for coop games
-----------------------FIELDS-------------------------
'playerclass'
1-Paladin
2-Crusader
3-Necromancer
4-Assassin
5-Succubus
--------------------------------------------------------
*/
void() info_player_coop =
{
};

/*
===============================================================================

RULES

===============================================================================
*/

/*
go to the next level for deathmatch
only called if a time or frag limit has expired
*/
void() NextLevel =
{
	entity o;

	FindDMLevel();

	if (nextmap == "")
	{
		// find a trigger changelevel
		o = find(world, classname, "trigger_changelevel");

		// go back to start if no trigger_changelevel
		if (!o)
		{
			o = spawn();
			mapname = "demo1";
			o.map = mapname;
		}
	}
	else
	{
		o = spawn();
		o.map = nextmap;
	}

	gameover = TRUE;
	
	if (o.nextthink < time)
	{
		o.think = execute_changelevel;
		thinktime o : 0.1;
	}
};

/*
============
CheckRules

Exit deathmatch games upon conditions
============
*/
void() CheckRules =
{
float		timelimit;
float		fraglimit;
	
	if (gameover)	// someone else quit the game already
		return;
		
	timelimit = cvar("timelimit") * 60;
	fraglimit = cvar("fraglimit");
	
	if (timelimit && time >= timelimit)
	{
		NextLevel ();
		return;
	}
	
	if (fraglimit && self.frags >= fraglimit)
	{
		NextLevel ();
		return;
	}	
};

//============================================================================

void() PlayerDeathThink =
{
float		forward;

	if ((self.flags & FL_ONGROUND))
	{
		forward = vlen (self.velocity);
		forward = forward - 20;
		if (forward <= 0)
			self.velocity = '0 0 0';
		else	
			self.velocity = forward * normalize(self.velocity);
	}

// wait for all buttons released
	if (self.deadflag == DEAD_DEAD)
	{
		if (self.button2 || self.button0)
			return;
		self.deadflag = DEAD_RESPAWNABLE;
		return;
	}

// wait for any button down
	if (!self.button2 && !self.button1 && !self.button0)
		return;

	self.button0 = 0;
	self.button1 = 0;
	self.button2 = 0;
	respawn();
};

void CheckWaterJump()
{
vector start, end;

// check for a climb out of water
	makevectors (self.angles);
	start = self.origin + self.proj_ofs - '0 0 8';
	v_forward_z = 0;
	normalize(v_forward);
	end = start + v_forward*24;
	traceline (start, end, TRUE, self);
	if (trace_fraction < 1)
	{	// solid at waist
		if(self.model=="models/sheep.mdl")
			start_z = self.origin_z + self.proj_ofs_z + 26;//was absmax - 8
		else
			start_z = self.origin_z + self.proj_ofs_z + 6;//was absmax - 8
		end = start + v_forward*24;
		self.movedir = trace_plane_normal * -50;
		traceline (start, end, TRUE, self);
		if (trace_fraction == 1)
		{	// open at eye level
			self.flags(+)FL_WATERJUMP;
			self.velocity_z = 225;
			self.flags(-)FL_JUMPRELEASED;
			self.teleport_time = time + 2;	// safety net
			return;
		}
	}
}

void()catapult_fire;
void() PlayerJump =
{
float wall_jump;
	if(self.flags&FL_ONGROUND)
	{
		traceline(self.origin,self.origin-'0 0 3',FALSE,self);
		if(trace_ent.classname=="catapult"&&trace_ent.frame==20)
		{
			trace_ent.think=catapult_fire;
			thinktime trace_ent : 0;
		}
	}

	if (self.flags & FL_WATERJUMP)
		return;

	if (self.movetype==MOVETYPE_FLY)
		return;

	if (self.waterlevel >= 2)
	{
		if (self.watertype == CONTENT_WATER)
			self.velocity_z = 100*self.scale;
		else if (self.watertype == CONTENT_SLIME)
			self.velocity_z = 80*self.scale;
		else
			self.velocity_z = 50*self.scale;

// play swiming sound
		if (self.swim_flag < time)
		{
			self.swim_flag = time + 1;
			if (random() < 0.5)
				sound (self, CHAN_BODY, "player/swim1.wav", 1, ATTN_NORM);
			else
				sound (self, CHAN_BODY, "player/swim2.wav", 1, ATTN_NORM);
		}

		return;
	}

	if (!(self.flags & FL_ONGROUND))
	{
		if(cvar("sv_gravity")>400)//On low-grav levels, allow players to push off walls
			return;
		makevectors(self.v_angle);
		traceline(self.origin+self.proj_ofs,self.origin+self.proj_ofs+v_forward*64,FALSE,self);
		if(trace_fraction<1&&trace_ent==world&&trace_plane_normal!='0 0 0')
			wall_jump=TRUE;
		else
			return;
	}
	
	if ( !(self.flags & FL_JUMPRELEASED) )
		return;		// don't pogo stick

	self.act_state=ACT_JUMP;

	self.flags(-)FL_JUMPRELEASED;

	self.flags(-)FL_ONGROUND;	// don't stairwalk
	
	self.button2 = 0;
// player jumping sound

	if(self.model=="models/sheep.mdl")//self.modelindex==modelindex_sheep)
		sheep_sound(1);
	else if(self.playerclass==CLASS_ASSASSIN||self.playerclass==CLASS_SUCCUBUS)
		sound (self, CHAN_BODY,"player/assjmp.wav", 1, ATTN_NORM);
	else
		sound (self, CHAN_BODY,"player/paljmp.wav", 1, ATTN_NORM);

//Unless can do a flip of angles, this looks weird
	if(wall_jump)
		self.velocity = v_forward*-270*self.scale;
	else
		self.velocity_z = self.velocity_z + 270*self.scale;
};




/*
===========
WaterMove

============
*/

void() WaterMove =
{

//dprint (ftos(self.waterlevel));
	if (self.movetype == MOVETYPE_NOCLIP)
		return;
	if (self.health <= 0)
		return;

	if ((self.flags & FL_INWATER) &&
		(self.watertype == CONTENT_WATER) &&
		(self.waterlevel == 3) &&
		(self.air_finished>=time+11.5))//&&!self.lefty
	{//OOPS- no free edicts crash?
//		DeathBubbles(10);//was using self.lefty
		DeathBubbles(1);
//		self.lefty = 1;
	}

/*	if ((self.flags & FL_INWATER) && (self.splash_time < time))
	{
		if (((self.velocity_x) || (self.velocity_y) || (self.velocity_z)) && (self.watertype == CONTENT_WATER))
		{
			if (self.waterlevel == 1)
			{
				CreateWaterSplash(self.origin + '0 0 10');
			}
			else if (self.waterlevel == 2)
			{
				CreateWaterSplash(self.origin + '0 0 20');
			}
		}

		self.splash_time = time + random(HX_FRAME_TIME,HX_FRAME_TIME*2);
	}
*/

	if (self.waterlevel != 3) // Not up to the eyes
	{
		if (self.air_finished < time)
		{
			if (self.model=="models/sheep.mdl")
				sheep_sound(1);
			else if(self.playerclass==CLASS_ASSASSIN||self.playerclass==CLASS_SUCCUBUS)
				sound (self, CHAN_VOICE, "player/assgasp1.wav", 1, ATTN_NORM);
			else
				sound (self, CHAN_VOICE, "player/palgasp1.wav", 1, ATTN_NORM);
		}
		else if (self.air_finished < time + 7)
		{
			if (self.model=="models/sheep.mdl")
				sheep_sound(1);
			else if(self.playerclass==CLASS_ASSASSIN||self.playerclass==CLASS_SUCCUBUS)
				sound (self, CHAN_VOICE, "player/assgasp2.wav", 1, ATTN_NORM);
			else
				sound (self, CHAN_VOICE, "player/palgasp2.wav", 1, ATTN_NORM);
		}
		self.air_finished = time + 12;
		self.dmg = 2;
	}
	// Completely submerged and no air
	else
	{
//		dprintf("time: %s\n",time);
//		dprintf("air fin: %s\n",self.air_finished);
		if ((self.air_finished < time) && (!self.rings & RING_WATER))
		{
//			dprint("checking drown\n");
			if(self.playerclass==CLASS_PALADIN&&self.flags&FL_SPECIAL_ABILITY1)
			{
//				dprint("paladin free action\n");
				self.air_finished = time + 12;
				self.dmg = 2;
			}
			else if (self.pain_finished < time)
			{// Drown
//				dprint("drowning\n");
				self.dmg = self.dmg + 2;
				if (self.dmg > 15)
					self.dmg = 10;
				T_Damage (self, world, world, self.dmg);
				self.pain_finished = time + 1;
			}
//			else
//				dprintf("pain_finished(%s) > time\n",self.pain_finished);
		}
	}
	
	if (!self.waterlevel)
	{  // Getting out of the water
		if (self.flags & FL_INWATER)	
		{	// play leave water sound
			sound (self, CHAN_BODY, "raven/outwater.wav", 1, ATTN_NORM);
			self.flags(-)FL_INWATER;
			self.lefty = 0;
		}
		return;
	}

	if (self.watertype == CONTENT_LAVA)
	{	// do damage
		if (self.dmgtime < time)
		{
			self.dmgtime = time + 0.5;

			if(self.flags2&FL2_FIREHEAL)
				self.health=self.health+5*self.waterlevel;
			else if(!self.flags2&FL2_FIRERESIST)
				T_Damage (self, world, world, 5*self.waterlevel);
			else
				T_Damage (self, world, world, 2*self.waterlevel);
		}
	}
	else if (self.watertype == CONTENT_SLIME)
	{	// do damage
		if (self.dmgtime < time)
		{
			self.dmgtime = time + 1;
			T_Damage (self, world, world, 4*self.waterlevel);
		}
	}

	// Just entering fluid
	if (!(self.flags & FL_INWATER))
	{
		self.splash_time = time + .05;

		// player enter water sound
		if (self.watertype == CONTENT_LAVA)
			sound (self, CHAN_BODY, "raven/inlava.wav", 1, ATTN_NORM);
		else if (self.watertype == CONTENT_WATER)
		{
			sound (self, CHAN_BODY, "raven/inh2o.wav", 1, ATTN_NORM);
		}
		else if (self.watertype == CONTENT_SLIME)
			sound (self, CHAN_BODY, "player/slimbrn1.wav", 1, ATTN_NORM);

		self.flags(+)FL_INWATER;
		self.dmgtime = 0;

	}
	
	if (! (self.flags & FL_WATERJUMP) )
		self.velocity = self.velocity - 0.8*self.waterlevel*frametime*self.velocity;
};

void CheckCrouch (void)
{
	if ((self.crouch_time) && (self.crouch_time < time))  // Time to crouch or uncrouch a little
	{
		if (self.hull==HULL_CROUCH) // Player crouching
		{
			self.crouch_stuck = 0;
			self.view_ofs_z -= 10;
			self.proj_ofs_z -= 10;
			if (self.view_ofs_z < 24)
			{
				self.view_ofs_z = 24;
				self.proj_ofs_z = 18;
				self.crouch_time = 0;
			}	 
			else
				self.crouch_time = time + HX_FRAME_TIME/4;
		}
		else
		{
			self.view_ofs_z += 10;
			self.proj_ofs_z += 10;
			if (self.view_ofs_z > 50)
			{
				self.view_ofs_z = 50;
				self.proj_ofs_z = 44;
				self.crouch_time = 0;
			}	 
			else
				self.crouch_time = time + HX_FRAME_TIME/4;
		}
	}

	if ((self.flags2 & FL2_CROUCHED||self.model=="models/sheep.mdl"||self.flags2&FL2_CROUCH_TOGGLE) && (self.hull!=HULL_CROUCH)) 
		PlayerCrouching ();
	else if (((!self.flags2 & FL2_CROUCHED&&self.model!="models/sheep.mdl"&&!self.flags2&FL2_CROUCH_TOGGLE) && (self.hull==HULL_CROUCH)) ||
			(self.crouch_stuck))  // If stuck, constantly try to unstick
		PlayerUnCrouching();

}

void CheckIncapacities ()
{
vector dir;
	if(self.frozen>0)
		if(self.flags2&FL_ALIVE&&self.health)
		{
			if(self.colormap>144)
			{
				self.colormap-=0.5;
				self.abslight-=0.025;
			}
			else
			{
				self.colormap=0;
				self.abslight=0.5;
				self.skin=GLOBAL_SKIN_ICE;
			}
			if(self.pausetime<=time)
			{
				if(self.skin==GLOBAL_SKIN_ICE)
					self.skin=self.oldskin;
				self.colormap=0;
				self.abslight=0;
				self.thingtype=THINGTYPE_FLESH;
				self.drawflags(-)DRF_TRANSLUCENT|MLS_ABSLIGHT;
				self.frozen=FALSE;
				self.artifact_active(-)ARTFLAG_FROZEN;
				self.touch=PlayerTouch;
			}
		}
		else
			self.frozen=self.pausetime=self.teleport_time=0;


	if(self.pausetime>time&&self.model!=self.headmodel)
	{
		if(self.model=="models/flesh1.mdl")
		{
			dir=normalize(self.wallspot-self.origin+self.view_ofs);
			dir=vectoangles(dir);
			self.o_angle_x=dir_x*-1;
			self.o_angle_y=dir_y;
			self.o_angle_z=self.v_angle_z;
		}
		else if(!self.flags2&FL_ALIVE&&self.enemy.flags2&FL_ALIVE)//&&visible(self.enemy))
		{//face enemy
			self.o_angle=normalize(self.enemy.origin+self.enemy.proj_ofs-self.origin+self.view_ofs);
			self.o_angle=vectoangles(self.o_angle);
			self.o_angle_x*=-1;//have to reverse the pitch
			if(self.o_angle_y>180)
				self.o_angle_y-=360;
			else if(self.o_angle_y<-180)
				self.o_angle_y+=360;
			self.o_angle_z=self.v_angle_z;
			self.o_angle-=self.v_angle;
			if(self.o_angle_x>7)
				self.o_angle_x=7;
			else if(self.o_angle_x<-7)
				self.o_angle_x=-7;
			if(self.o_angle_y>10)
				self.o_angle_y=10;
			else if(self.o_angle_y<-10)
				self.o_angle_y=-10;
			self.o_angle+=self.v_angle;
		}

        msg_entity = self;
	    WriteByte (MSG_ONE, 10);
        WriteAngle (MSG_ONE, self.o_angle_x);
	    WriteAngle (MSG_ONE, self.o_angle_y);
        WriteAngle (MSG_ONE, self.o_angle_z);
	    if(self.flags&FL_ONGROUND)
			self.velocity='0 0 0';
        self.button0=0;
		self.button2=0;
		self.impulse=0;
	}

	if(self.flags2&FL_CHAINED)
		self.button0=self.button1=self.button2=0;
}

/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void() PlayerPreThink =
{
	vector	spot1, spot2;	

	self.friction = 1;

	if ((self.health<=0) && (self.movetype!=MOVETYPE_NOCLIP))
	{
		if(pointcontents(self.origin)==CONTENT_SOLID&&self.origin!='0 0 0')
		{
			self.velocity='0 0 0';
			self.solid=SOLID_NOT;
			self.movetype=MOVETYPE_NOCLIP;
			setorigin(self,self.oldorigin);
		}
	}
	if (!self.flags & FL_INWATER) 
		self.aflag = 0;

	if (intermission_running)
	{
		IntermissionThink ();	// otherwise a button could be missed between
		return;					// the think tics
	}

	if (self.view_ofs == '0 0 0'&&
		self.viewentity.classname!="chasecam"&&
		!self.button0&&!self.button2)//Causing them to not be able to respawn?
		return;		// intermission or finale

	if (self.adjust_velocity_x != -999)
	{
		self.velocity_x = self.adjust_velocity_x;
	}
	if (self.adjust_velocity_y != -999)
	{
		self.velocity_y = self.adjust_velocity_y;
	}
	if (self.adjust_velocity_z != -999)
	{
		self.velocity_z = self.adjust_velocity_z;
	}
	self.adjust_velocity = '-999 -999 -999';

	CheckIncapacities();

	if(self.viewentity!=self)
	{
		CameraViewPort(self,self.viewentity);
		if(self.viewentity.classname!="chasecam")//&&self.viewentity.classname!="camera_remote")
		{
			self.weaponframe=self.viewentity.weaponframe;
			self.weaponmodel=self.viewentity.weaponmodel;
			CameraViewAngles(self,self.viewentity);
		}
		else
			self.weaponmodel="";
	}

	makevectors (self.v_angle);		// is this still used

	if(self.flags2&FL2_TEST_TRACE)
	{
		traceline(self.origin+self.view_ofs,self.origin+self.view_ofs+v_forward*128,TRUE,self);
		if(trace_ent.solid==SOLID_BSP)
		{
			dprintv("Normal : %s\n",trace_plane_normal);
			spawntestmarker(trace_endpos,0.05,1);
		}
	}

	CheckRules ();
	CheckRings ();
	CheckAbilities ();

	WaterMove ();

	if (self.waterlevel == 2)
		CheckWaterJump ();

	if (self.deadflag >= DEAD_DEAD)
	{
		PlayerDeathThink ();
		return;
	}

	CheckCrouch ();//don't try to uncrouch when dead

	// Turn off plaque if it is on
	if (self.plaqueflg)
	{	// Is moving or looking around so kill plaque
		if (((self.velocity_x) || (self.velocity_y) || (self.velocity_z)) ||
			(self.plaqueangle != self.v_angle))
		{
			makevectors (self.v_angle);

			spot1 = self.origin + self.view_ofs;
			spot2 = spot1 + (v_forward*25); // Look just a little ahead
			traceline (spot1, spot2 , FALSE, self);

			if ((trace_fraction == 1.0) || (trace_ent.classname!="plaque"))
			{
				traceline (spot1, spot2 - (v_up * 30), FALSE, self);  // 30 down
			
				if ((trace_fraction == 1.0) || (trace_ent.classname!="plaque"))
				{
					traceline (spot1, spot2 + v_up * 30, FALSE, self);  // 30 up
				
					if ((trace_fraction == 1.0) || (trace_ent.classname!="plaque"))
					{
						self.plaqueflg=0;
						msg_entity = self;
						plaque_draw(MSG_ONE,0);
					}
				}
			}

			if (self.plaqueflg)
				self.plaqueangle = self.v_angle;
		}
	}

	// Twitch every so often if not moving
	if ((!self.velocity_x) && (!self.velocity_y) && (!self.velocity_z))
	{
		// FIXME: needs to be a random number between 5 - 8 minutes or so
		if ((self.camptime + 600) < time)
		{
			if (self.playerclass==CLASS_PALADIN)
			{
				if (self.weapon==IT_WEAPON1)
					gauntlet_twitch();
				else if (self.weapon==IT_WEAPON2)
					vorpal_twitch();
				self.camptime = time + random(840,420);
			}
		}
	}
	else
		self.camptime = time + random(420,840);


	if (self.deadflag == DEAD_DYING)
		return;	// dying, so do nothing

	if (self.button2)
		PlayerJump ();
	else
		self.flags(+)FL_JUMPRELEASED;

// teleporters can force a non-moving pause time	
	if (time < self.pausetime)
		self.velocity = '0 0 0';

	//// TEMP::God Mode Mana Save
	if (self.flags & FL_GODMODE)
	{
		self.bluemana = self.max_mana;
		self.greenmana = self.max_mana;
	}

	// Change weapon
	if (time > self.attack_finished && self.weapon != IT_WEAPON1)
	{
		if (((self.weapon == IT_WEAPON3) && (self.greenmana<1)) ||
			((self.weapon == IT_WEAPON4) && (self.bluemana<1) && (self.greenmana<1)))
		{
			W_BestWeapon ();
			W_SetCurrentWeapon ();
		}
	}
};

void CheckRings (void)
{
	entity victim;
	vector dir;
	float chance;

	if (self.health <= 0)
		return;

	if (self.rings & RING_REGENERATION)
	{	
		if (self.ring_regen_time < time)
		{
			if (self.health < self.max_health)
			{
				self.ring_regeneration -= 100/RING_REGENERATION_MAX;
				self.health += 1;				
				self.ring_regen_time = time + 1;
			}	

			if ((self.ring_regeneration < 10)  && (!self.rings_low & RING_REGENERATION))
			{
				self.rings_low (+) RING_REGENERATION;
				centerprint (self, "Ring of Regeneration is running low");
				sound (self, CHAN_BODY, "misc/comm.wav", 1, ATTN_NORM);
			}

			if (self.ring_regeneration <=0)
			{
				self.ring_regeneration = 0;
				self.rings (-) RING_REGENERATION;
				self.rings_active (-) RING_REGENERATION;
			}
		}
	}

	if (self.rings & RING_FLIGHT)
	{	
		if (self.ring_flight_time < time)
		{
			self.ring_flight -= 100/RING_FLIGHT_MAX;

			if ((self.ring_flight < 25)  && (!self.rings_low & RING_FLIGHT))
			{
				self.rings_low (+) RING_FLIGHT;
				centerprint (self, "Ring of Flight is running low");
				sound (self, CHAN_BODY, "misc/comm.wav", 1, ATTN_NORM);
			}

			if (self.ring_flight <=0)
			{
				self.ring_flight = 0;
				self.rings (-) RING_FLIGHT;
				self.rings_active (-) RING_FLIGHT;
				player_stopfly();
				if (deathmatch)
					self.cnt_flight -= 1;
			}
			self.ring_flight_time = time + 1;
		}
	}


	if ((self.rings & RING_WATER) && (self.waterlevel == 3) && (self.air_finished < time))
	{	
		self.rings_active (+) RING_WATER;
		if (self.ring_water_time < time)
		{
			self.ring_water -= 100/RING_WATER_MAX;

			if ((self.ring_water < 25)  && (!self.rings_low & RING_WATER))
			{
				self.rings_low (+) RING_WATER;
				centerprint (self, "Ring of Water Breathing is running low");
				sound (self, CHAN_BODY, "misc/comm.wav", 1, ATTN_NORM);
			}

			if (self.ring_water <=0)
			{
				self.ring_water = 0;
				self.rings (-) RING_WATER;
				self.rings_active (-) RING_WATER;
			}
			self.ring_water_time = time + 1;
		}
	}
	else
		self.rings_active (-) RING_WATER;

	if (self.rings & RING_TURNING)
	{	
		victim = findradius( self.origin, 100);
		while(victim)
		{
			if ((victim.movetype == MOVETYPE_FLYMISSILE||
				victim.movetype == MOVETYPE_BOUNCEMISSILE||
				victim.movetype == MOVETYPE_BOUNCE)&&
				(victim.owner != self))
			{
				victim.frags=2;//For client death messages
				victim.owner = self;
				chance = random();
				dir = victim.origin + (v_forward * -1);
				CreateLittleWhiteFlash(dir);
				sound (self, CHAN_WEAPON, "weapons/vorpturn.wav", 1, ATTN_NORM);
				if (chance < 0.9)  // Deflect it
				{
					victim.v_angle = self.v_angle + randomv('0 0 0', '360 360 360'); 
					makevectors (victim.v_angle);
					victim.velocity = v_forward * 1000;
				}
				else  // reflect missile
					victim.velocity = '0 0 0' - victim.velocity;
				if(victim.movedir!='0 0 0')
					victim.movedir=normalize(victim.velocity);
				if(victim.classname=="pincer")
					victim.enemy=victim.owner;
			}
			victim = victim.chain;
		}


		if (self.ring_turning_time < time)
		{
			self.ring_turning -= 100/RING_TURNING_MAX;

			if ((self.ring_turning < 10)  && (!self.rings_low & RING_TURNING))
			{
				self.rings_low (+) RING_TURNING;
				centerprint (self, "Ring of Reflection is running low");
				sound (self, CHAN_BODY, "misc/comm.wav", 1, ATTN_NORM);
			}

			if (self.ring_turning <=0)
			{
				self.ring_turning = 0;
				self.rings (-) RING_TURNING;
				self.rings_active (-) RING_TURNING;
			}

			self.ring_turning_time = time + 1;
		}
	}

}
	
void remove_invincibility(entity loser)
{
	loser.artifact_low(-)ART_INVINCIBILITY;
	loser.artifact_active (-) ART_INVINCIBILITY;
	loser.invincible_time = 0;
	loser.air_finished = time + 12;
	
	switch(loser.playerclass)
	{
	case CLASS_PALADIN:
		loser.effects(-)EF_BRIGHTLIGHT; 
		break;
	case CLASS_CRUSADER:
		loser.skin = 0;
		break;
	case CLASS_NECROMANCER:
		loser.effects(-)EF_DARKLIGHT;
		break;
	case CLASS_SUCCUBUS:
		loser.abslight=0;
		loser.drawflags(-)DRF_TRANSLUCENT|MLS_ABSLIGHT;
		loser.effects(-)EF_BRIGHTFIELD;
		loser.flags2(-)FL2_FADE_UP;
		break;
	default:	//assassin
		loser.colormap=0;
		break;
	}
}
/*
================
CheckPowerups

Check for turning off powerups
================
*/
void() CheckPowerups =
{
	if (self.health <= 0)
		return;

	if (self.divine_time < time)
		self.artifact_active (-) ARTFLAG_DIVINE_INTERVENTION;
		
	// Crusader's special ability to smite 
	if (self.super_damage)
	{
		if (self.super_damage_time < time) 
		{
			self.super_damage = 0;		
		}
		else if (((self.super_damage_time - 10) < time) && (!self.super_damage_low))
		{
			self.super_damage_low = 1;
			sprint (self, "Holy Strength begins to wane\n");
			stuffcmd (self, "bf\n");
		}
	}

	if (self.artifact_active & ART_HASTE)
	{
		if (self.haste_time < time)
		{
			self.artifact_low =self.artifact_low - (self.artifact_low & ART_HASTE);
			self.artifact_active =self.artifact_active - (self.artifact_active & ART_HASTE);
			self.effects(-)EF_DARKFIELD;
			PlayerSpeed_Calc();
			self.haste_time = 0;
			self.air_finished = time + 12;
		}
		else if ((self.haste_time - 10) < time)
			self.artifact_low = self.artifact_low | ART_HASTE;
	}


	if (self.artifact_active & ART_INVINCIBILITY)
	{
		if (self.invincible_time < time)
			remove_invincibility(self);
		else if ((self.invincible_time - 10) < time)
			self.artifact_low = self.artifact_low | ART_INVINCIBILITY;
		if(self.playerclass==CLASS_SUCCUBUS)
		{
			vector vect, v1, v2;
		
			vect='0 0 0';
			vect_y=(self.invincible_time - time)*480;
			makevectors(vect);
			vect = self.origin + self.proj_ofs + v_forward*32;
			if (random() < 0.5)
			v1 = randomv('-10 -10 25', '10 10 45');
			v2 = randomv('-10 -10 25', '10 10 45');
			particle2(vect, v1, v2, 416,PARTICLETYPE_FIREBALL,7);
			vect = self.origin + self.proj_ofs - v_forward*32;
			v1_z=v2_z=0;
			particle2(vect,v1, v2, 135,PARTICLETYPE_REDFIRE,3);

			/*float add_dir;
			
			if(self.dflags<=time)
			{
				self.effects(+)EF_BRIGHTFIELD;
				self.dflags=time+1;
			}
			if(self.flags2&FL2_FADE_UP)
				add_dir=0.01;
			else
				add_dir=-0.01;
			self.abslight+=add_dir;
			if(self.abslight<=0.01)
			{//FIXME- stay invis for a short bit
				self.flags2(+)FL2_FADE_UP;
				self.abslight=0.01;
			}
			else if(self.abslight>=1)
			{
				self.flags2(-)FL2_FADE_UP;
				self.abslight=1;
			}*/
		}
	}

	if (self.artifact_active & ART_TOMEOFPOWER)
		self.drawflags(+)MLS_POWERMODE;
//	{
//		if ((self.drawflags & MLS_MASKIN) != MLS_POWERMODE)
//			self.drawflags = (self.drawflags & MLS_MASKOUT)| MLS_POWERMODE;

		if (self.tome_time < time)
		{
			self.artifact_low = self.artifact_low - (self.artifact_low & ART_TOMEOFPOWER);
			self.artifact_active = self.artifact_active - (self.artifact_active & ART_TOMEOFPOWER);
			self.tome_time = 0;
			self.drawflags(-)MLS_POWERMODE;
//			self.drawflags = (self.drawflags & MLS_MASKOUT)| 0;
		}
		else if ((self.tome_time - 10) < time)
			self.artifact_low = self.artifact_low | ART_TOMEOFPOWER;
//	}
	

// invisibility
	if (self.artifact_active & ART_INVISIBILITY)
	{
		if (self.invisible_time < time)
		{	// just stopped
			self.artifact_low = self.artifact_low - (self.artifact_low & ART_INVISIBILITY);
			self.artifact_active = self.artifact_active - (self.artifact_active & ART_INVISIBILITY);
			self.invisible_time = 0;
			msg_entity=self;
			WriteByte(MSG_ONE, SVC_CLEAR_VIEW_FLAGS);
			WriteByte(MSG_ONE,DRF_TRANSLUCENT);
			self.effects(-)EF_NODRAW|EF_LIGHT;
		}
		else if ((self.invisible_time - 10) < time)
			self.artifact_low = self.artifact_low | ART_INVISIBILITY;
	}

	if (self.sheep_time<time+3&&self.model=="models/sheep.mdl")//self.modelindex==modelindex_sheep)
	{
		// sound and screen flash when items starts to run out
		if (!self.sheep_sound_time)
		{
			sprint (self, "Polymorph Spell is wearing off...");
			stuffcmd (self, "bf\n");
//oops!
//			sound (self, CHAN_AUTO, "items/inv2.wav", 1, ATTN_NORM);
			self.sheep_sound_time=TRUE;
		}
			
		if (self.sheep_time < time)
		{
			sound(self,CHAN_VOICE,"misc/sheepfly.wav",1,ATTN_NORM);
			particleexplosion((self.absmin+self.absmax)*0.5,random(144,159),self.absmax_z-self.absmin_z,50);
			GenerateTeleportEffect(self.origin,1);

//			setsize (self, '-16 -16 0', '16 16 56');	
//			self.hull=HULL_PLAYER;
//			self.view_ofs = '0 0 50';
//			self.proj_ofs='0 0 44';

			self.oldweapon = self.weapon = IT_WEAPON1;
			self.attack_finished=self.sheep_time=0;
			restore_weapon();
			SetModelAndThinks();

//			W_SetCurrentAmmo ();

			setsize (self, '-16 -16 0', '16 16 28');	
			self.hull=HULL_CROUCH;
			PlayerSpeed_Calc();

			self.think=player_frames;
			thinktime self : 0;
		}
	}

	if(self.cameramode != world)
	{
		if(self.cameramode.classname=="player")
		{
			msg_entity = self;
			CameraViewPort(self,self.cameramode);
			WriteByte (MSG_ONE, 10);				// 10 = SVC_SETVIEWANGLES
			WriteAngle (MSG_ONE,self.cameramode.v_angle_x);		// pitch
			WriteAngle (MSG_ONE,self.cameramode.v_angle_y);		// yaw
			WriteAngle (MSG_ONE,self.cameramode.v_angle_z);		// roll
			self.weaponmodel=self.cameramode.weaponmodel;
			self.weaponframe=self.cameramode.weaponframe;
		}
		if(deathmatch)
		{
			if(self.velocity!='0 0 0'||self.pain_finished>time||self.button0||self.button2)
				CameraReturn ();
		}
		else if (self.camera_time < time)
			CameraReturn ();
	}
};

/*
================
Player Touch

Mainly used to allow player to climb on top of monsters,
other players, etc.
================
*/
void PlayerTouch (void)
{
	if(other.classname=="monster_eidolon")
		return;

	if(other.dmg==666&&(other.velocity!='0 0 0'||other.avelocity!='0 0 0'))
	{
		self.decap=TRUE;
		T_Damage (self, other, other, self.health+300);
		return;
	}

	if(((vlen(self.velocity)*(self.mass/10)>=100&&self.last_onground+0.3<time)||other.thingtype>=THINGTYPE_WEBS)&&self.last_impact+0.1<=time)
		obj_fly_hurt(other);

	if(other==world)
		return;

	if(coop||deathmatch)
		if(random()<0.5)
			if(other.classname=="player")
				if(self.velocity!='0 0 0')//push other players
					if(normalize(self.velocity)*normalize(other.origin-self.origin)>0.2)
						if(fabs(other.origin_z-self.origin_z)<48)
						{
						float push_mod;
							if(self.flags&FL_ONGROUND)
								push_mod=0.33;
							else
								push_mod=0.77;
							if(other.flags&FL_ONGROUND&&self.velocity_z<0)
							{
								other.velocity_x=(other.velocity_x/push_mod+self.velocity_x*push_mod)*push_mod;
								other.velocity_y=(other.velocity_y/push_mod+self.velocity_y*push_mod)*push_mod;
								other.flags(-)FL_ONGROUND;
							}
							else
								other.velocity=(other.velocity*(1/push_mod)+self.velocity*push_mod)*push_mod;
						}

	if(self.flags&FL_ONGROUND)
		return;

	if((other.classname=="player"||other.flags&FL_ONGROUND||other.health)&&self.origin_z>=(other.absmin_z+other.absmax_z)*0.5&&self.velocity_z<10)
		self.flags(+)FL_ONGROUND;
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void() PlayerPostThink =
{
	if (intermission_running)
		return;

	if (self.deadflag)
		return;
			
// do weapon stuff

	W_WeaponFrame ();

	if(self.viewentity.classname=="chasecam")
		self.weaponmodel="";

// check to see if player landed and play landing sound	
	if ((self.jump_flag*(self.mass/10) < -300) && (self.flags & FL_ONGROUND) && (self.health > 0))
	{
		if(self.absorb_time>=time)
			self.jump_flag/=2;
		if (self.watertype == CONTENT_WATER)
			sound (self, CHAN_BODY, "player/h2ojmp.wav", 1, ATTN_NORM);
		else if (self.jump_flag*(self.mass/10) < -500)//was -650
		{
//			T_Damage (self, world, world, 5); 
			if(self.playerclass==CLASS_ASSASSIN||self.playerclass==CLASS_SUCCUBUS)
				sound (self, CHAN_VOICE, "player/asslnd.wav", 1, ATTN_NORM);
			else
				sound (self, CHAN_VOICE, "player/pallnd.wav", 1, ATTN_NORM);
			self.deathtype = "falling";
		}
		else
			sound (self, CHAN_VOICE, "player/land.wav", 1, ATTN_NORM);
		if(self.scale>1&&self.jump_flag*(self.mass/10) < -500)
			MonsterQuake((self.mass/10)*self.jump_flag);
		self.jump_flag = 0;
	}

	if (!(self.flags & FL_ONGROUND))
	{
		if(self.playerclass==CLASS_SUCCUBUS)
			if(self.flags&FL_SPECIAL_ABILITY1)
				if(self.button2&&self.velocity_z<=0&&!self.waterlevel)
				{
					if(self.gravity==self.standard_grav&&self.standard_grav>0.2)
						sound (self, CHAN_BODY, "succubus/fwoomp.wav", 1, ATTN_NORM);
					self.gravity=0.2;
					self.flags(-)FL_JUMPRELEASED;
				}
				else
					self.gravity=self.standard_grav;
		self.jump_flag = self.velocity_z;
	}
	else
		self.last_onground=time;

	CheckPowerups ();

	if ((self.artifact_flags & AFL_TORCH) && (self.torchtime < time))
		self.torchthink ();

	if ((self.artifact_flags & AFL_SUPERHEALTH) && (self.healthtime < time))
		DecrementSuperHealth ();

	if(self.movechain!=world)
		if(self.movechain.origin!=self.origin)
			setorigin(self.movechain,self.origin);

//	if(self.friction!=1)
//		self.friction=1;
};


/*
===========
ClientConnect

called when a player connects to a server
============
*/
void() ClientConnect =
{
	bprint (self.netname);
	bprint (STR_JOINEDTHEGAME);
	
// a client connecting during an intermission can cause problems
	if (intermission_running)
		ExitIntermission ();
};


/*
===========
ClientDisconnect

called when a player disconnects from a server
============
*/

void() ClientDisconnect =
{
entity lastleader,newking;
	if (gameover)
		return;
	// if the level end trigger has been activated, just return
	// since they aren't *really* leaving

	// let everyone else know
	bprint (self.netname);
	bprint (STR_LEFTTHEGAMEWITH);
	bprint (ftos(self.frags));
	bprint (STR_FRAGS);
	sound (self, CHAN_BODY, "player/leave.wav", 1, ATTN_NONE);
	if(FindExpLeader()==self)
	{
		lastleader=self;
		drop_level(self,self.level);
		self.experience=0;
		newking=FindExpLeader();
		if(newking!=lastleader)
		{//Tell everyone if the king of the hill has changed
			sound (world, CHAN_BODY, "misc/comm.wav", 1, ATTN_NONE);
			bprint(newking.netname);
			bprint(" is the NEW King of the Hill!\n");
			WriteByte(MSG_ALL, SVC_UPDATE_KINGOFHILL);
			WriteEntity (MSG_ALL, newking);
		}
	}
	GibPlayer();
	set_suicide_frame ();
};

/*
===========
ClientObituary

called when a player dies
============
*/
void(entity targ, entity attacker, entity inflictor) ClientObituary =
{
float rnum,tclass,aclass,reversed,powered_up;
string deathstring, deathstring2,iclass;

//	if(inflictor.frags==2)
//		dprint("Inflictor was deflected\n");

	if (targ.classname != "player")
		return;

	tclass=targ.playerclass;
	aclass=attacker.playerclass;
	iclass=inflictor.classname;
	powered_up=inflictor.frags;
	exp_mult=1;
	rnum = random();

	if (targ.deathtype == "teledeath")
	{
		bprint (targ.netname);
		bprint (STR_WASTELEFRAGGEDBY);
		if(attacker.flags&FL_CLIENT)
		{
			bprint (attacker.netname);
			attacker.frags += 1;
		}
		else
			bprint (attacker.classname);
		bprint ("\n");

		return;
	}

	if (targ.deathtype == "teledeath2")
	{
		bprint ("The power of invincibility reflects ");
		bprint (targ.netname);
		bprint ("'s telefrag\n");

		targ.frags -= 1;
		return;
	}

	if (targ.deathtype == "teledeath3")
	{
		bprint (attacker.netname);
		bprint (" telefragged ");
		bprint (targ.netname);
		bprint (", his own teammate!\n");

		attacker.frags -= 1;
		return;
	}

	if (targ.deathtype == "teledeath4")
	{
		bprint (attacker.netname);
		bprint ("'s invincibility met ");
		bprint (targ.netname);
		bprint ("'s invincibility and mutual annihilation resulted!\n");

		targ.frags -= 1;
		return;
	}

	// Was killed by a player
	if (attacker.classname == "player")
	{
		if (targ == attacker)
		{
			// killed self
			attacker.frags -= 1;
			bprint (targ.netname);
			
			if(random()<0.5)
				bprint (" must be a masochist!\n");
			else
				bprint (" becomes bored with life...\n");
			return;
		}
		else if ( (teamplay == 2) && (targ.team > 0)&&(targ.team == attacker.team) )
		{
			if (rnum < 0.25)
				deathstring = " mows down a teammate\n";
			else if (rnum < 0.50)
				deathstring = " checks their glasses\n";
			else if (rnum < 0.75)
				deathstring = " gets a frag for the other team\n";
			else
				deathstring = " loses another friend\n";
			bprint (attacker.netname);
			bprint (deathstring);
			attacker.frags -= 1;
			return;
		}
		else
		{
			attacker.frags += 1;

			rnum = attacker.weapon;
			if(attacker.model=="models/sheep.mdl")
			{
				deathstring = " was nibbled to death by ";
				deathstring2 = "the sheep!!\n";
			}
			else if(targ.decap==1)
			{
				if(tclass==CLASS_ASSASSIN||tclass==CLASS_SUCCUBUS)
					deathstring = " lost her head over ";
				else
					deathstring = " lost his head over ";
				deathstring2 = "!\n";
			}
			else if (targ.decap==2)
			{
				if (tclass==CLASS_ASSASSIN||tclass==CLASS_SUCCUBUS)
				{
					deathstring = " got her head blown clean off by ";
				}
				else
				{
					deathstring = " got his head blown clean off by ";
				}
				deathstring2 = "!\n";
			}
			else if (iclass=="cube_of_force")
			{
				deathstring = " was ventilated by ";
				deathstring2 = "'s Force Cube!\n";
			}
			else if(iclass=="tripwire")
			{
				deathstring = " tripped on ";
				deathstring2 = "'s tripwire glyph!\n";
			}
			else if(iclass=="fireballblast")
			{
				deathstring = " was blown away by ";
				deathstring2 = "'s delayed fireball glyph!\n";
			}
			else if(iclass=="proximity")
			{
				deathstring = " got too close for comfort to ";
				deathstring2 = "'s proximity glyph!\n";
			}
			else if(iclass=="timebomb")
			{
				deathstring = " was in the wrong place at the wrong time thanks to ";
				deathstring2 = "'s timebomb glyph!\n";
			}
			else if(iclass=="poison grenade")
			{
				deathstring = " choked on ";
				deathstring2 = "'s gas!\n";
			}
			else if(iclass=="tornato")
			{
				deathstring = " isn't in kansas anymore thanks to ";
				deathstring2 = "'s tornado!\n";
			}
			else if(iclass=="blizzard")
			{
				deathstring = " was snowed in by ";
				deathstring2 = "'s blizzard!\n";
			}
			else if(targ.deathtype=="hammercrush")
			{
				deathstring = " was crushed by the righteous might of ";
				deathstring2 = "'s Hammer!\n";
			}
			else if (iclass == "monster_imp_lord")
			{
				deathstring =" was jacked up by ";
				deathstring2 ="'s Summoned Imp Lord!\n";
			}
			else if(inflictor.frags==2&&iclass!="player")
			{
				deathstring = " was destroyed by  ";
				deathstring2 = "'s deflected shot!\n";
			}
			else 
			switch (rnum)
			{
			case IT_WEAPON1:
				if(attacker.artifact_active&ART_TOMEOFPOWER)
					exp_mult=1.5;
				else
					exp_mult=2;
				
				switch (aclass)
				{
				case CLASS_ASSASSIN:
					deathstring = " got penetrated by ";
					deathstring2 = "'s Katar\n";
					break;
				case CLASS_CRUSADER:
					if(exp_mult==1.5)
					{
						deathstring = " was fried by the holy lightning of ";
						deathstring2 = "'s Mjolnir!\n";
					}
					else
					{
						deathstring = " was whalloped by ";
						deathstring2 = "'s hammer!\n";
					}
					break;
				case CLASS_PALADIN:
					deathstring = " got KO'd by ";
					deathstring2 = "'s fists of fury!\n";
					break;
				case CLASS_SUCCUBUS:
					deathstring = " got burned by ";
					deathstring2 = "'s BloodFire\n";
					break;
				default:
					deathstring = " was sliced and diced by ";
					deathstring2 = "'s sickle!\n";
					break;
				}
				break;

			case IT_WEAPON2:
				if(powered_up)
					exp_mult=1;
				else
					exp_mult=1.2;

				switch (aclass)
				{
				case CLASS_ASSASSIN:
					if(powered_up)
					{
						deathstring = " was stuck like a pig by ";
						deathstring2 = "'s arrows!\n";
					}
					else
					{
						deathstring = " took one of ";
						deathstring2 = "'s arrows to the heart!\n";
					}
					break;
				case CLASS_CRUSADER:
					if(powered_up)
					{
						deathstring = " befell the subzero temperatures of ";
						deathstring2 = "'s blizzard!\n";
					}
					else
					{
						deathstring = " gets the cold shoulder from ";
						deathstring2 = "!\n";
					}
					break;
				case CLASS_PALADIN:
					if(powered_up)
					{
						deathstring = " took a shock to the system from ";
						deathstring2 = "'s Vorpal Shockwave!\n";
					}
					else
					{
						deathstring = " was cut to pieces by ";
						deathstring2 = "'s vorpal sword!\n";
					}
					break;
				case CLASS_SUCCUBUS:
					if(powered_up)
					{
						deathstring = " was reduced to a pile of bubbling flesh by ";
						deathstring2 = "'s Acid Cloud!\n";
					}
					else
					{
						deathstring = " was vaporized by ";
						deathstring2 = "'s Acid Rune!\n";
					}
					break;
				default:
					if(powered_up)
					{
						deathstring = " was tracked down by ";
					}
					else
					{
						deathstring = " was mowed down by ";
					}
					deathstring2 = "'s Magic Missiles!\n";
					break;
				}
			break;
			case IT_WEAPON3:
				if(powered_up)
					exp_mult=0.8;
				else
					exp_mult=1;
				switch (aclass)
				{
				case CLASS_ASSASSIN:
					if(powered_up)
					{
						reversed=TRUE;
						deathstring = " opened up a nice big can o' whoop-ass on ";
						deathstring2 = "!\n";
					}
					else
					{
						deathstring = " sucked down ";
						deathstring2 = "'s grenade!\n";
					}
					break;
				case CLASS_CRUSADER:	
					if(powered_up)
					{
						deathstring = " was whisked away by ";
						deathstring2 = "'s tornado!\n";
					}
					else
					{
						deathstring = " took a nice hot meteor shower courtesy of ";
						deathstring2 = "!\n";
					}
					break;
				case CLASS_PALADIN:
					if(powered_up)
					{
						deathstring = " was cut down by ";
						deathstring2 = "'s magic axeblades!\n";
					}
					else
					{
						deathstring = " got a nasty papercut from ";
						deathstring2 = "'s axeblade!\n";
					}
					break;
				case CLASS_SUCCUBUS:
					if(powered_up)
					{
						deathstring = " was incinerated by ";
						deathstring2 = "'s FireStorm!\n";
					}
					else
					{
						reversed=TRUE;
						deathstring = " made ";
						deathstring2 = " burst into flames!\n";
					}
					break;
				default:
					if(powered_up)
					{
						deathstring = " was fragged by ";
						deathstring2 = "'s Frag Bones!\n";
					}
					else
					{
						reversed=TRUE;
						deathstring = " broke  ";
						deathstring2 = "'s bones with the bone shard spell!\n";
					}
					break;
				}
			break;
			case IT_WEAPON4:
				if(powered_up)
					exp_mult=0.5;
				else
					exp_mult=0.8;
				switch (aclass)
				{
				case CLASS_ASSASSIN:
					if(powered_up)
					{
						deathstring = " got into a little S&M with ";
						deathstring2 = "'s chains!\n";
					}
					else
					{
						deathstring = " got cored by ";
						deathstring2 = "'s Scarab Staff!\n";
					}
					break;
				case CLASS_CRUSADER:
					if(attacker.artifact_active&ART_TOMEOFPOWER)
					{
						exp_mult=0.5;
						deathstring = " needs some SPF 5,000,000 to stop ";
						deathstring2 = "'s Sunstaff!\n";
					}
					else
					{
						deathstring = " smells like fried chicken thanks to ";
						deathstring2 = "'s Sunstaff!\n";
					}
					break;
				case CLASS_PALADIN:
					if(powered_up)
					{
						deathstring = " was blown into next week by ";
						deathstring2 = "'s Purifier Seeker!\n";
					}
					else
					{
						deathstring = "'s evil ways were purified by ";
						deathstring2 = "!\n";
					}
					break;
				case CLASS_SUCCUBUS:
					if(powered_up)
					{
						reversed = TRUE;
						deathstring = " lit up ";
						deathstring2 = "'s life!\n";
					}
					else
					{
						deathstring = " got a charge out of ";
						deathstring2 = "'s balls... of lightning!\n";
					}
					break;
				default:
					if(powered_up)
					{
						deathstring = " succumbed to the black death of ";
						deathstring2 = "'s Ravens!\n";
					}
					else
					{
						deathstring = " befell the black magic of ";
						deathstring2 = "'s Ravenstaff!\n";
					}
					break;
				}
			break;
			}
			
			if(reversed)
			{
				bprint (attacker.netname);
				bprint (deathstring);
				bprint (targ.netname);
				bprint (deathstring2);
			}
			else
			{
				bprint (targ.netname);
				bprint (deathstring);
				bprint (attacker.netname);
				bprint (deathstring2);
			}
		}
		return;
	}

	// was not killed by a player
	else
	{
		targ.frags -= 1;
		bprint (targ.netname);

		if (attacker.flags & FL_MONSTER)
		{
			if(attacker.model=="models/sheep.mdl")
				if(random()<0.5)
					bprint (" was savagely mauled by a sheep!\n");
				else
					bprint (" says 'HELLO DOLLY!'\n");
			if (attacker.netname=="monster_archer_ice")
				bprint (" got the cold shoulder from the Tulku Archer!\n");
			else if (attacker.classname == "monster_archer")
				bprint (" was skewered by an Archer!\n");
			if (attacker.classname == "monster_archer_lord")
				bprint (" got Horshacked!\n");
			if (attacker.classname == "monster_fallen_angel")
				bprint (" was felled by the Fallen Angel\n");
			if (attacker.classname == "monster_fallen_angel_lord")
				bprint (" was decimated by a Fallen Angel Lord!\n");
			if (attacker.classname == "monster_golem_bronze")
				if(targ.decap==1)
					bprint ("'s head was taken as a trophy for the Bronze Golem!\n");
				else if(targ.decap==2)
					bprint (" became a permanent stain on the wall!\n");
				else
					bprint (" was squished like an insect by a Bronze Golem!\n");
			if (attacker.classname == "monster_golem_iron")
			{
				if (inflictor.classname == "golem_iron_proj")
					bprint(" felt the sting of the Iron Golem's jewel!\n");
				else if(targ.decap==2)
					bprint ("'s brains make nice wall decorations!\n");
				else
					bprint (" was crushed by the Iron Golem's fist!\n");
			}
			if (attacker.classname == "monster_golem_stone")
				if(targ.decap==2)
					bprint (" is feeling a little light-headed!\n");
				else
					bprint (" was pummeled by a Stone Golem!\n");
			if (attacker.classname == "monster_golem_crystal")
				bprint (" was mangled by the Enchanted Crystal Golem!\n");
			if (attacker.classname == "monster_hydra")
				bprint (" becomes food for the Hydra!\n");
			if (attacker.classname == "monster_imp_fire")
				bprint (" was roasted by a Fire Imp!\n");
			if (attacker.classname == "monster_imp_ice")
				bprint (" chills out with the Ice Imps!\n");
			if (attacker.classname == "monster_medusa")
				if (attacker.skin==1)
					bprint (" was stricken by the beauty of the Crimson Medusa!\n");
				else
					bprint (" is helpless in the face of the Medusa's beauty!\n");
			if (attacker.classname == "monster_mezzoman")
				if (attacker.strength>=3)
					bprint (" was squished like a bug by Siberian WereTiger!\n");
				else if (attacker.strength==2)
					bprint (" was cut down to size by the Snow WereLeopard!\n");
				else if (attacker.strength==1)
					bprint (" is not yet worthy of facing the WerePanther!\n");
				else
					bprint (" is no match for the WereJaguar!\n");
			if (attacker.classname == "monster_mummy")
				bprint (" got mummified!\n");
			if (attacker.classname == "monster_mummy_lord")
				bprint (" was escorted to the Underworld by a Mummy Lord!\n");
			if (attacker.classname == "monster_scorpion_black")
				bprint (" submits to the sting of the Black Scorpion!\n");
			if (attacker.classname == "monster_scorpion_yellow")
				bprint (" was poisoned by the fatal Golden Scorpion!\n");
			if (attacker.classname == "monster_skull_wizard")
				bprint (" succumbed to the Skull Wizard's magic!\n");
			if (attacker.classname == "monster_skull_wizard_lord")
				bprint (" was Skull-duggeried!\n");
			if (attacker.classname == "monster_snake")
				bprint (" was bitten by the lethal Cobra!\n");
			if (attacker.classname == "monster_spider_red_large")
				bprint (" was overcome by the Crimson Spiders!\n");
			if (attacker.classname == "monster_spider_red_small")
				bprint (" was eaten alive by the spiders!\n");
			if (attacker.classname == "monster_spider_yellow_large")
				bprint (" was overwhelmed by the Golden Spiders!\n");
			if (attacker.classname == "monster_spider_yellow_small")
				bprint (" is a meal for the spiders!\n");
			if (attacker.classname == "monster_yakman")
			{
				if(targ.frozen>0)
					bprint (" was shattered by the Yakman!\n");
				else if(inflictor.classname=="snowball")//fixme: you can't pummel someone with a snowball
					bprint (" was taken down by the Yakman!\n");
				else
					bprint (" got gored by the Yakman!\n");
			}
			if (attacker.classname == "monster_pentacles")
				bprint (" was humbled by the pentacles!\n");
			if (attacker.classname == "monster_buddha")
			{
				if (inflictor==attacker)
					bprint (" was not mighty enough to defeat Praevus!\n");
				else if (inflictor.classname=="buddha_firewalker")
					bprint (" was incinerated by Praevus' wall of fire!\n");
				else if (inflictor.classname=="buddha_firepillar")
					bprint (" couldn't take the heat of Praevus' pillars of fire!\n");
				else if (inflictor.classname=="buddha_missile")
					bprint (" was tracked down by Praevus' spinning stars of death!\n");
			}
			if (attacker.classname == "rider_famine")
				bprint(" was drained of life-force by Famine!\n");
			if (attacker.classname == "rider_death")
				if(inflictor==attacker)
					bprint(" was snuffed out of existance by Death!\n");
				else if(inflictor.netname=="deathbone")
					bprint(" had his bones crushed to a fine powder by Death!\n");
				else if(iclass=="deathmissile")
					bprint(" was shot down by Death's crimson bolts!\n");
				else
					bprint(" was smitten by Death's unholy fire\n");
			if (attacker.classname == "rider_pestilence")
				if(targ.deathtype=="poison")
					bprint(" was poisoned to death by Pestilence's Crossbow!\n");
				else
					bprint("'s rotted corpse is the possession of Pestilence!\n");
			if (attacker.classname == "rider_war")
				bprint(" was taught the true meaning of War!\n");
			if (attacker.classname == "monster_eidolon")
				if(inflictor==attacker)
					bprint(" was squashed like an insect by Eidolon!\n");
				else if(inflictor.classname=="eidolon fireball")
					bprint(" was obliterated by Eidolon's fireballs!\n");
				else if(inflictor.classname=="eidolon spell")
					bprint(" was introduced to a new level of pain by Eidolon's Magic!\n");
				else if(inflictor.classname=="eidolon flames")
					bprint(" was roasted to a crisp by Eidolon's Hellfire!\n");
			return;
		}

		// tricks and traps
		if(targ.decap==1)
		{
			if(targ.playerclass==CLASS_ASSASSIN||targ.playerclass==CLASS_SUCCUBUS)
				bprint(" should have quit while she was a head... oh, she IS a head!\n");
			else
				bprint(" should have quit while he was a head... oh, he IS a head!\n");
			return;
		}
		if(targ.decap==2)
		{
			if(targ.playerclass==CLASS_ASSASSIN||targ.playerclass==CLASS_SUCCUBUS)
				bprint(" got her head blown off!\n");
			else
				bprint(" got his head blown off!\n");
			return;
		}
		if(attacker.classname=="light_thunderstorm")
		{
			if(mapname=="eidolon")
				bprint(" was smited by Eidolon's unholy lightning!\n");
			else
				bprint(" shouldn't mess with Mother Nature!\n");
			return;
		}
		if(targ.deathtype=="zap")
		{
			bprint(" was electrocuted!\n");
			return;
		}
		if(targ.deathtype=="chopped")
		{
			bprint(" was sliced AND diced!\n");
			return;
		}
		if (attacker.solid == SOLID_BSP && attacker != world)
		{	
			bprint (" was squished\n");
			return;
		}
		if (attacker.classname == "trap_shooter" || attacker.classname == "trap_spikeshooter")
		{
			bprint (" was spiked");
			if (attacker.enemy.classname == "player" && attacker.enemy != targ)
			{
			 bprint(" by ");
			 bprint(attacker.enemy.netname);
			 attacker.enemy.frags += 1;
			}
			bprint("\n");
			return;
		}
		if (attacker.classname == "fireball")
		{
			bprint (" ate a lavaball\n");
			return;
		}
		if (attacker.classname == "trigger_changelevel")
		{
			bprint (" tried to leave\n");
			return;
		}

		// in-water deaths
		rnum = targ.watertype;
		if (rnum == -3)
		{
			if (random() < 0.5)
				bprint (" takes a nice, deep breath of H2O!\n");
			else
				bprint (" needed gills\n");
			return;
		}
		else if (rnum == -4)
		{
			if (random() < 0.5)
				bprint (" gulped a load of slime\n");
			else
				bprint (" can't exist on slime alone\n");
			return;
		}
		else if (rnum == -5)
		{
			if (random() < 0.3)
				bprint (" needs a cold shower\n");
			else if (random() < 0.5)
				bprint (" likes it HOT!\n");
			else
				bprint (" smells like burnt hair\n");
			return;
		}

		// fell to their death?
		if (targ.deathtype == "falling")
		{
			targ.deathtype = "";
			bprint (STR_CHUNKYSALSA);
			return;
		}

		// hell if I know; he's just dead!!!
		bprint (STR_CEASEDTOFUNCTION);
	}
};
/*
 * $Log: /H2 Mission Pack/HCode/Client.hc $
 * 
 * 72    3/23/98 1:08p Mgummelt
 * 
 * 71    3/23/98 1:06p Mgummelt
 * 
 * 70    3/20/98 7:42p Mgummelt
 * 
 * 69    3/19/98 12:52p Jmonroe
 * fixed select spot
 * 
 * 68    3/19/98 12:17a Mgummelt
 * last bug fixes
 * 
 * 67    3/17/98 4:40p Mgummelt
 * 
 * 66    3/17/98 4:15p Jmonroe
 * 
 * 65    3/17/98 4:08p Jmonroe
 * 
 * 64    3/17/98 4:06p Jmonroe
 * 
 * 63    3/16/98 8:31p Mgummelt
 * 
 * 62    3/16/98 8:26p Jweier
 * 
 * 61    3/16/98 6:21p Jweier
 * 
 * 60    3/16/98 6:38a Mgummelt
 * 
 * 59    3/16/98 2:19a Mgummelt
 * 
 * 58    3/14/98 9:24p Mgummelt
 * 
 * 57    3/14/98 6:37p Mgummelt
 * 
 * 56    3/14/98 5:13p Mgummelt
 * 
 * 55    3/13/98 7:44p Mgummelt
 * 
 * 54    3/13/98 6:31p Mgummelt
 * 
 * 53    3/13/98 4:34p Mgummelt
 * 
 * 52    3/13/98 1:51p Mgummelt
 * Fixed friction_change entity to work,  made checkbottom use the hull
 * mins/maxs for it's checks, not the bounding box's.
 * 
 * 51    3/13/98 3:02a Mgummelt
 * 
 * 50    3/11/98 6:20p Mgummelt
 * 
 * 49    3/10/98 12:21a Mgummelt
 * 
 * 48    3/09/98 11:24p Mgummelt
 * 
 * 47    3/09/98 11:09p Jweier
 * 
 * 46    3/09/98 7:06p Mgummelt
 * 
 * 45    3/06/98 4:55p Mgummelt
 * 
 * 44    3/04/98 4:34p Mgummelt
 * 
 * 43    3/04/98 4:24p Mgummelt
 * 
 * 42    3/04/98 3:39p Mgummelt
 * 
 * 41    3/03/98 10:44p Jmonroe
 * the air_finished time is not restored after a dead player reloads, so
 * this keeps it from going negative.
 * 
 * 40    3/03/98 7:31p Mgummelt
 * 
 * 39    3/02/98 7:57p Mgummelt
 * 
 * 38    3/02/98 5:41p Mgummelt
 * 
 * 37    3/02/98 1:19a Jmonroe
 * added dm map cycling for keep and tibet.
 * reduced code size by changing to switch
 * 
 * 36    3/01/98 3:30p Mgummelt
 * 
 * 35    3/01/98 3:12p Mgummelt
 * 
 * 34    2/28/98 6:46p Mgummelt
 * 
 * 33    2/27/98 11:52a Mgummelt
 * 
 * 32    2/26/98 12:29p Mgummelt
 * 
 * 31    2/25/98 9:00p Mgummelt
 * 
 * 30    2/25/98 1:49p Jmonroe
 * first pass at demoness' obit msgs
 * 
 * 29    2/24/98 6:39p Mgummelt
 * 
 * 28    2/23/98 3:13p Mgummelt
 * 
 * 27    2/20/98 6:46p Mgummelt
 * 
 * 26    2/16/98 11:37a Mgummelt
 * 
 * 25    2/16/98 10:54a Mgummelt
 * 
 * 24    2/13/98 2:49p Mgummelt
 * 
 * 23    2/13/98 11:25a Mgummelt
 * Fixing air_finished time when changing levels
 * 
 * 22    2/13/98 11:16a Jmonroe
 * changed succubus to demoness, made her water time the same as others
 * 
 * 21    2/12/98 5:55p Jmonroe
 * remove unreferenced funcs
 * 
 * 20    2/08/98 4:28p Mgummelt
 * 
 * 19    2/08/98 3:25p Mgummelt
 * Fixing player start selection
 * 
 * 18    2/06/98 9:59p Mgummelt
 * Implemented Succubus' special abilities.
 * 
 * 17    2/06/98 11:54a Jmonroe
 * removed useless code
 * 
 * 16    2/05/98 11:21p Mgummelt
 * Making weaps network friendly
 * 
 * 15    2/05/98 8:00p Jmonroe
 * fast sqrt is partially in, need to do tests to verify it
 * 
 * 14    1/31/98 10:23p Mgummelt
 * 
 * 13    1/22/98 5:52p Mgummelt
 * 
 * 12    1/19/98 6:20p Mgummelt
 * 
 * 11    1/15/98 12:02p Jmonroe
 * added snowflake texture
 * 
 * 292   10/29/97 6:38p Mgummelt
 * 
 * 291   10/28/97 1:46p Mgummelt
 * 
 * 290   10/28/97 1:34p Mgummelt
 * 
 * 289   10/28/97 1:00p Mgummelt
 * Massive replacement, rewrote entire code... just kidding.  Added
 * support for 5th class.
 * 
 * 286   10/23/97 11:34a Mgummelt
 * 
 * 285   9/30/97 11:38a Rlove
 * 
 * 284   9/30/97 9:59a Rjohnson
 * OEM Update
 * 
 * 283   9/23/97 4:48p Mgummelt
 * 
 * 282   9/19/97 2:10p Rlove
 * 
 * 281   9/19/97 2:07p Rlove
 * 
 * 280   9/19/97 2:06p Rlove
 * 
 * 279   9/15/97 3:43p Rlove
 * 
 * 278   9/11/97 8:25p Mgummelt
 * 
 * 277   9/11/97 8:13p Mgummelt
 * 
 * 276   9/11/97 7:13p Rjohnson
 * Caching Updates
 * 
 * 275   9/11/97 3:41p Mgummelt
 * 
 * 274   9/11/97 2:09p Mgummelt
 * 
 * 273   9/11/97 12:02p Mgummelt
 * 
 * 272   9/10/97 11:39p Mgummelt
 * 
 * 271   9/10/97 7:51p Mgummelt
 * 
 * 270   9/07/97 9:42a Mgummelt
 * 
 * 269   9/05/97 1:54p Rlove
 * 
 * 268   9/04/97 6:27p Mgummelt
 * 
 * 267   9/04/97 1:51p Rlove
 * 
 * 266   9/03/97 9:14p Mgummelt
 * Fixing targetting AI
 * 
 * 265   9/03/97 8:07p Mgummelt
 * 
 * 264   9/03/97 8:06p Rjohnson
 * Map Fix
 * 
 * 263   9/03/97 2:51a Mgummelt
 * 
 * 262   9/02/97 9:28p Mgummelt
 * 
 * 261   9/02/97 8:03p Mgummelt
 * 
 * 260   9/02/97 7:02p Mgummelt
 * 
 * 259   9/02/97 1:38p Rjohnson
 * Default coop points
 * 
 * 258   9/01/97 9:32p Rlove
 * 
 * 257   9/01/97 3:27p Mgummelt
 * 
 * 256   9/01/97 5:13a Mgummelt
 * 
 * 255   9/01/97 1:34a Mgummelt
 * 
 * 254   8/31/97 8:50p Rjohnson
 * DM Update
 * 
 * 253   8/31/97 12:12p Jweier
 * 
 * 252   8/31/97 11:39a Mgummelt
 * 
 * 251   8/31/97 11:38a Mgummelt
 * To which I say- shove where the sun don't shine- sideways!  Yeah!
 * How's THAT for paper cut!!!!
 * 
 * 250   8/31/97 8:52a Mgummelt
 * 
 * 249   8/31/97 12:07a Rjohnson
 * Freeze all entities in the world
 * 
 * 248   8/31/97 12:01a Mgummelt
 * 
 * 247   8/30/97 6:58p Mgummelt
 * 
 * 246   8/29/97 11:22p Mgummelt
 * 
 * 245   8/29/97 8:26p Mgummelt
 * 
 * 244   8/29/97 4:17p Mgummelt
 * Long night
 * 
 * 243   8/29/97 2:30a Mgummelt
 * 
 * 242   8/29/97 12:59a Mgummelt
 * 
 * 241   8/28/97 10:08p Mgummelt
 * 
 * 240   8/28/97 2:42p Mgummelt
 * 
 * 239   8/28/97 12:44a Mgummelt
 * 
 * 238   8/28/97 12:06a Mgummelt
 * 
 * 237   8/27/97 10:52p Mgummelt
 * 
 * 236   8/27/97 10:39p Rjohnson
 * Removed debuggings
 * 
 * 235   8/27/97 9:21p Mgummelt
 * 
 * 234   8/27/97 7:59p Mgummelt
 * 
 * 233   8/27/97 7:07p Mgummelt
 * 
 * 232   8/27/97 1:15a Rjohnson
 * Fix for deathmatch
 * 
 * 231   8/26/97 10:08p Mgummelt
 * 
 * 230   8/26/97 8:38p Mgummelt
 * 
 * 229   8/26/97 8:31p Mgummelt
 * 
 * 228   8/26/97 6:00p Mgummelt
 * 
 * 227   8/26/97 5:05p Mgummelt
 * 
 * 226   8/26/97 4:21p Rjohnson
 * Fixed players spawning with no class
 * 
 * 225   8/26/97 4:18p Rlove
 * 
 * 224   8/26/97 3:12p Rjohnson
 * Coop spawn spot
 * 
 * 223   8/26/97 12:02p Rlove
 * 
 * 222   8/26/97 8:15a Mgummelt
 * 
 * 221   8/26/97 7:38a Mgummelt
 * 
 * 219   8/25/97 8:14p Mgummelt
 * 
 * 218   8/25/97 7:27p Rlove
 * 
 * 217   8/25/97 2:30p Mgummelt
 * 
 * 216   8/25/97 1:38p Mgummelt
 * 
 * 215   8/25/97 9:16a Rjohnson
 * Worked on preserving stuff for singleplayer
 * 
 * 214   8/25/97 1:09a Mgummelt
 * 
 * 213   8/25/97 1:08a Mgummelt
 * 
 * 212   8/24/97 8:32p Mgummelt
 * 
 * 211   8/24/97 4:35p Rlove
 * Working on flight
 * 
 * 210   8/23/97 8:24p Mgummelt
 * 
 * 209   8/21/97 5:04p Mgummelt
 * 
 * 208   8/21/97 1:53p Mgummelt
 * 
 * 207   8/21/97 4:55a Mgummelt
 * 
 * 206   8/21/97 4:29a Mgummelt
 * 
 * 205   8/21/97 3:33a Mgummelt
 * 
 * 204   8/21/97 12:37a Mgummelt
 * 
 * 203   8/20/97 11:56p Mgummelt
 * 
 * 202   8/20/97 8:51p Mgummelt
 * 
 * 201   8/20/97 8:36p Mgummelt
 * 
 * 200   8/20/97 8:06p Rlove
 * 
 * 199   8/20/97 6:32p Rjohnson
 * co-op stuff
 * 
 * 198   8/20/97 4:22p Rlove
 * 
 * 197   8/20/97 4:06p Rlove
 * 
 * 196   8/20/97 3:29p Rlove
 * 
 * 195   8/20/97 7:59a Rlove
 * 
 * 194   8/19/97 11:59p Rjohnson
 * Updates
 * 
 * 193   8/19/97 8:14p Mgummelt
 * 
 * 192   8/19/97 7:15p Rjohnson
 * End of demo update
 * 
 * 191   8/19/97 10:04a Rjohnson
 * Removed camera stuff
 * 
 * 190   8/18/97 3:00p Rjohnson
 * Fix for camera mode
 * 
 * 189   8/17/97 3:14p Rjohnson
 * Removed tinting
 * 
 * 187   8/17/97 1:03p Rjohnson
 * Fix for going to next level
 * 
 * 186   8/17/97 3:12a Mgummelt
 * 
 * 185   8/15/97 4:59p Mgummelt
 * 
 * 184   8/15/97 4:26p Rjohnson
 * Update
 * 
 * 183   8/15/97 3:23p Rlove
 * 
 * 182   8/15/97 2:53p Rlove
 * 
 * 181   8/15/97 2:51p Rlove
 * 
 * 180   8/15/97 2:50p Rjohnson
 * Fix for precache
 * 
 * 179   8/15/97 11:36a Rlove
 * 
 * 178   8/14/97 7:59p Mgummelt
 * 
 * 177   8/14/97 4:54p Mgummelt
 * 
 * 176   8/14/97 3:40p Rlove
 * 
 * 175   8/14/97 11:38a Rjohnson
 * 
 * 174   8/12/97 6:10p Mgummelt
 * 
 * 173   8/11/97 2:52p Rlove
 * 
 * 172   8/11/97 11:30a Mgummelt
 * 
 * 171   8/09/97 1:49a Mgummelt
 * 
 * 170   8/08/97 6:21p Mgummelt
 * 
 * 169   8/08/97 3:33p Mgummelt
 * 
 * 168   8/07/97 10:30p Mgummelt
 * 
 * 167   8/06/97 10:18p Mgummelt
 * 
 * 166   8/05/97 6:47p Mgummelt
 * 
 * 165   8/04/97 4:45p Rjohnson
 * Initial work for between hubs
 * 
 * 164   8/01/97 4:38p Rlove
 * 
 * 163   7/30/97 10:42p Mgummelt
 * 
 * 162   7/30/97 3:32p Mgummelt
 * 
 * 161   7/29/97 9:51p Mgummelt
 * 
 * 160   7/28/97 7:50p Mgummelt
 * 
 * 159   7/28/97 1:51p Mgummelt
 * 
 * 158   7/26/97 8:38a Mgummelt
 * 
 * 157   7/25/97 6:34p Rlove
 * 
 * 156   7/25/97 11:26a Mgummelt
 * 
 * 155   7/25/97 11:20a Mgummelt
 * 
 * 154   7/25/97 9:50a Rlove
 * 
 * 153   7/24/97 5:22p Rlove
 * 
 * 152   7/24/97 12:36p Mgummelt
 * 
 * 151   7/24/97 12:35p Mgummelt
 * 
 * 150   7/24/97 12:33p Mgummelt
 * 
 * 149   7/24/97 3:26a Mgummelt
 * 
 * 148   7/21/97 6:42p Rlove
 * 
 * 147   7/21/97 3:03p Rlove
 * 
 * 146   7/21/97 10:50a Mgummelt
 * 
 * 145   7/19/97 10:12p Mgummelt
 * 
 * 140   7/18/97 11:06a Mgummelt
 * 
 * 139   7/17/97 6:53p Mgummelt
 * 
 * 138   7/17/97 4:54p Rlove
 * 
 * 137   7/17/97 4:11p Mgummelt
 * 
 * 136   7/17/97 2:32p Mgummelt
 * 
 * 135   7/17/97 2:17p Mgummelt
 * 
 * 134   7/17/97 11:44a Mgummelt
 * 
 * 133   7/16/97 3:55p Rjohnson
 * Fix for plaques
 * 
 * 132   7/16/97 12:44p Mgummelt
 * 
 * 131   7/15/97 8:03p Mgummelt
 * 
 * 130   7/15/97 3:19p Mgummelt
 * 
 * 129   7/15/97 2:31p Mgummelt
 * 
 * 128   7/15/97 11:29a Mgummelt
 * 
 * 127   7/15/97 11:28a Mgummelt
 * 
 * 126   7/15/97 9:49a Rlove
 * 
 * 125   7/14/97 2:11p Mgummelt
 * 
 * 124   7/11/97 12:34p Rjohnson
 * Added a puzzle precache, changed puzzle precaching, added a parameter
 * to the call back for client reentering a level
 * 
 * 123   7/08/97 6:31p Rjohnson
 * Fix for going back to a level
 * 
 * 122   7/08/97 12:19p Rjohnson
 * Fix for going back to a level
 * 
 * 121   7/07/97 11:12a Mgummelt
 * 
 * 120   7/03/97 4:46p Mgummelt
 * 
 * 119   7/03/97 4:07p Rjohnson
 * Flag was mis-set
 * 
 * 118   7/03/97 1:59p Rlove
 * 
 * 117   7/01/97 5:17p Rjohnson
 * Removed water splashes
 * 
 * 116   7/01/97 1:39p Rlove
 * 
 * 115   7/01/97 9:49a Rlove
 * 
 * 114   7/01/97 9:46a Rlove
 * Crusader soul sphere is in. It does double damage.
 * 
 * 113   6/30/97 3:23p Mgummelt
 * 
 * 112   6/27/97 5:36p Mgummelt
 * 
 * 111   6/25/97 8:35p Rjohnson
 * Made the plaque network friendly
 * 
 * 110   6/25/97 9:48a Rlove
 * 
 * 109   6/25/97 8:28a Rlove
 * Added ring of turning 
 * 
 * 108   6/24/97 5:44p Rlove
 * Rings of Flight and Regeneration are working
 * 
 * 107   6/24/97 3:54p Rlove
 * New ring system
 * 
 * 106   6/19/97 2:10p Jweier
 * 
 * 105   6/19/97 1:59p Jweier
 * 
 * 104   6/19/97 12:12p Jweier
 * 
 * 103   6/18/97 6:07p Mgummelt
 * 
 * 102   6/18/97 4:00p Mgummelt
 * 
 * 101   6/18/97 1:51p Mgummelt
 * 
 * 100   6/14/97 5:51p Mgummelt
 * 
 * 99    6/13/97 6:36p Mgummelt
 * 
 * 98    6/13/97 6:08p Rlove
 * 
 * 97    6/12/97 12:43p Jweier
 * 
 * 96    6/11/97 6:48p Jweier
 * Added water entry bubbles
 * 
 * 95    6/09/97 11:20a Rlove
 * 
 * 94    6/09/97 7:27a Rlove
 * Added water splash to player.
 * 
 * 93    6/07/97 6:50p Bgokey
 * 
 * 92    6/07/97 3:35p Rlove
 * Added water splash animation.  It ain't done yet.
 * 
 * 91    6/06/97 2:52p Rlove
 * Artifact of Super Health now functions properly
 * 
 * 90    6/05/97 4:44p Rlove
 * Fly mode is network friendly now.
 * 
 * 89    6/05/97 9:29a Rlove
 * Weapons now have deselect animations
 * 
 * 88    6/03/97 10:48p Mgummelt
 * 
 * 87    6/02/97 7:58p Mgummelt
 * 
 * 86    6/01/97 7:32a Mgummelt
 * 
 * 85    6/01/97 5:09a Mgummelt
 * 
 * 84    5/31/97 9:28p Mgummelt
 * 
 * 83    5/31/97 12:18a Mgummelt
 * 
 * 82    5/28/97 1:43p Rlove
 * Plaques now activate when you bump into them.
 * 
 * 81    5/27/97 10:57a Rlove
 * Took out old Id sound files
 * 
 * 80    5/27/97 9:40a Rlove
 * Took out super_damage and radsuit fields
 * 
 * 79    5/24/97 3:31p Mgummelt
 * 
 * 78    5/24/97 2:48p Rlove
 * Taking out old Id sounds
 * 
 * 77    5/23/97 11:51p Mgummelt
 * 
 * 76    5/23/97 3:57p Rlove
 * 
 * 75    5/23/97 1:29p Rlove
 * 
 * 74    5/22/97 6:30p Mgummelt
 * 
 * 73    5/22/97 12:00p Rjohnson
 * Added a new vector that allows you to adjust the player's velocity for
 * circumstances that you wouldn't normally be able to adjust it
 * 
 * 72    5/22/97 2:50a Mgummelt
 * 
 * 71    5/20/97 9:32p Mgummelt
 * 
 * 70    5/19/97 11:36p Mgummelt
 * 
 * 69    5/17/97 8:45p Mgummelt
 * 
 * 68    5/15/97 6:34p Rjohnson
 * Code cleanup
 * 
 * 67    5/15/97 11:43a Rjohnson
 * Stats updates
 * 
 * 66    5/14/97 3:36p Rjohnson
 * Inital stats implementation
 * 
 * 65    5/12/97 11:11p Mgummelt
 * 
 * 64    5/11/97 8:54p Mgummelt
 * 
 * 63    5/11/97 7:30a Mgummelt
 * 
 * 62    5/08/97 5:47p Mgummelt
 * 
 * 61    5/07/97 3:40p Mgummelt
 * 
 * 60    5/06/97 1:29p Mgummelt
 * 
 * 59    5/05/97 10:09p Mgummelt
 * 
 * 58    5/05/97 4:48p Mgummelt
 * 
 * 57    5/01/97 8:52p Mgummelt
 * 
 * 56    4/30/97 5:03p Mgummelt
 * 
 * 55    4/26/97 6:23p Mgummelt
 * 
 * 54    4/26/97 3:52p Mgummelt
 * 
 * 52    4/24/97 8:48p Mgummelt
 * 
 * 51    4/24/97 2:15p Mgummelt
 * 
 * 50    4/23/97 7:02a Rlove
 * Player now starts with Weapon 1
 * 
 * 49    4/18/97 8:22p Mgummelt
 * 
 * 48    4/18/97 5:24p Mgummelt
 * 
 * 47    4/18/97 3:22p Rjohnson
 * Removed the intermission option from the changelevel trigger - assumes
 * no intermission from now on
 * 
 * 46    4/18/97 2:37p Rjohnson
 * Fixed start spot problem
 * 
 * 45    4/18/97 1:06p Mgummelt
 * 
 * 44    4/17/97 9:12p Mgummelt
 * 
 * 43    4/17/97 1:44p Mgummelt
 * 
 * 42    4/16/97 7:59a Rlove
 * Removed references to ammo_  fields
 * 
 * 41    4/15/97 11:51a Rjohnson
 * Modifications from quake2 for multi-level trigger stuff
 * 
 * 40    4/15/97 10:14a Rlove
 * Changed cleric to crusader
 * 
 * 39    4/14/96 3:46p Mgummelt
 * 
 * 38    4/14/97 10:36a Rlove
 * 
 * 37    4/13/96 3:30p Mgummelt
 * 
 * 36    4/12/96 8:55p Mgummelt
 * 
 * 35    4/11/96 1:50p Mgummelt
 * 
 * 34    4/11/96 1:03p Mgummelt
 * 
 * 33    4/11/97 12:38a Mgummelt
 * 
 * 32    4/11/97 12:32a Mgummelt
 * 
 * 31    4/10/96 3:29p Mgummelt
 * 
 * 30    4/10/96 2:49p Mgummelt
 * 
 * 29    4/10/97 11:36a Mgummelt
 * 
 * 28    4/09/97 3:43p Rjohnson
 * Added code for multi-level triggers
 * 
 * 27    4/07/97 8:49a Rlove
 * Changed timing on weapons twitch
 * 
 * 26    4/04/97 5:40p Rlove
 * 
 * 25    4/01/97 11:25a Rjohnson
 * Added a builtin to set the player class
 * 
 * 24    3/25/97 11:28a Rlove
 * New camera entity
 * 
 * 23    3/21/97 9:38a Rlove
 * Created CHUNK.HC and MATH.HC, moved brush_die to chunk_death so others
 * can use it.
 * 
 * 22    3/20/97 4:01p Rlove
 * Added mummy, medusa for Brian R.
 * 
 * 21    3/18/97 7:37a Rlove
 * Added tome of power
 * 
 * 20    3/15/97 3:08p Rlove
 * Added COMA console command
 * 
 * 19    3/14/97 9:21a Rlove
 * Plaques are done 
 * 
 * 18    3/13/97 9:57a Rlove
 * Changed constant DAMAGE_AIM  to DAMAGE_YES and the old DAMAGE_YES to
 * DAMAGE_NO_GRENADE
 * 
 * 17    3/05/97 4:56p Jweier
 * spikeshooter awards frags in DM
 * 
 * 16    2/27/97 2:55p Rlove
 * Changed thief to assassin class
 * 
 * 15    2/19/97 10:06a Rlove
 * New Pull Object and Plaque Code
 * 
 * 14    2/13/97 4:22p Rlove
 * 
 * 13    2/12/97 3:59p Rlove
 * Invincibility is done, changed a few things with ring of water
 * breathing and the lava death
 * 
 * 12    2/12/97 10:17a Rlove
 * 
 * 11    2/06/97 3:15p Rjohnson
 * Added PlayerAdvanceLevel() function, which is called from the game-c
 * side.
 * 
 * 10    2/06/97 2:56p Rlove
 * For invulnerability
 * 
 * 9     2/04/97 4:10p Rlove
 * Rick leave me alone
 * 
 * 8     2/04/97 3:37p Rlove
 * Rewrote super health, made the code tighter
 * 
 * 7     2/04/97 3:05p Rlove
 * Rewrote the torch, doesn't use an entity anymore (what was I thinking?)
 * 
 * 6     1/13/97 3:32p Rlove
 * Haste has been added, currently doubles speed but amount can be set in
 * entity field 'hasted'
 * 
 * 5     12/30/96 8:30a Rlove
 * Push objects added
 * 
 * 4     12/26/96 10:02a Rlove
 * Ring of Water Breathing is working, 
 * 
 * 3     11/11/96 1:12p Rlove
 * Added Source Safe stuff
 */
