// protocol.h -- communications protocols

/*
 * $Header: /H2 Mission Pack/PROTOCOL.H 12    3/16/98 5:33p Jweier $
 */

// This is to mask out those items that need to generate a stat bar change
#define SC1_STAT_BAR   0x01ffffff
#define SC2_STAT_BAR   0x0

// This is to mask out those items in the inventory (for inventory changes)
#define SC1_INV 0x01fffc00
#define SC2_INV 0x00000000

// defaults for clientinfo messages
#define DEFAULT_ITEMS       16385
