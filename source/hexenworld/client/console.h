//
// console
//
extern byte* con_chars;

void Con_DrawCharacter(int cx, int line, int num);

void Con_Init(void);
void Con_Print(const char* txt);
void Con_Printf(const char* fmt, ...);
void Con_DPrintf(const char* fmt, ...);
void Con_ToggleConsole_f(void);

void Con_SafePrintf(const char* fmt, ...);
