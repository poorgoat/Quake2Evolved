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


// r_world.c -- world-related code


#include "r_local.h"


/*
 =================
 R_CullSurface
 =================
*/
static qboolean R_CullSurface (surface_t *surface, const vec3_t origin, int clipFlags){

	material_t	*material = surface->texInfo->material;
	int			side;

	if (r_skipCulling->integerValue)
		return false;

	// Cull face
	if (material->cullType != CT_TWO_SIDED){
		side = PointOnPlaneSide(origin, 0.0, surface->plane);

		if (material->cullType == CT_BACK_SIDED){
			if (!(surface->flags & SURF_PLANEBACK)){
				if (side != SIDE_BACK)
					return true;
			}
			else {
				if (side != SIDE_FRONT)
					return true;
			}
		}
		else {
			if (!(surface->flags & SURF_PLANEBACK)){
				if (side != SIDE_FRONT)
					return true;
			}
			else {
				if (side != SIDE_BACK)
					return true;
			}
		}
	}

	// Cull bounds
	if (clipFlags){
		if (R_CullBox(surface->mins, surface->maxs, clipFlags))
			return true;
	}

	return false;
}

/*
 =================
 R_AddSurface
 =================
*/
static void R_AddSurface (surface_t *surface, renderEntity_t *entity){

	texInfo_t	*texInfo = surface->texInfo;
	material_t	*material;
	int			c;

	// Mark as visible for this view
	surface->viewCount = tr.viewCount;

	// Select material
	if (texInfo->next){
		c = entity->frame % texInfo->numFrames;
		while (c){
			texInfo = texInfo->next;
			c--;
		}
	}

	material = texInfo->material;

	// If it has a subview, add the surface
	if (material->subview != SUBVIEW_NONE){
		if (!r_skipSuppress->integerValue && tr.renderViewParms.subview != SUBVIEW_NONE)
			return;

		R_AddSubviewSurface(material, entity, surface);
	}

	tr.pc.surfaces++;

	// Add it
	R_AddMeshToList(MESH_SURFACE, surface, material, entity);

	// Also add caustics
	if (r_caustics->integerValue && entity == tr.worldEntity){
		if (surface->flags & SURF_IN_WATER)
			R_AddMeshToList(MESH_SURFACE, surface, tr.waterCausticsMaterial, entity);
		if (surface->flags & SURF_IN_SLIME)
			R_AddMeshToList(MESH_SURFACE, surface, tr.slimeCausticsMaterial, entity);
		if (surface->flags & SURF_IN_LAVA)
			R_AddMeshToList(MESH_SURFACE, surface, tr.lavaCausticsMaterial, entity);
	}
}


/*
 =======================================================================

 DECORATIONS (MAP MODELS)

 =======================================================================
*/


/*
 =================
 R_AddDecorations
 =================
*/
void R_AddDecorations (void){

	decSurface_t	*surface;
	leaf_t			*leaf;
	int				i, j;

	if (r_skipDecorations->integerValue)
		return;

	for (i = 0, surface = tr.worldModel->decorationSurfaces; i < tr.worldModel->numDecorationSurfaces; i++, surface++){
		if (!surface->material->numStages)
			continue;		// Don't bother drawing

		// Cull
		if (R_CullBox(surface->mins, surface->maxs, 31))
			continue;

		// Check the PVS
		if (!r_skipVisibility->integerValue){
			for (j = 0; j < surface->numLeafs; j++){
				leaf = surface->leafList[j];

				if (leaf->visCount != tr.visCount)
					continue;		// Not in PVS

				// Check for door connected areas
				if (!r_skipAreas->integerValue && tr.renderView.areaBits){
					if (!(tr.renderView.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
						continue;
				}

				break;
			}

			if (j == surface->numLeafs)
				continue;
		}

		// Mark as visible for this view
		surface->viewCount = tr.viewCount;

		// If it has a subview, add the surface
		if (surface->material->subview != SUBVIEW_NONE){
			if (!r_skipSuppress->integerValue && tr.renderViewParms.subview != SUBVIEW_NONE)
				return;

			R_AddSubviewSurface(surface->material, tr.worldEntity, surface);
		}

		tr.pc.surfaces++;

		// Add it
		R_AddMeshToList(MESH_DECORATION, surface, surface->material, tr.worldEntity);
	}
}


/*
 =======================================================================

 BRUSH MODELS

 =======================================================================
*/


/*
 =================
 R_AddBrushModel
 =================
*/
void R_AddBrushModel (renderEntity_t *entity){

	model_t		*model = entity->model;
	surface_t	*surface;
	vec3_t		origin, tmp;
	vec3_t		mins, maxs;
	int			i;

	if (!model->numModelSurfaces)
		return;

	// Cull
	if (!AxisCompare(entity->axis, axisDefault)){
		if (R_CullSphere(entity->origin, model->radius, 31))
			return;

		VectorSubtract(tr.renderView.viewOrigin, entity->origin, tmp);
		VectorRotate(tmp, entity->axis, origin);
	}
	else {
		VectorAdd(entity->origin, model->mins, mins);
		VectorAdd(entity->origin, model->maxs, maxs);

		if (R_CullBox(mins, maxs, 31))
			return;

		VectorSubtract(tr.renderView.viewOrigin, entity->origin, origin);
	}

	tr.pc.entities++;

	// Add all the surfaces
	surface = model->surfaces + model->firstModelSurface;
	for (i = 0; i < model->numModelSurfaces; i++, surface++){
		if (surface->texInfo->flags & SURF_SKY)
			continue;		// Don't bother drawing

		if (!surface->texInfo->material->numStages)
			continue;		// Don't bother drawing

		// Cull
		if (R_CullSurface(surface, origin, 0))
			continue;

		// Add the surface
		R_AddSurface(surface, entity);
	}
}


/*
 =======================================================================

 WORLD MODEL

 =======================================================================
*/


/*
 =================
 R_MarkLeaves

 Mark the leaves and nodes that are in the PVS for the current cluster
 =================
*/
static void R_MarkLeaves (void){

	byte	vis[MAX_MAP_LEAFS/8], vis2[MAX_MAP_LEAFS/8];
	node_t	*node;
	leaf_t	*leaf;
	vec3_t	viewOrigin;
	int		i, c, n;

	// Current view cluster
	if (tr.renderViewParms.subview == SUBVIEW_MIRROR){
		tr.oldViewCluster = tr.viewCluster;
		tr.viewCluster = -1;

		tr.oldViewCluster2 = tr.viewCluster2;
		tr.viewCluster2 = -1;
	}
	else {
		leaf = R_PointInLeaf(tr.renderView.viewOrigin);

		if (r_showCluster->integerValue)
			Com_Printf("Cluster: %i, Area: %i\n", leaf->cluster, leaf->area);

		tr.oldViewCluster = tr.viewCluster;
		tr.viewCluster = leaf->cluster;

		tr.oldViewCluster2 = tr.viewCluster2;
		tr.viewCluster2 = leaf->cluster;

		// Check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents){
			// Look down a bit
			VectorCopy(tr.renderView.viewOrigin, viewOrigin);
			viewOrigin[2] -= 16;

			leaf = R_PointInLeaf(viewOrigin);
			if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != tr.viewCluster2))
				tr.viewCluster2 = leaf->cluster;
		}
		else {
			// Look up a bit
			VectorCopy(tr.renderView.viewOrigin, viewOrigin);
			viewOrigin[2] += 16;

			leaf = R_PointInLeaf(viewOrigin);
			if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != tr.viewCluster2))
				tr.viewCluster2 = leaf->cluster;
		}
	}

	if (tr.viewCluster == tr.oldViewCluster && tr.viewCluster2 == tr.oldViewCluster2 && !r_skipVisibility->integerValue && tr.viewCluster != -1)
		return;

	// Development aid to let you run around and see exactly where the
	// PVS ends
	if (r_lockPVS->integerValue)
		return;

	tr.visCount++;
	tr.oldViewCluster = tr.viewCluster;
	tr.oldViewCluster2 = tr.viewCluster2;

	if (r_skipVisibility->integerValue || !tr.worldModel->vis || (tr.viewCluster == -1 && tr.renderViewParms.subview != SUBVIEW_MIRROR)){
		// Mark everything
		for (i = 0, leaf = tr.worldModel->leafs; i < tr.worldModel->numLeafs; i++, leaf++)
			leaf->visCount = tr.visCount;
		for (i = 0, node = tr.worldModel->nodes; i < tr.worldModel->numNodes; i++, node++)
			node->visCount = tr.visCount;

		return;
	}

	// Grab PVS
	if (tr.renderViewParms.subview == SUBVIEW_MIRROR){
		// Combine multiple clusters
		for (n = 0; n < tr.renderSubviewParms.numSurfaces; n++){
			// Offset the origin slightly
			VectorMA(tr.renderSubviewParms.surfaces[n]->origin, 0.5, tr.renderSubviewParms.planeNormal, viewOrigin);

			leaf = R_PointInLeaf(viewOrigin);

			if (n == 0){
				R_ClusterPVS(leaf->cluster, vis);
				continue;
			}

			// Combine with previous clusters
			R_ClusterPVS(leaf->cluster, vis2);

			c = (tr.worldModel->numLeafs+31)/32;
			for (i = 0; i < c; i++)
				((int *)vis)[i] |= ((int *)vis2)[i];
		}
	}
	else {
		// May have to combine two clusters because of solid water
		// boundaries
		R_ClusterPVS(tr.viewCluster, vis);
		if (tr.viewCluster != tr.viewCluster2){
			R_ClusterPVS(tr.viewCluster2, vis2);

			c = (tr.worldModel->numLeafs+31)/32;
			for (i = 0; i < c; i++)
				((int *)vis)[i] |= ((int *)vis2)[i];
		}
	}

	// Mark the leaves and nodes in the PVS
	for (i = 0, leaf = tr.worldModel->leafs; i < tr.worldModel->numLeafs; i++, leaf++){
		if (leaf->cluster == -1)
			continue;

		// Check the PVS
		if (!(vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))))
			continue;

		node = (node_t *)leaf;
		do {
			if (node->visCount == tr.visCount)
				break;
			node->visCount = tr.visCount;

			node = node->parent;
		} while (node);
	}
}

/*
 =================
 R_RecursiveWorldNode
 =================
*/
static void R_RecursiveWorldNode (node_t *node, int clipFlags){

	leaf_t		*leaf;
	surface_t	*surface, **mark;
	cplane_t	*plane;
	int			i, side;

	if (node->contents == CONTENTS_SOLID)
		return;		// Solid

	if (node->visCount != tr.visCount)
		return;		// Not in PVS

	// Cull
	if (clipFlags){
		for (i = 0, plane = tr.renderViewParms.frustum; i < 5; i++, plane++){
			if (!(clipFlags & (1<<i)))
				continue;

			side = BoxOnPlaneSide(node->mins, node->maxs, plane);

			if (side == SIDE_BACK)
				return;

			if (side == SIDE_FRONT)
				clipFlags &= ~(1<<i);
		}
	}

	// Mark as visible for this view
	node->viewCount = tr.viewCount;

	// Recurse down the children
	if (node->contents == -1){
		R_RecursiveWorldNode(node->children[0], clipFlags);
		R_RecursiveWorldNode(node->children[1], clipFlags);
		return;
	}

	// If a leaf node, draw stuff
	leaf = (leaf_t *)node;

	if (!leaf->numMarkSurfaces)
		return;

	// Check for door connected areas
	if (!r_skipAreas->integerValue && tr.renderView.areaBits){
		if (!(tr.renderView.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
			return;
	}

	// Add to visible mins/maxs
	AddPointToBounds(leaf->mins, tr.renderViewParms.visMins, tr.renderViewParms.visMaxs);
	AddPointToBounds(leaf->maxs, tr.renderViewParms.visMins, tr.renderViewParms.visMaxs);

	tr.pc.leafs++;

	// Add all the surfaces
	for (i = 0, mark = leaf->firstMarkSurface; i < leaf->numMarkSurfaces; i++, mark++){
		surface = *mark;

		if (surface->worldCount == tr.worldCount)
			continue;		// Already added this surface from another leaf
		surface->worldCount = tr.worldCount;

		if (!surface->texInfo->material->numStages)
			continue;		// Don't bother drawing

		// Cull
		if (R_CullSurface(surface, tr.renderView.viewOrigin, clipFlags))
			continue;

		// Add the surface
		R_AddSurface(surface, tr.worldEntity);
	}
}

/*
 =================
 R_AddWorld
 =================
*/
void R_AddWorld (void){

	if (!tr.renderViewParms.primaryView)
		return;

	// Bump world count
	tr.worldCount++;

	// Auto cycle the world frame for texture animation
	tr.worldEntity->frame = (int)(tr.renderView.time * 2);

	// Clear visible mins/maxs
	ClearBounds(tr.renderViewParms.visMins, tr.renderViewParms.visMaxs);

	// Mark leaves
	R_MarkLeaves();

	// Walk the BSP tree
	if (!r_skipCulling->integerValue)
		R_RecursiveWorldNode(tr.worldModel->nodes, 31);
	else
		R_RecursiveWorldNode(tr.worldModel->nodes, 0);

	// Add decorations
	R_AddDecorations();
}
