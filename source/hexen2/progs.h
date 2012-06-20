

//============================================================================

extern int* pr_string_index;
extern char* pr_global_strings;
extern int pr_string_count;

//============================================================================

void PR_Init(void);

void PR_LoadProgs(void);
void PR_LoadStrings(void);
void PR_LoadInfoStrings(void);

void PR_InitBuiltins();

//============================================================================

extern unsigned short pr_crc;

extern qboolean ignore_precache;
