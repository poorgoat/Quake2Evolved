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


// r_batch.c -- geometry batching for back-end


#include "r_local.h"


/*
 =================
 RB_CheckOverflow
 =================
*/
void RB_CheckOverflow (int numIndices, int numVertices, int maxIndices, int maxVertices){

	if (backEnd.numIndices + numIndices <= maxIndices && backEnd.numVertices + numVertices <= maxVertices)
		return;

	RB_EndBatch();

	if (numIndices > maxIndices)
		Com_Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%i > %i)", numIndices, maxIndices);
	if (numVertices > maxVertices)
		Com_Error(ERR_DROP, "RB_CheckOverflow: vertices > MAX (%i > %i)", numVertices, maxVertices);

	RB_BeginBatch(backEnd.material, backEnd.entity, backEnd.stencilShadow, backEnd.shadowCaps);
}

/*
 =================
 RB_BeginBatch
 =================
*/
void RB_BeginBatch (material_t *material, renderEntity_t *entity, qboolean stencilShadow, qboolean shadowCaps){

	// Set the batch state
	backEnd.material = material;
	backEnd.entity = entity;

	backEnd.stencilShadow = stencilShadow;
	backEnd.shadowCaps = shadowCaps;
}

/*
 =================
 RB_EndBatch
 =================
*/
void RB_EndBatch (void){

	if (!backEnd.numIndices || !backEnd.numVertices)
		return;

	// Render the batch
	backEnd.renderBatch();

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	// Clear the arrays
	backEnd.numIndices = 0;
	backEnd.numVertices = 0;
}


/*
 =======================================================================

 STANDARD RENDERING

 =======================================================================
*/


/*
 =================
 RB_BatchSurface
 =================
*/
static void RB_BatchSurface (meshData_t *data){

	surface_t		*surface = data;
	surfTriangle_t	*triangle;
	surfVertex_t	*vertex;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i;

	// Check for overflow
	RB_CheckOverflow(surface->numTriangles * 3, surface->numVertices, MAX_INDICES, MAX_VERTICES);

	// Set up indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
		indices[0] = backEnd.numVertices + triangle->index[0];
		indices[1] = backEnd.numVertices + triangle->index[1];
		indices[2] = backEnd.numVertices + triangle->index[2];

		indices += 3;
	}

	backEnd.numIndices += surface->numTriangles * 3;

	// Set up vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		vertices->xyz[0] = vertex->xyz[0];
		vertices->xyz[1] = vertex->xyz[1];
		vertices->xyz[2] = vertex->xyz[2];
		vertices->normal[0] = vertex->normal[0];
		vertices->normal[1] = vertex->normal[1];
		vertices->normal[2] = vertex->normal[2];
		vertices->tangents[0][0] = vertex->tangents[0][0];
		vertices->tangents[0][1] = vertex->tangents[0][1];
		vertices->tangents[0][2] = vertex->tangents[0][2];
		vertices->tangents[1][0] = vertex->tangents[1][0];
		vertices->tangents[1][1] = vertex->tangents[1][1];
		vertices->tangents[1][2] = vertex->tangents[1][2];
		vertices->st[0] = vertex->st[0];
		vertices->st[1] = vertex->st[1];
		vertices->color[0] = vertex->color[0];
		vertices->color[1] = vertex->color[1];
		vertices->color[2] = vertex->color[2];
		vertices->color[3] = vertex->color[3];

		vertices++;
	}

	backEnd.numVertices += surface->numVertices;
}

/*
 =================
 RB_BatchDecoration
 =================
*/
static void RB_BatchDecoration (meshData_t *data){

	decSurface_t	*surface = data;
	surfTriangle_t	*triangle;
	surfVertex_t	*vertex;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i;

	// Check for overflow
	RB_CheckOverflow(surface->numTriangles * 3, surface->numVertices, MAX_INDICES, MAX_VERTICES);

	// Set up indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
		indices[0] = backEnd.numVertices + triangle->index[0];
		indices[1] = backEnd.numVertices + triangle->index[1];
		indices[2] = backEnd.numVertices + triangle->index[2];

		indices += 3;
	}

	backEnd.numIndices += surface->numTriangles * 3;

	// Set up vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		vertices->xyz[0] = vertex->xyz[0];
		vertices->xyz[1] = vertex->xyz[1];
		vertices->xyz[2] = vertex->xyz[2];
		vertices->normal[0] = vertex->normal[0];
		vertices->normal[1] = vertex->normal[1];
		vertices->normal[2] = vertex->normal[2];
		vertices->tangents[0][0] = vertex->tangents[0][0];
		vertices->tangents[0][1] = vertex->tangents[0][1];
		vertices->tangents[0][2] = vertex->tangents[0][2];
		vertices->tangents[1][0] = vertex->tangents[1][0];
		vertices->tangents[1][1] = vertex->tangents[1][1];
		vertices->tangents[1][2] = vertex->tangents[1][2];
		vertices->st[0] = vertex->st[0];
		vertices->st[1] = vertex->st[1];
		vertices->color[0] = vertex->color[0];
		vertices->color[1] = vertex->color[1];
		vertices->color[2] = vertex->color[2];
		vertices->color[3] = vertex->color[3];

		vertices++;
	}

	backEnd.numVertices += surface->numVertices;
}

/*
 =================
 RB_BatchAliasModel
 =================
*/
static void RB_BatchAliasModel (meshData_t *data){

	mdlSurface_t	*surface = data;
	mdlTriangle_t	*triangle;
	mdlSt_t			*st;
	mdlXyzNormal_t	*curXyzNormal, *oldXyzNormal;
	float			backLerp;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i;

	// Interpolate frames
	curXyzNormal = surface->xyzNormals + surface->numVertices * backEnd.entity->frame;
	oldXyzNormal = surface->xyzNormals + surface->numVertices * backEnd.entity->oldFrame;

	if (backEnd.entity->frame == backEnd.entity->oldFrame)
		backLerp = 0.0;
	else
		backLerp = backEnd.entity->backLerp;

	// Check for overflow
	RB_CheckOverflow(surface->numTriangles * 3, surface->numVertices, MAX_INDICES, MAX_VERTICES);

	// Set up indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
		indices[0] = backEnd.numVertices + triangle->index[0];
		indices[1] = backEnd.numVertices + triangle->index[1];
		indices[2] = backEnd.numVertices + triangle->index[2];

		indices += 3;
	}

	backEnd.numIndices += surface->numTriangles * 3;

	// Set up vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	if (backLerp == 0.0){
		// Optimized case
		for (i = 0, st = surface->st; i < surface->numVertices; i++, curXyzNormal++, st++){
			vertices->xyz[0] = curXyzNormal->xyz[0];
			vertices->xyz[1] = curXyzNormal->xyz[1];
			vertices->xyz[2] = curXyzNormal->xyz[2];
			vertices->normal[0] = curXyzNormal->normal[0];
			vertices->normal[1] = curXyzNormal->normal[1];
			vertices->normal[2] = curXyzNormal->normal[2];
			vertices->tangents[0][0] = curXyzNormal->tangents[0][0];
			vertices->tangents[0][1] = curXyzNormal->tangents[0][1];
			vertices->tangents[0][2] = curXyzNormal->tangents[0][2];
			vertices->tangents[1][0] = curXyzNormal->tangents[1][0];
			vertices->tangents[1][1] = curXyzNormal->tangents[1][1];
			vertices->tangents[1][2] = curXyzNormal->tangents[1][2];
			vertices->st[0] = st->st[0];
			vertices->st[1] = st->st[1];
			vertices->color[0] = 255;
			vertices->color[1] = 255;
			vertices->color[2] = 255;
			vertices->color[3] = 255;

			vertices++;
		}
	}
	else {
		// General case
		for (i = 0, st = surface->st; i < surface->numVertices; i++, curXyzNormal++, oldXyzNormal++, st++){
			vertices->xyz[0] = curXyzNormal->xyz[0] + (oldXyzNormal->xyz[0] - curXyzNormal->xyz[0]) * backLerp;
			vertices->xyz[1] = curXyzNormal->xyz[1] + (oldXyzNormal->xyz[1] - curXyzNormal->xyz[1]) * backLerp;
			vertices->xyz[2] = curXyzNormal->xyz[2] + (oldXyzNormal->xyz[2] - curXyzNormal->xyz[2]) * backLerp;
			vertices->normal[0] = curXyzNormal->normal[0] + (oldXyzNormal->normal[0] - curXyzNormal->normal[0]) * backLerp;
			vertices->normal[1] = curXyzNormal->normal[1] + (oldXyzNormal->normal[1] - curXyzNormal->normal[1]) * backLerp;
			vertices->normal[2] = curXyzNormal->normal[2] + (oldXyzNormal->normal[2] - curXyzNormal->normal[2]) * backLerp;
			vertices->tangents[0][0] = curXyzNormal->tangents[0][0] + (oldXyzNormal->tangents[0][0] - curXyzNormal->tangents[0][0]) * backLerp;
			vertices->tangents[0][1] = curXyzNormal->tangents[0][1] + (oldXyzNormal->tangents[0][1] - curXyzNormal->tangents[0][1]) * backLerp;
			vertices->tangents[0][2] = curXyzNormal->tangents[0][2] + (oldXyzNormal->tangents[0][2] - curXyzNormal->tangents[0][2]) * backLerp;
			vertices->tangents[1][0] = curXyzNormal->tangents[1][0] + (oldXyzNormal->tangents[1][0] - curXyzNormal->tangents[1][0]) * backLerp;
			vertices->tangents[1][1] = curXyzNormal->tangents[1][1] + (oldXyzNormal->tangents[1][1] - curXyzNormal->tangents[1][1]) * backLerp;
			vertices->tangents[1][2] = curXyzNormal->tangents[1][2] + (oldXyzNormal->tangents[1][2] - curXyzNormal->tangents[1][2]) * backLerp;
			vertices->st[0] = st->st[0];
			vertices->st[1] = st->st[1];
			vertices->color[0] = 255;
			vertices->color[1] = 255;
			vertices->color[2] = 255;
			vertices->color[3] = 255;

			VectorNormalizeFast(vertices->normal);
			VectorNormalizeFast(vertices->tangents[0]);
			VectorNormalizeFast(vertices->tangents[1]);

			vertices++;
		}
	}

	backEnd.numVertices += surface->numVertices;
}

/*
 =================
 RB_BatchSprite
 =================
*/
static void RB_BatchSprite (meshData_t *data){

	vec3_t			axis[3];
	float			rad, s, c;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i;

	// Calculate axes
	if (backEnd.entity->spriteRotation){
		rad = DEG2RAD(backEnd.entity->spriteRotation);
		s = sin(rad);
		c = cos(rad);

		if (backEnd.entity->spriteOriented){
			VectorCopy(backEnd.entity->axis[0], axis[0]);

			VectorScale(backEnd.entity->axis[1], c * backEnd.entity->spriteRadius, axis[1]);
			VectorMA(axis[1], -s * backEnd.entity->spriteRadius, backEnd.entity->axis[2], axis[1]);

			VectorScale(backEnd.entity->axis[2], c * backEnd.entity->spriteRadius, axis[2]);
			VectorMA(axis[2], s * backEnd.entity->spriteRadius, backEnd.entity->axis[1], axis[2]);
		}
		else {
			VectorNegate(backEnd.viewParms.axis[0], axis[0]);

			VectorScale(backEnd.viewParms.axis[1], c * backEnd.entity->spriteRadius, axis[1]);
			VectorMA(axis[1], -s * backEnd.entity->spriteRadius, backEnd.viewParms.axis[2], axis[1]);

			VectorScale(backEnd.viewParms.axis[2], c * backEnd.entity->spriteRadius, axis[2]);
			VectorMA(axis[2], s * backEnd.entity->spriteRadius, backEnd.viewParms.axis[1], axis[2]);
		}
	}
	else {
		if (backEnd.entity->spriteOriented){
			VectorCopy(backEnd.entity->axis[0], axis[0]);
			VectorScale(backEnd.entity->axis[1], backEnd.entity->spriteRadius, axis[1]);
			VectorScale(backEnd.entity->axis[2], backEnd.entity->spriteRadius, axis[2]);
		}
		else {
			VectorNegate(backEnd.viewParms.axis[0], axis[0]);
			VectorScale(backEnd.viewParms.axis[1], backEnd.entity->spriteRadius, axis[1]);
			VectorScale(backEnd.viewParms.axis[2], backEnd.entity->spriteRadius, axis[2]);
		}
	}

	// Check for overflow
	RB_CheckOverflow(6, 4, MAX_INDICES, MAX_VERTICES);

	// Set up indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 2; i < 4; i++){
		indices[0] = backEnd.numVertices + 0;
		indices[1] = backEnd.numVertices + i-1;
		indices[2] = backEnd.numVertices + i;

		indices += 3;
	}

	backEnd.numIndices += 6;

	// Set up vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	vertices[0].xyz[0] = backEnd.entity->origin[0] + axis[1][0] + axis[2][0];
	vertices[0].xyz[1] = backEnd.entity->origin[1] + axis[1][1] + axis[2][1];
	vertices[0].xyz[2] = backEnd.entity->origin[2] + axis[1][2] + axis[2][2];
	vertices[1].xyz[0] = backEnd.entity->origin[0] - axis[1][0] + axis[2][0];
	vertices[1].xyz[1] = backEnd.entity->origin[1] - axis[1][1] + axis[2][1];
	vertices[1].xyz[2] = backEnd.entity->origin[2] - axis[1][2] + axis[2][2];
	vertices[2].xyz[0] = backEnd.entity->origin[0] - axis[1][0] - axis[2][0];
	vertices[2].xyz[1] = backEnd.entity->origin[1] - axis[1][1] - axis[2][1];
	vertices[2].xyz[2] = backEnd.entity->origin[2] - axis[1][2] - axis[2][2];
	vertices[3].xyz[0] = backEnd.entity->origin[0] + axis[1][0] - axis[2][0];
	vertices[3].xyz[1] = backEnd.entity->origin[1] + axis[1][1] - axis[2][1];
	vertices[3].xyz[2] = backEnd.entity->origin[2] + axis[1][2] - axis[2][2];

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
		vertices->color[0] = 255;
		vertices->color[1] = 255;
		vertices->color[2] = 255;
		vertices->color[3] = 255;

		vertices++;
	}

	backEnd.numVertices += 4;
}

/*
 =================
 RB_BatchBeam
 =================
*/
static void RB_BatchBeam (meshData_t *data){

	vec3_t			axis[3];
	float			length;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i;

	// Find orientation vectors
	VectorSubtract(backEnd.viewParms.origin, backEnd.entity->origin, axis[0]);
	VectorSubtract(backEnd.entity->beamEnd, backEnd.entity->origin, axis[1]);

	CrossProduct(axis[0], axis[1], axis[2]);
	VectorNormalizeFast(axis[2]);

	// Find normal
	CrossProduct(axis[1], axis[2], axis[0]);
	VectorNormalizeFast(axis[0]);

	// Scale by radius
	VectorScale(axis[2], backEnd.entity->beamWidth * 0.5, axis[2]);

	// Find segment length
	length = VectorLength(axis[1]) / backEnd.entity->beamLength;

	// Check for overflow
	RB_CheckOverflow(6, 4, MAX_INDICES, MAX_VERTICES);

	// Set up indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 2; i < 4; i++){
		indices[0] = backEnd.numVertices + 0;
		indices[1] = backEnd.numVertices + i-1;
		indices[2] = backEnd.numVertices + i;

		indices += 3;
	}

	backEnd.numIndices += 6;

	// Set up vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	vertices[0].xyz[0] = backEnd.entity->origin[0] + axis[2][0];
	vertices[0].xyz[1] = backEnd.entity->origin[1] + axis[2][1];
	vertices[0].xyz[2] = backEnd.entity->origin[2] + axis[2][2];
	vertices[1].xyz[0] = backEnd.entity->beamEnd[0] + axis[2][0];
	vertices[1].xyz[1] = backEnd.entity->beamEnd[1] + axis[2][1];
	vertices[1].xyz[2] = backEnd.entity->beamEnd[2] + axis[2][2];
	vertices[2].xyz[0] = backEnd.entity->beamEnd[0] - axis[2][0];
	vertices[2].xyz[1] = backEnd.entity->beamEnd[1] - axis[2][1];
	vertices[2].xyz[2] = backEnd.entity->beamEnd[2] - axis[2][2];
	vertices[3].xyz[0] = backEnd.entity->origin[0] - axis[2][0];
	vertices[3].xyz[1] = backEnd.entity->origin[1] - axis[2][1];
	vertices[3].xyz[2] = backEnd.entity->origin[2] - axis[2][2];

	vertices[0].st[0] = 0.0;
	vertices[0].st[1] = 0.0;
	vertices[1].st[0] = length;
	vertices[1].st[1] = 0.0;
	vertices[2].st[0] = length;
	vertices[2].st[1] = 1.0;
	vertices[3].st[0] = 0.0;
	vertices[3].st[1] = 1.0;

	for (i = 0; i < 4; i++){
		vertices->normal[0] = axis[0][0];
		vertices->normal[1] = axis[0][1];
		vertices->normal[2] = axis[0][2];
		vertices->color[0] = 255;
		vertices->color[1] = 255;
		vertices->color[2] = 255;
		vertices->color[3] = 255;

		vertices++;
	}

	backEnd.numVertices += 4;
}

/*
 =================
 RB_BatchDecal
 =================
*/
static void RB_BatchDecal (meshData_t *data){

	decal_t			*decal = data;
	decalVertex_t	*vertex;
	decalInfo_t		*decalInfo = &backEnd.material->decalInfo;
	color_t			color;
	float			time, frac;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i;

	// Compute color, applying fading if needed
	time = backEnd.time - decal->time;

	if (time <= decalInfo->stayTime){
		color[0] = 255 * Clamp(decalInfo->startRGBA[0], 0.0, 1.0);
		color[1] = 255 * Clamp(decalInfo->startRGBA[1], 0.0, 1.0);
		color[2] = 255 * Clamp(decalInfo->startRGBA[2], 0.0, 1.0);
		color[3] = 255 * Clamp(decalInfo->startRGBA[3], 0.0, 1.0);
	}
	else {
		frac = (time - decalInfo->stayTime) * (1.0 / decalInfo->fadeTime);

		color[0] = 255 * Clamp(decalInfo->startRGBA[0] + (decalInfo->endRGBA[0] - decalInfo->startRGBA[0]) * frac, 0.0, 1.0);
		color[1] = 255 * Clamp(decalInfo->startRGBA[1] + (decalInfo->endRGBA[1] - decalInfo->startRGBA[1]) * frac, 0.0, 1.0);
		color[2] = 255 * Clamp(decalInfo->startRGBA[2] + (decalInfo->endRGBA[2] - decalInfo->startRGBA[2]) * frac, 0.0, 1.0);
		color[3] = 255 * Clamp(decalInfo->startRGBA[3] + (decalInfo->endRGBA[3] - decalInfo->startRGBA[3]) * frac, 0.0, 1.0);
	}

	// Check for overflow
	RB_CheckOverflow((decal->numVertices-2) * 3, decal->numVertices, MAX_INDICES, MAX_VERTICES);

	// Set up indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 2; i < decal->numVertices; i++){
		indices[0] = backEnd.numVertices + 0;
		indices[1] = backEnd.numVertices + i-1;
		indices[2] = backEnd.numVertices + i;

		indices += 3;
	}

	backEnd.numIndices += (decal->numVertices-2) * 3;

	// Set up vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	for (i = 0, vertex = decal->vertices; i < decal->numVertices; i++, vertex++){
		vertices->xyz[0] = vertex->xyz[0];
		vertices->xyz[1] = vertex->xyz[1];
		vertices->xyz[2] = vertex->xyz[2];
		vertices->normal[0] = vertex->normal[0];
		vertices->normal[1] = vertex->normal[1];
		vertices->normal[2] = vertex->normal[2];
		vertices->st[0] = vertex->st[0];
		vertices->st[1] = vertex->st[1];
		vertices->color[0] = color[0];
		vertices->color[1] = color[1];
		vertices->color[2] = color[2];
		vertices->color[3] = color[3];

		vertices++;
	}

	backEnd.numVertices += decal->numVertices;
}

/*
 =================
 RB_BatchPoly
 =================
*/
static void RB_BatchPoly (meshData_t *data){

	renderPoly_t		*poly = data;
	renderPolyVertex_t	*vertex;
	index_t				*indices;
	vertexArray_t		*vertices;
	int					i;

	// Check for overflow
	RB_CheckOverflow((poly->numVertices-2) * 3, poly->numVertices, MAX_INDICES, MAX_VERTICES);

	// Set up indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 2; i < poly->numVertices; i++){
		indices[0] = backEnd.numVertices + 0;
		indices[1] = backEnd.numVertices + i-1;
		indices[2] = backEnd.numVertices + i;

		indices += 3;
	}

	backEnd.numIndices += (poly->numVertices-2) * 3;

	// Set up vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	for (i = 0, vertex = poly->vertices; i < poly->numVertices; i++, vertex++){
		vertices->xyz[0] = vertex->xyz[0];
		vertices->xyz[1] = vertex->xyz[1];
		vertices->xyz[2] = vertex->xyz[2];
		vertices->normal[0] = vertex->normal[0];
		vertices->normal[1] = vertex->normal[1];
		vertices->normal[2] = vertex->normal[2];
		vertices->st[0] = vertex->st[0];
		vertices->st[1] = vertex->st[1];
		vertices->color[0] = vertex->color[0];
		vertices->color[1] = vertex->color[1];
		vertices->color[2] = vertex->color[2];
		vertices->color[3] = vertex->color[3];

		vertices++;
	}

	backEnd.numVertices += poly->numVertices;
}

/*
 =================
 RB_BatchMesh
 =================
*/
void RB_BatchMesh (meshType_t type, meshData_t *data){

	switch (type){
	case MESH_SURFACE:
		RB_BatchSurface(data);
		break;
	case MESH_DECORATION:
		RB_BatchDecoration(data);
		break;
	case MESH_ALIASMODEL:
		RB_BatchAliasModel(data);
		break;
	case MESH_SPRITE:
		RB_BatchSprite(data);
		break;
	case MESH_BEAM:
		RB_BatchBeam(data);
		break;
	case MESH_DECAL:
		RB_BatchDecal(data);
		break;
	case MESH_POLY:
		RB_BatchPoly(data);
		break;
	default:
		Com_Error(ERR_DROP, "RB_BatchMesh: bad type (%i)", type);
	}
}


/*
 =======================================================================

 SHADOW RENDERING

 =======================================================================
*/


/*
 =================
 RB_BatchSurfaceShadow
 =================
*/
static void RB_BatchSurfaceShadow (meshData_t *data){

	index_t				indexRemap[MAX_INDICES];
	surface_t			*surface = data;
	surfTriangle_t		*triangle;
	surfVertex_t		*vertex;
	index_t				*indices;
	shadowVertexArray_t	*vertices;
	int					i;

	// Check for overflow
	RB_CheckOverflow(surface->numTriangles * 24, surface->numVertices * 2, MAX_SHADOW_INDICES, MAX_SHADOW_VERTICES);

	// Set up vertices
	vertices = backEnd.shadowVertices + backEnd.numVertices;

	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		vertices[0].xyzw[0] = vertex->xyz[0];
		vertices[0].xyzw[1] = vertex->xyz[1];
		vertices[0].xyzw[2] = vertex->xyz[2];
		vertices[0].xyzw[3] = 1.0;

		vertices[1].xyzw[0] = vertex->xyz[0] - backEnd.localParms.lightOrigin[0];
		vertices[1].xyzw[1] = vertex->xyz[1] - backEnd.localParms.lightOrigin[1];
		vertices[1].xyzw[2] = vertex->xyz[2] - backEnd.localParms.lightOrigin[2];
		vertices[1].xyzw[3] = 0.0;

		vertices += 2;

		indexRemap[i] = backEnd.numVertices;
		backEnd.numVertices += 2;
	}

	// Set up indices for silhouette edges
	indices = backEnd.shadowIndices + backEnd.numIndices;

	for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
		if (triangle->neighbor[0] < 0){
			indices[0] = indexRemap[triangle->index[1]];
			indices[1] = indexRemap[triangle->index[0]];
			indices[2] = indexRemap[triangle->index[0]] + 1;
			indices[3] = indexRemap[triangle->index[1]];
			indices[4] = indexRemap[triangle->index[0]] + 1;
			indices[5] = indexRemap[triangle->index[1]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}

		if (triangle->neighbor[1] < 0){
			indices[0] = indexRemap[triangle->index[2]];
			indices[1] = indexRemap[triangle->index[1]];
			indices[2] = indexRemap[triangle->index[1]] + 1;
			indices[3] = indexRemap[triangle->index[2]];
			indices[4] = indexRemap[triangle->index[1]] + 1;
			indices[5] = indexRemap[triangle->index[2]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}

		if (triangle->neighbor[2] < 0){
			indices[0] = indexRemap[triangle->index[0]];
			indices[1] = indexRemap[triangle->index[2]];
			indices[2] = indexRemap[triangle->index[2]] + 1;
			indices[3] = indexRemap[triangle->index[0]];
			indices[4] = indexRemap[triangle->index[2]] + 1;
			indices[5] = indexRemap[triangle->index[0]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}
	}

	// Set up indices for front and back caps
	if (backEnd.shadowCaps){
		for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
			indices[0] = indexRemap[triangle->index[0]];
			indices[1] = indexRemap[triangle->index[1]];
			indices[2] = indexRemap[triangle->index[2]];
			indices[3] = indexRemap[triangle->index[2]] + 1;
			indices[4] = indexRemap[triangle->index[1]] + 1;
			indices[5] = indexRemap[triangle->index[0]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}
	}
}

/*
 =================
 RB_BatchDecorationShadow
 =================
*/
static void RB_BatchDecorationShadow (meshData_t *data){

	qboolean			triangleFacingLight[MAX_INDICES / 3];
	index_t				indexRemap[MAX_INDICES];
	decSurface_t		*surface = data;
	surfTriangle_t		*triangle;
	surfFacePlane_t		*facePlane;
	surfVertex_t		*vertex;
	index_t				*indices;
	shadowVertexArray_t	*vertices;
	int					i;

	// Check for overflow
	RB_CheckOverflow(surface->numTriangles * 24, surface->numVertices * 2, MAX_SHADOW_INDICES, MAX_SHADOW_VERTICES);

	// Set up vertices
	vertices = backEnd.shadowVertices + backEnd.numVertices;

	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		vertices[0].xyzw[0] = vertex->xyz[0];
		vertices[0].xyzw[1] = vertex->xyz[1];
		vertices[0].xyzw[2] = vertex->xyz[2];
		vertices[0].xyzw[3] = 1.0;

		vertices[1].xyzw[0] = vertex->xyz[0] - backEnd.localParms.lightOrigin[0];
		vertices[1].xyzw[1] = vertex->xyz[1] - backEnd.localParms.lightOrigin[1];
		vertices[1].xyzw[2] = vertex->xyz[2] - backEnd.localParms.lightOrigin[2];
		vertices[1].xyzw[3] = 0.0;

		vertices += 2;

		indexRemap[i] = backEnd.numVertices;
		backEnd.numVertices += 2;
	}

	// Find front facing triangles
	for (i = 0, facePlane = surface->facePlanes; i < surface->numTriangles; i++, facePlane++){
		if (DotProduct(backEnd.localParms.lightOrigin, facePlane->normal) - facePlane->dist > 0)
			triangleFacingLight[i] = true;
		else
			triangleFacingLight[i] = false;
	}

	// Set up indices for silhouette edges
	indices = backEnd.shadowIndices + backEnd.numIndices;

	for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
		if (!triangleFacingLight[i])
			continue;

		if (triangle->neighbor[0] < 0 || !triangleFacingLight[triangle->neighbor[0]]){
			indices[0] = indexRemap[triangle->index[1]];
			indices[1] = indexRemap[triangle->index[0]];
			indices[2] = indexRemap[triangle->index[0]] + 1;
			indices[3] = indexRemap[triangle->index[1]];
			indices[4] = indexRemap[triangle->index[0]] + 1;
			indices[5] = indexRemap[triangle->index[1]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}

		if (triangle->neighbor[1] < 0 || !triangleFacingLight[triangle->neighbor[1]]){
			indices[0] = indexRemap[triangle->index[2]];
			indices[1] = indexRemap[triangle->index[1]];
			indices[2] = indexRemap[triangle->index[1]] + 1;
			indices[3] = indexRemap[triangle->index[2]];
			indices[4] = indexRemap[triangle->index[1]] + 1;
			indices[5] = indexRemap[triangle->index[2]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}

		if (triangle->neighbor[2] < 0 || !triangleFacingLight[triangle->neighbor[2]]){
			indices[0] = indexRemap[triangle->index[0]];
			indices[1] = indexRemap[triangle->index[2]];
			indices[2] = indexRemap[triangle->index[2]] + 1;
			indices[3] = indexRemap[triangle->index[0]];
			indices[4] = indexRemap[triangle->index[2]] + 1;
			indices[5] = indexRemap[triangle->index[0]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}
	}

	// Set up indices for front and back caps
	if (backEnd.shadowCaps){
		for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
			if (!triangleFacingLight[i])
				continue;

			indices[0] = indexRemap[triangle->index[0]];
			indices[1] = indexRemap[triangle->index[1]];
			indices[2] = indexRemap[triangle->index[2]];
			indices[3] = indexRemap[triangle->index[2]] + 1;
			indices[4] = indexRemap[triangle->index[1]] + 1;
			indices[5] = indexRemap[triangle->index[0]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}
	}
}

/*
 =================
 RB_BatchAliasModelShadow
 =================
*/
static void RB_BatchAliasModelShadow (meshData_t *data){

	qboolean			triangleFacingLight[MAX_INDICES / 3];
	index_t				indexRemap[MAX_INDICES];
	mdlSurface_t		*surface = data;
	mdlTriangle_t		*triangle;
	mdlFacePlane_t		*curFacePlane, *oldFacePlane;
	mdlXyzNormal_t		*curXyzNormal, *oldXyzNormal;
	float				backLerp;
	vec3_t				xyz, normal;
	float				dist;
	index_t				*indices;
	shadowVertexArray_t	*vertices;
	int					i;

	// Interpolate frames
	curFacePlane = surface->facePlanes + surface->numTriangles * backEnd.entity->frame;
	oldFacePlane = surface->facePlanes + surface->numTriangles * backEnd.entity->oldFrame;

	curXyzNormal = surface->xyzNormals + surface->numVertices * backEnd.entity->frame;
	oldXyzNormal = surface->xyzNormals + surface->numVertices * backEnd.entity->oldFrame;

	if (backEnd.entity->frame == backEnd.entity->oldFrame)
		backLerp = 0.0;
	else
		backLerp = backEnd.entity->backLerp;

	// Check for overflow
	RB_CheckOverflow(surface->numTriangles * 24, surface->numVertices * 2, MAX_SHADOW_INDICES, MAX_SHADOW_VERTICES);

	// Set up vertices
	vertices = backEnd.shadowVertices + backEnd.numVertices;

	if (backLerp == 0.0){
		// Optimized case
		for (i = 0; i < surface->numVertices; i++, curXyzNormal++){
			vertices[0].xyzw[0] = curXyzNormal->xyz[0];
			vertices[0].xyzw[1] = curXyzNormal->xyz[1];
			vertices[0].xyzw[2] = curXyzNormal->xyz[2];
			vertices[0].xyzw[3] = 1.0;

			vertices[1].xyzw[0] = curXyzNormal->xyz[0] - backEnd.localParms.lightOrigin[0];
			vertices[1].xyzw[1] = curXyzNormal->xyz[1] - backEnd.localParms.lightOrigin[1];
			vertices[1].xyzw[2] = curXyzNormal->xyz[2] - backEnd.localParms.lightOrigin[2];
			vertices[1].xyzw[3] = 0.0;

			vertices += 2;

			indexRemap[i] = backEnd.numVertices;
			backEnd.numVertices += 2;
		}
	}
	else {
		// General case
		for (i = 0; i < surface->numVertices; i++, curXyzNormal++, oldXyzNormal++){
			xyz[0] = curXyzNormal->xyz[0] + (oldXyzNormal->xyz[0] - curXyzNormal->xyz[0]) * backLerp;
			xyz[1] = curXyzNormal->xyz[1] + (oldXyzNormal->xyz[1] - curXyzNormal->xyz[1]) * backLerp;
			xyz[2] = curXyzNormal->xyz[2] + (oldXyzNormal->xyz[2] - curXyzNormal->xyz[2]) * backLerp;

			vertices[0].xyzw[0] = xyz[0];
			vertices[0].xyzw[1] = xyz[1];
			vertices[0].xyzw[2] = xyz[2];
			vertices[0].xyzw[3] = 1.0;

			vertices[1].xyzw[0] = xyz[0] - backEnd.localParms.lightOrigin[0];
			vertices[1].xyzw[1] = xyz[1] - backEnd.localParms.lightOrigin[1];
			vertices[1].xyzw[2] = xyz[2] - backEnd.localParms.lightOrigin[2];
			vertices[1].xyzw[3] = 0.0;

			vertices += 2;

			indexRemap[i] = backEnd.numVertices;
			backEnd.numVertices += 2;
		}
	}

	// Find front facing triangles
	if (backLerp == 0.0){
		// Optimized case
		for (i = 0; i < surface->numTriangles; i++, curFacePlane++){
			if (DotProduct(backEnd.localParms.lightOrigin, curFacePlane->normal) - curFacePlane->dist > 0)
				triangleFacingLight[i] = true;
			else
				triangleFacingLight[i] = false;
		}
	}
	else {
		// General case
		for (i = 0; i < surface->numTriangles; i++, curFacePlane++, oldFacePlane++){
			normal[0] = curFacePlane->normal[0] + (oldFacePlane->normal[0] - curFacePlane->normal[0]) * backLerp;
			normal[1] = curFacePlane->normal[1] + (oldFacePlane->normal[1] - curFacePlane->normal[1]) * backLerp;
			normal[2] = curFacePlane->normal[2] + (oldFacePlane->normal[2] - curFacePlane->normal[2]) * backLerp;

			dist = curFacePlane->dist + (oldFacePlane->dist - curFacePlane->dist) * backLerp;

			if (DotProduct(backEnd.localParms.lightOrigin, normal) - dist > 0)
				triangleFacingLight[i] = true;
			else
				triangleFacingLight[i] = false;
		}
	}

	// Set up indices for silhouette edges
	indices = backEnd.shadowIndices + backEnd.numIndices;

	for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
		if (!triangleFacingLight[i])
			continue;

		if (triangle->neighbor[0] < 0 || !triangleFacingLight[triangle->neighbor[0]]){
			indices[0] = indexRemap[triangle->index[1]];
			indices[1] = indexRemap[triangle->index[0]];
			indices[2] = indexRemap[triangle->index[0]] + 1;
			indices[3] = indexRemap[triangle->index[1]];
			indices[4] = indexRemap[triangle->index[0]] + 1;
			indices[5] = indexRemap[triangle->index[1]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}

		if (triangle->neighbor[1] < 0 || !triangleFacingLight[triangle->neighbor[1]]){
			indices[0] = indexRemap[triangle->index[2]];
			indices[1] = indexRemap[triangle->index[1]];
			indices[2] = indexRemap[triangle->index[1]] + 1;
			indices[3] = indexRemap[triangle->index[2]];
			indices[4] = indexRemap[triangle->index[1]] + 1;
			indices[5] = indexRemap[triangle->index[2]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}

		if (triangle->neighbor[2] < 0 || !triangleFacingLight[triangle->neighbor[2]]){
			indices[0] = indexRemap[triangle->index[0]];
			indices[1] = indexRemap[triangle->index[2]];
			indices[2] = indexRemap[triangle->index[2]] + 1;
			indices[3] = indexRemap[triangle->index[0]];
			indices[4] = indexRemap[triangle->index[2]] + 1;
			indices[5] = indexRemap[triangle->index[0]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}
	}

	// Set up indices for front and back caps
	if (backEnd.shadowCaps){
		for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
			if (!triangleFacingLight[i])
				continue;

			indices[0] = indexRemap[triangle->index[0]];
			indices[1] = indexRemap[triangle->index[1]];
			indices[2] = indexRemap[triangle->index[2]];
			indices[3] = indexRemap[triangle->index[2]] + 1;
			indices[4] = indexRemap[triangle->index[1]] + 1;
			indices[5] = indexRemap[triangle->index[0]] + 1;

			indices += 6;

			backEnd.numIndices += 6;
		}
	}
}

/*
 =================
 RB_BatchMeshShadow
 =================
*/
void RB_BatchMeshShadow (meshType_t type, meshData_t *data){

	switch (type){
	case MESH_SURFACE:
		RB_BatchSurfaceShadow(data);
		break;
	case MESH_DECORATION:
		RB_BatchDecorationShadow(data);
		break;
	case MESH_ALIASMODEL:
		RB_BatchAliasModelShadow(data);
		break;
	default:
		Com_Error(ERR_DROP, "RB_BatchMeshShadow: bad type (%i)", type);
	}
}
