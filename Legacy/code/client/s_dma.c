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


#define MAX_PLAYSOUNDS		128

#define MAX_CHANNELS		64

static playSound_t	s_playSounds[MAX_PLAYSOUNDS];
static playSound_t	s_freePlaySounds;
static playSound_t	s_pendingPlaySounds;

static channel_t	s_channels[MAX_CHANNELS];
static int			s_numChannels;

static listener_t	s_listener;

static int			s_frameCount;

static qboolean		s_activeApp;

qboolean			s_initialized = false;

const GUID			DSPROPSETID_EAX20_ListenerProperties = {0x306a6a8, 0xb224, 0x11d2, {0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22}};
const GUID			DSPROPSETID_EAX20_BufferProperties = {0x306a6a7, 0xb224, 0x11d2, {0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22}};

cvar_t	*s_initSound;
cvar_t	*s_show;
cvar_t	*s_alDriver;
cvar_t	*s_alDevice;
cvar_t	*s_allowExtensions;
cvar_t	*s_ext_eax;
cvar_t	*s_ignoreALErrors;
cvar_t	*s_masterVolume;
cvar_t	*s_sfxVolume;
cvar_t	*s_musicVolume;
cvar_t	*s_minDistance;
cvar_t	*s_maxDistance;
cvar_t	*s_rolloffFactor;
cvar_t	*s_dopplerFactor;
cvar_t	*s_dopplerVelocity;


/*
 =================
 S_CheckForErrors
 =================
*/
static void S_CheckForErrors (void){

	int		err;
	char	*str;

	if ((err = qalGetError()) == AL_NO_ERROR)
		return;

	switch (err){
	case AL_INVALID_NAME:
		str = "AL_INVALID_NAME";
		break;
	case AL_INVALID_ENUM:
		str = "AL_INVALID_ENUM";
		break;
	case AL_INVALID_VALUE:
		str = "AL_INVALID_VALUE";
		break;
	case AL_INVALID_OPERATION:
		str = "AL_INVALID_OPERATION";
		break;
	case AL_OUT_OF_MEMORY:
		str = "AL_OUT_OF_MEMORY";
		break;
	default:
		str = "UNKNOWN ERROR";
		break;
	}

	Com_Error(ERR_DROP, "S_CheckForErrors: %s", str);
}

/*
 =================
 S_AllocChannels
 =================
*/
static void S_AllocChannels (void){

	channel_t	*ch;
	int			i;

	for (i = 0, ch = s_channels; i < MAX_CHANNELS; i++, ch++){
		qalGenSources(1, &ch->sourceNum);

		if (qalGetError() != AL_NO_ERROR)
			break;

		s_numChannels++;
	}
}

/*
 =================
 S_FreeChannels
 =================
*/
static void S_FreeChannels (void){

	channel_t	*ch;
	int			i;

	for (i = 0, ch = s_channels; i < s_numChannels; i++, ch++){
		qalDeleteSources(1, &ch->sourceNum);

		memset(ch, 0, sizeof(*ch));
	}

	s_numChannels = 0;
}

/*
 =================
 S_ChannelState
 =================
*/
static int S_ChannelState (channel_t *ch){

	int		state;

	qalGetSourcei(ch->sourceNum, AL_SOURCE_STATE, &state);

	return state;
}

/*
 =================
 S_PlayChannel
 =================
*/
static void S_PlayChannel (channel_t *ch, sfx_t *sfx){

	ch->sfx = sfx;

	qalSourcei(ch->sourceNum, AL_BUFFER, sfx->bufferNum);
	qalSourcei(ch->sourceNum, AL_LOOPING, ch->loopSound);
	qalSourcei(ch->sourceNum, AL_SOURCE_RELATIVE, AL_FALSE);
	qalSourcePlay(ch->sourceNum);
}

/*
 =================
 S_StopChannel
 =================
*/
static void S_StopChannel (channel_t *ch){

	ch->sfx = NULL;

	qalSourceStop(ch->sourceNum);
	qalSourcei(ch->sourceNum, AL_BUFFER, 0);
}

/*
 =================
 S_SpatializeChannel
 =================
*/
static void S_SpatializeChannel (channel_t *ch){

	vec3_t	position, velocity;

	// Update position and velocity
	if (ch->entNum == cl.clientNum || !ch->distanceMult){
		qalSourcefv(ch->sourceNum, AL_POSITION, s_listener.position);
		qalSourcefv(ch->sourceNum, AL_VELOCITY, s_listener.velocity);
	}
	else {
		if (ch->fixedPosition){
			qalSource3f(ch->sourceNum, AL_POSITION, ch->position[1], ch->position[2], -ch->position[0]);
			qalSource3f(ch->sourceNum, AL_VELOCITY, 0, 0, 0);
		}
		else {
			if (ch->loopSound)
				CL_GetEntitySoundSpatialization(ch->loopNum, position, velocity);
			else
				CL_GetEntitySoundSpatialization(ch->entNum, position, velocity);

			qalSource3f(ch->sourceNum, AL_POSITION, position[1], position[2], -position[0]);
			qalSource3f(ch->sourceNum, AL_VELOCITY, velocity[1], velocity[2], -velocity[0]);
		}
	}

	// Update min/max distance
	if (ch->distanceMult)
		qalSourcef(ch->sourceNum, AL_REFERENCE_DISTANCE, s_minDistance->value * ch->distanceMult);
	else
		qalSourcef(ch->sourceNum, AL_REFERENCE_DISTANCE, s_maxDistance->value);

	qalSourcef(ch->sourceNum, AL_MAX_DISTANCE, s_maxDistance->value);

	// Update volume and rolloff factor
	qalSourcef(ch->sourceNum, AL_GAIN, s_sfxVolume->value * ch->volume);
	qalSourcef(ch->sourceNum, AL_ROLLOFF_FACTOR, s_rolloffFactor->value);
}

/*
 =================
 S_PickChannel

 Tries to find a free channel, or tries to replace an active channel
 =================
*/
channel_t *S_PickChannel (int entNum, int entChannel){

	channel_t	*ch;
	int			i;
	int			firstToDie = -1;
	int			oldestTime = cl.time;

	if (entNum < 0 || entChannel < 0)
		Com_Error(ERR_DROP, "S_PickChannel: entNum < 0 || entChannel < 0");

	for (i = 0, ch = s_channels; i < s_numChannels; i++, ch++){
		// Don't let game sounds override streaming sounds
		if (ch->streaming)
			continue;

		// Check if this channel is active
		if (!ch->sfx){
			// Free channel
			firstToDie = i;
			break;
		}

		// Channel 0 never overrides
		if (entChannel != 0 && (ch->entNum == entNum && ch->entChannel == entChannel)){
			// Always override sound from same entity
			firstToDie = i;
			break;
		}

		// Don't let monster sounds override player sounds
		if (entNum != cl.clientNum && ch->entNum == cl.clientNum)
			continue;

		// Replace the oldest sound
		if (ch->startTime < oldestTime){
			oldestTime = ch->startTime;
			firstToDie = i;
		}
	}

	if (firstToDie == -1)
		return NULL;

	ch = &s_channels[firstToDie];

	ch->entNum = entNum;
	ch->entChannel = entChannel;
	ch->startTime = cl.time;

	// Make sure this channel is stopped
	qalSourceStop(ch->sourceNum);
	qalSourcei(ch->sourceNum, AL_BUFFER, 0);

	return ch;
}

/*
 =================
 S_AddLoopingSounds

 Entities with a sound field will generate looping sounds that are
 automatically started and stopped as the entities are sent to the
 client
 =================
*/
static void S_AddLoopingSounds (void){

	entity_state_t	*ent;
	sfx_t			*sfx;
	channel_t		*ch;
	int				i, j;

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.cinematicHandle || paused->integer)
		return;

	for (i = 0; i < cl.frame.numEntities; i++){
		ent = &cl.parseEntities[(cl.frame.parseEntitiesIndex+i) & (MAX_PARSE_ENTITIES-1)];
		if (!ent->sound)
			continue;

		sfx = cl.media.gameSounds[ent->sound];
		if (!sfx || !sfx->loaded)
			continue;		// Bad sound effect

		// If this entity is already playing the same sound effect on an
		// active channel, then simply update it
		for (j = 0, ch = s_channels; j < s_numChannels; j++, ch++){
			if (ch->sfx != sfx)
				continue;

			if (!ch->loopSound)
				continue;
			if (ch->loopNum != ent->number)
				continue;
			if (ch->loopFrame + 1 != s_frameCount)
				continue;

			ch->loopFrame = s_frameCount;
			break;
		}

		if (j != s_numChannels)
			continue;

		// Otherwise pick a channel and start the sound effect
		ch = S_PickChannel(0, 0);
		if (!ch){
			if (sfx->name[0] == '#')
				Com_DPrintf(S_COLOR_RED "Dropped sound %s\n", &sfx->name[1]);
			else
				Com_DPrintf(S_COLOR_RED "Dropped sound sound/%s\n", sfx->name);

			continue;
		}

		ch->loopSound = true;
		ch->loopNum = ent->number;
		ch->loopFrame = s_frameCount;
		ch->fixedPosition = false;
		ch->volume = 1.0;
		ch->distanceMult = 1.0 / ATTN_STATIC;

		S_SpatializeChannel(ch);

		S_PlayChannel(ch, sfx);
	}
}

/*
 =================
 S_AllocPlaySound
 =================
*/
static playSound_t *S_AllocPlaySound (void){

	playSound_t	*ps;

	ps = s_freePlaySounds.next;
	if (ps == &s_freePlaySounds)
		return NULL;		// No free playSounds

	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;

	return ps;
}

/*
 =================
 S_FreePlaySound
 =================
*/
static void S_FreePlaySound (playSound_t *ps){

	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;

	// Add to free list
	ps->next = s_freePlaySounds.next;
	s_freePlaySounds.next->prev = ps;
	ps->prev = &s_freePlaySounds;
	s_freePlaySounds.next = ps;
}

/*
 =================
 S_IssuePlaySounds

 Take all the pending playSounds and begin playing them.
 This is never called directly by S_Start*, but only by the update loop.
 =================
*/
static void S_IssuePlaySounds (void){

	playSound_t	*ps;
	channel_t	*ch;

	while (1){
		ps = s_pendingPlaySounds.next;
		if (ps == &s_pendingPlaySounds)
			break;		// No more pending playSounds

		if (ps->beginTime > cl.time)
			break;		// No more pending playSounds this frame

		// Pick a channel and start the sound effect
		ch = S_PickChannel(ps->entNum, ps->entChannel);
		if (!ch){
			if (ps->sfx->name[0] == '#')
				Com_DPrintf(S_COLOR_RED "Dropped sound %s\n", &ps->sfx->name[1]);
			else
				Com_DPrintf(S_COLOR_RED "Dropped sound sound/%s\n", ps->sfx->name);

			S_FreePlaySound(ps);
			continue;
		}

		ch->loopSound = false;
		ch->fixedPosition = ps->fixedPosition;
		VectorCopy(ps->position, ch->position);
		ch->volume = ps->volume;

		if (ps->attenuation != ATTN_NONE)
			ch->distanceMult = 1.0 / ps->attenuation;
		else
			ch->distanceMult = 0.0;

		S_SpatializeChannel(ch);

		S_PlayChannel(ch, ps->sfx);

		// Free the playSound
		S_FreePlaySound(ps);
	}
}

/*
 =================
 S_StartSound

 Validates the parms and queues the sound up.
 If origin is NULL, the sound will be dynamically sourced from the
 entity.
 entChannel 0 will never override a playing sound.
 =================
*/
void S_StartSound (const vec3_t position, int entNum, int entChannel, sfx_t *sfx, float volume, float attenuation, int timeOfs){

	playSound_t	*ps, *sort;

	if (!s_initialized)
		return;

	if (!sfx)
		return;

	if (sfx->name[0] == '*')
		sfx = S_RegisterSexedSound(sfx->name, &cl.entities[entNum].current);

	// Make sure the sound is loaded
	if (!S_LoadSound(sfx))
		return;

	// Allocate a playSound
	ps = S_AllocPlaySound();
	if (!ps){
		if (sfx->name[0] == '#')
			Com_DPrintf(S_COLOR_RED "Dropped sound %s\n", &sfx->name[1]);
		else
			Com_DPrintf(S_COLOR_RED "Dropped sound sound/%s\n", sfx->name);

		return;
	}

	ps->sfx = sfx;
	ps->entNum = entNum;
	ps->entChannel = entChannel;

	if (position){
		ps->fixedPosition = true;
		VectorCopy(position, ps->position);
	}
	else
		ps->fixedPosition = false;

	ps->volume = volume;
	ps->attenuation = attenuation;
	ps->beginTime = cl.time + timeOfs;

	// Sort into the pending playSounds list
	for (sort = s_pendingPlaySounds.next; sort != &s_pendingPlaySounds&& sort->beginTime < ps->beginTime; sort = sort->next)
		;

	ps->next = sort;
	ps->prev = sort->prev;

	ps->next->prev = ps;
	ps->prev->next = ps;
}

/*
 =================
 S_StartLocalSound
 =================
*/
void S_StartLocalSound (sfx_t *sfx){

	if (!s_initialized)
		return;

	if (!sfx)
		return;
		
	S_StartSound(NULL, cl.clientNum, 0, sfx, 1, ATTN_NONE, 0);
}

/*
 =================
 S_StopAllSounds
 =================
*/
void S_StopAllSounds (void){

	channel_t	*ch;
	int			i;

	if (!s_initialized)
		return;

	// Clear all the playSounds
	memset(s_playSounds, 0, sizeof(s_playSounds));

	s_freePlaySounds.next = s_freePlaySounds.prev = &s_freePlaySounds;
	s_pendingPlaySounds.next = s_pendingPlaySounds.prev = &s_pendingPlaySounds;

	for (i = 0; i < MAX_PLAYSOUNDS; i++){
		s_playSounds[i].prev = &s_freePlaySounds;
		s_playSounds[i].next = s_freePlaySounds.next;
		s_playSounds[i].prev->next = &s_playSounds[i];
		s_playSounds[i].next->prev = &s_playSounds[i];
	}

	// Stop all the channels
	for (i = 0, ch = s_channels; i < s_numChannels; i++, ch++){
		if (!ch->sfx)
			continue;

		S_StopChannel(ch);
	}

	// Stop streaming channel
	S_StopStreaming();

	// Stop background track
	S_StopBackgroundTrack();

	// Reset frame count
	s_frameCount = 0;
}

/*
 =================
 S_Update

 Called once each time through the main loop
 =================
*/
void S_Update (const vec3_t position, const vec3_t velocity, const vec3_t at, const vec3_t up){

	unsigned	eaxEnv;
	channel_t	*ch;
	int			i, total = 0;

	if (!s_initialized)
		return;

	// Bump frame count
	s_frameCount++;

	// Set up listener
	VectorSet(s_listener.position, position[1], position[2], -position[0]);
	VectorSet(s_listener.velocity, velocity[1], velocity[2], -velocity[0]);
	VectorSet(&s_listener.orientation[0], at[1], -at[2], -at[0]);
	VectorSet(&s_listener.orientation[3], up[1], -up[2], -up[0]);

	qalListenerfv(AL_POSITION, s_listener.position);
	qalListenerfv(AL_VELOCITY, s_listener.velocity);
	qalListenerfv(AL_ORIENTATION, s_listener.orientation);
	qalListenerf(AL_GAIN, (s_activeApp) ? s_masterVolume->value : 0.0);

	// Set state
	qalDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

	qalDopplerFactor(s_dopplerFactor->value);
	qalDopplerVelocity(s_dopplerVelocity->value);

	// If EAX is enabled, apply listener environmental effects
	if (alConfig.eax){
		if ((cls.state != CA_ACTIVE || cls.loading) || cls.cinematicHandle)
			eaxEnv = EAX_ENVIRONMENT_GENERIC;
		else {
			if (CL_PointContents(position, -1) & MASK_WATER)
				eaxEnv = EAX_ENVIRONMENT_UNDERWATER;
			else
				eaxEnv = EAX_ENVIRONMENT_GENERIC;
		}

		qalEAXSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT | DSPROPERTY_EAXLISTENER_DEFERRED, 0, &eaxEnv, sizeof(eaxEnv));
	}

	// Stream background track
	S_StreamBackgroundTrack();

	// Add looping sounds
	S_AddLoopingSounds();

	// Issue playSounds
	S_IssuePlaySounds();

	// Update spatialization for all sounds
	for (i = 0, ch = s_channels; i < s_numChannels; i++, ch++){
		if (!ch->sfx)
			continue;		// Not active

		// Check for stop
		if (ch->loopSound){
			if (ch->loopFrame != s_frameCount){
				S_StopChannel(ch);
				continue;
			}
		}
		else {
			if (S_ChannelState(ch) == AL_STOPPED){
				S_StopChannel(ch);
				continue;
			}
		}

		// Respatialize channel
		S_SpatializeChannel(ch);

		if (s_show->integer){
			if (ch->sfx->name[0] == '#')
				Com_Printf("%2i: %s\n", i+1, &ch->sfx->name[1]);
			else
				Com_Printf("%2i: sound/%s\n", i+1, ch->sfx->name);
		}

		total++;
	}

	if (s_show->integer)
		Com_Printf("--- ( %i ) ---\n", total);

	// Check for errors
	if (!s_ignoreALErrors->integer)
		S_CheckForErrors();
}

/*
 =================
 S_GetALConfig

 Used by other systems to get the AL config
 =================
*/
void S_GetALConfig (alConfig_t *config){

	if (!config)
		return;

	*config = alConfig;
}

/*
 =================
 S_Activate

 Called when the main window gains or loses focus.
 The window may have been destroyed and recreated between a deactivate 
 and an activate.
 =================
*/
void S_Activate (qboolean active){

	s_activeApp = active;

	if (!s_initialized)
		return;

	if (active)
		qalListenerf(AL_GAIN, s_masterVolume->value);
	else
		qalListenerf(AL_GAIN, 0.0);
}

/*
 =================
 S_Play_f
 =================
*/
void S_Play_f (void){

	int 	i = 1;
	char	name[MAX_QPATH];
	sfx_t	*sfx;

	if (Cmd_Argc() == 1){
		Com_Printf("Usage: play <soundfile> [...]\n");
		return;
	}
	
	while (i < Cmd_Argc()){
		Q_strncpyz(name, Cmd_Argv(i), sizeof(name));
		Com_DefaultExtension(name, sizeof(name), ".wav");

		sfx = S_RegisterSound(name);
		S_StartLocalSound(sfx);

		i++;
	}
}

/*
 =================
 S_Music_f
 =================
*/
void S_Music_f (void){

	char	intro[MAX_QPATH], loop[MAX_QPATH];

	if (Cmd_Argc() < 2 || Cmd_Argc() > 3){
		Com_Printf("Usage: music <musicfile> [loopfile]\n");
		return;
	}

	Q_strncpyz(intro, Cmd_Argv(1), sizeof(intro));
	Com_DefaultPath(intro, sizeof(intro), "music");
	Com_DefaultExtension(intro, sizeof(intro), ".ogg");

	if (Cmd_Argc() == 3){
		Q_strncpyz(loop, Cmd_Argv(2), sizeof(loop));
		Com_DefaultPath(loop, sizeof(loop), "music");
		Com_DefaultExtension(loop, sizeof(loop), ".ogg");

		S_StartBackgroundTrack(intro, loop);
	}
	else
		S_StartBackgroundTrack(intro, intro);
}

/*
 =================
 S_StopSound_f
 =================
*/
void S_StopSound_f (void){

	S_StopAllSounds();
}

/*
 =================
 S_SfxInfo_f
 =================
*/
void S_SfxInfo_f (void){

	if (!s_initialized){
		Com_Printf("Sound system not started\n");
		return;
	}

	Com_Printf("\n");
	Com_Printf("AL_VENDOR: %s\n", alConfig.vendorString);
	Com_Printf("AL_RENDERER: %s\n", alConfig.rendererString);
	Com_Printf("AL_VERSION: %s\n", alConfig.versionString);
	Com_Printf("AL_EXTENSIONS: %s\n", alConfig.extensionsString);
	Com_Printf("\n");
	Com_Printf("DEVICE: %s\n", alConfig.deviceName);
	Com_Printf("CHANNELS: %i\n", s_numChannels);
	Com_Printf("CPU: %s\n", Cvar_VariableString("sys_cpuString"));
	Com_Printf("eax: %s\n", (alConfig.eax) ? "enabled" : "disabled");
	Com_Printf("\n");
}

/*
 =================
 S_Restart_f

 Restart the sound subsystem so it can pick up new parameters and flush
 all sounds
 =================
*/
void S_Restart_f (void){

	// If needed, restart the video subsystem (which will in turn
	// restart the sound subsystem), so the level is reloaded
	if (cls.state > CA_LOADING){
		Cbuf_ExecuteText(EXEC_NOW, "vid_restart\n");
		return;
	}

	S_Shutdown();
	S_Init();
}

/*
 =================
 S_Init
 =================
*/
void S_Init (void){

	Com_Printf("------- Sound Initialization -------\n");

	s_initSound = Cvar_Get("s_initSound", "1", CVAR_INIT);
	s_show = Cvar_Get("s_show", "0", CVAR_CHEAT);
	s_alDriver = Cvar_Get("s_alDriver", AL_DRIVER_OPENAL, CVAR_LATCH | CVAR_ARCHIVE);
	s_alDevice = Cvar_Get("s_alDevice", "", CVAR_LATCH | CVAR_ARCHIVE);
	s_allowExtensions = Cvar_Get("s_allowExtensions", "1", CVAR_LATCH | CVAR_ARCHIVE);
	s_ext_eax = Cvar_Get("s_ext_eax", "1", CVAR_LATCH | CVAR_ARCHIVE);
	s_ignoreALErrors = Cvar_Get("s_ignoreALErrors", "1", CVAR_ARCHIVE);
	s_masterVolume = Cvar_Get("s_masterVolume", "1.0", CVAR_ARCHIVE);
	s_sfxVolume = Cvar_Get("s_sfxVolume", "1.0", CVAR_ARCHIVE);
	s_musicVolume = Cvar_Get("s_musicVolume", "1.0", CVAR_ARCHIVE);
	s_minDistance = Cvar_Get("s_minDistance", "240.0", CVAR_ARCHIVE);
	s_maxDistance = Cvar_Get("s_maxDistance", "8192.0", CVAR_ARCHIVE);
	s_rolloffFactor = Cvar_Get("s_rolloffFactor", "1.0", CVAR_ARCHIVE);
	s_dopplerFactor = Cvar_Get("s_dopplerFactor", "1.0", CVAR_ARCHIVE);
	s_dopplerVelocity = Cvar_Get("s_dopplerVelocity", "10976.0", CVAR_ARCHIVE);

	Cmd_AddCommand("play", S_Play_f);
	Cmd_AddCommand("music", S_Music_f);
	Cmd_AddCommand("stopsound", S_StopSound_f);
	Cmd_AddCommand("soundlist", S_SoundList_f);
	Cmd_AddCommand("sfxinfo", S_SfxInfo_f);
	Cmd_AddCommand("snd_restart", S_Restart_f);

	if (!s_initSound->integer){
		Com_Printf("Skipping sound initialization\n");
		Com_Printf("------------------------------------\n");
		return;
	}

	if (!ALimp_Init()){
		Com_Printf("------------------------------------\n");
		return;
	}

	s_initialized = true;

	S_AllocChannels();
	S_StopAllSounds();

	S_SfxInfo_f();

	if (!s_ignoreALErrors->integer)
		S_CheckForErrors();

	Com_Printf("------------------------------------\n");
}

/*
 =================
 S_Shutdown
 =================
*/
void S_Shutdown (void){

	Cmd_RemoveCommand("play");
	Cmd_RemoveCommand("music");
	Cmd_RemoveCommand("stopsound");
	Cmd_RemoveCommand("soundlist");
	Cmd_RemoveCommand("sfxinfo");
	Cmd_RemoveCommand("snd_restart");

	if (!s_initialized)
		return;

	S_FreeSounds();
	S_FreeChannels();

	ALimp_Shutdown();

	s_initialized = false;
}
