
// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

#define MAX_DISC 18

void Draw_Init(void);
void Draw_SmallCharacter(int x, int y, int num);
void Draw_SmallString(int x, int y, const char* str);
void Draw_FadeScreen(void);
void Draw_Crosshair(void);

void R_NetGraph(void);
