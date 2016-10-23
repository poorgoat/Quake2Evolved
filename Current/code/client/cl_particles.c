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


#include "client.h"


#define MAX_PARTICLES			8192

#define PARTICLE_BOUNCE			1
#define PARTICLE_FRICTION		2
#define PARTICLE_UNDERWATER		4
#define PARTICLE_STRETCH		8
#define PARTICLE_INSTANT		16

typedef struct particle_s {
	struct particle_s	*next;

	struct material_s	*material;
	int					time;
	int					flags;

	vec3_t				org;
	vec3_t				vel;
	vec3_t				accel;
	vec3_t				color;
	vec3_t				colorVel;
	float				alpha;
	float				alphaVel;
	float				radius;
	float				radiusVel;
	float				length;
	float				lengthVel;
	float				rotation;
	float				bounceFactor;

	vec3_t				lastOrg;

	renderPolyVertex_t	vertices[4];
} particle_t;

static particle_t	cl_particleList[MAX_PARTICLES];
static particle_t	*cl_activeParticles;
static particle_t	*cl_freeParticles;

static vec3_t		cl_particleVelocities[NUM_VERTEX_NORMALS];
static vec3_t		cl_particlePalette[256];


/*
 =================
 CL_FreeParticle
 =================
*/
static void CL_FreeParticle (particle_t *p){

	p->next = cl_freeParticles;
	cl_freeParticles = p;
}

/*
 =================
 CL_AllocParticle
 =================
*/
static particle_t *CL_AllocParticle (void){

	particle_t	*p;

	if (!cl_freeParticles)
		return NULL;

	p = cl_freeParticles;
	cl_freeParticles = p->next;
	p->next = cl_activeParticles;
	cl_activeParticles = p;

	return p;
}

/*
 =================
 CL_ClearParticles
 =================
*/
void CL_ClearParticles (void){

	int		i;
	byte	palette[] = {
#include "../render/palette.h"
	};

	cl_activeParticles = NULL;
	cl_freeParticles = cl_particleList;

	for (i = 0; i < MAX_PARTICLES; i++)
		cl_particleList[i].next = &cl_particleList[i+1];

	cl_particleList[MAX_PARTICLES-1].next = NULL;

	for (i = 0; i < NUM_VERTEX_NORMALS; i++){
		cl_particleVelocities[i][0] = (rand() & 255) * 0.01;
		cl_particleVelocities[i][1] = (rand() & 255) * 0.01;
		cl_particleVelocities[i][2] = (rand() & 255) * 0.01;
	}

	for (i = 0; i < 256; i++){
		cl_particlePalette[i][0] = palette[i*3+0] * (1.0/255);
		cl_particlePalette[i][1] = palette[i*3+1] * (1.0/255);
		cl_particlePalette[i][2] = palette[i*3+2] * (1.0/255);
	}
}

/*
 =================
 CL_AddParticles
 =================
*/
void CL_AddParticles (void){

	particle_t			*p, *next;
	particle_t			*active = NULL, *tail = NULL;
	renderPoly_t		poly;
	renderPolyVertex_t	*vertices;
	color_t				modulate;
	vec3_t				mins, maxs, vec, axis[3];
	vec3_t				org, org2, vel, color;
	float				alpha, radius, length;
	float				time, time2, gravity;
	float				dot, rad, s, c;
	int					i, contents;
	trace_t				trace;

	if (!cl_particles->integerValue)
		return;

	gravity = cl.playerState->pmove.gravity / 800.0;

	for (p = cl_activeParticles; p; p = next){
		// Grab next now, so if the particle is freed we still have it
		next = p->next;

		time = MS2SEC(cl.time - p->time);
		time2 = time * time;

		alpha = p->alpha + p->alphaVel * time;
		radius = p->radius + p->radiusVel * time;
		length = p->length + p->lengthVel * time;

		if (alpha <= 0 || radius <= 0 || length <= 0){
			// Faded out
			CL_FreeParticle(p);
			continue;
		}

		color[0] = p->color[0] + p->colorVel[0] * time;
		color[1] = p->color[1] + p->colorVel[1] * time;
		color[2] = p->color[2] + p->colorVel[2] * time;

		org[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		org[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		org[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2 * gravity;

		if (p->flags & PARTICLE_UNDERWATER){
			// Underwater particle
			VectorSet(org2, org[0], org[1], org[2] + radius);

			if (!(CL_PointContents(org2, -1) & MASK_WATER)){
				// Not underwater
				CL_FreeParticle(p);
				continue;
			}
		}

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else {
			tail->next = p;
			tail = p;
		}

		if (p->flags & PARTICLE_FRICTION){
			// Water friction affected particle
			contents = CL_PointContents(org, -1);
			if (contents & MASK_WATER){
				// Add friction
				if (contents & CONTENTS_WATER){
					VectorScale(p->vel, 0.25, p->vel);
					VectorScale(p->accel, 0.25, p->accel);
				}
				if (contents & CONTENTS_SLIME){
					VectorScale(p->vel, 0.20, p->vel);
					VectorScale(p->accel, 0.20, p->accel);
				}
				if (contents & CONTENTS_LAVA){
					VectorScale(p->vel, 0.10, p->vel);
					VectorScale(p->accel, 0.10, p->accel);
				}

				length = 1;

				// Don't add friction again
				p->flags &= ~PARTICLE_FRICTION;

				// Reset
				p->time = cl.time;
				VectorCopy(org, p->org);
				VectorCopy(color, p->color);
				p->alpha = alpha;
				p->radius = radius;

				// Don't stretch
				p->flags &= ~PARTICLE_STRETCH;

				p->length = length;
				p->lengthVel = 0;
			}
		}

		if (p->flags & PARTICLE_BOUNCE){
			// Bouncy particle
			VectorSet(mins, -radius, -radius, -radius);
			VectorSet(maxs, radius, radius, radius);

			trace = CL_Trace(p->lastOrg, mins, maxs, org, cl.clientNum, MASK_SOLID, true, NULL);
			if (trace.fraction != 0.0 && trace.fraction != 1.0){
				// Reflect velocity
				time = cl.time - SEC2MS(cls.frameTime + cls.frameTime * trace.fraction);
				time = MS2SEC(time - p->time);

				VectorSet(vel, p->vel[0], p->vel[1], p->vel[2] + p->accel[2] * gravity * time);
				VectorReflect(vel, trace.plane.normal, p->vel);
				VectorScale(p->vel, p->bounceFactor, p->vel);

				// Check for stop or slide along the plane
				if (trace.plane.normal[2] > 0 && p->vel[2] < 1){
					if (trace.plane.normal[2] == 1){
						VectorClear(p->vel);
						VectorClear(p->accel);

						p->flags &= ~PARTICLE_BOUNCE;
					}
					else {
						// FIXME: check for new plane or free fall
						dot = DotProduct(p->vel, trace.plane.normal);
						VectorMA(p->vel, -dot, trace.plane.normal, p->vel);

						dot = DotProduct(p->accel, trace.plane.normal);
						VectorMA(p->accel, -dot, trace.plane.normal, p->accel);
					}
				}

				VectorCopy(trace.endpos, org);
				length = 1;

				// Reset
				p->time = cl.time;
				VectorCopy(org, p->org);
				VectorCopy(color, p->color);
				p->alpha = alpha;
				p->radius = radius;

				// Don't stretch
				p->flags &= ~PARTICLE_STRETCH;

				p->length = length;
				p->lengthVel = 0;
			}
		}

		if (p->flags & PARTICLE_INSTANT){
			// Instant particle
			p->alpha = 0;
			p->alphaVel = 0;
		}

		// Save current origin if needed
		if (p->flags & (PARTICLE_BOUNCE | PARTICLE_STRETCH)){
			VectorCopy(p->lastOrg, org2);
			VectorCopy(org, p->lastOrg);	// FIXME: pause
		}

		// Cull
		VectorSubtract(org, cl.renderView.viewOrigin, vec);
		if (DotProduct(vec, cl.renderView.viewAxis[0]) < 0)
			continue;

		// Clamp color and alpha and convert to byte
		modulate[0] = 255 * Clamp(color[0], 0.0, 1.0);
		modulate[1] = 255 * Clamp(color[1], 0.0, 1.0);
		modulate[2] = 255 * Clamp(color[2], 0.0, 1.0);
		modulate[3] = 255 * Clamp(alpha, 0.0, 1.0);

		// Build the poly
		poly.material = p->material;
		poly.numVertices = 4;
		poly.vertices = p->vertices;

		// Build the vertices
		vertices = p->vertices;

		if (length != 1.0){
			// Find orientation vectors
			VectorSubtract(cl.renderView.viewOrigin, org, axis[0]);
			VectorSubtract(org2, org, axis[1]);
			CrossProduct(axis[0], axis[1], axis[2]);

			VectorNormalizeFast(axis[1]);
			VectorNormalizeFast(axis[2]);

			// Find normal
			CrossProduct(axis[1], axis[2], axis[0]);
			VectorNormalizeFast(axis[0]);

			VectorMA(org, -length, axis[1], org2);
			VectorScale(axis[2], radius, axis[2]);

			// Set up vertices
			vertices[0].xyz[0] = org2[0] + axis[2][0];
			vertices[0].xyz[1] = org2[1] + axis[2][1];
			vertices[0].xyz[2] = org2[2] + axis[2][2];
			vertices[1].xyz[0] = org[0] + axis[2][0];
			vertices[1].xyz[1] = org[1] + axis[2][1];
			vertices[1].xyz[2] = org[2] + axis[2][2];
			vertices[2].xyz[0] = org[0] - axis[2][0];
			vertices[2].xyz[1] = org[1] - axis[2][1];
			vertices[2].xyz[2] = org[2] - axis[2][2];
			vertices[3].xyz[0] = org2[0] - axis[2][0];
			vertices[3].xyz[1] = org2[1] - axis[2][1];
			vertices[3].xyz[2] = org2[2] - axis[2][2];
		}
		else {
			// Calculate axes
			if (p->rotation){
				rad = DEG2RAD(p->rotation);
				s = sin(rad);
				c = cos(rad);

				VectorNegate(cl.renderView.viewAxis[0], axis[0]);

				VectorScale(cl.renderView.viewAxis[1], c * radius, axis[1]);
				VectorMA(axis[1], -s * radius, cl.renderView.viewAxis[2], axis[1]);

				VectorScale(cl.renderView.viewAxis[2], c * radius, axis[2]);
				VectorMA(axis[2], s * radius, cl.renderView.viewAxis[1], axis[2]);
			}
			else {
				VectorNegate(cl.renderView.viewAxis[0], axis[0]);
				VectorScale(cl.renderView.viewAxis[1], radius, axis[1]);
				VectorScale(cl.renderView.viewAxis[2], radius, axis[2]);
			}

			// Set up vertices
			vertices[0].xyz[0] = org[0] + axis[1][0] + axis[2][0];
			vertices[0].xyz[1] = org[1] + axis[1][1] + axis[2][1];
			vertices[0].xyz[2] = org[2] + axis[1][2] + axis[2][2];
			vertices[1].xyz[0] = org[0] - axis[1][0] + axis[2][0];
			vertices[1].xyz[1] = org[1] - axis[1][1] + axis[2][1];
			vertices[1].xyz[2] = org[2] - axis[1][2] + axis[2][2];
			vertices[2].xyz[0] = org[0] - axis[1][0] - axis[2][0];
			vertices[2].xyz[1] = org[1] - axis[1][1] - axis[2][1];
			vertices[2].xyz[2] = org[2] - axis[1][2] - axis[2][2];
			vertices[3].xyz[0] = org[0] + axis[1][0] - axis[2][0];
			vertices[3].xyz[1] = org[1] + axis[1][1] - axis[2][1];
			vertices[3].xyz[2] = org[2] + axis[1][2] - axis[2][2];
		}

		vertices[0].st[0] = 0.0;
		vertices[0].st[1] = 0.0;
		vertices[1].st[0] = 1.0;
		vertices[1].st[1] = 0.0;
		vertices[2].st[0] = 1.0;
		vertices[2].st[1] = 1.0;
		vertices[3].st[0] = 0.0;
		vertices[3].st[1] = 1.0;

		for (i = 0; i < 4; i++){
			vertices->normal[0] = axis[0][0];
			vertices->normal[1] = axis[0][1];
			vertices->normal[2] = axis[0][2];
			vertices->color[0] = modulate[0];
			vertices->color[1] = modulate[1];
			vertices->color[2] = modulate[2];
			vertices->color[3] = modulate[3];

			vertices++;
		}

		// Send the particle geometry to the renderer
		R_AddPolyToScene(&poly);
	}

	cl_activeParticles = active;
}

/*
 =================
 CL_BlasterTrail
 =================
*/
void CL_BlasterTrail (const vec3_t start, const vec3_t end, float r, float g, float b){

	particle_t	*p;
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 1.5;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.3 + frand() * 0.2);
		p->radius = 2.4 + (1.2 * crand());
		p->radiusVel = -2.4 + (1.2 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_GrenadeTrail
 =================
*/
void CL_GrenadeTrail (const vec3_t start, const vec3_t end){

	particle_t	*p;
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(end, -1) & MASK_WATER){
		if (CL_PointContents(start, -1) & MASK_WATER)
			CL_BubbleTrail(start, end, 16, 1);

		return;
	}
	
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 20;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.liteSmokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->radius = 3 + (1.5 * crand());
		p->radiusVel = 3 + (1.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_RocketTrail
 =================
*/
void CL_RocketTrail (const vec3_t start, const vec3_t end){

	particle_t	*p;
	int			flags = 0; // warning fix
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(end, -1) & MASK_WATER){
		if (CL_PointContents(start, -1) & MASK_WATER)
			CL_BubbleTrail(start, end, 8, 3);

		return;
	}

	// Flames
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 1;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.flameParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -2.0 / (0.2 + frand() * 0.1);
		p->radius = 3 + (1.5 * crand());
		p->radiusVel = -6 + (3 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}
	
	// Smoke
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 10;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->colorVel[0] = 0.75;
		p->colorVel[1] = 0.75;
		p->colorVel[2] = 0.75;
		p->alpha = 0.5;
		p->alphaVel = -(0.2 + frand() * 0.1);
		p->radius = 3 + (1.5 * crand());
		p->radiusVel = 15 + (7.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_RailTrail
 =================
*/
void CL_RailTrail (const vec3_t start, const vec3_t end){

	particle_t	*p;
	vec3_t		move, vec;
	float		len, dist;
	int			i;
	vec3_t		right, up, dir;
	float		d, s, c;

	if (!cl_particles->integerValue)
		return;

	// Core
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 2;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = crand() * 1.5;
		p->vel[1] = crand() * 1.5;
		p->vel[2] = crand() * 1.5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1;
		p->color[1] = 1;
		p->color[2] = 1;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -0.5 / (1.0 + frand() * 0.2);
		p->radius = 1.5 + (0.5 * crand());
		p->radiusVel = 3 + (1.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}

	// Spiral
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	MakeNormalVectors(vec, right, up);

	for (i = 0; i < len; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		d = i * 0.1;
		s = sin(d);
		c = cos(d);

		dir[0] = right[0]*c + up[0]*s;
		dir[1] = right[1]*c + up[1]*s;
		dir[2] = right[2]*c + up[2]*s;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + dir[0] * 2.5;
		p->org[1] = move[1] + dir[1] * 2.5;
		p->org[2] = move[2] + dir[2] * 2.5;
		p->vel[0] = dir[0] * 5;
		p->vel[1] = dir[1] * 5;
		p->vel[2] = dir[2] * 5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.09;
		p->color[1] = 0.32;
		p->color[2] = 0.43;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0;
		p->radius = 2.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_BFGTrail
 =================
*/
void CL_BFGTrail (const vec3_t start, const vec3_t end){

	particle_t	*p;
	vec3_t		move, vec, org;
	float		len, dist, d, time;
	float		angle, sy, cy, sp, cp;
	int			i;

	if (!cl_particles->integerValue)
		return;

	// Particles
	time = MS2SEC(cl.time);

	VectorLerp(start, end, cl.lerpFrac, org);
	
	for (i = 0; i < NUM_VERTEX_NORMALS; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		angle = time * cl_particleVelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = time * cl_particleVelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);

		vec[0] = cp*cy;
		vec[1] = cp*sy;
		vec[2] = -sp;

		d = sin(time + i) * 64.0;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_INSTANT;

		p->org[0] = org[0] + byteDirs[i][0]*d + vec[0]*16;
		p->org[1] = org[1] + byteDirs[i][1]*d + vec[1]*16;
		p->org[2] = org[2] + byteDirs[i][2]*d + vec[2]*16;
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = 0;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.24;
		p->color[1] = 0.82;
		p->color[2] = 0.10;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0 - Distance(org, p->org) / 90.0;
		p->alphaVel = 0;
		p->radius = 2.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}

	if (CL_PointContents(end, -1) & MASK_WATER){
		CL_BubbleTrail(start, end, 75, 10);
		return;
	}

	// Smoke
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 75;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.liteSmokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->radius = 25 + (5 * crand());
		p->radiusVel = 25 + (5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_HeatBeamTrail
 =================
*/
void CL_HeatBeamTrail (const vec3_t start, const vec3_t forward){

	particle_t	*p;
	vec3_t		move, vec, end;
	float		len, dist, step;
	vec3_t		dir;
	float		rot, s, c;

	if (!cl_particles->integerValue)
		return;

	len = VectorNormalize2(forward, vec);
	VectorMA(start, len, vec, end);

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	VectorNormalize(vec);

	dist = fmod(cl.time * 0.096, 32);
	len -= dist;
	VectorMA(move, dist, vec, move);

	dist = 32;
	VectorScale(vec, dist, vec);

	step = M_PI * 0.1;

	while (len > 0){
		len -= dist;

		for (rot = 0; rot < M_PI2; rot += step){
			p = CL_AllocParticle();
			if (!p)
				return;

			s = sin(rot) * 0.5;
			c = cos(rot) * 0.5;

			dir[0] = cl.renderView.viewAxis[1][0] * c + cl.renderView.viewAxis[2][0] * s;
			dir[1] = cl.renderView.viewAxis[1][1] * c + cl.renderView.viewAxis[2][1] * s;
			dir[2] = cl.renderView.viewAxis[1][2] * c + cl.renderView.viewAxis[2][2] * s;

			p->material = cl.media.energyParticleMaterial;
			p->time = cl.time;
			p->flags = PARTICLE_INSTANT;

			p->org[0] = move[0] + dir[0] * 3;
			p->org[1] = move[1] + dir[1] * 3;
			p->org[2] = move[2] + dir[2] * 3;
			p->vel[0] = 0;
			p->vel[1] = 0;
			p->vel[2] = 0;
			p->accel[0] = 0;
			p->accel[1] = 0;
			p->accel[2] = 0;
			p->color[0] = 0.97;
			p->color[1] = 0.46;
			p->color[2] = 0.14;
			p->colorVel[0] = 0;
			p->colorVel[1] = 0;
			p->colorVel[2] = 0;
			p->alpha = 0.5;
			p->alphaVel = 0;
			p->radius = 1.5;
			p->radiusVel = 0;
			p->length = 1;
			p->lengthVel = 0;
			p->rotation = 0;
		}

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_TrackerTrail
 =================
*/
void CL_TrackerTrail (const vec3_t start, const vec3_t end){

	particle_t	*p;
	vec3_t		move, vec;
	vec3_t		angles, forward, up;
	float		len, dist, c;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	VectorToAngles(vec, angles);
	AngleVectors(angles, forward, NULL, up);

	dist = 2.5;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		c = 8 * cos(DotProduct(move, forward));

		p->material = cl.media.trackerParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + up[0] * c;
		p->org[1] = move[1] + up[1] * c;
		p->org[2] = move[2] + up[2] * c;
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = 5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -2.0;
		p->radius = 2.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_TagTrail
 =================
*/
void CL_TagTrail (const vec3_t start, const vec3_t end){

	particle_t	*p;
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 5;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand() * 16;
		p->org[1] = move[1] + crand() * 16;
		p->org[2] = move[2] + crand() * 16;
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1.00;
		p->color[1] = 1.00;
		p->color[2] = 0.15;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.8 + frand() * 0.2);
		p->radius = 1.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_BubbleTrail
 =================
*/
void CL_BubbleTrail (const vec3_t start, const vec3_t end, float dist, float radius){

	particle_t	*p;
	vec3_t		move, vec;
	float		len;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.bubbleParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_UNDERWATER;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1;
		p->color[1] = 1;
		p->color[2] = 1;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->radius = radius + ((radius * 0.5) * crand());
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_FlagTrail
 =================
*/
void CL_FlagTrail (const vec3_t start, const vec3_t end, float r, float g, float b){

	particle_t	*p;
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 5;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.glowParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand() * 16;
		p->org[1] = move[1] + crand() * 16;
		p->org[2] = move[2] + crand() * 16;
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.8 + frand() * 0.2);
		p->radius = 2.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_BlasterParticles
 =================
*/
void CL_BlasterParticles (const vec3_t org, const vec3_t dir, float r, float g, float b){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	// Sparks
	for (i = 0; i < 40; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_BOUNCE | PARTICLE_FRICTION;

		p->org[0] = org[0] + dir[0] * 3 + crand();
		p->org[1] = org[1] + dir[1] * 3 + crand();
		p->org[2] = org[2] + dir[2] * 3 + crand();
		p->vel[0] = dir[0] * 25 + crand() * 20;
		p->vel[1] = dir[1] * 25 + crand() * 20;
		p->vel[2] = dir[2] * 50 + crand() * 20;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -120 + (40 * crand());
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -0.25 / (0.3 + frand() * 0.2);
		p->radius = 2.4 + (1.2 * crand());
		p->radiusVel = -1.2 + (0.6 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.7;

		VectorCopy(p->org, p->lastOrg);
	}

	if (CL_PointContents(org, -1) & MASK_WATER)
		return;

	// Steam
	for (i = 0; i < 3; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.steamParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + dir[0] * 5 + crand();
		p->org[1] = org[1] + dir[1] * 5 + crand();
		p->org[2] = org[2] + dir[2] * 5 + crand();
		p->vel[0] = crand() * 2.5;
		p->vel[1] = crand() * 2.5;
		p->vel[2] = crand() * 2.5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->radius = 3 + (1.5 * crand());
		p->radiusVel = 5 + (2.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_BulletParticles
 =================
*/
void CL_BulletParticles (const vec3_t org, const vec3_t dir){

	particle_t	*p;
	int			i, count;

	if (!cl_particles->integerValue)
		return;

	count = 3 + (rand() % 5);

	if (CL_PointContents(org, -1) & MASK_WATER){
		CL_BubbleParticles(org, count, 0);
		return;
	}

	// Sparks
	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.sparkParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_BOUNCE | PARTICLE_FRICTION | PARTICLE_STRETCH;

		p->org[0] = org[0] + dir[0] * 2 + crand();
		p->org[1] = org[1] + dir[1] * 2 + crand();
		p->org[2] = org[2] + dir[2] * 2 + crand();
		p->vel[0] = dir[0] * 180 + crand() * 60;
		p->vel[1] = dir[1] * 180 + crand() * 60;
		p->vel[2] = dir[2] * 180 + crand() * 60;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -120 + (60 * crand());
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -8.0;
		p->radius = 0.4 + (0.2 * crand());
		p->radiusVel = 0;
		p->length = 8 + (4 * crand());
		p->lengthVel = 8 + (4 * crand());
		p->rotation = 0;
		p->bounceFactor = 0.2;

		VectorCopy(p->org, p->lastOrg);
	}

	// Smoke
	for (i = 0; i < 3; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + dir[0] * 5 + crand();
		p->org[1] = org[1] + dir[1] * 5 + crand();
		p->org[2] = org[2] + dir[2] * 5 + crand();
		p->vel[0] = crand() * 2.5;
		p->vel[1] = crand() * 2.5;
		p->vel[2] = crand() * 2.5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.4;
		p->color[1] = 0.4;
		p->color[2] = 0.4;
		p->colorVel[0] = 0.2;
		p->colorVel[1] = 0.2;
		p->colorVel[2] = 0.2;
		p->alpha = 0.5;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->radius = 3 + (1.5 * crand());
		p->radiusVel = 5 + (2.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_ExplosionParticles
 =================
*/
void CL_ExplosionParticles (const vec3_t org){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	// Sparks
	for (i = 0; i < 384; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.sparkParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_BOUNCE | PARTICLE_FRICTION | PARTICLE_STRETCH;

		p->org[0] = org[0] + ((rand() % 32) - 16);
		p->org[1] = org[1] + ((rand() % 32) - 16);
		p->org[2] = org[2] + ((rand() % 32) - 16);
		p->vel[0] = (rand() % 512) - 256;
		p->vel[1] = (rand() % 512) - 256;
		p->vel[2] = (rand() % 512) - 256;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -60 + (30 * crand());
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -3.0;
		p->radius = 0.5 + (0.2 * crand());
		p->radiusVel = 0;
		p->length = 8 + (4 * crand());
		p->lengthVel = 8 + (4 * crand());
		p->rotation = 0;
		p->bounceFactor = 0.2;

		VectorCopy(p->org, p->lastOrg);
	}

	// Smoke
	for (i = 0; i < 5; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + crand() * 10;
		p->org[1] = org[1] + crand() * 10;
		p->org[2] = org[2] + crand() * 10;
		p->vel[0] = crand() * 10;
		p->vel[1] = crand() * 10;
		p->vel[2] = crand() * 10 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->colorVel[0] = 0.75;
		p->colorVel[1] = 0.75;
		p->colorVel[2] = 0.75;
		p->alpha = 0.5;
		p->alphaVel = -(0.1 + frand() * 0.1);
		p->radius = 30 + (15 * crand());
		p->radiusVel = 15 + (7.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_BFGExplosionParticles
 =================
*/
void CL_BFGExplosionParticles (const vec3_t org){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	// Particles
	for (i = 0; i < 384; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_BOUNCE | PARTICLE_FRICTION;

		p->org[0] = org[0] + ((rand() % 32) - 16);
		p->org[1] = org[1] + ((rand() % 32) - 16);
		p->org[2] = org[2] + ((rand() % 32) - 16);
		p->vel[0] = (rand() % 384) - 192;
		p->vel[1] = (rand() % 384) - 192;
		p->vel[2] = (rand() % 384) - 192;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -40 + (10 * crand());
		p->color[0] = 0.24;
		p->color[1] = 0.82;
		p->color[2] = 0.10;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -0.8 / (0.5 + frand() * 0.3);
		p->radius = 2.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.7;

		VectorCopy(p->org, p->lastOrg);
	}

	if (CL_PointContents(org, -1) & MASK_WATER)
		return;

	// Smoke
	for (i = 0; i < 5; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.liteSmokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + crand() * 10;
		p->org[1] = org[1] + crand() * 10;
		p->org[2] = org[2] + crand() * 10;
		p->vel[0] = crand() * 10;
		p->vel[1] = crand() * 10;
		p->vel[2] = crand() * 10 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -(0.1 + frand() * 0.1);
		p->radius = 50 + (25 * crand());
		p->radiusVel = 15 + (7.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_TrackerExplosionParticles
 =================
*/
void CL_TrackerExplosionParticles (const vec3_t org){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < 384; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.trackerParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_BOUNCE | PARTICLE_FRICTION;

		p->org[0] = org[0] + ((rand() % 32) - 16);
		p->org[1] = org[1] + ((rand() % 32) - 16);
		p->org[2] = org[2] + ((rand() % 32) - 16);
		p->vel[0] = (rand() % 256) - 128;
		p->vel[1] = (rand() % 256) - 128;
		p->vel[2] = (rand() % 256) - 128;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -40 + (10 * crand());
		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -0.4 / (0.6 + frand() * 0.2);
		p->radius = 2.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.7;

		VectorCopy(p->org, p->lastOrg);
	}
}

/*
 =================
 CL_SmokePuffParticles
 =================
*/
void CL_SmokePuffParticles (const vec3_t org, float radius, int count){

	particle_t	*p;
	int			i;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + crand();
		p->org[1] = org[1] + crand();
		p->org[2] = org[2] + crand();
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.4;
		p->color[1] = 0.4;
		p->color[2] = 0.4;
		p->colorVel[0] = 0.75;
		p->colorVel[1] = 0.75;
		p->colorVel[2] = 0.75;
		p->alpha = 0.5;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->radius = radius + ((radius * 0.5) * crand());
		p->radiusVel = (radius * 2) + ((radius * 4) * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_BubbleParticles
 =================
*/
void CL_BubbleParticles (const vec3_t org, int count, float magnitude){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.bubbleParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_UNDERWATER;

		p->org[0] = org[0] + (magnitude * crand());
		p->org[1] = org[1] + (magnitude * crand());
		p->org[2] = org[2] + (magnitude * crand());
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1;
		p->color[1] = 1;
		p->color[2] = 1;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->radius = 1 + (0.5 * crand());
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_SparkParticles
 =================
*/
void CL_SparkParticles (const vec3_t org, const vec3_t dir, int count){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(org, -1) & MASK_WATER){
		CL_BubbleParticles(org, count, 0);
		return;
	}

	// Sparks
	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.sparkParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_BOUNCE | PARTICLE_FRICTION | PARTICLE_STRETCH;

		p->org[0] = org[0] + dir[0] * 2 + crand();
		p->org[1] = org[1] + dir[1] * 2 + crand();
		p->org[2] = org[2] + dir[2] * 2 + crand();
		p->vel[0] = dir[0] * 180 + crand() * 60;
		p->vel[1] = dir[1] * 180 + crand() * 60;
		p->vel[2] = dir[2] * 180 + crand() * 60;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -120 + (60 * crand());
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.5;
		p->radius = 0.4 + (0.2 * crand());
		p->radiusVel = 0;
		p->length = 8 + (4 * crand());
		p->lengthVel = 8 + (4 * crand());
		p->rotation = 0;
		p->bounceFactor = 0.2;

		VectorCopy(p->org, p->lastOrg);
	}

	// Smoke
	for (i = 0; i < 3; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + dir[0] * 5 + crand();
		p->org[1] = org[1] + dir[1] * 5 + crand();
		p->org[2] = org[2] + dir[2] * 5 + crand();
		p->vel[0] = crand() * 2.5;
		p->vel[1] = crand() * 2.5;
		p->vel[2] = crand() * 2.5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.4;
		p->color[1] = 0.4;
		p->color[2] = 0.4;
		p->colorVel[0] = 0.2;
		p->colorVel[1] = 0.2;
		p->colorVel[2] = 0.2;
		p->alpha = 0.5;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->radius = 3 + (1.5 * crand());
		p->radiusVel = 5 + (2.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_DamageSparkParticles
 =================
*/
void CL_DamageSparkParticles (const vec3_t org, const vec3_t dir, int count, int color){

	particle_t	*p;
	int			i, index;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(org, -1) & MASK_WATER){
		CL_BubbleParticles(org, count, 2.5);
		return;
	}

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		index = (color + (rand() & 7)) & 0xFF;

		p->material = cl.media.impactSparkParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_BOUNCE | PARTICLE_FRICTION;

		p->org[0] = org[0] + dir[0] * 2 + crand();
		p->org[1] = org[1] + dir[1] * 2 + crand();
		p->org[2] = org[2] + dir[2] * 2 + crand();
		p->vel[0] = crand() * 60;
		p->vel[1] = crand() * 60;
		p->vel[2] = crand() * 60;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -180 + (30 * crand());
		p->color[0] = cl_particlePalette[index][0];
		p->color[1] = cl_particlePalette[index][1];
		p->color[2] = cl_particlePalette[index][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -(0.75 + frand());
		p->radius = 0.4 + (0.2 * crand());
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.6;
	}
}

/*
 =================
 CL_LaserSparkParticles
 =================
*/
void CL_LaserSparkParticles (const vec3_t org, const vec3_t dir, int count, int color){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(org, -1) & MASK_WATER){
		CL_BubbleParticles(org, count, 2.5);
		return;
	}

	color &= 0xFF;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.impactSparkParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_BOUNCE | PARTICLE_FRICTION;

		p->org[0] = org[0] + dir[0] * 2 + crand();
		p->org[1] = org[1] + dir[1] * 2 + crand();
		p->org[2] = org[2] + dir[2] * 2 + crand();
		p->vel[0] = dir[0] * 180 + crand() * 60;
		p->vel[1] = dir[1] * 180 + crand() * 60;
		p->vel[2] = dir[2] * 180 + crand() * 60;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -120 + (60 * crand());
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.5;
		p->radius = 0.4 + (0.2 * crand());
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.2;

		VectorCopy(p->org, p->lastOrg);
	}
}

/*
 =================
 CL_SplashParticles
 =================
*/
void CL_SplashParticles (const vec3_t org, const vec3_t dir, int count, float magnitude, float spread){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.dropletParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + dir[0] * 3 + crand() * magnitude;
		p->org[1] = org[1] + dir[1] * 3 + crand() * magnitude;
		p->org[2] = org[2] + dir[2] * 3 + crand() * magnitude;
		p->vel[0] = dir[0] * spread + crand() * spread;
		p->vel[1] = dir[1] * spread + crand() * spread;
		p->vel[2] = dir[2] * spread + crand() * spread + 4 * spread;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -150 + (25 * crand());
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -(0.25 + frand() * 0.25);
		p->radius = 0.5 + (0.25 * crand());
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_LavaSteamParticles
 =================
*/
void CL_LavaSteamParticles (const vec3_t org, const vec3_t dir, int count){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.steamParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + dir[0] * 2 + crand();
		p->org[1] = org[1] + dir[1] * 2 + crand();
		p->org[2] = org[2] + dir[2] * 2 + crand();
		p->vel[0] = crand() * 2.5;
		p->vel[1] = crand() * 2.5;
		p->vel[2] = crand() * 2.5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.82;
		p->color[1] = 0.34;
		p->color[2] = 0.00;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->radius = 1.5 + (0.75 * crand());
		p->radiusVel = 5 + (2.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_FlyParticles
 =================
*/
void CL_FlyParticles (const vec3_t org, int count){

	particle_t	*p;
	vec3_t		vec;
	float		d, time;
	float		angle, sy, cy, sp, cp;
	int			i;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(org, -1) & MASK_WATER)
		return;

	time = MS2SEC(cl.time);

	for (i = 0; i < count ; i += 2){
		p = CL_AllocParticle();
		if (!p)
			return;

		angle = time * cl_particleVelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = time * cl_particleVelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);

		vec[0] = cp*cy;
		vec[1] = cp*sy;
		vec[2] = -sp;

		d = sin(time + i) * 64.0;

		p->material = cl.media.flyParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_INSTANT;

		p->org[0] = org[0] + byteDirs[i][0]*d + vec[0]*16;
		p->org[1] = org[1] + byteDirs[i][1]*d + vec[1]*16;
		p->org[2] = org[2] + byteDirs[i][2]*d + vec[2]*16;
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = 0;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = 0;
		p->radius = 1.0;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_TeleportParticles
 =================
*/
void CL_TeleportParticles (const vec3_t org){

	particle_t	*p;
	vec3_t		dir;
	float		vel, color;
	int			x, y, z;

	if (!cl_particles->integerValue)
		return;

	for (x = -16; x <= 16; x += 4){
		for (y = -16; y <= 16; y += 4){
			for (z = -16; z <= 32; z += 4){
				p = CL_AllocParticle();
				if (!p)
					return;

				VectorSet(dir, y*8, x*8, z*8);
				VectorNormalizeFast(dir);

				vel = 50 + (rand() & 63);

				color = 0.1 + (0.2 * frand());

				p->material = cl.media.glowParticleMaterial;
				p->time = cl.time;
				p->flags = 0;

				p->org[0] = org[0] + x + (rand() & 3);
				p->org[1] = org[1] + y + (rand() & 3);
				p->org[2] = org[2] + z + (rand() & 3);
				p->vel[0] = dir[0] * vel;
				p->vel[1] = dir[1] * vel;
				p->vel[2] = dir[2] * vel;
				p->accel[0] = 0;
				p->accel[1] = 0;
				p->accel[2] = -40;
				p->color[0] = color;
				p->color[1] = color;
				p->color[2] = color;
				p->colorVel[0] = 0;
				p->colorVel[1] = 0;
				p->colorVel[2] = 0;
				p->alpha = 1.0;
				p->alphaVel = -1.0 / (0.3 + (rand() & 7) * 0.02);
				p->radius = 2;
				p->radiusVel = 0;
				p->length = 1;
				p->lengthVel = 0;
				p->rotation = 0;
			}
		}
	}
}

/*
 =================
 CL_BigTeleportParticles
 =================
*/
void CL_BigTeleportParticles (const vec3_t org){

	particle_t	*p;
	float		d, angle, s, c, color;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < 4096; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		d = rand() & 31;

		angle = M_PI2 * (rand() & 1023) / 1023.0;
		s = sin(angle);
		c = cos(angle);

		color = 0.1 + (0.2 * frand());

		p->material = cl.media.glowParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + c * d;
		p->org[1] = org[1] + s * d;
		p->org[2] = org[2] + 8 + (rand() % 90);
		p->vel[0] = c * (70 + (rand() & 63));
		p->vel[1] = s * (70 + (rand() & 63));
		p->vel[2] = -100 + (rand() & 31);
		p->accel[0] = -100 * c;
		p->accel[1] = -100 * s;
		p->accel[2] = 160;
		p->color[0] = color;
		p->color[1] = color;
		p->color[2] = color;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -0.3 / (0.5 + frand() * 0.3);
		p->radius = 2;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_TeleporterParticles
 =================
*/
void CL_TeleporterParticles (const vec3_t org){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < 8; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] - 16 + (rand() & 31);
		p->org[1] = org[1] - 16 + (rand() & 31);
		p->org[2] = org[2] - 8 + (rand() & 7);
		p->vel[0] = crand() * 14;
		p->vel[1] = crand() * 14;
		p->vel[2] = 80 + (rand() & 7);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -40;
		p->color[0] = 0.97;
		p->color[1] = 0.46;
		p->color[2] = 0.14;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -0.5;
		p->radius = 3 + (1.5 * crand());
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_TrapParticles
 =================
*/
void CL_TrapParticles (const vec3_t org){

	particle_t	*p;
	vec3_t		start, end, move, vec, dir;
	float		len, dist, vel;
	int			x, y, z;

	if (!cl_particles->integerValue)
		return;

	VectorSet(start, org[0], org[1], org[2] + 18);
	VectorSet(end, org[0], org[1], org[2] + 82);

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 5;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = 15 * crand();
		p->vel[1] = 15 * crand();
		p->vel[2] = 15 * crand();
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 40;
		p->color[0] = 0.97;
		p->color[1] = 0.46;
		p->color[2] = 0.14;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.3 + frand() * 0.2);
		p->radius = 3 + (1.5 * crand());
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}

	for (x = -2; x <= 2; x += 4){
		for (y = -2; y <= 2; y += 4){
			for (z = -2; z <= 4; z += 4){
				p = CL_AllocParticle();
				if (!p)
					return;

				VectorSet(dir, y*8, x*8, z*8);
				VectorNormalizeFast(dir);

				vel = 50 + (rand() & 63);

				p->material = cl.media.energyParticleMaterial;
				p->time = cl.time;
				p->flags = 0;

				p->org[0] = org[0] + x + ((rand() & 23) * crand());
				p->org[1] = org[1] + y + ((rand() & 23) * crand());
				p->org[2] = org[2] + z + ((rand() & 23) * crand()) + 32;
				p->vel[0] = dir[0] * vel;
				p->vel[1] = dir[1] * vel;
				p->vel[2] = dir[2] * vel;
				p->accel[0] = 0;
				p->accel[1] = 0;
				p->accel[2] = -40;
				p->color[0] = 0.96;
				p->color[1] = 0.46;
				p->color[2] = 0.14;
				p->colorVel[0] = 0;
				p->colorVel[1] = 0;
				p->colorVel[2] = 0;
				p->alpha = 1.0;
				p->alphaVel = -1.0 / (0.3 + (rand() & 7) * 0.02);
				p->radius = 3 + (1.5 * crand());
				p->radiusVel = 0;
				p->length = 1;
				p->lengthVel = 0;
				p->rotation = 0;
			}
		}
	}
}

/*
 =================
 CL_LogParticles
 =================
*/
void CL_LogParticles (const vec3_t org, float r, float g, float b){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < 512; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.glowParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + crand() * 20;
		p->org[1] = org[1] + crand() * 20;
		p->org[2] = org[2] + crand() * 20;
		p->vel[0] = crand() * 20;
		p->vel[1] = crand() * 20;
		p->vel[2] = crand() * 20;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (1.0 + frand() * 0.3);
		p->radius = 2;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_ItemRespawnParticles
 =================
*/
void CL_ItemRespawnParticles (const vec3_t org){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < 64; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.glowParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + crand() * 8;
		p->org[1] = org[1] + crand() * 8;
		p->org[2] = org[2] + crand() * 8;
		p->vel[0] = crand() * 8;
		p->vel[1] = crand() * 8;
		p->vel[2] = crand() * 8;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.0;
		p->color[1] = 0.3;
		p->color[2] = 0.5;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (1.0 + frand() * 0.3);
		p->radius = 2;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_TrackerShellParticles
 =================
*/
void CL_TrackerShellParticles (const vec3_t org){

	particle_t	*p;
	vec3_t		vec;
	float		d, time;
	float		angle, sy, cy, sp, cp;
	int			i;

	if (!cl_particles->integerValue)
		return;

	time = MS2SEC(cl.time);

	for (i = 0; i < NUM_VERTEX_NORMALS; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		angle = time * cl_particleVelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = time * cl_particleVelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);

		vec[0] = cp*cy;
		vec[1] = cp*sy;
		vec[2] = -sp;

		d = sin(time + i) * 64.0;

		p->material = cl.media.trackerParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_INSTANT;

		p->org[0] = org[0] + byteDirs[i][0]*d + vec[0]*16;
		p->org[1] = org[1] + byteDirs[i][1]*d + vec[1]*16;
		p->org[2] = org[2] + byteDirs[i][2]*d + vec[2]*16;
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = 0;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = 0;
		p->radius = 2.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_NukeSmokeParticles
 =================
*/
void CL_NukeSmokeParticles (const vec3_t org){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < 5; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + crand() * 20;
		p->org[1] = org[1] + crand() * 20;
		p->org[2] = org[2] + crand() * 20;
		p->vel[0] = crand() * 20;
		p->vel[1] = crand() * 20;
		p->vel[2] = crand() * 20 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->colorVel[0] = 0.75;
		p->colorVel[1] = 0.75;
		p->colorVel[2] = 0.75;
		p->alpha = 0.5;
		p->alphaVel = -(0.1 + frand() * 0.1);
		p->radius = 60 + (30 * crand());
		p->radiusVel = 30 + (15 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_WeldingSparkParticles
 =================
*/
void CL_WeldingSparkParticles (const vec3_t org, const vec3_t dir, int count, int color){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	color &= 0xFF;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_BOUNCE | PARTICLE_FRICTION;

		p->org[0] = org[0] + dir[0] * 5 + (5 * crand());
		p->org[1] = org[1] + dir[1] * 5 + (5 * crand());
		p->org[2] = org[2] + dir[2] * 5 + (5 * crand());
		p->vel[0] = crand() * 20;
		p->vel[1] = crand() * 20;
		p->vel[2] = crand() * 20;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -40 + (5 * crand());
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.5 + frand() * 0.3);
		p->radius = 1.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.7;
	}
}

/*
 =================
 CL_TunnelSparkParticles
 =================
*/
void CL_TunnelSparkParticles (const vec3_t org, const vec3_t dir, int count, int color){

	particle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	color &= 0xFF;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_BOUNCE | PARTICLE_FRICTION;

		p->org[0] = org[0] + dir[0] * 5 + (5 * crand());
		p->org[1] = org[1] + dir[1] * 5 + (5 * crand());
		p->org[2] = org[2] + dir[2] * 5 + (5 * crand());
		p->vel[0] = crand() * 20;
		p->vel[1] = crand() * 20;
		p->vel[2] = crand() * 20;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 40 + (5 * crand());
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.5 + frand() * 0.3);
		p->radius = 1.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.7;
	}
}

/*
 =================
 CL_ForceWallParticles
 =================
*/
void CL_ForceWallParticles (const vec3_t start, const vec3_t end, int color){

	particle_t	*p;
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 4;
	VectorScale(vec, dist, vec);

	color &= 0xFF;

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand() * 3;
		p->org[1] = move[1] + crand() * 3;
		p->org[2] = move[2] + crand() * 3;
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = -40 - (crand() * 10);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (3.0 + frand() * 0.5);
		p->radius = 1.5;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_SteamParticles
 =================
*/
void CL_SteamParticles (const vec3_t org, const vec3_t dir, int count, int color, float magnitude){

	particle_t	*p;
	vec3_t		r, u;
	float		rd, ud;
	int			index;
	int			i;

	if (!cl_particles->integerValue)
		return;

	MakeNormalVectors(dir, r, u);

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		rd = crand() * magnitude / 3;
		ud = crand() * magnitude / 3;

		index = (color + (rand() & 7)) & 0xFF;

		p->material = cl.media.steamParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + magnitude * 0.1 * crand();
		p->org[1] = org[1] + magnitude * 0.1 * crand();
		p->org[2] = org[2] + magnitude * 0.1 * crand();
		p->vel[0] = dir[0] * magnitude + r[0] * rd + u[0] * ud;
		p->vel[1] = dir[1] * magnitude + r[1] * rd + u[1] * ud;
		p->vel[2] = dir[2] * magnitude + r[2] * rd + u[2] * ud;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -20;
		p->color[0] = cl_particlePalette[index][0];
		p->color[1] = cl_particlePalette[index][1];
		p->color[2] = cl_particlePalette[index][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.5 + frand() * 0.3);
		p->radius = 2;
		p->radiusVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}
