// comndef.h  -- general definitions

void COM_Init(const char* path);
void COM_InitArgv2(int argc, char** argv);

void Com_InitDebugLog();

void Con_Printf(const char* fmt, ...);
void Con_DPrintf(const char* fmt, ...);
