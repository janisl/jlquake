// cvar.c -- dynamic variable tracking

#ifdef SERVERONLY
#include "qwsvdef.h"
#else
#include "quakedef.h"
#endif

void Cvar_Changed(Cvar* var)
{
#ifdef SERVERONLY
	if (var->flags & CVAR_SERVERINFO && var->name[0] != '*')
	{
		Info_SetValueForKey(svs.qh_info, var->name, var->string, MAX_SERVERINFO_STRING,
			64, 64, !svqh_highchars || !svqh_highchars->value, false);
		SVQH_BroadcastCommand("fullserverinfo \"%s\"\n", svs.qh_info);
	}
#else
	if (var->flags & CVAR_USERINFO && var->name[0] != '*')
	{
		Info_SetValueForKey(cls.qh_userinfo, var->name, var->string, MAX_INFO_STRING_QW, 64, 64,
			String::ICmp(var->name, "name") != 0, String::ICmp(var->name, "team") == 0);
		if (cls.state == CA_CONNECTED || cls.state == CA_LOADING || cls.state == CA_ACTIVE)
		{
			clc.netchan.message.WriteByte(h2clc_stringcmd);
			clc.netchan.message.WriteString2(va("setinfo \"%s\" \"%s\"\n", var->name, var->string));
		}
	}
#endif
}

const char* Cvar_TranslateString(const char* string)
{
	return string;
}
