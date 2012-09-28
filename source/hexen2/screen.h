// screen.h

void SCR_Init(void);

void SCR_UpdateScreen(void);


void SCR_SizeUp(void);
void SCR_SizeDown(void);
void SCR_CenterPrint(char* str);

void SCRQH_BeginLoadingPlaque(void);
void SCR_DrawLoading(void);

extern int sbqh_lines;

extern int total_loading_size, current_loading_size, loading_stage;

extern const char* plaquemessage;		// Pointer to current plaque

void SCR_UpdateWholeScreen(void);
