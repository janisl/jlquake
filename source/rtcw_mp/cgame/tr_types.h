/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __TR_TYPES_H
#define __TR_TYPES_H


#define MAX_CORONAS     32          //----(SA)	not really a reason to limit this other than trying to keep a reasonable count
#define MAX_ENTITIES    1023        // can't be increased without changing drawsurf bit packing

// renderfx flags
#define RF_MINLIGHT         1       // allways have some light (viewmodel, some items)
#define RF_THIRD_PERSON     2       // don't draw through eyes, only mirrors (player bodies, chat sprites)
#define RF_FIRST_PERSON     4       // only draw through eyes (view weapon, damage blood blob)
#define RF_DEPTHHACK        8       // for view weapon Z crunching
#define RF_NOSHADOW         64      // don't add stencil shadows

#define RF_LIGHTING_ORIGIN  128     // use refEntity->lightingOrigin instead of refEntity->origin
									// for lighting.  This allows entities to sink into the floor
									// with their origin going solid, and allows all parts of a
									// player to get the same lighting
#define RF_SHADOW_PLANE     256     // use refEntity->shadowPlane
#define RF_WRAP_FRAMES      512     // mod the model frames by the maxframes to allow continuous
									// animation without needing to know the frame count

#define RF_HILIGHT          ( 1 << 8 )  // more than RF_MINLIGHT.  For when an object is "Highlighted" (looked at/training identification/etc)
#define RF_BLINK            ( 1 << 9 )  // eyes in 'blink' state

// refdef flags
#define RDF_NOWORLDMODEL    1       // used for player configuration screen
#define RDF_HYPERSPACE      4       // teleportation effect

// Rafael
#define RDF_SKYBOXPORTAL    8

//----(SA)
#define RDF_UNDERWATER      ( 1 << 4 )  // so the renderer knows to use underwater fog when the player is underwater
#define RDF_DRAWINGSKY      ( 1 << 5 )
#define RDF_SNOOPERVIEW     ( 1 << 6 )  //----(SA)	added


typedef struct {
	vec3_t xyz;
	float st[2];
	byte modulate[4];
} polyVert_t;

typedef struct poly_s {
	qhandle_t hShader;
	int numVerts;
	polyVert_t          *verts;
} poly_t;

typedef enum {
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_SPLASH,  // ripple effect
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_CORE_TAPER, // a modified core that creates a properly texture mapped core that's wider at one end
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,       // doesn't draw anything, just info for portals

	RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

#define ZOMBIEFX_FADEOUT_TIME   10000

#define REFLAG_ONLYHAND     1   // only draw hand surfaces
#define REFLAG_FORCE_LOD    8   // force a low lod
#define REFLAG_ORIENT_LOD   16  // on LOD switch, align the model to the player's camera
#define REFLAG_DEAD_LOD     32  // allow the LOD to go lower than recommended

typedef struct {
	refEntityType_t reType;
	int renderfx;

	qhandle_t hModel;               // opaque type outside refresh

	// most recent data
	vec3_t lightingOrigin;          // so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float shadowPlane;              // projection shadows go here, stencils go slightly lower

	vec3_t axis[3];                 // rotation vectors
	vec3_t torsoAxis[3];            // rotation vectors for torso section of skeletal animation
	qboolean nonNormalizedAxes;     // axis are not normalized, i.e. they have scale
	float origin[3];                // also used as MODEL_BEAM's "from"
	int frame;                      // also used as MODEL_BEAM's diameter
	int torsoFrame;                 // skeletal torso can have frame independant of legs frame

	// previous data for frame interpolation
	float oldorigin[3];             // also used as MODEL_BEAM's "to"
	int oldframe;
	int oldTorsoFrame;
	float backlerp;                 // 0.0 = current, 1.0 = old
	float torsoBacklerp;

	// texturing
	int skinNum;                    // inline skin index
	qhandle_t customSkin;           // NULL for default skin
	qhandle_t customShader;         // use one image for the entire thing

	// misc
	byte shaderRGBA[4];             // colors used by rgbgen entity shaders
	float shaderTexCoord[2];        // texture coordinates used by tcMod entity modifiers
	float shaderTime;               // subtracted from refdef time to control effect start times

	// extra sprite information
	float radius;
	float rotation;

	// Ridah
	vec3_t fireRiseDir;

	// Ridah, entity fading (gibs, debris, etc)
	int fadeStartTime, fadeEndTime;

	float hilightIntensity;         //----(SA)	added

	int reFlags;

	int entityNum;                  // currentState.number, so we can attach rendering effects to specific entities (Zombie)

} refEntity_t;

//----(SA)

//                                                                  //
// WARNING:: synch FOG_SERVER in sv_ccmds.c if you change anything	//
//                                                                  //
typedef enum {
	FOG_NONE,       //	0

	FOG_SKY,        //	1	fog values to apply to the sky when using density fog for the world (non-distance clipping fog) (only used if(glfogsettings[FOG_MAP].registered) or if(glfogsettings[FOG_MAP].registered))
	FOG_PORTALVIEW, //	2	used by the portal sky scene
	FOG_HUD,        //	3	used by the 3D hud scene

	//		The result of these for a given frame is copied to the scene.glFog when the scene is rendered

	// the following are fogs applied to the main world scene
	FOG_MAP,        //	4	use fog parameter specified using the "fogvars" in the sky shader
	FOG_WATER,      //	5	used when underwater
	FOG_SERVER,     //	6	the server has set my fog (probably a target_fog) (keep synch in sv_ccmds.c !!!)
	FOG_CURRENT,    //	7	stores the current values when a transition starts
	FOG_LAST,       //	8	stores the current values when a transition starts
	FOG_TARGET,     //	9	the values it's transitioning to.

	FOG_CMD_SWITCHFOG,  // 10	transition to the fog specified in the second parameter of R_SetFog(...) (keep synch in sv_ccmds.c !!!)

	NUM_FOGS
} glfogType_t;

typedef enum {
	STEREO_CENTER,
	STEREO_LEFT,
	STEREO_RIGHT
} stereoFrame_t;

// font support

#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND 127
#define GLYPHS_PER_FONT GLYPH_END - GLYPH_START + 1
typedef struct {
	int height;     // number of scan lines
	int top;        // top of glyph in buffer
	int bottom;     // bottom of glyph in buffer
	int pitch;      // width for copying
	int xSkip;      // x adjustment
	int imageWidth; // width of actual image
	int imageHeight; // height of actual image
	float s;        // x offset in image where glyph starts
	float t;        // y offset in image where glyph starts
	float s2;
	float t2;
	qhandle_t glyph; // handle to the shader with the glyph
	char shaderName[32];
} glyphInfo_t;

typedef struct {
	glyphInfo_t glyphs [GLYPHS_PER_FONT];
	float glyphScale;
	char name[MAX_QPATH];
} fontInfo_t;


#endif  // __TR_TYPES_H
