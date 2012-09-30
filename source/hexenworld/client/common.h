// comndef.h  -- general definitions

//============================================================================

void COM_Init(const char* path);
void COM_InitArgv2(int argc, char** argv);


//============================================================================

void COM_Gamedir(char* dir);
void Com_Quit_f();

void Com_InitDebugLog();
void Con_Printf(const char* fmt, ...);
void Con_DPrintf(const char* fmt, ...);
