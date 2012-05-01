/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// vm.c -- virtual machine

/*


intermix code and data
symbol table

a dll has one imported function: VM_SystemCall
and one exported function: Perform


*/

#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../common/virtual_machine/local.h"

void VM_VmInfo_f(void);
void VM_VmProfile_f(void);


/*
==============
VM_Init
==============
*/
void VM_Init(void)
{
	Cvar_Get("vm_cgame", "0", CVAR_ARCHIVE);
	Cvar_Get("vm_game",  "0", CVAR_ARCHIVE);
	Cvar_Get("vm_ui",    "0", CVAR_ARCHIVE);

	Cmd_AddCommand("vmprofile", VM_VmProfile_f);
	Cmd_AddCommand("vminfo", VM_VmInfo_f);

	Com_Memset(vmTable, 0, sizeof(vmTable));
}
