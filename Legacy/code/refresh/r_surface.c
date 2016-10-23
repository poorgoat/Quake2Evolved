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


// r_surface.c -- surface-related refresh code


#include "r_local.h"


#define BACKFACE_EPSILON	0.01

model_t		*r_worldModel;
entity_t	*r_worldEntity;

vec3_t		r_worldMins, r_worldMaxs;

int			r_frameCount;
int			r_visFrameCount;

int			r_viewCluster, r_viewCluster2;
int			r_oldViewCluster, r_oldViewCluster2;


/*
 =================
 R_DrawSurface
 =================
*/
void R_DrawSurface (void){

	surface_t		*surf = rb_mesh->mesh;
	surfPoly_t		*p;
	surfPolyVert_t	*v;
	int				i;

	for (p = surf->poly; p; p = p->next){
		RB_CheckMeshOverflow(p->numIndices, p->numVertices);

		for (i = 0; i < p->numIndices; i += 3){
			indexArray[numIndex++] = numVertex + p->indices[i+0];
			indexArray[numIndex++] = numVertex + p->indices[i+1];
			indexArray[numIndex++] = numVertex + p->indices[i+2];
		}

		for (i = 0, v = p->vertices; i < p->numVertices; i++, v++){
			vertexArray[numVertex][0] = v->xyz[0];
			vertexArray[numVertex][1] = v->xyz[1];
			vertexArray[numVertex][2] = v->xyz[2];
			tangentArray[numVertex][0] = surf->tangent[0];
			tangentArray[numVertex][1] = surf->tangent[1];
			tangentArray[numVertex][2] = surf->tangent[2];
			binormalArray[numVertex][0] = surf->binormal[0];
			binormalArray[numVertex][1] = surf->binormal[1];
			binormalArray[numVertex][2] = surf->binormal[2];
			normalArray[numVertex][0] = surf->normal[0];
			normalArray[numVertex][1] = surf->normal[1];
			normalArray[numVertex][2] = surf->normal[2];
			inTexCoordArray[numVertex][0] = v->st[0];
			inTexCoordArray[numVertex][1] = v->st[1];
			inTexCoordArray[numVertex][2] = v->lightmap[0];
			inTexCoordArray[numVertex][3] = v->lightmap[1];
			inColorArray[numVertex][0] = v->color[0];
			inColorArray[numVertex][1] = v->color[1];
			inColorArray[numVertex][2] = v->color[2];
			inColorArray[numVertex][3] = v->color[3];

			numVertex++;
		}
	}
}

/*
 =================
 R_CullSurface
 =================
*/
static qboolean R_CullSurface (surface_t *surf, const vec3_t origin, int clipFlags){

	cplane_t	*plane;
	float		dist;

	if (r_noCull->integer)
		return false;

	// Find which side of the node we are on
	plane = surf->plane;
	if (plane->type < 3)
		dist = origin[plane->type] - plane->dist;
	else
		dist = DotProduct(origin, plane->normal) - plane->dist;

	if (!(surf->flags & SURF_PLANEBACK)){
		if (dist <= BACKFACE_EPSILON)
			return true;	// Wrong side
	}
	else {
		if (dist >= -BACKFACE_EPSILON)
			return true;	// Wrong side
	}

	// Cull
	if (clipFlags){
		if (R_CullBox(surf->mins, surf->maxs, clipFlags))
			return true;
	}

	return false;
}

/*
 =================
 R_AddSurfaceToList
 =================
*/
static void R_AddSurfaceToList (surface_t *surf, entity_t *entity){

	texInfo_t	*tex = surf->texInfo;
	shader_t	*shader;
	int			c, map, lmNum;

	if (tex->flags & SURF_NODRAW)
		return;

	// Select shader
	if (tex->next){
		c = entity->frame % tex->numFrames;
		while (c){
			tex = tex->next;
			c--;
		}
	}

	shader = tex->shader;

	// Select lightmap
	lmNum = surf->lmNum;

	// Check for lightmap modification
	if (r_dynamicLights->integer && (shader->flags & SHADER_HASLIGHTMAP)){
		if (surf->dlightFrame == r_frameCount)
			lmNum = 255;
		else {
			for (map = 0; map < surf->numStyles; map++){
				if (surf->cachedLight[map] != r_lightStyles[surf->styles[map]].white){
					lmNum = 255;
					break;
				}
			}
		}
	}

	// Add it
	R_AddMeshToList(MESH_SURFACE, surf, shader, entity, lmNum);

	// Also add caustics
	if (r_caustics->integer){
		if (surf->flags & SURF_WATERCAUSTICS)
			R_AddMeshToList(MESH_SURFACE, surf, r_waterCausticsShader, entity, 0);
		if (surf->flags & SURF_SLIMECAUSTICS)
			R_AddMeshToList(MESH_SURFACE, surf, r_slimeCausticsShader, entity, 0);
		if (surf->flags & SURF_LAVACAUSTICS)
			R_AddMeshToList(MESH_SURFACE, surf, r_lavaCausticsShader, entity, 0);
	}
}


/*
 =======================================================================

 BRUSH MODELS

 =======================================================================
*/


/*
 =================
 R_AddBrushModelToList
 =================
*/
void R_AddBrushModelToList (entity_t *entity){

	model_t		*model = entity->model;
	surface_t	*surf;
	dlight_t	*dl;
	vec3_t		origin, tmp;
	vec3_t		mins, maxs;
	int			i, l;

	if (!model->numModelSurfaces)
		return;

	// Cull
	if (!AxisCompare(entity->axis, axisDefault)){
		for (i = 0; i < 3; i++){
			mins[i] = entity->origin[i] - model->radius;
			maxs[i] = entity->origin[i] + model->radius;
		}

		if (R_CullSphere(entity->origin, model->radius, 15))
			return;

		VectorSubtract(r_refDef.viewOrigin, entity->origin, tmp);
		VectorRotate(tmp, entity->axis, origin);
	}
	else {
		VectorAdd(entity->origin, model->mins, mins);
		VectorAdd(entity->origin, model->maxs, maxs);

		if (R_CullBox(mins, maxs, 15))
			return;

		VectorSubtract(r_refDef.viewOrigin, entity->origin, origin);
	}

	// Calculate dynamic lighting
	if (r_dynamicLights->integer){
		for (l = 0, dl = r_dlights; l < r_numDLights; l++, dl++){
			if (!BoundsAndSphereIntersect(mins, maxs, dl->origin, dl->intensity))
				continue;

			surf = model->surfaces + model->firstModelSurface;
			for (i = 0; i < model->numModelSurfaces; i++, surf++){
				if (surf->dlightFrame != r_frameCount){
					surf->dlightFrame = r_frameCount;
					surf->dlightBits = (1<<l);
				}
				else
					surf->dlightBits |= (1<<l);
			}
		}
	}

	// Add all the surfaces
	surf = model->surfaces + model->firstModelSurface;
	for (i = 0; i < model->numModelSurfaces; i++, surf++){
		// Cull
		if (R_CullSurface(surf, origin, 0))
			continue;

		// Add the surface
		R_AddSurfaceToList(surf, entity);
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

	byte	*vis, fatVis[MAX_MAP_LEAFS/8];
	node_t	*node;
	leaf_t	*leaf;
	vec3_t	tmp;
	int		i, c;

	// Current view cluster
	r_oldViewCluster = r_viewCluster;
	r_oldViewCluster2 = r_viewCluster2;

	leaf = R_PointInLeaf(r_refDef.viewOrigin);

	if (r_showCluster->integer)
		Com_Printf("Cluster: %i, Area: %i\n", leaf->cluster, leaf->area);

	r_viewCluster = r_viewCluster2 = leaf->cluster;

	// Check above and below so crossing solid water doesn't draw wrong
	if (!leaf->contents){
		// Look down a bit
		VectorCopy(r_refDef.viewOrigin, tmp);
		tmp[2] -= 16;
		leaf = R_PointInLeaf(tmp);
		if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != r_viewCluster2))
			r_viewCluster2 = leaf->cluster;
	}
	else {
		// Look up a bit
		VectorCopy(r_refDef.viewOrigin, tmp);
		tmp[2] += 16;
		leaf = R_PointInLeaf(tmp);
		if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != r_viewCluster2))
			r_viewCluster2 = leaf->cluster;
	}

	if (r_viewCluster == r_oldViewCluster && r_viewCluster2 == r_oldViewCluster2 && !r_noVis->integer && r_viewCluster != -1)
		return;

	// Development aid to let you run around and see exactly where the
	// PVS ends
	if (r_lockPVS->integer)
		return;

	r_visFrameCount++;
	r_oldViewCluster = r_viewCluster;
	r_oldViewCluster2 = r_viewCluster2;

	if (r_noVis->integer || r_viewCluster == -1 || !r_worldModel->vis){
		// Mark everything
		for (i = 0, leaf = r_worldModel->leafs; i < r_worldModel->numLeafs; i++, leaf++)
			leaf->visFrame = r_visFrameCount;
		for (i = 0, node = r_worldModel->nodes; i < r_worldModel->numNodes; i++, node++)
			node->visFrame = r_visFrameCount;

		return;
	}

	// May have to combine two clusters because of solid water 
	// boundaries
	vis = R_ClusterPVS(r_viewCluster);
	if (r_viewCluster != r_viewCluster2){
		memcpy(fatVis, vis, (r_worldModel->numLeafs+7)/8);
		vis = R_ClusterPVS(r_viewCluster2);
		c = (r_worldModel->numLeafs+31)/32;
		for (i = 0; i < c; i++)
			((int *)fatVis)[i] |= ((int *)vis)[i];

		vis = fatVis;
	}
	
	for (i = 0, leaf = r_worldModel->leafs; i < r_worldModel->numLeafs; i++, leaf++){
		if (leaf->cluster == -1)
			continue;

		if (!(vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))))
			continue;

		node = (node_t *)leaf;
		do {
			if (node->visFrame == r_visFrameCount)
				break;
			node->visFrame = r_visFrameCount;

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
	surface_t	*surf, **mark;
	cplane_t	*plane;
	int			clipped;
	int			i;

	if (node->contents == CONTENTS_SOLID)
		return;		// Solid

	if (node->visFrame != r_visFrameCount)
		return;

	// Cull
	if (clipFlags){
		for (i = 0, plane = r_frustum; i < 4; i++, plane++){
			if (!(clipFlags & (1<<i)))
				continue;

			clipped = BoxOnPlaneSide(node->mins, node->maxs, plane);
			if (clipped == 2)
				return;

			if (clipped == 1)
				clipFlags &= ~(1<<i);
		}
	}

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
	if (r_refDef.areaBits){
		if (!(r_refDef.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
			return;		// Not visible
	}

	// Add to world mins/maxs
	AddPointToBounds(leaf->mins, r_worldMins, r_worldMaxs);
	AddPointToBounds(leaf->maxs, r_worldMins, r_worldMaxs);

	r_stats.numLeafs++;

	// Add all the surfaces
	for (i = 0, mark = leaf->firstMarkSurface; i < leaf->numMarkSurfaces; i++, mark++){
		surf = *mark;

		if (surf->visFrame == r_frameCount)
			continue;	// Already added this surface from another leaf
		surf->visFrame = r_frameCount;

		// Cull
		if (R_CullSurface(surf, r_refDef.viewOrigin, clipFlags))
			continue;

		// Clip sky surfaces
		if (surf->texInfo->flags & SURF_SKY){
			R_ClipSkySurface(surf);
			continue;
		}

		// Add the surface
		R_AddSurfaceToList(surf, r_worldEntity);
	}
}

/*
 =================
 R_AddWorldToList
 =================
*/
void R_AddWorldToList (void){

	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		return;

	if (!r_drawWorld->integer)
		return;

	// Bump frame count
	r_frameCount++;

	// Auto cycle the world frame for texture animation
	r_worldEntity->frame = (int)(r_refDef.time * 2);

	// Clear world mins/maxs
	ClearBounds(r_worldMins, r_worldMaxs);

	R_MarkLeaves();
	R_MarkLights();

	R_ClearSky();

	if (r_noCull->integer)
		R_RecursiveWorldNode(r_worldModel->nodes, 0);
	else
		R_RecursiveWorldNode(r_worldModel->nodes, 15);

	R_AddSkyToList();
}
