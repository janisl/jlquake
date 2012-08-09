// sv_main.c -- server main program

/*
 * $Header: /H2 Mission Pack/SV_MAIN.C 26    3/20/98 12:12a Jmonroe $
 */

#include "quakedef.h"
#include "../common/file_formats/bsp29.h"

Cvar* sv_sound_distance;

Cvar* sv_idealrollscale;

qboolean skip_start = false;
int num_intro_msg = 0;

//============================================================================

void Sv_Edicts_f(void);

//JL Used in progs
Cvar* sv_walkpitch;

/*
===============
SV_Init
===============
*/
void SV_Init(void)
{
	int i;

	SVQH_RegisterPhysicsCvars();
	svqh_idealpitchscale = Cvar_Get("sv_idealpitchscale","0.8", 0);
	sv_idealrollscale = Cvar_Get("sv_idealrollscale","0.8", 0);
	svqh_aim = Cvar_Get("sv_aim", "0.93", 0);
	sv_walkpitch = Cvar_Get("sv_walkpitch", "0", 0);
	sv_sound_distance = Cvar_Get("sv_sound_distance","800", CVAR_ARCHIVE);
	svh2_update_player    = Cvar_Get("sv_update_player","1", CVAR_ARCHIVE);
	svh2_update_monsters  = Cvar_Get("sv_update_monsters","1", CVAR_ARCHIVE);
	svh2_update_missiles  = Cvar_Get("sv_update_missiles","1", CVAR_ARCHIVE);
	svh2_update_misc      = Cvar_Get("sv_update_misc","1", CVAR_ARCHIVE);
	sv_ce_scale         = Cvar_Get("sv_ce_scale","0", CVAR_ARCHIVE);
	sv_ce_max_size      = Cvar_Get("sv_ce_max_size","0", CVAR_ARCHIVE);

	Cmd_AddCommand("sv_edicts", Sv_Edicts_f);

	for (i = 0; i < MAX_MODELS_H2; i++)
		sprintf(svqh_localmodels[i], "*%i", i);

	svh2_kingofhill = 0;	//Initialize King of Hill to world
}

void SV_Edicts(const char* Name)
{
	fileHandle_t FH;
	int i;
	qhedict_t* e;

	FH = FS_FOpenFileWrite(Name);
	if (!FH)
	{
		common->Printf("Could not open %s\n",Name);
		return;
	}

	FS_Printf(FH,"Number of Edicts: %d\n",sv.qh_num_edicts);
	FS_Printf(FH,"Server Time: %f\n",sv.qh_time);
	FS_Printf(FH,"\n");
	FS_Printf(FH,"Num.     Time Class Name                     Model                          Think                                    Touch                                    Use\n");
	FS_Printf(FH,"---- -------- ------------------------------ ------------------------------ ---------------------------------------- ---------------------------------------- ----------------------------------------\n");

	for (i = 1; i < sv.qh_num_edicts; i++)
	{
		e = QH_EDICT_NUM(i);
		FS_Printf(FH,"%3d. %8.2f %-30s %-30s %-40s %-40s %-40s\n",
			i,e->GetNextThink(),PR_GetString(e->GetClassName()),PR_GetString(e->GetModel()),
			PR_GetString(pr_functions[e->GetThink()].s_name),PR_GetString(pr_functions[e->GetTouch()].s_name),
			PR_GetString(pr_functions[e->GetUse()].s_name));
	}
	FS_FCloseFile(FH);
}

void Sv_Edicts_f(void)
{
	const char* Name;

	if (sv.state == SS_DEAD)
	{
		common->Printf("This command can only be executed on a server running a map\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Name = "edicts.txt";
	}
	else
	{
		Name = Cmd_Argv(1);
	}

	SV_Edicts(Name);
}
