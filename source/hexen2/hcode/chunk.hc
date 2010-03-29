/*
 * $Header: /H2 Mission Pack/HCode/chunk.hc 18    3/01/98 3:12p Mgummelt $
 */
void ThrowSolidHead (float dm);

void blood_splatter()
{
	SpawnPuff(self.origin,normalize(self.velocity)*-20,10,self);
	remove(self);
}

void ThrowBlood (vector org,vector dir)
{
entity blood;
	blood=spawn_temp();
	blood.solid=SOLID_BBOX;
	blood.movetype=MOVETYPE_TOSS;
	blood.touch=blood_splatter;
	blood.velocity=dir;
	blood.avelocity=randomv('-700 -700 -700','700 700 700');
	blood.thingtype=THINGTYPE_FLESH;

	setmodel(blood,"models/bldspot4.spr");  // 8 x 8 sprite size
	setsize(blood,'0 0 0','0 0 0');
	setorigin(blood,org);
}

void ZeBrains (vector spot, vector normal, float scaling, float face, float roll)
{
	newmis=spawn();
	newmis.scale=scaling;
	newmis.angles=vectoangles(normal);
	if(face)
		newmis.angles_y+=180;
	newmis.angles_z=roll;

	setmodel(newmis,"models/brains.mdl");
	setsize(newmis,'0 0 0','0 0 0');
	setorigin(newmis,spot+normal*1);

	newmis.think=corpseblink;
	thinktime newmis : 30;

	spot=newmis.origin;
	makevectors(normal);
	ThrowBlood(spot,(normal+random(0.75,0.75)*v_up+random(0.75,0.75)*v_right)*random(200,400));
	ThrowBlood(spot,(normal+random(0.75,0.75)*v_up+random(0.75,0.75)*v_right)*random(200,400));
	ThrowBlood(spot,(normal+random(0.75,0.75)*v_up+random(0.75,0.75)*v_right)*random(200,400));
	ThrowBlood(spot,(normal+random(0.75,0.75)*v_up+random(0.75,0.75)*v_right)*random(200,400));
	ThrowBlood(spot,(normal+random(0.75,0.75)*v_up+random(0.75,0.75)*v_right)*random(200,400));
}

void ChunkRemove (void)
{
	chunk_cnt-=1;
	SUB_Remove ();
}

vector ChunkVelocity (void)
{
vector v;

	v_x = 300 * crandom();
	v_y = 300 * crandom();
	v_z = random(100,400);

	v = v * 0.7;
	return v;
}

/*
void ThrowSingleChunk (string chunkname,vector location,float life_time,float skinnum)
{
	entity chunk;

	if (chunk_cnt < CHUNK_MAX)
	{
		chunk=spawn_temp();
		setmodel (chunk, chunkname);
		chunk.frame = 0;

		setsize (chunk, '0 0 0', '0 0 0');
		chunk.movetype = MOVETYPE_BOUNCE;
		chunk.solid = SOLID_NOT;
		chunk.takedamage = DAMAGE_NO;
		chunk.velocity = ChunkVelocity();
		chunk.think = ChunkRemove;
		chunk.flags(-)FL_ONGROUND;
		chunk.origin = location;
	
		chunk.avelocity_x = random(10);
		chunk.avelocity_y = random(10);
		chunk.avelocity_z = random(30);
		chunk.skin = skinnum;
		chunk.ltime = time;
		thinktime chunk : life_time;
		chunk_cnt+=1;
	}
}
*/

void MeatChunks (vector org,vector dir,float chunk_count,entity loser)
{
float final,t_type;
entity chunk;

	if(deathmatch||coop)
	{
		if(dir=='0 0 0')
		{
			dir = ChunkVelocity();
			dir=loser.velocity+dir;
		}
		if(loser.frozen>0)
			t_type=THINGTYPE_ICE;
		else if(loser.model=="models/spider.mdl")
			t_type=THINGTYPE_GREENFLESH;
		else
			t_type=loser.thingtype;
		starteffect(CE_CHUNK, org, t_type, dir, chunk_count);
	}
	else while(chunk_count)
	{
		chunk=spawn_temp();
		chunk_count-=1;
		final = random();

		if(loser.frozen>0)
			setmodel (chunk, "models/shardice.mdl");
		else if(loser.model=="models/spider.mdl")
		{
			if (final < 0.33)
				setmodel (chunk, "models/sflesh1.mdl");
			else if (final < 0.66)
				setmodel (chunk, "models/sflesh2.mdl");
			else
				setmodel (chunk, "models/sflesh3.mdl");
		}
		else if (final < 0.33)
			setmodel (chunk, "models/flesh1.mdl");
		else if (final < 0.66)
			setmodel (chunk, "models/flesh2.mdl");
		else
			setmodel (chunk, "models/flesh3.mdl");
		setsize (chunk, '0 0 0', '0 0 0');
//		chunk.skin=1;
		chunk.movetype = MOVETYPE_BOUNCE;
		chunk.solid = SOLID_NOT;
		chunk.think = ChunkRemove;
		if(dir=='0 0 0')
		{
			dir = ChunkVelocity();
			dir=loser.velocity+dir;
		}
		else
			chunk.velocity=dir;
		chunk.avelocity_x = random(1200);
		chunk.avelocity_y = random(1200);
		chunk.avelocity_z = random(1200);

		chunk.scale = .45;

		chunk.ltime = time;
		thinktime chunk : random(2);
		setorigin (chunk, org);
	}
}

void chunk_hurt ()
{
float damage;
	if(!other.takedamage)
		return;

	if(self.attack_finished>time)
		return;
	
	if(self.velocity=='0 0 0')
		return;

	//SOUND
	self.attack_finished = time + 0.5;
	damage = self.scale * vlen(self.velocity)/100 * self.dmg;
	T_Damage(other,self,self.owner,damage);
}

void CreateModelChunks (vector space,float scalemod, float numChunks)
{
	entity chunk;
	float final, tried,t_type;
	vector chunk_vel,org;
	//return;//Magical Network-Friendly Code!

	chunk_vel = ChunkVelocity();
	if(!self.flags&FL_ONGROUND&&self.movetype!=MOVETYPE_NONE)
		chunk_vel=self.velocity+chunk_vel;

	if(deathmatch||coop)
	{
		if(self.origin=='0 0 0'&&self.solid==SOLID_BSP)
			org=(self.absmin+self.absmax)*0.5;
		else
			org=self.origin;
		if(self.frozen>0)
			t_type=THINGTYPE_ICE;
		else if(self.model=="models/spider.mdl")
			t_type=THINGTYPE_GREENFLESH;
		else
			t_type=self.thingtype;
		starteffect(CE_CHUNK, org, t_type, chunk_vel, numChunks);
		return;
	}

	chunk = spawn_temp();

	space_x = space_x * random();
	space_y = space_y * random();

	if(self.solid==SOLID_TRIGGER&&self.thingtype!=THINGTYPE_WEBS)//Trigger event
	{
		traceline(self.absmin + space, self.absmin + space + '0 0 1' * self.maxs_z,TRUE,self);
		tried = 0;
		while((trace_startsolid||pointcontents(trace_endpos)!=CONTENT_EMPTY) && tried < 20)
		{
			space_x = space_x * random();
			space_y = space_y * random();
			traceline(self.absmin + space, self.absmin + space + '0 0 1' * self.maxs_z,TRUE,self);
			tried+=1;
		}
		if(tried == 20)
			return;
		space_z = trace_endpos_z;
		setorigin (chunk, trace_endpos);
		chunk.solid = SOLID_BBOX;
		chunk.touch = chunk_hurt;
	}
	else
	{
		space_z = space_z * random();
		setorigin (chunk, self.absmin + space);
		chunk.solid = SOLID_NOT;
	}

	final = random();
	if ((self.thingtype==THINGTYPE_GLASS) || (self.thingtype==THINGTYPE_REDGLASS) || 
			(self.thingtype==THINGTYPE_CLEARGLASS) || (self.thingtype==THINGTYPE_WEBS))
	{
		if (final<0.20)
			setmodel (chunk, "models/shard1.mdl");
		else if (final<0.40)
			setmodel (chunk, "models/shard2.mdl");
		else if (final<0.60)
			setmodel (chunk, "models/shard3.mdl");
		else if (final<0.80)
			setmodel (chunk, "models/shard4.mdl");
		else 
			setmodel (chunk, "models/shard5.mdl");

		if (self.thingtype==THINGTYPE_CLEARGLASS)
		{
			chunk.skin=1;
			chunk.drawflags (+) DRF_TRANSLUCENT;
		}
		else if (self.thingtype==THINGTYPE_REDGLASS)
			chunk.skin=2;
		else if (self.thingtype==THINGTYPE_WEBS)
		{
			chunk.skin=3;
			chunk.drawflags (+) DRF_TRANSLUCENT;
			if(self.drawflags&MLS_ABSLIGHT)
			{
				chunk.drawflags(+)MLS_ABSLIGHT;
				chunk.abslight=self.abslight;
			}
			chunk_vel*=.1;
			chunk.gravity=random(0.3,0.8);
		}
	}
	else if (self.thingtype==THINGTYPE_WOOD)
	{
		if (final < 0.25)
			setmodel (chunk, "models/splnter1.mdl");
		else if (final < 0.50)
			setmodel (chunk, "models/splnter2.mdl");
		else if (final < 0.75)
			setmodel (chunk, "models/splnter3.mdl");
		else 
			setmodel (chunk, "models/splnter4.mdl");
	}
	else if (self.thingtype==THINGTYPE_METAL)
	{
		if (final < 0.25)
			setmodel (chunk, "models/metlchk1.mdl");
		else if (final < 0.50)
			setmodel (chunk, "models/metlchk2.mdl");
		else if (final < 0.75)
			setmodel (chunk, "models/metlchk3.mdl");
		else 
			setmodel (chunk, "models/metlchk4.mdl");
	}
	else if (self.thingtype==THINGTYPE_FLESH)
	{
		if(self.model=="models/spider.mdl")
		{
			if (final < 0.33)
				setmodel (chunk, "models/sflesh1.mdl");
			else if (final < 0.66)
				setmodel (chunk, "models/sflesh2.mdl");
			else
				setmodel (chunk, "models/sflesh3.mdl");
		}
		else if (final < 0.33)
			setmodel (chunk, "models/flesh1.mdl");
		else if (final < 0.66)
			setmodel (chunk, "models/flesh2.mdl");
		else
			setmodel (chunk, "models/flesh3.mdl");
		if(self.classname=="hive")
			chunk.skin=1;
	}
	else if (self.thingtype==THINGTYPE_BROWNSTONE)
	{
		if (final < 0.25)
			setmodel (chunk, "models/schunk1.mdl");
		else if (final < 0.50)
			setmodel (chunk, "models/schunk2.mdl");
		else if (final < 0.75)
			setmodel (chunk, "models/schunk3.mdl");
		else 
			setmodel (chunk, "models/schunk4.mdl");
		chunk.skin = 1;
	}
	else if ((self.thingtype==THINGTYPE_CLAY) || (self.thingtype==THINGTYPE_BONE))
	{
		if (final < 0.25)
			setmodel (chunk, "models/clshard1.mdl");
		else if (final < 0.50)
			setmodel (chunk, "models/clshard2.mdl");
		else if (final < 0.75)
			setmodel (chunk, "models/clshard3.mdl");
		else 
			setmodel (chunk, "models/clshard4.mdl");
		if (self.thingtype==THINGTYPE_BONE)
		{
			chunk.skin = 1;
		}
	}
	else if (self.thingtype==THINGTYPE_LEAVES)
	{
		if (final < 0.33)
			setmodel (chunk, "models/leafchk1.mdl");
		else if (final < 0.66)
			setmodel (chunk, "models/leafchk2.mdl");
		else 
			setmodel (chunk, "models/leafchk3.mdl");
	}
	else if (self.thingtype==THINGTYPE_HAY)
	{
		if (final < 0.33)
			setmodel (chunk, "models/hay1.mdl");
		else if (final < 0.66)
			setmodel (chunk, "models/hay2.mdl");
		else 
			setmodel (chunk, "models/hay3.mdl");
	}
	else if (self.thingtype==THINGTYPE_CLOTH)
	{
		if (final < 0.33)
			setmodel (chunk, "models/clthchk1.mdl");
		else if (final < 0.66)
			setmodel (chunk, "models/clthchk2.mdl");
		else 
			setmodel (chunk, "models/clthchk3.mdl");
	}
	else if (self.thingtype==THINGTYPE_WOOD_LEAF)
	{
		if (final < 0.14)
			setmodel (chunk, "models/splnter1.mdl");
		else if (final < 0.28)
			setmodel (chunk, "models/leafchk1.mdl");
		else if (final < 0.42)
			setmodel (chunk, "models/splnter2.mdl");
		else if (final < 0.56)
			setmodel (chunk, "models/leafchk2.mdl");
		else if (final < 0.70)
			setmodel (chunk, "models/splnter3.mdl");
		else if (final < 0.84)
			setmodel (chunk, "models/leafchk3.mdl");
		else 
			setmodel (chunk, "models/splnter4.mdl");
	}
	else if (self.thingtype==THINGTYPE_WOOD_METAL)
	{
		if (final < 0.125)
			setmodel (chunk, "models/splnter1.mdl");
		else if (final < 0.25)
			setmodel (chunk, "models/metlchk1.mdl");
		else if (final < 0.375)
			setmodel (chunk, "models/splnter2.mdl");
		else if (final < 0.50)
			setmodel (chunk, "models/metlchk2.mdl");
		else if (final < 0.625)
			setmodel (chunk, "models/splnter3.mdl");
		else if (final < 0.75)
			setmodel (chunk, "models/metlchk3.mdl");
		else if (final < 0.875)
			setmodel (chunk, "models/splnter4.mdl");
		else 
			setmodel (chunk, "models/metlchk4.mdl");
	}
	else if (self.thingtype==THINGTYPE_WOOD_STONE)
	{
		if (final < 0.125)
			setmodel (chunk, "models/splnter1.mdl");
		else if (final < 0.25)
			setmodel (chunk, "models/schunk1.mdl");
		else if (final < 0.375)
			setmodel (chunk, "models/splnter2.mdl");
		else if (final < 0.50)
			setmodel (chunk, "models/schunk2.mdl");
		else if (final < 0.625)
			setmodel (chunk, "models/splnter3.mdl");
		else if (final < 0.75)
			setmodel (chunk, "models/schunk3.mdl");
		else if (final < 0.875)
			setmodel (chunk, "models/splnter4.mdl");
		else 
			setmodel (chunk, "models/schunk4.mdl");
	}
	else if (self.thingtype==THINGTYPE_METAL_STONE)
	{
		if (final < 0.125)
			setmodel (chunk, "models/metlchk1.mdl");
		else if (final < 0.25)
			setmodel (chunk, "models/schunk1.mdl");
		else if (final < 0.375)
			setmodel (chunk, "models/metlchk2.mdl");
		else if (final < 0.50)
			setmodel (chunk, "models/schunk2.mdl");
		else if (final < 0.625)
			setmodel (chunk, "models/metlchk3.mdl");
		else if (final < 0.75)
			setmodel (chunk, "models/schunk3.mdl");
		else if (final < 0.875)
			setmodel (chunk, "models/metlchk4.mdl");
		else 
			setmodel (chunk, "models/schunk4.mdl");
	}
	else if (self.thingtype==THINGTYPE_METAL_CLOTH)
	{
		if (final < 0.14)
			setmodel (chunk, "models/metlchk1.mdl");
		else if (final < 0.28)
			setmodel (chunk, "models/clthchk1.mdl");
		else if (final < 0.42)
			setmodel (chunk, "models/metlchk2.mdl");
		else if (final < 0.56)
			setmodel (chunk, "models/clthchk2.mdl");
		else if (final < 0.70)
			setmodel (chunk, "models/metlchk3.mdl");
		else if (final < 0.84)
			setmodel (chunk, "models/clthchk3.mdl");
		else 
			setmodel (chunk, "models/metlchk4.mdl");
	}
	else if (self.thingtype==THINGTYPE_ICE)
	{
		setmodel(chunk,"models/shard.mdl");
		chunk.skin=0;
		chunk.frame=random(2);
		chunk.drawflags(+)DRF_TRANSLUCENT|MLS_ABSLIGHT;
		chunk.abslight=0.5;
	}
	else// if (self.thingtype==THINGTYPE_GREYSTONE)
	{
		if (final < 0.25)
			setmodel (chunk, "models/schunk1.mdl");
		else if (final < 0.50)
			setmodel (chunk, "models/schunk2.mdl");
		else if (final < 0.75)
			setmodel (chunk, "models/schunk3.mdl");
		else 
			setmodel (chunk, "models/schunk4.mdl");
		chunk.skin = 0;
	}

	if(self.solid==SOLID_TRIGGER)//Trigger event
	{
		setsize(chunk,'-1 -1 -1', '1 1 1');
		chunk.hull = HULL_POINT;
		thinktime chunk :  random(2)+2;
		chunk.dmg = self.dmg;
	}
	else
	{
		setsize (chunk, '0 0 0', '0 0 0');
		thinktime chunk :  random(2);
	}

	chunk.movetype = MOVETYPE_BOUNCE;
	chunk.think = ChunkRemove;
	chunk.velocity=chunk_vel;
	chunk.avelocity_x = random(1200);
	chunk.avelocity_y = random(1200);
	chunk.avelocity_z = random(1200);

	if(self.classname=="monster_eidolon")
		chunk.scale=random(2.1,2.5);
	else
		chunk.scale = random(scalemod,scalemod + .1);

	chunk.ltime = time;
}

void DropBackpack(void);  // in items.hc


// Put a little splat down if it will fit
/*
void TinySplat (vector location)
{
	vector holdplane;
	entity splat;

	traceline (location + v_up*8 + v_right * 8 + v_forward * 8,location - v_up*32 + v_right * 8 + v_forward * 8, TRUE, self);
	holdplane = trace_plane_normal;
	if(trace_fraction==1)	// Nothing below victim
		return;

	traceline (location + v_up*8 - v_right * 8 + v_forward * 8,location - v_up*32 - v_right * 8 + v_forward * 8, TRUE, self);
	if ((holdplane != trace_plane_normal) || (trace_fraction==1))
		return;

	traceline (location + v_up*8 + v_right * 8 - v_forward * 8,location - v_up*32 + v_right * 8 - v_forward * 8, TRUE, self);
	if ((holdplane != trace_plane_normal) || (trace_fraction==1))
		return;

	traceline (location + v_up*8 - v_right * 8 - v_forward * 8,location - v_up*32 - v_right * 8 - v_forward * 8, TRUE, self);
	if ((holdplane != trace_plane_normal) || (trace_fraction==1))
		return;

	traceline (location + v_up*8 ,location - v_up*32 , TRUE, self);

    splat=spawn();
    splat.owner=self;
    splat.classname="bloodsplat";
    splat.movetype=MOVETYPE_NONE;
    splat.solid=SOLID_NOT;

	// Flat to the surface
	trace_plane_normal_x = trace_plane_normal_x * -1;
	trace_plane_normal_y = trace_plane_normal_y * -1;
	splat.angles = vectoangles(trace_plane_normal);

    setmodel(splat,"models/bldspot4.spr");  // 8 x 8 sprite
    setsize(splat,'0 0 0','0 0 0');
    setorigin(splat,trace_endpos + '0 0 2');

}
*/
/*
void BloodSplat(void)
{
	entity splat;
	vector holdangles;

	if (random() < .5)
	{
		holdangles_x = random(-30,-20);
		holdangles_y = random(30,20);
	}
	else
	{
		holdangles_x = random(30,20);
		holdangles_y = random(-30,-20);
	}

	holdangles_z = 16;
	TinySplat (self.origin + holdangles);

	if (random() < .5)
	{
		holdangles_x = random(-30,-10);
		holdangles_y = random(30,10);
	}
	else
	{
		holdangles_x = random(30,10);
		holdangles_y = random(-30,-10);
	}

	holdangles_z = 16;
	TinySplat (self.origin + holdangles);

	makevectors (self.angles);

	traceline (self.origin + v_up*8,self.origin - v_up*32, TRUE, self);

	if(trace_fraction==1)	// Nothing below victim
	{
		dprint("\n no floor ");
		return;
	}

    splat=spawn();
    splat.owner=self;
    splat.classname="bloodsplat";
    splat.movetype=MOVETYPE_NONE;
    splat.solid=SOLID_NOT;

	// Flat to the surface
	trace_plane_normal_x = trace_plane_normal_x * -1;
	trace_plane_normal_y = trace_plane_normal_y * -1;
	splat.angles = vectoangles(trace_plane_normal);

//  setmodel(splat,"models/bldspot1.spr");  // 30 x 30 sprite size
    setmodel(splat,"models/bldspot2.spr");  // 20 x 20 sprite size
//  setmodel(splat,"models/bldspot3.spr");  // 18 x 18 sprite size
//  setmodel(splat,"models/bldspot4.spr");  // 8 x 8 sprite size
    setsize(splat,'0 0 0','0 0 0');
    setorigin(splat,trace_endpos + '0 0 2');

}
*/
void chunk_reset ()
{
	chunk_cnt=FALSE;
	remove(self);
}

void make_chunk_reset ()
{
	newmis=spawn();
	newmis.think=chunk_reset;
	thinktime newmis : 1.5;
}

void chunk_death (void)
{
	vector space;
	float spacecube,model_cnt,scalemod;
	string deathsound;

	DropBackpack();

//	BloodSplat();

	space = self.absmax - self.absmin;

	spacecube = space_x * space_y * space_z;

	model_cnt = spacecube / 8192;   // (16 * 16 * 16)

	if ((self.thingtype==THINGTYPE_GLASS) || (self.thingtype==THINGTYPE_CLEARGLASS) || (self.thingtype==THINGTYPE_REDGLASS))
		deathsound="fx/glassbrk.wav";
	else if ((self.thingtype==THINGTYPE_WOOD) || (self.thingtype==THINGTYPE_WOOD_METAL))
		if(self.classname=="bolt")
			deathsound="assassin/arrowbrk.wav";
		else	
			deathsound="fx/woodbrk.wav";
	else if ((self.thingtype==THINGTYPE_GREYSTONE) || (self.thingtype==THINGTYPE_BROWNSTONE) || 
		(self.thingtype==THINGTYPE_WOOD_STONE) || (self.thingtype==THINGTYPE_METAL_STONE))
		deathsound="fx/wallbrk.wav";
	else if ((self.thingtype==THINGTYPE_METAL) || (self.thingtype==THINGTYPE_METAL_CLOTH))
		deathsound="fx/metalbrk.wav";
	else if ((self.thingtype==THINGTYPE_CLOTH) || (self.thingtype==THINGTYPE_REDGLASS))
		deathsound="fx/clothbrk.wav";
	else if (self.thingtype==THINGTYPE_FLESH)//||(self.thingtype==THINGTYPE_ACID&&self.flags2&FL_ALIVE))
	{
		//Made temporary changes to make weapons look and sound
		//better, more blood and gory sounds.
		if(self.health<random(-40,-20))
			deathsound="player/megagib.wav";
		else if(random()<0.5)
			deathsound="player/gib1.wav";
		else
			deathsound="player/gib2.wav";
		sound(self,CHAN_ITEM,deathsound,1,ATTN_NORM);
		self.level=-666;
	}
	else if (self.thingtype==THINGTYPE_CLAY)
		deathsound="fx/claybrk.wav";
	else if (self.thingtype==THINGTYPE_BONE)
		deathsound="fx/bonebrk.wav";
	else if ((self.thingtype==THINGTYPE_LEAVES)  || (self.thingtype==THINGTYPE_WOOD_LEAF))
		deathsound="fx/leafbrk.wav";
	else if (self.thingtype==THINGTYPE_ICE)
		deathsound="misc/icestatx.wav";
	else if(self.thingtype!=THINGTYPE_WEBS)
		deathsound="fx/wallbrk.wav";
	else
		deathsound="misc/null.wav";

	if(self.level!=-666)
		sound (self, CHAN_VOICE, deathsound, 1, ATTN_NORM);
	// Scale        0 - 50,000   small 
	//		   50,000 - 500,000  medium
	//	      500,000            large
	//	    1,000,000 +          huge
	if (spacecube < 5000)
	{
		scalemod = .20;
		model_cnt = model_cnt * 3;	// Because so few pieces come out of a small object
	}
	else if (spacecube < 50000)
	{
		scalemod = .45;
		model_cnt = model_cnt * 3;	// Because so few pieces come out of a small object
	}
	else if (spacecube < 500000)
	{
		scalemod = .50;
	}
	else if (spacecube < 1000000)
	{
		scalemod = .75;
	}
	else
	{
		scalemod = 1;
	}

	if(model_cnt>CHUNK_MAX)
		model_cnt=CHUNK_MAX;

	if(deathmatch||coop)
	{	// this function handles deathmatch specially...
		CreateModelChunks(space,scalemod, model_cnt);
	}
	else
	{
		while (model_cnt>0)
		{
			if (chunk_cnt < CHUNK_MAX*2)
			{
				CreateModelChunks(space,scalemod, 1);
				chunk_cnt+=1;
			}

			model_cnt-=1;
		}
	}
	
	make_chunk_reset();

	if(self.classname=="monster_eidolon")
		return;

	SUB_UseTargets();

	if(self.headmodel!=""&&self.classname!="head")
		ThrowSolidHead (50);
	else
	{
		if(self.movechain)
			remove(self.movechain);
		remove(self);
	}
}

/*
 * $Log: /H2 Mission Pack/HCode/chunk.hc $
 * 
 * 18    3/01/98 3:12p Mgummelt
 * 
 * 17    2/28/98 6:46p Mgummelt
 * 
 * 16    2/25/98 6:10p Mgummelt
 * 
 * 15    2/20/98 1:00p Jmonroe
 * added thingtype_bone
 * 
 * 14    2/12/98 3:32p Jmonroe
 * removed unused funcs
 * 
 * 13    2/07/98 6:50p Mgummelt
 * 
 * 12    2/06/98 12:21p Mgummelt
 * 
 * 11    2/06/98 11:57a Mgummelt
 * 
 * 10    2/06/98 11:30a Mgummelt
 * 
 * 9     2/06/98 5:03a Nalbury
 * Put in preliminary client chunks for network games...
 * 
 * 8     2/05/98 10:23p Mgummelt
 * 
 * 7     1/30/98 10:14p Mgummelt
 * 
 * 6     1/26/98 6:18p Mgummelt
 * 
 * 5     1/20/98 5:16p Mgummelt
 * 
 * 4     1/19/98 6:20p Mgummelt
 * 
 * 73    10/28/97 1:00p Mgummelt
 * Massive replacement, rewrote entire code... just kidding.  Added
 * support for 5th class.
 * 
 * 71    9/04/97 3:50p Mgummelt
 * 
 * 70    9/03/97 2:31a Mgummelt
 * 
 * 69    9/02/97 2:55a Mgummelt
 * 
 * 68    9/01/97 6:34a Mgummelt
 * 
 * 67    8/29/97 4:17p Mgummelt
 * Long night
 * 
 * 66    8/29/97 1:00a Mgummelt
 * 
 * 65    8/28/97 10:11p Mgummelt
 * 
 * 64    8/28/97 9:19p Mgummelt
 * 
 * 63    8/28/97 9:16p Mgummelt
 * 
 * 62    8/28/97 8:54p Mgummelt
 * 
 * 61    8/28/97 8:51p Mgummelt
 * 
 * 60    8/26/97 8:31p Mgummelt
 * 
 * 59    8/23/97 8:24p Mgummelt
 * 
 * 58    8/23/97 5:16p Rlove
 * 
 * 57    8/19/97 6:27p Mgummelt
 * 
 * 56    8/16/97 5:46p Mgummelt
 * 
 * 55    8/07/97 9:55a Rlove
 * 
 * 54    8/07/97 8:43a Rlove
 * 
 * 53    8/04/97 12:19p Rlove
 * 
 * 51    7/30/97 3:32p Mgummelt
 * 
 * 50    7/29/97 7:52a Rlove
 * 
 * 49    7/21/97 3:03p Rlove
 * 
 * 48    7/19/97 9:57p Mgummelt
 * 
 * 47    7/15/97 2:31p Mgummelt
 * 
 * 46    7/10/97 6:21p Rlove
 * 
 * 45    7/09/97 7:43a Rlove
 * 
 * 44    7/09/97 7:35a Rlove
 * New thingtype of CLEARGLASS
 * 
 * 43    7/07/97 7:26p Mgummelt
 * 
 * 42    7/07/97 7:01p Mgummelt
 * 
 * 41    7/03/97 11:13a Rlove
 * 
 * 40    7/02/97 8:46p Mgummelt
 * 
 * 39    6/30/97 3:26p Mgummelt
 * 
 * 38    6/30/97 3:25p Mgummelt
 * 
 * 37    6/30/97 3:24p Rlove
 * 
 * 36    6/23/97 4:50p Mgummelt
 * 
 * 35    6/19/97 5:15p Mgummelt
 * 
 * 34    6/19/97 3:41p Mgummelt
 * 
 * 33    6/19/97 7:47a Rlove
 * 
 * 32    6/18/97 6:03p Mgummelt
 * 
 * 31    6/18/97 5:30p Mgummelt
 * 
 * 30    6/18/97 4:00p Mgummelt
 * 
 * 29    6/17/97 8:16p Mgummelt
 * 
 * 28    6/15/97 5:10p Mgummelt
 * 
 * 27    6/14/97 5:51p Mgummelt
 * 
 * 26    6/13/97 7:36p Mgummelt
 * 
 * 25    6/05/97 8:16p Mgummelt
 * 
 * 24    6/03/97 10:48p Mgummelt
 * 
 * 23    5/29/97 2:22p Rlove
 * Spawn less chunks but they are larger.
 * 
 * 22    5/29/97 8:57a Rlove
 * Added combo thingtypes wood/leaf, wood/metal, wood/stone, metal/stone,
 * metal/cloth
 * 
 * 21    5/28/97 3:36p Mgummelt
 * 
 * 20    5/27/97 8:22p Mgummelt
 * 
 * 19    5/27/97 10:57a Rlove
 * Took out old Id sound files
 * 
 * 18    5/27/97 7:58a Rlove
 * New thingtypes of GreyStone,BrownStone, and Cloth.
 * 
 * 17    5/21/97 3:34p Rlove
 * New chunks
 * 
 * 16    5/13/97 2:26p Rlove
 * 
 * 15    5/06/97 9:12a Rlove
 * Added thingtype_leaves
 * 
 * 14    4/30/97 5:03p Mgummelt
 * 
 * 13    4/29/97 1:08p Mgummelt
 * 
 * 12    4/26/97 6:30a Rlove
 * Added thingtype of CLAY for pots
 * 
 * 11    4/24/97 2:53p Rjohnson
 * Added backpack functionality and spawning of objects
 * 
 * 10    4/24/97 2:15p Mgummelt
 * 
 * 9     4/21/97 8:47p Mgummelt
 * 
 * 8     4/21/97 10:32a Rlove
 * Added stone chunk models 
 * 
 * 7     4/18/97 3:46p Rlove
 * 
 * 6     4/18/97 7:01a Rlove
 * Added new gib models
 * 
 * 5     4/17/97 1:28p Rlove
 * added new built advanceweaponframe
 * 
 * 4     3/31/97 6:37a Rlove
 * Chunks now scale to the size of the object they come from
 * 
 * 3     3/28/97 10:15a Jweier
 * removed old code (incorrect)
 * 
 * 2     3/26/97 2:43p Aleggett
 * Allowed breakable brushes to "use" an entity when they "die".
 * 
 * 1     3/21/97 9:35a Rlove
 * 
 */

