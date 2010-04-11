void	Cmd_Init (void);

void	Cmd_ForwardToServer (void);
// adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

void	Cmd_Print (char *text);
// used by command functions to send output to either the graphics console or
// passed as a print message to the client

void WriteCommands (FILE *FH);
