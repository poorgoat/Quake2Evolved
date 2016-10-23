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


#define MODELS_HASHSIZE		128

static model_t	*r_modelsHash[MODELS_HASHSIZE];
static model_t	*r_models[MAX_MODELS];
static int		r_numModels;

// The inline models from the current map are kept separate
static model_t	r_inlineModels[MAX_MODELS];

static byte		r_noMapVis[MAX_MAP_LEAFS/8];


/*
 =================
 R_PointInLeaf
 =================
*/
leaf_t *R_PointInLeaf (const vec3_t p){

	node_t		*node;
	cplane_t	*plane;
	float		d;

	if (!r_worldModel || !r_worldModel->nodes)
		Com_Error(ERR_DROP, "R_PointInLeaf: NULL worldmodel");

	node = r_worldModel->nodes;
	while (1){
		if (node->contents != -1)
			return (leaf_t *)node;

		plane = node->plane;
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct(p, plane->normal) - plane->dist;

		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return NULL;	// Never reached
}

/*
 =================
 R_DecompressVis
 =================
*/
static byte *R_DecompressVis (const byte *in){

	static byte	decompressed[MAX_MAP_LEAFS/8];
	byte		*out;
	int			c, row;

	row = (r_worldModel->vis->numClusters+7)>>3;	
	out = decompressed;

	if (!in){
		// No vis info, so make all visible
		while (row){
			*out++ = 0xff;
			row--;
		}

		return decompressed;		
	}

	do {
		if (*in){
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c){
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
	
	return decompressed;
}

/*
 =================
 R_ClusterPVS
 =================
*/
byte *R_ClusterPVS (int cluster){

	if (cluster == -1 || !r_worldModel || !r_worldModel->vis)
		return r_noMapVis;

	return R_DecompressVis((byte *)r_worldModel->vis + r_worldModel->vis->bitOfs[cluster][VIS_PVS]);
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
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", r_worldModel->realName);

	r_worldModel->numVertexes = l->fileLen / sizeof(dvertex_t);
	r_worldModel->vertexes = out = Hunk_Alloc(r_worldModel->numVertexes * sizeof(vertex_t));
	r_worldModel->size += r_worldModel->numVertexes * sizeof(vertex_t);

	for (i = 0; i < r_worldModel->numVertexes; i++, in++, out++){
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
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", r_worldModel->realName);

	r_worldModel->numEdges = l->fileLen / sizeof(dedge_t);
	r_worldModel->edges = out = Hunk_Alloc(r_worldModel->numEdges * sizeof(edge_t));
	r_worldModel->size += r_worldModel->numEdges * sizeof(edge_t);

	for (i = 0; i < r_worldModel->numEdges; i++, in++, out++){
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

	int	*in, *out;
	int	i;
	
	in = (int *)(data + l->fileOfs);
	if (l->fileLen % sizeof(int))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", r_worldModel->realName);

	r_worldModel->numSurfEdges = l->fileLen / sizeof(int);
	r_worldModel->surfEdges = out = Hunk_Alloc(r_worldModel->numSurfEdges * sizeof(int));
	r_worldModel->size += r_worldModel->numSurfEdges * sizeof(int);

	for (i = 0; i < r_worldModel->numSurfEdges; i++)
		out[i] = LittleLong(in[i]);
}

/*
 =================
 R_LoadLighting
 =================
*/
static void R_LoadLighting (const byte *data, const lump_t *l){

	if (r_fullbright->integer)
		return;

	if (!l->fileLen)
		return;

	r_worldModel->lightData = Hunk_Alloc(l->fileLen);
	r_worldModel->size += l->fileLen;

	memcpy(r_worldModel->lightData, data + l->fileOfs, l->fileLen);
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
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", r_worldModel->realName);

	r_worldModel->numPlanes = l->fileLen / sizeof(dplane_t);
	r_worldModel->planes = out = Hunk_Alloc(r_worldModel->numPlanes * sizeof(cplane_t));
	r_worldModel->size += r_worldModel->numPlanes * sizeof(cplane_t);

	for (i = 0; i < r_worldModel->numPlanes; i++, in++, out++){
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
static void R_GetTexSize (shader_t *shader, int *width, int *height){

	char			name[MAX_QPATH];
	fileHandle_t	f;
	mipTex_t		mt;
	int				i, j, k;

	// Look for a .wal texture first. This is so that retextures work.
	Q_snprintfz(name, sizeof(name), "%s.wal", shader->name);
	FS_FOpenFile(name, &f, FS_READ);
	if (f){
		// Found it, return its dimensions
		FS_Read(&mt, sizeof(mipTex_t), f);
		FS_FCloseFile(f);

		*width = LittleLong(mt.width);
		*height = LittleLong(mt.height);
		return;
	}

	// No, so see if the shader has a texture with the same name
	for (i = 0; i < shader->numStages; i++){
		for (j = 0; j < shader->stages[i]->numBundles; j++){
			if (shader->stages[i]->bundles[j]->texType != TEX_GENERIC)
				continue;

			for (k = 0; k < shader->stages[i]->bundles[j]->numTextures; k++){
				if (!Q_stricmp(shader->stages[i]->bundles[j]->textures[k]->name, shader->name)){
					// Yes, return its dimensions
					*width = shader->stages[i]->bundles[j]->textures[k]->sourceWidth;
					*height = shader->stages[i]->bundles[j]->textures[k]->sourceHeight;
					return;
				}
			}
		}
	}

	// No, so look for the first texture stage
	for (i = 0; i < shader->numStages; i++){
		for (j = 0; j < shader->stages[i]->numBundles; j++){
			if (shader->stages[i]->bundles[j]->texType != TEX_GENERIC)
				continue;

			// Found it, return its dimensions
			*width = shader->stages[i]->bundles[j]->textures[0]->sourceWidth;
			*height = shader->stages[i]->bundles[j]->textures[0]->sourceHeight;
			return;
		}
	}

	// Couldn't find shit, so just default to 64x64
	*width = 64;
	*height = 64;
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
	char		name[MAX_QPATH];
	unsigned	surfaceParm;

	in = (dtexinfo_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dtexinfo_t))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", r_worldModel->realName);

	r_worldModel->numTexInfo = l->fileLen / sizeof(dtexinfo_t);
	r_worldModel->texInfo = out = Hunk_Alloc(r_worldModel->numTexInfo * sizeof(texInfo_t));
	r_worldModel->size += r_worldModel->numTexInfo * sizeof(texInfo_t);

	for (i = 0; i < r_worldModel->numTexInfo; i++, in++, out++){
		for (j = 0; j < 4; j++){
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		out->flags = LittleLong(in->flags);

		next = LittleLong(in->nextTexInfo);
		if (next > 0)
			out->next = r_worldModel->texInfo + next;

		if (out->flags & (SURF_SKY | SURF_NODRAW)){
			// This is not actually needed
			out->shader = r_defaultShader;
			continue;
		}

		// Get surfaceParm
		if (out->flags & (SURF_WARP|SURF_TRANS33|SURF_TRANS66)){
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
			surfaceParm = SURFACEPARM_LIGHTMAP;

			if (out->flags & SURF_FLOWING)
				surfaceParm |= SURFACEPARM_FLOWING;
		}

		// Performance evaluation option
		if (r_singleShader->integer){
			out->shader = r_defaultShader;
			continue;
		}

		// Lightmap visualization tool
		if (r_lightmap->integer && (surfaceParm & SURFACEPARM_LIGHTMAP)){
			out->shader = r_lightmapShader;
			continue;
		}

		// Load the shader
		Q_snprintfz(name, sizeof(name), "textures/%s", in->texture);
		out->shader = R_FindShader(name, SHADER_BSP, surfaceParm);
	}

	// Find texture dimensions and count animation frames
	for (i = 0, out = r_worldModel->texInfo; i < r_worldModel->numTexInfo; i++, out++){
		R_GetTexSize(out->shader, &out->width, &out->height);

		out->numFrames = 1;
		for (step = out->next; step && step != out; step = step->next)
			out->numFrames++;
	}
}

/*
 =================
 R_CalcSurfaceBounds

 Fills in surf->mins and surf->maxs
 =================
*/
static void R_CalcSurfaceBounds (surface_t *surf){

	int			i, e;
	vertex_t	*v;

	ClearBounds(surf->mins, surf->maxs);

	for (i = 0; i < surf->numEdges; i++){
		e = r_worldModel->surfEdges[surf->firstEdge + i];
		if (e >= 0)
			v = &r_worldModel->vertexes[r_worldModel->edges[e].v[0]];
		else
			v = &r_worldModel->vertexes[r_worldModel->edges[-e].v[1]];

		AddPointToBounds(v->point, surf->mins, surf->maxs);
	}
}

/*
 =================
 R_CalcSurfaceExtents

 Fills in surf->textureMins and surf->extents
 =================
*/
static void R_CalcSurfaceExtents (surface_t *surf){

	float		mins[2], maxs[2], val;
	int			bmins[2], bmaxs[2];
	int			i, j, e;
	vertex_t	*v;

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -999999;

	for (i = 0; i < surf->numEdges; i++){
		e = r_worldModel->surfEdges[surf->firstEdge + i];
		if (e >= 0)
			v = &r_worldModel->vertexes[r_worldModel->edges[e].v[0]];
		else
			v = &r_worldModel->vertexes[r_worldModel->edges[-e].v[1]];

		for (j = 0; j < 2; j++){
			val = DotProduct(v->point, surf->texInfo->vecs[j]) + surf->texInfo->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i = 0; i < 2; i++){
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);

		surf->textureMins[i] = bmins[i] * 16;
		surf->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}

/*
 =================
 R_SubdividePolygon
 =================
*/
static void R_SubdividePolygon (surface_t *surf, int numVerts, float *verts){

	int			i, j;
	vec3_t		mins, maxs;
	float		*v;
	vec3_t		front[64], back[64];
	int			f, b;
	float		m, dist, dists[64];
	int			subdivideSize;
	unsigned	index;
	float		s, t;
	vec3_t		total;
	vec2_t		totalST, totalLM;
	surfPoly_t	*p;
	texInfo_t	*tex = surf->texInfo;

	subdivideSize = tex->shader->tessSize;

	ClearBounds(mins, maxs);
	for (i = 0, v = verts; i < numVerts; i++, v += 3)
		AddPointToBounds(v, mins, maxs);

	for (i = 0; i < 3; i++){
		m = subdivideSize * floor(((mins[i] + maxs[i]) * 0.5) / subdivideSize + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// Cut it
		v = verts + i;
		for (j = 0; j < numVerts; j++, v += 3)
			dists[j] = *v - m;

		// Wrap cases
		dists[j] = dists[0];
		v -= i;
		VectorCopy(verts, v);

		f = b = 0;

		for (j = 0, v = verts; j < numVerts; j++, v += 3){
			if (dists[j] >= 0){
				VectorCopy(v, front[f]);
				f++;
			}
			if (dists[j] <= 0){
				VectorCopy(v, back[b]);
				b++;
			}
			
			if (dists[j] == 0 || dists[j+1] == 0)
				continue;
			
			if ((dists[j] > 0) != (dists[j+1] > 0)){
				// Clip point
				dist = dists[j] / (dists[j] - dists[j+1]);

				front[f][0] = back[b][0] = v[0] + (v[3] - v[0]) * dist;
				front[f][1] = back[b][1] = v[1] + (v[4] - v[1]) * dist;
				front[f][2] = back[b][2] = v[2] + (v[5] - v[2]) * dist;

				f++;
				b++;
			}
		}

		R_SubdividePolygon(surf, f, front[0]);
		R_SubdividePolygon(surf, b, back[0]);
		return;
	}

	p = Hunk_Alloc(sizeof(surfPoly_t));
	p->next = surf->poly;
	surf->poly = p;

	r_worldModel->size += sizeof(surfPoly_t);

	// Create indices
	p->numIndices = (numVerts * 3);
	p->indices = Hunk_Alloc(p->numIndices * sizeof(unsigned));

	r_worldModel->size += p->numIndices * sizeof(unsigned);

	for (i = 0, index = 2; i < p->numIndices; i += 3, index++){
		p->indices[i+0] = 0;
		p->indices[i+1] = index-1;
		p->indices[i+2] = index;
	}

	// Create vertices
	p->numVertices = (numVerts + 2);
	p->vertices = Hunk_Alloc(p->numVertices * sizeof(surfPolyVert_t));

	r_worldModel->size += p->numVertices * sizeof(surfPolyVert_t);
	
	VectorClear(total);
	totalST[0] = totalST[1] = 0;
	totalLM[0] = totalLM[1] = 0;

	for (i = 0; i < numVerts; i++, verts += 3){
		// Vertex
		VectorCopy(verts, p->vertices[i+1].xyz);

		VectorAdd(total, verts, total);

		// Texture coordinates
		s = DotProduct(verts, tex->vecs[0]) + tex->vecs[0][3];
		s /= tex->width;

		t = DotProduct(verts, tex->vecs[1]) + tex->vecs[1][3];
		t /= tex->height;

		p->vertices[i+1].st[0] = s;
		p->vertices[i+1].st[1] = t;

		totalST[0] += s;
		totalST[1] += t;

		// Lightmap texture coordinates
		s = DotProduct(verts, tex->vecs[0]) + tex->vecs[0][3] - surf->textureMins[0];
		s += surf->lmS * 16;
		s += 8;
		s /= LIGHTMAP_WIDTH * 16;

		t = DotProduct(verts, tex->vecs[1]) + tex->vecs[1][3] - surf->textureMins[1];
		t += surf->lmT * 16;
		t += 8;
		t /= LIGHTMAP_HEIGHT * 16;

		p->vertices[i+1].lightmap[0] = s;
		p->vertices[i+1].lightmap[1] = t;

		totalLM[0] += s;
		totalLM[1] += t;

		// Vertex color
		p->vertices[i+1].color[0] = 255;
		p->vertices[i+1].color[1] = 255;
		p->vertices[i+1].color[2] = 255;
		p->vertices[i+1].color[3] = 255;

		if (tex->flags & SURF_TRANS33)
			p->vertices[i+1].color[3] *= 0.33;
		else if (tex->flags & SURF_TRANS66)
			p->vertices[i+1].color[3] *= 0.66;
	}

	// Vertex
	VectorScale(total, 1.0 / numVerts, p->vertices[0].xyz);

	// Texture coordinates
	p->vertices[0].st[0] = totalST[0] / numVerts;
	p->vertices[0].st[1] = totalST[1] / numVerts;

	// Lightmap texture coordinates
	p->vertices[0].lightmap[0] = totalLM[0] / numVerts;
	p->vertices[0].lightmap[1] = totalLM[1] / numVerts;

	// Vertex color
	p->vertices[0].color[0] = 255;
	p->vertices[0].color[1] = 255;
	p->vertices[0].color[2] = 255;
	p->vertices[0].color[3] = 255;

	if (tex->flags & SURF_TRANS33)
		p->vertices[0].color[3] *= 0.33;
	else if (tex->flags & SURF_TRANS66)
		p->vertices[0].color[3] *= 0.66;

	// Copy first vertex to last
	memcpy(&p->vertices[i+1], &p->vertices[1], sizeof(surfPolyVert_t));
}

/*
 =================
 R_BuildPolygon
 =================
*/
static void R_BuildPolygon (surface_t *surf, int numVerts, float *verts){

	int			i;
	unsigned	index;
	float		s, t;
	surfPoly_t	*p;
	texInfo_t	*tex = surf->texInfo;
	
	p = Hunk_Alloc(sizeof(surfPoly_t));
	p->next = surf->poly;
	surf->poly = p;

	r_worldModel->size += sizeof(surfPoly_t);

	// Create indices
	p->numIndices = (numVerts - 2) * 3;
	p->indices = Hunk_Alloc(p->numIndices * sizeof(unsigned));

	r_worldModel->size += p->numIndices * sizeof(unsigned);

	for (i = 0, index = 2; i < p->numIndices; i += 3, index++){
		p->indices[i+0] = 0;
		p->indices[i+1] = index-1;
		p->indices[i+2] = index;
	}

	// Create vertices
	p->numVertices = numVerts;
	p->vertices = Hunk_Alloc(p->numVertices * sizeof(surfPolyVert_t));

	r_worldModel->size += p->numVertices * sizeof(surfPolyVert_t);
	
	for (i = 0; i < numVerts; i++, verts += 3){
		// Vertex
		VectorCopy(verts, p->vertices[i].xyz);

		// Texture coordinates
		s = DotProduct(verts, tex->vecs[0]) + tex->vecs[0][3];
		s /= tex->width;

		t = DotProduct(verts, tex->vecs[1]) + tex->vecs[1][3];
		t /= tex->height;

		p->vertices[i].st[0] = s;
		p->vertices[i].st[1] = t;

		// Lightmap texture coordinates
		s = DotProduct(verts, tex->vecs[0]) + tex->vecs[0][3] - surf->textureMins[0];
		s += surf->lmS * 16;
		s += 8;
		s /= LIGHTMAP_WIDTH * 16;

		t = DotProduct(verts, tex->vecs[1]) + tex->vecs[1][3] - surf->textureMins[1];
		t += surf->lmT * 16;
		t += 8;
		t /= LIGHTMAP_HEIGHT * 16;

		p->vertices[i].lightmap[0] = s;
		p->vertices[i].lightmap[1] = t;

		// Vertex color
		p->vertices[i].color[0] = 255;
		p->vertices[i].color[1] = 255;
		p->vertices[i].color[2] = 255;
		p->vertices[i].color[3] = 255;

		if (tex->flags & SURF_TRANS33)
			p->vertices[i].color[3] *= 0.33;
		else if (tex->flags & SURF_TRANS66)
			p->vertices[i].color[3] *= 0.66;
	}
}

/*
 =================
 R_BuildSurfacePolygons
 =================
*/
static void R_BuildSurfacePolygons (surface_t *surf){

	int			i, e;
	vertex_t	*v;
	vec3_t		verts[64];

	// Convert edges back to a normal polygon
	for (i = 0; i < surf->numEdges; i++){
		e = r_worldModel->surfEdges[surf->firstEdge + i];
		if (e >= 0)
			v = &r_worldModel->vertexes[r_worldModel->edges[e].v[0]];
		else
			v = &r_worldModel->vertexes[r_worldModel->edges[-e].v[1]];

		VectorCopy(v->point, verts[i]);
	}

	if (surf->texInfo->shader->flags & SHADER_TESSSIZE)
		R_SubdividePolygon(surf, surf->numEdges, verts[0]);
	else
		R_BuildPolygon(surf, surf->numEdges, verts[0]);
}

/*
 =================
 R_LoadFaces
 =================
*/
static void R_LoadFaces (const byte *data, const lump_t *l){

	dface_t		*in;
	surface_t 	*out;
	int			i, lightOfs;

	in = (dface_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dface_t))
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", r_worldModel->realName);

	r_worldModel->numSurfaces = l->fileLen / sizeof(dface_t);
	r_worldModel->surfaces = out = Hunk_Alloc(r_worldModel->numSurfaces * sizeof(surface_t));
	r_worldModel->size += r_worldModel->numSurfaces * sizeof(surface_t);

	R_BeginBuildingLightmaps();

	for (i = 0; i < r_worldModel->numSurfaces; i++, in++, out++){
		out->firstEdge = LittleLong(in->firstEdge);
		out->numEdges = LittleShort(in->numEdges);

		if (LittleShort(in->side))
			out->flags |= SURF_PLANEBACK;

		out->plane = r_worldModel->planes + LittleShort(in->planeNum);
		out->texInfo = r_worldModel->texInfo + LittleShort(in->texInfo);

		R_CalcSurfaceBounds(out);

		R_CalcSurfaceExtents(out);

		// Tangent vectors
		VectorCopy(out->texInfo->vecs[0], out->tangent);
		VectorNegate(out->texInfo->vecs[1], out->binormal);

		if (!(out->flags & SURF_PLANEBACK))
			VectorCopy(out->plane->normal, out->normal);
		else
			VectorNegate(out->plane->normal, out->normal);

		VectorNormalize(out->tangent);
		VectorNormalize(out->binormal);
		VectorNormalize(out->normal);

		// Lighting info
		out->lmWidth = (out->extents[0] >> 4) + 1;
		out->lmHeight = (out->extents[1] >> 4) + 1;

		if (out->texInfo->flags & (SURF_SKY | SURF_WARP | SURF_NODRAW))
			lightOfs = -1;
		else
			lightOfs = LittleLong(in->lightOfs);

		if (r_worldModel->lightData && lightOfs != -1)
			out->lmSamples = r_worldModel->lightData + lightOfs;

		while (out->numStyles < MAX_STYLES && in->styles[out->numStyles] != 255){
			out->styles[out->numStyles] = in->styles[out->numStyles];
			out->numStyles++;
		}

		// Create lightmap
		R_BuildSurfaceLightmap(out);

		// Create polygons
		R_BuildSurfacePolygons(out);
	}

	R_EndBuildingLightmaps();
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
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", r_worldModel->realName);

	r_worldModel->numMarkSurfaces = l->fileLen / sizeof(short);
	r_worldModel->markSurfaces = out = Hunk_Alloc(r_worldModel->numMarkSurfaces * sizeof(surface_t *));
	r_worldModel->size += r_worldModel->numMarkSurfaces * sizeof(surface_t *);

	for (i = 0; i < r_worldModel->numMarkSurfaces; i++)
		out[i] = r_worldModel->surfaces + LittleShort(in[i]);
}

/*
 =================
 R_LoadVisibility
 =================
*/
static void R_LoadVisibility (const byte *data, const lump_t *l){

	int		i;

	if (!l->fileLen)
		return;

	r_worldModel->vis = Hunk_Alloc(l->fileLen);
	r_worldModel->size += l->fileLen;

	memcpy(r_worldModel->vis, data + l->fileOfs, l->fileLen);

	r_worldModel->vis->numClusters = LittleLong(r_worldModel->vis->numClusters);
	for (i = 0; i < r_worldModel->vis->numClusters; i++){
		r_worldModel->vis->bitOfs[i][0] = LittleLong(r_worldModel->vis->bitOfs[i][0]);
		r_worldModel->vis->bitOfs[i][1] = LittleLong(r_worldModel->vis->bitOfs[i][1]);
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
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", r_worldModel->realName);

	r_worldModel->numLeafs = l->fileLen / sizeof(dleaf_t);
	r_worldModel->leafs = out = Hunk_Alloc(r_worldModel->numLeafs * sizeof(leaf_t));
	r_worldModel->size += r_worldModel->numLeafs * sizeof(leaf_t);

	for (i = 0; i < r_worldModel->numLeafs; i++, in++, out++){
		for (j = 0; j < 3; j++){
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}

		out->contents = LittleLong(in->contents);
		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		out->firstMarkSurface = r_worldModel->markSurfaces + LittleShort(in->firstLeafFace);
		out->numMarkSurfaces = LittleShort(in->numLeafFaces);

		// Mark the surfaces for caustics
		if (out->contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
			for (j = 0; j < out->numMarkSurfaces; j++){
				if (out->firstMarkSurface[j]->texInfo->flags & SURF_WARP)
					continue;	// HACK: ignore warped surfaces

				if (out->contents & CONTENTS_WATER)
					out->firstMarkSurface[j]->flags |= SURF_WATERCAUSTICS;
				if (out->contents & CONTENTS_SLIME)
					out->firstMarkSurface[j]->flags |= SURF_SLIMECAUSTICS;
				if (out->contents & CONTENTS_LAVA)
					out->firstMarkSurface[j]->flags |= SURF_LAVACAUSTICS;
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
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", r_worldModel->realName);

	r_worldModel->numNodes = l->fileLen / sizeof(dnode_t);
	r_worldModel->nodes = out = Hunk_Alloc(r_worldModel->numNodes * sizeof(node_t));
	r_worldModel->size += r_worldModel->numNodes * sizeof(node_t);

	for (i = 0; i < r_worldModel->numNodes; i++, in++, out++){
		for (j = 0; j < 3; j++){
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}
	
		out->plane = r_worldModel->planes + LittleLong(in->planeNum);

		out->contents = -1;
		out->firstSurface = LittleShort(in->firstFace);
		out->numSurfaces = LittleShort(in->numFaces);

		for (j = 0; j < 2; j++){
			p = LittleLong(in->children[j]);
			if (p >= 0)
				out->children[j] = r_worldModel->nodes + p;
			else
				out->children[j] = (node_t *)(r_worldModel->leafs + (-1 - p));
		}
	}

	// Set nodes and leafs
	R_SetParent(r_worldModel->nodes, NULL);
}

/*
 =================
 R_SetupSubmodels
 =================
*/
static void R_SetupSubmodels (void){

	int			i;
	submodel_t	*bm;
	model_t		*model;

	for (i = 0; i < r_worldModel->numSubmodels; i++){
		bm = &r_worldModel->submodels[i];
		model = &r_inlineModels[i];

		*model = *r_worldModel;

		model->numModelSurfaces = bm->numFaces;
		model->firstModelSurface = bm->firstFace;
		model->firstNode = bm->headNode;
		VectorCopy(bm->maxs, model->maxs);
		VectorCopy(bm->mins, model->mins);
		model->radius = bm->radius;

		if (i == 0)
			*r_worldModel = *model;

		model->numLeafs = bm->visLeafs;
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
		Com_Error(ERR_DROP, "R_LoadWorldMap: funny lump size in '%s'", r_worldModel->realName);

	r_worldModel->numSubmodels = l->fileLen / sizeof(dmodel_t);
	r_worldModel->submodels = out = Hunk_Alloc(r_worldModel->numSubmodels * sizeof(submodel_t));
	r_worldModel->size += r_worldModel->numSubmodels * sizeof(submodel_t);

	for (i = 0; i < r_worldModel->numSubmodels; i++, in++, out++){
		for (j = 0; j < 3; j++){
			// Spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;

			out->origin[j] = LittleFloat(in->origin[j]);
		}

		out->radius = RadiusFromBounds(out->mins, out->maxs);

		out->headNode = LittleLong(in->headNode);
		out->firstFace = LittleLong(in->firstFace);
		out->numFaces = LittleLong(in->numFaces);
	}

	// Set up the submodels
	R_SetupSubmodels();
}

/*
 =================
 R_LoadLightgridFile
 =================
*/
static void R_LoadLightgridFile (void){

	char				name[MAX_QPATH];
	byte				*data;
	lightGridHeader_t	*header;
	lightGrid_t			*in, *out;
	int					i;

	// Load the file
	Q_snprintfz(name, sizeof(name), "%s.lightgrid", r_worldModel->name);
	FS_LoadFile(name, (void **)&data);
	if (!data)
		return;

	header = (lightGridHeader_t *)data;

	// Byte swap the header fields and sanity check
	if (LittleLong(header->ident) != Q2EL_IDENT){
		Com_DPrintf(S_COLOR_RED "R_LoadLightgridFile: '%s' has wrong file id\n", name);
		FS_FreeFile(data);
		return;
	}

	if (LittleLong(header->version) != Q2EL_VERSION){
		Com_DPrintf(S_COLOR_RED "R_LoadLightgridFile: '%s' has wrong version number (%i should be %i)\n", name, LittleLong(header->version), Q2EL_VERSION);
		FS_FreeFile(data);
		return;
	}

	// Load into heap
	r_worldModel->gridMins[0] = LittleFloat(header->gridMins[0]);
	r_worldModel->gridMins[1] = LittleFloat(header->gridMins[1]);
	r_worldModel->gridMins[2] = LittleFloat(header->gridMins[2]);

	r_worldModel->gridSize[0] = LittleFloat(header->gridSize[0]);
	r_worldModel->gridSize[1] = LittleFloat(header->gridSize[1]);
	r_worldModel->gridSize[2] = LittleFloat(header->gridSize[2]);

	r_worldModel->gridBounds[0] = LittleLong(header->gridBounds[0]);
	r_worldModel->gridBounds[1] = LittleLong(header->gridBounds[1]);
	r_worldModel->gridBounds[2] = LittleLong(header->gridBounds[2]);
	r_worldModel->gridBounds[3] = LittleLong(header->gridBounds[3]);

	r_worldModel->gridPoints = LittleLong(header->gridPoints);

	in = (lightGrid_t *)(data + sizeof(lightGridHeader_t));

	r_worldModel->lightGrid = out = Hunk_Alloc(r_worldModel->gridPoints * sizeof(lightGrid_t));
	r_worldModel->size += r_worldModel->gridPoints * sizeof(lightGrid_t);

	for (i = 0; i < r_worldModel->gridPoints; i++, in++, out++){
		out->lightDir[0] = LittleFloat(in->lightDir[0]);
		out->lightDir[1] = LittleFloat(in->lightDir[1]);
		out->lightDir[2] = LittleFloat(in->lightDir[2]);
	}

	FS_FreeFile(data);
}

/*
 =================
 R_LoadWorldMap
 =================
*/
void R_LoadWorldMap (const char *mapName, const char *skyName, float skyRotate, const vec3_t skyAxis){

	int			i;
	byte		*data;
	dheader_t	*header;
	unsigned	hashKey;

	if (r_worldModel)
		Com_Error(ERR_DROP, "R_LoadWorldMap: attempted to redundantly load world map");

	// Load the file
	FS_LoadFile(mapName, (void **)&data);
	if (!data)
		Com_Error(ERR_DROP, "R_LoadWorldMap: '%s' not found", mapName);

	r_models[r_numModels++] = r_worldModel = Hunk_Alloc(sizeof(model_t));

	// Fill it in
	Com_StripExtension(mapName, r_worldModel->name, sizeof(r_worldModel->name));
	Q_strncpyz(r_worldModel->realName, mapName, sizeof(r_worldModel->realName));
	r_worldModel->modelType = MODEL_BSP;

	header = (dheader_t *)data;

	// Byte swap the header fields and sanity check
	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int *)header)[i] = LittleLong(((int *)header)[i]);

	if (header->ident != BSP_IDENT)
		Com_Error(ERR_DROP, "R_LoadWorldMap: '%s' has wrong file id", r_worldModel->realName);

	if (header->version != BSP_VERSION)
		Com_Error(ERR_DROP, "R_LoadWorldMap: '%s' has wrong version number (%i should be %i)", r_worldModel->realName, header->version, BSP_VERSION);

	// Load into heap
	R_LoadVertexes(data, &header->lumps[LUMP_VERTEXES]);
	R_LoadEdges(data, &header->lumps[LUMP_EDGES]);
	R_LoadSurfEdges(data, &header->lumps[LUMP_SURFEDGES]);
	R_LoadLighting(data, &header->lumps[LUMP_LIGHTING]);
	R_LoadPlanes(data, &header->lumps[LUMP_PLANES]);
	R_LoadTexInfo(data, &header->lumps[LUMP_TEXINFO]);
	R_LoadFaces(data, &header->lumps[LUMP_FACES]);
	R_LoadMarkSurfaces(data, &header->lumps[LUMP_LEAFFACES]);
	R_LoadVisibility(data, &header->lumps[LUMP_VISIBILITY]);
	R_LoadLeafs(data, &header->lumps[LUMP_LEAFS]);
	R_LoadNodes(data, &header->lumps[LUMP_NODES]);
	R_LoadSubmodels(data, &header->lumps[LUMP_MODELS]);

	FS_FreeFile(data);

	// Set up some needed things
	r_worldEntity->model = r_worldModel;

	R_SetupSky(skyName, skyRotate, skyAxis);

	R_LoadLightgridFile();

	// Load some needed shaders
	r_waterCausticsShader = R_FindShader("waterCaustics", SHADER_BSP, 0);
	r_slimeCausticsShader = R_FindShader("slimeCaustics", SHADER_BSP, 0);
	r_lavaCausticsShader = R_FindShader("lavaCaustics", SHADER_BSP, 0);

	// Add to hash table
	hashKey = Com_HashKey(r_worldModel->name, MODELS_HASHSIZE);

	r_worldModel->nextHash = r_modelsHash[hashKey];
	r_modelsHash[hashKey] = r_worldModel;
}


/*
 =======================================================================

 ALIAS MODELS

 =======================================================================
*/


/*
 =================
 R_CalcTangentSpace
 =================
*/
static void R_CalcTangentSpace (int numTriangles, mdlTriangle_t *triangles, int numVertices, mdlXyzNormal_t *xyzNormals, mdlSt_t *st, int frame){

	mdlTriangle_t	*triangle;
	mdlXyzNormal_t	*xyzNormal;
	vec3_t			tempSVectors[MAX_VERTICES], tempTVectors[MAX_VERTICES];
	vec3_t			sVector, tVector, normal;
	float			*pXyz[3], *pSt[3];
	vec3_t			edge[2], cross;
	int				i, j;

	xyzNormals += numVertices * frame;

	memset(tempSVectors, 0, numVertices * sizeof(vec3_t));
	memset(tempTVectors, 0, numVertices * sizeof(vec3_t));

	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		pXyz[0] = (float *)(xyzNormals[triangle->index[0]].xyz);
		pXyz[1] = (float *)(xyzNormals[triangle->index[1]].xyz);
		pXyz[2] = (float *)(xyzNormals[triangle->index[2]].xyz);

		pSt[0] = (float *)(st[triangle->index[0]].st);
		pSt[1] = (float *)(st[triangle->index[1]].st);
		pSt[2] = (float *)(st[triangle->index[2]].st);

		// Find S and T vectors
		edge[0][1] = pSt[1][0] - pSt[0][0];
		edge[0][2] = pSt[1][1] - pSt[0][1];

		edge[1][1] = pSt[2][0] - pSt[0][0];
		edge[1][2] = pSt[2][1] - pSt[0][1];

		for (j = 0; j < 3; j++){
			edge[0][0] = pXyz[1][j] - pXyz[0][j];
			edge[1][0] = pXyz[2][j] - pXyz[0][j];

			CrossProduct(edge[0], edge[1], cross);

			if (cross[0]){
				sVector[j] = -cross[1] / cross[0];
				tVector[j] = -cross[2] / cross[0];
			}
			else {
				sVector[j] = 0;
				tVector[j] = 0;
			}
		}

		// Compute an orthogonal basis
		CrossProduct(sVector, tVector, normal);
		CrossProduct(normal, sVector, tVector);

		for (j = 0; j < 3; j++){
			VectorAdd(tempSVectors[triangle->index[j]], sVector, tempSVectors[triangle->index[j]]);
			VectorAdd(tempTVectors[triangle->index[j]], tVector, tempTVectors[triangle->index[j]]);
		}
	}

	// Normalize and store as lattitude/longitude encoded vectors (note
	// that S becomes binormal and T becomes tangent)
	for (i = 0, xyzNormal = xyzNormals; i < numVertices; i++, xyzNormal++){
		VectorNormalize(tempSVectors[i]);
		VectorNormalize(tempTVectors[i]);

		NormalToLatLong(tempTVectors[i], xyzNormal->tangent);
		NormalToLatLong(tempSVectors[i], xyzNormal->binormal);
	}
}

/*
 =================
 R_FindTriangleWithEdge
 =================
*/
static int R_FindTriangleWithEdge (int numTriangles, mdlTriangle_t *triangles, unsigned start, unsigned end, int ignore){

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
static void R_BuildTriangleNeighbors (int numTriangles, mdlTriangle_t *triangles, mdlNeighbor_t *neighbors){

	mdlTriangle_t	*triangle;
	mdlNeighbor_t	*neighbor;
	int				i;

	for (i = 0, triangle = triangles, neighbor = neighbors; i < numTriangles; i++, triangle++, neighbor++){
		neighbor->index[0] = R_FindTriangleWithEdge(numTriangles, triangles, triangle->index[1], triangle->index[0], i);
		neighbor->index[1] = R_FindTriangleWithEdge(numTriangles, triangles, triangle->index[2], triangle->index[1], i);
		neighbor->index[2] = R_FindTriangleWithEdge(numTriangles, triangles, triangle->index[0], triangle->index[2], i);
	}
}

/*
 =================
 R_SetupAmmoDisplay
 =================
*/
static void R_SetupAmmoDisplay (shader_t *shader, mdlAmmoDisplay_t *ammoDisplay){

	int		i;

	ammoDisplay->type = shader->ammoDisplay.type;
	ammoDisplay->lowAmmo = shader->ammoDisplay.lowAmmo;

	for (i = 0; i < 3; i++){
		if (!shader->ammoDisplay.remapShaders[i][0])
			continue;

		ammoDisplay->remapShaders[i] = R_FindShader(shader->ammoDisplay.remapShaders[i], SHADER_SKIN, 0);

		// Mark as ammo display shader
		ammoDisplay->remapShaders[i]->flags |= SHADER_AMMODISPLAY;
	}
}

/*
 =================
 R_LoadMD2
 =================
*/
static void R_LoadMD2 (model_t *model, const byte *data){

	md2Header_t		*inModel;
	md2St_t			*inSt;
	md2Triangle_t	*inTriangle;
	md2Frame_t		*inFrame;
	mdl_t			*outModel;
	mdlFrame_t		*outFrame;
	mdlSurface_t	*outSurface;
	mdlShader_t		*outShader;
	unsigned		*outIndices;
	mdlSt_t			*outSt;
	mdlXyzNormal_t	*outXyzNormal;
	int				i, j;
	int				ident, version;
	int				indRemap[MD2_MAX_TRIANGLES*3];
	unsigned		tempIndex[MD2_MAX_TRIANGLES*3], tempStIndex[MD2_MAX_TRIANGLES*3];
	int				numIndices, numVertices;
	double			skinWidth, skinHeight;
	vec3_t			normal;
	char			name[MAX_QPATH];

	inModel = (md2Header_t *)data;
	model->alias[model->numAlias++] = outModel = Hunk_Alloc(sizeof(mdl_t));
	
	model->size += sizeof(mdl_t);

	// Byte swap the header fields and sanity check
	ident = LittleLong(inModel->ident);
	if (ident != MD2_IDENT)
		Com_Error(ERR_DROP, "R_LoadMD2: '%s' has wrong file id", model->realName);

	version = LittleLong(inModel->version);
	if (version != MD2_VERSION)
		Com_Error(ERR_DROP, "R_LoadMD2: '%s' has wrong version number (%i should be %i)", model->realName, version, MD2_VERSION);

	outModel->numFrames = LittleLong(inModel->numFrames);
	if (outModel->numFrames > MD2_MAX_FRAMES || outModel->numFrames <= 0)
		Com_Error(ERR_DROP, "R_LoadMD2: '%s' has invalid number of frames (%i)", model->realName, outModel->numFrames);

	outModel->numTags = 0;
	outModel->numSurfaces = 1;

	skinWidth = 1.0 / (double)LittleLong(inModel->skinWidth);
	skinHeight = 1.0 / (double)LittleLong(inModel->skinHeight);

	outModel->surfaces = outSurface = Hunk_Alloc(sizeof(mdlSurface_t));

	model->size += sizeof(mdlSurface_t);

	outSurface->numTriangles = LittleLong(inModel->numTris);
	if (outSurface->numTriangles > MD2_MAX_TRIANGLES || outSurface->numTriangles <= 0)
		Com_Error(ERR_DROP, "R_LoadMD2: '%s' has invalid number of triangles (%i)", model->realName, outSurface->numTriangles);

	outSurface->numVertices = LittleLong(inModel->numXyz);
	if (outSurface->numVertices > MD2_MAX_VERTS || outSurface->numVertices <= 0)
		Com_Error(ERR_DROP, "R_LoadMD2: '%s' has invalid number of vertices (%i)", model->realName, outSurface->numVertices);
	
	outSurface->numShaders = LittleLong(inModel->numSkins);
	if (outSurface->numShaders > MD2_MAX_SKINS || outSurface->numShaders < 0)
		Com_Error(ERR_DROP, "R_LoadMD2: '%s' has invalid number of skins (%i)", model->realName, outSurface->numShaders);

	// Load triangle lists
	inTriangle = (md2Triangle_t *)((byte *)inModel + LittleLong(inModel->ofsTris));

	for (i = 0; i < outSurface->numTriangles; i++){
		tempIndex[i*3+0] = (unsigned)LittleShort(inTriangle[i].indexXyz[0]);
		tempIndex[i*3+1] = (unsigned)LittleShort(inTriangle[i].indexXyz[1]);
		tempIndex[i*3+2] = (unsigned)LittleShort(inTriangle[i].indexXyz[2]);

		tempStIndex[i*3+0] = (unsigned)LittleShort(inTriangle[i].indexSt[0]);
		tempStIndex[i*3+1] = (unsigned)LittleShort(inTriangle[i].indexSt[1]);
		tempStIndex[i*3+2] = (unsigned)LittleShort(inTriangle[i].indexSt[2]);
	}

	// Build list of unique vertices
	numIndices = outSurface->numTriangles * 3;
	numVertices = 0;

	outSurface->triangles = outIndices = Hunk_Alloc(outSurface->numTriangles * sizeof(mdlTriangle_t));

	model->size += outSurface->numTriangles * sizeof(mdlTriangle_t);

	memset(indRemap, -1, MD2_MAX_TRIANGLES * 3 * sizeof(int));

	for (i = 0; i < numIndices; i++){
		if (indRemap[i] != -1)
			continue;

		for (j = 0; j < numIndices; j++){
			if (j == i)
				continue;

			if ((tempIndex[j] == tempIndex[i]) && (tempStIndex[j] == tempStIndex[i]))
				indRemap[j] = i;
		}
	}

	// Count unique vertices
	for (i = 0; i < numIndices; i++){
		if (indRemap[i] != -1)
			continue;

		outIndices[i] = numVertices++;
		indRemap[i] = i;
	}

	outSurface->numVertices = numVertices;

	// Remap remaining indices
	for (i = 0; i < numIndices; i++){
		if (indRemap[i] != i)
			outIndices[i] = outIndices[indRemap[i]];
	}

	// Load base S and T vertices
	inSt = (md2St_t *)((byte *)inModel + LittleLong(inModel->ofsSt));
	outSurface->st = outSt = Hunk_Alloc(outSurface->numVertices * sizeof(mdlSt_t));

	model->size += outSurface->numVertices * sizeof(mdlSt_t);

	for (i = 0; i < numIndices; i++){
		outSt[outIndices[i]].st[0] = (float)(((double)LittleShort(inSt[tempStIndex[indRemap[i]]].s) + 0.5) * skinWidth);
		outSt[outIndices[i]].st[1] = (float)(((double)LittleShort(inSt[tempStIndex[indRemap[i]]].t) + 0.5) * skinHeight);
	}

	// Load the frames
	outModel->frames = outFrame = Hunk_Alloc(outModel->numFrames * sizeof(mdlFrame_t));
	outSurface->xyzNormals = outXyzNormal = Hunk_Alloc(outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t));

	model->size += outModel->numFrames * sizeof(mdlFrame_t);
	model->size += outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t);

	for (i = 0; i < outModel->numFrames; i++, outFrame++, outXyzNormal += numVertices){
		inFrame = (md2Frame_t *)((byte *)inModel + LittleLong(inModel->ofsFrames) + i*LittleLong(inModel->frameSize));

		outFrame->scale[0] = LittleFloat(inFrame->scale[0]);
		outFrame->scale[1] = LittleFloat(inFrame->scale[1]);
		outFrame->scale[2] = LittleFloat(inFrame->scale[2]);

		outFrame->translate[0] = LittleFloat(inFrame->translate[0]);
		outFrame->translate[1] = LittleFloat(inFrame->translate[1]);
		outFrame->translate[2] = LittleFloat(inFrame->translate[2]);

		VectorCopy(outFrame->translate, outFrame->mins);
		VectorMA(outFrame->translate, 255, outFrame->scale, outFrame->maxs);

		outFrame->radius = RadiusFromBounds(outFrame->mins, outFrame->maxs);

		// Load vertices and normals
		for (j = 0; j < numIndices; j++){
			outXyzNormal[outIndices[j]].xyz[0] = (short)inFrame->verts[tempIndex[indRemap[j]]].v[0];
			outXyzNormal[outIndices[j]].xyz[1] = (short)inFrame->verts[tempIndex[indRemap[j]]].v[1];
			outXyzNormal[outIndices[j]].xyz[2] = (short)inFrame->verts[tempIndex[indRemap[j]]].v[2];

			ByteToDir(inFrame->verts[tempIndex[indRemap[j]]].lightNormalIndex, normal);
			NormalToLatLong(normal, outXyzNormal[outIndices[j]].normal);
		}

		// Calculate tangent space vectors
		R_CalcTangentSpace(outSurface->numTriangles, outSurface->triangles, outSurface->numVertices, outSurface->xyzNormals, outSurface->st, i);
	}

	// Build triangle neighbors
	outSurface->neighbors = Hunk_Alloc(outSurface->numTriangles * sizeof(mdlNeighbor_t));
	model->size += outSurface->numTriangles * sizeof(mdlNeighbor_t);

	R_BuildTriangleNeighbors(outSurface->numTriangles, outSurface->triangles, outSurface->neighbors);

	// Register all skins
	outSurface->shaders = outShader = Hunk_Alloc(outSurface->numShaders * sizeof(mdlShader_t));

	model->size += outSurface->numShaders * sizeof(mdlShader_t);

	for (i = 0; i < outSurface->numShaders; i++, outShader++){
		Com_StripExtension((char *)inModel + LittleLong(inModel->ofsSkins) + i*MAX_QPATH, name, sizeof(name));
		outShader->shader = R_FindShader(name, SHADER_SKIN, 0);

		// If this is an ammo display, set up the necessary stuff
		if (outShader->shader->flags & SHADER_AMMODISPLAY){
			outShader->ammoDisplay = Hunk_Alloc(sizeof(mdlAmmoDisplay_t));
			model->size += sizeof(mdlAmmoDisplay_t);

			R_SetupAmmoDisplay(outShader->shader, outShader->ammoDisplay);
		}

		// HACK: allow these models to be translucent
		if (!(outShader->shader->flags & SHADER_EXTERNAL)){
			if (!Q_stricmp(model->name, "models/objects/black/tris") || 
				!Q_stricmp(model->name, "models/items/shell/tris") || 
				!Q_stricmp(model->name, "models/items/spawngro/tris") || 
				!Q_stricmp(model->name, "models/items/spawngro2/tris") || 
				!Q_stricmp(model->name, "models/items/spawngro3/tris")){

				outShader->shader->sort = SORT_BLEND;
				outShader->shader->stages[0]->flags |= SHADERSTAGE_BLENDFUNC;
				outShader->shader->stages[0]->flags &= ~SHADERSTAGE_DEPTHWRITE;
				outShader->shader->stages[0]->blendFunc.src = GL_SRC_ALPHA;
				outShader->shader->stages[0]->blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;
				outShader->shader->stages[0]->alphaGen.type = ALPHAGEN_ENTITY;
			}
		}
	}

	// Calculate model bounds and radius
	ClearBounds(model->mins, model->maxs);

	for (i = 0; i < outModel->numFrames; i++){
		AddPointToBounds(outModel->frames[i].mins, model->mins, model->maxs);
		AddPointToBounds(outModel->frames[i].maxs, model->mins, model->maxs);
	}

	model->radius = RadiusFromBounds(model->mins, model->maxs);
}

/*
 =================
 R_LoadMD3
 =================
*/
static void R_LoadMD3 (model_t *model, const byte *data){

	md3Header_t		*inModel;
	md3Frame_t		*inFrame;
	md3Tag_t		*inTag;
	md3Surface_t	*inSurface;
	md3Shader_t		*inShader;
	md3Triangle_t	*inTriangle;
	md3St_t			*inSt;
	md3XyzNormal_t	*inXyzNormal;
	mdl_t			*outModel;
	mdlFrame_t		*outFrame;
	mdlTag_t		*outTag;
	mdlSurface_t	*outSurface;
	mdlShader_t		*outShader;
	mdlTriangle_t	*outTriangle;
	mdlSt_t			*outSt;
	mdlXyzNormal_t	*outXyzNormal;
	int				i, j, k;
	int				ident, version;
	char			name[MAX_QPATH];

	inModel = (md3Header_t *)data;
	model->alias[model->numAlias++] = outModel = Hunk_Alloc(sizeof(mdl_t));

	model->size += sizeof(mdl_t);

	// Byte swap the header fields and sanity check
	ident = LittleLong(inModel->ident);
	if (ident != MD3_IDENT)
		Com_Error(ERR_DROP, "R_LoadMD3: '%s' has wrong file id", model->realName);

	version = LittleLong(inModel->version);
	if (version != MD3_VERSION)
		Com_Error(ERR_DROP, "R_LoadMD3: '%s' has wrong version number (%i should be %i)", model->realName, version, MD3_VERSION);

	outModel->numFrames = LittleLong(inModel->numFrames);
	if (outModel->numFrames > MD3_MAX_FRAMES || outModel->numFrames <= 0)
		Com_Error(ERR_DROP, "R_LoadMD3: '%s' has invalid number of frames (%i)", model->realName, outModel->numFrames);

	outModel->numTags = LittleLong(inModel->numTags);
	if (outModel->numTags > MD3_MAX_TAGS || outModel->numTags < 0)
		Com_Error(ERR_DROP, "R_LoadMD3: '%s' has invalid number of tags (%i)", model->realName, outModel->numTags);

	outModel->numSurfaces = LittleLong(inModel->numSurfaces);
	if (outModel->numSurfaces > MD3_MAX_SURFACES || outModel->numSurfaces <= 0)
		Com_Error(ERR_DROP, "R_LoadMD3: '%s' has invalid number of surfaces (%i)", model->realName, outModel->numSurfaces);

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

		outFrame->scale[0] = MD3_XYZ_SCALE;
		outFrame->scale[1] = MD3_XYZ_SCALE;
		outFrame->scale[2] = MD3_XYZ_SCALE;

		outFrame->translate[0] = LittleFloat(inFrame->localOrigin[0]);
		outFrame->translate[1] = LittleFloat(inFrame->localOrigin[1]);
		outFrame->translate[2] = LittleFloat(inFrame->localOrigin[2]);

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
		if (ident != MD3_IDENT)
			Com_Error(ERR_DROP, "R_LoadMD3: '%s' has wrong file id in surface '%s'", model->realName, inSurface->name);

		outSurface->numShaders = LittleLong(inSurface->numShaders);
		if (outSurface->numShaders > MD3_MAX_SHADERS || outSurface->numShaders <= 0)
			Com_Error(ERR_DROP, "R_LoadMD3: '%s' has invalid number of shaders in surface '%s' (%i)", model->realName, inSurface->name, outSurface->numShaders);

		outSurface->numTriangles = LittleLong(inSurface->numTriangles);
		if (outSurface->numTriangles > MD3_MAX_TRIANGLES || outSurface->numTriangles <= 0)
			Com_Error(ERR_DROP, "R_LoadMD3: '%s' has invalid number of triangles in surface '%s' (%i)", model->realName, inSurface->name, outSurface->numTriangles);

		outSurface->numVertices = LittleLong(inSurface->numVerts);
		if (outSurface->numVertices > MD3_MAX_VERTS || outSurface->numVertices <= 0)
			Com_Error(ERR_DROP, "R_LoadMD3: '%s' has invalid number of vertices in surface '%s' (%i)", model->realName, inSurface->name, outSurface->numVertices);

		// Register all shaders
		inShader = (md3Shader_t *)((byte *)inSurface + LittleLong(inSurface->ofsShaders));
		outSurface->shaders = outShader = Hunk_Alloc(outSurface->numShaders * sizeof(mdlShader_t));

		model->size += outSurface->numShaders * sizeof(mdlShader_t);

		for (j = 0; j < outSurface->numShaders; j++, inShader++, outShader++){
			Com_StripExtension(inShader->name, name, sizeof(name));
			outShader->shader = R_FindShader(name, SHADER_SKIN, 0);

			// If this is an ammo display, set up the necessary stuff
			if (outShader->shader->flags & SHADER_AMMODISPLAY){
				outShader->ammoDisplay = Hunk_Alloc(sizeof(mdlAmmoDisplay_t));
				model->size += sizeof(mdlAmmoDisplay_t);

				R_SetupAmmoDisplay(outShader->shader, outShader->ammoDisplay);
			}
		}

		// Load triangle lists
		inTriangle = (md3Triangle_t *)((byte *)inSurface + LittleLong(inSurface->ofsTriangles));
		outSurface->triangles = outTriangle = Hunk_Alloc(outSurface->numTriangles * sizeof(mdlTriangle_t));

		model->size += outSurface->numTriangles * sizeof(mdlTriangle_t);

		for (j = 0; j < outSurface->numTriangles; j++, inTriangle++, outTriangle++){
			outTriangle->index[0] = (unsigned)LittleLong(inTriangle->indexes[0]);
			outTriangle->index[1] = (unsigned)LittleLong(inTriangle->indexes[1]);
			outTriangle->index[2] = (unsigned)LittleLong(inTriangle->indexes[2]);
		}

		// Load base S and T vertices
		inSt = (md3St_t *)((byte *)inSurface + LittleLong(inSurface->ofsSt));
		outSurface->st = outSt = Hunk_Alloc(outSurface->numVertices * sizeof(mdlSt_t));

		model->size += outSurface->numVertices * sizeof(mdlSt_t);

		for (j = 0; j < outSurface->numVertices; j++, inSt++, outSt++){
			outSt->st[0] = LittleFloat(inSt->st[0]);
			outSt->st[1] = LittleFloat(inSt->st[1]);
		}

		// Load vertices and normals
		inXyzNormal = (md3XyzNormal_t *)((byte *)inSurface + LittleLong(inSurface->ofsXyzNormals));
		outSurface->xyzNormals = outXyzNormal = Hunk_Alloc(outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t));

		model->size += outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t);

		for (j = 0; j < outModel->numFrames; j++){
			for (k = 0; k < outSurface->numVertices; k++, inXyzNormal++, outXyzNormal++){
				outXyzNormal->xyz[0] = LittleShort(inXyzNormal->xyz[0]);
				outXyzNormal->xyz[1] = LittleShort(inXyzNormal->xyz[1]);
				outXyzNormal->xyz[2] = LittleShort(inXyzNormal->xyz[2]);

				outXyzNormal->normal[0] = (LittleShort(inXyzNormal->normal) >> 0) & 0xff;
				outXyzNormal->normal[1] = (LittleShort(inXyzNormal->normal) >> 8) & 0xff;
			}

			// Calculate tangent space vectors
			R_CalcTangentSpace(outSurface->numTriangles, outSurface->triangles, outSurface->numVertices, outSurface->xyzNormals, outSurface->st, j);
		}

		// Build triangle neighbors
		outSurface->neighbors = Hunk_Alloc(outSurface->numTriangles * sizeof(mdlNeighbor_t));
		model->size += outSurface->numTriangles * sizeof(mdlNeighbor_t);

		R_BuildTriangleNeighbors(outSurface->numTriangles, outSurface->triangles, outSurface->neighbors);

		// Skip to next surface
		inSurface = (md3Surface_t *)((byte *)inSurface + LittleLong(inSurface->ofsEnd));
	}

	// Calculate model bounds and radius
	ClearBounds(model->mins, model->maxs);

	for (i = 0; i < outModel->numFrames; i++){
		AddPointToBounds(outModel->frames[i].mins, model->mins, model->maxs);
		AddPointToBounds(outModel->frames[i].maxs, model->mins, model->maxs);
	}

	model->radius = RadiusFromBounds(model->mins, model->maxs);
}


/*
 =======================================================================

 SPRITE MODELS

 =======================================================================
*/


/*
 =================
 R_LoadSP2
 =================
*/
static void R_LoadSP2 (model_t *model, const byte *data){

	sp2Header_t	*inModel;
	sp2Frame_t	*inFrame;
	spr_t		*outModel;
	sprFrame_t	*outFrame;
	int			i, width, height;
	int			ident, version;
	char		name[MAX_QPATH];

	inModel = (sp2Header_t *)data;
	model->sprite = outModel = Hunk_Alloc(sizeof(spr_t));

	model->size += sizeof(spr_t);

	// Byte swap the header fields and sanity check
	ident = LittleLong(inModel->ident);
	if (ident != SP2_IDENT)
		Com_Error(ERR_DROP, "R_LoadSP2: '%s' has wrong file id", model->realName);

	version = LittleLong(inModel->version);
	if (version != SP2_VERSION)
		Com_Error(ERR_DROP, "R_LoadSP2: '%s' has wrong version number (%i should be %i)", model->realName, version, SP2_VERSION);

	outModel->numFrames = LittleLong(inModel->numFrames);
	if (outModel->numFrames > SP2_MAX_FRAMES || outModel->numFrames <= 0)
		Com_Error(ERR_DROP, "R_LoadSP2: '%s' has invalid number of frames (%i)", model->realName, outModel->numFrames);

	// Load the frames
	inFrame = inModel->frames;
	outModel->frames = outFrame = Hunk_Alloc(outModel->numFrames * sizeof(sprFrame_t));

	model->size += outModel->numFrames * sizeof(sprFrame_t);

	for (i = 0; i < outModel->numFrames; i++, inFrame++, outFrame++){
		// Byte swap everything
		width = LittleLong(inFrame->width);
		height = LittleLong(inFrame->height);

		// Register skin
		Com_StripExtension(inFrame->name, name, sizeof(name));
		outFrame->shader = R_FindShader(name, SHADER_GENERIC, 0);

		// Calculate radius
		outFrame->radius = sqrt(width * width + height * height);
	}
}


// =====================================================================


/*
 =================
 R_LoadModel
 =================
*/
model_t *R_LoadModel (const char *name, byte *data, modelType_t modelType){

	model_t		*model;
	unsigned	hashKey;
	int			i;
	char		lodName[MAX_QPATH];

	if (r_numModels == MAX_MODELS)
		Com_Error(ERR_DROP, "R_LoadModel: MAX_MODELS hit");

	r_models[r_numModels++] = model = Hunk_Alloc(sizeof(model_t));

	// Fill it in
	Com_StripExtension(name, model->name, sizeof(model->name));
	Q_strncpyz(model->realName, name, sizeof(model->realName));
	model->modelType = modelType;

	// Call the appropriate loader
	switch (modelType){
	case MODEL_MD3:
		R_LoadMD3(model, data);
		FS_FreeFile(data);

		// Try to load LOD models
		for (i = 1; i < MD3_MAX_LODS; i++){
			Q_snprintfz(lodName, sizeof(lodName), "%s_%i.md3", model->name, i);
			FS_LoadFile(lodName, (void **)&data);
			if (!data)
				break;

			R_LoadMD3(model, data);
			FS_FreeFile(data);
		}

		break;
	case MODEL_MD2:
		R_LoadMD2(model, data);
		FS_FreeFile(data);

		break;
	case MODEL_SP2:
		R_LoadSP2(model, data);
		FS_FreeFile(data);

		break;
	case MODEL_BAD:

		break;
	}

	// Add to hash table
	hashKey = Com_HashKey(model->name, MODELS_HASHSIZE);

	model->nextHash = r_modelsHash[hashKey];
	r_modelsHash[hashKey] = model;

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
	char		checkName[MAX_QPATH], loadName[MAX_QPATH];
	unsigned	hashKey;
	int			i;
	
	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindModel: NULL model name");

	if (strlen(name) >= MAX_QPATH)
		Com_Error(ERR_DROP, "R_FindModel: model name exceeds MAX_QPATH");

	// Inline models are grabbed only from worldmodel
	if (name[0] == '*'){
		i = atoi(name+1);
		if (!r_worldModel || (i < 1 || i >= r_worldModel->numSubmodels))
			Com_Error(ERR_DROP, "R_FindModel: bad inline model number (%i)", i);

		return &r_inlineModels[i];
	}

	// Strip extension
	Com_StripExtension(name, checkName, sizeof(checkName));

	// See if already loaded
	hashKey = Com_HashKey(checkName, MODELS_HASHSIZE);

	for (model = r_modelsHash[hashKey]; model; model = model->nextHash){
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

	Q_snprintfz(loadName, sizeof(loadName), "%s.sp2", checkName);
	FS_LoadFile(loadName, (void **)&data);
	if (data)
		return R_LoadModel(loadName, data, MODEL_SP2);

	// Not found
	Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find model '%s', using default...\n", name);
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
 R_ModelBounds
 =================
*/
void R_ModelBounds (model_t *model, vec3_t mins, vec3_t maxs){

	VectorCopy(model->mins, mins);
	VectorCopy(model->maxs, maxs);
}

/*
 =================
 R_ModelList_f
 =================
*/
void R_ModelList_f (void){

	model_t	*model;
	int		i, bytes = 0;

	Com_Printf("\n");
	Com_Printf("-----------------------------------\n");

	for (i = 0; i < r_numModels; i++){
		model = r_models[i];

		bytes += model->size;

		Com_Printf("%8i : ", model->size);

		if (model->modelType == MODEL_MD3 || model->modelType == MODEL_MD2)
			Com_Printf("(%i) ", model->numAlias);
		else
			Com_Printf("(1) ");

		Com_Printf("%s%s\n", model->realName, (model->modelType == MODEL_BAD) ? " (DEFAULTED)" : "");
	}

	Com_Printf("-----------------------------------\n");
	Com_Printf("%i bytes total resident\n", bytes);
	Com_Printf("%i total models\n", r_numModels);
	Com_Printf("\n");
}

/*
 =================
 R_InitModels
 =================
*/
void R_InitModels (void){

	memset(r_noMapVis, 255, sizeof(r_noMapVis));

	r_worldModel = NULL;
	r_worldEntity = &r_entities[0];			// First entity is the world

	memset(r_worldEntity, 0, sizeof(entity_t));
	r_worldEntity->entityType = ET_MODEL;
	r_worldEntity->model = r_worldModel;
	AxisClear(r_worldEntity->axis);
	MakeRGBA(r_worldEntity->shaderRGBA, 255, 255, 255, 255);

	r_frameCount = 1;						// No dlight cache
	r_viewCluster = r_oldViewCluster = -1;	// Force markleafs
}

/*
 =================
 R_ShutdownModels
 =================
*/
void R_ShutdownModels (void){

	r_worldModel = NULL;
	r_worldEntity = NULL;

	memset(r_modelsHash, 0, sizeof(r_modelsHash));
	memset(r_models, 0, sizeof(r_models));

	r_numModels = 0;
}
