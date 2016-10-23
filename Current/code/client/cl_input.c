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


// cl.input.c  -- builds an intended movement command to send to the 
//				  server


#include "client.h"


static unsigned	cl_frameMsec;
static int		cl_oldFrameTime;

cvar_t	*cl_freeLook;
cvar_t	*cl_upSpeed;
cvar_t	*cl_forwardSpeed;
cvar_t	*cl_sideSpeed;
cvar_t	*cl_yawSpeed;
cvar_t	*cl_pitchSpeed;
cvar_t	*cl_angleSpeedKey;
cvar_t	*cl_run;
cvar_t	*cl_noDelta;
cvar_t	*m_showMouseRate;
cvar_t	*m_showMouseMove;
cvar_t	*m_filter;
cvar_t	*m_accel;
cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;
cvar_t	*sensitivity;


/*
 =======================================================================

 KEY BUTTONS

 Continuous button event tracking is complicated by the fact that two 
 different input sources (say, mouse button 1 and the control key) can 
 both press the same button, but the button should only be released when 
 both of the pressing keys have been released.

 When a key event issues a button command (+forward, +attack, etc...),
 it appends its key number as a parameter to the command so it can be
 matched up with the release.

 State bit 0 is the current state of the key
 State bit 1 is edge triggered on the up to down transition
 State bit 2 is edge triggered on the down to up transition

 CL_KeyEvent (int key, qboolean down, unsigned time);

 +mLook src time

 =======================================================================
*/

typedef struct {
	int				down[2];		// Key nums holding it down
	unsigned		downTime;		// Msec timestamp
	unsigned		msec;			// Msec down this frame
	int				state;
} kbutton_t;

static qboolean		in_mLooking;

static kbutton_t	in_up;
static kbutton_t	in_down;
static kbutton_t	in_left;
static kbutton_t	in_right;
static kbutton_t	in_forward;
static kbutton_t	in_back;
static kbutton_t	in_lookUp;
static kbutton_t	in_lookDown;
static kbutton_t	in_strafe;
static kbutton_t	in_moveLeft;
static kbutton_t	in_moveRight;
static kbutton_t	in_speed;
static kbutton_t	in_attack;
static kbutton_t	in_use;


/*
 =================
 CL_KeyDown
 =================
*/
static void CL_KeyDown (kbutton_t *b){

	int			k;
	char		*c;
	
	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// Typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// Repeating key

	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else {
		Com_Printf("Three keys down for a button!\n");
		return;
	}
	
	if (b->state & 1)
		return;		// Still down

	// Save timestamp
	b->downTime = atoi(Cmd_Argv(2));
	if (!b->downTime)
		b->downTime = com_frameTime - 100;

	b->state |= 1 + 2;	// Down + impulse down
}

/*
 =================
 CL_KeyUp
 =================
*/
static void CL_KeyUp (kbutton_t *b){

	int			k;
	char		*c;
	unsigned	upTime;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else {
		// Typed manually at the console, assume for unsticking, so 
		// clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// Impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// Key up without coresponding down (menu pass through)

	if (b->down[0] || b->down[1])
		return;		// Some other key is still holding it down

	if (!(b->state & 1))
		return;		// Still up (this should not happen)

	// Save timestamp
	upTime = atoi(Cmd_Argv(2));
	if (upTime)
		b->msec += upTime - b->downTime;
	else
		b->msec += 10;

	b->state &= ~1;		// Now up
	b->state |= 4; 		// Impulse up
}

/*
 =================
 CL_KeyState

 Returns the fraction of the frame that the key was down
 =================
*/
static float CL_KeyState (kbutton_t *b){

	float	val;
	int		msec;

	b->state &= 1;		// Clear impulses

	msec = b->msec;
	b->msec = 0;

	if (b->state){
		// Still down
		msec += com_frameTime - b->downTime;
		b->downTime = com_frameTime;
	}

	val = (float)msec / cl_frameMsec;
	if (val < 0)
		val = 0;
	else if (val > 1)
		val = 1;

	return val;
}


// =====================================================================


/*
 =================
 CL_CenterView_f
 =================
*/
static void CL_CenterView_f (void){

	cl.viewAngles[PITCH] = -SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[PITCH]);
}

/*
 =================
 CL_MLookDown_f
 =================
*/
static void CL_MLookDown_f (void){
	
	in_mLooking = true;
}

/*
 =================
 CL_MLookUp_f
 =================
*/
static void CL_MLookUp_f (void){

	in_mLooking = false;

	if (!cl_freeLook->integerValue)
		CL_CenterView_f();
}

/*
 =================
 CL_ZoomDown_f
 =================
*/
static void CL_ZoomDown_f (void){

	if (cl.zooming)
		return;

	cl.zooming = true;
	cl.zoomTime = cl.time;
}

/*
 =================
 CL_ZoomUp_f
 =================
*/
static void CL_ZoomUp_f (void){

	if (!cl.zooming)
		return;

	cl.zooming = false;
	cl.zoomTime = cl.time;
}

/*
 =================
 CL_UpDown_f
 =================
*/
static void CL_UpDown_f (void){
	
	CL_KeyDown(&in_up);
}

/*
 =================
 CL_UpUp_f
 =================
*/
static void CL_UpUp_f (void){
	
	CL_KeyUp(&in_up);
}

/*
 =================
 CL_DownDown_f
 =================
*/
static void CL_DownDown_f (void){
	
	CL_KeyDown(&in_down);
}

/*
 =================
 CL_DownUp_f
 =================
*/
static void CL_DownUp_f (void){
	
	CL_KeyUp(&in_down);
}

/*
 =================
 CL_LeftDown_f
 =================
*/
static void CL_LeftDown_f (void){
	
	CL_KeyDown(&in_left);
}

/*
 =================
 CL_LeftUp_f
 =================
*/
static void CL_LeftUp_f (void){
	
	CL_KeyUp(&in_left);
}

/*
 =================
 CL_RightDown_f
 =================
*/
static void CL_RightDown_f (void){
	
	CL_KeyDown(&in_right);
}

/*
 =================
 CL_RightUp_f
 =================
*/
static void CL_RightUp_f (void){
	
	CL_KeyUp(&in_right);
}

/*
 =================
 CL_ForwardDown_f
 =================
*/
static void CL_ForwardDown_f (void){
	
	CL_KeyDown(&in_forward);
}

/*
 =================
 CL_ForwardUp_f
 =================
*/
static void CL_ForwardUp_f (void){
	
	CL_KeyUp(&in_forward);
}

/*
 =================
 CL_BackDown_f
 =================
*/
static void CL_BackDown_f (void){
	
	CL_KeyDown(&in_back);
}

/*
 =================
 CL_BackUp_f
 =================
*/
static void CL_BackUp_f (void){
	
	CL_KeyUp(&in_back);
}

/*
 =================
 CL_LookUpDown_f
 =================
*/
static void CL_LookUpDown_f (void){
	
	CL_KeyDown(&in_lookUp);
}

/*
 =================
 CL_LookUpUp_f
 =================
*/
static void CL_LookUpUp_f (void){
	
	CL_KeyUp(&in_lookUp);
}

/*
 =================
 CL_LookDownDown_f
 =================
*/
static void CL_LookDownDown_f (void){
	
	CL_KeyDown(&in_lookDown);
}

/*
 =================
 CL_LookDownUp_f
 =================
*/
static void CL_LookDownUp_f (void){
	
	CL_KeyUp(&in_lookDown);
}

/*
 =================
 CL_StrafeDown_f
 =================
*/
static void CL_StrafeDown_f (void){
	
	CL_KeyDown(&in_strafe);
}

/*
 =================
 CL_StrafeUp_f
 =================
*/
static void CL_StrafeUp_f (void){
	
	CL_KeyUp(&in_strafe);
}

/*
 =================
 CL_MoveLeftDown_f
 =================
*/
static void CL_MoveLeftDown_f (void){
	
	CL_KeyDown(&in_moveLeft);
}

/*
 =================
 CL_MoveLeftUp_f
 =================
*/
static void CL_MoveLeftUp_f (void){
	
	CL_KeyUp(&in_moveLeft);
}

/*
 =================
 CL_MoveRightDown_f
 =================
*/
static void CL_MoveRightDown_f (void){
	
	CL_KeyDown(&in_moveRight);
}

/*
 =================
 CL_MoveRightUp_f
 =================
*/
static void CL_MoveRightUp_f (void){
	
	CL_KeyUp(&in_moveRight);
}

/*
 =================
 CL_SpeedDown_f
 =================
*/
static void CL_SpeedDown_f (void){
	
	CL_KeyDown(&in_speed);
}

/*
 =================
 CL_SpeedUp_f
 =================
*/
static void CL_SpeedUp_f (void){
	
	CL_KeyUp(&in_speed);
}

/*
 =================
 CL_AttackDown_f
 =================
*/
static void CL_AttackDown_f (void){
	
	CL_KeyDown(&in_attack);
}

/*
 =================
 CL_AttackUp_f
 =================
*/
static void CL_AttackUp_f (void){
	
	CL_KeyUp(&in_attack);
}

/*
 =================
 CL_UseDown_f
 =================
*/
static void CL_UseDown_f (void){
	
	CL_KeyDown(&in_use);
}

/*
 =================
 CL_UseUp_f
 =================
*/
static void CL_UseUp_f (void){
	
	CL_KeyUp(&in_use);
}


// =====================================================================


/*
 =================
 CL_AdjustAngles

 Moves the local angle positions
 =================
*/
static void CL_AdjustAngles (void){

	float	speed;
	
	if (in_speed.state & 1)
		speed = cls.frameTime * cl_angleSpeedKey->floatValue;
	else
		speed = cls.frameTime;

	if (!(in_strafe.state & 1)){
		cl.viewAngles[YAW] -= speed*cl_yawSpeed->floatValue * CL_KeyState(&in_right);
		cl.viewAngles[YAW] += speed*cl_yawSpeed->floatValue * CL_KeyState(&in_left);
	}

	cl.viewAngles[PITCH] -= speed*cl_pitchSpeed->floatValue * CL_KeyState(&in_lookUp);
	cl.viewAngles[PITCH] += speed*cl_pitchSpeed->floatValue * CL_KeyState(&in_lookDown);
}

/*
 =================
 CL_KeyMove

 Sets the usercmd_t based on key states
 =================
*/
static void CL_KeyMove (usercmd_t *cmd){

	CL_AdjustAngles();

	if (in_strafe.state & 1){
		cmd->sidemove += cl_sideSpeed->floatValue * CL_KeyState(&in_right);
		cmd->sidemove -= cl_sideSpeed->floatValue * CL_KeyState(&in_left);
	}

	cmd->sidemove += cl_sideSpeed->floatValue * CL_KeyState(&in_moveRight);
	cmd->sidemove -= cl_sideSpeed->floatValue * CL_KeyState(&in_moveLeft);

	cmd->upmove += cl_upSpeed->floatValue * CL_KeyState(&in_up);
	cmd->upmove -= cl_upSpeed->floatValue * CL_KeyState(&in_down);

	cmd->forwardmove += cl_forwardSpeed->floatValue * CL_KeyState(&in_forward);
	cmd->forwardmove -= cl_forwardSpeed->floatValue * CL_KeyState(&in_back);

	// Adjust for speed key / running
	if ((in_speed.state & 1) ^ cl_run->integerValue){
		cmd->forwardmove *= 2;
		cmd->sidemove *= 2;
		cmd->upmove *= 2;
	}	
}

/*
 =================
 CL_MouseMove

 Sets the usercmd_t based on mouse states
 =================
*/
static void CL_MouseMove (usercmd_t *cmd){

	float	x, y;
	float	rate, accelSensitivity;

	// If the menu is visible, don't move
	if (UI_IsVisible()){
		cl.mouseX = 0;
		cl.mouseY = 0;
		return;
	}

	// If the game is not active or paused, don't move
	if (cls.state != CA_ACTIVE || cls.cinematicPlaying || com_paused->integerValue){
		cl.mouseX = 0;
		cl.mouseY = 0;
		return;
	}

	// Allow mouse smoothing
	if (m_filter->integerValue){
		x = (cl.mouseX + cl.oldMouseX) * 0.5;
		y = (cl.mouseY + cl.oldMouseY) * 0.5;
	}
	else {
		x = cl.mouseX;
		y = cl.mouseY;
	}

	cl.oldMouseX = cl.mouseX;
	cl.oldMouseY = cl.mouseY;

	cl.mouseX = 0;
	cl.mouseY = 0;

	// Apply sensitivity and acceleration
	rate = sqrt(x*x + y*y) / (float)cl_frameMsec;
	accelSensitivity = sensitivity->floatValue + rate * m_accel->floatValue;

	if (cl.zooming)
		accelSensitivity *= cl.zoomSensitivity;

	if (m_showMouseRate->integerValue && rate)
		Com_Printf("%f : %f\n", rate, accelSensitivity);

	x *= accelSensitivity;
	y *= accelSensitivity;

	// Add mouse movement to command
	if (!x && !y)
		return;

	if (m_showMouseMove->integerValue)
		Com_Printf("%f, %f\n", x, y);

	if (in_strafe.state & 1)
		cmd->sidemove += m_side->floatValue * x;
	else
		cl.viewAngles[YAW] -= m_yaw->floatValue * x;

	if (!(in_strafe.state & 1) && (cl_freeLook->integerValue || in_mLooking))
		cl.viewAngles[PITCH] += m_pitch->floatValue * y;
	else
		cmd->forwardmove -= m_forward->floatValue * y;
}

/*
 =================
 CL_ClampPitch
 =================
*/
static void CL_ClampPitch (void){

	float	pitch;

	pitch = SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[PITCH]);
	if (pitch > 180)
		pitch -= 360;

	if (cl.viewAngles[PITCH] + pitch < -360)
		cl.viewAngles[PITCH] += 360;	// Wrapped
	if (cl.viewAngles[PITCH] + pitch > 360)
		cl.viewAngles[PITCH] -= 360;	// Wrapped

	if (cl.viewAngles[PITCH] + pitch > 89)
		cl.viewAngles[PITCH] = 89 - pitch;
	if (cl.viewAngles[PITCH] + pitch < -89)
		cl.viewAngles[PITCH] = -89 - pitch;
}

/*
 =================
 CL_FinishMove
 =================
*/
static void CL_FinishMove (usercmd_t *cmd){

	int		ms;

	// Figure button bits
	if (in_attack.state & 3)
		cmd->buttons |= BUTTON_ATTACK;
	in_attack.state &= ~2;
	
	if (in_use.state & 3)
		cmd->buttons |= BUTTON_USE;
	in_use.state &= ~2;

	if (CL_AnyKeyIsDown())
		cmd->buttons |= BUTTON_ANY;

	// Send milliseconds of time to apply to the move
	ms = SEC2MS(cls.frameTime);
	if (ms > 250)
		ms = 100;		// Time was unreasonable
	cmd->msec = ms;

	CL_ClampPitch();

	cmd->angles[0] = ANGLE2SHORT(cl.viewAngles[0]);
	cmd->angles[1] = ANGLE2SHORT(cl.viewAngles[1]);
	cmd->angles[2] = ANGLE2SHORT(cl.viewAngles[2]);

	cmd->impulse = 0;
	cmd->lightlevel = 150;
}

/*
 =================
 CL_CreateCmd
 =================
*/
static void CL_CreateCmd (void){

	usercmd_t	*cmd;
	int			index;

	cl_frameMsec = com_frameTime - cl_oldFrameTime;
	if (cl_frameMsec < 1)
		cl_frameMsec = 1;
	else if (cl_frameMsec > 200)
		cl_frameMsec = 200;

	cl_oldFrameTime = com_frameTime;

	index = cls.netChan.outgoingSequence & CMD_MASK;
	cmd = &cl.cmds[index];

	memset(cmd, 0, sizeof(usercmd_t));

	// Get basic movement from keyboard
	CL_KeyMove(cmd);

	// Get basic movement from mouse
	CL_MouseMove(cmd);

	// Finish movement
	CL_FinishMove(cmd);

	// Save the time for ping calculations
	cl.cmdTime[index] = cls.realTime;
}

/*
 =================
 CL_SendCmd

 Build a command even if not connected
 =================
*/
void CL_SendCmd (void){

	msg_t		msg;
	byte		data[128];
	usercmd_t	*cmd, *oldCmd, nullCmd;
	int			checksumIndex;

	CL_CreateCmd();

	if (cls.state < CA_CONNECTED)
		return;

	if (cls.state < CA_ACTIVE){
		if (cls.netChan.message.curSize	|| Sys_Milliseconds() - cls.netChan.lastSent > 1000)
			NetChan_Transmit(&cls.netChan, NULL, 0);

		return;
	}

	// Send a user info update if needed
	if (cvar_modifiedFlags & CVAR_USERINFO){
		cvar_modifiedFlags &= ~CVAR_USERINFO;

		MSG_WriteByte(&cls.netChan.message, CLC_USERINFO);
		MSG_WriteString(&cls.netChan.message, Cvar_UserInfo());
	}

	MSG_Init(&msg, data, sizeof(data), false);

	// Begin a client move command
	MSG_WriteByte(&msg, CLC_MOVE);

	// Save the position for a checksum byte
	checksumIndex = msg.curSize;
	MSG_WriteByte(&msg, 0);

	// Let the server know what the last frame we got was, so the next 
	// message can be delta compressed
	if (cl_noDelta->integerValue || !cl.frame.valid || cls.demoWaiting)
		MSG_WriteLong(&msg, -1);	// No compression
	else
		MSG_WriteLong(&msg, cl.frame.serverFrame);

	// Send this and the previous commands in the message, so if the
	// last packet was dropped, it can be recovered
	memset(&nullCmd, 0, sizeof(usercmd_t));

	cmd = &cl.cmds[(cls.netChan.outgoingSequence-2) & CMD_MASK];
	MSG_WriteDeltaUserCmd(&msg, &nullCmd, cmd);

	oldCmd = cmd;
	cmd = &cl.cmds[(cls.netChan.outgoingSequence-1) & CMD_MASK];
	MSG_WriteDeltaUserCmd(&msg, oldCmd, cmd);

	oldCmd = cmd;
	cmd = &cl.cmds[(cls.netChan.outgoingSequence) & CMD_MASK];
	MSG_WriteDeltaUserCmd(&msg, oldCmd, cmd);

	// Calculate a checksum over the move commands
	msg.data[checksumIndex] = Com_BlockSequenceCRCByte(msg.data + checksumIndex + 1, msg.curSize - checksumIndex - 1, cls.netChan.outgoingSequence);

	// Deliver the message
	NetChan_Transmit(&cls.netChan, msg.data, msg.curSize);
}

/*
 =================
 CL_MouseMoveEvent
 =================
*/
void CL_MouseMoveEvent (int x, int y){

	// If the menu is visible, move the menu cursor
	if (UI_IsVisible()){
		UI_MouseMove(x, y);
		return;
	}

	cl.mouseX = x;
	cl.mouseY = y;
}

/*
 =================
 CL_InitInput
 =================
*/
void CL_InitInput (void){

	// Register our variables and commands
	cl_freeLook = Cvar_Get("cl_freeLook", "1", CVAR_ARCHIVE, "Free look");
	cl_upSpeed = Cvar_Get("cl_upSpeed", "200", CVAR_ARCHIVE, "Up speed");
	cl_forwardSpeed = Cvar_Get("cl_forwardSpeed", "200", CVAR_ARCHIVE, "Forward speed");
	cl_sideSpeed = Cvar_Get("cl_sideSpeed", "200", CVAR_ARCHIVE, "Side speed");
	cl_yawSpeed = Cvar_Get("cl_yawSpeed", "140", CVAR_ARCHIVE, "Yaw speed");
	cl_pitchSpeed = Cvar_Get("cl_pitchSpeed", "150", CVAR_ARCHIVE, "Pitch speed");
	cl_angleSpeedKey = Cvar_Get("cl_angleSpeedKey", "1.5", CVAR_ARCHIVE, "Angle speed key");
	cl_run = Cvar_Get("cl_run", "0", CVAR_ARCHIVE, "Always run");
	cl_noDelta = Cvar_Get("cl_noDelta", "0", 0, "Don't use delta compression");
	m_showMouseRate = Cvar_Get("m_showMouseRate", "0", CVAR_CHEAT, "Report mouse rate");
	m_showMouseMove = Cvar_Get("m_showMouseMove", "0", CVAR_CHEAT, "Report mouse movement");
	m_filter = Cvar_Get("m_filter",	"0", CVAR_ARCHIVE, "Smooth mouse movement");
	m_accel = Cvar_Get("m_accel", "0.0", CVAR_ARCHIVE, "Mouse acceleration");
	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE, "Mouse pitch speed");
	m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE, "Mouse yaw speed");
	m_forward = Cvar_Get("m_forward", "1.0", CVAR_ARCHIVE, "Mouse forward speed");
	m_side = Cvar_Get("m_side", "1.0", CVAR_ARCHIVE, "Mouse side speed");
	sensitivity = Cvar_Get("sensitivity", "5", CVAR_ARCHIVE, "Mouse sensitivity scale factor");

	Cmd_AddCommand("centerView", CL_CenterView_f, "Center the view");
	Cmd_AddCommand("+mLook", CL_MLookDown_f, NULL);
	Cmd_AddCommand("-mLook", CL_MLookUp_f, NULL);
	Cmd_AddCommand("+zoom", CL_ZoomDown_f, NULL);
	Cmd_AddCommand("-zoom", CL_ZoomUp_f, NULL);
	Cmd_AddCommand("+moveUp", CL_UpDown_f, NULL);
	Cmd_AddCommand("-moveUp", CL_UpUp_f, NULL);
	Cmd_AddCommand("+moveDown", CL_DownDown_f, NULL);
	Cmd_AddCommand("-moveDown", CL_DownUp_f, NULL);
	Cmd_AddCommand("+left", CL_LeftDown_f, NULL);
	Cmd_AddCommand("-left", CL_LeftUp_f, NULL);
	Cmd_AddCommand("+right", CL_RightDown_f, NULL);
	Cmd_AddCommand("-right", CL_RightUp_f, NULL);
	Cmd_AddCommand("+forward", CL_ForwardDown_f, NULL);
	Cmd_AddCommand("-forward", CL_ForwardUp_f, NULL);
	Cmd_AddCommand("+back", CL_BackDown_f, NULL);
	Cmd_AddCommand("-back", CL_BackUp_f, NULL);
	Cmd_AddCommand("+lookUp", CL_LookUpDown_f, NULL);
	Cmd_AddCommand("-lookUp", CL_LookUpUp_f, NULL);
	Cmd_AddCommand("+lookDown", CL_LookDownDown_f, NULL);
	Cmd_AddCommand("-lookDown", CL_LookDownUp_f, NULL);
	Cmd_AddCommand("+strafe", CL_StrafeDown_f, NULL);
	Cmd_AddCommand("-strafe", CL_StrafeUp_f, NULL);
	Cmd_AddCommand("+moveLeft", CL_MoveLeftDown_f, NULL);
	Cmd_AddCommand("-moveLeft", CL_MoveLeftUp_f, NULL);
	Cmd_AddCommand("+moveRight", CL_MoveRightDown_f, NULL);
	Cmd_AddCommand("-moveRight", CL_MoveRightUp_f, NULL);
	Cmd_AddCommand("+speed", CL_SpeedDown_f, NULL);
	Cmd_AddCommand("-speed", CL_SpeedUp_f, NULL);
	Cmd_AddCommand("+attack", CL_AttackDown_f, NULL);
	Cmd_AddCommand("-attack", CL_AttackUp_f, NULL);
	Cmd_AddCommand("+use", CL_UseDown_f, NULL);
	Cmd_AddCommand("-use", CL_UseUp_f, NULL);
}

/*
 =================
 CL_ShutdownInput
 =================
*/
void CL_ShutdownInput (void){

	Cmd_RemoveCommand("centerView");
	Cmd_RemoveCommand("+mLook");
	Cmd_RemoveCommand("-mLook");
	Cmd_RemoveCommand("+zoom");
	Cmd_RemoveCommand("-zoom");
	Cmd_RemoveCommand("+moveUp");
	Cmd_RemoveCommand("-moveUp");
	Cmd_RemoveCommand("+moveDown");
	Cmd_RemoveCommand("-moveDown");
	Cmd_RemoveCommand("+left");
	Cmd_RemoveCommand("-left");
	Cmd_RemoveCommand("+right");
	Cmd_RemoveCommand("-right");
	Cmd_RemoveCommand("+forward");
	Cmd_RemoveCommand("-forward");
	Cmd_RemoveCommand("+back");
	Cmd_RemoveCommand("-back");
	Cmd_RemoveCommand("+lookUp");
	Cmd_RemoveCommand("-lookUp");
	Cmd_RemoveCommand("+lookDown");
	Cmd_RemoveCommand("-lookDown");
	Cmd_RemoveCommand("+strafe");
	Cmd_RemoveCommand("-strafe");
	Cmd_RemoveCommand("+moveLeft");
	Cmd_RemoveCommand("-moveLeft");
	Cmd_RemoveCommand("+moveRight");
	Cmd_RemoveCommand("-moveRight");
	Cmd_RemoveCommand("+speed");
	Cmd_RemoveCommand("-speed");
	Cmd_RemoveCommand("+attack");
	Cmd_RemoveCommand("-attack");
	Cmd_RemoveCommand("+use");
	Cmd_RemoveCommand("-use");
}
