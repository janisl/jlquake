// client.h

/*
 * $Header: /H2 Mission Pack/CLIENT.H 4     3/12/98 6:31p Mgummelt $
 */

#define NAME_LENGTH 64

//
// cvars
//
extern Cvar* cl_playerclass;

extern Cvar* cl_prettylights;

extern Cvar* cl_shownet;

extern Cvar* cl_lightlevel;		// FIXME HACK

//=============================================================================

//
// cl_main
//
void CL_Init(void);

void CL_EstablishConnection(const char* host);
void CL_Signon1(void);
void CL_Signon2(void);
void CL_Signon3(void);
void CL_Signon4(void);
void CL_SignonReply(void);

void CL_Disconnect(void);
void CL_Disconnect_f(void);
void CL_NextDemo(void);

//
// cl_input
//
void CL_InitInput(void);
void CL_SendCmd(void);

void CL_ClearState(void);

int  CL_ReadFromServer(void);
void CL_WriteToServer(h2usercmd_t* cmd);
void CL_MouseEvent(int mx, int my);


const char* Key_KeynumToString(int keynum);

//
// cl_demo.c
//
void CL_StopPlayback(void);
int CL_GetMessage(void);

void CL_Stop_f(void);
void CL_Record_f(void);
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
// cl_tent
//
void SV_UpdateEffects(QMsg* sb);

void CL_RemoveGIPFiles(const char* path);
void CL_CopyFiles(const char* source, const char* ext, const char* dest);
