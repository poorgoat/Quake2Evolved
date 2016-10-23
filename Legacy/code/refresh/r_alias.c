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


// r_alias.c -- triangle model functions


#include "r_local.h"
#include "normals.h"


/*
 =================
 R_CalcAmmoDisplayOffsets
 =================
*/
static void R_CalcAmmoDisplayOffsets (entity_t *entity, mdlSurface_t *surface){

	mdlAmmoDisplay_t	*ammoDisplay;
	float				offset;
	int					i, digit;

	ammoDisplay = surface->shaders[entity->skinNum].ammoDisplay;
	if (!ammoDisplay)
		return;

	switch (ammoDisplay->type){
	case AMMODISPLAY_DIGIT1:
		if (entity->renderFX & RF_CULLHACK)
			digit = 9 - (entity->ammoValue % 10);
		else
			digit = entity->ammoValue / 100;

		offset = digit * 0.1;

		break;
	case AMMODISPLAY_DIGIT2:
		if (entity->renderFX & RF_CULLHACK)
			digit = 9 - ((entity->ammoValue % 100) / 10);
		else
			digit = (entity->ammoValue % 100) / 10;

		offset = digit * 0.1;

		break;
	case AMMODISPLAY_DIGIT3:
		if (entity->renderFX & RF_CULLHACK)
			digit = 9 - (entity->ammoValue / 100);
		else
			digit = entity->ammoValue % 10;

		offset = digit * 0.1;

		break;
	case AMMODISPLAY_WARNING:
		offset = 0.0;

		break;
	}

	for (i = 0; i < numVertex; i++)
		inTexCoordArray[i][0] += offset;

	if (entity->renderFX & RF_CULLHACK){
		for (i = 0; i < numVertex; i++)
			inTexCoordArray[i][0] = -inTexCoordArray[i][0];
	}
}

/*
 =================
 R_RemapAmmoDisplayShader
 =================
*/
static shader_t *R_RemapAmmoDisplayShader (entity_t *entity, mdlSurface_t *surface){

	mdlAmmoDisplay_t	*ammoDisplay;

	ammoDisplay = surface->shaders[entity->skinNum].ammoDisplay;
	if (!ammoDisplay)
		return NULL;

	switch (ammoDisplay->type){
	case AMMODISPLAY_DIGIT1:
		if (!(entity->renderFX & RF_CULLHACK)){
			if (entity->ammoValue < 100)
				return NULL;
		}

		break;
	case AMMODISPLAY_DIGIT2:
		if (entity->ammoValue < 10)
			return NULL;

		break;
	case AMMODISPLAY_DIGIT3:
		if (entity->renderFX & RF_CULLHACK){
			if (entity->ammoValue < 100)
				return NULL;
		}

		break;
	case AMMODISPLAY_WARNING:
		if (entity->ammoValue > ammoDisplay->lowAmmo)
			return NULL;

		break;
	}

	if (entity->ammoValue > ammoDisplay->lowAmmo)
		return ammoDisplay->remapShaders[0];
	else if (entity->ammoValue > 0)
		return ammoDisplay->remapShaders[1];
	else
		return ammoDisplay->remapShaders[2];
}

/*
 =================
 R_GetAliasModelLOD
 =================
*/
static mdl_t *R_GetAliasModelLOD (entity_t *entity){

	model_t	*model = entity->model;
	int		maxLod, minLod, lod;
	float	dist;

	if (model->numAlias == 1)
		return model->alias[0];

	if (r_lodBias->integer < 0 || r_lodDistance->value <= 0)
		return model->alias[0];

	// Get max and min LODs
	maxLod = model->numAlias - 1;
	minLod = Clamp(r_lodBias->integer, 0, maxLod);

	// Find model distance
	dist = Distance(entity->origin, r_refDef.viewOrigin);

	// Select a LOD for the current distance
	lod = (int)floor(dist / r_lodDistance->value);

	// Clamp to min and max and return the LOD model
	return model->alias[Clamp(lod, minLod, maxLod)];
}

/*
 =================
 R_CullAliasModel
 =================
*/
static qboolean R_CullAliasModel (entity_t *entity, mdl_t *alias){

	model_t		*model = entity->model;
	mdlFrame_t	*curFrame, *oldFrame;
	vec3_t		mins, maxs, tmp, bbox[8];
	cplane_t	*plane;
	int			mask, aggregateMask = ~0;
	int			i, j;

	if (entity->renderFX & RF_VIEWERMODEL)
		return true;
	if (entity->renderFX & RF_WEAPONMODEL)
		return false;

	if (r_noCull->integer)
		return false;

	// Compute axially aligned mins and maxs
	curFrame = alias->frames + entity->frame;
	oldFrame = alias->frames + entity->oldFrame;

	if (curFrame == oldFrame){
		VectorCopy(curFrame->mins, mins);
		VectorCopy(curFrame->maxs, maxs);
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
	}

	// Compute a full bounding box
	for (i = 0; i < 8; i++){
		tmp[0] = (i & 1) ? mins[0] : maxs[0];
		tmp[1] = (i & 2) ? mins[1] : maxs[1];
		tmp[2] = (i & 4) ? mins[2] : maxs[2];

		// Rotate and translate
		VectorRotate(tmp, entity->axis, bbox[i]);
		VectorAdd(bbox[i], entity->origin, bbox[i]);
	}

	// Cull
	for (i = 0; i < 8; i++){
		mask = 0;

		for (j = 0, plane = r_frustum; j < 4; j++, plane++){
			if (DotProduct(bbox[i], plane->normal) - plane->dist < 0)
				mask |= (1<<j);
		}

		aggregateMask &= mask;
	}

	if (aggregateMask)
		return true;

	return false;
}

/*
 =================
 R_DrawAliasModel
 =================
*/
void R_DrawAliasModel (void){

	mdl_t			*alias = rb_mesh->mesh;
	mdlSurface_t	*surface;
	mdlTriangle_t	*triangle;
	mdlSt_t			*st;
	mdlFrame_t		*curFrame, *oldFrame;
	mdlXyzNormal_t	*curXyzNormal, *oldXyzNormal;
	vec3_t			curScale, oldScale;
	vec3_t			curTangent, oldTangent;
	vec3_t			curBinormal, oldBinormal;
	vec3_t			curNormal, oldNormal;
	vec3_t			delta, move;
	int				i;

	// Draw all the triangles
	surface = alias->surfaces + rb_infoKey;

	curFrame = alias->frames + rb_entity->frame;
	oldFrame = alias->frames + rb_entity->oldFrame;

	curXyzNormal = surface->xyzNormals + surface->numVertices * rb_entity->frame;
	oldXyzNormal = surface->xyzNormals + surface->numVertices * rb_entity->oldFrame;

	// Interpolate frames
	VectorSubtract(rb_entity->oldOrigin, rb_entity->origin, delta);
	VectorRotate(delta, rb_entity->axis, move);

	move[0] = curFrame->translate[0] + (move[0] + oldFrame->translate[0] - curFrame->translate[0]) * rb_entity->backLerp;
	move[1] = curFrame->translate[1] + (move[1] + oldFrame->translate[1] - curFrame->translate[1]) * rb_entity->backLerp;
	move[2] = curFrame->translate[2] + (move[2] + oldFrame->translate[2] - curFrame->translate[2]) * rb_entity->backLerp;

	VectorScale(curFrame->scale, 1.0 - rb_entity->backLerp, curScale);
	VectorScale(oldFrame->scale, rb_entity->backLerp, oldScale);

	// Draw it
	RB_CheckMeshOverflow(surface->numTriangles * 3, surface->numVertices);

	for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
		indexArray[numIndex++] = numVertex + triangle->index[0];
		indexArray[numIndex++] = numVertex + triangle->index[1];
		indexArray[numIndex++] = numVertex + triangle->index[2];
	}

	for (i = 0, st = surface->st; i < surface->numVertices; i++, curXyzNormal++, oldXyzNormal++, st++){
		// Decode tangents, binormals and normals
		curTangent[0] = r_sinTable[curXyzNormal->tangent[0]] * r_cosTable[curXyzNormal->tangent[1]];
		curTangent[1] = r_sinTable[curXyzNormal->tangent[0]] * r_sinTable[curXyzNormal->tangent[1]];
		curTangent[2] = r_cosTable[curXyzNormal->tangent[0]];

		oldTangent[0] = r_sinTable[oldXyzNormal->tangent[0]] * r_cosTable[oldXyzNormal->tangent[1]];
		oldTangent[1] = r_sinTable[oldXyzNormal->tangent[0]] * r_sinTable[oldXyzNormal->tangent[1]];
		oldTangent[2] = r_cosTable[oldXyzNormal->tangent[0]];

		curBinormal[0] = r_sinTable[curXyzNormal->binormal[0]] * r_cosTable[curXyzNormal->binormal[1]];
		curBinormal[1] = r_sinTable[curXyzNormal->binormal[0]] * r_sinTable[curXyzNormal->binormal[1]];
		curBinormal[2] = r_cosTable[curXyzNormal->binormal[0]];

		oldBinormal[0] = r_sinTable[oldXyzNormal->binormal[0]] * r_cosTable[oldXyzNormal->binormal[1]];
		oldBinormal[1] = r_sinTable[oldXyzNormal->binormal[0]] * r_sinTable[oldXyzNormal->binormal[1]];
		oldBinormal[2] = r_cosTable[oldXyzNormal->binormal[0]];

		curNormal[0] = r_sinTable[curXyzNormal->normal[0]] * r_cosTable[curXyzNormal->normal[1]];
		curNormal[1] = r_sinTable[curXyzNormal->normal[0]] * r_sinTable[curXyzNormal->normal[1]];
		curNormal[2] = r_cosTable[curXyzNormal->normal[0]];

		oldNormal[0] = r_sinTable[oldXyzNormal->normal[0]] * r_cosTable[oldXyzNormal->normal[1]];
		oldNormal[1] = r_sinTable[oldXyzNormal->normal[0]] * r_sinTable[oldXyzNormal->normal[1]];
		oldNormal[2] = r_cosTable[oldXyzNormal->normal[0]];

		// Interpolate vertices, tangents, binormals and normals
		vertexArray[numVertex][0] = move[0] + curXyzNormal->xyz[0]*curScale[0] + oldXyzNormal->xyz[0]*oldScale[0];
		vertexArray[numVertex][1] = move[1] + curXyzNormal->xyz[1]*curScale[1] + oldXyzNormal->xyz[1]*oldScale[1];
		vertexArray[numVertex][2] = move[2] + curXyzNormal->xyz[2]*curScale[2] + oldXyzNormal->xyz[2]*oldScale[2];
		tangentArray[numVertex][0] = curTangent[0] + (oldTangent[0] - curTangent[0]) * rb_entity->backLerp;
		tangentArray[numVertex][1] = curTangent[1] + (oldTangent[1] - curTangent[1]) * rb_entity->backLerp;
		tangentArray[numVertex][2] = curTangent[2] + (oldTangent[2] - curTangent[2]) * rb_entity->backLerp;
		binormalArray[numVertex][0] = curBinormal[0] + (oldBinormal[0] - curBinormal[0]) * rb_entity->backLerp;
		binormalArray[numVertex][1] = curBinormal[1] + (oldBinormal[1] - curBinormal[1]) * rb_entity->backLerp;
		binormalArray[numVertex][2] = curBinormal[2] + (oldBinormal[2] - curBinormal[2]) * rb_entity->backLerp;
		normalArray[numVertex][0] = curNormal[0] + (oldNormal[0] - curNormal[0]) * rb_entity->backLerp;
		normalArray[numVertex][1] = curNormal[1] + (oldNormal[1] - curNormal[1]) * rb_entity->backLerp;
		normalArray[numVertex][2] = curNormal[2] + (oldNormal[2] - curNormal[2]) * rb_entity->backLerp;
		inTexCoordArray[numVertex][0] = st->st[0];
		inTexCoordArray[numVertex][1] = st->st[1];
		inColorArray[numVertex][0] = 255;
		inColorArray[numVertex][1] = 255;
		inColorArray[numVertex][2] = 255;
		inColorArray[numVertex][3] = 255;

		VectorNormalizeFast(tangentArray[numVertex]);
		VectorNormalizeFast(binormalArray[numVertex]);
		VectorNormalizeFast(normalArray[numVertex]);

		numVertex++;
	}

	// If this is an ammo display, calculate texture offsets
	if (rb_shader->flags & SHADER_AMMODISPLAY)
		R_CalcAmmoDisplayOffsets(rb_entity, surface);

	// Hack the front face to prevent view model from being culled
	if (rb_entity->renderFX & RF_CULLHACK)
		qglFrontFace(GL_CW);

	// Hack the depth range to prevent view model from poking into walls
	if (rb_entity->renderFX & RF_DEPTHHACK)
		qglDepthRange(0.0, 0.3);

	// Flush
	RB_RenderMesh();

	if (rb_entity->renderFX & RF_DEPTHHACK)
		qglDepthRange(0, 1);

	if (rb_entity->renderFX & RF_CULLHACK)
		qglFrontFace(GL_CCW);
}

/*
 =================
 R_AddAliasModelToList
 =================
*/
void R_AddAliasModelToList (entity_t *entity){

	int				i;
	mdl_t			*alias;
	mdlSurface_t	*surface;
	shader_t		*shader;

	// Select LOD model
	alias = R_GetAliasModelLOD(entity);

	if ((entity->frame < 0 || entity->frame >= alias->numFrames) || (entity->oldFrame < 0 || entity->oldFrame >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelToList: no such frame %i to %i (%s)\n", entity->frame, entity->oldFrame, entity->model->realName);
		entity->frame = 0;
		entity->oldFrame = 0;
	}

	// Add it to the shadow list
	R_AddShadowToList(entity, alias);

	// Cull
	if (R_CullAliasModel(entity, alias))
		return;

	// Add all the surfaces
	for (i = 0, surface = alias->surfaces; i < alias->numSurfaces; i++, surface++){
		// Select shader
		if (entity->customShader)
			shader = entity->customShader;
		else {
			if (!surface->numShaders){
				Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelToList: no shaders for surface (%s)\n", entity->model->realName);
				continue;
			}

			if (entity->skinNum < 0 || entity->skinNum >= surface->numShaders){
				Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelToList: no such shader %i (%s)\n", entity->skinNum, entity->model->realName);
				entity->skinNum = 0;
			}

			shader = surface->shaders[entity->skinNum].shader;

			// If this is an ammo display, remap the shader
			if (shader->flags & SHADER_AMMODISPLAY){
				shader = R_RemapAmmoDisplayShader(entity, surface);
				if (!shader)
					continue;	// Not visible
			}
		}

		// Add it
		R_AddMeshToList(MESH_ALIASMODEL, alias, shader, entity, i);
	}
}
