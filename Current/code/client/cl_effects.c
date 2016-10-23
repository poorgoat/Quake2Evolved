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


// cl_effects.c -- client side effects parsing and management


#include "client.h"


/*
 =======================================================================

 LIGHT STYLE MANAGEMENT

 =======================================================================
*/

typedef struct {
	int		length;
	float	map[MAX_QPATH];
	vec3_t	rgb;
} clightStyle_t;

static clightStyle_t	cl_lightStyles[MAX_LIGHTSTYLES];
static int				cl_lastOfs;


/*
 =================
 CL_ClearLightStyles
 =================
*/
void CL_ClearLightStyles (void){

	memset(cl_lightStyles, 0, sizeof(cl_lightStyles));

	cl_lastOfs = -1;
}

/*
 =================
 CL_RunLightStyles
 =================
*/
void CL_RunLightStyles (void){

	clightStyle_t	*ls;
	int				i, ofs;

	ofs = cl.time / 100;
	if (ofs == cl_lastOfs)
		return;
	cl_lastOfs = ofs;

	for (i = 0, ls = cl_lightStyles; i < MAX_LIGHTSTYLES; i++, ls++){
		if (!ls->length){
			ls->rgb[0] = ls->rgb[1] = ls->rgb[2] = 1.0;
			continue;
		}

		if (ls->length == 1)
			ls->rgb[0] = ls->rgb[1] = ls->rgb[2] = ls->map[0];
		else
			ls->rgb[0] = ls->rgb[1] = ls->rgb[2] = ls->map[ofs % ls->length];
	}
}

/*
 =================
 CL_AddLightStyles
 =================
*/
void CL_AddLightStyles (void){

	clightStyle_t	*ls;
	int				i;

	for (i = 0, ls = cl_lightStyles; i < MAX_LIGHTSTYLES; i++, ls++)
		R_SetLightStyle(i, ls->rgb[0], ls->rgb[1], ls->rgb[2]);
}

/*
 =================
 CL_SetLightStyle
 =================
*/
void CL_SetLightStyle (int style){

	char	*s;
	int		i, len;

	if (style < 0 || style >= MAX_LIGHTSTYLES)
		Com_Error(ERR_DROP, "CL_SetLightStyle: style = %i", style);

	s = cl.configStrings[CS_LIGHTS + style];

	len = strlen(s);
	if (len >= MAX_QPATH)
		Com_Error(ERR_DROP, "CL_SetLightStyle: style length = %i", len);

	cl_lightStyles[style].length = len;

	for (i = 0; i < len; i++)
		cl_lightStyles[style].map[i] = (float)(s[i] - 'a') / (float)('m' - 'a');
}


/*
 =======================================================================

 DYNAMIC LIGHT MANAGEMENT

 =======================================================================
*/

#define MAX_DLIGHTS			32

typedef struct {
	qboolean	active;
	int			start;
	int			end;
	vec3_t		origin;
	float		intensity;
	vec3_t		color;
	qboolean	fade;
} dlight_t;

static dlight_t	cl_dynamicLights[MAX_DLIGHTS];


/*
 =================
 CL_AllocDynamicLight
 =================
*/
static dlight_t *CL_AllocDynamicLight (void){

	int		i;
	int		time, index;

	for (i = 0; i < MAX_DLIGHTS; i++){
		if (!cl_dynamicLights[i].active){
			memset(&cl_dynamicLights[i], 0, sizeof(dlight_t));
			return &cl_dynamicLights[i];
		}
	}

	// Find the oldest light
	time = cl.time;
	index = 0;

	for (i = 0; i < MAX_DLIGHTS; i++){
		if (cl_dynamicLights[i].start < time){
			time = cl_dynamicLights[i].start;
			index = i;
		}
	}

	memset(&cl_dynamicLights[index], 0, sizeof(dlight_t));
	return &cl_dynamicLights[index];
}

/*
 =================
 CL_ClearDynamicLights
 =================
*/
void CL_ClearDynamicLights (void){

	memset(cl_dynamicLights, 0, sizeof(cl_dynamicLights));
}

/*
 =================
 CL_AddDynamicLights
 =================
*/
void CL_AddDynamicLights (void){

	int				i;
	dlight_t		*dl;
	renderLight_t	light;
	float			intensity;

	for (i = 0, dl = cl_dynamicLights; i < MAX_DLIGHTS; i++, dl++){
		if (!dl->active)
			continue;

		if (cl.time >= dl->end){
			dl->active = false;
			continue;
		}

		if (!dl->fade)
			intensity = dl->intensity;
		else {
			intensity = (float)(cl.time - dl->start) / (dl->end - dl->start);
			intensity = dl->intensity * (1.0 - intensity);
		}

		// Create a new light
		VectorCopy(dl->origin, light.origin);
		VectorClear(light.center);
		AxisClear(light.axis);
		VectorSet(light.radius, intensity, intensity, intensity);

		light.style = 0;
		light.detailLevel = 10;
		light.parallel = false;
		light.noShadows = false;

		light.customMaterial = NULL;
		light.materialParms[MATERIALPARM_RED] = dl->color[0];
		light.materialParms[MATERIALPARM_GREEN] = dl->color[1];
		light.materialParms[MATERIALPARM_BLUE] = dl->color[2];
		light.materialParms[MATERIALPARM_ALPHA] = 1.0;
		light.materialParms[MATERIALPARM_TIME_OFFSET] = MS2SEC(dl->start);
		light.materialParms[MATERIALPARM_DIVERSITY] = 0.0;
		light.materialParms[MATERIALPARM_GENERAL] = 0.0;
		light.materialParms[MATERIALPARM_MODE] = 1.0;

		// Send it to the renderer
		R_AddLightToScene(&light);
	}
}

/*
 =================
 CL_DynamicLight
 =================
*/
void CL_DynamicLight (const vec3_t org, float intensity, float r, float g, float b, qboolean fade, int duration){

	dlight_t		*dl;
	renderLight_t	light;

	// A duration of 0 means a temporary light, so we should just send
	// it to the renderer and forget about it
	if (duration == 0){
		VectorCopy(org, light.origin);
		VectorClear(light.center);
		AxisClear(light.axis);
		VectorSet(light.radius, intensity, intensity, intensity);

		light.style = 0;
		light.detailLevel = 10;
		light.parallel = false;
		light.noShadows = false;

		light.customMaterial = NULL;
		light.materialParms[MATERIALPARM_RED] = r;
		light.materialParms[MATERIALPARM_GREEN] = g;
		light.materialParms[MATERIALPARM_BLUE] = b;
		light.materialParms[MATERIALPARM_ALPHA] = 1.0;
		light.materialParms[MATERIALPARM_TIME_OFFSET] = 0.0;
		light.materialParms[MATERIALPARM_DIVERSITY] = 0.0;
		light.materialParms[MATERIALPARM_GENERAL] = 0.0;
		light.materialParms[MATERIALPARM_MODE] = 1.0;

		// Send it to the renderer
		R_AddLightToScene(&light);
		return;
	}

	// Add a new dynamic light
	dl = CL_AllocDynamicLight();
	dl->active = true;

	dl->start = cl.time;
	dl->end = dl->start + duration;
	VectorCopy(org, dl->origin);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->fade = fade;
}


/*
 =======================================================================

 DECALS

 =======================================================================
*/


/*
 =================
 CL_ProjectDecal
 =================
*/
void CL_ProjectDecal (const vec3_t org, const vec3_t dir, float radius, struct material_s *material){

	if (!cl_decals->integerValue)
		return;

	R_ProjectDecal(org, dir, rand() % 360, radius, cl.time, material);
}


/*
 =======================================================================

 MUZZLE FLASH PARSING

 =======================================================================
*/


/*
 =================
 CL_ParsePlayerMuzzleFlash
 =================
*/
void CL_ParsePlayerMuzzleFlash (void){

	int			ent;
	entity_t	*cent;
	vec3_t		forward, right;
	int			weapon;
	float		volume = 1.0;
	char		name[MAX_OSPATH];
	vec3_t		origin;

	ent = MSG_ReadShort(&net_message);
	if (ent < 1 || ent >= MAX_EDICTS)
		Com_Error(ERR_DROP, "CL_ParsePlayerMuzzleFlash: ent = %i", ent);

	weapon = MSG_ReadByte(&net_message);
	
	if (weapon & MZ_SILENCED){
		weapon &= ~MZ_SILENCED;
		volume = 0.2;
	}

	cent = &cl.entities[ent];

	if (cl_muzzleFlashes->integerValue){
		cent->flashStartTime = MS2SEC(cl.time);
		cent->flashRotation = rand() % 360;
	}

	AngleVectors(cent->current.angles, forward, right, NULL);

	origin[0] = cent->current.origin[0] + forward[0] * 18 + right[0] * 16;
	origin[1] = cent->current.origin[1] + forward[1] * 18 + right[1] * 16;
	origin[2] = cent->current.origin[2] + forward[2] * 18 + right[2] * 16;

	switch (weapon){
	case MZ_BLASTER:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);

		break;
	case MZ_BLUEHYPERBLASTER:
		CL_DynamicLight(origin, 200 + (rand()&31), 0, 0, 1, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);

		break;
	case MZ_HYPERBLASTER:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);

		break;
	case MZ_MACHINEGUN:
		CL_MachinegunEjectBrass(cent, 1, 15, -8, 18);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		Q_snprintfz(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound(name), volume, ATTN_NORM, 0);

		break;
	case MZ_SHOTGUN:
		CL_ShotgunEjectBrass(cent, 1, 12, -6, 16);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/shotgf1b.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, ent, CHAN_AUTO, S_RegisterSound("weapons/shotgr1b.wav"), volume, ATTN_NORM, 100);

		break;
	case MZ_SSHOTGUN:
		CL_ShotgunEjectBrass(cent, 2, 8, -8, 16);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/sshotf1b.wav"), volume, ATTN_NORM, 0);

		break;
	case MZ_CHAINGUN1:
		CL_MachinegunEjectBrass(cent, 1, 10, -8, 18);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0.25, 0, false, 1);
		Q_snprintfz(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound(name), volume, ATTN_NORM, 0);

		break;
	case MZ_CHAINGUN2:
		CL_MachinegunEjectBrass(cent, 2, 10, -8, 18);
		CL_DynamicLight(origin, 255 + (rand()&31), 1, 0.5, 0, false, 1);
		Q_snprintfz(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound(name), volume, ATTN_NORM, 0);
		Q_snprintfz(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound(name), volume, ATTN_NORM, 50);

		break;
	case MZ_CHAINGUN3:
		CL_MachinegunEjectBrass(cent, 3, 10, -8, 18);
		CL_DynamicLight(origin, 250 + (rand()&31), 1, 1, 0, false, 1);
		Q_snprintfz(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound(name), volume, ATTN_NORM, 0);
		Q_snprintfz(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound(name), volume, ATTN_NORM, 33);
		Q_snprintfz(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound(name), volume, ATTN_NORM, 66);

		break;
	case MZ_RAILGUN:
		CL_DynamicLight(origin, 200 + (rand()&31), 0.5, 0.5, 1, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/railgf1a.wav"), volume, ATTN_NORM, 0);

		break;
	case MZ_ROCKET:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0.5, 0.2, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/rocklf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, ent, CHAN_AUTO, S_RegisterSound("weapons/rocklr1b.wav"), volume, ATTN_NORM, 100);

		break;
	case MZ_GRENADE:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0.5, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, ent, CHAN_AUTO, S_RegisterSound("weapons/grenlr1b.wav"), volume, ATTN_NORM, 100);

		break;
	case MZ_BFG:
		CL_DynamicLight(origin, 200 + (rand()&31), 0, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/bfg__f1y.wav"), volume, ATTN_NORM, 0);

		break;
	case MZ_LOGIN:
		CL_DynamicLight(origin, 200 + (rand()&31), 0, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogParticles(cent->current.origin, 0, 1, 0);

		break;
	case MZ_LOGOUT:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogParticles(cent->current.origin, 1, 0, 0);

		break;
	case MZ_PHALANX:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0.5, 0.5, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/plasshot.wav"), volume, ATTN_NORM, 0);

		break;
	case MZ_IONRIPPER:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0.5, 0.5, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/rippfire.wav"), volume, ATTN_NORM, 0);

		break;
	case MZ_ETF_RIFLE:
		CL_DynamicLight(origin, 200 + (rand()&31), 0.9, 0.7, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/nail1.wav"), volume, ATTN_NORM, 0);

		break;
	case MZ_HEATBEAM:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 100);

		break;
	case MZ_BLASTER2:
		CL_DynamicLight(origin, 200 + (rand()&31), 0, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);

		break;
	case MZ_TRACKER:
		CL_DynamicLight(origin, 200 + (rand()&31), -1, -1, -1, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav"), volume, ATTN_NORM, 0);

		break;		
	case MZ_NUKE1:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0, 0, false, 100);

		break;
	case MZ_NUKE2:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 100);

		break;
	case MZ_NUKE4:
		CL_DynamicLight(origin, 200 + (rand()&31), 0, 0, 1, false, 100);

		break;
	case MZ_NUKE8:
		CL_DynamicLight(origin, 200 + (rand()&31), 0, 1, 1, false, 100);

		break;
	}
}

/*
 =================
 CL_ParseMonsterMuzzleFlash
 =================
*/
void CL_ParseMonsterMuzzleFlash (void){

	int			ent;
	entity_t	*cent;
	vec3_t		forward, right;
	int			flash;
	char		name[MAX_OSPATH];
	vec3_t		origin;

	ent = MSG_ReadShort(&net_message);
	if (ent < 1 || ent >= MAX_EDICTS)
		Com_Error(ERR_DROP, "CL_ParseMonsterMuzzleFlash: ent = %i", ent);

	flash = MSG_ReadByte(&net_message);

	cent = &cl.entities[ent];

	if (cl_muzzleFlashes->integerValue){
		cent->flashStartTime = MS2SEC(cl.time);
		cent->flashRotation = rand() % 360;
	}

	AngleVectors(cent->current.angles, forward, right, NULL);
	
	origin[0] = cent->current.origin[0] + forward[0] * monster_flash_offset[flash][0] + right[0] * monster_flash_offset[flash][1];
	origin[1] = cent->current.origin[1] + forward[1] * monster_flash_offset[flash][0] + right[1] * monster_flash_offset[flash][1];
	origin[2] = cent->current.origin[2] + forward[2] * monster_flash_offset[flash][0] + right[2] * monster_flash_offset[flash][1] + monster_flash_offset[flash][2];
	
	switch (flash){
	case MZ2_INFANTRY_MACHINEGUN_1:
	case MZ2_INFANTRY_MACHINEGUN_2:
	case MZ2_INFANTRY_MACHINEGUN_3:
	case MZ2_INFANTRY_MACHINEGUN_4:
	case MZ2_INFANTRY_MACHINEGUN_5:
	case MZ2_INFANTRY_MACHINEGUN_6:
	case MZ2_INFANTRY_MACHINEGUN_7:
	case MZ2_INFANTRY_MACHINEGUN_8:
	case MZ2_INFANTRY_MACHINEGUN_9:
	case MZ2_INFANTRY_MACHINEGUN_10:
	case MZ2_INFANTRY_MACHINEGUN_11:
	case MZ2_INFANTRY_MACHINEGUN_12:
	case MZ2_INFANTRY_MACHINEGUN_13:
		CL_MachinegunEjectBrass(cent, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_SOLDIER_MACHINEGUN_1:
	case MZ2_SOLDIER_MACHINEGUN_2:
	case MZ2_SOLDIER_MACHINEGUN_3:
	case MZ2_SOLDIER_MACHINEGUN_4:
	case MZ2_SOLDIER_MACHINEGUN_5:
	case MZ2_SOLDIER_MACHINEGUN_6:
	case MZ2_SOLDIER_MACHINEGUN_7:
	case MZ2_SOLDIER_MACHINEGUN_8:
		CL_MachinegunEjectBrass(cent, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck3.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_GUNNER_MACHINEGUN_1:
	case MZ2_GUNNER_MACHINEGUN_2:
	case MZ2_GUNNER_MACHINEGUN_3:
	case MZ2_GUNNER_MACHINEGUN_4:
	case MZ2_GUNNER_MACHINEGUN_5:
	case MZ2_GUNNER_MACHINEGUN_6:
	case MZ2_GUNNER_MACHINEGUN_7:
	case MZ2_GUNNER_MACHINEGUN_8:
		CL_MachinegunEjectBrass(cent, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("gunner/gunatck2.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_ACTOR_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_2:
	case MZ2_SUPERTANK_MACHINEGUN_3:
	case MZ2_SUPERTANK_MACHINEGUN_4:
	case MZ2_SUPERTANK_MACHINEGUN_5:
	case MZ2_SUPERTANK_MACHINEGUN_6:
	case MZ2_TURRET_MACHINEGUN:
		CL_MachinegunEjectBrass(cent, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_BOSS2_MACHINEGUN_L1:
	case MZ2_BOSS2_MACHINEGUN_L2:
	case MZ2_BOSS2_MACHINEGUN_L3:
	case MZ2_BOSS2_MACHINEGUN_L4:
	case MZ2_BOSS2_MACHINEGUN_L5:
	case MZ2_CARRIER_MACHINEGUN_L1:
	case MZ2_CARRIER_MACHINEGUN_L2:
		CL_MachinegunEjectBrass(cent, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NONE, 0);

		break;
	case MZ2_SOLDIER_BLASTER_1:
	case MZ2_SOLDIER_BLASTER_2:
	case MZ2_SOLDIER_BLASTER_3:
	case MZ2_SOLDIER_BLASTER_4:
	case MZ2_SOLDIER_BLASTER_5:
	case MZ2_SOLDIER_BLASTER_6:
	case MZ2_SOLDIER_BLASTER_7:
	case MZ2_SOLDIER_BLASTER_8:
	case MZ2_TURRET_BLASTER:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck2.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_FLYER_BLASTER_1:
	case MZ2_FLYER_BLASTER_2:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("flyer/flyatck3.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_MEDIC_BLASTER_1:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("medic/medatck1.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_HOVER_BLASTER_1:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("hover/hovatck1.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_FLOAT_BLASTER_1:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("floater/fltatck1.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_SOLDIER_SHOTGUN_1:
	case MZ2_SOLDIER_SHOTGUN_2:
	case MZ2_SOLDIER_SHOTGUN_3:
	case MZ2_SOLDIER_SHOTGUN_4:
	case MZ2_SOLDIER_SHOTGUN_5:
	case MZ2_SOLDIER_SHOTGUN_6:
	case MZ2_SOLDIER_SHOTGUN_7:
	case MZ2_SOLDIER_SHOTGUN_8:
		CL_ShotgunEjectBrass(cent, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck1.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_TANK_BLASTER_1:
	case MZ2_TANK_BLASTER_2:
	case MZ2_TANK_BLASTER_3:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_TANK_MACHINEGUN_1:
	case MZ2_TANK_MACHINEGUN_2:
	case MZ2_TANK_MACHINEGUN_3:
	case MZ2_TANK_MACHINEGUN_4:
	case MZ2_TANK_MACHINEGUN_5:
	case MZ2_TANK_MACHINEGUN_6:
	case MZ2_TANK_MACHINEGUN_7:
	case MZ2_TANK_MACHINEGUN_8:
	case MZ2_TANK_MACHINEGUN_9:
	case MZ2_TANK_MACHINEGUN_10:
	case MZ2_TANK_MACHINEGUN_11:
	case MZ2_TANK_MACHINEGUN_12:
	case MZ2_TANK_MACHINEGUN_13:
	case MZ2_TANK_MACHINEGUN_14:
	case MZ2_TANK_MACHINEGUN_15:
	case MZ2_TANK_MACHINEGUN_16:
	case MZ2_TANK_MACHINEGUN_17:
	case MZ2_TANK_MACHINEGUN_18:
	case MZ2_TANK_MACHINEGUN_19:
		CL_MachinegunEjectBrass(cent, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);
		Q_snprintfz(name, sizeof(name), "tank/tnkatk2%c.wav", 'a' + rand() % 5);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound(name), 1, ATTN_NORM, 0);

		break;
	case MZ2_CHICK_ROCKET_1:
	case MZ2_TURRET_ROCKET:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0.5, 0.2, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("chick/chkatck2.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_TANK_ROCKET_1:
	case MZ2_TANK_ROCKET_2:
	case MZ2_TANK_ROCKET_3:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0.5, 0.2, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/tnkatck1.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_SUPERTANK_ROCKET_1:
	case MZ2_SUPERTANK_ROCKET_2:
	case MZ2_SUPERTANK_ROCKET_3:
	case MZ2_BOSS2_ROCKET_1:
	case MZ2_BOSS2_ROCKET_2:
	case MZ2_BOSS2_ROCKET_3:
	case MZ2_BOSS2_ROCKET_4:
	case MZ2_CARRIER_ROCKET_1:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0.5, 0.2, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/rocket.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_GUNNER_GRENADE_1:
	case MZ2_GUNNER_GRENADE_2:
	case MZ2_GUNNER_GRENADE_3:
	case MZ2_GUNNER_GRENADE_4:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 0.5, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("gunner/gunatck3.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_GLADIATOR_RAILGUN_1:
	case MZ2_CARRIER_RAILGUN:
	case MZ2_WIDOW_RAIL:
		CL_DynamicLight(origin, 200 + (rand()&31), 0.5, 0.5, 1.0, false, 1);

		break;
	case MZ2_MAKRON_BFG:
		CL_DynamicLight(origin, 200 + (rand()&31), 0.5, 1, 0.5, false, 1);

		break;
	case MZ2_MAKRON_BLASTER_1:
	case MZ2_MAKRON_BLASTER_2:
	case MZ2_MAKRON_BLASTER_3:
	case MZ2_MAKRON_BLASTER_4:
	case MZ2_MAKRON_BLASTER_5:
	case MZ2_MAKRON_BLASTER_6:
	case MZ2_MAKRON_BLASTER_7:
	case MZ2_MAKRON_BLASTER_8:
	case MZ2_MAKRON_BLASTER_9:
	case MZ2_MAKRON_BLASTER_10:
	case MZ2_MAKRON_BLASTER_11:
	case MZ2_MAKRON_BLASTER_12:
	case MZ2_MAKRON_BLASTER_13:
	case MZ2_MAKRON_BLASTER_14:
	case MZ2_MAKRON_BLASTER_15:
	case MZ2_MAKRON_BLASTER_16:
	case MZ2_MAKRON_BLASTER_17:
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("makron/blaster.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_JORG_MACHINEGUN_L1:
	case MZ2_JORG_MACHINEGUN_L2:
	case MZ2_JORG_MACHINEGUN_L3:
	case MZ2_JORG_MACHINEGUN_L4:
	case MZ2_JORG_MACHINEGUN_L5:
	case MZ2_JORG_MACHINEGUN_L6:
		CL_MachinegunEjectBrass(cent, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("boss3/xfire.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_JORG_MACHINEGUN_R1:
	case MZ2_JORG_MACHINEGUN_R2:
	case MZ2_JORG_MACHINEGUN_R3:
	case MZ2_JORG_MACHINEGUN_R4:
	case MZ2_JORG_MACHINEGUN_R5:
	case MZ2_JORG_MACHINEGUN_R6:
		CL_MachinegunEjectBrass(cent, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);

		break;
	case MZ2_JORG_BFG_1:
		CL_DynamicLight(origin, 200 + (rand()&31), 0.5, 1, 0.5, false, 1);

		break;
	case MZ2_BOSS2_MACHINEGUN_R1:
	case MZ2_BOSS2_MACHINEGUN_R2:
	case MZ2_BOSS2_MACHINEGUN_R3:
	case MZ2_BOSS2_MACHINEGUN_R4:
	case MZ2_BOSS2_MACHINEGUN_R5:
	case MZ2_CARRIER_MACHINEGUN_R1:
	case MZ2_CARRIER_MACHINEGUN_R2:
		CL_MachinegunEjectBrass(cent, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand()&31), 1, 1, 0, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);

		break;
	case MZ2_STALKER_BLASTER:
	case MZ2_DAEDALUS_BLASTER:
	case MZ2_MEDIC_BLASTER_2:
	case MZ2_WIDOW_BLASTER:
	case MZ2_WIDOW_BLASTER_SWEEP1:
	case MZ2_WIDOW_BLASTER_SWEEP2:
	case MZ2_WIDOW_BLASTER_SWEEP3:
	case MZ2_WIDOW_BLASTER_SWEEP4:
	case MZ2_WIDOW_BLASTER_SWEEP5:
	case MZ2_WIDOW_BLASTER_SWEEP6:
	case MZ2_WIDOW_BLASTER_SWEEP7:
	case MZ2_WIDOW_BLASTER_SWEEP8:
	case MZ2_WIDOW_BLASTER_SWEEP9:
	case MZ2_WIDOW_BLASTER_100:
	case MZ2_WIDOW_BLASTER_90:
	case MZ2_WIDOW_BLASTER_80:
	case MZ2_WIDOW_BLASTER_70:
	case MZ2_WIDOW_BLASTER_60:
	case MZ2_WIDOW_BLASTER_50:
	case MZ2_WIDOW_BLASTER_40:
	case MZ2_WIDOW_BLASTER_30:
	case MZ2_WIDOW_BLASTER_20:
	case MZ2_WIDOW_BLASTER_10:
	case MZ2_WIDOW_BLASTER_0:
	case MZ2_WIDOW_BLASTER_10L:
	case MZ2_WIDOW_BLASTER_20L:
	case MZ2_WIDOW_BLASTER_30L:
	case MZ2_WIDOW_BLASTER_40L:
	case MZ2_WIDOW_BLASTER_50L:
	case MZ2_WIDOW_BLASTER_60L:
	case MZ2_WIDOW_BLASTER_70L:
	case MZ2_WIDOW_RUN_1:
	case MZ2_WIDOW_RUN_2:
	case MZ2_WIDOW_RUN_3:
	case MZ2_WIDOW_RUN_4:
	case MZ2_WIDOW_RUN_5:
	case MZ2_WIDOW_RUN_6:
	case MZ2_WIDOW_RUN_7:
	case MZ2_WIDOW_RUN_8:
		CL_DynamicLight(origin, 200 + (rand()&31), 0, 1, 0, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_WIDOW_DISRUPTOR:
		CL_DynamicLight(origin, 200 + (rand()&31), -1, -1, -1, false, 1);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav"), 1, ATTN_NORM, 0);

		break;
	case MZ2_WIDOW_PLASMABEAM:
	case MZ2_WIDOW2_BEAMER_1:
	case MZ2_WIDOW2_BEAMER_2:
	case MZ2_WIDOW2_BEAMER_3:
	case MZ2_WIDOW2_BEAMER_4:
	case MZ2_WIDOW2_BEAMER_5:
	case MZ2_WIDOW2_BEAM_SWEEP_1:
	case MZ2_WIDOW2_BEAM_SWEEP_2:
	case MZ2_WIDOW2_BEAM_SWEEP_3:
	case MZ2_WIDOW2_BEAM_SWEEP_4:
	case MZ2_WIDOW2_BEAM_SWEEP_5:
	case MZ2_WIDOW2_BEAM_SWEEP_6:
	case MZ2_WIDOW2_BEAM_SWEEP_7:
	case MZ2_WIDOW2_BEAM_SWEEP_8:
	case MZ2_WIDOW2_BEAM_SWEEP_9:
	case MZ2_WIDOW2_BEAM_SWEEP_10:
	case MZ2_WIDOW2_BEAM_SWEEP_11:
		CL_DynamicLight(origin, 300 + (rand()&100), 1, 1, 0, false, 200);

		break;
	}
}
