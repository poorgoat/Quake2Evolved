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


// r_alias.c -- alias model code


#include "r_local.h"


/*
 =================
 R_CullAliasModel
 =================
*/
static qboolean R_CullAliasModel (renderEntity_t *entity, mdl_t *alias){

	mdlFrame_t	*curFrame, *oldFrame;
	cplane_t	*plane;
	vec3_t		mins, maxs, corners[8], tmp;
	int			mask, aggregateMask = ~0;
	int			i, j;

	if (r_skipCulling->integerValue)
		return false;

	if (entity->renderFX & RF_WEAPONMODEL)
		return false;

	// Compute axially aligned mins and maxs
	curFrame = alias->frames + entity->frame;
	oldFrame = alias->frames + entity->oldFrame;

	if (curFrame == oldFrame){
		VectorCopy(curFrame->mins, mins);
		VectorCopy(curFrame->maxs, maxs);
	}
	else {
		VectorMin(curFrame->mins, oldFrame->mins, mins);
		VectorMax(curFrame->maxs, oldFrame->maxs, maxs);
	}

	// Compute the corners of the bounding volume
	for (i = 0; i < 8; i++){
		tmp[0] = (i & 1) ? mins[0] : maxs[0];
		tmp[1] = (i & 2) ? mins[1] : maxs[1];
		tmp[2] = (i & 4) ? mins[2] : maxs[2];

		// Rotate and translate
		VectorRotate(tmp, entity->axis, corners[i]);
		corners[i][1] = -corners[i][1];
		VectorAdd(corners[i], entity->origin, corners[i]);
	}

	// Cull
	for (i = 0; i < 8; i++){
		mask = 0;

		for (j = 0, plane = tr.renderViewParms.frustum; j < 5; j++, plane++){
			if (DotProduct(corners[i], plane->normal) < plane->dist)
				mask |= (1<<j);
		}

		aggregateMask &= mask;
	}

	if (aggregateMask){
		tr.pc.boxOut++;
		return true;
	}

	tr.pc.boxIn++;

	return false;
}

/*
 =================
 R_AddAliasModel
 =================
*/
void R_AddAliasModel (renderEntity_t *entity){

	mdl_t			*alias = entity->model->alias;
	mdlSurface_t	*surface;
	material_t		*material;
	int				i;

	if (entity->renderFX & RF_VIEWERMODEL){
		if (!r_skipSuppress->integerValue && tr.renderViewParms.subview == SUBVIEW_NONE)
			return;
	}
	if (entity->renderFX & RF_WEAPONMODEL){
		if (!r_skipSuppress->integerValue && tr.renderViewParms.subview != SUBVIEW_NONE)
			return;
	}

	if ((entity->frame < 0 || entity->frame >= alias->numFrames) || (entity->oldFrame < 0 || entity->oldFrame >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModel: no such frame %i to %i (%s)\n", entity->frame, entity->oldFrame, entity->model->realName);

		entity->frame = 0;
		entity->oldFrame = 0;
	}

	// Cull
	if (R_CullAliasModel(entity, alias))
		return;

	tr.pc.entities++;

	// Add all the surfaces
	for (i = 0, surface = alias->surfaces; i < alias->numSurfaces; i++, surface++){
		// Select material
		if (entity->customMaterial)
			material = entity->customMaterial;
		else {
			if (surface->numMaterials){
				if (entity->skinIndex < 0 || entity->skinIndex >= surface->numMaterials){
					Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModel: no such material %i (%s)\n", entity->skinIndex, entity->model->realName);

					entity->skinIndex = 0;
				}

				material = surface->materials[entity->skinIndex].material;
			}
			else {
				Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModel: no materials for surface (%s)\n", entity->model->realName);

				material = tr.defaultMaterial;
			}
		}

		if (!material->numStages)
			continue;

		// If it has a subview, add the surface
		if (material->subview != SUBVIEW_NONE){
			if (!r_skipSuppress->integerValue && tr.renderViewParms.subview != SUBVIEW_NONE)
				continue;

			R_AddSubviewSurface(material, entity, NULL);
		}

		// Add the surface
		R_AddMeshToList(MESH_ALIASMODEL, surface, material, entity);
	}
}
