
//
// console
//

#define		CON_TEXTSIZE	16384
typedef struct
{
	char	text[CON_TEXTSIZE];
	byte	text_attr[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line
} console_t;

extern	console_t	con_main;
extern	console_t	con_chat;
extern	console_t	*con;			// point to either con_main or con_chat

extern	int			con_ormask;

extern int con_totallines;
extern qboolean con_initialized;
extern byte *con_chars;
extern	int	con_notifylines;		// scan lines to clear for notify lines

void Con_DrawCharacter (int cx, int line, int num);

void Con_CheckResize (void);
void Con_Init (void);
void Con_DrawConsole (int lines);
void Con_Print (char *txt);
void Con_Printf (char *fmt, ...);
void Con_DPrintf (char *fmt, ...);
void Con_Clear_f (void);
void Con_DrawNotify (void);
void Con_ClearNotify (void);
void Con_ToggleConsole_f (void);

void Con_NotifyBox (char *text);	// during startup for sound / cd warnings

