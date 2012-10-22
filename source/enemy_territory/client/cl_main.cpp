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

// cl_main.c  -- client main loop

#include "../game/q_shared.h"
#include "../../client/client.h"
#include "../qcommon/qcommon.h"
#include "../../client/game/et/local.h"
#include <limits.h>

#include "../../server/public.h"

/*
==================
CL_WWWBadChecksum

FS code calls this when doing FS_ComparePaks
we can detect files that we got from a www dl redirect with a wrong checksum
this indicates that the redirect setup is broken, and next dl attempt should NOT redirect
==================
*/
bool CL_WWWBadChecksum(const char* pakname)
{
	if (strstr(clc.et_redirectedList, va("@%s", pakname)))
	{
		common->Printf("WARNING: file %s obtained through download redirect has wrong checksum\n", pakname);
		common->Printf("         this likely means the server configuration is broken\n");
		if (String::Length(clc.et_badChecksumList) + String::Length(pakname) + 1 >= (int)sizeof(clc.et_badChecksumList))
		{
			common->Printf("ERROR: badChecksumList overflowed (%s)\n", clc.et_badChecksumList);
			return false;
		}
		strcat(clc.et_badChecksumList, "@");
		strcat(clc.et_badChecksumList, pakname);
		common->DPrintf("bad checksums: %s\n", clc.et_badChecksumList);
		return true;
	}
	return false;
}
