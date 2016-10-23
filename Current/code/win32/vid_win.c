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


static byte		vid_scanCodeToKey[128] = {
	0			, K_ESCAPE		, '1'			, '2'			,
	'3'			, '4'			, '5'			, '6'			,
	'7'			, '8'			, '9'			, '0'			,
	'-'			, '='			, K_BACKSPACE	, K_TAB			,
	'q'			, 'w'			, 'e'			, 'r'			,
	't'			, 'y'			, 'u'			, 'i'			,
	'o'			, 'p'			, '['			, ']'			,
	K_ENTER		, K_CTRL		, 'a'			, 's'			,
	'd'			, 'f'			, 'g'			, 'h'			,
	'j'			, 'k'			, 'l'			, ';'			,
	'\''		, '`'			, K_SHIFT		, '\\'			,
	'z'			, 'x'			, 'c'			, 'v'			,
	'b'			, 'n'			, 'm'			, ','			,
	'.'			, '/'			, K_SHIFT		, K_KP_STAR		,
	K_ALT		, ' '			, K_CAPSLOCK	, K_F1			,
	K_F2		, K_F3			, K_F4			, K_F5			,
	K_F6		, K_F7			, K_F8			, K_F9			,
	K_F10		, K_PAUSE		, 0				, K_HOME		,
	K_UPARROW	, K_PGUP		, K_KP_MINUS	, K_LEFTARROW	,
	K_KP_5		, K_RIGHTARROW	, K_KP_PLUS		, K_END			,
	K_DOWNARROW	, K_PGDN		, K_INS			, K_DEL			,
	0			, 0				, 0				, K_F11			,
	K_F12		, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0
}; 

static UINT		vid_msgMouseWheel;

static qboolean	vid_initialized;

cvar_t			*vid_xPos;
cvar_t			*vid_yPos;


/*	
 =================
 VID_MapKey

 Map from Windows to Quake key numbers
 =================
*/
int VID_MapKey (int key){

	int			result;
	int			scanCode;
	qboolean	isExtended;

	scanCode = (key >> 16) & 255;
	if (scanCode > 127)
		return 0;

	if (key & (1 << 24))
		isExtended = true;
	else
		isExtended = false;

	result = vid_scanCodeToKey[scanCode];

	if (!isExtended){
		switch (result){
		case K_INS:
			return K_KP_INS;
		case K_DEL:
			return K_KP_DEL;
		case K_HOME:
			return K_KP_HOME;
		case K_END:
			return K_KP_END;
		case K_PGUP:
			return K_KP_PGUP;
		case K_PGDN:
			return K_KP_PGDN;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		}
	}
	else {
		switch (result){
		case K_ENTER:
			return K_KP_ENTER;
		case '/':
			return K_KP_SLASH;
		case K_PAUSE:
			return K_KP_NUMLOCK;
		}
	}

	return result;
}

/*
 =================
 VID_AppActivate
 =================
*/
static void VID_AppActivate (qboolean active){

	Com_DPrintf("VID_AppActivate( %i )\n", active);

	CL_ClearKeyStates();
	IN_ClearStates();

	// Restore/minimize on demand
	if (active){
		if (vid_initialized && sys.hWndMain && cls.glConfig.isFullscreen){
			ShowWindow(sys.hWndMain, SW_RESTORE);
			SetForegroundWindow(sys.hWndMain);
			SetFocus(sys.hWndMain);
		}

		S_Activate(true);
		IN_Activate(true);
		CDAudio_Activate(true);
	} else {
		CDAudio_Activate(false);
		IN_Activate(false);
		S_Activate(false);

		if (vid_initialized && sys.hWndMain && cls.glConfig.isFullscreen)
			ShowWindow(sys.hWndMain, SW_MINIMIZE);
	}

	sys.activeApp = active;
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
			CL_KeyEvent(K_MWHEELUP, true, sys.msgTime);
			CL_KeyEvent(K_MWHEELUP, false, sys.msgTime);
		} else {
			CL_KeyEvent(K_MWHEELDOWN, true, sys.msgTime);
			CL_KeyEvent(K_MWHEELDOWN, false, sys.msgTime);
		}

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg){
	case WM_CREATE:
		sys.hWndMain = hWnd;
		vid_msgMouseWheel = RegisterWindowMessage("MSWHEEL_ROLLMSG"); 
        
		break;
	case WM_DESTROY:
		sys.hWndMain = NULL;

        break;
	case WM_ACTIVATE:
		// We don't want to act like we're active if we're minimized
		if ((LOWORD(wParam) != WA_INACTIVE) && !(HIWORD(wParam)))
			VID_AppActivate(true);
		else
			VID_AppActivate(false);
		
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		
		break;
	case WM_MOVE:
		if (vid_initialized && sys.hWndMain && !cls.glConfig.isFullscreen){
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
			
			Cvar_ForceSet("vid_xPos", va("%i", x + r.left));
			Cvar_ForceSet("vid_yPos", va("%i", y + r.top));
		}
        break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		// This is complicated because Win32 seems to pack multiple 
		// mouse events into one update sometimes, so we always check 
		// all states and look for events
		{
			int		state = 0x00;

			if (wParam & MK_LBUTTON)
				state |= 0x01;
			if (wParam & MK_RBUTTON)
				state |= 0x02;
			if (wParam & MK_MBUTTON)
				state |= 0x04;
			if (wParam & MK_XBUTTON1)
				state |= 0x08;
			if (wParam & MK_XBUTTON2)
				state |= 0x10;

			IN_MouseEvent(state);
		}

		break;
	case WM_MOUSEWHEEL:
		if ((short)HIWORD(wParam) > 0){
			CL_KeyEvent(K_MWHEELUP, true, sys.msgTime);
			CL_KeyEvent(K_MWHEELUP, false, sys.msgTime);
		} else {
			CL_KeyEvent(K_MWHEELDOWN, true, sys.msgTime);
			CL_KeyEvent(K_MWHEELDOWN, false, sys.msgTime);
		}

		break;
	case WM_SYSCOMMAND:
		if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER)
			return 0;

        break;
	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN){
			if (vid_initialized){
				Cvar_ForceSet("r_fullscreen", va("%i", !cls.glConfig.isFullscreen));
				Cbuf_AddText("vid_restart\n");
			}

			return 0;
		// Fall through
		}
	case WM_KEYDOWN:
		CL_KeyEvent(VID_MapKey(lParam), true, sys.msgTime);
		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		CL_KeyEvent(VID_MapKey(lParam), false, sys.msgTime);
		break;
	case WM_CHAR:
		if (((lParam >> 16) & 255) == 41)
			break;		// Ignore the console key

		CL_CharEvent(wParam);
		break;
	case MM_MCINOTIFY:
		return CDAudio_MessageHandler(hWnd, uMsg, wParam, lParam);
		break;
    }
	
	// Pass all unhandled messages to DefWindowProc
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 =================
 FakeWndProc

 Fake window procedure
 =================
*/
LONG WINAPI FakeWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	switch (uMsg){
	case WM_CREATE:
		sys.hWndFake = hWnd;

		break;
	case WM_DESTROY:
		sys.hWndFake = NULL;

		break;
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
static void VID_Restart_f (void){

	// Shutdown UI, CD audio, sound and renderer
	UI_Shutdown();
	CDAudio_Shutdown();
	S_Shutdown();
	VID_Shutdown();

	// Clear the hunk
	Hunk_Clear();

	// Initialize renderer, sound, CD audio and UI
	VID_Init();
	S_Init();
	CDAudio_Init();
	UI_Init();

	// Load all local media
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
 VID_Init
 =================
*/
void VID_Init (void){

	Cvar_Get("vid_xPos", "3", CVAR_ROM, "Window X position");
	Cvar_Get("vid_yPos", "22", CVAR_ROM, "Window Y position");

	Cmd_AddCommand("vid_restart", VID_Restart_f, "Restart the video system");

	R_Init(true);

	vid_initialized = true;
}

/*
 =================
 VID_Shutdown
 =================
*/
void VID_Shutdown (void){

	if (!vid_initialized)
		return;

	Cmd_RemoveCommand("vid_restart");

	R_Shutdown(true);

	vid_initialized = false;
}
