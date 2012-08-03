// protocol.h -- communications protocols

//=========================================

#define PORT_CLIENT 26901	//27001
#define PORT_MASTER 26900	//27000
#define PORT_SERVER 26950	//27500

//=========================================

// out of band message id bytes

// M = master, S = server, C = client, A = any
// the second character will allways be \n if the message isn't a single
// byte long (?? not true anymore?)

#define A2S_ECHO            'g'	// echo back a message

#define S2C_CONNECTION      'j'
#define A2A_PING            'k'	// respond with an A2A_ACK
#define A2A_ACK             'l'	// general acknowledgement without info
#define A2A_NACK            'm'	// [+ comment] general failure
#define A2C_PRINT           'n'	// print a message on client

#define S2M_HEARTBEAT       'a'	// + serverinfo + userlist + fraglist
#define A2C_CLIENT_COMMAND  'B'	// + command line
#define S2M_SHUTDOWN        'C'

// This is to mask out those items that need to generate a stat bar change
#define SC1_STAT_BAR   0x01ffffff
#define SC2_STAT_BAR   0x0

// This is to mask out those items in the inventory (for inventory changes)
#define SC1_INV 0x01fffc00
#define SC2_INV 0x00000000
