void ice_melt (void)
{
	self.scale -= 0.05;
	if (self.scale<=0.05)
		remove(self);
	else
		self.think=ice_melt;
	thinktime self : 0.05;
}

void ice_think (void)
{
	if(self.velocity=='0 0 0')
	{
		self.touch=SUB_Null;
		self.think=ice_melt;
		thinktime self : 1.5;
	}
	else
	{
		self.think=ice_think;
		thinktime self : 0.1;
	}
}

void ice_hit (void)
{
	if (random()<0.2)
	{
		particleexplosion(self.origin,14,20,5);
		remove(self);
	}
}

void todust (void)
{
		particleexplosion(self.origin,self.aflag,20,5);
		remove(self);
}

void pebble_hit (void)
{
	self.wait=self.wait + 1;
	sound(self,CHAN_BODY,"misc/rubble.wav",1,ATTN_NORM);
	if(self.wait>=3||random()<0.1)
		todust();
	else
	{
		self.think=todust;
		thinktime self : 2;
	}
}
/*
void ash_hit (void)
{
	sound(self,CHAN_BODY,"misc/rubble.wav",1,ATTN_NORM);
	self.wait=self.wait + 1;
	if(self.wait>=3||random()<0.2)
		todust();
	else
	{
		self.think=todust;
		thinktime self : 1;
	}

}
*/
void throw_shard (vector org,vector dir,vector spin,string type,vector ownersize)
{
float chunk_size;
		newmis=spawn_temp();
		newmis.movetype=MOVETYPE_BOUNCE;
		newmis.solid=SOLID_TRIGGER;
		newmis.velocity=dir;
		newmis.avelocity=spin;
		chunk_size=(ownersize_x+ownersize_y+ownersize_z)/3;
		newmis.scale=random(0.5)*chunk_size/24;
		if(!newmis.scale)
			newmis.scale=0.3;
		newmis.classname="type";
		setmodel(newmis,"models/shard.mdl");
		if(type=="ice")
		{
			newmis.skin=0;
			newmis.frame=0;
			newmis.touch=ice_hit;
			newmis.think=ice_think;
			thinktime newmis : 1;
			newmis.drawflags(+)DRF_TRANSLUCENT|MLS_ABSLIGHT;
			newmis.abslight=0.75;
		}
		else if(type=="pebbles")
		{
			newmis.skin=1;
			newmis.frame=rint(random(1,2));
			newmis.touch=pebble_hit;
			newmis.speed=16;
			newmis.aflag=10;
		}
/*		else if(type=="ashes")
		{
			newmis.skin=2;
			newmis.frame=rint(random(1,2));
			newmis.touch=ash_hit;
			newmis.speed=1;
			newmis.aflag=10;
		}
*/		setsize(newmis,'0 0 0','0 0 0');
		setorigin(newmis,org);
}

void shatter ()
{
vector dir,spin,org;
float numshards,maxshards,rng;
string type;
	if(self.movechain!=world&&!self.movechain.flags&FL_CLIENT)
		remove(self.movechain);
	if(self.scale==0)
		self.scale=1;
	if(self.classname=="snowball")
		maxshards=random(4,2);
	else
		maxshards=random(7,10);
	org=(self.absmin+self.absmax)*0.5;
	if(self.deathtype=="ice shatter"||self.deathtype=="ice melt")
	{
//origin color radius count
		particleexplosion(org,14,25,50);
//		particle2(org,'-50 -50 -50','50 50 50',145,14,50);
		if(self.deathtype=="ice shatter")
			rng=600;
		else
			rng=self.size_x/2;
		type="ice";
	}
	else if(self.deathtype=="stone crumble")
	{
		sound(self,CHAN_BODY,"misc/sshatter.wav",1,ATTN_NORM);
		particleexplosion(org,10,60,50);
//		particle2(org,'-30 -30 -30','30 30 30',16,10,50);
		rng=450;
		type="pebbles";
	}
/*	else if(self.deathtype=="burnt crumble")
	{
		sound(self,CHAN_BODY,"misc/bshatter.wav",1,ATTN_NORM);
		particleexplosion(org,1,60,50);
//		particle2(org,'-30 -30 -30','30 30 30',1,10,50);
		rng=200;
		type="ashes";
	}
*/
	if(type == "ice")
	{
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_ICEHIT);
		WriteCoord (MSG_MULTICAST, self.origin_x);
		WriteCoord (MSG_MULTICAST, self.origin_y);
		WriteCoord (MSG_MULTICAST, self.origin_z);
		WriteByte (MSG_MULTICAST, 2);	// type of icehit -> shatter type
		multicast(self.origin,MULTICAST_PHS_R);
	}
	else
	{
		// do stone junk
		while(numshards<maxshards)
		{
			dir_x=random(0-rng,rng);
			dir_y=random(0-rng,rng);
			dir_z=random(0-rng,rng);
			spin_x=random(300,-300);
			spin_y=random(300,-300);
			spin_z=random(300,-300);
			throw_shard(org,dir,spin,type,self.size);
			numshards+=1;
		}
	}

	if(self.movechain!=world&&!self.movechain.flags&FL_CLIENT)
		remove(self.movechain);
	if(self.classname!="player")	
		remove(self);
}

