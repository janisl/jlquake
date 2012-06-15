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

builtin_t* pr_builtins;
int pr_numbuiltins;

bool pr_trace;
dfunction_t* pr_xfunction;
int pr_xstatement;
int pr_argc;

prstack_t pr_stack[MAX_STACK_DEPTH];
int pr_depth;

int localstack[LOCALSTACK_SIZE];
int localstack_used;

static const char* pr_opnames[] =
{
	"DONE",
	"MUL_F", "MUL_V", "MUL_FV", "MUL_VF",
	"DIV",
	"ADD_F", "ADD_V",
	"SUB_F", "SUB_V",
	"EQ_F", "EQ_V", "EQ_S", "EQ_E", "EQ_FNC",
	"NE_F", "NE_V", "NE_S", "NE_E", "NE_FNC",
	"LE", "GE", "LT", "GT",
	"INDIRECT", "INDIRECT", "INDIRECT",
	"INDIRECT", "INDIRECT", "INDIRECT",
	"ADDRESS",
	"STORE_F", "STORE_V", "STORE_S",
	"STORE_ENT", "STORE_FLD", "STORE_FNC",
	"STOREP_F", "STOREP_V", "STOREP_S",
	"STOREP_ENT", "STOREP_FLD", "STOREP_FNC",
	"RETURN",
	"NOT_F", "NOT_V", "NOT_S", "NOT_ENT", "NOT_FNC",
	"IF", "IFNOT",
	"CALL0", "CALL1", "CALL2", "CALL3", "CALL4",
	"CALL5", "CALL6", "CALL7", "CALL8",
	"STATE",
	"GOTO",
	"AND", "OR",
	"BITAND", "BITOR",
	"OP_MULSTORE_F", "OP_MULSTORE_V", "OP_MULSTOREP_F", "OP_MULSTOREP_V",
	"OP_DIVSTORE_F", "OP_DIVSTOREP_F",
	"OP_ADDSTORE_F", "OP_ADDSTORE_V", "OP_ADDSTOREP_F", "OP_ADDSTOREP_V",
	"OP_SUBSTORE_F", "OP_SUBSTORE_V", "OP_SUBSTOREP_F", "OP_SUBSTOREP_V",
	"OP_FETCH_GBL_F",
	"OP_FETCH_GBL_V",
	"OP_FETCH_GBL_S",
	"OP_FETCH_GBL_E",
	"OP_FETCH_GBL_FNC",
	"OP_CSTATE", "OP_CWSTATE",

	"OP_THINKTIME",

	"OP_BITSET", "OP_BITSETP", "OP_BITCLR", "OP_BITCLRP",

	"OP_RAND0", "OP_RAND1", "OP_RAND2", "OP_RANDV0", "OP_RANDV1", "OP_RANDV2",

	"OP_SWITCH_F", "OP_SWITCH_V", "OP_SWITCH_S", "OP_SWITCH_E", "OP_SWITCH_FNC",

	"OP_CASE",
	"OP_CASERANGE"
};

void PR_PrintStatement(const dstatement_t* s)
{
	if ((unsigned)s->op < sizeof(pr_opnames) / sizeof(pr_opnames[0]))
	{
		Log::write("%s ", pr_opnames[s->op]);
		int i = String::Length(pr_opnames[s->op]);
		for (; i < 10; i++)
		{
			Log::write(" ");
		}
	}

	if (s->op == OP_IF || s->op == OP_IFNOT)
	{
		Log::write("%sbranch %i", PR_GlobalString(s->a), s->b);
	}
	else if (s->op == OP_GOTO)
	{
		Log::write("branch %i", s->a);
	}
	else if ((unsigned)(s->op - OP_STORE_F) < 6)
	{
		Log::write("%s", PR_GlobalString(s->a));
		Log::write("%s", PR_GlobalStringNoContents(s->b));
	}
	else
	{
		if (s->a)
		{
			Log::write("%s", PR_GlobalString(s->a));
		}
		if (s->b)
		{
			Log::write("%s", PR_GlobalString(s->b));
		}
		if (s->c)
		{
			Log::write("%s", PR_GlobalStringNoContents(s->c));
		}
	}
	Log::write("\n");
}

static void PR_StackTrace()
{
	if (pr_depth == 0)
	{
		common->Printf("<NO STACK>\n");
		return;
	}

	pr_stack[pr_depth].f = pr_xfunction;
	for (int i = pr_depth; i >= 0; i--)
	{
		dfunction_t* f = pr_stack[i].f;
		if (!f)
		{
			common->Printf("<NO FUNCTION>\n");
		}
		else
		{
			common->Printf("%12s : %s\n", PR_GetString(f->s_file), PR_GetString(f->s_name));
		}
	}
}

//	Aborts the currently executing function
void PR_RunError(const char* error, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);

	PR_PrintStatement(pr_statements + pr_xstatement);
	PR_StackTrace();
	common->Printf("%s\n", string);

	pr_depth = 0;		// dump the stack so host_error can shutdown functions

	common->Error("Program error");
}

//	Returns the new program statement counter
int PR_EnterFunction(dfunction_t* f)
{
	int i, j, c, o;

	pr_stack[pr_depth].s = pr_xstatement;
	pr_stack[pr_depth].f = pr_xfunction;
	pr_depth++;
	if (pr_depth >= MAX_STACK_DEPTH)
	{
		PR_RunError("stack overflow");
	}

	// save off any locals that the new function steps on
	c = f->locals;
	if (localstack_used + c > LOCALSTACK_SIZE)
	{
		PR_RunError("PR_ExecuteProgram: locals stack overflow\n");
	}

	for (i = 0; i < c; i++)
	{
		localstack[localstack_used + i] = ((int*)pr_globals)[f->parm_start + i];
	}
	localstack_used += c;

	// copy parameters
	o = f->parm_start;
	for (i = 0; i < f->numparms; i++)
	{
		for (j = 0; j < f->parm_size[i]; j++)
		{
			((int*)pr_globals)[o] = ((int*)pr_globals)[OFS_PARM0 + i * 3 + j];
			o++;
		}
	}

	pr_xfunction = f;
	return f->first_statement - 1;	// offset the s++
}

int PR_LeaveFunction()
{
	int i, c;

	if (pr_depth <= 0)
	{
		common->FatalError("prog stack underflow");
	}

	// restore locals from the stack
	c = pr_xfunction->locals;
	localstack_used -= c;
	if (localstack_used < 0)
	{
		PR_RunError("PR_ExecuteProgram: locals stack underflow\n");
	}

	for (i = 0; i < c; i++)
	{
		((int*)pr_globals)[pr_xfunction->parm_start + i] = localstack[localstack_used + i];
	}

	// up stack
	pr_depth--;
	pr_xfunction = pr_stack[pr_depth].f;
	return pr_stack[pr_depth].s;
}

bool PR_ExecuteProgramCommon(const dstatement_t* st, eval_t* a, eval_t* b, eval_t* c, int& s, int exitdepth)
{
		switch (st->op)
		{
		case OP_ADD_F:
			c->_float = a->_float + b->_float;
			break;
		case OP_ADD_V:
			c->vector[0] = a->vector[0] + b->vector[0];
			c->vector[1] = a->vector[1] + b->vector[1];
			c->vector[2] = a->vector[2] + b->vector[2];
			break;

		case OP_SUB_F:
			c->_float = a->_float - b->_float;
			break;
		case OP_SUB_V:
			c->vector[0] = a->vector[0] - b->vector[0];
			c->vector[1] = a->vector[1] - b->vector[1];
			c->vector[2] = a->vector[2] - b->vector[2];
			break;

		case OP_MUL_F:
			c->_float = a->_float * b->_float;
			break;
		case OP_MUL_V:
			c->_float = a->vector[0] * b->vector[0]
						+ a->vector[1] * b->vector[1]
						+ a->vector[2] * b->vector[2];
			break;
		case OP_MUL_FV:
			c->vector[0] = a->_float * b->vector[0];
			c->vector[1] = a->_float * b->vector[1];
			c->vector[2] = a->_float * b->vector[2];
			break;
		case OP_MUL_VF:
			c->vector[0] = b->_float * a->vector[0];
			c->vector[1] = b->_float * a->vector[1];
			c->vector[2] = b->_float * a->vector[2];
			break;

		case OP_DIV_F:
			c->_float = a->_float / b->_float;
			break;

		case OP_BITAND:
			c->_float = (int)a->_float & (int)b->_float;
			break;

		case OP_BITOR:
			c->_float = (int)a->_float | (int)b->_float;
			break;

		case OP_GE:
			c->_float = a->_float >= b->_float;
			break;
		case OP_LE:
			c->_float = a->_float <= b->_float;
			break;
		case OP_GT:
			c->_float = a->_float > b->_float;
			break;
		case OP_LT:
			c->_float = a->_float < b->_float;
			break;
		case OP_AND:
			c->_float = a->_float && b->_float;
			break;
		case OP_OR:
			c->_float = a->_float || b->_float;
			break;

		case OP_NOT_F:
			c->_float = !a->_float;
			break;
		case OP_NOT_V:
			c->_float = !a->vector[0] && !a->vector[1] && !a->vector[2];
			break;
		case OP_NOT_S:
			c->_float = !a->string || !*PR_GetString(a->string);
			break;
		case OP_NOT_FNC:
			c->_float = !a->function;
			break;
		case OP_NOT_ENT:
			c->_float = (PROG_TO_EDICT(a->edict) == sv.qh_edicts);
			break;

		case OP_EQ_F:
			c->_float = a->_float == b->_float;
			break;
		case OP_EQ_V:
			c->_float = (a->vector[0] == b->vector[0]) &&
						(a->vector[1] == b->vector[1]) &&
						(a->vector[2] == b->vector[2]);
			break;
		case OP_EQ_S:
			c->_float = !String::Cmp(PR_GetString(a->string), PR_GetString(b->string));
			break;
		case OP_EQ_E:
			c->_float = a->_int == b->_int;
			break;
		case OP_EQ_FNC:
			c->_float = a->function == b->function;
			break;

		case OP_NE_F:
			c->_float = a->_float != b->_float;
			break;
		case OP_NE_V:
			c->_float = (a->vector[0] != b->vector[0]) ||
						(a->vector[1] != b->vector[1]) ||
						(a->vector[2] != b->vector[2]);
			break;
		case OP_NE_S:
			c->_float = String::Cmp(PR_GetString(a->string), PR_GetString(b->string));
			break;
		case OP_NE_E:
			c->_float = a->_int != b->_int;
			break;
		case OP_NE_FNC:
			c->_float = a->function != b->function;
			break;

		case OP_STORE_F:
		case OP_STORE_ENT:
		case OP_STORE_FLD:	// integers
		case OP_STORE_S:
		case OP_STORE_FNC:	// pointers
			b->_int = a->_int;
			break;
		case OP_STORE_V:
			b->vector[0] = a->vector[0];
			b->vector[1] = a->vector[1];
			b->vector[2] = a->vector[2];
			break;

		case OP_STOREP_F:
		case OP_STOREP_ENT:
		case OP_STOREP_FLD:	// integers
		case OP_STOREP_S:
		case OP_STOREP_FNC:	// pointers
		{
			eval_t* ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			ptr->_int = a->_int;
			break;
		}
		case OP_STOREP_V:
		{
			eval_t* ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			ptr->vector[0] = a->vector[0];
			ptr->vector[1] = a->vector[1];
			ptr->vector[2] = a->vector[2];
			break;
		}

		case OP_MULSTORE_F:	// f *= f
			b->_float *= a->_float;
			break;
		case OP_MULSTORE_V:	// v *= f
			b->vector[0] *= a->_float;
			b->vector[1] *= a->_float;
			b->vector[2] *= a->_float;
			break;
		case OP_MULSTOREP_F:// e.f *= f
		{
			eval_t* ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			c->_float = (ptr->_float *= a->_float);
			break;
		}
		case OP_MULSTOREP_V:// e.v *= f
		{
			eval_t* ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			c->vector[0] = (ptr->vector[0] *= a->_float);
			c->vector[0] = (ptr->vector[1] *= a->_float);
			c->vector[0] = (ptr->vector[2] *= a->_float);
			break;
		}

		case OP_DIVSTORE_F:	// f /= f
			b->_float /= a->_float;
			break;
		case OP_DIVSTOREP_F:// e.f /= f
		{
			eval_t* ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			c->_float = (ptr->_float /= a->_float);
			break;
		}

		case OP_ADDSTORE_F:	// f += f
			b->_float += a->_float;
			break;
		case OP_ADDSTORE_V:	// v += v
			b->vector[0] += a->vector[0];
			b->vector[1] += a->vector[1];
			b->vector[2] += a->vector[2];
			break;
		case OP_ADDSTOREP_F:// e.f += f
		{
			eval_t* ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			c->_float = (ptr->_float += a->_float);
			break;
		}
		case OP_ADDSTOREP_V:// e.v += v
		{
			eval_t* ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			c->vector[0] = (ptr->vector[0] += a->vector[0]);
			c->vector[1] = (ptr->vector[1] += a->vector[1]);
			c->vector[2] = (ptr->vector[2] += a->vector[2]);
			break;
		}

		case OP_SUBSTORE_F:	// f -= f
			b->_float -= a->_float;
			break;
		case OP_SUBSTORE_V:	// v -= v
			b->vector[0] -= a->vector[0];
			b->vector[1] -= a->vector[1];
			b->vector[2] -= a->vector[2];
			break;
		case OP_SUBSTOREP_F:// e.f -= f
		{
			eval_t* ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			c->_float = (ptr->_float -= a->_float);
			break;
		}
		case OP_SUBSTOREP_V:// e.v -= v
		{
			eval_t* ptr = (eval_t*)((byte*)sv.qh_edicts + b->_int);
			c->vector[0] = (ptr->vector[0] -= a->vector[0]);
			c->vector[1] = (ptr->vector[1] -= a->vector[1]);
			c->vector[2] = (ptr->vector[2] -= a->vector[2]);
			break;
		}

		case OP_ADDRESS:
		{
			qhedict_t* ed = PROG_TO_EDICT(a->edict);
#ifdef PARANOID
			QH_NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
			if (ed == (qhedict_t*)sv.qh_edicts && sv.state == SS_GAME)
			{
				PR_RunError("assignment to world entity");
			}
			c->_int = (byte*)((int*)&ed->v + b->_int) - (byte*)sv.qh_edicts;
			break;
		}

		case OP_LOAD_F:
		case OP_LOAD_FLD:
		case OP_LOAD_ENT:
		case OP_LOAD_S:
		case OP_LOAD_FNC:
		{
			qhedict_t* ed = PROG_TO_EDICT(a->edict);
#ifdef PARANOID
			QH_NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
			a = (eval_t*)((int*)&ed->v + b->_int);
			c->_int = a->_int;
			break;
		}

		case OP_LOAD_V:
		{
			qhedict_t* ed = PROG_TO_EDICT(a->edict);
#ifdef PARANOID
			QH_NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
			a = (eval_t*)((int*)&ed->v + b->_int);
			c->vector[0] = a->vector[0];
			c->vector[1] = a->vector[1];
			c->vector[2] = a->vector[2];
			break;
		}

		case OP_FETCH_GBL_F:
		case OP_FETCH_GBL_S:
		case OP_FETCH_GBL_E:
		case OP_FETCH_GBL_FNC:
		{
			int i = (int)b->_float;
			if (i < 0 || i > G_INT((unsigned short)st->a - 1))
			{
				PR_RunError("array index out of bounds: %d", i);
			}
			a = (eval_t*)&pr_globals[(unsigned short)st->a + i];
			c->_int = a->_int;
			break;
		}
		case OP_FETCH_GBL_V:
		{
			int i = (int)b->_float;
			if (i < 0 || i > G_INT((unsigned short)st->a - 1))
			{
				PR_RunError("array index out of bounds: %d", i);
			}
			a = (eval_t*)&pr_globals[(unsigned short)st->a
									 + ((int)b->_float) * 3];
			c->vector[0] = a->vector[0];
			c->vector[1] = a->vector[1];
			c->vector[2] = a->vector[2];
			break;
		}

		case OP_IFNOT:
			if (!a->_int)
			{
				s += st->b - 1;	// offset the s++
			}
			break;

		case OP_IF:
			if (a->_int)
			{
				s += st->b - 1;	// offset the s++
			}
			break;

		case OP_GOTO:
			s += st->a - 1;	// offset the s++
			break;

		case OP_CALL8:
		case OP_CALL7:
		case OP_CALL6:
		case OP_CALL5:
		case OP_CALL4:
		case OP_CALL3:
		case OP_CALL2:
			if (GGameType & GAME_Hexen2)
			{
				// Copy second arg to shared space
				VectorCopy(c->vector, G_VECTOR(OFS_PARM1));
			}
		case OP_CALL1:
			if (GGameType & GAME_Hexen2)
			{
				// Copy first arg to shared space
				VectorCopy(b->vector, G_VECTOR(OFS_PARM0));
			}
		case OP_CALL0:
		{
			pr_argc = st->op - OP_CALL0;
			if (!a->function)
			{
				PR_RunError("NULL function");
			}

			dfunction_t* newf = &pr_functions[a->function];

			if (newf->first_statement < 0)
			{
				// negative statements are built in functions
				int i = -newf->first_statement;
				if (i >= pr_numbuiltins)
				{
					PR_RunError("Bad builtin call number");
				}
				pr_builtins[i]();
				break;
			}

			s = PR_EnterFunction(newf);
			break;
		}

		case OP_DONE:
		case OP_RETURN:
			pr_globals[OFS_RETURN] = pr_globals[(unsigned short)st->a];
			pr_globals[OFS_RETURN + 1] = pr_globals[(unsigned short)st->a + 1];
			pr_globals[OFS_RETURN + 2] = pr_globals[(unsigned short)st->a + 2];

			s = PR_LeaveFunction();
			if (pr_depth == exitdepth)
			{
				return true;	// all done
			}
			break;

		default:
			PR_RunError("Bad opcode %i", st->op);
		}
	return false;
}
