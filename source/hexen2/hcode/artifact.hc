/*
 * $Header: /H2 Mission Pack/HCode/artifact.hc 10    3/23/98 7:25p Jmonroe $
 */




/*
 * artifact_touch() -- Called when an artifact is being touched.
 *                     Awards players random amounts of whatever they represent.
 */
void() SUB_regen;
void() StartItem;
void ring_touch(void);

void artifact_touch()
{
	float amount;

	if(other.classname != "player"||other.model=="models/sheep.mdl")
	{ // Only players can take artifacts
		return;
	}
	if(other.health <= 0)
	{ // Player is dead
		return;
	}

	if (self.owner == other && self.artifact_ignore_owner_time > time)
		return;

	if (self.artifact_ignore_time > time) 
		return;

	// Take appropriate action
	if(self.netname == STR_TORCH)
	{
		if ((other.cnt_torch + 1)  > 15)
			return;	
		else
			other.cnt_torch += 1;		
	}
	else if(self.netname == STR_HEALTHBOOST)   // 25 limit
	{
		if ((other.cnt_h_boost + 1)  > 30||(other.playerclass!=CLASS_CRUSADER&&other.cnt_h_boost + 1  > 15))
			return;	
		else
			other.cnt_h_boost += 1; 
	}
	else if(self.netname == STR_SUPERHEALTHBOOST) // 5 limit
	{
		if (deathmatch&&(other.cnt_sh_boost + 1) > 2)
			return;	
		else if ((other.cnt_sh_boost + 1) > 5)
			return;	
		else
			other.cnt_sh_boost += 1; 
	}
	else if(self.netname == STR_MANABOOST)
	{
		if ((other.cnt_mana_boost + 1) > 15)
			return;	
		else
			other.cnt_mana_boost += 1; 
	}
	else if(self.netname == STR_TELEPORT)
	{
		if ((other.cnt_teleport + 1) > 15)
			return;	
		else
			other.cnt_teleport += 1;
	}
	else if(self.netname == STR_TOME)
	{
		if ((other.cnt_tome + 1)  > 15)
			return;	
		else
			other.cnt_tome += 1;
	}
	else if(self.netname == STR_SUMMON)
	{
		if ((other.cnt_summon + 1) > 15)
			return;	
		else
			other.cnt_summon += 1;
	}
	else if(self.netname == STR_INVISIBILITY)
	{
		if ((other.cnt_invisibility + 1) > 15)
			return;	
		else
			other.cnt_invisibility += 1;
	}
	else if(self.netname == STR_GLYPH)
	{
		if(other.playerclass==CLASS_CRUSADER)
		{
			if ((other.cnt_glyph + 5) > 50)
				return;	
			else	
				other.cnt_glyph += 5;
		}
		else
		{
			if ((other.cnt_glyph + 1) > 15)
				return;	
			else	
				other.cnt_glyph += 1;
		}
	}
	else if(self.netname == STR_HASTE)
	{
		if ((other.cnt_haste + 1)  > 15)
			return;	
		else
			other.cnt_haste += 1;
	}
	else if(self.netname == STR_BLAST)
	{
		if ((other.cnt_blast + 1)  > 15)
			return;	
		else
			other.cnt_blast += 1;
	}
	else if(self.netname == STR_POLYMORPH)
	{
		if ((other.cnt_polymorph + 1)  > 15)
			return;	
		else
			other.cnt_polymorph += 1;
	}
	else if(self.netname == STR_FLIGHT)
	{
		if ((other.cnt_flight + 1)  > 15)
			return;	
		else
			other.cnt_flight += 1;
	}
	else if(self.netname == STR_CUBEOFFORCE)
	{
		if ((other.cnt_cubeofforce + 1)  > 15)
			return;	
		else
			other.cnt_cubeofforce += 1;
	}
	else if(self.netname == STR_INVINCIBILITY)
	{
		if ((other.cnt_invincibility + 1)  > 15)
			return;	
		else
			other.cnt_invincibility += 1;
	}
	else if(self.classname == "art_sword_and_crown"&&other.team==2)
	{
		sound(self,CHAN_AUTO,"crusader/Lghtn2.mdl",1,ATTN_NONE);
		centerprint(other,"You are victorious!\n");
		bprint(other.netname);
		bprint(" has captured the Crown!\n");
	} 


	amount = random();
	if (amount < 0.5)
	{
		sprint (other, STR_YOUPOSSESS);
		sprint (other, self.netname);
	}
	else
	{
		sprint (other, STR_YOUHAVEACQUIRED);
		sprint (other, self.netname);
	}

	sprint (other,"\n");

	if (self.artifact_respawn)
	{
		self.mdl = self.model;
		if(self.netname==STR_INVINCIBILITY)
			thinktime self : 120;
		else if(self.netname==STR_INVISIBILITY)
			thinktime self : 90;
		else
			thinktime self : 60;
		self.think = SUB_regen;
	}

	sound(other, CHAN_VOICE, "items/artpkup.wav", 1, ATTN_NORM);
	stuffcmd(other, "bf\n");
	self.solid = SOLID_NOT;
//	other.items = other.items | self.items;
	self.model = string_null;

	activator = other;
	SUB_UseTargets(); // Fire all targets / killtargets

	if(!self.artifact_respawn)
	{
		remove(self);
	}
}


void Artifact_Cheat(void)
{
	self.cnt_sh_boost = 20;
	self.cnt_summon = 20;
	self.cnt_glyph = 20;
	self.cnt_blast = 20;
	self.cnt_polymorph = 20;
	self.cnt_flight = 20;
	self.cnt_cubeofforce = 20;
	self.cnt_invincibility = 20;
	self.cnt_invisibility = 20;
	self.cnt_haste = 20;
	self.cnt_mana_boost = 20;
	self.cnt_sh_boost = 20;
	self.cnt_h_boost = 20;
	self.cnt_teleport = 20;
	self.cnt_tome = 20;
	self.cnt_torch = 20;
}


/*-----------------------------------------
	GenerateArtifactModel - generate the artifact 
  -----------------------------------------*/
void GenerateArtifactModel(string modelname,string art_name,float respawnflag) 
{
	if (respawnflag)	// Should this thing respawn
	{
		self.artifact_respawn = deathmatch;
		
		if((art_name==STR_TOME||art_name==STR_MANABOOST)&&mapname=="tibet10")
			self.artifact_respawn = TRUE;
		else if((art_name==STR_HEALTHBOOST||art_name==STR_MANABOOST)&&skill>3)
			self.artifact_respawn = TRUE;	//jfm: this should help out a bit...
	}
	setmodel(self, modelname);
	self.netname = art_name;

	if (modelname == "models/ringft.mdl")
	{
		self.netname = "Ring of Flight";
		self.touch	 = ring_touch;
	}
	else //if (modelname != "models/a_xray.mdl")
		self.touch	 = artifact_touch;
	setsize (self, '0 0 0', '0 0 0');

	StartItem();
}


/*-----------------------------------------
	spawn_artifact - decide which artifact to spawn
  -----------------------------------------*/
void spawn_artifact (float artifact,float respawnflag)
{
	if (artifact == ARTIFACT_HASTE)
		GenerateArtifactModel("models/a_haste.mdl",STR_HASTE,respawnflag);
	else if (artifact == ARTIFACT_POLYMORPH)
		GenerateArtifactModel("models/a_poly.mdl",STR_POLYMORPH,respawnflag);
	else if (artifact == ARTIFACT_GLYPH)
		GenerateArtifactModel("models/a_glyph.mdl",STR_GLYPH,respawnflag);
	else if (artifact == ARTIFACT_INVISIBILITY)
		GenerateArtifactModel("models/a_invis.mdl",STR_INVISIBILITY,respawnflag);
	else if (artifact == ARTIFACT_INVINCIBILITY)
		GenerateArtifactModel("models/a_invinc.mdl",STR_INVINCIBILITY,respawnflag);
	else if (artifact == ARTIFACT_CUBEOFFORCE)
		GenerateArtifactModel("models/a_cube.mdl",STR_CUBEOFFORCE,respawnflag);
	else if (artifact == ARTIFACT_SUMMON)
		GenerateArtifactModel("models/a_summon.mdl",STR_SUMMON,respawnflag);
	else if (artifact == ARTIFACT_TOME)
		GenerateArtifactModel("models/a_tome.mdl",STR_TOME,respawnflag);
	else if (artifact == ARTIFACT_TELEPORT)
		GenerateArtifactModel("models/a_telprt.mdl",STR_TELEPORT,respawnflag);
	else if (artifact == ARTIFACT_MANA_BOOST)
		GenerateArtifactModel("models/a_mboost.mdl",STR_MANABOOST,respawnflag);
	else if (artifact == ARTIFACT_BLAST)
		GenerateArtifactModel("models/a_blast.mdl",STR_BLAST,respawnflag);
	else if (artifact == ARTIFACT_TORCH)
		GenerateArtifactModel("models/a_torch.mdl",STR_TORCH,respawnflag);
	else if (artifact == ARTIFACT_HP_BOOST)
		GenerateArtifactModel("models/a_hboost.mdl",STR_HEALTHBOOST,respawnflag);
	else if (artifact == ARTIFACT_SUPER_HP_BOOST)
		GenerateArtifactModel("models/a_shbost.mdl",STR_SUPERHEALTHBOOST,respawnflag);
	else if (artifact == ARTIFACT_FLIGHT)
		GenerateArtifactModel("models/ringft.mdl",STR_FLIGHT,respawnflag);
}


/*
====================================================================================================

SUPER HP BOOST

====================================================================================================
*/

void DecrementSuperHealth()
{
	float wait_time,over,decr_health;

	if (self.health > self.max_health)
	{
		if (self.health<200)
		{
			wait_time = 2;
			decr_health = 1;
		}
		else if (self.health<400)  // Vary rate of update time
		{
			decr_health = 1;
			over = 200 - (self.health - 200);
			wait_time = over/400;
			if (wait_time < .10)
				wait_time = .10;
		}
		else						// Vary the amount of the decrement
		{
			wait_time = .10;
			over = self.health - 400;
			decr_health = over * .016;
			decr_health = ceil(decr_health);
			if (decr_health < 2)
				decr_health = 2;
		}
		
		self.health = self.health - decr_health;

		self.healthtime = time + wait_time;
	}
	else  // All done, get rid of it
		self.artifact_flags (-) AFL_SUPERHEALTH;  

}


void use_super_healthboost()
{
	self.healthtime = time + .05;

	if(self.health<-100)
		self.health=1;
	else if(self.health<0)
		self.health+=100;
	else if (self.health < 899)
		self.health = self.health + 100;
	else if (self.health > 999)
		self.health = 999;

	self.cnt_sh_boost -= 1;
	self.artifact_flags(+)AFL_SUPERHEALTH;   // Show the health is in use

	if(self.flags2&FL2_POISONED)
	{
		self.flags2(-)FL2_POISONED;
		centerprint(self,"The poison has been cleansed from your blood...\n");
	}
}


/*QUAKED art_SuperHBoost (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for the Super Health Boost
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_SuperHBoost()
{
	spawn_artifact(ARTIFACT_SUPER_HP_BOOST,RESPAWN);
}




/*
====================================================================================================

HP BOOST

====================================================================================================
*/

void use_healthboost()
{
	if(self.health >= self.max_health)
	{ // Already at max health
		return;
	}
	self.cnt_h_boost -= 1;
	self.health += 25;
  	if(self.health > self.max_health)
	{
  		self.health = self.max_health;
	}
	if(self.flags2&FL2_POISONED)
	{
		self.flags2(-)FL2_POISONED;
		centerprint(self,"The poison has been cleansed from your blood...\n");
	}
}


/*QUAKED art_HealthBoost (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for the Health Boost
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_HealthBoost()
{
	spawn_artifact(ARTIFACT_HP_BOOST,RESPAWN);
}




/*
====================================================================================================

The TORCH

====================================================================================================
*/

/*QUAKED art_torch (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for the torch
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_torch()
{
	spawn_artifact(ARTIFACT_TORCH,RESPAWN);
}


void KillTorch()
{
	if(!self.artifact_active&ART_INVISIBILITY)
		self.effects(-)EF_DIMLIGHT;   // Turn off lights
	self.artifact_flags(-)AFL_TORCH;  // Turn off torch flag
	if(self.netname==STR_TORCH)
		remove(self);
	else
		self.cnt_torch	-= 1;
}

/*
void DouseTorch()//Never called?!
{//water?
	sound (self, CHAN_BODY, "raven/douse.wav", 1, ATTN_IDLE);
	self.torchtime = 0;
	KillTorch();
}
*/
void DimTorch()
{
	sound (self, CHAN_BODY, "raven/kiltorch.wav", 1, ATTN_IDLE);

	self.effects(-)EF_TORCHLIGHT;
	self.torchtime = time + 7;
	self.torchthink = KillTorch;
}


void FullTorch()
{
	sound (self, CHAN_BODY, "raven/fire1.wav", 1, ATTN_NORM);
	self.effects(+)EF_TORCHLIGHT;
	self.torchtime = time + 23;
	self.torchthink = DimTorch;
}

void thrown_torch_think ()
{//FIXME: If you pick it back up, it should still be lit and timing out
	if (self.torchtime < time)
		self.torchthink ();
	self.think=thrown_torch_think;
	thinktime self : 0.5;
}

void throw_torch (entity throwtorch)
{
	makevectors(self.v_angle);
	throwtorch.netname=STR_TORCH;
	throwtorch.torchtime = self.torchtime;
	if(self.effects&EF_DIMLIGHT)
		throwtorch.effects(+)EF_DIMLIGHT;
	if(self.effects&EF_TORCHLIGHT)
		throwtorch.effects(+)EF_TORCHLIGHT;
	throwtorch.torchthink=self.torchthink;

	throwtorch.think=thrown_torch_think;
	thinktime throwtorch : 0;

	if(!self.artifact_active&ART_INVISIBILITY)
		self.effects(-)EF_DIMLIGHT;   // Turn off lights
	self.artifact_flags(-)AFL_TORCH;  // Turn off torch flag
	self.effects(-)EF_TORCHLIGHT;
	self.torchtime = 0;
}

/*
============
TorchBurn

============
*/
void UseTorch()
{
	if((self.effects!=EF_DIMLIGHT) && (self.effects!=EF_TORCHLIGHT))
	{
		sound (self, CHAN_WEAPON, "raven/littorch.wav", 1, ATTN_NORM);

		self.effects(+)EF_DIMLIGHT;   // set player to emit light
		self.torchtime		= time + 1;
		self.torchthink		= FullTorch;
		self.artifact_flags (+) AFL_TORCH;   // Show the torch is in use
	}
}


/*QUAKED art_blastradius (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Blast Radius
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_blastradius()
{
	spawn_artifact(ARTIFACT_BLAST,RESPAWN);
}



void UseManaBoost()
{
	self.bluemana  = self.max_mana;
	self.greenmana = self.max_mana;

	self.cnt_mana_boost -= 1;
}


/*QUAKED art_manaboost (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Mana Boost
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_manaboost()
{
	spawn_artifact(ARTIFACT_MANA_BOOST,RESPAWN);
}


/*QUAKED art_teleport (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Teleportation
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_teleport()
{
	spawn_artifact(ARTIFACT_TELEPORT,RESPAWN);
}


/*QUAKED art_tomeofpower (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Tome of Power
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_tomeofpower()
{
	spawn_artifact(ARTIFACT_TOME,RESPAWN);
}


/*QUAKED art_summon (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Summoning
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_summon()
{
	spawn_artifact(ARTIFACT_SUMMON,RESPAWN);
}

/*QUAKED art_glyph (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Glyph of the Ancients
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_glyph()
{
	spawn_artifact(ARTIFACT_GLYPH,RESPAWN);
}


/*QUAKED art_haste (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Haste
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_haste()
{
	spawn_artifact(ARTIFACT_HASTE,RESPAWN);
}


/*QUAKED art_polymorph (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Polymorph
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_polymorph()
{
	spawn_artifact(ARTIFACT_POLYMORPH,RESPAWN);
}

/*QUAKED art_cubeofforce (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Cube Of Force
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_cubeofforce()
{
	spawn_artifact(ARTIFACT_CUBEOFFORCE,RESPAWN);
}


/*QUAKED art_invincibility (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Invincibility
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_invincibility()
{
	spawn_artifact(ARTIFACT_INVINCIBILITY,RESPAWN);
}

/*QUAKED art_invisibility (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Invisibility
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_invisibility()
{
	spawn_artifact(ARTIFACT_INVISIBILITY,RESPAWN);
}

void spawn_art_sword_and_crown(void)
{
	self.effects=EF_BRIGHTLIGHT;
	setmodel(self, "models/xcalibur.mdl");
	self.netname = "Sword";
	self.touch	 = artifact_touch;
	setsize (self, '-8 -8 -44', '8 8 20');

	StartItem();
}

/*QUAKED art_sword_and_crown (.0 .0 .5) (-8 -8 -44) (8 8 20) FLOATING
Artifact for Sword and Crown
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void art_sword_and_crown()
{
	precache_model2("models/xcalibur.mdl");
	self.artifact_respawn = deathmatch;
	spawn_art_sword_and_crown();
}


void item_spawner_use(void)
{
	DropBackpack();
}

/*QUAKED item_spawner (.0 .0 .5) (-8 -8 -44) (8 8 20) 
Generic item spawner
-------------------------FIELDS-------------------------
None
--------------------------------------------------------
*/
void item_spawner()
{
	setmodel(self, self.model);       // set size and link into world
	self.solid		= SOLID_NOT;
	self.movetype	= MOVETYPE_NONE;
	self.modelindex = 0;
	self.model		= "";
	self.effects = EF_NODRAW;
	
	self.use = item_spawner_use;
}

/*
 * $Log: /H2 Mission Pack/HCode/artifact.hc $
 * 
 * 10    3/23/98 7:25p Jmonroe
 * added item respawn in nightmare
 * 
 * 9     3/22/98 6:27p Jmonroe
 * fixed spiders for nightmare mode
 * 
 * 8     3/19/98 12:17a Mgummelt
 * last bug fixes
 * 
 * 7     2/12/98 5:55p Jmonroe
 * remove unreferenced funcs
 * 
 * 6     2/08/98 3:09p Mgummelt
 * 
 * 5     1/31/98 10:42p Mgummelt
 * 
 * 4     1/31/98 10:30p Mgummelt
 * 
 * 3     1/28/98 6:55p Mgummelt
 * 
 * 2     1/28/98 3:10p Mgummelt
 * 
 * 77    10/28/97 1:00p Mgummelt
 * Massive replacement, rewrote entire code... just kidding.  Added
 * support for 5th class.
 * 
 * 75    9/10/97 8:00p Mgummelt
 * 
 * 74    9/02/97 2:01a Rlove
 * 
 * 73    9/01/97 6:45p Mgummelt
 * 
 * 72    9/01/97 1:35a Mgummelt
 * 
 * 70    8/29/97 11:14p Mgummelt
 * 
 * 69    8/26/97 2:26a Mgummelt
 * 
 * 68    8/25/97 6:37p Rlove
 * 
 * 67    8/25/97 6:01p Rlove
 * 
 * 66    8/23/97 7:15p Rlove
 * 
 * 65    8/20/97 3:44p Mgummelt
 * 
 * 64    8/16/97 5:46p Mgummelt
 * 
 * 63    8/06/97 10:11p Mgummelt
 * 
 * 62    7/28/97 12:31p Rlove
 * Health doesn't count down as fast
 * 
 * 61    7/24/97 6:14p Rlove
 * Artifacts can no longer be used if the current one is still in use.
 * 
 * 60    7/24/97 3:53p Rlove
 * 
 * 59    7/24/97 11:29a Mgummelt
 * 
 * 58    7/24/97 3:26a Mgummelt
 * 
 * 57    7/21/97 3:03p Rlove
 * 
 * 56    7/19/97 2:30a Bgokey
 * 
 * 55    7/17/97 2:17p Mgummelt
 * 
 * 54    7/14/97 1:00p Mgummelt
 * 
 * 53    7/08/97 3:09p Rlove
 * 
 * 52    7/08/97 7:00a Rlove
 * 
 * 51    7/07/97 2:58p Mgummelt
 * 
 * 50    7/07/97 10:56a Mgummelt
 * 
 * 49    6/28/97 2:43p Rlove
 * 
 * 48    6/26/97 4:44p Rjohnson
 * Cheat gives you all artifacts
 * 
 * 47    6/20/97 9:43a Rlove
 * New mana system
 * 
 * 46    6/19/97 7:51a Rlove
 * 
 * 45    6/18/97 4:00p Mgummelt
 * 
 * 44    6/17/97 10:26a Rlove
 * 
 * 43    6/16/97 11:49p Bgokey
 * 
 * 42    6/16/97 6:40p Bgokey
 * 
 * 41    6/16/97 4:01p Rlove
 * 
 * 40    6/16/97 3:37p Rlove
 * Fixed the cheat so not everything is given (E3)
 * 
 * 39    6/16/97 3:00p Rlove
 * 
 * 38    6/13/97 10:11a Rlove
 * Moved all message.hc to strings.hc
 * 
 * 37    6/09/97 7:26a Rlove
 * Torch Artifact bit was being turned off without being checked if it was
 * on.
 * 
 * 36    6/06/97 2:52p Rlove
 * Artifact of Super Health now functions properly
 * 
 * 35    6/04/97 8:16p Mgummelt
 * 
 * 34    6/03/97 7:59a Rlove
 * Change take_art.wav to artpkup.wav
 * 
 * 33    5/27/97 3:52p Rjohnson
 * Added a generic item spawner
 * 
 * 32    5/23/97 12:22p Bgokey
 * 
 * 31    5/22/97 3:30p Mgummelt
 * 
 * 30    5/19/97 5:29p Rlove
 * 
 * 29    5/19/97 4:35p Rlove
 * 
 * 28    5/19/97 8:58a Rlove
 * Adding sprites and such to the axe.
 * 
 * 27    5/11/97 8:54p Mgummelt
 * 
 * 26    4/28/97 10:17a Rlove
 * New artifacts and items
 * 
 * 25    4/24/97 10:00p Rjohnson
 * Fixed problem with precache and spawning artifacts
 * 
 * 24    4/24/97 2:53p Rjohnson
 * Added backpack functionality and spawning of objects
 * 
 * 23    4/21/97 4:44p Rlove
 * Changed bounding box and color for artifacts
 * 
 * 22    4/15/97 8:46a Rlove
 * Weapon pick ups are working better.  Instant health is also working
 * 
 * 21    4/09/96 7:33p Mgummelt
 * 
 * 20    4/09/96 7:31p Mgummelt
 * 
 * 19    4/04/97 5:40p Rlove
 * 
 * 18    3/29/97 1:02p Aleggett
 * 
 * 17    2/13/97 3:22p Rlove
 * Blast Radius
 * 
 * 16    2/12/97 10:17a Rlove
 * 
 * 15    2/07/97 1:34p Rlove
 * Artifact of invincibility
 * 
 * 14    2/04/97 3:37p Rlove
 * Rewrote super health, made the code tighter
 * 
 * 13    2/04/97 3:05p Rlove
 * Rewrote the torch, doesn't use an entity anymore (what was I thinking?)
 * 
 * 12    12/18/96 3:14p Rlove
 * Mana system started
 * 
 * 11    12/18/96 11:50a Rlove
 * Changes for Health and Super Health use
 * 
 * 10    12/18/96 8:56a Rlove
 * Inventory screen is operational now
 * 
 * 9     12/16/96 12:42p Rlove
 * New gauntlets, artifacts, and inventory
 * 
 * 1     12/13/96 3:46p Rlove
 *
 * 8     12/09/96 4:02p Rlove
 * Inventory now keeps track of what the hero has
 *
 * 7     12/09/96 10:34a Rlove
 *
 * 6     12/02/96 8:47a Rlove
 * Added mana and blast radius artifacts
 *
 * 5     11/12/96 2:39p Rlove
 * Updates for the inventory system. Torch, HP boost and Super HP Boost.
 *
 * 4     11/11/96 1:03p Rlove
 * Put in Source Safe stuff
 */


