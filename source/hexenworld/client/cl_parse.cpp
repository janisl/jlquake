// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"

const char *svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",		// [long] server version
	"svc_setview",		// [short] entity number
	"svc_sound",			// <see code>
	"svc_time",			// [float] server time
	"svc_print",			// [string] null terminated string
	"svc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"svc_setangle",		// [vec3] set the view angle to this absolute value
	
	"svc_serverdata",		// [long] version ...
	"svc_lightstyle",		// [byte] [string]
	"svc_updatename",		// [byte] [string]
	"svc_updatefrags",	// [byte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",	// [byte] [byte]
	"svc_particle",		// [vec3] <variable>
	"svc_damage",			// [byte] impact [byte] blood [vec3] from
	
	"svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"svc_spawnbaseline",
	
	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",

	"svc_cdtrack",
	"svc_sellscreen",

	"svc_smallkick",
	"svc_bigkick",

	"svc_updateping",
	"svc_updateentertime",

	"svc_updatestatlong",
	"svc_muzzleflash",
	"svc_updateuserinfo",
	"svc_download",
	"svc_playerinfo",
	"svc_nails",
	"svc_choke",
	"svc_modellist",
	"svc_soundlist",
	"svc_packetentities",
 	"svc_deltapacketentities",
	"svc_maxspeed",
	"svc_entgravity",

	"svc_plaque",
	"svc_particle_explosion",
	"svc_set_view_tint",
	"svc_start_effect",
	"svc_end_effect",
	"svc_set_view_flags",
	"svc_clear_view_flags",
	"svc_update_inv",
	"svc_particle2",
	"svc_particle3",
	"svc_particle4",
	"svc_turn_effect",
	"svc_update_effect",
	"svc_multieffect",
	"svc_midi_name",
	"svc_raineffect",
	"svc_packmissile",

	"svc_indexed_print",
	"svc_targetupdate",
	"svc_name_print",
	"svc_sound_update_pos",
	"svc_update_piv",
	"svc_player_sound",
	"svc_updatepclass",
	"svc_updatedminfo",	// [byte] [short] [byte]
	"svc_updatesiegeinfo",	// [byte] [byte]
	"svc_updatesiegeteam",	// [byte] [byte]
	"svc_updatesiegelosses",	// [byte] [byte]
	"svc_haskey",		//[byte] [byte]
	"svc_nonehaskey",		//[byte]
	"svc_isdoc",		//[byte] [byte]
	"svc_nodoc",		//[byte]
	"svc_playerskipped"	//[byte]
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL"
};

int	oldparsecountmod;
int	parsecountmod;
double	parsecounttime;

qhandle_t	player_models[MAX_PLAYER_CLASS];

image_t*	playertextures[MAX_CLIENTS];		// up to 16 color translated skins

int		cl_spikeindex, cl_playerindex[MAX_PLAYER_CLASS], cl_flagindex;
int		cl_ballindex, cl_missilestarindex, cl_ravenindex, cl_raven2index;

//=============================================================================

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qboolean	CL_CheckOrDownloadFile (char *filename)
{
	fileHandle_t	f;

	if (strstr (filename, ".."))
	{
		Con_Printf ("Refusing to download a path with ..\n");
		return true;
	}

	FS_FOpenFileRead (filename, &f, true);
	if (f)
	{	// it exists, no need to download
		FS_FCloseFile (f);
		return true;
	}

	//ZOID - can't download when recording
	if (cls.demorecording) {
		Con_Printf("Unable to download %s in record mode.\n", cls.downloadname);
		return true;
	}
	//ZOID - can't download when playback
	if (cls.demoplayback)
		return true;

	String::Cpy(cls.downloadname, filename);
	Con_Printf ("Downloading %s...\n", cls.downloadname);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	String::StripExtension (cls.downloadname, cls.downloadtempname);
	String::Cat(cls.downloadtempname, sizeof(cls.downloadtempname), ".tmp");

	cls.netchan.message.WriteByte(clc_stringcmd);
	cls.netchan.message.WriteString2(
		va("download %s", cls.downloadname));

	cls.downloadnumber++;

	return false;
}

/*
===============
R_NewMap
===============
*/
static void R_NewMap (void)
{
	int		i;
	
	for (i=0 ; i<256 ; i++)
		cl_lightstylevalue[i] = 264;		// normal light value

	CL_ClearParticles ();

	R_EndRegistration();
}

/*
=================
Model_NextDownload
=================
*/
void Model_NextDownload (void)
{
	char	*s;
	int		i;
	extern	char gamedirfile[];

	if (cls.downloadnumber == 0)
	{
		Con_Printf ("Checking models...\n");
		cls.downloadnumber = 1;
	}

	cls.downloadtype = dl_model;
	for ( 
		; cl.model_name[cls.downloadnumber][0]
		; cls.downloadnumber++)
	{
		s = cl.model_name[cls.downloadnumber];
		if (s[0] == '*')
			continue;	// inline brush model
		if (!CL_CheckOrDownloadFile(s))
			return;		// started a download
	}

	CM_LoadMap(cl.model_name[1], true, NULL);
	cl.clip_models[1] = 0;
	R_LoadWorld(cl.model_name[1]);

	for (i = 2; i < MAX_MODELS; i++)
	{
		if (!cl.model_name[i][0])
			break;
		cl.model_precache[i] = R_RegisterModel(cl.model_name[i]);
		if (cl.model_name[i][0] == '*')
		{
			cl.clip_models[i] = CM_InlineModel(String::Atoi(cl.model_name[i] + 1));
		}
		if (!cl.model_precache[i])
		{
			Con_Printf ("\nThe required model file '%s' could not be found or downloaded.\n\n"
				, cl.model_name[i]);
			Con_Printf ("You may need to download or purchase a %s client "
				"pack in order to play on this server.\n\n", gamedirfile);
			CL_Disconnect ();
			return;
		}
	}

	// all done
	R_NewMap();

	PR_LoadStrings();

	Hunk_Check ();		// make sure nothing is hurt

	// done with modellist, request first of static signon messages
	cls.netchan.message.WriteByte(clc_stringcmd);
	cls.netchan.message.WriteString2(
		va("prespawn %i", cl.servercount));
}

/*
=================
Sound_NextDownload
=================
*/
void Sound_NextDownload (void)
{
	char	*s;
	int		i;

	if (cls.downloadnumber == 0)
	{
		Con_Printf ("Checking sounds...\n");
		cls.downloadnumber = 1;
	}

	cls.downloadtype = dl_sound;
	for ( 
		; cl.sound_name[cls.downloadnumber][0]
		; cls.downloadnumber++)
	{
		s = cl.sound_name[cls.downloadnumber];
		if (!CL_CheckOrDownloadFile(va("sound/%s",s)))
			return;		// started a download
	}

	S_BeginRegistration();
	for (i=1 ; i<MAX_SOUNDS ; i++)
	{
		if (!cl.sound_name[i][0])
			break;
		cl.sound_precache[i] = S_RegisterSound(cl.sound_name[i]);
	}
	S_EndRegistration();

	// done with sounds, request models now
	cls.netchan.message.WriteByte(clc_stringcmd);
	cls.netchan.message.WriteString2(
		va("modellist %i", cl.servercount));
}


/*
======================
CL_RequestNextDownload
======================
*/
void CL_RequestNextDownload (void)
{
	switch (cls.downloadtype)
	{
	case dl_single:
		break;
	case dl_model:
		Model_NextDownload ();
		break;
	case dl_sound:
		Sound_NextDownload ();
		break;
	case dl_none:
		break;
	}
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload (void)
{
	int		size, percent;

	// read the data
	size = net_message.ReadShort ();
	percent = net_message.ReadByte ();

	if (cls.demoplayback) {
		if (size > 0)
			net_message.readcount += size;
		return; // not in demo playback
	}

	if (size == -1)
	{
		Con_Printf ("File not found.\n");
		if (cls.download)
		{
			Con_Printf ("cls.download shouldn't have been set\n");
			FS_FCloseFile (cls.download);
			cls.download = 0;
		}
		CL_RequestNextDownload ();
		return;
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		cls.download = FS_FOpenFileWrite(cls.downloadtempname);

		if (!cls.download)
		{
			net_message.readcount += size;
			Con_Printf ("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload ();
			return;
		}
	}

	FS_Write (net_message._data + net_message.readcount, size, cls.download);
	net_message.readcount += size;

	if (percent != 100)
	{
// change display routines by zoid
		// request next block
#if 0
		Con_Printf (".");
		if (10*(percent/10) != cls.downloadpercent)
		{
			cls.downloadpercent = 10*(percent/10);
			Con_Printf ("%i%%", cls.downloadpercent);
		}
#endif
		cls.downloadpercent = percent;

		cls.netchan.message.WriteByte(clc_stringcmd);
		cls.netchan.message.WriteString2("nextdl");
	}
	else
	{
#if 0
		Con_Printf ("100%%\n");
#endif

		FS_FCloseFile (cls.download);

		// rename the temp file to it's final name
		FS_Rename(cls.downloadtempname, cls.downloadname);

		cls.download = NULL;
		cls.downloadpercent = 0;

		// get another file if needed

		CL_RequestNextDownload ();
	}
}


/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

/*
==================
CL_ParseServerData
==================
*/
int	cl_keyholder = -1;
int	cl_doc = -1;
void CL_ParseServerData (void)
{
	char	*str;
	qboolean	cflag = false;
	extern	char	gamedirfile[MAX_OSPATH];
	int protover;
	
	Con_DPrintf ("Serverdata packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();

// parse protocol version number
	protover = net_message.ReadLong ();
	if (protover != PROTOCOL_VERSION && protover != OLD_PROTOCOL_VERSION)
		Host_EndGame ("Server returned version %i, not %i", protover, PROTOCOL_VERSION);

	cl.servercount = net_message.ReadLong ();

	// game directory
	str = const_cast<char*>(net_message.ReadString2());

	if (String::ICmp(gamedirfile, str)) {
		// save current config
		Host_WriteConfiguration ("config.cfg"); 
		cflag = true;
	}

	COM_Gamedir(str);

	//ZOID--run the autoexec.cfg in the gamedir
	//if it exists
	if (cflag)
	{
		if (FS_FileExists("config.cfg"))
		{
			Cbuf_AddText("cl_warncmd 0\n");
			Cbuf_AddText("exec config.cfg\n");
			Cbuf_AddText("exec frontend.cfg\n");
			Cbuf_AddText("cl_warncmd 1\n");
		}
	}

	// parse player slot, high bit means spectator
	cl.playernum = net_message.ReadByte ();
	if (cl.playernum & 128)
	{
		cl.spectator = true;
		cl.playernum &= ~128;
	}

	// get the full level name
	str = const_cast<char*>(net_message.ReadString2());
	String::NCpy(cl.levelname, str, sizeof(cl.levelname)-1);

	// get the movevars
	if (protover == PROTOCOL_VERSION) {
		movevars.gravity			= net_message.ReadFloat();
		movevars.stopspeed          = net_message.ReadFloat();
		movevars.maxspeed           = net_message.ReadFloat();
		movevars.spectatormaxspeed  = net_message.ReadFloat();
		movevars.accelerate         = net_message.ReadFloat();
		movevars.airaccelerate      = net_message.ReadFloat();
		movevars.wateraccelerate    = net_message.ReadFloat();
		movevars.friction           = net_message.ReadFloat();
		movevars.waterfriction      = net_message.ReadFloat();
		movevars.entgravity         = net_message.ReadFloat();
	} else {
		movevars.gravity			= 800;
		movevars.stopspeed          = 100;    
		movevars.maxspeed           = 320;    
		movevars.spectatormaxspeed  = 500;
		movevars.accelerate         = 10;     
		movevars.airaccelerate      = 0.7;    
		movevars.wateraccelerate    = 10;     
		movevars.friction           = 6;      
		movevars.waterfriction      = 1;      
		movevars.entgravity         = 1.0;      
	}

	// seperate the printfs so the server message can have a color
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf ("%c%s\n", 2, str);

	// ask for the sound list next
	cls.netchan.message.WriteByte(clc_stringcmd);
	cls.netchan.message.WriteString2(
		va("soundlist %i", cl.servercount));

	// now waiting for downloads, etc
	cls.state = ca_onserver;
	cl_keyholder = -1;
	cl_doc = -1;
}

/*
==================
CL_ParseSoundlist
==================
*/
void CL_ParseSoundlist (void)
{
	int	numsounds;
	char	*str;

// precache sounds
	Com_Memset(cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds=1 ; ; numsounds++)
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
			break;
		if (numsounds==MAX_SOUNDS)
			Host_EndGame ("Server sent too many sound_precache");
		String::Cpy(cl.sound_name[numsounds], str);
	}

	cls.downloadnumber = 0;
	cls.downloadtype = dl_sound;
	Sound_NextDownload ();
}

/*
==================
CL_ParseModellist
==================
*/
void CL_ParseModellist (void)
{
	int	nummodels;
	char	*str;

// precache models and note certain default indexes
	Com_Memset(cl.model_precache, 0, sizeof(cl.model_precache));
	cl_playerindex[0] = -1;
	cl_playerindex[1] = -1;
	cl_playerindex[2] = -1;
	cl_playerindex[3] = -1;
	cl_playerindex[4] = -1;
	cl_playerindex[5] = -1;//mg-siege
	cl_spikeindex = -1;
	cl_flagindex = -1;
	cl_ballindex = -1;
	cl_missilestarindex = -1;
	cl_ravenindex = -1;
	cl_raven2index = -1;

	for (nummodels=1 ; ; nummodels++)
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
			break;
		if (nummodels==MAX_MODELS)
			Host_EndGame ("Server sent too many model_precache");
		String::Cpy(cl.model_name[nummodels], str);

		if (!String::Cmp(cl.model_name[nummodels],"progs/spike.mdl"))
			cl_spikeindex = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"models/paladin.mdl"))
			cl_playerindex[0] = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"models/crusader.mdl"))
			cl_playerindex[1] = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"models/necro.mdl"))
			cl_playerindex[2] = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"models/assassin.mdl"))
			cl_playerindex[3] = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"models/succubus.mdl"))
			cl_playerindex[4] = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"models/hank.mdl"))
			cl_playerindex[5] = nummodels;//mg-siege
		if (!String::Cmp(cl.model_name[nummodels],"progs/flag.mdl"))
			cl_flagindex = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"models/ball.mdl"))
			cl_ballindex = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"models/newmmis.mdl"))
			cl_missilestarindex = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"models/ravproj.mdl"))
			cl_ravenindex = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"models/vindsht1.mdl"))
			cl_raven2index = nummodels;
	}

	player_models[0] = R_RegisterModel("models/paladin.mdl");
	player_models[1] = R_RegisterModel("models/crusader.mdl");
	player_models[2] = R_RegisterModel("models/necro.mdl");
	player_models[3] = R_RegisterModel("models/assassin.mdl");
	player_models[4] = R_RegisterModel("models/succubus.mdl");
//siege
	player_models[5] = R_RegisterModel("models/hank.mdl");

	cls.downloadnumber = 0;
	cls.downloadtype = dl_model;
	Model_NextDownload ();
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (entity_state_t *es)
{
	int			i;
	
	es->modelindex = net_message.ReadShort ();
	es->frame = net_message.ReadByte ();
	es->colormap = net_message.ReadByte();
	es->skinnum = net_message.ReadByte();
	es->scale = net_message.ReadByte();
	es->drawflags = net_message.ReadByte();
	es->abslight = net_message.ReadByte();

	for (i=0 ; i<3 ; i++)
	{
		es->origin[i] = net_message.ReadCoord ();
		es->angles[i] = net_message.ReadAngle ();
	}
}



/*
=====================
CL_ParseStatic

Static entities are non-interactive world objects
like torches
=====================
*/
void CL_ParseStatic (void)
{
	entity_t *ent;
	int		i;
	entity_state_t	es;

	CL_ParseBaseline (&es);
		
	i = cl.num_statics;
	if (i >= MAX_STATIC_ENTITIES)
		Host_EndGame ("Too many static entities");
	ent = &cl_static_entities[i];
	cl.num_statics++;

// copy it to the current state
	ent->model = cl.model_precache[es.modelindex];
	ent->frame = es.frame;
	ent->skinnum = es.skinnum;
	ent->scale = es.scale;
	ent->drawflags = es.drawflags;
	ent->abslight = es.abslight;

	VectorCopy (es.origin, ent->origin);
	VectorCopy (es.angles, ent->angles);
}

/*
===================
CL_ParseStaticSound
===================
*/
void CL_ParseStaticSound (void)
{
	vec3_t		org;
	int			sound_num, vol, atten;
	int			i;
	
	for (i=0 ; i<3 ; i++)
		org[i] = net_message.ReadCoord ();
	sound_num = net_message.ReadByte ();
	vol = net_message.ReadByte ();
	atten = net_message.ReadByte ();
	
	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
}



/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos;
    int 	channel, ent;
    int 	sound_num;
    int 	volume;
    float 	attenuation;  
 	int		i;
	           
    channel = net_message.ReadShort(); 

    if (channel & SND_VOLUME)
		volume = net_message.ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
    if (channel & SND_ATTENUATION)
		attenuation = net_message.ReadByte () / 32.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	
	sound_num = net_message.ReadByte ();

	for (i=0 ; i<3 ; i++)
		pos[i] = net_message.ReadCoord ();
 
	ent = (channel>>3)&1023;
	channel &= 7;

	if (ent > MAX_EDICTS)
		Host_EndGame ("CL_ParseStartSoundPacket: ent = %i", ent);
	
    S_StartSound(pos, ent, channel, cl.sound_precache[sound_num], volume / 255.0, attenuation);
}       


/*
==================
CL_ParseClientdata

Server information pertaining to this client only, sent every frame
==================
*/
void CL_ParseClientdata (void)
{
	int				i;
	float		latency;
	frame_t		*frame;

// calculate simulated time of message
	oldparsecountmod = parsecountmod;

	i = cls.netchan.incoming_acknowledged;
	cl.parsecount = i;
	i &= UPDATE_MASK;
	parsecountmod = i;
	frame = &cl.frames[i];
	parsecounttime = cl.frames[i].senttime;

	frame->receivedtime = realtime;

// calculate latency
	latency = frame->receivedtime - frame->senttime;

	if (latency < 0 || latency > 1.0)
	{
//		Con_Printf ("Odd latency: %5.2f\n", latency);
	}
	else
	{
	// drift the average latency towards the observed latency
		if (latency < cls.latency)
			cls.latency = latency;
		else
			cls.latency += 0.001;	// drift up, so correction are needed
	}	
}

/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
void R_TranslatePlayerSkin(int playernum)
{
	player_info_t* player = &cl.players[playernum];
	if (!player->name[0])
	{
		return;
	}
	if (!player->playerclass)
	{
		return;
	}

	byte translate[256];
	CL_CalcHexen2SkinTranslation(player->topcolor, player->bottomcolor, player->playerclass, translate);

	//
	// locate the original skin pixels
	//
	if (player->modelindex <= 0)
	{
		return;
	}

	int classIndex;
	if (player->playerclass >= 1 && player->playerclass <= MAX_PLAYER_CLASS)
	{
		classIndex = (int)player->playerclass - 1;
		player->Translated = true;
	}
	else
	{
		classIndex = 0;
	}

	R_CreateOrUpdateTranslatedModelSkinH2(playertextures[playernum], va("*player%d", playernum), player_models[classIndex], translate, classIndex);
}

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation (int slot)
{
	if (slot > MAX_CLIENTS)
		Sys_Error ("CL_NewTranslation: slot > MAX_CLIENTS");

	R_TranslatePlayerSkin(slot);
}

/*
==============
CL_UpdateUserinfo
==============
*/
void CL_UpdateUserinfo (void)
{
	int		slot;
	player_info_t	*player;

	slot = net_message.ReadByte ();
	if (slot >= MAX_CLIENTS)
		Host_EndGame ("CL_ParseServerMessage: svc_updateuserinfo > MAX_SCOREBOARD");

	player = &cl.players[slot];
	player->userid = net_message.ReadLong ();
	String::NCpy(player->userinfo, net_message.ReadString2(), sizeof(player->userinfo)-1);

	String::NCpy(player->name, Info_ValueForKey (player->userinfo, "name"), sizeof(player->name)-1);
	player->topcolor = String::Atoi(Info_ValueForKey (player->userinfo, "topcolor"));
	player->bottomcolor = String::Atoi(Info_ValueForKey (player->userinfo, "bottomcolor"));
	if (Info_ValueForKey (player->userinfo, "*spectator")[0])
		player->spectator = true;
	else
		player->spectator = false;

	player->playerclass = String::Atoi(Info_ValueForKey (player->userinfo, "playerclass"));
/*	if (cl.playernum == slot && player->playerclass != playerclass.value)
	{
		Cvar_SetValue ("playerclass",player->playerclass);
	}
*/
	Sbar_Changed ();
	player->Translated = false;
	CL_NewTranslation (slot);
}


/*
=====================
CL_SetStat
=====================
*/
void CL_SetStat (int stat, int value)
{
	int	j;
	if (stat < 0 || stat >= MAX_CL_STATS)
		Sys_Error ("CL_SetStat: %i is invalid", stat);

	Sbar_Changed ();
	
	if (stat == STAT_ITEMS)
	{	// set flash times
		Sbar_Changed ();
		for (j=0 ; j<32 ; j++)
			if ( (value & (1<<j)) && !(cl.stats[stat] & (1<<j)))
				cl.item_gettime[j] = cl.time;
	}

	cl.stats[stat] = value;
}

/*
==============
CL_MuzzleFlash
==============
*/
void CL_MuzzleFlash (void)
{
	vec3_t		fv, rv, uv;
	cdlight_t	*dl;
	int			i;
	player_state_t	*pl;

	i = net_message.ReadShort ();

	if ((unsigned)(i-1) >= MAX_CLIENTS)
		return;

	pl = &cl.frames[parsecountmod].playerstate[i-1];

	dl = CL_AllocDlight (i);
	VectorCopy (pl->origin,  dl->origin);
	AngleVectors (pl->viewangles, fv, rv, uv);
		
	VectorMA (dl->origin, 18, fv, dl->origin);
	dl->radius = 200 + (rand()&31);
	dl->minlight = 32;
	dl->die = cl.time + 0.1;
	dl->color[0] = 0.2;
	dl->color[1] = 0.1;
	dl->color[2] = 0.05;
	dl->color[3] = 0.7;
}

void CL_Plaque(void)
{
	int index;

	index = net_message.ReadShort ();

	if (index > 0 && index <= pr_string_count)
		plaquemessage = &pr_global_strings[pr_string_index[index-1]];
	else
		plaquemessage = "";
}

void CL_IndexedPrint(void)
{
	int index,i;

	i = net_message.ReadByte ();
	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
		con_ormask = 1;
	}

	index = net_message.ReadShort ();

	if (index > 0 && index <= pr_string_count)
	{
		Con_Printf ("%s",&pr_global_strings[pr_string_index[index-1]]);
	}
	else
	{
		Con_Printf ("");
	}
	con_ormask = 0;
}

void CL_NamePrint(void)
{
	int index,i;

	i = net_message.ReadByte ();
	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
		con_ormask = 1;
	}

	index = net_message.ReadByte ();

	if (index >= 0 && index < MAX_CLIENTS)
	{
		Con_Printf ("%s",&cl.players[index].name);
	}
	else
	{
		Con_Printf ("");
	}
	con_ormask = 0;
}

void CL_ParticleExplosion(void)
{
	vec3_t org;
	short color, radius, counter;

	org[0] = net_message.ReadCoord();
	org[1] = net_message.ReadCoord();
	org[2] = net_message.ReadCoord();
	color = net_message.ReadShort();
	radius = net_message.ReadShort();
	counter = net_message.ReadShort();

	R_ColoredParticleExplosion(org,color,radius,counter);
}


#define SHOWNET(x) if(cl_shownet->value==2)Con_Printf ("%3i:%s\n", net_message.readcount-1, x);
/*
=====================
CL_ParseServerMessage
=====================
*/
int	received_framecount;
int LastServerMessageSize = 0;

qboolean cl_siege;
byte cl_fraglimit;
float cl_timelimit;
float cl_server_time_offset;
unsigned int	defLosses;	// Defenders losses in Siege
unsigned int	attLosses;	// Attackers Losses in Siege
void CL_ParseServerMessage (void)
{
	int			cmd;
	char		*s;
	int			i, j;
	unsigned	sc1, sc2;
	byte		test;
	char		temp[100];
	vec3_t		pos;

	LastServerMessageSize += net_message.cursize;

	received_framecount = host_framecount;
	cl.last_servermessage = realtime;
	CL_ClearProjectiles ();
	CL_ClearMissiles ();
	v_targDist = 0; // This clears out the target field on each netupdate; it won't be drawn unless another update comes...

//
// if recording demos, copy the message out
//
	if (cl_shownet->value == 1)
		Con_Printf ("%i ",net_message.cursize);
	else if (cl_shownet->value == 2)
		Con_Printf ("------------------\n");


	CL_ParseClientdata ();

//
// parse the message
//
	while (1)
	{
		if (net_message.badread)
		{
			Host_EndGame ("CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = net_message.ReadByte ();

		if (cmd == -1)
		{
			net_message.readcount++;	// so the EOM showner has the right value
			SHOWNET("END OF MESSAGE");
			break;
		}

		SHOWNET(svc_strings[cmd]);
	
	// other commands
		switch (cmd)
		{
		default:
			Host_EndGame ("CL_ParseServerMessage: Illegible server message\n");
			break;
			
		case svc_nop:
//			Con_Printf ("svc_nop\n");
			break;
			
		case svc_disconnect:
			Host_EndGame ("Server disconnected\n");
			break;

		case svc_print:
			i = net_message.ReadByte ();
			if (i == PRINT_CHAT)
			{
				S_StartLocalSound("misc/talk.wav");
				con_ormask = 1;
			}
			else if (i >= PRINT_SOUND)
			{
				if (talksounds->value)
				{
					sprintf(temp,"taunt/taunt%.3d.wav",i-PRINT_SOUND+1);
					S_StartLocalSound(temp);
					con_ormask = 1;
				}
				else
				{
					net_message.ReadString2();
					break;
				}
			}
			Con_Printf ("%s", net_message.ReadString2 ());
			con_ormask = 0;
			break;
			
		case svc_centerprint:
			SCR_CenterPrint (const_cast<char*>(net_message.ReadString2()));
			break;
			
		case svc_stufftext:
			s = const_cast<char*>(net_message.ReadString2());
			Con_DPrintf ("stufftext: %s\n", s);
			Cbuf_AddText (s);
			break;
			
		case svc_damage:
			V_ParseDamage ();
			break;
			
		case svc_serverdata:
			Cbuf_Execute ();		// make sure any stuffed commands are done
			CL_ParseServerData ();
			break;
			
		case svc_setangle:
			for (i=0 ; i<3 ; i++)
				cl.viewangles[i] = net_message.ReadAngle ();
//			cl.viewangles[PITCH] = cl.viewangles[ROLL] = 0;
			break;
			
		case svc_lightstyle:
			i = net_message.ReadByte ();
			if (i >= MAX_LIGHTSTYLES_Q1)
				Sys_Error ("svc_lightstyle > MAX_LIGHTSTYLES_Q1");
			String::Cpy(cl_lightstyle[i].map,  net_message.ReadString2());
			cl_lightstyle[i].length = String::Length(cl_lightstyle[i].map);
			break;
			
		case svc_sound:
			CL_ParseStartSoundPacket();
			break;
			
		case svc_sound_update_pos:
		{//FIXME: put a field on the entity that lists the channels
			//it should update when it moves- if a certain flag
			//is on the ent, this update_channels field could
			//be set automatically by each sound and stopSound
			//called for this ent?
			vec3_t  pos;
			int 	channel, ent;
			
			channel = net_message.ReadShort ();
			
			ent = channel >> 3;
			channel &= 7;
			
			if (ent > MAX_EDICTS)
				Host_Error ("svc_sound_update_pos: ent = %i", ent);
			
			for (i=0 ; i<3 ; i++)
				pos[i] = net_message.ReadCoord ();
			
			S_UpdateSoundPos (ent, channel, pos);
		}
			break;

		case svc_stopsound:
			i = net_message.ReadShort();
			S_StopSound(i>>3, i&7);
			break;
		
		case svc_updatefrags:
			Sbar_Changed ();
			i = net_message.ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
			cl.players[i].frags = net_message.ReadShort ();
			break;			

		case svc_updateping:
			i = net_message.ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updateping > MAX_SCOREBOARD");
			cl.players[i].ping = net_message.ReadShort ();
			break;
			
		case svc_updateentertime:
		// time is sent over as seconds ago
			i = net_message.ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updateentertime > MAX_SCOREBOARD");
			cl.players[i].entertime = realtime - net_message.ReadFloat ();
			break;
			
		case svc_updatepclass:
		// playerclass has changed for this dude
			i = net_message.ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatepclass > MAX_SCOREBOARD");
			cl.players[i].playerclass = net_message.ReadByte ();
			cl.players[i].level = cl.players[i].playerclass&31;
			cl.players[i].playerclass = cl.players[i].playerclass>>5;
			break;

		case svc_updatedminfo:
		// This dude killed someone, update his frags and level
			i = net_message.ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatedminfo > MAX_SCOREBOARD");
			cl.players[i].frags = net_message.ReadShort ();
			cl.players[i].playerclass = net_message.ReadByte ();
			cl.players[i].level = cl.players[i].playerclass&31;
			cl.players[i].playerclass = cl.players[i].playerclass>>5;
			break;

		case svc_updatesiegelosses:
		// This dude killed someone, update his frags and level
			defLosses = net_message.ReadByte ();
			attLosses = net_message.ReadByte ();
			break;

		case svc_updatesiegeteam:
		// This dude killed someone, update his frags and level
			i = net_message.ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatesiegeteam > MAX_SCOREBOARD");
			cl.players[i].siege_team = net_message.ReadByte ();
			break;

		case svc_updatesiegeinfo:
		// This dude killed someone, update his frags and level
			cl_siege = true;
			cl_timelimit = net_message.ReadByte () * 60;
			cl_fraglimit = net_message.ReadByte ();
			break;

		case svc_haskey:
			cl_keyholder = net_message.ReadShort() - 1;
			break;

		case svc_isdoc:
			cl_doc = net_message.ReadShort() - 1;
			break;

		case svc_nonehaskey:
			cl_keyholder = -1;
			break;

		case svc_nodoc:
			cl_doc = -1;
			break;
		
		case svc_time:
			cl_server_time_offset = ((int)net_message.ReadFloat()) - cl.time;
			break;

		case svc_spawnbaseline:
			i = net_message.ReadShort ();
			CL_ParseBaseline (&cl_baselines[i]);
			break;
		case svc_spawnstatic:
			CL_ParseStatic ();
			break;			
		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_killedmonster:
			cl.stats[STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			cl.stats[STAT_SECRETS]++;
			break;

		case svc_updatestat:
			i = net_message.ReadByte ();
			j = net_message.ReadByte ();
			CL_SetStat (i, j);
			break;
		case svc_updatestatlong:
			i = net_message.ReadByte ();
			j = net_message.ReadLong ();
			CL_SetStat (i, j);
			break;
			
		case svc_spawnstaticsound:
			CL_ParseStaticSound ();
			break;

		case svc_cdtrack:
			cl.cdtrack = net_message.ReadByte ();
			CDAudio_Play ((byte)cl.cdtrack, true);
			break;

		case svc_intermission:
//			if(cl_siege)
//			{//MG
				cl.intermission = net_message.ReadByte();
				cl.completed_time = realtime;
				break;
/*			}
			else
			{//Old Quake way- won't work
				cl.intermission = 1;
				cl.completed_time = realtime;
				vid.recalc_refdef = true;	// go to full screen
				for (i=0 ; i<3 ; i++)
					cl.simorg[i] = net_message.ReadCoord ();			
				for (i=0 ; i<3 ; i++)
					cl.simangles[i] = net_message.ReadAngle ();
				VectorCopy (vec3_origin, cl.simvel);
				break;
			}
*/
		case svc_finale:
			cl.intermission = 2;
			cl.completed_time = realtime;
			SCR_CenterPrint (const_cast<char*>(net_message.ReadString2()));
			break;
			
		case svc_sellscreen:
			Cmd_ExecuteString ("help");
			break;

		case svc_smallkick:
			cl.punchangle = -2;
			break;
		case svc_bigkick:
			cl.punchangle = -4;
			break;

		case svc_muzzleflash:
			CL_MuzzleFlash ();
			break;

		case svc_updateuserinfo:
			CL_UpdateUserinfo ();
			break;

		case svc_download:
			CL_ParseDownload ();
			break;

		case svc_playerinfo:
			CL_ParsePlayerinfo ();
			break;

		case svc_playerskipped:
			CL_SavePlayer ();
			break;

		case svc_nails:
			CL_ParseProjectiles ();
			break;

		case svc_packmissile:
			CL_ParsePackMissiles ();
			break;

		case svc_chokecount:		// some preceding packets were choked
			i = net_message.ReadByte ();
			for (j=0 ; j<i ; j++)
				cl.frames[ (cls.netchan.incoming_acknowledged-1-j)&UPDATE_MASK ].receivedtime = -2;
			break;

		case svc_modellist:
			CL_ParseModellist ();
			break;

		case svc_soundlist:
			CL_ParseSoundlist ();
			break;

		case svc_packetentities:
			CL_ParsePacketEntities (false);
			break;

		case svc_deltapacketentities:
			CL_ParsePacketEntities (true);
			break;

		case svc_maxspeed :
			movevars.maxspeed = net_message.ReadFloat();
			break;

		case svc_entgravity :
			movevars.entgravity = net_message.ReadFloat();
			break;

		case svc_plaque:
			CL_Plaque();
			break;

		case svc_indexed_print:
			CL_IndexedPrint();
			break;

		case svc_name_print:
			CL_NamePrint();
			break;

		case svc_particle_explosion:
			CL_ParticleExplosion();
			break;

		case svc_set_view_tint:
			i = net_message.ReadByte();
//				cl.viewent.colorshade = i;
			break;

		case svc_start_effect:
			CL_ParseEffect();
			break;

		case svc_end_effect:
			CL_EndEffect();
			break;

		case svc_set_view_flags:
			cl.viewent.drawflags |= net_message.ReadByte();
			break;

		case svc_clear_view_flags:
			cl.viewent.drawflags &= ~net_message.ReadByte();
			break;

		case svc_update_inv:
			sc1 = sc2 = 0;

			test = net_message.ReadByte();
			if (test & 1)
				sc1 |= ((int)net_message.ReadByte());
			if (test & 2)
				sc1 |= ((int)net_message.ReadByte())<<8;
			if (test & 4)
				sc1 |= ((int)net_message.ReadByte())<<16;
			if (test & 8)
				sc1 |= ((int)net_message.ReadByte())<<24;
			if (test & 16)
				sc2 |= ((int)net_message.ReadByte());
			if (test & 32)
				sc2 |= ((int)net_message.ReadByte())<<8;
			if (test & 64)
				sc2 |= ((int)net_message.ReadByte())<<16;
			if (test & 128)
				sc2 |= ((int)net_message.ReadByte())<<24;

			if (sc1 & SC1_HEALTH)
				cl.v.health = net_message.ReadShort();
			if (sc1 & SC1_LEVEL)
				cl.v.level = net_message.ReadByte();
			if (sc1 & SC1_INTELLIGENCE)
				cl.v.intelligence = net_message.ReadByte();
			if (sc1 & SC1_WISDOM)
				cl.v.wisdom = net_message.ReadByte();
			if (sc1 & SC1_STRENGTH)
				cl.v.strength = net_message.ReadByte();
			if (sc1 & SC1_DEXTERITY)
				cl.v.dexterity = net_message.ReadByte();
//				if (sc1 & SC1_WEAPON)
//					cl.v.weapon = net_message.ReadByte();
			if (sc1 & SC1_TELEPORT_TIME)
			{
//				Con_Printf("Teleport_time>time, got bit\n");
				cl.v.teleport_time = realtime + 2;//can't airmove for 2 seconds
			}

			if (sc1 & SC1_BLUEMANA)
				cl.v.bluemana = net_message.ReadByte();
			if (sc1 & SC1_GREENMANA)
				cl.v.greenmana = net_message.ReadByte();
			if (sc1 & SC1_EXPERIENCE)
				cl.v.experience = net_message.ReadLong();
			if (sc1 & SC1_CNT_TORCH)
				cl.v.cnt_torch = net_message.ReadByte();
			if (sc1 & SC1_CNT_H_BOOST)
				cl.v.cnt_h_boost = net_message.ReadByte();
			if (sc1 & SC1_CNT_SH_BOOST)
				cl.v.cnt_sh_boost = net_message.ReadByte();
			if (sc1 & SC1_CNT_MANA_BOOST)
				cl.v.cnt_mana_boost = net_message.ReadByte();
			if (sc1 & SC1_CNT_TELEPORT)
				cl.v.cnt_teleport = net_message.ReadByte();
			if (sc1 & SC1_CNT_TOME)
				cl.v.cnt_tome = net_message.ReadByte();
			if (sc1 & SC1_CNT_SUMMON)
				cl.v.cnt_summon = net_message.ReadByte();
			if (sc1 & SC1_CNT_INVISIBILITY)
				cl.v.cnt_invisibility = net_message.ReadByte();
			if (sc1 & SC1_CNT_GLYPH)
				cl.v.cnt_glyph = net_message.ReadByte();
			if (sc1 & SC1_CNT_HASTE)
				cl.v.cnt_haste = net_message.ReadByte();
			if (sc1 & SC1_CNT_BLAST)
				cl.v.cnt_blast = net_message.ReadByte();
			if (sc1 & SC1_CNT_POLYMORPH)
				cl.v.cnt_polymorph = net_message.ReadByte();
			if (sc1 & SC1_CNT_FLIGHT)
				cl.v.cnt_flight = net_message.ReadByte();
			if (sc1 & SC1_CNT_CUBEOFFORCE)
				cl.v.cnt_cubeofforce = net_message.ReadByte();
			if (sc1 & SC1_CNT_INVINCIBILITY)
				cl.v.cnt_invincibility = net_message.ReadByte();
			if (sc1 & SC1_ARTIFACT_ACTIVE)
				cl.v.artifact_active = net_message.ReadByte();
			if (sc1 & SC1_ARTIFACT_LOW)
				cl.v.artifact_low = net_message.ReadByte();
			if (sc1 & SC1_MOVETYPE)
				cl.v.movetype = net_message.ReadByte();
			if (sc1 & SC1_CAMERAMODE)
				cl.v.cameramode = net_message.ReadByte();
			if (sc1 & SC1_HASTED)
				cl.v.hasted = net_message.ReadFloat();
			if (sc1 & SC1_INVENTORY)
				cl.v.inventory = net_message.ReadByte();
			if (sc1 & SC1_RINGS_ACTIVE)
				cl.v.rings_active = net_message.ReadByte();

			if (sc2 & SC2_RINGS_LOW)
				cl.v.rings_low = net_message.ReadByte();
			if (sc2 & SC2_AMULET)
				cl.v.armor_amulet = net_message.ReadByte();
			if (sc2 & SC2_BRACER)
				cl.v.armor_bracer = net_message.ReadByte();
			if (sc2 & SC2_BREASTPLATE)
				cl.v.armor_breastplate = net_message.ReadByte();
			if (sc2 & SC2_HELMET)
				cl.v.armor_helmet = net_message.ReadByte();
			if (sc2 & SC2_FLIGHT_T)
				cl.v.ring_flight = net_message.ReadByte();
			if (sc2 & SC2_WATER_T)
				cl.v.ring_water = net_message.ReadByte();
			if (sc2 & SC2_TURNING_T)
				cl.v.ring_turning = net_message.ReadByte();
			if (sc2 & SC2_REGEN_T)
				cl.v.ring_regeneration = net_message.ReadByte();
//				if (sc2 & SC2_HASTE_T)
//					cl.v.haste_time = net_message.ReadFloat();
//				if (sc2 & SC2_TOME_T)
//					cl.v.tome_time = net_message.ReadFloat();
			if (sc2 & SC2_PUZZLE1)
				sprintf(cl.puzzle_pieces[0], "%.9s", net_message.ReadString2());
			if (sc2 & SC2_PUZZLE2)
				sprintf(cl.puzzle_pieces[1], "%.9s", net_message.ReadString2());
			if (sc2 & SC2_PUZZLE3)
				sprintf(cl.puzzle_pieces[2], "%.9s", net_message.ReadString2());
			if (sc2 & SC2_PUZZLE4)
				sprintf(cl.puzzle_pieces[3], "%.9s", net_message.ReadString2());
			if (sc2 & SC2_PUZZLE5)
				sprintf(cl.puzzle_pieces[4], "%.9s", net_message.ReadString2());
			if (sc2 & SC2_PUZZLE6)
				sprintf(cl.puzzle_pieces[5], "%.9s", net_message.ReadString2());
			if (sc2 & SC2_PUZZLE7)
				sprintf(cl.puzzle_pieces[6], "%.9s", net_message.ReadString2());
			if (sc2 & SC2_PUZZLE8)
				sprintf(cl.puzzle_pieces[7], "%.9s", net_message.ReadString2());
			if (sc2 & SC2_MAXHEALTH)
				cl.v.max_health = net_message.ReadShort();
			if (sc2 & SC2_MAXMANA)
				cl.v.max_mana = net_message.ReadByte();
			if (sc2 & SC2_FLAGS)
				cl.v.flags = net_message.ReadFloat();

			if ((sc1 & SC1_INV) || (sc2 & SC2_INV))
				SB_InvChanged();
			break;

		case svc_particle:
			R_ParseParticleEffect ();
			break;
		case svc_particle2:
			R_ParseParticleEffect2 ();
			break;
		case svc_particle3:
			R_ParseParticleEffect3 ();
			break;
		case svc_particle4:
			R_ParseParticleEffect4 ();
			break;
		case svc_turn_effect:
			CL_TurnEffect ();
			break;
		case svc_update_effect:
			CL_ReviseEffect();
			break;

		case svc_multieffect:
			CL_ParseMultiEffect();
			break;

		case svc_midi_name:
			String::Cpy(cl.midi_name,net_message.ReadString2 ());
			if (String::ICmp(bgmtype->string,"midi") == 0)
				MIDI_Play(cl.midi_name);
			else 
				MIDI_Stop();
			break;

		case svc_raineffect:
			R_ParseRainEffect();
			break;

		case svc_targetupdate:
			V_ParseTarget();
			break;

		case svc_update_piv:
			cl.PIV = net_message.ReadLong();
			break;

		case svc_player_sound:
			test = net_message.ReadByte();
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();
			i = net_message.ReadShort ();
			S_StartSound(pos, test, 1, cl.sound_precache[i], 1.0, 1.0);
			break;
		}
	}

	CL_SetSolidEntities ();
}


