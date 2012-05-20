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

#include "../server.h"
#include "local.h"

static int GeneticSelection(int numranks, float* rankings)
{
	float sum = 0;
	for (int i = 0; i < numranks; i++)
	{
		if (rankings[i] < 0)
		{
			continue;
		}
		sum += rankings[i];
	}
	if (sum > 0)
	{
		//select a bot where the ones with the higest rankings have
		//the highest chance of being selected
		float select = random() * sum;
		for (int i = 0; i < numranks; i++)
		{
			if (rankings[i] < 0)
			{
				continue;
			}
			sum -= rankings[i];
			if (sum <= 0)
			{
				return i;
			}
		}
	}
	//select a bot randomly
	int index = random() * numranks;
	for (int i = 0; i < numranks; i++)
	{
		if (rankings[index] >= 0)
		{
			return index;
		}
		index = (index + 1) % numranks;
	}
	return 0;
}

bool GeneticParentsAndChildSelection(int numranks, float* ranks, int* parent1, int* parent2, int* child)
{
	if (numranks > 256)
	{
		BotImport_Print(PRT_WARNING, "GeneticParentsAndChildSelection: too many bots\n");
		*parent1 = *parent2 = *child = 0;
		return false;
	}
	float max = 0;
	for (int i = 0; i < numranks; i++)
	{
		if (ranks[i] < 0)
		{
			continue;
		}
		max++;
	}
	if (max < 3)
	{
		BotImport_Print(PRT_WARNING, "GeneticParentsAndChildSelection: too few valid bots\n");
		*parent1 = *parent2 = *child = 0;
		return false;
	}
	float rankings[256];
	Com_Memcpy(rankings, ranks, sizeof(float) * numranks);
	//select first parent
	*parent1 = GeneticSelection(numranks, rankings);
	rankings[*parent1] = -1;
	//select second parent
	*parent2 = GeneticSelection(numranks, rankings);
	rankings[*parent2] = -1;
	//reverse the rankings
	max = 0;
	for (int i = 0; i < numranks; i++)
	{
		if (rankings[i] < 0)
		{
			continue;
		}
		if (rankings[i] > max)
		{
			max = rankings[i];
		}
	}
	for (int i = 0; i < numranks; i++)
	{
		if (rankings[i] < 0)
		{
			continue;
		}
		rankings[i] = max - rankings[i];
	}
	//select child
	*child = GeneticSelection(numranks, rankings);
	return true;
}
