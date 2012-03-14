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

extern int snd_vol;

void S_TransferPaintBuffer( int endtime );

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
