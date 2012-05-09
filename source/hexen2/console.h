//
// console
//
extern qboolean con_forcedup;	// because no entities to refresh

void Con_Init(void);
void Con_Printf(const char* fmt, ...);
void Con_DPrintf(const char* fmt, ...);
void Con_ToggleConsole_f(void);
