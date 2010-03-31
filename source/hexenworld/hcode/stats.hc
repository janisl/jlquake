/*
 * $Header: /HexenWorld/HCode/stats.hc 14    5/26/98 1:26a Mgummelt $
 */

// ExperienceValues for each level indicate the minimum at which
// you can become that level.  The 11th entry indicates the amount
// of experience needed for each level past 10


float ExperienceValues[55] =
{
	// Paladin
	945,			// Level 2 - spot 0
	2240,			// Level 3
	5250,			// Level 4
	10150,			// Level 5
	21000,			// Level 6
	39900,			// Level 7
	72800,			// Level 8
	120400,			// Level 9
	154000,			// Level 10
	210000,			// Level 11 - spot + 9
	210000,			// Required amount for each level afterwards

	// Crusader
	911,			// Level 2 - spot 11
	2160,			// Level 3
	5062,			// Level 4
	9787,			// Level 5
	20250,			// Level 6
	38475,			// Level 7
	70200,			// Level 8
	116100,			// Level 9
	148500,			// Level 10
	202500,			// Level 11 - spot + 9
	202500,			// Required amount for each level afterwards

	// Necromancer
	823,			// Level 2 - spot 22
	1952,			// Level 3
	4575,			// Level 4
	8845,			// Level 5
	18300,			// Level 6
	34770,			// Level 7
	63440,			// Level 8
	104920,			// Level 9
	134200,			// Level 10
	183000,			// Level 11 - spot + 9
	183000,			// Required amount for each level afterwards

	// Assassin
	675,			// Level 2 - spot 33
	1600,			// Level 3
	3750,			// Level 4
	7250,			// Level 5
	15000,			// Level 6
	28500,			// Level 7
	52000,			// Level 8
	86000,			// Level 9
	110000,			// Level 10
	150000,			// Level 11 - spot + 9
	150000,			// Required amount for each level afterwards

	// Succubus
	 871,			// Level 2 - spot 44
	2060,			// Level 3
	4822,			// Level 4
	9319,			// Level 5
	19278,			// Level 6
	36626,			// Level 7
	66804,			// Level 8
	110494,			// Level 9
	141334,			// Level 10
	192700,			// Level 11 - spot + 9
	192700			// Required amount for each level afterwards
};

//  min health, max health,
//  min health per level up to level 10,  min health per level up to level 10, 
//  health per level past level 10
/*
float hitpoint_table[25] =
{
	70,		85,				// Paladin
	8,		13,      4,

	65,		75,				// Crusader
	5,		10,      3,

	65,		75,				// Necromancer
	5,		10,      3,

	65,		75,				// Assassin
	5,		10,      3,

	65,		75,				// Succubus
	5,		10,      3
};
*/
float mana_table[25] =
{
//    Startup    Per Level     Past
//  min    max    min		max     10th Level
	84,		94,		6,		9, 		1,		// Paladin
	88,		98,		7,		10, 	2, 	// Crusader
    96,	   106,		10,		12, 	4,     // Necromancer
	92,	   102,		9,		11, 	3,		// Assassin
	90,	   100,		8,		11, 	3		// Succubus
};


float strength_table[10] =
{
	15,		18,		// Paladin
	12,		15,		// Crusader
	6,		10,		// Necromancer
	10,		13,		// Assassin
	11,		14		// Succubus
};

float intelligence_table[10] =
{
	6,		10,		// Paladin
	10,		13,		// Crusader
	15,		18,		// Necromancer
	6,		10,		// Assassin
	9,		13		// Succubus
};

float wisdom_table[10] =
{
	6,		10,		// Paladin
	15,		18,		// Crusader
	10,		13,		// Necromancer
	12,		15,		// Assassin
	11,		14		// Succubus
};

float dexterity_table[10] =
{
	10,		13,		// Paladin
	6,		10,		// Crusader
	8,		12,		// Necromancer
	15,		18,		// Assassin
	9,		13		// Succubus
};

void PlayerSpeed_Calc (void)
{
	switch (self.playerclass)
	{
	case CLASS_ASSASSIN:
	case CLASS_SUCCUBUS:
		self.hasted=1;
	break;
	case CLASS_PALADIN:
		self.hasted=.96;
	break;
	case CLASS_CRUSADER:
		self.hasted=.93;
	break;
	case CLASS_NECROMANCER:
		self.hasted=.9;
	break;
	}

	if (self.artifact_active & ART_HASTE)
		self.hasted *= 1.6;

	if (self.hull==HULL_CROUCH)   // Player crouched
		self.hasted *= .6;

}
/*
float CLASS_PALADIN					= 1;
float CLASS_CRUSADER				= 2;
float CLASS_NECROMANCER				= 3;
float CLASS_ASSASSIN				= 4;
float CLASS_SUCCUBUS				= 5;
*/

// Make sure we get a real distribution beteen
// min-max, otherwise, max will only get choosen when
// random() returns 1.0
float stats_compute(float min, float max)
{
	float value;

	value = (max-min+1)*random() + min;
	if (value > max) value = max;

	value = ceil(value);

	return value;
}

void stats_NewPlayer(entity e)
{
	float index;

	// Stats already set?
	if (e.strength) return;

	if (e.playerclass < CLASS_PALADIN || e.playerclass > CLASS_SUCCUBUS)
	{
		sprint(e,PRINT_MEDIUM, "Invalid player class ");
		sprint(e,PRINT_MEDIUM, ftos(e.playerclass));
		sprint(e,PRINT_MEDIUM, "\n");
		return;
	}

	// Calc initial health
	index = (e.playerclass - 1) * 5;
	e.health = stats_compute(hitpoint_table[index],
							 hitpoint_table[index+1]);
	e.max_health = e.health;

	// Calc initial mana
	index = (e.playerclass - 1) * 5;
	e.max_mana = stats_compute(mana_table[index],
							 mana_table[index+1]);


	index = (e.playerclass - 1) * 2;
	e.strength = stats_compute(strength_table[index],
							   strength_table[index+1]);
	e.intelligence = stats_compute(intelligence_table[index],
								   intelligence_table[index+1]);
	e.wisdom = stats_compute(wisdom_table[index],
							 wisdom_table[index+1]);
	e.dexterity = stats_compute(dexterity_table[index],
								dexterity_table[index+1]);

	e.level = 1;
	e.experience = 0;
}

// Jump ahead one level
void player_level_cheat()
{
	float index;

	index = (self.playerclass - 1) * (MAX_LEVELS+1);

	if (self.level > MAX_LEVELS)
		index += MAX_LEVELS - 1;
	else
		index += self.level - 1;

	self.experience = ExperienceValues[index];

	if (self.level > MAX_LEVELS)
		self.experience += (self.level - MAX_LEVELS) * ExperienceValues[index+1];

	PlayerAdvanceLevel(self.level+1);
}

void player_experience_cheat(void)
{
	AwardExperience(self,self,350);
}

/*
================
PlayerAdvanceLevel

This routine is called (from the game C side) when a player is advanced a level
(self.level)
================
*/
void PlayerAdvanceLevel(float NewLevel)
{
	string s2;
	float OldLevel,Diff;
	float index,HealthInc,ManaInc;

	sound (self, CHAN_VOICE, "misc/comm.wav", 1, ATTN_NONE);

	OldLevel = self.level;
	self.level = NewLevel;
	Diff = self.level - OldLevel;

	if(!Diff)
	{
		return;
	}

	sprint(self,PRINT_MEDIUM, "You are now level ");
	s2 = ftos(self.level);
	sprint(self,PRINT_MEDIUM, s2);
	sprint(self,PRINT_MEDIUM, "!\n");

	if(!self.newclass)
	{
		if (self.playerclass == CLASS_PALADIN)

		{
		   sprint(self,PRINT_MEDIUM, "Paladin gained a level\n");
		}
		else if (self.playerclass == CLASS_CRUSADER)
		{
		   sprint(self,PRINT_MEDIUM, "Crusader gained a level\n");

			// Special ability #1, full mana at level advancement
			self.bluemana = self.greenmana = self.max_mana;

		}
		else if (self.playerclass == CLASS_NECROMANCER)
		{
		   sprint(self,PRINT_MEDIUM, "Necromancer gained a level\n");
		}
		else if (self.playerclass == CLASS_ASSASSIN)
		{
		   sprint(self,PRINT_MEDIUM, "Assassin gained a level\n");

		}
		else if (self.playerclass == CLASS_SUCCUBUS)
		{
			sprint(self,PRINT_MEDIUM,"Demoness gained a level\n");
		}
/*
		switch (self.playerclass)
		{
		case CLASS_PALADIN:
		   centerprint(self, "Paladin gained a level\n");
		break;
		case CLASS_CRUSADER:
			centerprint(self,"Crusader gained a level\n");
			// Special ability #1, full mana at level advancement
			self.bluemana = self.greenmana = self.max_mana;
		break;
		case CLASS_NECROMANCER:
		   centerprint(self,"Necromancer gained a level\n");
		break;
		case CLASS_ASSASSIN:
		   centerprint(self,"Assassin gained a level\n");
		break;
		case CLASS_SUCCUBUS:
		   centerprint(self,"Demoness gained a level\n");
		break;
		}
*/
	}

	if (self.playerclass < CLASS_PALADIN ||
		self.playerclass > CLASS_SUCCUBUS)
		return;

	index = (self.playerclass - 1) * 5;

	// Have to do it this way in case they go up more than 1 level at a time
	while(Diff > 0)
	{
		OldLevel += 1;
		Diff -= 1;

		if (OldLevel <= MAX_LEVELS)
		{
			HealthInc = stats_compute(hitpoint_table[index+2],hitpoint_table[index+3]);
			ManaInc = stats_compute(mana_table[index+2],mana_table[index+3]);
		}
		else
		{
			HealthInc = hitpoint_table[index+4];
			ManaInc = mana_table[index+4];
		}
		self.health += HealthInc;
		self.max_health += HealthInc;

		//	An upper limit of 150 on health
		if (self.health > 150) 
			self.health = 150;

		if (self.max_health > 150)
			self.max_health = 150;


		self.greenmana += ManaInc;
		self.bluemana += ManaInc;
		self.max_mana += ManaInc;
		
		if(!deathmatch)
		{
			sprint(self, PRINT_LOW,"Stats: MP +");
			s2 = ftos(ManaInc);
			sprint(self, PRINT_LOW, s2);

			sprint(self, PRINT_LOW, "  HP +");
			s2 = ftos(HealthInc);
			sprint(self, PRINT_LOW, s2);
			sprint(self, PRINT_LOW, "\n");
		}
	}

	if (self.level > 2)
		self.flags(+)FL_SPECIAL_ABILITY1;

	if (self.level >5)
		self.flags(+)FL_SPECIAL_ABILITY2;

}


float FindLevel(entity WhichPlayer)
{
	float Chart;
	float Amount,Position,Level;

	if (WhichPlayer.playerclass < CLASS_PALADIN ||
		WhichPlayer.playerclass > CLASS_SUCCUBUS)
		return WhichPlayer.level;

	Chart = (WhichPlayer.playerclass - 1) * (MAX_LEVELS+1);
	//paladin : 0
	//crusader: 11
	//necro   : 22
	//assassin: 33
	//succubus: 44

	Level = 0;
	Position=0;
	while(Position < MAX_LEVELS && Level == 0)
	{
		if (WhichPlayer.experience < ExperienceValues[Chart+Position])
			Level = Position+1;

		Position += 1;
	}

	if (!Level)
	{
		Amount = WhichPlayer.experience - ExperienceValues[Chart + MAX_LEVELS - 1];
		Level = ceil(Amount / ExperienceValues[Chart + MAX_LEVELS]) + MAX_LEVELS;
	}

	return Level;
}


void AwardExperience(entity ToEnt, entity FromEnt, float Amount)
{
	float AfterLevel;
	float IsPlayer;
	entity SaveSelf;
	float index,test40,test80,diff,index2,totalnext,wis_mod;
	
	if (!Amount) 
		return;

	if(ToEnt.deadflag>=DEAD_DYING)
		return;
		
	IsPlayer = (ToEnt.classname == "player");

	if (!IsPlayer)
	{
		return;
	}

	if (FromEnt != world && Amount == 0.0)
	{
		Amount = FromEnt.experience_value;
	}

	if (ToEnt.level <4)
		Amount *= .5;

	if (ToEnt.playerclass == CLASS_PALADIN)
		Amount *= 1.4;
	else if (ToEnt.playerclass == CLASS_CRUSADER)
		Amount *= 1.35;
	else if (ToEnt.playerclass == CLASS_NECROMANCER)
		Amount *= 1.22;
	
	wis_mod = ToEnt.wisdom - 11;
	Amount+=Amount*wis_mod/20;//from .75 to 1.35

	if(ToEnt.experience+Amount>2500000)//Cap exp at 2.5 million, @ level 20
		return;

	ToEnt.experience += Amount;

	if (IsPlayer)
	{
		AfterLevel = FindLevel(ToEnt);

//		dprintf("Total Experience: %s\n",ToEnt.experience);
		if(AfterLevel>20)
			AfterLevel = 20;

		if (ToEnt.level != AfterLevel)
		{
			SaveSelf = self;
			self = ToEnt;

			PlayerAdvanceLevel(AfterLevel);

			self = SaveSelf;
		}
	}


	// Crusader Special Ability #1: award full health at 40% and 80% of levels experience
	if (IsPlayer)
	{
		if (ToEnt.playerclass == CLASS_CRUSADER)
		{
			index = (ToEnt.playerclass - 1) * (MAX_LEVELS+1);
			if ((ToEnt.level - 1) > MAX_LEVELS)
				index += MAX_LEVELS;
			else
				index += ToEnt.level - 1;

			if (ToEnt.level == 1)
			{
				test40 = ExperienceValues[index] * .4;
				test80 = ExperienceValues[index] * .8;
			}	
			else if ((ToEnt.level - 1) <= MAX_LEVELS)
			{			
				index2 = index - 1;		
				diff = ExperienceValues[index] - ExperienceValues[index2]; 
				test40 = ExperienceValues[index2] + (diff * .4);
				test80 = ExperienceValues[index2] + (diff * .8);
			}
			else // Past MAX_LEVELS
			{	
				totalnext = ExperienceValues[index - 1];   // index is 1 past MAXLEVEL at this point
				totalnext += ((ToEnt.level - 1) - MAX_LEVELS) * ExperienceValues[index];

				test40 = totalnext + (ExperienceValues[index] * .4);
				test80 = totalnext + (ExperienceValues[index] * .8);

			}
		
			if (((ToEnt.experience - Amount) < test40) && (ToEnt.experience> test40))
				ToEnt.health = ToEnt.max_health;
			else if (((ToEnt.experience - Amount) < test80) && (ToEnt.experience> test80))
				ToEnt.health = ToEnt.max_health;
		}
	}
}


/*
======================================
void stats_NewClass(entity e)
MG
Used when doing a quick changeclass
======================================
*/
void stats_NewClass(entity e)
{
entity oself;
float index,newlevel;

	if (e.playerclass < CLASS_PALADIN || e.playerclass > CLASS_SUCCUBUS)
	{
		sprint(e,PRINT_MEDIUM, "Invalid player class ");
		sprint(e,PRINT_MEDIUM, ftos(e.playerclass));
		sprint(e,PRINT_MEDIUM, "\n");
		return;
	}

	// Calc initial health
	index = (e.playerclass - 1) * 5;
	e.health = stats_compute(hitpoint_table[index],
							 hitpoint_table[index+1]);
	e.max_health = e.health;

	// Calc initial mana
	index = (e.playerclass - 1) * 5;
	e.max_mana = stats_compute(mana_table[index],
							 mana_table[index+1]);

	index = (e.playerclass - 1) * 2;
	e.strength = stats_compute(strength_table[index],
							   strength_table[index+1]);
	e.intelligence = stats_compute(intelligence_table[index],
								   intelligence_table[index+1]);
	e.wisdom = stats_compute(wisdom_table[index],
							 wisdom_table[index+1]);
	e.dexterity = stats_compute(dexterity_table[index],
								dexterity_table[index+1]);

	//Add level diff stuff
	newlevel = FindLevel(e);
	e.level=1;
	if(newlevel>1)
	{
		oself=self;
		self=e;
		PlayerAdvanceLevel(newlevel);
		self=oself;
	}
}

/*
======================================
drop_level
MG
Used in deathmatch where you don't
lose all exp, just enough to drop you
down one level.
======================================
*/

void drop_level (entity loser,float number)
{
float pos,lev_pos,new_exp,mana_dec,health_dec,dec_pos;
//string printnum;
	if(fixedLevel)
	{
		return;
	}

	if(loser.classname!="player")
		return;

/*	sprint(loser,PRINT_HIGH,"Dropping ");
	sprint(loser,PRINT_HIGH,loser.netname);
	sprint(loser,PRINT_HIGH," ");
	printnum = ftos(number);
	sprint(loser,PRINT_HIGH,printnum);
	sprint(loser,PRINT_HIGH," levels from L");
	printnum=ftos(loser.level);
	sprint(loser,PRINT_HIGH,printnum);
	sprint(loser,PRINT_HIGH," to L");
	if(loser.level - number < 1)
	{
		sprint(loser,PRINT_HIGH,"1");
	}
	else
	{
		printnum=ftos(loser.level-number);
		sprint(loser,PRINT_HIGH,printnum);
	}
	sprint(loser,PRINT_HIGH,"!\n");
*/
	if(loser.level-number<1)
	{//would drop below level 1, set to level 1
		loser.experience=0;
		dec_pos = (loser.playerclass - 1) * 5;
		loser.max_health= hitpoint_table[dec_pos];
		loser.max_mana = mana_table[dec_pos];
		if(loser.health>loser.max_health)
			loser.health=loser.max_health;
		if(loser.bluemana>loser.max_mana)
			loser.bluemana=loser.max_mana;
		if(loser.greenmana>loser.max_mana)
			loser.greenmana=loser.max_mana;
		return;
	}

	pos = (loser.playerclass - 1) * (MAX_LEVELS+1);
	if(loser.level-number>1)
	{
		loser.level-=number;
		lev_pos+=loser.level - 2;
		if(lev_pos>9)//last number in that char's 
		{
			new_exp=ExperienceValues[pos+10];
			loser.experience=new_exp+new_exp*(lev_pos - 9);
		}
		else
			loser.experience = ExperienceValues[pos+lev_pos];
	}
	else
	{
		loser.level=1;
		loser.experience=0;
	}

	if (loser.level <= 2)
		loser.flags(-)FL_SPECIAL_ABILITY1;

	if (loser.level <=5)
		loser.flags(-)FL_SPECIAL_ABILITY2;

	dec_pos = (loser.playerclass - 1) * 5;
	health_dec = hitpoint_table[dec_pos+4];
	mana_dec = mana_table[dec_pos+4];

	loser.max_health -= health_dec *number;
	if(loser.health>loser.max_health)
		loser.health=loser.max_health;

	loser.max_mana -= mana_dec *number;
	if(loser.bluemana>loser.max_mana)
		loser.bluemana=loser.max_mana;
	if(loser.greenmana>loser.max_mana)
		loser.greenmana=loser.max_mana;
}



/*
 * $Log: /HexenWorld/HCode/stats.hc $
 * 
 * 14    5/26/98 1:26a Mgummelt
 * 
 * 13    5/18/98 1:11p Mgummelt
 * 
 * 12    4/10/98 7:36p Mgummelt
 * 
 * 11    4/10/98 7:34p Mgummelt
 * Attempting to fix crazy super experience gain, capped level at 20, no
 * matter what
 * 
 * 10    4/09/98 1:57p Mgummelt
 * Some experience changes
 * 
 * 9     4/09/98 1:39p Ssengele
 * put in extra check to make sure i'm not awarding experience to world,
 * award exper. to right fella in t_damage for lava, frozen damage.
 * 
 * 8     4/08/98 7:10p Mgummelt
 * Added in script-side support for new spartanPrint option- only sends
 * chat and relevant death messages to clients
 * 
 * 7     4/07/98 9:59p Mgummelt
 * took out a couple sprints and made it so can't die from falling damage
 * 
 * 6     4/01/98 4:43p Rmidthun
 * fixed level dropping message
 * 
 * 5     3/30/98 2:12a Rmidthun
 * capped exp to 1 billion max, just in case
 * 
 * 4     3/29/98 6:47p Rmidthun
 * 
 * 3     3/26/98 4:58p Mgummelt
 * Fixed droplevel, added playerspeedcalc, added succubus stats
 * 
 * 10    3/17/98 11:02a Mgummelt
 * 
 * 9     3/16/98 6:21p Jweier
 * 
 * 8     3/13/98 3:02a Mgummelt
 * 
 * 7     3/12/98 11:06p Jmonroe
 * change ifs to switch
 * 
 * 6     2/24/98 6:39p Mgummelt
 * 
 * 5     2/13/98 11:16a Jmonroe
 * changed succubus to demoness
 * 
 * 4     1/21/98 12:12p Jweier
 * made level up more apparent
 * 
 * 27    10/28/97 1:01p Mgummelt
 * Massive replacement, rewrote entire code... just kidding.  Added
 * support for 5th class.
 * 
 * 24    9/10/97 11:40p Mgummelt
 * 
 * 23    9/10/97 7:51p Mgummelt
 * 
 * 22    9/10/97 7:08p Mgummelt
 * 
 * 21    9/03/97 7:49p Mgummelt
 * 
 * 20    8/15/97 3:59p Rlove
 * 
 * 19    8/11/97 4:35p Rlove
 * 
 * 18    8/09/97 10:51a Rlove
 * 
 * 17    7/26/97 8:39a Mgummelt
 * 
 * 16    7/25/97 11:45a Mgummelt
 * 
 * 15    7/25/97 11:12a Mgummelt
 * 
 * 14    7/25/97 11:10a Mgummelt
 * 
 * 13    7/14/97 2:29p Rlove
 * 
 * 12    7/08/97 5:17p Rlove
 * 
 * 11    7/03/97 10:07a Rlove
 * 
 * 10    6/30/97 3:33p Rlove
 * 
 * 9     6/30/97 9:41a Rlove
 * 
 * 8     6/20/97 9:25a Rlove
 * 
 * 7     6/20/97 9:12a Rlove
 * New mana system added
 * 
 * 6     6/06/97 2:52p Rlove
 * Artifact of Super Health now functions properly
 * 
 * 5     5/15/97 1:15p Rjohnson
 * Added the appriate experience tables and hitpoint advancement for level
 * gains
 * 
 * 4     5/15/97 11:43a Rjohnson
 * Stats updates
 * 
 * 3     5/14/97 4:12p Rjohnson
 * Minor fix from C-side conversion
 * 
 * 2     5/14/97 3:36p Rjohnson
 * Inital stats implementation
 * 
 * 1     5/13/97 2:23p Rjohnson
 * Initial Version
 */
