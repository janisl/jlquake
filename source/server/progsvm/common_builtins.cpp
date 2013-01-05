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

#include "progsvm.h"
#include "../../common/strings.h"
#include "../../common/command_buffer.h"
#include "../../common/console_variable.h"
#include "../../common/Common.h"
#include "../global.h"

static char pr_string_temp[128];

const char* PF_VarString(int first)
{
	int i;
	static char out[256];

	out[0] = 0;
	for (i = first; i < pr_argc; i++)
	{
		String::Cat(out, sizeof(out), G_STRING((OFS_PARM0 + i * 3)));
	}
	return out;
}

void PF_Fixme()
{
	PR_RunError("unimplemented bulitin");
}

//	This is a TERMINAL error, which will kill off the entire server.
// Dumps self.
void PF_error()
{
	const char* s = PF_VarString(0);
	common->Printf("======SERVER ERROR in %s:\n%s\n",
		PR_GetString(pr_xfunction->s_name), s);
	const qhedict_t* ed = PROG_TO_EDICT(*pr_globalVars.self);
	ED_Print(ed);

	common->Error("Program error");
}

//	Writes new values for v_forward, v_up, and v_right based on angles
void PF_makevectors()
{
	AngleVectors(G_VECTOR(OFS_PARM0), pr_globalVars.v_forward, pr_globalVars.v_right, pr_globalVars.v_up);
}

void PF_normalize()
{
	float* value1 = G_VECTOR(OFS_PARM0);

	float len = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	len = sqrt(len);

	vec3_t newvalue;
	if (len == 0)
	{
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	}
	else
	{
		len = 1 / len;
		newvalue[0] = value1[0] * len;
		newvalue[1] = value1[1] * len;
		newvalue[2] = value1[2] * len;
	}

	VectorCopy(newvalue, G_VECTOR(OFS_RETURN));
}

void PF_vlen()
{
	float* value1 = G_VECTOR(OFS_PARM0);

	float len = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	len = sqrt(len);

	G_FLOAT(OFS_RETURN) = len;
}

void PF_vhlen()
{
	float* value1 = G_VECTOR(OFS_PARM0);

	float len = value1[0] * value1[0] + value1[1] * value1[1];
	len = sqrt(len);

	G_FLOAT(OFS_RETURN) = len;
}

void PF_vectoyaw()
{
	float* value1 = G_VECTOR(OFS_PARM0);

	float yaw = VecToYaw(value1);

	G_FLOAT(OFS_RETURN) = (int)yaw;
}

void PF_vectoangles()
{
	float* value1 = G_VECTOR(OFS_PARM0);

	vec3_t angles;
	VecToAnglesBuggy(value1, angles);

	G_FLOAT(OFS_RETURN + 0) = (int)angles[0];
	G_FLOAT(OFS_RETURN + 1) = (int)angles[1];
	G_FLOAT(OFS_RETURN + 2) = (int)angles[2];
}

//	Returns a number from 0<= num < 1
void PF_random()
{
	float num = (rand() & 0x7fff) / ((float)0x7fff);

	G_FLOAT(OFS_RETURN) = num;
}

void PF_break()
{
	common->Printf("break statement\n");
	*(int*)-4 = 0;	// dump to debugger
}

//	Sends text over to the client's execution buffer
void PF_localcmd()
{
	const char* str = G_STRING(OFS_PARM0);
	Cbuf_AddText(str);
}

void PF_cvar()
{
	const char* str = G_STRING(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = Cvar_VariableValue(str);
}

void PF_cvar_set()
{
	const char* var = G_STRING(OFS_PARM0);
	const char* val = G_STRING(OFS_PARM1);

	Cvar_Set(var, val);
}

void PF_dprint()
{
	common->DPrintf("%s", PF_VarString(0));
}

void PF_dprintf()
{
	float v = G_FLOAT(OFS_PARM1);

	char temp[256];
	if (v == (int)v)
	{
		String::Sprintf(temp, sizeof(temp), "%d",(int)v);
	}
	else
	{
		String::Sprintf(temp, sizeof(temp), "%5.1f",v);
	}

	common->DPrintf(G_STRING(OFS_PARM0),temp);
}

void PF_dprintv()
{
	float* v = G_VECTOR(OFS_PARM1);

	char temp[256];
	String::Sprintf(temp, sizeof(temp), "'%5.1f %5.1f %5.1f'", v[0], v[1], v[2]);

	common->DPrintf(G_STRING(OFS_PARM0),temp);
}

void PF_ftos()
{
	float v = G_FLOAT(OFS_PARM0);

	if (v == (int)v)
	{
		String::Sprintf(pr_string_temp, sizeof(pr_string_temp), "%d",(int)v);
	}
	else
	{
		String::Sprintf(pr_string_temp, sizeof(pr_string_temp), "%5.1f",v);
	}
	G_INT(OFS_RETURN) = PR_SetString(pr_string_temp);
}

void PF_vtos()
{
	float* v = G_VECTOR(OFS_PARM0);
	String::Sprintf(pr_string_temp, sizeof(pr_string_temp), "'%5.1f %5.1f %5.1f'", v[0], v[1], v[2]);
	G_INT(OFS_RETURN) = PR_SetString(pr_string_temp);
}

void PF_coredump()
{
	ED_PrintEdicts();
}

void PF_traceon()
{
	pr_trace = true;
}

void PF_traceoff()
{
	pr_trace = false;
}

void PF_eprint()
{
	ED_PrintNum(G_EDICTNUM(OFS_PARM0));
}

void PF_fabs()
{
	float v = G_FLOAT(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = Q_fabs(v);
}

void PF_rint()
{
	float f = G_FLOAT(OFS_PARM0);
	if (f > 0)
	{
		G_FLOAT(OFS_RETURN) = (int)(f + 0.5);
	}
	else
	{
		G_FLOAT(OFS_RETURN) = (int)(f - 0.5);
	}
}

void PFH2_rint()
{
	float f = G_FLOAT(OFS_PARM0);
	if (f > 0)
	{
		G_FLOAT(OFS_RETURN) = (int)(f + 0.1);
	}
	else
	{
		G_FLOAT(OFS_RETURN) = (int)(f - 0.1);
	}
}

void PF_floor()
{
	G_FLOAT(OFS_RETURN) = floor(G_FLOAT(OFS_PARM0));
}

void PF_ceil()
{
	G_FLOAT(OFS_RETURN) = ceil(G_FLOAT(OFS_PARM0));
}

void PF_sqrt()
{
	G_FLOAT(OFS_RETURN) = sqrt(G_FLOAT(OFS_PARM0));
}

void PF_Cos()
{
	float angle = G_FLOAT(OFS_PARM0);

	angle = angle * M_PI * 2 / 360;

	G_FLOAT(OFS_RETURN) = cos(angle);
}

void PF_Sin()
{
	float angle = G_FLOAT(OFS_PARM0);

	angle = angle * M_PI * 2 / 360;

	G_FLOAT(OFS_RETURN) = sin(angle);
}

void PF_stof()
{
	const char* s = G_STRING(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = String::Atof(s);
}

void PF_randomrange()
{
	float minv = G_FLOAT(OFS_PARM0);
	float maxv = G_FLOAT(OFS_PARM1);

	float num = rand() * (1.0 / RAND_MAX);

	G_FLOAT(OFS_RETURN) = ((maxv - minv) * num) + minv;
}

void PF_randomvalue()
{
	float range = G_FLOAT(OFS_PARM0);

	float num = rand() * (1.0 / RAND_MAX);

	G_FLOAT(OFS_RETURN) = range * num;
}

void PF_randomvrange()
{
	float* minv = G_VECTOR(OFS_PARM0);
	float* maxv = G_VECTOR(OFS_PARM1);

	vec3_t result;
	float num = rand() * (1.0 / RAND_MAX);
	result[0] = ((maxv[0] - minv[0]) * num) + minv[0];
	num = rand() * (1.0 / RAND_MAX);
	result[1] = ((maxv[1] - minv[1]) * num) + minv[1];
	num = rand() * (1.0 / RAND_MAX);
	result[2] = ((maxv[2] - minv[2]) * num) + minv[2];

	VectorCopy(result, G_VECTOR(OFS_RETURN));
}

void PF_randomvvalue()
{
	float* range = G_VECTOR(OFS_PARM0);

	vec3_t result;
	float num = rand() * (1.0 / RAND_MAX);
	result[0] = range[0] * num;
	num = rand() * (1.0 / RAND_MAX);
	result[1] = range[1] * num;
	num = rand() * (1.0 / RAND_MAX);
	result[2] = range[2] * num;

	VectorCopy(result, G_VECTOR(OFS_RETURN));
}

void PF_concatv()
{
	float* in = G_VECTOR(OFS_PARM0);
	float* range = G_VECTOR(OFS_PARM1);

	vec3_t result;
	VectorCopy(in, result);
	if (result[0] < -range[0])
	{
		result[0] = -range[0];
	}
	if (result[0] > range[0])
	{
		result[0] = range[0];
	}
	if (result[1] < -range[1])
	{
		result[1] = -range[1];
	}
	if (result[1] > range[1])
	{
		result[1] = range[1];
	}
	if (result[2] < -range[2])
	{
		result[2] = -range[2];
	}
	if (result[2] > range[2])
	{
		result[2] = range[2];
	}

	VectorCopy(result, G_VECTOR(OFS_RETURN));
}

// returns (v_right * factor_x) + (v_forward * factor_y) + (v_up * factor_z)
void PF_v_factor()
{
	float* range = G_VECTOR(OFS_PARM0);

	vec3_t result;
	result[0] = (pr_globalVars.v_right[0] * range[0]) +
				(pr_globalVars.v_forward[0] * range[1]) +
				(pr_globalVars.v_up[0] * range[2]);

	result[1] = (pr_globalVars.v_right[1] * range[0]) +
				(pr_globalVars.v_forward[1] * range[1]) +
				(pr_globalVars.v_up[1] * range[2]);

	result[2] = (pr_globalVars.v_right[2] * range[0]) +
				(pr_globalVars.v_forward[2] * range[1]) +
				(pr_globalVars.v_up[2] * range[2]);

	VectorCopy(result, G_VECTOR(OFS_RETURN));
}

void PF_v_factorrange()
{
	float* minv = G_VECTOR(OFS_PARM0);
	float* maxv = G_VECTOR(OFS_PARM1);

	vec3_t result;
	float num = rand() * (1.0 / RAND_MAX);
	result[0] = ((maxv[0] - minv[0]) * num) + minv[0];
	num = rand() * (1.0 / RAND_MAX);
	result[1] = ((maxv[1] - minv[1]) * num) + minv[1];
	num = rand() * (1.0 / RAND_MAX);
	result[2] = ((maxv[2] - minv[2]) * num) + minv[2];

	vec3_t r2;
	r2[0] = (pr_globalVars.v_right[0] * result[0]) +
			(pr_globalVars.v_forward[0] * result[1]) +
			(pr_globalVars.v_up[0] * result[2]);

	r2[1] = (pr_globalVars.v_right[1] * result[0]) +
			(pr_globalVars.v_forward[1] * result[1]) +
			(pr_globalVars.v_up[1] * result[2]);

	r2[2] = (pr_globalVars.v_right[2] * result[0]) +
			(pr_globalVars.v_forward[2] * result[1]) +
			(pr_globalVars.v_up[2] * result[2]);

	VectorCopy(r2, G_VECTOR(OFS_RETURN));
}
