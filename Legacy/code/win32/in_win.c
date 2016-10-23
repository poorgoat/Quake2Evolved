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


// in_win.c -- DirectInput and Windows mouse and joystick code


#include "winquake.h"
#include "../client/client.h"


static qboolean	in_activeApp;
static qboolean	in_mLooking;

cvar_t			*in_mouse;
cvar_t			*in_initMouse;
cvar_t			*in_joystick;
cvar_t			*in_initJoystick;

extern unsigned	sys_msgTime;


/*
 =======================================================================

 MOUSE CONTROL

 =======================================================================
*/

#define MAX_MOUSE_BUTTONS	3

cvar_t		*m_showMouse;
cvar_t		*m_filter;
cvar_t		*m_pitch;
cvar_t		*m_yaw;
cvar_t		*m_forward;
cvar_t		*m_side;
cvar_t		*sensitivity;

typedef struct {
	qboolean	initialized;
	qboolean	active;

	int			numButtons;
	int			oldButtonState;
	int			oldMouseX;
	int			oldMouseY;

	int			originalAccel[3];
	qboolean	restoreAccel;
	int			windowCenterX;
	int			windowCenterY;
} mouse_t;

static mouse_t	mouse;


/*
 =================
 IN_MLookDown_f
 =================
*/
void IN_MLookDown_f (void){
	
	in_mLooking = true;
}

/*
 =================
 IN_MLookUp_f
 =================
*/
void IN_MLookUp_f (void){

	in_mLooking = false;

	if (!cl_freeLook->integer && cl_lookSpring->integer)
		CL_CenterView_f();
}

/*
 =================
 IN_ActivateMouse

 Called when the window gains focus or changes in some way
 =================
*/
static void IN_ActivateMouse (void){

	int		width, height;
	RECT	windowRect;
	int		accel[3] = {0, 0, 1};

	if (!mouse.initialized)
		return;

	if (mouse.active)
		return;
	mouse.active = true;

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	GetWindowRect(vid_hWnd, &windowRect);
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
	if (mouse.restoreAccel)
		SystemParametersInfo(SPI_SETMOUSE, 0, accel, 0);

	ClipCursor(&windowRect);
#endif

	SetCapture(vid_hWnd);

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

	if (mouse.restoreAccel)
		SystemParametersInfo(SPI_SETMOUSE, 0, mouse.originalAccel, 0);
}

/*
 =================
 IN_StartupMouse
 =================
*/
static void IN_StartupMouse (void){

	memset(&mouse, 0, sizeof(mouse_t));

	if (!in_initMouse->integer){
		Com_Printf("Skipping mouse initialization\n");
		return; 
	}

	if (!in_mouse->integer){
		Com_Printf("Mouse is not active\n");
		return;
	}

	if (!GetSystemMetrics(SM_MOUSEPRESENT)){
		Com_Printf(S_COLOR_YELLOW "WARNING: no mouse device installed\n");
		return;
	}

	mouse.numButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);
	if (mouse.numButtons > MAX_MOUSE_BUTTONS)
		mouse.numButtons = MAX_MOUSE_BUTTONS;

	mouse.restoreAccel = SystemParametersInfo(SPI_GETMOUSE, 0, mouse.originalAccel, 0);

	Com_Printf("...mouse detected\n");

	mouse.initialized = true;

	IN_ActivateMouse();
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
	for (i = 0; i < mouse.numButtons; i++){
		if ((state & (1<<i)) && !(mouse.oldButtonState & (1<<i)))
			Key_Event(K_MOUSE1 + i, true, sys_msgTime);
		
		if (!(state & (1<<i)) && (mouse.oldButtonState & (1<<i)))
			Key_Event(K_MOUSE1 + i, false, sys_msgTime);
	}	
		
	mouse.oldButtonState = state;
}

/*
 =================
 IN_MouseMove
 =================
*/
static void IN_MouseMove (usercmd_t *cmd){

	POINT	currentPos;
	int		mx, my;
	int		mouseX, mouseY;

	if (!mouse.initialized)
		return;

	if (!mouse.active)
		return;

	if (!GetCursorPos(&currentPos))
		return;

	mx = currentPos.x - mouse.windowCenterX;
	my = currentPos.y - mouse.windowCenterY;

	// Force the mouse to the center, so there's room to move
	if (mx || my)
		SetCursorPos(mouse.windowCenterX, mouse.windowCenterY);

	// If the menu is visible, move the menu cursor
	if (UI_IsVisible()){
		UI_MouseMove(mx, my);
		return;
	}

	// If game is not active or paused, don't move
	if ((cls.state != CA_ACTIVE || cls.loading) || cls.cinematicHandle || paused->integer)
		return;

	if (m_filter->integer){
		mouseX = (mx + mouse.oldMouseX) * 0.5;
		mouseY = (my + mouse.oldMouseY) * 0.5;
	}
	else {
		mouseX = mx;
		mouseY = my;
	}

	mouse.oldMouseX = mx;
	mouse.oldMouseY = my;

	mouseX *= sensitivity->value;
	mouseY *= sensitivity->value;

	if (cl.zooming){
		mouseX *= cl.zoomSensitivity;
		mouseY *= cl.zoomSensitivity;
	}

	if (m_showMouse->integer && (mouseX || mouseY))
		Com_Printf("%i %i\n", mouseX, mouseY);

	// Add mouse X/Y movement to cmd
	if ((in_strafe.state & 1) || (cl_lookStrafe->integer && in_mLooking))
		cmd->sidemove += m_side->value * mouseX;
	else
		cl.viewAngles[YAW] -= m_yaw->value * mouseX;

	if (!(in_strafe.state & 1) && (cl_freeLook->integer || in_mLooking))
		cl.viewAngles[PITCH] += m_pitch->value * mouseY;
	else
		cmd->forwardmove -= m_forward->value * mouseY;
}


/*
 =======================================================================

 JOYSTICK CONTROL

 =======================================================================
*/

// Joystick defines and variables
#define JOY_ABSOLUTE_AXIS	0x00000000		// Control like a joystick
#define JOY_RELATIVE_AXIS	0x00000010		// Control like a mouse, spinner, trackball
#define	JOY_MAX_AXES		6				// X, Y, Z, R, U, V
#define JOY_AXIS_X			0
#define JOY_AXIS_Y			1
#define JOY_AXIS_Z			2
#define JOY_AXIS_R			3
#define JOY_AXIS_U			4
#define JOY_AXIS_V			5

enum _ControlList {
	AxisNada,
	AxisForward,
	AxisLook,
	AxisSide,
	AxisTurn,
	AxisUp
};

DWORD		dwAxisFlags[JOY_MAX_AXES] = {
	JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNR, JOY_RETURNU, JOY_RETURNV
};

DWORD		dwAxisMap[JOY_MAX_AXES];
DWORD		dwControlMap[JOY_MAX_AXES];
PDWORD		pdwRawValue[JOY_MAX_AXES];

// None of these cvars are saved over a session. This means that 
// advanced controller configuration needs to be executed each time. 
// This avoids any problems with getting back to a default usage or when 
// changing from one controller to another. This way at least something
// works.
cvar_t		*joy_name;
cvar_t		*joy_advanced;
cvar_t		*joy_advAxisX;
cvar_t		*joy_advAxisY;
cvar_t		*joy_advAxisZ;
cvar_t		*joy_advAxisR;
cvar_t		*joy_advAxisU;
cvar_t		*joy_advAxisV;
cvar_t		*joy_forwardThreshold;
cvar_t		*joy_sideThreshold;
cvar_t		*joy_pitchThreshold;
cvar_t		*joy_yawThreshold;
cvar_t		*joy_forwardSensitivity;
cvar_t		*joy_sideSensitivity;
cvar_t		*joy_pitchSensitivity;
cvar_t		*joy_yawSensitivity;
cvar_t		*joy_upThreshold;
cvar_t		*joy_upSensitivity;

typedef struct {
	int			id;
	DWORD		flags;
	DWORD		numButtons;
	JOYINFOEX	info;

	qboolean	available;
	qboolean	advancedInit;
	qboolean	hasPOV;

	DWORD		oldButtonState;
	DWORD		oldPOVState;
} joystick_t;

static joystick_t	joystick;


/*
 =================
 IN_StartupJoystick
 =================
*/
static void IN_StartupJoystick (void){

	int			numDevs;
	JOYCAPS		jCaps;
	MMRESULT	mmResult;

	memset(&joystick, 0, sizeof(joystick_t));

	// Abort startup if user requests no joystick
	if (!in_initJoystick->integer){
		Com_Printf("Skipping joystick initialization\n");
		return; 
	}

	if (!in_joystick->integer){
		Com_Printf("Joystick is not active\n");
		return;
	}
 
	// Verify joystick driver is present
	if ((numDevs = joyGetNumDevs()) == 0){
		Com_Printf(S_COLOR_YELLOW "WARNING: no joystick device installed\n");
		return;
	}

	// Cycle through the joystick ids for the first valid one
	for (joystick.id = 0; joystick.id < numDevs; joystick.id++){
		memset(&joystick.info, 0, sizeof(joystick.info));
		joystick.info.dwSize = sizeof(joystick.info);
		joystick.info.dwFlags = JOY_RETURNCENTERED;

		if ((mmResult = joyGetPosEx(joystick.id, &joystick.info)) == JOYERR_NOERROR)
			break;
	} 

	// Abort startup if we didn't find a valid joystick
	if (mmResult != JOYERR_NOERROR){
		Com_Printf(S_COLOR_YELLOW "Joystick not found. No valid joysticks (%x)\n", mmResult);
		return;
	}

	// Get the capabilities of the selected joystick 
	// Abort startup if command fails
	memset(&jCaps, 0, sizeof(jCaps));
	if ((mmResult = joyGetDevCaps(joystick.id, &jCaps, sizeof(jCaps))) != JOYERR_NOERROR){
		Com_Printf(S_COLOR_YELLOW "Joystick not found. Invalid joystick capabilities (%x)\n", mmResult); 
		return;
	}

	// Save the joystick's number of buttons and POV status
	joystick.numButtons = jCaps.wNumButtons;
	joystick.hasPOV = jCaps.wCaps & JOYCAPS_HASPOV;

	// Mark the joystick as available and advanced initialization not 
	// completed
	// This is needed as cvars are not available during initialization
	joystick.available = true; 

	Com_Printf("Joystick detected\n"); 
}

/*
 =================
 IN_RawValuePointer
 =================
*/
static PDWORD IN_RawValuePointer (int axis){

	switch (axis){
	case JOY_AXIS_X:
		return &joystick.info.dwXpos;
	case JOY_AXIS_Y:
		return &joystick.info.dwYpos;
	case JOY_AXIS_Z:
		return &joystick.info.dwZpos;
	case JOY_AXIS_R:
		return &joystick.info.dwRpos;
	case JOY_AXIS_U:
		return &joystick.info.dwUpos;
	case JOY_AXIS_V:
		return &joystick.info.dwVpos;
	default:
		return 0;
	}
}

/*
 =================
 Joy_AdvancedUpdate_f

 Called once by IN_ReadJoystick and by user whenever an update is needed
 =================
*/
static void Joy_AdvancedUpdate_f (void){

	int		i;
	DWORD	dwTemp;

	// Initialize all the maps
	for (i = 0; i < JOY_MAX_AXES; i++){
		dwAxisMap[i] = AxisNada;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
		pdwRawValue[i] = IN_RawValuePointer(i);
	}

	if (joy_advanced->integer == 0){
		// Default joystick initialization

		// 2 axes only with joystick control
		dwAxisMap[JOY_AXIS_X] = AxisTurn;
		dwAxisMap[JOY_AXIS_Y] = AxisForward;
	}
	else {
		if (Q_stricmp(joy_name->string, "joystick") != 0)
			// Notify user of advanced controller
			Com_Printf("'%s' configured\n", joy_name->string);
		
		// Advanced initialization here
		// Data supplied by user via joy_axisn cvars
		dwTemp = (DWORD) joy_advAxisX->value;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advAxisY->value;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advAxisZ->value;
		dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advAxisR->value;
		dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advAxisU->value;
		dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advAxisV->value;
		dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
	}

	// Compute the axes to collect
	joystick.flags = JOY_RETURNCENTERED | JOY_RETURNBUTTONS | JOY_RETURNPOV;
	for (i = 0; i < JOY_MAX_AXES; i++){
		if (dwAxisMap[i] != AxisNada)
			joystick.flags |= dwAxisFlags[i];
	}
}

/*
 =================
 IN_JoystickCommands
 =================
*/
static void IN_JoystickCommands (void){

	int		i, keyIndex;
	DWORD	buttonState, povState;

	if (!joystick.available)
		return;
	
	// Loop through the joystick buttons
	// Key a joystick event or auxillary event for higher number buttons 
	// for each state change
	buttonState = joystick.info.dwButtons;
	for (i = 0; i < joystick.numButtons; i++){
		if ((buttonState & (1<<i)) && !(joystick.oldButtonState & (1<<i))){
			keyIndex = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event(keyIndex + i, true, 0);
		}

		if (!(buttonState & (1<<i)) && (joystick.oldButtonState & (1<<i))){
			keyIndex = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event(keyIndex + i, false, 0);
		}
	}
	joystick.oldButtonState = buttonState;

	if (joystick.hasPOV){
		// Convert POV information into 4 bits of state information
		// This avoids any potential problems related to moving from one
		// direction to another without going through the center 
		// position
		povState = 0;
		if (joystick.info.dwPOV != JOY_POVCENTERED){
			if (joystick.info.dwPOV == JOY_POVFORWARD)
				povState |= 0x01;
			if (joystick.info.dwPOV == JOY_POVRIGHT)
				povState |= 0x02;
			if (joystick.info.dwPOV == JOY_POVBACKWARD)
				povState |= 0x04;
			if (joystick.info.dwPOV == JOY_POVLEFT)
				povState |= 0x08;
		}
		
		// Determine which bits have changed and key an auxillary event 
		// for each change
		for (i = 0; i < 4; i++){
			if ((povState & (1<<i)) && !(joystick.oldPOVState & (1<<i)))
				Key_Event(K_AUX29 + i, true, 0);
			
			if (!(povState & (1<<i)) && (joystick.oldPOVState & (1<<i)))
				Key_Event(K_AUX29 + i, false, 0);
		}

		joystick.oldPOVState = povState;
	}
}

/*
 =================
 IN_ReadJoystick
 =================
*/
static qboolean IN_ReadJoystick (void){

	memset(&joystick.info, 0, sizeof(joystick.info));
	joystick.info.dwSize = sizeof(joystick.info);
	joystick.info.dwFlags = joystick.flags;

	if (joyGetPosEx(joystick.id, &joystick.info) == JOYERR_NOERROR)
		return true;
	else {
		// Read error occurred.
		// Turning off the joystick seems too harsh for 1 read error,
		// but what should be done?
		Com_DPrintf(S_COLOR_RED "joyGetPosEx() failed\n");
		return false;
	}
}

/*
 =================
 IN_JoystickMove
 =================
*/
static void IN_JoystickMove (usercmd_t *cmd){

	float	speed, aSpeed;
	float	fAxisValue;
	int		i;

	// Complete initialization if first time in
	// This is needed as cvars are not available at initialization time
	if (!joystick.advancedInit){
		Joy_AdvancedUpdate_f();
		joystick.advancedInit = true;
	}

	// Verify joystick is available and that the user wants to use it
	if (!joystick.available || !in_joystick->integer)
		return;

	// Collect the joystick data, if possible
	if (!IN_ReadJoystick())
		return;

	if ((in_speed.state & 1) ^ cl_run->integer)
		speed = 2;
	else
		speed = 1;

	aSpeed = speed * cls.frameTime;

	// Loop through the axes
	for (i = 0; i < JOY_MAX_AXES; i++){
		// Get the floating point zero-centered, potentially-inverted 
		// data for the current axis
		fAxisValue = (float) *pdwRawValue[i];
		// Move centerpoint to zero
		fAxisValue -= 32768.0;
		// Convert range from -32768..32767 to -1..1 
		fAxisValue /= 32768.0;

		switch (dwAxisMap[i]){
		case AxisForward:
			if ((joy_advanced->integer == 0) && in_mLooking){
				// User wants forward control to become look control
				if (fabs(fAxisValue) > joy_pitchThreshold->value){
					// If mouse invert is on, invert the joystick pitch 
					// value only absolute control support here 
					// (joy_advanced is false)
					if (m_pitch->value < 0.0)
						cl.viewAngles[PITCH] -= (fAxisValue * joy_pitchSensitivity->value) * aSpeed * cl_pitchSpeed->value;
					else
						cl.viewAngles[PITCH] += (fAxisValue * joy_pitchSensitivity->value) * aSpeed * cl_pitchSpeed->value;
				}
			}
			else {
				// User wants forward control to be forward control
				if (fabs(fAxisValue) > joy_forwardThreshold->value)
					cmd->forwardmove += (fAxisValue * joy_forwardSensitivity->value) * speed * cl_forwardSpeed->value;
			}

			break;
		case AxisSide:
			if (fabs(fAxisValue) > joy_sideThreshold->value)
				cmd->sidemove += (fAxisValue * joy_sideSensitivity->value) * speed * cl_sideSpeed->value;

			break;
		case AxisUp:
			if (fabs(fAxisValue) > joy_upThreshold->value)
				cmd->upmove += (fAxisValue * joy_upSensitivity->value) * speed * cl_upSpeed->value;

			break;
		case AxisTurn:
			if ((in_strafe.state & 1) || (cl_lookStrafe->integer && in_mLooking)){
				// User wants turn control to become side control
				if (fabs(fAxisValue) > joy_sideThreshold->value)
					cmd->sidemove -= (fAxisValue * joy_sideSensitivity->value) * speed * cl_sideSpeed->value;
			}
			else {
				// User wants turn control to be turn control
				if (fabs(fAxisValue) > joy_yawThreshold->value){
					if(dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewAngles[YAW] += (fAxisValue * joy_yawSensitivity->value) * aSpeed * cl_yawSpeed->value;
					else
						cl.viewAngles[YAW] += (fAxisValue * joy_yawSensitivity->value) * speed * 180.0;
				}
			}

			break;
		case AxisLook:
			if (in_mLooking){
				if (fabs(fAxisValue) > joy_pitchThreshold->value){
					// Pitch movement detected and pitch movement 
					// desired by user
					if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewAngles[PITCH] += (fAxisValue * joy_pitchSensitivity->value) * aSpeed * cl_pitchSpeed->value;
					else
						cl.viewAngles[PITCH] += (fAxisValue * joy_pitchSensitivity->value) * speed * 180.0;
				}
			}

			break;
		}
	}
}


/*
 =======================================================================

 INPUT

 =======================================================================
*/


/*
 =================
 IN_Move
 =================
*/
void IN_Move (usercmd_t *cmd){

	if (!in_activeApp)
		return;

	IN_MouseMove(cmd);
	IN_JoystickMove(cmd);
}

/*
 =================
 IN_Commands
 =================
*/
void IN_Commands (void){

	if (!in_activeApp)
		return;

	IN_JoystickCommands();
}

/*
 =================
 IN_Frame

 Called every frame, even if not generating commands
 =================
*/
void IN_Frame (void){

	if (!in_activeApp || (!cls.glConfig.isFullscreen && Key_GetKeyDest() == KEY_CONSOLE))
		IN_DeactivateMouse();
	else
		IN_ActivateMouse();
}

/*
 =================
 IN_ClearStates
 =================
*/
void IN_ClearStates (void){

	mouse.oldButtonState = 0;
	mouse.oldMouseX = 0;
	mouse.oldMouseY = 0;
	
	joystick.oldButtonState = 0;
	joystick.oldPOVState = 0;
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

	in_activeApp = active;

	if (active)
		IN_ActivateMouse();
	else
		IN_DeactivateMouse();
}

/*
 =================
 IN_Restart_f
 =================
*/
void IN_Restart_f (void){

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

	// General variables
    in_mouse = Cvar_Get("in_mouse",	"1", CVAR_ARCHIVE | CVAR_LATCH);
	in_initMouse = Cvar_Get("in_initMouse", "1", CVAR_INIT);
	in_joystick	= Cvar_Get("in_joystick", "0", CVAR_ARCHIVE | CVAR_LATCH);
	in_initJoystick = Cvar_Get("in_initJoystick", "1", CVAR_INIT);

	// Mouse variables
	m_showMouse = Cvar_Get("m_showMouse", "0", CVAR_CHEAT);
	m_filter = Cvar_Get("m_filter",	"0", CVAR_ARCHIVE);
	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE);
	m_forward = Cvar_Get("m_forward", "1.0", CVAR_ARCHIVE);
	m_side = Cvar_Get("m_side", "1.0", CVAR_ARCHIVE);
	sensitivity = Cvar_Get("sensitivity", "5", CVAR_ARCHIVE);

	// Joystick variables
	joy_name = Cvar_Get("joy_name",	"joystick",	0);
	joy_advanced = Cvar_Get("joy_advanced",	"0", 0);
	joy_advAxisX = Cvar_Get("joy_advAxisX",	"0", 0);
	joy_advAxisY = Cvar_Get("joy_advAxisY",	"0", 0);
	joy_advAxisZ = Cvar_Get("joy_advAxisZ",	"0", 0);
	joy_advAxisR = Cvar_Get("joy_advAxisR",	"0", 0);
	joy_advAxisU = Cvar_Get("joy_advAxisU",	"0", 0);
	joy_advAxisV = Cvar_Get("joy_advAxisV",	"0", 0);
	joy_forwardThreshold = Cvar_Get("joy_forwardThreshold", "0.15", 0);
	joy_sideThreshold = Cvar_Get("joy_sideThreshold", "0.15", 0);
	joy_upThreshold = Cvar_Get("joy_upThreshold", "0.15", 0);
	joy_pitchThreshold = Cvar_Get("joy_pitchThreshold", "0.15", 0);
	joy_yawThreshold = Cvar_Get("joy_yawThreshold", "0.15", 0);
	joy_forwardSensitivity = Cvar_Get("joy_forwardSensitivity", "-1", 0);
	joy_sideSensitivity = Cvar_Get("joy_sideSensitivity", "-1", 0);
	joy_upSensitivity = Cvar_Get("joy_upSensitivity", "-1", 0);
	joy_pitchSensitivity = Cvar_Get("joy_pitchSensitivity", "1", 0);
	joy_yawSensitivity = Cvar_Get("joy_yawSensitivity", "-1", 0);

	Cmd_AddCommand("+mlook", IN_MLookDown_f);
	Cmd_AddCommand("-mlook", IN_MLookUp_f);
	Cmd_AddCommand("joy_advancedupdate", Joy_AdvancedUpdate_f);
	Cmd_AddCommand("in_restart", IN_Restart_f);

	IN_StartupMouse();
	IN_StartupJoystick();

	Com_Printf("------------------------------------\n");
}

/*
 =================
 IN_Shutdown
 =================
*/
void IN_Shutdown (void){

	Cmd_RemoveCommand("+mlook");
	Cmd_RemoveCommand("-mlook");
	Cmd_RemoveCommand("joy_advancedupdate");
	Cmd_RemoveCommand("in_restart");

	IN_DeactivateMouse();
}
