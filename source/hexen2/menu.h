/*
 * $Header: /H3/game/MENU.H 5     9/04/97 4:44p Rjohnson $
 */

#include "../client/game/quake_hexen2/menu.h"

#define MNET_TCP        2

extern int m_activenet;

extern const char* plaquemessage;		// Pointer to current plaque
extern char* errormessage;		// Pointer to current plaque

extern char BigCharWidth[27][27];

//
// menus
//
void M_Init(void);
void M_Keydown(int key);
void M_CharEvent(int key);
void M_Draw(void);
void M_ToggleMenu_f(void);

void M_DrawTextBox(int x, int y, int width, int lines);
void M_Print(int cx, int cy, const char* str);
void M_Print2(int cx, int cy, const char* str);
void M_DrawTextBox2(int x, int y, int width, int lines, qboolean bottom);
void M_PrintWhite(int cx, int cy, const char* str);
void M_DrawTransPic(int x, int y, image_t* pic);

extern image_t* translate_texture[NUM_CLASSES];
