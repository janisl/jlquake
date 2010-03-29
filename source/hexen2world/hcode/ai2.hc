/*
 * $Header: /HexenWorld/HCode/ai2.hc 1     2/04/98 1:59p Rjohnson $
 */
void(entity etemp, entity stemp, entity stemp, float dmg) T_Damage;
/*

.enemy
Will be world if not currently angry at anyone.

.pathcorner
The next path spot to walk toward.  If .enemy, ignore .pathcorner
When an enemy is killed, the monster will try to return to it's path.

.hunt_time
Set to time + something when the player is in sight, but movement straight for
him is blocked.  This causes the monster to use wall following code for
movement direction instead of sighting on the player.

.ideal_yaw
A yaw angle of the intended direction, which will be turned towards at up
to 45 deg / state.  If the enemy is in view and hunt_time is not active,
this will be the exact line towards the enemy.

.pausetime
A monster will leave it's stand state and head towards it's .pathcorner when
time > .pausetime.

walkmove(angle, speed) primitive is all or nothing
*/

void()SetNextWaypoint;
float(entity targ , entity from)infront_of_ent;
float()eidolon_check_attack;
void()multiplayer_health;

//void()check_climb;

//
// globals
//
float	current_yaw;

//
// when a monster becomes angry at a player, that monster will be used
// as the sight target the next frame so that monsters near that one
// will wake up even if they wouldn't have noticed the player
//
float	sight_entity_time;
void()riderpath_init;
void(float move_speed)riderpath_move;
float(float move_speed)eidolon_riderpath_move;
void() eidolon_guarding;
void()hive_die;
float(float v) anglemod =
{
	while (v >= 360)
		v = v - 360;
	while (v < 0)
		v = v + 360;
	return v;
};

//============================================================================

/*
=============
range

returns the range catagorization of an entity reletive to self
0	melee range, will become hostile even if back is turned
1	visibility and infront, or visibility and show hostile
2	infront and show hostile
3	only triggered by damage
=============
*/
float(entity targ) range =
{
	local vector	spot1, spot2;
	local float		r,melee;	

	if((self.solid==SOLID_BSP||self.solid==SOLID_TRIGGER)&&self.origin=='0 0 0')
		spot1=(self.absmax+self.absmin)*0.5;
	else
		spot1 = self.origin + self.view_ofs;

	if((targ.solid==SOLID_BSP||targ.solid==SOLID_TRIGGER)&&targ.origin=='0 0 0')
		spot2=(targ.absmax+targ.absmin)*0.5;
	else
		spot2 = targ.origin + targ.view_ofs;
	
	r = vlen (spot1 - spot2);

	if (self.classname=="monster_mummy")
		melee = 50;
	else
		melee = 100;

	if (r < melee)
		return RANGE_MELEE;
	if (r < 500)
		return RANGE_NEAR;
	if (r < 1000)
		return RANGE_MID;
	return RANGE_FAR;
};

/*
=============
visible2ent

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
float visible2ent (entity targ, entity forent)
{
vector	spot1, spot2;
	if((forent.solid==SOLID_BSP||forent.solid==SOLID_TRIGGER)&&forent.origin=='0 0 0')
		spot1=(forent.absmax+forent.absmin)*0.5;
	else
		spot1 = forent.origin + forent.view_ofs;
		
	if((targ.solid==SOLID_BSP||targ.solid==SOLID_TRIGGER)&&targ.origin=='0 0 0')
		spot2=(targ.absmax+targ.absmin)*0.5;
	else
		spot2 = targ.origin + targ.view_ofs;

    traceline (spot1, spot2, TRUE, forent);   // see through other monsters

	if(trace_ent.thingtype>=THINGTYPE_WEBS)
		traceline (trace_endpos, spot2, TRUE, trace_ent);
	else if (trace_inopen && trace_inwater)
		return FALSE;			// sight line crossed contents

	if (trace_fraction == 1)
		return TRUE;

	return FALSE;
}

/*
=============
infront_of_ent

returns 1 if the targ is in front (in sight) of from
=============
*/
float infront_of_ent (entity targ , entity from)
{
	vector	vec,spot1,spot2;
	float	accept,dot;

	if(from.classname=="player")
	    makevectors (from.v_angle);
/*	else if(from.classname=="monster_medusa")
		makevectors (from.angles+from.angle_ofs);
*/	else
	    makevectors (from.angles);

	if((from.solid==SOLID_BSP||from.solid==SOLID_TRIGGER)&&from.origin=='0 0 0')
		spot1=(from.absmax+from.absmin)*0.5;
	else
		spot1 = from.origin + from.view_ofs;

	spot2=(targ.absmax+targ.absmin)*0.5;

    vec = normalize (spot2 - spot1);
	dot = vec * v_forward;

    accept = 0.3;
	
    if ( dot > accept)
		return TRUE;
	return FALSE;
}

/*
=============
visible

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
float visible (entity targ)
{
	return visible2ent(targ,self);
}

/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
float infront (entity targ)
{
	return infront_of_ent(targ,self);
}


//============================================================================

/*
===========
ChangeYaw

Turns towards self.ideal_yaw at self.yaw_speed
Sets the global variable current_yaw
Called every 0.1 sec by monsters
============
*/
/*

void ChangeYaw ()
{
float		ideal, move;

//current_yaw = self.ideal_yaw;
// mod down the current angle
	current_yaw = anglemod( self.angles_y );
	ideal = self.ideal_yaw;


	if (current_yaw == ideal)
		return;
	
	move = ideal - current_yaw;
	if (ideal > current_yaw)
	{
		if (move > 180)
			move = move - 360;
	}
	else
	{
		if (move < -180)
			move = move + 360;
	}
		
	if (move > 0)
	{
		if (move > self.yaw_speed)
			move = self.yaw_speed;
	}
	else
	{
		if (move < 0-self.yaw_speed )
			move = 0-self.yaw_speed;
	}

	current_yaw = anglemod (current_yaw + move);

	self.angles_y = current_yaw;
}

*/


//============================================================================

void() HuntTarget =
{
	self.goalentity = self.enemy;
	if(self.spawnflags&PLAY_DEAD)
	{
//		dprint("getting up!!!\n");
		self.think=self.th_possum_up;
		self.spawnflags(-)PLAY_DEAD;
	}
	else
		self.think = self.th_run;
//	self.ideal_yaw = vectoyaw(self.enemy.origin - self.origin);
	self.ideal_yaw = vectoyaw(self.goalentity.origin - self.origin);
	thinktime self : 0.1;
//	SUB_AttackFinished (1);	// wait a while before first attack
};

void SightSound (void)
{
	if (self.classname == "monster_archer")
		sound (self, CHAN_VOICE, "archer/sight.wav", 1, ATTN_NORM);
	else if (self.classname == "monster_archer_lord")
		sound (self, CHAN_VOICE, "archer/sight2.wav", 1, ATTN_NORM);
	else if (self.classname == "monster_mummy")
		sound (self, CHAN_WEAPON, "mummy/sight.wav", 1, ATTN_NORM);
	else if (self.classname == "monster_mummy_lord")
		sound (self, CHAN_WEAPON, "mummy/sight2.wav", 1, ATTN_NORM);

}

void() FoundTarget =
{
	if (self.enemy.classname == "player")
	{	// let other monsters see this monster for a while
		sight_entity = self;
		sight_entity_time = time + 1;
	}
	
	self.show_hostile = time + 1;		// wake up other monsters

	SightSound ();

	HuntTarget ();
};

/*
===========
FindTarget

Self is currently not attacking anything, so try to find a target

Returns TRUE if an enemy was sighted

When a player fires a missile, the point of impact becomes a fakeplayer so
that monsters that see the impact will respond as if they had seen the
player.

To avoid spending too much time, only a single client (or fakeclient) is
checked each frame.  This means multi player games will have slightly
slower noticing monsters.
============
*/
float(float dont_hunt) FindTarget =
{
entity	client;
float		r;

// if the first spawnflag bit is set, the monster will only wake up on
// really seeing the player, not another monster getting angry

// spawnflags & 3 is a big hack, because zombie crucified used the first
// spawn flag prior to the ambush flag, and I forgot about it, so the second
// spawn flag works as well
	if(!deathmatch&&(self.classname=="monster_imp_lord"||self.classname=="cube_of_force"))
		return FindMonsterTarget();

	if (sight_entity_time >= time)
	{
		client = sight_entity;
		if (client.enemy == self.enemy)
			return TRUE;
	}
	else
	{
		client = checkclient ();
		if (!client)
			return FALSE;	// current check entity isn't in PVS
	}

	if (client == self.enemy)
		return FALSE;

	if (client.flags & FL_NOTARGET)
		return FALSE;

	r = range (client);
	if (r == RANGE_FAR)
		return FALSE;

	if (r == RANGE_NEAR)
	{
		if (client.show_hostile < time && !infront (client))
			return FALSE;
	}
	else if (r == RANGE_MID)
	{
		if (!infront (client))
			return FALSE;
	}
	
	if (!visible (client))
		return FALSE;

//
// got one
//
	self.enemy = client;

	if (self.enemy.classname != "player")
	{
		self.enemy = self.enemy.enemy;
		if (self.enemy.classname != "player")
		{
			self.enemy = world;
			return FALSE;
		}
	}

/*	if(self.spawnflags&PLAY_DEAD)
	{
		if(r==RANGE_MELEE)
		{
			if(!dont_hunt)
				FoundTarget ();
			return TRUE;
		}
		else if(!infront_of_ent(self,self.enemy)&&random()<0.1&&random()<0.1)
		{
			if(!dont_hunt)
				FoundTarget ();
			return TRUE;
		}
		else
		{
			self.enemy=world;
			return FALSE;
		}
	}
*/
	if(!dont_hunt)
		FoundTarget ();
	return TRUE;
};

//=============================================================================

void(float dist) ai_forward =
{
	walkmove (self.angles_y, dist, FALSE);
};

void(float dist) ai_back =
{
	walkmove ( (self.angles_y+180), dist, FALSE);
};


/*
=============
ai_pain

stagger back a bit
=============
*/
void(float dist) ai_pain =
{
//	ai_back (dist);
float	away;
	
	away = vectoyaw (self.origin - self.enemy.origin)+90*random(0.5,-0.5);
	
	walkmove (away, dist,FALSE);
};

/*
=============
ai_painforward

stagger back a bit
=============
*/
void(float dist) ai_painforward =
{
	walkmove (self.ideal_yaw, dist, FALSE);
};

/*
=============
ai_walk

The monster is walking it's beat
=============
*/
void(float dist) ai_walk =
{
	
	MonsterCheckContents();

	movedist = dist;
	
	// check for noticing a player
	if (FindTarget (FALSE))
		return;

	if(self.classname=="monster_eidolon")
	{
		if (!self.path_current)
			riderpath_init();
		if(!eidolon_riderpath_move(dist))
		{
			if(self.think==self.th_walk)
				self.think=eidolon_guarding;
		}
		else if(self.think==eidolon_guarding)
			self.think=self.th_walk;
	}
	else
		movetogoal (dist);
};


/*
=============
ai_stand

The monster is staying in one place for a while, with slight angle turns
=============
*/
void() ai_stand =
{
	MonsterCheckContents();
	
	if (FindTarget (FALSE))
		return;
	
	if(self.spawnflags&PLAY_DEAD)
		return;

	if (time > self.pausetime)
	{
		self.th_walk ();
		return;
	}

// change angle slightly
};

/*
=============
ai_turn

don't move, but turn towards ideal_yaw
=============
*/
void() ai_turn =
{
	if (FindTarget (FALSE))
		return;
	
	ChangeYaw ();
};

//=============================================================================

/*
=============
ChooseTurn
=============
*/
void(vector dest3) ChooseTurn =
{
	local vector	dir, newdir;
	
	dir = self.origin - dest3;

	newdir_x = trace_plane_normal_y;
	newdir_y = 0 - trace_plane_normal_x;
	newdir_z = 0;
	
	if (dir * newdir > 0)
	{
		dir_x = 0 - trace_plane_normal_y;
		dir_y = trace_plane_normal_x;
	}
	else
	{
		dir_x = trace_plane_normal_y;
		dir_y = 0 - trace_plane_normal_x;
	}

	dir_z = 0;
	self.ideal_yaw = vectoyaw(dir);	
};

/*
============
FacingIdeal

Within angle to launch attack?
============
*/
float() FacingIdeal =
{
	local	float	delta;
	
	delta = anglemod(self.angles_y - self.ideal_yaw);
	if (delta > 45 && delta < 315)
		return FALSE;
	return TRUE;
};


//=============================================================================

float() CheckAnyAttack =
{
	if (!enemy_vis)
		return FALSE;

	if(self.classname=="monster_eidolon")
		if(self.goalentity==self.controller)
			return FALSE;
		else
			return eidolon_check_attack();

	return CheckAttack ();
};


/*
=============
ai_attack_face

Turn in place until within an angle to launch an attack
=============
*/
void() ai_attack_face =
{
	self.ideal_yaw = enemy_yaw;
	ChangeYaw ();
	if (FacingIdeal())  // Ready to go get em
	{
		if (self.attack_state == AS_MISSILE)
			self.th_missile ();
		else if (self.attack_state == AS_MELEE)
			self.th_melee ();
		self.attack_state = AS_STRAIGHT;
	}
};


/*
=============
ai_run_slide

Strafe sideways, but stay at aproximately the same range
=============
*/
void ai_run_slide ()
{
float	ofs;
	
	self.ideal_yaw = enemy_yaw;
	ChangeYaw ();
	if (self.lefty)
		ofs = 90;
	else
		ofs = -90;
	
	if (walkmove (self.ideal_yaw + ofs, movedist, FALSE))
		return;
		
	self.lefty = 1 - self.lefty;
	
	walkmove (self.ideal_yaw - ofs, movedist, FALSE);
}


/*
=============
ai_run

The monster has an enemy it is trying to kill
=============
*/
void(float dist) ai_run =
{
	
	MonsterCheckContents();
	
	movedist = dist;
// see if the enemy is dead
	if (!self.enemy.flags2&FL_ALIVE||(self.enemy.artifact_active&ARTFLAG_STONED&&self.classname!="monster_medusa"))
	{
		self.enemy = world;
	// FIXME: look all around for other targets
		if (self.oldenemy.health > 0)
		{
			self.enemy = self.oldenemy;
			HuntTarget ();
		}
		else if(coop)
		{
			if(!FindTarget(TRUE))	//Look for other enemies in the area
			{
				if (self.pathentity)
					self.th_walk ();
				else
					self.th_stand ();
				return;
			}
		}
		else
		{
			if (self.pathentity)
				self.th_walk ();
			else
				self.th_stand ();
			return;
		}
	}

	self.show_hostile = time + 1;		// wake up other monsters

// check knowledge of enemy
	enemy_vis = visible(self.enemy);
	if (enemy_vis)
	{
		self.search_time = time + 5;
		if(self.mintel)
		{
			self.goalentity=self.enemy;
		    self.wallspot=(self.enemy.absmin+self.enemy.absmax)*0.5;
		}
	}
	else
	{
		if(coop)
		{
			if(!FindTarget(TRUE))
				if(self.model=="models/spider.mdl")
				{
					if(random()<0.5)
						SetNextWaypoint();
				}
				else 
					SetNextWaypoint();
		}
		if(self.mintel)
			if(self.model=="models/spider.mdl")
			{
				if(random()<0.5)
					SetNextWaypoint();
			}
			else 
				SetNextWaypoint();
	}

	if(random()<0.5&&(!self.flags&FL_SWIM)&&(!self.flags&FL_FLY)&&(self.spawnflags&JUMP))
		CheckJump();

// look for other coop players
	if (coop && self.search_time < time)
	{
		if (FindTarget (FALSE))
			return;
	}

	enemy_infront = infront(self.enemy);
	enemy_range = range(self.enemy);
	if(self.classname!="monster_eidolon")
		enemy_yaw = vectoyaw(self.goalentity.origin - self.origin);
	
	if ((self.attack_state == AS_MISSILE) || (self.attack_state == AS_MELEE))  // turning to attack
	{
		if(self.classname!="monster_eidolon")
			ai_attack_face ();
		return;
	}

	if (CheckAnyAttack ())
		return;					// beginning an attack
		
	if (self.attack_state == AS_SLIDING)
	{
		ai_run_slide ();
		return;
	}
		
// head straight in
//	if(self.netname=="spider")
//		check_climb();
	if(self.classname=="monster_eidolon")
	{
		if(!self.path_current)
			riderpath_init();
		if(!eidolon_riderpath_move(dist))
		{
			if(self.think==self.th_run)
				eidolon_guarding();
		}
		else if(self.think==eidolon_guarding)
			self.th_run();
	}
	else
		movetogoal (dist);		// done in C code...
};

//FAKE FUNCTIONS
void SpiderJumpBegin()
{
}

float mezzo_choose_roll (entity null)
{
	return FALSE;
}

/*
 * $Log: /HexenWorld/HCode/ai2.hc $
 * 
 * 1     2/04/98 1:59p Rjohnson
 * 
 * 18    9/04/97 3:00p Mgummelt
 * 
 * 17    9/03/97 9:14p Mgummelt
 * Fixing targetting AI
 * 
 * 16    9/03/97 12:26a Mgummelt
 * 
 * 15    9/01/97 12:07a Mgummelt
 * 
 * 14    8/31/97 8:40p Rlove
 * 
 * 13    8/31/97 6:55p Mgummelt
 * 
 * 12    8/31/97 4:21p Mgummelt
 * 
 * 11    8/28/97 5:41p Mgummelt
 * 
 * 10    8/26/97 9:00a Mgummelt
 * 
 * 9     8/26/97 8:29a Mgummelt
 * 
 * 91    8/21/97 1:53p Mgummelt
 * 
 * 90    8/21/97 3:33a Mgummelt
 * 
 * 89    8/19/97 3:26p Mgummelt
 * 
 * 88    8/18/97 12:20p Mgummelt
 * 
 * 87    8/15/97 11:27p Mgummelt
 * 
 * 86    8/15/97 8:11p Mgummelt
 * 
 * 85    8/15/97 2:55a Mgummelt
 * 
 * 84    8/14/97 8:30p Mgummelt
 * 
 * 83    8/14/97 7:12p Mgummelt
 * 
 * 82    8/14/97 5:17p Mgummelt
 * 
 * 81    8/14/97 1:13p Mgummelt
 * 
 * 80    8/13/97 11:53p Mgummelt
 * 
 * 79    8/13/97 5:35p Mgummelt
 * 
 * 78    8/13/97 1:47a Mgummelt
 * 
 * 77    8/13/97 1:28a Mgummelt
 * 
 * 76    8/12/97 6:10p Mgummelt
 * 
 * 75    8/11/97 6:08p Mgummelt
 * 
 * 74    8/09/97 5:27a Mgummelt
 * 
 * 73    8/09/97 2:03a Mgummelt
 * 
 * 72    8/09/97 1:49a Mgummelt
 * 
 * 71    8/06/97 10:09p Mgummelt
 * 
 * 70    8/06/97 2:27p Mgummelt
 * 
 * 69    8/06/97 11:05a Mgummelt
 * 
 * 68    8/04/97 8:07p Mgummelt
 * 
 * 67    8/04/97 8:03p Mgummelt
 * 
 * 66    8/04/97 3:48p Mgummelt
 * 
 * 65    8/04/97 3:47p Mgummelt
 * 
 * 64    7/28/97 12:31p Rlove
 * 
 * 63    7/21/97 4:03p Mgummelt
 * 
 * 62    7/21/97 4:02p Mgummelt
 * 
 * 61    7/21/97 3:03p Rlove
 * 
 * 60    7/18/97 2:06p Rlove
 * 
 * 59    7/07/97 2:51p Mgummelt
 * 
 * 58    7/03/97 5:58p Mgummelt
 * 
 * 57    7/03/97 8:47a Rlove
 * 
 * 56    6/30/97 3:23p Mgummelt
 * 
 * 55    6/28/97 6:32p Mgummelt
 * 
 * 54    6/25/97 9:23p Mgummelt
 * 
 * 53    6/25/97 3:01p Mgummelt
 * 
 * 52    6/18/97 8:14p Mgummelt
 * 
 * 51    6/18/97 5:42p Mgummelt
 * 
 * 50    6/18/97 5:40p Mgummelt
 * 
 * 48    6/18/97 4:00p Mgummelt
 * 
 * 47    6/18/97 2:42p Mgummelt
 * 
 * 46    6/17/97 8:16p Mgummelt
 * 
 * 45    6/16/97 9:04p Mgummelt
 * 
 * 44    6/16/97 7:35p Mgummelt
 * 
 * 43    6/16/97 7:00p Mgummelt
 * 
 * 42    6/14/97 2:22p Mgummelt
 * 
 * 41    6/11/97 10:14a Rlove
 * Added sight sounds
 * 
 * 40    6/10/97 9:27p Mgummelt
 * 
 * 39    6/10/97 12:09a Mgummelt
 * 
 * 38    6/09/97 10:21p Mgummelt
 * 
 * 37    6/09/97 3:07p Mgummelt
 * 
 * 36    6/06/97 9:17p Mgummelt
 * 
 * 35    6/05/97 8:16p Mgummelt
 * 
 * 34    6/02/97 7:58p Mgummelt
 * 
 * 33    5/30/97 10:03p Mgummelt
 * 
 * 32    5/23/97 4:19p Mgummelt
 * 
 * 31    5/23/97 3:43p Mgummelt
 * 
 * 30    5/23/97 2:55p Mgummelt
 * 
 * 28    5/22/97 7:29p Mgummelt
 * 
 * 27    5/22/97 6:30p Mgummelt
 * 
 * 26    5/22/97 2:50a Mgummelt
 * 
 * 25    5/20/97 9:35p Mgummelt
 * 
 * 24    5/19/97 11:36p Mgummelt
 * 
 * 23    5/15/97 8:28p Mgummelt
 * 
 * 22    5/12/97 10:31a Rlove
 * 
 * 21    5/07/97 11:12a Rjohnson
 * Added a new field to walkmove and movestep to allow for setting the
 * traceline info
 * 
 * 20    5/07/97 11:03a Rlove
 * 
 * 17    4/24/97 2:14p Mgummelt
 * 
 * 16    4/24/97 9:15a Rlove
 * Pulling out old Id sounds
 * 
 * 15    4/22/97 8:20a Rlove
 * Mummy AI
 * 
 * 14    4/14/97 6:54a Bgokey
 * 
 * 13    3/18/97 1:44p Aleggett
 * 
 * 11    3/12/97 4:35p Rlove
 * New monster AI
 * 
 * 10    3/10/97 8:29a Rlove
 * Halfway through rewriting Monster AI
 * 
 * 9     3/07/97 10:57a Rlove
 * 
 * 8     2/26/97 3:14p Rlove
 * Changes to basic monster ai
 * 
 * 7     1/15/97 12:02p Rjohnson
 * Removed all of quake's monsters
 * 
 * 6     1/02/97 11:19a Rjohnson
 * Christmas changes
 * 
 * 5     11/11/96 1:12p Rlove
 * Added Source Safe stuff
 * 
 * 4     11/11/96 11:14a Rlove
 * another test
 */


