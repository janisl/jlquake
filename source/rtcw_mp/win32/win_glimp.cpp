#include <assert.h>
#include "../renderer/tr_local.h"
#include "../qcommon/qcommon.h"
#include "resource.h"
#include "win_local.h"

/*
** GLimp_EndFrame
*/
void GLimp_EndFrame( void ) {
	//
	// swapinterval stuff
	//
	if ( r_swapInterval->modified ) {
		r_swapInterval->modified = qfalse;

		if ( !glConfig.stereoEnabled ) {    // why?
			if ( qwglSwapIntervalEXT ) {
				qwglSwapIntervalEXT( r_swapInterval->integer );
			}
		}
	}


	// don't flip if drawing to front buffer
	if ( String::ICmp( r_drawBuffer->string, "GL_FRONT" ) != 0 ) {
		GLimp_SwapBuffers();
	}

	// check logging
	QGL_EnableLogging( r_logFile->integer );
}
