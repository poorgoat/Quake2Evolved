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


// editor_win.c -- integrated editor (only light properties for now)


#include "winquake.h"
#include "../qcommon/qcommon.h"
#include "../qcommon/editor.h"


#define LIGHT_PROPERTIES_WINDOW_NAME		"Quake II Evolved Light Properties Editor"
#define LIGHT_PROPERTIES_WINDOW_CLASS		"Q2E LightProps"
#define LIGHT_PROPERTIES_WINDOW_STYLE		(WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

typedef struct {
	lightProperties_t	initialLightProperties;
	lightProperties_t	currentLightProperties;

	void				(*updateLight)(lightProperties_t *);
	void				(*removeLight)(void);
	void				(*closeLight)(void);

	HWND				hWnd;

	HWND				hWndFrame1;
	HWND				hWndFrame2;
	HWND				hWndFrame3;
	HWND				hWndFrame4;

	HWND				hWndX;
	HWND				hWndY;
	HWND				hWndZ;

	HWND				hWndOrigin;
	HWND				hWndOriginX;
	HWND				hWndOriginY;
	HWND				hWndOriginZ;

	HWND				hWndCenter;
	HWND				hWndCenterX;
	HWND				hWndCenterY;
	HWND				hWndCenterZ;

	HWND				hWndAngles;
	HWND				hWndAnglesX;
	HWND				hWndAnglesY;
	HWND				hWndAnglesZ;

	HWND				hWndRadius;
	HWND				hWndRadiusX;
	HWND				hWndRadiusY;
	HWND				hWndRadiusZ;

	HWND				hWndParallel;

	HWND				hWndDetailLevel;
	HWND				hWndDetailLevelValue;

	HWND				hWndPositioning;
	HWND				hWndOriginCenter;

	HWND				hWndXInc;
	HWND				hWndXDec;
	HWND				hWndYInc;
	HWND				hWndYDec;
	HWND				hWndZInc;
	HWND				hWndZDec;

	HWND				hWndCastShadows;

	HWND				hWndColor;
	HWND				hWndColorDisplay;

	HWND				hWndStyle;
	HWND				hWndStyleValue;

	HWND				hWndAlpha;
	HWND				hWndAlphaValue;

	HWND				hWndMaterial;
	HWND				hWndMaterialName;

	HWND				hWndApply;
	HWND				hWndReset;
	HWND				hWndRemove;
	HWND				hWndClose;

	HFONT				hFont;

	HBRUSH				hBrush;

	COLORREF			crCurrent;
	COLORREF			crCustom[16];
} lightPropertiesEditor_t;

static lightPropertiesEditor_t	e_lightPropertiesEditor;


/*
 =================
 E_GetLightProperties
 =================
*/
static void E_GetLightProperties (void){

	lightProperties_t	*lightProperties = &e_lightPropertiesEditor.currentLightProperties;
	char				string[MAX_OSPATH];
	int					result;

	// Get the current color
	lightProperties->materialParms[0] = GetRValue(e_lightPropertiesEditor.crCurrent) * (1.0/255);
	lightProperties->materialParms[1] = GetGValue(e_lightPropertiesEditor.crCurrent) * (1.0/255);
	lightProperties->materialParms[2] = GetBValue(e_lightPropertiesEditor.crCurrent) * (1.0/255);

	// Read the controls
	if (GetWindowText(e_lightPropertiesEditor.hWndOriginX, string, sizeof(string)))
		lightProperties->origin[0] = atof(string);
	else
		lightProperties->origin[0] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndOriginY, string, sizeof(string)))
		lightProperties->origin[1] = atof(string);
	else
		lightProperties->origin[1] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndOriginZ, string, sizeof(string)))
		lightProperties->origin[2] = atof(string);
	else
		lightProperties->origin[2] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndCenterX, string, sizeof(string)))
		lightProperties->center[0] = atof(string);
	else
		lightProperties->center[0] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndCenterY, string, sizeof(string)))
		lightProperties->center[1] = atof(string);
	else
		lightProperties->center[1] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndCenterZ, string, sizeof(string)))
		lightProperties->center[2] = atof(string);
	else
		lightProperties->center[2] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndAnglesX, string, sizeof(string)))
		lightProperties->angles[0] = atof(string);
	else
		lightProperties->angles[0] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndAnglesY, string, sizeof(string)))
		lightProperties->angles[1] = atof(string);
	else
		lightProperties->angles[1] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndAnglesZ, string, sizeof(string)))
		lightProperties->angles[2] = atof(string);
	else
		lightProperties->angles[2] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndRadiusX, string, sizeof(string)))
		lightProperties->radius[0] = atof(string);
	else
		lightProperties->radius[0] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndRadiusY, string, sizeof(string)))
		lightProperties->radius[1] = atof(string);
	else
		lightProperties->radius[1] = 0.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndRadiusZ, string, sizeof(string)))
		lightProperties->radius[2] = atof(string);
	else
		lightProperties->radius[2] = 0.0;

	result = SendMessage(e_lightPropertiesEditor.hWndParallel, BM_GETCHECK, 0, 0);
	if (result == BST_CHECKED)
		lightProperties->parallel = true;
	else
		lightProperties->parallel = false;

	result = SendMessage(e_lightPropertiesEditor.hWndDetailLevelValue, CB_GETCURSEL, 0, 0);
	if (result != CB_ERR)
		lightProperties->detailLevel = Clamp(result, 0, 10);
	else
		lightProperties->detailLevel = 10;

	result = SendMessage(e_lightPropertiesEditor.hWndCastShadows, BM_GETCHECK, 0, 0);
	if (result == BST_UNCHECKED)
		lightProperties->noShadows = true;
	else
		lightProperties->noShadows = false;

	result = SendMessage(e_lightPropertiesEditor.hWndStyleValue, CB_GETCURSEL, 0, 0);
	if (result != CB_ERR)
		lightProperties->style = Clamp(result, 0, MAX_LIGHTSTYLES-1);
	else
		lightProperties->style = 0;

	if (GetWindowText(e_lightPropertiesEditor.hWndAlphaValue, string, sizeof(string)))
		lightProperties->materialParms[3] = atof(string);
	else
		lightProperties->materialParms[3] = 1.0;

	if (GetWindowText(e_lightPropertiesEditor.hWndMaterialName, string, sizeof(string)))
		Q_strncpyz(lightProperties->material, string, sizeof(lightProperties->material));
	else
		Q_strncpyz(lightProperties->material, "_defaultLight", sizeof(lightProperties->material));
}

/*
 =================
 E_SetLightProperties
 =================
*/
static void E_SetLightProperties (void){

	lightProperties_t	*lightProperties = &e_lightPropertiesEditor.initialLightProperties;
	byte				r, g, b;

	e_lightPropertiesEditor.currentLightProperties = *lightProperties;

	// Set the current color
	r = 255 * Clamp(lightProperties->materialParms[0], 0.0, 1.0);
	g = 255 * Clamp(lightProperties->materialParms[1], 0.0, 1.0);
	b = 255 * Clamp(lightProperties->materialParms[2], 0.0, 1.0);

	e_lightPropertiesEditor.crCurrent = RGB(r, g, b);

	// Create the brush
	if (e_lightPropertiesEditor.hBrush)
		DeleteObject(e_lightPropertiesEditor.hBrush);

	e_lightPropertiesEditor.hBrush = CreateSolidBrush(RGB(r, g, b));

	// Update the controls
	SetWindowText(e_lightPropertiesEditor.hWndOriginX, va("%g", lightProperties->origin[0]));
	SetWindowText(e_lightPropertiesEditor.hWndOriginY, va("%g", lightProperties->origin[1]));
	SetWindowText(e_lightPropertiesEditor.hWndOriginZ, va("%g", lightProperties->origin[2]));

	SetWindowText(e_lightPropertiesEditor.hWndCenterX, va("%g", lightProperties->center[0]));
	SetWindowText(e_lightPropertiesEditor.hWndCenterY, va("%g", lightProperties->center[1]));
	SetWindowText(e_lightPropertiesEditor.hWndCenterZ, va("%g", lightProperties->center[2]));

	SetWindowText(e_lightPropertiesEditor.hWndAnglesX, va("%g", lightProperties->angles[0]));
	SetWindowText(e_lightPropertiesEditor.hWndAnglesY, va("%g", lightProperties->angles[1]));
	SetWindowText(e_lightPropertiesEditor.hWndAnglesZ, va("%g", lightProperties->angles[2]));

	SetWindowText(e_lightPropertiesEditor.hWndRadiusX, va("%g", lightProperties->radius[0]));
	SetWindowText(e_lightPropertiesEditor.hWndRadiusY, va("%g", lightProperties->radius[1]));
	SetWindowText(e_lightPropertiesEditor.hWndRadiusZ, va("%g", lightProperties->radius[2]));

	if (lightProperties->parallel)
		SendMessage(e_lightPropertiesEditor.hWndParallel, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	else
		SendMessage(e_lightPropertiesEditor.hWndParallel, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);

	SendMessage(e_lightPropertiesEditor.hWndDetailLevelValue, CB_SETCURSEL, (WPARAM)lightProperties->detailLevel, 0);

	SetWindowText(e_lightPropertiesEditor.hWndOriginCenter, "Origin");
	SendMessage(e_lightPropertiesEditor.hWndOriginCenter, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);

	if (lightProperties->noShadows)
		SendMessage(e_lightPropertiesEditor.hWndCastShadows, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	else
		SendMessage(e_lightPropertiesEditor.hWndCastShadows, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

	InvalidateRect(e_lightPropertiesEditor.hWndColorDisplay, NULL, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndStyleValue, CB_SETCURSEL, (WPARAM)lightProperties->style, 0);

	SetWindowText(e_lightPropertiesEditor.hWndAlphaValue, va("%g", lightProperties->materialParms[3]));

	SetWindowText(e_lightPropertiesEditor.hWndMaterialName, lightProperties->material);
}

/*
 =================
 E_LightPropertiesWndProc
 =================
*/
static LONG WINAPI E_LightPropertiesWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	switch (uMsg){
	case WM_CREATE:
		sys.hWndLightProps = hWnd;

		break;
	case WM_DESTROY:
		sys.hWndLightProps = NULL;

		break;
	case WM_CLOSE:
		ShowWindow(e_lightPropertiesEditor.hWnd, SW_HIDE);

		e_lightPropertiesEditor.closeLight();

		return 0;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED){
			CHOOSECOLOR	chooseColor;
			char		string[MAX_OSPATH];

			if ((HWND)lParam == e_lightPropertiesEditor.hWndOriginCenter){
				if (SendMessage(e_lightPropertiesEditor.hWndOriginCenter, BM_GETCHECK, 0, 0) == BST_CHECKED)
					SetWindowText(e_lightPropertiesEditor.hWndOriginCenter, "Center");
				else
					SetWindowText(e_lightPropertiesEditor.hWndOriginCenter, "Origin");

				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndXInc){
				if (SendMessage(e_lightPropertiesEditor.hWndOriginCenter, BM_GETCHECK, 0, 0) == BST_CHECKED){
					GetWindowText(e_lightPropertiesEditor.hWndCenterX, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndCenterX, va("%g", atof(string) + 1.0));
				}
				else {
					GetWindowText(e_lightPropertiesEditor.hWndOriginX, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndOriginX, va("%g", atof(string) + 1.0));
				}

				E_GetLightProperties();

				e_lightPropertiesEditor.updateLight(&e_lightPropertiesEditor.currentLightProperties);
				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndXDec){
				if (SendMessage(e_lightPropertiesEditor.hWndOriginCenter, BM_GETCHECK, 0, 0) == BST_CHECKED){
					GetWindowText(e_lightPropertiesEditor.hWndCenterX, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndCenterX, va("%g", atof(string) - 1.0));
				}
				else {
					GetWindowText(e_lightPropertiesEditor.hWndOriginX, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndOriginX, va("%g", atof(string) - 1.0));
				}

				E_GetLightProperties();

				e_lightPropertiesEditor.updateLight(&e_lightPropertiesEditor.currentLightProperties);
				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndYInc){
				if (SendMessage(e_lightPropertiesEditor.hWndOriginCenter, BM_GETCHECK, 0, 0) == BST_CHECKED){
					GetWindowText(e_lightPropertiesEditor.hWndCenterY, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndCenterY, va("%g", atof(string) + 1.0));
				}
				else {
					GetWindowText(e_lightPropertiesEditor.hWndOriginY, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndOriginY, va("%g", atof(string) + 1.0));
				}

				E_GetLightProperties();

				e_lightPropertiesEditor.updateLight(&e_lightPropertiesEditor.currentLightProperties);
				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndYDec){
				if (SendMessage(e_lightPropertiesEditor.hWndOriginCenter, BM_GETCHECK, 0, 0) == BST_CHECKED){
					GetWindowText(e_lightPropertiesEditor.hWndCenterY, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndCenterY, va("%g", atof(string) - 1.0));
				}
				else {
					GetWindowText(e_lightPropertiesEditor.hWndOriginY, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndOriginY, va("%g", atof(string) - 1.0));
				}

				E_GetLightProperties();

				e_lightPropertiesEditor.updateLight(&e_lightPropertiesEditor.currentLightProperties);
				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndZInc){
				if (SendMessage(e_lightPropertiesEditor.hWndOriginCenter, BM_GETCHECK, 0, 0) == BST_CHECKED){
					GetWindowText(e_lightPropertiesEditor.hWndCenterZ, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndCenterZ, va("%g", atof(string) + 1.0));
				}
				else {
					GetWindowText(e_lightPropertiesEditor.hWndOriginZ, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndOriginZ, va("%g", atof(string) + 1.0));
				}

				E_GetLightProperties();

				e_lightPropertiesEditor.updateLight(&e_lightPropertiesEditor.currentLightProperties);
				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndZDec){
				if (SendMessage(e_lightPropertiesEditor.hWndOriginCenter, BM_GETCHECK, 0, 0) == BST_CHECKED){
					GetWindowText(e_lightPropertiesEditor.hWndCenterZ, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndCenterZ, va("%g", atof(string) - 1.0));
				}
				else {
					GetWindowText(e_lightPropertiesEditor.hWndOriginZ, string, sizeof(string));
					SetWindowText(e_lightPropertiesEditor.hWndOriginZ, va("%g", atof(string) - 1.0));
				}

				E_GetLightProperties();

				e_lightPropertiesEditor.updateLight(&e_lightPropertiesEditor.currentLightProperties);
				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndColor){
				chooseColor.lStructSize = sizeof(CHOOSECOLOR);
				chooseColor.hwndOwner = e_lightPropertiesEditor.hWnd;
				chooseColor.hInstance = NULL;
				chooseColor.rgbResult = e_lightPropertiesEditor.crCurrent;
				chooseColor.lpCustColors = e_lightPropertiesEditor.crCustom;
				chooseColor.Flags = CC_FULLOPEN | CC_RGBINIT;
				chooseColor.lCustData = 0;
				chooseColor.lpfnHook = NULL;
				chooseColor.lpTemplateName = NULL;

				if (ChooseColor(&chooseColor)){
					e_lightPropertiesEditor.crCurrent = chooseColor.rgbResult;

					if (e_lightPropertiesEditor.hBrush)
						DeleteObject(e_lightPropertiesEditor.hBrush);

					e_lightPropertiesEditor.hBrush = CreateSolidBrush(chooseColor.rgbResult);

					InvalidateRect(e_lightPropertiesEditor.hWndColorDisplay, NULL, FALSE);
				}

				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndApply){
				E_GetLightProperties();

				e_lightPropertiesEditor.updateLight(&e_lightPropertiesEditor.currentLightProperties);
				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndReset){
				E_SetLightProperties();

				e_lightPropertiesEditor.updateLight(&e_lightPropertiesEditor.initialLightProperties);
				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndRemove){
				ShowWindow(e_lightPropertiesEditor.hWnd, SW_HIDE);

				e_lightPropertiesEditor.removeLight();
				break;
			}

			if ((HWND)lParam == e_lightPropertiesEditor.hWndClose){
				ShowWindow(e_lightPropertiesEditor.hWnd, SW_HIDE);

				e_lightPropertiesEditor.closeLight();
				break;
			}
		}

		break;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == e_lightPropertiesEditor.hWndColorDisplay){
			SetBkColor((HDC)wParam, e_lightPropertiesEditor.crCurrent);
			SetTextColor((HDC)wParam, e_lightPropertiesEditor.crCurrent);

			return (LONG)e_lightPropertiesEditor.hBrush;
		}

		break;
	}

	// Pass all unhandled messages to DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 =================
 E_CreateLightPropertiesWindow
 =================
*/
void E_CreateLightPropertiesWindow (void){

	WNDCLASSEX	wndClass;
	HDC			hDC;
	RECT		r;
	int			x, y, w, h;
	int			i;

	// Center the window in the desktop
	hDC = GetDC(0);
	w = GetDeviceCaps(hDC, HORZRES);
	h = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(0, hDC);
	
	r.left = (w - 392) / 2;
	r.top = (h - 288) / 2;
	r.right = r.left + 392;
	r.bottom = r.top + 288;

	AdjustWindowRect(&r, LIGHT_PROPERTIES_WINDOW_STYLE, FALSE);
	
	x = r.left;
	y = r.top;
	w = r.right - r.left;
	h = r.bottom - r.top;

	// Register the frame class
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = (WNDPROC)E_LightPropertiesWndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = sys.hInstance;
	wndClass.hIcon = LoadIcon(sys.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hIconSm = 0;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndClass.lpszMenuName = 0;
	wndClass.lpszClassName = LIGHT_PROPERTIES_WINDOW_CLASS;

	if (!RegisterClassEx(&wndClass))
		return;

	// Create the window
	e_lightPropertiesEditor.hWnd = CreateWindowEx(0, LIGHT_PROPERTIES_WINDOW_CLASS, LIGHT_PROPERTIES_WINDOW_NAME, LIGHT_PROPERTIES_WINDOW_STYLE, x, y, w, h, NULL, NULL, sys.hInstance, NULL);
	if (!e_lightPropertiesEditor.hWnd){
		UnregisterClass(LIGHT_PROPERTIES_WINDOW_CLASS, sys.hInstance);
		return;
	}

	// Create the controls
	e_lightPropertiesEditor.hWndFrame1 = CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 8, 3, 178, 206, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndFrame2 = CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 194, 3, 190, 103, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndFrame3 = CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 194, 110, 190, 99, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndFrame4 = CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 8, 212, 376, 35, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndX = CreateWindowEx(0, "STATIC", "X", WS_CHILD | WS_VISIBLE | SS_CENTER, 54, 12, 40, 14, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndY = CreateWindowEx(0, "STATIC", "Y", WS_CHILD | WS_VISIBLE | SS_CENTER, 98, 12, 40, 14, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndZ = CreateWindowEx(0, "STATIC", "Z", WS_CHILD | WS_VISIBLE | SS_CENTER, 142, 12, 40, 14, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndOrigin = CreateWindowEx(0, "STATIC", "Origin", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 29, 38, 16, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndOriginX = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 54, 26, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndOriginY = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 98, 26, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndOriginZ = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 142, 26, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndCenter = CreateWindowEx(0, "STATIC", "Center", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 59, 38, 16, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndCenterX = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 54, 56, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndCenterY = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 98, 56, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndCenterZ = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 142, 56, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndAngles = CreateWindowEx(0, "STATIC", "Angles", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 89, 38, 16, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndAnglesX = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 54, 86, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndAnglesY = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 98, 86, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndAnglesZ = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 142, 86, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndRadius = CreateWindowEx(0, "STATIC", "Radius", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 119, 38, 16, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndRadiusX = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 54, 116, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndRadiusY = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 98, 116, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndRadiusZ = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 142, 116, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndParallel = CreateWindowEx(0, "BUTTON", "Parallel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 12, 158, 170, 16, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndDetailLevel = CreateWindowEx(0, "STATIC", "Detail Level", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 188, 120, 16, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndDetailLevelValue = CreateWindowEx(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 136, 184, 46, 200, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndPositioning = CreateWindowEx(0, "STATIC", "Positioning:", WS_CHILD | WS_VISIBLE | SS_LEFT, 198, 12, 182, 14, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndOriginCenter = CreateWindowEx(0, "BUTTON", "Origin", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_PUSHLIKE, 198, 58, 46, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndXInc = CreateWindowEx(0, "BUTTON", "X+", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 312, 58, 20, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndXDec = CreateWindowEx(0, "BUTTON", "X-", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 264, 58, 20, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndYInc = CreateWindowEx(0, "BUTTON", "Y+", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 288, 34, 20, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndYDec = CreateWindowEx(0, "BUTTON", "Y-", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 288, 82, 20, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndZInc = CreateWindowEx(0, "BUTTON", "Z+", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 360, 34, 20, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndZDec = CreateWindowEx(0, "BUTTON", "Z-", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 360, 82, 20, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndCastShadows = CreateWindowEx(0, "BUTTON", "Cast Shadows", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 198, 119, 182, 16, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndColor = CreateWindowEx(0, "BUTTON", "Color", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 198, 154, 46, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndColorDisplay = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_SUNKEN, 248, 154, 38, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndStyle = CreateWindowEx(0, "STATIC", "Style", WS_CHILD | WS_VISIBLE | SS_RIGHT, 292, 158, 38, 16, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndStyleValue = CreateWindowEx(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST, 334, 154, 46, 200, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndAlpha = CreateWindowEx(0, "STATIC", "Alpha / Distance to Opaque", WS_CHILD | WS_VISIBLE | SS_RIGHT, 198, 187, 138, 16, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndAlphaValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 340, 184, 40, 20, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndMaterial = CreateWindowEx(0, "STATIC", "Material", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 225, 82, 16, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndMaterialName = CreateWindowEx(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWN | CBS_SORT | CBS_AUTOHSCROLL, 98, 222, 282, 200, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	e_lightPropertiesEditor.hWndApply = CreateWindowEx(0, "BUTTON", "Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 8, 256, 80, 24, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndReset = CreateWindowEx(0, "BUTTON", "Reset", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 92, 256, 80, 24, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndRemove = CreateWindowEx(0, "BUTTON", "Remove", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 220, 256, 80, 24, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);
	e_lightPropertiesEditor.hWndClose = CreateWindowEx(0, "BUTTON", "Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, 304, 256, 80, 24, e_lightPropertiesEditor.hWnd, NULL, sys.hInstance, NULL);

	// Create and set the font
	e_lightPropertiesEditor.hFont = CreateFont(14, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "MS Sans Serif");

	SendMessage(e_lightPropertiesEditor.hWndFrame1, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndFrame2, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndFrame3, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndFrame4, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndX, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndY, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndZ, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndOrigin, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndOriginX, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndOriginY, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndOriginZ, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndCenter, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndCenterX, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndCenterY, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndCenterZ, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndAngles, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndAnglesX, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndAnglesY, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndAnglesZ, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndRadius, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndRadiusX, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndRadiusY, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndRadiusZ, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndParallel, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndDetailLevel, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndDetailLevelValue, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndPositioning, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndOriginCenter, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndXInc, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndXDec, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndYInc, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndYDec, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndZInc, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndZDec, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndCastShadows, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndColor, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndColorDisplay, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndStyle, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndStyleValue, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndAlpha, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndAlphaValue, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndMaterial, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndMaterialName, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	SendMessage(e_lightPropertiesEditor.hWndApply, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndReset, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndRemove, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);
	SendMessage(e_lightPropertiesEditor.hWndClose, WM_SETFONT, (WPARAM)e_lightPropertiesEditor.hFont, FALSE);

	// Fill the combo boxes
	for (i = 0; i <= 10; i++)
		SendMessage(e_lightPropertiesEditor.hWndDetailLevelValue, CB_ADDSTRING, 0, (LPARAM)va("%i", i));

	for (i = 0; i < MAX_LIGHTSTYLES; i++)
		SendMessage(e_lightPropertiesEditor.hWndStyleValue, CB_ADDSTRING, 0, (LPARAM)va("%i", i));

	SendMessage(e_lightPropertiesEditor.hWndMaterialName, CB_ADDSTRING, 0, (LPARAM)"_defaultLight");

	// Set text limit for material combo box
	SendMessage(e_lightPropertiesEditor.hWndMaterialName, CB_LIMITTEXT, (WPARAM)(MAX_OSPATH-1), 0);

	// Create the brush
	e_lightPropertiesEditor.hBrush = CreateSolidBrush(RGB(255, 255, 255));

	// Set the current color
	e_lightPropertiesEditor.crCurrent = RGB(255, 255, 255);

	// Set the custom colors
	for (i = 0; i < 16; i++)
		e_lightPropertiesEditor.crCustom[i] = RGB(255, 255, 255);
}

/*
 =================
 E_DestroyLightPropertiesWindow
 =================
*/
void E_DestroyLightPropertiesWindow (void){

	if (!e_lightPropertiesEditor.hWnd)
		return;

	if (e_lightPropertiesEditor.hBrush)
		DeleteObject(e_lightPropertiesEditor.hBrush);

	if (e_lightPropertiesEditor.hFont)
		DeleteObject(e_lightPropertiesEditor.hFont);

	ShowWindow(e_lightPropertiesEditor.hWnd, SW_HIDE);
	DestroyWindow(e_lightPropertiesEditor.hWnd);
	UnregisterClass(LIGHT_PROPERTIES_WINDOW_CLASS, sys.hInstance);

	memset(&e_lightPropertiesEditor, 0, sizeof(lightPropertiesEditor_t));
}

/*
 =================
 E_SetLightPropertiesCallbacks
 =================
*/
void E_SetLightPropertiesCallbacks (void (*updateLight)(lightProperties_t *), void (*removeLight)(void), void (*closeLight)(void)){

	e_lightPropertiesEditor.updateLight = updateLight;
	e_lightPropertiesEditor.removeLight = removeLight;
	e_lightPropertiesEditor.closeLight = closeLight;
}

/*
 =================
 E_AddLightPropertiesMaterial
 =================
*/
void E_AddLightPropertiesMaterial (const char *name){

	if (!e_lightPropertiesEditor.hWnd)
		return;

	if (!Q_strnicmp("lights/", name, 7)){
		SendMessage(e_lightPropertiesEditor.hWndMaterialName, CB_ADDSTRING, 0, (LPARAM)name);
		return;
	}

	if (!Q_strnicmp("fogs/", name, 5)){
		SendMessage(e_lightPropertiesEditor.hWndMaterialName, CB_ADDSTRING, 0, (LPARAM)name);
		return;
	}
}

/*
 =================
 E_EditLightProperties
 =================
*/
void E_EditLightProperties (lightProperties_t *lightProperties){

	if (!e_lightPropertiesEditor.hWnd){
		e_lightPropertiesEditor.closeLight();
		return;
	}

	// Save the initial light properties
	e_lightPropertiesEditor.initialLightProperties = *lightProperties;

	// Set the current light properties
	E_SetLightProperties();

	// Show the window
	ShowWindow(e_lightPropertiesEditor.hWnd, SW_SHOW);
	UpdateWindow(e_lightPropertiesEditor.hWnd);
	SetForegroundWindow(e_lightPropertiesEditor.hWnd);
	SetFocus(e_lightPropertiesEditor.hWnd);
}

/*
 =================
 E_ApplyLightProperties
 =================
*/
void E_ApplyLightProperties (void){

	if (!e_lightPropertiesEditor.hWnd){
		e_lightPropertiesEditor.closeLight();
		return;
	}

	// Get the current light properties
	E_GetLightProperties();

	// Update the light
	e_lightPropertiesEditor.updateLight(&e_lightPropertiesEditor.currentLightProperties);
}
