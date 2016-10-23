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


/*
 =================
 R_QSortMeshes
 =================
*/
static void R_QSortMeshes (mesh_t *meshes, int numMeshes){

	mesh_t		tmp;
	int			stack[4096];
	int			stackDepth = 0;
	int			L, R, l, r, median;
	unsigned	pivot;

	if (numMeshes < 2)
		return;

	L = 0;
	R = numMeshes - 1;

start:
	l = L;
	r = R;

	median = (L + R) >> 1;

	if (meshes[L].sort > meshes[median].sort){
		if (meshes[L].sort < meshes[R].sort) 
			median = L;
	} 
	else if (meshes[R].sort < meshes[median].sort)
		median = R;

	pivot = meshes[median].sort;

	while (l < r){
		while (meshes[l].sort < pivot)
			l++;
		while (meshes[r].sort > pivot)
			r--;

		if (l <= r){
			tmp = meshes[r];
			meshes[r] = meshes[l];
			meshes[l] = tmp;

			l++;
			r--;
		}
	}

	if ((L < r) && (stackDepth < 4096)){
		stack[stackDepth++] = l;
		stack[stackDepth++] = R;
		R = r;
		goto start;
	}

	if (l < R){
		L = l;
		goto start;
	}

	if (stackDepth){
		R = stack[--stackDepth];
		L = stack[--stackDepth];
		goto start;
	}
}

/*
 =================
 R_SortMeshes
 =================
*/
void R_SortMeshes (void){

	viewData_t	*viewData = &tr.viewData;
	light_t		*light;
	int			i;

	R_QSortMeshes(&viewData->meshes[viewData->firstMesh], viewData->numMeshes - viewData->firstMesh);
	R_QSortMeshes(&viewData->postProcessMeshes[viewData->firstPostProcessMesh], viewData->numPostProcessMeshes - viewData->firstPostProcessMesh);

	for (i = viewData->firstLight, light = &viewData->lights[viewData->firstLight]; i < viewData->numLights; i++, light++){
		R_QSortMeshes(light->shadows[0], light->numShadows[0]);
		R_QSortMeshes(light->shadows[1], light->numShadows[1]);

		R_QSortMeshes(light->interactions[0], light->numInteractions[0]);
		R_QSortMeshes(light->interactions[1], light->numInteractions[1]);
	}

	for (i = viewData->firstFogLight, light = &viewData->fogLights[viewData->firstFogLight]; i < viewData->numFogLights; i++, light++){
		R_QSortMeshes(light->shadows[0], light->numShadows[0]);
		R_QSortMeshes(light->interactions[0], light->numInteractions[0]);
	}
}

/*
 =================
 R_AddMeshToList
 =================
*/
void R_AddMeshToList (meshType_t type, meshData_t *data, material_t *material, renderEntity_t *entity){

	viewData_t	*viewData = &tr.viewData;
	mesh_t		*mesh;

	if (material->sort != SORT_POST_PROCESS){
		if (viewData->numMeshes == MAX_MESHES)
			Com_Error(ERR_DROP, "R_AddMeshToList: MAX_MESHES hit");

		mesh = &viewData->meshes[viewData->numMeshes++];
	}
	else {
		if (viewData->numPostProcessMeshes == MAX_POST_PROCESS_MESHES)
			Com_Error(ERR_DROP, "R_AddMeshToList: MAX_POST_PROCESS_MESHES hit");

		mesh = &viewData->postProcessMeshes[viewData->numPostProcessMeshes++];
	}

	mesh->sort = (material->index << 20) | (entity->index << 9) | (type << 5);
	mesh->data = data;
}

/*
 =================
 R_DecomposeSort
 =================
*/
void R_DecomposeSort (unsigned sort, material_t **material, renderEntity_t **entity, meshType_t *type, qboolean *caps){

	*material = tr.sortedMaterials[(sort >> 20) & (MAX_MATERIALS-1)];
	*entity = &tr.scene.entities[(sort >> 9) & (MAX_RENDER_ENTITIES-1)];
	*type = (sort >> 5) & 15;
	*caps = sort & 1;
}
