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

#include "winquake.h"
#include "../qcommon/qcommon.h"
#include <float.h>

char CONSOLE_WINDOW_NAME[] = "Quake II Evolved";
char CONSOLE_WINDOW_CLASS_NAME[] = "Q2E";

#define CONSOLE_WINDOW_STYLE		(WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN|WS_GROUP)

#define MAX_OUTPUT					32768
#define MAX_INPUT					256
#define	MAX_PRINTMSG				8192

typedef struct {
	int			outLen;					// To keep track of output buffer len
	char		cmdBuffer[MAX_INPUT];	// Buffered input from dedicated console
	qboolean	timerActive;			// Timer is active (for fatal errors)
	qboolean	flashColor;				// If true, flash error message to red

	// Window stuff
	HWND		hWnd;
	HWND		hWndCopy;
	HWND		hWndClear;
	HWND		hWndQuit;
	HWND		hWndOutput;
	HWND		hWndInput;
	HWND		hWndMsg;
	HFONT		hFont;
	HFONT		hFontBold;
	HBRUSH		hBrushMsg;
	HBRUSH		hBrushOutput;
	HBRUSH		hBrushInput;
	WNDPROC		defOutputProc;
	WNDPROC		defInputProc;
} sysConsole_t;

static sysConsole_t	sys_console;

HINSTANCE			sys_hInstance;

unsigned			sys_msgTime;
unsigned			sys_frameTime;


/*
 =======================================================================

 DEDICATED CONSOLE

 =======================================================================
*/


/*
 =================
 Sys_GetCommand
 =================
*/
char *Sys_GetCommand (void){

	static char buffer[MAX_INPUT];

	if (!sys_console.cmdBuffer[0])
		return NULL;

	Q_strncpyz(buffer, sys_console.cmdBuffer, sizeof(buffer));
	sys_console.cmdBuffer[0] = 0;

	return buffer;
}

/*
 =================
 Sys_Print
 =================
*/
void Sys_Print (const char *text){

	char	buffer[MAX_PRINTMSG];
	int		len = 0;

	// Change \n to \r\n so it displays properly in the edit box and
	// remove color escapes
	while (*text){
		if (*text == '\n'){
			buffer[len++] = '\r';
			buffer[len++] = '\n';
		}
		else if (Q_IsColorString(text))
			text++;
		else
			buffer[len++] = *text;

		text++;
	}
	buffer[len] = 0;

	sys_console.outLen += len;
	if (sys_console.outLen >= MAX_OUTPUT){
		SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
		sys_console.outLen = len;
	}
	SendMessage(sys_console.hWndOutput, EM_REPLACESEL, FALSE, (LPARAM)buffer);

	// Scroll down
	SendMessage(sys_console.hWndOutput, EM_LINESCROLL, 0, 0xFFFF);
	SendMessage(sys_console.hWndOutput, EM_SCROLLCARET, 0, 0);
}

/*
 =================
 Sys_Error
 =================
*/
void Sys_Error (const char *fmt, ...){

	char	string[MAX_PRINTMSG];
	va_list	argPtr;
	MSG		msg;

	// Make sure all subsystems are down
	Com_Shutdown();

	va_start(argPtr, fmt);
	vsnprintf(string, sizeof(string), fmt, argPtr);
	va_end(argPtr);

	// Echo to console
	Sys_Print("\n");
	Sys_Print(string);
	Sys_Print("\n");

	// Display the message and set a timer so we can flash the text
	SetWindowText(sys_console.hWndMsg, string);
	SetTimer(sys_console.hWnd, 1, 1000, NULL);

	sys_console.timerActive = true;

	// Show/hide everything we need
	ShowWindow(sys_console.hWndMsg, SW_SHOW);
	ShowWindow(sys_console.hWndInput, SW_HIDE);

	Sys_ShowConsole(true);

	// Wait for the user to quit
	while (1){
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
			if (!GetMessage(&msg, NULL, 0, 0))
				Sys_Quit();

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Don't hog the CPU
		Sleep(25);
	}
}

/*
 =================
 Sys_ShowConsole
 =================
*/
void Sys_ShowConsole (qboolean show){

	if (!show){
		ShowWindow(sys_console.hWnd, SW_HIDE);
		return;
	}

	ShowWindow(sys_console.hWnd, SW_SHOW);
	UpdateWindow(sys_console.hWnd);
	SetForegroundWindow(sys_console.hWnd);
	SetFocus(sys_console.hWnd);

	// Set the focus to the input edit box if possible
	SetFocus(sys_console.hWndInput);

	// Scroll down
	SendMessage(sys_console.hWndOutput, EM_LINESCROLL, 0, 0xFFFF);
	SendMessage(sys_console.hWndOutput, EM_SCROLLCARET, 0, 0);
}

/*
 =================
 Sys_ConsoleProc
 =================
*/
static LONG WINAPI Sys_ConsoleProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	switch (uMsg){
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE){
			SetFocus(sys_console.hWndInput);
			return 0;
		}

		break;
	case WM_CLOSE:
		Sys_Quit();

		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED){
			if ((HWND)lParam == sys_console.hWndCopy){
				SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
				SendMessage(sys_console.hWndOutput, WM_COPY, 0, 0);
			}
			else if ((HWND)lParam == sys_console.hWndClear){
				SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
				SendMessage(sys_console.hWndOutput, WM_CLEAR, 0, 0);
			}
			else if ((HWND)lParam == sys_console.hWndQuit)
				Sys_Quit();
		}
		else if (HIWORD(wParam) == EN_VSCROLL)
			InvalidateRect(sys_console.hWndOutput, NULL, TRUE);

		break;
	case WM_CTLCOLOREDIT:
		if ((HWND)lParam == sys_console.hWndOutput){
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetBkColor((HDC)wParam, RGB(54, 66, 83));
			SetTextColor((HDC)wParam, RGB(255, 255, 255));
			return (LONG)sys_console.hBrushOutput;
		}
		else if ((HWND)lParam == sys_console.hWndInput){
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetBkColor((HDC)wParam, RGB(255, 255, 255));
			SetTextColor((HDC)wParam, RGB(0, 0, 0));
			return (LONG)sys_console.hBrushInput;
		}

		break;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == sys_console.hWndMsg){
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetBkColor((HDC)wParam, RGB(127, 127, 127));

			if (sys_console.flashColor)
				SetTextColor((HDC)wParam, RGB(255, 0, 0));
			else
				SetTextColor((HDC)wParam, RGB(0, 0, 0));

			return (LONG)sys_console.hBrushMsg;
		}

		break;
	case WM_TIMER:
		sys_console.flashColor = !sys_console.flashColor;
		InvalidateRect(sys_console.hWndMsg, NULL, TRUE);

		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 =================
 Sys_ConsoleEditProc
 =================
*/
static LONG WINAPI Sys_ConsoleEditProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	switch (uMsg){
	case WM_CHAR:
		if (hWnd == sys_console.hWndInput){
			if (wParam == VK_RETURN){
				if (GetWindowText(sys_console.hWndInput, sys_console.cmdBuffer, sizeof(sys_console.cmdBuffer))){
					SetWindowText(sys_console.hWndInput, "");

					Com_Printf("]%s\n", sys_console.cmdBuffer);
				}

				return 0;	// Keep it from beeping
			}
		}
		else if (hWnd == sys_console.hWndOutput)
			return 0;	// Read only

		break;
	case WM_VSCROLL:
		if (LOWORD(wParam) == SB_THUMBTRACK)
			return 0;

		break;
	}

	if (hWnd == sys_console.hWndOutput)
		return CallWindowProc(sys_console.defOutputProc, hWnd, uMsg, wParam, lParam);
	else if (hWnd == sys_console.hWndInput)
		return CallWindowProc(sys_console.defInputProc, hWnd, uMsg, wParam, lParam);
}

/*
 =================
 Sys_ShutdownConsole
 =================
*/
static void Sys_ShutdownConsole (void){

	if (sys_console.timerActive)
		KillTimer(sys_console.hWnd, 1);

	if (sys_console.hBrushMsg)
		DeleteObject(sys_console.hBrushMsg);
	if (sys_console.hBrushOutput)
		DeleteObject(sys_console.hBrushOutput);
	if (sys_console.hBrushInput)
		DeleteObject(sys_console.hBrushInput);

	if (sys_console.hFont)
		DeleteObject(sys_console.hFont);
	if (sys_console.hFontBold)
		DeleteObject(sys_console.hFontBold);

	if (sys_console.defOutputProc)
		SetWindowLong(sys_console.hWndOutput, GWL_WNDPROC, (LONG)sys_console.defOutputProc);
	if (sys_console.defInputProc)
		SetWindowLong(sys_console.hWndInput, GWL_WNDPROC, (LONG)sys_console.defInputProc);

	ShowWindow(sys_console.hWnd, SW_HIDE);
	DestroyWindow(sys_console.hWnd);
	UnregisterClass(CONSOLE_WINDOW_CLASS_NAME, sys_hInstance);
}

/*
 =================
 Sys_InitConsole
 =================
*/
static void Sys_InitConsole (void){

	WNDCLASSEX	wc;
	HDC			hDC;
	RECT		r;
	int			x, y, w, h;

	// Center the window in the desktop
	hDC = GetDC(0);
	w = GetDeviceCaps(hDC, HORZRES);
	h = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(0, hDC);

	r.left = (w - 540) / 2;
	r.top = (h - 455) / 2;
	r.right = r.left + 540;
	r.bottom = r.top + 455;

	AdjustWindowRect(&r, CONSOLE_WINDOW_STYLE, FALSE);

	x = r.left;
	y = r.top;
	w = r.right - r.left;
	h = r.bottom - r.top;

	wc.style			= 0;
	wc.lpfnWndProc		= (WNDPROC)Sys_ConsoleProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= sys_hInstance;
	wc.hIcon			= LoadIcon(sys_hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hIconSm			= 0;
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)COLOR_WINDOW;
	wc.lpszMenuName		= 0;
	wc.lpszClassName	= CONSOLE_WINDOW_CLASS_NAME;
	wc.cbSize			= sizeof(WNDCLASSEX);

	if (!RegisterClassEx(&wc)){
		MessageBox(NULL, "Could not register console window class", "ERROR", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		exit(0);
	}

	sys_console.hWnd = CreateWindowEx(0, CONSOLE_WINDOW_CLASS_NAME, CONSOLE_WINDOW_NAME, CONSOLE_WINDOW_STYLE, x, y, w, h, NULL, NULL, sys_hInstance, NULL);
	if (!sys_console.hWnd){
		UnregisterClass(CONSOLE_WINDOW_CLASS_NAME, sys_hInstance);

		MessageBox(NULL, "Could not create console window", "ERROR", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		exit(0);
	}

	sys_console.hWndMsg = CreateWindowEx(0, "STATIC", "", WS_CHILD | SS_SUNKEN, 5, 5, 530, 30, sys_console.hWnd, NULL, sys_hInstance, NULL);
	sys_console.hWndOutput = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE, 5, 40, 530, 350, sys_console.hWnd, NULL, sys_hInstance, NULL);
	sys_console.hWndInput = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 5, 395, 530, 20, sys_console.hWnd, NULL, sys_hInstance, NULL);
	sys_console.hWndCopy = CreateWindowEx(0, "BUTTON", "copy", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 5, 425, 70, 25, sys_console.hWnd, NULL, sys_hInstance, NULL);
	sys_console.hWndClear = CreateWindowEx(0, "BUTTON", "clear", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 80, 425, 70, 25, sys_console.hWnd, NULL, sys_hInstance, NULL);
	sys_console.hWndQuit = CreateWindowEx(0, "BUTTON", "quit", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 465, 425, 70, 25, sys_console.hWnd, NULL, sys_hInstance, NULL);

	// Create and set fonts
	sys_console.hFont = CreateFont(14, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");
	sys_console.hFontBold = CreateFont(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "System");

	SendMessage(sys_console.hWndMsg, WM_SETFONT, (WPARAM)sys_console.hFont, FALSE);
	SendMessage(sys_console.hWndOutput, WM_SETFONT, (WPARAM)sys_console.hFont, FALSE);
	SendMessage(sys_console.hWndInput, WM_SETFONT, (WPARAM)sys_console.hFont, FALSE);
	SendMessage(sys_console.hWndCopy, WM_SETFONT, (WPARAM)sys_console.hFontBold, FALSE);
	SendMessage(sys_console.hWndClear, WM_SETFONT, (WPARAM)sys_console.hFontBold, FALSE);
	SendMessage(sys_console.hWndQuit, WM_SETFONT, (WPARAM)sys_console.hFontBold, FALSE);

	// Create brushes
	sys_console.hBrushMsg = CreateSolidBrush(RGB(127, 127, 127));
	sys_console.hBrushOutput = CreateSolidBrush(RGB(54, 66, 83));
	sys_console.hBrushInput = CreateSolidBrush(RGB(255, 255, 255));

	// Subclass edit boxes
	sys_console.defOutputProc = (WNDPROC)SetWindowLong(sys_console.hWndOutput, GWL_WNDPROC, (LONG)Sys_ConsoleEditProc);
	sys_console.defInputProc = (WNDPROC)SetWindowLong(sys_console.hWndInput, GWL_WNDPROC, (LONG)Sys_ConsoleEditProc);

	// Set text limit for input edit box
	SendMessage(sys_console.hWndInput, EM_SETLIMITTEXT, (WPARAM)(MAX_INPUT-1), 0);

	// Show it
	Sys_ShowConsole(true);
}


// =====================================================================


/*
 =================
 Sys_FindFiles

 Walks a directory adding every file and/or subdirectory whose name
 matches the specified pattern.
 The file list is sorted alphabetically.
 =================
*/
int Sys_FindFiles (const char *path, const char *pattern, char **fileList, int maxFiles, qboolean addFiles, qboolean addDirs){

	WIN32_FIND_DATA	findInfo;
	HANDLE			findHandle;
	BOOL			findRes = TRUE;
	char			findPath[MAX_OSPATH], searchPath[MAX_OSPATH];
	int				fileCount = 0;

	Q_snprintfz(searchPath, sizeof(searchPath), "%s/*", path);

	findHandle = FindFirstFile(searchPath, &findInfo);
	if (findHandle == INVALID_HANDLE_VALUE)
		return 0;

	while (findRes == TRUE){
		// Check for invalid file name
		if (findInfo.cFileName[strlen(findInfo.cFileName)-1] == '.'){
			findRes = FindNextFile(findHandle, &findInfo);
			continue;
		}

		// Match pattern
		if (!Q_GlobMatch(pattern, findInfo.cFileName, false)){
			findRes = FindNextFile(findHandle, &findInfo);
			continue;
		}

		Q_snprintfz(findPath, sizeof(findPath), "%s/%s", path, findInfo.cFileName);

		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			// Add a directory
			if (addDirs && (fileCount < maxFiles))
				fileList[fileCount++] = CopyString(findPath);
		}
		else {
			// Add a file
			if (addFiles && (fileCount < maxFiles))
				fileList[fileCount++] = CopyString(findPath);
		}

		findRes = FindNextFile(findHandle, &findInfo);
	}

	FindClose(findHandle);

	// Sort the list
	qsort(fileList, fileCount, sizeof(char *), Q_SortStrcmp);

	return fileCount;
}

/*
 =================
 Sys_RecursiveFindFiles

 Walks a directory adding every file and/or subdirectory in the tree.
 The file list is sorted alphabetically.
 =================
*/
int Sys_RecursiveFindFiles (const char *path, char **fileList, int maxFiles, int fileCount, qboolean addFiles, qboolean addDirs){

	WIN32_FIND_DATA	findInfo;
	HANDLE			findHandle;
	BOOL			findRes = TRUE;
	char			findPath[MAX_OSPATH], searchPath[MAX_OSPATH];

	Q_snprintfz(searchPath, sizeof(searchPath), "%s/*", path);

	findHandle = FindFirstFile(searchPath, &findInfo);
	if (findHandle == INVALID_HANDLE_VALUE)
		return fileCount;

	while (findRes == TRUE){
		// Check for invalid file name
		if (findInfo.cFileName[strlen(findInfo.cFileName)-1] == '.'){
			findRes = FindNextFile(findHandle, &findInfo);
			continue;
		}

		Q_snprintfz(findPath, sizeof(findPath), "%s/%s", path, findInfo.cFileName);

		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			// Add a directory
			if (addDirs && (fileCount < maxFiles))
				fileList[fileCount++] = CopyString(findPath);
		}
		else {
			// Add a file
			if (addFiles && (fileCount < maxFiles))
				fileList[fileCount++] = CopyString(findPath);
		}

		// If a directory, recurse into it
		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			fileCount = Sys_RecursiveFindFiles(findPath, fileList, maxFiles, fileCount, addFiles, addDirs);

		findRes = FindNextFile(findHandle, &findInfo);
	}

	FindClose(findHandle);

	// Sort the list
	qsort(fileList, fileCount, sizeof(char *), Q_SortStrcmp);

	return fileCount;
}

/*
 =================
 Sys_CreateDirectory
 =================
*/
void Sys_CreateDirectory (const char *path){

	CreateDirectory(path, NULL);
}

/*
 =================
 Sys_RemoveDirectory
 =================
*/
void Sys_RemoveDirectory (const char *path){

	RemoveDirectory(path);
}

/*
 =================
 Sys_GetCurrentDirectory
 =================
*/
char *Sys_GetCurrentDirectory (void){

	static char	curDir[MAX_OSPATH];

	if (!GetCurrentDirectory(sizeof(curDir), curDir))
		Com_Error(ERR_FATAL, "Couldn't get current working directory");

	return curDir;
}

/*
 =================
 Sys_ScanForCD
 =================
*/
char *Sys_ScanForCD (void){

	static char	cdDir[MAX_OSPATH];
	char		drive[4], test[MAX_OSPATH];
	FILE		*f;

	drive[0] = 'C';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	// Scan the drives
	for (drive[0] = 'C'; drive[0] <= 'Z'; drive[0]++){
		if (GetDriveType(drive) != DRIVE_CDROM)
			continue;

		// Where Activision put the stuff...
		Q_snprintfz(cdDir, sizeof(cdDir), "%sinstall\\data\\baseq2", drive);
		Q_snprintfz(test, sizeof(test), "%sinstall\\data\\quake2.exe", drive);

		f = fopen(test, "r");
		if (f){
			fclose(f);
			return cdDir;
		}
	}

	cdDir[0] = 0;
	return cdDir;
}

/*
 =================
 Sys_GetClipboardText
 =================
*/
char *Sys_GetClipboardText (void){

	HANDLE	hClipboardData;
	LPVOID	lpData;
	DWORD	dwSize;
	char	*text;

	if (!OpenClipboard(NULL))
		return NULL;

	hClipboardData = GetClipboardData(CF_TEXT);
	if (!hClipboardData){
		CloseClipboard();
		return NULL;
	}

	lpData = GlobalLock(hClipboardData);
	if (!lpData){
		CloseClipboard();
		return NULL;
	}

	dwSize = GlobalSize(hClipboardData);

	text = Z_Malloc(dwSize+1);
	memcpy(text, lpData, dwSize);

	GlobalUnlock(hClipboardData);
	CloseClipboard();

	return text;
}

/*
 =================
 Sys_ShellExecute
 =================
*/
void Sys_ShellExecute (const char *path, const char *parms, qboolean exit){

	ShellExecute(NULL, "open", path, parms, NULL, SW_SHOW);

	if (exit)
		Sys_Quit();
}

/*
 =================
 Sys_Milliseconds
 =================
*/
int Sys_Milliseconds (void){

	static qboolean	initialized;
	static int		base;

	if (!initialized){
		// Let base retain 16 bits of effectively random data
		base = timeGetTime() & 0xffff0000;
		initialized = true;
	}

	return timeGetTime() - base;
}

/*
 =================
 Sys_PumpMessages

 Pump window messages
 =================
*/
void Sys_PumpMessages (void){

    MSG		msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
		if (!GetMessage(&msg, NULL, 0, 0))
			Sys_Quit();

		sys_msgTime = msg.time;

      	TranslateMessage(&msg);
      	DispatchMessage(&msg);
	}

	// Grab frame time
	sys_frameTime = timeGetTime();	// FIXME: should this be at start?
}

/*
 =================
 Sys_DetectCPU
 =================
*/
static qboolean Sys_DetectCPU (char *cpuString, int maxSize){
#if defined(__MINGW32__)
#elif defined(__LINUX__)
#else
#if defined _M_IX86

	char				vendor[16];
	int					stdBits, features, extFeatures;
	int					family, model;
	unsigned __int64	start, end, counter, stop, frequency;
	unsigned			speed;
	qboolean			hasMMX, hasMMXExt, has3DNow, has3DNowExt, hasSSE, hasSSE2;

	// Check if CPUID instruction is supported
	__try {
		__asm {
			mov eax, 0
			cpuid
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER){
		return false;
	}

	// Get CPU info
	__asm {
		; // Get vendor identifier
		mov eax, 0
		cpuid
		mov dword ptr[vendor+0], ebx
		mov dword ptr[vendor+4], edx
		mov dword ptr[vendor+8], ecx
		mov dword ptr[vendor+12], 0

		; // Get standard bits and features
		mov eax, 1
		cpuid
		mov stdBits, eax
		mov features, edx

		; // Check if extended functions are present
		mov extFeatures, 0
		mov eax, 80000000h
		cpuid
		cmp eax, 80000000h
		jbe NoExtFunction

		; // Get extended features
		mov eax, 80000001h
		cpuid
		mov extFeatures, edx

NoExtFunction:
	}

	// Get CPU name
	family = (stdBits >> 8) & 15;
	model = (stdBits >> 4) & 15;

	if (!Q_stricmp(vendor, "AuthenticAMD")){
		Q_strncpyz(cpuString, "AMD", maxSize);

		switch (family){
		case 5:
			switch (model){
			case 0:
			case 1:
			case 2:
			case 3:
				Q_strncatz(cpuString, " K5", maxSize);
				break;
			case 6:
			case 7:
				Q_strncatz(cpuString, " K6", maxSize);
				break;
			case 8:
				Q_strncatz(cpuString, " K6-2", maxSize);
				break;
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				Q_strncatz(cpuString, " K6-III", maxSize);
				break;
			}
			break;
		case 6:
			switch (model){
			case 1:		// 0.25 core
			case 2:		// 0.18 core
				Q_strncatz(cpuString, " Athlon", maxSize);
				break;
			case 3:		// Spitfire core
				Q_strncatz(cpuString, " Duron", maxSize);
				break;
			case 4:		// Thunderbird core
			case 6:		// Palomino core
				Q_strncatz(cpuString, " Athlon", maxSize);
				break;
			case 7:		// Morgan core
				Q_strncatz(cpuString, " Duron", maxSize);
				break;
			case 8:		// Thoroughbred core
			case 10:	// Barton core
				Q_strncatz(cpuString, " Athlon", maxSize);
				break;
			}
			break;
		}
	}
	else if (!Q_stricmp(vendor, "GenuineIntel")){
		Q_strncpyz(cpuString, "Intel", maxSize);

		switch (family){
		case 5:
			switch (model){
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 7:
			case 8:
				Q_strncatz(cpuString, " Pentium", maxSize);
				break;
			}
			break;
		case 6:
			switch (model){
			case 0:
			case 1:
				Q_strncatz(cpuString, " Pentium Pro", maxSize);
				break;
			case 3:
			case 5:		// Actual differentiation depends on cache settings
				Q_strncatz(cpuString, " Pentium II", maxSize);
				break;
			case 6:
				Q_strncatz(cpuString, " Celeron", maxSize);
				break;
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:	// Actual differentiation depends on cache settings
				Q_strncatz(cpuString, " Pentium III", maxSize);
				break;
			}
			break;
		case 15:
			Q_strncatz(cpuString, " Pentium 4", maxSize);
			break;
		}
	}
	else
		return false;

	// Check if RDTSC instruction is supported
	if ((features >> 4) & 1){
		// Measure CPU speed
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);

		__asm {
			rdtsc
			mov dword ptr[start+0], eax
			mov dword ptr[start+4], edx
		}

		QueryPerformanceCounter((LARGE_INTEGER *)&stop);
		stop += frequency;

		do {
			QueryPerformanceCounter((LARGE_INTEGER *)&counter);
		} while (counter < stop);

		__asm {
			rdtsc
			mov dword ptr[end+0], eax
			mov dword ptr[end+4], edx
		}

		speed = (unsigned)((end - start) / 1000000);

		Q_strncatz(cpuString, va(" %u MHz", speed), maxSize);
	}

	// Get extended instruction sets supported
	hasMMX = (features >> 23) & 1;
	hasMMXExt = (extFeatures >> 22) & 1;
	has3DNow = (extFeatures >> 31) & 1;
	has3DNowExt = (extFeatures >> 30) & 1;
	hasSSE = (features >> 25) & 1;
	hasSSE2 = (features >> 26) & 1;

	if (hasMMX || has3DNow || hasSSE){
		Q_strncatz(cpuString, " w/", maxSize);

		if (hasMMX){
			Q_strncatz(cpuString, " MMX", maxSize);
			if (hasMMXExt)
				Q_strncatz(cpuString, "+", maxSize);
		}
		if (has3DNow){
			Q_strncatz(cpuString, " 3DNow!", maxSize);
			if (has3DNowExt)
				Q_strncatz(cpuString, "+", maxSize);
		}
		if (hasSSE){
			Q_strncatz(cpuString, " SSE", maxSize);
			if (hasSSE2)
				Q_strncatz(cpuString, "2", maxSize);
		}
	}

	return true;

#else

	Q_strncpyz(cpuString, "Alpha AXP", maxSize);

	return true;

#endif
#endif
}

/*
 =================
 Sys_Init
 =================
*/
void Sys_Init (void){

	OSVERSIONINFO	osInfo;
	MEMORYSTATUS	memStatus;
	char			string[64];
	int				len;

	Cvar_Get("sys_hInstance", va("%i", sys_hInstance), CVAR_ROM);
	Cvar_Get("sys_wndProc", va("%i", MainWndProc), CVAR_ROM);

	// Get OS version info
	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&osInfo))
		Com_Error(ERR_FATAL, "Couldn't get OS version info");

	if (osInfo.dwMajorVersion < 4)
		Com_Error(ERR_FATAL, "Quake 2 Evolved requires Windows version 4 or greater");
	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Com_Error(ERR_FATAL, "Quake 2 Evolved doesn't run on Win32s");

	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT){
		if (osInfo.dwMajorVersion == 4)
			Q_strncpyz(string, "Windows NT", sizeof(string));
		else if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 0)
			Q_strncpyz(string, "Windows 2000", sizeof(string));
		else if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 1)
			Q_strncpyz(string, "Windows XP", sizeof(string));
		else
			Q_snprintfz(string, sizeof(string), "Windows %i.%i", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
	}
	else if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS){
		if (osInfo.dwMajorVersion == 4 && osInfo.dwMinorVersion == 0){
			if (osInfo.szCSDVersion[1] == 'C' || osInfo.szCSDVersion[1] == 'B')
				Q_strncpyz(string, "Windows 95 OSR2", sizeof(string));
			else
				Q_strncpyz(string, "Windows 95", sizeof(string));
		}
		else if (osInfo.dwMajorVersion == 4 && osInfo.dwMinorVersion == 10){
			if (osInfo.szCSDVersion[1] == 'A')
				Q_strncpyz(string, "Windows 98 SE", sizeof(string));
			else
				Q_strncpyz(string, "Windows 98", sizeof(string));
		}
		else if (osInfo.dwMajorVersion == 4 && osInfo.dwMinorVersion == 90)
			Q_strncpyz(string, "Windows ME", sizeof(string));
		else
			Q_snprintfz(string, sizeof(string), "Windows %i.%i", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
	}
	else
		Q_snprintfz(string, sizeof(string), "Windows %i.%i", osInfo.dwMajorVersion, osInfo.dwMinorVersion);

	Com_Printf("OS: %s\n", string);
	Cvar_Get("sys_osVersion", string, CVAR_ROM);

	// Get physical memory
	GlobalMemoryStatus(&memStatus);
	Q_snprintfz(string, sizeof(string), "%u", memStatus.dwTotalPhys >> 20);

	Com_Printf("RAM: %s MB\n", string);
	Cvar_Get("sys_ramMegs", string, CVAR_ROM);

	// Detect CPU
	Com_Printf("Detecting CPU... ");

	if (Sys_DetectCPU(string, sizeof(string))){
		Com_Printf("Found %s\n", string);
		Cvar_Get("sys_cpuString", string, CVAR_ROM);
	}
	else {
		Com_Printf("Forcing to 'Unknown'\n");
		Cvar_Get("sys_cpuString", "Unknown", CVAR_ROM);
	}

	// Get user name
	len = sizeof(string);
	if (GetUserName(string, &len))
		Cvar_Get("sys_userName", string, CVAR_ROM);
	else
		Cvar_Get("sys_userName", "", CVAR_ROM);

	// Initialize system timer
	timeBeginPeriod(1);
}

/*
 =================
 Sys_Quit
 =================
*/
void Sys_Quit (void){

	Com_Shutdown();

	timeEndPeriod(1);

	Sys_ShutdownConsole();

	exit(0);
}


/*
 =======================================================================

 DLL LOADING

 =======================================================================
*/

#if defined _M_IX86
#define DEBUG_DLL_DIR	"d:\\code\\q2e\\debug"
#elif defined _M_ALPHA
#define DEBUG_DLL_DIR	"d:\\code\\q2e\\debugaxp"
#endif

#if defined _M_IX86
#define GAMENAME		"q2e_gamex86.dll"
#define CGAMENAME		"q2e_cgamex86.dll"
#define UINAME			"q2e_uix86.dll"
#define OLDGAMENAME		"gamex86.dll"		// For compatibility
#elif defined _M_ALPHA
#define GAMENAME		"q2e_gameaxp.dll"
#define CGAMENAME		"q2e_cgameaxp.dll"
#define UINAME			"q2e_uiaxp.dll"
#define OLDGAMENAME		"gameaxp.dll"		// For compatibility
#endif

static HINSTANCE	sys_hInstGame;
static HINSTANCE	sys_hInstCGame;
static HINSTANCE	sys_hInstUI;


/*
 =================
 Sys_FreeLibrary
 =================
*/
void Sys_FreeLibrary (sysLib_t lib){

	HINSTANCE	*hInst;
	char		*libName;

	switch (lib){
	case LIB_GAME:
		hInst = &sys_hInstGame;
		libName = GAMENAME;

		break;
	case LIB_CGAME:
		hInst = &sys_hInstCGame;
		libName = CGAMENAME;

		break;
	case LIB_UI:
		hInst = &sys_hInstUI;
		libName = UINAME;

		break;
	default:
		Com_Error(ERR_FATAL, "Sys_FreeLibrary: bad lib (%i)", lib);
	}

	Com_Printf("Unloading DLL file '%s'...\n", libName);

	if (!*hInst)
		Com_Error(ERR_FATAL, "Sys_FreeLibrary: '%s' not loaded", libName);

	if (!FreeLibrary(*hInst))
		Com_Error(ERR_FATAL, "Sys_FreeLibrary: FreeLibrary() failed for '%s'", libName);

	*hInst = NULL;
}

/*
 =================
 Sys_LoadLibrary
 =================
*/
void *Sys_LoadLibrary (sysLib_t lib, void *import){

	HINSTANCE	*hInst;
	char		*libName;
	char		*apiName;
	void		*(*GetAPI) (void *);
	char		name[MAX_OSPATH];
	char		*path = NULL;

	switch (lib){
	case LIB_GAME:
		hInst = &sys_hInstGame;
		libName = GAMENAME;
		apiName = "GetGameAPI";

		break;
	case LIB_CGAME:
		hInst = &sys_hInstCGame;
		libName = CGAMENAME;
		apiName = "GetCGameAPI";

		break;
	case LIB_UI:
		hInst = &sys_hInstUI;
		libName = UINAME;
		apiName = "GetUIAPI";

		break;
	default:
		Com_Error(ERR_FATAL, "Sys_LoadLibrary: bad lib (%i)", lib);
	}

	Com_Printf("Loading DLL file %s...\n", libName);

	if (*hInst)
		Com_Error(ERR_FATAL, "Sys_LoadLibrary: '%s' already loaded", libName);

#ifdef DEBUG_DLL

	Q_snprintfz(name, sizeof(name), "%s\\%s", DEBUG_DLL_DIR, libName);
	if ((*hInst = LoadLibrary(name)) == NULL)
		Com_Error(ERR_FATAL, "Sys_LoadLibrary: LoadLibrary() failed for '%s'", libName);

#else

	// Run through the search paths
	while (1){
		path = FS_NextPath(path);
		if (!path)	// Couldn't find one anywhere
			Com_Error(ERR_FATAL, "Sys_LoadLibrary: LoadLibrary() failed for '%s'", libName);

		Q_snprintfz(name, sizeof(name), "%s\\%s", path, libName);
		if ((*hInst = LoadLibrary(name)) == NULL){
			if (lib == LIB_GAME){
				// For compatibility with the original game DLL
				Q_snprintfz(name, sizeof(name), "%s\\%s", path, OLDGAMENAME);
				if ((*hInst = LoadLibrary(name)) == NULL)
					continue;

				break;
			}

			continue;
		}

		break;
	}

#endif

	if ((GetAPI = (void *)GetProcAddress(*hInst, apiName)) == NULL){
		FreeLibrary(*hInst);
		Com_Error(ERR_FATAL, "Sys_LoadLibrary: GetProcAddress() failed for '%s'", apiName);
	}

	return GetAPI(import);
}


// =====================================================================


/*
 =================
 WinMain
 =================
*/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){

	int		time, oldTime, newTime;

	// Previous instances do not exist in Win32
	if (hPrevInstance)
		return FALSE;

	sys_hInstance = hInstance;

	// No abort/retry/fail errors
	SetErrorMode(SEM_FAILCRITICALERRORS);

	// Initialize the dedicated console
	Sys_InitConsole();

	// Initialize all the subsystems
	Com_Init(lpCmdLine);

	// Main message loop
	oldTime = Sys_Milliseconds();

	while (1){
		// Don't update unless needed
		if (!vid_activeApp || dedicated->integer)
			Sleep(5);

		Sys_PumpMessages();

		do {
			newTime = Sys_Milliseconds();
			time = newTime - oldTime;
		} while (time < 1);

		_controlfp(_PC_24, _MCW_PC);

		Com_Frame(time);

		oldTime = newTime;
	}

	// Never gets here
    return TRUE;
}
