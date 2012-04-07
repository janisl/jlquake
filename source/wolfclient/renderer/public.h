//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

typedef int qhandle_t;

//==========================================================================
//
//	Definitions starting here are also used by Quake 3 vms and should not
// be changed.
//

#define MAX_DLIGHTS		32			// can't be increased, because bit flags are used on surfaces

#define ZOMBIEFX_FADEOUT_TIME	10000

/*
** glconfig_t
**
** Contains variables specific to the OpenGL configuration
** being run right now.  These are constant once the OpenGL
** subsystem is initialized.
*/
enum textureCompression_t
{
	TC_NONE,
	TC_S3TC,
	TC_EXT_COMP_S3TC
};

enum glDriverType_t
{
	GLDRV_ICD,					// driver is integrated with window system
};

enum glHardwareType_t
{
	GLHW_GENERIC,			// where everthing works the way it should
};

// renderfx flags
#define RF_MINLIGHT			1		// allways have some light (viewmodel, some items)
#define RF_THIRD_PERSON		2		// don't draw through eyes, only mirrors (player bodies, chat sprites)
#define RF_FIRST_PERSON		4		// only draw through eyes (view weapon, damage blood blob)
#define RF_DEPTHHACK		8		// for view weapon Z crunching
#define RF_NOSHADOW			64		// don't add stencil shadows
#define RF_LIGHTING_ORIGIN	128		// use refEntity->lightingOrigin instead of refEntity->origin
									// for lighting.  This allows entities to sink into the floor
									// with their origin going solid, and allows all parts of a
									// player to get the same lighting
#define RF_SHADOW_PLANE		256		// use refEntity->shadowPlane
#define RF_WRAP_FRAMES		512		// mod the model frames by the maxframes to allow continuous
									// animation without needing to know the frame count
//	New flags
#define RF_WATERTRANS		16		// translucent, alpha from r_wateralpha cvar.
#define RF_TRANSLUCENT		32		// translucent, alpha in shaderRGBA
#define RF_COLORSHADE		1024	// multiply light with colour in shaderRGBA
#define RF_ABSOLUTE_LIGHT	2048	// fixed light, value in radius
#define RF_COLOUR_SHELL		4096	// draws coloured shell for the model
#define RF_GLOW				8192	// pulse lighting for bonus items
#define RF_IR_VISIBLE		16384	// in red light when infrared googles are on
#define RF_LEFTHAND			0x8000	// left hand weapon, flip projection matrix.
#define RF_BLINK			BIT(16)	// eyes in 'blink' state
#define RF_FORCENOLOD       BIT(17)

//	reFlags flags
#define REFLAG_ONLYHAND			1   // only draw hand surfaces
#define REFLAG_FORCE_LOD		8   // force a low lod
#define REFLAG_ORIENT_LOD		16  // on LOD switch, align the model to the player's camera
#define REFLAG_DEAD_LOD			32  // allow the LOD to go lower than recommended
#define REFLAG_SCALEDSPHERECULL	64  // on LOD switch, align the model to the player's camera
#define REFLAG_FULL_LOD			8   // force a FULL lod

struct polyVert_t
{
	vec3_t			xyz;
	float			st[2];
	byte			modulate[4];
};

struct poly_t
{
	qhandle_t		hShader;
	int				numVerts;
	polyVert_t		*verts;
};

// Gordon, these MUST NOT exceed the values for SHADER_MAX_VERTEXES/SHADER_MAX_INDEXES
#define MAX_PB_VERTS		1025
#define MAX_PB_INDICIES		(MAX_PB_VERTS * 6)

struct polyBuffer_t
{
	vec4_t xyz[MAX_PB_VERTS];
	vec2_t st[MAX_PB_VERTS];
	byte color[MAX_PB_VERTS][4];
	int numVerts;

	int indicies[MAX_PB_INDICIES];
	int numIndicies;

	qhandle_t shader;
};

// refdef flags
#define RDF_NOWORLDMODEL	1		// used for player configuration screen
#define RDF_HYPERSPACE		4		// teleportation effect
#define RDF_SKYBOXPORTAL	BIT(3)
#define RDF_UNDERWATER		BIT(4)	// so the renderer knows to use underwater fog when the player is underwater
#define RDF_DRAWINGSKY		BIT(5)
#define RDF_SNOOPERVIEW		BIT(6)
#define RDF_DRAWSKYBOX		BIT(7)	// the above marks a scene as being a 'portal sky'.  this flag says to draw it or not
//	New flags
#define RDF_IRGOGGLES		2

#define MAX_RENDER_STRINGS			8
#define MAX_RENDER_STRING_LENGTH	32

//                                                                  //
// WARNING:: synch FOG_SERVER in sv_ccmds.c if you change anything	//
//                                                                  //
enum glfogType_t
{
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
};

#define REF_FORCE_DLIGHT	BIT(31)	// RF, passed in through overdraw parameter, force this dlight under all conditions
#define REF_JUNIOR_DLIGHT	BIT(30)	// (SA) this dlight does not light surfaces.  it only affects dynamic light grid
#define REF_DIRECTED_DLIGHT	BIT(29)	// ydnar: global directional light, origin should be interpreted as a normal vector

// markfragments are returned by R_MarkFragments()
struct markFragment_t
{
	int		firstPoint;
	int		numPoints;
};

// font support 

#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND 127
#define GLYPHS_PER_FONT GLYPH_END - GLYPH_START + 1

struct glyphInfo_t
{
	int			height;			// number of scan lines
	int			top;			// top of glyph in buffer
	int			bottom;			// bottom of glyph in buffer
	int			pitch;			// width for copying
	int			xSkip;			// x adjustment
	int			imageWidth;		// width of actual image
	int			imageHeight;	// height of actual image
	float		s;				// x offset in image where glyph starts
	float		t;				// y offset in image where glyph starts
	float		s2;
	float		t2;
	qhandle_t	glyph;			// handle to the shader with the glyph
	char		shaderName[32];
};

struct fontInfo_t
{
	glyphInfo_t	glyphs [GLYPHS_PER_FONT];
	float		glyphScale;
	char		name[MAX_QPATH];
};

//
//	End of definitions used by Quake 3 vms.
//
//==========================================================================

enum refEntityType_t
{
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_SPLASH,  // ripple effect
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_CORE_TAPER, // a modified core that creates a properly texture mapped core that's wider at one end
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,		// doesn't draw anything, just info for portals

	RT_MAX_REF_ENTITY_TYPE
};

enum QParticleTexture
{
	PARTTEX_Default,
	//	Hexen 2 snow particles
	PARTTEX_Snow1,
	PARTTEX_Snow2,
	PARTTEX_Snow3,
	PARTTEX_Snow4,
};

enum stereoFrame_t
{
	STEREO_CENTER,
	STEREO_LEFT,
	STEREO_RIGHT
};

//	Games must have their own constants because in Quake/Hexen 2 it's
// 64 and it's used by server.
#define MAX_LIGHTSTYLES			256

struct image_t;

struct refEntity_t
{
	refEntityType_t reType;
	int renderfx;

	qhandle_t hModel;			// opaque type outside refresh

	// most recent data
	vec3_t lightingOrigin;		// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float shadowPlane;			// projection shadows go here, stencils go slightly lower

	vec3_t axis[3];				// rotation vectors
	vec3_t torsoAxis[3];		// rotation vectors for torso section of skeletal animation
	bool nonNormalizedAxes;		// axis are not normalized, i.e. they have scale
	vec3_t origin;				// also used as MODEL_BEAM's "from"
	int frame;					// also used as MODEL_BEAM's diameter
	qhandle_t frameModel;
	int torsoFrame;				// skeletal torso can have frame independant of legs frame
	qhandle_t torsoFrameModel;

	// previous data for frame interpolation
	vec3_t oldorigin;			// also used as MODEL_BEAM's "to"
	int oldframe;
	qhandle_t oldframeModel;
	int oldTorsoFrame;
	qhandle_t oldTorsoFrameModel;
	float backlerp;				// 0.0 = current, 1.0 = old
	float torsoBacklerp;

	// texturing
	int skinNum;				// inline skin index
	qhandle_t customSkin;		// NULL for default skin
	qhandle_t customShader;		// use one image for the entire thing

	// misc
	byte shaderRGBA[4];			// colors used by rgbgen entity shaders
	float shaderTexCoord[2];	// texture coordinates used by tcMod entity modifiers
	float shaderTime;			// subtracted from refdef time to control effect start times

	// extra sprite information
	float radius;
	float rotation;

	float syncBase;

	vec3_t scale;       //----(SA)	added

	// Ridah
	vec3_t fireRiseDir;

	// Ridah, entity fading (gibs, debris, etc)
	int fadeStartTime, fadeEndTime;

	float hilightIntensity;         //----(SA)	added

	int reFlags;

	int entityNum;                  // currentState.number, so we can attach rendering effects to specific entities (Zombie)
};

struct glfog_t
{
	int mode;                   // GL_LINEAR, GL_EXP
	int hint;                   // GL_DONT_CARE
	int startTime;              // in ms
	int finishTime;             // in ms
	float color[4];
	float start;                // near
	float end;                  // far
	bool useEndForClip;     // use the 'far' value for the far clipping plane
	float density;              // 0.0-1.0
	bool registered;        // has this fog been set up?
	bool drawsky;           // draw skybox
	bool clearscreen;       // clear the GL color buffer

	int dirty;
};

struct refdef_t
{
	int			x;
	int			y;
	int			width;
	int			height;
	float		fov_x;
	float		fov_y;
	vec3_t		vieworg;
	vec3_t		viewaxis[3];		// transformation matrix

	// time in milliseconds for shader effects and other time dependent rendering issues
	int			time;

	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	//	needed to pass fog infos into the portal sky scene
	glfog_t glfog;
};

struct glconfig_t
{
	char					renderer_string[MAX_STRING_CHARS];
	char					vendor_string[MAX_STRING_CHARS];
	char					version_string[MAX_STRING_CHARS];
	char					extensions_string[BIG_INFO_STRING];

	int						maxTextureSize;			// queried from GL
	int						maxActiveTextures;		// multitexture ability

	int						colorBits, depthBits, stencilBits;

	glDriverType_t			driverType;
	glHardwareType_t		hardwareType;

	qboolean				deviceSupportsGamma;
	textureCompression_t	textureCompression;
	qboolean				textureEnvAddAvailable;
	bool anisotropicAvailable;                  //----(SA)	added
	bool textureFilterAnisotropicAvailable;                 //DAJ
	float maxAnisotropy;                            //----(SA)	added

	// vendor-specific support
	// NVidia
	bool NVFogAvailable;                        //----(SA)	added
	int NVFogMode;                                  //----(SA)	added
	// ATI
	int ATIMaxTruformTess;                          // for truform support
	int ATINormalMode;                          // for truform support
	int ATIPointMode;                           // for truform support

	int						vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float					windowAspect;

	int						displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Win32 ICD that used CDS will have this set to TRUE.
	qboolean				isFullscreen;
	qboolean				stereoEnabled;
	qboolean				smpActive;		// dual processor
};

#if 0
void R_BeginRegistration(glconfig_t* glconfig);
void R_EndRegistration();
void R_Shutdown(bool destroyWindow);

void R_BeginFrame(stereoFrame_t stereoFrame);
void R_EndFrame(int* frontEndMsec, int* backEndMsec);
#endif

image_t* R_PicFromWad(const char* Name);
image_t* R_PicFromWadRepeat(const char* name);
qhandle_t R_GetImageHandle(image_t* Image);
void R_CreateOrUpdateTranslatedImage(image_t*& image, const char* name, byte* pixels, byte *translation, int width, int height);
void R_CreateOrUpdateTranslatedSkin(image_t*& image, const char* name, byte* pixels, byte *translation, int width, int height);
image_t* R_LoadRawFontImageFromFile(const char* name, int width, int height);
image_t* R_LoadRawFontImageFromWad(const char* name, int width, int height);
image_t* R_LoadBigFontImage(const char* name);
image_t* R_LoadQuake2FontImage(const char* name);
image_t* R_CreateCrosshairImage();
image_t* R_CachePic(const char* path);
image_t* R_CachePicRepeat(const char* path);
image_t* R_CachePicWithTransPixels(const char* path, byte* TransPixels);
image_t* R_RegisterPic(const char* name);
image_t* R_RegisterPicRepeat(const char* name);
#if 0
void R_CreateOrUpdateTranslatedModelSkinQ1(image_t*& image, const char* name, qhandle_t modelHandle, byte *translation);
void R_CreateOrUpdateTranslatedModelSkinH2(image_t*& image, const char* name, qhandle_t modelHandle, byte *translation, int classIndex);
byte* R_LoadQuakeWorldSkinData(const char* name);
#endif

const char* R_GetImageName(qhandle_t Handle);
int R_GetImageWidth(image_t* pic);
int R_GetImageHeight(image_t* pic);
void R_GetPicSize(int* w, int* h, const char* name);

void R_UploadCinematic(int Width, int Height, const byte* Data, int Client, bool Dirty);

#if 0
void R_LoadWorld(const char* Name);
bool R_GetEntityToken(char* Buffer, int Size);
int R_MarkFragments(int NumberOfPoints, const vec3_t* Points, const vec3_t Projection,
	int MaxPoints, vec3_t PointBuffer, int MaxFragments, markFragment_t* FragmentBuffer);
int R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);

qhandle_t R_RegisterModel(const char* Name);
void R_ModelBounds(qhandle_t Handle, vec3_t Mins, vec3_t Maxs);
int R_ModelNumFrames(qhandle_t Handle);
int R_ModelFlags(qhandle_t Handle);
bool R_IsMeshModel(qhandle_t Handle);
const char* R_ModelName(qhandle_t Handle);
int R_ModelSyncType(qhandle_t Handle);
void R_PrintModelFrameName(qhandle_t Handle, int Frame);
void R_CalculateModelScaleOffset(qhandle_t Handle, float ScaleX, float ScaleY, float ScaleZ, float ScaleZOrigin, vec3_t Out);
bool R_LerpTag(orientation_t* Tag, qhandle_t Handle, int StartFrame, int EndFrame, float Frac, const char* TagName);

qhandle_t R_RegisterShader(const char* Name);
qhandle_t R_RegisterShaderNoMip(const char* Name);
void R_RemapShader(const char* OldShader, const char* NewShader, const char* TimeOffset);

qhandle_t R_RegisterSkin(const char* Name);
image_t* R_RegisterSkinQ2(const char* name);

void R_RegisterFont(const char* FontName, int PointSize, fontInfo_t* Font);

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void R_ClearScene();
void R_AddRefEntityToScene(const refEntity_t* Entity);
void R_AddLightToScene(const vec3_t Origin, float Intensity, float r, float g, float b);
void R_AddAdditiveLightToScene(const vec3_t Origin, float Intensity, float r, float g, float b);
void R_AddPolyToScene(qhandle_t hShader , int NumVerts, const polyVert_t* Verts, int Num);
void R_AddLightStyleToScene(int style, float r, float g, float b);
void R_AddParticleToScene(vec3_t org, int r, int g, int b, int a, float size, QParticleTexture Texture);
void R_RenderScene(const refdef_t* fd);

float R_CalcEntityLight(refEntity_t* e);
void R_SetSky(const char* name, float rotate, vec3_t axis);

void R_SetColor(const float* rgba);
void R_StretchPic(float x, float y, float w, float h, 
	float s1, float t1, float s2, float t2, qhandle_t hShader);
void R_Draw2DQuad(float x, float y, float width, float height,
	image_t* image, float s1, float t1, float s2, float t2,
	float r, float g, float b, float a);
bool R_GetScreenPosFromWorldPos(vec3_t origin, int& u, int& v);

void R_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, bool dirty);
#endif

void R_CaptureRemoteScreenShot(const char* string1, const char* string2, const char* string3, Array<byte>& buffer);

void R_SetFog(int fogvar, int var1, int var2, float r, float g, float b, float density);

extern byte			r_palette[256][4];

extern int			ColorIndex[16];
extern unsigned		ColorPercent[16];

extern void (*BotDrawDebugPolygonsFunc)(void (*drawPoly)(int color, int numPoints, float *points), int value);
