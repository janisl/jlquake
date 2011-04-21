// vid.h -- video driver defs

#define VID_CBITS	6
#define VID_GRADES	(1 << VID_CBITS)

// a pixel can be one, two, or four bytes
typedef byte pixel_t;

typedef struct vrect_s
{
	int				x,y,width,height;
	struct vrect_s	*pnext;
} vrect_t;

typedef struct
{
	pixel_t			*buffer;		// invisible buffer
	pixel_t			*colormap;		// 256 * VID_GRADES size
	int				fullbright;		// index of first fullbright color
	unsigned		rowbytes;	// may be > width if displayed in a window
	unsigned		width;		
	unsigned		height;
	float			aspect;		// width / height -- < 0 is taller than wide
	int				numpages;
	int				recalc_refdef;	// if true, recalc vid-based stuff
	unsigned		conwidth;
	unsigned		conheight;
} viddef_t;

extern byte globalcolormap[VID_GRADES*256], lastglobalcolor, *lastsourcecolormap;

extern	viddef_t	vid;				// global video state
extern	unsigned	d_8to24table[256];
extern	unsigned	d_8to24TranslucentTable[256];
extern void (*vid_menudrawfn)(void);
extern void (*vid_menukeyfn)(int key);

void	VID_SetPalette (unsigned char *palette);
// called at startup and after any gamma correction

void	VID_Init (unsigned char *palette);
// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void	VID_Shutdown (void);
// Called at shutdown

void	VID_Update (vrect_t *rects);
// flushes the given rectangles from the view buffer to the screen
