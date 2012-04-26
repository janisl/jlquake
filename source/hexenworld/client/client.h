// client.h

// edict->flags
#define FL_FLY                  1
#define FL_SWIM                 2
//#define	FL_GLIMPSE				4
#define FL_CONVEYOR             4
#define FL_CLIENT               8
#define FL_INWATER              16
#define FL_MONSTER              32
#define FL_GODMODE              64
#define FL_NOTARGET             128
#define FL_ITEM                 256
#define FL_ONGROUND             512
#define FL_PARTIALGROUND        1024	// not all corners are valid
#define FL_WATERJUMP            2048	// player jumping out of water
#define FL_JUMPRELEASED         4096	// for jump debouncing
#define FL_FLASHLIGHT           8192
#define FL_ARCHIVE_OVERRIDE     1048576
#define FL_ARTIFACTUSED         16384
#define FL_MOVECHAIN_ANGLE      32768	// when in a move chain, will update the angle
#define FL_CLASS_DEPENDENT      2097152	// model will appear different to each player
#define FL_SPECIAL_ABILITY1     4194304	// has 1st special ability
#define FL_SPECIAL_ABILITY2     8388608	// has 2nd special ability

#define FL2_CROUCHED            4096

// edict->movetype values
#define QHMOVETYPE_NONE         0		// never moves
#define QHMOVETYPE_ANGLENOCLIP  1
#define QHMOVETYPE_ANGLECLIP        2
#define QHMOVETYPE_WALK         3		// gravity
#define QHMOVETYPE_STEP         4		// gravity, special edge handling
#define QHMOVETYPE_FLY          5
#define QHMOVETYPE_TOSS         6		// gravity
#define QHMOVETYPE_PUSH         7		// no clip to world, push and crush
#define QHMOVETYPE_NOCLIP           8
#define QHMOVETYPE_FLYMISSILE       9		// extra size to monsters
#define QHMOVETYPE_BOUNCE           10
#define H2MOVETYPE_BOUNCEMISSILE    11		// bounce w/o gravity
#define H2MOVETYPE_FOLLOW           12		// track movement of aiment
#define H2MOVETYPE_PUSHPULL     13		// pushable/pullable object
#define H2MOVETYPE_SWIM         14		// should keep the object in water

//
// cvars
//
extern Cvar* cl_warncmd;

extern Cvar* cl_shownet;
extern Cvar* cl_sbar;
extern Cvar* cl_hudswap;

extern Cvar* playerclass;
extern Cvar* spectator;

extern Cvar* cl_lightlevel;		// FIXME HACK

extern Cvar* cl_teamcolor;

//=============================================================================

extern qboolean nomaster;

//
// cl_main
//
void CL_Init(void);

void CL_Disconnect(void);
void CL_Disconnect_f(void);
void CL_NextDemo(void);
qboolean CL_DemoBehind(void);

//
// cl_input
//
void CL_InitInput(void);
void CL_SendCmd(void);
void CL_SendMove(hwusercmd_t* cmd);

void CL_ClearState(void);

void CL_ReadPackets(void);

int  CL_ReadFromServer(void);
void CL_WriteToServer(hwusercmd_t* cmd);
void CL_MouseEvent(int mx, int my);


const char* Key_KeynumToString(int keynum);

//
// cl_demo.c
//
void CL_StopPlayback(void);
qboolean CL_GetMessage(void);

void CL_Stop_f(void);
void CL_Record_f(void);
void CL_ReRecord_f(void);
void CL_PlayDemo_f(void);
void CL_TimeDemo_f(void);

//
// cl_parse.c
//
void CL_ParseServerMessage(void);
void CL_NewTranslation(int slot);

//
// view
//
void V_RenderView(void);
void V_UpdatePalette(void);
void V_Register(void);
void V_ParseDamage(void);

//
// cl_ents.c
//
void CL_SetSolidPlayers(int playernum);

//
// cl_pred.c
//
void CL_InitPrediction(void);
void CL_PredictMove(void);

qboolean CL_CheckOrDownloadFile(char* filename);
void CL_ParsePlayerinfo(void);
void CL_SavePlayer(void);
void CL_ParsePacketEntities(qboolean delta);
void CL_SetSolidEntities(void);
void CL_SetUpPlayerPrediction(qboolean dopred);
void CL_EmitEntities(void);
void CL_WriteDemoCmd(hwusercmd_t* pcmd);
void CL_PredictUsercmd(hwplayer_state_t* from, hwplayer_state_t* to, hwusercmd_t* u, qboolean spectator);
void CL_SendConnectPacket(void);
void Host_WriteConfiguration(const char* fname);
void Cam_FinishMove(hwusercmd_t* cmd);
