/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// vm.c -- virtual machine

/*


intermix code and data
symbol table

a dll has one imported function: VM_SystemCall
and one exported function: Perform


*/

#include "../../common/qcommon.h"
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
	Cvar_Get("vm_cgame", "2", CVAR_ARCHIVE);	// !@# SHIP WITH SET TO 2
	Cvar_Get("vm_game", "2", CVAR_ARCHIVE);		// !@# SHIP WITH SET TO 2
	Cvar_Get("vm_ui", "2", CVAR_ARCHIVE);		// !@# SHIP WITH SET TO 2

	Cmd_AddCommand("vmprofile", VM_VmProfile_f);
	Cmd_AddCommand("vminfo", VM_VmInfo_f);

	Com_Memset(vmTable, 0, sizeof(vmTable));
}
