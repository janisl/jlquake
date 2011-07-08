// r_light.c

#include "quakedef.h"
#include "glquake.h"

//==========================================================================
//
// R_AnimateLight
//
//==========================================================================

void R_AnimateLight(void)
{
	int i;
	int v;
	int c;
	int defaultLocus;
	int locusHz[3];

	defaultLocus = locusHz[0] = (int)(cl.time*10);
	locusHz[1] = (int)(cl.time*20);
	locusHz[2] = (int)(cl.time*30);
	for(i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		if(!cl_lightstyle[i].length)
		{ // No style def
			d_lightstylevalue[i] = 256;
			continue;
		}
		c = cl_lightstyle[i].map[0];
		if(c == '1' || c == '2' || c == '3')
		{ // Explicit anim rate
			if(cl_lightstyle[i].length == 1)
			{ // Bad style def
				d_lightstylevalue[i] = 256;
				continue;
			}
			v = locusHz[c-'1']%(cl_lightstyle[i].length-1);
			d_lightstylevalue[i] = (cl_lightstyle[i].map[v+1]-'a')*22;
			continue;
		}
		// Default anim rate (10 Hz)
		v = defaultLocus%cl_lightstyle[i].length;
		d_lightstylevalue[i] = (cl_lightstyle[i].map[v]-'a')*22;
	}
}
