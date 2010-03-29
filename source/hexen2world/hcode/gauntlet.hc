/*
==============================================================================

GAUNTLET

==============================================================================
*/

// For building the model
$cd q:/art/models/weapons/gauntlet/final
$origin 0 5 10
$base base4 512 473
$skin skin3

// FRAME: 1
$frame GntRoot1

// FRAMES: 2 - 14
$frame 1stGnt1      1stGnt2      1stGnt3      1stGnt4      1stGnt5      
$frame 1stGnt6      1stGnt7      1stGnt8      1stGnt9      1stGnt10     
$frame 1stGnt11     1stGnt12     1stGnt14     

// FRAMES 15 - 28
$frame 2ndGnt1      2ndGnt2      2ndGnt3      2ndGnt4      2ndGnt5
$frame 2ndGnt6      2ndGnt7      2ndGnt8      2ndGnt9      2ndGnt10     
$frame 2ndGnt11     2ndGnt12     2ndGnt13     2ndGnt15     
$frame 2ndGnt16     2ndGnt19     

// FRAMES 29 - 40
$frame 3rdGnt1      3rdGnt3      3rdGnt5      
$frame 3rdGnt10
$frame 3rdGnt11     3rdGnt12     3rdGnt13     3rdGnt14     3rdGnt15     
$frame 3rdGnt17               
$frame 3rdGnt21     3rdGnt22          

// Frames 41 - 54
$frame GntTap1      GntTap2      GntTap3      GntTap4      GntTap5
$frame GntTap11     GntTap12     GntTap13     GntTap14     GntTap15
$frame GntTap16     GntTap17     GntTap18     GntTap19     


// FRAMES: 55 - 67
$frame 7thGnt1      7thGnt2      7thGnt3            
$frame 7thGnt6      7thGnt7      7thGnt8      7thGnt9      7thGnt10     
$frame 7thGnt11     7thGnt12     7thGnt13     7thGnt14
$frame 7thGnt19      


float GAUNT_BASE_DAMAGE			= 16;
float GAUNT_ADD_DAMAGE			= 12;
float GAUNT_PWR_BASE_DAMAGE		= 30;
float GAUNT_PWR_ADD_DAMAGE		= 20;
float GAUNT_PUSH				= 4;

void W_SetCurrentWeapon(void);


void gauntlet_fire (float anim)
{
	vector	source;
	vector	org;
	float damg;
	entity	hitGuy;

	makevectors (self.v_angle);
	source = self.origin + self.proj_ofs;
	traceline (source, source + v_forward*64, FALSE, self);  // Straight in front

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
		SpawnPuff (org, '0 0 0', 20,trace_ent);

		if (self.artifact_active & ART_TOMEOFPOWER)
		{
			//damg = GAUNT_PWR_BASE_DAMAGE + random(GAUNT_PWR_ADD_DAMAGE);
			if(trace_ent.classname == "player")
			{
				damg = 30 + random(25);
			}
			else
			{
				damg = 2; // let the wall collision do the damage
			}
			org = trace_endpos + (v_forward * -1);
			CreateWhiteFlash(org);
			sound (self, CHAN_WEAPON, "weapons/gauntht1.wav", 1, ATTN_NORM);
		}
		else
		{
			damg = GAUNT_BASE_DAMAGE + random(GAUNT_ADD_DAMAGE);
			sound (self, CHAN_WEAPON, "weapons/gauntht1.wav", 1, ATTN_NORM);
		}

		hitGuy = trace_ent;

		T_Damage (trace_ent, self, self, damg);

		if(self.artifact_active & ART_TOMEOFPOWER)
		{
			hitGuy.velocity += normalize(hitGuy.origin - self.origin)*800;
			hitGuy.velocity_z += 600;
			hitGuy.flags(-)FL_ONGROUND;
		}


	}
	else
	{	// hit wall
		sound (self, CHAN_WEAPON, "weapons/gauntht2.wav", 1, ATTN_NORM);
		WriteByte (MSG_BROADCAST, SVC_TEMPENTITY);
		WriteByte (MSG_BROADCAST, TE_GUNSHOT);
		WriteByte (MSG_BROADCAST, 1);
		WriteCoord (MSG_BROADCAST, org_x);
		WriteCoord (MSG_BROADCAST, org_y);
		WriteCoord (MSG_BROADCAST, org_z);

		org = trace_endpos + (v_forward * -1);
		org += '0 0 10';
		CreateWhiteSmoke(org,'0 0 2',HX_FRAME_TIME);
	}
}

void gauntlet_ready (void)
{
	self.th_weapon=gauntlet_ready;
	self.weaponframe = $GntRoot1;
}

void gauntlet_twitch (void)
{
	self.wfs = advanceweaponframe($GntTap1,$GntTap19);
	self.th_weapon = gauntlet_twitch;

	if (self.weaponframe == $GntTap3)
		sound (self, CHAN_VOICE, "fx/wallbrk.wav", 1, ATTN_NORM);

	if (self.wfs == WF_LAST_FRAME)
		gauntlet_ready();
}

void gauntlet_select (void)
{
	self.wfs = advanceweaponframe($2ndGnt6,$2ndGnt1);
	self.weaponmodel = "models/gauntlet.mdl";
	self.th_weapon=gauntlet_select;
	self.last_attack=time;
	self.attack_cnt = 0;	

	if (self.wfs == WF_LAST_FRAME)
	{
		self.attack_finished = time - 1;
		gauntlet_twitch();
	}
}

void gauntlet_deselect (void)
{
	self.wfs = advanceweaponframe($2ndGnt1,$2ndGnt6);
	self.th_weapon=gauntlet_deselect;
	self.oldweapon = IT_WEAPON1;

	if (self.wfs == WF_LAST_FRAME)
	{
		W_SetCurrentAmmo();
	}
}

void gauntlet_d (void)
{
	self.wfs = advanceweaponframe($7thGnt3,$7thGnt19);
	self.th_weapon = gauntlet_d;

	if (self.weaponframe == $7thGnt6)	// Frame 57
		sound (self, CHAN_WEAPON, "weapons/gaunt1.wav", 1, ATTN_NORM);
	else if (self.weaponframe == $7thGnt9)	// Frame 63
		gauntlet_fire(4);

	if (self.wfs == WF_LAST_FRAME)
		gauntlet_ready();
}

void gauntlet_c (void) 
{
	self.wfs = advanceweaponframe($3rdGnt1,$3rdGnt22);
	self.th_weapon = gauntlet_c;

	if (self.weaponframe == $3rdGnt5)
		sound (self, CHAN_WEAPON, "weapons/gaunt1.wav", 1, ATTN_NORM);
	else if (self.weaponframe == $3rdGnt12)
		gauntlet_fire(3);

	if (self.wfs == WF_LAST_FRAME)
		gauntlet_ready();
}

void gauntlet_b (void)
{
	self.wfs = advanceweaponframe($2ndGnt1,$2ndGnt19);
	self.th_weapon = gauntlet_b;

	if ((self.weaponframe == $2ndGnt4) || (self.weaponframe == $2ndGnt5))
		self.weaponframe == $2ndGnt6;

	if (self.weaponframe == $2ndGnt6)
		sound (self, CHAN_WEAPON, "weapons/gaunt1.wav", 1, ATTN_NORM);
	else if (self.weaponframe == $2ndGnt9)
		gauntlet_fire(2);

	if (self.wfs == WF_LAST_FRAME)
		gauntlet_ready();
}

void gauntlet_a (void)
{
	self.wfs = advanceweaponframe($1stGnt1,$1stGnt14);
	self.th_weapon = gauntlet_a;

	if (self.weaponframe == $1stGnt2)
		sound (self, CHAN_WEAPON, "weapons/gaunt1.wav", 1, ATTN_NORM);
	else if (self.weaponframe == $1stGnt4)
		gauntlet_fire(1);

	if (self.wfs == WF_LAST_FRAME)
		gauntlet_ready();
}

void pal_gauntlet_fire(void)
{

//	r = random();   // Eventually attacks will be random in order

	if (self.attack_cnt < 1)
		gauntlet_a ();
	else if (self.attack_cnt < 2)
		gauntlet_b ();
	else if (self.attack_cnt < 3)
		gauntlet_c ();
	else if (self.attack_cnt < 4)
	{
		gauntlet_d ();
		self.attack_cnt=-1;
	}

	self.attack_cnt += 1;

  	self.attack_finished = time + 0.5;
}


/*
 * $Log: /HexenWorld/HCode/gauntlet.hc $
 * 
 * 5     3/10/98 2:59p Nalbury
 * Fixed up powered up gauntlet.
 * 
 * 4     3/07/98 12:45a Rjohnson
 * Fix
 * 
 * 3     3/06/98 6:46a Nalbury
 * added ability to tome of power punch
 * 
 * 2     2/05/98 5:00p Rjohnson
 * Temp entity changed
 * 
 * 1     2/04/98 1:59p Rjohnson
 * 
 * 58    8/31/97 9:42p Rlove
 * 
 * 57    8/26/97 7:38a Mgummelt
 * 
 * 56    8/20/97 5:47a Rlove
 * 
 * 55    8/19/97 7:53p Rlove
 * 
 * 54    8/19/97 3:13p Rlove
 * 
 * 53    8/12/97 6:10p Mgummelt
 * 
 * 52    8/08/97 6:21p Mgummelt
 * 
 * 51    8/05/97 11:10a Rlove
 * Slowed it down a little
 * 
 * 50    7/31/97 1:37p Mgummelt
 * 
 * 49    7/24/97 4:06p Rlove
 * 
 * 48    7/22/97 7:31a Rlove
 * 
 * 47    7/15/97 2:33p Mgummelt
 * 
 * 46    7/15/97 2:30p Mgummelt
 * 
 * 45    7/12/97 9:09a Rlove
 * Reworked Assassin Punch Dagger
 * 
 * 44    7/10/97 6:39p Rlove
 * 
 * 43    7/10/97 11:19a Rlove
 * 
 * 42    6/18/97 6:33p Mgummelt
 * 
 * 41    6/18/97 1:58p Mgummelt
 * 
 * 40    6/12/97 12:15p Rlove
 * 
 * 39    6/09/97 11:20a Rlove
 * 
 * 38    6/09/97 7:22a Rlove
 * ONGROUND was being turned off without being checked if it was turned
 * on.
 * 
 * 37    6/05/97 9:29a Rlove
 * Weapons now have deselect animations
 * 
 * 36    6/02/97 9:55a Rlove
 * Changed where firing is done
 * 
 * 35    6/02/97 9:45a Rlove
 * 
 * 34    5/23/97 2:54p Mgummelt
 * 
 * 33    5/15/97 5:05a Mgummelt
 * 
 * 32    5/15/97 12:30a Mgummelt
 * 
 * 31    5/13/97 3:25p Rlove
 * It now aims up or down 30 degrees
 * 
 * 30    5/12/97 11:06a Rlove
 * 
 * 29    5/12/97 11:06a Rlove
 * 
 * 27    5/12/97 10:31a Rlove
 * 
 * 26    5/06/97 1:29p Mgummelt
 * 
 * 25    5/05/97 10:29a Mgummelt
 * 
 * 24    4/26/97 6:30a Rlove
 * Added thingtype of CLAY for pots
 * 
 * 23    4/25/97 4:19p Rlove
 * 
 * 22    4/24/97 4:04p Rlove
 * 
 * 21    4/24/97 4:03p Rlove
 * 
 * 20    4/24/97 2:43p Rlove
 * Now it won't push everything
 * 
 * 19    4/21/97 9:16a Rlove
 * Tried new axe animations
 * 
 * 18    4/18/97 2:24p Rlove
 * Changed vorpal sword over to new weapon code
 * 
 * 17    4/18/97 12:06p Rlove
 * Adapted gauntlets to fit new weapons code
 * 
 * 16    4/18/97 11:44a Rlove
 * changed advanceweaponframe to return frame state
 * 
 * 15    4/18/97 11:16a Rlove
 * Added smoke sprite
 * 
 * 14    4/17/97 1:28p Rlove
 * added new built advanceweaponframe
 * 
 * 13    4/16/96 11:51p Mgummelt
 * 
 * 12    4/16/97 10:36a Rlove
 * Gauntlets now spin or move enemies depending on the gauntlet animation
 * 
 * 11    4/14/97 5:04p Rlove
 * 
 * 8     4/10/97 2:14p Rlove
 * Some tweaking of gauntlets and vorpal sword.
 * 
 * 7     4/09/97 2:41p Rlove
 * 
 * 6     4/04/97 5:40p Rlove
 * 
 * 5     4/01/97 2:44p Rlove
 * New weapons, vorpal sword and purifier
 * 
 */
