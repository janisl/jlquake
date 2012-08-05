// protocol.h -- communications protocols

//=========================================

#define PORT_CLIENT 26901	//27001
#define PORT_MASTER 26900	//27000
#define PORT_SERVER 26950	//27500

//=========================================
// This is to mask out those items that need to generate a stat bar change
#define SC1_STAT_BAR   0x01ffffff
#define SC2_STAT_BAR   0x0

// This is to mask out those items in the inventory (for inventory changes)
#define SC1_INV 0x01fffc00
#define SC2_INV 0x00000000
