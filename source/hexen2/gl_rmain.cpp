// gl_main.c

/*
 * $Header: /H2 Mission Pack/gl_rmain.c 4     3/30/98 10:57a Jmonroe $
 */

#include "quakedef.h"
#include "glquake.h"

qboolean	r_cache_thrash;		// compatability

image_t*	playertextures[16];		// up to 16 color translated skins
image_t*	gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

float		r_time1;
float		r_lasttime1 = 0;

extern qhandle_t	player_models[NUM_CLASSES];

//
// screen size info
//
refdef_t	r_refdef;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


QCvar*	r_norefresh;
QCvar*	r_drawviewmodel;
QCvar*	r_wholeframe;

QCvar*	gl_polyblend;
QCvar*	gl_nocolors;
QCvar*	gl_reporttjunctions;

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

void R_HandleCustomSkin(refEntity_t* Ent, int PlayerNum)
{
	if (Ent->skinNum >= 100)
	{
		if (Ent->skinNum > 255) 
		{
			Sys_Error ("skinnum > 255");
		}

		if (!gl_extra_textures[Ent->skinNum - 100])  // Need to load it in
		{
			char temp[40];
			QStr::Sprintf(temp, sizeof(temp), "gfx/skin%d.lmp", Ent->skinNum);
			gl_extra_textures[Ent->skinNum - 100] = Draw_CachePic(temp);
		}

		Ent->customSkin = R_GetImageHandle(gl_extra_textures[Ent->skinNum - 100]);
	}
	else if (PlayerNum >= 0 && !gl_nocolors->value && Ent->hModel)
	{
		// we can't dynamically colormap textures, so they are cached
		// seperately for the players.  Heads are just uncolored.
		if (Ent->hModel == player_models[0] ||
			Ent->hModel == player_models[1] ||
			Ent->hModel == player_models[2] ||
			Ent->hModel == player_models[3] ||
			Ent->hModel == player_models[4])
		{
			Ent->customSkin = R_GetImageHandle(playertextures[PlayerNum]);
		}
	}
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!gl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_TEXTURE_2D);

    qglLoadIdentity ();

    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

	qglColor4fv (v_blend);

	qglBegin (GL_QUADS);

	qglVertex3f (10, 10, 10);
	qglVertex3f (10, -10, 10);
	qglVertex3f (10, -10, -10);
	qglVertex3f (10, 10, -10);
	qglEnd ();

	qglEnable (GL_TEXTURE_2D);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80);
}


/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int				edgecount;
	vrect_t			vrect;
	float			w, h;

// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
		Cvar_Set ("r_fullbright", "0");

	R_AnimateLight ();

	tr.viewCount++;

	//
	// texturemode stuff
	//
	if (r_textureMode->modified)
	{
		GL_TextureMode(r_textureMode->string);
		r_textureMode->modified = false;
	}

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeafQ1(tr.viewParms.orient.origin, tr.worldModel);

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	r_cache_thrash = false;

	c_brush_polys = c_alias_polys = 0;

}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	int		x, x2, y2, y, w, h;

	//
	// set up viewpoint
	//
	// JACK: Changes for non-scaled
	x = tr.refdef.x;
	x2 = tr.refdef.x + tr.refdef.width;
	y = glConfig.vidHeight - tr.refdef.y;
	y2 = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < glConfig.vidWidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < glConfig.vidHeight)
		y++;

	w = x2 - x;
	h = y - y2;

	tr.viewParms.viewportX = x;
	tr.viewParms.viewportY = y2; 
	tr.viewParms.viewportWidth = w;
	tr.viewParms.viewportHeight = h;
	R_SetupProjection();
	R_RotateForViewer();

	backEnd.viewParms = tr.viewParms;
	RB_BeginDrawingView();
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene ()
{
	R_SetupFrame ();

	VectorCopy(tr.refdef.vieworg, tr.viewParms.orient.origin);
	VectorCopy(tr.refdef.viewaxis[0], tr.viewParms.orient.axis[0]);
	VectorCopy(tr.refdef.viewaxis[1], tr.viewParms.orient.axis[1]);
	VectorCopy(tr.refdef.viewaxis[2], tr.viewParms.orient.axis[2]);
	tr.viewParms.fovX = tr.refdef.fov_x;
	tr.viewParms.fovY = tr.refdef.fov_y;
	R_SetupFrustum();

	R_SetupGL ();

	R_GenerateDrawSurfs();
}


/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (r_clear->value)
	{
		qglClearColor(1, 0, 0, 0);
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}

/*
=============
R_PrintTimes
=============
*/
void R_PrintTimes(void)
{
	float r_time2;
	float ms, fps;

	r_lasttime1 = r_time2 = Sys_DoubleTime();

	ms = 1000*(r_time2-r_time1);
	fps = 1000/ms;

	Con_Printf("%3.1f fps %5.0f ms\n%4i wpoly  %4i epoly\n",
		fps, ms, c_brush_polys, c_alias_polys);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	double	time1, time2;

	if (r_norefresh->value)
		return;

	for (int i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		float Val = d_lightstylevalue[i] / 256.0;
		R_AddLightStyleToScene(i, Val, Val, Val);
	}
	CL_AddParticles();

	tr.frameCount++;
	tr.frameSceneNum = 0;

	glState.finishCalled = false;

	if (!tr.worldModel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		if (r_wholeframe->value)
		   r_time1 = r_lasttime1;
	   else
		   r_time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	QGL_EnableLogging(!!r_logFile->integer);

	r_refdef.time = (int)(cl.time * 1000);
	R_CommonRenderScene(&r_refdef);

	R_PushDlightsQ1 ();

	R_Clear ();

	// render normal view
	R_RenderScene ();

	R_PolyBlend ();

	if (r_speeds->value)
		R_PrintTimes ();
}

void R_RenderView(viewParms_t *parms)
{
}
