
extern char *keybindings[256];
extern	int		key_repeats[256];
extern	int		key_count;			// incremented every key event
extern	int		key_lastpress;

extern char chat_buffer[];
extern	int chat_bufferlen;
extern	qboolean	chat_team;

void Key_Event (int key, qboolean down);
void Key_Init (void);
void Key_WriteBindings (fileHandle_t f);
void Key_SetBinding (int keynum, char *binding);
void Key_ClearStates (void);


void IN_ProcessEvents();
