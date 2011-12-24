/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
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

	"svc_setinfo",
	"svc_serverinfo",
	"svc_updatepl",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL"
};

int	oldparsecountmod;
int	parsecountmod;
double	parsecounttime;

int		cl_spikeindex, cl_playerindex, cl_flagindex;

image_t*	playertextures[MAX_CLIENTS];		// up to 16 color translated skins

//=============================================================================

int packet_latency[NET_TIMINGS];

int CL_CalcNet (void)
{
	int		a, i;
	frame_t	*frame;
	int lost;

	for (i=cls.netchan.outgoingSequence-UPDATE_BACKUP+1
		; i <= cls.netchan.outgoingSequence
		; i++)
	{
		frame = &cl.frames[i&UPDATE_MASK];
		if (frame->receivedtime == -1)
			packet_latency[i&NET_TIMINGSMASK] = 9999;	// dropped
		else if (frame->receivedtime == -2)
			packet_latency[i&NET_TIMINGSMASK] = 10000;	// choked
		else if (frame->invalid)
			packet_latency[i&NET_TIMINGSMASK] = 9998;	// invalid delta
		else
			packet_latency[i&NET_TIMINGSMASK] = (frame->receivedtime - frame->senttime)*20;
	}

	lost = 0;
	for (a=0 ; a<NET_TIMINGS ; a++)
	{
		i = (cls.netchan.outgoingSequence-a) & NET_TIMINGSMASK;
		if (packet_latency[i] == 9999)
			lost++;
	}
	return lost * 100 / NET_TIMINGS;
}

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
	cls.netchan.message.WriteString2(va("download %s", cls.downloadname));

	cls.downloadnumber++;

	return false;
}

static void CL_CalcModelChecksum(const char* ModelName, const char* CVarName)
{
	Array<byte> Buffer;
	if (!FS_ReadFile(ModelName, Buffer))
	{
		throw DropException(va("Couldn't load %s", ModelName));
	}

	unsigned short crc;
	CRC_Init(&crc);
	for (int i = 0; i < Buffer.Num(); i++)
	{
		CRC_ProcessByte(&crc, Buffer[i]);
	}

	char st[40];
	sprintf(st, "%d", (int)crc);
	Info_SetValueForKey(cls.userinfo, CVarName, st, MAX_INFO_STRING, 64, 64, true, false);

	cls.netchan.message.WriteByte(clc_stringcmd);
	sprintf(st, "setinfo %s %d", CVarName, (int)crc);
	cls.netchan.message.WriteString2(st);
}

/*
===============
R_NewMap
===============
*/
static void R_NewMap (void)
{
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

	CL_CalcModelChecksum("progs/player.mdl", pmodel_name);
	CL_CalcModelChecksum("progs/eyes.mdl", emodel_name);

	// all done
	R_NewMap ();
	Hunk_Check ();		// make sure nothing is hurt

	int CheckSum1;
	int CheckSum2;
	CM_MapChecksums(CheckSum1, CheckSum2);

	// done with modellist, request first of static signon messages
	cls.netchan.message.WriteByte(clc_stringcmd);
//	cls.netchan.message.WriteString2(va("prespawn %i 0 %i", cl.servercount, cl.worldmodel->checksum2));
	cls.netchan.message.WriteString2(va(prespawn_name, cl.servercount, CheckSum2));
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
	Com_Memset(cl.model_precache, 0, sizeof(cl.model_precache));
	cl_playerindex = -1;
	cl_spikeindex = -1;
	cl_flagindex = -1;
	cls.netchan.message.WriteByte(clc_stringcmd);
//	cls.netchan.message.WriteString2(va("modellist %i 0", cl.servercount));
	cls.netchan.message.WriteString2(va(modellist_name, cl.servercount, 0));
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
	case dl_skin:
		Skin_NextDownload ();
		break;
	case dl_model:
		Model_NextDownload ();
		break;
	case dl_sound:
		Sound_NextDownload ();
		break;
	case dl_none:
	default:
		Con_DPrintf("Unknown download type.\n");
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
	char	name[1024];

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
			FS_FCloseFile(cls.download);
			cls.download = 0;
		}
		CL_RequestNextDownload();
		return;
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		if (String::NCmp(cls.downloadtempname, "skins/", 6))
		{
			cls.download = FS_FOpenFileWrite(cls.downloadtempname);
		}
		else
		{
			sprintf(name, "qw/%s", cls.downloadtempname);
			cls.download = FS_SV_FOpenFileWrite(name);
		}

		if (!cls.download)
		{
			net_message.readcount += size;
			Con_Printf ("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload ();
			return;
		}
	}

	FS_Write(net_message._data + net_message.readcount, size, cls.download);
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
		char	oldn[MAX_OSPATH];
		char	newn[MAX_OSPATH];

#if 0
		Con_Printf ("100%%\n");
#endif

		FS_FCloseFile(cls.download);

		// rename the temp file to it's final name
		if (String::Cmp(cls.downloadtempname, cls.downloadname))
		{
			if (String::NCmp(cls.downloadtempname,"skins/",6))
			{
				FS_Rename(cls.downloadtempname, cls.downloadname);
			}
			else
			{
				sprintf(oldn, "qw/%s", cls.downloadtempname);
				sprintf(newn, "qw/%s", cls.downloadname);
				FS_SV_Rename(oldn, newn);
			}
		}

		cls.download = 0;
		cls.downloadpercent = 0;

		// get another file if needed

		CL_RequestNextDownload ();
	}
}

static byte *upload_data;
static int upload_pos;
static int upload_size;

void CL_NextUpload(void)
{
	byte	buffer[1024];
	int		r;
	int		percent;
	int		size;

	if (!upload_data)
		return;

	r = upload_size - upload_pos;
	if (r > 768)
		r = 768;
	Com_Memcpy(buffer, upload_data + upload_pos, r);
	cls.netchan.message.WriteByte(clc_upload);
	cls.netchan.message.WriteShort(r);

	upload_pos += r;
	size = upload_size;
	if (!size)
		size = 1;
	percent = upload_pos*100/size;
	cls.netchan.message.WriteByte(percent);
	cls.netchan.message.WriteData(buffer, r);

Con_DPrintf ("UPLOAD: %6d: %d written\n", upload_pos - r, r);

	if (upload_pos != upload_size)
		return;

	Con_Printf ("Upload completed\n");

	free(upload_data);
	upload_data = 0;
	upload_pos = upload_size = 0;
}

void CL_StartUpload (byte *data, int size)
{
	if (cls.state < ca_onserver)
		return; // gotta be connected

	// override
	if (upload_data)
		free(upload_data);

Con_DPrintf("Upload starting of %d...\n", size);

	upload_data = (byte*)malloc(size);
	Com_Memcpy(upload_data, data, size);
	upload_size = size;
	upload_pos = 0;

	CL_NextUpload();
} 

qboolean CL_IsUploading(void)
{
	if (upload_data)
		return true;
	return false;
}

void CL_StopUpload(void)
{
	if (upload_data)
		free(upload_data);
	upload_data = NULL;
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
// allow 2.2 and 2.29 demos to play
	protover = net_message.ReadLong ();
	if (protover != PROTOCOL_VERSION && 
		!(cls.demoplayback && (protover == 26 || protover == 27 || protover == 28)))
		Host_EndGame ("Server returned version %i, not %i\nYou probably need to upgrade.\nCheck http://www.quakeworld.net/", protover, PROTOCOL_VERSION);

	cl.servercount = net_message.ReadLong ();

	// game directory
	str = const_cast<char*>(net_message.ReadString2());

	if (String::ICmp(gamedirfile, str)) {
		// save current config
		Host_WriteConfiguration (); 
		cflag = true;
	}

	COM_Gamedir(str);

	//ZOID--run the autoexec.cfg in the gamedir
	//if it exists
	if (cflag)
	{
		if (FS_FileExists("config.cfg"))
		{
			Cbuf_AddText ("cl_warncmd 0\n");
			Cbuf_AddText("exec config.cfg\n");
			Cbuf_AddText("exec frontend.cfg\n");
			Cbuf_AddText ("cl_warncmd 1\n");
		}
	}

	// parse player slot, high bit means spectator
	cl.playernum = net_message.ReadByte ();
	if (cl.playernum & 128)
	{
		cl.spectator = true;
		cl.playernum &= ~128;
	}
	cl.viewentity = cl.playernum + 1;

	// get the full level name
	str = const_cast<char*>(net_message.ReadString2());
	String::NCpy(cl.levelname, str, sizeof(cl.levelname)-1);

	// get the movevars
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

	// seperate the printfs so the server message can have a color
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf ("%c%s\n", 2, str);

	// ask for the sound list next
	Com_Memset(cl.sound_name, 0, sizeof(cl.sound_name));
	cls.netchan.message.WriteByte(clc_stringcmd);
//	cls.netchan.message.WriteString2(va("soundlist %i 0", cl.servercount));
	cls.netchan.message.WriteString2(va(soundlist_name, cl.servercount, 0));

	// now waiting for downloads, etc
	cls.state = ca_onserver;
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
	int n;

// precache sounds
//	Com_Memset(cl.sound_precache, 0, sizeof(cl.sound_precache));

	numsounds = net_message.ReadByte();

	for (;;) {
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
			break;
		numsounds++;
		if (numsounds == MAX_SOUNDS)
			Host_EndGame ("Server sent too many sound_precache");
		String::Cpy(cl.sound_name[numsounds], str);
	}

	n = net_message.ReadByte();

	if (n) {
		cls.netchan.message.WriteByte(clc_stringcmd);
//		cls.netchan.message.WriteString2(va("soundlist %i %i", cl.servercount, n));
		cls.netchan.message.WriteString2(va(soundlist_name, cl.servercount, n));
		return;
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
	int n;

// precache models and note certain default indexes
	nummodels = net_message.ReadByte();

	for (;;)
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
			break;
		nummodels++;
		if (nummodels==MAX_MODELS)
			Host_EndGame ("Server sent too many model_precache");
		String::Cpy(cl.model_name[nummodels], str);

		if (!String::Cmp(cl.model_name[nummodels],"progs/spike.mdl"))
			cl_spikeindex = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"progs/player.mdl"))
			cl_playerindex = nummodels;
		if (!String::Cmp(cl.model_name[nummodels],"progs/flag.mdl"))
			cl_flagindex = nummodels;
	}

	n = net_message.ReadByte();

	if (n) {
		cls.netchan.message.WriteByte(clc_stringcmd);
//		cls.netchan.message.WriteString2(va("modellist %i %i", cl.servercount, n));
		cls.netchan.message.WriteString2(va(modellist_name, cl.servercount, n));
		return;
	}

	cls.downloadnumber = 0;
	cls.downloadtype = dl_model;
	Model_NextDownload ();
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (q1entity_state_t *es)
{
	int			i;
	
	es->modelindex = net_message.ReadByte ();
	es->frame = net_message.ReadByte ();
	es->colormap = net_message.ReadByte();
	es->skinnum = net_message.ReadByte();
	for (i=0 ; i<3 ; i++)
	{
		es->origin[i] = net_message.ReadCoord();
		es->angles[i] = net_message.ReadAngle();
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
	q1entity_state_t	es;

	CL_ParseBaseline (&es);
		
	i = cl.num_statics;
	if (i >= MAX_STATIC_ENTITIES)
		Host_EndGame ("Too many static entities");
	ent = &cl_static_entities[i];
	cl.num_statics++;

// copy it to the current state
	ent->state.modelindex = es.modelindex;
	ent->state.frame = es.frame;
	ent->state.skinnum = es.skinnum;

	VectorCopy (es.origin, ent->state.origin);
	VectorCopy (es.angles, ent->state.angles);
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
		attenuation = net_message.ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	
	sound_num = net_message.ReadByte ();

	for (i=0 ; i<3 ; i++)
		pos[i] = net_message.ReadCoord ();
 
	ent = (channel>>3)&1023;
	channel &= 7;

	if (ent > MAX_EDICTS_Q1)
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

	i = cls.netchan.incomingAcknowledged;
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
		return;

	char s[512];
	String::Cpy(s, Info_ValueForKey(player->userinfo, "skin"));
	String::StripExtension(s, s);
	if (player->skin && !String::ICmp(s, player->skin->name))
	{
		player->skin = NULL;
	}

	if (player->_topcolor == player->topcolor &&
		player->_bottomcolor == player->bottomcolor && player->skin)
	{
		return;
	}

	player->_topcolor = player->topcolor;
	player->_bottomcolor = player->bottomcolor;

	byte translate[256];
	CL_CalcQuakeSkinTranslation(player->topcolor, player->bottomcolor, translate);

	//
	// locate the original skin pixels
	//

	if (!player->skin)
	{
		Skin_Find(player);
	}
	byte* original = Skin_Cache(player->skin);
	if (original != NULL)
	{
		//skin data width
		R_CreateOrUpdateTranslatedSkin(playertextures[playernum], va("*player%d", playernum), original, translate, 320, 200);
	}
	else
	{
		R_CreateOrUpdateTranslatedModelSkinQ1(playertextures[playernum], va("*player%d", playernum),
			cl.model_precache[cl_playerindex], translate);
	}
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
void CL_ProcessUserInfo (int slot, player_info_t *player)
{
	String::NCpy(player->name, Info_ValueForKey (player->userinfo, "name"), sizeof(player->name)-1);
	player->topcolor = String::Atoi(Info_ValueForKey (player->userinfo, "topcolor"));
	player->bottomcolor = String::Atoi(Info_ValueForKey (player->userinfo, "bottomcolor"));
	if (Info_ValueForKey (player->userinfo, "*spectator")[0])
		player->spectator = true;
	else
		player->spectator = false;

	if (cls.state == ca_active)
		Skin_Find (player);

	CL_NewTranslation (slot);
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
		Host_EndGame ("CL_ParseServerMessage: svc_updateuserinfo > MAX_CLIENTS");

	player = &cl.players[slot];
	player->userid = net_message.ReadLong ();
	String::NCpy(player->userinfo, net_message.ReadString2(), sizeof(player->userinfo)-1);

	CL_ProcessUserInfo (slot, player);
}

/*
==============
CL_SetInfo
==============
*/
void CL_SetInfo (void)
{
	int		slot;
	player_info_t	*player;
	char key[MAX_MSGLEN_QW];
	char value[MAX_MSGLEN_QW];

	slot = net_message.ReadByte ();
	if (slot >= MAX_CLIENTS)
		Host_EndGame ("CL_ParseServerMessage: svc_setinfo > MAX_CLIENTS");

	player = &cl.players[slot];

	String::NCpy(key, net_message.ReadString2(), sizeof(key) - 1);
	key[sizeof(key) - 1] = 0;
	String::NCpy(value, net_message.ReadString2(), sizeof(value) - 1);
	key[sizeof(value) - 1] = 0;

	Con_DPrintf("SETINFO %s: %s=%s\n", player->name, key, value);

	if (key[0] != '*')
	{
		Info_SetValueForKey(player->userinfo, key, value, MAX_INFO_STRING, 64, 64,
			String::ICmp(key, "name") != 0, String::ICmp(key, "team") == 0);
	}

	CL_ProcessUserInfo (slot, player);
}

/*
==============
CL_ServerInfo
==============
*/
void CL_ServerInfo (void)
{
	char key[MAX_MSGLEN_QW];
	char value[MAX_MSGLEN_QW];

	String::NCpy(key, net_message.ReadString2(), sizeof(key) - 1);
	key[sizeof(key) - 1] = 0;
	String::NCpy(value, net_message.ReadString2(), sizeof(value) - 1);
	key[sizeof(value) - 1] = 0;

	Con_DPrintf("SERVERINFO: %s=%s\n", key, value);

	if (key[0] != '*')
	{
		Info_SetValueForKey(cl.serverinfo, key, value, MAX_SERVERINFO_STRING, 64, 64,
			String::ICmp(key, "name") != 0, String::ICmp(key, "team") == 0);
	}
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

	if (stat == STAT_ITEMS)
	{	// set flash times
		for (j=0 ; j<32 ; j++)
			if ( (value & (1<<j)) && !(cl.stats[stat] & (1<<j)))
				cl.item_gettime[j] = cl.serverTimeFloat;
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
	int i = net_message.ReadShort();

	if ((unsigned)(i - 1) >= MAX_CLIENTS)
	{
		return;
	}
	player_state_t* pl = &cl.frames[parsecountmod].playerstate[i - 1];
	CLQ1_MuzzleFlashLight(i, pl->origin, pl->viewangles);
}


#define SHOWNET(x) if(cl_shownet->value==2)Con_Printf ("%3i:%s\n", net_message.readcount-1, x);
/*
=====================
CL_ParseServerMessage
=====================
*/
int	received_framecount;
void CL_ParseServerMessage (void)
{
	int			cmd;
	char		*s;
	int			i, j;

	received_framecount = host_framecount;
	cl.last_servermessage = realtime;
	CL_ClearProjectiles ();

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
			Host_EndGame ("CL_ParseServerMessage: Illegible server message");
			break;
			
		case svc_nop:
//			Con_Printf ("svc_nop\n");
			break;
			
		case svc_disconnect:
			if (cls.state == ca_connected)
				Host_EndGame ("Server disconnected\n"
					"Server version may not be compatible");
			else
				Host_EndGame ("Server disconnected");
			break;

		case svc_print:
			i = net_message.ReadByte ();
			if (i == PRINT_CHAT)
			{
				S_StartLocalSound("misc/talk.wav");
				con_ormask = 128;
			}
			Con_Printf ("%s", net_message.ReadString2 ());
			con_ormask = 0;
			break;
			
		case svc_centerprint:
			SCR_CenterPrint(const_cast<char*>(net_message.ReadString2()));
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
				cl.viewangles[i] = net_message.ReadAngle();
//			cl.viewangles[PITCH] = cl.viewangles[ROLL] = 0;
			break;
			
		case svc_lightstyle:
			i = net_message.ReadByte();
			CL_SetLightStyle(i, net_message.ReadString2());
			break;
			
		case svc_sound:
			CL_ParseStartSoundPacket();
			break;
			
		case svc_stopsound:
			i = net_message.ReadShort();
			S_StopSound(i>>3, i&7);
			break;
		
		case svc_updatefrags:
			i = net_message.ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatefrags > MAX_CLIENTS");
			cl.players[i].frags = net_message.ReadShort ();
			break;			

		case svc_updateping:
			i = net_message.ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updateping > MAX_CLIENTS");
			cl.players[i].ping = net_message.ReadShort ();
			break;
			
		case svc_updatepl:
			i = net_message.ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatepl > MAX_CLIENTS");
			cl.players[i].pl = net_message.ReadByte ();
			break;
			
		case svc_updateentertime:
		// time is sent over as seconds ago
			i = net_message.ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updateentertime > MAX_CLIENTS");
			cl.players[i].entertime = realtime - net_message.ReadFloat ();
			break;
			
		case svc_spawnbaseline:
			i = net_message.ReadShort ();
			CL_ParseBaseline (&clq1_baselines[i]);
			break;
		case svc_spawnstatic:
			CL_ParseStatic ();
			break;			
		case svc_temp_entity:
			CLQW_ParseTEnt(net_message);
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
			cl.intermission = 1;
			cl.completed_time = realtime;
			for (i=0 ; i<3 ; i++)
				cl.simorg[i] = net_message.ReadCoord ();			
			for (i=0 ; i<3 ; i++)
				cl.simangles[i] = net_message.ReadAngle();
			VectorCopy (vec3_origin, cl.simvel);
			break;

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

		case svc_setinfo:
			CL_SetInfo ();
			break;

		case svc_serverinfo:
			CL_ServerInfo ();
			break;

		case svc_download:
			CL_ParseDownload ();
			break;

		case svc_playerinfo:
			CL_ParsePlayerinfo ();
			break;

		case svc_nails:
			CL_ParseProjectiles ();
			break;

		case svc_chokecount:		// some preceding packets were choked
			i = net_message.ReadByte ();
			for (j=0 ; j<i ; j++)
				cl.frames[ (cls.netchan.incomingAcknowledged-1-j)&UPDATE_MASK ].receivedtime = -2;
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

		case svc_setpause:
			cl.paused = net_message.ReadByte ();
			if (cl.paused)
				CDAudio_Pause ();
			else
				CDAudio_Resume ();
			break;

		}
	}

	CL_SetSolidEntities ();
}


