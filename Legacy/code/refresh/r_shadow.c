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


#include "r_local.h"


typedef struct {
	entity_t	*entity;
	mdl_t		*alias;
} shadow_t;

static shadow_t	r_shadowList[MAX_ENTITIES];
static int		r_numShadows;

static qboolean	r_firstShadow;
static vec3_t	r_triangleNormals[MAX_INDICES / 3];
static qboolean	r_triangleFacingLight[MAX_INDICES / 3];


/*
 =================
 R_LerpShadowVertices
 =================
*/
static void R_LerpShadowVertices (entity_t *entity, mdl_t *alias, mdlSurface_t *surface){

	mdlFrame_t		*curFrame, *oldFrame;
	mdlXyzNormal_t	*curXyzNormal, *oldXyzNormal;
	vec3_t			curScale, oldScale;
	vec3_t			delta, move;
	int				i;

	// Draw all the triangles
	curFrame = alias->frames + entity->frame;
	oldFrame = alias->frames + entity->oldFrame;

	curXyzNormal = surface->xyzNormals + surface->numVertices * entity->frame;
	oldXyzNormal = surface->xyzNormals + surface->numVertices * entity->oldFrame;

	// Interpolate frames
	VectorSubtract(entity->oldOrigin, entity->origin, delta);
	VectorRotate(delta, entity->axis, move);

	move[0] = curFrame->translate[0] + (move[0] + oldFrame->translate[0] - curFrame->translate[0]) * entity->backLerp;
	move[1] = curFrame->translate[1] + (move[1] + oldFrame->translate[1] - curFrame->translate[1]) * entity->backLerp;
	move[2] = curFrame->translate[2] + (move[2] + oldFrame->translate[2] - curFrame->translate[2]) * entity->backLerp;

	VectorScale(curFrame->scale, 1.0 - entity->backLerp, curScale);
	VectorScale(oldFrame->scale, entity->backLerp, oldScale);

	// Interpolate vertices
	for (i = 0; i < surface->numVertices; i++, curXyzNormal++, oldXyzNormal++){
		vertexArray[numVertex][0] = move[0] + curXyzNormal->xyz[0]*curScale[0] + oldXyzNormal->xyz[0]*oldScale[0];
		vertexArray[numVertex][1] = move[1] + curXyzNormal->xyz[1]*curScale[1] + oldXyzNormal->xyz[1]*oldScale[1];
		vertexArray[numVertex][2] = move[2] + curXyzNormal->xyz[2]*curScale[2] + oldXyzNormal->xyz[2]*oldScale[2];

		numVertex++;
	}
}

/*
 =================
 R_CalcShadowVolumeTriangleNormals
 =================
*/
static void R_CalcShadowVolumeTriangleNormals (int numTriangles, mdlTriangle_t *triangles){

	mdlTriangle_t	*triangle;
	vec3_t			edge[2];
	float			*v[3];
	int				i;

	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		v[0] = (float *)(vertexArray + triangle->index[0]);
		v[1] = (float *)(vertexArray + triangle->index[1]);
		v[2] = (float *)(vertexArray + triangle->index[2]);

		VectorSubtract(v[0], v[1], edge[0]);
		VectorSubtract(v[2], v[1], edge[1]);
		CrossProduct(edge[0], edge[1], r_triangleNormals[i]);
	}
}

/*
 =================
 R_BuildShadowVolumeTriangles
 =================
*/
static void R_BuildShadowVolumeTriangles (const vec3_t lightOrg, int numTriangles, mdlTriangle_t *triangles, mdlNeighbor_t *neighbors){

	mdlTriangle_t	*triangle;
	mdlNeighbor_t	*neighbor;
	vec3_t			dist;
	float			*v, dot;
	unsigned		*index;
	int				i;

	// Find front facing triangles
	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		v = (float *)(vertexArray + triangle->index[0]);

		VectorSubtract(lightOrg, v, dist);
		dot = DotProduct(dist, r_triangleNormals[i]);

		r_triangleFacingLight[i] = (dot > 0);
	}

	// Set up indices
	index = indexArray;
	numIndex = 0;

	for (i = 0, triangle = triangles, neighbor = neighbors; i < numTriangles; i++, triangle++, neighbor++){
		if (!r_triangleFacingLight[i])
			continue;

		// Front and back caps
		index[0] = triangle->index[0];
		index[1] = triangle->index[1];
		index[2] = triangle->index[2];
		index[3] = triangle->index[2] + numVertex;
		index[4] = triangle->index[1] + numVertex;
		index[5] = triangle->index[0] + numVertex;

		index += 6;
		numIndex += 6;

		// Check the edges
		if (neighbor->index[0] < 0 || !r_triangleFacingLight[neighbor->index[0]]){
			index[0] = triangle->index[1];
			index[1] = triangle->index[0];
			index[2] = triangle->index[0] + numVertex;
			index[3] = triangle->index[1];
			index[4] = triangle->index[0] + numVertex;
			index[5] = triangle->index[1] + numVertex;

			index += 6;
			numIndex += 6;
		}

		if (neighbor->index[1] < 0 || !r_triangleFacingLight[neighbor->index[1]]){
			index[0] = triangle->index[2];
			index[1] = triangle->index[1];
			index[2] = triangle->index[1] + numVertex;
			index[3] = triangle->index[2];
			index[4] = triangle->index[1] + numVertex;
			index[5] = triangle->index[2] + numVertex;

			index += 6;
			numIndex += 6;
		}

		if (neighbor->index[2] < 0 || !r_triangleFacingLight[neighbor->index[2]]){
			index[0] = triangle->index[0];
			index[1] = triangle->index[2];
			index[2] = triangle->index[2] + numVertex;
			index[3] = triangle->index[0];
			index[4] = triangle->index[2] + numVertex;
			index[5] = triangle->index[0] + numVertex;

			index += 6;
			numIndex += 6;
		}
	}
}

/*
 =================
 R_CastShadowVolume
 =================
*/
static void R_CastShadowVolume (entity_t *entity, mdl_t *alias, mdlSurface_t *surface, const vec3_t mins, const vec3_t maxs, float radius, const vec3_t origin, float intensity){

	vec3_t	lightOrg, dir;
	float	dist;
	int		i;

	if (R_CullSphere(origin, intensity, 15))
		return;		// Light is completely outside the frustum

	if (VectorCompare(origin, entity->origin))
		return;		// Light is inside bounding box

	VectorSubtract(origin, entity->origin, dir);
	VectorRotate(dir, entity->axis, lightOrg);

	if (!BoundsAndSphereIntersect(mins, maxs, lightOrg, intensity))
		return;		// Light is too far away

	// If this is the first shadow, set up some things
	if (r_firstShadow){
		// Load the matrix
		R_RotateForEntity(entity);

		// Interpolate vertices
		R_LerpShadowVertices(entity, alias, surface);

		// Calculate triangle normals
		R_CalcShadowVolumeTriangleNormals(surface->numTriangles, surface->triangles);

		r_firstShadow = false;
	}

	// Build triangles
	R_BuildShadowVolumeTriangles(lightOrg, surface->numTriangles, surface->triangles, surface->neighbors);

	// Extrude silhouette
	for (i = 0; i < numVertex; i++){
		VectorSubtract(vertexArray[i], lightOrg, dir);
		dist = VectorNormalize(dir);
		VectorMA(vertexArray[i], radius - dist + intensity, dir, vertexArray[numVertex+i]);
	}

	r_stats.numVertices += numVertex * 2;
	r_stats.numIndices += numIndex;
	r_stats.totalIndices += numIndex;

	// Draw it
	if (glConfig.vertexBufferObject){
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, rb_vbo.indexBuffer);
		qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, numIndex * sizeof(unsigned), indexArray, GL_STREAM_DRAW_ARB);

		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, rb_vbo.vertexBuffer);
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, numVertex * 2 * sizeof(vec3_t), vertexArray, GL_STREAM_DRAW_ARB);
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglVertexPointer(3, GL_FLOAT, 0, VBO_OFFSET(0));

		if (!glConfig.stencilTwoSide && !glConfig.separateStencil && !r_showShadowVolumes->integer){
			r_stats.totalIndices += numIndex;

			GL_CullFace(GL_BACK);
			qglStencilOp(GL_KEEP, GL_INCR, GL_KEEP);

			if (glConfig.drawRangeElements)
				qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numVertex * 2, numIndex, GL_UNSIGNED_INT, VBO_OFFSET(0));
			else
				qglDrawElements(GL_TRIANGLES, numIndex, GL_UNSIGNED_INT, VBO_OFFSET(0));

			GL_CullFace(GL_FRONT);
			qglStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
		}

		if (glConfig.drawRangeElements)
			qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numVertex * 2, numIndex, GL_UNSIGNED_INT, VBO_OFFSET(0));
		else
			qglDrawElements(GL_TRIANGLES, numIndex, GL_UNSIGNED_INT, VBO_OFFSET(0));
	}
	else {
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglVertexPointer(3, GL_FLOAT, 0, vertexArray);

		if (glConfig.compiledVertexArray)
			qglLockArraysEXT(0, numVertex * 2);

		if (!glConfig.stencilTwoSide && !glConfig.separateStencil && !r_showShadowVolumes->integer){
			r_stats.totalIndices += numIndex;

			GL_CullFace(GL_BACK);
			qglStencilOp(GL_KEEP, GL_INCR, GL_KEEP);

			if (glConfig.drawRangeElements)
				qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numVertex * 2, numIndex, GL_UNSIGNED_INT, indexArray);
			else
				qglDrawElements(GL_TRIANGLES, numIndex, GL_UNSIGNED_INT, indexArray);

			GL_CullFace(GL_FRONT);
			qglStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
		}

		if (glConfig.drawRangeElements)
			qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numVertex * 2, numIndex, GL_UNSIGNED_INT, indexArray);
		else
			qglDrawElements(GL_TRIANGLES, numIndex, GL_UNSIGNED_INT, indexArray);

		if (glConfig.compiledVertexArray)
			qglUnlockArraysEXT();
	}
}

/*
 =================
 R_DrawShadowVolumes
 =================
*/
static void R_DrawShadowVolumes (entity_t *entity, mdl_t *alias, mdlSurface_t *surface){

	mdlFrame_t	*curFrame, *oldFrame;
	vec3_t		mins, maxs;
	float		radius;
	dlight_t	*dl;
	int			i;

	r_firstShadow = true;

	// Find bounds and radius
	curFrame = alias->frames + entity->frame;
	oldFrame = alias->frames + entity->oldFrame;

	if (curFrame == oldFrame){
		VectorCopy(curFrame->mins, mins);
		VectorCopy(curFrame->maxs, maxs);

		radius = curFrame->radius;
	}
	else {
		for (i = 0; i < 3; i++){
			if (curFrame->mins[i] < oldFrame->mins[i])
				mins[i] = curFrame->mins[i];
			else
				mins[i] = oldFrame->mins[i];

			if (curFrame->maxs[i] > oldFrame->maxs[i])
				maxs[i] = curFrame->maxs[i];
			else
				maxs[i] = oldFrame->maxs[i];
		}

		if (curFrame->radius > oldFrame->radius)
			radius = curFrame->radius;
		else
			radius = oldFrame->radius;
	}

	// Cast shadow volumes
	for (i = 0, dl = r_dlights; i < r_numDLights; i++, dl++)
		R_CastShadowVolume(entity, alias, surface, mins, maxs, radius, dl->origin, dl->intensity);

	// TODO!!!
	{
		vec3_t	org;

		org[0] = entity->origin[0] + (entity->oldOrigin[0] - entity->origin[0]) * entity->backLerp;
		org[1] = entity->origin[1] + (entity->oldOrigin[1] - entity->origin[1]) * entity->backLerp;
		org[2] = entity->origin[2] + (entity->oldOrigin[2] - entity->origin[2]) * entity->backLerp;

		org[0] += 32;
		org[2] += 64;

		R_CastShadowVolume(entity, alias, surface, mins, maxs, radius, org, 200);
	}
}

/*
 =================
 R_DrawPlanarShadow
 =================
*/
static void R_DrawPlanarShadow (entity_t *entity, mdl_t *alias, mdlSurface_t *surface){

	vec3_t	start, end, point;
	vec3_t	dir, lightDir, planeNormal;
	float	dist, planeDist;
	trace_t	trace;
	int		i;

	// Find shadow direction
	R_LightDir(entity->origin, lightDir);

	VectorNormalizeFast(lightDir);
	VectorSet(dir, -lightDir[0], -lightDir[1], -1);

	// Find shadow plane
	VectorSet(start, entity->origin[0], entity->origin[1], entity->origin[2] + 8);
	VectorMA(start, 256, dir, end);

	trace = CM_BoxTrace(start, end, vec3_origin, vec3_origin, 0, MASK_SOLID);
	if (trace.fraction == 0.0 || trace.fraction == 1.0)
		return;		// Didn't hit anything

	if (r_refDef.viewOrigin[2] < trace.endpos[2])
		return;		// View origin is below shadow

	if (R_CullSphere(trace.endpos, entity->model->radius, 15))
		return;		// Shadow is completely outside the frustum

	// Transform shadow plane
	VectorRotate(trace.plane.normal, entity->axis, planeNormal);
	VectorNormalizeFast(planeNormal);

	VectorSubtract(trace.endpos, entity->origin, point);
	planeDist = DotProduct(point, trace.plane.normal) + 0.1;

	// Transform shadow direction
	VectorRotate(dir, entity->axis, lightDir);
	VectorNormalizeFast(lightDir);

	dist = -1.0 / DotProduct(lightDir, planeNormal);
	VectorScale(lightDir, dist, lightDir);

	// Load the matrix
	R_RotateForEntity(entity);

	// Interpolate vertices
	R_LerpShadowVertices(entity, alias, surface);

	// Project vertices
	for (i = 0; i < numVertex; i++){
		dist = DotProduct(vertexArray[i], planeNormal) - planeDist;
		if (dist > 0)
			VectorMA(vertexArray[i], dist, lightDir, vertexArray[i]);
	}

	r_stats.numVertices += numVertex;
	r_stats.numIndices += numIndex;
	r_stats.totalIndices += numIndex;

	// Draw it
	if (glConfig.vertexBufferObject){
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, rb_vbo.indexBuffer);
		qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, surface->numTriangles * sizeof(mdlTriangle_t), surface->triangles, GL_STREAM_DRAW_ARB);

		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, rb_vbo.vertexBuffer);
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, numVertex * sizeof(vec3_t), vertexArray, GL_STREAM_DRAW_ARB);
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglVertexPointer(3, GL_FLOAT, 0, VBO_OFFSET(0));

		if (glConfig.drawRangeElements)
			qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numVertex, surface->numTriangles * 3, GL_UNSIGNED_INT, VBO_OFFSET(0));
		else
			qglDrawElements(GL_TRIANGLES, surface->numTriangles * 3, GL_UNSIGNED_INT, VBO_OFFSET(0));
	}
	else {
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglVertexPointer(3, GL_FLOAT, 0, vertexArray);

		if (glConfig.compiledVertexArray)
			qglLockArraysEXT(0, numVertex);

		if (glConfig.drawRangeElements)
			qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numVertex, surface->numTriangles * 3, GL_UNSIGNED_INT, surface->triangles);
		else
			qglDrawElements(GL_TRIANGLES, surface->numTriangles * 3, GL_UNSIGNED_INT, surface->triangles);

		if (glConfig.compiledVertexArray)
			qglUnlockArraysEXT();
	}
}

/*
 =================
 R_SetShadowState
 =================
*/
static void R_SetShadowState (void){

	if (r_shadows->integer == 2 && glConfig.stencilBits){
		if (r_showShadowVolumes->integer){
			GL_Enable(GL_CULL_FACE);
			GL_Disable(GL_POLYGON_OFFSET_FILL);
			GL_Disable(GL_VERTEX_PROGRAM_ARB);
			GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
			GL_Disable(GL_ALPHA_TEST);
			GL_Enable(GL_BLEND);
			GL_Enable(GL_DEPTH_TEST);

			GL_CullFace(GL_FRONT);
			GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
			GL_DepthFunc(GL_LESS);
			GL_DepthMask(GL_FALSE);

			qglColor4ub(255, 0, 127, 127);
		}
		else {
			if (glConfig.stencilTwoSide){
				qglEnable(GL_STENCIL_TEST);
				qglEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);

				qglActiveStencilFaceEXT(GL_BACK);
				qglStencilFunc(GL_ALWAYS, 128, 255);
				qglStencilOp(GL_KEEP, GL_INCR, GL_KEEP);

				qglActiveStencilFaceEXT(GL_FRONT);
				qglStencilFunc(GL_ALWAYS, 128, 255);
				qglStencilOp(GL_KEEP, GL_DECR, GL_KEEP);

				GL_Disable(GL_CULL_FACE);
			}
			else if (glConfig.separateStencil){
				qglEnable(GL_STENCIL_TEST);

				qglStencilFuncSeparateATI(GL_ALWAYS, GL_ALWAYS, 128, 255);
				qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_INCR, GL_KEEP);
				qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_DECR, GL_KEEP);

				GL_Disable(GL_CULL_FACE);
			}
			else {
				qglEnable(GL_STENCIL_TEST);
				qglStencilFunc(GL_ALWAYS, 128, 255);
				qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

				GL_Enable(GL_CULL_FACE);
			}

			GL_Disable(GL_POLYGON_OFFSET_FILL);
			GL_Disable(GL_VERTEX_PROGRAM_ARB);
			GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
			GL_Disable(GL_ALPHA_TEST);
			GL_Disable(GL_BLEND);
			GL_Enable(GL_DEPTH_TEST);

			GL_DepthFunc(GL_LESS);
			GL_DepthMask(GL_FALSE);

			qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		}
	}
	else {
		if (glConfig.stencilBits){
			qglEnable(GL_STENCIL_TEST);
			qglStencilFunc(GL_EQUAL, 128, 255);
			qglStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
		}

		GL_Enable(GL_CULL_FACE);
		GL_Enable(GL_POLYGON_OFFSET_FILL);
		GL_Disable(GL_VERTEX_PROGRAM_ARB);
		GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
		GL_Disable(GL_ALPHA_TEST);
		GL_Enable(GL_BLEND);
		GL_Enable(GL_DEPTH_TEST);

		GL_CullFace(GL_FRONT);
		GL_PolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GL_DepthFunc(GL_LEQUAL);
		GL_DepthMask(GL_FALSE);

		qglColor4ub(0, 0, 0, 127);
	}

	qglDisableClientState(GL_NORMAL_ARRAY);
	qglDisableClientState(GL_COLOR_ARRAY);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

/*
 =================
 R_ResetShadowState
 =================
*/
static void R_ResetShadowState (void){

	if (r_shadows->integer == 2 && glConfig.stencilBits){
		if (!r_showShadowVolumes->integer){
			qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			if (glConfig.stencilTwoSide)
				qglDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);

			qglDisable(GL_STENCIL_TEST);
		}
	}
	else {
		if (glConfig.stencilBits)
			qglDisable(GL_STENCIL_TEST);
	}
}

/*
 =================
 R_StencilBlend
 =================
*/
static void R_StencilBlend (void){

	if (r_shadows->integer != 2 || !glConfig.stencilBits)
		return;

	if (r_showShadowVolumes->integer)
		return;

	qglLoadIdentity();

	// Set the state
	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_VERTEX_PROGRAM_ARB);
	GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
	GL_Disable(GL_ALPHA_TEST);
	GL_Enable(GL_BLEND);
	GL_Disable(GL_DEPTH_TEST);

	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_DepthMask(GL_FALSE);

	qglEnable(GL_STENCIL_TEST);
	qglStencilFunc(GL_NOTEQUAL, 128, 255);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// Draw it
	qglColor4ub(0, 0, 0, 127);

	qglBegin(GL_QUADS);
	qglVertex3f( -100,  100, -10);
	qglVertex3f(  100,  100, -10);
	qglVertex3f(  100, -100, -10);
	qglVertex3f( -100, -100, -10);
	qglEnd();

	qglDisable(GL_STENCIL_TEST);
}

/*
 =================
 R_AddShadowToList
 =================
*/
void R_AddShadowToList (entity_t *entity, mdl_t *alias){

	shadow_t	*shadow;

	if (r_numShadows >= MAX_ENTITIES)
		return;

	shadow = &r_shadowList[r_numShadows++];

	shadow->entity = entity;
	shadow->alias = alias;
}

/*
 =================
 R_RenderShadows
 =================
*/
void R_RenderShadows (void){

	int				i, j;
	shadow_t		*shadow;
	entity_t		*entity;
	mdl_t			*alias;
	mdlSurface_t	*surface;
	shader_t		*shader;

	if (!r_shadows->integer || (r_refDef.rdFlags & RDF_NOWORLDMODEL)){
		r_numShadows = 0;
		return;
	}

	if (!r_numShadows)
		return;

	// Set the state
	R_SetShadowState();

	for (i = 0, shadow = r_shadowList; i < r_numShadows; i++, shadow++){
		entity = shadow->entity;
		alias = shadow->alias;

		// Never cast shadows from viewer or weapon model
		if (entity->renderFX & (RF_VIEWERMODEL | RF_WEAPONMODEL))
			continue;

		// Run through the surfaces
		for (j = 0, surface = alias->surfaces; j < alias->numSurfaces; j++, surface++){
			// Select shader
			if (entity->customShader)
				shader = entity->customShader;
			else {
				if (!surface->numShaders)
					continue;

				if (entity->skinNum < 0 || entity->skinNum >= surface->numShaders)
					entity->skinNum = 0;

				shader = surface->shaders[entity->skinNum].shader;
			}

			// Check if this surface doesn't cast shadows
			if (shader->flags & SHADER_NOSHADOWS)
				continue;

			// Cast shadows
			if (r_shadows->integer == 2 && glConfig.stencilBits)
				R_DrawShadowVolumes(entity, alias, surface);
			else
				R_DrawPlanarShadow(entity, alias, surface);

			// Clear arrays
			numIndex = numVertex = 0;
		}
	}

	r_numShadows = 0;

	// Reset the state
	R_ResetShadowState();

	// Final screen blend
	R_StencilBlend();
}
