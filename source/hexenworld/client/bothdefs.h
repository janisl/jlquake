
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

//
// item flags
//
#define IT_SHOTGUN              1
#define IT_SUPER_SHOTGUN        2
#define IT_NAILGUN              4
#define IT_SUPER_NAILGUN        8

#define IT_GRENADE_LAUNCHER     16
#define IT_ROCKET_LAUNCHER      32
#define IT_LIGHTNING            64
#define IT_SUPER_LIGHTNING      128

#define IT_SHELLS               256
#define IT_NAILS                512
#define IT_ROCKETS              1024
#define IT_CELLS                2048

#define IT_AXE                  4096

#define IT_ARMOR1               8192
#define IT_ARMOR2               16384
#define IT_ARMOR3               32768

#define IT_SUPERHEALTH          65536

#define IT_KEY1                 131072
#define IT_KEY2                 262144

#define IT_INVISIBILITY         524288

#define IT_INVULNERABILITY      1048576
#define IT_SUIT                 2097152
#define IT_QUAD                 4194304

#define IT_SIGIL1               (1 << 28)

#define IT_SIGIL2               (1 << 29)
#define IT_SIGIL3               (1 << 30)
#define IT_SIGIL4               (1 << 31)
