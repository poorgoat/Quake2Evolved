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


#define SFX_HASHSIZE		256

#define	MAX_SFX				1024

static sfx_t		*s_sfxHash[SFX_HASHSIZE];
static sfx_t		*s_sfx[MAX_SFX];
static int			s_numSfx;


/*
 =================
 S_SoundList_f
 =================
*/
void S_SoundList_f (void){

	sfx_t	*sfx;
	int		i, samples = 0;

	Com_Printf("\n");
	Com_Printf("      -samples -hz-- -format- -name--------\n");

	for (i = 0; i < s_numSfx; i++){
		sfx = s_sfx[i];

		Com_Printf("%4i: ", i);

		if (sfx->loaded){
			samples += sfx->samples;

			Com_Printf("%8i ", sfx->samples);

			Com_Printf("%5i ", sfx->rate);

			switch (sfx->format){
			case AL_FORMAT_STEREO16:
				Com_Printf("STEREO16 ");
				break;
			case AL_FORMAT_STEREO8:
				Com_Printf("STEREO8  ");
				break;
			case AL_FORMAT_MONO16:
				Com_Printf("MONO16   ");
				break;
			case AL_FORMAT_MONO8:
				Com_Printf("MONO8    ");
				break;
			default:
				Com_Printf("???????? ");
				break;
			}

			if (sfx->name[0] == '#')
				Com_Printf("%s", &sfx->name[1]);
			else
				Com_Printf("sound/%s", sfx->name);

			if (sfx->defaulted)
				Com_Printf(" (DEFAULTED)\n");
			else
				Com_Printf("\n");
		}
		else {
			if (sfx->name[0] == '*')
				Com_Printf("      placeholder       %s\n", sfx->name);
			else
				Com_Printf("      not loaded        %s\n", sfx->name);
		}
	}

	Com_Printf("-------------------------------------------\n");
	Com_Printf("%i total samples\n", samples);
	Com_Printf("%i total sounds\n", s_numSfx);
	Com_Printf("\n");
}


/*
 =======================================================================

 WAV LOADING

 =======================================================================
*/

static byte *iff_data;
static byte	*iff_dataPtr;
static byte *iff_end;
static byte *iff_lastChunk;
static int	iff_chunkLen;


/*
 =================
 S_GetLittleShort
 =================
*/
static short S_GetLittleShort (void){

	short	val = 0;

	val += (*(iff_dataPtr+0) << 0);
	val += (*(iff_dataPtr+1) << 8);

	iff_dataPtr += 2;

	return val;
}

/*
 =================
 S_GetLittleLong
 =================
*/
static int S_GetLittleLong (void){

	int		val = 0;

	val += (*(iff_dataPtr+0) << 0);
	val += (*(iff_dataPtr+1) << 8);
	val += (*(iff_dataPtr+2) << 16);
	val += (*(iff_dataPtr+3) << 24);

	iff_dataPtr += 4;

	return val;
}

/*
 =================
 S_FindNextChunk
 =================
*/
static void S_FindNextChunk (const char *name){

	while (1){
		iff_dataPtr = iff_lastChunk;

		if (iff_dataPtr >= iff_end){
			// Didn't find the chunk
			iff_dataPtr = NULL;
			return;
		}

		iff_dataPtr += 4;
		iff_chunkLen = S_GetLittleLong();
		if (iff_chunkLen < 0){
			iff_dataPtr = NULL;
			return;
		}

		iff_dataPtr -= 8;
		iff_lastChunk = iff_dataPtr + 8 + ((iff_chunkLen + 1) & ~1);
		if (!Q_strncmp(iff_dataPtr, name, 4))
			return;
	}
}

/*
 =================
 S_FindChunk
 =================
*/
static void S_FindChunk (const char *name){

	iff_lastChunk = iff_data;

	S_FindNextChunk(name);
}

/*
 =================
 S_LoadWAV
 =================
*/
static qboolean S_LoadWAV (const char *name, byte **wav, wavInfo_t *info){

	byte	*buffer, *out;
	int		length;

	length = FS_LoadFile(name, (void **)&buffer);
	if (!buffer)
		return false;

	iff_data = buffer;
	iff_end = buffer + length;

	// Find "RIFF" chunk
	S_FindChunk("RIFF");
	if (!(iff_dataPtr && !Q_strncmp(iff_dataPtr+8, "WAVE", 4))){
		Com_DPrintf(S_COLOR_YELLOW "S_LoadWAV: missing 'RIFF/WAVE' chunks (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	// Get "fmt " chunk
	iff_data = iff_dataPtr + 12;

	S_FindChunk("fmt ");
	if (!iff_dataPtr){
		Com_DPrintf(S_COLOR_YELLOW "S_LoadWAV: missing 'fmt ' chunk (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	iff_dataPtr += 8;

	if (S_GetLittleShort() != 1){
		Com_DPrintf(S_COLOR_YELLOW "S_LoadWAV: Microsoft PCM format only (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	info->channels = S_GetLittleShort();
	if (info->channels != 1){
		Com_DPrintf(S_COLOR_YELLOW "S_LoadWAV: only mono WAV files supported (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	info->rate = S_GetLittleLong();

	iff_dataPtr += 4+2;

	info->width = S_GetLittleShort() / 8;
	if (info->width != 1 && info->width != 2){
		Com_DPrintf(S_COLOR_YELLOW "S_LoadWAV: only 8 and 16 bit WAV files supported (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	// Find data chunk
	S_FindChunk("data");
	if (!iff_dataPtr){
		Com_DPrintf(S_COLOR_YELLOW "S_LoadWAV: missing 'data' chunk (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	iff_dataPtr += 4;
	info->samples = S_GetLittleLong() / info->width;

	if (info->samples <= 0){
		Com_DPrintf(S_COLOR_YELLOW "S_LoadWAV: file with 0 samples (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	// Load the data
	*wav = out = Z_Malloc(info->samples * info->width);
	memcpy(out, buffer + (iff_dataPtr - buffer), info->samples * info->width);

	FS_FreeFile(buffer);

	return true;
}


// =====================================================================


/*
 =================
 S_UploadSound
 =================
*/
static void S_UploadSound (byte *data, int width, int channels, sfx_t *sfx){

	int		size;

	// Calculate buffer size
	size = sfx->samples * width * channels;

	// Set buffer format
	if (width == 2){
		if (channels == 2)
			sfx->format = AL_FORMAT_STEREO16;
		else
			sfx->format = AL_FORMAT_MONO16;
	}
	else {
		if (channels == 2)
			sfx->format = AL_FORMAT_STEREO8;
		else
			sfx->format = AL_FORMAT_MONO8;
	}

	// Upload the sound
	qalGenBuffers(1, &sfx->bufferNum);
	qalBufferData(sfx->bufferNum, sfx->format, data, size, sfx->rate);
}

/*
 =================
 S_CreateDefaultSound
 =================
*/
static void S_CreateDefaultSound (byte **wav, wavInfo_t *info){

	byte	*out;
	int		i;

	info->rate = 22050;
	info->width = 2;
	info->channels = 1;
	info->samples = 11025;

	*wav = out = Z_Malloc(info->samples * info->width);

	for (i = 0; i < info->samples; i++)
		((short *)out)[i] = sin(i * 0.1) * 20000;
}

/*
 =================
 S_LoadSound
 =================
*/
qboolean S_LoadSound (sfx_t *sfx){

    char		name[MAX_QPATH];
	byte		*data;
	wavInfo_t	info;

	if (sfx->name[0] == '*')
#if defined(__MINGW32__)
        return 0;
#elif defined(__LINUX__)
        return 0;
#else
		return NULL;
#endif

	// See if still in memory
	if (sfx->loaded)
#if defined(__MINGW32__)
        return 1;
#elif defined(__LINUX__)
        return 1;
#else
		return sfx;
#endif

	// Load it from disk
	if (sfx->name[0] == '#')
		Q_snprintfz(name, sizeof(name), "%s", &sfx->name[1]);
	else
		Q_snprintfz(name, sizeof(name), "sound/%s", sfx->name);

	if (!S_LoadWAV(name, &data, &info)){
		sfx->defaulted = true;

		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find sound '%s', using default...\n", name);
		S_CreateDefaultSound(&data, &info);
	}

	// Load it in
	sfx->loaded = true;
	sfx->samples = info.samples;
	sfx->rate = info.rate;

	S_UploadSound(data, info.width, info.channels, sfx);

	Z_Free(data);

	return true;
}

/*
 =================
 S_FindSound
 =================
*/
sfx_t *S_FindSound (const char *name){

	sfx_t		*sfx;
	unsigned	hashKey;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "S_FindSound: NULL sound name");

	if (strlen(name) >= MAX_QPATH)
		Com_Error(ERR_DROP, "S_FindSound: sound name exceeds MAX_QPATH");

	// See if already loaded
	hashKey = Com_HashKey(name, SFX_HASHSIZE);

	for (sfx = s_sfxHash[hashKey]; sfx; sfx = sfx->nextHash){
		if (!Q_stricmp(sfx->name, name))
			return sfx;
	}

	// Create a new sfx_t
	if (s_numSfx == MAX_SFX)
		Com_Error(ERR_DROP, "S_FindSound: MAX_SFX hit");

	s_sfx[s_numSfx++] = sfx = Z_Malloc(sizeof(sfx_t));

	// Fill it in
	Q_strncpyz(sfx->name, name, sizeof(sfx->name));

	// Add to hash table
	sfx->nextHash = s_sfxHash[hashKey];
	s_sfxHash[hashKey] = sfx;

	return sfx;
}

/*
 =================
 S_RegisterSexedSound
 =================
*/
sfx_t *S_RegisterSexedSound (const char *base, entity_state_t *ent){

	sfx_t			*sfx;
	clientInfo_t	*ci;
	char			model[MAX_QPATH], sexedName[MAX_QPATH];
	char			*ch;

	if (!s_initialized)
		return NULL;

	// Determine what model the client is using
	ci = &cl.clientInfo[ent->number - 1];
	if (!ci->valid)
		ci = &cl.baseClientInfo;

	Q_strncpyz(model, ci->info, sizeof(model));
	ch = strchr(model, '/');
	if (!ch)
		ch = strchr(model, '\\');
	if (ch)
		*ch = 0;

	// See if the model specific sound exists
	Q_snprintfz(sexedName, sizeof(sexedName), "#players/%s/%s", model, base+1);
	if (FS_LoadFile(&sexedName[1], NULL) != -1){
		// Yes, register it
		sfx = S_FindSound(sexedName);
		S_LoadSound(sfx);
	}
	else {
		// No, revert to the male sound
		Q_snprintfz(sexedName, sizeof(sexedName), "#players/male/%s", base+1);

		sfx = S_FindSound(sexedName);
		S_LoadSound(sfx);
	}

	return sfx;
}

/*
 =================
 S_RegisterSound
 =================
*/
sfx_t *S_RegisterSound (const char *name){

	sfx_t	*sfx;

	if (!s_initialized)
		return NULL;

	sfx = S_FindSound(name);
	S_LoadSound(sfx);

	return sfx;
}

/*
 =================
 S_FreeSounds
 =================
*/
void S_FreeSounds (void){

	sfx_t	*sfx;
	int		i;

	// Stop all sounds
	S_StopAllSounds();

	// Free all sounds
	for (i = 0; i < s_numSfx; i++){
		sfx = s_sfx[i];

		qalDeleteBuffers(1, &sfx->bufferNum);

		Z_Free(sfx);
	}

	memset(s_sfxHash, 0, sizeof(s_sfxHash));
	memset(s_sfx, 0, sizeof(s_sfx));

	s_numSfx = 0;
}
