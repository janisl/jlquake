// client.h

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
// cl_pred.c
//
qboolean CL_CheckOrDownloadFile(char* filename);
void CL_WriteDemoCmd(hwusercmd_t* pcmd);
void CL_SendConnectPacket(void);
void Host_WriteConfiguration(const char* fname);
