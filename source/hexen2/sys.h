// sys.h -- non-portable functions

//
// system IO
//
void Sys_Error (char *error, ...);
// an error will cause the entire program to exit

void Sys_Quit (void);

double Sys_FloatTime (void);

void Sys_SendKeyEvents (void);
// Perform Key_Event () callbacks until the input que is empty
