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

#ifndef _AI_WEIGHT_H
#define _AI_WEIGHT_H

#define WT_BALANCE          1
#define MAX_WEIGHTS         128

//fuzzy seperator
struct fuzzyseperator_t
{
	int index;
	int value;
	int type;
	float weight;
	float minweight;
	float maxweight;
	fuzzyseperator_t* child;
	fuzzyseperator_t* next;
};

//fuzzy weight
struct weight_t
{
	char* name;
	fuzzyseperator_t* firstseperator;
};

//weight configuration
struct weightconfig_t
{
	int numweights;
	weight_t weights[MAX_WEIGHTS];
	char filename[MAX_QPATH];
};

//reads a weight configuration
weightconfig_t* ReadWeightConfig(const char* filename);
//free a weight configuration
void FreeWeightConfig(weightconfig_t* config);
//find the fuzzy weight with the given name
int FindFuzzyWeight(weightconfig_t* wc, const char* name);
//returns the fuzzy weight for the given inventory and weight
float FuzzyWeight(const int* inventory, const weightconfig_t* wc, int weightnum);
float FuzzyWeightUndecided(const int* inventory, const weightconfig_t* wc, int weightnum);
//evolves the weight configuration
void EvolveWeightConfig(weightconfig_t* config);
//interbreed the weight configurations and stores the interbreeded one in configout
void InterbreedWeightConfigs(weightconfig_t* config1, weightconfig_t* config2, weightconfig_t* configout);
//frees cached weight configurations
void BotShutdownWeights();

#endif
