/*
 * $Header: /H3/game/DRAW.H 5     8/20/97 2:05p Rjohnson $
 */

// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

#define MAX_DISC 18

struct image_t;
extern image_t* draw_backtile;

void Draw_Init(void);
void Draw_Character(int x, int y, unsigned int num);
void Draw_ConsoleBackground(int lines);
void Draw_FadeScreen(void);
void Draw_String(int x, int y, const char* str);
void Draw_SmallCharacter(int x, int y, int num);
void Draw_SmallString(int x, int y, const char* str);
