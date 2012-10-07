//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../client.h"
#include "local.h"
#include "../quake_hexen2/parse.h"
#include "../../../server/public.h"
#include "../../../common/hexen2strings.h"

bool clhw_siege;
int clhw_keyholder = -1;
int clhw_doc = -1;
float clhw_timelimit;
float clhw_server_time_offset;
byte clhw_fraglimit;
unsigned int clhw_defLosses;		// Defenders losses in Siege
unsigned int clhw_attLosses;		// Attackers Losses in Siege

static ptype_t hexen2ParticleTypeTable[] =
{
	pt_h2static,
	pt_h2grav,
	pt_h2fastgrav,
	pt_h2slowgrav,
	pt_h2fire,
	pt_h2explode,
	pt_h2explode2,
	pt_h2blob,
	pt_h2blob2,
	pt_h2rain,
	pt_h2c_explode,
	pt_h2c_explode2,
	pt_h2spit,
	pt_h2fireball,
	pt_h2ice,
	pt_h2spell,
	pt_h2test,
	pt_h2quake,
	pt_h2rd,
	pt_h2vorpal,
	pt_h2setstaff,
	pt_h2magicmissile,
	pt_h2boneshard,
	pt_h2scarab,
	pt_h2acidball,
	pt_h2darken,
	pt_h2snow,
	pt_h2gravwell,
	pt_h2redfire,
};

static ptype_t hexenWorldParticleTypeTable[] =
{
	pt_h2static,
	pt_h2grav,
	pt_h2fastgrav,
	pt_h2slowgrav,
	pt_h2fire,
	pt_h2explode,
	pt_h2explode2,
	pt_h2blob_hw,
	pt_h2blob2_hw,
	pt_h2rain,
	pt_h2c_explode,
	pt_h2c_explode2,
	pt_h2spit,
	pt_h2fireball,
	pt_h2ice,
	pt_h2spell,
	pt_h2test,
	pt_h2quake,
	pt_h2rd,
	pt_h2vorpal,
	pt_h2setstaff,
	pt_h2magicmissile,
	pt_h2boneshard,
	pt_h2scarab,
	pt_h2darken,
	pt_h2grensmoke,
	pt_h2redfire,
	pt_h2acidball,
	pt_h2bluestep,
};

//	Server information pertaining to this client only
void CLH2_ParseClientdata(QMsg& message)
{
	int bits = message.ReadShort();

	if (bits & Q1SU_VIEWHEIGHT)
	{
		cl.qh_viewheight = message.ReadChar();
	}

	if (bits & Q1SU_IDEALPITCH)
	{
		cl.qh_idealpitch = message.ReadChar();
	}
	else
	{
	}

	if (bits & H2SU_IDEALROLL)
	{
		cl.h2_idealroll = message.ReadChar();
	}

	VectorCopy(cl.qh_mvelocity[0], cl.qh_mvelocity[1]);
	for (int i = 0; i < 3; i++)
	{
		if (bits & (Q1SU_PUNCH1 << i))
		{
			cl.qh_punchangles[i] = message.ReadChar();
		}
		if (bits & (Q1SU_VELOCITY1 << i))
		{
			cl.qh_mvelocity[0][i] = message.ReadChar() * 16;
		}
	}

	cl.qh_onground = (bits & Q1SU_ONGROUND) != 0;

	if (bits & Q1SU_WEAPONFRAME)
	{
		cl.qh_stats[Q1STAT_WEAPONFRAME] = message.ReadByte();
	}

	if (bits & Q1SU_ARMOR)
	{
		cl.qh_stats[Q1STAT_ARMOR] = message.ReadByte();
	}

	if (bits & Q1SU_WEAPON)
	{
		cl.qh_stats[Q1STAT_WEAPON] = message.ReadShort();
	}
}

//	Server information pertaining to this client only, sent every frame
void CLHW_ParseClientdata()
{
	// calculate simulated time of message
	cl.qh_parsecount = clc.netchan.incomingAcknowledged;
	hwframe_t* frame = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW];

	frame->receivedtime = cls.realtime * 0.001;

	// calculate latency
	float latency = frame->receivedtime - frame->senttime;

	if (latency < 0 || latency > 1.0)
	{
		//common->Printf ("Odd latency: %5.2f\n", latency);
	}
	else
	{
		// drift the average latency towards the observed latency
		if (latency < cls.qh_latency)
		{
			cls.qh_latency = latency;
		}
		else
		{
			cls.qh_latency += 0.001;	// drift up, so correction are needed
		}
	}
}

void CLH2_ParseVersion(QMsg& message)
{
	int i = message.ReadLong();
	if (i != H2PROTOCOL_VERSION)
	{
		common->Error("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, H2PROTOCOL_VERSION);
	}
}

void CLHW_ParseDisconnect()
{
	common->Error("Server disconnected\n");
}

void CLH2_ParsePrint(QMsg& message)
{
	const char* txt = message.ReadString2();

	if (h2intro_playing)
	{
		return;
	}
	if (txt[0] == 1)
	{
		S_StartLocalSound("misc/comm.wav");
	}
	if (txt[0] == 1 || txt[0] == 2)
	{
		common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE, txt + 1);
	}
	else
	{
		common->Printf("%s", txt);
	}
}

void CLHW_ParsePrint(QMsg& message)
{
	int i = message.ReadByte();
	const char* txt = message.ReadString2();

	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
		common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE, txt);
	}
	else if (i >= PRINT_SOUND)
	{
		if (!clhw_talksounds->value)
		{
			return;
		}
		char temp[100];
		sprintf(temp, "taunt/taunt%.3d.wav", i - PRINT_SOUND + 1);
		S_StartLocalSound(temp);
		common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE, txt);
	}
	else
	{
		common->Printf("%s", txt);
	}
}

void CLH2_ParseSetAngleInterpolate(QMsg& message)
{
	float compangles[2][3];
	compangles[0][0] = message.ReadAngle();
	compangles[0][1] = message.ReadAngle();
	compangles[0][2] = message.ReadAngle();

	for (int i = 0; i < 3; i++)
	{
		compangles[1][i] = cl.viewangles[i];
		for (int j = 0; j < 2; j++)
		{
			//standardize both old and new angles to +-180
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
		float deltaangle = compangles[0][i] - compangles[1][i];
		//cap delta to <=180,>=-180
		if (deltaangle > 180)
		{
			deltaangle += -360;
		}
		else if (deltaangle < -180)
		{
			deltaangle += 360;
		}
		//add the delta
		cl.viewangles[i] += (deltaangle / 8);	//8 step interpolation
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
}

void CLH2_ParseStartSoundPacket(QMsg& message)
{
	CLQH_ParseStartSoundPacket(message, H2SND_OVERFLOW);
}

void CLHW_ParseStartSoundPacket(QMsg& message)
{
	CLQHW_ParseStartSoundPacket(message, 32.0);
}

void CLH2_ParseSoundUpdatePos(QMsg& message)
{
	//FIXME: put a field on the entity that lists the channels
	//it should update when it moves- if a certain flag
	//is on the ent, this update_channels field could
	//be set automatically by each sound and stopSound
	//called for this ent?
	int channel = message.ReadShort();
	vec3_t pos;
	message.ReadPos(pos);

	int ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS_QH)
	{
		common->Error("svc_sound_update_pos: ent = %i", ent);
	}

	S_UpdateSoundPos(ent, channel, pos);
}

void CLH2_ParseUpdateName(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= cl.qh_maxclients)
	{
		common->Error("CL_ParseServerMessage: svc_updatename > MAX_CLIENTS_QH");
	}
	String::Cpy(cl.h2_players[i].name, message.ReadString2());
}

void CLH2_ParseUpdateClass(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= cl.qh_maxclients)
	{
		common->Error("CL_ParseServerMessage: h2svc_updateclass > MAX_CLIENTS_QH");
	}
	cl.h2_players[i].playerclass = message.ReadByte();
	CLH2_TranslatePlayerSkin(i);
}

void CLHW_ParseUpdateClass(QMsg& message)
{
	// playerclass has changed for this dude
	int i = message.ReadByte();
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: hwsvc_updatepclass > MAX_CLIENTS_QHW");
	}
	int value = message.ReadByte();
	cl.h2_players[i].level = value & 31;
	cl.h2_players[i].playerclass = value >> 5;
}

void CLH2_UpdateFrags(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= cl.qh_maxclients)
	{
		common->Error("CL_ParseServerMessage: h2svc_updatefrags > MAX_CLIENTS_QH");
	}
	cl.h2_players[i].frags = message.ReadShort();
}

void CLHW_UpdateFrags(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: h2svc_updatefrags > MAX_CLIENTS_QHW");
	}
	cl.h2_players[i].frags = message.ReadShort();
}

void CLH2_ParseUpdateKingOfHill(QMsg& message)
{
	svh2_kingofhill = message.ReadShort() - 1;
}

void CLH2_ParseUpdateColors(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= cl.qh_maxclients)
	{
		common->Error("CL_ParseServerMessage: h2svc_updatecolors > MAX_CLIENTS_QH");
	}
	int j = message.ReadByte();
	cl.h2_players[i].topColour = (j & 0xf0) >> 4;
	cl.h2_players[i].bottomColour = (j & 15);
	CLH2_TranslatePlayerSkin(i);
}

void CLHW_ParseUpdatePing(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: hwsvc_updateping > MAX_CLIENTS_QHW");
	}
	cl.h2_players[i].ping = message.ReadShort();
}

void CLHW_ParseUpdateEnterTime(QMsg& message)
{
	// time is sent over as seconds ago
	int i = message.ReadByte();
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: hwsvc_updateentertime > MAX_CLIENTS_QHW");
	}
	cl.h2_players[i].entertime = cls.realtime * 0.001 - message.ReadFloat();
}

void CLHW_ParseUpdateDMInfo(QMsg& message)
{
	// This dude killed someone, update his frags and level
	int i = message.ReadByte();
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: hwsvc_updatedminfo > MAX_CLIENTS_QHW");
	}
	cl.h2_players[i].frags = message.ReadShort();
	cl.h2_players[i].playerclass = message.ReadByte();
	cl.h2_players[i].level = cl.h2_players[i].playerclass & 31;
	cl.h2_players[i].playerclass = cl.h2_players[i].playerclass >> 5;
}

void CLHW_ParseUpdateSiegeLosses(QMsg& message)
{
	clhw_defLosses = message.ReadByte();
	clhw_attLosses = message.ReadByte();
}

void CLHW_ParseUpdateSiegeTeam(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: hwsvc_updatesiegeteam > MAX_CLIENTS_QHW");
	}
	cl.h2_players[i].siege_team = message.ReadByte();
}

void CLHW_ParseUpdateSiegeInfo(QMsg& message)
{
	clhw_siege = true;
	clhw_timelimit = message.ReadByte() * 60;
	clhw_fraglimit = message.ReadByte();
}

void CLHW_ParseHasKey(QMsg& message)
{
	clhw_keyholder = message.ReadShort() - 1;
}

void CLHW_ParseIsDoc(QMsg& message)
{
	clhw_doc = message.ReadShort() - 1;
}

void CLHW_ParseNoneHasKey()
{
	clhw_keyholder = -1;
}

void CLHW_ParseNoDoc()
{
	clhw_doc = -1;
}

void CLHW_ParseTime(QMsg& message)
{
	clhw_server_time_offset = ((int)message.ReadFloat()) - cl.qh_serverTimeFloat;
}

void CLH2_ParseParticleEffect(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	vec3_t dir;
	for (int i = 0; i < 3; i++)
	{
		dir[i] = message.ReadChar() * (1.0 / 16);
	}
	int count = message.ReadByte();
	message.ReadByte();	//	Colour, unused

	if (count == 255)
	{
		CLH2_ParticleExplosion(org);
	}
	else
	{
		CLH2_RunParticleEffect(org, dir, count);
	}
}

void CLH2_ParseParticleEffect2(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	vec3_t dmin;
	for (int i = 0; i < 3; i++)
	{
		dmin[i] = message.ReadFloat();
	}
	vec3_t dmax;
	for (int i = 0; i < 3; i++)
	{
		dmax[i] = message.ReadFloat();
	}
	int colour = message.ReadShort();
	int msgcount = message.ReadByte();
	int effect = message.ReadByte();

	CLH2_RunParticleEffect2(org, dmin, dmax, colour, hexen2ParticleTypeTable[effect], msgcount);
}

void CLHW_ParseParticleEffect2(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	vec3_t dmin;
	for (int i = 0; i < 3; i++)
	{
		dmin[i] = message.ReadFloat();
	}
	vec3_t dmax;
	for (int i = 0; i < 3; i++)
	{
		dmax[i] = message.ReadFloat();
	}
	int colour = message.ReadShort();
	int msgcount = message.ReadByte();
	int effect = message.ReadByte();

	CLH2_RunParticleEffect2(org, dmin, dmax, colour, hexenWorldParticleTypeTable[effect], msgcount);
}

void CLH2_ParseParticleEffect3(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	vec3_t box;
	for (int i = 0; i < 3; i++)
	{
		box[i] = message.ReadByte();
	}
	int colour = message.ReadShort();
	int msgcount = message.ReadByte();
	int effect = message.ReadByte();

	CLH2_RunParticleEffect3(org, box, colour, hexen2ParticleTypeTable[effect], msgcount);
}

void CLHW_ParseParticleEffect3(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	vec3_t box;
	for (int i = 0; i < 3; i++)
	{
		box[i] = message.ReadByte();
	}
	int colour = message.ReadShort();
	int msgcount = message.ReadByte();
	int effect = message.ReadByte();

	CLH2_RunParticleEffect3(org, box, colour, hexenWorldParticleTypeTable[effect], msgcount);
}

void CLH2_ParseParticleEffect4(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	int radius = message.ReadByte();
	int color = message.ReadShort();
	int msgcount = message.ReadByte();
	int effect = message.ReadByte();

	CLH2_RunParticleEffect4(org, radius, color, hexen2ParticleTypeTable[effect], msgcount);
}

void CLHW_ParseParticleEffect4(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	int radius = message.ReadByte();
	int color = message.ReadShort();
	int msgcount = message.ReadByte();
	int effect = message.ReadByte();

	CLH2_RunParticleEffect4(org, radius, color, hexenWorldParticleTypeTable[effect], msgcount);
}

void CLH2_ParseRainEffect(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	vec3_t e_size;
	message.ReadPos(e_size);
	int x_dir = message.ReadAngle();
	int y_dir = message.ReadAngle();
	short colour = message.ReadShort();
	short count = message.ReadShort();

	CLH2_RainEffect(org, e_size, x_dir, y_dir, colour, count);
}

void CLH2_ParseSignonNum(QMsg& message)
{
	int i = message.ReadByte();
	if (i <= clc.qh_signon)
	{
		common->Error("Received signon %i when at %i", i, clc.qh_signon);
	}
	clc.qh_signon = i;
	CLH2_SignonReply();
}

static void CLHW_SetStat(int stat, int value)
{
	if (stat < 0 || stat >= MAX_CL_STATS)
	{
		common->FatalError("CLHW_SetStat: %i is invalid", stat);
	}

	cl.qh_stats[stat] = value;
}

void CLHW_ParseUpdateStat(QMsg& message)
{
	int i = message.ReadByte();
	int j = message.ReadByte();
	CLHW_SetStat(i, j);
}

void CLHW_ParseUpdateStatLong(QMsg& message)
{
	int i = message.ReadByte();
	int j = message.ReadLong();
	CLHW_SetStat(i, j);
}

void CLH2_ParseStaticSound(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	int sound_num = message.ReadShort();
	int vol = message.ReadByte();
	int atten = message.ReadByte();

	S_StaticSound(cl.sound_precache[sound_num], org, vol, atten);
}

void CLH2_ParseCDTrack(QMsg& message)
{
	int cdtrack = message.ReadByte();
	message.ReadByte();	//	looptrack

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

void CLH2_ParseMidiName(QMsg& message)
{
	char midi_name[MAX_QPATH];		// midi file name
	String::Cpy(midi_name, message.ReadString2());
	if (String::ICmp(bgmtype->string, "midi") == 0)
	{
		MIDI_Play(midi_name);
	}
	else
	{
		MIDI_Stop();
	}
}

void CLH2_ParseIntermission(QMsg& message)
{
	cl.qh_intermission = message.ReadByte();
	cl.qh_completed_time = cl.qh_serverTimeFloat;
}

void CLHW_ParseIntermission(QMsg& message)
{
	cl.qh_intermission = message.ReadByte();
	cl.qh_completed_time = cls.realtime * 0.001;
}

void CLHW_MuzzleFlash(QMsg& message)
{
	int i = message.ReadShort();
	if ((unsigned)(i - 1) >= MAX_CLIENTS_QHW)
	{
		return;
	}
	hwplayer_state_t* pl = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].playerstate[i - 1];
	CLH2_MuzzleFlashLight(i, pl->origin, pl->viewangles, false);
}

void CLHW_UpdateUserinfo(QMsg& message)
{
	int slot = message.ReadByte();
	if (slot >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: hwsvc_updateuserinfo > MAX_CLIENTS_QHW");
	}

	h2player_info_t* player = &cl.h2_players[slot];
	player->userid = message.ReadLong();
	String::NCpy(player->userinfo, message.ReadString2(), sizeof(player->userinfo) - 1);

	String::NCpy(player->name, Info_ValueForKey(player->userinfo, "name"), sizeof(player->name) - 1);
	player->topColour = String::Atoi(Info_ValueForKey(player->userinfo, "topcolor"));
	player->bottomColour = String::Atoi(Info_ValueForKey(player->userinfo, "bottomcolor"));
	if (Info_ValueForKey(player->userinfo, "*spectator")[0])
	{
		player->spectator = true;
	}
	else
	{
		player->spectator = false;
	}

	player->playerclass = String::Atoi(Info_ValueForKey(player->userinfo, "playerclass"));
	player->Translated = false;
	CLH2_TranslatePlayerSkin(slot);
}

static void CLHW_Model_NextDownload()
{
	if (clc.downloadNumber == 0)
	{
		common->Printf("Checking models...\n");
		clc.downloadNumber = 1;
	}

	clc.downloadType = dl_model;
	for (; cl.qh_model_name[clc.downloadNumber][0]; clc.downloadNumber++)
	{
		const char* s = cl.qh_model_name[clc.downloadNumber];
		if (s[0] == '*')
		{
			continue;	// inline brush model
		}
		if (!CL_CheckOrDownloadFile(s))
		{
			return;		// started a download
		}
	}

	CM_LoadMap(cl.qh_model_name[1], true, NULL);
	cl.model_clip[1] = 0;
	R_LoadWorld(cl.qh_model_name[1]);

	for (int i = 2; i < MAX_MODELS_H2; i++)
	{
		if (!cl.qh_model_name[i][0])
		{
			break;
		}
		cl.model_draw[i] = R_RegisterModel(cl.qh_model_name[i]);
		if (cl.qh_model_name[i][0] == '*')
		{
			cl.model_clip[i] = CM_InlineModel(String::Atoi(cl.qh_model_name[i] + 1));
		}
		if (!cl.model_draw[i])
		{
			common->Printf("\nThe required model file '%s' could not be found or downloaded.\n\n",
				cl.qh_model_name[i]);
			common->Printf("You may need to download or purchase a %s client "
					   "pack in order to play on this server.\n\n", fs_gamedir);
			CL_Disconnect();
			return;
		}
	}

	// all done
	R_EndRegistration();

	// done with modellist, request first of static signon messages
	CL_AddReliableCommand(va("prespawn %i", cl.servercount));
}

static void CLHW_Sound_NextDownload()
{
	if (clc.downloadNumber == 0)
	{
		common->Printf("Checking sounds...\n");
		clc.downloadNumber = 1;
	}

	clc.downloadType = dl_sound;
	for (; cl.qh_sound_name[clc.downloadNumber][0]
		; clc.downloadNumber++)
	{
		const char* s = cl.qh_sound_name[clc.downloadNumber];
		if (!CL_CheckOrDownloadFile(va("sound/%s", s)))
		{
			return;		// started a download
		}
	}

	S_BeginRegistration();
	for (int i = 1; i < MAX_SOUNDS_HW; i++)
	{
		if (!cl.qh_sound_name[i][0])
		{
			break;
		}
		cl.sound_precache[i] = S_RegisterSound(cl.qh_sound_name[i]);
	}
	S_EndRegistration();

	// done with sounds, request models now
	CL_AddReliableCommand(va("modellist %i", cl.servercount));
}

static void CLHW_RequestNextDownload()
{
	switch (clc.downloadType)
	{
	case dl_single:
		break;
	case dl_model:
		CLHW_Model_NextDownload();
		break;
	case dl_sound:
		CLHW_Sound_NextDownload();
		break;
	case dl_none:
	case dl_skin:
		break;
	}
}

//	A download message has been received from the server
void CLHW_ParseDownload(QMsg& message)
{
	// read the data
	int size = message.ReadShort();
	int percent = message.ReadByte();

	if (clc.demoplaying)
	{
		if (size > 0)
		{
			message.readcount += size;
		}
		return;	// not in demo playback
	}

	if (size == -1)
	{
		common->Printf("File not found.\n");
		if (clc.download)
		{
			common->Printf("cls.download shouldn't have been set\n");
			FS_FCloseFile(clc.download);
			clc.download = 0;
		}
		CLHW_RequestNextDownload();
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		clc.download = FS_FOpenFileWrite(clc.downloadTempName);

		if (!clc.download)
		{
			message.readcount += size;
			common->Printf("Failed to open %s\n", clc.downloadTempName);
			CLHW_RequestNextDownload();
			return;
		}
	}

	FS_Write(message._data + message.readcount, size, clc.download);
	message.readcount += size;

	if (percent != 100)
	{
		// request next block
		clc.downloadPercent = percent;

		CL_AddReliableCommand("nextdl");
	}
	else
	{
		FS_FCloseFile(clc.download);

		// rename the temp file to it's final name
		FS_Rename(clc.downloadTempName, clc.downloadName);

		clc.download = 0;
		clc.downloadPercent = 0;

		// get another file if needed
		CLHW_RequestNextDownload();
	}
}

// some preceding packets were choked
void CLHW_ParseChokeCount(QMsg& message)
{
	int i = message.ReadByte();
	for (int j = 0; j < i; j++)
	{
		cl.hw_frames[(clc.netchan.incomingAcknowledged - 1 - j) & UPDATE_MASK_HW].receivedtime = -2;
	}
}

void CLHW_ParseModelList(QMsg& message)
{
	// precache models and note certain default indexes
	Com_Memset(cl.model_draw, 0, sizeof(cl.model_draw));
	clhw_playerindex[0] = -1;
	clhw_playerindex[1] = -1;
	clhw_playerindex[2] = -1;
	clhw_playerindex[3] = -1;
	clhw_playerindex[4] = -1;
	clhw_playerindex[5] = -1;	//mg-siege
	clh2_ballindex = -1;
	clh2_missilestarindex = -1;
	clh2_ravenindex = -1;
	clh2_raven2index = -1;

	for (int nummodels = 1;; nummodels++)
	{
		const char* str = message.ReadString2();
		if (!str[0])
		{
			break;
		}
		if (nummodels == MAX_MODELS_H2)
		{
			common->Error("Server sent too many model_precache");
		}
		String::Cpy(cl.qh_model_name[nummodels], str);

		if (!String::Cmp(cl.qh_model_name[nummodels],"models/paladin.mdl"))
		{
			clhw_playerindex[0] = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/crusader.mdl"))
		{
			clhw_playerindex[1] = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/necro.mdl"))
		{
			clhw_playerindex[2] = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/assassin.mdl"))
		{
			clhw_playerindex[3] = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/succubus.mdl"))
		{
			clhw_playerindex[4] = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/hank.mdl"))
		{
			clhw_playerindex[5] = nummodels;	//mg-siege
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/ball.mdl"))
		{
			clh2_ballindex = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/newmmis.mdl"))
		{
			clh2_missilestarindex = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/ravproj.mdl"))
		{
			clh2_ravenindex = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/vindsht1.mdl"))
		{
			clh2_raven2index = nummodels;
		}
	}

	clh2_player_models[0] = R_RegisterModel("models/paladin.mdl");
	clh2_player_models[1] = R_RegisterModel("models/crusader.mdl");
	clh2_player_models[2] = R_RegisterModel("models/necro.mdl");
	clh2_player_models[3] = R_RegisterModel("models/assassin.mdl");
	clh2_player_models[4] = R_RegisterModel("models/succubus.mdl");
	//siege
	clh2_player_models[5] = R_RegisterModel("models/hank.mdl");

	clc.downloadNumber = 0;
	clc.downloadType = dl_model;
	CLHW_Model_NextDownload();
}

void CLHW_ParseSoundList(QMsg& message)
{
	// precache sounds
	Com_Memset(cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (int numsounds = 1;; numsounds++)
	{
		const char* str = message.ReadString2();
		if (!str[0])
		{
			break;
		}
		if (numsounds == MAX_SOUNDS_HW)
		{
			common->Error("Server sent too many sound_precache");
		}
		String::Cpy(cl.qh_sound_name[numsounds], str);
	}

	clc.downloadNumber = 0;
	clc.downloadType = dl_sound;
	CLHW_Sound_NextDownload();
}

void CLH2_ParseSetViewFlags(QMsg& message)
{
	cl.h2_viewent.state.drawflags |= message.ReadByte();
}

void CLH2_ParseClearViewFlags(QMsg& message)
{
	cl.h2_viewent.state.drawflags &= ~message.ReadByte();
}

void CLH2_ParsePlaque(QMsg& message)
{
	int index = message.ReadShort();

	if (index > 0 && index <= prh2_string_count)
	{
		clh2_plaquemessage = &prh2_global_strings[prh2_string_index[index - 1]];
	}
	else
	{
		clh2_plaquemessage = "";
	}
}

void CLHW_IndexedPrint(QMsg& message)
{
	int i = message.ReadByte();
	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
	}

	int index = message.ReadShort();

	if (index > 0 && index <= prh2_string_count)
	{
		if (i == PRINT_CHAT)
		{
			common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE, &prh2_global_strings[prh2_string_index[index - 1]]);
		}
		else
		{
			common->Printf("%s",&prh2_global_strings[prh2_string_index[index - 1]]);
		}
	}
}

void CLHW_NamePrint(QMsg& message)
{
	int i = message.ReadByte();
	int index = message.ReadByte();

	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
	}

	if (index >= 0 && index < MAX_CLIENTS_QHW)
	{
		if (i == PRINT_CHAT)
		{
			common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE, cl.h2_players[index].name);
		}
		else
		{
			common->Printf("%s", cl.h2_players[index].name);
		}
	}
}

void CLH2_ParseParticleExplosion(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	short colour = message.ReadShort();
	short radius = message.ReadShort();
	short counter = message.ReadShort();

	CLH2_ColouredParticleExplosion(org, colour, radius, counter);
}

void CLH2_ParseSetViewTint(QMsg& message)
{
	int i = message.ReadByte();
	cl.h2_viewent.state.colormap = i;
}

void CLHw_ParseSetViewTint(QMsg& message)
{
	message.ReadByte();
	//cl.h2_viewent.colorshade = i;
}

void CLH2_ParseUpdateInv(QMsg& message)
{
	int sc1 = 0;
	int sc2 = 0;

	byte test = message.ReadByte();
	if (test & 1)
	{
		sc1 |= ((int)message.ReadByte());
	}
	if (test & 2)
	{
		sc1 |= ((int)message.ReadByte()) << 8;
	}
	if (test & 4)
	{
		sc1 |= ((int)message.ReadByte()) << 16;
	}
	if (test & 8)
	{
		sc1 |= ((int)message.ReadByte()) << 24;
	}
	if (test & 16)
	{
		sc2 |= ((int)message.ReadByte());
	}
	if (test & 32)
	{
		sc2 |= ((int)message.ReadByte()) << 8;
	}
	if (test & 64)
	{
		sc2 |= ((int)message.ReadByte()) << 16;
	}
	if (test & 128)
	{
		sc2 |= ((int)message.ReadByte()) << 24;
	}

	if (sc1 & SC1_HEALTH)
	{
		cl.h2_v.health = message.ReadShort();
	}
	if (sc1 & SC1_LEVEL)
	{
		cl.h2_v.level = message.ReadByte();
	}
	if (sc1 & SC1_INTELLIGENCE)
	{
		cl.h2_v.intelligence = message.ReadByte();
	}
	if (sc1 & SC1_WISDOM)
	{
		cl.h2_v.wisdom = message.ReadByte();
	}
	if (sc1 & SC1_STRENGTH)
	{
		cl.h2_v.strength = message.ReadByte();
	}
	if (sc1 & SC1_DEXTERITY)
	{
		cl.h2_v.dexterity = message.ReadByte();
	}
	if (sc1 & SC1_WEAPON)
	{
		cl.h2_v.weapon = message.ReadByte();
	}
	if (sc1 & SC1_BLUEMANA)
	{
		cl.h2_v.bluemana = message.ReadByte();
	}
	if (sc1 & SC1_GREENMANA)
	{
		cl.h2_v.greenmana = message.ReadByte();
	}
	if (sc1 & SC1_EXPERIENCE)
	{
		cl.h2_v.experience = message.ReadLong();
	}
	if (sc1 & SC1_CNT_TORCH)
	{
		cl.h2_v.cnt_torch = message.ReadByte();
	}
	if (sc1 & SC1_CNT_H_BOOST)
	{
		cl.h2_v.cnt_h_boost = message.ReadByte();
	}
	if (sc1 & SC1_CNT_SH_BOOST)
	{
		cl.h2_v.cnt_sh_boost = message.ReadByte();
	}
	if (sc1 & SC1_CNT_MANA_BOOST)
	{
		cl.h2_v.cnt_mana_boost = message.ReadByte();
	}
	if (sc1 & SC1_CNT_TELEPORT)
	{
		cl.h2_v.cnt_teleport = message.ReadByte();
	}
	if (sc1 & SC1_CNT_TOME)
	{
		cl.h2_v.cnt_tome = message.ReadByte();
	}
	if (sc1 & SC1_CNT_SUMMON)
	{
		cl.h2_v.cnt_summon = message.ReadByte();
	}
	if (sc1 & SC1_CNT_INVISIBILITY)
	{
		cl.h2_v.cnt_invisibility = message.ReadByte();
	}
	if (sc1 & SC1_CNT_GLYPH)
	{
		cl.h2_v.cnt_glyph = message.ReadByte();
	}
	if (sc1 & SC1_CNT_HASTE)
	{
		cl.h2_v.cnt_haste = message.ReadByte();
	}
	if (sc1 & SC1_CNT_BLAST)
	{
		cl.h2_v.cnt_blast = message.ReadByte();
	}
	if (sc1 & SC1_CNT_POLYMORPH)
	{
		cl.h2_v.cnt_polymorph = message.ReadByte();
	}
	if (sc1 & SC1_CNT_FLIGHT)
	{
		cl.h2_v.cnt_flight = message.ReadByte();
	}
	if (sc1 & SC1_CNT_CUBEOFFORCE)
	{
		cl.h2_v.cnt_cubeofforce = message.ReadByte();
	}
	if (sc1 & SC1_CNT_INVINCIBILITY)
	{
		cl.h2_v.cnt_invincibility = message.ReadByte();
	}
	if (sc1 & SC1_ARTIFACT_ACTIVE)
	{
		cl.h2_v.artifact_active = message.ReadFloat();
	}
	if (sc1 & SC1_ARTIFACT_LOW)
	{
		cl.h2_v.artifact_low = message.ReadFloat();
	}
	if (sc1 & SC1_MOVETYPE)
	{
		cl.h2_v.movetype = message.ReadByte();
	}
	if (sc1 & SC1_CAMERAMODE)
	{
		cl.h2_v.cameramode = message.ReadByte();
	}
	if (sc1 & SC1_HASTED)
	{
		cl.h2_v.hasted = message.ReadFloat();
	}
	if (sc1 & SC1_INVENTORY)
	{
		cl.h2_v.inventory = message.ReadByte();
	}
	if (sc1 & SC1_RINGS_ACTIVE)
	{
		cl.h2_v.rings_active = message.ReadFloat();
	}

	if (sc2 & SC2_RINGS_LOW)
	{
		cl.h2_v.rings_low = message.ReadFloat();
	}
	if (sc2 & SC2_AMULET)
	{
		cl.h2_v.armor_amulet = message.ReadByte();
	}
	if (sc2 & SC2_BRACER)
	{
		cl.h2_v.armor_bracer = message.ReadByte();
	}
	if (sc2 & SC2_BREASTPLATE)
	{
		cl.h2_v.armor_breastplate = message.ReadByte();
	}
	if (sc2 & SC2_HELMET)
	{
		cl.h2_v.armor_helmet = message.ReadByte();
	}
	if (sc2 & SC2_FLIGHT_T)
	{
		cl.h2_v.ring_flight = message.ReadByte();
	}
	if (sc2 & SC2_WATER_T)
	{
		cl.h2_v.ring_water = message.ReadByte();
	}
	if (sc2 & SC2_TURNING_T)
	{
		cl.h2_v.ring_turning = message.ReadByte();
	}
	if (sc2 & SC2_REGEN_T)
	{
		cl.h2_v.ring_regeneration = message.ReadByte();
	}
	if (sc2 & SC2_HASTE_T)
	{
		cl.h2_v.haste_time = message.ReadFloat();
	}
	if (sc2 & SC2_TOME_T)
	{
		cl.h2_v.tome_time = message.ReadFloat();
	}
	if (sc2 & SC2_PUZZLE1)
	{
		sprintf(cl.h2_puzzle_pieces[0], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE2)
	{
		sprintf(cl.h2_puzzle_pieces[1], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE3)
	{
		sprintf(cl.h2_puzzle_pieces[2], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE4)
	{
		sprintf(cl.h2_puzzle_pieces[3], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE5)
	{
		sprintf(cl.h2_puzzle_pieces[4], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE6)
	{
		sprintf(cl.h2_puzzle_pieces[5], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE7)
	{
		sprintf(cl.h2_puzzle_pieces[6], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE8)
	{
		sprintf(cl.h2_puzzle_pieces[7], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_MAXHEALTH)
	{
		cl.h2_v.max_health = message.ReadShort();
	}
	if (sc2 & SC2_MAXMANA)
	{
		cl.h2_v.max_mana = message.ReadByte();
	}
	if (sc2 & SC2_FLAGS)
	{
		cl.h2_v.flags = message.ReadFloat();
	}
	if (sc2 & SC2_OBJ)
	{
		cl.h2_info_mask = message.ReadLong();
	}
	if (sc2 & SC2_OBJ2)
	{
		cl.h2_info_mask2 = message.ReadLong();
	}

	if ((sc1 & SC1_INV) || (sc2 & SC2_INV))
	{
		SbarH2_InvChanged();
	}
}

void CLHW_ParseUpdateInv(QMsg& message)
{
	unsigned sc1 = 0;
	unsigned sc2 = 0;

	byte test = message.ReadByte();
	if (test & 1)
	{
		sc1 |= ((int)message.ReadByte());
	}
	if (test & 2)
	{
		sc1 |= ((int)message.ReadByte()) << 8;
	}
	if (test & 4)
	{
		sc1 |= ((int)message.ReadByte()) << 16;
	}
	if (test & 8)
	{
		sc1 |= ((int)message.ReadByte()) << 24;
	}
	if (test & 16)
	{
		sc2 |= ((int)message.ReadByte());
	}
	if (test & 32)
	{
		sc2 |= ((int)message.ReadByte()) << 8;
	}
	if (test & 64)
	{
		sc2 |= ((int)message.ReadByte()) << 16;
	}
	if (test & 128)
	{
		sc2 |= ((int)message.ReadByte()) << 24;
	}

	if (sc1 & SC1_HEALTH)
	{
		cl.h2_v.health = message.ReadShort();
	}
	if (sc1 & SC1_LEVEL)
	{
		cl.h2_v.level = message.ReadByte();
	}
	if (sc1 & SC1_INTELLIGENCE)
	{
		cl.h2_v.intelligence = message.ReadByte();
	}
	if (sc1 & SC1_WISDOM)
	{
		cl.h2_v.wisdom = message.ReadByte();
	}
	if (sc1 & SC1_STRENGTH)
	{
		cl.h2_v.strength = message.ReadByte();
	}
	if (sc1 & SC1_DEXTERITY)
	{
		cl.h2_v.dexterity = message.ReadByte();
	}
	if (sc1 & SC1_TELEPORT_TIME)
	{
		cl.h2_v.teleport_time = cls.realtime * 0.001 + 2;	//can't airmove for 2 seconds
	}
	if (sc1 & SC1_BLUEMANA)
	{
		cl.h2_v.bluemana = message.ReadByte();
	}
	if (sc1 & SC1_GREENMANA)
	{
		cl.h2_v.greenmana = message.ReadByte();
	}
	if (sc1 & SC1_EXPERIENCE)
	{
		cl.h2_v.experience = message.ReadLong();
	}
	if (sc1 & SC1_CNT_TORCH)
	{
		cl.h2_v.cnt_torch = message.ReadByte();
	}
	if (sc1 & SC1_CNT_H_BOOST)
	{
		cl.h2_v.cnt_h_boost = message.ReadByte();
	}
	if (sc1 & SC1_CNT_SH_BOOST)
	{
		cl.h2_v.cnt_sh_boost = message.ReadByte();
	}
	if (sc1 & SC1_CNT_MANA_BOOST)
	{
		cl.h2_v.cnt_mana_boost = message.ReadByte();
	}
	if (sc1 & SC1_CNT_TELEPORT)
	{
		cl.h2_v.cnt_teleport = message.ReadByte();
	}
	if (sc1 & SC1_CNT_TOME)
	{
		cl.h2_v.cnt_tome = message.ReadByte();
	}
	if (sc1 & SC1_CNT_SUMMON)
	{
		cl.h2_v.cnt_summon = message.ReadByte();
	}
	if (sc1 & SC1_CNT_INVISIBILITY)
	{
		cl.h2_v.cnt_invisibility = message.ReadByte();
	}
	if (sc1 & SC1_CNT_GLYPH)
	{
		cl.h2_v.cnt_glyph = message.ReadByte();
	}
	if (sc1 & SC1_CNT_HASTE)
	{
		cl.h2_v.cnt_haste = message.ReadByte();
	}
	if (sc1 & SC1_CNT_BLAST)
	{
		cl.h2_v.cnt_blast = message.ReadByte();
	}
	if (sc1 & SC1_CNT_POLYMORPH)
	{
		cl.h2_v.cnt_polymorph = message.ReadByte();
	}
	if (sc1 & SC1_CNT_FLIGHT)
	{
		cl.h2_v.cnt_flight = message.ReadByte();
	}
	if (sc1 & SC1_CNT_CUBEOFFORCE)
	{
		cl.h2_v.cnt_cubeofforce = message.ReadByte();
	}
	if (sc1 & SC1_CNT_INVINCIBILITY)
	{
		cl.h2_v.cnt_invincibility = message.ReadByte();
	}
	if (sc1 & SC1_ARTIFACT_ACTIVE)
	{
		cl.h2_v.artifact_active = message.ReadByte();
	}
	if (sc1 & SC1_ARTIFACT_LOW)
	{
		cl.h2_v.artifact_low = message.ReadByte();
	}
	if (sc1 & SC1_MOVETYPE)
	{
		cl.h2_v.movetype = message.ReadByte();
	}
	if (sc1 & SC1_CAMERAMODE)
	{
		cl.h2_v.cameramode = message.ReadByte();
	}
	if (sc1 & SC1_HASTED)
	{
		cl.h2_v.hasted = message.ReadFloat();
	}
	if (sc1 & SC1_INVENTORY)
	{
		cl.h2_v.inventory = message.ReadByte();
	}
	if (sc1 & SC1_RINGS_ACTIVE)
	{
		cl.h2_v.rings_active = message.ReadByte();
	}

	if (sc2 & SC2_RINGS_LOW)
	{
		cl.h2_v.rings_low = message.ReadByte();
	}
	if (sc2 & SC2_AMULET)
	{
		cl.h2_v.armor_amulet = message.ReadByte();
	}
	if (sc2 & SC2_BRACER)
	{
		cl.h2_v.armor_bracer = message.ReadByte();
	}
	if (sc2 & SC2_BREASTPLATE)
	{
		cl.h2_v.armor_breastplate = message.ReadByte();
	}
	if (sc2 & SC2_HELMET)
	{
		cl.h2_v.armor_helmet = message.ReadByte();
	}
	if (sc2 & SC2_FLIGHT_T)
	{
		cl.h2_v.ring_flight = message.ReadByte();
	}
	if (sc2 & SC2_WATER_T)
	{
		cl.h2_v.ring_water = message.ReadByte();
	}
	if (sc2 & SC2_TURNING_T)
	{
		cl.h2_v.ring_turning = message.ReadByte();
	}
	if (sc2 & SC2_REGEN_T)
	{
		cl.h2_v.ring_regeneration = message.ReadByte();
	}
	if (sc2 & SC2_PUZZLE1)
	{
		sprintf(cl.h2_puzzle_pieces[0], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE2)
	{
		sprintf(cl.h2_puzzle_pieces[1], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE3)
	{
		sprintf(cl.h2_puzzle_pieces[2], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE4)
	{
		sprintf(cl.h2_puzzle_pieces[3], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE5)
	{
		sprintf(cl.h2_puzzle_pieces[4], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE6)
	{
		sprintf(cl.h2_puzzle_pieces[5], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE7)
	{
		sprintf(cl.h2_puzzle_pieces[6], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_PUZZLE8)
	{
		sprintf(cl.h2_puzzle_pieces[7], "%.9s", message.ReadString2());
	}
	if (sc2 & SC2_MAXHEALTH)
	{
		cl.h2_v.max_health = message.ReadShort();
	}
	if (sc2 & SC2_MAXMANA)
	{
		cl.h2_v.max_mana = message.ReadByte();
	}
	if (sc2 & SC2_FLAGS)
	{
		cl.h2_v.flags = message.ReadFloat();
	}

	if ((sc1 & SC1_INV) || (sc2 & SC2_INV))
	{
		SbarH2_InvChanged();
	}
}

void CLHW_ParseUpdatePiv(QMsg& message)
{
	cl.hw_PIV = message.ReadLong();
}

void CLHW_ParsePlayerSound(QMsg& message)
{
	byte entityNumber = message.ReadByte();
	vec3_t pos;
	message.ReadPos(pos);
	int soundNum = message.ReadShort();

	S_StartSound(pos, entityNumber, 1, cl.sound_precache[soundNum], 1.0, 1.0);
}
