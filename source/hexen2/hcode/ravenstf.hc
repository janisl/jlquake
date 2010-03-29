/*
 * $Header: /H2 Mission Pack/HCode/ravenstf.hc 5     3/19/98 12:17a Mgummelt $
 */

// For building the model
$cd Q:\art\models\weapons\newass
$origin 0 0 0
$base BASE SKIN
$skin SKIN
$flags 0

//
$frame rootpose     

//
$frame fire1        fire2        fire3        fire4        

//
$frame select1      select2      select3      select4      select5      
$frame select6      select7      select8      select9      select10     
$frame select11     select12     

void ravenstaff_fire (void);
void ravenstaff_idle (void);
void split (void);
void raven_track(void);
void raven_flap(void);
void raven_touch (void);
void raven_track_init(void);
void ravenmissile_explode(void);	

void raven_spark (void)
{
	if (pointcontents(self.origin) == CONTENT_SKY)
	{
		remove(self);
		return;
	}

	CreateWhiteSmoke(self.origin + '0 0 -10', '0 8 -10', HX_FRAME_TIME *3);
	CreateRedSmoke(self.origin + '0 0 -10', '0 0 -10', HX_FRAME_TIME *3);
	CreateWhiteSmoke(self.origin + '0 0 -10', '0 -8 -10', HX_FRAME_TIME *3);
	sound(self,CHAN_WEAPON,"raven/death.wav",1,ATTN_NORM);

	self.touch = SUB_Null;
	self.effects=EF_NODRAW;
	self.think=SUB_Remove;
	thinktime self : HX_FRAME_TIME;
//	thinktime self : HX_FRAME_TIME * 2;

}

void raven_death_init (void)
{
	self.controller.raven_cnt-=1;
	self.takedamage = DAMAGE_NO;

	traceline(self.origin,self.origin + '0 0 600',FALSE,self);
	if (trace_fraction < 1)
	{
		self.touch = raven_spark;
		self.nextthink = 0;
	}
	else
	{
		self.touch = raven_spark;
		self.think = raven_spark;
		thinktime self : 1;				
	}

	self.velocity = normalize('0 0 600');
	self.velocity = self.velocity * 400;	
	self.angles = vectoangles(self.velocity);
}

void raven_bounce(void)
{
	self.flags (-) FL_ONGROUND;

	self.angles = vectoangles(self.velocity);	// Flip it around to match the velocity set by the BOUNCEMISSLE code
	self.angles_y += random(-90,90);			// Change it's yaw a little
	self.angles_x = random(-20,20);			// Change it's pitch a little 

	makevectors (self.angles);
	self.velocity = normalize (v_forward);
	self.velocity = self.velocity * self.speed;

	self.think = raven_flap;
	self.nextthink = time + HX_FRAME_TIME;

	self.think1 = raven_track_init;
	self.next_action = time + HX_FRAME_TIME * random(1,3);

	self.touch = raven_touch;
}

// Bite the enemy
void raven_touch (void)
{
	if ((other == self.enemy) && (other.takedamage != DAMAGE_NO))
	{
		if (other.monsterclass >= CLASS_BOSS)	// Bosses only take half damage
			T_Damage(other,self,self.owner,10);
		else
			T_Damage(other,self,self.owner,20);

		self.damage_max += 20;
		SpawnPuff (self.origin, '0 0 -5', random(5,10),other);
		MeatChunks (self.origin,self.velocity*0.5+'0 0 20', 2,other);
		sound(self,CHAN_WEAPON,"weapons/gauntht1.wav",1,ATTN_NORM);
	}

	if (self.damage_max > 200)
		raven_death_init();
	else
	{
		self.touch = SUB_Null;
		self.think = raven_bounce;
		self.nextthink = time + .05;  // Need to wait a little before flipping model to match velocity
	}

	if ((self.lifetime < time) || (self.controller.raven_cnt > 6))
	{
		raven_death_init();
		return;
	}
}

//  Search for an enemy
void raven_search(void)
{
	entity victim;

	self.nextthink = time + HX_FRAME_TIME;	//	Gotta flap
	self.think = raven_flap;

	victim = findradius( self.origin,1000);
	while(victim)
	{	// the controller check is for the summoned imp
		if (((victim.flags & FL_MONSTER) || (victim.flags & FL_CLIENT)) && 
			(victim.owner != self) && (victim.controller != self.owner) && (victim.health>0) && (victim!=self.owner))
		{
			if (coop && self.enemy.team == self.team) 
				victim = victim;		// Do nothing if its a player on your team.
			else
			{
				traceline(self.origin,(victim.absmin+victim.absmax)*0.5,TRUE,self);
				if (trace_fraction == 1.0)  
				{
					self.enemy = victim;
					self.think1 = raven_track;
					self.think1 = raven_track_init;
					self.next_action = time + .1;
					self.searchtime = 0;
					return;
				}
			}
		}
		victim = victim.chain;
	}

	self.think1 = raven_search;
	self.next_action = time + HX_FRAME_TIME * 3;

	if (self.searchtime == 0)  // Done only on birth of raven
	{
		self.searchtime = time + .5;

		self.angles_y = random(0, 360);
		self.angles_x = 0;

		makevectors (self.angles);
		self.velocity = normalize (v_forward);
		self.velocity = self.velocity * self.speed;
		
	}

	if ((self.searchtime < time) || (self.lifetime < time) || (self.controller.raven_cnt > 6))
		raven_death_init();
}



//
// Chase after the enemy
//
void raven_track (void)
{
	vector delta;
	vector hold_spot;

//	dprint("\n  trk:");
//	dprint(self.enemy.classname);

	// The FL_MONSTER flag gets flipped when it becomes a head
	if ((self.enemy.health <= 0) || (self.enemy == world) || (!self.enemy.flags & FL_MONSTER))
		raven_search();
	else
	{
		traceline(self.origin,self.enemy.origin,TRUE,self);
		if (trace_fraction == 1)
		{
			hold_spot = self.enemy.origin;
			hold_spot_z += self.enemy.maxs_z;  // Hit 'em on the head
			delta = hold_spot - self.origin;

			self.velocity = normalize(delta);
			self.velocity = self.velocity * self.speed;
			self.angles = vectoangles(self.velocity);

			self.think1 = raven_track;
			self.next_action = time + HX_FRAME_TIME * 3;

			self.think = raven_flap;
			self.nextthink = time + HX_FRAME_TIME;
		}
		else
			raven_search();
	}

	if ((self.lifetime < time) || (self.controller.raven_cnt > 6))
	{
		raven_death_init();
		return;
	}
}

void raven_track_init (void)
{
	vector delta;
	vector hold_spot;

	if ((self.enemy.health <= 0) || (self.enemy == world))
		raven_search();
	else
	{
		hold_spot = self.enemy.origin;
		hold_spot_z += self.enemy.maxs_z;
		delta = hold_spot - self.origin;
		self.velocity = normalize(delta);
		self.angles = vectoangles(self.velocity);

		self.idealpitch = self.angles_x;

		makevectors(self.angles);
		self.velocity = normalize(v_forward);
		self.velocity = self.velocity * self.speed;
		self.pitchdowntime = time + HX_FRAME_TIME *3;

		self.think = raven_track;
		self.nextthink = time;
	}
}

// Everything comes back to here
void raven_flap(void)
{
	AdvanceFrame(0,7);  // Flapping wings

	if ((self.frame == 1) && (random() < .2))
	{
		sound(self,CHAN_VOICE,"raven/squawk2.wav",1,ATTN_NORM);
	}

	if (self.next_action < time)
	{
		self.think = self.think1;	
		self.nextthink = time;
	}
	else
	{
//		ChangeYaw(); 
		self.think = raven_flap;
		self.nextthink = time + HX_FRAME_TIME;
	}

	if ((self.lifetime < time) || (self.controller.raven_cnt > 6))
	{
		raven_death_init();
		return;
	}
}


/*--------------------
Create one raven
----------------------*/
void create_raven(void)
{
	entity missile;
	vector spot1, spot2;

	missile = spawn ();
	missile.frags=TRUE;
	missile.owner = missile.controller = self.owner;

	missile.controller.raven_cnt += 1;

	missile.movetype = MOVETYPE_BOUNCEMISSILE;
	missile.solid = SOLID_BBOX;
	missile.takedamage = DAMAGE_NO;

	// set missile speed	
	makevectors (self.v_angle);
	missile.velocity = normalize (v_forward);
	if(deathmatch)
		missile.speed=1000;
	else
		missile.speed=600;
	missile.velocity = missile.velocity * missile.speed;
	missile.angles = vectoangles(missile.velocity);
	missile.searchtime = 0;
	missile.yaw_speed = 50;

	setmodel (missile, "models/ravproj.mdl");
	setsize (missile, '-8 -8 8', '8 8 8');

	setorigin (missile, self.origin + self.proj_ofs - v_forward * 14 + v_right * random(-8,8));
		
	missile.touch = raven_touch;
	missile.lifetime = time + 10;
	missile.classname = "bird_missile";
	sound(missile,CHAN_VOICE,"raven/ravengo.wav",1,ATTN_NORM);

	missile.th_die=raven_death_init;
	if(self.enemy.flags2&FL_ALIVE)
	{
		missile.enemy=self.enemy;
		missile.nextthink = time + HX_FRAME_TIME;
		missile.think = raven_flap;
		missile.next_action = time + .01;
		missile.think1 = raven_track;
		missile.think1 = raven_track_init;
	// Find an enemy
	}
	else
	{
		makevectors(self.v_angle);
		spot1 = self.origin + self.proj_ofs;
		spot2 = spot1 + (v_forward*600); // Look ahead
		traceline(spot1,spot2,FALSE,self);

		// We have a victim in sights
		if ((trace_ent!=world) && 
			(trace_ent.flags & FL_MONSTER) && (trace_ent.owner != self) && (trace_ent.health>0))
		{	
			missile.enemy = trace_ent;

			missile.nextthink = time + HX_FRAME_TIME;
			missile.think = raven_flap;

			missile.next_action = time + .01;
			missile.think1 = raven_track;
			missile.think1 = raven_track_init;
		}
		else
		{	
			missile.nextthink = time + .01;
			missile.think = raven_search;
		}
	}
}

void ravenmissile_explode (void)
{
	create_raven();
	create_raven();
	create_raven();

	CreateWhiteSmoke(self.origin + '0 0  0','0 0 8',HX_FRAME_TIME * 3);
	CreateWhiteSmoke(self.origin + '0 0  5','0 0 8',HX_FRAME_TIME * 3);
	CreateWhiteSmoke(self.origin + '0 0 10','0 0 8',HX_FRAME_TIME * 3);

	remove(self);

}

void ravenmissile_touch (void)
{
entity found;
	if (pointcontents(self.origin) == CONTENT_SKY)
	{
		remove(self);
		return;
	}

	if (other.health)
	{
		sound (self, CHAN_WEAPON, "weapons/explode.wav", 1, ATTN_NORM);
		starteffect(CE_SM_EXPLOSION , self.origin);
		self.enemy = other;
		T_Damage(other,self,self,10);
	}
	else
	{
		found=findradius(self.origin,100);
		while(found)
		{
			if(found.flags2&FL_ALIVE)
			{
				self.enemy=found;
				found=world;
			}
			else
				found=found.chain;
		}
	}
	ravenmissile_explode();
}

void ravenmissile_puff (void)
{
	makevectors(self.angles);

	if (self.lifetime < time)
		ravenmissile_explode();	
	else
	{
		thinktime self : HX_FRAME_TIME * 3;
		self.think = ravenmissile_puff;
	}
}


/*--------------------
Launch all ravens
----------------------*/
void launch_superraven (void)
{
	entity newmis;

	self.attack_finished = time + 0.5;

	makevectors(self.v_angle);

	newmis = spawn();
	setmodel (newmis, "models/birdmsl2.mdl");
	newmis.movetype = MOVETYPE_FLYMISSILE;
	newmis.solid = SOLID_BBOX;
	newmis.takedamage = DAMAGE_NO;
	newmis.owner = self;
	setsize (newmis, '0 0 0', '0 0 0');		

	newmis.velocity = normalize (v_forward);
	newmis.velocity = newmis.velocity * 800;
	newmis.angles = vectoangles(newmis.velocity);
	setorigin(newmis, self.origin + self.proj_ofs  + v_forward*10);

	newmis.touch = ravenmissile_touch;
	newmis.lifetime = time + 3;
	newmis.avelocity_z = 1000; 
	newmis.scale = .40;
	thinktime newmis : HX_FRAME_TIME * 3;
	newmis.think = ravenmissile_puff;

	self.punchangle_x= random(-3);
}


void ravenshot_touch (void)
{
	if (pointcontents(self.origin) == CONTENT_SKY)
	{
		remove(self);
		return;
	}

	T_Damage (other, self, self.owner, self.dmg );

	sound (self, CHAN_WEAPON, "weapons/explode.wav", 1, ATTN_NORM);

	starteffect(CE_SM_EXPLOSION , self.origin);

	remove(self);

}

void create_raven_shot2(vector location,float add_yaw,float nexttime,float rotate,void() nextfunc)
{
	entity missile;
	vector holdangle;

	missile = spawn ();
	missile.owner = self.owner;
	missile.movetype = MOVETYPE_FLYMISSILE;
	missile.solid = SOLID_BBOX;
	missile.solid = DAMAGE_YES;
		
// set missile speed	
	missile.dmg = 30;
	missile.angles = self.angles;

	holdangle = self.angles;
	holdangle_z = 0;
	holdangle_x = 0 - holdangle_x;
	holdangle_y += add_yaw;
	makevectors (holdangle);
	missile.velocity = normalize (v_forward);
	missile.velocity = missile.velocity * 800;

	if (rotate)
		missile.avelocity_z = 1000; 
	else
		missile.avelocity_z = -1000; 

	missile.touch = ravenshot_touch;

	setmodel (missile, "models/vindsht1.mdl");
	setsize (missile, '0 0 0', '0 0 0');		
	setorigin (missile, location);

	missile.classname = "set_missile";

	thinktime missile : nexttime;
	missile.think = nextfunc;

}

void create_raven_shot1(vector location,float nexttime,void() nextfunc,vector fire_angle)
{
	entity missile;

	missile = spawn ();
	missile.owner = self;
	missile.movetype = MOVETYPE_FLYMISSILE;
	missile.solid = SOLID_BBOX;
		
// set missile speed	
	makevectors (fire_angle);
	missile.velocity = normalize (v_forward);
	missile.velocity = missile.velocity * 800;

	missile.avelocity_z = 1000; 

	missile.angles = vectoangles(missile.velocity);
	missile.dmg = 90;
	
	missile.touch = ravenshot_touch;

	setmodel (missile, "models/vindsht1.mdl");
	setsize (missile, '0 0 0', '0 0 0');		
	setorigin (missile, location);

	missile.classname = "set_missile";

	thinktime missile : nexttime;

	missile.think = nextfunc;
}

void missle_straight(void)
{
	vector holdangles;

	holdangles = self.angles;
	holdangles_z = 0;
	holdangles_x = 0 - holdangles_x;
	makevectors (holdangles);

	self.velocity = normalize (v_forward);
	self.velocity = self.velocity * 800;

}

void missle_straight1(void)
{
	vector holdangles;

	holdangles = self.angles;
	holdangles_z = 0;
	holdangles_x = 0 - holdangles_x;
	makevectors (holdangles);

	self.velocity = normalize (v_forward);
	self.velocity = self.velocity * 800;

	create_raven_shot2(self.origin,-10,.25,1,missle_straight);

	CreateLittleBlueFlash(self.origin);
	sound(self,CHAN_WEAPON,"raven/split.wav",1,ATTN_NORM);

}

void missle_straight2(void)
{
	vector holdangles;

	holdangles = self.angles;
	holdangles_z = 0;
	holdangles_x = 0 - holdangles_x;
	makevectors (holdangles);

	self.velocity = normalize (v_forward);
	self.velocity = self.velocity * 800;
	create_raven_shot2(self.origin,10,.25,1,missle_straight);

	CreateLittleBlueFlash(self.origin);
	sound(self,CHAN_WEAPON,"raven/split.wav",1,ATTN_NORM);

}


void split (void)
{
	vector holdangles;

	// RIGHT SIDE
	create_raven_shot2(self.origin,-6,.50,0,missle_straight1);

	// LEFT SIDE
	create_raven_shot2(self.origin,6,.50,0,missle_straight2);

	CreateLittleBlueFlash(self.origin);

	sound(self,CHAN_WEAPON,"raven/split.wav",1,ATTN_NORM);

	self.dmg = 30;
	holdangles = self.angles;
	holdangles_z = 0;
	holdangles_x = 0 - holdangles_x;
	makevectors (holdangles);

	self.velocity = normalize (v_forward);
	self.velocity = self.velocity * 800;
}

void launch_set (vector dir_mod)
{

	self.attack_finished = time + 0.5;

	create_raven_shot1(self.origin + self.proj_ofs + v_forward*14,0.05,split,self.v_angle);
}


void ravenstaff_power (void)
{
	self.wfs=advanceweaponframe($fire1,$fire4);
	self.th_weapon=ravenstaff_power;

	if (self.weaponframe==$fire1)
	{
		self.punchangle_x = -4;
		launch_superraven();
		self.greenmana -= 16;
		self.bluemana -= 16;
	}
	else if(self.weaponframe == $fire4)
	{
		self.weaponframe = $fire4;
		self.th_weapon=ravenstaff_idle;
	}

	thinktime self : HX_FRAME_TIME;
}

void ravenstaff_normal (void)
{
	self.wfs=advanceweaponframe($fire1,$fire4);
	self.th_weapon=ravenstaff_normal;

	if(self.weaponframe==$fire3)
	{
		self.punchangle_x = -2;
		launch_set('0 0 0');
		self.greenmana -= 8;
		self.bluemana -= 8;
	}

	else if(self.wfs==WF_CYCLE_WRAPPED)
	{
		self.weaponframe = $rootpose;
		self.th_weapon=ravenstaff_idle;
	}

	thinktime self : HX_FRAME_TIME;

}


void ravenstaff_fire (void)
{
	vector holdvelocity;

	if ((self.artifact_active & ART_TOMEOFPOWER) &&
		(self.greenmana >= 16) && (self.bluemana >= 16))
	{
		sound (self, CHAN_WEAPON, "raven/rfire2.wav", 1, ATTN_NORM);
		stuffcmd (self, "bf\n");
		ravenstaff_power();
	}
	else if ((self.greenmana >= 8) && (self.bluemana >= 8))
	{
		stuffcmd (self, "bf\n");
		makevectors(self.v_angle);
		holdvelocity = normalize(v_right);
		holdvelocity = holdvelocity * 10;
		starteffect(CE_TELESMK1, self.origin + self.proj_ofs  + v_forward * 14,holdvelocity,HX_FRAME_TIME * 3);
		starteffect(CE_TELESMK1, self.origin + self.proj_ofs  + v_forward * 14,holdvelocity * -1,HX_FRAME_TIME * 3);
		sound (self, CHAN_WEAPON, "raven/rfire1.wav", 1, ATTN_NORM);
		ravenstaff_normal();
	}

  	self.attack_finished = time + 0.5;
}

/*
============
ravenstaff_ready - just sit there until fired
============
*/
void ravenstaff_idle (void)
{
	self.weaponframe= $rootpose;
	self.th_weapon=ravenstaff_idle;
}
	
void ravenstaff_select (void)
{
	self.wfs=advanceweaponframe($select1,$select12);
	self.weaponmodel=("models/ravenstf.mdl");
	self.th_weapon=ravenstaff_select;
	if(self.weaponframe==$select12)
	{
		self.attack_finished = time - 1;
		ravenstaff_idle();
	}
}

void ravenstaff_deselect (void)
{
	self.wfs=advanceweaponframe($select12,$select1);
	self.th_weapon=ravenstaff_deselect;
	thinktime self : HX_FRAME_TIME;
	
	self.oldweapon = IT_WEAPON4;
	if(self.wfs==WF_CYCLE_WRAPPED)
		W_SetCurrentAmmo();

}

/*
 * $Log: /H2 Mission Pack/HCode/ravenstf.hc $
 * 
 * 5     3/19/98 12:17a Mgummelt
 * last bug fixes
 * 
 * 4     3/16/98 2:19a Mgummelt
 * 
 * 3     3/14/98 11:09p Mgummelt
 * 
 * 2     3/14/98 9:24p Mgummelt
 * 
 * 54    10/28/97 1:01p Mgummelt
 * Massive replacement, rewrote entire code... just kidding.  Added
 * support for 5th class.
 * 
 * 52    10/21/97 2:24p Rlove
 * Fixed a bug with bone shards
 * 
 * 51    10/17/97 3:56p Rlove
 * 
 * 50    10/17/97 11:13a Rlove
 * 
 * 49    9/04/97 5:06p Mgummelt
 * Fixing Meat chunk colors and wrong autoaiming in coop
 * 
 * 48    9/04/97 3:50p Mgummelt
 * 
 * 47    9/02/97 8:00p Rlove
 * 
 * 46    9/01/97 9:49p Rlove
 * 
 * 45    9/01/97 6:01p Rlove
 * 
 * 44    9/01/97 1:19a Rlove
 * 
 * 43    8/31/97 9:46p Rlove
 * 
 * 42    8/31/97 3:45p Rlove
 * 
 * 41    8/30/97 7:32p Jweier
 * 
 * 40    8/26/97 10:49a Rlove
 * 
 * 39    8/26/97 10:16a Rlove
 * 
 * 38    8/26/97 7:41a Mgummelt
 * Removing one last old player frame code reference
 * 
 * 37    8/26/97 5:55a Rlove
 * 
 * 36    8/24/97 2:13p Rlove
 * 
 * 35    8/23/97 7:15p Rlove
 * 
 * 33    8/11/97 10:55a Rlove
 * 
 * 31    8/08/97 10:02a Rlove
 * 
 * 28    8/08/97 8:03a Rlove
 * 
 * 25    8/07/97 11:13a Rlove
 * 
 * 18    7/24/97 12:02p Mgummelt
 * 
 * 17    7/21/97 3:03p Rlove
 * 
 * 15    7/12/97 9:09a Rlove
 * Reworked Assassin Punch Dagger
 * 
 * 12    7/03/97 8:12a Rlove
 * 
 * 10    6/30/97 9:41a Rlove
 * 
 * 4     6/26/97 7:36a Rlove
 * Changed Vindictus to Ravenstaff
 * 
 * 3     6/24/97 7:48a Rlove
 * 
 */
