//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef __idHexen2EffectsRandom__
#define __idHexen2EffectsRandom__

// this random generator can have its effects duplicated on the client
// side by passing the randomseed over the network, as opposed to sending
// all the generated values
class idHexen2EffectsRandom
{
public:
	unsigned int seed;

	void setSeed(unsigned int seed);
	float seedRand();
};

inline void idHexen2EffectsRandom::setSeed(unsigned int seed)
{
	this->seed = seed;
}

inline float idHexen2EffectsRandom::seedRand()
{
	seed = (seed * 877 + 573) % 9968;
	return (float)seed / 9968;
}

#endif
