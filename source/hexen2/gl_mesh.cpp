// gl_mesh.c: triangle model functions

#include "quakedef.h"
#include "glquake.h"

/*
=================================================================

ALIAS MODEL DISPLAY LIST GENERATION

=================================================================
*/

model_t		*aliasmodel;
aliashdr_t	*paliashdr;

qboolean	used[8192];

// the command list holds counts and s/t values that are valid for
// every frame
int		commands[8192];
int		numcommands;

// all frames will have their vertexes rearranged and expanded
// so they are in the order expected by the command list
int		vertexorder[8192];
int		numorder;

int		allverts, alltris;

int		stripverts[128];
int		striptris[128];
int		stripstverts[128];
int		stripcount;

/*
================
StripLength
================
*/
int	StripLength (int starttri, int startv)
{
	int			m1, m2;
	int			st1, st2;
	int			j;
	mtriangle_t	*last, *check;
	int			k;

	used[starttri] = 2;

	last = &triangles[starttri];

	stripverts[0] = last->vertindex[(startv)%3];
	stripstverts[0] = last->stindex[(startv)%3];

	stripverts[1] = last->vertindex[(startv+1)%3];
	stripstverts[1] = last->stindex[(startv+1)%3];

	stripverts[2] = last->vertindex[(startv+2)%3];
	stripstverts[2] = last->stindex[(startv+2)%3];

	striptris[0] = starttri;
	stripcount = 1;

	m1 = last->vertindex[(startv+2)%3];
	st1 = last->stindex[(startv+2)%3];
	m2 = last->vertindex[(startv+1)%3];
	st2 = last->stindex[(startv+1)%3];

	// look for a matching triangle
nexttri:
	for (j=starttri+1, check=&triangles[starttri+1] ; j<pheader->numtris ; j++, check++)
	{
		if (check->facesfront != last->facesfront)
			continue;
		for (k=0 ; k<3 ; k++)
		{
			if (check->vertindex[k] != m1)
				continue;
			if (check->stindex[k] != st1)
				continue;
			if (check->vertindex[ (k+1)%3 ] != m2)
				continue;
			if (check->stindex[ (k+1)%3 ] != st2)
				continue;

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if (used[j])
				goto done;

			// the new edge
			if (stripcount & 1)
			{
				m2 = check->vertindex[ (k+2)%3 ];
				st2 = check->stindex[ (k+2)%3 ];
			}
			else
			{
				m1 = check->vertindex[ (k+2)%3 ];
				st1 = check->stindex[ (k+2)%3 ];
			}

			stripverts[stripcount+2] = check->vertindex[ (k+2)%3 ];
			stripstverts[stripcount+2] = check->stindex[ (k+2)%3 ];
			striptris[stripcount] = j;
			stripcount++;

			used[j] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for (j=starttri+1 ; j<pheader->numtris ; j++)
		if (used[j] == 2)
			used[j] = 0;

	return stripcount;
}

/*
===========
FanLength
===========
*/
int	FanLength (int starttri, int startv)
{
	int		m1, m2;
	int		st1, st2;
	int		j;
	mtriangle_t	*last, *check;
	int		k;

	used[starttri] = 2;

	last = &triangles[starttri];

	stripverts[0] = last->vertindex[(startv)%3];
	stripstverts[0] = last->stindex[(startv)%3];

	stripverts[1] = last->vertindex[(startv+1)%3];
	stripstverts[1] = last->stindex[(startv+1)%3];

	stripverts[2] = last->vertindex[(startv+2)%3];
	stripstverts[2] = last->stindex[(startv+2)%3];

	striptris[0] = starttri;
	stripcount = 1;

	m1 = last->vertindex[(startv+0)%3];
	st1 = last->stindex[(startv+2)%3];
	m2 = last->vertindex[(startv+2)%3];
	st2 = last->stindex[(startv+1)%3];


	// look for a matching triangle
nexttri:
	for (j=starttri+1, check=&triangles[starttri+1] ; j<pheader->numtris ; j++, check++)
	{
		if (check->facesfront != last->facesfront)
			continue;
		for (k=0 ; k<3 ; k++)
		{
			if (check->vertindex[k] != m1)
				continue;
			if (check->stindex[k] != st1)
				continue;
			if (check->vertindex[ (k+1)%3 ] != m2)
				continue;
			if (check->stindex[ (k+1)%3 ] != st2)
				continue;

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if (used[j])
				goto done;

			// the new edge
			m2 = check->vertindex[ (k+2)%3 ];
			st2 = check->stindex[ (k+2)%3 ];

			stripverts[stripcount+2] = m2;
			stripstverts[stripcount+2] = st2;
			striptris[stripcount] = j;
			stripcount++;

			used[j] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for (j=starttri+1 ; j<pheader->numtris ; j++)
		if (used[j] == 2)
			used[j] = 0;

	return stripcount;
}


/*
================
BuildTris

Generate a list of trifans or strips
for the model, which holds for all frames
================
*/
void BuildTris (void)
{
	int		i, j, k;
	int		startv;
	mtriangle_t	*last, *check;
	int		m1, m2;
	int		striplength;
	trivertx_t	*v;
	mtriangle_t *tv;
	float	s, t;
	int		index;
	int		len, bestlen, besttype;
	int		bestverts[1024];
	int		besttris[1024];
	int		beststverts[1024];
	int		type;
	int sttemp;
	//
	// build tristrips
	//
	numorder = 0;
	numcommands = 0;
	Com_Memset(used, 0, sizeof(used));
	for (i=0 ; i<pheader->numtris ; i++)
	{
		// pick an unused triangle and start the trifan
		if (used[i])
			continue;

		bestlen = 0;
		for (type = 0 ; type < 2 ; type++)
//	type = 1;
		{
			for (startv =0 ; startv < 3 ; startv++)
			{
				if (type == 1)
					len = StripLength (i, startv);
				else
					len = FanLength (i, startv);
				if (len > bestlen)
				{
					besttype = type;
					bestlen = len;
					for (j=0 ; j<bestlen+2 ; j++)
					{
						beststverts[j] = stripstverts[j];
						bestverts[j] = stripverts[j];
					}
					for (j=0 ; j<bestlen ; j++)
						besttris[j] = striptris[j];
				}
			}
		}

		// mark the tris on the best strip as used
		for (j=0 ; j<bestlen ; j++)
			used[besttris[j]] = 1;

		if (besttype == 1)
			commands[numcommands++] = (bestlen+2);
		else
			commands[numcommands++] = -(bestlen+2);

		
		for (j=0 ; j<bestlen+2 ; j++)
		{
			// emit a vertex into the reorder buffer
			k = bestverts[j];
			vertexorder[numorder++] = k;

			k = beststverts[j];

			// emit s/t coords into the commands stream
			s = stverts[k].s;
			t = stverts[k].t;

			if (!triangles[besttris[0]].facesfront && stverts[k].onseam)
				s += pheader->skinwidth / 2;	// on back side
			s = (s + 0.5) / pheader->skinwidth;
			t = (t + 0.5) / pheader->skinheight;

			*(float *)&commands[numcommands++] = s;
			*(float *)&commands[numcommands++] = t;
		}
	}

	commands[numcommands++] = 0;		// end of list marker

	Con_DPrintf ("%3i tri %3i vert %3i cmd\n", pheader->numtris, numorder, numcommands);

	allverts += numorder;
	alltris += pheader->numtris;
}


/*
================
GL_MakeAliasModelDisplayLists
================
*/
void GL_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr)
{
	int		i, j;
	maliasgroup_t	*paliasgroup;
	int			*cmds;
	trivertx_t	*verts;
	char	cache[MAX_QPATH], fullpath[MAX_OSPATH], *c;
	fileHandle_t	f;
	int		len;
	byte	*data;

	aliasmodel = m;
	paliashdr = hdr;	// (aliashdr_t *)Mod_Extradata (m);

	//
	// look for a cached version
	//
	QStr::Cpy(cache, "glhexen/");
	QStr::StripExtension (m->name+QStr::Length("models/"), cache+QStr::Length("glhexen/"));
	QStr::Cat(cache, sizeof(cache), ".ms2");

	FS_FOpenFileRead (cache, &f, true);	
	if (f)
	{
		FS_Read(&numcommands, 4, f);
		FS_Read(&numorder, 4, f);
		FS_Read(&commands, numcommands * sizeof(commands[0]), f);
		FS_Read(&vertexorder, numorder * sizeof(vertexorder[0]), f);
		FS_FCloseFile(f);
	}
	else
	{
		//
		// build it from scratch
		//
		Con_Printf ("meshing %s...\n",m->name);

		BuildTris ();		// trifans or lists

		//
		// save out the cached version
		//
		f = FS_FOpenFileWrite (cache);
		if (f)
		{
			FS_Write(&numcommands, 4, f);
			FS_Write(&numorder, 4, f);
			FS_Write(&commands, numcommands * sizeof(commands[0]), f);
			FS_Write(&vertexorder, numorder * sizeof(vertexorder[0]), f);
			FS_FCloseFile(f);
		}
	}


	// save the data out

	paliashdr->poseverts = numorder;

	cmds = (int*)Hunk_Alloc (numcommands * 4);
	paliashdr->commands = (byte *)cmds - (byte *)paliashdr;
	Com_Memcpy(cmds, commands, numcommands * 4);

	verts = (trivertx_t*)Hunk_Alloc (paliashdr->numposes * paliashdr->poseverts 
		* sizeof(trivertx_t) );
	paliashdr->posedata = (byte *)verts - (byte *)paliashdr;
	for (i=0 ; i<paliashdr->numposes ; i++)
		for (j=0 ; j<numorder ; j++)
			*verts++ = poseverts[i][vertexorder[j]];
}

