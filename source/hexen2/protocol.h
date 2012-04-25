// protocol.h -- communications protocols

/*
 * $Header: /H2 Mission Pack/PROTOCOL.H 12    3/16/98 5:33p Jweier $
 */

#define PROTOCOL_VERSION    19

#define BE_ON           (1 << 0)

#define SU_VIEWHEIGHT   (1 << 0)
#define SU_IDEALPITCH   (1 << 1)
#define SU_PUNCH1       (1 << 2)
#define SU_PUNCH2       (1 << 3)
#define SU_PUNCH3       (1 << 4)
#define SU_VELOCITY1    (1 << 5)
#define SU_VELOCITY2    (1 << 6)
#define SU_VELOCITY3    (1 << 7)
//define	SU_AIMENT		(1<<8)  AVAILABLE BIT
#define SU_IDEALROLL    (1 << 8)	// I'll take that available bit
#define SU_SC1          (1 << 9)
#define SU_ONGROUND     (1 << 10)		// no data follows, the bit is it
#define SU_INWATER      (1 << 11)		// no data follows, the bit is it
#define SU_WEAPONFRAME  (1 << 12)
#define SU_ARMOR        (1 << 13)
#define SU_WEAPON       (1 << 14)
#define SU_SC2          (1 << 15)

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

// a sound with no channel is a local only sound
#define SND_VOLUME      (1 << 0)		// a byte
#define SND_ATTENUATION (1 << 1)		// a byte
#define SND_OVERFLOW    (1 << 2)		// add 255 to snd num
//gonna use the rest of the bits to pack the ent+channel

// Bits to help send server info about the client's edict variables
#define SC1_HEALTH              (1 << 0)		// changes stat bar
#define SC1_LEVEL               (1 << 1)		// changes stat bar
#define SC1_INTELLIGENCE        (1 << 2)		// changes stat bar
#define SC1_WISDOM              (1 << 3)		// changes stat bar
#define SC1_STRENGTH            (1 << 4)		// changes stat bar
#define SC1_DEXTERITY           (1 << 5)		// changes stat bar
#define SC1_WEAPON              (1 << 6)		// changes stat bar
#define SC1_BLUEMANA            (1 << 7)		// changes stat bar
#define SC1_GREENMANA           (1 << 8)		// changes stat bar
#define SC1_EXPERIENCE          (1 << 9)		// changes stat bar
#define SC1_CNT_TORCH           (1 << 10)		// changes stat bar
#define SC1_CNT_H_BOOST         (1 << 11)		// changes stat bar
#define SC1_CNT_SH_BOOST        (1 << 12)		// changes stat bar
#define SC1_CNT_MANA_BOOST      (1 << 13)		// changes stat bar
#define SC1_CNT_TELEPORT        (1 << 14)		// changes stat bar
#define SC1_CNT_TOME            (1 << 15)		// changes stat bar
#define SC1_CNT_SUMMON          (1 << 16)		// changes stat bar
#define SC1_CNT_INVISIBILITY    (1 << 17)		// changes stat bar
#define SC1_CNT_GLYPH           (1 << 18)		// changes stat bar
#define SC1_CNT_HASTE           (1 << 19)		// changes stat bar
#define SC1_CNT_BLAST           (1 << 20)		// changes stat bar
#define SC1_CNT_POLYMORPH       (1 << 21)		// changes stat bar
#define SC1_CNT_FLIGHT          (1 << 22)		// changes stat bar
#define SC1_CNT_CUBEOFFORCE     (1 << 23)		// changes stat bar
#define SC1_CNT_INVINCIBILITY   (1 << 24)		// changes stat bar
#define SC1_ARTIFACT_ACTIVE     (1 << 25)
#define SC1_ARTIFACT_LOW        (1 << 26)
#define SC1_MOVETYPE            (1 << 27)
#define SC1_CAMERAMODE          (1 << 28)
#define SC1_HASTED              (1 << 29)
#define SC1_INVENTORY           (1 << 30)
#define SC1_RINGS_ACTIVE        (1 << 31)

#define SC2_RINGS_LOW           (1 << 0)
#define SC2_AMULET              (1 << 1)
#define SC2_BRACER              (1 << 2)
#define SC2_BREASTPLATE         (1 << 3)
#define SC2_HELMET              (1 << 4)
#define SC2_FLIGHT_T            (1 << 5)
#define SC2_WATER_T             (1 << 6)
#define SC2_TURNING_T           (1 << 7)
#define SC2_REGEN_T             (1 << 8)
#define SC2_HASTE_T             (1 << 9)
#define SC2_TOME_T              (1 << 10)
#define SC2_PUZZLE1             (1 << 11)
#define SC2_PUZZLE2             (1 << 12)
#define SC2_PUZZLE3             (1 << 13)
#define SC2_PUZZLE4             (1 << 14)
#define SC2_PUZZLE5             (1 << 15)
#define SC2_PUZZLE6             (1 << 16)
#define SC2_PUZZLE7             (1 << 17)
#define SC2_PUZZLE8             (1 << 18)
#define SC2_MAXHEALTH           (1 << 19)
#define SC2_MAXMANA             (1 << 20)
#define SC2_FLAGS               (1 << 21)
#define SC2_OBJ                 (1 << 22)
#define SC2_OBJ2                (1 << 23)


// This is to mask out those items that need to generate a stat bar change
#define SC1_STAT_BAR   0x01ffffff
#define SC2_STAT_BAR   0x0

// This is to mask out those items in the inventory (for inventory changes)
#define SC1_INV 0x01fffc00
#define SC2_INV 0x00000000

// defaults for clientinfo messages
#define DEFAULT_VIEWHEIGHT  22
#define DEFAULT_ITEMS       16385


// game types sent by serverinfo
// these determine which intermission screen plays
#define GAME_COOP           0
#define GAME_DEATHMATCH     1
