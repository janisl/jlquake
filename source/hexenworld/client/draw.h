
// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

#define MAX_DISC 18

struct image_t;
extern	image_t		*draw_disc[MAX_DISC]; // also used on sbar

void Draw_Init (void);
void Draw_Character (int x, int y, unsigned int num);
void Draw_SmallCharacter(int x, int y, int num);
void Draw_SmallString(int x, int y, char *str);
void Draw_DebugChar (char num);
void Draw_SubPic(int x, int y, image_t *pic, int srcx, int srcy, int width, int height);
void Draw_Pic (int x, int y, image_t *pic);
void Draw_PicCropped(int x, int y, image_t *pic);
void Draw_SubPicCropped(int x, int y, int h, image_t *pic);
void Draw_TransPic (int x, int y, image_t *pic);
void Draw_TransPicCropped(int x, int y, image_t *pic);
void Draw_TransPicTranslate (int x, int y, image_t *pic, byte *translation);
void Draw_ConsoleBackground (int lines);
void Draw_BeginDisc (void);
void Draw_EndDisc (void);
void Draw_TileClear (int x, int y, int w, int h);
void Draw_Fill (int x, int y, int w, int h, int c);
void Draw_FadeScreen (void);
void Draw_String (int x, int y, char *str);
image_t *Draw_PicFromWad (char *name);
image_t *Draw_CachePic (char *path);
int Draw_GetWidth(image_t* pic);
int Draw_GetHeight(image_t* pic);

extern int	trans_level;
