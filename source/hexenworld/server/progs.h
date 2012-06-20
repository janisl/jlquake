
extern int* pr_string_index;
extern char* pr_global_strings;
extern int pr_string_count;

//============================================================================

void PR_Init(void);

void PR_LoadProgs(void);

void PR_InitBuiltins();

//============================================================================

extern func_t SpectatorConnect;
extern func_t SpectatorThink;
extern func_t SpectatorDisconnect;

extern qboolean ignore_precache;
