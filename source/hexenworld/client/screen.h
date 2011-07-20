// screen.h

void SCR_Init (void);

void SCR_UpdateScreen (void);


void SCR_SizeUp (void);
void SCR_SizeDown (void);
void SCR_BringDownConsole (void);
void SCR_CenterPrint (char *str);

int SCR_ModalMessage (const char *text);

extern	float		scr_con_current;
extern	float		scr_conlines;		// lines of console to display

extern	int			sb_lines;

extern	qboolean	scr_disabled_for_loading;

extern	QCvar*		scr_viewsize;

void SCR_UpdateWholeScreen (void);

