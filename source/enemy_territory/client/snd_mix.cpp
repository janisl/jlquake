/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		snd_mix.c
 *
 * desc:		portable code to mix sounds for snd_dma.c
 *
 * $Archive: /Wolfenstein MP/src/client/snd_mix.c $
 *
 *****************************************************************************/

#include "snd_local.h"

static int snd_vol;

#if defined __linux__ && defined __i386__

// snd_mixa.s
void S_WriteLinearBlastStereo16( void );

#elif id386

__declspec( naked ) void S_WriteLinearBlastStereo16( void ) {
	__asm {

		push edi
		push ebx
		mov ecx,ds : dword ptr[snd_linear_count]
		mov ebx,ds : dword ptr[snd_p]
		mov edi,ds : dword ptr[snd_out]
LWLBLoopTop:
		mov eax,ds : dword ptr[-8 + ebx + ecx * 4]
		sar eax,8
		cmp eax,07FFFh
		jg LClampHigh
		cmp eax,0FFFF8000h
		jnl LClampDone
		mov eax,0FFFF8000h
		jmp LClampDone
LClampHigh:
		mov eax,07FFFh
LClampDone:
		mov edx,ds : dword ptr[-4 + ebx + ecx * 4]
		sar edx,8
		cmp edx,07FFFh
		jg LClampHigh2
		cmp edx,0FFFF8000h
		jnl LClampDone2
		mov edx,0FFFF8000h
		jmp LClampDone2
LClampHigh2:
		mov edx,07FFFh
LClampDone2:
		shl edx,16
		and eax,0FFFFh
		or edx,eax
		mov ds : dword ptr[-4 + edi + ecx * 2],edx
		sub ecx,2
		jnz LWLBLoopTop
		pop ebx
		pop edi
		ret
	}
}

#else

/*
===================
S_WriteLinearBlastStereo16
===================
*/
void S_WriteLinearBlastStereo16( void ) {
	int i;
	int val;

	for ( i = 0 ; i < snd_linear_count ; i += 2 )
	{
		val = snd_p[i] >> 8;
		if ( val > 0x7fff ) {
			snd_out[i] = 0x7fff;
		} else if ( val < (short)0x8000 ) {
			snd_out[i] = (short)0x8000;
		} else {
			snd_out[i] = val;
		}

		val = snd_p[i + 1] >> 8;
		if ( val > 0x7fff ) {
			snd_out[i + 1] = 0x7fff;
		} else if ( val < (short)0x8000 ) {
			snd_out[i + 1] = (short)0x8000;
		} else {
			snd_out[i + 1] = val;
		}
	}
}

#endif

/*
===================
S_TransferStereo16
===================
*/
void S_TransferStereo16( unsigned long *pbuf, int endtime ) {
	int lpos;
	int ls_paintedtime;

	snd_p = (int *) paintbuffer;
	ls_paintedtime = s_paintedtime;

	while ( ls_paintedtime < endtime )
	{
		// handle recirculating buffer issues
		lpos = ls_paintedtime & ( ( dma.samples >> 1 ) - 1 );

		snd_out = (short *) pbuf + ( lpos << 1 );

		snd_linear_count = ( dma.samples >> 1 ) - lpos;
		if ( ls_paintedtime + snd_linear_count > endtime ) {
			snd_linear_count = endtime - ls_paintedtime;
		}

		snd_linear_count <<= 1;

		// write a linear blast of samples
		S_WriteLinearBlastStereo16();

		snd_p += snd_linear_count;
		ls_paintedtime += ( snd_linear_count >> 1 );
	}
}

/*
===================
S_TransferPaintBuffer
===================
*/
void S_TransferPaintBuffer( int endtime ) {
	int out_idx;
	int count;
	int out_mask;
	int     *p;
	int step;
	int val;
	unsigned long *pbuf;

	pbuf = (unsigned long *)dma.buffer;
	if ( !pbuf ) {
		return;
	}

	if ( s_testsound->integer ) {
		int i;
		int count;

		// write a fixed sine wave
		count = ( endtime - s_paintedtime );
		for ( i = 0 ; i < count ; i++ )
			paintbuffer[i].left = paintbuffer[i].right = sin( ( s_paintedtime + i ) * 0.1 ) * 20000 * 256;
	}


	if ( dma.samplebits == 16 && dma.channels == 2 ) {
		// optimized case
		S_TransferStereo16( pbuf, endtime );
	} else
	{   // general case
		p = (int *) paintbuffer;
		count = ( endtime - s_paintedtime ) * dma.channels;
		out_mask = dma.samples - 1;
		out_idx = s_paintedtime * dma.channels & out_mask;
		step = 3 - dma.channels;

		if ( dma.samplebits == 16 ) {
			short *out = (short *) pbuf;
			while ( count-- )
			{
				val = *p >> 8;
				p += step;
				if ( val > 0x7fff ) {
					val = 0x7fff;
				} else if ( val < -32768 ) {
					val = -32768;
				}
				out[out_idx] = val;
				out_idx = ( out_idx + 1 ) & out_mask;
			}
		} else if ( dma.samplebits == 8 )     {
			unsigned char *out = (unsigned char *) pbuf;
			while ( count-- )
			{
				val = *p >> 8;
				p += step;
				if ( val > 0x7fff ) {
					val = 0x7fff;
				} else if ( val < -32768 ) {
					val = -32768;
				}
				out[out_idx] = ( val >> 8 ) + 128;
				out_idx = ( out_idx + 1 ) & out_mask;
			}
		}
	}
}

/*
===============================================================================

LIP SYNCING

===============================================================================
*/

#ifdef TALKANIM

unsigned char s_entityTalkAmplitude[MAX_CLIENTS_ET];

/*
===================
S_SetVoiceAmplitudeFrom16
===================
*/
void S_SetVoiceAmplitudeFrom16( const sfx_t *sc, int sampleOffset, int count, int entnum ) {
	int data, i, sfx_count;
	short *samples;

	if ( count <= 0 ) {
		return; // must have gone ahead of the end of the sound
	}

	sfx_count = 0;
	samples = sc->Data;
	for ( i = 0; i < count; i++ ) {
		data  = samples[sampleOffset++];
		if ( abs( data ) > 5000 ) {
			sfx_count += ( data * 255 ) >> 8;
		}
	}
	//Com_Printf("Voice sfx_count = %d, count = %d\n", sfx_count, count );
	// adjust the sfx_count according to the frametime (scale down for longer frametimes)
	sfx_count = abs( sfx_count );
	sfx_count = (int)( (float)sfx_count / ( 2.0 * (float)count ) );
	if ( sfx_count > 255 ) {
		sfx_count = 255;
	}
	if ( sfx_count < 25 ) {
		sfx_count = 0;
	}
	//Com_Printf("sfx_count = %d\n", sfx_count );
	// update the amplitude for this entity
	s_entityTalkAmplitude[entnum] = (unsigned char)sfx_count;
}

/*
===================
S_GetVoiceAmplitude
===================
*/
int S_GetVoiceAmplitude( int entityNum ) {
	if ( entityNum >= MAX_CLIENTS_ET ) {
		Com_Printf( "Error: S_GetVoiceAmplitude() called for a non-client\n" );
		return 0;
	}

	return (int)s_entityTalkAmplitude[entityNum];
}
#else

// NERVE - SMF
int S_GetVoiceAmplitude( int entityNum ) {
	return 0;
}
// -NERVE - SMF

#endif

/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

/*
===================
S_PaintChannelFrom16
===================
*/
static void S_PaintChannelFrom16( channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int data, aoff, boff;
	int leftvol, rightvol;
	int i, j;
	portable_samplepair_t   *samp;
	short                   *samples;
	float ooff, fdata, fdiv, fleftvol, frightvol;

	samp = &paintbuffer[ bufferOffset ];

	if ( ch->doppler ) {
		sampleOffset = sampleOffset * ch->oldDopplerScale;
	}

	if ( !ch->doppler ) {
		leftvol = ch->leftvol * snd_vol;
		rightvol = ch->rightvol * snd_vol;

		samples = sc->Data;
		for ( i = 0; i < count; i++ ) {
			data  = samples[sampleOffset++];
			samp[i].left += ( data * leftvol ) >> 8;
			samp[i].right += ( data * rightvol ) >> 8;
		}
	} else {
		fleftvol = ch->leftvol * snd_vol;
		frightvol = ch->rightvol * snd_vol;

		ooff = sampleOffset;
		samples = sc->Data;

		for ( i = 0 ; i < count ; i++ ) {
			aoff = ooff;
			ooff = ooff + ch->dopplerScale;
			boff = ooff;
			fdata = 0;
			for ( j = aoff; j < boff; j++ ) {
				fdata += samples[j];
			}
			fdiv = 256 * ( boff - aoff );
			samp[i].left += ( fdata * fleftvol ) / fdiv;
			samp[i].right += ( fdata * frightvol ) / fdiv;
		}
	}
}

#define TALK_FUTURE_SEC 0.25        // go this far into the future (seconds)

//bani - cl_main.c
void CL_WriteWaveFilePacket( int endtime );

/*
===================
S_PaintChannels
===================
*/
void S_PaintChannels( int endtime ) {
	int i, si;
	int end;
	channel_t *ch;
	sfx_t   *sc;
	int ltime, count;
	int sampleOffset;
	streamingSound_t *ss;
	qboolean firstPass = qtrue;

	if ( s_mute->value ) {
		snd_vol = 0;
	} else {
		snd_vol = s_volume->value * 256;
	}

	if ( snd.volCurrent < 1 ) { // only when fading (at map start/end)
		snd_vol = (int)( (float)snd_vol * snd.volCurrent );
	}

	//Com_Printf ("%i to %i\n", s_paintedtime, endtime);
	while ( s_paintedtime < endtime ) {
		// if paintbuffer is smaller than DMA buffer
		// we may need to fill it multiple times
		end = endtime;
		if ( endtime - s_paintedtime > PAINTBUFFER_SIZE ) {
			//%	Com_DPrintf("endtime exceeds PAINTBUFFER_SIZE %i\n", endtime - s_paintedtime);
			end = s_paintedtime + PAINTBUFFER_SIZE;
		}

		// clear paint buffer for the current time
		Com_Memset( paintbuffer, 0, ( end - s_paintedtime ) * sizeof( portable_samplepair_t ) );
		// mix all streaming sounds into paint buffer
		for ( si = 0, ss = streamingSounds; si < MAX_STREAMING_SOUNDS; si++, ss++ ) {
			// if this streaming sound is still playing
			if ( s_rawend[si] >= s_paintedtime ) {
				// copy from the streaming sound source
				int s;
				int stop;
//				float	fsir, fsil; // TTimo: unused

				stop = ( end < s_rawend[si] ) ? end : s_rawend[si];

				// precalculating this saves zillions of cycles
//DAJ				fsir = ((float)s_rawVolume[si].left/255.0f);
//DAJ				fsil = ((float)s_rawVolume[si].right/255.0f);
				for ( i = s_paintedtime ; i < stop ; i++ ) {
					s = i & ( MAX_RAW_SAMPLES - 1 );
//DAJ					paintbuffer[i-s_paintedtime].left += (int)((float)s_rawsamples[si][s].left * fsir);
//DAJ					paintbuffer[i-s_paintedtime].right += (int)((float)s_rawsamples[si][s].right * fsil);
					//DAJ even faster
					paintbuffer[i - s_paintedtime].left += ( s_rawsamples[si][s].left * s_rawVolume[si].left ) >> 8;
					paintbuffer[i - s_paintedtime].right += ( s_rawsamples[si][s].right * s_rawVolume[si].right ) >> 8;
				}
#ifdef TALKANIM
				if ( firstPass && ss->channel == CHAN_VOICE && ss->entnum < MAX_CLIENTS_ET ) {
					int talkcnt, talktime;
					int sfx_count, vstop;
					int data;

					// we need to go into the future, since the interpolated behaviour of the facial
					// animation creates lag in the time it takes to display the current facial frame
					talktime = s_paintedtime + (int)( TALK_FUTURE_SEC * (float)s_khz->integer * 1000 );
					vstop = ( talktime + 100 < s_rawend[si] ) ? talktime + 100 : s_rawend[si];
					talkcnt = 1;
					sfx_count = 0;

					for ( i = talktime ; i < vstop ; i++ ) {
						s = i & ( MAX_RAW_SAMPLES - 1 );
						data = abs( ( s_rawsamples[si][s].left ) / 8000 );
						if ( data > sfx_count ) {
							sfx_count = data;
						}
					}

					if ( sfx_count > 255 ) {
						sfx_count = 255;
					}
					if ( sfx_count < 25 ) {
						sfx_count = 0;
					}

					//Com_Printf("sfx_count = %d\n", sfx_count );

					// update the amplitude for this entity
					// rain - the announcer is ent -1, so make sure we're >= 0
					if ( ss->entnum >= 0 ) {
						s_entityTalkAmplitude[ss->entnum] = (unsigned char)sfx_count;
					}
				}
#endif
			}
		}

		// paint in the channels.
		ch = s_channels;
		for ( i = 0; i < MAX_CHANNELS; i++, ch++ ) {
			if ( ch->startSample == START_SAMPLE_IMMEDIATE || !ch->sfx || ( ch->leftvol < 0.25 && ch->rightvol < 0.25 ) ) {
				continue;
			}

			ltime = s_paintedtime;
			sc = ch->sfx;

			// (SA) hmm, why was this commented out?
			if ( !sc->InMemory ) {
				S_memoryLoad( sc );
			}

			sampleOffset = ltime - ch->startSample;
			count = end - ltime;
			if ( sampleOffset + count > sc->Length ) {
				count = sc->Length - sampleOffset;
			}

			if ( count > 0 ) {
#ifdef TALKANIM
				// Ridah, talking animations
				// TODO: check that this entity has talking animations enabled!
				if ( firstPass && ch->entchannel == CHAN_VOICE && ch->entnum < MAX_CLIENTS_ET ) {
					int talkofs, talkcnt, talktime;
					// we need to go into the future, since the interpolated behaviour of the facial
					// animation creates lag in the time it takes to display the current facial frame
					talktime = ltime + (int)( TALK_FUTURE_SEC * (float)s_khz->integer * 1000 );
					talkofs = talktime - ch->startSample;
					talkcnt = 100;
					if ( talkofs + talkcnt < sc->Length ) {
						S_SetVoiceAmplitudeFrom16( sc, talkofs, talkcnt, ch->entnum );
					}
				}
#endif
				S_PaintChannelFrom16( ch, sc, count, sampleOffset, ltime - s_paintedtime );
			}
		}

		// paint in the looped channels.
		ch = loop_channels;
		for ( i = 0; i < numLoopChannels ; i++, ch++ ) {
			if ( !ch->sfx || ( !ch->leftvol && !ch->rightvol ) ) {
				continue;
			}

			ltime = s_paintedtime;
			sc = ch->sfx;

			if ( sc->Data == NULL || sc->Length == 0 ) {
				continue;
			}
			// we might have to make two passes if it
			// is a looping sound effect and the end of
			// the sample is hit
			do {
				//%	sampleOffset = (ltime % sc->soundLength);
				//%	sampleOffset = (ltime - ch->startSample) % sc->soundLength;	// ydnar
				sampleOffset = ( ltime /*- ch->startSample*/ ) % sc->Length; // ydnar

				count = end - ltime;
				if ( sampleOffset + count > sc->Length ) {
					count = sc->Length - sampleOffset;
				}

				if ( count > 0 ) {
#ifdef TALKANIM
					// Ridah, talking animations
					// TODO: check that this entity has talking animations enabled!
					if ( firstPass && ch->entchannel == CHAN_VOICE && ch->entnum < MAX_CLIENTS_ET ) {
						int talkofs, talkcnt, talktime;
						// we need to go into the future, since the interpolated behaviour of the facial
						// animation creates lag in the time it takes to display the current facial frame
						talktime = ltime + (int)( TALK_FUTURE_SEC * (float)s_khz->integer * 1000 );
						talkofs = talktime % sc->Length;
						talkcnt = 100;
						if ( talkofs + talkcnt < sc->Length ) {
							S_SetVoiceAmplitudeFrom16( sc, talkofs, talkcnt, ch->entnum );
						}
					}
#endif
					S_PaintChannelFrom16( ch, sc, count, sampleOffset, ltime - s_paintedtime );
					ltime += count;
				}
			} while ( ltime < end );
		}

		// transfer out according to DMA format
		S_TransferPaintBuffer( end );
		//bani
		CL_WriteWaveFilePacket( end );
		s_paintedtime = end;
		firstPass = qfalse;
	}
}
