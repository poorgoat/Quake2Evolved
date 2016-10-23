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
static unsigned	cl_oldFrameTime;

extern unsigned	sys_frameTime;


/*
 =======================================================================

 KEY BUTTONS

 Continuous button event tracking is complicated by the fact that two 
 different input sources (say, mouse button 1 and the control key) can 
 both press the same button, but the button should only be released when 
 both of the pressing key have been released.

 When a key event issues a button command (+forward, +attack, etc...),
 it appends its key number as a parameter to the command so it can be
 matched up with the release.

 State bit 0 is the current state of the key
 State bit 1 is edge triggered on the up to down transition
 State bit 2 is edge triggered on the down to up transition

 Key_Event (int key, qboolean down, unsigned time);

 +mlook src time

 =======================================================================
*/

kbutton_t		in_up;
kbutton_t		in_down;
kbutton_t		in_left;
kbutton_t		in_right;
kbutton_t		in_forward;
kbutton_t		in_back;
kbutton_t		in_lookUp;
kbutton_t		in_lookDown;
kbutton_t		in_strafe;
kbutton_t		in_moveLeft;
kbutton_t		in_moveRight;
kbutton_t		in_speed;
kbutton_t		in_attack;
kbutton_t		in_use;
kbutton_t		in_kLook;
int				in_impulse;


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
		b->downTime = sys_frameTime - 100;

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
		msec += sys_frameTime - b->downTime;
		b->downTime = sys_frameTime;
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
void CL_CenterView_f (void){

	cl.viewAngles[PITCH] = -SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[PITCH]);
}

/*
 =================
 CL_ZoomDown_f
 =================
*/
void CL_ZoomDown_f (void){

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
void CL_ZoomUp_f (void){

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
void CL_UpDown_f (void){
	
	CL_KeyDown(&in_up);
}

/*
 =================
 CL_UpUp_f
 =================
*/
void CL_UpUp_f (void){
	
	CL_KeyUp(&in_up);
}

/*
 =================
 CL_DownDown_f
 =================
*/
void CL_DownDown_f (void){
	
	CL_KeyDown(&in_down);
}

/*
 =================
 CL_DownUp_f
 =================
*/
void CL_DownUp_f (void){
	
	CL_KeyUp(&in_down);
}

/*
 =================
 CL_LeftDown_f
 =================
*/
void CL_LeftDown_f (void){
	
	CL_KeyDown(&in_left);
}

/*
 =================
 CL_LeftUp_f
 =================
*/
void CL_LeftUp_f (void){
	
	CL_KeyUp(&in_left);
}

/*
 =================
 CL_RightDown_f
 =================
*/
void CL_RightDown_f (void){
	
	CL_KeyDown(&in_right);
}

/*
 =================
 CL_RightUp_f
 =================
*/
void CL_RightUp_f (void){
	
	CL_KeyUp(&in_right);
}

/*
 =================
 CL_ForwardDown_f
 =================
*/
void CL_ForwardDown_f (void){
	
	CL_KeyDown(&in_forward);
}

/*
 =================
 CL_ForwardUp_f
 =================
*/
void CL_ForwardUp_f (void){
	
	CL_KeyUp(&in_forward);
}

/*
 =================
 CL_BackDown_f
 =================
*/
void CL_BackDown_f (void){
	
	CL_KeyDown(&in_back);
}

/*
 =================
 CL_BackUp_f
 =================
*/
void CL_BackUp_f (void){
	
	CL_KeyUp(&in_back);
}

/*
 =================
 CL_LookUpDown_f
 =================
*/
void CL_LookUpDown_f (void){
	
	CL_KeyDown(&in_lookUp);
}

/*
 =================
 CL_LookUpUp_f
 =================
*/
void CL_LookUpUp_f (void){
	
	CL_KeyUp(&in_lookUp);
}

/*
 =================
 CL_LookDownDown_f
 =================
*/
void CL_LookDownDown_f (void){
	
	CL_KeyDown(&in_lookDown);
}

/*
 =================
 CL_LookDownUp_f
 =================
*/
void CL_LookDownUp_f (void){
	
	CL_KeyUp(&in_lookDown);
}

/*
 =================
 CL_StrafeDown_f
 =================
*/
void CL_StrafeDown_f (void){
	
	CL_KeyDown(&in_strafe);
}

/*
 =================
 CL_StrafeUp_f
 =================
*/
void CL_StrafeUp_f (void){
	
	CL_KeyUp(&in_strafe);
}

/*
 =================
 CL_MoveLeftDown_f
 =================
*/
void CL_MoveLeftDown_f (void){
	
	CL_KeyDown(&in_moveLeft);
}

/*
 =================
 CL_MoveLeftUp_f
 =================
*/
void CL_MoveLeftUp_f (void){
	
	CL_KeyUp(&in_moveLeft);
}

/*
 =================
 CL_MoveRightDown_f
 =================
*/
void CL_MoveRightDown_f (void){
	
	CL_KeyDown(&in_moveRight);
}

/*
 =================
 CL_MoveRightUp_f
 =================
*/
void CL_MoveRightUp_f (void){
	
	CL_KeyUp(&in_moveRight);
}

/*
 =================
 CL_SpeedDown_f
 =================
*/
void CL_SpeedDown_f (void){
	
	CL_KeyDown(&in_speed);
}

/*
 =================
 CL_SpeedUp_f
 =================
*/
void CL_SpeedUp_f (void){
	
	CL_KeyUp(&in_speed);
}

/*
 =================
 CL_AttackDown_f
 =================
*/
void CL_AttackDown_f (void){
	
	CL_KeyDown(&in_attack);
}

/*
 =================
 CL_AttackUp_f
 =================
*/
void CL_AttackUp_f (void){
	
	CL_KeyUp(&in_attack);
}

/*
 =================
 CL_UseDown_f
 =================
*/
void CL_UseDown_f (void){
	
	CL_KeyDown(&in_use);
}

/*
 =================
 CL_UseUp_f
 =================
*/
void CL_UseUp_f (void){
	
	CL_KeyUp(&in_use);
}

/*
 =================
 CL_KLookDown_f
 =================
*/
void CL_KLookDown_f (void){
	
	CL_KeyDown(&in_kLook);
}

/*
 =================
 CL_KLookUp_f
 =================
*/
void CL_KLookUp_f (void){
	
	CL_KeyUp(&in_kLook);
}

/*
 =================
 CL_Impulse_f
 =================
*/
void CL_Impulse_f (void){

	in_impulse = atoi(Cmd_Argv(1));
}


// =====================================================================


/*
 =================
 CL_AdjustAngles

 Moves the local angles positions
 =================
*/
static void CL_AdjustAngles (void){

	float	speed;
	
	if (in_speed.state & 1)
		speed = cls.frameTime * cl_angleSpeedKey->value;
	else
		speed = cls.frameTime;

	if (!(in_strafe.state & 1)){
		cl.viewAngles[YAW] -= speed*cl_yawSpeed->value * CL_KeyState(&in_right);
		cl.viewAngles[YAW] += speed*cl_yawSpeed->value * CL_KeyState(&in_left);
	}

	if (in_kLook.state & 1){
		cl.viewAngles[PITCH] -= speed*cl_pitchSpeed->value * CL_KeyState(&in_forward);
		cl.viewAngles[PITCH] += speed*cl_pitchSpeed->value * CL_KeyState(&in_back);
	}
	
	cl.viewAngles[PITCH] -= speed*cl_pitchSpeed->value * CL_KeyState(&in_lookUp);
	cl.viewAngles[PITCH] += speed*cl_pitchSpeed->value * CL_KeyState(&in_lookDown);
}

/*
 =================
 CL_BaseMove

 Send the intended movement message to the server
 =================
*/
static void CL_BaseMove (usercmd_t *cmd){

	CL_AdjustAngles();

	if (in_strafe.state & 1){
		cmd->sidemove += cl_sideSpeed->value * CL_KeyState(&in_right);
		cmd->sidemove -= cl_sideSpeed->value * CL_KeyState(&in_left);
	}

	cmd->sidemove += cl_sideSpeed->value * CL_KeyState(&in_moveRight);
	cmd->sidemove -= cl_sideSpeed->value * CL_KeyState(&in_moveLeft);

	cmd->upmove += cl_upSpeed->value * CL_KeyState(&in_up);
	cmd->upmove -= cl_upSpeed->value * CL_KeyState(&in_down);

	if (!(in_kLook.state & 1)){
		cmd->forwardmove += cl_forwardSpeed->value * CL_KeyState(&in_forward);
		cmd->forwardmove -= cl_forwardSpeed->value * CL_KeyState(&in_back);
	}	

	// Adjust for speed key / running
	if ((in_speed.state & 1) ^ cl_run->integer){
		cmd->forwardmove *= 2;
		cmd->sidemove *= 2;
		cmd->upmove *= 2;
	}	
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
 CL_GetLightLevel
 =================
*/
static byte CL_GetLightLevel (void){

	vec3_t	ambientLight;
	float	max;

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.cinematicHandle)
		return 0;
	
	// HACK: get light value for server to look at
	R_LightForPoint(cl.refDef.viewOrigin, ambientLight);

	// Pick the greatest component, which should be the same as the mono
	// value returned by software
	max = ambientLight[0];
	if (max < ambientLight[1])
		max = ambientLight[1];
	if (max < ambientLight[2])
		max = ambientLight[2];

	return ((byte)150 * max);
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

	if (Key_IsAnyKeyDown())
		cmd->buttons |= BUTTON_ANY;

	// Send milliseconds of time to apply to the move
	ms = cls.frameTime * 1000;
	if (ms > 250)
		ms = 100;		// Time was unreasonable
	cmd->msec = ms;

	CL_ClampPitch();

	cmd->angles[0] = ANGLE2SHORT(cl.viewAngles[0]);
	cmd->angles[1] = ANGLE2SHORT(cl.viewAngles[1]);
	cmd->angles[2] = ANGLE2SHORT(cl.viewAngles[2]);

	cmd->impulse = in_impulse;
	in_impulse = 0;

	// Send the ambient light level at the player's current position
	cmd->lightlevel = CL_GetLightLevel();
}

/*
 =================
 CL_CreateCmd
 =================
*/
static void CL_CreateCmd (void){

	usercmd_t	*cmd;
	int			index;

	index = cls.netChan.outgoingSequence & CMD_MASK;
	cmd = &cl.cmds[index];

	cl_frameMsec = sys_frameTime - cl_oldFrameTime;
	if (cl_frameMsec < 1)
		cl_frameMsec = 1;
	else if (cl_frameMsec > 200)
		cl_frameMsec = 200;

	cl_oldFrameTime = sys_frameTime;

	memset(cmd, 0, sizeof(usercmd_t));

	// Get basic movement from keyboard
	CL_BaseMove(cmd);

	// Allow mouse and joystick to add to the move
	IN_Move(cmd);

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
	if (cvar_userInfoModified){
		cvar_userInfoModified = false;

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
	if (cl_noDelta->integer || !cl.frame.valid || cls.demoWaiting)
		MSG_WriteLong(&msg, -1);	// No compression
	else
		MSG_WriteLong(&msg, cl.frame.serverFrame);

	// Send this and the previous cmds in the message, so if the last 
	// packet was dropped, it can be recovered
	memset(&nullCmd, 0, sizeof(nullCmd));

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
