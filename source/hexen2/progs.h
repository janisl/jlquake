

//============================================================================

extern int* pr_string_index;
extern char* pr_global_strings;
extern int pr_string_count;

//============================================================================

void PR_Init(void);

void PR_LoadProgs(void);
void PR_LoadStrings(void);
void PR_LoadInfoStrings(void);

qhedict_t* ED_Alloc(void);
qhedict_t* ED_Alloc_Temp(void);
void ED_Free(qhedict_t* ed);

void ED_LoadFromFile(const char* data);
void PR_InitBuiltins();

//============================================================================

extern unsigned short pr_crc;

extern Cvar* max_temp_edicts;

extern qboolean ignore_precache;
