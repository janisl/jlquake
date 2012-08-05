
// defs common to client and server

#define GLQUAKE_VERSION 0.95
#define VERSION     0.15
#define LINUX_VERSION 0.94


#ifdef SERVERONLY		// no asm in dedicated server
#undef id386
#endif

#if id386
#define UNALIGNED_OK    1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK    0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE  32		// used to align key data structures

#define UNUSED(x)   (x = x)	// for pesky compiler / lint warnings

#define MINIMUM_MEMORY  0x550000


#define SOUND_CHANNELS      8


#define SAVEGAME_COMMENT_LENGTH 39
