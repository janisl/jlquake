/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
*(at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <alsa/asoundlib.h>

#include "quakedef.h"

#define BUFFER_SAMPLES 4096
#define SUBMISSION_CHUNK BUFFER_SAMPLES / 2

static snd_pcm_t *pcm_handle;
static snd_pcm_hw_params_t *hw_params;

static struct sndinfo * si;

static int sample_bytes;
static int buffer_bytes;

QCvar *sndbits;
QCvar *sndspeed;
QCvar *sndchannels;
QCvar *snddevice;


/*
*  The sample rates which will be attempted.
*/
static int RATES[] = {
	44100, 22050, 11025, 8000
};

/*
*  Initialize ALSA pcm device, and bind it to sndinfo.
*/
qboolean SNDDMA_Init(void){
	int i, err, dir;
	unsigned int r;
	snd_pcm_uframes_t p;

	shm = &sn;

	if (!snddevice)
	{
		sndbits = Cvar_Get("sndbits", "16", CVAR_ARCHIVE);
		sndspeed = Cvar_Get("sndspeed", "0", CVAR_ARCHIVE);
		sndchannels = Cvar_Get("sndchannels", "2", CVAR_ARCHIVE);
		snddevice = Cvar_Get("snddevice", "/dev/dsp", CVAR_ARCHIVE);
	}

	if(!strcmp(snddevice->string, "/dev/dsp"))  //silly oss default
	snddevice->string = "default";

	if((err = snd_pcm_open(&pcm_handle, snddevice->string,
				SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
	{
		Con_Printf("ALSA: cannot open device %s(%s)\n",
			snddevice->string, snd_strerror(err));
		return false;
	}

	if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0){
	Con_Printf("ALSA: cannot allocate hw params(%s)\n",
			snd_strerror(err));
	return false;
	}

	if((err = snd_pcm_hw_params_any(pcm_handle, hw_params)) < 0){
	Con_Printf("ALSA: cannot init hw params(%s)\n", snd_strerror(err));
	snd_pcm_hw_params_free(hw_params);
	return false;
	}

	if((err = snd_pcm_hw_params_set_access
		(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		Con_Printf("ALSA: cannot set access(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	shm->samplebits = (int)sndbits->value;
	if(shm->samplebits != 8){  //try 16 by default
	shm->samplebits = 16;  //ensure this is set for other calculations

	if((err = snd_pcm_hw_params_set_format(pcm_handle, hw_params,
						SND_PCM_FORMAT_S16)) < 0){
		Con_Printf("ALSA: 16 bit not supported, trying 8\n");
		shm->samplebits = 8;
	}
	}
	if(shm->samplebits == 8){  //or 8 if specifically asked to
	if((err = snd_pcm_hw_params_set_format(pcm_handle, hw_params,
						SND_PCM_FORMAT_U8)) < 0){
		Con_Printf("ALSA: cannot set format(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}
	}

	shm->speed = (int)sndspeed->value;
	if(shm->speed){  //try specified rate
	r = shm->speed;

	if((err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &r, &dir)) < 0)
		Con_Printf("ALSA: cannot set rate %d(%s)\n", r, snd_strerror(err));
	else {  //rate succeeded, but is perhaps slightly different
		if(dir != 0)
		Con_Printf("ALSA: rate %d not supported, using %d\n", sndspeed->value, r);
		shm->speed = r;
	}
	}
	if(!shm->speed){  //or all available ones
	for(i = 0; i < sizeof(RATES); i++){
		r = RATES[i];
		dir = 0;

		if((err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &r, &dir)) < 0)
		Con_Printf("ALSA: cannot set rate %d(%s)\n", r, snd_strerror(err));
		else {  //rate succeeded, but is perhaps slightly different
		shm->speed = r;
		if(dir != 0)
		Con_Printf("ALSA: rate %d not supported, using %d\n", RATES[i], r);
		break;
		}
	}
	}
	if(!shm->speed){  //failed
	Con_Printf("ALSA: cannot set rate\n");
	snd_pcm_hw_params_free(hw_params);
	return false;
	}

	shm->channels = sndchannels->value;
	if(shm->channels < 1 || shm->channels > 2)
	shm->channels = 2;  //ensure either stereo or mono



	if((err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params,
						shm->channels)) < 0)
	{
		Con_Printf("ALSA: cannot set channels %d(%s)\n",
			sndchannels->value, snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	p = BUFFER_SAMPLES / shm->channels;
	if((err = snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params,
							&p, &dir)) < 0){
	Con_Printf("ALSA: cannot set period size (%s)\n", snd_strerror(err));
	snd_pcm_hw_params_free(hw_params);
	return false;
	}
	else {  //rate succeeded, but is perhaps slightly different
	if(dir != 0)
		Con_Printf("ALSA: period %d not supported, using %d\n", (BUFFER_SAMPLES/shm->channels), p);
	}

	if((err = snd_pcm_hw_params(pcm_handle, hw_params)) < 0){  //set params
	Con_Printf("ALSA: cannot set params(%s)\n", snd_strerror(err));
	snd_pcm_hw_params_free(hw_params);
	return false;
	}

	sample_bytes = shm->samplebits / 8;
	buffer_bytes = BUFFER_SAMPLES * sample_bytes;

	shm->buffer = (byte*)malloc(buffer_bytes);  //allocate pcm frame buffer
	memset(shm->buffer, 0, buffer_bytes);

	shm->samplepos = 0;

	shm->samples = BUFFER_SAMPLES;
	shm->submission_chunk = SUBMISSION_CHUNK;

	snd_pcm_prepare(pcm_handle);

	return true;
}

/*
*  Returns the current sample position, if sound is running.
*/
int SNDDMA_GetDMAPos(void){

	if(shm->buffer)
	return shm->samplepos;

	Con_Printf("Sound not inizialized\n");
	return 0;
}

/*
*  Closes the ALSA pcm device and frees the dma buffer.
*/
void SNDDMA_Shutdown(void){

	if(shm->buffer){
	snd_pcm_drop(pcm_handle);
	snd_pcm_close(pcm_handle);
	}

	free(shm->buffer);
	shm->buffer = 0;
}

/*
*  Writes the dma buffer to the ALSA pcm device.
*/
void SNDDMA_Submit(void){
	int s, w, frames;
	void *start;

	if(!shm->buffer)
	return;

	s = shm->samplepos * sample_bytes;
	start = (void *)&shm->buffer[s];

	frames = shm->submission_chunk / shm->channels;

	if((w = snd_pcm_writei(pcm_handle, start, frames)) < 0){  //write to card
		snd_pcm_prepare(pcm_handle);  //xrun occured
		return;
	}

	shm->samplepos += w * shm->channels;  //mark progress

	if(shm->samplepos >= shm->samples)
	shm->samplepos = 0;  //wrap buffer
}

/*
*  Callback provided by the engine in case we need it.  We don't.
*/
void SNDDMA_BeginPainting(void){}
