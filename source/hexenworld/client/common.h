// comndef.h  -- general definitions

#define MAX_LOCALINFO_STRING    32768

//============================================================================

void COM_Init(const char* path);
void COM_InitArgv2(int argc, char** argv);


//============================================================================

extern int com_filesize;

byte* COM_LoadStackFile(const char* path, void* buffer, int bufsize);
byte* COM_LoadHunkFile(const char* path);
void COM_Gamedir(char* dir);

extern qboolean standard_quake, rogue, hipnotic;

extern byte cl_fraglimit;
extern float cl_timelimit;
extern float cl_server_time_offset;