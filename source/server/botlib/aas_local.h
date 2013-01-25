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

#define CACHETYPE_PORTAL        0
#define CACHETYPE_AREA          1

#define MAX_AAS_WORLDS      2	// one for each bounding box type

//area settings
struct aas_areasettings_t {
	//could also add all kind of statistic fields
	int contents;					//contents of the convex area
	int areaflags;					//several area flags
	int presencetype;				//how a bot can be present in this convex area
	int cluster;					//cluster the area belongs to, if negative it's a portal
	int clusterareanum;				//number of the area in the cluster
	int numreachableareas;			//number of reachable areas from this one
	int firstreachablearea;			//first reachable area in the reachable area index
	// Ridah, add a ground steepness stat, so we can avoid terrain when we can take a close-by flat route
	float groundsteepness;			// 0 = flat, 1 = steep
};

//structure to link entities to areas and areas to entities
struct aas_link_t {
	int entnum;
	int areanum;
	aas_link_t* next_ent;
	aas_link_t* prev_ent;
	aas_link_t* next_area;
	aas_link_t* prev_area;
};

//structure to link entities to leaves and leaves to entities
struct bsp_link_t {
	int entnum;
	int leafnum;
	bsp_link_t* next_ent;
	bsp_link_t* prev_ent;
	bsp_link_t* next_leaf;
	bsp_link_t* prev_leaf;
};

//entity
struct aas_entity_t {
	//entity info
	aas_entityinfo_t i;
	//links into the AAS areas
	aas_link_t* areas;
	//links into the BSP leaves
	bsp_link_t* leaves;
};

struct aas_settings_t {
	float phys_friction;
	float phys_stopspeed;
	float phys_gravity;
	float phys_waterfriction;
	float phys_watergravity;
	float phys_maxvelocity;
	float phys_maxwalkvelocity;
	float phys_maxcrouchvelocity;
	float phys_maxswimvelocity;
	float phys_walkaccelerate;
	float phys_airaccelerate;
	float phys_swimaccelerate;
	float phys_maxstep;
	float phys_maxsteepness;
	float phys_maxwaterjump;
	float phys_maxbarrier;
	float phys_jumpvel;
	float phys_falldelta5;
	float phys_falldelta10;
	float rs_waterjump;
	float rs_teleport;
	float rs_barrierjump;
	float rs_startcrouch;
	float rs_startgrapple;
	float rs_startwalkoffledge;
	float rs_startjump;
	float rs_rocketjump;
	float rs_bfgjump;
	float rs_jumppad;
	float rs_aircontrolledjumppad;
	float rs_funcbob;
	float rs_startelevator;
	float rs_falldamage5;
	float rs_falldamage10;
	float rs_maxfallheight;
	float rs_maxjumpfallheight;
};

//routing cache
struct aas_routingcache_t {
	byte type;									//portal or area cache
	float time;									//last time accessed or updated
	int size;									//size of the routing cache
	int cluster;								//cluster the cache is for
	int areanum;								//area the cache is created for
	vec3_t origin;								//origin within the area
	float starttraveltime;						//travel time to start with
	int travelflags;							//combinations of the travel flags
	aas_routingcache_t* prev;
	aas_routingcache_t* next;
	aas_routingcache_t* time_prev;
	aas_routingcache_t* time_next;
	unsigned char* reachabilities;				//reachabilities used for routing
	unsigned short int traveltimes[ 1 ];			//travel time for every area (variable sized)
};

//fields for the routing algorithm
struct aas_routingupdate_t {
	int cluster;
	int areanum;								//area number of the update
	vec3_t start;								//start point the area was entered
	unsigned short int tmptraveltime;			//temporary travel time
	unsigned short int* areatraveltimes;		//travel times within the area
	qboolean inlist;							//true if the update is in the list
	aas_routingupdate_t* next;
	aas_routingupdate_t* prev;
};

//reversed reachability link
struct aas_reversedlink_t {
	int linknum;								//the aas_areareachability_t
	int areanum;								//reachable from this area
	aas_reversedlink_t* next;			//next link
};

//reversed area reachability
struct aas_reversedreachability_t {
	int numlinks;
	aas_reversedlink_t* first;
};

//areas a reachability goes through
struct aas_reachabilityareas_t {
	int firstarea;
	int numareas;
};

struct aas_t {
	int loaded;									//true when an AAS file is loaded
	int initialized;							//true when AAS has been initialized
	int savefile;								//set true when file should be saved
	int bspchecksum;
	//current time
	float time;
	int numframes;
	//name of the aas file
	char filename[ MAX_QPATH ];
	char mapname[ MAX_QPATH ];
	//bounding boxes
	int numbboxes;
	aas_bbox_t* bboxes;
	//vertexes
	int numvertexes;
	aas_vertex_t* vertexes;
	//planes
	int numplanes;
	aas_plane_t* planes;
	//edges
	int numedges;
	aas_edge_t* edges;
	//edge index
	int edgeindexsize;
	aas_edgeindex_t* edgeindex;
	//faces
	int numfaces;
	aas_face_t* faces;
	//face index
	int faceindexsize;
	aas_faceindex_t* faceindex;
	//convex areas
	int numareas;
	aas_area_t* areas;
	//convex area settings
	int numareasettings;
	aas_areasettings_t* areasettings;
	//reachablity list
	int reachabilitysize;
	aas_reachability_t* reachability;
	//nodes of the bsp tree
	int numnodes;
	aas_node_t* nodes;
	//cluster portals
	int numportals;
	aas_portal_t* portals;
	//cluster portal index
	int portalindexsize;
	aas_portalindex_t* portalindex;
	//clusters
	int numclusters;
	aas_cluster_t* clusters;
	//
	int numreachabilityareas;
	float reachabilitytime;
	//enities linked in the areas
	aas_link_t* linkheap;						//heap with link structures
	int linkheapsize;							//size of the link heap
	aas_link_t* freelinks;						//first free link
	aas_link_t** arealinkedentities;			//entities linked into areas
	//entities
	int maxentities;
	int maxclients;
	aas_entity_t* entities;
	//index to retrieve travel flag for a travel type
	int travelflagfortype[ MAX_TRAVELTYPES ];
	//travel flags for each area based on contents
	int* areacontentstravelflags;
	//routing update
	aas_routingupdate_t* areaupdate;
	aas_routingupdate_t* portalupdate;
	//number of routing updates during a frame (reset every frame)
	int frameroutingupdates;
	//reversed reachability links
	aas_reversedreachability_t* reversedreachability;
	//travel times within the areas
	unsigned short*** areatraveltimes;
	//array of size numclusters with cluster cache
	aas_routingcache_t*** clusterareacache;
	aas_routingcache_t** portalcache;
	//cache list sorted on time
	aas_routingcache_t* oldestcache;		// start of cache list sorted on time
	aas_routingcache_t* newestcache;		// end of cache list sorted on time
	//maximum travel time through portal areas
	int* portalmaxtraveltimes;
	//areas the reachabilities go through
	int* reachabilityareaindex;
	aas_reachabilityareas_t* reachabilityareas;
	//hide travel times
	unsigned short int* hidetraveltimes;

	// Distance from Dangerous areas
	unsigned short int* distanceFromDanger;

	// Priority Queue Implementation
	unsigned short int* PQ_accumulator;

	// How many items are in the PQ
	int PQ_size;

	//vis data
	byte* decompressedvis;
	int decompressedvisarea;
	byte** areavisibility;
	// done.
	// Ridah, store the area's waypoint for hidepos calculations (center traced downwards)
	vec3_t* areawaypoints;
	// Ridah, so we can cache the areas that have already been tested for visibility/attackability
	byte* visCache;
	// RF, cluster team flags (-1 means not calculated)
	int* clusterTeamTravelFlags;
};

struct aas_clientmove_t {
	vec3_t endpos;			//position at the end of movement prediction
	int endarea;			//area at end of movement prediction
	vec3_t velocity;		//velocity at the end of movement prediction
	aas_trace_t aasTrace;	//last trace
	bsp_trace_t bspTrace;	//last trace
	int presencetype;		//presence type at end of movement prediction
	int stopevent;			//event that made the prediction stop
	int endcontents;		//contents at the end of movement prediction
	float time;				//time predicted ahead
	int frames;				//number of frames predicted ahead
};

extern aas_t* aasworld;
extern aas_t aasworlds[ MAX_AAS_WORLDS ];

extern aas_settings_t aassettings;

//loads the given BSP file
int AAS_LoadBSPFile();
//dump the loaded BSP data
void AAS_DumpBSPData();
//gets the mins, maxs and origin of a BSP model
void AAS_BSPModelMinsMaxs( int modelnum, const vec3_t angles, vec3_t mins, vec3_t maxs );
//trace through the world
bsp_trace_t AAS_Trace( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	int passent, int contentmask );
//calculates collision with given entity
bool AAS_EntityCollision( int entnum, const vec3_t start, const vec3_t boxmins, const vec3_t boxmaxs,
	const vec3_t end, int contentmask, bsp_trace_t* trace );
//returns true when p2 is in the PVS of p1
bool AAS_inPVS( const vec3_t p1, const vec3_t p2 );

//initialize the AAS clustering
void AAS_InitClustering();

void AAS_ClearShownPolygons();
//clear the shown debug lines
void AAS_ClearShownDebugLines();
//show a debug line
void AAS_DebugLine( const vec3_t start, const vec3_t end, int color );
//show a permenent line
void AAS_PermanentLine( const vec3_t start, const vec3_t end, int color );
void AAS_ShowAreaPolygons( int areanum, int color, int groundfacesonly );
//print the travel type
void AAS_PrintTravelType( int traveltype );
//visualize the given reachability
void AAS_ShowReachability( const aas_reachability_t* reach );

//returns the origin of the entity
void AAS_EntityOrigin( int entnum, vec3_t origin );
//returns the model index of the entity
int AAS_EntityModelindex( int entnum );
//returns the entity type
int AAS_EntityType( int entnum );
//returns the BSP model number of the entity
int AAS_EntityModelNum( int entnum );
//returns the origin of an entity with the given model number
bool AAS_OriginOfMoverWithModelNum( int modelnum, vec3_t origin );
//resets the entity AAS and BSP links (sets areas and leaves pointers to NULL)
void AAS_ResetEntityLinks();
//invalidates all entity infos
void AAS_InvalidateEntities();
//returns the next entity
int AAS_NextEntity( int entnum );
bool AAS_IsEntityInArea( int entnumIgnore, int entnumIgnore2, int areanum );
//updates an entity
int AAS_UpdateEntity( int ent, const bot_entitystate_t* state );
//unlink not updated entities
void AAS_UnlinkInvalidEntities();
//returns the best reachable area the entity is situated in
int AAS_BestReachableEntityArea( int entnum );

//dumps the loaded AAS data
void AAS_DumpAASData();
//loads the AAS file with the given name
int AAS_LoadAASFile( const char* filename );
//writes an AAS file with the given name
bool AAS_WriteAASFile( const char* filename );

//AAS error message
void AAS_Error( const char* fmt, ... ) id_attribute( ( format( printf, 1, 2 ) ) );
//returns true if the AAS file is loaded
bool AAS_Loaded();
//setup AAS with the given number of entities and clients
int AAS_Setup();
//shutdown AAS
void AAS_Shutdown();
//start a new time frame
int AAS_StartFrame( float time );
//start a new map
int AAS_LoadMap( const char* mapname );

void AAS_InitSettings();
//returns true if against a ladder at the given origin
bool AAS_AgainstLadder( const vec3_t origin, int ms_areanum );
//calculates the horizontal velocity needed for a jump and returns true this velocity could be calculated
bool AAS_HorizontalVelocityForJump( float zvel, const vec3_t start, const vec3_t end, float* velocity );
bool AAS_DropToFloor( vec3_t origin, const vec3_t mins, const vec3_t maxs );
//returns true if on the ground at the given origin
bool AAS_OnGround( const vec3_t origin, int presencetype, int passent );
//rocket jump Z velocity when rocket-jumping at origin
float AAS_RocketJumpZVelocity( const vec3_t origin );
//bfg jump Z velocity when bfg-jumping at origin
float AAS_BFGJumpZVelocity( const vec3_t origin );
bool AAS_PredictClientMovement( aas_clientmove_t* move,
	int entnum, const vec3_t origin,
	int presencetype, int hitent, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, bool visualize );
//predict movement until bounding box is hit
bool AAS_ClientMovementHitBBox( aas_clientmove_t* move,
	int entnum, const vec3_t origin,
	int presencetype, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	const vec3_t mins, const vec3_t maxs, bool visualize );
//returns the jump reachability run start point
void AAS_JumpReachRunStart( const aas_reachability_t* reach, vec3_t runstart );

void AAS_Optimize();

//returns the total area of the ground faces of the given area
float AAS_AreaGroundFaceArea( int areanum );
//returns true if the area is crouch only
int AAS_AreaCrouch( int areanum );
//returns true if a player can swim in this area
int AAS_AreaSwim( int areanum );
//returns true if the area contains lava
int AAS_AreaLava( int areanum );
//returns true if the area contains slime
int AAS_AreaSlime( int areanum );
//returns true if the area is a jump pad
int AAS_AreaJumpPad( int areanum );
//returns true if the area is donotenter
int AAS_AreaDoNotEnter( int areanum );
//returns true if the area is donotenterlarge
int AAS_AreaDoNotEnterLarge( int areanum );
int AAS_BestReachableLinkArea( aas_link_t* areas );
//returns the best reachable area and goal origin for a bounding box at the given origin
int AAS_BestReachableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs, vec3_t goalorigin );
//returns the best jumppad area from which the bbox at origin is reachable
int AAS_BestReachableFromJumpPadArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
//continue calculating the reachabilities
bool AAS_ContinueInitReachability( float time );
//initialize calculating the reachabilities
void AAS_InitReachability();

void AAS_RoutingInfo();
//returns the travel flag for the given travel type
int AAS_TravelFlagForType( int traveltype );
void AAS_EnableAllAreas();
//return the travel flag(s) for traveling through this area
int AAS_AreaContentsTravelFlags( int areanum );
int AAS_AreaVisible( int srcarea, int destarea );
void AAS_WriteRouteCache();
//returns the reachability with the given index
void AAS_ReachabilityFromNum( int num, aas_reachability_t* reach );
//returns the index of the next reachability for the given area
int AAS_NextAreaReachability( int areanum, int reachnum );
//returns the next reachability using the given model
int AAS_NextModelReachability( int num, int modelnum );
//returns the travel time from start to end in the given area
unsigned short int AAS_AreaTravelTime( int areanum, const vec3_t start, const vec3_t end );
bool AAS_AreaRouteToGoalArea( int areanum, const vec3_t origin, int goalareanum, int travelflags,
	int* traveltime, int* reachnum );
//returns the travel time from the area to the goal area using the given travel flags
int AAS_AreaTravelTimeToGoalAreaCheckLoop( int areanum, const vec3_t origin, int goalareanum, int travelflags, int loopareanum );
//free the AAS routing caches
void AAS_FreeRoutingCaches();
//initialize the AAS routing
void AAS_InitRouting();

void AAS_InitAlternativeRouting();
void AAS_ShutdownAlternativeRouting();

void AAS_InitAASLinkHeap();
void AAS_FreeAASLinkHeap();
void AAS_InitAASLinkedEntities();
void AAS_FreeAASLinkedEntities();
//returns the presence type(s) of the area
int AAS_AreaPresenceType( int areanum );
//returns the presence type(s) at the given point
int AAS_PointPresenceType( const vec3_t point );
bool AAS_PointInsideFace( int facenum, const vec3_t point, float epsilon );
void AAS_UnlinkFromAreas( aas_link_t* areas );
aas_link_t* AAS_AASLinkEntity( const vec3_t absmins, const vec3_t absmaxs, int entnum );
aas_link_t* AAS_LinkEntityClientBBox( const vec3_t absmins, const vec3_t absmaxs, int entnum, int presencetype );
aas_plane_t* AAS_PlaneFromNum( int planenum );
//returns the result of the trace of a client bbox
aas_trace_t AAS_TraceClientBBox( const vec3_t start, const vec3_t end, int presencetype, int passent );
