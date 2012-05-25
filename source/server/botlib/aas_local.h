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
#define MAX_AAS_WORLDS_ET   1

//area settings
struct aas_areasettings_t
{
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
struct aas_link_t
{
	int entnum;
	int areanum;
	aas_link_t* next_ent;
	aas_link_t* prev_ent;
	aas_link_t* next_area;
	aas_link_t* prev_area;
};

//structure to link entities to leaves and leaves to entities
struct bsp_link_t
{
	int entnum;
	int leafnum;
	bsp_link_t* next_ent;
	bsp_link_t* prev_ent;
	bsp_link_t* next_leaf;
	bsp_link_t* prev_leaf;
};

//entity
struct aas_entity_t
{
	//entity info
	aas_entityinfo_t i;
	//links into the AAS areas
	aas_link_t* areas;
	//links into the BSP leaves
	bsp_link_t* leaves;
};

struct aas_settings_t
{
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
struct aas_routingcache_t
{
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
	unsigned short int traveltimes[1];			//travel time for every area (variable sized)
};

//fields for the routing algorithm
struct aas_routingupdate_t
{
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
struct aas_reversedlink_t
{
	int linknum;								//the aas_areareachability_t
	int areanum;								//reachable from this area
	aas_reversedlink_t* next;			//next link
};

//reversed area reachability
struct aas_reversedreachability_t
{
	int numlinks;
	aas_reversedlink_t* first;
};

//areas a reachability goes through
struct aas_reachabilityareas_t
{
	int firstarea;
	int numareas;
};

struct aas_rt_parent_link_t
{
	unsigned short int parent;				// parent we belong to
	unsigned short int childIndex;			// our index in the parent's list of children
};

struct aas_rt_child_t
{
	unsigned short int areanum;
	int numParentLinks;
	int startParentLinks;
};

struct aas_rt_parent_t
{
	unsigned short int areanum;						// out area number in the global list
	int numParentChildren;
	int startParentChildren;
	int numVisibleParents;
	int startVisibleParents;						// list of other parents that we can see (used for fast hide/retreat checks)
};

// this is what each aasworld attaches itself to
struct aas_rt_t
{
	unsigned short int* areaChildIndexes;			// each aas area that is part of the Route-Table has a pointer here to their position in the list of children

	int numChildren;
	aas_rt_child_t* children;

	int numParents;
	aas_rt_parent_t* parents;

	int numParentChildren;
	unsigned short int* parentChildren;

	int numVisibleParents;
	unsigned short int* visibleParents;

	int numParentLinks;
	aas_rt_parent_link_t* parentLinks;				// links from each child to the parent's it belongs to
};

typedef struct aas_s
{
	int loaded;									//true when an AAS file is loaded
	int initialized;							//true when AAS has been initialized
	int savefile;								//set true when file should be saved
	int bspchecksum;
	//current time
	float time;
	int numframes;
	//name of the aas file
	char filename[MAX_QPATH];
	char mapname[MAX_QPATH];
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
	int travelflagfortype[MAX_TRAVELTYPES];
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
	// Ridah, pointer to Route-Table information
	aas_rt_t* routetable;
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
	// RF, last time a death influenced this area. Seperate lists for axis/allies
	int* teamDeathTime;
	// RF, number of deaths accumulated before the time expired
	byte* teamDeathCount;
	// RF, areas that are influenced by a death count
	byte* teamDeathAvoid;
} aas_t;

extern aas_t* aasworld;
extern aas_t aasworlds[MAX_AAS_WORLDS];

//loads the given BSP file
int AAS_LoadBSPFile();
//dump the loaded BSP data
void AAS_DumpBSPData();
//gets the mins, maxs and origin of a BSP model
void AAS_BSPModelMinsMaxs(int modelnum, const vec3_t angles, vec3_t mins, vec3_t maxs);


#define AAS_MAX_PORTALS                 65536
#define AAS_MAX_PORTALINDEXSIZE         65536
#define AAS_MAX_CLUSTERS                65536

#define MAX_PORTALAREAS         1024

void AAS_RemoveClusterAreas();
bool AAS_FloodClusterAreas_r(int areanum, int clusternum);
bool AAS_FloodClusterAreasUsingReachabilities(int clusternum);
void AAS_CreatePortals();
void AAS_FindPossiblePortals();
bool AAS_TestPortals();
void AAS_CountForcedClusterPortals();
void AAS_CreateViewPortals();
void AAS_SetViewPortalsAsClusterPortals();

void AAS_ClearShownPolygons();
void AAS_ShowPolygon(int color, int numpoints, const vec3_t* points);
//clear the shown debug lines
void AAS_ClearShownDebugLines();
//show a debug line
void AAS_DebugLine(const vec3_t start, const vec3_t end, int color);
//show a permenent line
void AAS_PermanentLine(const vec3_t start, const vec3_t end, int color);
//show a permanent cross
void AAS_DrawPermanentCross(const vec3_t origin, float size, int color);
void AAS_ShowAreaPolygons(int areanum, int color, int groundfacesonly);
//draw a cros
void AAS_DrawCross(const vec3_t origin, float size, int color);
//print the travel type
void AAS_PrintTravelType(int traveltype);
//draw an arrow
void AAS_DrawArrow(const vec3_t start, const vec3_t end, int linecolor, int arrowcolor);

extern aas_t* defaultaasworld;

//returns the origin of the entity
void AAS_EntityOrigin(int entnum, vec3_t origin);
//returns the model index of the entity
int AAS_EntityModelindex(int entnum);
//returns the entity type
int AAS_EntityType(int entnum);
//returns the BSP model number of the entity
int AAS_EntityModelNum(int entnum);
//returns the origin of an entity with the given model number
bool AAS_OriginOfMoverWithModelNum(int modelnum, vec3_t origin);
//resets the entity AAS and BSP links (sets areas and leaves pointers to NULL)
void AAS_ResetEntityLinks();
//invalidates all entity infos
void AAS_InvalidateEntities();
//returns the next entity
int AAS_NextEntity(int entnum);
bool AAS_IsEntityInArea(int entnumIgnore, int entnumIgnore2, int areanum);

//dumps the loaded AAS data
void AAS_DumpAASData();
//loads the AAS file with the given name
int AAS_LoadAASFile(const char* filename);
//writes an AAS file with the given name
bool AAS_WriteAASFile(const char* filename);

//AAS error message
void AAS_Error(const char* fmt, ...) id_attribute((format(printf, 1, 2)));
//returns true if the AAS file is loaded
bool AAS_Loaded();

#define ROUTING_DEBUG

//travel time in hundreths of a second = distance * 100 / speed
#define DISTANCEFACTOR_CROUCH       1.3f	//crouch speed = 100
#define DISTANCEFACTOR_SWIM         1		//should be 0.66, swim speed = 150
#define DISTANCEFACTOR_WALK         0.33f	//walk speed = 300

// Ridah, scale traveltimes with ground steepness of area
#define GROUNDSTEEPNESS_TIMESCALE       20	// this is the maximum scale, 1 being the usual for a flat ground

//cache refresh time
#define CACHE_REFRESHTIME       15.0f	//15 seconds refresh time

//maximum number of routing updates each frame
#define MAX_FRAMEROUTINGUPDATES_Q3      10
#define MAX_FRAMEROUTINGUPDATES_WOLF    100

#define DEFAULT_MAX_ROUTINGCACHESIZE        "16384"

#ifdef ROUTING_DEBUG
extern int numareacacheupdates;
extern int numportalcacheupdates;
#endif

extern int routingcachesize;
extern int max_routingcachesize;
extern int max_frameroutingupdates;

void AAS_RoutingInfo();
int AAS_ClusterAreaNum(int cluster, int areanum);
void AAS_InitTravelFlagFromType();
//returns the travel flag for the given travel type
int AAS_TravelFlagForType(int traveltype);
float AAS_RoutingTime();
void AAS_LinkCache(aas_routingcache_t* cache);
void AAS_UnlinkCache(aas_routingcache_t* cache);
aas_routingcache_t* AAS_AllocRoutingCache(int numtraveltimes);
void AAS_FreeRoutingCache(aas_routingcache_t* cache);
void AAS_RemoveRoutingCacheUsingArea(int areanum);
void AAS_ClearClusterTeamFlags(int areanum);
void AAS_EnableAllAreas();
void AAS_InitAreaContentsTravelFlags();
//return the travel flag(s) for traveling through this area
int AAS_AreaContentsTravelFlags(int areanum);
void AAS_CreateReversedReachability();
float AAS_AreaGroundSteepnessScale(int areanum);
