// screen.h

void SCR_Init(void);

void SCR_UpdateScreen(void);


void SCR_SizeUp(void);
void SCR_SizeDown(void);

extern int sbqh_lines;

extern const char* plaquemessage;		// Pointer to current plaque

void SCR_UpdateWholeScreen(void);
