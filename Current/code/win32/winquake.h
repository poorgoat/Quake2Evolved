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


// winquake.h -- Win32 specific Quake header file


#ifndef __WINQUAKE_H__
#define __WINQUAKE_H__


#ifndef WIN32
#error "You should not be including this file on this platform"
#endif


#include <windows.h>
#include <winsock.h>
#include <mmsystem.h>

#include "resource.h"


#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN		0x020B
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP		0x020C
#endif

#ifndef MK_XBUTTON1
#define MK_XBUTTON1			0x0020
#endif
#ifndef MK_XBUTTON2
#define MK_XBUTTON2			0x0040
#endif

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL		(WM_MOUSELAST+1)
#endif

typedef struct {
	HINSTANCE		hInstance;

	HWND			hWndMain;
	HWND			hWndFake;
	HWND			hWndConsole;
	HWND			hWndLightProps;

	HINSTANCE		hInstGame;

	BOOL			activeApp;
	DWORD			msgTime;
} sysWin_t;

extern sysWin_t		sys;

LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LONG WINAPI	FakeWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LONG		CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif	// __WINQUAKE_H__
