// gl_ngraph.c

#include "quakedef.h"
#include "glquake.h"

extern byte		*draw_chars;				// 8*8 graphic characters

image_t*	netgraphtexture;	// netgraph texture

#define NET_GRAPHHEIGHT 32

#define	NET_TIMINGS	256
static	int	packet_latency[NET_TIMINGS];

static	byte ngraph_texels[NET_GRAPHHEIGHT][NET_TIMINGS];

static void R_LineGraph (int x, int h)
{
	int		i;
	int		s;
	int		color;

	s = NET_GRAPHHEIGHT;

	if (h == 10000)
		color = 0x6f;	// yellow
	else if (h == 9999)
		color = 0x4f;	// red
	else if (h == 9998)
		color = 0xd0;	// blue
	else
		color = 0xfe;	// white

	if (h>s)
		h = s;
	
	for (i=0 ; i<h ; i++)
		if (i & 1)
			ngraph_texels[NET_GRAPHHEIGHT - i - 1][x] = 0xff;
		else
			ngraph_texels[NET_GRAPHHEIGHT - i - 1][x] = (byte)color;

	for ( ; i<s ; i++)
		ngraph_texels[NET_GRAPHHEIGHT - i - 1][x] = (byte)0xff;
}

void Draw_CharToNetGraph (int x, int y, int num)
{
	int		row, col;
	byte	*source;
	int		drawline;
	int		nx;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	for (drawline = 8; drawline; drawline--, y++)
	{
		for (nx=0 ; nx<8 ; nx++)
			if (source[nx] != 255)
				ngraph_texels[y][nx+x] = 0x60 + source[nx];
		source += 128;
	}
}


/*
==============
R_NetGraph
==============
*/
void R_NetGraph (void)
{
	int		a, x, i, y;
	frame_t	*frame;
	int lost;
	char st[80];
	unsigned	ngraph_pixels[NET_GRAPHHEIGHT][NET_TIMINGS];

	for (i=cls.netchan.outgoing_sequence-UPDATE_BACKUP+1
		; i <= cls.netchan.outgoing_sequence
		; i++)
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
	lost = 0;
	for (a=0 ; a<NET_TIMINGS ; a++)
	{
		i = (cls.netchan.outgoing_sequence-a) & 255;
		if (packet_latency[i] == 9999)
			lost++;
		R_LineGraph (NET_TIMINGS-1-a, packet_latency[i]);
	}

	// now load the netgraph texture into gl and draw it
	for (y = 0; y < NET_GRAPHHEIGHT; y++)
		for (x = 0; x < NET_TIMINGS; x++)
			ngraph_pixels[y][x] = d_8to24table[ngraph_texels[y][x]];

	x =	-((vid.width - 320)>>1);
	y = vid.height - sb_lines - 24 - NET_GRAPHHEIGHT - 1;

	M_DrawTextBox (x, y, NET_TIMINGS/8, NET_GRAPHHEIGHT/8 + 1);
	y += 8;

	sprintf(st, "%3i%% packet loss", lost*100/NET_TIMINGS);
	Draw_String(8, y, st);
	y += 8;
	
	if (!netgraphtexture)
	{
		netgraphtexture = R_CreateImage("*netgraph", (byte*)ngraph_pixels, NET_TIMINGS, NET_GRAPHHEIGHT, false, false, GL_CLAMP, false);
	}
	else
	{
		R_ReUploadImage(netgraphtexture, (byte*)ngraph_pixels);
	}

	GL_Bind(netgraphtexture);
	GL_TexEnv(GL_MODULATE);

	x = 8;
	qglColor3f (1,1,1);
	qglBegin (GL_QUADS);
	qglTexCoord2f (0, 0);
	qglVertex2f (x, y);
	qglTexCoord2f (1, 0);
	qglVertex2f (x+NET_TIMINGS, y);
	qglTexCoord2f (1, 1);
	qglVertex2f (x+NET_TIMINGS, y+NET_GRAPHHEIGHT);
	qglTexCoord2f (0, 1);
	qglVertex2f (x, y+NET_GRAPHHEIGHT);
	qglEnd ();
}

