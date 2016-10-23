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

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: testModel [modelName]\n");
		return;
	}

	if (cls.state != CA_ACTIVE || cls.cinematicPlaying || cl.demoPlaying){
		Com_Printf("You must be in a map to test a model\n");
		return;
	}

	if (!Com_AllowCheats()){
		Com_Printf("You must enable cheats to test a model\n");
		return;
	}

	cl.testGun = false;
	cl.testModel = false;
	cl.testModelTime = 0;
	cl.testModelFrames = 0;
	memset(&cl.testModelEntity, 0, sizeof(renderEntity_t));

	if (Cmd_Argc() < 2)
		return;

	cl.testModelEntity.reType = RE_MODEL;
	cl.testModelEntity.model = R_RegisterModel(Cmd_Argv(1));

	VectorMA(cl.renderView.viewOrigin, 100, cl.renderView.viewAxis[0], cl.testModelEntity.origin);
	AxisClear(cl.testModelEntity.axis);

	cl.testModelEntity.materialParms[MATERIALPARM_RED] = 1.0;
	cl.testModelEntity.materialParms[MATERIALPARM_GREEN] = 1.0;
	cl.testModelEntity.materialParms[MATERIALPARM_BLUE] = 1.0;
	cl.testModelEntity.materialParms[MATERIALPARM_ALPHA] = 1.0;
	cl.testModelEntity.materialParms[MATERIALPARM_TIME_OFFSET] = MS2SEC(cl.time);
	cl.testModelEntity.materialParms[MATERIALPARM_DIVERSITY] = crand();
	cl.testModelEntity.materialParms[MATERIALPARM_GENERAL] = 0.0;
	cl.testModelEntity.materialParms[MATERIALPARM_MODE] = 1.0;

	cl.testModel = true;
	cl.testModelTime = cl.time;
	cl.testModelFrames = R_ModelFrames(cl.testModelEntity.model);
}

/*
 =================
 CL_TestGun_f
 =================
*/
void CL_TestGun_f (void){

	if (!cl.testModel){
		Com_Printf("No active testModel\n");
		return;
	}

	cl.testGun = !cl.testGun;

	if (cl.testGun){
		cl.testModelEntity.renderFX |= RF_WEAPONMODEL;
		cl.testModelEntity.depthHack = 0.3;
	}
	else {
		cl.testModelEntity.renderFX &= ~RF_WEAPONMODEL;
		cl.testModelEntity.depthHack = 0.0;

		VectorMA(cl.renderView.viewOrigin, 100, cl.renderView.viewAxis[0], cl.testModelEntity.origin);
		AxisClear(cl.testModelEntity.axis);
	}
}

/*
 =================
 CL_TestMaterial_f
 =================
*/
void CL_TestMaterial_f (void){

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: testMaterial [materialName]\n");
		return;
	}

	if (!cl.testModel){
		Com_Printf("No active testModel\n");
		return;
	}

	if (Cmd_Argc() < 2){
		cl.testModelEntity.customMaterial = NULL;
		return;
	}

	cl.testModelEntity.customMaterial = R_RegisterMaterial(Cmd_Argv(1), true);
}

/*
 =================
 CL_TestMaterialParm_f
 =================
*/
void CL_TestMaterialParm_f (void){

	int		index;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: testMaterialParm <index> <value | \"time\">\n");
		return;
	}

	if (!cl.testModel){
		Com_Printf("No active testModel\n");
		return;
	}

	index = atoi(Cmd_Argv(1));
	if (index < 0 || index >= MAX_ENTITY_MATERIAL_PARMS){
		Com_Printf("Specified index is out of range\n");
		return;
	}

	if (!Q_stricmp(Cmd_Argv(2), "time"))
		cl.testModelEntity.materialParms[index] = MS2SEC(cl.time);
	else
		cl.testModelEntity.materialParms[index] = atof(Cmd_Argv(2));
}

/*
 =================
 CL_NextFrame_f
 =================
*/
void CL_NextFrame_f (void){

	if (!cl.testModel){
		Com_Printf("No active testModel\n");
		return;
	}

	if (cl_testModelAnimate->integerValue)
		return;

	cl.testModelEntity.frame++;
	if (cl.testModelEntity.frame >= cl.testModelFrames)
		cl.testModelEntity.frame = 0;

	cl.testModelEntity.oldFrame = cl.testModelEntity.frame;

	Com_Printf("Frame %i\n", cl.testModelEntity.frame);
}

/*
 =================
 CL_PrevFrame_f
 =================
*/
void CL_PrevFrame_f (void){

	if (!cl.testModel){
		Com_Printf("No active testModel\n");
		return;
	}

	if (cl_testModelAnimate->integerValue)
		return;

	cl.testModelEntity.frame--;
	if (cl.testModelEntity.frame < 0)
		cl.testModelEntity.frame = cl.testModelFrames - 1;

	cl.testModelEntity.oldFrame = cl.testModelEntity.frame;

	Com_Printf("Frame %i\n", cl.testModelEntity.frame);
}

/*
 =================
 CL_NextSkin_f
 =================
*/
void CL_NextSkin_f (void){

	if (!cl.testModel){
		Com_Printf("No active testModel\n");
		return;
	}

	cl.testModelEntity.skinIndex++;

	Com_Printf("Skin %i\n", cl.testModelEntity.skinIndex);
}

/*
 =================
 CL_PrevSkin_f
 =================
*/
void CL_PrevSkin_f (void){

	if (!cl.testModel){
		Com_Printf("No active testModel\n");
		return;
	}

	cl.testModelEntity.skinIndex--;
	if (cl.testModelEntity.skinIndex < 0)
		cl.testModelEntity.skinIndex = 0;

	Com_Printf("Skin %i\n", cl.testModelEntity.skinIndex);
}

/*
 =================
 CL_AddTestModel
 =================
*/
static void CL_AddTestModel (void){

	vec3_t	angles;

	if (!cl.testModel)
		return;

	if (cl.testGun){
		VectorCopy(cl.renderView.viewOrigin, cl.testModelEntity.origin);
		AxisCopy(cl.renderView.viewAxis, cl.testModelEntity.axis);

		VectorMA(cl.testModelEntity.origin, cl_testGunX->floatValue, cl.testModelEntity.axis[0], cl.testModelEntity.origin);
		VectorMA(cl.testModelEntity.origin, cl_testGunY->floatValue, cl.testModelEntity.axis[1], cl.testModelEntity.origin);
		VectorMA(cl.testModelEntity.origin, cl_testGunZ->floatValue, cl.testModelEntity.axis[2], cl.testModelEntity.origin);
	}
	else {
		if (cl_testModelRotate->floatValue){
			VectorSet(angles, 0, AngleMod(cl_testModelRotate->floatValue * MS2SEC(cl.time)), 0);
			AnglesToAxis(angles, cl.testModelEntity.axis);
		}
		else
			AxisClear(cl.testModelEntity.axis);
	}

	if (cl_testModelAnimate->integerValue){
		if (cl.time - cl.testModelTime >= 100){
			cl.testModelTime = cl.time;

			cl.testModelEntity.oldFrame = cl.testModelEntity.frame;

			cl.testModelEntity.frame++;
			if (cl.testModelEntity.frame >= cl.testModelFrames)
				cl.testModelEntity.frame = 0;
		}

		cl.testModelEntity.backLerp = 1.0 - (cl.time - cl.testModelTime) * 0.01;
	}
	else {
		cl.testModelTime = cl.time;

		cl.testModelEntity.backLerp = 0.0;
	}

	R_AddEntityToScene(&cl.testModelEntity);
}


/*
 =======================================================================

 BLOOD BLENDS

 =======================================================================
*/

#define MAX_BLOOD_BLENDS		8

#define BLOODBLEND_TIME			1000
#define BLOODBLEND_FADETIME		750

typedef struct {
	qboolean	active;
	int			time;
	float		x;
	float		y;
	float		radius;
	float		rotation;
} bloodBlend_t;

static bloodBlend_t	cl_bloodBlends[MAX_BLOOD_BLENDS];


/*
 =================
 CL_AllocBloodBlend
 =================
*/
static bloodBlend_t *CL_AllocBloodBlend (void){

	int		i;
	int		time, index;

	for (i = 0; i < MAX_BLOOD_BLENDS; i++){
		if (!cl_bloodBlends[i].active){
			memset(&cl_bloodBlends[i], 0, sizeof(bloodBlend_t));
			return &cl_bloodBlends[i];
		}
	}

	// Find the oldest blend
	time = cl.time;
	index = 0;

	for (i = 0; i < MAX_BLOOD_BLENDS; i++){
		if (cl_bloodBlends[i].time < time){
			time = cl_bloodBlends[i].time;
			index = i;
		}
	}

	memset(&cl_bloodBlends[index], 0, sizeof(bloodBlend_t));
	return &cl_bloodBlends[index];
}

/*
 =================
 CL_ClearBloodBlends
 =================
*/
void CL_ClearBloodBlends (void){

	memset(cl_bloodBlends, 0, sizeof(cl_bloodBlends));
}

/*
 =================
 CL_AddBloodBlends
 =================
*/
void CL_AddBloodBlends (void){

	int				i;
	bloodBlend_t	*bb;
	renderEntity_t	entity;
	int				time;
	float			alpha;

	if (!cl_viewBlend->integerValue || cl_thirdPerson->integerValue)
		return;

	for (i = 0, bb = cl_bloodBlends; i < MAX_BLOOD_BLENDS; i++, bb++){
		if (!bb->active)
			continue;

		time = cl.time - bb->time;
		if (time >= BLOODBLEND_TIME){
			bb->active = false;
			continue;
		}

		if (BLOODBLEND_TIME - time < BLOODBLEND_FADETIME)
			alpha = (float)(BLOODBLEND_TIME - time) * (1.0 / BLOODBLEND_FADETIME);
		else
			alpha = 1.0;

		memset(&entity, 0, sizeof(renderEntity_t));

		VectorMA(cl.renderView.viewOrigin, 8, cl.renderView.viewAxis[0], entity.origin);
		VectorMA(entity.origin, bb->x * 8, cl.renderView.viewAxis[1], entity.origin);
		VectorMA(entity.origin, bb->y * 8, cl.renderView.viewAxis[2], entity.origin);

		entity.reType = RE_SPRITE;
		entity.spriteRadius = bb->radius;
		entity.spriteRotation = bb->rotation;
		entity.customMaterial = cl.media.bloodBlendMaterial;
		MakeRGBA(entity.materialParms, 1.0, 1.0, 1.0, alpha);

		R_AddEntityToScene(&entity);
	}
}

/*
 =================
 CL_DamageFeedback
 =================
*/
void CL_DamageFeedback (void){

	bloodBlend_t	*bb;
	int				damage;
	float			scale;

	if (!cl_viewBlend->integerValue || cl_thirdPerson->integerValue)
		return;

	if (cl.playerState->stats[STAT_HEALTH] >= 100)
		return;		// Still full health

	damage = cl.oldPlayerState->stats[STAT_HEALTH] - cl.playerState->stats[STAT_HEALTH];
	if (damage <= 0)
		return;		// Didn't take any damage

	// Add a blood blend
	if (cl.playerState->stats[STAT_HEALTH] <= 50)
		scale = 1.0;
	else
		scale = 50.0 / cl.playerState->stats[STAT_HEALTH];

	bb = CL_AllocBloodBlend();
	bb->active = true;

	bb->time = cl.time;
	bb->x = crand();
	bb->y = crand();
	bb->radius = Clamp(damage * scale, 5, 25) * 2;
	bb->rotation = rand() % 360;

	// Set the end time for the double vision effect
	cl.doubleVisionEndTime = cl.time + 1000;
}


// =====================================================================


/*
 =================
 CL_TestPostProcess_f
 =================
*/
void CL_TestPostProcess_f (void){

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: testPostProcess [materialName]\n");
		return;
	}

	if (cls.state != CA_ACTIVE || cls.cinematicPlaying || cl.demoPlaying){
		Com_Printf("You must be in a map to test a post-process material\n");
		return;
	}

	if (!Com_AllowCheats()){
		Com_Printf("You must enable cheats to test a post-process material\n");
		return;
	}

	cl.testPostProcess = false;
	cl.testPostProcessMaterial = NULL;

	if (Cmd_Argc() < 2)
		return;

	cl.testPostProcess = true;
	cl.testPostProcessMaterial = R_RegisterMaterialNoMip(Cmd_Argv(1));
}

/*
 =================
 CL_ViewPos_f
 =================
*/
void CL_ViewPos_f (void){

	if (cls.state != CA_ACTIVE || cls.cinematicPlaying || cl.demoPlaying){
		Com_Printf("You must be in a map to view the current position\n");
		return;
	}

	Com_Printf("(%i %i %i) : %i\n", (int)cl.renderView.viewOrigin[0], (int)cl.renderView.viewOrigin[1], (int)cl.renderView.viewOrigin[2], (int)cl.renderViewAngles[YAW]);
}

/*
 =================
 CL_CalcFov
 =================
*/
static void CL_CalcFov (void){

	float	f;

	// Interpolate field of view
    cl.renderView.fovX = cl.oldPlayerState->fov + (cl.playerState->fov - cl.oldPlayerState->fov) * cl.lerpFrac;
	
	if (cl.renderView.fovX < 1)
		cl.renderView.fovX = 1;
	else if (cl.renderView.fovX > 179)
		cl.renderView.fovX = 179;

	// Interpolate and account for zoom
	if (cl_zoomFov->integerValue < 1)
		Cvar_SetInteger("cl_zoomFov", 1);
	else if (cl_zoomFov->integerValue > 179)
		Cvar_SetInteger("cl_zoomFov", 179);

	if (cl.zooming){
		f = (cl.time - cl.zoomTime) / 250.0;
		if (f > 1.0)
			cl.renderView.fovX = cl_zoomFov->floatValue;
		else
			cl.renderView.fovX = cl.renderView.fovX + f * (cl_zoomFov->floatValue - cl.renderView.fovX);
	}
	else {
		f = (cl.time - cl.zoomTime) / 250.0;
		if (f > 1.0)
			cl.renderView.fovX = cl.renderView.fovX;
		else
			cl.renderView.fovX = cl_zoomFov->floatValue + f * (cl.renderView.fovX - cl_zoomFov->floatValue);
	}

	// Calculate Y field of view using a 640x480 virtual screen
	f = 640.0 / tan(cl.renderView.fovX / 360.0 * M_PI);
	cl.renderView.fovY = atan2(480.0, f) * 360.0 / M_PI;

	if (cl.zooming)
		cl.zoomSensitivity = cl.renderView.fovY / 75.0;

	// Warp if underwater
	if (cl.underWater){
		f = sin(MS2SEC(cl.time) * 0.4 * M_PI2);

		cl.renderView.fovX += f;
		cl.renderView.fovY -= f;
	}
}

/*
 =================
 CL_CalcFirstPersonView
 =================
*/
static void CL_CalcFirstPersonView (void){

	vec3_t		viewOffset, kickAngles;
	unsigned	delta;

    // Calculate the origin
	if (cl_predict->integerValue && !(cl.playerState->pmove.pm_flags & PMF_NO_PREDICTION)){
		// Use predicted values
		VectorLerp(cl.oldPlayerState->viewoffset, cl.playerState->viewoffset, cl.lerpFrac, viewOffset);
		VectorAdd(cl.predictedOrigin, viewOffset, cl.renderView.viewOrigin);
		VectorMA(cl.renderView.viewOrigin, -(1.0 - cl.lerpFrac), cl.predictedError, cl.renderView.viewOrigin);

        // Smooth out stair climbing
        delta = cls.realTime - cl.predictedStepTime;
        if (delta < 100)
            cl.renderView.viewOrigin[2] -= cl.predictedStep * (100 - delta) * 0.01;
    }
    else {
		// Just use interpolated values
		cl.renderView.viewOrigin[0] = cl.oldPlayerState->pmove.origin[0]*0.125 + cl.oldPlayerState->viewoffset[0] + cl.lerpFrac * (cl.playerState->pmove.origin[0]*0.125 + cl.playerState->viewoffset[0] - (cl.oldPlayerState->pmove.origin[0]*0.125 + cl.oldPlayerState->viewoffset[0]));
		cl.renderView.viewOrigin[1] = cl.oldPlayerState->pmove.origin[1]*0.125 + cl.oldPlayerState->viewoffset[1] + cl.lerpFrac * (cl.playerState->pmove.origin[1]*0.125 + cl.playerState->viewoffset[1] - (cl.oldPlayerState->pmove.origin[1]*0.125 + cl.oldPlayerState->viewoffset[1]));
		cl.renderView.viewOrigin[2] = cl.oldPlayerState->pmove.origin[2]*0.125 + cl.oldPlayerState->viewoffset[2] + cl.lerpFrac * (cl.playerState->pmove.origin[2]*0.125 + cl.playerState->viewoffset[2] - (cl.oldPlayerState->pmove.origin[2]*0.125 + cl.oldPlayerState->viewoffset[2]));
    }

    // Calculate the angles
    if (cl.playerState->pmove.pm_type < PM_DEAD)
		// Use predicted values
		VectorCopy(cl.predictedAngles, cl.renderViewAngles);
    else
		// Just use interpolated values
		LerpAngles(cl.oldPlayerState->viewangles, cl.playerState->viewangles, cl.lerpFrac, cl.renderViewAngles);

	// Account for kick angles
	LerpAngles(cl.oldPlayerState->kick_angles, cl.playerState->kick_angles, cl.lerpFrac, kickAngles);
	VectorAdd(cl.renderViewAngles, kickAngles, cl.renderViewAngles);
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
	cl.renderView.viewOrigin[0] = cl.oldPlayerState->pmove.origin[0]*0.125 + cl.oldPlayerState->viewoffset[0] + cl.lerpFrac * (cl.playerState->pmove.origin[0]*0.125 + cl.playerState->viewoffset[0] - (cl.oldPlayerState->pmove.origin[0]*0.125 + cl.oldPlayerState->viewoffset[0]));
	cl.renderView.viewOrigin[1] = cl.oldPlayerState->pmove.origin[1]*0.125 + cl.oldPlayerState->viewoffset[1] + cl.lerpFrac * (cl.playerState->pmove.origin[1]*0.125 + cl.playerState->viewoffset[1] - (cl.oldPlayerState->pmove.origin[1]*0.125 + cl.oldPlayerState->viewoffset[1]));
	cl.renderView.viewOrigin[2] = cl.oldPlayerState->pmove.origin[2]*0.125 + cl.oldPlayerState->viewoffset[2] + cl.lerpFrac * (cl.playerState->pmove.origin[2]*0.125 + cl.playerState->viewoffset[2] - (cl.oldPlayerState->pmove.origin[2]*0.125 + cl.oldPlayerState->viewoffset[2]));

	// Calculate the angles
	LerpAngles(cl.oldPlayerState->viewangles, cl.playerState->viewangles, cl.lerpFrac, cl.renderViewAngles);

	VectorCopy(cl.renderViewAngles, angles);
	if (angles[PITCH] > 45)
		angles[PITCH] = 45;

	AngleVectors(angles, forward, NULL, NULL);
	VectorMA(cl.renderView.viewOrigin, 512, forward, spot);

	// Calculate exact origin
	VectorCopy(cl.renderView.viewOrigin, origin);
	origin[2] += 8;

	cl.renderViewAngles[PITCH] *= 0.5;
	AngleVectors(cl.renderViewAngles, forward, right, NULL);

	rad = DEG2RAD(cl_thirdPersonAngle->floatValue);
	VectorMA(origin, -cl_thirdPersonRange->floatValue * cos(rad), forward, origin);
	VectorMA(origin, -cl_thirdPersonRange->floatValue * sin(rad), right, origin);

	// Trace a line to make sure the view isn't inside solid geometry
	trace = CL_Trace(cl.renderView.viewOrigin, mins, maxs, origin, cl.clientNum, MASK_PLAYERSOLID, false, NULL);
	if (trace.fraction != 1.0){
		VectorCopy(trace.endpos, origin);
		origin[2] += (1.0 - trace.fraction) * 32;

		trace = CL_Trace(cl.renderView.viewOrigin, mins, maxs, origin, cl.clientNum, MASK_PLAYERSOLID, false, NULL);
		VectorCopy(trace.endpos, origin);
	}

	VectorCopy(origin, cl.renderView.viewOrigin);

	// Calculate pitch to look at spot from camera
	VectorSubtract(spot, cl.renderView.viewOrigin, spot);
	dist = sqrt(spot[0] * spot[0] + spot[1] * spot[1]);
	if (dist < 1)
		dist = 1;

	cl.renderViewAngles[PITCH] = -RAD2DEG(atan2(spot[2], dist));
	cl.renderViewAngles[YAW] -= cl_thirdPersonAngle->floatValue;
}

/*
 =================
 CL_CalcViewValues

 Sets cl.renderView view values
 =================
*/
static void CL_CalcViewValues (void){

	trace_t		trace;
	vec3_t		mins = {-16, -16, -24}, maxs = {16, 16, 32};

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

	if (com_timeDemo->integerValue)
        cl.lerpFrac = 1.0;

	// Calculate view origin and angles
	if (!cl_thirdPerson->integerValue)
		CL_CalcFirstPersonView();
	else
		CL_CalcThirdPersonView();

	// Never let it sit exactly on a node line, because a water plane
	// can disappear when viewed with the eye exactly on it. The server
	// protocol only specifies to 1/8 pixel, so add 1/16 in each axis.
	cl.renderView.viewOrigin[0] += 1.0/16;
	cl.renderView.viewOrigin[1] += 1.0/16;
	cl.renderView.viewOrigin[2] += 1.0/16;

	AnglesToAxis(cl.renderViewAngles, cl.renderView.viewAxis);

	// Check if underwater
	if (CL_PointContents(cl.renderView.viewOrigin, -1) & MASK_WATER){
		cl.underWater = true;

		// Set the end time for the underwater vision effect
		cl.underWaterVisionEndTime = cl.time + 1000;
	}
	else
		cl.underWater = false;

	// See if the player is touching lava
	if (cl.playerState->pmove.pm_flags & PMF_DUCKED){
		mins[2] = -22;
		maxs[2] = 6;
	}
	else {
		mins[2] = -46;
		maxs[2] = 10;
	}

	trace = CL_Trace(cl.renderView.viewOrigin, mins, maxs, cl.renderView.viewOrigin, -1, CONTENTS_LAVA, false, NULL);
	if (trace.contents & CONTENTS_LAVA)
		cl.fireScreenEndTime = cl.time + 1000;

	// Set view rect
	cl.renderView.x = 0;
	cl.renderView.y = 0;
	cl.renderView.width = 640;
	cl.renderView.height = 480;

	// Calculate field of view
	CL_CalcFov();

	// Finish up the rest of the renderView
	cl.renderView.time = MS2SEC(cl.time);
	cl.renderView.areaBits = cl.frame.areaBits;

	cl.renderView.materialParms[0] = 1.0;
	cl.renderView.materialParms[1] = 1.0;
	cl.renderView.materialParms[2] = 1.0;
	cl.renderView.materialParms[3] = 1.0;
	cl.renderView.materialParms[4] = -0.50 * cl.renderView.time;
	cl.renderView.materialParms[5] = -0.25 * cl.renderView.time;
	cl.renderView.materialParms[6] = (cl.playerState->rdflags & RDF_IRGOGGLES) ? 1.0 : 0.0;
	cl.renderView.materialParms[7] = 0.0;
}

/*
 =================
 CL_RenderView
 =================
*/
static void CL_RenderView (void){

	color_t	color;
	float	alpha;
	int		time;

	// Render the scene
	R_RenderScene(&cl.renderView, true);

	// Draw screen blends on top of the game view
	if (!cl_viewBlend->integerValue || cl_thirdPerson->integerValue)
		return;

	// Fire screen
	if (cl.time < cl.fireScreenEndTime){
		// Calculate alpha
		time = cl.fireScreenEndTime - cl.time;

		if (time < 750)
			alpha = (float)time * (1.0 / 750);
		else
			alpha = 1.0;

		// Draw it
		MakeRGBA(color, 255, 255, 255, 192 * alpha);
		R_DrawStretchPic(0, 0, 640, 480, 0, 0, 1, 1, color, cl.media.fireScreenMaterial);
	}

	// Underwater blur
	if (cl.time < cl.underWaterVisionEndTime && (cl.captureFrame + 1 == cls.frameCount)){
		// Calculate alpha
		time = cl.underWaterVisionEndTime - cl.time;

		if (time < 750)
			alpha = (float)time * (1.0 / 750);
		else
			alpha = 1.0;

		// Draw it
		MakeRGBA(color, 255, 255, 255, 255 * alpha);
		R_DrawStretchPic(0, 0, 640, 480, 0, 0, 1, 1, color, cl.media.waterBlurMaterial);
	}
}

/*
 =================
 CL_DoubleVision
 =================
*/
static void CL_DoubleVision (void){

	color_t	color;
	float	offset;
	int		time;

	// Calculate view offset
	time = cl.doubleVisionEndTime - cl.time;

	if (time < 750)
		offset = (float)time * (1.0 / 750);
	else
		offset = 1.0;

	offset *= 0.05;		// Don't offset it too much

	// Render to a texture
	R_CropRender(512, 256, true);
	CL_RenderView();
	R_CaptureRenderToTexture("_accum");
	R_UnCropRender();

	// Draw it
	MakeRGBA(color, 255, 255, 255, 255);
	R_DrawStretchPic(0, 0, 640, 480, offset, 1, 1, 0, color, cl.media.doubleVisionMaterial);
	MakeRGBA(color, 255, 255, 255, 128);
	R_DrawStretchPic(0, 0, 640, 480, 0, 1, 1 - offset, 0, color, cl.media.doubleVisionMaterial);

	cl.captureFrame = cls.frameCount;
}

/*
 =================
 CL_UnderWaterVision
 =================
*/
static void CL_UnderWaterVision (void){

	// Render to a texture
	R_CropRender(512, 256, true);
	CL_RenderView();
	R_CaptureRenderToTexture("_accum");
	R_UnCropRender();

	// Draw it
	R_DrawStretchPic(0, 0, 640, 480, 0, 1, 1, 0, colorTable[COLOR_WHITE], cl.media.underWaterVisionMaterial);

	cl.captureFrame = cls.frameCount;
}

/*
 =================
 CL_RenderActiveFrame
 =================
*/
void CL_RenderActiveFrame (void){

	if (cl_noRender->integerValue)
		return;

	if (!cl.frame.valid){
		CL_FillRect(0, 0, 640, 480, colorTable[COLOR_BLACK]);
		return;
	}

	// Clear render lists
	R_ClearScene();

	// Build renderView
	CL_CalcViewValues();

	// Build render lists
	CL_AddViewWeapon();
	CL_AddPacketEntities();
	CL_AddTempEntities();
	CL_AddLocalEntities();
	CL_AddDynamicLights();
	CL_AddParticles();
	CL_AddBloodBlends();
	CL_AddLightStyles();

	// Add test model
	CL_AddTestModel();

	// Render the view
	if (!cl_viewBlend->integerValue || cl_thirdPerson->integerValue)
		CL_RenderView();
	else {
		if (cl.time < cl.doubleVisionEndTime)
			CL_DoubleVision();
		else if (cl.time < cl.underWaterVisionEndTime)
			CL_UnderWaterVision();
		else
			CL_RenderView();
	}

	// Draw IR Goggles
	if (cl.playerState->rdflags & RDF_IRGOGGLES)
		R_DrawStretchPic(0, 0, 640, 480, 0, 0, 1, 1, colorTable[COLOR_WHITE], cl.media.irGogglesMaterial);

	// Test a post-process material
	if (cl.testPostProcess)
		R_DrawStretchPic(0, 0, 640, 480, 0, 0, 1, 1, colorTable[COLOR_WHITE], cl.testPostProcessMaterial);

	// Draw all on-screen information
	CL_Draw2D();
}
