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


// in_win.c -- Windows mouse code


#include "winquake.h"
#include "../client/client.h"


#define NUM_MOUSE_BUTTONS	5

typedef struct {
	qboolean	initialized;
	qboolean	active;

	int			windowCenterX;
	int			windowCenterY;

	int			oldButtonState;
} mouse_t;

static mouse_t	mouse;

static qboolean	in_active;

cvar_t	*in_mouse;


/*
 =================
 IN_ActivateMouse

 Called when the window gains focus or changes in some way
 =================
*/
static void IN_ActivateMouse (void){

	int		width, height;
	RECT	windowRect;

	if (!mouse.initialized)
		return;

	if (mouse.active)
		return;
	mouse.active = true;

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	GetWindowRect(sys.hWndMain, &windowRect);

	if (windowRect.left < 0)
		windowRect.left = 0;
	if (windowRect.top < 0)
		windowRect.top = 0;
	if (windowRect.right >= width)
		windowRect.right = width - 1;
	if (windowRect.bottom >= height)
		windowRect.bottom = height - 1;

	mouse.windowCenterX = (windowRect.right + windowRect.left) / 2;
	mouse.windowCenterY = (windowRect.top + windowRect.bottom) / 2;

	SetCursorPos(mouse.windowCenterX, mouse.windowCenterY);

#ifndef _DEBUG
	ClipCursor(&windowRect);
#endif

	SetCapture(sys.hWndMain);

	while (ShowCursor(FALSE) >= 0)
		;
}

/*
 =================
 IN_DeactivateMouse

 Called when the window loses focus
 =================
*/
static void IN_DeactivateMouse (void){

	if (!mouse.initialized)
		return;

	if (!mouse.active)
		return;
	mouse.active = false;

	while (ShowCursor(TRUE) < 0)
		;

	ReleaseCapture();

	ClipCursor(NULL);
}

/*
 =================
 IN_MouseMove
 =================
*/
static void IN_MouseMove (usercmd_t *cmd){

	POINT	currentPos;
	int		x, y;

	if (!mouse.initialized)
		return;

	if (!mouse.active)
		return;

	// Find mouse movement
	if (!GetCursorPos(&currentPos))
		return;

	// Force the mouse to the center, so there's room to move
	SetCursorPos(mouse.windowCenterX, mouse.windowCenterY);

	x = currentPos.x - mouse.windowCenterX;
	y = currentPos.y - mouse.windowCenterY;

	if (!x && !y)
		return;

	CL_MouseMoveEvent(x, y);
}

/*
 =================
 IN_MouseEvent
 =================
*/
void IN_MouseEvent (int state){

	int		i;

	if (!mouse.initialized)
		return;

	if (!mouse.active)
		return;

	// Perform button actions
	for (i = 0; i < NUM_MOUSE_BUTTONS; i++){
		if ((state & (1<<i)) && !(mouse.oldButtonState & (1<<i)))
			CL_KeyEvent(K_MOUSE1 + i, true, sys.msgTime);

		if (!(state & (1<<i)) && (mouse.oldButtonState & (1<<i)))
			CL_KeyEvent(K_MOUSE1 + i, false, sys.msgTime);
	}	

	mouse.oldButtonState = state;
}

/*
 =================
 IN_Frame

 Called every frame, even if not generating commands
 =================
*/
void IN_Frame (void){

	if (!in_active){
		IN_DeactivateMouse();
		return;
	}

	if (!cls.glConfig.isFullscreen && (cls.state == CA_LOADING || CL_GetKeyDest() == KEY_CONSOLE)){
		IN_DeactivateMouse();
		return;
	}

	IN_ActivateMouse();

	// Post mouse movement events
	IN_MouseMove(NULL);
}

/*
 =================
 IN_ClearStates
 =================
*/
void IN_ClearStates (void){

	mouse.oldButtonState = 0;
}

/*
 =================
 IN_Activate

 Called when the main window gains or loses focus.
 The window may have been destroyed and recreated between a deactivate 
 and an activate.
 =================
*/
void IN_Activate (qboolean active){

	in_active = active;

	if (in_active)
		IN_ActivateMouse();
	else
		IN_DeactivateMouse();
}

/*
 =================
 IN_Restart_f
 =================
*/
static void IN_Restart_f (void){

	IN_Shutdown();
	IN_Init();
}

/*
 =================
 IN_Init
 =================
*/
void IN_Init (void){

	Com_Printf("------- Input Initialization -------\n");

	// Register our variables and commands
    in_mouse = Cvar_Get("in_mouse",	"1", CVAR_ARCHIVE | CVAR_LATCH, "Don't check for mouse input");

	Cmd_AddCommand("in_restart", IN_Restart_f, "Restart the input system");

	if (!in_mouse->integerValue){
		Com_Printf("...mouse is not active\n");
		Com_Printf("------------------------------------\n");
		return;
	}

	if (!GetSystemMetrics(SM_MOUSEPRESENT))
		Com_Printf("...no mouse detected\n");
	else {
		Com_Printf("...mouse detected\n");

		mouse.initialized = true;

		IN_ActivateMouse();
	}

	Com_Printf("------------------------------------\n");
}

/*
 =================
 IN_Shutdown
 =================
*/
void IN_Shutdown (void){

	Cmd_RemoveCommand("in_restart");

	IN_DeactivateMouse();

	memset(&mouse, 0, sizeof(mouse_t));
}
