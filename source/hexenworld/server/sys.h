// sys.h -- non-portable functions

void Sys_Error (char *error, ...);
// an error will cause the entire program to exit

void Sys_Quit (void);
double Sys_DoubleTime (void);
void Sys_Init (void);
