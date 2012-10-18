// client.h

/*
 * $Header: /H2 Mission Pack/CLIENT.H 4     3/12/98 6:31p Mgummelt $
 */

#define NAME_LENGTH 64

//
// cl_main
//
void CL_Init(void);

void CL_Disconnect_f(void);
void CL_NextDemo(void);

int  CL_ReadFromServer(void);
