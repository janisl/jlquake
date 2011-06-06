
#include "quakedef.h"
#include "../../client/render_local.h"

QCvar*		baseskin;
QCvar*		noskins;

char		allskins[128];
#define	MAX_CACHED_SKINS		128
qw_skin_t		skins[MAX_CACHED_SKINS];
int			numskins;

/*
================
Skin_Find

  Determines the best skin for the given scoreboard
  slot, and sets scoreboard->skin

================
*/
void Skin_Find (player_info_t *sc)
{
	qw_skin_t		*skin;
	int			i;
	char		name[128];
	const char *s;

	if (allskins[0])
		QStr::Cpy(name, allskins);
	else
	{
		s = Info_ValueForKey (sc->userinfo, "skin");
		if (s && s[0])
			QStr::Cpy(name, s);
		else
			QStr::Cpy(name, baseskin->string);
	}

	if (strstr (name, "..") || *name == '.')
		QStr::Cpy(name, "base");

	QStr::StripExtension (name, name);

	for (i=0 ; i<numskins ; i++)
	{
		if (!QStr::Cmp(name, skins[i].name))
		{
			sc->skin = &skins[i];
			Skin_Cache (sc->skin);
			return;
		}
	}

	if (numskins == MAX_CACHED_SKINS)
	{	// ran out of spots, so flush everything
		Skin_Skins_f ();
		return;
	}

	skin = &skins[numskins];
	sc->skin = skin;
	numskins++;

	Com_Memset(skin, 0, sizeof(*skin));
	QStr::Cpy(skin->name, name);
}


/*
==========
Skin_Cache

Returns a pointer to the skin bitmap, or NULL to use the default
==========
*/
byte	*Skin_Cache (qw_skin_t *skin)
{
	char	name[1024];
	byte	*out, *pix;

	if (cls.downloadtype == dl_skin)
		return NULL;		// use base until downloaded

	if (noskins->value==1) // JACK: So NOSKINS > 1 will show skins, but
		return NULL;	  // not download new ones.

	if (skin->failedload)
		return NULL;

	out = (byte*)Cache_Check (&skin->cache);
	if (out)
		return out;

//
// load the pic from disk
//
	sprintf (name, "skins/%s.pcx", skin->name);
	if (!FS_FOpenFileRead(name, NULL, false))
	{
		Con_Printf ("Couldn't load skin %s\n", name);
		sprintf (name, "skins/%s.pcx", baseskin->string);
		if (!FS_FOpenFileRead(name, NULL, false))
		{
			skin->failedload = true;
			return NULL;
		}
	}

	int width;
	int height;
	R_LoadPCX(name, &pix, NULL, &width, &height);

	if (!pix)
	{
		skin->failedload = true;
		Con_Printf ("Skin %s was malformed.  You should delete it.\n", name);
		return NULL;
	}

	out = (byte*)Cache_Alloc (&skin->cache, 320*200, skin->name);
	if (!out)
		Sys_Error ("Skin_Cache: couldn't allocate");

	Com_Memset(out, 0, 320*200);

	byte* outp = out;
	byte* pixp = pix;
	for (int y = 0; y < height; y++, outp += 320, pixp += width)
	{
		Com_Memcpy(outp, pixp, width);
	}

	skin->failedload = false;

	return out;
}


/*
=================
Skin_NextDownload
=================
*/
void Skin_NextDownload (void)
{
	char	*s;
	player_info_t	*sc;
	FILE		*f;
	int			i;
	char		name[1024];

	if (cls.downloadnumber == 0)
		Con_Printf ("Checking skins...\n");
	cls.downloadtype = dl_skin;

	for ( 
		; cls.downloadnumber != MAX_CLIENTS
		; cls.downloadnumber++)
	{
		sc = &cl.players[cls.downloadnumber];
		if (!sc->name[0])
			continue;
		Skin_Find (sc);
		if (noskins->value)
			continue;
		if (!CL_CheckOrDownloadFile(va("skins/%s.pcx", sc->skin->name)))
			return;		// started a download
	}

	cls.downloadtype = dl_none;

	// now load them in for real
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		sc = &cl.players[i];
		if (!sc->name[0])
			continue;
		Skin_Cache (sc->skin);
		sc->skin = NULL;
	}

	if (cls.state != ca_active)
	{	// get next signon phase
		cls.netchan.message.WriteByte(clc_stringcmd);
		cls.netchan.message.WriteString2(
			va("begin %i", cl.servercount));
		Cache_Report ();		// print remaining memory
	}
}


/*
==========
Skin_Skins_f

Refind all skins, downloading if needed.
==========
*/
void	Skin_Skins_f (void)
{
	int		i;

	for (i=0 ; i<numskins ; i++)
	{
		if (skins[i].cache.data)
			Cache_Free (&skins[i].cache);
	}
	numskins = 0;

	if (cls.state == ca_disconnected)
	{
		Con_Printf("WARNING: cannot complete command because there is no connection to a server\n");
		return;
	}

	cls.downloadnumber = 0;
	cls.downloadtype = dl_skin;
	Skin_NextDownload ();
}


/*
==========
Skin_AllSkins_f

Sets all skins to one specific one
==========
*/
void	Skin_AllSkins_f (void)
{
	QStr::Cpy(allskins, Cmd_Argv(1));

	Skin_Skins_f ();
}
