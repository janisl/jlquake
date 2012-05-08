// cl_parse.c  -- parse a message received from the server

/*
 * $Header: /H2 Mission Pack/Cl_parse.c 25    3/20/98 3:45p Jmonroe $
 */

#include "quakedef.h"
#include "../common/file_formats/spr.h"
#ifdef _WIN32
#include <windows.h>
#endif

extern Cvar* sv_flypitch;
extern Cvar* sv_walkpitch;

const char* svc_strings[] =
{
	"h2svc_bad",
	"h2svc_nop",
	"h2svc_disconnect",
	"h2svc_updatestat",
	"h2svc_version",		// [long] server version
	"h2svc_setview",		// [short] entity number
	"h2svc_sound",			// <see code>
	"h2svc_time",			// [float] server time
	"h2svc_print",			// [string] null terminated string
	"h2svc_stufftext",		// [string] stuffed into client's console buffer
	// the string should be \n terminated
	"h2svc_setangle",		// [vec3] set the view angle to this absolute value

	"h2svc_serverinfo",		// [long] version
	// [string] signon string
	// [string]..[0]model cache [string]...[0]sounds cache
	// [string]..[0]item cache
	"h2svc_lightstyle",		// [byte] [string]
	"h2svc_updatename",		// [byte] [string]
	"h2svc_updatefrags",	// [byte] [short]
	"h2svc_clientdata",		// <shortbits + data>
	"h2svc_stopsound",		// <see code>
	"h2svc_updatecolors",	// [byte] [byte]
	"h2svc_particle",		// [vec3] <variable>
	"h2svc_damage",			// [byte] impact [byte] blood [vec3] from

	"h2svc_spawnstatic",
	"h2svc_raineffect",
	"h2svc_spawnbaseline",

	"h2svc_temp_entity",		// <variable>
	"h2svc_setpause",
	"h2svc_signonnum",
	"h2svc_centerprint",
	"h2svc_killedmonster",
	"h2svc_foundsecret",
	"h2svc_spawnstaticsound",
	"h2svc_intermission",
	"h2svc_finale",			// [string] music [string] text
	"h2svc_cdtrack",			// [byte] track [byte] looptrack
	"h2svc_sellscreen",
	"h2svc_particle2",		// [vec3] <variable>
	"h2svc_cutscene",
	"h2svc_midi_name",
	"h2svc_updateclass",	// [byte] client [byte] class
	"h2svc_particle3",
	"h2svc_particle4",
	"h2svc_set_view_flags",
	"h2svc_clear_view_flags",
	"h2svc_start_effect",
	"h2svc_end_effect",
	"h2svc_plaque",
	"h2svc_particle_explosion",
	"h2svc_set_view_tint",
	"h2svc_reference",
	"h2svc_clear_edicts",
	"h2svc_update_inv",
	"h2svc_setangle_interpolate",
	"h2svc_update_kingofhill",
	"h2svc_toggle_statbar"
};

char* puzzle_strings;
int LastServerMessageSize;
extern Cvar* precache;

//=============================================================================

/*
===============
CL_EntityNum

This error checks and tracks the total number of entities
===============
*/
h2entity_t* CL_EntityNum(int num)
{
	if (num >= cl.qh_num_entities)
	{
		if (num >= MAX_EDICTS_H2)
		{
			Host_Error("CL_EntityNum: %i is an invalid number",num);
		}
		while (cl.qh_num_entities <= num)
		{
			cl.qh_num_entities++;
		}
	}

	return &h2cl_entities[num];
}


/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
	vec3_t pos;
	int channel, ent;
	int sound_num;
	int volume;
	int field_mask;
	float attenuation;
	int i;

	field_mask = net_message.ReadByte();

	if (field_mask & SND_VOLUME)
	{
		volume = net_message.ReadByte();
	}
	else
	{
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	}

	if (field_mask & SND_ATTENUATION)
	{
		attenuation = net_message.ReadByte() / 64.0;
	}
	else
	{
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	}

	channel = net_message.ReadShort();
	sound_num = net_message.ReadByte();
	if (field_mask & SND_OVERFLOW)
	{
		sound_num += 255;
	}

	ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS_H2)
	{
		Host_Error("CL_ParseStartSoundPacket: ent = %i", ent);
	}

	for (i = 0; i < 3; i++)
		pos[i] = net_message.ReadCoord();

	S_StartSound(pos, ent, channel, cl.sound_precache[sound_num], volume / 255.0, attenuation);
}

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/
void CL_KeepaliveMessage(void)
{
	float time;
	static float lastmsg;
	int ret;
	QMsg old;
	byte olddata[MAX_MSGLEN_H2];

	if (sv.active)
	{
		return;		// no need if server is local
	}
	if (clc.demoplaying)
	{
		return;
	}

// read messages from server, should just be nops
	old.Copy(olddata, sizeof(olddata), net_message);

	do
	{
		ret = CL_GetMessage();
		switch (ret)
		{
		default:
			Host_Error("CL_KeepaliveMessage: CL_GetMessage failed");
		case 0:
			break;	// nothing waiting
		case 1:
			Host_Error("CL_KeepaliveMessage: received a message");
			break;
		case 2:
			if (net_message.ReadByte() != h2svc_nop)
			{
				Host_Error("CL_KeepaliveMessage: datagram wasn't a nop");
			}
			break;
		}
	}
	while (ret);

	net_message.Copy(net_message._data, net_message.maxsize, old);

// check time
	time = Sys_DoubleTime();
	if (time - lastmsg < 5)
	{
		return;
	}
	lastmsg = time;

// write out a nop
	Con_Printf("--> client to server keepalive\n");

	clc.netchan.message.WriteByte(h2clc_nop);
	NET_SendMessage(cls.qh_netcon, &clc.netchan, &clc.netchan.message);
	clc.netchan.message.Clear();
}

/*
===============
R_NewMap
===============
*/
static void R_NewMap(void)
{
	CL_ClearParticles();

	R_EndRegistration();
}

/*
==================
CL_ParseServerInfo
==================
*/
void CL_ParseServerInfo(void)
{
	char* str;
	int i;
	int nummodels, numsounds;
	char model_precache[MAX_MODELS_H2][MAX_QPATH];
	char sound_precache[MAX_SOUNDS_H2][MAX_QPATH];
// rjr	qhedict_t		*ent;

	Con_DPrintf("Serverinfo packet received.\n");
//
// wipe the clientActive_t struct
//
	CL_ClearState();

// parse protocol version number
	i = net_message.ReadLong();
	if (i != PROTOCOL_VERSION)
	{
		Con_Printf("Server returned version %i, not %i", i, PROTOCOL_VERSION);
		return;
	}

// parse maxclients
	cl.qh_maxclients = net_message.ReadByte();
	if (cl.qh_maxclients < 1 || cl.qh_maxclients > H2MAX_CLIENTS)
	{
		Con_Printf("Bad maxclients (%u) from server\n", cl.qh_maxclients);
		return;
	}

// parse gametype
	cl.qh_gametype = net_message.ReadByte();

	if (cl.qh_gametype == GAME_DEATHMATCH)
	{
		sv_kingofhill = net_message.ReadShort();
	}

// parse signon message
	str = const_cast<char*>(net_message.ReadString2());
	String::NCpy(cl.qh_levelname, str, sizeof(cl.qh_levelname) - 1);

// seperate the printfs so the server message can have a color
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf(S_COLOR_RED "%s" S_COLOR_WHITE "\n", str);

//
// first we go through and touch all of the precache data that still
// happens to be in the cache, so precaching something else doesn't
// needlessly purge it
//

// precache models
	Com_Memset(cl.model_draw, 0, sizeof(cl.model_draw));
	for (nummodels = 1;; nummodels++)
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
		{
			break;
		}
		if (nummodels == MAX_MODELS_H2)
		{
			Con_Printf("Server sent too many model precaches\n");
			return;
		}
		String::Cpy(model_precache[nummodels], str);
	}

// precache sounds
	Com_Memset(cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds = 1;; numsounds++)
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
		{
			break;
		}
		if (numsounds == MAX_SOUNDS_H2)
		{
			Con_Printf("Server sent too many sound precaches\n");
			return;
		}
		String::Cpy(sound_precache[numsounds], str);
	}

//
// now we try to load everything else until a cache allocation fails
//

	total_loading_size = nummodels + numsounds;
	current_loading_size = 1;
	loading_stage = 2;

	//always precache the world!!!
	CM_LoadMap(model_precache[1], true, NULL);
	R_LoadWorld(model_precache[1]);

	for (i = 2; i < nummodels; i++)
	{
		cl.model_draw[i] = R_RegisterModel(model_precache[i]);
		current_loading_size++;
		SCR_UpdateScreen();

		if (cl.model_draw[i] == 0)
		{
			Con_Printf("Model %s not found\n", model_precache[i]);
			return;
		}
		CL_KeepaliveMessage();
	}

	clh2_player_models[0] = R_RegisterModel("models/paladin.mdl");
	clh2_player_models[1] = R_RegisterModel("models/crusader.mdl");
	clh2_player_models[2] = R_RegisterModel("models/necro.mdl");
	clh2_player_models[3] = R_RegisterModel("models/assassin.mdl");
	clh2_player_models[4] = R_RegisterModel("models/succubus.mdl");

	S_BeginRegistration();
	for (i = 1; i < numsounds; i++)
	{
		cl.sound_precache[i] = S_RegisterSound(sound_precache[i]);
		current_loading_size++;
		SCR_UpdateScreen();

		CL_KeepaliveMessage();
	}
	S_EndRegistration();

	total_loading_size = 0;
	loading_stage = 0;


// local state
	R_NewMap();

	puzzle_strings = (char*)COM_LoadHunkFile("puzzles.txt");

	Hunk_Check();		// make sure nothing is hurt
}

/*
==================
CL_ParseUpdate

Parse an entity update message from the server
If an entities model or origin changes from frame to frame, it must be
relinked.  Other attributes can change without relinking.
==================
*/

void CL_ParseUpdate(int bits)
{
	int i;
	qhandle_t model;
	int modnum;
	qboolean forcelink;
	h2entity_t* ent;
	int num;
	h2entity_state_t* ref_ent,* set_ent,build_ent,dummy;

	if (clc.qh_signon == SIGNONS - 1)
	{	// first update is the final signon stage
		clc.qh_signon = SIGNONS;
		CL_SignonReply();
	}

	if (bits & H2U_MOREBITS)
	{
		i = net_message.ReadByte();
		bits |= (i << 8);
	}

	if (bits & H2U_MOREBITS2)
	{
		i = net_message.ReadByte();
		bits |= (i << 16);
	}

	if (bits & H2U_LONGENTITY)
	{
		num = net_message.ReadShort();
	}
	else
	{
		num = net_message.ReadByte();
	}
	ent = CL_EntityNum(num);
	h2entity_state_t& baseline = clh2_baselines[num];

	baseline.flags |= BE_ON;

	ref_ent = NULL;

	for (i = 0; i < cl.h2_frames[0].count; i++)
		if (cl.h2_frames[0].states[i].number == num)
		{
			ref_ent = &cl.h2_frames[0].states[i];
//			if (num == 2) fprintf(FH,"Found Reference\n");
			break;
		}

	if (!ref_ent)
	{
		ref_ent = &build_ent;

		build_ent.number = num;
		build_ent.origin[0] = baseline.origin[0];
		build_ent.origin[1] = baseline.origin[1];
		build_ent.origin[2] = baseline.origin[2];
		build_ent.angles[0] = baseline.angles[0];
		build_ent.angles[1] = baseline.angles[1];
		build_ent.angles[2] = baseline.angles[2];
		build_ent.modelindex = baseline.modelindex;
		build_ent.frame = baseline.frame;
		build_ent.colormap = baseline.colormap;
		build_ent.skinnum = baseline.skinnum;
		build_ent.effects = baseline.effects;
		build_ent.scale = baseline.scale;
		build_ent.drawflags = baseline.drawflags;
		build_ent.abslight = baseline.abslight;
	}

	if (cl.h2_need_build)
	{	// new sequence, first valid frame
		set_ent = &cl.h2_frames[1].states[cl.h2_frames[1].count];
		cl.h2_frames[1].count++;
	}
	else
	{
		set_ent = &dummy;
	}

	if (bits & H2U_CLEAR_ENT)
	{
		Com_Memset(ent, 0, sizeof(h2entity_t));
		Com_Memset(ref_ent, 0, sizeof(*ref_ent));
		ref_ent->number = num;
	}

	*set_ent = *ref_ent;

	if (ent->msgtime != cl.qh_mtime[1])
	{
		forcelink = true;	// no previous frame to lerp from
	}
	else
	{
		forcelink = false;
	}

	ent->msgtime = cl.qh_mtime[0];

	if (bits & H2U_MODEL)
	{
		modnum = net_message.ReadShort();
		if (modnum >= MAX_MODELS_H2)
		{
			Host_Error("CL_ParseModel: bad modnum");
		}
	}
	else
	{
		modnum = ref_ent->modelindex;
	}

	model = cl.model_draw[modnum];
	set_ent->modelindex = modnum;
	if (modnum != ent->state.modelindex)
	{
		ent->state.modelindex = modnum;

		// automatic animation (torches, etc) can be either all together
		// or randomized
		if (model)
		{
			if (R_ModelSyncType(model) == ST_RAND)
			{
				ent->syncbase = rand() * (1.0 / RAND_MAX);	//(float)(rand()&0x7fff) / 0x7fff;
			}
			else
			{
				ent->syncbase = 0.0;
			}
		}
		else
		{
			forcelink = true;	// hack to make null model players work
		}
		if (num > 0 && num <= cl.qh_maxclients)
		{
			CLH2_TranslatePlayerSkin(num - 1);
		}
	}

	if (bits & H2U_FRAME)
	{
		set_ent->frame = ent->state.frame = net_message.ReadByte();
	}
	else
	{
		ent->state.frame = ref_ent->frame;
	}

	if (bits & H2U_COLORMAP)
	{
		set_ent->colormap = ent->state.colormap = net_message.ReadByte();
	}
	else
	{
		ent->state.colormap = ref_ent->colormap;
	}

	if (bits & H2U_SKIN)
	{
		set_ent->skinnum = ent->state.skinnum = net_message.ReadByte();
		set_ent->drawflags = ent->state.drawflags = net_message.ReadByte();
	}
	else
	{
		ent->state.skinnum = ref_ent->skinnum;
		ent->state.drawflags = ref_ent->drawflags;
	}

	if (bits & H2U_EFFECTS)
	{
		set_ent->effects = ent->state.effects = net_message.ReadByte();
//		if (num == 2) fprintf(FH,"Read effects %d\n",set_ent->effects);
	}
	else
	{
		ent->state.effects = ref_ent->effects;
		//if (num == 2) fprintf(FH,"restored effects %d\n",ref_ent->effects);
	}

// shift the known values for interpolation
	VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);

	if (bits & H2U_ORIGIN1)
	{
		set_ent->origin[0] = ent->msg_origins[0][0] = net_message.ReadCoord();
		//if (num == 2) fprintf(FH,"Read origin[0] %f\n",set_ent->angles[0]);
	}
	else
	{
		ent->msg_origins[0][0] = ref_ent->origin[0];
		//if (num == 2) fprintf(FH,"Restored origin[0] %f\n",ref_ent->angles[0]);
	}
	if (bits & H2U_ANGLE1)
	{
		set_ent->angles[0] = ent->msg_angles[0][0] = net_message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][0] = ref_ent->angles[0];
	}

	if (bits & H2U_ORIGIN2)
	{
		set_ent->origin[1] = ent->msg_origins[0][1] = net_message.ReadCoord();
	}
	else
	{
		ent->msg_origins[0][1] = ref_ent->origin[1];
	}
	if (bits & H2U_ANGLE2)
	{
		set_ent->angles[1] = ent->msg_angles[0][1] = net_message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][1] = ref_ent->angles[1];
	}

	if (bits & H2U_ORIGIN3)
	{
		set_ent->origin[2] = ent->msg_origins[0][2] = net_message.ReadCoord();
	}
	else
	{
		ent->msg_origins[0][2] = ref_ent->origin[2];
	}
	if (bits & H2U_ANGLE3)
	{
		set_ent->angles[2] = ent->msg_angles[0][2] = net_message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][2] = ref_ent->angles[2];
	}

	if (bits & H2U_SCALE)
	{
		set_ent->scale = ent->state.scale = net_message.ReadByte();
		set_ent->abslight = ent->state.abslight = net_message.ReadByte();
	}
	else
	{
		ent->state.scale = ref_ent->scale;
		ent->state.abslight = ref_ent->abslight;
	}

	if (bits & H2U_NOLERP)
	{
		forcelink = true;
	}

	if (forcelink)
	{	// didn't have an update last message
		VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
		VectorCopy(ent->msg_origins[0], ent->state.origin);
		VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);
		VectorCopy(ent->msg_angles[0], ent->state.angles);
	}


//	if (sv.active || num != 2)
//		return;
}

void CL_ParseUpdate2(int bits)
{
	int i;

	if (bits & H2U_MOREBITS)
	{
		i = net_message.ReadByte();
		bits |= (i << 8);
	}

	if (bits & H2U_MOREBITS2)
	{
		i = net_message.ReadByte();
		bits |= (i << 16);
	}

	if (bits & H2U_LONGENTITY)
	{
		net_message.ReadShort();
	}
	else
	{
		net_message.ReadByte();
	}

	if (bits & H2U_MODEL)
	{
		net_message.ReadShort();
	}

	if (bits & H2U_FRAME)
	{
		net_message.ReadByte();
	}

	if (bits & H2U_COLORMAP)
	{
		net_message.ReadByte();
	}

	if (bits & H2U_SKIN)
	{
		net_message.ReadByte();
		net_message.ReadByte();
	}

	if (bits & H2U_EFFECTS)
	{
		net_message.ReadByte();
	}

	if (bits & H2U_ORIGIN1)
	{
		net_message.ReadCoord();
	}
	if (bits & H2U_ANGLE1)
	{
		net_message.ReadAngle();
	}

	if (bits & H2U_ORIGIN2)
	{
		net_message.ReadCoord();
	}
	if (bits & H2U_ANGLE2)
	{
		net_message.ReadAngle();
	}

	if (bits & H2U_ORIGIN3)
	{
		net_message.ReadCoord();
	}
	if (bits & H2U_ANGLE3)
	{
		net_message.ReadAngle();
	}

	if (bits & H2U_SCALE)
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
void CL_ParseBaseline(h2entity_state_t* ent)
{
	int i;

	ent->modelindex = net_message.ReadShort();
	ent->frame = net_message.ReadByte();
	ent->colormap = net_message.ReadByte();
	ent->skinnum = net_message.ReadByte();
	ent->scale = net_message.ReadByte();
	ent->drawflags = net_message.ReadByte();
	ent->abslight = net_message.ReadByte();
	for (i = 0; i < 3; i++)
	{
		ent->origin[i] = net_message.ReadCoord();
		ent->angles[i] = net_message.ReadAngle();
	}
}


/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
void CL_ParseClientdata(int bits)
{
	if (bits & SU_VIEWHEIGHT)
	{
		cl.qh_viewheight = net_message.ReadChar();
	}
//rjr	else
//rjr		cl.h2_viewheight = DEFAULT_VIEWHEIGHT;

	if (bits & SU_IDEALPITCH)
	{
		cl.qh_idealpitch = net_message.ReadChar();
	}
	else
	{
		//rjr   is sv_flypitch   useable on the client's end?
//rjr		if (cl.h2_v.movetype==QHMOVETYPE_FLY)
//rjr			cl.idealpitch = sv_flypitch.value;
//rjr		else
//rjr			cl.idealpitch = sv_walkpitch.value;
	}

	if (bits & SU_IDEALROLL)
	{
		cl.h2_idealroll = net_message.ReadChar();
	}
//rjr	else
//rjr		cl.idealroll = 0;

	VectorCopy(cl.qh_mvelocity[0], cl.qh_mvelocity[1]);
	for (int i = 0; i < 3; i++)
	{
		if (bits & (SU_PUNCH1 << i))
		{
			cl.qh_punchangles[i] = net_message.ReadChar();
		}
//rjr		else
//rjr			cl.punchangle[i] = 0;
		if (bits & (SU_VELOCITY1 << i))
		{
			cl.qh_mvelocity[0][i] = net_message.ReadChar() * 16;
		}
//rjr		else
//rjr			cl.mvelocity[0][i] = 0;
	}

/*	if (bits & SU_ITEMS)
        i = net_message.ReadLong ();
*/

	cl.qh_onground = (bits & SU_ONGROUND) != 0;

	if (bits & SU_WEAPONFRAME)
	{
		cl.qh_stats[STAT_WEAPONFRAME] = net_message.ReadByte();
	}
//rjr	else
//rjr		cl.stats[STAT_WEAPONFRAME] = 0;

	if (bits & SU_ARMOR)
	{
		cl.qh_stats[STAT_ARMOR] = net_message.ReadByte();
	}

	if (bits & SU_WEAPON)
	{
		cl.qh_stats[STAT_WEAPON] = net_message.ReadShort();
	}

/*	sc1 = sc2 = 0;

    if (bits & SU_SC1)
        sc1 = net_message.ReadLong ();
    if (bits & SU_SC2)
        sc2 = net_message.ReadLong ();

    if (sc1 & SC1_HEALTH)
        cl.h2_v.health = net_message.ReadShort();
    if (sc1 & SC1_LEVEL)
        cl.h2_v.level = net_message.ReadByte();
    if (sc1 & SC1_INTELLIGENCE)
        cl.h2_v.intelligence = net_message.ReadByte();
    if (sc1 & SC1_WISDOM)
        cl.h2_v.wisdom = net_message.ReadByte();
    if (sc1 & SC1_STRENGTH)
        cl.h2_v.strength = net_message.ReadByte();
    if (sc1 & SC1_DEXTERITY)
        cl.h2_v.dexterity = net_message.ReadByte();
    if (sc1 & SC1_WEAPON)
        cl.h2_v.weapon = net_message.ReadByte();
    if (sc1 & SC1_BLUEMANA)
        cl.h2_v.bluemana = net_message.ReadByte();
    if (sc1 & SC1_GREENMANA)
        cl.h2_v.greenmana = net_message.ReadByte();
    if (sc1 & SC1_EXPERIENCE)
        cl.h2_v.experience = net_message.ReadLong();
    if (sc1 & SC1_CNT_TORCH)
        cl.h2_v.cnt_torch = net_message.ReadByte();
    if (sc1 & SC1_CNT_H_BOOST)
        cl.h2_v.cnt_h_boost = net_message.ReadByte();
    if (sc1 & SC1_CNT_SH_BOOST)
        cl.h2_v.cnt_sh_boost = net_message.ReadByte();
    if (sc1 & SC1_CNT_MANA_BOOST)
        cl.h2_v.cnt_mana_boost = net_message.ReadByte();
    if (sc1 & SC1_CNT_TELEPORT)
        cl.h2_v.cnt_teleport = net_message.ReadByte();
    if (sc1 & SC1_CNT_TOME)
        cl.h2_v.cnt_tome = net_message.ReadByte();
    if (sc1 & SC1_CNT_SUMMON)
        cl.h2_v.cnt_summon = net_message.ReadByte();
    if (sc1 & SC1_CNT_INVISIBILITY)
        cl.h2_v.cnt_invisibility = net_message.ReadByte();
    if (sc1 & SC1_CNT_GLYPH)
        cl.h2_v.cnt_glyph = net_message.ReadByte();
    if (sc1 & SC1_CNT_HASTE)
        cl.h2_v.cnt_haste = net_message.ReadByte();
    if (sc1 & SC1_CNT_BLAST)
        cl.h2_v.cnt_blast = net_message.ReadByte();
    if (sc1 & SC1_CNT_POLYMORPH)
        cl.h2_v.cnt_polymorph = net_message.ReadByte();
    if (sc1 & SC1_CNT_FLIGHT)
        cl.h2_v.cnt_flight = net_message.ReadByte();
    if (sc1 & SC1_CNT_CUBEOFFORCE)
        cl.h2_v.cnt_cubeofforce = net_message.ReadByte();
    if (sc1 & SC1_CNT_INVINCIBILITY)
        cl.h2_v.cnt_invincibility = net_message.ReadByte();
    if (sc1 & SC1_ARTIFACT_ACTIVE)
        cl.h2_v.artifact_active = net_message.ReadFloat();
    if (sc1 & SC1_ARTIFACT_LOW)
        cl.h2_v.artifact_low = net_message.ReadFloat();
    if (sc1 & SC1_MOVETYPE)
        cl.h2_v.movetype = net_message.ReadByte();
    if (sc1 & SC1_CAMERAMODE)
        cl.h2_v.cameramode = net_message.ReadByte();
    if (sc1 & SC1_HASTED)
        cl.h2_v.hasted = net_message.ReadFloat();
    if (sc1 & SC1_INVENTORY)
        cl.h2_v.inventory = net_message.ReadByte();
    if (sc1 & SC1_RINGS_ACTIVE)
        cl.h2_v.rings_active = net_message.ReadFloat();

    if (sc2 & SC2_RINGS_LOW)
        cl.h2_v.rings_low = net_message.ReadFloat();
    if (sc2 & SC2_AMULET)
        cl.h2_v.armor_amulet = net_message.ReadByte();
    if (sc2 & SC2_BRACER)
        cl.h2_v.armor_bracer = net_message.ReadByte();
    if (sc2 & SC2_BREASTPLATE)
        cl.h2_v.armor_breastplate = net_message.ReadByte();
    if (sc2 & SC2_HELMET)
        cl.h2_v.armor_helmet = net_message.ReadByte();
    if (sc2 & SC2_FLIGHT_T)
        cl.h2_v.ring_flight = net_message.ReadByte();
    if (sc2 & SC2_WATER_T)
        cl.h2_v.ring_water = net_message.ReadByte();
    if (sc2 & SC2_TURNING_T)
        cl.h2_v.ring_turning = net_message.ReadByte();
    if (sc2 & SC2_REGEN_T)
        cl.h2_v.ring_regeneration = net_message.ReadByte();
    if (sc2 & SC2_HASTE_T)
        cl.h2_v.haste_time = net_message.ReadFloat();
    if (sc2 & SC2_TOME_T)
        cl.h2_v.tome_time = net_message.ReadFloat();
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
        cl.h2_v.max_health = net_message.ReadShort();
    if (sc2 & SC2_MAXMANA)
        cl.h2_v.max_mana = net_message.ReadByte();
    if (sc2 & SC2_FLAGS)
        cl.h2_v.flags = net_message.ReadFloat();

    if ((sc1 & SC1_STAT_BAR) || (sc2 & SC2_STAT_BAR))
        SB_Changed();

    if ((sc1 & SC1_INV) || (sc2 & SC2_INV))
        SB_InvChanged();*/
}

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation(int slot)
{
	if (slot > cl.qh_maxclients)
	{
		Sys_Error("CL_NewTranslation: slot > cl.maxclients");
	}
	if (!cl.h2_players[slot].playerclass)
	{
		return;
	}

	CLH2_TranslatePlayerSkin(slot);
}

/*
=====================
CL_ParseStatic
=====================
*/
void CL_ParseStatic(void)
{
	h2entity_t* ent;
	h2entity_state_t baseline;
	int i;

	i = cl.qh_num_statics;
	if (i >= MAX_STATIC_ENTITIES_H2)
	{
		Host_Error("Too many static entities");
	}
	ent = &h2cl_static_entities[i];
	cl.qh_num_statics++;
	CL_ParseBaseline(&baseline);

// copy it to the current state
	ent->state.modelindex = baseline.modelindex;
	ent->state.frame = baseline.frame;
	ent->state.skinnum = baseline.skinnum;
	ent->state.scale = baseline.scale;
	ent->state.effects = baseline.effects;
	ent->state.drawflags = baseline.drawflags;
	ent->state.abslight = baseline.abslight;

	VectorCopy(baseline.origin, ent->state.origin);
	VectorCopy(baseline.angles, ent->state.angles);
}

/*
===================
CL_ParseStaticSound
===================
*/
void CL_ParseStaticSound(void)
{
	vec3_t org;
	int sound_num, vol, atten;
	int i;

	for (i = 0; i < 3; i++)
		org[i] = net_message.ReadCoord();

	sound_num = net_message.ReadShort();
	vol = net_message.ReadByte();
	atten = net_message.ReadByte();

	S_StaticSound(cl.sound_precache[sound_num], org, vol, atten);
}


void CL_Plaque(void)
{
	int index;

	index = net_message.ReadShort();

	if (index > 0 && index <= pr_string_count)
	{
		plaquemessage = &pr_global_strings[pr_string_index[index - 1]];
	}
	else
	{
		plaquemessage = "";
	}
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

	CLH2_ColouredParticleExplosion(org,color,radius,counter);
}

void CL_ParseRainEffect(void)
{
	vec3_t org, e_size;
	short color,count;
	int x_dir, y_dir;

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

	CLH2_RainEffect(org,e_size,x_dir,y_dir,color,count);
}

static void CL_ParsePrint()
{
	const char* txt = net_message.ReadString2();
	if (intro_playing)
	{
		return;
	}
	if (txt[0] == 1)
	{
		S_StartLocalSound("misc/comm.wav");
	}
	if (txt[0] == 1 || txt[0] == 2)
	{
		Con_Printf(S_COLOR_RED "%s" S_COLOR_WHITE, txt + 1);
	}
	else
	{
		Con_Printf("%s", txt);
	}
}

#define SHOWNET(x) if (cl_shownet->value == 2) {Con_Printf("%3i:%s\n", net_message.readcount - 1, x); }

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage(void)
{
	int cmd;
	int i,j,k;
	int EntityCount = 0;
	int EntitySize = 0;
	int before;
	static double lasttime;
	static qboolean packet_loss = false;
	h2entity_t* ent;
	short RemovePlace, OrigPlace, NewPlace, AddedIndex;
	int sc1, sc2;
	byte test;
	float compangles[2][3];
	vec3_t deltaangles;

//
// if recording demos, copy the message out
//
	if (net_message.cursize > LastServerMessageSize)
	{
		LastServerMessageSize = net_message.cursize;
	}
	if (cl_shownet->value == 1)
	{
		Con_Printf("Time: %2.2f Pck: %i ",host_time - lasttime,net_message.cursize);
		lasttime = host_time;
	}
	else if (cl_shownet->value == 2)
	{
		Con_Printf("------------------\n");
	}

	cl.qh_onground = false;	// unless the server says otherwise
//
// parse the message
//
	net_message.BeginReadingOOB();

	while (1)
	{
		if (net_message.badread)
		{
			Host_Error("CL_ParseServerMessage: Bad server message");
		}

		cmd = net_message.ReadByte();

		if (cmd == -1)
		{
			if (cl_shownet->value == 1)
			{
				Con_Printf("Ent: %i (%i bytes)",EntityCount,EntitySize);
			}

			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

		// if the high bit of the command byte is set, it is a fast update
		if (cmd & 128)
		{
			before = net_message.readcount;
			SHOWNET("fast update");
			if (packet_loss)
			{
				CL_ParseUpdate2(cmd & 127);
			}
			else
			{
				CL_ParseUpdate(cmd & 127);
			}

			EntityCount++;
			EntitySize += net_message.readcount - before + 1;
			continue;
		}

		SHOWNET(svc_strings[cmd]);

		// other commands
		switch (cmd)
		{
		default:
			Host_Error("CL_ParseServerMessage: Illegible server message\n");
			break;

		case h2svc_nop:
//			Con_Printf ("h2svc_nop\n");
			break;

		case h2svc_time:
			cl.qh_mtime[1] = cl.qh_mtime[0];
			cl.qh_mtime[0] = net_message.ReadFloat();
			break;

		case h2svc_clientdata:
			i = net_message.ReadShort();
			CL_ParseClientdata(i);
			break;

		case h2svc_version:
			i = net_message.ReadLong();
			if (i != PROTOCOL_VERSION)
			{
				Host_Error("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, PROTOCOL_VERSION);
			}
			break;

		case h2svc_disconnect:
			Host_EndGame("Server disconnected\n");

		case h2svc_print:
			CL_ParsePrint();
			break;

		case h2svc_centerprint:
			//Bottom_Plaque_Draw(net_message.ReadString2(),true);
			SCR_CenterPrint(const_cast<char*>(net_message.ReadString2()));
			break;

		case h2svc_stufftext:
			Cbuf_AddText(const_cast<char*>(net_message.ReadString2()));
			break;

		case h2svc_damage:
			V_ParseDamage();
			break;

		case h2svc_serverinfo:
			CL_ParseServerInfo();
			break;

		case h2svc_setangle:
			for (i = 0; i < 3; i++)
				cl.viewangles[i] = net_message.ReadAngle();

			break;

		case h2svc_setangle_interpolate:

			compangles[0][0] = net_message.ReadAngle();
			compangles[0][1] = net_message.ReadAngle();
			compangles[0][2] = net_message.ReadAngle();

			for (i = 0; i < 3; i++)
			{
				compangles[1][i] = cl.viewangles[i];
				for (j = 0; j < 2; j++)
				{	//standardize both old and new angles to +-180
					if (compangles[j][i] >= 360)
					{
						compangles[j][i] -= 360 * ((int)(compangles[j][i] / 360));
					}
					else if (compangles[j][i] <= 360)
					{
						compangles[j][i] += 360 * (1 + (int)(-compangles[j][i] / 360));
					}
					if (compangles[j][i] > 180)
					{
						compangles[j][i] = -360 + compangles[j][i];
					}
					else if (compangles[j][i] < -180)
					{
						compangles[j][i] = 360 + compangles[j][i];
					}
				}
				//get delta
				deltaangles[i] = compangles[0][i] - compangles[1][i];
				//cap delta to <=180,>=-180
				if (deltaangles[i] > 180)
				{
					deltaangles[i] += -360;
				}
				else if (deltaangles[i] < -180)
				{
					deltaangles[i] += 360;
				}
				//add the delta
				cl.viewangles[i] += (deltaangles[i] / 8);	//8 step interpolation
				//cap newangles to +-180
				if (cl.viewangles[i] >= 360)
				{
					cl.viewangles[i] -= 360 * ((int)(cl.viewangles[i] / 360));
				}
				else if (cl.viewangles[i] <= 360)
				{
					cl.viewangles[i] += 360 * (1 + (int)(-cl.viewangles[i] / 360));
				}
				if (cl.viewangles[i] > 180)
				{
					cl.viewangles[i] += -360;
				}
				else if (cl.viewangles[i] < -180)
				{
					cl.viewangles[i] += 360;
				}
			}
			break;

		case h2svc_setview:
			cl.viewentity = net_message.ReadShort();
			break;

		case h2svc_lightstyle:
			i = net_message.ReadByte();
			CL_SetLightStyle(i, net_message.ReadString2());
			break;

		case h2svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case h2svc_sound_update_pos:
		{	//FIXME: put a field on the entity that lists the channels
		//it should update when it moves- if a certain flag
		//is on the ent, this update_channels field could
		//be set automatically by each sound and stopSound
		//called for this ent?
			vec3_t pos;
			int channel, ent;

			channel = net_message.ReadShort();

			ent = channel >> 3;
			channel &= 7;

			if (ent > MAX_EDICTS_H2)
			{
				Host_Error("h2svc_sound_update_pos: ent = %i", ent);
			}

			for (i = 0; i < 3; i++)
				pos[i] = net_message.ReadCoord();

			S_UpdateSoundPos(ent, channel, pos);
		}
		break;

		case h2svc_stopsound:
			i = net_message.ReadShort();
			S_StopSound(i >> 3, i & 7);
			break;

		case h2svc_updatename:
			i = net_message.ReadByte();
			if (i >= cl.qh_maxclients)
			{
				Host_Error("CL_ParseServerMessage: h2svc_updatename > H2MAX_CLIENTS");
			}
			String::Cpy(cl.h2_players[i].name, net_message.ReadString2());
			break;

		case h2svc_updateclass:
			i = net_message.ReadByte();
			if (i >= cl.qh_maxclients)
			{
				Host_Error("CL_ParseServerMessage: h2svc_updateclass > H2MAX_CLIENTS");
			}
			cl.h2_players[i].playerclass = net_message.ReadByte();
			CL_NewTranslation(i);	// update the color
			break;

		case h2svc_updatefrags:
			i = net_message.ReadByte();
			if (i >= cl.qh_maxclients)
			{
				Host_Error("CL_ParseServerMessage: h2svc_updatefrags > H2MAX_CLIENTS");
			}
			cl.h2_players[i].frags = net_message.ReadShort();
			break;

		case h2svc_update_kingofhill:
			sv_kingofhill = net_message.ReadShort() - 1;
			break;

		case h2svc_updatecolors:
			i = net_message.ReadByte();
			if (i >= cl.qh_maxclients)
			{
				Host_Error("CL_ParseServerMessage: h2svc_updatecolors > H2MAX_CLIENTS");
			}
			j = net_message.ReadByte();
			cl.h2_players[i].topColour = (j & 0xf0) >> 4;
			cl.h2_players[i].bottomColour = (j & 15);
			CL_NewTranslation(i);
			break;

		case h2svc_particle:
			R_ParseParticleEffect();
			break;

		case h2svc_particle2:
			R_ParseParticleEffect2();
			break;
		case h2svc_particle3:
			R_ParseParticleEffect3();
			break;
		case h2svc_particle4:
			R_ParseParticleEffect4();
			break;

		case h2svc_spawnbaseline:
			i = net_message.ReadShort();
			// must use CL_EntityNum() to force cl.num_entities up
			CL_EntityNum(i);
			CL_ParseBaseline(&clh2_baselines[i]);
			break;
		case h2svc_spawnstatic:
			CL_ParseStatic();
			break;

		case h2svc_raineffect:
			CL_ParseRainEffect();
			break;

		case h2svc_temp_entity:
			CLH2_ParseTEnt(net_message);
			break;

		case h2svc_setpause:
		{
			cl.qh_paused = net_message.ReadByte();

			if (cl.qh_paused)
			{
				CDAudio_Pause();
			}
			else
			{
				CDAudio_Resume();
			}
		}
		break;

		case h2svc_signonnum:
			i = net_message.ReadByte();
			if (i <= clc.qh_signon)
			{
				Host_Error("Received signon %i when at %i", i, clc.qh_signon);
			}
			clc.qh_signon = i;
			CL_SignonReply();
			break;

		case h2svc_killedmonster:
			cl.qh_stats[STAT_MONSTERS]++;
			break;

		case h2svc_foundsecret:
			cl.qh_stats[STAT_SECRETS]++;
			break;

		case h2svc_updatestat:
			i = net_message.ReadByte();
			if (i < 0 || i >= MAX_CL_STATS)
			{
				Sys_Error("h2svc_updatestat: %i is invalid", i);
			}
			cl.qh_stats[i] = net_message.ReadLong();;
			break;

		case h2svc_spawnstaticsound:
			CL_ParseStaticSound();
			break;

		case h2svc_cdtrack:
		{
			int cdtrack = net_message.ReadByte();
			net_message.ReadByte();	//	looptrack
			if (String::ICmp(bgmtype->string,"cd") == 0)
			{
				if ((clc.demoplaying || clc.demorecording) && (cls.qh_forcetrack != -1))
				{
					CDAudio_Play((byte)cls.qh_forcetrack, true);
				}
				else
				{
					CDAudio_Play(cdtrack, true);
				}
			}
			else
			{
				CDAudio_Stop();
			}
		}
		break;

		case h2svc_midi_name:
		{
			char midi_name[MAX_QPATH];		// midi file name
			String::Cpy(midi_name,net_message.ReadString2());
			if (String::ICmp(bgmtype->string,"midi") == 0)
			{
				MIDI_Play(midi_name);
			}
			else
			{
				MIDI_Stop();
			}
		}
		break;

		case h2svc_toggle_statbar:
			break;

		case h2svc_intermission:
			cl.qh_intermission = net_message.ReadByte();
			cl.qh_completed_time = cl.qh_serverTimeFloat;
			break;

		case h2svc_set_view_flags:
			cl.h2_viewent.state.drawflags |= net_message.ReadByte();
			break;

		case h2svc_clear_view_flags:
			cl.h2_viewent.state.drawflags &= ~net_message.ReadByte();
			break;

		case h2svc_start_effect:
			CLH2_ParseEffect(net_message);
			break;
		case h2svc_end_effect:
			CLH2_ParseEndEffect(net_message);
			break;

		case h2svc_plaque:
			CL_Plaque();
			break;

		case h2svc_particle_explosion:
			CL_ParticleExplosion();
			break;

		case h2svc_set_view_tint:
			i = net_message.ReadByte();
			cl.h2_viewent.state.colormap = i;
			break;

		case h2svc_reference:
			packet_loss = false;
			cl.h2_last_sequence = cl.h2_current_sequence;
			cl.h2_current_frame = net_message.ReadByte();
			cl.h2_current_sequence = net_message.ReadByte();
			if (cl.h2_need_build == 2)
			{
//					Con_Printf("CL: NB2 CL(%d,%d) R(%d)\n", cl.current_sequence, cl.current_frame,cl.reference_frame);
				cl.h2_frames[0].count = cl.h2_frames[1].count = cl.h2_frames[2].count = 0;
				cl.h2_need_build = 1;
				cl.h2_reference_frame = cl.h2_current_frame;
			}
			else if (cl.h2_last_sequence != cl.h2_current_sequence)
			{
//					Con_Printf("CL: Sequence CL(%d,%d) R(%d)\n", cl.current_sequence, cl.current_frame,cl.reference_frame);
				if (cl.h2_reference_frame >= 1 && cl.h2_reference_frame <= MAX_FRAMES)
				{
					RemovePlace = OrigPlace = NewPlace = AddedIndex = 0;
					for (i = 0; i < cl.qh_num_entities; i++)
					{
						if (RemovePlace >= cl.h2_NumToRemove || cl.h2_RemoveList[RemovePlace] != i)
						{
							if (NewPlace < cl.h2_frames[1].count &&
								cl.h2_frames[1].states[NewPlace].number == i)
							{
								cl.h2_frames[2].states[AddedIndex] = cl.h2_frames[1].states[NewPlace];
								AddedIndex++;
								cl.h2_frames[2].count++;
							}
							else if (OrigPlace < cl.h2_frames[0].count &&
									 cl.h2_frames[0].states[OrigPlace].number == i)
							{
								cl.h2_frames[2].states[AddedIndex] = cl.h2_frames[0].states[OrigPlace];
								AddedIndex++;
								cl.h2_frames[2].count++;
							}
						}
						else
						{
							RemovePlace++;
						}

						if (cl.h2_frames[0].states[OrigPlace].number == i)
						{
							OrigPlace++;
						}
						if (cl.h2_frames[1].states[NewPlace].number == i)
						{
							NewPlace++;
						}
					}
					cl.h2_frames[0] = cl.h2_frames[2];
				}
				cl.h2_frames[1].count = cl.h2_frames[2].count = 0;
				cl.h2_need_build = 1;
				cl.h2_reference_frame = cl.h2_current_frame;
			}
			else
			{
//					Con_Printf("CL: Normal CL(%d,%d) R(%d)\n", cl.current_sequence, cl.current_frame,cl.reference_frame);
				cl.h2_need_build = 0;
			}

			for (i = 1; i < cl.qh_num_entities; i++)
			{
				clh2_baselines[i].flags &= ~BE_ON;
			}

			for (i = 0; i < cl.h2_frames[0].count; i++)
			{
				ent = CL_EntityNum(cl.h2_frames[0].states[i].number);
				ent->state.modelindex = cl.h2_frames[0].states[i].modelindex;
				clh2_baselines[cl.h2_frames[0].states[i].number].flags |= BE_ON;
			}
			break;

		case h2svc_clear_edicts:
			j = net_message.ReadByte();
			if (cl.h2_need_build)
			{
				cl.h2_NumToRemove = j;
			}
			for (i = 0; i < j; i++)
			{
				k = net_message.ReadShort();
				if (cl.h2_need_build)
				{
					cl.h2_RemoveList[i] = k;
				}
				ent = CL_EntityNum(k);
				clh2_baselines[k].flags &= ~BE_ON;
			}
			break;

		case h2svc_update_inv:
			sc1 = sc2 = 0;

			test = net_message.ReadByte();
			if (test & 1)
			{
				sc1 |= ((int)net_message.ReadByte());
			}
			if (test & 2)
			{
				sc1 |= ((int)net_message.ReadByte()) << 8;
			}
			if (test & 4)
			{
				sc1 |= ((int)net_message.ReadByte()) << 16;
			}
			if (test & 8)
			{
				sc1 |= ((int)net_message.ReadByte()) << 24;
			}
			if (test & 16)
			{
				sc2 |= ((int)net_message.ReadByte());
			}
			if (test & 32)
			{
				sc2 |= ((int)net_message.ReadByte()) << 8;
			}
			if (test & 64)
			{
				sc2 |= ((int)net_message.ReadByte()) << 16;
			}
			if (test & 128)
			{
				sc2 |= ((int)net_message.ReadByte()) << 24;
			}

			if (sc1 & SC1_HEALTH)
			{
				cl.h2_v.health = net_message.ReadShort();
			}
			if (sc1 & SC1_LEVEL)
			{
				cl.h2_v.level = net_message.ReadByte();
			}
			if (sc1 & SC1_INTELLIGENCE)
			{
				cl.h2_v.intelligence = net_message.ReadByte();
			}
			if (sc1 & SC1_WISDOM)
			{
				cl.h2_v.wisdom = net_message.ReadByte();
			}
			if (sc1 & SC1_STRENGTH)
			{
				cl.h2_v.strength = net_message.ReadByte();
			}
			if (sc1 & SC1_DEXTERITY)
			{
				cl.h2_v.dexterity = net_message.ReadByte();
			}
			if (sc1 & SC1_WEAPON)
			{
				cl.h2_v.weapon = net_message.ReadByte();
			}
			if (sc1 & SC1_BLUEMANA)
			{
				cl.h2_v.bluemana = net_message.ReadByte();
			}
			if (sc1 & SC1_GREENMANA)
			{
				cl.h2_v.greenmana = net_message.ReadByte();
			}
			if (sc1 & SC1_EXPERIENCE)
			{
				cl.h2_v.experience = net_message.ReadLong();
			}
			if (sc1 & SC1_CNT_TORCH)
			{
				cl.h2_v.cnt_torch = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_H_BOOST)
			{
				cl.h2_v.cnt_h_boost = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_SH_BOOST)
			{
				cl.h2_v.cnt_sh_boost = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_MANA_BOOST)
			{
				cl.h2_v.cnt_mana_boost = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_TELEPORT)
			{
				cl.h2_v.cnt_teleport = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_TOME)
			{
				cl.h2_v.cnt_tome = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_SUMMON)
			{
				cl.h2_v.cnt_summon = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_INVISIBILITY)
			{
				cl.h2_v.cnt_invisibility = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_GLYPH)
			{
				cl.h2_v.cnt_glyph = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_HASTE)
			{
				cl.h2_v.cnt_haste = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_BLAST)
			{
				cl.h2_v.cnt_blast = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_POLYMORPH)
			{
				cl.h2_v.cnt_polymorph = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_FLIGHT)
			{
				cl.h2_v.cnt_flight = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_CUBEOFFORCE)
			{
				cl.h2_v.cnt_cubeofforce = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_INVINCIBILITY)
			{
				cl.h2_v.cnt_invincibility = net_message.ReadByte();
			}
			if (sc1 & SC1_ARTIFACT_ACTIVE)
			{
				cl.h2_v.artifact_active = net_message.ReadFloat();
			}
			if (sc1 & SC1_ARTIFACT_LOW)
			{
				cl.h2_v.artifact_low = net_message.ReadFloat();
			}
			if (sc1 & SC1_MOVETYPE)
			{
				cl.h2_v.movetype = net_message.ReadByte();
			}
			if (sc1 & SC1_CAMERAMODE)
			{
				cl.h2_v.cameramode = net_message.ReadByte();
			}
			if (sc1 & SC1_HASTED)
			{
				cl.h2_v.hasted = net_message.ReadFloat();
			}
			if (sc1 & SC1_INVENTORY)
			{
				cl.h2_v.inventory = net_message.ReadByte();
			}
			if (sc1 & SC1_RINGS_ACTIVE)
			{
				cl.h2_v.rings_active = net_message.ReadFloat();
			}

			if (sc2 & SC2_RINGS_LOW)
			{
				cl.h2_v.rings_low = net_message.ReadFloat();
			}
			if (sc2 & SC2_AMULET)
			{
				cl.h2_v.armor_amulet = net_message.ReadByte();
			}
			if (sc2 & SC2_BRACER)
			{
				cl.h2_v.armor_bracer = net_message.ReadByte();
			}
			if (sc2 & SC2_BREASTPLATE)
			{
				cl.h2_v.armor_breastplate = net_message.ReadByte();
			}
			if (sc2 & SC2_HELMET)
			{
				cl.h2_v.armor_helmet = net_message.ReadByte();
			}
			if (sc2 & SC2_FLIGHT_T)
			{
				cl.h2_v.ring_flight = net_message.ReadByte();
			}
			if (sc2 & SC2_WATER_T)
			{
				cl.h2_v.ring_water = net_message.ReadByte();
			}
			if (sc2 & SC2_TURNING_T)
			{
				cl.h2_v.ring_turning = net_message.ReadByte();
			}
			if (sc2 & SC2_REGEN_T)
			{
				cl.h2_v.ring_regeneration = net_message.ReadByte();
			}
			if (sc2 & SC2_HASTE_T)
			{
				cl.h2_v.haste_time = net_message.ReadFloat();
			}
			if (sc2 & SC2_TOME_T)
			{
				cl.h2_v.tome_time = net_message.ReadFloat();
			}
			if (sc2 & SC2_PUZZLE1)
			{
				sprintf(cl.h2_puzzle_pieces[0], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE2)
			{
				sprintf(cl.h2_puzzle_pieces[1], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE3)
			{
				sprintf(cl.h2_puzzle_pieces[2], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE4)
			{
				sprintf(cl.h2_puzzle_pieces[3], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE5)
			{
				sprintf(cl.h2_puzzle_pieces[4], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE6)
			{
				sprintf(cl.h2_puzzle_pieces[5], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE7)
			{
				sprintf(cl.h2_puzzle_pieces[6], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE8)
			{
				sprintf(cl.h2_puzzle_pieces[7], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_MAXHEALTH)
			{
				cl.h2_v.max_health = net_message.ReadShort();
			}
			if (sc2 & SC2_MAXMANA)
			{
				cl.h2_v.max_mana = net_message.ReadByte();
			}
			if (sc2 & SC2_FLAGS)
			{
				cl.h2_v.flags = net_message.ReadFloat();
			}
			if (sc2 & SC2_OBJ)
			{
				cl.h2_info_mask = net_message.ReadLong();
			}
			if (sc2 & SC2_OBJ2)
			{
				cl.h2_info_mask2 = net_message.ReadLong();
			}

			if ((sc1 & SC1_INV) || (sc2 & SC2_INV))
			{
				SB_InvChanged();
			}
			break;
		}
	}

}
