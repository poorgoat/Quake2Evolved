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

sysWin_t sys;

/*
 =======================================================================

 DEDICATED CONSOLE

 =======================================================================
*/

char CONSOLE_WINDOW_NAME[] = "Quake II Evolved";
char CONSOLE_WINDOW_CLASS[] = "Q2E";

#define CONSOLE_WINDOW_STYLE		(WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

#define MAX_OUTPUT					32768
#define MAX_INPUT					256

typedef struct {
	int			outputLength;			// To keep track of output buffer length
	char		inputBuffer[MAX_INPUT];	// Buffered input from dedicated console

	qboolean	onError;				// If true, we're on a fatal error
	qboolean	flashError;				// If true, flash error message to red

	// Window stuff
	HWND		hWnd;
	HWND		hWndError;
	HWND		hWndOutput;
	HWND		hWndInput;
	HWND		hWndCopy;
	HWND		hWndClear;
	HWND		hWndQuit;

	HFONT		hFont;
	HFONT		hFontBold;

	HBRUSH		hBrush;
	HBRUSH		hBrushGray;

	WNDPROC		defInputProc;
} sysConsole_t;

static sysConsole_t	sys_console;


/*
 ==================
 Sys_ConsoleWndProc
 ==================
*/
static LONG WINAPI Sys_ConsoleWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
	case WM_CREATE:
		sys.hWndConsole = hWnd;

		break;
	case WM_DESTROY:
		sys.hWndConsole = NULL;

		break;
	case WM_ACTIVATE:
		if (LOWORD (wParam) != WA_INACTIVE) {
			SetFocus (sys_console.hWndInput);
			return 0;
		}

		break;
	case WM_CLOSE:
		PostQuitMessage (0);

		break;
	case WM_COMMAND:
		if (HIWORD (wParam) == BN_CLICKED) {
			if ((HWND) lParam == sys_console.hWndCopy) {
				SendMessage (sys_console.hWndOutput, EM_SETSEL, 0, -1);
				SendMessage (sys_console.hWndOutput, WM_COPY, 0, 0);

				break;
			}

			if ((HWND) lParam == sys_console.hWndClear) {
				SendMessage (sys_console.hWndOutput, EM_SETSEL, 0, -1);
				SendMessage (sys_console.hWndOutput, EM_REPLACESEL, FALSE, (LPARAM)"");

				break;
			}

			if ((HWND) lParam == sys_console.hWndQuit)
				PostQuitMessage (0);
		}

		break;
	case WM_CTLCOLORSTATIC:
		if ((HWND) lParam == sys_console.hWndError) {
			if (sys_console.flashError) {
				SetBkColor ((HDC) wParam, RGB (128, 128, 128));
				SetTextColor ((HDC) wParam, RGB (255, 0, 0));
			} else {
				SetBkColor ((HDC) wParam, RGB (128, 128, 128));
				SetTextColor ((HDC) wParam, RGB (0, 0, 0));
			}

			return (LONG) sys_console.hBrushGray;
		}

		if ((HWND) lParam == sys_console.hWndOutput) {
			SetBkColor ((HDC) wParam, RGB (54, 66, 83));
			SetTextColor ((HDC) wParam, RGB (255, 255, 255));

			return (LONG) sys_console.hBrush;
		}

		break;
	case WM_TIMER:
		if (sys_console.onError) {
			sys_console.flashError = !sys_console.flashError;

			InvalidateRect (sys_console.hWndError, NULL, FALSE);
		}

		break;
	}

	// Pass all unhandled messages to DefWindowProc
	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}

/*
 ====================
 Sys_ConsoleInputProc
 ====================
*/
static LONG WINAPI Sys_ConsoleInputProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	char text[MAX_INPUT];

	switch (uMsg) {
	case WM_CHAR:
		if (wParam == VK_RETURN) {
			if (GetWindowText (sys_console.hWndInput, text, sizeof (text))) {
				SetWindowText (sys_console.hWndInput, "");

				Com_Printf ("]%s\n", text);

				Q_strncatz (sys_console.inputBuffer, text, sizeof (sys_console.inputBuffer));
				Q_strncatz (sys_console.inputBuffer, "\n", sizeof (sys_console.inputBuffer));
			}

			return 0;	// Keep it from beeping
		}

		break;
	}

	// Pass all unhandled messages to DefWindowProc
	return CallWindowProc (sys_console.defInputProc, hWnd, uMsg, wParam, lParam);
}


/*
 =================
 Sys_CreateConsole
 =================
*/
void Sys_CreateConsole (void) {

	WNDCLASSEX	wndClass;
	HDC			hDC;
	RECT		r;
	int			x, y, w, h;

	// Center the window in the desktop
	hDC = GetDC (0);
	w = GetDeviceCaps (hDC, HORZRES);
	h = GetDeviceCaps (hDC, VERTRES);
	ReleaseDC (0, hDC);

	r.left = (w - 540) / 2;
	r.top = (h - 450) / 2;
	r.right = r.left + 540;
	r.bottom = r.top + 450;

	AdjustWindowRect (&r, CONSOLE_WINDOW_STYLE, FALSE);

	x = r.left;
	y = r.top;
	w = r.right - r.left;
	h = r.bottom - r.top;

	// Register the frame class
	wndClass.cbSize = sizeof (WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = (WNDPROC) Sys_ConsoleWndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = sys.hInstance;
	wndClass.hIcon = LoadIcon (sys.hInstance, MAKEINTRESOURCE (IDI_ICON1));
	wndClass.hIconSm = 0;
	wndClass.hCursor = LoadCursor (NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH) COLOR_WINDOW;
	wndClass.lpszMenuName = 0;
	wndClass.lpszClassName = CONSOLE_WINDOW_CLASS;

	if (!RegisterClassEx (&wndClass)) {
		MessageBox (NULL, "Could not register console window class", "ERROR", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		exit (0);
	}

	// Create the window
	sys_console.hWnd = CreateWindowEx (0, CONSOLE_WINDOW_CLASS, CONSOLE_WINDOW_NAME, CONSOLE_WINDOW_STYLE, x, y, w, h, NULL, NULL, sys.hInstance, NULL);
	if (!sys_console.hWnd) {
		UnregisterClass (CONSOLE_WINDOW_CLASS, sys.hInstance);

		MessageBox (NULL, "Could not create console window", "ERROR", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		exit (0);
	}

	// Create the controls
	sys_console.hWndError = CreateWindowEx (0, "STATIC", "", WS_CHILD | SS_SUNKEN | SS_LEFT, 5, 5, 530, 30, sys_console.hWnd, NULL, sys.hInstance, NULL);
	sys_console.hWndOutput = CreateWindowEx (0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 5, 40, 530, 350, sys_console.hWnd, NULL, sys.hInstance, NULL);
	sys_console.hWndInput = CreateWindowEx (0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 5, 395, 530, 20, sys_console.hWnd, NULL, sys.hInstance, NULL);
	sys_console.hWndCopy = CreateWindowEx (0, "BUTTON", "copy", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_CENTER | BS_VCENTER, 5, 420, 70, 25, sys_console.hWnd, NULL, sys.hInstance, NULL);
	sys_console.hWndClear = CreateWindowEx (0, "BUTTON", "clear", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_CENTER | BS_VCENTER, 80, 420, 70, 25, sys_console.hWnd, NULL, sys.hInstance, NULL);
	sys_console.hWndQuit = CreateWindowEx (0, "BUTTON", "quit", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_CENTER | BS_VCENTER, 465, 420, 70, 25, sys_console.hWnd, NULL, sys.hInstance, NULL);

	// Create and set the fonts
	sys_console.hFont = CreateFont (14, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");
	sys_console.hFontBold = CreateFont (20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "System");

	SendMessage (sys_console.hWndError, WM_SETFONT, (WPARAM) sys_console.hFont, FALSE);
	SendMessage (sys_console.hWndOutput, WM_SETFONT, (WPARAM) sys_console.hFont, FALSE);
	SendMessage (sys_console.hWndInput, WM_SETFONT, (WPARAM) sys_console.hFont, FALSE);
	SendMessage (sys_console.hWndCopy, WM_SETFONT, (WPARAM) sys_console.hFontBold, FALSE);
	SendMessage (sys_console.hWndClear, WM_SETFONT, (WPARAM) sys_console.hFontBold, FALSE);
	SendMessage (sys_console.hWndQuit, WM_SETFONT, (WPARAM) sys_console.hFontBold, FALSE);

	// Create the brushes
	sys_console.hBrush = CreateSolidBrush (RGB (54, 66, 83));
	sys_console.hBrushGray = CreateSolidBrush (RGB (128, 128, 128));

	// Subclass input edit box
	sys_console.defInputProc = (WNDPROC) SetWindowLong (sys_console.hWndInput, GWL_WNDPROC, (LONG) Sys_ConsoleInputProc);

	// Set a timer for flashing error messages
	SetTimer (sys_console.hWnd, 1, 1000, NULL);

	// Set text limit for input edit box
	SendMessage (sys_console.hWndInput, EM_SETLIMITTEXT, (WPARAM) (MAX_INPUT - 1), 0);

	// Show it
	ShowWindow (sys_console.hWnd, SW_SHOW);
	UpdateWindow (sys_console.hWnd);
	SetForegroundWindow (sys_console.hWnd);
	SetFocus (sys_console.hWndInput);
}


/*
 ==================
 Sys_DestroyConsole
 ==================
*/
static void Sys_DestroyConsole (void){

	KillTimer (sys_console.hWnd, 1);

	if (sys_console.defInputProc)
		SetWindowLong (sys_console.hWndInput, GWL_WNDPROC, (LONG) sys_console.defInputProc);

	if (sys_console.hBrush)
		DeleteObject (sys_console.hBrush);
	if (sys_console.hBrushGray)
		DeleteObject (sys_console.hBrushGray);

	if (sys_console.hFont)
		DeleteObject (sys_console.hFont);
	if (sys_console.hFontBold)
		DeleteObject (sys_console.hFontBold);

	ShowWindow (sys_console.hWnd, SW_HIDE);
	DestroyWindow (sys_console.hWnd);
	UnregisterClass (CONSOLE_WINDOW_CLASS, sys.hInstance);

	memset (&sys_console, 0, sizeof (sysConsole_t));
}

/*
 =================
 Sys_ConsoleOutput
 =================
*/
static void Sys_ConsoleOutput (const char *text) {

	char	buffer[MAX_PRINTMSG];
	int		length = 0;

	// Copy into an intermediate buffer
	while (*text && (length < MAX_PRINTMSG - 2)){
		// Copy \r\n or convert \n\r to \r\n
		if ((*text == '\r' && text[1] == '\n') || (*text == '\n' && text[1] == '\r')){
			buffer[length++] = '\r';
			buffer[length++] = '\n';

			text += 2;
			continue;
		}

		// Convert \n or \r to \r\n
		if (*text == '\n' || *text == '\r'){
			buffer[length++] = '\r';
			buffer[length++] = '\n';

			text += 1;
			continue;
		}

		// Ignore color escape sequences
		if (Q_IsColorString(text)){
			text += 2;
			continue;
		}

		// Ignore non-printable characters
		if (*text < ' '){
			text += 1;
			continue;
		}

		// Copy the character
		buffer[length++] = *text++;
	}
	buffer[length] = 0;

	// Check for overflow
	sys_console.outputLength += length;
	if (sys_console.outputLength >= MAX_OUTPUT){
		sys_console.outputLength = length;

		SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
	}

	// Scroll down
	SendMessage(sys_console.hWndOutput, EM_LINESCROLL, 0, 0xFFFF);
	SendMessage(sys_console.hWndOutput, EM_SCROLLCARET, 0, 0);

	// Add the text
	SendMessage(sys_console.hWndOutput, EM_REPLACESEL, FALSE, (LPARAM)buffer);

	// Update
	UpdateWindow(sys_console.hWnd);
}

/*
 ================
 Sys_ConsoleInput
 ================
*/
static char *Sys_ConsoleInput (void) {

	static char	buffer[MAX_INPUT];

	if (!sys_console.inputBuffer[0])
		return NULL;

	Q_strncpyz (buffer, sys_console.inputBuffer, sizeof (buffer));
	sys_console.inputBuffer[0] = 0;

	return buffer;
}

/*
 ===================
 Sys_SetConsoleError
 ===================
*/
static void Sys_SetConsoleError (const char *text) {

	sys_console.onError = true;

	SetWindowText (sys_console.hWndError, text);

	ShowWindow (sys_console.hWndError, SW_SHOW);
	ShowWindow (sys_console.hWndInput, SW_HIDE);

	ShowWindow (sys_console.hWnd, SW_SHOW);
	UpdateWindow (sys_console.hWnd);
	SetForegroundWindow (sys_console.hWnd);
	SetFocus (sys_console.hWnd);
}

/*
 ===============
 Sys_ShowConsole
 ===============
*/
void Sys_ShowConsole (qboolean show) {

	if (!show) {
		ShowWindow (sys_console.hWnd, SW_HIDE);
		return;
	}

	ShowWindow (sys_console.hWnd, SW_SHOW);
	UpdateWindow (sys_console.hWnd);
	SetForegroundWindow (sys_console.hWnd);
	SetFocus (sys_console.hWndInput);
}

/*
 =======================================================================

 FILE SYSTEM

 =======================================================================
*/

#define MAX_LIST_FILES			65536


/*
 ==============================
 Sys_RecursiveListFilteredFiles
 ==============================
*/
static int Sys_RecursiveListFilteredFiles (const char *directory, const char *subdirectory, const char *filter, char **files, int fileCount) {

	WIN32_FIND_DATA	findInfo;
	HANDLE			findHandle;
	BOOL			findRes = TRUE;
	char			name[MAX_OSPATH];

	if (subdirectory[0])
		Q_snprintfz(name, sizeof (name), "%s/%s/*", directory, subdirectory);
	else
		Q_snprintfz(name, sizeof (name), "%s/*", directory);

	findHandle = FindFirstFile (name, &findInfo);
	if (findHandle == INVALID_HANDLE_VALUE)
		return fileCount;

	while (findRes == TRUE) {
		// Check for invalid file name
		if (!Q_stricmp (findInfo.cFileName, ".") || !Q_stricmp (findInfo.cFileName, "..")) {
			findRes = FindNextFile (findHandle, &findInfo);
			continue;
		}

		// If a directory, recurse into it
		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (subdirectory[0])
				Q_snprintfz (name, sizeof (name), "%s/%s", subdirectory, findInfo.cFileName);
			else
				Q_snprintfz (name, sizeof (name), "%s", findInfo.cFileName);

			fileCount = Sys_RecursiveListFilteredFiles (directory, name, filter, files, fileCount);
		}

		if (fileCount == MAX_LIST_FILES - 1)
			break;

		// Copy the name
		if (subdirectory[0])
			Q_snprintfz (name, sizeof (name), "%s/%s", subdirectory, findInfo.cFileName);
		else
			Q_snprintfz (name, sizeof (name), "%s", findInfo.cFileName);

		// Match filter
		if (!Q_MatchFilter (name, filter, false)) {
			findRes = FindNextFile (findHandle, &findInfo);
			continue;
		}

		// Add it to the list
		files[fileCount++] = CopyString (name);
		findRes = FindNextFile (findHandle, &findInfo);
	}

	FindClose (findHandle);

	return fileCount;
}

/*
 =====================
 Sys_ListFilteredFiles

 Returns a list of files and subdirectories that match the given filter.
 The returned list can optionally be sorted.
 =====================
*/
char **Sys_ListFilteredFiles (const char *directory, const char *filter, qboolean sort, int *numFiles) {

	char	**fileList;
	char	*files[MAX_LIST_FILES];
	int		fileCount = 0;
	int		i;

	// List files
	fileCount = Sys_RecursiveListFilteredFiles (directory, "", filter, files, 0);
	if (!fileCount) {
		*numFiles = 0;
		return NULL;
	}

	// Sort the list if needed
	if (sort)
		qsort (files, fileCount, sizeof (char *), Q_SortStrcmp);

	// Copy the list
	fileList = Z_Malloc ((fileCount + 1) * sizeof (char *));

	for (i = 0; i < fileCount; i++)
		fileList[i] = files[i];

	fileList[i] = NULL;
	*numFiles = fileCount;

	return fileList;
}

/*
 =============
 Sys_ListFiles

 Returns a list of files and subdirectories that match the given
 extension (which must include a leading '.' and must not contain
 wildcards).
 If extension is NULL, all the files will be returned and all the
 subdirectories ignored.
 If extension is "/", all the subdirectories will be returned and all
 the files ignored.
 The returned list can optionally be sorted.
 =============
*/
char **Sys_ListFiles (const char *directory, const char *extension, qboolean sort, int *numFiles) {

	WIN32_FIND_DATA	findInfo;
	HANDLE			findHandle;
	BOOL			findRes = TRUE;
	qboolean		listDirectories, listFiles;
	char			name[MAX_OSPATH];
	char			**fileList;
	char			*files[MAX_LIST_FILES];
	int				fileCount = 0;
	int				i;

	if (extension != NULL && !Q_stricmp (extension, "/")) {
		listDirectories = true;
		listFiles = false;
	} else {
		listDirectories = false;
		listFiles = true;
	}

	Q_snprintfz (name, sizeof (name), "%s/*", directory);

	findHandle = FindFirstFile (name, &findInfo);
	if (findHandle == INVALID_HANDLE_VALUE) {
		*numFiles = 0;
		return NULL;
	}

	while (findRes == TRUE) {
		// Check for invalid file name
		if (!Q_stricmp (findInfo.cFileName, ".") || !Q_stricmp (findInfo.cFileName, "..")) {
			findRes = FindNextFile (findHandle, &findInfo);
			continue;
		}

		if (fileCount == MAX_LIST_FILES - 1)
			break;

		// Add it to the list
		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (listDirectories)
				files[fileCount++] = CopyString (findInfo.cFileName);
		} else {
			if (listFiles) {
				if (extension) {
					Com_FileExtension (findInfo.cFileName, name, sizeof (name));
					if (!Q_stricmp (extension, name))
						files[fileCount++] = CopyString (findInfo.cFileName);
				} else {
					files[fileCount++] = CopyString (findInfo.cFileName);
				}
			}
		}

		findRes = FindNextFile (findHandle, &findInfo);
	}

	FindClose (findHandle);

	if (!fileCount) {
		*numFiles = fileCount;
		return NULL;
	}

	// Sort the list if needed
	if (sort)
		qsort (files, fileCount, sizeof (char *), Q_SortStrcmp);

	// Copy the list
	fileList = Z_Malloc ((fileCount + 1) * sizeof (char *));

	for (i = 0; i < fileCount; i++)
		fileList[i] = files[i];

	fileList[i] = NULL;
	*numFiles = fileCount;

	return fileList;
}

/*
 ================
 Sys_FreeFileList

 Frees the memory allocated by Sys_ListFilteredFiles and Sys_ListFiles
 ================
*/
void Sys_FreeFileList (char **fileList) {

	int	i;

	if (!fileList)
		return;

	for (i = 0; fileList[i]; i++)
		FreeString(fileList[i]);

	Z_Free (fileList);
}

/*
 ===================
 Sys_CreateDirectory
 ===================
*/
void Sys_CreateDirectory (const char *directory) {

	CreateDirectory (directory, NULL);
}

/*
 =======================
 Sys_GetCurrentDirectory
 =======================
*/
char *Sys_GetCurrentDirectory (void) {

	static char	directory[MAX_OSPATH];

	if (!GetCurrentDirectory (sizeof (directory), directory))
		Com_Error (ERR_FATAL, "Couldn't get current working directory");

	return directory;
}

/*
 =================
 Sys_ScanForCD
 =================
*/
char *Sys_ScanForCD (void) {

	static char	directory[MAX_OSPATH];
	char		drive[4], test[MAX_OSPATH];
	FILE		*f;

	drive[0] = 'C';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	// Scan the drives
	for (drive[0] = 'C'; drive[0] <= 'Z'; drive[0]++) {
		if (GetDriveType(drive) != DRIVE_CDROM)
			continue;

		// Where Activision put the stuff...
		Q_snprintfz (directory, sizeof (directory), "%sinstall\\data\\baseq2", drive);
		Q_snprintfz (test, sizeof (test), "%sinstall\\data\\quake2.exe", drive);

#ifdef SECURE
		f = fopen_s (f, test, "r");
#else
		f = fopen (test, "r");
#endif
		if (f) {
			fclose (f);
			return directory;
		}
	}

	directory[0] = 0;
	return directory;
}


// =====================================================================


/*
 =========
 Sys_Print
 =========
*/
void Sys_Print (const char *text) {

	Sys_ConsoleOutput (text);

#ifdef _DEBUG
	OutputDebugString (text);
#endif
}

/*
 =========
 Sys_Error
 =========
*/
void Sys_Error (const char *fmt, ...) {

	char	string[MAX_PRINTMSG];
	va_list	argPtr;
	MSG		msg;

	// Make sure all subsystems are down
	Com_Shutdown ();

	// Get the message
	va_start (argPtr, fmt);
#ifdef SECURE
	vsnprintf (string, sizeof (string), MAX_PRINTMSG, fmt, argPtr);
#else
	vsnprintf (string, sizeof (string), fmt, argPtr);
#endif
	va_end (argPtr);

	// Echo to console
	Sys_Print ("\n");
	Sys_Print (string);
	Sys_Print ("\n");

	// Show the error message
	Sys_SetConsoleError (string);

	// Wait for the user to quit
	while (1) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
}

/*
 ====================
 Sys_GetClipboardText
 ====================
*/
char *Sys_GetClipboardText (void) {

	HANDLE	hClipboardData;
	char	*data, *text;
	int		size;

	if (!OpenClipboard (NULL))
		return NULL;

	hClipboardData = GetClipboardData (CF_TEXT);
	if (!hClipboardData) {
		CloseClipboard ();
		return NULL;
	}

	data = GlobalLock (hClipboardData);
	if (!data) {
		CloseClipboard ();
		return NULL;
	}

	size = GlobalSize(hClipboardData);
	if (!size) {
		GlobalUnlock (hClipboardData);
		CloseClipboard ();
		return NULL;
	}

	text = Z_Malloc (size+1);
	memcpy (text, data, size);
	text[size] = 0;

	GlobalUnlock (hClipboardData);
	CloseClipboard ();

	return text;
}

/*
 ================
 Sys_ShellExecute
 ================
*/
void Sys_ShellExecute (const char *path, const char *parms, qboolean quit) {

	ShellExecute (NULL, "open", path, parms, NULL, SW_SHOW);

	if (quit)
		Sys_Quit ();
}

/*
 ================
 Sys_Milliseconds
 ================
*/
int Sys_Milliseconds (void) {

	static qboolean	initialized;
	static int		base;

	if (!initialized) {
		initialized = true;

		base = timeGetTime () & 0xFFFF0000;	// Let base retain 16 bits of effectively random data
	}

	return timeGetTime() - base;
}

/*
 =================
 Sys_GetClockTicks
 =================
*/
double Sys_GetClockTicks (void) {

	static qboolean	initialized;
	static double	ticksPerSecond;
	double			ticks;

	if (!initialized) {
		initialized = true;

		QueryPerformanceFrequency ((LARGE_INTEGER *) &ticksPerSecond);
	}

	QueryPerformanceCounter ((LARGE_INTEGER *) &ticks);

	return ticks / ticksPerSecond;
}

/*
 =============
 Sys_GetEvents
 =============
*/
int Sys_GetEvents (void) {

	MSG		msg;
	char	*cmd;

	// If the light editor is active, pump its message loop
	if (sys.hWndLightProps) {
		while (PeekMessage (&msg, sys.hWndLightProps, 0, 0, PM_NOREMOVE)) {
			if (!GetMessage (&msg, sys.hWndLightProps, 0, 0))
				Sys_Quit ();

			if (IsDialogMessage (sys.hWndLightProps, &msg))
				continue;

			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
	}

	// Pump the message loop for other windows
	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();

		sys.msgTime = msg.time;
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	// Check for console commands
	cmd = Sys_ConsoleInput ();
	if (cmd) {
		Cbuf_AddText (cmd);
		Cbuf_AddText ("\n");
	}

	// Return the current time
	return timeGetTime ();
}

/*
 =============
 Sys_DetectGPU
 =============
*/
static void Sys_DetectGPU (char *gpuString, int maxSize) {

	DWORD iDevNum = 0;
	DISPLAY_DEVICE lpDisplayDevice;
	DWORD dwFlags = 0;
	char *Detected;

    // Get the manufacturers based on product name...
	if (Q_stricmp (lpDisplayDevice.DeviceString, "RADEON")) {
		Detected = "ATI ";
		Detected = lpDisplayDevice.DeviceString;
	} else if (Q_stricmp (lpDisplayDevice.DeviceString, "GEFORCE")) {
		Detected = "Nvidia ";
		Detected = lpDisplayDevice.DeviceString;
	} else if (Q_stricmp (lpDisplayDevice.DeviceString, "VOODOO")) {
		Detected = "3dfx ";
		Detected = lpDisplayDevice.DeviceString;
	} else if (Q_stricmp (lpDisplayDevice.DeviceString, "VIRGE") || Q_stricmp (lpDisplayDevice.DeviceString, "SAVAGE") || Q_stricmp (lpDisplayDevice.DeviceString, "SAVAGE2000")) {
		Detected = "S3 ";
		Detected = lpDisplayDevice.DeviceString;
	} else {
		Detected = "Unknown ";
		Detected = lpDisplayDevice.DeviceString;
	}

    // Get the actual product name...
	lpDisplayDevice.cb = sizeof (lpDisplayDevice);
	EnumDisplayDevices (NULL, iDevNum, &lpDisplayDevice, dwFlags);
	Q_strncpyz (gpuString, Detected, maxSize);
}

/*
 =============
 Sys_DetectCPU
 =============
*/
static char Sys_DetectCPU (char *cpuString, int maxSize) {
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
	} else if (!Q_stricmp(vendor, "GenuineIntel")) {
		Q_strncpyz(cpuString, "Intel", maxSize);

		switch (family) {
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
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
			case 18:
			case 19:
			case 20:
			case 21:
			case 22:
			case 23:
			case 24:
			case 25:
			case 26:
			case 27:
			case 28:
				Q_strncatz(cpuString, " Atom", maxSize);
				break;
			}
			break;
		case 15:
			Q_strncatz(cpuString, " Pentium 4", maxSize);
			break;
		}
	} else return false;

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

		// TODO: recalculate mhz to ghz if the speed exceeds 999mhz...
		//if (speed > 999)
		//	Q_strncatz(cpuString, va(" @ %f GHz", (float)speed), maxSize);
		//else
			Q_strncatz(cpuString, va(" @ %u MHz", speed), maxSize);
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
 ==================
 Sys_GetVideoMemory
 ==================
*/
static int Sys_GetVideoMemory (void) {
	return 0;
}

/*
 =============
 Sys_DetectDSP
 =============
*/
static void Sys_DetectDSP (char *string, int maxSize) {
#if defined(__LINUX__)
#else
	MMRESULT        MmResult;
    WAVEOUTCAPS     MmCaps;
	int             i = 0;

	MmResult = waveOutGetDevCaps(i, &MmCaps, sizeof(MmCaps));
	Q_strncpyz (string, MmCaps.szPname, maxSize);
#endif
}

/*
 ========
 Sys_Init
 ========
*/
void Sys_Init (void) {

	OSVERSIONINFO	osInfo;
	MEMORYSTATUS	memStatus;
	char		    string[256];
	int	        	len;

	Com_Printf ("------- System Initialization -------\n");

	// Get OS version
	osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	if (!GetVersionEx (&osInfo))
		Com_Error (ERR_FATAL, "Couldn't get OS version info");

	if (osInfo.dwMajorVersion < 4)
		Com_Error (ERR_FATAL, "Quake II Evolved requires Windows version 4 or greater");
	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Com_Error (ERR_FATAL, "Quake II Evolved doesn't run on Win32s");

	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		if (osInfo.dwMajorVersion == 4)
			Q_strncpyz (string, "Microsoft Windows NT", sizeof (string));
		else if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 0)
			Q_strncpyz (string, "Microsoft Windows 2000", sizeof (string));
		else if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 1)
			Q_strncpyz (string, "Microsoft Windows XP", sizeof (string));
		else
			Q_snprintfz (string, sizeof (string), "Microsoft Windows v%i.%i", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
	} else if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		if (osInfo.dwMajorVersion == 4 && osInfo.dwMinorVersion == 0) {
			if (osInfo.szCSDVersion[1] == 'B' || osInfo.szCSDVersion[1] == 'C')
				Q_strncpyz (string, "Microsoft Windows 95 OSR2", sizeof (string));
			else
				Q_strncpyz (string, "Microsoft Windows 95", sizeof (string));
		} else if (osInfo.dwMajorVersion == 4 && osInfo.dwMinorVersion == 10) {
			if (osInfo.szCSDVersion[1] == 'A')
				Q_strncpyz (string, "Microsoft Windows 98 SE", sizeof (string));
			else
				Q_strncpyz (string, "Microsoft Windows 98", sizeof (string));
		} else if (osInfo.dwMajorVersion == 4 && osInfo.dwMinorVersion == 90)
			Q_strncpyz (string, "Microsoft Windows ME", sizeof (string));
		else
			Q_snprintfz (string, sizeof (string), "Microsoft Windows v%i.%i", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
	} else {
		Q_snprintfz (string, sizeof (string), "Microsoft Windows v%i.%i", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
	}

	Com_Printf ("OS: %s\n", string);
	Cvar_Get ("sys_osVersion", string, CVAR_ROM, "OS version");

	// Detect CPU
	Sys_DetectCPU (string, sizeof (string));
	Com_Printf ("Processor: %s\n", string);
	Cvar_Get ("sys_cpuString", string, CVAR_ROM, "CPU string");

	// Get physical memory
	GlobalMemoryStatus (&memStatus);
	Q_snprintfz (string, sizeof (string), "%u", (memStatus.dwTotalPhys >> 20) + 1);
	Com_Printf ("RAM: %s MB\n", string);
	Cvar_Get ("sys_ramMegs", string, CVAR_ROM, "MB of RAM");

	// Detect GPU
	Sys_DetectGPU (string, sizeof (string));
	Com_Printf ("Video Card: %s\n", string);
	Cvar_Get ("sys_gpuString", string, CVAR_ROM, "GPU string");

	// Get sound card
    Sys_DetectDSP (string, sizeof (string));
	Com_Printf ("Sound Card: %s\n", string);
	Cvar_Get ("sys_soundCard", string, CVAR_ROM, "sound card string");

	// Get user name
	len = sizeof (string);
	if (GetUserName (string, &len))
		Cvar_Get ("sys_userName", string, CVAR_ROM, "User name");
	else
		Cvar_Get ("sys_userName", "", CVAR_ROM, "User name");

	// Initialize system timer
	timeBeginPeriod (1);

	Com_Printf ("-------------------------------------\n");
}

/*
 ========
 Sys_Quit
 ========
*/
void Sys_Quit (void) {

	Com_Shutdown ();
	timeEndPeriod (1);
	Sys_DestroyConsole ();
	exit (0);
}


/*
 =======================================================================

 GAME DLL LOADING

 =======================================================================
*/

#if defined _M_IX86
#define DEBUG_DLL_PATH			"f:\\code\\q2e\\debug"
#elif defined _M_ALPHA
#define DEBUG_DLL_PATH			"f:\\code\\q2e\\debugaxp"
#endif

#if defined _M_IX86
#define GAMENAME				"gamex86.dll"
#elif defined _M_ALPHA
#define GAMENAME				"gameaxp.dll"
#endif


/*
 ============
 Sys_LoadGame
 ============
*/
void *Sys_LoadGame (void *import) {

	void	*(*GetGameAPI)(void *);
	char	name[MAX_OSPATH];
	char	*path = NULL;

	Com_Printf ("Loading game library...\n");

	if (sys.hInstGame)
		Com_Error (ERR_FATAL, "Sys_LoadGame: '%s' already loaded", GAMENAME);

#ifdef DEBUG_DLL

	Q_snprintfz (name, sizeof (name), "%s/%s", DEBUG_DLL_PATH, GAMENAME);
	if ((sys.hInstGame = LoadLibrary (name)) == NULL)
		Com_Error (ERR_FATAL, "Sys_LoadGame: LoadLibrary () failed for '%s'", GAMENAME);

#else

	// Run through the search paths
	while (1) {
		path = FS_NextPath (path);
		if (!path)		// Couldn't find one anywhere
			Com_Error (ERR_FATAL, "Sys_LoadGame: LoadLibrary() failed for '%s'", GAMENAME);

		Q_snprintfz (name, sizeof (name), "%s/%s", path, GAMENAME);
		if ((sys.hInstGame = LoadLibrary (name)) != NULL)
			break;
	}

#endif

	if ((GetGameAPI = (void *) GetProcAddress (sys.hInstGame, "GetGameAPI")) == NULL) {
		FreeLibrary (sys.hInstGame);
		Com_Error (ERR_FATAL, "Sys_LoadGame: GetProcAddress () failed for 'GetGameAPI'");
	}

	return GetGameAPI (import);
}

/*
 ==============
 Sys_UnloadGame
 ==============
*/
void Sys_UnloadGame (void) {

	Com_Printf ("Unloading game library...\n");

	if (!sys.hInstGame)
		Com_Error (ERR_FATAL, "Sys_UnloadGame: '%s' not loaded", GAMENAME);

	if (!FreeLibrary (sys.hInstGame))
		Com_Error (ERR_FATAL, "Sys_UnloadGame: FreeLibrary() failed for '%s'", GAMENAME);

	sys.hInstGame = NULL;
}


// =====================================================================


/*
 =================
 WinMain
 =================
*/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	// Previous instances do not exist in Win32
	if (hPrevInstance)
		return FALSE;

	sys.hInstance = hInstance;

	// No abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	// Placeholder for additional code related to splash/console displaying...
	Sys_CreateConsole ();

	// Initialize all the subsystems
	Com_Init (lpCmdLine);

	// Main message loop
	while (1) {
		// If not active or running a dedicated server, sleep a bit
		if (!sys.activeApp || com_dedicated->integerValue)
			Sleep(5);

		// Run a frame
		Com_Frame ();
	}

	// Never gets here
	return TRUE;
}
