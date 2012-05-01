//
// console
//
extern qboolean con_forcedup;	// because no entities to refresh
extern byte* con_chars;

void Con_DrawCharacter(int cx, int line, int num);

void Con_Init(void);
void Con_DrawConsole(int lines, qboolean drawinput);
void Con_Print(const char* txt);
void Con_Printf(const char* fmt, ...);
void Con_DPrintf(const char* fmt, ...);
void Con_SafePrintf(const char* fmt, ...);
void Con_Clear_f(void);
void Con_DrawNotify(void);
void Con_ToggleConsole_f(void);

void Con_NotifyBox(const char* text);	// during startup for sound / cd warnings
