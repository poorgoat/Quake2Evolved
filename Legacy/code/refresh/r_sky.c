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


// r_sky.c -- sky code


#include "r_local.h"


#define	MAX_CLIP_VERTS	64

#define ST_SCALE		1.0 / SKY_SIZE
#define ST_MIN			1.0 / 512
#define ST_MAX			511.0 / 512

#define SPHERE_RAD		10.0
#define SPHERE_RAD2		SPHERE_RAD * SPHERE_RAD

#define EYE_RAD			9.0
#define EYE_RAD2		EYE_RAD * EYE_RAD

#define BOX_SIZE		1.0
#define BOX_STEP		BOX_SIZE / SKY_SIZE * 2.0

static vec3_t	r_skyClip[6] = {
	{  1,  1,  0},
	{  1, -1,  0},
	{  0, -1,  1},
	{  0,  1,  1},
	{  1,  0,  1},
	{ -1,  0,  1} 
};

static int		r_skyStToVec[6][3] = {
	{  3, -1,  2},
	{ -3,  1,  2},
	{  1,  3,  2},
	{ -1, -3,  2},
	{ -2, -1,  3},				// 0 degrees yaw, look straight up
	{  2, -1, -3}				// Look straight down
};

static int		r_skyVecToSt[6][3] = {
	{ -2,  3,  1},
	{  2,  3, -1},
	{  1,  3,  2},
	{ -1,  3, -2},
	{ -2, -1,  3},
	{ -2,  1, -3}
};


/*
 =================
 R_FillSkySide
 =================
*/
static void R_FillSkySide (skySide_t *skySide, float skyHeight, vec3_t org, vec3_t row, vec3_t col){

	vec3_t	xyz, normal, w;
	vec2_t	st, sphere;
	float	dist;
	int		r, c;
	int		vert = 0;

	skySide->numIndices = SKY_INDICES;
	skySide->numVertices = SKY_VERTICES;

	// Find normal
	CrossProduct(col, row, normal);
	VectorNormalize(normal);

	for (r = 0; r <= SKY_SIZE; r++){
		VectorCopy(org, xyz);

		for (c = 0; c <= SKY_SIZE; c++){
			// Find s and t for box
			st[0] = c * ST_SCALE;
			st[1] = 1.0 - r * ST_SCALE;

			if (!glConfig.textureEdgeClamp){
				// Avoid bilerp seam
				st[0] = Clamp(st[0], ST_MIN, ST_MAX);
				st[1] = Clamp(st[1], ST_MIN, ST_MAX);
			}

			// Normalize
			VectorNormalize2(xyz, w);

			// Find distance along w to sphere
			dist = sqrt(EYE_RAD2 * (w[2] * w[2] - 1.0) + SPHERE_RAD2) - EYE_RAD * w[2];

			// Use x and y on sphere as s and t
			// Minus is here so skies scroll in correct (Q3's) direction
			sphere[0] = ((-(w[0] * dist) * ST_SCALE) + 1) * 0.5;
			sphere[1] = ((-(w[1] * dist) * ST_SCALE) + 1) * 0.5;

			// Store vertex
			skySide->vertices[vert].xyz[0] = xyz[0] * skyHeight;
			skySide->vertices[vert].xyz[1] = xyz[1] * skyHeight;
			skySide->vertices[vert].xyz[2] = xyz[2] * skyHeight;
			skySide->vertices[vert].normal[0] = normal[0];
			skySide->vertices[vert].normal[1] = normal[1];
			skySide->vertices[vert].normal[2] = normal[2];
			skySide->vertices[vert].st[0] = st[0];
			skySide->vertices[vert].st[1] = st[1];
			skySide->vertices[vert].sphere[0] = sphere[0];
			skySide->vertices[vert].sphere[1] = sphere[1];

			vert++;

			VectorAdd(xyz, col, xyz);
		}

		VectorAdd(org, row, org);
	}
}

/*
 =================
 R_MakeSkyVec
 =================
*/
static void R_MakeSkyVec (int axis, float x, float y, float z, vec3_t v){

	vec3_t	b;
	int		i, j;

	VectorSet(b, x, y, z);

	for (i = 0; i < 3; i++){
		j = r_skyStToVec[axis][i];
		if (j < 0)
			v[i] = -b[-j - 1];
		else
			v[i] = b[j - 1];
	}
}

/*
 =================
 R_AddSkyPolygon
 =================
*/
static void R_AddSkyPolygon (int numVerts, vec3_t verts){

	sky_t	*sky = r_worldModel->sky;
	int		i, j, axis;
	vec3_t	v, av;
	float	s, t, div;
	float	*vp;

	// Decide which face it maps to
	VectorClear(v);
	for (i = 0, vp = verts; i < numVerts; i++, vp += 3)
		VectorAdd(vp, v, v);

	VectorSet(av, fabs(v[0]), fabs(v[1]), fabs(v[2]));

	if (av[0] > av[1] && av[0] > av[2]){
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	}
	else if (av[1] > av[2] && av[1] > av[0]){
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	}
	else {
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// Project new texture coords
	for (i = 0; i < numVerts; i++, verts += 3){
		j = r_skyVecToSt[axis][2];
		if (j > 0)
			div = verts[j - 1];
		else
			div = -verts[-j - 1];
		
		if (div < 0.001)
			continue;	// Don't divide by zero

		div = 1.0 / div;

		j = r_skyVecToSt[axis][0];
		if (j < 0)
			s = -verts[-j - 1] * div;
		else
			s = verts[j - 1] * div;
		
		j = r_skyVecToSt[axis][1];
		if (j < 0)
			t = -verts[-j - 1] * div;
		else
			t = verts[j - 1] * div;

		if (s < sky->mins[0][axis])
			sky->mins[0][axis] = s;
		if (t < sky->mins[1][axis])
			sky->mins[1][axis] = t;
		if (s > sky->maxs[0][axis])
			sky->maxs[0][axis] = s;
		if (t > sky->maxs[1][axis])
			sky->maxs[1][axis] = t;
	}
}

/*
 =================
 R_ClipSkyPolygon
 =================
*/
static void R_ClipSkyPolygon (int numVerts, vec3_t verts, int stage){

	int			i;
	float		*v;
	qboolean	frontSide, backSide;
	vec3_t		front[MAX_CLIP_VERTS], back[MAX_CLIP_VERTS];
	int			f, b;
	float		dist;
	float		dists[MAX_CLIP_VERTS];
	int			sides[MAX_CLIP_VERTS];

	if (numVerts > MAX_CLIP_VERTS-2)
		Com_Error(ERR_DROP, "R_ClipSkyPolygon: MAX_CLIP_VERTS hit");

	if (stage == 6){
		// Fully clipped, so add it
		R_AddSkyPolygon(numVerts, verts);
		return;
	}

	frontSide = backSide = false;

	for (i = 0, v = verts; i < numVerts; i++, v += 3){
		dists[i] = dist = DotProduct(v, r_skyClip[stage]);

		if (dist > ON_EPSILON){
			frontSide = true;
			sides[i] = SIDE_FRONT;
		}
		else if (dist < -ON_EPSILON){
			backSide = true;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
	}

	if (!frontSide || !backSide){
		// Not clipped
		R_ClipSkyPolygon(numVerts, verts, stage+1);
		return;
	}

	// Clip it
	dists[i] = dists[0];
	sides[i] = sides[0];
	VectorCopy(verts, (verts + (i*3)));

	f = b = 0;

	for (i = 0, v = verts; i < numVerts; i++, v += 3){
		switch (sides[i]){
		case SIDE_FRONT:
			VectorCopy(v, front[f]);
			f++;

			break;
		case SIDE_BACK:
			VectorCopy(v, back[b]);
			b++;

			break;
		case SIDE_ON:
			VectorCopy(v, front[f]);
			VectorCopy(v, back[b]);
			f++;
			b++;

			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		dist = dists[i] / (dists[i] - dists[i+1]);

		front[f][0] = back[b][0] = v[0] + (v[3] - v[0]) * dist;
		front[f][1] = back[b][1] = v[1] + (v[4] - v[1]) * dist;
		front[f][2] = back[b][2] = v[2] + (v[5] - v[2]) * dist;

		f++;
		b++;
	}

	// Continue
	R_ClipSkyPolygon(f, front[0], stage+1);
	R_ClipSkyPolygon(b, back[0], stage+1);
}

/*
 =================
 R_DrawSkyBox
 =================
*/
static void R_DrawSkyBox (texture_t *textures[6], qboolean blended){

	sky_t		*sky = r_worldModel->sky;
	skySide_t	*skySide;
	int			i;

	// Set the state
	GL_TexEnv(GL_REPLACE);

	if (blended){
		GL_Enable(GL_CULL_FACE);
		GL_Disable(GL_POLYGON_OFFSET_FILL);
		GL_Disable(GL_VERTEX_PROGRAM_ARB);
		GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
		GL_Enable(GL_ALPHA_TEST);
		GL_Enable(GL_BLEND);
		GL_Enable(GL_DEPTH_TEST);

		GL_CullFace(GL_FRONT);
		GL_AlphaFunc(GL_GREATER, 0.0);
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GL_DepthFunc(GL_LEQUAL);
		GL_DepthMask(GL_FALSE);
	}
	else {
		GL_Enable(GL_CULL_FACE);
		GL_Disable(GL_POLYGON_OFFSET_FILL);
		GL_Disable(GL_VERTEX_PROGRAM_ARB);
		GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
		GL_Disable(GL_ALPHA_TEST);
		GL_Disable(GL_BLEND);
		GL_Enable(GL_DEPTH_TEST);

		GL_CullFace(GL_FRONT);
		GL_DepthFunc(GL_LEQUAL);
		GL_DepthMask(GL_FALSE);
	}

	qglDepthRange(1, 1);
	qglEnable(GL_TEXTURE_2D);

	qglColor4ub(255, 255, 255, 255);

	qglDisableClientState(GL_NORMAL_ARRAY);
	qglDisableClientState(GL_COLOR_ARRAY);

	// Draw it
	for (i = 0, skySide = sky->skySides; i < 6; i++, skySide++){
		if (sky->mins[0][i] >= sky->maxs[0][i] || sky->mins[1][i] >= sky->maxs[1][i])
			continue;

		r_stats.numVertices += skySide->numVertices;
		r_stats.numIndices += skySide->numIndices;
		r_stats.totalIndices += skySide->numIndices;

		GL_BindTexture(textures[i]);

		if (glConfig.vertexBufferObject){
			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, rb_vbo.indexBuffer);
			qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, skySide->numIndices * sizeof(unsigned), skySide->indices, GL_STREAM_DRAW_ARB);

			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, sky->vbo[i]);

			qglEnableClientState(GL_VERTEX_ARRAY);
			qglVertexPointer(3, GL_FLOAT, 20, VBO_OFFSET(0));

			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, 20, VBO_OFFSET(12));

			if (glConfig.drawRangeElements)
				qglDrawRangeElementsEXT(GL_TRIANGLES, 0, skySide->numVertices, skySide->numIndices, GL_UNSIGNED_INT, VBO_OFFSET(0));
			else
				qglDrawElements(GL_TRIANGLES, skySide->numIndices, GL_UNSIGNED_INT, VBO_OFFSET(0));
		}
		else {
			qglEnableClientState(GL_VERTEX_ARRAY);
			qglVertexPointer(3, GL_FLOAT, sizeof(skySideVert_t), skySide->vertices->xyz);

			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(2, GL_FLOAT, sizeof(skySideVert_t), skySide->vertices->st);

			if (glConfig.compiledVertexArray)
				qglLockArraysEXT(0, skySide->numVertices);

			if (glConfig.drawRangeElements)
				qglDrawRangeElementsEXT(GL_TRIANGLES, 0, skySide->numVertices, skySide->numIndices, GL_UNSIGNED_INT, skySide->indices);
			else
				qglDrawElements(GL_TRIANGLES, skySide->numIndices, GL_UNSIGNED_INT, skySide->indices);

			if (glConfig.compiledVertexArray)
				qglUnlockArraysEXT();
		}
	}

	qglDisable(GL_TEXTURE_2D);
}

/*
 =================
 R_DrawSky
 =================
*/
void R_DrawSky (void){

	sky_t			*sky = r_worldModel->sky;
	skySide_t		*skySide;
	skySideVert_t	*v;
	mat4_t			matrix;
	unsigned		*index;
	int				s, t, sMin, sMax, tMin, tMax;
    int				i, j;

	// Compute and load matrix
	matrix[ 0] = -r_refDef.viewAxis[1][0];
	matrix[ 1] = r_refDef.viewAxis[2][0];
	matrix[ 2] = -r_refDef.viewAxis[0][0];
	matrix[ 3] = 0.0;
	matrix[ 4] = -r_refDef.viewAxis[1][1];
	matrix[ 5] = r_refDef.viewAxis[2][1];
	matrix[ 6] = -r_refDef.viewAxis[0][1];
	matrix[ 7] = 0.0;
	matrix[ 8] = -r_refDef.viewAxis[1][2];
	matrix[ 9] = r_refDef.viewAxis[2][2];
	matrix[10] = -r_refDef.viewAxis[0][2];
	matrix[11] = 0.0;
	matrix[12] = 0.0;
	matrix[13] = 0.0;
	matrix[14] = 0.0;
	matrix[15] = 1.0;

	if (sky->rotate)
		Matrix4_Rotate(matrix, r_refDef.time * sky->rotate, sky->axis[0], sky->axis[1], sky->axis[2]);

	qglLoadMatrixf(matrix);

	// Build indices in tri-strip order
	for (i = 0, skySide = sky->skySides; i < 6; i++, skySide++){
		if (sky->mins[0][i] >= sky->maxs[0][i] || sky->mins[1][i] >= sky->maxs[1][i])
			continue;

		sMin = (int)((sky->mins[0][i] + 1) * 0.5 * (float)(SKY_SIZE));
		sMax = (int)((sky->maxs[0][i] + 1) * 0.5 * (float)(SKY_SIZE)) + 1;
		tMin = (int)((sky->mins[1][i] + 1) * 0.5 * (float)(SKY_SIZE));
		tMax = (int)((sky->maxs[1][i] + 1) * 0.5 * (float)(SKY_SIZE)) + 1;

		sMin = Clamp(sMin, 0, SKY_SIZE);
		sMax = Clamp(sMax, 0, SKY_SIZE);
		tMin = Clamp(tMin, 0, SKY_SIZE);
		tMax = Clamp(tMax, 0, SKY_SIZE);

		index = skySide->indices;
		for (t = tMin; t < tMax; t++){
			for (s = sMin; s < sMax; s++){
				index[0] = t * (SKY_SIZE+1) + s;
				index[1] = index[4] = index[0] + (SKY_SIZE+1);
				index[2] = index[3] = index[0] + 1;
				index[5] = index[1] + 1;

				index += 6;
			}
		}

		skySide->numIndices = (sMax-sMin) * (tMax-tMin) * 6;
	}

	// Draw the far box
	if (sky->shader->skyParms.farBox[0])
		R_DrawSkyBox(sky->shader->skyParms.farBox, false);

	// Draw the cloud layers
	if (sky->shader->numStages){
		qglDepthRange(1, 1);

		for (i = 0, skySide = sky->skySides; i < 6; i++, skySide++){
			if (sky->mins[0][i] >= sky->maxs[0][i] || sky->mins[1][i] >= sky->maxs[1][i])
				continue;

			// Draw it
			RB_CheckMeshOverflow(skySide->numIndices, skySide->numVertices);

			for (j = 0; j < skySide->numIndices; j += 3){
				indexArray[numIndex++] = numVertex + skySide->indices[j+0];
				indexArray[numIndex++] = numVertex + skySide->indices[j+1];
				indexArray[numIndex++] = numVertex + skySide->indices[j+2];
			}

			for (j = 0, v = skySide->vertices; j < skySide->numVertices; j++, v++){
				vertexArray[numVertex][0] = v->xyz[0];
				vertexArray[numVertex][1] = v->xyz[1];
				vertexArray[numVertex][2] = v->xyz[2];
				normalArray[numVertex][0] = v->normal[0];
				normalArray[numVertex][1] = v->normal[1];
				normalArray[numVertex][2] = v->normal[2];
				inTexCoordArray[numVertex][0] = v->sphere[0];
				inTexCoordArray[numVertex][1] = v->sphere[1];
				inColorArray[numVertex][0] = 255;
				inColorArray[numVertex][1] = 255;
				inColorArray[numVertex][2] = 255;
				inColorArray[numVertex][3] = 255;

				numVertex++;
			}
		}

		// Flush
		RB_RenderMesh();
	}

	// Draw the near box
	if (sky->shader->skyParms.nearBox[0])
		R_DrawSkyBox(sky->shader->skyParms.nearBox, true);

	qglDepthRange(0, 1);

	qglLoadMatrixf(r_worldMatrix);
}

/*
 =================
 R_ClearSky
 =================
*/
void R_ClearSky (void){

	sky_t	*sky = r_worldModel->sky;
	int		i;

	for (i = 0; i < 6; i++){
		sky->mins[0][i] = sky->mins[1][i] = 999999;
		sky->maxs[0][i] = sky->maxs[1][i] = -999999;
	}
}

/*
 =================
 R_ClipSkySurface
 =================
*/
void R_ClipSkySurface (surface_t *surf){

	surfPoly_t		*p;
	surfPolyVert_t	*v;
	vec3_t			verts[MAX_CLIP_VERTS];
	int				i;

	// Calculate vertex values for sky box
	for (p = surf->poly; p; p = p->next){
		for (i = 0, v = p->vertices; i < p->numVertices; i++, v++)
			VectorSubtract(v->xyz, r_refDef.viewOrigin, verts[i]);

		R_ClipSkyPolygon(p->numVertices, verts[0], 0);
	}
}

/*
 =================
 R_AddSkyToList
 =================
*/
void R_AddSkyToList (void){

	sky_t	*sky = r_worldModel->sky;
	int		i;

	// Check for no sky at all
	for (i = 0; i < 6; i++){
		if (sky->mins[0][i] < sky->maxs[0][i] && sky->mins[1][i] < sky->maxs[1][i])
			break;
	}
		
	if (i == 6)
		return;		// Nothing visible

	// HACK: force full sky to draw when rotating
	if (sky->rotate){
		for (i = 0; i < 6; i++){
			sky->mins[0][i] = -1;
			sky->mins[1][i] = -1;
			sky->maxs[0][i] = 1;
			sky->maxs[1][i] = 1;
		}
	}

	// Add it
	R_AddMeshToList(MESH_SKY, NULL, sky->shader, r_worldEntity, 0);
}

/*
 =================
 R_SetupSky
 =================
*/
void R_SetupSky (const char *name, float rotate, const vec3_t axis){

	sky_t			*sky;
	skySide_t		*skySide;
	skySideVert_t	*v;
    vec3_t			org, row, col;
	void			*map;
	int				i, j;

	r_worldModel->sky = sky = Hunk_Alloc(sizeof(sky_t));
	r_worldModel->size += sizeof(sky_t);

	sky->shader = R_FindShader(name, SHADER_SKY, 0);

	sky->rotate = rotate;
	VectorCopy(axis, sky->axis);

	// Fill sky sides
	for (i = 0, skySide = sky->skySides; i < 6; i++, skySide++){
		R_MakeSkyVec(i, -BOX_SIZE, -BOX_SIZE, BOX_SIZE, org);
		R_MakeSkyVec(i, 0, BOX_STEP, 0, row);
		R_MakeSkyVec(i, BOX_STEP, 0, 0, col);

		R_FillSkySide(skySide, sky->shader->skyParms.cloudHeight, org, row, col);

		// If VBO is enabled, put the sky box arrays in a static buffer
		if (glConfig.vertexBufferObject){
			sky->vbo[i] = RB_AllocStaticBuffer(GL_ARRAY_BUFFER_ARB, skySide->numVertices * (sizeof(vec3_t) + sizeof(vec2_t)));

			// Fill it in with vertices and texture coordinates
			map = qglMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
			if (map){
				for (j = 0, v = skySide->vertices; j < skySide->numVertices; j++, v++){
					((float *)map)[j*5+0] = v->xyz[0];
					((float *)map)[j*5+1] = v->xyz[1];
					((float *)map)[j*5+2] = v->xyz[2];
					((float *)map)[j*5+3] = v->st[0];
					((float *)map)[j*5+4] = v->st[1];
				}

				qglUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
			}
		}
	}
}
