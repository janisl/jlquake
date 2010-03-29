/*
 * $Header: /H2 Mission Pack/HCode/weather.hc 26    3/05/98 7:54p Jmonroe $
 */

void reset_solid_trigger ()
{
	self.solid=SOLID_TRIGGER;
}

void dust_touch ()
{
float numPuffs, thisPuff;
vector dustpos, dustvect;
	
	if(self.inactive)
		return;

	if(random()*100>self.count)
		return;

	if(other.solid==SOLID_TRIGGER)
		return;

	numPuffs = random(5, 10) + 2;
	thisPuff = 1;
	
	while (thisPuff < numPuffs)
	{
		dustpos_x = random(self.maxs_x, self.mins_x);
		dustpos_y = random(self.maxs_y, self.mins_y);
		dustpos_z = self.maxs_z;
		
		dustvect_x = random(10, 20);
		dustvect_y = random(10, 20);
		dustvect_z = random(-10, -1);
		
		particle4(dustpos,dustvect_x,self.color,PARTICLETYPE_FASTGRAV,3);
		thisPuff += 1;
	}
	self.solid=SOLID_NOT;
	self.think=reset_solid_trigger;
	thinktime self : random()*0.5 + 0.2;
}

/*QUAKED weather_dust (0 1 1) ? x x x INACTIVE
Dust falls, or is stirred up within this entity.
INACTIVE - Must be tuirned on by a trigger_activated before usable
-------------------------FIELDS-------------------------
color - palette number of the dust's color (add 256 - (0 to 15) for translucency)
        default = 272 (light grey + 256 for translucency)
		88 to 95 (+ 256 for transluc) is light brownish
count - How much dust falls (0 - 100)
		default = 10
*/

void weather_dust ()
{
	InitTrigger();
	        
	self.touch = dust_touch;
	self.use = dust_touch;
	
	if (!self.color) 
		self.color=285;

	if (!self.count) 
		self.count=50;
}

void rubble_touch ()
{
	if(self.inactive)
		return;

	if(random()*100>self.count)
		return;

	if(other.solid==SOLID_TRIGGER)
		return;

	CreateModelChunks(self.size,1,1);

	self.solid=SOLID_NOT;
	self.think=reset_solid_trigger;
	thinktime self : random()*0.5 + 0.2;
}

void rubble_turn_on ()
{
	if(!self.spawnflags&2)
		self.touch = rubble_touch;
	self.use = rubble_touch;
}

/*QUAKED trigger_rubble (0 1 1) ? NOTOUCH x x INACTIVE 
THE BOTTOM PLANE OF THIS ENT CANNOT BE IN A SOLID!!!
Rubble falls from the top plane of this entity.
INACTIVE - Must be activated before usable
NOTOUCH - can't be activated by touching it.
-------------------------FIELDS-------------------------
'count' - How much rubble falls (0 - 100)
		default = 50
'dmg' - How much damage (generally) each rock does if hit person
		default = 1
'thingtype' - type of chunks and sprites it will generate
GREYSTONE		= 1 (default)
WOOD			= 2
METAL			= 3
FLESH			= 4
FIRE			= 5
CLAY			= 6
LEAVES			= 7
HAY				= 8
BROWNSTONE		= 9
CLOTH			= 10
WOOD_LEAF		= 11
WOOD_METAL		= 12
WOOD_STONE		= 13
METAL_STONE		= 14
METAL_CLOTH 	= 15
WEBS		 	= 16
GLASS 			= 17
ICE 			= 18
CLEARGLASS 		= 19
REDGLASS 		= 20
ACID	 		= 21
*/

void trigger_rubble ()
{
	InitTrigger();
	        
	if(!self.targetname)
	{
		if(!self.spawnflags&2)
			self.touch = rubble_touch;
		self.use = rubble_touch;
	}
	else
		self.use = rubble_turn_on;
	
	if (!self.count) 
		self.count=10;

	if (!self.dmg) 
		self.dmg=1;

	if(!self.thingtype)
		self.thingtype = THINGTYPE_GREYSTONE;
}

/*
void () rain_use =
{
	dprint(" rain ");

	rain_go(self.mins,self.maxs,self.size,'125 100 0',self.color+random(8),self.counter);
	self.nextthink = time + self.wait;
};
*/

float RAIN_STRAIGHT = 1;

/*QUAKED weather_rain (0 1 1) ? STRAIGHT NO_SPLAT
MG&RL
Rain falls within this entity.
STRAIGHT = Rain will fall straight down rather than at an angle
NO_SPLAT = Rain will not make "splats" on the ground
-------------------------FIELDS-------------------------
color   - palette number of the rain's color - will range between this color and this color + 8 unless monochrome is checked
        default - 414 (translucent blue)
counter - number of rain particles to generate every 10th of a second.
        default - 300
wait    - how often to generate rain particles
        default - .1 of a second
soundtype - 0 = rain (default)
			1 = drip (cave-type- wetness)
--------------------------------------------------------
*/
void rain_splatter ()
{//MG
vector spot1,spot2;
	spot1=self.mins;
	spot1_x+=random(self.size_x);
	spot1_y+=random(self.size_y);
	spot1_z=self.maxs_z;
	spot2=spot1;
	spot2_z=self.mins_z - 10;
	
	traceline(spot1,spot2,FALSE,self);
	if(trace_fraction<1)
	{
//		particle4(trace_endpos,10,random(408,412),PARTICLETYPE_FASTGRAV,random(5,10));
		particle4(trace_endpos,10,random(408,412),PARTICLETYPE_GRAV,random(5,10));
		thinktime self : random(0.01,0.2);
	}
	else
		thinktime self : 0;
}

void make_splatter ()
{
	newmis=spawn();
	setorigin(newmis,self.origin);
	setsize(newmis,self.mins,self.maxs);
	newmis.think=rain_splatter;
	thinktime newmis : 0;
}

void() weather_rain =
{//MG&BL
vector dir;
float splat_count;
	if (!self.soundtype)
	{
		precache_sound2("ambience/rain.wav");
		self.noise1="ambience/rain.wav";
	}
	else
	{
		precache_sound2("ambience/drip1.wav");
		self.noise1 = ("ambience/drip1.wav");
		self.think = sound_again;
		self.nextthink = time + random(5,30);
	}

  	self.movetype = MOVETYPE_NOCLIP;
  	self.owner = self;
  	self.solid = SOLID_NOT;
  	setsize (self, self.mins , self.maxs);
  	setorigin (self, self.origin);
  	setmodel (self, self.model);
   self.modelindex = 0;
  	self.model = "";
	
	if (!self.wait)
	   self.wait=0.10;
	       
	if (!self.color)
		self.color=414;

	if (!self.counter)
		self.counter=300;

	if (self.spawnflags & RAIN_STRAIGHT)
		dir = '0 0 0';
	else
		dir = '125 100 0';

	starteffect(CE_RAIN, self.mins, self.maxs, self.size, dir, self.color, self.counter, self.wait);

	if(!self.soundtype)
		ambientsound (self.origin, "ambience/rain.wav",1 , ATTN_NONE);
	else
		sound (self, CHAN_VOICE, self.noise1, 1, ATTN_NORM);

	if(!self.spawnflags&2)
		splat_count=10;
	while(splat_count)
	{
		make_splatter();
		splat_count-=1;
	}
	if(!self.soundtype)
		remove(self);
};

void snow_use(void)
{
	starteffect(CE_SNOW, self.mins, self.maxs, self.spawnflags, self.movedir, self.counter);
	ambientsound ((self.absmin+self.absmax)*0.5, "ambience/wind.wav",1 , ATTN_NORM);
	remove(self);
}

/*QUAKED weather_snow (0 1 1) ? fluffy mixed half_bright no_melt in_bounds no_trans
MG
Snow falls within this entity. (Client effect)
NOTE: In GL, may have actual snow texture on polys for snow

------SPAWNFLAGS------
fluffy - All largish flakes
mixed - Mixed flakes
half_bright - All flakes start at most half bright (darker snow)
no_melt - Flakes don't melt when his surface, just go away
in_bounds - Flakes cannot leave the bounds of their box
no_trans - Flakes are not translucent (perhaps slight render boost)

--------FIELDS--------
counter - number of rain particles to generate every 10th of a second.
default - 100

speed - How fast the snow falls 
default = 200
NOTE: Fast snowflakes could go through thin walls, so a check is made, faster snowflakes than 300 may cause a slight slowdown

anglespeed - How fast the lateral movement is on the snow
default is 125

movedir - direction snow blows, from '-1 -1 -1' to '1 1 1', 'x y z' format, default is random between '-1 -1 -1' and '1 1 -0.5'
soundtype - 0 = wind (default)
--------------------------------------------------------
*/
void() weather_snow =
{//MG

//	remove(self);
//	return;
//FIXME:
//Command to stop it?
//Changes in density- fade in/out?
//Owner ent in C that can change dir and veer (wind)?

	precache_sound4("fx/snowwind.wav");

  	self.movetype = MOVETYPE_NOCLIP;
  	self.owner = self;
  	self.solid = SOLID_NOT;
  	setsize (self, self.mins , self.maxs);
  	setorigin (self, self.origin);
  	setmodel (self, self.model);
	self.modelindex = 0;
  	self.model = "";
	
//	if (!self.wait)
//	   self.wait = 0.10;
	       
	if (!self.counter)
		self.counter = 10;
	else
		self.counter/=10;

	if(!self.speed)
		self.speed = 200;

	if(!self.anglespeed)
		self.anglespeed = 125;

	if(!self.movedir)
		self.movedir=randomv('-1 -1 -1','1 1 -0.5');

	if(self.spawnflags&SFL_FLUFFY&&self.spawnflags&SFL_MIXED)
		self.spawnflags(-)SFL_MIXED;//All fluffy overrides mixed

	self.movedir_x *= self.anglespeed;
	self.movedir_y *= self.anglespeed;
	self.movedir_z *= self.speed;

//Color defualts to a range of translucent to opaque whites
	if (self.targetname)
	{
		self.use = snow_use;
	}
	else
	{
		starteffect(CE_SNOW, self.mins, self.maxs, self.spawnflags, self.movedir, self.counter);
	//	if(!self.soundtype)
			ambientsound ((self.absmin+self.absmax), "fx/snowwind.wav",1 , ATTN_NORM);
	//	else
	//		sound (self, CHAN_VOICE, self.noise1, 1, ATTN_NORM);

		remove(self);
	}
};

void () weather_lightning_use =
{
vector p1,p2;
entity targ;

	
	if (!self.target)
	{
		dprint("No target for lightning\n");
		return;
	}
	
	targ = find (world, targetname, self.target);  // Get ending point
	
	if (!targ)
	{
		dprint("No target for beam effect\n");
		return;
	}

/*	if(!self.aflag&&self.spawnflags&2)
	{
		self.aflag=TRUE;
		if(random()<0.5)
			sound (self,CHAN_AUTO,"crusader/lghtn1.wav",1,ATTN_NORM);
		else
			sound (self,CHAN_AUTO,"crusader/lghtn1.wav",1,ATTN_NORM);
	}*/

	
	p1 = self.origin;
	p2 = targ.origin;
	p1+=normalize(p2-p1)*15;	//So beam is drawn at startpoint
				
	if(self.classname=="weather_lightning_start")
		do_lightning (self,1,0,4,p1,p2,10,TE_STREAM_LIGHTNING);

	else if(self.classname=="weather_sunbeam_start")
	{
		sound(self,CHAN_WEAPON,"crusader/sunhum.wav",1,ATTN_NORM);

		WriteByte (MSG_BROADCAST, SVC_TEMPENTITY);
		WriteByte (MSG_BROADCAST, TE_STREAM_SUNSTAFF1);
		WriteEntity (MSG_BROADCAST, self);
		WriteByte (MSG_BROADCAST, 0);
		WriteByte (MSG_BROADCAST, 4);
	
		WriteCoord (MSG_BROADCAST, p1_x);
		WriteCoord (MSG_BROADCAST, p1_y);
		WriteCoord (MSG_BROADCAST, p1_z);

		WriteCoord (MSG_BROADCAST, p2_x);
		WriteCoord (MSG_BROADCAST, p2_y);
		WriteCoord (MSG_BROADCAST, p2_z);
		
		LightningDamage (p1, p2, self, 10,"sunbeam");
	}
	else if(self.classname=="fx_colorbeam_start")
	{
//FIXME: make a temp ent colored beam where you can pass the color
//	Uses 1 model, but several diff. color skins
		WriteByte (MSG_BROADCAST, SVC_TEMPENTITY);
		WriteByte (MSG_BROADCAST, TE_STREAM_COLORBEAM);	//beam type
		WriteEntity (MSG_BROADCAST, self);				//owner
		WriteByte (MSG_BROADCAST, 0);					//tag + flags
		WriteByte (MSG_BROADCAST, 4);					//time
		WriteByte (MSG_BROADCAST, self.color);			//color
	
		WriteCoord (MSG_BROADCAST, p1_x);
		WriteCoord (MSG_BROADCAST, p1_y);
		WriteCoord (MSG_BROADCAST, p1_z);

		WriteCoord (MSG_BROADCAST, p2_x);
		WriteCoord (MSG_BROADCAST, p2_y);
		WriteCoord (MSG_BROADCAST, p2_z);
	}
	else
		return;

	if (self.lifetime > time )  // Not done living
		thinktime self : 0.2;
  	else if (self.wait>-1)  // constantly running lightning needs to be reset
  	{
		thinktime self : self.wait;
		self.think = weather_lightning_use;
		self.lifetime = self.lifespan + self.nextthink;
		self.aflag=FALSE;
  	}
	else
		self.nextthink=-1;
};


void () lightning_init =
{
	if (self.lifetime > time )  // Being triggered right now
		return; 

	self.think = weather_lightning_use;
	self.lifetime = self.lifespan + time;
	if(self.spawnflags&2)
	{
		if(random()<0.5)
			sound (self,CHAN_AUTO,"crusader/lghtn1.wav",1,ATTN_NORM);
		else
			sound (self,CHAN_AUTO,"crusader/lghtn1.wav",1,ATTN_NORM);
		self.aflag=TRUE;
	}
	weather_lightning_use ();  // Make it run

};


/*QUAKED weather_lightning_start (0 1 1) (-8 -8 -8) (8 8 8) STARTOFF thunder_sound
Generates a bolt of lightning which ends at the weather_lightning_end that is the target

STARTOFF - if clicked the lightning becomes triggerable, will run constantly once triggered. Wait will still tell how long to wait between strikes
thunder_sound - will randomly play lightning strike sounds when it fires
-------------------------FIELDS-------------------------
noise  - sound generated when lightning appears
      1 - no sound
      2 - lightning (default)

target - be sure to give this a target fx_lightning_end to hit

wait   - amount of time between strikes
      -1   lightning is triggerable.  
      > -1 lightning is constantly running.
      default -1

lifespan - amount of time lightning bolt will exist.
--------------------------------------------------------
*/
void () weather_lightning_start =
{

	self.movetype = MOVETYPE_NOCLIP;
	self.owner = self;
	self.solid = SOLID_NOT;
	setorigin (self,self.origin);
	setmodel (self,self.model);
	setsize (self,self.mins, self.maxs);

	if(self.classname=="weather_lightning_start")
	{
		precache_model3("models/stlghtng.mdl");
		if (!self.noise)
		{
			self.noise = "raven/lightng1.wav"; 
			precache_sound3("raven/lightng1.wav");
		}
		if(self.spawnflags&2)
		{
			self.noise="";
			precache_sound2("crusader/lghtn1.wav");
		}
	}
	else if (self.classname == "weather_sunbeam_start" && self.noise == "2")
	{
		dprint("Shazzaam!\n");
		self.noise = "crusader/lghtn1.wav";
		precache_sound3("crusader/lghtn1.wav");
	}
	
	self.use = lightning_init;		// For triggered lightning
	self.think = weather_lightning_use;

	if (!self.wait)	// Set wait to default
		self.wait = 5;

	if ((self.wait>-1) && (!self.spawnflags & 1))   // Constantly running lightning
	{
		thinktime self : self.wait;
		self.lifetime = time + self.lifespan + self.wait;
	}
};

/*QUAKED weather_lightning_end (0 1 1) (-8 -8 -8) (8 8 8)
Where lightning from weather_lightning_start will hit.
-------------------------FIELDS-------------------------
none
--------------------------------------------------------
*/
void () weather_lightning_end =
{
	self.movetype = MOVETYPE_NOCLIP;
	self.owner = self;
	self.solid = SOLID_NOT;
	setsize (self,self.mins, self.maxs);
	setorigin (self,self.origin);
};


/*QUAKED weather_sunbeam_start (0 1 1) (-8 -8 -8) (8 8 8) STARTOFF
Generates a ray of sunlight which ends at the weather_sunbeam_end that is the target

STARTOFF - if clicked the sunbeam becomes triggerable, will run constantly once triggered. Wait will still tell how long to wait between strikes
-------------------------FIELDS-------------------------
noise  - sound generated when lightning appears
      1 - no sound (default)
      2 - lightning 

target - be sure to give this a target fx_lightning_end to hit

wait   - amount of time between strikes
      -1   sunbeam is triggerable.  
      > -1 sunbeam is constantly running.
      default -1

lifespan - amount of time sunbeam will exist.
--------------------------------------------------------
*/
void () weather_sunbeam_start =
{
	precache_model3("models/stsunsf1.mdl");
	precache_model3("models/stsunsf2.mdl");
	precache_model3("models/stsunsf3.mdl");
	precache_model3("models/stsunsf4.mdl");

	self.noise = "crusader/lghtn1.wav";

	weather_lightning_start();
};

/*QUAKED weather_sunbeam_end (0 1 1) (-8 -8 -8) (8 8 8)
Where sunbeam from weather_sunbeam_start will hit.
-------------------------FIELDS-------------------------
none
--------------------------------------------------------
*/
void () weather_sunbeam_end =
{
	weather_lightning_end();
};

/*QUAKED fx_colorbeam_start (0 1 1) (-8 -8 -8) (8 8 8) STARTOFF
Generates a ray of colored light which ends at the fx_colorbeam_end that is the target

STARTOFF - if clicked the colorbeam becomes triggerable, will run constantly once triggered. Wait will still tell how long to wait between strikes
-------------------------FIELDS-------------------------
noise  - sound generated when lightning appears
      1 - no sound (default)
      2 - lightning 

target - be sure to give this a target fx_lightning_end to hit

wait   - amount of time between strikes
      -1   beam is triggerable.  
      > -1 beam is constantly running.
      default -1

color - color of the beam
	0 = red (default)
	1 = blue
	2 = green
	3 = white
	4 = yellow

lifespan - amount of time beam will exist.
--------------------------------------------------------
*/

void () fx_colorbeam_start =
{
	precache_model2("models/stclrbm.mdl");
	weather_lightning_start();
};


/*QUAKED fx_colorbeam_end (0 1 1) (-8 -8 -8) (8 8 8)
Where colorbeam from weather_colorbeam_start will hit.
-------------------------FIELDS-------------------------
none
--------------------------------------------------------
*/

void () fx_colorbeam_end =
{
	weather_lightning_end();
};

/*
 * $Log: /H2 Mission Pack/HCode/weather.hc $
 * 
 * 26    3/05/98 7:54p Jmonroe
 * fixed startRain, optimized particle struct
 * 
 * 25    3/03/98 4:36p Jmonroe
 * changed over to precache 4 to build my pak
 * 
 * 24    2/26/98 2:03p Mgummelt
 * 
 * 23    2/25/98 2:08p Mgummelt
 * 
 * 22    2/24/98 11:20a Jweier
 * 
 * 21    2/23/98 4:37p Jmonroe
 * removed erroneous references to wait on snow
 * 
 * 20    2/12/98 5:55p Jmonroe
 * remove unreferenced funcs
 * 
 * 19    2/10/98 2:42p Jmonroe
 * improved precache some more
 * 
 * 18    2/06/98 5:03a Nalbury
 * Put in preliminary client chunks for network games...
 * 
 * 17    2/05/98 9:21p Mgummelt
 * 
 * 16    1/21/98 3:24p Mgummelt
 * 
 * 15    1/21/98 2:49p Mgummelt
 * 
 * 14    1/20/98 3:20p Mgummelt
 * 
 * 13    1/20/98 2:37p Mgummelt
 * 
 * 12    1/20/98 1:47p Mgummelt
 * 
 * 11    1/07/98 2:34p Mgummelt
 * 
 * 49    10/28/97 1:01p Mgummelt
 * Massive replacement, rewrote entire code... just kidding.  Added
 * support for 5th class.
 * 
 * 47    9/16/97 4:17p Rjohnson
 * Updates
 * 
 * 46    9/03/97 12:26a Mgummelt
 * 
 * 45    8/31/97 8:52a Mgummelt
 * 
 * 44    8/31/97 12:02a Mgummelt
 * 
 * 43    8/29/97 7:08a Mgummelt
 * 
 * 42    8/27/97 6:04p Jweier
 * 
 * 41    8/23/97 11:17p Mgummelt
 * 
 * 40    8/23/97 8:23p Mgummelt
 * 
 * 39    8/22/97 3:47p Mgummelt
 * 
 * 38    8/22/97 2:41p Rjohnson
 * Sound problem
 * 
 * 37    8/18/97 8:07p Mgummelt
 * 
 * 36    8/16/97 5:46p Mgummelt
 * 
 * 35    8/13/97 5:51p Mgummelt
 * 
 * 34    8/06/97 10:14p Mgummelt
 * 
 * 33    8/06/97 12:04p Mgummelt
 * 
 * 32    8/04/97 10:29a Mgummelt
 * 
 * 31    7/29/97 11:20p Mgummelt
 * 
 * 30    7/29/97 9:15p Mgummelt
 * 
 * 29    7/18/97 3:56p Mgummelt
 * 
 * 28    7/09/97 6:20p Mgummelt
 * 
 * 27    7/02/97 3:32p Rlove
 * 
 * 26    7/01/97 3:29p Mgummelt
 * 
 * 25    7/01/97 2:21p Mgummelt
 * 
 * 24    6/30/97 12:33p Rlove
 * 
 * 23    6/30/97 10:29a Rlove
 * Added wait to rain entity
 * 
 * 22    6/25/97 2:58p Mgummelt
 * 
 * 21    6/18/97 6:02p Jweier
 * 
 * 20    6/12/97 8:55p Mgummelt
 * 
 * 19    5/30/97 11:41a Rjohnson
 * Removed the message field of the starteffect
 * 
 * 18    5/21/97 2:42p Rjohnson
 * Removed a debugging message
 * 
 * 17    5/20/97 11:31a Rjohnson
 * Revised Effects
 * 
 * 16    4/28/97 2:50p Rlove
 * Little change to rain
 * 
 * 15    4/28/97 1:12p Mgummelt
 * 
 * 14    4/24/97 2:22p Mgummelt
 * 
 * 13    4/14/96 3:47p Mgummelt
 * 
 * 12    4/07/97 5:34p Rlove
 * Chenged type of lightning generated.
 * 
 * 11    3/31/97 6:36a Rlove
 * Changed SpawnBlood to SpawnPuff
 * 
 * 10    3/25/97 4:04p Rjohnson
 * Fixed pre-cache problem
 * 
 * 9     3/18/97 5:57p Jweier
 * weather_dust should be done
 * 
 * 8     3/18/97 5:16p Jweier
 * fixed weather_dust bounds problem
 * 
 * 7     3/14/97 6:01p Jweier
 * Dust is functional
 * 
 * 6     3/13/97 4:58p Jweier
 * Added weather_dust (basic)
 * 
 * 5     11/25/96 2:16p Rlove
 * added flash to lightning entity
 * 
 * 4     11/19/96 11:40a Rlove
 * Particle explosion entity
 * 
 * 3     11/18/96 3:31p Rlove
 * added weather_lightning entity
 * 
 * 2     11/14/96 8:12a Rlove
 * 
 * 1     11/12/96 2:42p Rlove
 */
