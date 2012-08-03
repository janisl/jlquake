// comndef.h  -- general definitions

//============================================================================

void COM_Init(const char* path);
void COM_InitArgv2(int argc, char** argv);


//============================================================================

extern int com_filesize;

byte* COM_LoadStackFile(const char* path, void* buffer, int bufsize);
byte* COM_LoadHunkFile(const char* path);
void COM_Gamedir(char* dir);

extern byte cl_fraglimit;
extern float cl_timelimit;
extern float cl_server_time_offset;