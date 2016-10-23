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


// r_model.c -- model loading and caching


#include "r_local.h"


// The inline models from the current map are kept separate
static model_t		r_inlineModels[MAX_MODELS];

static model_t		*r_modelsHashTable[MODELS_HASH_SIZE];
static model_t		*r_models[MAX_MODELS];
static int			r_numModels;


/*
 =================
 R_PointInLeaf
 =================
*/
leaf_t *R_PointInLeaf (const vec3_t point){

	node_t	*node;
	int		side;

	if (!tr.worldModel || !tr.worldModel->nodes)
		Com_Error(ERR_DROP, "R_PointInLeaf: NULL worldModel");

	node = tr.worldModel->nodes;
	while (1){
		if (node->contents != -1)
			return (leaf_t *)node;

		side = PointOnPlaneSide(point, 0.0, node->plane);

		if (side == SIDE_FRONT)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return NULL;	// Never reached
}

/*
 =================
 R_BoxInLeaves
 =================
*/
int R_BoxInLeaves (node_t *node, const vec3_t mins, const vec3_t maxs, leaf_t **leafList, int numLeafs, int maxLeafs){

	int		side;

	if (node->contents != -1){
		if (numLeafs >= maxLeafs)
			return numLeafs;

		leafList[numLeafs++] = (leaf_t *)node;
		return numLeafs;
	}

	side = BoxOnPlaneSide(mins, maxs, node->plane);

	if (side == SIDE_FRONT)
		return R_BoxInLeaves(node->children[0], mins, maxs, leafList, numLeafs, maxLeafs);
	if (side == SIDE_BACK)
		return R_BoxInLeaves(node->children[1], mins, maxs, leafList, numLeafs, maxLeafs);

	numLeafs = R_BoxInLeaves(node->children[0], mins, maxs, leafList, numLeafs, maxLeafs);
	numLeafs = R_BoxInLeaves(node->children[1], mins, maxs, leafList, numLeafs, maxLeafs);

	return numLeafs;
}

/*
 =================
 R_SphereInLeaves
 =================
*/
int R_SphereInLeaves (node_t *node, const vec3_t center, float radius, leaf_t **leafList, int numLeafs, int maxLeafs){

	int		side;

	if (node->contents != -1){
		if (numLeafs >= maxLeafs)
			return numLeafs;

		leafList[numLeafs++] = (leaf_t *)node;
		return numLeafs;
	}

	side = SphereOnPlaneSide(center, radius, node->plane);

	if (side == SIDE_FRONT)
		return R_SphereInLeaves(node->children[0], center, radius, leafList, numLeafs, maxLeafs);
	if (side == SIDE_BACK)
		return R_SphereInLeaves(node->children[1], center, radius, leafList, numLeafs, maxLeafs);

	numLeafs = R_SphereInLeaves(node->children[0], center, radius, leafList, numLeafs, maxLeafs);
	numLeafs = R_SphereInLeaves(node->children[1], center, radius, leafList, numLeafs, maxLeafs);

	return numLeafs;
}

/*
 =================
 R_DecompressVis
 =================
*/
static void R_DecompressVis (const byte *in, byte *out){

	byte	*vis;
	int		c, row;

	row = (tr.worldModel->vis->numClusters+7)>>3;
	vis = out;

	if (!in){
		// No vis info, so make all visible
		while (row){
			*vis++ = 0xFF;
			row--;
		}

		return;
	}

	do {
		if (*in){
			*vis++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c){
			*vis++ = 0;
			c--;
		}
	} while (vis - out < row);
}

/*
 =================
 R_ClusterPVS
 =================
*/
void R_ClusterPVS (int cluster, byte *vis){

	if (cluster == -1 || !tr.worldModel || !tr.worldModel->vis){
		memset(vis, 0xFF, MAX_MAP_LEAFS/8);
		return;
	}

	R_DecompressVis((byte *)tr.worldModel->vis + tr.worldModel->vis->bitOfs[cluster][VIS_PVS], vis);
}


/*
 =======================================================================

 BRUSH MODELS

 =======================================================================
*/


/*
 =================
 R_LoadVertexes
 =================
*/
static void R_LoadVertexes (const byte *data, const lump_t *l){

	dvertex_t	*in;
	vertex_t	*out;
	int			i;

	in = (dvertex_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dvertex_t))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", tr.worldModel->realName);

	tr.worldModel->numVertexes = l->fileLen / sizeof(dvertex_t);
	tr.worldModel->vertexes = out = Hunk_Alloc(tr.worldModel->numVertexes * sizeof(vertex_t));
	tr.worldModel->size += tr.worldModel->numVertexes * sizeof(vertex_t);

	for (i = 0; i < tr.worldModel->numVertexes; i++, in++, out++){
		out->point[0] = LittleFloat(in->point[0]);
		out->point[1] = LittleFloat(in->point[1]);
		out->point[2] = LittleFloat(in->point[2]);
	}
}

/*
 =================
 R_LoadEdges
 =================
*/
static void R_LoadEdges (const byte *data, const lump_t *l){

	dedge_t	*in;
	edge_t	*out;
	int 	i;

	in = (dedge_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dedge_t))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", tr.worldModel->realName);

	tr.worldModel->numEdges = l->fileLen / sizeof(dedge_t);
	tr.worldModel->edges = out = Hunk_Alloc(tr.worldModel->numEdges * sizeof(edge_t));
	tr.worldModel->size += tr.worldModel->numEdges * sizeof(edge_t);

	for (i = 0; i < tr.worldModel->numEdges; i++, in++, out++){
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
 =================
 R_LoadSurfEdges
 =================
*/
static void R_LoadSurfEdges (const byte *data, const lump_t *l){

	int		*in, *out;
	int		i;
	
	in = (int *)(data + l->fileOfs);
	if (l->fileLen % sizeof(int))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", tr.worldModel->realName);

	tr.worldModel->numSurfEdges = l->fileLen / sizeof(int);
	tr.worldModel->surfEdges = out = Hunk_Alloc(tr.worldModel->numSurfEdges * sizeof(int));
	tr.worldModel->size += tr.worldModel->numSurfEdges * sizeof(int);

	for (i = 0; i < tr.worldModel->numSurfEdges; i++)
		out[i] = LittleLong(in[i]);
}

/*
 =================
 R_LoadPlanes
 =================
*/
static void R_LoadPlanes (const byte *data, const lump_t *l){

	dplane_t	*in;
	cplane_t	*out;
	int			i;
	
	in = (dplane_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dplane_t))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", tr.worldModel->realName);

	tr.worldModel->numPlanes = l->fileLen / sizeof(dplane_t);
	tr.worldModel->planes = out = Hunk_Alloc(tr.worldModel->numPlanes * sizeof(cplane_t));
	tr.worldModel->size += tr.worldModel->numPlanes * sizeof(cplane_t);

	for (i = 0; i < tr.worldModel->numPlanes; i++, in++, out++){
		out->normal[0] = LittleFloat(in->normal[0]);
		out->normal[1] = LittleFloat(in->normal[1]);
		out->normal[2] = LittleFloat(in->normal[2]);

		out->dist = LittleFloat(in->dist);
		out->type = PlaneTypeForNormal(out->normal);
		SetPlaneSignbits(out);
	}
}

/*
 =================
 R_GetTexSize
 =================
*/
static void R_GetTexSize (texInfo_t *texInfo){

	material_t		*material = texInfo->material;
	stage_t			*stage;
	char			name[MAX_OSPATH];
	mipTex_t		mt;
	fileHandle_t	f;
	int				i;

	// Look for a .wal texture first. This is so that retextures work.
	Q_snprintfz(name, sizeof(name), "%s.wal", material->name);
	FS_OpenFile(name, &f, FS_READ);
	if (f){
		// Found it, use its dimensions
		FS_Read(&mt, sizeof(mipTex_t), f);
		FS_CloseFile(f);

		texInfo->width = LittleLong(mt.width);
		texInfo->height = LittleLong(mt.height);
		return;
	}

	// No, so look for the first texture stage in the material
	for (i = 0, stage = material->stages; i < material->numStages; i++, stage++){
		if (stage->programs)
			continue;

		if (stage->textureStage.texture->flags & TF_INTERNAL)
			continue;

		// Found it, use its dimensions
		texInfo->width = stage->textureStage.texture->sourceWidth;
		texInfo->height = stage->textureStage.texture->sourceHeight;
		return;
	}

	// Couldn't find shit, so just default to 64x64
	texInfo->width = 64;
	texInfo->height = 64;
}

/*
 =================
 R_CheckTexClamp
 =================
*/
static void R_CheckTexClamp (texInfo_t *texInfo){

	material_t	*material = texInfo->material;
	stage_t		*stage;
	int			i;

	// If at least one stage uses any form of texture clamping, then all
	// surfaces using this material will need their texture coordinates
	// to be clamped to the 0.0 - 1.0 range for things to work properly
	for (i = 0, stage = material->stages; i < material->numStages; i++, stage++){
		if (stage->programs)
			continue;

		if (stage->textureStage.texture->isCubeMap)
			continue;

		if (stage->textureStage.texture->wrap != TW_REPEAT){
			texInfo->clamp = true;
			return;
		}
	}

	// No need to clamp texture coordinates
	texInfo->clamp = false;
}

/*
 =================
 R_LoadTexInfo
 =================
*/
static void R_LoadTexInfo (const byte *data, const lump_t *l){

	dtexinfo_t	*in;
	texInfo_t	*out, *step;
	int 		i, j;
	int			next;
	char		name[MAX_OSPATH];
	unsigned	surfaceParm;

	in = (dtexinfo_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dtexinfo_t))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", tr.worldModel->realName);

	tr.worldModel->numTexInfo = l->fileLen / sizeof(dtexinfo_t);
	tr.worldModel->texInfo = out = Hunk_Alloc(tr.worldModel->numTexInfo * sizeof(texInfo_t));
	tr.worldModel->size += tr.worldModel->numTexInfo * sizeof(texInfo_t);

	for (i = 0; i < tr.worldModel->numTexInfo; i++, in++, out++){
		out->flags = LittleLong(in->flags);

		for (j = 0; j < 4; j++){
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		out->numFrames = 1;

		next = LittleLong(in->nextTexInfo);
		if (next > 0)
			out->next = tr.worldModel->texInfo + next;
		else
			out->next = NULL;

		// Special case for sky surfaces
		if (out->flags & SURF_SKY){
			out->material = tr.worldModel->sky->material;
			out->width = 64;
			out->height = 64;
			out->clamp = false;

			continue;
		}

		// Special case for no-draw surfaces
		if (out->flags & SURF_NODRAW){
			out->material = tr.noDrawMaterial;
			out->width = 64;
			out->height = 64;
			out->clamp = false;

			continue;
		}

		// Get surfaceParm
		if (out->flags & (SURF_WARP | SURF_TRANS33 | SURF_TRANS66)){
			surfaceParm = 0;

			if (out->flags & SURF_WARP)
				surfaceParm |= SURFACEPARM_WARP;
			if (out->flags & SURF_TRANS33)
				surfaceParm |= SURFACEPARM_TRANS33;
			if (out->flags & SURF_TRANS66)
				surfaceParm |= SURFACEPARM_TRANS66;
			if (out->flags & SURF_FLOWING)
				surfaceParm |= SURFACEPARM_FLOWING;
		}
		else {
			surfaceParm = SURFACEPARM_LIGHTING;

			if (out->flags & SURF_FLOWING)
				surfaceParm |= SURFACEPARM_FLOWING;
		}

		// Load the material
		Q_snprintfz(name, sizeof(name), "textures/%s", in->texture);
		out->material = R_FindMaterial(name, MT_GENERIC, surfaceParm);

		// Find texture dimensions
		R_GetTexSize(out);

		// Check if surfaces will need clamping
		R_CheckTexClamp(out);
	}

	// Count animation frames
	for (i = 0, out = tr.worldModel->texInfo; i < tr.worldModel->numTexInfo; i++, out++){
		for (step = out->next; step && step != out; step = step->next)
			out->numFrames++;
	}
}

/*
 =================
 R_CalcSurfaceOriginAndBounds

 Fills in surface origin and mins/maxs
 =================
*/
static void R_CalcSurfaceOriginAndBounds (surface_t *surface){

	vertex_t	*v;
	int			i, e;

	VectorClear(surface->origin);
	ClearBounds(surface->mins, surface->maxs);

	for (i = 0; i < surface->numEdges; i++){
		e = tr.worldModel->surfEdges[surface->firstEdge + i];
		if (e >= 0)
			v = &tr.worldModel->vertexes[tr.worldModel->edges[e].v[0]];
		else
			v = &tr.worldModel->vertexes[tr.worldModel->edges[-e].v[1]];

		VectorAdd(surface->origin, v->point, surface->origin);
		AddPointToBounds(v->point, surface->mins, surface->maxs);
	}

	VectorScale(surface->origin, 1.0 / surface->numEdges, surface->origin);
}

/*
 =================
 R_FindSurfaceTriangleWithEdge
 =================
*/
static int R_FindSurfaceTriangleWithEdge (int numTriangles, surfTriangle_t *triangles, index_t start, index_t end, int ignore){

	surfTriangle_t	*triangle;
	int				count, match;
	int				i;

	count = 0;
	match = -1;

	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		if ((triangle->index[0] == start && triangle->index[1] == end) || (triangle->index[1] == start && triangle->index[2] == end) || (triangle->index[2] == start && triangle->index[0] == end)){
			if (i != ignore)
				match = i;

			count++;
		}
		else if ((triangle->index[1] == start && triangle->index[0] == end) || (triangle->index[2] == start && triangle->index[1] == end) || (triangle->index[0] == start && triangle->index[2] == end))
			count++;
	}

	// Detect edges shared by three triangles and make them seams
	if (count > 2)
		match = -1;

	return match;
}

/*
 =================
 R_BuildSurfaceTriangleNeighbors
 =================
*/
static void R_BuildSurfaceTriangleNeighbors (int numTriangles, surfTriangle_t *triangles){

	surfTriangle_t	*triangle;
	int				i;

	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		triangle->neighbor[0] = R_FindSurfaceTriangleWithEdge(numTriangles, triangles, triangle->index[1], triangle->index[0], i);
		triangle->neighbor[1] = R_FindSurfaceTriangleWithEdge(numTriangles, triangles, triangle->index[2], triangle->index[1], i);
		triangle->neighbor[2] = R_FindSurfaceTriangleWithEdge(numTriangles, triangles, triangle->index[0], triangle->index[2], i);
	}
}

/*
 =================
 R_BuildSurfacePolygon
 =================
*/
static void R_BuildSurfacePolygon (surface_t *surface){

	surfTriangle_t	*triangle;
	surfVertex_t	*vertex;
	texInfo_t		*texInfo = surface->texInfo;
	vertex_t		*v;
	int				i, e;

	// Create triangles
	surface->numTriangles = (surface->numEdges - 2);
	surface->triangles = Hunk_Alloc(surface->numTriangles * sizeof(surfTriangle_t));

	tr.worldModel->size += surface->numTriangles * sizeof(surfTriangle_t);

	for (i = 2, triangle = surface->triangles; i < surface->numEdges; i++, triangle++){
		triangle->index[0] = 0;
		triangle->index[1] = i-1;
		triangle->index[2] = i;
	}

	// Create vertices
	surface->numVertices = surface->numEdges;
	surface->vertices = Hunk_Alloc(surface->numVertices * sizeof(surfVertex_t));

	tr.worldModel->size += surface->numVertices * sizeof(surfVertex_t);

	for (i = 0, vertex = surface->vertices; i < surface->numEdges; i++, vertex++){
		// Vertex
		e = tr.worldModel->surfEdges[surface->firstEdge + i];
		if (e >= 0)
			v = &tr.worldModel->vertexes[tr.worldModel->edges[e].v[0]];
		else
			v = &tr.worldModel->vertexes[tr.worldModel->edges[-e].v[1]];

		VectorCopy(v->point, vertex->xyz);

		// Normal
		if (!(surface->flags & SURF_PLANEBACK))
			VectorCopy(surface->plane->normal, vertex->normal);
		else
			VectorNegate(surface->plane->normal, vertex->normal);

		// Tangents
		VectorNormalize2(texInfo->vecs[0], vertex->tangents[0]);
		VectorNormalize2(texInfo->vecs[1], vertex->tangents[1]);

		// Texture coordinates
		vertex->st[0] = (DotProduct(v->point, texInfo->vecs[0]) + texInfo->vecs[0][3]) / texInfo->width;
		vertex->st[1] = (DotProduct(v->point, texInfo->vecs[1]) + texInfo->vecs[1][3]) / texInfo->height;

		// Color
		if (texInfo->flags & SURF_TRANS33)
			MakeRGBA(vertex->color, 255, 255, 255, 255 * 0.33);
		else if (texInfo->flags & SURF_TRANS66)
			MakeRGBA(vertex->color, 255, 255, 255, 255 * 0.66);
		else
			MakeRGBA(vertex->color, 255, 255, 255, 255);
	}

	// Build triangle neighbors
	R_BuildSurfaceTriangleNeighbors(surface->numTriangles, surface->triangles);
}

/*
 =================
 R_FixSurfaceTextureCoords
 =================
*/
static void R_FixSurfaceTextureCoords (surface_t *surface){

	surfVertex_t	*vertex;
	vec2_t			bias = {999999, 999999};
	float			scale = 1.0, max = 0.0;
	int				i;

	if (!surface->texInfo->clamp)
		return;

	// Find the coordinate bias for each axis, which corresponds to the
	// minimum coordinate values
	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		if (vertex->st[0] < bias[0])
			bias[0] = vertex->st[0];
		if (vertex->st[1] < bias[1])
			bias[1] = vertex->st[1];
	}

	// Bias so that both axes end up with a minimum coordinate of zero,
	// and find the maximum coordinate value for transforming them to
	// the 0.0 - 1.0 range
	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		vertex->st[0] -= bias[0];
		vertex->st[1] -= bias[1];

		if (vertex->st[0] > max)
			max = vertex->st[0];
		if (vertex->st[1] > max)
			max = vertex->st[1];
	}

	// The scale factor is the inverse of the maximum value
	if (max)
		scale = 1.0 / max;

	// Finally scale so that both axes end up with a maximum coordinate
	// of one
	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		vertex->st[0] *= scale;
		vertex->st[1] *= scale;
	}
}

/*
 =================
 R_LoadFaces
 =================
*/
static void R_LoadFaces (const byte *data, const lump_t *l){

	dface_t		*in;
	surface_t 	*out;
	int			i;

	in = (dface_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dface_t))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", tr.worldModel->realName);

	tr.worldModel->numSurfaces = l->fileLen / sizeof(dface_t);
	tr.worldModel->surfaces = out = Hunk_Alloc(tr.worldModel->numSurfaces * sizeof(surface_t));
	tr.worldModel->size += tr.worldModel->numSurfaces * sizeof(surface_t);

	for (i = 0; i < tr.worldModel->numSurfaces; i++, in++, out++){
		out->flags = 0;
		out->firstEdge = LittleLong(in->firstEdge);
		out->numEdges = LittleShort(in->numEdges);

		if (LittleShort(in->side))
			out->flags |= SURF_PLANEBACK;

		out->plane = tr.worldModel->planes + LittleShort(in->planeNum);
		out->texInfo = tr.worldModel->texInfo + LittleShort(in->texInfo);

		out->viewCount = 0;
		out->worldCount = 0;
		out->lightCount = 0;
		out->fragmentCount = 0;

		// Find origin and bounds
		R_CalcSurfaceOriginAndBounds(out);

		// Create the polygon
		R_BuildSurfacePolygon(out);

		// Fix texture coordinates for clamping
		R_FixSurfaceTextureCoords(out);
	}
}

/*
 =================
 R_LoadMarkSurfaces
 =================
*/
static void R_LoadMarkSurfaces (const byte *data, const lump_t *l){

	short		*in;
	surface_t	**out;
	int			i;
	
	in = (short *)(data + l->fileOfs);
	if (l->fileLen % sizeof(short))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", tr.worldModel->realName);

	tr.worldModel->numMarkSurfaces = l->fileLen / sizeof(short);
	tr.worldModel->markSurfaces = out = Hunk_Alloc(tr.worldModel->numMarkSurfaces * sizeof(surface_t *));
	tr.worldModel->size += tr.worldModel->numMarkSurfaces * sizeof(surface_t *);

	for (i = 0; i < tr.worldModel->numMarkSurfaces; i++)
		out[i] = tr.worldModel->surfaces + LittleShort(in[i]);
}

/*
 =================
 R_LoadVisibility
 =================
*/
static void R_LoadVisibility (const byte *data, const lump_t *l){

	int		i;

	if (!l->fileLen){
		tr.worldModel->vis = NULL;
		return;
	}

	tr.worldModel->vis = Hunk_Alloc(l->fileLen);
	tr.worldModel->size += l->fileLen;

	memcpy(tr.worldModel->vis, data + l->fileOfs, l->fileLen);

	tr.worldModel->vis->numClusters = LittleLong(tr.worldModel->vis->numClusters);
	for (i = 0; i < tr.worldModel->vis->numClusters; i++){
		tr.worldModel->vis->bitOfs[i][0] = LittleLong(tr.worldModel->vis->bitOfs[i][0]);
		tr.worldModel->vis->bitOfs[i][1] = LittleLong(tr.worldModel->vis->bitOfs[i][1]);
	}
}

/*
 =================
 R_LoadLeafs
 =================
*/
static void R_LoadLeafs (const byte *data, const lump_t *l){

	dleaf_t	*in;
	leaf_t	*out;
	int		i, j;

	in = (dleaf_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dleaf_t))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", tr.worldModel->realName);

	tr.worldModel->numLeafs = l->fileLen / sizeof(dleaf_t);
	tr.worldModel->leafs = out = Hunk_Alloc(tr.worldModel->numLeafs * sizeof(leaf_t));
	tr.worldModel->size += tr.worldModel->numLeafs * sizeof(leaf_t);

	for (i = 0; i < tr.worldModel->numLeafs; i++, in++, out++){
		out->contents = LittleLong(in->contents);
		out->viewCount = 0;
		out->visCount = 0;

		for (j = 0; j < 3; j++){
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}

		out->parent = NULL;

		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		out->firstMarkSurface = tr.worldModel->markSurfaces + LittleShort(in->firstLeafFace);
		out->numMarkSurfaces = LittleShort(in->numLeafFaces);

		// Mark the surfaces for caustics
		if (out->contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
			for (j = 0; j < out->numMarkSurfaces; j++){
				if (out->firstMarkSurface[j]->texInfo->flags & (SURF_WARP | SURF_TRANS33 | SURF_TRANS66))
					continue;

				if (out->firstMarkSurface[j]->texInfo->material->noOverlays)
					continue;

				if (out->contents & CONTENTS_WATER)
					out->firstMarkSurface[j]->flags |= SURF_IN_WATER;
				if (out->contents & CONTENTS_SLIME)
					out->firstMarkSurface[j]->flags |= SURF_IN_SLIME;
				if (out->contents & CONTENTS_LAVA)
					out->firstMarkSurface[j]->flags |= SURF_IN_LAVA;
			}
		}
	}
}

/*
 =================
 R_SetParent
 =================
*/
static void R_SetParent (node_t *node, node_t *parent){

	node->parent = parent;
	if (node->contents != -1)
		return;

	R_SetParent(node->children[0], node);
	R_SetParent(node->children[1], node);
}

/*
 =================
 R_LoadNodes
 =================
*/
static void R_LoadNodes (const byte *data, const lump_t *l){

	dnode_t	*in;
	node_t	*out;
	int		i, j, p;

	in = (dnode_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dnode_t))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", tr.worldModel->realName);

	tr.worldModel->numNodes = l->fileLen / sizeof(dnode_t);
	tr.worldModel->nodes = out = Hunk_Alloc(tr.worldModel->numNodes * sizeof(node_t));
	tr.worldModel->size += tr.worldModel->numNodes * sizeof(node_t);

	for (i = 0; i < tr.worldModel->numNodes; i++, in++, out++){
		out->contents = -1;
		out->viewCount = 0;
		out->visCount = 0;

		for (j = 0; j < 3; j++){
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}

		out->parent = NULL;

		out->plane = tr.worldModel->planes + LittleLong(in->planeNum);

		for (j = 0; j < 2; j++){
			p = LittleLong(in->children[j]);
			if (p >= 0)
				out->children[j] = tr.worldModel->nodes + p;
			else
				out->children[j] = (node_t *)(tr.worldModel->leafs + (-1 - p));
		}

		out->firstSurface = LittleShort(in->firstFace);
		out->numSurfaces = LittleShort(in->numFaces);
	}

	// Set nodes and leafs
	R_SetParent(tr.worldModel->nodes, NULL);
}

/*
 =================
 R_SetupSubmodels
 =================
*/
static void R_SetupSubmodels (void){

	int			i;
	submodel_t	*submodel;
	model_t		*model;

	for (i = 0; i < tr.worldModel->numSubmodels; i++){
		submodel = &tr.worldModel->submodels[i];
		model = &r_inlineModels[i];

		*model = *tr.worldModel;

		VectorCopy(submodel->mins, model->mins);
		VectorCopy(submodel->maxs, model->maxs);
		model->radius = submodel->radius;
		model->firstModelSurface = submodel->firstFace;
		model->numModelSurfaces = submodel->numFaces;

		if (i == 0)
			*tr.worldModel = *model;
	}
}

/*
 =================
 R_LoadSubmodels
 =================
*/
static void R_LoadSubmodels (const byte *data, const lump_t *l){

	dmodel_t	*in;
	submodel_t	*out;
	int			i, j;

	in = (dmodel_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dmodel_t))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", tr.worldModel->realName);

	tr.worldModel->numSubmodels = l->fileLen / sizeof(dmodel_t);
	tr.worldModel->submodels = out = Hunk_Alloc(tr.worldModel->numSubmodels * sizeof(submodel_t));
	tr.worldModel->size += tr.worldModel->numSubmodels * sizeof(submodel_t);

	for (i = 0; i < tr.worldModel->numSubmodels; i++, in++, out++){
		for (j = 0; j < 3; j++){
			out->origin[j] = LittleFloat(in->origin[j]);

			// Spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1.0;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1.0;
		}

		out->radius = RadiusFromBounds(out->mins, out->maxs);

		out->firstFace = LittleLong(in->firstFace);
		out->numFaces = LittleLong(in->numFaces);
	}

	// Set up the submodels
	R_SetupSubmodels();
}

/*
 =================
 R_LoadSky
 =================
*/
static void R_LoadSky (const char *name, float rotate, const vec3_t axis){

	sky_t	*sky;

	tr.worldModel->sky = sky = Hunk_Alloc(sizeof(sky_t));
	tr.worldModel->size += sizeof(sky_t);

	sky->material = R_FindMaterial(name, MT_GENERIC, SURFACEPARM_SKY);

	sky->rotate = rotate;
	VectorCopy(axis, sky->axis);
}

/*
 =================
 R_LoadDecorations
 =================
*/
static void R_LoadDecorations (void){

	char				name[MAX_OSPATH];
	byte				*data;
	decHeader_t			*header;
	decSurfaceInfo_t	*inSurface;
	decSurface_t		*outSurface;
	surfTriangle_t		*inTriangle, *outTriangle;
	surfFacePlane_t		*inFacePlane, *outFacePlane;
	surfVertex_t		*inVertex, *outVertex;
	leaf_t				*leafList[MAX_MAP_LEAFS];
	int					numLeafs;
	int					i, j;

	// Load the file
	Q_snprintfz(name, sizeof(name), "%s.dec", tr.worldModel->name);
	FS_LoadFile(name, (void **)&data);
	if (!data){
		tr.worldModel->numDecorationSurfaces = 0;
		tr.worldModel->decorationSurfaces = NULL;

		return;
	}

	header = (decHeader_t *)data;

	// Byte swap the header fields and sanity check
	for (i = 0; i < sizeof(decHeader_t) / 4; i++)
		((int *)header)[i] = LittleLong(((int *)header)[i]);

	if (header->ident != DEC_IDENT)
		Com_Error(ERR_DROP, "R_LoadDecorations: '%s' has wrong file id", name);

	if (header->version != DEC_VERSION)
		Com_Error(ERR_DROP, "R_LoadDecorations: '%s' has wrong version number (%i should be %i)", name, header->version, DEC_VERSION);

	// Load the surfaces
	inSurface = (decSurfaceInfo_t *)(data + sizeof(decHeader_t));

	tr.worldModel->numDecorationSurfaces = header->count;
	tr.worldModel->decorationSurfaces = outSurface = Hunk_Alloc(tr.worldModel->numDecorationSurfaces * sizeof(decSurface_t));

	tr.worldModel->size += tr.worldModel->numDecorationSurfaces * sizeof(decSurface_t);

	for (i = 0; i < tr.worldModel->numDecorationSurfaces; i++, outSurface++){
		outSurface->origin[0] = LittleFloat(inSurface->origin[0]);
		outSurface->origin[1] = LittleFloat(inSurface->origin[1]);
		outSurface->origin[2] = LittleFloat(inSurface->origin[2]);

		outSurface->mins[0] = LittleFloat(inSurface->mins[0]);
		outSurface->mins[1] = LittleFloat(inSurface->mins[1]);
		outSurface->mins[2] = LittleFloat(inSurface->mins[2]);

		outSurface->maxs[0] = LittleFloat(inSurface->maxs[0]);
		outSurface->maxs[1] = LittleFloat(inSurface->maxs[1]);
		outSurface->maxs[2] = LittleFloat(inSurface->maxs[2]);

		// Load the leaves
		numLeafs = R_BoxInLeaves(tr.worldModel->nodes, outSurface->mins, outSurface->maxs, leafList, 0, MAX_MAP_LEAFS);

		outSurface->numLeafs = numLeafs;
		outSurface->leafList = Hunk_Alloc(outSurface->numLeafs * sizeof(leaf_t *));

		tr.worldModel->size += outSurface->numLeafs * sizeof(leaf_t *);

		for (j = 0; j < outSurface->numLeafs; j++)
			outSurface->leafList[j] = leafList[j];

		outSurface->viewCount = 0;

		// Load the material
		outSurface->material = R_FindMaterial(inSurface->material, MT_GENERIC, SURFACEPARM_LIGHTING);

		// Load the triangles
		inTriangle = (surfTriangle_t *)((byte *)inSurface + sizeof(decSurfaceInfo_t));

		outSurface->numTriangles = LittleLong(inSurface->numTriangles);
		outSurface->triangles = outTriangle = Hunk_Alloc(outSurface->numTriangles * sizeof(surfTriangle_t));

		tr.worldModel->size += outSurface->numTriangles * sizeof(surfTriangle_t);

		for (j = 0; j < outSurface->numTriangles; j++, inTriangle++, outTriangle++){
			outTriangle->index[0] = (index_t)LittleLong(inTriangle->index[0]);
			outTriangle->index[1] = (index_t)LittleLong(inTriangle->index[1]);
			outTriangle->index[2] = (index_t)LittleLong(inTriangle->index[2]);

			outTriangle->neighbor[0] = LittleLong(inTriangle->neighbor[0]);
			outTriangle->neighbor[1] = LittleLong(inTriangle->neighbor[1]);
			outTriangle->neighbor[2] = LittleLong(inTriangle->neighbor[2]);
		}

		// Load the face planes
		inFacePlane = (surfFacePlane_t *)((byte *)inSurface + sizeof(decSurfaceInfo_t) + outSurface->numTriangles * sizeof(surfTriangle_t));

		outSurface->facePlanes = outFacePlane = Hunk_Alloc(outSurface->numTriangles * sizeof(surfFacePlane_t));

		tr.worldModel->size += outSurface->numTriangles * sizeof(surfFacePlane_t);

		for (j = 0; j < outSurface->numTriangles; j++, inFacePlane++, outFacePlane++){
			outFacePlane->normal[0] = LittleFloat(inFacePlane->normal[0]);
			outFacePlane->normal[1] = LittleFloat(inFacePlane->normal[1]);
			outFacePlane->normal[2] = LittleFloat(inFacePlane->normal[2]);

			outFacePlane->dist = LittleFloat(inFacePlane->dist);
		}

		// Load the vertices
		inVertex = (surfVertex_t *)((byte *)inSurface + sizeof(decSurfaceInfo_t) + outSurface->numTriangles * (sizeof(surfTriangle_t) + sizeof(surfFacePlane_t)));

		outSurface->numVertices = LittleLong(inSurface->numVertices);
		outSurface->vertices = outVertex = Hunk_Alloc(outSurface->numVertices * sizeof(surfVertex_t));

		tr.worldModel->size += outSurface->numVertices * sizeof(surfVertex_t);

		for (j = 0; j < outSurface->numVertices; j++, inVertex++, outVertex++){
			outVertex->xyz[0] = LittleFloat(inVertex->xyz[0]);
			outVertex->xyz[1] = LittleFloat(inVertex->xyz[1]);
			outVertex->xyz[2] = LittleFloat(inVertex->xyz[2]);

			outVertex->normal[0] = LittleFloat(inVertex->normal[0]);
			outVertex->normal[1] = LittleFloat(inVertex->normal[1]);
			outVertex->normal[2] = LittleFloat(inVertex->normal[2]);

			outVertex->tangents[0][0] = LittleFloat(inVertex->tangents[0][0]);
			outVertex->tangents[0][1] = LittleFloat(inVertex->tangents[0][1]);
			outVertex->tangents[0][2] = LittleFloat(inVertex->tangents[0][2]);
			outVertex->tangents[1][0] = LittleFloat(inVertex->tangents[1][0]);
			outVertex->tangents[1][1] = LittleFloat(inVertex->tangents[1][1]);
			outVertex->tangents[1][2] = LittleFloat(inVertex->tangents[1][2]);

			outVertex->st[0] = LittleFloat(inVertex->st[0]);
			outVertex->st[1] = LittleFloat(inVertex->st[1]);

			outVertex->color[0] = inVertex->color[0];
			outVertex->color[1] = inVertex->color[1];
			outVertex->color[2] = inVertex->color[2];
			outVertex->color[3] = inVertex->color[3];
		}

		// Skip to next surface
		inSurface = (decSurfaceInfo_t *)((byte *)inSurface + sizeof(decSurfaceInfo_t) + outSurface->numTriangles * (sizeof(surfTriangle_t) + sizeof(surfFacePlane_t)) + outSurface->numVertices * sizeof(surfVertex_t));
	}

	FS_FreeFile(data);
}

/*
 =================
 R_LoadWorldMap
 =================
*/
void R_LoadWorldMap (const char *mapName, const char *skyName, float skyRotate, const vec3_t skyAxis){

	byte		*data;
	dheader_t	*header;
	unsigned	hash;
	int			i;

	if (tr.worldModel)
		Com_Error(ERR_DROP, "R_LoadWorldMap: attempted to redundantly load world map");

	// Load the file
	FS_LoadFile(mapName, (void **)&data);
	if (!data)
		Com_Error(ERR_DROP, "R_LoadWorldMap: '%s' not found", mapName);

	r_models[r_numModels++] = tr.worldModel = Z_Malloc(sizeof(model_t));

	// Fill it in
	Com_StripExtension(mapName, tr.worldModel->name, sizeof(tr.worldModel->name));
	Q_strncpyz(tr.worldModel->realName, mapName, sizeof(tr.worldModel->realName));
	tr.worldModel->size = 0;
	tr.worldModel->modelType = MODEL_BSP;

	header = (dheader_t *)data;

	// Byte swap the header fields and sanity check
	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int *)header)[i] = LittleLong(((int *)header)[i]);

	if (header->ident != BSP_IDENT)
		Com_Error(ERR_DROP, "R_LoadWorldMap: '%s' has wrong file id", tr.worldModel->realName);

	if (header->version != BSP_VERSION)
		Com_Error(ERR_DROP, "R_LoadWorldMap: '%s' has wrong version number (%i should be %i)", tr.worldModel->realName, header->version, BSP_VERSION);

	// Load into heap
	R_LoadSky(skyName, skyRotate, skyAxis);

	R_LoadVertexes(data, &header->lumps[LUMP_VERTEXES]);
	R_LoadEdges(data, &header->lumps[LUMP_EDGES]);
	R_LoadSurfEdges(data, &header->lumps[LUMP_SURFEDGES]);
	R_LoadPlanes(data, &header->lumps[LUMP_PLANES]);
	R_LoadTexInfo(data, &header->lumps[LUMP_TEXINFO]);
	R_LoadFaces(data, &header->lumps[LUMP_FACES]);
	R_LoadMarkSurfaces(data, &header->lumps[LUMP_LEAFFACES]);
	R_LoadVisibility(data, &header->lumps[LUMP_VISIBILITY]);
	R_LoadLeafs(data, &header->lumps[LUMP_LEAFS]);
	R_LoadNodes(data, &header->lumps[LUMP_NODES]);
	R_LoadSubmodels(data, &header->lumps[LUMP_MODELS]);

	FS_FreeFile(data);

	// Load external files
	R_LoadDecorations();
	R_LoadLights();

	// Set up some needed things
	tr.worldEntity->model = tr.worldModel;

	// Load some needed materials
	tr.waterCausticsMaterial = R_FindMaterial("waterCaustics", MT_GENERIC, 0);
	tr.slimeCausticsMaterial = R_FindMaterial("slimeCaustics", MT_GENERIC, 0);
	tr.lavaCausticsMaterial = R_FindMaterial("lavaCaustics", MT_GENERIC, 0);

	// Add to hash table
	hash = Com_HashKey(tr.worldModel->name, MODELS_HASH_SIZE);

	tr.worldModel->nextHash = r_modelsHashTable[hash];
	r_modelsHashTable[hash] = tr.worldModel;
}


/*
 =======================================================================

 ALIAS MODELS

 =======================================================================
*/


/*
 =================
 R_CalcTangentVectors
 =================
*/
static void R_CalcTangentVectors (int numTriangles, mdlTriangle_t *triangles, int numVertices, mdlXyzNormal_t *xyzNormals, mdlSt_t *st, int frameNum){

	mdlTriangle_t	*triangle;
	mdlXyzNormal_t	*xyzNormal;
	float			*pXyz[3], *pSt[3];
	vec5_t			edge[2];
	vec3_t			normal, tangents[2], cross;
	float			d;
	int				i, j;

	xyzNormals += numVertices * frameNum;

	// Clear
	for (i = 0, xyzNormal = xyzNormals; i < numVertices; i++, xyzNormal++){
		VectorClear(xyzNormal->tangents[0]);
		VectorClear(xyzNormal->tangents[1]);
	}

	// Calculate normal and tangent vectors
	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		pXyz[0] = (float *)(xyzNormals[triangle->index[0]].xyz);
		pXyz[1] = (float *)(xyzNormals[triangle->index[1]].xyz);
		pXyz[2] = (float *)(xyzNormals[triangle->index[2]].xyz);

		pSt[0] = (float *)(st[triangle->index[0]].st);
		pSt[1] = (float *)(st[triangle->index[1]].st);
		pSt[2] = (float *)(st[triangle->index[2]].st);

		// Find edges
		edge[0][0] = pXyz[1][0] - pXyz[0][0];
		edge[0][1] = pXyz[1][1] - pXyz[0][1];
		edge[0][2] = pXyz[1][2] - pXyz[0][2];
		edge[0][3] = pSt[1][0] - pSt[0][0];
		edge[0][4] = pSt[1][1] - pSt[0][1];

		edge[1][0] = pXyz[2][0] - pXyz[0][0];
		edge[1][1] = pXyz[2][1] - pXyz[0][1];
		edge[1][2] = pXyz[2][2] - pXyz[0][2];
		edge[1][3] = pSt[2][0] - pSt[0][0];
		edge[1][4] = pSt[2][1] - pSt[0][1];

		// Compute normal vector
		normal[0] = edge[1][1] * edge[0][2] - edge[1][2] * edge[0][1];
		normal[1] = edge[1][2] * edge[0][0] - edge[1][0] * edge[0][2];
		normal[2] = edge[1][0] * edge[0][1] - edge[1][1] * edge[0][0];

		VectorNormalize(normal);

		// Compute first tangent vector
		tangents[0][0] = edge[1][4] * edge[0][0] - edge[1][0] * edge[0][4];
		tangents[0][1] = edge[1][4] * edge[0][1] - edge[1][1] * edge[0][4];
		tangents[0][2] = edge[1][4] * edge[0][2] - edge[1][2] * edge[0][4];

		d = DotProduct(tangents[0], normal);
		VectorMA(tangents[0], -d, normal, tangents[0]);
		VectorNormalize(tangents[0]);

		// Compute second tangent vector
		tangents[1][0] = edge[1][0] * edge[0][3] - edge[1][3] * edge[0][0];
		tangents[1][1] = edge[1][1] * edge[0][3] - edge[1][3] * edge[0][1];
		tangents[1][2] = edge[1][2] * edge[0][3] - edge[1][3] * edge[0][2];

		d = DotProduct(tangents[1], normal);
		VectorMA(tangents[1], -d, normal, tangents[1]);
		VectorNormalize(tangents[1]);

		// Inverse tangent vectors if needed
		CrossProduct(tangents[1], tangents[0], cross);
		if (DotProduct(cross, normal) < 0){
			VectorInverse(tangents[0]);
			VectorInverse(tangents[1]);
		}

		// Add the vectors
		for (j = 0; j < 3; j++){
			VectorAdd(xyzNormals[triangle->index[j]].tangents[0], tangents[0], xyzNormals[triangle->index[j]].tangents[0]);
			VectorAdd(xyzNormals[triangle->index[j]].tangents[1], tangents[1], xyzNormals[triangle->index[j]].tangents[1]);
		}
	}

	// Renormalize
	for (i = 0, xyzNormal = xyzNormals; i < numVertices; i++, xyzNormal++){
		VectorNormalize(xyzNormal->tangents[0]);
		VectorNormalize(xyzNormal->tangents[1]);
	}
}

/*
 =================
 R_CalcFacePlanes
 =================
*/
static void R_CalcFacePlanes (int numTriangles, mdlTriangle_t *triangles, mdlFacePlane_t *facePlanes, int numVertices, mdlXyzNormal_t *xyzNormals, int frameNum){

	mdlTriangle_t	*triangle;
	mdlFacePlane_t	*facePlane;
	float			*pXyz[3];
	vec3_t			edge[2];
	int				i;

	facePlanes += numTriangles * frameNum;
	xyzNormals += numVertices * frameNum;

	// Calculate face planes
	for (i = 0, triangle = triangles, facePlane = facePlanes; i < numTriangles; i++, triangle++, facePlane++){
		pXyz[0] = (float *)(xyzNormals[triangle->index[0]].xyz);
		pXyz[1] = (float *)(xyzNormals[triangle->index[1]].xyz);
		pXyz[2] = (float *)(xyzNormals[triangle->index[2]].xyz);

		// Find edges
		VectorSubtract(pXyz[1], pXyz[0], edge[0]);
		VectorSubtract(pXyz[2], pXyz[0], edge[1]);

		// Compute normal
		CrossProduct(edge[1], edge[0], facePlane->normal);
		VectorNormalize(facePlane->normal);

		// Compute distance
		facePlane->dist = DotProduct(pXyz[0], facePlane->normal);
	}
}

/*
 =================
 R_FindTriangleWithEdge
 =================
*/
static int R_FindTriangleWithEdge (int numTriangles, mdlTriangle_t *triangles, index_t start, index_t end, int ignore){

	mdlTriangle_t	*triangle;
	int				count, match;
	int				i;

	count = 0;
	match = -1;

	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		if ((triangle->index[0] == start && triangle->index[1] == end) || (triangle->index[1] == start && triangle->index[2] == end) || (triangle->index[2] == start && triangle->index[0] == end)){
			if (i != ignore)
				match = i;

			count++;
		}
		else if ((triangle->index[1] == start && triangle->index[0] == end) || (triangle->index[2] == start && triangle->index[1] == end) || (triangle->index[0] == start && triangle->index[2] == end))
			count++;
	}

	// Detect edges shared by three triangles and make them seams
	if (count > 2)
		match = -1;

	return match;
}

/*
 =================
 R_BuildTriangleNeighbors
 =================
*/
static void R_BuildTriangleNeighbors (int numTriangles, mdlTriangle_t *triangles){

	mdlTriangle_t	*triangle;
	int				i;

	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		triangle->neighbor[0] = R_FindTriangleWithEdge(numTriangles, triangles, triangle->index[1], triangle->index[0], i);
		triangle->neighbor[1] = R_FindTriangleWithEdge(numTriangles, triangles, triangle->index[2], triangle->index[1], i);
		triangle->neighbor[2] = R_FindTriangleWithEdge(numTriangles, triangles, triangle->index[0], triangle->index[2], i);
	}
}

/*
 =================
 R_LoadMD2
 =================
*/
static qboolean R_LoadMD2 (model_t *model, const byte *data){

	md2Header_t		*inModel;
	md2Frame_t		*inFrame;
	md2Triangle_t	*inTriangle;
	md2St_t			*inSt;
	mdl_t			*outModel;
	mdlFrame_t		*outFrame;
	mdlSurface_t	*outSurface;
	mdlMaterial_t	*outMaterial;
	mdlTriangle_t	*outTriangle;
	mdlSt_t			*outSt;
	mdlXyzNormal_t	*outXyzNormal;
	int				i, j;
	int				ident, version;
	int				indexRemap[MD2_MAX_TRIANGLES*3];
	index_t			indexTable[MD2_MAX_TRIANGLES*3];
	index_t			tmpXyzIndices[MD2_MAX_TRIANGLES*3], tmpStIndices[MD2_MAX_TRIANGLES*3];
	int				numIndices;
	float			skinWidth, skinHeight;
	vec3_t			scale, translate;
	char			name[MAX_OSPATH];

	inModel = (md2Header_t *)data;
	model->alias = outModel = Hunk_Alloc(sizeof(mdl_t));
	
	model->size += sizeof(mdl_t);

	// Byte swap the header fields and sanity check
	ident = LittleLong(inModel->ident);
	if (ident != MD2_IDENT){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD2: wrong file id (%s)\n", model->realName);
		return false;
	}

	version = LittleLong(inModel->version);
	if (version != MD2_VERSION){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD2: wrong version number (%i should %i) (%s)\n", version, MD2_VERSION, model->realName);
		return false;
	}

	outModel->numFrames = LittleLong(inModel->numFrames);
	if (outModel->numFrames > MD2_MAX_FRAMES || outModel->numFrames <= 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD2: invalid number of frames (%i) (%s)\n", outModel->numFrames, model->realName);
		return false;
	}

	outModel->numTags = 0;
	outModel->numSurfaces = 1;

	skinWidth = (float)LittleLong(inModel->skinWidth);
	if (skinWidth <= 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD2: invalid skin width (%i) (%s)\n", (int)skinWidth, model->realName);
		return false;
	}

	skinHeight = (float)LittleLong(inModel->skinHeight);
	if (skinHeight <= 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD2: invalid skin height (%i) (%s)\n", (int)skinHeight, model->realName);
		return false;
	}

	skinWidth = 1.0 / skinWidth;
	skinHeight = 1.0 / skinHeight;

	outModel->surfaces = outSurface = Hunk_Alloc(sizeof(mdlSurface_t));

	model->size += sizeof(mdlSurface_t);

	outSurface->numTriangles = LittleLong(inModel->numTris);
	if (outSurface->numTriangles > MD2_MAX_TRIANGLES || outSurface->numTriangles <= 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD2: invalid number of triangles (%i) (%s)\n", outSurface->numTriangles, model->realName);
		return false;
	}

	outSurface->numVertices = LittleLong(inModel->numXyz);
	if (outSurface->numVertices > MD2_MAX_VERTS || outSurface->numVertices <= 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD2: invalid number of vertices (%i) (%s)\n", outSurface->numVertices, model->realName);
		return false;
	}
	
	outSurface->numMaterials = LittleLong(inModel->numSkins);
	if (outSurface->numMaterials > MD2_MAX_SKINS || outSurface->numMaterials < 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD2: invalid number of skins (%i) (%s)\n", outSurface->numMaterials, model->realName);
		return false;
	}

	// Load triangle lists
	inTriangle = (md2Triangle_t *)((byte *)inModel + LittleLong(inModel->ofsTris));

	for (i = 0; i < outSurface->numTriangles; i++){
		tmpXyzIndices[i*3+0] = (index_t)LittleShort(inTriangle[i].indexXyz[0]);
		tmpXyzIndices[i*3+1] = (index_t)LittleShort(inTriangle[i].indexXyz[1]);
		tmpXyzIndices[i*3+2] = (index_t)LittleShort(inTriangle[i].indexXyz[2]);

		tmpStIndices[i*3+0] = (index_t)LittleShort(inTriangle[i].indexSt[0]);
		tmpStIndices[i*3+1] = (index_t)LittleShort(inTriangle[i].indexSt[1]);
		tmpStIndices[i*3+2] = (index_t)LittleShort(inTriangle[i].indexSt[2]);
	}

	// Build list of unique vertices
	outSurface->numVertices = 0;

	numIndices = outSurface->numTriangles * 3;
	memset(indexRemap, -1, MD2_MAX_TRIANGLES * 3 * sizeof(int));

	for (i = 0; i < numIndices; i++){
		if (indexRemap[i] != -1)
			continue;

		// Remap duplicates
		for (j = i + 1; j < numIndices; j++){
			if ((tmpXyzIndices[j] == tmpXyzIndices[i]) && (tmpStIndices[j] == tmpStIndices[i])){
				indexRemap[j] = i;
				indexTable[j] = outSurface->numVertices;
			}
		}

		// Add unique vertex
		indexRemap[i] = i;
		indexTable[i] = outSurface->numVertices++;
	}

	// Copy triangle lists
	outSurface->triangles = outTriangle = Hunk_Alloc(outSurface->numTriangles * sizeof(mdlTriangle_t));

	model->size += outSurface->numTriangles * sizeof(mdlTriangle_t);

	for (i = 0; i < numIndices; i += 3, outTriangle++){
		outTriangle->index[0] = indexTable[i+0];
		outTriangle->index[1] = indexTable[i+1];
		outTriangle->index[2] = indexTable[i+2];
	}

	// Load base S and T vertices
	inSt = (md2St_t *)((byte *)inModel + LittleLong(inModel->ofsSt));
	outSurface->st = outSt = Hunk_Alloc(outSurface->numVertices * sizeof(mdlSt_t));

	model->size += outSurface->numVertices * sizeof(mdlSt_t);

	for (i = 0; i < numIndices; i++){
		if (indexRemap[i] != i)
			continue;

		outSt[indexTable[i]].st[0] = ((float)LittleShort(inSt[tmpStIndices[i]].s) + 0.5) * skinWidth;
		outSt[indexTable[i]].st[1] = ((float)LittleShort(inSt[tmpStIndices[i]].t) + 0.5) * skinHeight;
	}

	// Allocate space for face planes
	outSurface->facePlanes = Hunk_Alloc(outModel->numFrames * outSurface->numTriangles * sizeof(mdlFacePlane_t));
	model->size += outModel->numFrames * outSurface->numTriangles * sizeof(mdlFacePlane_t);

	// Load the frames
	outModel->frames = outFrame = Hunk_Alloc(outModel->numFrames * sizeof(mdlFrame_t));
	outSurface->xyzNormals = outXyzNormal = Hunk_Alloc(outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t));

	model->size += outModel->numFrames * sizeof(mdlFrame_t);
	model->size += outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t);

	for (i = 0; i < outModel->numFrames; i++, outFrame++, outXyzNormal += outSurface->numVertices){
		inFrame = (md2Frame_t *)((byte *)inModel + LittleLong(inModel->ofsFrames) + i*LittleLong(inModel->frameSize));

		scale[0] = LittleFloat(inFrame->scale[0]);
		scale[1] = LittleFloat(inFrame->scale[1]);
		scale[2] = LittleFloat(inFrame->scale[2]);

		translate[0] = LittleFloat(inFrame->translate[0]);
		translate[1] = LittleFloat(inFrame->translate[1]);
		translate[2] = LittleFloat(inFrame->translate[2]);

		VectorCopy(translate, outFrame->mins);
		VectorMA(translate, 255.0, scale, outFrame->maxs);

		outFrame->radius = RadiusFromBounds(outFrame->mins, outFrame->maxs);

		// Load XYZ vertices
		for (j = 0; j < numIndices; j++){
			if (indexRemap[j] != j)
				continue;

			outXyzNormal[indexTable[j]].xyz[0] = (float)inFrame->verts[tmpXyzIndices[j]].v[0] * scale[0] + translate[0];
			outXyzNormal[indexTable[j]].xyz[1] = (float)inFrame->verts[tmpXyzIndices[j]].v[1] * scale[1] + translate[1];
			outXyzNormal[indexTable[j]].xyz[2] = (float)inFrame->verts[tmpXyzIndices[j]].v[2] * scale[2] + translate[2];

			ByteToDir(inFrame->verts[tmpXyzIndices[j]].lightNormalIndex, outXyzNormal[indexTable[j]].normal);
		}

		// Calculate tangent vectors
		R_CalcTangentVectors(outSurface->numTriangles, outSurface->triangles, outSurface->numVertices, outSurface->xyzNormals, outSurface->st, i);

		// Calculate face planes
		R_CalcFacePlanes(outSurface->numTriangles, outSurface->triangles, outSurface->facePlanes, outSurface->numVertices, outSurface->xyzNormals, i);
	}

	// Build triangle neighbors
	R_BuildTriangleNeighbors(outSurface->numTriangles, outSurface->triangles);

	// Register all skins
	outSurface->materials = outMaterial = Hunk_Alloc(outSurface->numMaterials * sizeof(mdlMaterial_t));

	model->size += outSurface->numMaterials * sizeof(mdlMaterial_t);

	for (i = 0; i < outSurface->numMaterials; i++, outMaterial++){
		Com_StripExtension((char *)inModel + LittleLong(inModel->ofsSkins) + i*MAX_QPATH, name, sizeof(name));
		outMaterial->material = R_FindMaterial(name, MT_GENERIC, SURFACEPARM_LIGHTING);
	}

	return true;
}


/*
 =========
 R_LoadMD3
 =========
*/
static qboolean R_LoadMD3 (model_t *model, const byte *data) {

	md3Header_t		*inModel;
	md3Frame_t		*inFrame;
	md3Tag_t		*inTag;
	md3Surface_t	*inSurface;
	md3Material_t	*inMaterial;
	md3Triangle_t	*inTriangle;
	md3St_t			*inSt;
	md3XyzNormal_t	*inXyzNormal;
	mdl_t			*outModel;
	mdlFrame_t		*outFrame;
	mdlTag_t		*outTag;
	mdlSurface_t	*outSurface;
	mdlMaterial_t	*outMaterial;
	mdlTriangle_t	*outTriangle;
	mdlSt_t			*outSt;
	mdlXyzNormal_t	*outXyzNormal;
	int				i, j, k;
	int				ident, version;
	vec3_t			scale, translate;
	float			lat, lng;
	char			name[MAX_OSPATH];

	inModel = (md3Header_t *)data;
	model->alias = outModel = Hunk_Alloc(sizeof(mdl_t));

	model->size += sizeof(mdl_t);

	// Byte swap the header fields and sanity check
	ident = LittleLong(inModel->ident);
	if (ident != MD3_IDENT){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD3: wrong file id (%s)\n", model->realName);
		return false;
	}

	version = LittleLong(inModel->version);
	if (version != MD3_VERSION){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD3: wrong version number (%i should %i) (%s)\n", version, MD3_VERSION, model->realName);
		return false;
	}

	outModel->numFrames = LittleLong(inModel->numFrames);
	if (outModel->numFrames > MD3_MAX_FRAMES || outModel->numFrames <= 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD3: invalid number of frames (%i) (%s)\n", outModel->numFrames, model->realName);
		return false;
	}

	outModel->numTags = LittleLong(inModel->numTags);
	if (outModel->numTags > MD3_MAX_TAGS || outModel->numTags < 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD3: invalid number of tags (%i) (%s)\n", outModel->numTags, model->realName);
		return false;
	}

	outModel->numSurfaces = LittleLong(inModel->numSurfaces);
	if (outModel->numSurfaces > MD3_MAX_SURFACES || outModel->numSurfaces <= 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadMD3: invalid number of surfaces (%i) (%s)\n", outModel->numSurfaces, model->realName);
		return false;
	}

	// Load the frames
	inFrame = (md3Frame_t *)((byte *)inModel + LittleLong(inModel->ofsFrames));
	outModel->frames = outFrame = Hunk_Alloc(outModel->numFrames * sizeof(mdlFrame_t));

	model->size += outModel->numFrames * sizeof(mdlFrame_t);

	for (i = 0; i < outModel->numFrames; i++, inFrame++, outFrame++){
		outFrame->mins[0] = LittleFloat(inFrame->bounds[0][0]);
		outFrame->mins[1] = LittleFloat(inFrame->bounds[0][1]);
		outFrame->mins[2] = LittleFloat(inFrame->bounds[0][2]);
		outFrame->maxs[0] = LittleFloat(inFrame->bounds[1][0]);
		outFrame->maxs[1] = LittleFloat(inFrame->bounds[1][1]);
		outFrame->maxs[2] = LittleFloat(inFrame->bounds[1][2]);

		outFrame->radius = LittleFloat(inFrame->radius);
	}

	// Load the tags
	inTag = (md3Tag_t *)((byte *)inModel + LittleLong(inModel->ofsTags));
	outModel->tags = outTag = Hunk_Alloc(outModel->numFrames * outModel->numTags * sizeof(mdlTag_t));

	model->size += outModel->numFrames * outModel->numTags * sizeof(mdlTag_t);

	for (i = 0; i < outModel->numFrames; i++){
		for (j = 0; j < outModel->numTags; j++, inTag++, outTag++){
			Q_strncpyz(outTag->name, inTag->name, sizeof(outTag->name));

			outTag->origin[0] = LittleFloat(inTag->origin[0]);
			outTag->origin[1] = LittleFloat(inTag->origin[1]);
			outTag->origin[2] = LittleFloat(inTag->origin[2]);

			outTag->axis[0][0] = LittleFloat(inTag->axis[0][0]);
			outTag->axis[0][1] = LittleFloat(inTag->axis[0][1]);
			outTag->axis[0][2] = LittleFloat(inTag->axis[0][2]);
			outTag->axis[1][0] = LittleFloat(inTag->axis[1][0]);
			outTag->axis[1][1] = LittleFloat(inTag->axis[1][1]);
			outTag->axis[1][2] = LittleFloat(inTag->axis[1][2]);
			outTag->axis[2][0] = LittleFloat(inTag->axis[2][0]);
			outTag->axis[2][1] = LittleFloat(inTag->axis[2][1]);
			outTag->axis[2][2] = LittleFloat(inTag->axis[2][2]);
		}
	}

	// Load the surfaces
	inSurface = (md3Surface_t *)((byte *)inModel + LittleLong(inModel->ofsSurfaces));
	outModel->surfaces = outSurface = Hunk_Alloc(outModel->numSurfaces * sizeof(mdlSurface_t));

	model->size += outModel->numSurfaces * sizeof(mdlSurface_t);

	for (i = 0; i < outModel->numSurfaces; i++, outSurface++){
		// Byte swap the header fields and sanity check
		ident = LittleLong(inSurface->ident);
		if (ident != MD3_IDENT){
			Com_DPrintf(S_COLOR_YELLOW "R_LoadMD3: wrong file id in surface '%s' (%s)\n", inSurface->name, model->realName);
			return false;
		}

		outSurface->numMaterials = LittleLong(inSurface->numMaterials);
		if (outSurface->numMaterials > MD3_MAX_MATERIALS || outSurface->numMaterials <= 0){
			Com_DPrintf(S_COLOR_YELLOW "R_LoadMD3: invalid number of materials in surface '%s' (%i) (%s)\n", inSurface->name, outSurface->numMaterials, model->realName);
			return false;
		}

		outSurface->numTriangles = LittleLong(inSurface->numTriangles);
		if (outSurface->numTriangles > MD3_MAX_TRIANGLES || outSurface->numTriangles <= 0){
			Com_DPrintf(S_COLOR_YELLOW "R_LoadMD3: invalid number of triangles in surface '%s' (%i) (%s)\n", inSurface->name, outSurface->numTriangles, model->realName);
			return false;
		}

		outSurface->numVertices = LittleLong(inSurface->numVerts);
		if (outSurface->numVertices > MD3_MAX_VERTS || outSurface->numVertices <= 0){
			Com_DPrintf(S_COLOR_YELLOW "R_LoadMD3: invalid number of vertices in surface '%s' (%i) (%s)\n", inSurface->name, outSurface->numVertices, model->realName);
			return false;
		}

		// Register all materials
		inMaterial = (md3Material_t *)((byte *)inSurface + LittleLong(inSurface->ofsMaterials));
		outSurface->materials = outMaterial = Hunk_Alloc(outSurface->numMaterials * sizeof(mdlMaterial_t));

		model->size += outSurface->numMaterials * sizeof(mdlMaterial_t);

		for (j = 0; j < outSurface->numMaterials; j++, inMaterial++, outMaterial++){
			Com_StripExtension(inMaterial->name, name, sizeof(name));
			outMaterial->material = R_FindMaterial(name, MT_GENERIC, SURFACEPARM_LIGHTING);
		}

		// Load triangle lists
		inTriangle = (md3Triangle_t *)((byte *)inSurface + LittleLong(inSurface->ofsTriangles));
		outSurface->triangles = outTriangle = Hunk_Alloc(outSurface->numTriangles * sizeof(mdlTriangle_t));

		model->size += outSurface->numTriangles * sizeof(mdlTriangle_t);

		for (j = 0; j < outSurface->numTriangles; j++, inTriangle++, outTriangle++){
			outTriangle->index[0] = (index_t)LittleLong(inTriangle->indexes[0]);
			outTriangle->index[1] = (index_t)LittleLong(inTriangle->indexes[1]);
			outTriangle->index[2] = (index_t)LittleLong(inTriangle->indexes[2]);
		}

		// Load base S and T vertices
		inSt = (md3St_t *)((byte *)inSurface + LittleLong(inSurface->ofsSt));
		outSurface->st = outSt = Hunk_Alloc(outSurface->numVertices * sizeof(mdlSt_t));

		model->size += outSurface->numVertices * sizeof(mdlSt_t);

		for (j = 0; j < outSurface->numVertices; j++, inSt++, outSt++){
			outSt->st[0] = LittleFloat(inSt->st[0]);
			outSt->st[1] = LittleFloat(inSt->st[1]);
		}

		// Allocate space for face planes
		outSurface->facePlanes = Hunk_Alloc(outModel->numFrames * outSurface->numTriangles * sizeof(mdlFacePlane_t));
		model->size += outModel->numFrames * outSurface->numTriangles * sizeof(mdlFacePlane_t);

		// Load XYZ vertices
		inXyzNormal = (md3XyzNormal_t *)((byte *)inSurface + LittleLong(inSurface->ofsXyzNormals));
		outSurface->xyzNormals = outXyzNormal = Hunk_Alloc(outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t));

		model->size += outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t);

		inFrame = (md3Frame_t *)((byte *)inModel + LittleLong(inModel->ofsFrames));

		for (j = 0; j < outModel->numFrames; j++, inFrame++){
			scale[0] = MD3_XYZ_SCALE;
			scale[1] = MD3_XYZ_SCALE;
			scale[2] = MD3_XYZ_SCALE;

			translate[0] = LittleFloat(inFrame->localOrigin[0]);
			translate[1] = LittleFloat(inFrame->localOrigin[1]);
			translate[2] = LittleFloat(inFrame->localOrigin[2]);

			for (k = 0; k < outSurface->numVertices; k++, inXyzNormal++, outXyzNormal++){
				outXyzNormal->xyz[0] = (float)LittleShort(inXyzNormal->xyz[0]) * scale[0] + translate[0];
				outXyzNormal->xyz[1] = (float)LittleShort(inXyzNormal->xyz[1]) * scale[1] + translate[1];
				outXyzNormal->xyz[2] = (float)LittleShort(inXyzNormal->xyz[2]) * scale[2] + translate[2];

				lat = (float)((inXyzNormal->normal >> 8) & 0xFF) * M_PI/128.0;
				lng = (float)((inXyzNormal->normal >> 0) & 0xFF) * M_PI/128.0;

				outXyzNormal->normal[0] = sin(lng) * cos(lat);
				outXyzNormal->normal[1] = sin(lng) * sin(lat);
				outXyzNormal->normal[2] = cos(lng);
			}

			// Calculate tangent vectors
			R_CalcTangentVectors(outSurface->numTriangles, outSurface->triangles, outSurface->numVertices, outSurface->xyzNormals, outSurface->st, j);

			// Calculate face planes
			R_CalcFacePlanes(outSurface->numTriangles, outSurface->triangles, outSurface->facePlanes, outSurface->numVertices, outSurface->xyzNormals, j);
		}

		// Build triangle neighbors
		R_BuildTriangleNeighbors(outSurface->numTriangles, outSurface->triangles);

		// Skip to next surface
		inSurface = (md3Surface_t *)((byte *)inSurface + LittleLong(inSurface->ofsEnd));
	}

	return true;
}


// =====================================================================


/*
 =================
 R_LoadModel
 =================
*/
static model_t *R_LoadModel (const char *name, byte *data, modelType_t modelType){

	model_t		*model;
	unsigned	hash;

	if (r_numModels == MAX_MODELS)
		Com_Error(ERR_DROP, "R_LoadModel: MAX_MODELS hit");

	r_models[r_numModels++] = model = Z_Malloc(sizeof(model_t));

	// Fill it in
	Com_StripExtension(name, model->name, sizeof(model->name));
	Q_strncpyz(model->realName, name, sizeof(model->realName));
	model->size = 0;
	model->modelType = modelType;

	// Call the appropriate loader
	switch (modelType){
	case MODEL_MD3:
		if (!R_LoadMD3(model, data))
			model->modelType = MODEL_BAD;

		break;
	case MODEL_MD2:
		if (!R_LoadMD2(model, data))
			model->modelType = MODEL_BAD;

		break;
	case MODEL_BAD:

		break;
	}

	// Free model data
	if (data != NULL)
		FS_FreeFile(data);

	// Add to hash table
	hash = Com_HashKey(model->name, MODELS_HASH_SIZE);

	model->nextHash = r_modelsHashTable[hash];
	r_modelsHashTable[hash] = model;

	return model;
}

/*
 =================
 R_FindModel
 =================
*/
model_t *R_FindModel (const char *name){

	model_t		*model;
	byte		*data;
	char		checkName[MAX_OSPATH], loadName[MAX_OSPATH];
	unsigned	hash;
	int			i;
	
	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindModel: NULL model name");

	if (strlen(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "R_FindModel: model name exceeds MAX_OSPATH");

	// Inline models are grabbed only from worldModel
	if (name[0] == '*'){
		i = atoi(name+1);
		if (!tr.worldModel || (i < 1 || i >= tr.worldModel->numSubmodels))
			Com_Error(ERR_DROP, "R_FindModel: bad inline model number (%i)", i);

		return &r_inlineModels[i];
	}

	// Strip extension
	Com_StripExtension(name, checkName, sizeof(checkName));

	// See if already loaded
	hash = Com_HashKey(checkName, MODELS_HASH_SIZE);

	for (model = r_modelsHashTable[hash]; model; model = model->nextHash){
		if (!Q_stricmp(model->name, checkName))
			return model;
	}

	// Load it from disk
	Q_snprintfz(loadName, sizeof(loadName), "%s.md3", checkName);
	FS_LoadFile(loadName, (void **)&data);
	if (data)
		return R_LoadModel(loadName, data, MODEL_MD3);

	Q_snprintfz(loadName, sizeof(loadName), "%s.md2", checkName);
	FS_LoadFile(loadName, (void **)&data);
	if (data)
		return R_LoadModel(loadName, data, MODEL_MD2);

	// Not found
	Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find model '%s'\n", name);

	return R_LoadModel(name, NULL, MODEL_BAD);
}

/*
 =================
 R_RegisterModel
 =================
*/
model_t *R_RegisterModel (const char *name){

	return R_FindModel(name);
}

/*
 =================
 R_ModelFrames
 =================
*/
int R_ModelFrames (model_t *model){

	mdl_t	*alias = model->alias;

	if (model->modelType != MODEL_MD3 && model->modelType != MODEL_MD2)
		return 0;

	return alias->numFrames;
}

/*
 =================
 R_ModelBounds
 =================
*/
void R_ModelBounds (model_t *model, int curFrameNum, int oldFrameNum, vec3_t mins, vec3_t maxs){

	mdl_t		*alias = model->alias;
	mdlFrame_t	*curFrame, *oldFrame;

	if (model->modelType != MODEL_MD3 && model->modelType != MODEL_MD2){
		VectorCopy(model->mins, mins);
		VectorCopy(model->maxs, maxs);

		return;
	}

	if ((curFrameNum < 0 || curFrameNum >= alias->numFrames) || (oldFrameNum < 0 || oldFrameNum >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_ModelBounds: no such frame %i to %i (%s)\n", curFrameNum, oldFrameNum, model->realName);

		curFrameNum = 0;
		oldFrameNum = 0;
	}

	curFrame = alias->frames + curFrameNum;
	oldFrame = alias->frames + oldFrameNum;

	// Find bounds
	if (curFrame == oldFrame){
		VectorCopy(curFrame->mins, mins);
		VectorCopy(curFrame->maxs, maxs);
	}
	else {
		VectorMin(curFrame->mins, oldFrame->mins, mins);
		VectorMax(curFrame->maxs, oldFrame->maxs, maxs);
	}
}

/*
 =================
 R_ModelRadius
 =================
*/
float R_ModelRadius (model_t *model, int curFrameNum, int oldFrameNum){

	mdl_t		*alias = model->alias;
	mdlFrame_t	*curFrame, *oldFrame;

	if (model->modelType != MODEL_MD3 && model->modelType != MODEL_MD2)
		return model->radius;

	if ((curFrameNum < 0 || curFrameNum >= alias->numFrames) || (oldFrameNum < 0 || oldFrameNum >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_ModelRadius: no such frame %i to %i (%s)\n", curFrameNum, oldFrameNum, model->realName);

		curFrameNum = 0;
		oldFrameNum = 0;
	}

	curFrame = alias->frames + curFrameNum;
	oldFrame = alias->frames + oldFrameNum;

	// Find radius
	if (curFrame == oldFrame)
		return curFrame->radius;
	else {
		if (curFrame->radius > oldFrame->radius)
			return curFrame->radius;
		else
			return oldFrame->radius;
	}
}

/*
 =================
 R_LerpTag
 =================
*/
qboolean R_LerpTag (model_t *model, int curFrameNum, int oldFrameNum, float backLerp, const char *tagName, tag_t *tag){

	mdl_t		*alias = model->alias;
	mdlTag_t	*curTag, *oldTag;
	int			i;

	if (model->modelType != MODEL_MD3)
		return false;

	// Find the tag
	for (i = 0; i < alias->numTags; i++){
		if (!Q_stricmp(alias->tags[i].name, tagName))
			break;
	}

	if (i == alias->numTags){
		Com_DPrintf(S_COLOR_YELLOW "R_LerpTag: no such tag %s (%s)\n", tagName, model->realName);
		return false;
	}

	if ((curFrameNum < 0 || curFrameNum >= alias->numFrames) || (oldFrameNum < 0 || oldFrameNum >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_LerpTag: no such frame %i to %i (%s)\n", curFrameNum, oldFrameNum, model->realName);

		curFrameNum = 0;
		oldFrameNum = 0;
	}

	curTag = alias->tags + alias->numTags * curFrameNum + i;
	oldTag = alias->tags + alias->numTags * oldFrameNum + i;

	// Interpolate origin
	VectorLerp(curTag->origin, oldTag->origin, backLerp, tag->origin);

	// Interpolate axes
	VectorLerp(curTag->axis[0], oldTag->axis[0], backLerp, tag->axis[0]);
	VectorLerp(curTag->axis[1], oldTag->axis[1], backLerp, tag->axis[1]);
	VectorLerp(curTag->axis[2], oldTag->axis[2], backLerp, tag->axis[2]);

	// Normalize axes
	VectorNormalize(tag->axis[0]);
	VectorNormalize(tag->axis[1]);
	VectorNormalize(tag->axis[2]);

	return true;
}

/*
 =================
 R_ListModels_f
 =================
*/
void R_ListModels_f (void){

	model_t	*model;
	int		i, j;
	int		triangles, vertices;
	int		bytes = 0;

	Com_Printf("\n");
	Com_Printf("      surfs -tris--- -verts-- -size- -name--------\n");

	for (i = 0; i < r_numModels; i++){
		model = r_models[i];

		bytes += model->size;

		triangles = 0;
		vertices = 0;

		Com_Printf("%4i: ", i);

		switch (model->modelType){
		case MODEL_BSP:
			for (j = 0; j < model->numSurfaces; j++){
				triangles += model->surfaces[j].numTriangles;
				vertices += model->surfaces[j].numVertices;
			}

			Com_Printf("%5i ", model->numSurfaces);
			break;
		case MODEL_MD3:
		case MODEL_MD2:
			for (j = 0; j < model->alias->numSurfaces; j++){
				triangles += model->alias->surfaces[j].numTriangles;
				vertices += model->alias->surfaces[j].numVertices;
			}

			Com_Printf("%5i ", model->alias->numSurfaces);
			break;
		default:
			Com_Printf("    0 ");
			break;
		}

		Com_Printf("%8i %8i ", triangles, vertices);

		Com_Printf("%5ik ", model->size / 1024);

		Com_Printf("%s%s\n", model->realName, (model->modelType == MODEL_BAD) ? " (DEFAULTED)" : "");
	}

	Com_Printf("--------------------------------------------------\n");
	Com_Printf("%i total models\n", r_numModels);
	Com_Printf("%.2f total megabytes of models\n", bytes / 1048576.0);
	Com_Printf("\n");
}

/*
 =================
 R_InitModels
 =================
*/
void R_InitModels (void){

	// First entity is the world
	tr.worldEntity = &tr.scene.entities[0];
	memset(tr.worldEntity, 0, sizeof(renderEntity_t));

	tr.worldEntity->reType = RE_MODEL;
	tr.worldEntity->index = 0;
	tr.worldEntity->model = tr.worldModel;

	VectorClear(tr.worldEntity->origin);
	AxisClear(tr.worldEntity->axis);

	tr.worldEntity->customMaterial = NULL;

	tr.worldEntity->materialParms[MATERIALPARM_RED] = 1.0;
	tr.worldEntity->materialParms[MATERIALPARM_GREEN] = 1.0;
	tr.worldEntity->materialParms[MATERIALPARM_BLUE] = 1.0;
	tr.worldEntity->materialParms[MATERIALPARM_ALPHA] = 1.0;
	tr.worldEntity->materialParms[MATERIALPARM_TIME_OFFSET] = 0.0;
	tr.worldEntity->materialParms[MATERIALPARM_DIVERSITY] = 0.0;
	tr.worldEntity->materialParms[MATERIALPARM_GENERAL] = 0.0;
	tr.worldEntity->materialParms[MATERIALPARM_MODE] = 1.0;
}

/*
 =================
 R_ShutdownModels
 =================
*/
void R_ShutdownModels (void){

	model_t	*model;
	int		i;

	for (i = 0; i < r_numModels; i++){
		model = r_models[i];

		Z_Free(model);
	}

	memset(r_modelsHashTable, 0, sizeof(r_modelsHashTable));
	memset(r_models, 0, sizeof(r_models));

	r_numModels = 0;
}
