/*
 * $Header: /HexenWorld/HCode/sickle.hc 3     4/10/98 1:26p Ssengele $
 */

/*
==============================================================================

Q:\art\models\weapons\sickle\final\sickle.hc

==============================================================================
*/

// For building the model
$cd Q:\art\models\weapons\sickle\final
$origin 0 0 0
$base BASE skin
$skin skin
$flags 0


//
$frame rootpose     

//
$frame 1swipe4      1swipe5      
$frame 1swipe6      1swipe7      1swipe10     
$frame 1swipe14     
$frame 1swipe15     1swipe16     1swipe17     

//  frame 10 - 
$frame 2swipe1      2swipe2      2swipe3            
$frame 2swipe6      2swipe7      2swipe8      2swipe9          
$frame 2swipe12     2swipe13     2swipe14     

// frame 20
$frame 3swipe1      3swipe5      
$frame 3swipe7      3swipe8      3swipe9      3swipe10     
$frame 3swipe11     3swipe12     3swipe13     3swipe14     

//
$frame select1      select2      select3      select4      select5      
$frame select6      select7      select8      select9      select10     


void()sickle_decide_attack;


void sickle_fire ()
{
	vector	source;
	vector	org,dir;
	float damg, inertia;
	float damage_mod;
	float damage_base;
	float chance,point_chance,drain_ok;

	damage_mod = 10;

	makevectors (self.v_angle);
	source = self.origin + self.proj_ofs;
	traceline (source, source + v_forward*64, FALSE, self);
	if (trace_fraction == 1.0)  
	{
		traceline (source, source + v_forward*64 - (v_up * 30), FALSE, self);  // 30 down	
		if (trace_fraction == 1.0)  
		{
			traceline (source, source + v_forward*64 + v_up * 30, FALSE, self);  // 30 up
			if (trace_fraction == 1.0)  
				return;
		}
	}
	
	org = trace_endpos + (v_forward * 4);

	if (trace_ent.takedamage)
	{
		// Necromancer stands a chance of vampirically stealing health points
		if(teamplay && trace_ent.team == self.team)
			drain_ok=FALSE;
		else
			drain_ok=TRUE;

		if  (drain_ok && (trace_ent.flags & FL_MONSTER || trace_ent.flags & FL_CLIENT) && (self.level >= 6))	
		{
//			msg_entity=self;
//			WriteByte (MSG_ONE, SVC_SET_VIEW_TINT);
//			WriteByte (MSG_ONE, 168);

			chance = (self.level - 5) * .04;
			if (chance > .20)
				chance = .2;

			if (random() < chance)
			{
				point_chance = (self.level - 5) * 2;
				if (point_chance > 10)
					point_chance = 10;

				sound (self, CHAN_BODY, "weapons/drain.wav", 1, ATTN_NORM);

				self.health += point_chance;
				if (self.health>self.max_health)
					self.health = self.max_health;
			}
		}

		if (self.artifact_active & ART_TOMEOFPOWER)
		{
			damage_base = WEAPON1_PWR_BASE_DAMAGE;
			damage_mod = WEAPON1_PWR_ADD_DAMAGE;

			CreateWhiteFlash(org);
		
			if(trace_ent.mass<=10)
				inertia=1;
			else
				inertia=trace_ent.mass/10;

			if ((trace_ent.hull != HULL_BIG) && (inertia<1000) && (trace_ent.classname != "breakable_brush"))
			{
				if (trace_ent.mass < 1000)
				{
					dir =  trace_ent.origin - self.origin;
					trace_ent.velocity = dir * WEAPON1_PUSH*(1/inertia);
					if(trace_ent.movetype==MOVETYPE_FLY)
					{
						if(trace_ent.flags&FL_ONGROUND)
							trace_ent.velocity_z=200/inertia;
					}
					else
						trace_ent.velocity_z = 200/inertia;
					trace_ent.flags(-)FL_ONGROUND;
				}
			}	
		}
		else
		{
			damage_base = WEAPON1_BASE_DAMAGE;
			damage_mod = WEAPON1_ADD_DAMAGE;
		}

		damg = random(damage_mod + damage_base,damage_base);
		SpawnPuff (org, '0 0 0', damg,trace_ent);
		T_Damage (trace_ent, self, self, damg);

		if (!MetalHitSound(trace_ent.thingtype))
			sound (self, CHAN_WEAPON, "weapons/slash.wav", 1, ATTN_NORM);
	}
	else
	{	// hit wall
		sound (self, CHAN_WEAPON, "weapons/hitwall.wav", 1, ATTN_NORM);
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_GUNSHOT);
		WriteByte (MSG_MULTICAST, 1);
		WriteCoord (MSG_MULTICAST, org_x);
		WriteCoord (MSG_MULTICAST, org_y);
		WriteCoord (MSG_MULTICAST, org_z);
		multicast(self.origin,MULTICAST_PHS);

		if (self.artifact_active & ART_TOMEOFPOWER)
			CreateWhiteFlash(org);
		else
		{
			org = trace_endpos + (v_forward * -1) + (v_right * 15);
			org -= '0 0 26';
			CreateSpark (org);
		}
	}

}


void sickle_ready (void)
{
	self.th_weapon=sickle_ready;
	self.weaponframe = $rootpose;
}


void () sickle_c =
{

	self.th_weapon=sickle_c;
	self.wfs = advanceweaponframe($3swipe1,$3swipe14);

	if (self.weaponframe==$3swipe1)
		sound (self, CHAN_WEAPON, "weapons/gaunt1.wav", 1, ATTN_NORM);
	else if (self.weaponframe == $3swipe7)
		sickle_fire();
	if (self.wfs==WF_LAST_FRAME)
		sickle_ready();
};

void () sickle_b =
{
	self.th_weapon=sickle_b;
	self.wfs = advanceweaponframe($2swipe1,$2swipe14);

	if (self.weaponframe==$2swipe1)
		sound (self, CHAN_WEAPON, "weapons/gaunt1.wav", 1, ATTN_NORM);
	else if (self.weaponframe == $2swipe3)
		sickle_fire();
	else if (self.wfs == WF_LAST_FRAME)
		sickle_ready();
};

void () sickle_a =
{
	self.th_weapon=sickle_a;
	self.wfs = advanceweaponframe($1swipe4,$1swipe17);

	if (self.weaponframe==$1swipe4)
		sound (self, CHAN_WEAPON, "weapons/gaunt1.wav", 1, ATTN_NORM);
	else if (self.weaponframe == $1swipe5)
		sickle_fire();
	else if (self.wfs == WF_LAST_FRAME)
		sickle_ready();

};

void sickle_select (void)
{
	//selection sound?
	self.th_weapon=sickle_select;
	self.wfs = advanceweaponframe($select10,$select1);
	self.weaponmodel = "models/sickle.mdl";
	if(self.wfs==WF_CYCLE_STARTED)
		sound(self,CHAN_WEAPON,"weapons/unsheath.wav",1,ATTN_NORM);
	if (self.wfs==WF_CYCLE_WRAPPED)
	{
		self.attack_finished = time - 1;
		sickle_ready();
	}
}

void sickle_deselect (void)
{
	self.th_weapon=sickle_deselect;
	self.wfs = advanceweaponframe($select1,$select10);
	if (self.wfs==WF_CYCLE_WRAPPED)
		W_SetCurrentAmmo();
}

void sickle_decide_attack (void)
{
	if (self.attack_cnt < 1)
		sickle_a ();
	else 
	{
		sickle_b ();
		self.attack_cnt = -1;
	}

	self.attack_cnt += 1;
  	self.attack_finished = time + 0.5;
}
/*
 * $Log: /HexenWorld/HCode/sickle.hc $
 * 
 * 3     4/10/98 1:26p Ssengele
 * switched a few msg_broadcasts to multicasts
 * 
 * 2     2/09/98 3:24p Rjohnson
 * Update temp ents
 * 
 * 1     2/04/98 1:59p Rjohnson
 * 
 * 34    10/07/97 12:45p Mgummelt
 * 
 * 33    9/01/97 3:53p Rlove
 * 
 * 32    8/26/97 7:38a Mgummelt
 * 
 * 31    8/22/97 2:44p Jweier
 * 
 * 30    8/20/97 1:20p Rlove
 * 
 * 29    8/18/97 6:45a Rlove
 * 
 * 28    8/12/97 6:10p Mgummelt
 * 
 * 27    8/11/97 9:50a Rlove
 * 
 * 26    8/05/97 3:15p Rlove
 * 
 * 23    7/21/97 3:03p Rlove
 * 
 * 22    7/12/97 9:09a Rlove
 * Reworked Assassin Punch Dagger
 * 
 * 18    6/30/97 9:41a Rlove
 * 
 * 17    6/26/97 4:39p Rlove
 * Special ability #2 for Necromancer: Vampiric Drain
 * 
 * 16    6/18/97 7:33p Mgummelt
 * 
 * 15    6/09/97 11:20a Rlove
 * 
 * 14    6/05/97 9:29a Rlove
 * Weapons now have deselect animations
 * 
 * 13    5/27/97 10:58a Rlove
 * Took out old Id sound files
 * 
 * 12    5/06/97 1:29p Mgummelt
 * 
 * 11    5/03/97 12:59p Rlove
 * 
 * 10    5/02/97 8:05a Rlove
 * 
 * 9     4/28/97 6:54p Mgummelt
 * 
 * 8     4/25/97 8:32p Mgummelt
 * 
 * 7     4/24/97 8:49p Mgummelt
 * 
 * 6     4/11/96 1:51p Mgummelt
 * 
 * 5     4/09/97 2:41p Rlove
 * New Raven weapon sounds
 * 
 * 4     4/04/97 5:40p Rlove
 * 
 * 3     4/01/97 2:44p Rlove
 * New weapons, vorpal sword and purifier
 * 
 * 2     2/28/97 3:33p Rlove
 * New crossbow and sickle weapon models
 * 
 * 1     2/28/97 2:58p Rlove
 */
