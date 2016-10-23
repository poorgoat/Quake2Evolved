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


// cl_view.c -- player view rendering


#include "client.h"


/*
 =======================================================================

 MODEL TESTING

 =======================================================================
*/


/*
 =================
 CL_TestModel_f
 =================
*/
void CL_TestModel_f (void){

	struct model_s	*model;

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.cinematicHandle || cl.demoPlayback)
		return;

	cl.testGun = false;
	cl.testModel = false;
	memset(&cl.testModelName, 0, sizeof(cl.testModelName));
	memset(&cl.testModelEntity, 0, sizeof(cl.testModelEntity));

	if (Cmd_Argc() != 2)
		return;

	model = R_RegisterModel(Cmd_Argv(1));
	if (!model)
		return;

	Q_strncpyz(cl.testModelName, Cmd_Argv(1), sizeof(cl.testModelName));

	cl.testModelEntity.entityType = ET_MODEL;
	cl.testModelEntity.model = model;
	MakeRGBA(cl.testModelEntity.shaderRGBA, 255, 255, 255, 255);

	VectorMA(cl.refDef.viewOrigin, 100, cl.refDef.viewAxis[0], cl.testModelEntity.origin);
	AxisClear(cl.testModelEntity.axis);

	cl.testModel = true;
}

/*
 =================
 CL_TestGun_f
 =================
*/
void CL_TestGun_f (void){

	CL_TestModel_f();

	if (cl.testModel){
		cl.testGun = true;
		cl.testModelEntity.renderFX = RF_MINLIGHT | RF_DEPTHHACK | RF_WEAPONMODEL;
	}
}

/*
 =================
 CL_NextFrame_f
 =================
*/
void CL_NextFrame_f (void){

	if (!cl.testModel)
		return;

	cl.testModelEntity.frame++;
	Com_Printf("Frame %i\n", cl.testModelEntity.frame);
}

/*
 =================
 CL_PrevFrame_f
 =================
*/
void CL_PrevFrame_f (void){

	if (!cl.testModel)
		return;

	cl.testModelEntity.frame--;
	if (cl.testModelEntity.frame < 0)
		cl.testModelEntity.frame = 0;
	Com_Printf("Frame %i\n", cl.testModelEntity.frame);
}

/*
 =================
 CL_NextSkin_f
 =================
*/
void CL_NextSkin_f (void){

	if (!cl.testModel)
		return;

	cl.testModelEntity.skinNum++;
	Com_Printf("Skin %i\n", cl.testModelEntity.skinNum);
}

/*
 =================
 CL_PrevSkin_f
 =================
*/
void CL_PrevSkin_f (void){

	if (!cl.testModel)
		return;

	cl.testModelEntity.skinNum--;
	if (cl.testModelEntity.skinNum < 0)
		cl.testModelEntity.skinNum = 0;
	Com_Printf("Skin %i\n", cl.testModelEntity.skinNum);
}

/*
 =================
 CL_AddTestModel
 =================
*/
static void CL_AddTestModel (void){

	cl.testModelEntity.model = R_RegisterModel(cl.testModelName);
	if (!cl.testModelEntity.model){
		cl.testGun = false;
		cl.testModel = false;
		memset(&cl.testModelName, 0, sizeof(cl.testModelName));
		memset(&cl.testModelEntity, 0, sizeof(cl.testModelEntity));
		return;
	}

	if (cl.testGun){
		VectorCopy(cl.refDef.viewOrigin, cl.testModelEntity.origin);
		AxisCopy(cl.refDef.viewAxis, cl.testModelEntity.axis);

		VectorMA(cl.testModelEntity.origin, cl_testGunX->value, cl.testModelEntity.axis[0], cl.testModelEntity.origin);
		VectorMA(cl.testModelEntity.origin, cl_testGunY->value, cl.testModelEntity.axis[1], cl.testModelEntity.origin);
		VectorMA(cl.testModelEntity.origin, cl_testGunZ->value, cl.testModelEntity.axis[2], cl.testModelEntity.origin);

		cl.testModelEntity.ammoValue = cl.playerState->stats[STAT_AMMO];
	}

	R_AddEntityToScene(&cl.testModelEntity);
}


// =====================================================================


/*
 =================
 CL_Viewpos_f
 =================
*/
void CL_Viewpos_f (void){

	Com_Printf("(%i %i %i) : %i\n", (int)cl.refDef.viewOrigin[0], (int)cl.refDef.viewOrigin[1], (int)cl.refDef.viewOrigin[2], (int)cl.refDefViewAngles[YAW]);
}

/*
 =================
 CL_TimeRefresh_f
 =================
*/
void CL_TimeRefresh_f (void){

	int		i;
	int		start, stop;
	float	time;

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.cinematicHandle || cl.demoPlayback)
		return;

	start = Sys_Milliseconds();

	if (Cmd_Argc() > 1){
		R_BeginFrame(cls.realTime, 0);
		for (i = 0; i < 128; i++){
			cl.refDefViewAngles[1] = i/128.0 * 360.0;
			AnglesToAxis(cl.refDefViewAngles, cl.refDef.viewAxis);

			R_RenderScene(&cl.refDef);
		}
		R_EndFrame();
	}
	else {
		for (i = 0; i < 128; i++){
			cl.refDefViewAngles[1] = i/128.0 * 360.0;
			AnglesToAxis(cl.refDefViewAngles, cl.refDef.viewAxis);

			R_BeginFrame(cls.realTime, 0);
			R_RenderScene(&cl.refDef);
			R_EndFrame();
		}
	}

	stop = Sys_Milliseconds();

	time = (stop - start) * 0.001;
	Com_Printf("%f seconds (%f FPS)\n", time, 128.0/time);
}

/*
 =================
 CL_SizeUp_f
 =================
*/
void CL_SizeUp_f (void){

	Cvar_SetInteger("cl_viewSize", cl_viewSize->integer + 10);
	if (cl_viewSize->integer > 100)
		Cvar_SetInteger("cl_viewSize", 100);
}

/*
 =================
 CL_SizeDown_f
 =================
*/
void CL_SizeDown_f (void){

	Cvar_SetInteger("cl_viewSize", cl_viewSize->integer - 10);
	if (cl_viewSize->integer < 30)
		Cvar_SetInteger("cl_viewSize", 30);
}

/*
 =================
 CL_CalcVRect

 Sets the coordinates of the rendered window
 =================
*/
static void CL_CalcVRect (void){

	// Bound view size
	if (cl_viewSize->integer < 30)
		Cvar_SetInteger("cl_viewSize", 30);
	else if (cl_viewSize->integer > 100)
		Cvar_SetInteger("cl_viewSize", 100);

	cl.refDef.width = cls.glConfig.videoWidth * cl_viewSize->integer / 100;
	cl.refDef.width &= ~7;

	cl.refDef.height = cls.glConfig.videoHeight * cl_viewSize->integer / 100;
	cl.refDef.height &= ~1;

	cl.refDef.x = (cls.glConfig.videoWidth - cl.refDef.width) / 2;
	cl.refDef.y = (cls.glConfig.videoHeight - cl.refDef.height) / 2;
}

/*
 =================
 CL_CalcFov
 =================
*/
static void CL_CalcFov (void){

	float	f;

	// Interpolate field of view
    cl.refDef.fovX = cl.oldPlayerState->fov + (cl.playerState->fov - cl.oldPlayerState->fov) * cl.lerpFrac;
	
	if (cl.refDef.fovX < 1)
		cl.refDef.fovX = 1;
	else if (cl.refDef.fovX > 179)
		cl.refDef.fovX = 179;

	// Interpolate and account for zoom
	if (cl_zoomFov->value < 1)
		Cvar_SetInteger("cl_zoomFov", 1);
	else if (cl_zoomFov->value > 179)
		Cvar_SetInteger("cl_zoomFov", 179);

	if (cl.zooming){
		f = (cl.time - cl.zoomTime) / 250.0;
		if (f > 1.0)
			cl.refDef.fovX = cl_zoomFov->value;
		else
			cl.refDef.fovX = cl.refDef.fovX + f * (cl_zoomFov->value - cl.refDef.fovX);
	}
	else {
		f = (cl.time - cl.zoomTime) / 250.0;
		if (f > 1.0)
			cl.refDef.fovX = cl.refDef.fovX;
		else
			cl.refDef.fovX = cl_zoomFov->value + f * (cl.refDef.fovX - cl_zoomFov->value);
	}

	f = cl.refDef.width / tan(cl.refDef.fovX / 360.0 * M_PI);
	cl.refDef.fovY = atan(cl.refDef.height / f) * 360.0 / M_PI;

	if (cl.zooming)
		cl.zoomSensitivity = cl.refDef.fovY / 75.0;

	// Warp if underwater
	if (CL_PointContents(cl.refDef.viewOrigin, -1) & MASK_WATER){
		f = sin(cl.time * 0.001 * 0.4 * M_PI2);

		cl.refDef.fovX += f;
		cl.refDef.fovY -= f;
	}
}

/*
 =================
 CL_CalcFirstPersonView
 =================
*/
static void CL_CalcFirstPersonView (void){

    float		backLerp;
	unsigned	delta;

    // Calculate the origin
	if (cl_predict->integer && !(cl.playerState->pmove.pm_flags & PMF_NO_PREDICTION)){
		// Use predicted values
        backLerp = 1.0 - cl.lerpFrac;

		cl.refDef.viewOrigin[0] = cl.predictedOrigin[0] + cl.oldPlayerState->viewoffset[0] + cl.lerpFrac * (cl.playerState->viewoffset[0] - cl.oldPlayerState->viewoffset[0]) - backLerp * cl.predictedError[0];
		cl.refDef.viewOrigin[1] = cl.predictedOrigin[1] + cl.oldPlayerState->viewoffset[1] + cl.lerpFrac * (cl.playerState->viewoffset[1] - cl.oldPlayerState->viewoffset[1]) - backLerp * cl.predictedError[1];
		cl.refDef.viewOrigin[2] = cl.predictedOrigin[2] + cl.oldPlayerState->viewoffset[2] + cl.lerpFrac * (cl.playerState->viewoffset[2] - cl.oldPlayerState->viewoffset[2]) - backLerp * cl.predictedError[2];

        // Smooth out stair climbing
        delta = cls.realTime - cl.predictedStepTime;
        if (delta < 100)
            cl.refDef.viewOrigin[2] -= cl.predictedStep * (100 - delta) * 0.01;
    }
    else {
		// Just use interpolated values
		cl.refDef.viewOrigin[0] = cl.oldPlayerState->pmove.origin[0]*0.125 + cl.oldPlayerState->viewoffset[0] + cl.lerpFrac * (cl.playerState->pmove.origin[0]*0.125 + cl.playerState->viewoffset[0] - (cl.oldPlayerState->pmove.origin[0]*0.125 + cl.oldPlayerState->viewoffset[0]));
		cl.refDef.viewOrigin[1] = cl.oldPlayerState->pmove.origin[1]*0.125 + cl.oldPlayerState->viewoffset[1] + cl.lerpFrac * (cl.playerState->pmove.origin[1]*0.125 + cl.playerState->viewoffset[1] - (cl.oldPlayerState->pmove.origin[1]*0.125 + cl.oldPlayerState->viewoffset[1]));
		cl.refDef.viewOrigin[2] = cl.oldPlayerState->pmove.origin[2]*0.125 + cl.oldPlayerState->viewoffset[2] + cl.lerpFrac * (cl.playerState->pmove.origin[2]*0.125 + cl.playerState->viewoffset[2] - (cl.oldPlayerState->pmove.origin[2]*0.125 + cl.oldPlayerState->viewoffset[2]));
    }

    // Calculate the angles
    if (cl.playerState->pmove.pm_type < PM_DEAD)
		// Use predicted values
		VectorCopy(cl.predictedAngles, cl.refDefViewAngles);
    else {
		// Just use interpolated values
		cl.refDefViewAngles[0] = LerpAngle(cl.oldPlayerState->viewangles[0], cl.playerState->viewangles[0], cl.lerpFrac);
		cl.refDefViewAngles[1] = LerpAngle(cl.oldPlayerState->viewangles[1], cl.playerState->viewangles[1], cl.lerpFrac);
		cl.refDefViewAngles[2] = LerpAngle(cl.oldPlayerState->viewangles[2], cl.playerState->viewangles[2], cl.lerpFrac);
    }

	// Account for kick angles
	cl.refDefViewAngles[0] += LerpAngle(cl.oldPlayerState->kick_angles[0], cl.playerState->kick_angles[0], cl.lerpFrac);
	cl.refDefViewAngles[1] += LerpAngle(cl.oldPlayerState->kick_angles[1], cl.playerState->kick_angles[1], cl.lerpFrac);
	cl.refDefViewAngles[2] += LerpAngle(cl.oldPlayerState->kick_angles[2], cl.playerState->kick_angles[2], cl.lerpFrac);
}

/*
 =================
 CL_CalcThirdPersonView
 =================
*/
static void CL_CalcThirdPersonView (void){

	vec3_t	forward, right, spot;
	vec3_t	origin, angles;
	vec3_t	mins = {-4, -4, -4}, maxs = {4, 4, 4};
	float	dist, rad;
	trace_t	trace;

	// Calculate the origin
	cl.refDef.viewOrigin[0] = cl.oldPlayerState->pmove.origin[0]*0.125 + cl.oldPlayerState->viewoffset[0] + cl.lerpFrac * (cl.playerState->pmove.origin[0]*0.125 + cl.playerState->viewoffset[0] - (cl.oldPlayerState->pmove.origin[0]*0.125 + cl.oldPlayerState->viewoffset[0]));
	cl.refDef.viewOrigin[1] = cl.oldPlayerState->pmove.origin[1]*0.125 + cl.oldPlayerState->viewoffset[1] + cl.lerpFrac * (cl.playerState->pmove.origin[1]*0.125 + cl.playerState->viewoffset[1] - (cl.oldPlayerState->pmove.origin[1]*0.125 + cl.oldPlayerState->viewoffset[1]));
	cl.refDef.viewOrigin[2] = cl.oldPlayerState->pmove.origin[2]*0.125 + cl.oldPlayerState->viewoffset[2] + cl.lerpFrac * (cl.playerState->pmove.origin[2]*0.125 + cl.playerState->viewoffset[2] - (cl.oldPlayerState->pmove.origin[2]*0.125 + cl.oldPlayerState->viewoffset[2]));

	// Calculate the angles
	cl.refDefViewAngles[0] = LerpAngle(cl.oldPlayerState->viewangles[0], cl.playerState->viewangles[0], cl.lerpFrac);
	cl.refDefViewAngles[1] = LerpAngle(cl.oldPlayerState->viewangles[1], cl.playerState->viewangles[1], cl.lerpFrac);
	cl.refDefViewAngles[2] = LerpAngle(cl.oldPlayerState->viewangles[2], cl.playerState->viewangles[2], cl.lerpFrac);

	VectorCopy(cl.refDefViewAngles, angles);
	if (angles[PITCH] > 45)
		angles[PITCH] = 45;

	AngleVectors(angles, forward, NULL, NULL);
	VectorMA(cl.refDef.viewOrigin, 512, forward, spot);

	// Calculate exact origin
	VectorCopy(cl.refDef.viewOrigin, origin);
	origin[2] += 8;

	cl.refDefViewAngles[PITCH] *= 0.5;
	AngleVectors(cl.refDefViewAngles, forward, right, NULL);

	rad = DEG2RAD(cl_thirdPersonAngle->value);
	VectorMA(origin, -cl_thirdPersonRange->value * cos(rad), forward, origin);
	VectorMA(origin, -cl_thirdPersonRange->value * sin(rad), right, origin);

	// Trace a line to make sure the view isn't inside solid geometry
	trace = CL_Trace(cl.refDef.viewOrigin, mins, maxs, origin, cl.clientNum, MASK_PLAYERSOLID, false, NULL);
	if (trace.fraction != 1.0){
		VectorCopy(trace.endpos, origin);
		origin[2] += (1.0 - trace.fraction) * 32;

		trace = CL_Trace(cl.refDef.viewOrigin, mins, maxs, origin, cl.clientNum, MASK_PLAYERSOLID, false, NULL);
		VectorCopy(trace.endpos, origin);
	}

	VectorCopy(origin, cl.refDef.viewOrigin);

	// Calculate pitch to look at spot from camera
	VectorSubtract(spot, cl.refDef.viewOrigin, spot);
	dist = sqrt(spot[0] * spot[0] + spot[1] * spot[1]);
	if (dist < 1)
		dist = 1;

	cl.refDefViewAngles[PITCH] = -RAD2DEG(atan2(spot[2], dist));
	cl.refDefViewAngles[YAW] -= cl_thirdPersonAngle->value;
}

/*
 =================
 CL_CalcViewValues

 Sets cl.refDef view values
 =================
*/
static void CL_CalcViewValues (void){

	frame_t	*oldFrame;
	short	delta[3];

	// Clamp time
	if (cl.time > cl.frame.serverTime){
		cl.time = cl.frame.serverTime;
		cl.lerpFrac = 1.0;
	}
	else if (cl.time < cl.frame.serverTime - 100){
		cl.time = cl.frame.serverTime - 100;
		cl.lerpFrac = 0.0;
	}
	else
		cl.lerpFrac = 1.0 - (cl.frame.serverTime - cl.time) * 0.01;

	if (timedemo->integer)
        cl.lerpFrac = 1.0;
    
    // Find the previous frame to interpolate from
    oldFrame = &cl.frames[(cl.frame.serverFrame - 1) & UPDATE_MASK];
    if ((oldFrame->serverFrame != cl.frame.serverFrame -1) || !oldFrame->valid)
        oldFrame = &cl.frame;		// Previous frame was dropped or invalid
    
	// Find the previous player state to interpolate from
	cl.playerState = &cl.frame.playerState;
	cl.oldPlayerState = &oldFrame->playerState;

	// See if the player respawned this frame
	if (cl.playerState->stats[STAT_HEALTH] > 0 && cl.oldPlayerState->stats[STAT_HEALTH] <= 0){
		// Clear a few things
		cl.damageTime = 0;
		cl.damageAngle = 0;
		cl.underwaterTime = 0;
		cl.underwaterMask = 0;
		cl.underwaterContents = 0;

		cl.crosshairEntNumber = 0;
		cl.crosshairEntTime = 0;
	}

    // See if the player entity was teleported this frame
	delta[0] = cl.oldPlayerState->pmove.origin[0] - cl.playerState->pmove.origin[0];
	delta[1] = cl.oldPlayerState->pmove.origin[1] - cl.playerState->pmove.origin[1];
	delta[2] = cl.oldPlayerState->pmove.origin[2] - cl.playerState->pmove.origin[2];

    if (abs(delta[0]) > 2048 || abs(delta[1]) > 2048 || abs(delta[2]) > 2048)
		cl.oldPlayerState = &cl.playerState;	// Don't interpolate

	// Calculate view origin and angles
	if (!cl_thirdPerson->integer)
		CL_CalcFirstPersonView();
	else
		CL_CalcThirdPersonView();

	// Never let it sit exactly on a node line, because a water plane
	// can disappear when viewed with the eye exactly on it. The server
	// protocol only specifies to 1/8 pixel, so add 1/16 in each axis.
	cl.refDef.viewOrigin[0] += 1.0/16;
	cl.refDef.viewOrigin[1] += 1.0/16;
	cl.refDef.viewOrigin[2] += 1.0/16;

	AnglesToAxis(cl.refDefViewAngles, cl.refDef.viewAxis);

	// Calculate view rect
	CL_CalcVRect();

	// Calculate field of view
	CL_CalcFov();
		
    // Add the view weapon
	CL_AddViewWeapon();
}

/*
 =================
 CL_FadeBlend
 =================
*/
static byte *CL_FadeBlend (int startTime, int totalTime, int fadeInTime, int fadeOutTime, float maxAlpha){

	static color_t	fadeColor;
	int				time;
	float			scale;

	time = cl.time - startTime;
	if (time >= totalTime)
		return NULL;

	if (time < fadeInTime && fadeInTime != 0)
		scale = 1.0 - ((float)(fadeInTime - time) * (1.0 / fadeInTime));
	else {
		if (totalTime - time < fadeOutTime && fadeOutTime != 0)
			scale = (float)(totalTime - time) * (1.0 / fadeOutTime);
		else
			scale = 1.0;
	}

	if (scale > maxAlpha)
		scale = maxAlpha;	// Don't obscure the screen

	fadeColor[0] = 255;
	fadeColor[1] = 255;
	fadeColor[2] = 255;
	fadeColor[3] = 255 * scale;

	return fadeColor;
}

/*
 =================
 CL_DrawViewBlends

 Draw screen blends on top of the game view
 =================
*/
static void CL_DrawViewBlends (void){

	int		contents;
	color_t	color = {255, 255, 255, 192};
	byte	*fadeColor;

	if (!cl_viewBlend->integer || cl_thirdPerson->integer)
		return;

	if (cl_viewBlend->integer == 1){
		// This is just the old poly blend
		if (!cl.playerState->blend[3])
			return;

		*(unsigned *)color = ColorBytes(cl.playerState->blend[0], cl.playerState->blend[1], cl.playerState->blend[2], cl.playerState->blend[3]);

		R_DrawStretchPic(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height, 0, 0, 1, 1, color, cls.media.whiteShader);
		return;
	}

	// See if the player received damage
	if (cl.playerState->stats[STAT_HEALTH] > 0 && cl.playerState->stats[STAT_HEALTH] < 100){
		if (cl.playerState->stats[STAT_HEALTH] < cl.oldPlayerState->stats[STAT_HEALTH]){
			if (!cl.damageTime){
				cl.damageTime = cl.time;
				cl.damageAngle = rand() % 360;
			}
			else {
				if (cl.time - cl.damageTime >= 250)
					cl.damageTime = cl.time + 250;
			}
		}
	}

	// See if the player is underwater
	if ((contents = CL_PointContents(cl.refDef.viewOrigin, -1)) & MASK_WATER){
		if (!cl.underwaterMask){
			// Just entered
			cl.underwaterTime = cl.time;
			cl.underwaterMask = contents;
			cl.underwaterContents = contents;
		}
	}
	else {
		if (cl.underwaterMask){
			// Just exited
			cl.underwaterTime = cl.time;
			cl.underwaterMask = 0;
		}
	}

	// Blood blend
	if (cl.damageTime){
		fadeColor = CL_FadeBlend(cl.damageTime, 1000, 250, 750, 0.75);
		if (fadeColor && !cl.underwaterTime){
			R_DrawRotatedPic(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height, 0, 0, 1, 1, cl.damageAngle, fadeColor, cl.media.viewBloodBlend);

			R_DrawScreenRect(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height, fadeColor);
		}
		else
			cl.damageTime = 0;
	}

	// Water/Slime/Lava blend
	if (cl.underwaterTime){
		if (cl.underwaterMask){
			fadeColor = CL_FadeBlend(cl.underwaterTime, 1000, 1000, 0, 0.75);
			if (fadeColor){
				if (cl.underwaterContents & CONTENTS_LAVA)
					R_DrawStretchPic(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height, 0, 0, 1, 1, fadeColor, cl.media.viewFireBlend);

				R_DrawScreenRect(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height, fadeColor);
			}
			else {
				if (cl.underwaterContents & CONTENTS_LAVA)
					R_DrawStretchPic(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height, 0, 0, 1, 1, color, cl.media.viewFireBlend);

				R_DrawScreenRect(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height, color);
			}
		}
		else {
			fadeColor = CL_FadeBlend(cl.underwaterTime, 1000, 0, 1000, 0.75);
			if (fadeColor){
				if (cl.underwaterContents & CONTENTS_LAVA)
					R_DrawStretchPic(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height, 0, 0, 1, 1, fadeColor, cl.media.viewFireBlend);

				R_DrawScreenRect(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height, fadeColor);
			}
			else
				cl.underwaterTime = 0;
		}
	}

	// Copy the screen if needed
	if (cl.damageTime || cl.underwaterTime)
		R_CopyScreenRect(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height);

	// IR Goggles
	if (cl.refDef.rdFlags & RDF_IRGOGGLES)
		R_DrawStretchPic(cl.refDef.x, cl.refDef.y, cl.refDef.width, cl.refDef.height, 0, 0, 1, 1, colorWhite, cl.media.viewIrGoggles);
}

/*
 =================
 CL_RenderView
 =================
*/
void CL_RenderView (float stereoSeparation){

	vec3_t	viewOrigin;

	if (cls.state != CA_ACTIVE)
		return;

	if (timedemo->integer){
		if (!cl.timeDemoStart)
			cl.timeDemoStart = Sys_Milliseconds();

		cl.timeDemoFrames++;
	}

	if (!cl.frame.valid){
		CL_FillRect(0, 0, 640, 480, colorBlack);
		return;
	}

	// Clear refresh lists
	R_ClearScene();

	// Build refDef
	CL_CalcViewValues();

	// Build refresh lists
	CL_AddPacketEntities();
	CL_AddTempEntities();
	CL_AddLocalEntities();
	CL_AddDynamicLights();
	CL_AddMarks();
	CL_AddParticles();
	CL_AddLightStyles();

	// Finish up the rest of the refDef
	if (cl.testModel)
		CL_AddTestModel();

	cl.refDef.time = cl.time * 0.001;
	cl.refDef.rdFlags = cl.playerState->rdflags;
	cl.refDef.areaBits = cl.frame.areaBits;

	// Offset view origin appropriately if we're doing stereo separation
	if (stereoSeparation){
		VectorCopy(cl.refDef.viewOrigin, viewOrigin);
		VectorMA(cl.refDef.viewOrigin, -stereoSeparation, cl.refDef.viewAxis[1], cl.refDef.viewOrigin);
	}

	// Clear around a sized down screen
	CL_TileClear();

	// Render the refresh lists
	R_RenderScene(&cl.refDef);

	// Restore view origin if we're doing stereo separation
	if (stereoSeparation)
		VectorCopy(viewOrigin, cl.refDef.viewOrigin);

	// Draw screen blends on top of the game view
	CL_DrawViewBlends();

	// Draw all on-screen information
	CL_Draw2D();
}
