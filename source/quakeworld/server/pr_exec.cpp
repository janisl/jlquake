/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "qwsvdef.h"

/*
============
PR_Profile_f

============
*/
void PR_Profile_f(void)
{
	dfunction_t* f, * best;
	int max;
	int num;
	int i;

	num = 0;
	do
	{
		max = 0;
		best = NULL;
		for (i = 0; i < progs->numfunctions; i++)
		{
			f = &pr_functions[i];
			if (f->profile > max)
			{
				max = f->profile;
				best = f;
			}
		}
		if (best)
		{
			if (num < 10)
			{
				Con_Printf("%7i %s\n", best->profile, PR_GetString(best->s_name));
			}
			num++;
			best->profile = 0;
		}
	}
	while (best);
}


/*
============================================================================
PR_ExecuteProgram

The interpretation main loop
============================================================================
*/

/*
====================
PR_ExecuteProgram
====================
*/
void PR_ExecuteProgram(func_t fnum)
{
	eval_t* a, * b, * c;
	int s;
	dstatement_t* st;
	dfunction_t* f;
	int runaway;
	qhedict_t* ed;
	int exitdepth;

	if (!fnum || fnum >= progs->numfunctions)
	{
		if (*pr_globalVars.self)
		{
			ED_Print(PROG_TO_EDICT(*pr_globalVars.self));
		}
		SV_Error("PR_ExecuteProgram: NULL function");
	}

	f = &pr_functions[fnum];

	runaway = 100000;
	pr_trace = false;

// make a stack frame
	exitdepth = pr_depth;

	s = PR_EnterFunction(f);

	while (1)
	{
		s++;// next statement

		st = &pr_statements[s];
		a = (eval_t*)&pr_globals[st->a];
		b = (eval_t*)&pr_globals[st->b];
		c = (eval_t*)&pr_globals[st->c];

		if (--runaway == 0)
		{
			PR_RunError("runaway loop error");
		}

		pr_xfunction->profile++;
		pr_xstatement = s;

		if (pr_trace)
		{
			PR_PrintStatement(st);
		}

		switch (st->op)
		{
		case OP_STATE:
			ed = PROG_TO_EDICT(*pr_globalVars.self);
			ed->SetNextThink(*pr_globalVars.time + (GGameType & GAME_Hexen2 ? HX_FRAME_TIME : 0.1));
			if (a->_float != ed->GetFrame())
			{
				ed->SetFrame(a->_float);
			}
			ed->SetThink(b->function);
			break;

		default:
			if (PR_ExecuteProgramCommon(st, a, b, c, s, exitdepth))
				return;
		}
	}

}
