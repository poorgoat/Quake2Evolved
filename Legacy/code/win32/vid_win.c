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


// vid_win.c -- main graphics interface module


#include "winquake.h"
#include "../client/client.h"


#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)  // Message that will be supported by the OS 
#endif

static UINT		vid_msgMouseWheel;

static qboolean	vid_initialized;

HWND			vid_hWnd = NULL;		// Main window handle for life of program
BOOL			vid_activeApp = FALSE;

cvar_t			*vid_xPos;
cvar_t			*vid_yPos;

extern unsigned	sys_msgTime;

void	GLW_Activate (qboolean active);


/*
 =================
 VID_AppActivate
 =================
*/
static void VID_AppActivate (qboolean active){

	Com_DPrintf("VID_AppActivate( %i )\n", active);

	Key_ClearStates();

	IN_ClearStates();

	// Minimize/restore on demand
	if (active){
		if (vid_initialized)
			GLW_Activate(true);

		S_Activate(true);
		IN_Activate(true);
		CDAudio_Activate(true);
	} else {
		CDAudio_Activate(false);
		IN_Activate(false);
		S_Activate(false);

		if (vid_initialized)
			GLW_Activate(false);
	}

	vid_activeApp = active;
}

/*
 =================
 MainWndProc

 Main window procedure
 =================
*/
LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	if (uMsg == vid_msgMouseWheel){
		// For Win95
		if (((int)wParam) > 0){
			Key_Event(K_MWHEELUP, true, sys_msgTime);
			Key_Event(K_MWHEELUP, false, sys_msgTime);
		}
		else {
			Key_Event(K_MWHEELDOWN, true, sys_msgTime);
			Key_Event(K_MWHEELDOWN, false, sys_msgTime);
		}

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg){
	case WM_CREATE:
		vid_hWnd = hWnd;
		vid_msgMouseWheel = RegisterWindowMessage("MSWHEEL_ROLLMSG"); 
        
		break;
	case WM_DESTROY:
		vid_hWnd = NULL;

        break;
	case WM_ACTIVATE:
		// We don't want to act like we're active if we're minimized
		if ((LOWORD(wParam) != WA_INACTIVE) && !(HIWORD(wParam)))
			VID_AppActivate(true);
		else
			VID_AppActivate(false);
		
		break;
	case WM_CLOSE:
		Sys_Quit();
		
		break;
	case WM_MOVE:
		if (vid_initialized && vid_hWnd && !cls.glConfig.isFullscreen){
			RECT	r;
			int		style;
			int		x, y;
				
			x = (short)LOWORD(lParam);    // Horizontal position 
			y = (short)HIWORD(lParam);    // Vertical position 

			r.left = 0;
			r.top = 0;
			r.right = 1;
			r.bottom = 1;

			style = GetWindowLong(hWnd, GWL_STYLE);
			AdjustWindowRect(&r, style, FALSE);

			Cvar_SetInteger("vid_xPos", x + r.left);
			Cvar_SetInteger("vid_yPos", y + r.top);
			vid_xPos->modified = false;
			vid_yPos->modified = false;
		}

        break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		// This is complicated because Win32 seems to pack multiple 
		// mouse events into one update sometimes, so we always check 
		// all states and look for events
		{
			int	state = 0;

			if (wParam & MK_LBUTTON)
				state |= 1;
			if (wParam & MK_RBUTTON)
				state |= 2;
			if (wParam & MK_MBUTTON)
				state |= 4;

			IN_MouseEvent(state);
		}

		break;
	case WM_MOUSEWHEEL:
		if ((short)HIWORD(wParam) > 0){
			Key_Event(K_MWHEELUP, true, sys_msgTime);
			Key_Event(K_MWHEELUP, false, sys_msgTime);
		}
		else {
			Key_Event(K_MWHEELDOWN, true, sys_msgTime);
			Key_Event(K_MWHEELDOWN, false, sys_msgTime);
		}

		break;
	case WM_SYSCOMMAND:
		if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER)
			return 0;

        break;
	case WM_SYSKEYDOWN:
		if (wParam == 13){
			if (vid_initialized){
				Cvar_ForceSet("r_fullscreen", va("%i", !cls.glConfig.isFullscreen));
				Cbuf_AddText("vid_restart\n");
			}

			return 0;
		}
		// Fall through

	case WM_KEYDOWN:
		Key_Event(Key_MapKey(lParam), true, sys_msgTime);

		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		Key_Event(Key_MapKey(lParam), false, sys_msgTime);

		break;
	case MM_MCINOTIFY:
		return CDAudio_MessageHandler(hWnd, uMsg, wParam, lParam);
    }
	
	// Pass all unhandled messages to DefWindowProc
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 =================
 VID_Restart_f

 Restart the video subsystem so it can pick up new parameters and flush
 all graphics
 =================
*/
void VID_Restart_f (void){

	UI_Shutdown();
	CDAudio_Stop();
	S_Shutdown();
	VID_Shutdown();

	// If server is not running, clear the hunk
	if (!Com_ServerState())
		Hunk_Clear();

	VID_Init();
	S_Init();
	UI_Init();

	CL_LoadLocalMedia();

	// Set menu visibility
	if (cls.state == CA_DISCONNECTED)
		UI_SetActiveMenu(UI_MAINMENU);
	else
		UI_SetActiveMenu(UI_CLOSEMENU);

	// Reload the level if needed
	if (cls.state > CA_LOADING){
		CL_Loading();
		CL_LoadGameMedia();
	}
}

/*
 =================
 VID_CheckChanges
 =================
*/
void VID_CheckChanges (void){

	// Update our window position
	if (vid_xPos->modified || vid_yPos->modified){
		if (vid_initialized && vid_hWnd && !cls.glConfig.isFullscreen){
			RECT	r;
			int		style;
			int		w, h;

			r.left = 0;
			r.top = 0;
			r.right = cls.glConfig.videoWidth;
			r.bottom = cls.glConfig.videoHeight;

			style = GetWindowLong(vid_hWnd, GWL_STYLE);
			AdjustWindowRect(&r, style, FALSE);

			w = r.right - r.left;
			h = r.bottom - r.top;

			MoveWindow(vid_hWnd, vid_xPos->integer, vid_yPos->integer, w, h, TRUE);
		}

		vid_xPos->modified = false;
		vid_yPos->modified = false;
	}
}

/*
 =================
 VID_Init
 =================
*/
void VID_Init (void){

	Com_Printf("----- Initializing Renderer -----\n");

	vid_xPos = Cvar_Get("vid_xPos", "3", CVAR_ARCHIVE);
	vid_yPos = Cvar_Get("vid_yPos", "22", CVAR_ARCHIVE);

	Cmd_AddCommand("vid_restart", VID_Restart_f);

	R_Init(true);

	vid_initialized = true;

	Com_Printf("---------------------------------\n");
}

/*
 =================
 VID_Shutdown
 =================
*/
void VID_Shutdown (void){

	Cmd_RemoveCommand("vid_restart");

	if (vid_initialized)
		R_Shutdown(true);

	vid_initialized = false;
}
