/*
 * $Header: /HexenWorld/HCode/torch.hc 6     4/20/98 2:03p Rjohnson $
 */
/*
==========================================================
TORCH.HC
MG

Torches can be toggled/faded, shot out, etc.
==========================================================
*/
void fire_hurt_field_touch ()
{
	if(self.attack_finished>time)
		return;
	if(self.inactive)
		return;
	if(other.health<=0)
		return;

	self.attack_finished=time+HX_FRAME_TIME;
	T_Damage(other,self,self,self.dmg);
	if(self.t_width<time)
	{
		self.t_width=time+0.6;
		sound(self,CHAN_WEAPON,"crusader/sunhit.wav",1,ATTN_NORM);
	}
}

void init_fire_hurt_field ()
{
	InitTrigger();
	self.touch=fire_hurt_field_touch;
}

void spawn_burnfield (vector org)
{//Make a trigger_flame_hurt around flame
entity fhf;
	fhf=spawn();
	fhf.model=self.model;
	fhf.effects=EF_NODRAW;
	self.trigger_field=fhf;
	//	setsize(fhf,'-3 -3 0','3 3 9');
	setorigin(fhf,org);
	if(self.dmg)
		fhf.dmg=self.dmg;
	else
		fhf.dmg=self.dmg=.2;

	fhf.classname="fire hurt field";
	fhf.think=init_fire_hurt_field;
	fhf.nextthink=time;
}

void Init_Torch ()
{
	if(self.targetname)
		self.use=torch_use;

	self.solid		= SOLID_BBOX;
	self.movetype	= MOVETYPE_NONE;
	if(self.health)
	{
//		self.skin=1;
		if(!self.targetname)
			dprint("No targetname for breakable torch\n");
		if(self.style<32)
		{
			dprint(ftos(self.style));
			dprint(": Bad lightstyle for breakable torch\n");
		}
		self.takedamage=DAMAGE_YES;
		self.th_die=torch_death;
		self.classname="breakable_torch";
	}

	if(self.style>=32)
	{
		if(!self.lightvalue2)
			self.lightvalue2=11;
		if(!self.fadespeed)
			self.fadespeed = 1;
		initialize_lightstyle();
		self.think = torch_think;
		thinktime self : 1;
	}
	else
	{
		self.drawflags(+)MLS_ABSLIGHT;
		setmodel(self,self.weaponmodel);
		if(self.health!=0||self.targetname!="")
		{
			dprint(ftos(self.style));
			dprint(": Bad lightstyle for breakable torch\n");
		}
		else
			makestatic(self);
	}
}

/*QUAKED light_torch_eqypt (0 1 0) (-8 -8 -8) (8 8 8) START_LOW
An Egyptian style torch that displays light
Default light value is 300

.health = If you give the torch health, it can be shot out.  It will automatically select it's second skin (the beat-up torch look)
You must give it a targetname too, just any junk targetname will do like "junk"
----------------------------------
If triggered, will toggle between lightvalue1 and lightvalue2
.lightvalue1 (default 0) 
.lightvalue2 (default 11, equivalent to 300 brightness)
Two values the light will fade-toggle between, 0 is black, 25 is brightest, 11 is equivalent to a value of 300.
.fadespeed (default 1) = How many seconds it will take to complete the desired lighting change
The light will start on at a default of the higher light value unless you turn on the startlow flag.
START_LOW = will make the light start at the lower of the lightvalues you specify (default uses brighter)

NOTE: IF YOU DON'T PLAN ON USING THE DEFAULTS, ALL LIGHTS IN THE BANK OF LIGHTS NEED THIS INFO
*/
void light_torch_eqypt()
{
	precache_model2("models/egtorch.mdl");
	precache_model2("models/eflmtrch.mdl");
	precache_sound("raven/flame1.wav");

	self.abslight = .75;

	self.mdl = "models/egtorch.mdl";
	self.weaponmodel = "models/eflmtrch.mdl";

	self.thingtype	= THINGTYPE_WOOD;
	setsize(self, '-6 -6 -8','6 6 8');

	Init_Torch();
}


/*QUAKED light_torch_castle (0 1 0) (-8 -8 -8) (8 8 8) START_LOW
The Castle style torch that displays light
Default light value is 300

.health = If you give the torch health, it can be shot out.  It will automatically select it's second skin (the beat-up torch look)
You must give it a targetname too, just any junk targetname will do like "junk"
----------------------------------
If triggered, will toggle between lightvalue1 and lightvalue2
.lightvalue1 (default 0) 
.lightvalue2 (default 11, equivalent to 300 brightness)
Two values the light will fade-toggle between, 0 is black, 25 is brightest, 11 is equivalent to a value of 300.
.fadespeed (default 1) = How many seconds it will take to complete the desired lighting change
The light will start on at a default of the higher light value unless you turn on the startlow flag.
START_LOW = will make the light start at the lower of the lightvalues you specify (default uses brighter)

NOTE: IF YOU DON'T PLAN ON USING THE DEFAULTS, ALL LIGHTS IN THE BANK OF LIGHTS NEED THIS INFO
*/
void light_torch_castle()
{
	precache_model("models/castrch.mdl");
	precache_model("models/cflmtrch.mdl");
	precache_sound("raven/flame1.wav");

	self.weaponmodel="models/cflmtrch.mdl";
	self.mdl= "models/castrch.mdl";

	self.thingtype	= THINGTYPE_METAL;
	setsize(self, '-6 -6 -8','6 6 8');

	self.abslight = .75;

	Init_Torch();

}


/*QUAKED light_torch_meso (0 1 0) (-12 -12 -16) (12 12 16) START_LOW
The Meso style torch that displays light
Default light value is 300

.health = If you give the torch health, it can be shot out.  It will automatically select it's second skin (the beat-up torch look)
You must give it a targetname too, just any junk targetname will do like "junk"
----------------------------------
If triggered, will toggle between lightvalue1 and lightvalue2
.lightvalue1 (default 0) 
.lightvalue2 (default 11, equivalent to 300 brightness)
Two values the light will fade-toggle between, 0 is black, 25 is brightest, 11 is equivalent to a value of 300.
.fadespeed (default 1) = How many seconds it will take to complete the desired lighting change
The light will start on at a default of the higher light value unless you turn on the startlow flag.
START_LOW = will make the light start at the lower of the lightvalues you specify (default uses brighter)

NOTE: IF YOU DON'T PLAN ON USING THE DEFAULTS, ALL LIGHTS IN THE BANK OF LIGHTS NEED THIS INFO
*/
void light_torch_meso()
{
	precache_model2("models/mesotrch.mdl");
	precache_model2("models/mflmtrch.mdl");
	precache_sound("raven/flame1.wav");

	self.weaponmodel="models/mflmtrch.mdl";
	self.mdl	= "models/mesotrch.mdl";

	self.abslight = .75;

	self.thingtype	= THINGTYPE_BROWNSTONE;
	setsize(self, '-12 -12 -16','12 12 16');

	Init_Torch();
}


/*QUAKED light_torch_rome (0 1 0) (-8 -8 -8) (8 8 8) START_LOW
The Roman style torch that displays light
Default light value is 300

.health = If you give the torch health, it can be shot out.  It will automatically select it's second skin (the beat-up torch look)
You must give it a targetname too, just any junk targetname will do like "junk"
----------------------------------
If triggered, will toggle between lightvalue1 and lightvalue2
.lightvalue1 (default 0) 
.lightvalue2 (default 11, equivalent to 300 brightness)
Two values the light will fade-toggle between, 0 is black, 25 is brightest, 11 is equivalent to a value of 300.
.fadespeed (default 1) = How many seconds it will take to complete the desired lighting change
The light will start on at a default of the higher light value unless you turn on the startlow flag.
START_LOW = will make the light start at the lower of the lightvalues you specify (default uses brighter)

NOTE: IF YOU DON'T PLAN ON USING THE DEFAULTS, ALL LIGHTS IN THE BANK OF LIGHTS NEED THIS INFO
*/

void light_torch_rome()
{
	precache_model("models/rometrch.mdl");
	precache_model("models/rflmtrch.mdl");
	precache_sound("raven/flame1.wav");

	self.weaponmodel="models/rflmtrch.mdl";
	self.mdl	= "models/rometrch.mdl";

	self.thingtype	= THINGTYPE_GREYSTONE;
	setsize(self, '-6 -6 -8','6 6 8');

	self.abslight = .75;

	Init_Torch();

}

/*QUAKED light_lantern (0 1 0) (-11 -11 -41) (11 11 5) START_LOW
A castle lantern that hangs on the wall
Default light value is 300

.health = If you give the torch health, it can be shot out.  It will automatically select it's second skin (the beat-up torch look)
You must give it a targetname too, just any junk targetname will do like "junk"
----------------------------------
If triggered, will toggle between lightvalue1 and lightvalue2
"lightvalue1" (default 0) 
"lightvalue2" (default 11, equivalent to 300 brightness)
"abslight" You can give it explicit lighting so it doesn't glow (0 to 2.5)
Two values the light will fade-toggle between, 0 is black, 25 is brightest, 11 is equivalent to a value of 300.
"fadespeed" (default 1) = How many seconds it will take to complete the desired lighting change
The light will start on at a default of the higher light value unless you turn on the startlow flag.
START_LOW = will make the light start at the lower of the lightvalues you specify (default uses brighter)

NOTE: IF YOU DON'T PLAN ON USING THE DEFAULTS, ALL LIGHTS IN THE BANK OF LIGHTS NEED THIS INFO
*/
void light_lantern ()
{
	precache_model5("models/lantern.mdl");//fixme-no flame this version
	precache_model5("models/lantern.mdl");
	precache_sound("raven/flame1.wav");

	self.drawflags(+)MLS_ABSLIGHT;
	self.abslight = .75;

	self.mdl = "models/lantern.mdl";
	self.weaponmodel = "models/lantern.mdl";

	self.thingtype	= THINGTYPE_METAL;
	setsize(self, '-11 -11 -41','11 11 5');

	Init_Torch();
	FireAmbient();
}


/*QUAKED light_burner (0 1 0) (-16 -18 -52) (16 18 0) START_LOW DAMAGE
A brazier
Default light value is 300

.health = If you give the torch health, it can be shot out.  It will automatically select it's second skin (the beat-up torch look)
You must give it a targetname too, just any junk targetname will do like "junk"
----------------------------------
If triggered, will toggle between lightvalue1 and lightvalue2
"lightvalue1" (default 0) 
"lightvalue2" (default 11, equivalent to 300 brightness)
"abslight" You can give it explicit lighting so it doesn't glow (0 to 2.5)
Two values the light will fade-toggle between, 0 is black, 25 is brightest, 11 is equivalent to a value of 300.
"fadespeed" (default 1) = How many seconds it will take to complete the desired lighting change
The light will start on at a default of the higher light value unless you turn on the startlow flag.
START_LOW = will make the light start at the lower of the lightvalues you specify (default uses brighter)

NOTE: IF YOU DON'T PLAN ON USING THE DEFAULTS, ALL LIGHTS IN THE BANK OF LIGHTS NEED THIS INFO
--------------------------------------------------------
*/
void light_burner (void)
{
	precache_model5("models/burner.mdl");
	precache_model5("models/burnerfl.mdl");
	precache_model ("models/flame2.mdl");
	if(!self.abslight)
		self.abslight = .75;
	self.drawflags(+)MLS_ABSLIGHT;

	self.mdl = "models/burner.mdl";
	self.weaponmodel = "models/burnerfl.mdl";	//Flame On!

	self.movechain=spawn();
	setorigin(self.movechain,self.origin+'0 0 6');

	if(self.spawnflags&2)
		spawn_burnfield(self.movechain.origin);
	else
		self.dmg=0;

	self.thingtype	= THINGTYPE_CLAY;
	setsize(self, '-17 -17 -52','17 17 0');

	self.solid=SOLID_BBOX;

//Put Flame on top

	self.movechain.abslight = .75;

	setmodel(self.movechain,"models/flame2.mdl");
	self.movechain.drawflags(+)MLS_ABSLIGHT;
	FireAmbient();

	Init_Torch();
	makestatic (self.movechain);
}

/*QUAKED light_palace_torch (0 1 0) (-19 -19 0) (19 19 35) START_LOW DAMAGE
A palatial wall torch of some sort
Default light value is 300

.health = If you give the torch health, it can be shot out.  It will automatically select it's second skin (the beat-up torch look)
You must give it a targetname too, just any junk targetname will do like "junk"
----------------------------------
If triggered, will toggle between lightvalue1 and lightvalue2
"lightvalue1" (default 0) 
"lightvalue2" (default 11, equivalent to 300 brightness)
"abslight" You can give it explicit lighting so it doesn't glow (0 to 2.5)
Two values the light will fade-toggle between, 0 is black, 25 is brightest, 11 is equivalent to a value of 300.
"fadespeed" (default 1) = How many seconds it will take to complete the desired lighting change
The light will start on at a default of the higher light value unless you turn on the startlow flag.
START_LOW = will make the light start at the lower of the lightvalues you specify (default uses brighter)

NOTE: IF YOU DON'T PLAN ON USING THE DEFAULTS, ALL LIGHTS IN THE BANK OF LIGHTS NEED THIS INFO
--------------------------------------------------------
*/
void light_palace_torch (void)
{
	precache_model5("models/palight.mdl");
	precache_model ("models/flame2.mdl");
	if(!self.abslight)
		self.abslight = .75;
	self.drawflags(+)MLS_ABSLIGHT;

	self.mdl = "models/burner.mdl";
	self.weaponmodel = "models/palight.mdl";	//Flame On!

	self.movechain=spawn();
	setorigin(self.movechain,self.origin+'0 0 32');

	if(self.spawnflags&2)
		spawn_burnfield(self.movechain.origin);
	else
		self.dmg=0;

	self.thingtype	= THINGTYPE_BROWNSTONE;
	setsize(self, '-19 -19 0','19 19 35');

	self.solid=SOLID_BBOX;

//Put Flame on top

	self.movechain.abslight = .75;

	setmodel(self.movechain,"models/flame2.mdl");
	self.movechain.drawflags(+)MLS_ABSLIGHT;
	FireAmbient();

	Init_Torch();
	makestatic (self.movechain);
}
/*
 * $Log: /HexenWorld/HCode/torch.hc $
 * 
 * 6     4/20/98 2:03p Rjohnson
 * Big update
 * 
 * 5     4/06/98 6:41p Rjohnson
 * Minor fixes
 * 
 * 4     3/29/98 10:39p Rmidthun
 * Added MP palace_light
 * 
 * 3     3/29/98 10:19p Rmidthun
 * Added burner and support for burning hurt triggers on flames
 * 
 * 2     3/29/98 10:09p Rmidthun
 * Adding in MP lantern
 * 
 * 1     2/04/98 1:59p Rjohnson
 * 
 * 29    9/22/97 5:36p Rjohnson
 * Precache update
 * 
 * 28    9/02/97 3:06p Mgummelt
 * Put back in abslight for non-switchable torches
 * 
 * 27    9/02/97 2:55a Mgummelt
 * 
 * 26    8/21/97 5:10p Rjohnson
 * Change for ablight
 * 
 * 25    8/21/97 3:19p Rjohnson
 * Abslight change
 * 
 * 24    8/20/97 5:18p Rjohnson
 * Abslight for everything
 * 
 * 23    8/16/97 5:46p Mgummelt
 * 
 * 22    8/15/97 2:50p Rjohnson
 * Fix for precache
 * 
 * 21    8/13/97 5:53p Mgummelt
 * 
 * 20    7/19/97 9:56p Mgummelt
 * 
 * 19    7/14/97 4:46p Mgummelt
 * 
 * 18    6/18/97 4:00p Mgummelt
 * 
 * 17    6/16/97 8:05a Rlove
 * 
 * 16    6/15/97 5:10p Mgummelt
 * 
 * 15    6/06/97 11:35a Mgummelt
 * 
 * 14    6/05/97 8:51p Mgummelt
 * 
 * 12    5/27/97 7:58a Rlove
 * New thingtypes of GreyStone,BrownStone, and Cloth.
 * 
 * 11    5/21/97 1:35p Rlove
 * Changed roman torch back to old blocking
 * 
 * 10    5/12/97 9:02a Rlove
 * Changed blocking on roman torch
 * 
 * 8     4/01/97 5:03p Aleggett
 * Added flickering ability
 * 
 * 7     3/31/97 3:57p Aleggett
 * Improved breaking and toggling code
 * 
 * 6     3/31/97 9:07a Aleggett
 * 
 * 5     3/29/97 10:58a Aleggett
 * Nice-looking switchable torches, and nifty comments!
 * 
 * 4     3/24/97 9:51a Rlove
 * Changing blocking of meso torch
 * 
 * 3     3/20/97 4:54p Aleggett
 * Made switchable torches
 * 
 * 2     3/13/97 9:57a Rlove
 * Changed constant DAMAGE_AIM  to DAMAGE_YES and the old DAMAGE_YES to
 * DAMAGE_YES
 * 
 * 1     3/06/97 1:01p Aleggett
 */
