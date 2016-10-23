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


#ifndef __SOUND_H__
#define __SOUND_H__


typedef struct {
	const char			*vendorString;
	const char			*rendererString;
	const char			*versionString;
	const char			*extensionsString;

	const char			*deviceList;
	const char			*deviceName;

	qboolean			eax;
} alConfig_t;

struct sfx_s	*S_RegisterSexedSound (const char *base, entity_state_t *ent);
struct sfx_s	*S_RegisterSound (const char *name);

void			S_StartSound (const vec3_t position, int entNum, int entChannel, struct sfx_s *sfx, float volume, float attenuation, int timeOfs);
void			S_StartLocalSound (struct sfx_s *sfx);

void			S_StartBackgroundTrack (const char *introTrack, const char *loopTrack);
void			S_StopBackgroundTrack (void);

void			S_StartStreaming (void);
void			S_StopStreaming (void);

void			S_StreamRawSamples (const byte *data, int samples, int rate, int width, int channels);

void			S_StopAllSounds (void);
void			S_FreeSounds (void);

void			S_Update (const vec3_t position, const vec3_t velocity, const vec3_t at, const vec3_t up);

void			S_GetALConfig (alConfig_t *config);

void			S_Activate (qboolean active);

void			S_Init (void);
void			S_Shutdown (void);


#endif	// __SOUND_H__
