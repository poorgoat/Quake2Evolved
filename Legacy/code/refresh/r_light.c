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
 =======================================================================

 DYNAMIC LIGHTS

 =======================================================================
*/


/*
 =================
 R_RecursiveLightNode
 =================
*/
static void R_RecursiveLightNode (node_t *node, dlight_t *dl, int bit){

	surface_t	*surf;
	cplane_t	*plane;
	float		dist;
	int			i;

	if (node->contents != -1)
		return;

	if (node->visFrame != r_visFrameCount)
		return;

	// Find which side of the node we are on
	plane = node->plane;
	if (plane->type < 3)
		dist = dl->origin[plane->type] - plane->dist;
	else
		dist = DotProduct(dl->origin, plane->normal) - plane->dist;

	// Go down the appropriate sides
	if (dist > dl->intensity){
		R_RecursiveLightNode(node->children[0], dl, bit);
		return;
	}
	if (dist < -dl->intensity){
		R_RecursiveLightNode(node->children[1], dl, bit);
		return;
	}

	// Mark the surfaces
	surf = r_worldModel->surfaces + node->firstSurface;
	for (i = 0; i < node->numSurfaces; i++, surf++){
		if (!BoundsAndSphereIntersect(surf->mins, surf->maxs, dl->origin, dl->intensity))
			continue;		// No intersection

		if (surf->dlightFrame != r_frameCount){
			surf->dlightFrame = r_frameCount;
			surf->dlightBits = bit;
		}
		else
			surf->dlightBits |= bit;
	}

	// Recurse down the children
	R_RecursiveLightNode(node->children[0], dl, bit);
	R_RecursiveLightNode(node->children[1], dl, bit);
}

/*
 =================
 R_MarkLights
 =================
*/
void R_MarkLights (void){

	dlight_t	*dl;
	int			l;

	if (!r_dynamicLights->integer || !r_numDLights)
		return;

	r_stats.numDLights += r_numDLights;

	for (l = 0, dl = r_dlights; l < r_numDLights; l++, dl++){
		if (R_CullSphere(dl->origin, dl->intensity, 15))
			continue;

		R_RecursiveLightNode(r_worldModel->nodes, dl, 1<<l);
	}
}


/*
 =======================================================================

 AMBIENT & DIFFUSE LIGHTING

 =======================================================================
*/

static vec3_t	r_pointColor;
static vec3_t	r_lightColors[MAX_VERTICES];


/*
 =================
 R_RecursiveLightPoint
 =================
*/
static qboolean R_RecursiveLightPoint (node_t *node, const vec3_t start, const vec3_t end){

	float		front, back, frac;
	vec3_t		mid;
	int			side;
	cplane_t	*plane;
	surface_t	*surf;
	texInfo_t	*tex;
	int			i, map, size, s, t;
	byte		*lm;
	vec3_t		scale;

	if (node->contents != -1)
		return false;	// Didn't hit anything

	// Calculate mid point
	plane = node->plane;
	if (plane->type < 3){
		front = start[plane->type] - plane->dist;
		back = end[plane->type] - plane->dist;
	}
	else {
		front = DotProduct(start, plane->normal) - plane->dist;
		back = DotProduct(end, plane->normal) - plane->dist;
	}

	side = front < 0;
	if ((back < 0) == side)
		return R_RecursiveLightPoint(node->children[side], start, end);

	frac = front / (front - back);

	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;

	// Go down front side	
	if (R_RecursiveLightPoint(node->children[side], start, mid))
		return true;	// Hit something

	if ((back < 0) == side)
		return false;	// Didn't hit anything

	// Check for impact on this node
	surf = r_worldModel->surfaces + node->firstSurface;
	for (i = 0; i < node->numSurfaces; i++, surf++){
		tex = surf->texInfo;

		if (tex->flags & (SURF_SKY | SURF_WARP | SURF_NODRAW))
			continue;	// No lightmaps

		s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3] - surf->textureMins[0];
		t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3] - surf->textureMins[1];

		if ((s < 0 || s > surf->extents[0]) || (t < 0 || t > surf->extents[1]))
			continue;

		s >>= 4;
		t >>= 4;

		if (!surf->lmSamples)
			return true;

		VectorClear(r_pointColor);

		lm = surf->lmSamples + 3 * (t * surf->lmWidth + s);
		size = surf->lmWidth * surf->lmHeight * 3;

		for (map = 0; map < surf->numStyles; map++){
			VectorScale(r_lightStyles[surf->styles[map]].rgb, r_modulate->value * (1.0/255), scale);

			r_pointColor[0] += lm[0] * scale[0];
			r_pointColor[1] += lm[1] * scale[1];
			r_pointColor[2] += lm[2] * scale[2];

			lm += size;		// Skip to next lightmap
		}

		return true;
	}

	// Go down back side
	return R_RecursiveLightPoint(node->children[!side], mid, end);
}

/*
 =================
 R_LightForPoint
 =================
*/
void R_LightForPoint (const vec3_t point, vec3_t ambientLight){

	dlight_t	*dl;
	vec3_t		end, dir;
	float		dist, add;
	int			l;

	// Set to full bright if no light data
	if (!r_worldModel || !r_worldModel->lightData){
		VectorSet(ambientLight, 1, 1, 1);
		return;
	}

	// Get lighting at this point
	VectorSet(end, point[0], point[1], point[2] - 8192);
	VectorSet(r_pointColor, 1, 1, 1);

	R_RecursiveLightPoint(r_worldModel->nodes, point, end);

	VectorCopy(r_pointColor, ambientLight);

	// Add dynamic lights
	if (r_dynamicLights->integer){
		for (l = 0, dl = r_dlights; l < r_numDLights; l++, dl++){
			VectorSubtract(dl->origin, point, dir);
			dist = VectorLength(dir);
			if (!dist || dist > dl->intensity)
				continue;

			add = (dl->intensity - dist) * (1.0/255);
			VectorMA(ambientLight, add, dl->color, ambientLight);
		}
	}
}

/*
 =================
 R_ReadLightGrid
 =================
*/
static void R_ReadLightGrid (const vec3_t origin, vec3_t lightDir){

	vec3_t	vf1, vf2;
	float	scale[8];
	int		vi[3], index[4];
	int		i;

	if (!r_worldModel->lightGrid){
		VectorSet(lightDir, 1, 0, -1);
		return;
	}

	for (i = 0; i < 3; i++){
		vf1[i] = (origin[i] - r_worldModel->gridMins[i]) / r_worldModel->gridSize[i];
		vi[i] = (int)vf1[i];
		vf1[i] = vf1[i] - floor(vf1[i]);
		vf2[i] = 1.0 - vf1[i];
	}

	index[0] = vi[2] * r_worldModel->gridBounds[3] + vi[1] * r_worldModel->gridBounds[0] + vi[0];
	index[1] = index[0] + r_worldModel->gridBounds[0];
	index[2] = index[0] + r_worldModel->gridBounds[3];
	index[3] = index[2] + r_worldModel->gridBounds[0];

	for (i = 0; i < 4; i++){
		if (index[i] < 0 || index[i] >= r_worldModel->gridPoints -1){
			VectorSet(lightDir, 1, 0, -1);
			return;
		}
	}

	scale[0] = vf2[0] * vf2[1] * vf2[2];
	scale[1] = vf1[0] * vf2[1] * vf2[2];
	scale[2] = vf2[0] * vf1[1] * vf2[2];
	scale[3] = vf1[0] * vf1[1] * vf2[2];
	scale[4] = vf2[0] * vf2[1] * vf1[2];
	scale[5] = vf1[0] * vf2[1] * vf1[2];
	scale[6] = vf2[0] * vf1[1] * vf1[2];
	scale[7] = vf1[0] * vf1[1] * vf1[2];

	VectorClear(lightDir);

	for (i = 0; i < 4; i++){
		VectorMA(lightDir, scale[i*2+0], r_worldModel->lightGrid[index[i]+0].lightDir, lightDir);
		VectorMA(lightDir, scale[i*2+1], r_worldModel->lightGrid[index[i]+1].lightDir, lightDir);
	}
}

/*
 =================
 R_LightDir
 =================
*/
void R_LightDir (const vec3_t origin, vec3_t lightDir){

	dlight_t	*dl;
	vec3_t		dir;
	float		dist;
	int			l;

	// Get light direction from light grid
	R_ReadLightGrid(origin, lightDir);

	// Add dynamic lights
	if (r_dynamicLights->integer){
		for (l = 0, dl = r_dlights; l < r_numDLights; l++, dl++){
			VectorSubtract(dl->origin, origin, dir);
			dist = VectorLength(dir);
			if (!dist || dist > dl->intensity)
				continue;

			VectorAdd(lightDir, dir, lightDir);
		}
	}
}

/*
 =================
 R_LightingAmbient
 =================
*/
void R_LightingAmbient (void){

	dlight_t	*dl;
	vec3_t		end, dir;
	float		add, dist, radius;
	int			i, l;
	vec3_t		ambientLight;
	color_t		color;

	// Set to full bright if no light data
	if ((r_refDef.rdFlags & RDF_NOWORLDMODEL) || !r_worldModel->lightData){
		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = 255;
			colorArray[i][1] = 255;
			colorArray[i][2] = 255;
			colorArray[i][3] = 255;
		}

		return;
	}

	// Get lighting at this point
	VectorSet(end, rb_entity->origin[0], rb_entity->origin[1], rb_entity->origin[2] - 8192);
	VectorSet(r_pointColor, 1, 1, 1);

	R_RecursiveLightPoint(r_worldModel->nodes, rb_entity->origin, end);

	VectorScale(r_pointColor, r_ambientScale->value, ambientLight);

	// Always have some light
	if (rb_entity->renderFX & RF_MINLIGHT){
		for (i = 0; i < 3; i++){
			if (ambientLight[i] > 0.1)
				break;
		}

		if (i == 3)
			VectorSet(ambientLight, 0.1, 0.1, 0.1);
	}

	// Add dynamic lights
	if (r_dynamicLights->integer){
		if (rb_entity->entityType == ET_MODEL)
			radius = rb_entity->model->radius;
		else
			radius = rb_entity->radius;

		for (l = 0, dl = r_dlights; l < r_numDLights; l++, dl++){
			VectorSubtract(dl->origin, rb_entity->origin, dir);
			dist = VectorLength(dir);
			if (!dist || dist > dl->intensity + radius)
				continue;

			add = (dl->intensity - dist) * (1.0/255);
			VectorMA(ambientLight, add, dl->color, ambientLight);
		}
	}

	// Normalize and convert to byte
	*(unsigned *)color = ColorNormalize(ambientLight);

	for (i = 0; i < numVertex; i++)
		*(unsigned *)colorArray[i] = *(unsigned *)color;
}

/*
 =================
 R_LightingDiffuse
 =================
*/
void R_LightingDiffuse (void){

	dlight_t	*dl;
	vec3_t		end, dir;
	float		add, dot, dist, intensity, radius;
	int			i, l;
	vec3_t		ambientLight, directedLight, lightDir;

	// Set to full bright if no light data
	if ((r_refDef.rdFlags & RDF_NOWORLDMODEL) || !r_worldModel->lightData){
		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = 255;
			colorArray[i][1] = 255;
			colorArray[i][2] = 255;
			colorArray[i][3] = 255;
		}

		return;
	}

	// Get lighting at this point
	VectorSet(end, rb_entity->origin[0], rb_entity->origin[1], rb_entity->origin[2] - 8192);
	VectorSet(r_pointColor, 1, 1, 1);

	R_RecursiveLightPoint(r_worldModel->nodes, rb_entity->origin, end);

	VectorScale(r_pointColor, r_ambientScale->value, ambientLight);
	VectorScale(r_pointColor, r_directedScale->value, directedLight);

	R_ReadLightGrid(rb_entity->origin, lightDir);

	// Always have some light
	if (rb_entity->renderFX & RF_MINLIGHT){
		for (i = 0; i < 3; i++){
			if (ambientLight[i] > 0.1)
				break;
		}

		if (i == 3)
			VectorSet(ambientLight, 0.1, 0.1, 0.1);
	}

	// Compute lighting at each vertex
	VectorRotate(lightDir, rb_entity->axis, dir);
	VectorNormalizeFast(dir);

	for (i = 0; i < numVertex; i++){
		dot = DotProduct(normalArray[i], dir);
		if (dot <= 0){
			VectorCopy(ambientLight, r_lightColors[i]);
			continue;
		}

		VectorMA(ambientLight, dot, directedLight, r_lightColors[i]);
	}

	// Add dynamic lights
	if (r_dynamicLights->integer){
		if (rb_entity->entityType == ET_MODEL)
			radius = rb_entity->model->radius;
		else
			radius = rb_entity->radius;

		for (l = 0, dl = r_dlights; l < r_numDLights; l++, dl++){
			VectorSubtract(dl->origin, rb_entity->origin, dir);
			dist = VectorLength(dir);
			if (!dist || dist > dl->intensity + radius)
				continue;

			VectorRotate(dir, rb_entity->axis, lightDir);
			intensity = dl->intensity * 8;

			// Compute lighting at each vertex
			for (i = 0; i < numVertex; i++){
				VectorSubtract(lightDir, vertexArray[i], dir);
				add = DotProduct(normalArray[i], dir);
				if (add <= 0)
					continue;

				dot = DotProduct(dir, dir);
				add *= (intensity / dot) * Q_rsqrt(dot);
				VectorMA(r_lightColors[i], add, dl->color, r_lightColors[i]);
			}
		}
	}

	// Normalize and convert to byte
	for (i = 0; i < numVertex; i++)
		*(unsigned *)colorArray[i] = ColorNormalize(r_lightColors[i]);
}


/*
 =======================================================================

 LIGHT SAMPLING

 =======================================================================
*/

static vec3_t	r_blockLights[128*128];


/*
 =================
 R_SetCacheState
 =================
*/
static void R_SetCacheState (surface_t *surf){

	int		map;

	for (map = 0; map < surf->numStyles; map++)
		surf->cachedLight[map] = r_lightStyles[surf->styles[map]].white;
}

/*
 =================
 R_AddDynamicLights
 =================
*/
static void R_AddDynamicLights (surface_t *surf){

	int			l;
	int			s, t, sd, td;
	float		sl, tl, sacc, tacc;
	float		dist, rad, scale;
	cplane_t	*plane;
	vec3_t		origin, tmp, impact;
	texInfo_t	*tex = surf->texInfo;
	dlight_t	*dl;
	float		*bl;

	for (l = 0, dl = r_dlights; l < r_numDLights; l++, dl++){
		if (!(surf->dlightBits & (1<<l)))
			continue;		// Not lit by this light

		if (!AxisCompare(rb_entity->axis, axisDefault)){
			VectorSubtract(dl->origin, rb_entity->origin, tmp);
			VectorRotate(tmp, rb_entity->axis, origin);
		}
		else
			VectorSubtract(dl->origin, rb_entity->origin, origin);

		plane = surf->plane;
		if (plane->type < 3)
			dist = origin[plane->type] - plane->dist;
		else
			dist = DotProduct(origin, plane->normal) - plane->dist;

		// rad is now the highest intensity on the plane
		rad = dl->intensity - fabs(dist);
		if (rad < 0)
			continue;

		if (plane->type < 3){
			VectorCopy(origin, impact);
			impact[plane->type] -= dist;
		}
		else
			VectorMA(origin, -dist, plane->normal, impact);

		sl = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3] - surf->textureMins[0];
		tl = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3] - surf->textureMins[1];

		bl = r_blockLights;

		for (t = 0, tacc = 0; t < surf->lmHeight; t++, tacc += 16){
			td = tl - tacc;
			if (td < 0)
				td = -td;

			for (s = 0, sacc = 0; s < surf->lmWidth; s++, sacc += 16){
				sd = sl - sacc;
				if (sd < 0)
					sd = -sd;

				if (sd > td)
					dist = sd + (td >> 1);
				else
					dist = td + (sd >> 1);

				if (dist < rad){
					scale = rad - dist;

					bl[0] += dl->color[0] * scale;
					bl[1] += dl->color[1] * scale;
					bl[2] += dl->color[2] * scale;
				}

				bl += 3;
			}
		}
	}
}

/*
 =================
 R_BuildLightmap

 Combine and scale multiple lightmaps into the floating format in 
 r_blockLights
 =================
*/
static void R_BuildLightmap (surface_t *surf, byte *dest, int stride){

	int		i, map, size, s, t;
	byte	*lm;
	vec3_t	scale;
	float	*bl, max;

	lm = surf->lmSamples;
	size = surf->lmWidth * surf->lmHeight;

	if (!lm){
		// Set to full bright if no light data
		for (i = 0, bl = r_blockLights; i < size; i++, bl += 3){
			bl[0] = 255;
			bl[1] = 255;
			bl[2] = 255;
		}
	}
	else {
		// Add all the lightmaps
		VectorScale(r_lightStyles[surf->styles[0]].rgb, r_modulate->value, scale);

		for (i = 0, bl = r_blockLights; i < size; i++, bl += 3, lm += 3){
			bl[0] = lm[0] * scale[0];
			bl[1] = lm[1] * scale[1];
			bl[2] = lm[2] * scale[2];
		}

		if (surf->numStyles > 1){
			for (map = 1; map < surf->numStyles; map++){
				VectorScale(r_lightStyles[surf->styles[map]].rgb, r_modulate->value, scale);

				for (i = 0, bl = r_blockLights; i < size; i++, bl += 3, lm += 3){
					bl[0] += lm[0] * scale[0];
					bl[1] += lm[1] * scale[1];
					bl[2] += lm[2] * scale[2];
				}
			}
		}

		// Add all the dynamic lights
		if (surf->dlightFrame == r_frameCount)
			R_AddDynamicLights(surf);
	}

	// Put into texture format
	stride -= (surf->lmWidth << 2);
	bl = r_blockLights;

	for (t = 0; t < surf->lmHeight; t++){
		for (s = 0; s < surf->lmWidth; s++){
			// Catch negative lights
			if (bl[0] < 0)
				bl[0] = 0;
			if (bl[1] < 0)
				bl[1] = 0;
			if (bl[2] < 0)
				bl[2] = 0;

			// Determine the brightest of the three color components
			max = bl[0];
			if (max < bl[1])
				max = bl[1];
			if (max < bl[2])
				max = bl[2];

			// Rescale all the color components if the intensity of the
			// greatest channel exceeds 255
			if (max > 255.0){
				max = 255.0 / max;

				dest[0] = bl[0] * max;
				dest[1] = bl[1] * max;
				dest[2] = bl[2] * max;
				dest[3] = 255;
			}
			else {
				dest[0] = bl[0];
				dest[1] = bl[1];
				dest[2] = bl[2];
				dest[3] = 255;
			}

			bl += 3;
			dest += 4;
		}

		dest += stride;
	}
}


/*
 =======================================================================

 LIGHTMAP ALLOCATION

 =======================================================================
*/

typedef struct {
	int		currentNum;
	int		allocated[LIGHTMAP_WIDTH];
	byte	buffer[LIGHTMAP_WIDTH*LIGHTMAP_HEIGHT*4];
} lmState_t;

static lmState_t	r_lmState;

texture_t *R_LoadTexture (const char *name, byte *data, int width, int height, unsigned flags, float bumpScale);


/*
 =================
 R_UploadLightmap
 =================
*/
static void R_UploadLightmap (void){

	char	name[MAX_QPATH];

	if (r_lmState.currentNum == MAX_LIGHTMAPS)
		Com_Error(ERR_DROP, "R_UploadLightmap: MAX_LIGHTMAPS hit");

	Q_snprintfz(name, sizeof(name), "*lightmap%i", r_lmState.currentNum);
	r_lightmapTextures[r_lmState.currentNum++] = R_LoadTexture(name, r_lmState.buffer, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, TF_CLAMP, 0);

	// Reset
	memset(r_lmState.allocated, 0, sizeof(r_lmState.allocated));
	memset(r_lmState.buffer, 255, sizeof(r_lmState.buffer));
}

/*
 =================
 R_AllocLightmapBlock
 =================
*/
static byte *R_AllocLightmapBlock (int width, int height, int *s, int *t){

	int		i, j;
	int		best1, best2;

	best1 = LIGHTMAP_HEIGHT;

	for (i = 0; i < LIGHTMAP_WIDTH-width; i++){
		best2 = 0;

		for (j = 0; j < width; j++){
			if (r_lmState.allocated[i+j] >= best1)
				break;
			if (r_lmState.allocated[i+j] > best2)
				best2 = r_lmState.allocated[i+j];
		}
		if (j == width){
			// This is a valid spot
			*s = i;
			*t = best1 = best2;
		}
	}

	if (best1 + height > LIGHTMAP_HEIGHT)
		return NULL;

	for (i = 0; i < width; i++)
		r_lmState.allocated[*s + i] = best1 + height;

	return r_lmState.buffer + ((*t * LIGHTMAP_WIDTH + *s) * 4);
}

/*
 =================
 R_BeginBuildingLightmaps
 =================
*/
void R_BeginBuildingLightmaps (void){

	int		i;

	// Setup the base lightstyles so the lightmaps won't have to be 
	// regenerated the first time they're seen
	for (i = 0; i < MAX_LIGHTSTYLES; i++){
		r_lightStyles[i].white = 3;
		r_lightStyles[i].rgb[0] = 1;
		r_lightStyles[i].rgb[1] = 1;
		r_lightStyles[i].rgb[2] = 1;
	}

	r_lmState.currentNum = -1;

	memset(r_lmState.allocated, 0, sizeof(r_lmState.allocated));
	memset(r_lmState.buffer, 255, sizeof(r_lmState.buffer));
}

/*
 =================
 R_EndBuildingLightmaps
 =================
*/
void R_EndBuildingLightmaps (void){

	if (r_lmState.currentNum == -1)
		return;

	R_UploadLightmap();
}

/*
 =================
 R_BuildSurfaceLightmap
 =================
*/
void R_BuildSurfaceLightmap (surface_t *surf){

	byte	*base;

	if (!(surf->texInfo->shader->flags & SHADER_HASLIGHTMAP))
		return;		// No lightmaps

	base = R_AllocLightmapBlock(surf->lmWidth, surf->lmHeight, &surf->lmS, &surf->lmT);
	if (!base){
		if (r_lmState.currentNum != -1)
			R_UploadLightmap();

		base = R_AllocLightmapBlock(surf->lmWidth, surf->lmHeight, &surf->lmS, &surf->lmT);
		if (!base)
			Com_Error(ERR_DROP, "R_BuildSurfaceLightmap: couldn't allocate lightmap block (%i x %i)", surf->lmWidth, surf->lmHeight);
	}

	if (r_lmState.currentNum == -1)
		r_lmState.currentNum = 0;

	surf->lmNum = r_lmState.currentNum;

	R_SetCacheState(surf);
	R_BuildLightmap(surf, base, LIGHTMAP_WIDTH * 4);
}

/*
 =================
 R_UpdateSurfaceLightmap
 =================
*/
void R_UpdateSurfaceLightmap (surface_t *surf){

	if (surf->dlightFrame == r_frameCount)
		GL_BindTexture(r_dlightTexture);
	else {
		GL_BindTexture(r_lightmapTextures[surf->lmNum]);

		R_SetCacheState(surf);
	}

	R_BuildLightmap(surf, r_lmState.buffer, surf->lmWidth * 4);

	qglTexSubImage2D(GL_TEXTURE_2D, 0, surf->lmS, surf->lmT, surf->lmWidth, surf->lmHeight, GL_RGBA, GL_UNSIGNED_BYTE, r_lmState.buffer);
}
