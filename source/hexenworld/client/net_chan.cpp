
#include "quakedef.h"
#ifdef _WIN32
#include <windows.h>
#endif

#define	PACKET_HEADER	8

/*

packet header
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message

The remote connection never knows if it missed a reliable message, the
local side detects that it has been dropped by seeing a sequence acknowledge
higher thatn the last reliable sequence, but without the correct evon/odd
bit for the reliable set.

If the sender notices that a reliable message has been dropped, it will be
retransmitted.  It will not be retransmitted again until a message after
the retransmit has been acknowledged and the reliable still failed to get there.

if the sequence number is -1, the packet should be handled without a netcon

The reliable message can be added to at any time by doing
MSG_Write* (&netchan->message, <data>).

If the message buffer is overflowed, either by a single message, or by
multiple frames worth piling up while the last reliable transmit goes
unacknowledged, the netchan signals a fatal error.

Reliable messages are allways placed first in a packet, then the unreliable
message is included if there is sufficient room.

To the receiver, there is no distinction between the reliable and unreliable
parts of the message, they are just processed out as a single larger message.

Illogical packet sequence numbers cause the packet to be dropped, but do
not kill the connection.  This, combined with the tight window of valid
reliable acknowledgement numbers provides protection against malicious
address spoofing.

*/

int		net_drop;
QCvar*	showpackets;
QCvar*	showdrop;

/*
===============
Netchan_Init

===============
*/
void Netchan_Init (void)
{
	showpackets = Cvar_Get("showpackets", "0", 0);
	showdrop = Cvar_Get("showdrop", "0", 0);
}

/*
===============
Netchan_OutOfBand

Sends an out-of-band datagram
================
*/
void Netchan_OutOfBand (netadr_t adr, int length, byte *data)
{
	QMsg		send;
	byte		send_buf[MAX_MSGLEN + PACKET_HEADER];

// write the packet header
	send.InitOOB(send_buf, sizeof(send_buf));

	send.WriteLong(-1);	// -1 sequence means out of band
	send.WriteData(data, length);

// send the datagram
	//zoid, no input in demo playback mode
#ifndef SERVERONLY
	if (!cls.demoplayback)
#endif
		NET_SendPacket (send.cursize, send._data, adr);
}

/*
===============
Netchan_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void Netchan_OutOfBandPrint (netadr_t adr, char *format, ...)
{
	va_list		argptr;
	static char		string[8192];		// ??? why static?
	
	va_start (argptr, format);
	Q_vsnprintf(string, 8192, format, argptr);
	va_end (argptr);


	Netchan_OutOfBand (adr, QStr::Length(string), (byte*)string);
}


/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup (netchan_t *chan, netadr_t adr)
{
	Com_Memset(chan, 0, sizeof(*chan));
	
	chan->remote_address = adr;
	chan->last_received = realtime;
	
	chan->message.InitOOB(chan->message_buf, sizeof(chan->message_buf));
	chan->message.allowoverflow = true;
	
	chan->rate = 1.0/2500;
}


/*
===============
Netchan_CanPacket

Returns true if the bandwidth choke isn't active
================
*/
#define	MAX_BACKUP	200
qboolean Netchan_CanPacket (netchan_t *chan)
{
	if (chan->cleartime < realtime + MAX_BACKUP*chan->rate)
		return true;
	return false;
}


/*
===============
Netchan_CanReliable

Returns true if the bandwidth choke isn't 
================
*/
qboolean Netchan_CanReliable (netchan_t *chan)
{
	if (chan->reliable_length)
		return false;			// waiting for ack
	return Netchan_CanPacket (chan);
}


/*
===============
Netchan_Transmit

tries to send an unreliable message to a connection, and handles the
transmition / retransmition of the reliable messages.

A 0 length will still generate a packet and deal with the reliable messages.
================
*/
void Netchan_Transmit (netchan_t *chan, int length, byte *data)
{
	QMsg		send;
	byte		send_buf[MAX_MSGLEN + PACKET_HEADER];
	qboolean	send_reliable;
	unsigned	w1, w2;
	int			i;

// check for message overflow
	if (chan->message.overflowed)
	{
		chan->fatal_error = true;
		Con_Printf ("%s:Outgoing message overflow\n"
			, SOCK_AdrToString (chan->remote_address));
		return;
	}

// if the remote side dropped the last reliable message, resend it
	send_reliable = false;

	if (chan->incoming_acknowledged > chan->last_reliable_sequence
	&& chan->incoming_reliable_acknowledged != chan->reliable_sequence)
		send_reliable = true;

// if the reliable transmit buffer is empty, copy the current message out
	if (!chan->reliable_length && chan->message.cursize)
	{
		Com_Memcpy(chan->reliable_buf, chan->message_buf, chan->message.cursize);
		chan->reliable_length = chan->message.cursize;
		chan->message.cursize = 0;
		chan->reliable_sequence ^= 1;
		send_reliable = true;
	}

// write the packet header
	send.InitOOB(send_buf, sizeof(send_buf));

	w1 = chan->outgoing_sequence | (send_reliable<<31);
	w2 = chan->incoming_sequence | (chan->incoming_reliable_sequence<<31);

	chan->outgoing_sequence++;

	send.WriteLong(w1);
	send.WriteLong(w2);

// copy the reliable message to the packet first
	if (send_reliable)
	{
		send.WriteData(chan->reliable_buf, chan->reliable_length);
		chan->last_reliable_sequence = chan->outgoing_sequence;
	}
	
// add the unreliable part if space is available
	if (send.maxsize - send.cursize >= length)
		send.WriteData(data, length);

// send the datagram
	i = chan->outgoing_sequence & (MAX_LATENT-1);
	chan->outgoing_size[i] = send.cursize;
	chan->outgoing_time[i] = realtime;

	//zoid, no input in demo playback mode
#ifndef SERVERONLY
	if (!cls.demoplayback)
#endif
		NET_SendPacket (send.cursize, send._data, chan->remote_address);

	if (chan->cleartime < realtime)
		chan->cleartime = realtime + send.cursize*chan->rate;
	else
		chan->cleartime += send.cursize*chan->rate;

	if (showpackets->value)
		Con_Printf ("--> s=%i(%i) a=%i(%i) %i\n"
			, chan->outgoing_sequence
			, send_reliable
			, chan->incoming_sequence
			, chan->incoming_reliable_sequence
			, send.cursize);

}

int	packet_latency[256];

/*
=================
Netchan_Process

called when the current net_message is from remote_address
modifies net_message so that it points to the packet payload
=================
*/
qboolean Netchan_Process (netchan_t *chan)
{
	unsigned		sequence, sequence_ack;
	unsigned		reliable_ack, reliable_message;

	if (
#ifndef SERVERONLY
			!cls.demoplayback && 
#endif
			!NET_CompareAdr (net_from, chan->remote_address))
		return false;
	
// get sequence numbers		
	net_message.BeginReadingOOB();
	sequence = net_message.ReadLong();
	sequence_ack = net_message.ReadLong();

	reliable_message = sequence >> 31;
	reliable_ack = sequence_ack >> 31;

	sequence &= ~(1<<31);	
	sequence_ack &= ~(1<<31);	

	if (showpackets->value)
		Con_Printf ("<-- s=%i(%i) a=%i(%i) %i\n"
			, sequence
			, reliable_message
			, sequence_ack
			, reliable_ack
			, net_message.cursize);

// get a rate estimation
#if 0
	if (chan->outgoing_sequence - sequence_ack < MAX_LATENT)
	{
		int				i;
		double			time, rate;
	
		i = sequence_ack & (MAX_LATENT - 1);
		time = realtime - chan->outgoing_time[i];
		time -= 0.1;	// subtract 100 ms
		if (time <= 0)
		{	// gotta be a digital link for <100 ms ping
			if (chan->rate > 1.0/5000)
				chan->rate = 1.0/5000;
		}
		else
		{
			if (chan->outgoing_size[i] < 512)
			{	// only deal with small messages
				rate = chan->outgoing_size[i]/time;
				if (rate > 5000)
					rate = 5000;
				rate = 1.0/rate;
				if (chan->rate > rate)
					chan->rate = rate;
			}
		}
	}
#endif

//
// discard stale or duplicated packets
//
	if (sequence <= chan->incoming_sequence)
	{
		if (showdrop->value)
			Con_Printf ("%s:Out of order packet %i at %i\n"
				, SOCK_AdrToString (chan->remote_address)
				,  sequence
				, chan->incoming_sequence);
		return false;
	}

//
// dropped packets don't keep the message from being used
//
	net_drop = sequence - (chan->incoming_sequence+1);
	if (net_drop > 0)
	{
		chan->drop_count += 1;

		if (showdrop->value)
			Con_Printf ("%s:Dropped %i packets at %i\n"
			, SOCK_AdrToString (chan->remote_address)
			, sequence-(chan->incoming_sequence+1)
			, sequence);
	}

//
// if the current outgoing reliable message has been acknowledged
// clear the buffer to make way for the next
//
	if (reliable_ack == chan->reliable_sequence)
		chan->reliable_length = 0;	// it has been received
	
//
// if this message contains a reliable message, bump incoming_reliable_sequence 
//
	chan->incoming_sequence = sequence;
	chan->incoming_acknowledged = sequence_ack;
	chan->incoming_reliable_acknowledged = reliable_ack;
	if (reliable_message)
		chan->incoming_reliable_sequence ^= 1;

//
// the message can now be read from the current message pointer
// update statistics counters
//
	chan->frame_latency = chan->frame_latency*OLD_AVG
		+ (chan->outgoing_sequence-sequence_ack)*(1.0-OLD_AVG);
	chan->frame_rate = chan->frame_rate*OLD_AVG
		+ (realtime-chan->last_received)*(1.0-OLD_AVG);		
	chan->good_count += 1;

	chan->last_received = realtime;

	return true;
}

typedef struct huffnode_s
{
	struct huffnode_s *zero;
	struct huffnode_s *one;
	unsigned char val;
	float freq;
} huffnode_t;

typedef struct
{
	unsigned int bits;
	int len;
} hufftab_t;
static huffnode_t *HuffTree=0;
static hufftab_t HuffLookup[256];
#if _DEBUG
static int HuffIn=0;
static int HuffOut=0;
#endif

/*static float HuffFreq[256]=
{
	0.27588720,
	0.04243389,
	0.01598893,
	0.00737722,
	0.00557754,
	0.00547342,
	0.00823988,
	0.00449177,
	0.00986108,
	0.00560728,
	0.00654431,
	0.00376298,
	0.00498260,
	0.00400095,
	0.00655918,
	0.00232025,
	0.00504209,
	0.00285570,
	0.00124937,
	0.00147247,
	0.00226076,
	0.00141298,
	0.00467026,
	0.00336139,
	0.00123449,
	0.00126424,
	0.00166582,
	0.00129399,
	0.00114525,
	0.00116013,
	0.00078829,
	0.00080317,
	0.00639557,
	0.00123449,
	0.00127911,
	0.00102627,
	0.00182943,
	0.00141298,
	0.00269209,
	0.00127911,
	0.00224589,
	0.00093703,
	0.01972216,
	0.00135348,
	0.00477437,
	0.00337627,
	0.00743671,
	0.00765981,
	0.01951394,
	0.00319779,
	0.00330190,
	0.00267722,
	0.00235000,
	0.00159146,
	0.00285570,
	0.00141298,
	0.00151709,
	0.00139810,
	0.00468513,
	0.00117500,
	0.00202279,
	0.00544367,
	0.00096677,
	0.00136836,
	0.00913228,
	0.00316804,
	0.00138323,
	0.00078829,
	0.00942975,
	0.00590475,
	0.00168070,
	0.00089241,
	0.00095190,
	0.00166582,
	0.00077342,
	0.00071392,
	0.00086266,
	0.00060981,
	0.00075854,
	0.00065443,
	0.00126424,
	0.00047595,
	0.00068418,
	0.00099652,
	0.00065443,
	0.00096677,
	0.00072880,
	0.00050570,
	0.00071392,
	0.00102627,
	0.00120475,
	0.00056519,
	0.00281108,
	0.00175506,
	0.00047595,
	0.00154684,
	0.00160633,
	0.01091710,
	0.00365886,
	0.00355475,
	0.00937026,
	0.01105096,
	0.00404557,
	0.00206741,
	0.00389684,
	0.00456614,
	0.00215665,
	0.00191867,
	0.01072374,
	0.01051551,
	0.00467026,
	0.00867121,
	0.00542880,
	0.00105601,
	0.00649969,
	0.01280602,
	0.00510159,
	0.00316804,
	0.00456614,
	0.00523545,
	0.00157658,
	0.00191867,
	0.00121962,
	0.00065443,
	0.00080317,
	0.00052057,
	0.00080317,
	0.00147247,
	0.02236963,
	0.00258798,
	0.00059494,
	0.00060981,
	0.00038671,
	0.00044620,
	0.00055032,
	0.00093703,
	0.00072880,
	0.00062468,
	0.00059494,
	0.00047595,
	0.00044620,
	0.00182943,
	0.00066930,
	0.00077342,
	0.00114525,
	0.00062468,
	0.00059494,
	0.00049082,
	0.00059494,
	0.00074367,
	0.00090728,
	0.00069905,
	0.00075854,
	0.00077342,
	0.00069905,
	0.00055032,
	0.00075854,
	0.00049082,
	0.00104114,
	0.00062468,
	0.00397121,
	0.00080317,
	0.00072880,
	0.00062468,
	0.00163608,
	0.00046108,
	0.00055032,
	0.00056519,
	0.00102627,
	0.00071392,
	0.00092215,
	0.00078829,
	0.00660380,
	0.00058006,
	0.00065443,
	0.00049082,
	0.00118987,
	0.00077342,
	0.00065443,
	0.00077342,
	0.00151709,
	0.00083291,
	0.00074367,
	0.00098165,
	0.00108576,
	0.00081804,
	0.00145760,
	0.00144272,
	0.00394146,
	0.00104114,
	0.00099652,
	0.00209715,
	0.00502722,
	0.00309367,
	0.00123449,
	0.00096677,
	0.00090728,
	0.00206741,
	0.00147247,
	0.00147247,
	0.00129399,
	0.00087753,
	0.00120475,
	0.00116013,
	0.00145760,
	0.00120475,
	0.00138323,
	0.00123449,
	0.00166582,
	0.00087753,
	0.00171044,
	0.00239462,
	0.00156171,
	0.00154684,
	0.00217152,
	0.00226076,
	0.00211203,
	0.00153196,
	0.00174019,
	0.00145760,
	0.00168070,
	0.00145760,
	0.00123449,
	0.00165095,
	0.00188893,
	0.00138323,
	0.00120475,
	0.00154684,
	0.00138323,
	0.00191867,
	0.01866615,
	0.00139810,
	0.00153196,
	0.00093703,
	0.00121962,
	0.00116013,
	0.00075854,
	0.00105601,
	0.00432817,
	0.00315317,
	0.00407532,
	0.00227563,
	0.00081804,
	0.00121962,
	0.00110063,
	0.00090728,
	0.00108576,
	0.00065443,
	0.00096677,
	0.00655918,
	0.00153196,
	0.00251361,
	0.00312342,
	0.00243924,
	0.00660380,
	0.01700033,
};
*/

static float HuffFreq[256]=
{
 0.14473691,
 0.01147017,
 0.00167522,
 0.03831121,
 0.00356579,
 0.03811315,
 0.00178254,
 0.00199644,
 0.00183511,
 0.00225716,
 0.00211240,
 0.00308829,
 0.00172852,
 0.00186608,
 0.00215921,
 0.00168891,
 0.00168603,
 0.00218586,
 0.00284414,
 0.00161833,
 0.00196043,
 0.00151029,
 0.00173932,
 0.00218370,
 0.00934121,
 0.00220530,
 0.00381211,
 0.00185456,
 0.00194675,
 0.00161977,
 0.00186680,
 0.00182071,
 0.06421956,
 0.00537786,
 0.00514019,
 0.00487155,
 0.00493925,
 0.00503143,
 0.00514019,
 0.00453520,
 0.00454241,
 0.00485642,
 0.00422407,
 0.00593387,
 0.00458130,
 0.00343687,
 0.00342823,
 0.00531592,
 0.00324890,
 0.00333388,
 0.00308613,
 0.00293776,
 0.00258918,
 0.00259278,
 0.00377105,
 0.00267488,
 0.00227516,
 0.00415997,
 0.00248763,
 0.00301555,
 0.00220962,
 0.00206990,
 0.00270369,
 0.00231694,
 0.00273826,
 0.00450928,
 0.00384380,
 0.00504728,
 0.00221251,
 0.00376961,
 0.00232990,
 0.00312574,
 0.00291688,
 0.00280236,
 0.00252436,
 0.00229461,
 0.00294353,
 0.00241201,
 0.00366590,
 0.00199860,
 0.00257838,
 0.00225860,
 0.00260646,
 0.00187256,
 0.00266552,
 0.00242641,
 0.00219450,
 0.00192082,
 0.00182071,
 0.02185930,
 0.00157439,
 0.00164353,
 0.00161401,
 0.00187544,
 0.00186248,
 0.03338637,
 0.00186968,
 0.00172132,
 0.00148509,
 0.00177749,
 0.00144620,
 0.00192442,
 0.00169683,
 0.00209439,
 0.00209439,
 0.00259062,
 0.00194531,
 0.00182359,
 0.00159096,
 0.00145196,
 0.00128199,
 0.00158376,
 0.00171412,
 0.00243433,
 0.00345704,
 0.00156359,
 0.00145700,
 0.00157007,
 0.00232342,
 0.00154198,
 0.00140730,
 0.00288807,
 0.00152830,
 0.00151246,
 0.00250203,
 0.00224420,
 0.00161761,
 0.00714383,
 0.08188576,
 0.00802537,
 0.00119484,
 0.00123805,
 0.05632671,
 0.00305156,
 0.00105584,
 0.00105368,
 0.00099246,
 0.00090459,
 0.00109473,
 0.00115379,
 0.00261223,
 0.00105656,
 0.00124381,
 0.00100326,
 0.00127550,
 0.00089739,
 0.00162481,
 0.00100830,
 0.00097229,
 0.00078864,
 0.00107240,
 0.00084409,
 0.00265760,
 0.00116891,
 0.00073102,
 0.00075695,
 0.00093916,
 0.00106880,
 0.00086786,
 0.00185600,
 0.00608367,
 0.00133600,
 0.00075695,
 0.00122077,
 0.00566955,
 0.00108249,
 0.00259638,
 0.00077063,
 0.00166586,
 0.00090387,
 0.00087074,
 0.00084914,
 0.00130935,
 0.00162409,
 0.00085922,
 0.00093340,
 0.00093844,
 0.00087722,
 0.00108249,
 0.00098598,
 0.00095933,
 0.00427593,
 0.00496661,
 0.00102775,
 0.00159312,
 0.00118404,
 0.00114947,
 0.00104936,
 0.00154342,
 0.00140082,
 0.00115883,
 0.00110769,
 0.00161112,
 0.00169107,
 0.00107816,
 0.00142747,
 0.00279804,
 0.00085922,
 0.00116315,
 0.00119484,
 0.00128559,
 0.00146204,
 0.00130215,
 0.00101551,
 0.00091756,
 0.00161184,
 0.00236375,
 0.00131872,
 0.00214120,
 0.00088875,
 0.00138570,
 0.00211960,
 0.00094060,
 0.00088083,
 0.00094564,
 0.00090243,
 0.00106160,
 0.00088659,
 0.00114514,
	0.00095861,
	0.00108753,
	0.00124165,
	0.00427016,
	0.00159384,
	0.00170547,
	0.00104431,
	0.00091395,
	0.00095789,
	0.00134681,
	0.00095213,
	0.00105944,
	0.00094132,
	0.00141883,
	0.00102127,
	0.00101911,
	0.00082105,
	0.00158448,
	0.00102631,
	0.00087938,
	0.00139290,
	0.00114658,
	0.00095501,
	0.00161329,
	0.00126542,
	0.00113218,
	0.00123661,
	0.00101695,
	0.00112930,
	0.00317976,
	0.00085346,
	0.00101190,
	0.00189849,
	0.00105728,
	0.00186824,
	0.00092908,
	0.00160896,
};

#ifdef _DEBUG
static int freqs[256];
static void ZeroFreq()
{
	Com_Memset(freqs, 0, 256*sizeof(int));
}


static void CalcFreq(unsigned char *packet, int packetlen)
{
	int ix;

	for (ix=0; ix<packetlen; ix++)
	{
		freqs[packet[ix]]++;
	}
}

static void PrintFreqs()
{
	int ix;
	float total=0;
	char string[100];

	for (ix=0; ix<256; ix++)
	{
		total += freqs[ix];
	}
	if (total>.01)
	{
		for (ix=0; ix<256; ix++)
		{
			sprintf(string, "\t%.8f,\n",((float)freqs[ix])/total);
			OutputDebugString(string);
		}
	}
	ZeroFreq();
}
#endif

//=============================================================================

void FindTab(huffnode_t *tmp,int len,unsigned int bits)
{
	if(!tmp)
		Sys_Error("no huff node");
	if (tmp->zero)
	{
		if(!tmp->one)
			Sys_Error("no one in node");
		if(len>=32)
			Sys_Error("compression screwd");
		FindTab(tmp->zero,len+1,bits<<1);
		FindTab(tmp->one,len+1,(bits<<1)|1);
		return;
	}
	HuffLookup[tmp->val].len=len;
	HuffLookup[tmp->val].bits=bits;
	return;
}
static unsigned char Masks[8]=
{
	0x1,
	0x2,
	0x4,
	0x8,
	0x10,
	0x20,
	0x40,
	0x80
};

void PutBit(unsigned char *buf,int pos,int bit)
{
	if (bit)
		buf[pos/8]|=Masks[pos%8];
	else
		buf[pos/8]&=~Masks[pos%8];
}

int GetBit(unsigned char *buf,int pos)
{
	if (buf[pos/8]&Masks[pos%8])
		return 1;
	else
		return 0;
}


static void BuildTree(float *freq)
{
	float min1,min2;
	int i,j,minat1,minat2;
	huffnode_t *work[256];	
	huffnode_t *tmp;	
	for (i=0;i<256;i++)
	{
		work[i]=(huffnode_t*)malloc(sizeof(huffnode_t));
		work[i]->val=(unsigned char)i;
		work[i]->freq=freq[i];
		work[i]->zero=0;
		work[i]->one=0;
		HuffLookup[i].len=0;
	}
	for (i=0;i<255;i++)
	{
		minat1=-1;
		minat2=-1;
		min1=1E30;
		min2=1E30;
		for (j=0;j<256;j++)
		{
			if (!work[j])
				continue;
			if (work[j]->freq<min1)
			{
				minat2=minat1;
				min2=min1;
				minat1=j;
				min1=work[j]->freq;
			}
			else if (work[j]->freq<min2)
			{
				minat2=j;
				min2=work[j]->freq;
			}
		}
		if (minat1<0)
			Sys_Error("minatl: %f",minat1);
		if (minat2<0)
			Sys_Error("minat2: %f",minat2);
		tmp=(huffnode_t*)malloc(sizeof(huffnode_t));
		tmp->zero=work[minat2];
		tmp->one=work[minat1];
		tmp->freq=work[minat2]->freq+work[minat1]->freq;
		tmp->val=0xff;
		work[minat1]=tmp;
		work[minat2]=0;
	}		
	HuffTree=tmp;
	FindTab(HuffTree,0,0);
#if _DEBUG
	for (i=0;i<256;i++)
	{
		if(!HuffLookup[i].len&&HuffLookup[i].len<=32)
			Sys_Error("bad frequency table");
//		CON_Printf("%d %d %2X\n", HuffLookup[i].len, HuffLookup[i].bits, i);
	}
#endif
}

static void HuffDecode(unsigned char *in,unsigned char *out,int inlen,int *outlen)
{
	int bits,tbits;
	huffnode_t *tmp;	
	if (*in==0xff)
	{
		Com_Memcpy(out,in+1,inlen-1);
		*outlen=inlen-1;
		return;
	}
	tbits=(inlen-1)*8-*in;
	bits=0;
	*outlen=0;
	while (bits<tbits)
	{
		tmp=HuffTree;
		do
		{
			if (GetBit(in+1,bits))
				tmp=tmp->one;
			else
				tmp=tmp->zero;
			bits++;
		} while (tmp->zero);
		*out++=tmp->val;
		(*outlen)++;
	}
}

static void HuffEncode(unsigned char *in,unsigned char *out,int inlen,int *outlen)
{
	int i,j,bitat;
	unsigned int t;
	bitat=0;
	for (i=0;i<inlen;i++)
	{
		t=HuffLookup[in[i]].bits;
		for (j=0;j<HuffLookup[in[i]].len;j++)
		{
			PutBit(out+1,bitat+HuffLookup[in[i]].len-j-1,t&1);
			t>>=1;
		}
		bitat+=HuffLookup[in[i]].len;
	}
	*outlen=1+(bitat+7)/8;
	*out=8*((*outlen)-1)-bitat;
	if(*outlen >= inlen+1)
	{
		*out=0xff;
		Com_Memcpy(out+1,in,inlen);
		*outlen=inlen+1;
	}
#if _DEBUG

	HuffIn+=inlen;
	HuffOut+=*outlen;

	{
		unsigned char *buf;
		int tlen;
		buf=(byte*)malloc(inlen);
		HuffDecode(out,buf,*outlen,&tlen);
		if(!tlen==inlen)
			Sys_Error("bogus compression");
		for (i=0;i<inlen;i++)
		{
			if(in[i]!=buf[i])
				Sys_Error("bogus compression");
		}
		free(buf);
	}
#endif
}

//=============================================================================

static int LastCompMessageSize = 0;

netadr_t	net_local_adr;

netadr_t	net_from;
QMsg		net_message;
int			net_socket;

#define	MAX_UDP_PACKET	(MAX_MSGLEN+9)	// one more than msg + header
static byte		net_message_buffer[MAX_UDP_PACKET];

//=============================================================================

qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;
	return false;
}

char	*NET_BaseAdrToString (netadr_t a)
{
	static	char	s[64];
	
	sprintf (s, "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);

	return s;
}

//=============================================================================
static unsigned char huffbuff[65536];

qboolean NET_GetPacket (void)
{
	int ret = SOCK_Recv(net_socket, huffbuff, sizeof(net_message_buffer), &net_from);
	if (ret == SOCKRECV_NO_DATA)
	{
		return false;
	}
	if (ret == SOCKRECV_ERROR)
	{
		Sys_Error("NET_GetPacket failed");
	}

	if (ret == sizeof(net_message_buffer) )
	{
		Con_Printf ("Oversize packet from %s\n", SOCK_AdrToString (net_from));
		return false;
	}

	LastCompMessageSize += ret;//keep track of bytes actually received for debugging

	HuffDecode(huffbuff, (unsigned char *)net_message_buffer,ret,&ret);
	net_message.cursize = ret;

	return ret;
}

//=============================================================================

void NET_SendPacket (int length, void *data, netadr_t to)
{
	int outlen;

	HuffEncode((unsigned char *)data, huffbuff, length, &outlen);

#if defined(_WIN32) && defined(_DEBUG)
	char string[120];
	sprintf(string,"in: %d  out: %d  ratio: %f\n",HuffIn, HuffOut, 1-(float)HuffOut/(float)HuffIn);
	OutputDebugString(string);

	CalcFreq((unsigned char *)data, length);
#endif

	SOCK_Send(net_socket, huffbuff, outlen, &to);
}

//=============================================================================

static int UDP_OpenSocket (int port)
{
	int newsocket = SOCK_Open(NULL, port);
	if (newsocket == 0)
		Sys_Error ("UDP_OpenSocket: failed");
	return newsocket;
}

static void NET_GetLocalAddress()
{
	SOCK_GetLocalAddress();

	net_local_adr.type = NA_IP;
	net_local_adr.ip[0] = localIP[0][0];
	net_local_adr.ip[1] = localIP[0][1];
	net_local_adr.ip[2] = localIP[0][2];
	net_local_adr.ip[3] = localIP[0][3];

	netadr_t address;
	if (!SOCK_GetAddr(net_socket, &address))
	{
		Sys_Error("NET_Init: getsockname failed");
	}
	net_local_adr.port = address.port;
}

/*
====================
NET_Init
====================
*/
void NET_Init (int port)
{
#ifdef _DEBUG
	ZeroFreq();
#endif

	BuildTree(HuffFreq);

	if (!SOCK_Init())
		Sys_Error ("Sockets initialization failed.");

	//
	// open the single socket to be used for all communications
	//
	net_socket = UDP_OpenSocket (port);

	//
	// init the message buffer
	//
	net_message.InitOOB(net_message_buffer, sizeof(net_message_buffer));

	//
	// determine my name & address
	//
	NET_GetLocalAddress ();
}

/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	SOCK_Close(net_socket);
	SOCK_Shutdown();

#ifdef _DEBUG
	PrintFreqs();
#endif
}
