// r_misc.c

#include "quakedef.h"
#include "../client/render_local.h"

void D_ShowLoadingSize(void)
{
	if (!tr.registered)
		return;

	qglDrawBuffer  (GL_FRONT);

	SCR_DrawLoading();

	qglDrawBuffer  (GL_BACK);
}
