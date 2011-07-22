
// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

#define MAX_DISC 18

#define PLAYER_PIC_WIDTH 68
#define PLAYER_PIC_HEIGHT 114
#define PLAYER_DEST_WIDTH 128
#define PLAYER_DEST_HEIGHT 128

struct image_t;
extern	image_t		*draw_disc[MAX_DISC]; // also used on sbar

extern byte		menuplyr_pixels[MAX_PLAYER_CLASS][PLAYER_PIC_WIDTH*PLAYER_PIC_HEIGHT];

void Draw_Init (void);
void Draw_Character (int x, int y, unsigned int num);
void Draw_SmallCharacter(int x, int y, int num);
void Draw_SmallString(int x, int y, const char *str);
void Draw_SubPic(int x, int y, image_t *pic, int srcx, int srcy, int width, int height);
void Draw_PicCropped(int x, int y, image_t *pic);
void Draw_SubPicCropped(int x, int y, int h, image_t *pic);
void Draw_TransPicCropped(int x, int y, image_t *pic);
void Draw_TransPicTranslate (int x, int y, image_t *pic, byte *translation);
void Draw_ConsoleBackground (int lines);
void Draw_BeginDisc (void);
void Draw_EndDisc (void);
void Draw_TileClear (int x, int y, int w, int h);
void Draw_Fill (int x, int y, int w, int h, int c);
void Draw_FadeScreen (void);
void Draw_String (int x, int y, const char *str);

extern int	trans_level;
