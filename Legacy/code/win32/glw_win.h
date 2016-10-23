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


#ifndef __GLW_WIN_H__
#define __GLW_WIN_H__


#ifndef _WIN32
#error "You should not be including this file on this platform"
#endif


typedef struct {
	HINSTANCE	hInstance;
	void		*WndProc;

	HINSTANCE	hInstOpenGL;

	HWND		hWnd;
	HDC			hDC;
	HGLRC		hGLRC;

	qboolean	allowDisplayDepthChange;
	
	qboolean	allowDeviceGammaRamp;
	WORD		deviceGammaRamp[768];

	FILE		*logFile;
} glwState_t;

extern glwState_t	glwState;

void		GLW_SetDeviceGammaRamp (unsigned short *gammaRamp);
void		GLW_SwapBuffers (void);
void		GLW_Activate (qboolean active);
void		GLW_Init (void);
void		GLW_Shutdown (void);


#endif	// __GLW_WIN_H__
