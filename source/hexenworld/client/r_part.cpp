
#include "quakedef.h"

ptype_t hexenWorldParticleTypeTable[] =
{
	pt_h2static,
	pt_h2grav,
	pt_h2fastgrav,
	pt_h2slowgrav,
	pt_h2fire,
	pt_h2explode,
	pt_h2explode2,
	pt_h2blob_hw,
	pt_h2blob2_hw,
	pt_h2rain,
	pt_h2c_explode,
	pt_h2c_explode2,
	pt_h2spit,
	pt_h2fireball,
	pt_h2ice,
	pt_h2spell,
	pt_h2test,
	pt_h2quake,
	pt_h2rd,
	pt_h2vorpal,
	pt_h2setstaff,
	pt_h2magicmissile,
	pt_h2boneshard,
	pt_h2scarab,
	pt_h2darken,
	pt_h2grensmoke,
	pt_h2redfire,
	pt_h2acidball,
	pt_h2bluestep,
};

/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect(void)
{
	vec3_t org, dir;
	int i, count, color;

	for (i = 0; i < 3; i++)
		org[i] = net_message.ReadCoord();
	for (i = 0; i < 3; i++)
		dir[i] = net_message.ReadChar() * (1.0 / 16);
	count = net_message.ReadByte();
	color = net_message.ReadByte();

	if (count == 255)
	{
		CLH2_ParticleExplosion(org);
	}
	else
	{
		CLH2_RunParticleEffect(org, dir, count);
	}
}

/*
===============
R_ParseParticleEffect2

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect2(void)
{
	vec3_t org, dmin, dmax;
	int i, msgcount, color, effect;

	for (i = 0; i < 3; i++)
		org[i] = net_message.ReadCoord();
	for (i = 0; i < 3; i++)
		dmin[i] = net_message.ReadFloat();
	for (i = 0; i < 3; i++)
		dmax[i] = net_message.ReadFloat();
	color = net_message.ReadShort();
	msgcount = net_message.ReadByte();
	effect = net_message.ReadByte();

	CLH2_RunParticleEffect2(org, dmin, dmax, color, hexenWorldParticleTypeTable[effect], msgcount);
}

/*
===============
R_ParseParticleEffect3

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect3(void)
{
	vec3_t org, box;
	int i, msgcount, color, effect;

	for (i = 0; i < 3; i++)
		org[i] = net_message.ReadCoord();
	for (i = 0; i < 3; i++)
		box[i] = net_message.ReadByte();
	color = net_message.ReadShort();
	msgcount = net_message.ReadByte();
	effect = net_message.ReadByte();

	CLH2_RunParticleEffect3(org, box, color, hexenWorldParticleTypeTable[effect], msgcount);
}

/*
===============
R_ParseParticleEffect4

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect4(void)
{
	vec3_t org;
	int i, msgcount, color, effect;
	float radius;

	for (i = 0; i < 3; i++)
		org[i] = net_message.ReadCoord();
	radius = net_message.ReadByte();
	color = net_message.ReadShort();
	msgcount = net_message.ReadByte();
	effect = net_message.ReadByte();

	CLH2_RunParticleEffect4(org, radius, color, hexenWorldParticleTypeTable[effect], msgcount);
}

void R_ParseRainEffect(void)
{
	vec3_t org, e_size;
	short color,count;
	int x_dir, y_dir;

	org[0] = net_message.ReadCoord();
	org[1] = net_message.ReadCoord();
	org[2] = net_message.ReadCoord();
	e_size[0] = net_message.ReadCoord();
	e_size[1] = net_message.ReadCoord();
	e_size[2] = net_message.ReadCoord();
	x_dir = net_message.ReadAngle();
	y_dir = net_message.ReadAngle();
	color = net_message.ReadShort();
	count = net_message.ReadShort();

	CLH2_RainEffect(org,e_size,x_dir,y_dir,color,count);
}
