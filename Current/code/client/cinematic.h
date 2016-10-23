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


#ifndef __CINEMATIC_H__
#define __CINEMATIC_H__


#define CIN_SYSTEM		1	// Handled by the client system
#define CIN_SILENT		2	// Don't play audio
#define CIN_LOOP		4	// Looped playback
#define CIN_HOLD		8	// Hold at end

// Will run a frame of the cinematic but will not draw it. Will return
// false if the end of the cinematic has been reached.
qboolean	CIN_RunCinematic (void);

// Draws the current frame
void		CIN_DrawCinematic (int x, int y, int w, int h);

// Starts playing a cinematic
qboolean	CIN_PlayCinematic (const char *name, unsigned flags);

// Stops playing a cinematic
void		CIN_StopCinematic (void);


#endif	// __CINEMATIC_H__
