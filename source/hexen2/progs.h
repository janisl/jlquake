

//============================================================================

extern int* pr_string_index;
extern char* pr_global_strings;
extern int pr_string_count;

//============================================================================

void PR_LoadStrings(void);
void PR_LoadInfoStrings(void);

void PR_InitBuiltins();

extern qboolean ignore_precache;
