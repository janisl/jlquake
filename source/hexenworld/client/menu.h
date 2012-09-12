
#include "../../client/game/quake_hexen2/menu.h"

#define MNET_TCP        2

extern int m_activenet;

extern const char* plaquemessage;		// Pointer to current plaque
extern char* errormessage;		// Pointer to current plaque

//
// menus
//
void M_Init(void);
void M_Keydown(int key);
void M_Draw(void);
void M_ToggleMenu_f(void);


void M_Print2(int cx, int cy, const char* str);
