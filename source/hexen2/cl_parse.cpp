// cl_parse.c  -- parse a message received from the server

/*
 * $Header: /H2 Mission Pack/Cl_parse.c 25    3/20/98 3:45p Jmonroe $
 */

#include "quakedef.h"
#include "glquake.h"
#ifdef _WIN32
#include <windows.h>
#endif

extern	QCvar*	sv_flypitch;
extern	QCvar*	sv_walkpitch;

qhandle_t	player_models[NUM_CLASSES];

char *svc_strings[] =
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
	
	"svc_serverinfo",		// [long] version
						// [string] signon string
						// [string]..[0]model cache [string]...[0]sounds cache
						// [string]..[0]item cache
	"svc_lightstyle",		// [byte] [string]
	"svc_updatename",		// [byte] [string]
	"svc_updatefrags",	// [byte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",	// [byte] [byte]
	"svc_particle",		// [vec3] <variable>
	"svc_damage",			// [byte] impact [byte] blood [vec3] from
	
	"svc_spawnstatic",
	"svc_raineffect",
	"svc_spawnbaseline",
	
	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",			// [string] music [string] text
	"svc_cdtrack",			// [byte] track [byte] looptrack
	"svc_sellscreen",
	"svc_particle2",		// [vec3] <variable>
	"svc_cutscene",
	"svc_midi_name",
	"svc_updateclass",  // [byte] client [byte] class
	"svc_particle3",
	"svc_particle4",
	"svc_set_view_flags",
	"svc_clear_view_flags",
	"svc_start_effect",
	"svc_end_effect",
	"svc_plaque",
	"svc_particle_explosion",
	"svc_set_view_tint",
	"svc_reference",
	"svc_clear_edicts",
	"svc_update_inv",
	"svc_setangle_interpolate",
	"svc_update_kingofhill",
	"svc_toggle_statbar"
};

char *puzzle_strings;
int LastServerMessageSize;
extern QCvar* precache;

//=============================================================================

/*
===============
CL_EntityNum

This error checks and tracks the total number of entities
===============
*/
entity_t	*CL_EntityNum (int num)
{
	if (num >= cl.num_entities)
	{
		if (num >= MAX_EDICTS)
			Host_Error ("CL_EntityNum: %i is an invalid number",num);
		while (cl.num_entities<=num)
		{
			cl_entities[cl.num_entities].colormap = vid.colormap;
			cl.num_entities++;
		}
	}
		
	return &cl_entities[num];
}


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
    int 	field_mask;
    float 	attenuation;  
 	int		i;
	           
    field_mask = net_message.ReadByte(); 

    if (field_mask & SND_VOLUME)
		volume = net_message.ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
    if (field_mask & SND_ATTENUATION)
		attenuation = net_message.ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	
	channel = net_message.ReadShort ();
	sound_num = net_message.ReadByte ();
    if (field_mask & SND_OVERFLOW)
		sound_num += 255;

	ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS)
		Host_Error ("CL_ParseStartSoundPacket: ent = %i", ent);
	
	for (i=0 ; i<3 ; i++)
		pos[i] = net_message.ReadCoord ();
 
    S_StartSound(pos, ent, channel, cl.sound_precache[sound_num], volume / 255.0, attenuation);
}       

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/
void CL_KeepaliveMessage (void)
{
	float	time;
	static float lastmsg;
	int		ret;
	QMsg		old;
	byte		olddata[NET_MAXMESSAGE];
	
	if (sv.active)
		return;		// no need if server is local
	if (cls.demoplayback)
		return;

// read messages from server, should just be nops
	old.Copy(olddata, sizeof(olddata), net_message);
	
	do
	{
		ret = CL_GetMessage ();
		switch (ret)
		{
		default:
			Host_Error ("CL_KeepaliveMessage: CL_GetMessage failed");		
		case 0:
			break;	// nothing waiting
		case 1:
			Host_Error ("CL_KeepaliveMessage: received a message");
			break;
		case 2:
			if (net_message.ReadByte() != svc_nop)
				Host_Error ("CL_KeepaliveMessage: datagram wasn't a nop");
			break;
		}
	} while (ret);

	net_message.Copy(net_message._data, net_message.maxsize, old);

// check time
	time = Sys_DoubleTime ();
	if (time - lastmsg < 5)
		return;
	lastmsg = time;

// write out a nop
	Con_Printf ("--> client to server keepalive\n");

	cls.message.WriteByte(clc_nop);
	NET_SendMessage (cls.netcon, &cls.message);
	cls.message.Clear();
}

/*
==================
CL_ParseServerInfo
==================
*/
void CL_ParseServerInfo (void)
{
	char	*str;
	int		i;
	int		nummodels, numsounds;
	char	model_precache[MAX_MODELS][MAX_QPATH];
	char	sound_precache[MAX_SOUNDS][MAX_QPATH];
// rjr	edict_t		*ent;
	
	Con_DPrintf ("Serverinfo packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();

// parse protocol version number
	i = net_message.ReadLong ();
	if (i != PROTOCOL_VERSION)
	{
		Con_Printf ("Server returned version %i, not %i", i, PROTOCOL_VERSION);
		return;
	}

// parse maxclients
	cl.maxclients = net_message.ReadByte ();
	if (cl.maxclients < 1 || cl.maxclients > MAX_SCOREBOARD)
	{
		Con_Printf("Bad maxclients (%u) from server\n", cl.maxclients);
		return;
	}
	cl.scores = (scoreboard_t*)Hunk_AllocName (cl.maxclients*sizeof(*cl.scores), "scores");

// parse gametype
	cl.gametype = net_message.ReadByte ();
	
	if (cl.gametype == GAME_DEATHMATCH)
		sv_kingofhill = net_message.ReadShort ();

// parse signon message
	str = const_cast<char*>(net_message.ReadString2());
	QStr::NCpy(cl.levelname, str, sizeof(cl.levelname)-1);

// seperate the printfs so the server message can have a color
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf ("%c%s\n", 2, str);

//
// first we go through and touch all of the precache data that still
// happens to be in the cache, so precaching something else doesn't
// needlessly purge it
//

// precache models
	Com_Memset(cl.model_precache, 0, sizeof(cl.model_precache));
	for (nummodels=1 ; ; nummodels++)
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
			break;
		if (nummodels==MAX_MODELS)
		{
			Con_Printf ("Server sent too many model precaches\n");
			return;
		}
		QStr::Cpy(model_precache[nummodels], str);
		Mod_TouchModel (str);
	}

// precache sounds
	Com_Memset(cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds=1 ; ; numsounds++)
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
			break;
		if (numsounds==MAX_SOUNDS)
		{
			Con_Printf ("Server sent too many sound precaches\n");
			return;
		}
		QStr::Cpy(sound_precache[numsounds], str);
	}

//
// now we try to load everything else until a cache allocation fails
//

	total_loading_size = nummodels + numsounds;
	current_loading_size = 1;
	loading_stage = 2;

	//always precache the world!!!
	cl.model_precache[1] = Mod_ForName (model_precache[1], false);
	for (i=2 ; i<nummodels ; i++)
	{
		cl.model_precache[i] = Mod_ForName (model_precache[i], false);
		current_loading_size++;
		D_ShowLoadingSize();

		if (cl.model_precache[i] == 0)
		{
			Con_Printf("Model %s not found\n", model_precache[i]);
			return;
		}
		CL_KeepaliveMessage ();
	}

	player_models[0] = Mod_ForName("models/paladin.mdl", false);
	player_models[1] = Mod_ForName("models/crusader.mdl", false);
	player_models[2] = Mod_ForName("models/necro.mdl", false);
	player_models[3] = Mod_ForName("models/assassin.mdl", false);
	player_models[4] = Mod_ForName("models/succubus.mdl", false);

	S_BeginRegistration();
	for (i=1 ; i<numsounds ; i++)
	{
		cl.sound_precache[i] = S_RegisterSound(sound_precache[i]);
		current_loading_size++;
		D_ShowLoadingSize();

		CL_KeepaliveMessage ();
	}
	S_EndRegistration();

	total_loading_size = 0;
	loading_stage = 0;


// local state
	cl_entities[0].model = cl.model_precache[1];
	cl.worldmodel = cl.model_precache[1];
	CM_LoadMap(model_precache[1], true, NULL);
	
	R_NewMap ();

	puzzle_strings = (char *)COM_LoadHunkFile ("puzzles.txt");

	Hunk_Check ();		// make sure nothing is hurt
	
	noclip_anglehack = false;		// noclip is turned off at start	
}


/*
==================
CL_ParseUpdate

Parse an entity update message from the server
If an entities model or origin changes from frame to frame, it must be
relinked.  Other attributes can change without relinking.
==================
*/

void CL_ParseUpdate (int bits)
{
	int			i;
	float		f;
	qhandle_t	model;
	int			modnum;
	qboolean	forcelink;
	entity_t	*ent;
	int			num;
	entity_state2_t *ref_ent,*set_ent,build_ent,dummy;

	if (cls.signon == SIGNONS - 1)
	{	// first update is the final signon stage
		cls.signon = SIGNONS;
		CL_SignonReply ();
	}

	if (bits & U_MOREBITS)
	{
		i = net_message.ReadByte ();
		bits |= (i<<8);
	}

	if (bits & U_MOREBITS2)
	{
		i = net_message.ReadByte ();
		bits |= (i<<16);
	}

	if (bits & U_LONGENTITY)	
		num = net_message.ReadShort ();
	else
		num = net_message.ReadByte ();
	ent = CL_EntityNum (num);

	ent->baseline.flags |= BE_ON;

	ref_ent = NULL;

	for(i=0;i<cl.frames[0].count;i++)
		if (cl.frames[0].states[i].index == num)
		{
			ref_ent = &cl.frames[0].states[i];
//			if (num == 2) fprintf(FH,"Found Reference\n");
			break;
		}

	if (!ref_ent)
	{
		ref_ent = &build_ent;

		build_ent.index = num;
		build_ent.origin[0] = ent->baseline.origin[0];
		build_ent.origin[1] = ent->baseline.origin[1];
		build_ent.origin[2] = ent->baseline.origin[2];
		build_ent.angles[0] = ent->baseline.angles[0];
		build_ent.angles[1] = ent->baseline.angles[1];
		build_ent.angles[2] = ent->baseline.angles[2];
		build_ent.modelindex = ent->baseline.modelindex;
		build_ent.frame = ent->baseline.frame;
		build_ent.colormap = ent->baseline.colormap;
		build_ent.skin = ent->baseline.skin;
		build_ent.effects = ent->baseline.effects;
		build_ent.scale = ent->baseline.scale;
		build_ent.drawflags = ent->baseline.drawflags;
		build_ent.abslight = ent->baseline.abslight;
	}

	if (cl.need_build)
	{	// new sequence, first valid frame
		set_ent = &cl.frames[1].states[cl.frames[1].count];
		cl.frames[1].count++;
	}
	else
		set_ent = &dummy;

	if (bits & U_CLEAR_ENT)
	{
		Com_Memset(ent, 0, sizeof(entity_t));
		Com_Memset(ref_ent, 0, sizeof(*ref_ent));
		ref_ent->index = num;
	}

	*set_ent = *ref_ent;

	if (ent->msgtime != cl.mtime[1])
		forcelink = true;	// no previous frame to lerp from
	else
		forcelink = false;

	ent->msgtime = cl.mtime[0];
	
	if (bits & U_MODEL)
	{
		modnum = net_message.ReadShort ();
		if (modnum >= MAX_MODELS)
			Host_Error ("CL_ParseModel: bad modnum");
	}
	else
		modnum = ref_ent->modelindex;
		
	model = cl.model_precache[modnum];
	set_ent->modelindex = modnum;
	if (model != ent->model)
	{
		ent->model = model;

	// automatic animation (torches, etc) can be either all together
	// or randomized
		if (model)
		{
			if (Mod_GetSyncType(model) == ST_RAND)
				ent->syncbase = rand()*(1.0/RAND_MAX);//(float)(rand()&0x7fff) / 0x7fff;
			else
				ent->syncbase = 0.0;
		}
		else
			forcelink = true;	// hack to make null model players work
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin (num - 1);
	}
	
	if (bits & U_FRAME)
		set_ent->frame = ent->frame = net_message.ReadByte ();
	else
		ent->frame = ref_ent->frame;

	if (bits & U_COLORMAP)
		set_ent->colormap = i = net_message.ReadByte();
	else
		i = ref_ent->colormap;

	if (num && num <= cl.maxclients)
		ent->colormap = ent->sourcecolormap = cl.scores[num-1].translations;
	else
		ent->sourcecolormap = vid.colormap;

//	ent->colormap = vid.colormap;

	if (!i)
	{
		ent->colorshade = i;
		ent->colormap = ent->sourcecolormap;
	}
	else
	{
		ent->colorshade = i;
//		ent->colormap = vid.colormap;
		ent->colormap = globalcolormap;
	}

	if(bits & U_SKIN)
	{
		set_ent->skin = ent->skinnum = net_message.ReadByte();
		set_ent->drawflags = ent->drawflags = net_message.ReadByte();
	}
	else
	{
		ent->skinnum = ref_ent->skin;
		ent->drawflags = ref_ent->drawflags;
	}

	if (bits & U_EFFECTS)
	{
		set_ent->effects = ent->effects = net_message.ReadByte();
//		if (num == 2) fprintf(FH,"Read effects %d\n",set_ent->effects);
	}
	else
	{
		ent->effects = ref_ent->effects;
		//if (num == 2) fprintf(FH,"restored effects %d\n",ref_ent->effects);
	}

// shift the known values for interpolation
	VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);

	if (bits & U_ORIGIN1)
	{
		set_ent->origin[0] = ent->msg_origins[0][0] = net_message.ReadCoord ();
		//if (num == 2) fprintf(FH,"Read origin[0] %f\n",set_ent->angles[0]);
	}
	else
	{
		ent->msg_origins[0][0] = ref_ent->origin[0];
		//if (num == 2) fprintf(FH,"Restored origin[0] %f\n",ref_ent->angles[0]);
	}
	if (bits & U_ANGLE1)
		set_ent->angles[0] = ent->msg_angles[0][0] = net_message.ReadAngle();
	else
		ent->msg_angles[0][0] = ref_ent->angles[0];

	if (bits & U_ORIGIN2)
		set_ent->origin[1] = ent->msg_origins[0][1] = net_message.ReadCoord ();
	else
		ent->msg_origins[0][1] = ref_ent->origin[1];
	if (bits & U_ANGLE2)
		set_ent->angles[1] = ent->msg_angles[0][1] = net_message.ReadAngle();
	else
		ent->msg_angles[0][1] = ref_ent->angles[1];

	if (bits & U_ORIGIN3)
		set_ent->origin[2] = ent->msg_origins[0][2] = net_message.ReadCoord ();
	else
		ent->msg_origins[0][2] = ref_ent->origin[2];
	if (bits & U_ANGLE3)
		set_ent->angles[2] = ent->msg_angles[0][2] = net_message.ReadAngle();
	else
		ent->msg_angles[0][2] = ref_ent->angles[2];

	if(bits&U_SCALE)
	{
		set_ent->scale = ent->scale = net_message.ReadByte();
		set_ent->abslight = ent->abslight = net_message.ReadByte();
	}
	else
	{
		ent->scale = ref_ent->scale;
		ent->abslight = ref_ent->abslight;
	}

	if ( bits & U_NOLERP )
		ent->forcelink = true;

	if ( forcelink )
	{	// didn't have an update last message
		VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
		VectorCopy (ent->msg_origins[0], ent->origin);
		VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);
		VectorCopy (ent->msg_angles[0], ent->angles);
		ent->forcelink = true;
	}


//	if (sv.active || num != 2)
//		return;
}

void CL_ParseUpdate2 (int bits)
{
	int			i;

	if (bits & U_MOREBITS)
	{
		i = net_message.ReadByte ();
		bits |= (i<<8);
	}

	if (bits & U_MOREBITS2)
	{
		i = net_message.ReadByte ();
		bits |= (i<<16);
	}

	if (bits & U_LONGENTITY)	
		net_message.ReadShort ();
	else
		net_message.ReadByte ();
	
	if (bits & U_MODEL)
	{
		net_message.ReadShort ();
	}
		
	if (bits & U_FRAME)
		net_message.ReadByte ();

	if (bits & U_COLORMAP)
		net_message.ReadByte();

	if(bits & U_SKIN)
	{
		net_message.ReadByte();
		net_message.ReadByte();
	}

	if (bits & U_EFFECTS)
		net_message.ReadByte();

	if (bits & U_ORIGIN1)
		net_message.ReadCoord ();
	if (bits & U_ANGLE1)
		net_message.ReadAngle();

	if (bits & U_ORIGIN2)
		net_message.ReadCoord ();
	if (bits & U_ANGLE2)
		net_message.ReadAngle();

	if (bits & U_ORIGIN3)
		net_message.ReadCoord ();
	if (bits & U_ANGLE3)
		net_message.ReadAngle();

	if(bits&U_SCALE)
	{
		net_message.ReadByte();
		net_message.ReadByte();
	}
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (entity_t *ent)
{
	int			i;
	
	ent->baseline.modelindex = net_message.ReadShort ();
	ent->baseline.frame = net_message.ReadByte ();
	ent->baseline.colormap = net_message.ReadByte();
	ent->baseline.skin = net_message.ReadByte();
	ent->baseline.scale = net_message.ReadByte();
	ent->baseline.drawflags = net_message.ReadByte();
	ent->baseline.abslight = net_message.ReadByte();
	for (i=0 ; i<3 ; i++)
	{
		ent->baseline.origin[i] = net_message.ReadCoord ();
		ent->baseline.angles[i] = net_message.ReadAngle ();
	}
}


/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
void CL_ParseClientdata (int bits)
{
	int		i, j;
	int max_order;
	

	if (bits & SU_VIEWHEIGHT)
		cl.viewheight = net_message.ReadChar ();
//rjr	else
//rjr		cl.viewheight = DEFAULT_VIEWHEIGHT;

	if (bits & SU_IDEALPITCH)
		cl.idealpitch = net_message.ReadChar ();
	else
	{
		//rjr   is sv_flypitch   useable on the client's end?
//rjr		if (cl.v.movetype==MOVETYPE_FLY)
//rjr			cl.idealpitch = sv_flypitch.value;
//rjr		else
//rjr			cl.idealpitch = sv_walkpitch.value;
	}

	if (bits & SU_IDEALROLL)
		cl.idealroll = net_message.ReadChar ();	
//rjr	else
//rjr		cl.idealroll = 0;
	
	VectorCopy (cl.mvelocity[0], cl.mvelocity[1]);
	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i) )
			cl.punchangle[i] = net_message.ReadChar();
//rjr		else
//rjr			cl.punchangle[i] = 0;
		if (bits & (SU_VELOCITY1<<i) )
			cl.mvelocity[0][i] = net_message.ReadChar()*16;
//rjr		else
//rjr			cl.mvelocity[0][i] = 0;
	}

/*	if (bits & SU_ITEMS)
		i = net_message.ReadLong ();
*/

	if (cl.items != i)
	{	// set flash times
		SB_Changed();
		for (j=0 ; j<32 ; j++)
			if ( (i & (1<<j)) && !(cl.items & (1<<j)))
				cl.item_gettime[j] = cl.time;
		cl.items = i;
	}
		
	cl.onground = (bits & SU_ONGROUND) != 0;
	cl.inwater = (bits & SU_INWATER) != 0;

	if (bits & SU_WEAPONFRAME)
		cl.stats[STAT_WEAPONFRAME] = net_message.ReadByte ();
//rjr	else
//rjr		cl.stats[STAT_WEAPONFRAME] = 0;

	if (bits & SU_ARMOR)
	{
		cl.stats[STAT_ARMOR] = net_message.ReadByte ();
		SB_Changed();
	}

	if (bits & SU_WEAPON)
	{
		cl.stats[STAT_WEAPON] = net_message.ReadShort ();
		SB_Changed();
	}

/*	sc1 = sc2 = 0;

	if (bits & SU_SC1)
		sc1 = net_message.ReadLong ();
	if (bits & SU_SC2)
		sc2 = net_message.ReadLong ();

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
	if (sc1 & SC1_WEAPON)
		cl.v.weapon = net_message.ReadByte();
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
		cl.v.artifact_active = net_message.ReadFloat();
	if (sc1 & SC1_ARTIFACT_LOW)
		cl.v.artifact_low = net_message.ReadFloat();
	if (sc1 & SC1_MOVETYPE)
		cl.v.movetype = net_message.ReadByte();
	if (sc1 & SC1_CAMERAMODE)
		cl.v.cameramode = net_message.ReadByte();
	if (sc1 & SC1_HASTED)
		cl.v.hasted = net_message.ReadFloat();
	if (sc1 & SC1_INVENTORY)
		cl.v.inventory = net_message.ReadByte();
	if (sc1 & SC1_RINGS_ACTIVE)
		cl.v.rings_active = net_message.ReadFloat();

	if (sc2 & SC2_RINGS_LOW)
		cl.v.rings_low = net_message.ReadFloat();
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
	if (sc2 & SC2_HASTE_T)
		cl.v.haste_time = net_message.ReadFloat();
	if (sc2 & SC2_TOME_T)
		cl.v.tome_time = net_message.ReadFloat();
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

	if ((sc1 & SC1_STAT_BAR) || (sc2 & SC2_STAT_BAR))
		SB_Changed();

	if ((sc1 & SC1_INV) || (sc2 & SC2_INV))
		SB_InvChanged();*/
}

int color_offsets[NUM_CLASSES] =
{
	2*14*256,
	0,
	1*14*256,
	2*14*256,
	2*14*256
};

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation (int slot)
{
	int		i, j;
	int		top, bottom;
	byte	*dest, *source, *sourceA, *sourceB, *colorA, *colorB;
	
	if (slot > cl.maxclients)
		Sys_Error ("CL_NewTranslation: slot > cl.maxclients");
	if (!cl.scores[slot].playerclass)
		return;

	R_TranslatePlayerSkin (slot);
}

/*
=====================
CL_ParseStatic
=====================
*/
void CL_ParseStatic (void)
{
	entity_t *ent;
	int		i;
		
	i = cl.num_statics;
	if (i >= MAX_STATIC_ENTITIES)
		Host_Error ("Too many static entities");
	ent = &cl_static_entities[i];
	cl.num_statics++;
	CL_ParseBaseline (ent);

// copy it to the current state
	ent->model = cl.model_precache[ent->baseline.modelindex];
	ent->frame = ent->baseline.frame;
	ent->colormap = vid.colormap;
	ent->skinnum = ent->baseline.skin;
	ent->scale = ent->baseline.scale;
	ent->effects = ent->baseline.effects;
	ent->drawflags = ent->baseline.drawflags;
	ent->abslight = ent->baseline.abslight;

	VectorCopy (ent->baseline.origin, ent->origin);
	VectorCopy (ent->baseline.angles, ent->angles);	
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

	sound_num = net_message.ReadShort ();
	vol = net_message.ReadByte ();
	atten = net_message.ReadByte ();
	
	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
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

void CL_ParseRainEffect(void)
{
	vec3_t		org, e_size;
	short		color,count;
	int			x_dir, y_dir;

	org[0] = net_message.ReadCoord();
	org[1] = net_message.ReadCoord();
	org[2] = net_message.ReadCoord();
	e_size[0] = net_message.ReadCoord();
	e_size[1] = net_message.ReadCoord();
	e_size[2] = net_message.ReadCoord();
	x_dir = net_message.ReadAngle();
	y_dir = net_message.ReadAngle();
	color = net_message.ReadShort();
	count = net_message.ReadShort();

	R_RainEffect(org,e_size,x_dir,y_dir,color,count);
}

#define SHOWNET(x) if(cl_shownet->value==2)Con_Printf ("%3i:%s\n", net_message.readcount-1, x);

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage (void)
{
	int			cmd;
	int			i,j,k;
	int			EntityCount = 0;
	int			EntitySize = 0;
	int			before;
	static		double lasttime;
	static		qboolean packet_loss = false;
	entity_t	*ent;
	short		RemovePlace, OrigPlace, NewPlace, AddedIndex;
	int sc1, sc2;
	byte test;
	float x,y,z,dx,dy,dz;
	float		compangles[2][3];
	vec3_t		deltaangles;
	
//
// if recording demos, copy the message out
//
	if(net_message.cursize > LastServerMessageSize)
	{
		LastServerMessageSize = net_message.cursize;
	}
	if (cl_shownet->value == 1)
	{
		Con_Printf ("Time: %2.2f Pck: %i ",host_time-lasttime,net_message.cursize);
		lasttime = host_time;
	}
	else if (cl_shownet->value == 2)
		Con_Printf ("------------------\n");
	
	cl.onground = false;	// unless the server says otherwise	
//
// parse the message
//
	net_message.BeginReadingOOB();
	
	while (1)
	{
		if (net_message.badread)
			Host_Error ("CL_ParseServerMessage: Bad server message");

		cmd = net_message.ReadByte ();

		if (cmd == -1)
		{
			if (cl_shownet->value == 1)
				Con_Printf ("Ent: %i (%i bytes)",EntityCount,EntitySize);

			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

	// if the high bit of the command byte is set, it is a fast update
		if (cmd & 128)
		{
			before = net_message.readcount;
			SHOWNET("fast update");
			if (packet_loss)
				CL_ParseUpdate2 (cmd&127);
			else
				CL_ParseUpdate (cmd&127);

			EntityCount++;
			EntitySize += net_message.readcount - before + 1;
			continue;
		}

		SHOWNET(svc_strings[cmd]);
	
	// other commands
		switch (cmd)
		{
		default:
			Host_Error ("CL_ParseServerMessage: Illegible server message\n");
			break;
			
		case svc_nop:
//			Con_Printf ("svc_nop\n");
			break;
			
		case svc_time:
			cl.mtime[1] = cl.mtime[0];
			cl.mtime[0] = net_message.ReadFloat ();			
			break;
			
		case svc_clientdata:
			i = net_message.ReadShort ();
			CL_ParseClientdata (i);
			break;
		
		case svc_version:
			i = net_message.ReadLong ();
			if (i != PROTOCOL_VERSION)
				Host_Error ("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, PROTOCOL_VERSION);
			break;
			
		case svc_disconnect:
			Host_EndGame ("Server disconnected\n");

		case svc_print:
			if(intro_playing)
				net_message.ReadString2 ();
			else
				Con_Printf ("%s", net_message.ReadString2 ());
			break;
			
		case svc_centerprint:
			//Bottom_Plaque_Draw(net_message.ReadString2(),true);
			SCR_CenterPrint (const_cast<char*>(net_message.ReadString2()));
			break;
			
		case svc_stufftext:
			Cbuf_AddText (const_cast<char*>(net_message.ReadString2()));
			break;
			
		case svc_damage:
			V_ParseDamage ();
			break;
			
		case svc_serverinfo:
			CL_ParseServerInfo ();
			vid.recalc_refdef = true;	// leave intermission full screen
			break;
			
		case svc_setangle:
			for (i=0 ; i<3 ; i++)
				cl.viewangles[i] = net_message.ReadAngle ();

			break;

		case svc_setangle_interpolate:
			
			compangles[0][0] = net_message.ReadAngle();
			compangles[0][1] = net_message.ReadAngle();
			compangles[0][2] = net_message.ReadAngle();

			for (i=0 ; i<3 ; i++)
			{
				compangles[1][i] = cl.viewangles[i];
				for (j=0 ; j<2 ; j++)
				{//standardize both old and new angles to +-180
					if(compangles[j][i]>=360)
						compangles[j][i] -= 360*((int)(compangles[j][i]/360));
					else if(compangles[j][i]<=360)
						compangles[j][i] += 360*(1+(int)(-compangles[j][i]/360));
					if(compangles[j][i]>180)
						compangles[j][i]=-360 + compangles[j][i];
					else if(compangles[j][i]<-180)
						compangles[j][i]=360 + compangles[j][i];
				}
				//get delta
				deltaangles[i] = compangles[0][i] - compangles[1][i];
					//cap delta to <=180,>=-180
				if(deltaangles[i]>180)
					deltaangles[i]+=-360;
				else if(deltaangles[i]<-180)
					deltaangles[i]+=360;
				//add the delta
				cl.viewangles[i]+=(deltaangles[i]/8);//8 step interpolation
				//cap newangles to +-180
				if(cl.viewangles[i]>=360)
					cl.viewangles[i] -= 360*((int)(cl.viewangles[i]/360));
				else if(cl.viewangles[i]<=360)
					cl.viewangles[i] += 360*(1+(int)(-cl.viewangles[i]/360));
				if(cl.viewangles[i]>180)
					cl.viewangles[i]+=-360;
				else if(cl.viewangles[i]<-180)
					cl.viewangles[i]+=360;
			}
			break;
			
		case svc_setview:
			cl.viewentity = net_message.ReadShort ();
			break;
					
		case svc_lightstyle:
			i = net_message.ReadByte ();
			if (i >= MAX_LIGHTSTYLES)
				Sys_Error ("svc_lightstyle > MAX_LIGHTSTYLES");
			QStr::Cpy(cl_lightstyle[i].map,  net_message.ReadString2());
			cl_lightstyle[i].length = QStr::Length(cl_lightstyle[i].map);
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
		
		case svc_updatename:
			SB_Changed();
			i = net_message.ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatename > MAX_SCOREBOARD");
			QStr::Cpy(cl.scores[i].name, net_message.ReadString2 ());
			break;

		case svc_updateclass:
			SB_Changed();
			i = net_message.ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updateclass > MAX_SCOREBOARD");
			cl.scores[i].playerclass = (float)net_message.ReadByte();
			CL_NewTranslation(i); // update the color
			break;
		
		case svc_updatefrags:
			SB_Changed();
			i = net_message.ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
			cl.scores[i].frags = net_message.ReadShort ();
			break;			

		case svc_update_kingofhill:
			sv_kingofhill = net_message.ReadShort() - 1;
			break;

		case svc_updatecolors:
			SB_Changed();
			i = net_message.ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatecolors > MAX_SCOREBOARD");
			cl.scores[i].colors = net_message.ReadByte ();
			CL_NewTranslation (i);
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

		case svc_spawnbaseline:
			i = net_message.ReadShort ();
			// must use CL_EntityNum() to force cl.num_entities up
			CL_ParseBaseline (CL_EntityNum(i));
			break;
		case svc_spawnstatic:
			CL_ParseStatic ();
			break;			

		case svc_raineffect:
			CL_ParseRainEffect();
			break;

		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_setpause:
			{
				cl.paused = net_message.ReadByte ();

				if (cl.paused)
				{
					CDAudio_Pause ();
				}
				else
				{
					CDAudio_Resume ();
				}
			}
			break;
			
		case svc_signonnum:
			i = net_message.ReadByte ();
			if (i <= cls.signon)
				Host_Error ("Received signon %i when at %i", i, cls.signon);
			cls.signon = i;
			CL_SignonReply ();
			break;

		case svc_killedmonster:
			cl.stats[STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			cl.stats[STAT_SECRETS]++;
			break;

		case svc_updatestat:
			i = net_message.ReadByte ();
			if (i < 0 || i >= MAX_CL_STATS)
				Sys_Error ("svc_updatestat: %i is invalid", i);
			cl.stats[i] = net_message.ReadLong ();;
			break;
			
		case svc_spawnstaticsound:
			CL_ParseStaticSound ();
			break;

		case svc_cdtrack:
			cl.cdtrack = net_message.ReadByte ();
			cl.looptrack = net_message.ReadByte ();
			if (QStr::ICmp(bgmtype->string,"cd") == 0)
			{
				if ( (cls.demoplayback || cls.demorecording) && (cls.forcetrack != -1) )
					CDAudio_Play ((byte)cls.forcetrack, true);
				else
					CDAudio_Play ((byte)cl.cdtrack, true);
			}
			else
			   CDAudio_Stop();
			break;

		case svc_midi_name:
			QStr::Cpy(cl.midi_name,net_message.ReadString2 ());
			if (QStr::ICmp(bgmtype->string,"midi") == 0)
				MIDI_Play(cl.midi_name);
			else 
				MIDI_Stop();
			break;

		case svc_toggle_statbar:
			break;

		case svc_intermission:
			cl.intermission = net_message.ReadByte();
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			break;

/*		case svc_finale:
			cl.intermission = 2;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			SCR_CenterPrint (net_message.ReadString2 ());			
			break;

		case svc_cutscene:
			cl.intermission = 3;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			SCR_CenterPrint (net_message.ReadString2 ());			
			break;

		case svc_sellscreen:
			Cmd_ExecuteString ("help", src_command);
			break;*/

			case svc_set_view_flags:
				cl.viewent.drawflags |= net_message.ReadByte();
				break;

			case svc_clear_view_flags:
				cl.viewent.drawflags &= ~net_message.ReadByte();
				break;

			case svc_start_effect:
				CL_ParseEffect();
				break;
			case svc_end_effect:
				CL_EndEffect();
				break;

			case svc_plaque:
				CL_Plaque();
				break;

			case svc_particle_explosion:
				CL_ParticleExplosion();
				break;

			case svc_set_view_tint:
				i = net_message.ReadByte();
				cl.viewent.colorshade = i;
				break;

			case svc_reference:
				packet_loss = false;
				cl.last_frame = cl.current_frame;
				cl.last_sequence = cl.current_sequence;
				cl.current_frame = net_message.ReadByte();
				cl.current_sequence = net_message.ReadByte();
				if (cl.need_build == 2)
				{
//					Con_Printf("CL: NB2 CL(%d,%d) R(%d)\n", cl.current_sequence, cl.current_frame,cl.reference_frame);
					cl.frames[0].count = cl.frames[1].count = cl.frames[2].count = 0;
					cl.need_build = 1;
					cl.reference_frame = cl.current_frame;
				}
				else if (cl.last_sequence != cl.current_sequence)
				{
//					Con_Printf("CL: Sequence CL(%d,%d) R(%d)\n", cl.current_sequence, cl.current_frame,cl.reference_frame);
					if (cl.reference_frame >= 1 && cl.reference_frame <= MAX_FRAMES)
					{
						RemovePlace = OrigPlace = NewPlace = AddedIndex = 0;
						for(i=0;i<cl.num_entities;i++)
						{
							if (RemovePlace >= cl.NumToRemove || cl.RemoveList[RemovePlace] != i)
							{
								if (NewPlace < cl.frames[1].count &&
									cl.frames[1].states[NewPlace].index == i)
								{
									cl.frames[2].states[AddedIndex] = cl.frames[1].states[NewPlace];
									AddedIndex++;
									cl.frames[2].count++;
								}
								else if (OrigPlace < cl.frames[0].count &&
									     cl.frames[0].states[OrigPlace].index == i)
								{
									cl.frames[2].states[AddedIndex] = cl.frames[0].states[OrigPlace];
									AddedIndex++;
									cl.frames[2].count++;
								}
							}
							else
								RemovePlace++;

							if (cl.frames[0].states[OrigPlace].index == i)
								OrigPlace++;
							if (cl.frames[1].states[NewPlace].index == i)
								NewPlace++;
						}
						cl.frames[0] = cl.frames[2];
					}
					cl.frames[1].count = cl.frames[2].count = 0;
					cl.need_build = 1;
					cl.reference_frame = cl.current_frame;
				}
				else
				{
//					Con_Printf("CL: Normal CL(%d,%d) R(%d)\n", cl.current_sequence, cl.current_frame,cl.reference_frame);
					cl.need_build = 0;
				}

				for (i=1,ent=cl_entities+1 ; i<cl.num_entities ; i++,ent++)
				{
					ent->baseline.flags &= ~BE_ON;
				}

				for(i=0;i<cl.frames[0].count;i++)
				{
					ent = CL_EntityNum (cl.frames[0].states[i].index);
					ent->model = cl.model_precache[cl.frames[0].states[i].modelindex];
					ent->baseline.flags |= BE_ON;
				}
				break;

			case svc_clear_edicts:
				j = net_message.ReadByte();
				if (cl.need_build)
				{
					cl.NumToRemove = j;
				}
				for(i=0;i<j;i++)
				{
					k = net_message.ReadShort();
					if (cl.need_build)
						cl.RemoveList[i] = k;
					ent = CL_EntityNum (k);
					ent->baseline.flags &= ~BE_ON;
				}
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
				if (sc1 & SC1_WEAPON)
					cl.v.weapon = net_message.ReadByte();
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
					cl.v.artifact_active = net_message.ReadFloat();
				if (sc1 & SC1_ARTIFACT_LOW)
					cl.v.artifact_low = net_message.ReadFloat();
				if (sc1 & SC1_MOVETYPE)
					cl.v.movetype = net_message.ReadByte();
				if (sc1 & SC1_CAMERAMODE)
					cl.v.cameramode = net_message.ReadByte();
				if (sc1 & SC1_HASTED)
					cl.v.hasted = net_message.ReadFloat();
				if (sc1 & SC1_INVENTORY)
					cl.v.inventory = net_message.ReadByte();
				if (sc1 & SC1_RINGS_ACTIVE)
					cl.v.rings_active = net_message.ReadFloat();

				if (sc2 & SC2_RINGS_LOW)
					cl.v.rings_low = net_message.ReadFloat();
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
				if (sc2 & SC2_HASTE_T)
					cl.v.haste_time = net_message.ReadFloat();
				if (sc2 & SC2_TOME_T)
					cl.v.tome_time = net_message.ReadFloat();
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
				if (sc2 & SC2_OBJ)
					cl.info_mask = net_message.ReadLong();
				if (sc2 & SC2_OBJ2)
					cl.info_mask2 = net_message.ReadLong();

				if ((sc1 & SC1_STAT_BAR) || (sc2 & SC2_STAT_BAR))
					SB_Changed();

				if ((sc1 & SC1_INV) || (sc2 & SC2_INV))
					SB_InvChanged();
				break;
		}
	}

}
