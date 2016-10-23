/*
Copyright (C) 1997-2001 Id Software, Inc.

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


#include "s_local.h"


#define BUFFER_SIZE		16384

static bgTrack_t	s_bgTrack;

static channel_t	*s_streamingChannel;


/*
 =======================================================================

 OGG VORBIS STREAMING

 =======================================================================
*/


/*
 =================
 S_ReadOGG
 =================
*/
static size_t S_ReadOGG (void *buffer, size_t size, size_t count, void *data){

	bgTrack_t	*track = (bgTrack_t *)data;

	if (!size || !count)
		return 0;

	return FS_Read(buffer, size * count, track->file) / size;
}

/*
 =================
 S_SeekOGG
 =================
*/
static int S_SeekOGG (void *data, ogg_int64_t offset, int origin){

	bgTrack_t	*track = (bgTrack_t *)data;

	switch (origin){
	case SEEK_SET:
		FS_Seek(track->file, (int)offset, FS_SEEK_SET);
		break;
	case SEEK_CUR:
		FS_Seek(track->file, (int)offset, FS_SEEK_CUR);
		break;
	case SEEK_END:
		FS_Seek(track->file, (int)offset, FS_SEEK_END);
		break;
	default:
		return -1;
	}

	return 0;
}

/*
 =================
 S_CloseOGG
 =================
*/
static int S_CloseOGG (void *data){

	return 0;
}

/*
 =================
 S_TellOGG
 =================
*/
static long S_TellOGG (void *data){

	bgTrack_t	*track = (bgTrack_t *)data;

	return FS_Tell(track->file);
}

/*
 =================
 S_OpenBackgroundTrack
 =================
*/
static qboolean S_OpenBackgroundTrack (const char *name, bgTrack_t *track){

	OggVorbis_File	*vorbisFile;
	vorbis_info		*vorbisInfo;
	ov_callbacks	vorbisCallbacks = {S_ReadOGG, S_SeekOGG, S_CloseOGG, S_TellOGG};

	FS_OpenFile(name, &track->file, FS_READ);
	if (!track->file){
		Com_DPrintf(S_COLOR_YELLOW "S_OpenBackgroundTrack: couldn't find %s\n", name);
		return false;
	}

	track->vorbisFile = vorbisFile = Z_Malloc(sizeof(OggVorbis_File));

	if (ov_open_callbacks(track, vorbisFile, NULL, 0, vorbisCallbacks) < 0){
		Com_DPrintf(S_COLOR_YELLOW "S_OpenBackgroundTrack: couldn't open OGG stream (%s)\n", name);
		return false;
	}

	vorbisInfo = ov_info(vorbisFile, -1);
	if (vorbisInfo->channels != 1 && vorbisInfo->channels != 2){
		Com_DPrintf(S_COLOR_YELLOW "S_OpenBackgroundTrack: only mono and stereo OGG files supported (%s)\n", name);
		return false;
	}

	track->header = ov_raw_tell(vorbisFile);
	track->rate = vorbisInfo->rate;
	track->format = (vorbisInfo->channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

	return true;
}

/*
 =================
 S_CloseBackgroundTrack
 =================
*/
static void S_CloseBackgroundTrack (bgTrack_t *track){

	if (track->vorbisFile){
		ov_clear(track->vorbisFile);

		Z_Free(track->vorbisFile);
		track->vorbisFile = NULL;
	}

	if (track->file){
		FS_CloseFile(track->file);
		track->file = 0;
	}
}

/*
 =================
 S_StreamBackgroundTrack
 =================
*/
void S_StreamBackgroundTrack (void){

	byte		data[BUFFER_SIZE];
	int			processed, queued, state;
	int			size, read, dummy;
	unsigned	buffer;

	if (!s_bgTrack.file || !s_musicVolume->floatValue)
		return;

	if (!s_streamingChannel)
		return;

	// Unqueue and delete any processed buffers
	qalGetSourcei(s_streamingChannel->sourceNum, AL_BUFFERS_PROCESSED, &processed);
	if (processed > 0){
		while (processed--){
			qalSourceUnqueueBuffers(s_streamingChannel->sourceNum, 1, &buffer);
			qalDeleteBuffers(1, &buffer);
		}
	}

	// Make sure we always have at least 4 buffers in the queue
	qalGetSourcei(s_streamingChannel->sourceNum, AL_BUFFERS_QUEUED, &queued);
	while (queued < 4){
		size = 0;

		// Stream from disk
		while (size < BUFFER_SIZE){
			read = ov_read(s_bgTrack.vorbisFile, data + size, BUFFER_SIZE - size, 0, 2, 1, &dummy);
			if (read == 0){
				// End of file
				if (!s_bgTrack.looping){
					// Close the intro track
					S_CloseBackgroundTrack(&s_bgTrack);

					// Open the loop track
					if (!S_OpenBackgroundTrack(s_bgTrack.loopName, &s_bgTrack)){
						S_StopBackgroundTrack();
						return;
					}

					s_bgTrack.looping = true;
				}

				// Restart the track, skipping over the header
				ov_raw_seek(s_bgTrack.vorbisFile, (ogg_int64_t)s_bgTrack.header);

				// Try streaming again
				read = ov_read(s_bgTrack.vorbisFile, data + size, BUFFER_SIZE - size, 0, 2, 1, &dummy);
			}

			if (read <= 0){
				if (!s_bgTrack.looping)
					Com_DPrintf(S_COLOR_RED "S_StreamBackgroundTrack: failed to read from OGG stream (%s)\n", s_bgTrack.introName);
				else
					Com_DPrintf(S_COLOR_RED "S_StreamBackgroundTrack: failed to read from OGG stream (%s)\n", s_bgTrack.loopName);

				S_StopBackgroundTrack();
				return;
			}

			size += read;
		}

		// Upload and queue the new buffer
		qalGenBuffers(1, &buffer);
		qalBufferData(buffer, s_bgTrack.format, data, size, s_bgTrack.rate);
		qalSourceQueueBuffers(s_streamingChannel->sourceNum, 1, &buffer);

		queued++;
	}

	// Update volume
	qalSourcef(s_streamingChannel->sourceNum, AL_GAIN, s_musicVolume->floatValue);

	// If not playing, then do so
	qalGetSourcei(s_streamingChannel->sourceNum, AL_SOURCE_STATE, &state);
	if (state != AL_PLAYING)
		qalSourcePlay(s_streamingChannel->sourceNum);
}


// =====================================================================


/*
 =================
 S_StartBackgroundTrack
 =================
*/
void S_StartBackgroundTrack (const char *introTrack, const char *loopTrack){

	// Stop any playing tracks
	S_StopBackgroundTrack();

	// Start it up
	Q_strncpyz(s_bgTrack.introName, introTrack, sizeof(s_bgTrack.introName));
	Q_strncpyz(s_bgTrack.loopName, loopTrack, sizeof(s_bgTrack.loopName));

	S_StartStreaming();

	// Open the intro track
	if (!S_OpenBackgroundTrack(s_bgTrack.introName, &s_bgTrack)){
		S_StopBackgroundTrack();
		return;
	}

	// Begin streaming
	S_StreamBackgroundTrack();
}

/*
 =================
 S_StopBackgroundTrack
 =================
*/
void S_StopBackgroundTrack (void){

	S_StopStreaming();

	S_CloseBackgroundTrack(&s_bgTrack);

	memset(&s_bgTrack, 0, sizeof(bgTrack_t));
}

/*
 =================
 S_StartStreaming
 =================
*/
void S_StartStreaming (void){

	if (s_streamingChannel)
		return;		// Already started

	s_streamingChannel = S_PickChannel(0, 0);
	if (!s_streamingChannel){
		Com_DPrintf(S_COLOR_RED "Dropped stream\n");
		return;
	}

	s_streamingChannel->streaming = true;

	// Set up the source
	qalSourcei(s_streamingChannel->sourceNum, AL_BUFFER, 0);
	qalSourcei(s_streamingChannel->sourceNum, AL_LOOPING, AL_FALSE);
	qalSourcei(s_streamingChannel->sourceNum, AL_SOURCE_RELATIVE, AL_TRUE);
	qalSourcefv(s_streamingChannel->sourceNum, AL_POSITION, vec3_origin);
	qalSourcefv(s_streamingChannel->sourceNum, AL_VELOCITY, vec3_origin);
	qalSourcef(s_streamingChannel->sourceNum, AL_REFERENCE_DISTANCE, 1.0);
	qalSourcef(s_streamingChannel->sourceNum, AL_MAX_DISTANCE, 1.0);
	qalSourcef(s_streamingChannel->sourceNum, AL_ROLLOFF_FACTOR, 0.0);
	qalSourcef(s_streamingChannel->sourceNum, AL_PITCH, 1.0);
}

/*
 =================
 S_StopStreaming
 =================
*/
void S_StopStreaming (void){

	int			processed;
	unsigned	buffer;

	if (!s_streamingChannel)
		return;		// Already stopped

	s_streamingChannel->streaming = false;

	// Clean up the source
	qalSourceStop(s_streamingChannel->sourceNum);

	qalGetSourcei(s_streamingChannel->sourceNum, AL_BUFFERS_PROCESSED, &processed);
	if (processed > 0){
		while (processed--){
			qalSourceUnqueueBuffers(s_streamingChannel->sourceNum, 1, &buffer);
			qalDeleteBuffers(1, &buffer);
		}
	}

	qalSourcei(s_streamingChannel->sourceNum, AL_BUFFER, 0);

	s_streamingChannel = NULL;
}

/*
 =================
 S_StreamRawSamples

 Cinematic streaming
 =================
*/
void S_StreamRawSamples (const byte *data, int samples, int rate, int width, int channels){

	int			processed, state, size;
	unsigned	format, buffer;

	if (!s_streamingChannel)
		return;

	// Unqueue and delete any processed buffers
	qalGetSourcei(s_streamingChannel->sourceNum, AL_BUFFERS_PROCESSED, &processed);
	if (processed > 0){
		while (processed--){
			qalSourceUnqueueBuffers(s_streamingChannel->sourceNum, 1, &buffer);
			qalDeleteBuffers(1, &buffer);
		}
	}

	// Calculate buffer size
	size = samples * width * channels;

	// Set buffer format
	if (width == 2){
		if (channels == 2)
			format = AL_FORMAT_STEREO16;
		else
			format = AL_FORMAT_MONO16;
	}
	else {
		if (channels == 2)
			format = AL_FORMAT_STEREO8;
		else
			format = AL_FORMAT_MONO8;
	}

	// Upload and queue the new buffer
	qalGenBuffers(1, &buffer);
	qalBufferData(buffer, format, (byte *)data, size, rate);
	qalSourceQueueBuffers(s_streamingChannel->sourceNum, 1, &buffer);

	// Update volume
	qalSourcef(s_streamingChannel->sourceNum, AL_GAIN, s_sfxVolume->floatValue);

	// If not playing, then do so
	qalGetSourcei(s_streamingChannel->sourceNum, AL_SOURCE_STATE, &state);
	if (state != AL_PLAYING)
		qalSourcePlay(s_streamingChannel->sourceNum);
}
