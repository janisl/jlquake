/*
 * $Header: /H3/game/DRAW.H 5     8/20/97 2:05p Rjohnson $
 */

// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

#define MAX_DISC 18

#define PLAYER_PIC_WIDTH 68
#define PLAYER_PIC_HEIGHT 114

struct image_t;
extern image_t		*draw_disc[MAX_DISC]; // also used on sbar

extern byte		menuplyr_pixels[NUM_CLASSES][PLAYER_PIC_WIDTH*PLAYER_PIC_HEIGHT];

void Draw_Init (void);
void Draw_Character (int x, int y, unsigned int num);
void Draw_PicCropped(int x, int y, image_t *pic);
void Draw_TransPicCropped(int x, int y, image_t *pic);
void Draw_TransPicTranslate (int x, int y, image_t *pic, byte *translation);
void Draw_ConsoleBackground (int lines);
void Draw_BeginDisc (void);
void Draw_EndDisc (void);
void Draw_TileClear (int x, int y, int w, int h);
void Draw_Fill (int x, int y, int w, int h, int c);
void Draw_FadeScreen (void);
void Draw_String (int x, int y, const char *str);
void Draw_SmallCharacter(int x, int y, int num);
void Draw_SmallString(int x, int y, const char *str);
