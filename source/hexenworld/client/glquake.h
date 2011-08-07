// disable data conversion warnings

#ifdef _WIN32
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
#else
#define PROC void*
#endif

#include "../../client/render_local.h"

#include <GL/glu.h>

// r_local.h -- private refresh defs

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

void R_TimeRefresh_f (void);

//====================================================

//
// screen size info
//
extern	QCvar*	gl_reporttjunctions;

void R_InitParticles (void);
void R_ClearParticles (void);
void Draw_RedString (int x, int y, const char *str);
