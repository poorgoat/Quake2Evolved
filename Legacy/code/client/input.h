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


// input.h -- external (non-keyboard) input devices


#ifndef __INPUT_H__
#define __INPUT_H__


void IN_MouseEvent (int state);

// Add additional movement on top of the keyboard move cmd
void IN_Move (usercmd_t *cmd);

// Opportunity for devices to stick commands on the script buffer
void IN_Commands (void);

void IN_Frame (void);

void IN_ClearStates (void);

void IN_Activate (qboolean active);

void IN_Init (void);
void IN_Shutdown (void);


#endif	// __INPUT_H__
