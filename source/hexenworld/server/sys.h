// sys.h -- non-portable functions

void Sys_Error (char *error, ...);
// an error will cause the entire program to exit

void Sys_Printf (char *fmt, ...);
// send text to the console

void Sys_Quit (void);
double Sys_DoubleTime (void);
void Sys_Init (void);
