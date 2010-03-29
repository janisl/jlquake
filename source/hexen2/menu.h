/*
 * $Header: /H3/game/MENU.H 5     9/04/97 4:44p Rjohnson $
 */

//
// the net drivers should just set the apropriate bits in m_activenet,
// instead of having the menu code look through their internal tables
//
#define	MNET_IPX		1
#define	MNET_TCP		2

extern	int	m_activenet;

extern char	*plaquemessage;     // Pointer to current plaque
extern char	*errormessage;     // Pointer to current plaque

extern char BigCharWidth[27][27];

//
// menus
//
void M_Init (void);
void M_Keydown (int key);
void M_Draw (void);
void M_ToggleMenu_f (void);

void M_DrawTextBox (int x, int y, int width, int lines);

/*
 * $Log: /H3/game/MENU.H $
 * 
 * 5     9/04/97 4:44p Rjohnson
 * Updates
 * 
 * 4     4/17/97 3:42p Rjohnson
 * Modifications for the gl version for menus
 * 
 * 3     2/19/97 11:29a Rjohnson
 * Id Updates
 */

