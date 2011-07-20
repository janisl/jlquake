// vid.h -- video driver defs

extern	viddef_t	vid;				// global video state
extern	unsigned	d_8to24TranslucentTable[256];

void	VID_Init ();
// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void	VID_Shutdown (void);
// Called at shutdown

void	VID_Update (vrect_t *rects);
// flushes the given rectangles from the view buffer to the screen
