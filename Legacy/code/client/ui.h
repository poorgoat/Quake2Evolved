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


#ifndef __UI_H__
#define __UI_H__


typedef enum {
	UI_CLOSEMENU,
	UI_MAINMENU,
	UI_INGAMEMENU
} uiActiveMenu_t;

void		UI_UpdateMenu (int realTime);
void		UI_KeyDown (int key);
void		UI_MouseMove (int x, int y);
void		UI_SetActiveMenu (uiActiveMenu_t activeMenu);
void		UI_AddServerToList (netAdr_t adr, const char *info);
qboolean	UI_IsVisible (void);
void		UI_Precache (void);
void		UI_Init (void);
void		UI_Shutdown (void);


#endif	// __UI_H__
