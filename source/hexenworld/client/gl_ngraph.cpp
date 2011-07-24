// gl_ngraph.c

#include "quakedef.h"

#define NET_GRAPHHEIGHT 32

#define	NET_TIMINGS	256
static	int	packet_latency[NET_TIMINGS];

static void R_LineGraph(int x, int y, int h)
{
	int r, g, b;
	if (h == 10000)
	{
		// yellow
		r = 1;
		g = 1;
		b = 0;
	}
	else if (h == 9999)
	{
		// red
		r = 1;
		g = 0;
		b = 0;
	}
	else if (h == 9998)
	{
		// blue
		r = 0;
		g = 0;
		b = 1;
	}
	else
	{
		// white
		r = 1;
		g = 1;
		b = 1;
	}

	if (h > NET_GRAPHHEIGHT)
	{
		h = NET_GRAPHHEIGHT;
	}

	UI_Fill(8 + x, y + NET_GRAPHHEIGHT - h, 1, h, r, g, b, 1);
}

/*
==============
R_NetGraph
==============
*/
void R_NetGraph (void)
{
	int		x, y;
	frame_t	*frame;
	char st[80];

	for (int i = cls.netchan.outgoing_sequence - UPDATE_BACKUP + 1; i <= cls.netchan.outgoing_sequence; i++)
	{
		frame = &cl.frames[i&UPDATE_MASK];
		if (frame->receivedtime == -1)
			packet_latency[i&255] = 9999;	// dropped
		else if (frame->receivedtime == -2)
			packet_latency[i&255] = 10000;	// choked
		else if (frame->invalid)
			packet_latency[i&255] = 9998;	// invalid delta
		else
			packet_latency[i&255] = (frame->receivedtime - frame->senttime)*20;
	}

	x = 0;
	int lost = 0;
	for (int a = 0; a < NET_TIMINGS; a++)
	{
		int i = (cls.netchan.outgoing_sequence - a) & 255;
		if (packet_latency[i] == 9999)
		{
			lost++;
		}
	}

	x =	-((viddef.width - 320) >> 1);
	y = viddef.height - sb_lines - 24 - NET_GRAPHHEIGHT - 1;

	M_DrawTextBox(x, y, NET_TIMINGS / 8, NET_GRAPHHEIGHT / 8 + 1);
	y += 8;

	sprintf(st, "%3i%% packet loss", lost * 100 / NET_TIMINGS);
	Draw_String(8, y, st);
	y += 8;

	for (int a = 0; a < NET_TIMINGS; a++)
	{
		int i = (cls.netchan.outgoing_sequence - a) & 255;
		R_LineGraph(NET_TIMINGS - 1 - a, y, packet_latency[i]);
	}
}
