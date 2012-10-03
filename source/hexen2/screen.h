// screen.h

void SCR_Init(void);

void SCR_UpdateScreen(void);


void SCR_SizeUp(void);
void SCR_SizeDown(void);

void SCRQH_BeginLoadingPlaque(void);

extern int sbqh_lines;

extern qboolean con_forcedup;	// because no entities to refresh

void SCR_UpdateWholeScreen(void);
