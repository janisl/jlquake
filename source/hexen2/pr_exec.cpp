
//**************************************************************************
//**
//** pr_exec.c
//**
//** $Header: /H2 Mission Pack/PR_EXEC.C 5     3/06/98 12:35a Jmonroe $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// PR_ExecuteProgram
//
//==========================================================================

//switch types
enum {SWITCH_F,SWITCH_V,SWITCH_S,SWITCH_E,SWITCH_FNC};

void PR_ExecuteProgram(func_t fnum)
{
	int i;
	int s;
	eval_t* a, * b, * c;
	eval_t* ptr;
	dstatement_t* st;
	dfunction_t* f;
	int runaway;
	qhedict_t* ed;
	int exitdepth;
	int startFrame;
	int endFrame;
	float val;
	int case_type = -1;
	float switch_float;

	if (!fnum || fnum >= progs->numfunctions)
	{
		if (*pr_globalVars.self)
		{
			ED_Print(PROG_TO_EDICT(*pr_globalVars.self));
		}
		Host_Error("PR_ExecuteProgram: NULL function");
	}

	f = &pr_functions[fnum];

	runaway = 100000;
	pr_trace = false;

	exitdepth = pr_depth;

	s = PR_EnterFunction(f);

	while (1)
	{
		s++;// Next statement

		st = &pr_statements[s];
		a = (eval_t*)&pr_globals[(unsigned short)st->a];
		b = (eval_t*)&pr_globals[(unsigned short)st->b];
		c = (eval_t*)&pr_globals[(unsigned short)st->c];

		if (!--runaway)
		{
			PR_RunError("runaway loop error");
		}

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

		case OP_CSTATE:	// Cycle state
			ed = PROG_TO_EDICT(*pr_globalVars.self);
			ed->SetNextThink(*pr_globalVars.time + HX_FRAME_TIME);
			ed->SetThink(pr_xfunction - pr_functions);
			*pr_globalVars.cycle_wrapped = false;
			startFrame = (int)a->_float;
			endFrame = (int)b->_float;
			if (startFrame <= endFrame)
			{	// Increment
				if (ed->GetFrame() < startFrame || ed->GetFrame() > endFrame)
				{
					ed->SetFrame(startFrame);
					break;
				}
				ed->SetFrame(ed->GetFrame() + 1);
				if (ed->GetFrame() > endFrame)
				{
					*pr_globalVars.cycle_wrapped = true;
					ed->SetFrame(startFrame);
				}
				break;
			}
			// Decrement
			if (ed->GetFrame() > startFrame || ed->GetFrame() < endFrame)
			{
				ed->SetFrame(startFrame);
				break;
			}
			ed->SetFrame(ed->GetFrame() - 1);
			if (ed->GetFrame() < endFrame)
			{
				*pr_globalVars.cycle_wrapped = true;
				ed->SetFrame(startFrame);
			}
			break;

		case OP_CWSTATE:// Cycle weapon state
			ed = PROG_TO_EDICT(*pr_globalVars.self);
			ed->SetNextThink(*pr_globalVars.time + HX_FRAME_TIME);
			ed->SetThink(pr_xfunction - pr_functions);
			*pr_globalVars.cycle_wrapped = false;
			startFrame = (int)a->_float;
			endFrame = (int)b->_float;
			if (startFrame <= endFrame)
			{	// Increment
				if (ed->GetWeaponFrame() < startFrame ||
					ed->GetWeaponFrame() > endFrame)
				{
					ed->SetWeaponFrame(startFrame);
					break;
				}
				ed->SetWeaponFrame(ed->GetWeaponFrame() + 1);
				if (ed->GetWeaponFrame() > endFrame)
				{
					*pr_globalVars.cycle_wrapped = true;
					ed->SetWeaponFrame(startFrame);
				}
				break;
			}
			// Decrement
			if (ed->GetWeaponFrame() > startFrame ||
				ed->GetWeaponFrame() < endFrame)
			{
				ed->SetWeaponFrame(startFrame);
				break;
			}
			ed->SetWeaponFrame(ed->GetWeaponFrame() - 1);
			if (ed->GetWeaponFrame() < endFrame)
			{
				*pr_globalVars.cycle_wrapped = true;
				ed->SetWeaponFrame(startFrame);
			}
			break;

		case OP_THINKTIME:
			ed = PROG_TO_EDICT(a->edict);
#ifdef PARANOID
			QH_NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
			if (ed == (qhedict_t*)sv.qh_edicts && sv.state == SS_GAME)
			{
				PR_RunError("assignment to world entity");
			}
			ed->SetNextThink(*pr_globalVars.time + b->_float);
			break;

		case OP_BITSET:	// f (+) f
			b->_float = (int)b->_float | (int)a->_float;
			break;
		case OP_BITSETP:// e.f (+) f
			ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			ptr->_float = (int)ptr->_float | (int)a->_float;
			break;
		case OP_BITCLR:	// f (-) f
			b->_float = (int)b->_float & ~((int)a->_float);
			break;
		case OP_BITCLRP:// e.f (-) f
			ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			ptr->_float = (int)ptr->_float & ~((int)a->_float);
			break;

		case OP_RAND0:
			val = rand() * (1.0 / RAND_MAX);//(rand()&0x7fff)/((float)0x7fff);
			G_FLOAT(OFS_RETURN) = val;
			break;
		case OP_RAND1:
			val = rand() * (1.0 / RAND_MAX) * a->_float;
			G_FLOAT(OFS_RETURN) = val;
			break;
		case OP_RAND2:
			if (a->_float < b->_float)
			{
				val = a->_float + (rand() * (1.0 / RAND_MAX)
								   * (b->_float - a->_float));
			}
			else
			{
				val = b->_float + (rand() * (1.0 / RAND_MAX)
								   * (a->_float - b->_float));
			}
			G_FLOAT(OFS_RETURN) = val;
			break;
		case OP_RANDV0:
			val = rand() * (1.0 / RAND_MAX);
			G_FLOAT(OFS_RETURN + 0) = val;
			val = rand() * (1.0 / RAND_MAX);
			G_FLOAT(OFS_RETURN + 1) = val;
			val = rand() * (1.0 / RAND_MAX);
			G_FLOAT(OFS_RETURN + 2) = val;
			break;
		case OP_RANDV1:
			val = rand() * (1.0 / RAND_MAX) * a->vector[0];
			G_FLOAT(OFS_RETURN + 0) = val;
			val = rand() * (1.0 / RAND_MAX) * a->vector[1];
			G_FLOAT(OFS_RETURN + 1) = val;
			val = rand() * (1.0 / RAND_MAX) * a->vector[2];
			G_FLOAT(OFS_RETURN + 2) = val;
			break;
		case OP_RANDV2:
			for (i = 0; i < 3; i++)
			{
				if (a->vector[i] < b->vector[i])
				{
					val = a->vector[i] + (rand() * (1.0 / RAND_MAX)
										  * (b->vector[i] - a->vector[i]));
				}
				else
				{
					val = b->vector[i] + (rand() * (1.0 / RAND_MAX)
										  * (a->vector[i] - b->vector[i]));
				}
				G_FLOAT(OFS_RETURN + i) = val;
			}
			break;
		case OP_SWITCH_F:
			case_type = SWITCH_F;
			switch_float = a->_float;
			s += st->b - 1;	// -1 to offset the s++
			break;
		case OP_SWITCH_V:
			PR_RunError("switch v not done yet!");
			break;
		case OP_SWITCH_S:
			PR_RunError("switch s not done yet!");
			break;
		case OP_SWITCH_E:
			PR_RunError("switch e not done yet!");
			break;
		case OP_SWITCH_FNC:
			PR_RunError("switch fnc not done yet!");
			break;

		case OP_CASERANGE:
			if (case_type != SWITCH_F)
			{
				PR_RunError("caserange fucked!");
			}
			if ((switch_float >= a->_float) && (switch_float <= b->_float))
			{
				s += st->c - 1;	// -1 to offset the s++
			}
			break;
		case OP_CASE:
			switch (case_type)
			{
			case SWITCH_F:
				if (switch_float == a->_float)
				{
					s += st->b - 1;	// -1 to offset the s++
				}
				break;
			case SWITCH_V:
			case SWITCH_S:
			case SWITCH_E:
			case SWITCH_FNC:
				PR_RunError("case not done yet!");
				break;
			default:
				PR_RunError("fucked case!");

			}
			break;

		default:
			if (PR_ExecuteProgramCommon(st, a, b, c, s, exitdepth))
				return;
		}
	}

}

//==========================================================================
//
// PR_Profile_f
//
//==========================================================================

void PR_Profile_f(void)
{
	int i, j;
	int max;
	dfunction_t* f, * bestFunc;
	int total;
	int funcCount;
	qboolean byHC;
	char saveName[128];
	fileHandle_t saveFile;
	int currentFile;
	int bestFile;
	int tally;
	char* s;

	byHC = false;
	funcCount = 10;
	*saveName = 0;
	for (i = 1; i < Cmd_Argc(); i++)
	{
		s = Cmd_Argv(i);
		if (String::ToLower(*s) == 'h')
		{	// Sort by HC source file
			byHC = true;
		}
		else if (String::ToLower(*s) == 's')
		{	// Save to file
			if (i + 1 < Cmd_Argc() && !String::IsDigit(*Cmd_Argv(i + 1)))
			{
				i++;
				sprintf(saveName, "%s", Cmd_Argv(i));
			}
			else
			{
				String::Cpy(saveName, "profile.txt");
			}
		}
		else if (String::IsDigit(*s))
		{	// Specify function count
			funcCount = String::Atoi(Cmd_Argv(i));
			if (funcCount < 1)
			{
				funcCount = 1;
			}
		}
	}

	total = 0;
	for (i = 0; i < progs->numfunctions; i++)
	{
		total += pr_functions[i].profile;
	}

	if (*saveName)
	{	// Create the output file
		if ((saveFile = FS_FOpenFileWrite(saveName)) == 0)
		{
			Con_Printf("Could not open %s\n", saveName);
			return;
		}
	}

	if (byHC == false)
	{
		j = 0;
		do
		{
			max = 0;
			bestFunc = NULL;
			for (i = 0; i < progs->numfunctions; i++)
			{
				f = &pr_functions[i];
				if (f->profile > max)
				{
					max = f->profile;
					bestFunc = f;
				}
			}
			if (bestFunc)
			{
				if (j < funcCount)
				{
					if (*saveName)
					{
						FS_Printf(saveFile, "%05.2f %s\n",
							((float)bestFunc->profile / (float)total) * 100.0,
							PR_GetString(bestFunc->s_name));
					}
					else
					{
						Con_Printf("%05.2f %s\n",
							((float)bestFunc->profile / (float)total) * 100.0,
							PR_GetString(bestFunc->s_name));
					}
				}
				j++;
				bestFunc->profile = 0;
			}
		}
		while (bestFunc);
		if (*saveName)
		{
			FS_FCloseFile(saveFile);
		}
		return;
	}

	currentFile = -1;
	do
	{
		tally = 0;
		bestFile = MAX_QINT32;
		for (i = 0; i < progs->numfunctions; i++)
		{
			if (pr_functions[i].s_file > currentFile &&
				pr_functions[i].s_file < bestFile)
			{
				bestFile = pr_functions[i].s_file;
				tally = pr_functions[i].profile;
				continue;
			}
			if (pr_functions[i].s_file == bestFile)
			{
				tally += pr_functions[i].profile;
			}
		}
		currentFile = bestFile;
		if (tally && currentFile != MAX_QINT32)
		{
			if (*saveName)
			{
				FS_Printf(saveFile, "\"%s\"\n", PR_GetString(currentFile));
			}
			else
			{
				Con_Printf("\"%s\"\n", PR_GetString(currentFile));
			}
			j = 0;
			do
			{
				max = 0;
				bestFunc = NULL;
				for (i = 0; i < progs->numfunctions; i++)
				{
					f = &pr_functions[i];
					if (f->s_file == currentFile && f->profile > max)
					{
						max = f->profile;
						bestFunc = f;
					}
				}
				if (bestFunc)
				{
					if (j < funcCount)
					{
						if (*saveName)
						{
							FS_Printf(saveFile, "   %05.2f %s\n",
								((float)bestFunc->profile
								 / (float)total) * 100.0,
								PR_GetString(bestFunc->s_name));
						}
						else
						{
							Con_Printf("   %05.2f %s\n",
								((float)bestFunc->profile
								 / (float)total) * 100.0,
								PR_GetString(bestFunc->s_name));
						}
					}
					j++;
					bestFunc->profile = 0;
				}
			}
			while (bestFunc);
		}
	}
	while (currentFile != MAX_QINT32);
	if (*saveName)
	{
		FS_FCloseFile(saveFile);
	}
}
