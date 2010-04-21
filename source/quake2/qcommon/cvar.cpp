/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "qcommon.h"

void Cvar_Changed(QCvar* var)
{
}

/*
============
Cvar_GetLatchedVars

Any variables with latched values will now be updated
============
*/
void Cvar_GetLatchedVars (void)
{
	QCvar	*var;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (!var->latchedString)
			continue;
		Mem_Free (var->string);
		var->string = var->latchedString;
		var->latchedString = NULL;
		var->value = QStr::Atof(var->string);
		if (!QStr::Cmp(var->name, "game"))
		{
			FS_SetGamedir (var->string);
			FS_ExecAutoexec ();
		}
	}
}
