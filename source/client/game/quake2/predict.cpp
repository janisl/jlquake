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

#include "../../client.h"
#include "local.h"

void CLQ2_CheckPredictionError()
{
	int frame;
	int delta[3];
	int i;
	int len;

	if (!clq2_predict->value || (cl.q2_frame.playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION))
	{
		return;
	}

	// calculate the last q2usercmd_t we sent that the server has processed
	frame = clc.netchan.incomingAcknowledged;
	frame &= (CMD_BACKUP_Q2 - 1);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract(cl.q2_frame.playerstate.pmove.origin, cl.q2_predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);
	if (len > 640)	// 80 world units
	{	// a teleport or something
		VectorClear(cl.q2_prediction_error);
	}
	else
	{
		if (clq2_showmiss->value && (delta[0] || delta[1] || delta[2]))
		{
			common->Printf("prediction miss on %i: %i\n", cl.q2_frame.serverframe,
				delta[0] + delta[1] + delta[2]);
		}

		VectorCopy(cl.q2_frame.playerstate.pmove.origin, cl.q2_predicted_origins[frame]);

		// save for error itnerpolation
		for (i = 0; i < 3; i++)
			cl.q2_prediction_error[i] = delta[i] * 0.125;
	}
}
