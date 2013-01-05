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

/* Additional features that would be nice for this code:
    * Only display <gamepath>/<file>, i.e., etpro/etpro-3_0_1.pk3 in the UI.
    * Add server as referring URL
*/

#include <curl/curl.h>

#include "dl_public.h"
#include "../../../common/Common.h"
#include "../../../common/strings.h"
#include "../../../common/console_variable.h"

#define APP_NAME        "ID_DOWNLOAD"
#define APP_VERSION     "2.0"

// initialize once
static bool dl_initialized = false;

static CURLM* dl_multi = NULL;
static CURL* dl_request = NULL;
static FILE* dl_file = NULL;

static size_t DL_cb_FWriteFile(void* ptr, size_t size, size_t nmemb, void* stream)
{
	FILE* file = (FILE*)stream;
	return fwrite(ptr, size, nmemb, file);
}

static int DL_cb_Progress(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	//	cl_downloadSize and cl_downloadTime are set by the Q3 protocol...
	// and it would probably be expensive to verify them here.   -zinx */

	Cvar_SetValue("cl_downloadCount", (float)dlnow);
	return 0;
}

static void DL_InitDownload()
{
	if (dl_initialized)
	{
		return;
	}

	//	Make sure curl has initialized, so the cleanup doesn't get confused
	curl_global_init(CURL_GLOBAL_ALL);

	dl_multi = curl_multi_init();

	common->Printf("Client download subsystem initialized\n");
	dl_initialized = true;
}

void DL_Shutdown()
{
	if (!dl_initialized)
	{
		return;
	}

	curl_multi_cleanup(dl_multi);
	dl_multi = NULL;

	curl_global_cleanup();

	dl_initialized = false;
}

//	inspired from http://www.w3.org/Library/Examples/LoadToFile.c
//	setup the download, return once we have a connection
bool DL_BeginDownload(const char* localName, const char* remoteName, int debug)
{
	if (dl_request)
	{
		common->Printf("ERROR: DL_BeginDownload called with a download request already active\n"); \
		return false;
	}

	if (!localName || !remoteName)
	{
		common->DPrintf("Empty download URL or empty local file name\n");
		return false;
	}

	FS_CreatePath(localName);
	dl_file = fopen(localName, "wb+");
	if (!dl_file)
	{
		common->Printf("ERROR: DL_BeginDownload unable to open '%s' for writing\n", localName);
		return false;
	}

	DL_InitDownload();

	/* ET://ip:port */
	char referer[MAX_STRING_CHARS + 5 /*"ET://"*/];
	String::Cpy(referer, "ET://");
	String::NCpyZ(referer + 5, Cvar_VariableString("cl_currentServerIP"), MAX_STRING_CHARS);

	dl_request = curl_easy_init();
	curl_easy_setopt(dl_request, CURLOPT_USERAGENT, va("%s %s", APP_NAME "/" APP_VERSION, curl_version()));
	curl_easy_setopt(dl_request, CURLOPT_REFERER, referer);
	curl_easy_setopt(dl_request, CURLOPT_URL, remoteName);
	curl_easy_setopt(dl_request, CURLOPT_WRITEFUNCTION, DL_cb_FWriteFile);
	curl_easy_setopt(dl_request, CURLOPT_WRITEDATA, (void*)dl_file);
	curl_easy_setopt(dl_request, CURLOPT_PROGRESSFUNCTION, DL_cb_Progress);
	curl_easy_setopt(dl_request, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(dl_request, CURLOPT_FAILONERROR, 1);

	curl_multi_add_handle(dl_multi, dl_request);

	Cvar_Set("cl_downloadName", remoteName);

	return true;
}

// (maybe this should be CL_DL_DownloadLoop)
dlStatus_t DL_DownloadLoop()
{
	if (!dl_request)
	{
		common->DPrintf("DL_DownloadLoop: unexpected call with dl_request == NULL\n");
		return DL_DONE;
	}

	int dls = 0;
	if (curl_multi_perform(dl_multi, &dls) == CURLM_CALL_MULTI_PERFORM && dls)
	{
		return DL_CONTINUE;
	}

	CURLMsg* msg;
	while ((msg = curl_multi_info_read(dl_multi, &dls)) && msg->easy_handle != dl_request)
		;

	if (!msg || msg->msg != CURLMSG_DONE)
	{
		return DL_CONTINUE;
	}

	const char* err = NULL;
	if (msg->data.result != CURLE_OK)
	{
		err = curl_easy_strerror(msg->data.result);
	}
	else
	{
		err = NULL;
	}

	curl_multi_remove_handle(dl_multi, dl_request);
	curl_easy_cleanup(dl_request);

	fclose(dl_file);
	dl_file = NULL;

	dl_request = NULL;

	Cvar_Set("ui_dl_running", "0");

	if (err)
	{
		common->DPrintf("DL_DownloadLoop: request terminated with failure status '%s'\n", err);
		return DL_FAILED;
	}

	return DL_DONE;
}
