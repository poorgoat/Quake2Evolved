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


// cl_tempents.c -- client side temporary entities


#include "client.h"


#define MAX_BEAMS		32
#define MAX_STEAMS		32

typedef enum {
	BEAM_PARASITE,
	BEAM_GRAPPLE,
	BEAM_LIGHTNING,
	BEAM_HEAT
} beamType_t;

typedef struct {
	beamType_t	type;
	int			entity;
	int			destEntity;
	int			endTime;
	vec3_t		start;
	vec3_t		end;
	vec3_t		offset;
} beam_t;

typedef struct {
	int			id;
	int			count;
	vec3_t		org;
	vec3_t		dir;
	int			color;
	int			magnitude;
	int			endTime;
	int			thinkInterval;
	int			nextThink;
} steam_t;

static beam_t	cl_beams[MAX_BEAMS];
static steam_t	cl_steams[MAX_STEAMS];


/*
 =================
 CL_ParseParasite
 =================
*/
static int CL_ParseParasite (void){

	beam_t	*b;
	int		i;
	int		entity;
	vec3_t	start, end;

	entity = MSG_ReadShort(&net_message);
	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);

	// Override any beam with the same entity
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++){
		if (b->entity == entity){
			b->type = BEAM_PARASITE;
			b->entity = entity;
			b->endTime = cl.time + 200;
			VectorCopy(start, b->start);
			VectorCopy(end, b->end);
			VectorClear(b->offset);

			return entity;
		}
	}

	// Find a free beam
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++){
		if (b->endTime < cl.time)
			break;
	}

	if (i == MAX_BEAMS){
		Com_DPrintf(S_COLOR_RED "CL_ParseParasite: list overflow\n");
		return entity;
	}

	b->type = BEAM_PARASITE;
	b->entity = entity;
	b->endTime = cl.time + 200;
	VectorCopy(start, b->start);
	VectorCopy(end, b->end);
	VectorClear(b->offset);

	return entity;
}

/*
 =================
 CL_ParseGrapple
 =================
*/
static int CL_ParseGrapple (void){

	beam_t	*b;
	int		i;
	int		entity;
	vec3_t	start, end, offset;

	entity = MSG_ReadShort(&net_message);
	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);
	MSG_ReadPos(&net_message, offset);

	// Override any beam with the same entity
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++){
		if (b->entity == entity){
			b->type = BEAM_GRAPPLE;
			b->entity = entity;
			b->endTime = cl.time + 200;
			VectorCopy(start, b->start);
			VectorCopy(end, b->end);
			VectorCopy(offset, b->offset);
			
			return entity;
		}
	}

	// Find a free beam
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++){
		if (b->endTime < cl.time)
			break;
	}

	if (i == MAX_BEAMS){
		Com_DPrintf(S_COLOR_RED "CL_ParseGrapple: list overflow\n");
		return entity;
	}

	b->type = BEAM_GRAPPLE;
	b->entity = entity;
	b->endTime = cl.time + 200;
	VectorCopy(start, b->start);
	VectorCopy(end, b->end);
	VectorCopy(offset, b->offset);

	return entity;
}

/*
 =================
 CL_ParseLightning
 =================
*/
static int CL_ParseLightning (void){

	beam_t	*b;
	int		i;
	int		entity, destEntity;
	vec3_t	start, end;

	entity = MSG_ReadShort(&net_message);
	destEntity = MSG_ReadShort(&net_message);
	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);

	// Override any beam with the same entity
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++){
		if (b->entity == entity && b->destEntity == destEntity){
			b->type = BEAM_LIGHTNING;
			b->entity = entity;
			b->destEntity = destEntity;
			b->endTime = cl.time + 200;
			VectorCopy(start, b->start);
			VectorSet(b->end, end[0], end[1], end[2] - 5);
			VectorClear(b->offset);

			return entity;
		}
	}

	// Find a free beam
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++){
		if (b->endTime < cl.time)
			break;
	}

	if (i == MAX_BEAMS){
		Com_DPrintf(S_COLOR_RED "CL_ParseLightning: list overflow\n");
		return entity;
	}

	b->type = BEAM_LIGHTNING;
	b->entity = entity;
	b->destEntity = destEntity;
	b->endTime = cl.time + 200;
	VectorCopy(start, b->start);
	VectorSet(b->end, end[0], end[1], end[2] - 5);
	VectorClear(b->offset);

	return entity;
}

/*
 =================
 CL_ParseHeatBeam
 =================
*/
static int CL_ParseHeatBeam (qboolean player){

	beam_t	*b;
	int		i;
	int		entity;
	vec3_t	start, end, offset;

	entity = MSG_ReadShort(&net_message);
	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);

	// Network optimization
	if (player)
		VectorSet(offset, 2, 7, -3);
	else
		VectorSet(offset, 0, 0, 0);

	// Override any beam with the same entity
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++){
		if (b->entity == entity){
			b->type = BEAM_HEAT;
			b->entity = entity;
			b->endTime = cl.time + 200;
			VectorCopy(start, b->start);
			VectorCopy(end, b->end);
			VectorCopy(offset, b->offset);

			return entity;
		}
	}

	// Find a free beam
	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++){
		if (b->endTime < cl.time)
			break;
	}

	if (i == MAX_BEAMS){
		Com_DPrintf(S_COLOR_RED "CL_ParseHeatBeam: list overflow\n");
		return entity;
	}

	b->type = BEAM_HEAT;
	b->entity = entity;
	b->endTime = cl.time + 100;
	VectorCopy(start, b->start);
	VectorCopy(end, b->end);
	VectorCopy(offset, b->offset);

	return entity;
}

/*
 =================
 CL_ParseSteam
 =================
*/
static void CL_ParseSteam (void){

	steam_t	*s;
	int		i, id;
	vec3_t	pos, dir;
	int		count, color, magnitude, time;

	id = MSG_ReadShort(&net_message);
	count = MSG_ReadByte(&net_message);
	MSG_ReadPos(&net_message, pos);
	MSG_ReadDir(&net_message, dir);
	color = MSG_ReadByte(&net_message);
	magnitude = MSG_ReadShort(&net_message);

	// And id of -1 is an instant effect
	if (id == -1){
		CL_SteamParticles(pos, dir, count, color, magnitude);
		return;
	}

	time = MSG_ReadLong(&net_message);

	// Find a free steam
	for (i = 0, s = cl_steams; i < MAX_STEAMS; i++, s++){
		if (s->id == 0)
			break;
	}

	if (i == MAX_STEAMS){
		Com_DPrintf(S_COLOR_RED "CL_ParseSteam: list overflow\n");
		return;
	}

	s->id = id;
	s->count = count;
	VectorCopy(pos, s->org);
	VectorCopy(dir, s->dir);
	s->color = color;
	s->magnitude = magnitude;
	s->endTime = cl.time + time;
	s->thinkInterval = 100;
	s->nextThink = cl.time;
}

/*
 =================
 CL_FindTrailPlane

 Disgusting hack
 =================
*/
static void CL_FindTrailPlane (const vec3_t start, const vec3_t end, vec3_t dir){

	trace_t	trace;
	vec3_t	vec, point;
	float	len;

	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	VectorMA(start, len + 0.5, vec, point);

	trace = CM_BoxTrace(start, point, vec3_origin, vec3_origin, 0, MASK_SOLID);
	if (trace.allsolid || trace.fraction == 1.0){
		VectorClear(dir);
		return;
	}

	VectorCopy(trace.plane.normal, dir);
}

/*
 =================
 CL_FindExplosionPlane

 Disgusting hack
 =================
*/
static void CL_FindExplosionPlane (const vec3_t org, float radius, vec3_t dir){

	static vec3_t	planes[6] = {{0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, {0, -1, 0}, {-1, 0, 0}};
	trace_t			trace;
	vec3_t			point;
	float			best = 1.0;
	int				i;

	VectorClear(dir);

	for (i = 0; i < 6; i++){
		VectorMA(org, radius, planes[i], point);

		trace = CM_BoxTrace(org, point, vec3_origin, vec3_origin, 0, MASK_SOLID);
		if (trace.allsolid || trace.fraction == 1.0)
			continue;

		if (trace.fraction < best){
			best = trace.fraction;
			VectorCopy(trace.plane.normal, dir);
		}
	}
}

/*
 =================
 CL_ParseTempEntity
 =================
*/
void CL_ParseTempEntity (void){

	int		teType;
	vec3_t	pos, pos2, dir;
	int		count, color, i;
	int		ent;

	teType = MSG_ReadByte(&net_message);

	switch (teType){
	case TE_BLOOD:			// Bullet hitting flesh
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_Bleed(pos, dir, 4, false);
		
		break;
	case TE_GUNSHOT:		// Bullet hitting wall
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BulletParticles(pos, dir);
		CL_ImpactMark(pos, dir, rand() % 360, 3, 1, 1, 1, 1, false, cl.media.bulletMarkShader, false);

		i = rand() & 15;
		if (i >= 1 && i <= 3)
			S_StartSound(pos, 0, 0, cl.media.sfxRichotecs[i-1], 1, ATTN_NORM, 0);

		break;
	case TE_SHOTGUN:		// Bullet hitting wall
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BulletParticles(pos, dir);
		CL_ImpactMark(pos, dir, rand() % 360, 3, 1, 1, 1, 1, false, cl.media.bulletMarkShader, false);

		break;
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_DamageSparkParticles(pos, dir, 6, 0xe0);

		if (teType == TE_BULLET_SPARKS){
			i = rand() & 15;
			if (i >= 1 && i <= 3)
				S_StartSound(pos, 0, 0, cl.media.sfxRichotecs[i-1], 1, ATTN_NORM, 0);
		}

		break;
	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		if (teType == TE_SCREEN_SPARKS)
			CL_DamageSparkParticles(pos, dir, 40, 0xd0);
		else
			CL_DamageSparkParticles(pos, dir, 40, 0xb0);

		S_StartSound(pos, 0, 0, cl.media.sfxLaserHit, 1, ATTN_NORM, 0);

		break;
	case TE_LASER_SPARKS:
		count = MSG_ReadByte(&net_message);
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		color = MSG_ReadByte(&net_message);

		CL_LaserSparkParticles(pos, dir, count, color);

		break;
	case TE_SPLASH:
		count = MSG_ReadByte(&net_message);
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		i = MSG_ReadByte(&net_message);

		switch (i){
		case SPLASH_SPARKS:
			CL_SparkParticles(pos, dir, count);
			S_StartSound(pos, 0, 0, cl.media.sfxSparks[rand() & 3], 1, ATTN_STATIC, 0);
			break;
		case SPLASH_BLUE_WATER:
			CL_WaterSplash(pos, dir);
			CL_SplashParticles(pos, dir, count * 2, 1, 25);
			break;
		case SPLASH_BROWN_WATER:
			CL_WaterSplash(pos, dir);
			CL_SplashParticles(pos, dir, count * 2, 1, 25);
			break;
		case SPLASH_SLIME:
			CL_WaterSplash(pos, dir);
			CL_SplashParticles(pos, dir, count * 2, 1, 25);
			break;
		case SPLASH_LAVA:
			CL_LavaSteamParticles(pos, dir, count);
			break;
		case SPLASH_BLOOD:
			CL_Bleed(pos, dir, count / 15, false);
			break;
		default:
			CL_SplashParticles(pos, dir, count * 2, 1, 25);
			break;
		}

		break;
	case TE_BLUEHYPERBLASTER:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BlasterParticles(pos, dir, 0.00, 0.00, 1.00);
		CL_ImpactMark(pos, dir, rand() % 360, 3, 0, 0, 1, 1, true, cl.media.energyMarkShader, false);

		break;
	case TE_BLASTER:		// Blaster hitting wall
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BlasterParticles(pos, dir, 0.97, 0.46, 0.14);
		CL_ImpactMark(pos, dir, rand() % 360, 3, 0.97, 0.46, 0.14, 1, true, cl.media.energyMarkShader, false);
		CL_DynamicLight(pos, 150, 1.0, 1.0, 0.0, true, 350);
		S_StartSound(pos, 0, 0, cl.media.sfxLaserHit, 1, ATTN_NORM, 0);

		break;
	case TE_RAILTRAIL:		// Railgun effect
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);

		// HACK!!!
		CL_FindTrailPlane(pos, pos2, dir);

		CL_RailTrail(pos, pos2);
		CL_ImpactMark(pos2, dir, rand() % 360, 3, 0.09, 0.32, 0.43, 1, true, cl.media.energyMarkShader, false);
		S_StartSound(pos2, 0, 0, cl.media.sfxRailgun, 1, ATTN_NORM, 0);

		break;
	case TE_GRENADE_EXPLOSION:
	case TE_GRENADE_EXPLOSION_WATER:
		MSG_ReadPos(&net_message, pos);
		VectorSet(dir, 0, 0, 1);

		if (teType != TE_GRENADE_EXPLOSION_WATER){
			CL_Explosion(pos, dir, 40, rand() % 360, 350, 1.0, 0.5, 0.5, cl.media.grenadeExplosionShader);
			CL_ExplosionParticles(pos);
			CL_ImpactMark(pos, dir, rand() % 360, 40, 1, 1, 1, 1, false, cl.media.burnMarkShader, false);
			S_StartSound(pos, 0, 0, cl.media.sfxGrenadeExplosion, 1, ATTN_NORM, 0);
		}
		else {
			CL_Explosion(pos, dir, 40, rand() % 360, 350, 1.0, 0.5, 0.5, cl.media.grenadeExplosionWaterShader);
			CL_ExplosionWaterSplash(pos);
			CL_BubbleParticles(pos, 384, 30);
			CL_ImpactMark(pos, dir, rand() % 360, 40, 1, 1, 1, 1, false, cl.media.burnMarkShader, false);
			S_StartSound(pos, 0, 0, cl.media.sfxWaterExplosion, 1, ATTN_NORM, 0);
		}

		break;
	case TE_EXPLOSION1:
	case TE_EXPLOSION2:
	case TE_ROCKET_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
		MSG_ReadPos(&net_message, pos);

		// HACK!!!
		CL_FindExplosionPlane(pos, 40, dir);

		if (teType != TE_ROCKET_EXPLOSION_WATER){
			CL_Explosion(pos, dir, 40, rand() % 360, 350, 1.0, 0.5, 0.5, cl.media.rocketExplosionShader);
			CL_ExplosionParticles(pos);
			CL_ImpactMark(pos, dir, rand() % 360, 40, 1, 1, 1, 1, false, cl.media.burnMarkShader, false);
			S_StartSound(pos, 0, 0, cl.media.sfxRocketExplosion, 1, ATTN_NORM, 0);
		}
		else {
			CL_Explosion(pos, dir, 40, rand() % 360, 350, 1.0, 0.5, 0.5, cl.media.rocketExplosionWaterShader);
			CL_ExplosionWaterSplash(pos);
			CL_BubbleParticles(pos, 384, 30);
			CL_ImpactMark(pos, dir, rand() % 360, 40, 1, 1, 1, 1, false, cl.media.burnMarkShader, false);
			S_StartSound(pos, 0, 0, cl.media.sfxWaterExplosion, 1, ATTN_NORM, 0);
		}

		break;
	case TE_EXPLOSION1_NP:
	case TE_EXPLOSION1_BIG:
	case TE_PLAIN_EXPLOSION:
	case TE_PLASMA_EXPLOSION:
		MSG_ReadPos(&net_message, pos);

		if (teType != TE_EXPLOSION1_BIG){
			// HACK!!!
			CL_FindExplosionPlane(pos, 40, dir);

			CL_Explosion(pos, dir, 40, rand() % 360, 350, 1.0, 0.5, 0.5, cl.media.rocketExplosionShader);
			CL_ExplosionParticles(pos);
			CL_ImpactMark(pos, dir, rand() % 360, 40, 1, 1, 1, 1, false, cl.media.burnMarkShader, false);
			S_StartSound(pos, 0, 0, cl.media.sfxRocketExplosion, 1, ATTN_NORM, 0);
		}
		else {
			// HACK!!!
			CL_FindExplosionPlane(pos, 60, dir);

			CL_Explosion(pos, dir, 60, rand() % 360, 500, 1.0, 0.5, 0.5, cl.media.rocketExplosionShader);
			CL_ExplosionParticles(pos);
			CL_ImpactMark(pos, dir, rand() % 360, 60, 1, 1, 1, 1, false, cl.media.burnMarkShader, false);
			S_StartSound(pos, 0, 0, cl.media.sfxRocketExplosion, 1, ATTN_NORM, 0);
		}

		break;
	case TE_BFG_EXPLOSION:
	case TE_BFG_BIGEXPLOSION:
		MSG_ReadPos(&net_message, pos);
		
		if (teType != TE_BFG_BIGEXPLOSION){
			// HACK!!!
			CL_FindExplosionPlane(pos, 40, dir);

			CL_Explosion(pos, vec3_origin, 40, 0, 350, 0.0, 1.0, 0.0, cl.media.bfgExplosionShader);
			CL_ImpactMark(pos, dir, rand() % 360, 40, 1, 1, 1, 1, false, cl.media.burnMarkShader, false);
		}
		else {
			// HACK!!!
			CL_FindExplosionPlane(pos, 60, dir);

			CL_Explosion(pos, vec3_origin, 60, 0, 500, 0.0, 1.0, 0.0, cl.media.bfgExplosionShader);
			CL_BFGExplosionParticles(pos);
			CL_ImpactMark(pos, dir, rand() % 360, 60, 1, 1, 1, 1, false, cl.media.burnMarkShader, false);
		}

		break;
	case TE_BFG_LASER:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);

		CL_LaserBeam(pos, pos2, 4, 0xd0d1d2d3, 75, 100, cl.media.laserBeamShader);

		break;
	case TE_BUBBLETRAIL:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);

		CL_BubbleTrail(pos, pos2, 32, 1);

		break;
	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
		CL_ParseParasite();

		break;
	case TE_GRAPPLE_CABLE:
		CL_ParseGrapple();

		break;
	case TE_BOSSTPORT:		// Boss teleporting to station
		MSG_ReadPos(&net_message, pos);

		CL_BigTeleportParticles(pos);
		S_StartSound(pos, 0, 0, S_RegisterSound("misc/bigtele.wav"), 1, ATTN_NONE, 0);

		break;
	case TE_WELDING_SPARKS:
		count = MSG_ReadByte(&net_message);
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		color = MSG_ReadByte(&net_message);

		CL_WeldingSparkParticles(pos, dir, count, color);
		CL_DynamicLight(pos, 100 + (rand() & 75), 1.0, 1.0, 0.3, true, 100);

		break;
	case TE_TUNNEL_SPARKS:
		count = MSG_ReadByte(&net_message);
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		color = MSG_ReadByte(&net_message);

		CL_TunnelSparkParticles(pos, dir, count, color);

		break;
	case TE_GREENBLOOD:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_Bleed(pos, dir, 4, true);

		break;
	case TE_MOREBLOOD:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_Bleed(pos, dir, 16, false);
		
		break;
	case TE_BLASTER2:		// Green blaster hitting wall
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BlasterParticles(pos, dir, 0.00, 1.00, 0.00);
		CL_ImpactMark(pos, dir, rand() % 360, 3, 0, 1, 0, 1, true, cl.media.energyMarkShader, false);
		CL_DynamicLight(pos, 150, 0.0, 1.0, 0.0, true, 350);
		S_StartSound(pos, 0, 0, cl.media.sfxLaserHit, 1, ATTN_NORM, 0);

		break;
	case TE_FLECHETTE:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BulletParticles(pos, dir);
		CL_ImpactMark(pos, dir, rand() % 360, 3, 1, 1, 1, 1, false, cl.media.bulletMarkShader, false);
		CL_DynamicLight(pos, 150, 0.19, 0.41, 0.75, true, 350);
		S_StartSound(pos, 0, 0, cl.media.sfxLaserHit, 1, ATTN_NORM, 0);

		break;
	case TE_LIGHTNING:
		ent = CL_ParseLightning();
		S_StartSound(NULL, ent, CHAN_WEAPON, cl.media.sfxLightning, 1, ATTN_NORM, 0);

		break;
	case TE_FLASHLIGHT:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadShort(&net_message);

		CL_DynamicLight(pos, 400, 1.0, 1.0, 1.0, false, 100);

		break;
	case TE_FORCEWALL:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);
		color = MSG_ReadByte(&net_message);

		CL_ForceWallParticles(pos, pos2, color);

		break;
	case TE_HEATBEAM:
		CL_ParseHeatBeam(true);

		break;
	case TE_MONSTER_HEATBEAM:
		CL_ParseHeatBeam(false);

		break;
	case TE_HEATBEAM_SPARKS:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BlasterParticles(pos, dir, 0.97, 0.46, 0.14);
		S_StartSound(pos, 0, 0, cl.media.sfxLaserHit, 1, ATTN_NORM, 0);
		
		break;
	case TE_HEATBEAM_STEAM:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
	
		CL_BlasterParticles(pos, dir, 0.97, 0.46, 0.14);
		CL_ImpactMark(pos, dir, rand() % 360, 3, 0.97, 0.46, 0.14, 1, true, cl.media.energyMarkShader, false);
		S_StartSound(pos, 0, 0, cl.media.sfxLaserHit, 1, ATTN_NORM, 0);
		
		break;
	case TE_STEAM:
		CL_ParseSteam();

		break;
	case TE_BUBBLETRAIL2:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);

		CL_BubbleTrail(pos, pos2, 8, 1);
		S_StartSound(pos, 0, 0, cl.media.sfxLaserHit, 1, ATTN_NORM, 0);
		
		break;
	case TE_CHAINFIST_SMOKE:
		MSG_ReadPos(&net_message, pos);

		CL_SmokePuffParticles(pos, 3, 10);

		break;
	case TE_ELECTRIC_SPARKS:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BulletParticles(pos, dir);
		S_StartSound(pos, 0, 0, cl.media.sfxLaserHit, 1, ATTN_NORM, 0);
		
		break;
	case TE_TRACKER_EXPLOSION:
		MSG_ReadPos(&net_message, pos);

		CL_TrackerExplosionParticles(pos);
		CL_DynamicLight(pos, 150, -1.0, -1.0, -1.0, false, 100);
		S_StartSound(pos, 0, 0, cl.media.sfxDisruptorExplosion, 1, ATTN_NORM, 0);
		
		break;
	case TE_TELEPORT_EFFECT:
	case TE_DBALL_GOAL:
		MSG_ReadPos(&net_message, pos);

		CL_TeleportParticles(pos);

		break;
	case TE_WIDOWBEAMOUT:
		MSG_ReadShort(&net_message);
		MSG_ReadPos(&net_message, pos);

		break;
	case TE_WIDOWSPLASH:
		MSG_ReadPos(&net_message, pos);

		break;
	case TE_NUKEBLAST:
		MSG_ReadPos(&net_message, pos);
		VectorSet(dir, 0, 0, 1);

		CL_NukeShockwave(pos);

		for (i = 0; i < 5; i++){
			pos2[0] = pos[0];
			pos2[1] = pos[1];
			pos2[2] = pos[2] + 60 + (i * 60);

			CL_Explosion(pos2, dir, 90, rand() % 360, 0, 0, 0, 0, cl.media.rocketExplosionShader);
			CL_NukeSmokeParticles(pos2);
		}

		for (i = 0; i < 25; i++){
			pos2[0] = pos[0] + crand() * 150;
			pos2[1] = pos[1] + crand() * 150;
			pos2[2] = pos[2] + crand() * 150 + 360;

			CL_Explosion(pos2, dir, 90, rand() % 360, 0, 0, 0, 0, cl.media.rocketExplosionShader);
			CL_NukeSmokeParticles(pos2);
		}

		CL_ImpactMark(pos, dir, rand() % 360, 240, 1, 1, 1, 1, false, cl.media.burnMarkShader, false);

		break;
	default:
		Com_Error(ERR_DROP, "CL_ParseTempEntity: bad teType (%i)", teType);
	}
}

/*
 =================
 CL_AddBeams
 =================
*/
static void CL_AddBeams (void){

	beam_t		*b;
	int			i;
	entity_t	ent;
	vec3_t		org, vec, angles;
	float		dist, len, handMult;

	if (cl_hand->integer == 2)
		handMult = 0;
	else if (cl_hand->integer == 1)
		handMult = -1;
	else
		handMult = 1;

	for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++){
		if (b->endTime < cl.time)
			continue;

		if (b->type != BEAM_HEAT){
			// If coming from the player, update the start position
			if (b->entity == cl.clientNum && !cl_thirdPerson->integer){
				b->start[0] = cl.refDef.viewOrigin[0];
				b->start[1] = cl.refDef.viewOrigin[1];
				b->start[2] = cl.refDef.viewOrigin[2] - 22;
			}

			VectorAdd(b->start, b->offset, org);
			VectorSubtract(b->end, org, vec);
		}
		else {
			// If coming from the player, update the start position
			if (b->entity == cl.clientNum && !cl_thirdPerson->integer){
				// Set up gun position
				b->start[0] = cl.refDef.viewOrigin[0] + cl.oldPlayerState->gunoffset[0] + (cl.playerState->gunoffset[0] - cl.oldPlayerState->gunoffset[0]) * cl.lerpFrac;
				b->start[1] = cl.refDef.viewOrigin[1] + cl.oldPlayerState->gunoffset[1] + (cl.playerState->gunoffset[1] - cl.oldPlayerState->gunoffset[1]) * cl.lerpFrac;
				b->start[2] = cl.refDef.viewOrigin[2] + cl.oldPlayerState->gunoffset[2] + (cl.playerState->gunoffset[2] - cl.oldPlayerState->gunoffset[2]) * cl.lerpFrac;

				VectorMA(b->start, -(handMult * b->offset[0]), cl.refDef.viewAxis[1], org);
				VectorMA(org, b->offset[1], cl.refDef.viewAxis[0], org);
				VectorMA(org, b->offset[2], cl.refDef.viewAxis[2], org);

				if (cl_hand->integer == 2)
					VectorMA(org, -1, cl.refDef.viewAxis[2], org);

				VectorSubtract(b->end, org, vec);

				len = VectorLength(vec);
				VectorScale(cl.refDef.viewAxis[0], len, vec);

				VectorMA(vec, -(handMult * b->offset[0]), cl.refDef.viewAxis[1], vec);
				VectorMA(vec, b->offset[1], cl.refDef.viewAxis[0], vec);
				VectorMA(vec, b->offset[2], cl.refDef.viewAxis[2], vec);
			}
			else {
				VectorAdd(b->start, b->offset, org);
				VectorSubtract(b->end, org, vec);
			}

			// Add the particle trail
			CL_HeatBeamTrail(org, vec);
		}

		// Add the beam entity
		memset(&ent, 0, sizeof(ent));

		if (b->type != BEAM_PARASITE){
			ent.entityType = ET_BEAM;
			VectorCopy(org, ent.origin);
			VectorAdd(org, vec, ent.oldOrigin);
			MakeRGBA(ent.shaderRGBA, 255, 255, 255, 255);

			switch (b->type){
			case BEAM_GRAPPLE:
				ent.frame = 2;
				ent.oldFrame = 30;
				ent.customShader = cl.media.grappleBeamShader;
				break;
			case BEAM_LIGHTNING:
				ent.frame = 10;
				ent.oldFrame = 30;
				ent.customShader = cl.media.lightningBeamShader;
				break;
			case BEAM_HEAT:
				ent.frame = 2;
				ent.oldFrame = 30;
				ent.customShader = cl.media.heatBeamShader;
				break;
			}

			R_AddEntityToScene(&ent);
			continue;
		}

		VectorToAngles(vec, angles);
		angles[2] = rand() % 360;

		ent.entityType = ET_MODEL;
		ent.model = cl.media.modParasiteBeam;
		AnglesToAxis(angles, ent.axis);
		MakeRGBA(ent.shaderRGBA, 255, 255, 255, 255);

		dist = VectorNormalize(vec);
		len = (dist - 30) / (ceil(dist / 30) - 1);

		while (dist > 0){
			VectorCopy(org, ent.origin);
			VectorCopy(org, ent.oldOrigin);

			R_AddEntityToScene(&ent);

			VectorMA(org, len, vec, org);
			dist -= 30;
		}
	}
}

/*
 =================
 CL_AddSteams
 =================
*/
static void CL_AddSteams (void){

	steam_t	*s;
	int		i;

	for (i = 0, s = cl_steams; i < MAX_STEAMS; i++, s++){
		if (s->id == 0)
			continue;

		if (s->endTime < cl.time){
			s->id = 0;
			continue;
		}

		if (cl.time >= s->nextThink){
			CL_SteamParticles(s->org, s->dir, s->count, s->color, s->magnitude);

			s->nextThink += s->thinkInterval;
		}
	}
}

/*
 =================
 CL_ClearTempEntities
 =================
*/
void CL_ClearTempEntities (void){

	memset(cl_beams, 0, sizeof(cl_beams));
	memset(cl_steams, 0, sizeof(cl_steams));
}

/*
 =================
 CL_AddTempEntities
 =================
*/
void CL_AddTempEntities (void){

	CL_AddBeams();
	CL_AddSteams();
}
