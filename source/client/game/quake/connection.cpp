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

static byte* clqw_upload_data;
static int clqw_upload_pos;
static int clqw_upload_size;

//	An q1svc_signonnum has been received, perform a client side setup
void CLQ1_SignonReply()
{
	char str[8192];

	common->DPrintf("CLQ1_SignonReply: %i\n", clc.qh_signon);

	switch (clc.qh_signon)
	{
	case 1:
		CL_AddReliableCommand("prespawn");
		break;

	case 2:
		CL_AddReliableCommand(va("name \"%s\"\n", clqh_name->string));
		CL_AddReliableCommand(va("color %i %i\n", clqh_color->integer >> 4, clqh_color->integer & 15));
		sprintf(str, "spawn %s", cls.qh_spawnparms);
		CL_AddReliableCommand(str);
		break;

	case 3:
		CL_AddReliableCommand("begin");
		break;

	case 4:
		SCR_EndLoadingPlaque();		// allow normal screen updates
		break;
	}
}

//	Returns true if the file exists, otherwise it attempts
// to start a download from the server.
bool CLQW_CheckOrDownloadFile(const char* filename)
{
	fileHandle_t f;

	if (strstr(filename, ".."))
	{
		common->Printf("Refusing to download a path with ..\n");
		return true;
	}

	FS_FOpenFileRead(filename, &f, true);
	if (f)
	{
		// it exists, no need to download
		FS_FCloseFile(f);
		return true;
	}

	//ZOID - can't download when recording
	if (clc.demorecording)
	{
		common->Printf("Unable to download %s in record mode.\n", clc.downloadName);
		return true;
	}
	//ZOID - can't download when playback
	if (clc.demoplaying)
	{
		return true;
	}

	String::Cpy(clc.downloadName, filename);
	common->Printf("Downloading %s...\n", clc.downloadName);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	String::StripExtension(clc.downloadName, clc.downloadTempName);
	String::Cat(clc.downloadTempName, sizeof(clc.downloadTempName), ".tmp");

	CL_AddReliableCommand(va("download %s", clc.downloadName));

	clc.downloadNumber++;

	return false;
}

int CLQW_CalcNet()
{
	for (int i = clc.netchan.outgoingSequence - UPDATE_BACKUP_QW + 1
		 ; i <= clc.netchan.outgoingSequence
		 ; i++)
	{
		qwframe_t* frame = &cl.qw_frames[i & UPDATE_MASK_QW];
		if (frame->receivedtime == -1)
		{
			clqh_packet_latency[i & NET_TIMINGSMASK_QH] = 9999;		// dropped
		}
		else if (frame->receivedtime == -2)
		{
			clqh_packet_latency[i & NET_TIMINGSMASK_QH] = 10000;	// choked
		}
		else if (frame->invalid)
		{
			clqh_packet_latency[i & NET_TIMINGSMASK_QH] = 9998;		// invalid delta
		}
		else
		{
			clqh_packet_latency[i & NET_TIMINGSMASK_QH] = (frame->receivedtime - frame->senttime) * 20;
		}
	}

	int lost = 0;
	for (int a = 0; a < NET_TIMINGS_QH; a++)
	{
		int i = (clc.netchan.outgoingSequence - a) & NET_TIMINGSMASK_QH;
		if (clqh_packet_latency[i] == 9999)
		{
			lost++;
		}
	}
	return lost * 100 / NET_TIMINGS_QH;
}

void CLQW_NextUpload()
{
	if (!clqw_upload_data)
	{
		return;
	}

	int r = clqw_upload_size - clqw_upload_pos;
	if (r > 768)
	{
		r = 768;
	}
	byte buffer[1024];
	Com_Memcpy(buffer, clqw_upload_data + clqw_upload_pos, r);
	clc.netchan.message.WriteByte(qwclc_upload);
	clc.netchan.message.WriteShort(r);

	clqw_upload_pos += r;
	int size = clqw_upload_size;
	if (!size)
	{
		size = 1;
	}
	int percent = clqw_upload_pos * 100 / size;
	clc.netchan.message.WriteByte(percent);
	clc.netchan.message.WriteData(buffer, r);

	common->DPrintf("UPLOAD: %6d: %d written\n", clqw_upload_pos - r, r);

	if (clqw_upload_pos != clqw_upload_size)
	{
		return;
	}

	common->Printf("Upload completed\n");

	Mem_Free(clqw_upload_data);
	clqw_upload_data = 0;
	clqw_upload_pos = clqw_upload_size = 0;
}

void CLQW_StartUpload(const byte* data, int size)
{
	if (cls.state < CA_LOADING)
	{
		return;	// gotta be connected

	}
	// override
	if (clqw_upload_data)
	{
		free(clqw_upload_data);
	}

	common->DPrintf("Upload starting of %d...\n", size);

	clqw_upload_data = (byte*)Mem_Alloc(size);
	Com_Memcpy(clqw_upload_data, data, size);
	clqw_upload_size = size;
	clqw_upload_pos = 0;

	CLQW_NextUpload();
}

bool CLQW_IsUploading()
{
	if (clqw_upload_data)
	{
		return true;
	}
	return false;
}

void CLQW_StopUpload()
{
	if (clqw_upload_data)
	{
		Mem_Free(clqw_upload_data);
	}
	clqw_upload_data = NULL;
}
