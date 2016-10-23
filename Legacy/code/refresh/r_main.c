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


mat4_t			r_projectionMatrix;
mat4_t			r_worldMatrix;
mat4_t			r_entityMatrix;
mat4_t			r_textureMatrix;

cplane_t		r_frustum[4];

float			r_frameTime;

mesh_t			r_solidMeshes[MAX_MESHES];
int				r_numSolidMeshes;

mesh_t			r_transMeshes[MAX_MESHES];
int				r_numTransMeshes;

entity_t		r_entities[MAX_ENTITIES];
int				r_numEntities;

dlight_t		r_dlights[MAX_DLIGHTS];
int				r_numDLights;

particle_t		r_particles[MAX_PARTICLES];
int				r_numParticles;

poly_t			r_polys[MAX_POLYS];
int				r_numPolys;
polyVert_t		r_polyVerts[MAX_POLY_VERTS];
int				r_numPolyVerts;

entity_t		*r_nullModels[MAX_ENTITIES];
int				r_numNullModels;

lightStyle_t	r_lightStyles[MAX_LIGHTSTYLES];

refDef_t		r_refDef;

refStats_t		r_stats;

#define NUM_VIDEO_MODES	(sizeof(r_videoModes) / sizeof(videoMode_t))

typedef struct {
	const char	*description;
	int			width, height;
	int			mode;
} videoMode_t;

videoMode_t		r_videoModes[] = {
	{"Mode  0: 320x240",			 320,  240,  0},
	{"Mode  1: 400x300",			 400,  300,  1},
	{"Mode  2: 512x384",			 512,  384,  2},
	{"Mode  3: 640x480",			 640,  480,  3},
	{"Mode  4: 800x600",			 800,  600,  4},
	{"Mode  5: 960x720",			 960,  720,  5},
	{"Mode  6: 1024x768",			1024,  768,  6},
	{"Mode  7: 1152x864",			1152,  864,  7},
	{"Mode  8: 1280x1024",			1280, 1024,  8},
	{"Mode  9: 1600x1200",			1600, 1200,  9},
	{"Mode 10: 2048x1536",			2048, 1536, 10},
	{"Mode 11: 856x480 (wide)",		 856,  480, 11},
	{"Mode 12: 1920x1200 (wide)",	1920, 1200, 12},
};

cvar_t	*r_noRefresh;
cvar_t	*r_noVis;
cvar_t	*r_noCull;
cvar_t	*r_noBind;
cvar_t	*r_drawWorld;
cvar_t	*r_drawEntities;
cvar_t	*r_drawParticles;
cvar_t	*r_drawPolys;
cvar_t	*r_fullbright;
cvar_t	*r_lightmap;
cvar_t	*r_lockPVS;
cvar_t	*r_logFile;
cvar_t	*r_frontBuffer;
cvar_t	*r_showCluster;
cvar_t	*r_showTris;
cvar_t	*r_showNormals;
cvar_t	*r_showTangentSpace;
cvar_t	*r_showModelBounds;
cvar_t	*r_showShadowVolumes;
cvar_t	*r_offsetFactor;
cvar_t	*r_offsetUnits;
cvar_t	*r_debugSort;
cvar_t	*r_speeds;
cvar_t	*r_clear;
cvar_t	*r_singleShader;
cvar_t	*r_skipBackEnd;
cvar_t	*r_skipFrontEnd;
cvar_t	*r_allowSoftwareGL;
cvar_t	*r_maskMiniDriver;
cvar_t	*r_glDriver;
cvar_t	*r_allowExtensions;
cvar_t	*r_arb_multitexture;
cvar_t	*r_arb_texture_env_add;
cvar_t	*r_arb_texture_env_combine;
cvar_t	*r_arb_texture_env_dot3;
cvar_t	*r_arb_texture_cube_map;
cvar_t	*r_arb_texture_compression;
cvar_t	*r_arb_vertex_buffer_object;
cvar_t	*r_arb_vertex_program;
cvar_t	*r_arb_fragment_program;
cvar_t	*r_ext_draw_range_elements;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_ext_texture_edge_clamp;
cvar_t	*r_ext_texture_filter_anisotropic;
cvar_t	*r_ext_texture_rectangle;
cvar_t	*r_ext_stencil_two_side;
cvar_t	*r_ext_generate_mipmap;
cvar_t	*r_ext_swap_control;
cvar_t	*r_swapInterval;
cvar_t	*r_finish;
cvar_t	*r_stereo;
cvar_t	*r_colorBits;
cvar_t	*r_depthBits;
cvar_t	*r_stencilBits;
cvar_t	*r_mode;
cvar_t	*r_fullscreen;
cvar_t	*r_customWidth;
cvar_t	*r_customHeight;
cvar_t	*r_displayRefresh;
cvar_t	*r_ignoreHwGamma;
cvar_t	*r_gamma;
cvar_t	*r_overBrightBits;
cvar_t	*r_ignoreGLErrors;
cvar_t	*r_shadows;
cvar_t	*r_caustics;
cvar_t	*r_lodBias;
cvar_t	*r_lodDistance;
cvar_t	*r_dynamicLights;
cvar_t	*r_modulate;
cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_intensity;
cvar_t	*r_roundImagesDown;
cvar_t	*r_maxTextureSize;
cvar_t	*r_picmip;
cvar_t	*r_textureBits;
cvar_t	*r_textureFilter;
cvar_t	*r_textureFilterAnisotropy;
cvar_t	*r_jpegCompressionQuality;
cvar_t	*r_detailTextures;
cvar_t	*r_inGameVideo;


/*
 =================
 R_CullBox

 Returns true if the box is completely outside the frustum
 =================
*/
qboolean R_CullBox (const vec3_t mins, const vec3_t maxs, int clipFlags){

	int			i;
	cplane_t	*plane;

	if (r_noCull->integer)
		return false;

	for (i = 0, plane = r_frustum; i < 4; i++, plane++){
		if (!(clipFlags & (1<<i)))
			continue;

		switch (plane->signbits){
		case 0:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*maxs[1] + plane->normal[2]*maxs[2] < plane->dist)
				return true;
			break;
		case 1:
			if (plane->normal[0]*mins[0] + plane->normal[1]*maxs[1] + plane->normal[2]*maxs[2] < plane->dist)
				return true;
			break;
		case 2:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*mins[1] + plane->normal[2]*maxs[2] < plane->dist)
				return true;
			break;
		case 3:
			if (plane->normal[0]*mins[0] + plane->normal[1]*mins[1] + plane->normal[2]*maxs[2] < plane->dist)
				return true;
			break;
		case 4:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*maxs[1] + plane->normal[2]*mins[2] < plane->dist)
				return true;
			break;
		case 5:
			if (plane->normal[0]*mins[0] + plane->normal[1]*maxs[1] + plane->normal[2]*mins[2] < plane->dist)
				return true;
			break;
		case 6:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*mins[1] + plane->normal[2]*mins[2] < plane->dist)
				return true;
			break;
		case 7:
			if (plane->normal[0]*mins[0] + plane->normal[1]*mins[1] + plane->normal[2]*mins[2] < plane->dist)
				return true;
			break;
		default:
			return false;
		}
	}

	return false;
}

/*
 =================
 R_CullSphere

 Returns true if the sphere is completely outside the frustum
 =================
*/
qboolean R_CullSphere (const vec3_t origin, float radius, int clipFlags){

	int			i;
	cplane_t	*plane;

	if (r_noCull->integer)
		return false;

	for (i = 0, plane = r_frustum; i < 4; i++, plane++){
		if (!(clipFlags & (1<<i)))
			continue;

		if (DotProduct(origin, plane->normal) - plane->dist <= -radius)
			return true;
	}

	return false;
}

/*
 =================
 R_RotateForEntity
 =================
*/
void R_RotateForEntity (entity_t *entity){

	mat4_t	rotateMatrix;

	rotateMatrix[ 0] = entity->axis[0][0];
	rotateMatrix[ 1] = entity->axis[0][1];
	rotateMatrix[ 2] = entity->axis[0][2];
	rotateMatrix[ 3] = 0.0;
	rotateMatrix[ 4] = entity->axis[1][0];
	rotateMatrix[ 5] = entity->axis[1][1];
	rotateMatrix[ 6] = entity->axis[1][2];
	rotateMatrix[ 7] = 0.0;
	rotateMatrix[ 8] = entity->axis[2][0];
	rotateMatrix[ 9] = entity->axis[2][1];
	rotateMatrix[10] = entity->axis[2][2];
	rotateMatrix[11] = 0.0;
	rotateMatrix[12] = entity->origin[0];
	rotateMatrix[13] = entity->origin[1];
	rotateMatrix[14] = entity->origin[2];
	rotateMatrix[15] = 1.0;

	Matrix4_MultiplyFast(r_worldMatrix, rotateMatrix, r_entityMatrix);

	qglLoadMatrixf(r_entityMatrix);
}


// =====================================================================


/*
 =================
 R_AddSpriteModelToList

 I am only keeping this for backwards compatibility
 =================
*/
static void R_AddSpriteModelToList (entity_t *entity){

	spr_t		*sprite = entity->model->sprite;
	sprFrame_t	*frame;
	vec3_t		vec;

	frame = &sprite->frames[entity->frame % sprite->numFrames];

	// Cull
	if (!r_noCull->integer){
		VectorSubtract(entity->origin, r_refDef.viewOrigin, vec);
		VectorNormalizeFast(vec);

		if (DotProduct(vec, r_refDef.viewAxis[0]) < 0)
			return;
	}

	// HACK: make it a sprite entity
	entity->entityType = ET_SPRITE;
	entity->radius = frame->radius;
	entity->rotation = 0;
	entity->customShader = frame->shader;

	// HACK: make it translucent
	if (!(entity->customShader->flags & SHADER_EXTERNAL)){
		entity->customShader->sort = SORT_BLEND;
		entity->customShader->stages[0]->flags |= SHADERSTAGE_BLENDFUNC;
		entity->customShader->stages[0]->flags &= ~SHADERSTAGE_DEPTHWRITE;
		entity->customShader->stages[0]->blendFunc.src = GL_SRC_ALPHA;
		entity->customShader->stages[0]->blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;
	}

	// Add it
	R_AddMeshToList(MESH_SPRITE, NULL, entity->customShader, entity, 0);
}

/*
 =================
 R_DrawSprite
 =================
*/
void R_DrawSprite (void){

	vec3_t	axis[3];
	int		i;

	if (rb_entity->rotation){
		// Rotate it around its normal
		RotatePointAroundVector(axis[1], r_refDef.viewAxis[0], r_refDef.viewAxis[1], rb_entity->rotation);
		CrossProduct(r_refDef.viewAxis[0], axis[1], axis[2]);

		// The normal should point at the viewer
		VectorNegate(r_refDef.viewAxis[0], axis[0]);

		// Scale the axes by radius
		VectorScale(axis[1], rb_entity->radius, axis[1]);
		VectorScale(axis[2], rb_entity->radius, axis[2]);
	}
	else {
		// The normal should point at the viewer
		VectorNegate(r_refDef.viewAxis[0], axis[0]);

		// Scale the axes by radius
		VectorScale(r_refDef.viewAxis[1], rb_entity->radius, axis[1]);
		VectorScale(r_refDef.viewAxis[2], rb_entity->radius, axis[2]);
	}

	// Draw it
	RB_CheckMeshOverflow(6, 4);
	
	for (i = 2; i < 4; i++){
		indexArray[numIndex++] = numVertex + 0;
		indexArray[numIndex++] = numVertex + i-1;
		indexArray[numIndex++] = numVertex + i;
	}

	vertexArray[numVertex+0][0] = rb_entity->origin[0] + axis[1][0] + axis[2][0];
	vertexArray[numVertex+0][1] = rb_entity->origin[1] + axis[1][1] + axis[2][1];
	vertexArray[numVertex+0][2] = rb_entity->origin[2] + axis[1][2] + axis[2][2];
	vertexArray[numVertex+1][0] = rb_entity->origin[0] - axis[1][0] + axis[2][0];
	vertexArray[numVertex+1][1] = rb_entity->origin[1] - axis[1][1] + axis[2][1];
	vertexArray[numVertex+1][2] = rb_entity->origin[2] - axis[1][2] + axis[2][2];
	vertexArray[numVertex+2][0] = rb_entity->origin[0] - axis[1][0] - axis[2][0];
	vertexArray[numVertex+2][1] = rb_entity->origin[1] - axis[1][1] - axis[2][1];
	vertexArray[numVertex+2][2] = rb_entity->origin[2] - axis[1][2] - axis[2][2];
	vertexArray[numVertex+3][0] = rb_entity->origin[0] + axis[1][0] - axis[2][0];
	vertexArray[numVertex+3][1] = rb_entity->origin[1] + axis[1][1] - axis[2][1];
	vertexArray[numVertex+3][2] = rb_entity->origin[2] + axis[1][2] - axis[2][2];

	inTexCoordArray[numVertex+0][0] = 0;
	inTexCoordArray[numVertex+0][1] = 0;
	inTexCoordArray[numVertex+1][0] = 1;
	inTexCoordArray[numVertex+1][1] = 0;
	inTexCoordArray[numVertex+2][0] = 1;
	inTexCoordArray[numVertex+2][1] = 1;
	inTexCoordArray[numVertex+3][0] = 0;
	inTexCoordArray[numVertex+3][1] = 1;

	for (i = 0; i < 4; i++){
		normalArray[numVertex][0] = axis[0][0];
		normalArray[numVertex][1] = axis[0][1];
		normalArray[numVertex][2] = axis[0][2];
		inColorArray[numVertex][0] = rb_entity->shaderRGBA[0];
		inColorArray[numVertex][1] = rb_entity->shaderRGBA[1];
		inColorArray[numVertex][2] = rb_entity->shaderRGBA[2];
		inColorArray[numVertex][3] = rb_entity->shaderRGBA[3];

		numVertex++;
	}
}

/*
 =================
 R_AddSpriteToList
 =================
*/
static void R_AddSpriteToList (entity_t *entity){

	vec3_t	vec;

	// Cull
	if (!r_noCull->integer){
		VectorSubtract(entity->origin, r_refDef.viewOrigin, vec);
		VectorNormalizeFast(vec);

		if (DotProduct(vec, r_refDef.viewAxis[0]) < 0)
			return;
	}

	// Add it
	R_AddMeshToList(MESH_SPRITE, NULL, entity->customShader, entity, 0);
}

/*
 =================
 R_DrawBeam
 =================
*/
void R_DrawBeam (void){

	vec3_t	axis[3];
	float	length;
	int		i;

	// Find orientation vectors
	VectorSubtract(r_refDef.viewOrigin, rb_entity->origin, axis[0]);
	VectorSubtract(rb_entity->oldOrigin, rb_entity->origin, axis[1]);

	CrossProduct(axis[0], axis[1], axis[2]);
	VectorNormalizeFast(axis[2]);

	// Find normal
	CrossProduct(axis[1], axis[2], axis[0]);
	VectorNormalizeFast(axis[0]);

	// Scale by radius
	VectorScale(axis[2], rb_entity->frame / 2, axis[2]);

	// Find segment length
	length = VectorLength(axis[1]) / rb_entity->oldFrame;

	// Draw it
	RB_CheckMeshOverflow(6, 4);
	
	for (i = 2; i < 4; i++){
		indexArray[numIndex++] = numVertex + 0;
		indexArray[numIndex++] = numVertex + i-1;
		indexArray[numIndex++] = numVertex + i;
	}

	vertexArray[numVertex+0][0] = rb_entity->origin[0] + axis[2][0];
	vertexArray[numVertex+0][1] = rb_entity->origin[1] + axis[2][1];
	vertexArray[numVertex+0][2] = rb_entity->origin[2] + axis[2][2];
	vertexArray[numVertex+1][0] = rb_entity->oldOrigin[0] + axis[2][0];
	vertexArray[numVertex+1][1] = rb_entity->oldOrigin[1] + axis[2][1];
	vertexArray[numVertex+1][2] = rb_entity->oldOrigin[2] + axis[2][2];
	vertexArray[numVertex+2][0] = rb_entity->oldOrigin[0] - axis[2][0];
	vertexArray[numVertex+2][1] = rb_entity->oldOrigin[1] - axis[2][1];
	vertexArray[numVertex+2][2] = rb_entity->oldOrigin[2] - axis[2][2];
	vertexArray[numVertex+3][0] = rb_entity->origin[0] - axis[2][0];
	vertexArray[numVertex+3][1] = rb_entity->origin[1] - axis[2][1];
	vertexArray[numVertex+3][2] = rb_entity->origin[2] - axis[2][2];

	inTexCoordArray[numVertex+0][0] = 0;
	inTexCoordArray[numVertex+0][1] = 0;
	inTexCoordArray[numVertex+1][0] = length;
	inTexCoordArray[numVertex+1][1] = 0;
	inTexCoordArray[numVertex+2][0] = length;
	inTexCoordArray[numVertex+2][1] = 1;
	inTexCoordArray[numVertex+3][0] = 0;
	inTexCoordArray[numVertex+3][1] = 1;

	for (i = 0; i < 4; i++){
		normalArray[numVertex][0] = axis[0][0];
		normalArray[numVertex][1] = axis[0][1];
		normalArray[numVertex][2] = axis[0][2];
		inColorArray[numVertex][0] = rb_entity->shaderRGBA[0];
		inColorArray[numVertex][1] = rb_entity->shaderRGBA[1];
		inColorArray[numVertex][2] = rb_entity->shaderRGBA[2];
		inColorArray[numVertex][3] = rb_entity->shaderRGBA[3];

		numVertex++;
	}
}

/*
 =================
 R_AddBeamToList
 =================
*/
static void R_AddBeamToList (entity_t *entity){

	R_AddMeshToList(MESH_BEAM, NULL, entity->customShader, entity, 0);
}

/*
 =================
 R_AddEntitiesToList
 =================
*/
static void R_AddEntitiesToList (void){

	entity_t	*entity;
	model_t		*model;
	int			i;

	if (!r_drawEntities->integer || r_numEntities == 1)
		return;

	r_stats.numEntities += (r_numEntities - 1);

	for (i = 1, entity = &r_entities[1]; i < r_numEntities; i++, entity++){
		switch (entity->entityType){
		case ET_MODEL:
			model = entity->model;

			if (!model || model->modelType == MODEL_BAD){
				r_nullModels[r_numNullModels++] = entity;
				break;
			}

			switch (model->modelType){
			case MODEL_BSP:
				R_AddBrushModelToList(entity);
				break;
			case MODEL_MD3:
			case MODEL_MD2:
				R_AddAliasModelToList(entity);
				break;
			case MODEL_SP2:
				R_AddSpriteModelToList(entity);
				break;
			default:
				Com_Error(ERR_DROP, "R_AddEntitiesToList: bad modelType (%i)", model->modelType);
			}

			break;
		case ET_SPRITE:
			R_AddSpriteToList(entity);
			break;
		case ET_BEAM:
			R_AddBeamToList(entity);
			break;
		default:
			Com_Error(ERR_DROP, "R_AddEntitiesToList: bad entityType (%i)", entity->entityType);
		}
	}
}

/*
 =================
 R_DrawNullModels
 =================
*/
static void R_DrawNullModels (void){

	entity_t	*entity;
	vec3_t		points[3];
	int			i;

	if (!r_numNullModels)
		return;

	qglLoadMatrixf(r_worldMatrix);

	// Set the state
	GL_Enable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_VERTEX_PROGRAM_ARB);
	GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
	GL_Disable(GL_ALPHA_TEST);
	GL_Enable(GL_BLEND);
	GL_Enable(GL_DEPTH_TEST);

	GL_CullFace(GL_FRONT);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_DepthFunc(GL_LEQUAL);
	GL_DepthMask(GL_FALSE);

	// Draw them
	for (i = 0; i < r_numNullModels; i++){
		entity = r_nullModels[i];

		VectorMA(entity->origin, 15, entity->axis[0], points[0]);
		VectorMA(entity->origin, -15, entity->axis[1], points[1]);
		VectorMA(entity->origin, 15, entity->axis[2], points[2]);

		qglBegin(GL_LINES);

		qglColor4ub(255, 0, 0, 127);
		qglVertex3fv(entity->origin);
		qglVertex3fv(points[0]);

		qglColor4ub(0, 255, 0, 127);
		qglVertex3fv(entity->origin);
		qglVertex3fv(points[1]);

		qglColor4ub(0, 0, 255, 127);
		qglVertex3fv(entity->origin);
		qglVertex3fv(points[2]);

		qglEnd();
	}

	r_numNullModels = 0;
}

/*
 =================
 R_DrawParticle
 =================
*/
void R_DrawParticle (void){

	particle_t	*particle = rb_mesh->mesh;
	vec3_t		axis[3];
	int			i;

	// Draw it
	RB_CheckMeshOverflow(6, 4);
	
	for (i = 2; i < 4; i++){
		indexArray[numIndex++] = numVertex + 0;
		indexArray[numIndex++] = numVertex + i-1;
		indexArray[numIndex++] = numVertex + i;
	}

	if (particle->length != 1){
		// Find orientation vectors
		VectorSubtract(r_refDef.viewOrigin, particle->origin, axis[0]);
		VectorSubtract(particle->oldOrigin, particle->origin, axis[1]);
		CrossProduct(axis[0], axis[1], axis[2]);

		VectorNormalizeFast(axis[1]);
		VectorNormalizeFast(axis[2]);

		// Find normal
		CrossProduct(axis[1], axis[2], axis[0]);
		VectorNormalizeFast(axis[0]);

		VectorMA(particle->origin, -particle->length, axis[1], particle->oldOrigin);
		VectorScale(axis[2], particle->radius, axis[2]);

		vertexArray[numVertex+0][0] = particle->oldOrigin[0] + axis[2][0];
		vertexArray[numVertex+0][1] = particle->oldOrigin[1] + axis[2][1];
		vertexArray[numVertex+0][2] = particle->oldOrigin[2] + axis[2][2];
		vertexArray[numVertex+1][0] = particle->origin[0] + axis[2][0];
		vertexArray[numVertex+1][1] = particle->origin[1] + axis[2][1];
		vertexArray[numVertex+1][2] = particle->origin[2] + axis[2][2];
		vertexArray[numVertex+2][0] = particle->origin[0] - axis[2][0];
		vertexArray[numVertex+2][1] = particle->origin[1] - axis[2][1];
		vertexArray[numVertex+2][2] = particle->origin[2] - axis[2][2];
		vertexArray[numVertex+3][0] = particle->oldOrigin[0] - axis[2][0];
		vertexArray[numVertex+3][1] = particle->oldOrigin[1] - axis[2][1];
		vertexArray[numVertex+3][2] = particle->oldOrigin[2] - axis[2][2];
	}
	else {
		if (particle->rotation){
			// Rotate it around its normal
			RotatePointAroundVector(axis[1], r_refDef.viewAxis[0], r_refDef.viewAxis[1], particle->rotation);
			CrossProduct(r_refDef.viewAxis[0], axis[1], axis[2]);

			// The normal should point at the viewer
			VectorNegate(r_refDef.viewAxis[0], axis[0]);

			// Scale the axes by radius
			VectorScale(axis[1], particle->radius, axis[1]);
			VectorScale(axis[2], particle->radius, axis[2]);
		}
		else {
			// The normal should point at the viewer
			VectorNegate(r_refDef.viewAxis[0], axis[0]);

			// Scale the axes by radius
			VectorScale(r_refDef.viewAxis[1], particle->radius, axis[1]);
			VectorScale(r_refDef.viewAxis[2], particle->radius, axis[2]);
		}

		vertexArray[numVertex+0][0] = particle->origin[0] + axis[1][0] + axis[2][0];
		vertexArray[numVertex+0][1] = particle->origin[1] + axis[1][1] + axis[2][1];
		vertexArray[numVertex+0][2] = particle->origin[2] + axis[1][2] + axis[2][2];
		vertexArray[numVertex+1][0] = particle->origin[0] - axis[1][0] + axis[2][0];
		vertexArray[numVertex+1][1] = particle->origin[1] - axis[1][1] + axis[2][1];
		vertexArray[numVertex+1][2] = particle->origin[2] - axis[1][2] + axis[2][2];
		vertexArray[numVertex+2][0] = particle->origin[0] - axis[1][0] - axis[2][0];
		vertexArray[numVertex+2][1] = particle->origin[1] - axis[1][1] - axis[2][1];
		vertexArray[numVertex+2][2] = particle->origin[2] - axis[1][2] - axis[2][2];
		vertexArray[numVertex+3][0] = particle->origin[0] + axis[1][0] - axis[2][0];
		vertexArray[numVertex+3][1] = particle->origin[1] + axis[1][1] - axis[2][1];
		vertexArray[numVertex+3][2] = particle->origin[2] + axis[1][2] - axis[2][2];
	}

	inTexCoordArray[numVertex+0][0] = 0;
	inTexCoordArray[numVertex+0][1] = 0;
	inTexCoordArray[numVertex+1][0] = 1;
	inTexCoordArray[numVertex+1][1] = 0;
	inTexCoordArray[numVertex+2][0] = 1;
	inTexCoordArray[numVertex+2][1] = 1;
	inTexCoordArray[numVertex+3][0] = 0;
	inTexCoordArray[numVertex+3][1] = 1;

	for (i = 0; i < 4; i++){
		normalArray[numVertex][0] = axis[0][0];
		normalArray[numVertex][1] = axis[0][1];
		normalArray[numVertex][2] = axis[0][2];
		inColorArray[numVertex][0] = particle->modulate[0];
		inColorArray[numVertex][1] = particle->modulate[1];
		inColorArray[numVertex][2] = particle->modulate[2];
		inColorArray[numVertex][3] = particle->modulate[3];

		numVertex++;
	}
}

/*
 =================
 R_AddParticlesToList
 =================
*/
static void R_AddParticlesToList (void){

	particle_t	*particle;
	vec3_t		vec;
	int			i;

	if (!r_drawParticles->integer || !r_numParticles)
		return;

	r_stats.numParticles += r_numParticles;

	for (i = 0, particle = r_particles; i < r_numParticles; i++, particle++){
		// Cull
		if (!r_noCull->integer){
			VectorSubtract(particle->origin, r_refDef.viewOrigin, vec);
			VectorNormalizeFast(vec);

			if (DotProduct(vec, r_refDef.viewAxis[0]) < 0)
				continue;
		}

		// Add it
		R_AddMeshToList(MESH_PARTICLE, particle, particle->shader, r_worldEntity, 0);
	}
}

/*
 =================
 R_DrawPoly
 =================
*/
void R_DrawPoly (void){

	poly_t		*poly = rb_mesh->mesh;
	polyVert_t	*vert;
	int			i;

	RB_CheckMeshOverflow((poly->numVerts-2) * 3, poly->numVerts);

	for (i = 2; i < poly->numVerts; i++){
		indexArray[numIndex++] = numVertex + 0;
		indexArray[numIndex++] = numVertex + i-1;
		indexArray[numIndex++] = numVertex + i;
	}

	for (i = 0, vert = poly->verts; i < poly->numVerts; i++, vert++){
		vertexArray[numVertex][0] = vert->xyz[0];
		vertexArray[numVertex][1] = vert->xyz[1];
		vertexArray[numVertex][2] = vert->xyz[2];
		inTexCoordArray[numVertex][0] = vert->st[0];
		inTexCoordArray[numVertex][1] = vert->st[1];
		inColorArray[numVertex][0] = vert->modulate[0];
		inColorArray[numVertex][1] = vert->modulate[1];
		inColorArray[numVertex][2] = vert->modulate[2];
		inColorArray[numVertex][3] = vert->modulate[3];

		numVertex++;
	}
}

/*
 =================
 R_AddPolysToList
 =================
*/
static void R_AddPolysToList (void){

	poly_t	*poly;
	int		i;

	if (!r_drawPolys->integer || !r_numPolys)
		return;

	r_stats.numPolys += r_numPolys;

	for (i = 0, poly = r_polys; i < r_numPolys; i++, poly++)
		R_AddMeshToList(MESH_POLY, poly, poly->shader, r_worldEntity, 0);
}


// =====================================================================


/*
 =================
 R_QSortMeshes
 =================
*/
static void R_QSortMeshes (mesh_t *meshes, int numMeshes){

	static mesh_t	tmp;
	static int		stack[4096];
	int				depth = 0;
	int				L, R, l, r, median;
	unsigned		pivot;

	if (!numMeshes)
		return;

	L = 0;
	R = numMeshes - 1;

start:
	l = L;
	r = R;

	median = (L + R) >> 1;

	if (meshes[L].sortKey > meshes[median].sortKey){
		if (meshes[L].sortKey < meshes[R].sortKey) 
			median = L;
	} 
	else if (meshes[R].sortKey < meshes[median].sortKey)
		median = R;

	pivot = meshes[median].sortKey;

	while (l < r){
		while (meshes[l].sortKey < pivot)
			l++;
		while (meshes[r].sortKey > pivot)
			r--;

		if (l <= r){
			tmp = meshes[r];
			meshes[r] = meshes[l];
			meshes[l] = tmp;

			l++;
			r--;
		}
	}

	if ((L < r) && (depth < 4096)){
		stack[depth++] = l;
		stack[depth++] = R;
		R = r;
		goto start;
	}

	if (l < R){
		L = l;
		goto start;
	}

	if (depth){
		R = stack[--depth];
		L = stack[--depth];
		goto start;
	}
}

/*
 =================
 R_ISortMeshes
 =================
*/
static void R_ISortMeshes (mesh_t *meshes, int numMeshes){

	static mesh_t	tmp;
	int				i, j;

	if (!numMeshes)
		return;

	for (i = 1; i < numMeshes; i++){
		tmp = meshes[i];
		j = i - 1;

		while ((j >= 0) && (meshes[j].sortKey > tmp.sortKey)){
			meshes[j+1] = meshes[j];
			j--;
		}

		if (i != j+1)
			meshes[j+1] = tmp;
	}
}

/*
 =================
 R_AddMeshToList

 Calculates sort key and stores info used for sorting and batching.
 All 3D geometry passes this function.
 =================
*/
void R_AddMeshToList (meshType_t meshType, void *mesh, shader_t *shader, entity_t *entity, int infoKey){

	mesh_t	*m;

	if (shader->sort <= SORT_DECAL){
		if (r_numSolidMeshes == MAX_MESHES)
			Com_Error(ERR_DROP, "R_AddMeshToList: MAX_MESHES hit");

		m = &r_solidMeshes[r_numSolidMeshes++];
	}
	else {
		if (r_numTransMeshes == MAX_MESHES)
			Com_Error(ERR_DROP, "R_AddMeshToList: MAX_MESHES hit");

		m = &r_transMeshes[r_numTransMeshes++];
	}

	m->sortKey = (shader->sort << 28) | (shader->shaderNum << 18) | ((entity - r_entities) << 8) | (infoKey);

	m->meshType = meshType;
	m->mesh = mesh;
}


// =====================================================================


/*
 =================
 R_SetFrustum
 =================
*/
static void R_SetFrustum (void){

	int		i;

	RotatePointAroundVector(r_frustum[0].normal, r_refDef.viewAxis[2], r_refDef.viewAxis[0], -(90 - r_refDef.fovX / 2));
	RotatePointAroundVector(r_frustum[1].normal, r_refDef.viewAxis[2], r_refDef.viewAxis[0], 90 - r_refDef.fovX / 2);
	RotatePointAroundVector(r_frustum[2].normal, r_refDef.viewAxis[1], r_refDef.viewAxis[0], 90 - r_refDef.fovY / 2);
	RotatePointAroundVector(r_frustum[3].normal, r_refDef.viewAxis[1], r_refDef.viewAxis[0], -(90 - r_refDef.fovY / 2));

	for (i = 0; i < 4; i++){
		r_frustum[i].type = PLANE_NON_AXIAL;
		r_frustum[i].dist = DotProduct(r_refDef.viewOrigin, r_frustum[i].normal);
		SetPlaneSignbits(&r_frustum[i]);
	}
}

/*
 =================
 R_SetFarClip
 =================
*/
static float R_SetFarClip (void){

	float	farDist, dirDist, worldDist = 0;
	int		i;

	if (r_refDef.rdFlags & RDF_NOWORLDMODEL)
		return 4096.0;

	dirDist = DotProduct(r_refDef.viewOrigin, r_refDef.viewAxis[0]);
	farDist = dirDist + 256.0;

	for (i = 0; i < 3; i++){
		if (r_refDef.viewAxis[0][i] < 0)
			worldDist += (r_worldMins[i] * r_refDef.viewAxis[0][i]);
		else
			worldDist += (r_worldMaxs[i] * r_refDef.viewAxis[0][i]);
	}

	if (farDist < worldDist)
		farDist = worldDist;

	return farDist - dirDist + 256.0;
}

/*
 =================
 R_SetMatrices
 =================
*/
static void R_SetMatrices (void){

	float	xMax, xMin, yMax, yMin;
	float	xDiv, yDiv, zDiv;
	float	zNear, zFar;

	zNear = 4.0;
	zFar = R_SetFarClip();

	xMax = zNear * tan(r_refDef.fovX * M_PI / 360.0);
	xMin = -xMax;

	yMax = zNear * tan(r_refDef.fovY * M_PI / 360.0);
	yMin = -yMax;

	if (glConfig.stereoEnabled){
		xMax += -(2 * glState.stereoSeparation) / zNear;
		yMin += -(2 * glState.stereoSeparation) / zNear;
	}

	xDiv = 1.0 / (xMax - xMin);
	yDiv = 1.0 / (yMax - yMin);
	zDiv = 1.0 / (zFar - zNear);

	r_projectionMatrix[ 0] = (2.0 * zNear) * xDiv;
	r_projectionMatrix[ 1] = 0.0;
	r_projectionMatrix[ 2] = 0.0;
	r_projectionMatrix[ 3] = 0.0;
	r_projectionMatrix[ 4] = 0.0;
	r_projectionMatrix[ 5] = (2.0 * zNear) * yDiv;
	r_projectionMatrix[ 6] = 0.0;
	r_projectionMatrix[ 7] = 0.0;
	r_projectionMatrix[ 8] = (xMax + xMin) * xDiv;
	r_projectionMatrix[ 9] = (yMax + yMin) * yDiv;
	r_projectionMatrix[10] = -(zNear + zFar) * zDiv;
	r_projectionMatrix[11] = -1.0;
	r_projectionMatrix[12] = 0.0;
	r_projectionMatrix[13] = 0.0;
	r_projectionMatrix[14] = -(2.0 * zNear * zFar) * zDiv;
	r_projectionMatrix[15] = 0.0;

	r_worldMatrix[ 0] = -r_refDef.viewAxis[1][0];
	r_worldMatrix[ 1] = r_refDef.viewAxis[2][0];
	r_worldMatrix[ 2] = -r_refDef.viewAxis[0][0];
	r_worldMatrix[ 3] = 0.0;
	r_worldMatrix[ 4] = -r_refDef.viewAxis[1][1];
	r_worldMatrix[ 5] = r_refDef.viewAxis[2][1];
	r_worldMatrix[ 6] = -r_refDef.viewAxis[0][1];
	r_worldMatrix[ 7] = 0.0;
	r_worldMatrix[ 8] = -r_refDef.viewAxis[1][2];
	r_worldMatrix[ 9] = r_refDef.viewAxis[2][2];
	r_worldMatrix[10] = -r_refDef.viewAxis[0][2];
	r_worldMatrix[11] = 0.0;
	r_worldMatrix[12] = DotProduct(r_refDef.viewOrigin, r_refDef.viewAxis[1]);
	r_worldMatrix[13] = -DotProduct(r_refDef.viewOrigin, r_refDef.viewAxis[2]);
	r_worldMatrix[14] = DotProduct(r_refDef.viewOrigin, r_refDef.viewAxis[0]);
	r_worldMatrix[15] = 1.0;

	r_textureMatrix[ 0] = r_refDef.viewAxis[1][0];
	r_textureMatrix[ 1] = -r_refDef.viewAxis[1][1];
	r_textureMatrix[ 2] = -r_refDef.viewAxis[1][2];
	r_textureMatrix[ 3] = 0.0;
	r_textureMatrix[ 4] = -r_refDef.viewAxis[2][0];
	r_textureMatrix[ 5] = r_refDef.viewAxis[2][1];
	r_textureMatrix[ 6] = r_refDef.viewAxis[2][2];
	r_textureMatrix[ 7] = 0.0;
	r_textureMatrix[ 8] = r_refDef.viewAxis[0][0];
	r_textureMatrix[ 9] = -r_refDef.viewAxis[0][1];
	r_textureMatrix[10] = -r_refDef.viewAxis[0][2];
	r_textureMatrix[11] = 0.0;
	r_textureMatrix[12] = 0.0;
	r_textureMatrix[13] = 0.0;
	r_textureMatrix[14] = 0.0;
	r_textureMatrix[15] = 1.0;
}

/*
 =================
 R_SetGammaRamp
 =================
*/
static void R_SetGammaRamp (void){

	float	gamma;
	int		i, v;

	if (r_gamma->value > 3.0)
		Cvar_SetValue("r_gamma", 3.0);
	else if (r_gamma->value < 0.5)
		Cvar_SetValue("r_gamma", 0.5);

	gamma = (1.0 / (float)(1 << r_overBrightBits->integer)) / r_gamma->value;

	for (i = 0; i < 256; i++){
		v = 255 * pow(i / 255.0, gamma);
		v = Clamp(v, 0, 255);

		glState.gammaRamp[i] = glState.gammaRamp[i+256] = glState.gammaRamp[i+512] = ((unsigned short)v) << 8;
	}

	GLimp_SetDeviceGammaRamp(glState.gammaRamp);
}

/*
 =================
 R_RenderView
 =================
*/
void R_RenderView (void){

	if (r_skipFrontEnd->integer)
		return;

	r_numSolidMeshes = 0;
	r_numTransMeshes = 0;

	// Set up frustum
	R_SetFrustum();

	// Build mesh lists
	R_AddWorldToList();
	R_AddEntitiesToList();
	R_AddParticlesToList();
	R_AddPolysToList();

	// Sort mesh lists
	R_QSortMeshes(r_solidMeshes, r_numSolidMeshes);
	R_ISortMeshes(r_transMeshes, r_numTransMeshes);

	// Set up matrices
	R_SetMatrices();

	// Go into 3D mode
	GL_Setup3D();

	// Render everything
	RB_RenderMeshes(r_solidMeshes, r_numSolidMeshes);

	R_RenderShadows();

	RB_RenderMeshes(r_transMeshes, r_numTransMeshes);

	// Finish up
	R_DrawNullModels();
}

/*
 =================
 R_ClearScene
 =================
*/
void R_ClearScene (void){

	r_numEntities = 1;
	r_numDLights = 0;
	r_numParticles = 0;
	r_numPolys = 0;
	r_numPolyVerts = 0;
}

/*
 =================
 R_AddEntityToScene
 =================
*/
void R_AddEntityToScene (const entity_t *entity){

	if (r_numEntities >= MAX_ENTITIES)
		return;

	r_entities[r_numEntities++] = *entity;
}

/*
 =================
 R_AddLightToScene
 =================
*/
void R_AddLightToScene (const vec3_t origin, float intensity, float r, float g, float b){

	dlight_t	*dl;

	if (r_numDLights >= MAX_DLIGHTS)
		return;

	dl = &r_dlights[r_numDLights++];

	VectorCopy(origin, dl->origin);
	VectorSet(dl->color, r, g, b);
	dl->intensity = intensity;
}

/*
 =================
 R_AddParticleToScene
 =================
*/
void R_AddParticleToScene (shader_t *shader, const vec3_t origin, const vec3_t oldOrigin, float radius, float length, float rotation, const color_t modulate){

	particle_t	*particle;

	if (r_numParticles >= MAX_PARTICLES)
		return;

	particle = &r_particles[r_numParticles++];

	particle->shader = shader;
	VectorCopy(origin, particle->origin);
	VectorCopy(oldOrigin, particle->oldOrigin);
	particle->radius = radius;
	particle->length = length;
	particle->rotation = rotation;
	MakeRGBA(particle->modulate, modulate[0], modulate[1], modulate[2], modulate[3]);
}

/*
 =================
 R_AddPolyToScene
 =================
*/
void R_AddPolyToScene (shader_t *shader, int numVerts, const polyVert_t *verts){

	poly_t		*poly;

	if (r_numPolys >= MAX_POLYS || r_numPolyVerts + numVerts > MAX_POLY_VERTS)
		return;

	poly = &r_polys[r_numPolys++];

	poly->shader = shader;
	poly->numVerts = numVerts;
	poly->verts = &r_polyVerts[r_numPolyVerts];

	memcpy(poly->verts, verts, numVerts * sizeof(polyVert_t));
	r_numPolyVerts += numVerts;
}

/*
 =================
 R_RenderScene
 =================
*/
void R_RenderScene (const refDef_t *rd){

	if (r_noRefresh->integer)
		return;

	r_refDef = *rd;

	if (!(r_refDef.rdFlags & RDF_NOWORLDMODEL)){
		if (!r_worldModel)
			Com_Error(ERR_DROP, "R_RenderScene: NULL worldmodel");
	}

	// Make sure all 2D stuff is flushed
	RB_RenderMesh();

	// Render view
	R_RenderView();

	// Go into 2D mode
	GL_Setup2D();
}

/*
 =================
 R_SetLightStyle
 =================
*/
void R_SetLightStyle (int style, float r, float g, float b){

	lightStyle_t	*ls;

	if (style < 0 || style >= MAX_LIGHTSTYLES)
		return;

	ls = &r_lightStyles[style];

	ls->white = r + g + b;
	VectorSet(ls->rgb, r, g, b);
}

/*
 =================
 R_LerpTag
 =================
*/
qboolean R_LerpTag (tag_t *tag, model_t *model, int curFrame, int oldFrame, float backLerp, const char *tagName){

	int			i;
	mdl_t		*alias;
	mdlTag_t	*curTag, *oldTag;

	if (model->modelType != MODEL_MD3)
		return false;

	alias = model->alias[0];

	// Find the tag
	for (i = 0; i < alias->numTags; i++){
		if (!Q_stricmp(alias->tags[i].name, tagName))
			break;
	}

	if (i == alias->numTags){
		Com_DPrintf(S_COLOR_YELLOW "R_LerpTag: no such tag %s (%s)\n", tagName, model->realName);
		return false;
	}

	if ((curFrame < 0 || curFrame >= alias->numFrames) || (oldFrame < 0 || oldFrame >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_LerpTag: no such frame %i to %i (%s)\n", curFrame, oldFrame, model->realName);
		return false;
	}

	curTag = alias->tags + alias->numTags * curFrame + i;
	oldTag = alias->tags + alias->numTags * oldFrame + i;

	// Interpolate origin
	tag->origin[0] = curTag->origin[0] + (oldTag->origin[0] - curTag->origin[0]) * backLerp;
	tag->origin[1] = curTag->origin[1] + (oldTag->origin[1] - curTag->origin[1]) * backLerp;
	tag->origin[2] = curTag->origin[2] + (oldTag->origin[2] - curTag->origin[2]) * backLerp;

	// Interpolate axes
	tag->axis[0][0] = curTag->axis[0][0] + (oldTag->axis[0][0] - curTag->axis[0][0]) * backLerp;
	tag->axis[0][1] = curTag->axis[0][1] + (oldTag->axis[0][1] - curTag->axis[0][1]) * backLerp;
	tag->axis[0][2] = curTag->axis[0][2] + (oldTag->axis[0][2] - curTag->axis[0][2]) * backLerp;
	tag->axis[1][0] = curTag->axis[1][0] + (oldTag->axis[1][0] - curTag->axis[1][0]) * backLerp;
	tag->axis[1][1] = curTag->axis[1][1] + (oldTag->axis[1][1] - curTag->axis[1][1]) * backLerp;
	tag->axis[1][2] = curTag->axis[1][2] + (oldTag->axis[1][2] - curTag->axis[1][2]) * backLerp;
	tag->axis[2][0] = curTag->axis[2][0] + (oldTag->axis[2][0] - curTag->axis[2][0]) * backLerp;
	tag->axis[2][1] = curTag->axis[2][1] + (oldTag->axis[2][1] - curTag->axis[2][1]) * backLerp;
	tag->axis[2][2] = curTag->axis[2][2] + (oldTag->axis[2][2] - curTag->axis[2][2]) * backLerp;

	// Normalize axes
	VectorNormalize(tag->axis[0]);
	VectorNormalize(tag->axis[1]);
	VectorNormalize(tag->axis[2]);

	return true;
}

/*
 =================
 R_GetGLConfig

 Used by other systems to get the GL config
 =================
*/
void R_GetGLConfig (glConfig_t *config){

	if (!config)
		return;

	*config = glConfig;
}

/*
 =================
 R_GetModeInfo
 =================
*/
qboolean R_GetModeInfo (int *width, int *height, int mode){

	if (mode == -1){
		if (r_customWidth->integer < 1 || r_customHeight->integer < 1)
			return false;

		*width = r_customWidth->integer;
		*height = r_customHeight->integer;
		return true;
	}

	if (mode < 0 || mode >= NUM_VIDEO_MODES)
		return false;

	*width = r_videoModes[mode].width;
	*height = r_videoModes[mode].height;

	return true;
}

/*
 =================
 R_ModeList_t
 =================
*/
void R_ModeList_f (void){

	int i;

	Com_Printf("Mode -1: %ix%i (custom)\n", r_customWidth->integer, r_customHeight->integer);

	for (i = 0; i < NUM_VIDEO_MODES; i++)
		Com_Printf("%s\n", r_videoModes[i].description);
}

/*
 =================
 R_GfxInfo_f
 =================
*/
void R_GfxInfo_f (void){

	Com_Printf("\n");
	Com_Printf("GL_VENDOR: %s\n", glConfig.vendorString);
	Com_Printf("GL_RENDERER: %s\n", glConfig.rendererString);
	Com_Printf("GL_VERSION: %s\n", glConfig.versionString);
	Com_Printf("GL_EXTENSIONS: %s\n", glConfig.extensionsString);

	Com_Printf("GL_MAX_TEXTURE_SIZE: %i\n", glConfig.maxTextureSize);
	
	if (glConfig.multitexture)
		Com_Printf("GL_MAX_TEXTURE_UNITS_ARB: %i\n", glConfig.maxTextureUnits);
	if (glConfig.textureCubeMap)
		Com_Printf("GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB: %i\n", glConfig.maxCubeMapTextureSize);
	if (glConfig.fragmentProgram){
		Com_Printf("GL_MAX_TEXTURE_COORDS_ARB: %i\n", glConfig.maxTextureCoords);
		Com_Printf("GL_MAX_TEXTURE_IMAGE_UNITS_ARB: %i\n", glConfig.maxTextureImageUnits);
	}
	if (glConfig.textureFilterAnisotropic)
		Com_Printf("GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: %.1f\n", glConfig.maxTextureMaxAnisotropy);
	if (glConfig.textureRectangle)
		Com_Printf("GL_MAX_RECTANGLE_TEXTURE_SIZE_NV: %i\n", glConfig.maxRectangleTextureSize);

	Com_Printf("\n");
	Com_Printf("PIXELFORMAT: color(%i-bits) Z(%i-bits) stencil(%i-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
	Com_Printf("MODE: %i, %i x %i %s hz:%s\n", r_mode->integer, glConfig.videoWidth, glConfig.videoHeight, (glConfig.isFullscreen) ? "fullscreen" : "windowed", (glConfig.displayFrequency) ? va("%i", glConfig.displayFrequency) : "N/A");
	Com_Printf("GAMMA: %s w/ %i overbright bits\n", (glConfig.deviceSupportsGamma) ? "hardware" : "software", (glConfig.deviceSupportsGamma) ? r_overBrightBits->integer : 0);
	Com_Printf("CPU: %s\n", Cvar_VariableString("sys_cpuString"));
	Com_Printf("picmip: %i\n", r_picmip->integer);
	Com_Printf("texture bits: %i\n", r_textureBits->integer);
	Com_Printf("texture filter: %s\n", r_textureFilter->string);
	Com_Printf("multitexture: %s\n", (glConfig.multitexture) ? "enabled" : "disabled");
	Com_Printf("texture env add: %s\n", (glConfig.textureEnvAdd) ? "enabled" : "disabled");
	Com_Printf("texture env combine: %s\n", (glConfig.textureEnvCombine) ? "enabled" : "disabled");
	Com_Printf("texture env dot3: %s\n", (glConfig.textureEnvDot3) ? "enabled" : "disabled");
	Com_Printf("texture cube map: %s\n", (glConfig.textureCubeMap) ? "enabled" : "disabled");
	Com_Printf("texture compression: %s\n", (glConfig.textureCompression) ? "enabled" : "disabled");
	Com_Printf("vertex buffer object: %s\n", (glConfig.vertexBufferObject) ? "enabled" : "disabled");
	Com_Printf("vertex program: %s\n", (glConfig.vertexProgram) ? "enabled" : "disabled");
	Com_Printf("fragment program %s\n", (glConfig.fragmentProgram) ? "enabled" : "disabled");
	Com_Printf("draw range elements: %s\n", (glConfig.drawRangeElements) ? "enabled" : "disabled");
	Com_Printf("compiled vertex array: %s\n", (glConfig.compiledVertexArray) ? "enabled" : "disabled");
	Com_Printf("texture edge clamp: %s\n", (glConfig.textureEdgeClamp) ? "enabled" : "disabled");
	Com_Printf("texture filter anisotropic: %s\n", (glConfig.textureFilterAnisotropic) ? "enabled" : "disabled");
	Com_Printf("texture rectangle: %s\n", (glConfig.textureRectangle) ? "enabled" : "disabled");
	Com_Printf("stencil two side: %s\n", (glConfig.stencilTwoSide || glConfig.separateStencil) ? "enabled" : "disabled");
	Com_Printf("generate mipmap: %s\n", (glConfig.generateMipmap) ? "enabled" : "disabled");
	Com_Printf("swap control: %s\n", (glConfig.swapControl) ? "enabled" : "disabled");
	Com_Printf("\n");
}

/*
 =================
 R_BeginFrame
 =================
*/
void R_BeginFrame (int realTime, float stereoSeparation){

	// Set frame time
	r_frameTime = realTime * 0.001;

	// Clear r_speeds statistics
	memset(&r_stats, 0, sizeof(refStats_t));

	// Log file
	if (r_logFile->modified){
		QGL_EnableLogging(r_logFile->integer);

		r_logFile->modified = false;
	}

	if (r_logFile->integer)
		QGL_LogPrintf("\n======= R_BeginFrame =======\n\n");	

	// Update gamma
	if (r_gamma->modified){
		R_SetGammaRamp();

		r_gamma->modified = false;
	}

	// Update texture filtering
	if (r_textureFilter->modified || r_textureFilterAnisotropy->modified){
		R_TextureFilter();

		r_textureFilter->modified = false;
		r_textureFilterAnisotropy->modified = false;
	}

	// Set draw buffer
	glState.stereoSeparation = stereoSeparation;

	if (!glConfig.stereoEnabled){
		if (glState.stereoSeparation != 0)
			Com_Error(ERR_DROP, "R_BeginFrame: stereoSeparation != 0 with stereo disabled");

		if (r_frontBuffer->integer)
			qglDrawBuffer(GL_FRONT);
		else
			qglDrawBuffer(GL_BACK);
	}
	else {
		if (glState.stereoSeparation == 0)
			Com_Error(ERR_DROP, "R_BeginFrame: stereoSeparation == 0 with stereo enabled");

		if (r_frontBuffer->integer)
			qglDrawBuffer((glState.stereoSeparation < 0) ? GL_FRONT_LEFT : GL_FRONT_RIGHT);
		else
			qglDrawBuffer((glState.stereoSeparation < 0) ? GL_BACK_LEFT : GL_BACK_RIGHT);
	}

	// Clear screen if desired
	if (r_clear->integer){
		GL_DepthMask(GL_TRUE);

		qglClearColor(1.0, 0.0, 0.5, 0.5);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// Go into 2D mode
	GL_Setup2D();

	// Check for errors
	if (!r_ignoreGLErrors->integer)
		GL_CheckForErrors();
}

/*
 =================
 R_EndFrame
 =================
*/
void R_EndFrame (void){

	// Make sure all 2D stuff is flushed
	RB_RenderMesh();

	// Swap the buffers
	GLimp_SwapBuffers();

	// Print r_speeds statistics
	if (r_speeds->integer){
		switch (r_speeds->integer){
		case 1:
			Com_Printf("%i/%i shaders/stages %i meshes %i leafs %i verts %i/%i tris\n", r_stats.numShaders, r_stats.numStages, r_stats.numMeshes, r_stats.numLeafs, r_stats.numVertices, (r_stats.numIndices / 3), (r_stats.totalIndices / 3));
			break;
		case 2:
			Com_Printf("%i entities %i dlights %i particles %i polys\n", r_stats.numEntities, r_stats.numDLights, r_stats.numParticles, r_stats.numPolys);
			break;
		}
	}

	// Log file
	if (r_logFile->integer){
		QGL_LogPrintf("\n======= R_EndFrame =======\n\n");

		if (r_logFile->integer > 0)		// Negative is infinite logging
			Cvar_SetInteger("r_logFile", r_logFile->integer - 1);
	}

	// Check for errors
	if (!r_ignoreGLErrors->integer)
		GL_CheckForErrors();
}

/*
 =================
 R_Register
 =================
*/
static void R_Register (void){

	r_noRefresh = Cvar_Get("r_noRefresh", "0", CVAR_CHEAT);
	r_noVis = Cvar_Get("r_noVis", "0", CVAR_CHEAT);
	r_noCull = Cvar_Get("r_noCull", "0", CVAR_CHEAT);
	r_noBind = Cvar_Get("r_noBind", "0", CVAR_CHEAT);
	r_drawWorld = Cvar_Get("r_drawWorld", "1", CVAR_CHEAT);
	r_drawEntities = Cvar_Get("r_drawEntities", "1", CVAR_CHEAT);
	r_drawParticles = Cvar_Get("r_drawParticles", "1", CVAR_CHEAT);
	r_drawPolys = Cvar_Get("r_drawPolys", "1", CVAR_CHEAT);
	r_fullbright = Cvar_Get("r_fullbright", "0", CVAR_CHEAT | CVAR_LATCH);
	r_lightmap = Cvar_Get("r_lightmap", "0", CVAR_CHEAT | CVAR_LATCH);
	r_lockPVS = Cvar_Get("r_lockPVS", "0", CVAR_CHEAT);
	r_logFile = Cvar_Get("r_logFile", "0", CVAR_CHEAT);
	r_frontBuffer = Cvar_Get("r_frontBuffer", "0", CVAR_CHEAT);
	r_showCluster = Cvar_Get("r_showCluster", "0", CVAR_CHEAT);
	r_showTris = Cvar_Get("r_showTris", "0", CVAR_CHEAT);
	r_showNormals = Cvar_Get("r_showNormals", "0", CVAR_CHEAT);
	r_showTangentSpace = Cvar_Get("r_showTangentSpace", "0", CVAR_CHEAT);
	r_showModelBounds = Cvar_Get("r_showModelBounds", "0", CVAR_CHEAT);
	r_showShadowVolumes = Cvar_Get("r_showShadowVolumes", "0", CVAR_CHEAT);
	r_offsetFactor = Cvar_Get("r_offsetFactor", "-1", CVAR_CHEAT);
	r_offsetUnits = Cvar_Get("r_offsetUnits", "-2", CVAR_CHEAT);
	r_debugSort = Cvar_Get("r_debugSort", "0", CVAR_CHEAT);
	r_speeds = Cvar_Get("r_speeds", "0", CVAR_CHEAT);
	r_clear = Cvar_Get("r_clear", "0", CVAR_CHEAT);
	r_singleShader = Cvar_Get("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH);
	r_skipBackEnd = Cvar_Get("r_skipBackEnd", "0", CVAR_CHEAT);
	r_skipFrontEnd = Cvar_Get("r_skipFrontEnd", "0", CVAR_CHEAT);
	r_allowSoftwareGL = Cvar_Get("r_allowSoftwareGL", "0", CVAR_LATCH);
	r_maskMiniDriver = Cvar_Get("r_maskMiniDriver", "0", CVAR_LATCH);
	r_glDriver = Cvar_Get("r_glDriver", GL_DRIVER_OPENGL, CVAR_ARCHIVE | CVAR_LATCH);
	r_allowExtensions = Cvar_Get("r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_multitexture = Cvar_Get("r_arb_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_texture_env_add = Cvar_Get("r_arb_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_texture_env_combine = Cvar_Get("r_arb_texture_env_combine", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_texture_env_dot3 = Cvar_Get("r_arb_texture_env_dot3", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_texture_cube_map = Cvar_Get("r_arb_texture_cube_map", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_texture_compression = Cvar_Get("r_arb_texture_compression", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_vertex_buffer_object = Cvar_Get("r_arb_vertex_buffer_object", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_vertex_program = Cvar_Get("r_arb_vertex_program", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_fragment_program = Cvar_Get("r_arb_fragment_program", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_draw_range_elements = Cvar_Get("r_ext_draw_range_elements", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_compiled_vertex_array = Cvar_Get("r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_edge_clamp = Cvar_Get("r_ext_texture_edge_clamp", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_filter_anisotropic = Cvar_Get("r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_rectangle = Cvar_Get("r_ext_texture_rectangle", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_stencil_two_side = Cvar_Get("r_ext_stencil_two_side", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_generate_mipmap = Cvar_Get("r_ext_generate_mipmap", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_swap_control = Cvar_Get("r_ext_swap_control", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_swapInterval = Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE);
	r_finish = Cvar_Get("r_finish", "0", CVAR_ARCHIVE);
	r_stereo = Cvar_Get("r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_colorBits = Cvar_Get("r_colorBits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_depthBits = Cvar_Get("r_depthBits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_stencilBits = Cvar_Get("r_stencilBits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = Cvar_Get("r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH);
	r_fullscreen = Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_customWidth = Cvar_Get("r_customWidth", "1600", CVAR_ARCHIVE | CVAR_LATCH);
	r_customHeight = Cvar_Get("r_customHeight", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	r_displayRefresh = Cvar_Get("r_displayRefresh", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignoreHwGamma = Cvar_Get("r_ignoreHwGamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_gamma = Cvar_Get("r_gamma", "1.0", CVAR_ARCHIVE);
	r_overBrightBits = Cvar_Get("r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignoreGLErrors = Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE);
	r_shadows = Cvar_Get("r_shadows", "0", CVAR_ARCHIVE);
	r_caustics = Cvar_Get("r_caustics", "0", CVAR_ARCHIVE);
	r_lodBias = Cvar_Get("r_lodBias", "0", CVAR_ARCHIVE);
	r_lodDistance = Cvar_Get("r_lodDistance", "250.0", CVAR_ARCHIVE);
	r_dynamicLights = Cvar_Get("r_dynamicLights", "1", CVAR_ARCHIVE);
	r_modulate = Cvar_Get("r_modulate", "1.0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ambientScale = Cvar_Get("r_ambientScale", "0.6", CVAR_ARCHIVE);
	r_directedScale = Cvar_Get("r_directedScale", "1.0", CVAR_ARCHIVE);
	r_intensity = Cvar_Get("r_intensity", "1.0", CVAR_ARCHIVE | CVAR_LATCH);
	r_roundImagesDown = Cvar_Get("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_maxTextureSize = Cvar_Get("r_maxTextureSize", "512", CVAR_ARCHIVE | CVAR_LATCH);
	r_picmip = Cvar_Get("r_picmip", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_textureBits = Cvar_Get("r_textureBits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_textureFilter = Cvar_Get("r_textureFilter", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE);
	r_textureFilterAnisotropy = Cvar_Get("r_textureFilterAnisotropy", "2.0", CVAR_ARCHIVE);
	r_jpegCompressionQuality = Cvar_Get("r_jpegCompressionQuality", "75", CVAR_ARCHIVE);
	r_detailTextures = Cvar_Get("r_detailTextures", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_inGameVideo = Cvar_Get("r_inGameVideo", "0", CVAR_ARCHIVE);
	
	Cmd_AddCommand("screenshot", R_ScreenShot_f);
	Cmd_AddCommand("screenshotJPEG", R_ScreenShot_f);
	Cmd_AddCommand("envshot", R_EnvShot_f);
	Cmd_AddCommand("gfxinfo", R_GfxInfo_f);
	Cmd_AddCommand("vboinfo", RB_VBOInfo_f);
	Cmd_AddCommand("modelist", R_ModeList_f);
	Cmd_AddCommand("texturelist", R_TextureList_f);
	Cmd_AddCommand("programlist", R_ProgramList_f);
	Cmd_AddCommand("shaderlist", R_ShaderList_f);
	Cmd_AddCommand("modellist", R_ModelList_f);

	// Range check some cvars
	if (r_gamma->value > 3.0)
		Cvar_ForceSet("r_gamma", "3.0");
	else if (r_gamma->value < 0.5)
		Cvar_ForceSet("r_gamma", "0.5");

	if (r_overBrightBits->integer > 2)
		Cvar_ForceSet("r_overBrightBits", "2");
	else if (r_overBrightBits->integer < 0)
		Cvar_ForceSet("r_overBrightBits", "0");

	if (r_modulate->value < 1.0)
		Cvar_ForceSet("r_modulate", "1.0");

	if (r_intensity->value < 1.0)
		Cvar_ForceSet("r_intensity", "1.0");
}

/*
 =================
 R_Unregister
 =================
*/
static void R_Unregister (void){

	Cmd_RemoveCommand("screenshot");
	Cmd_RemoveCommand("screenshotJPEG");
	Cmd_RemoveCommand("envshot");
	Cmd_RemoveCommand("gfxinfo");
	Cmd_RemoveCommand("vboinfo");
	Cmd_RemoveCommand("modelist");
	Cmd_RemoveCommand("texturelist");
	Cmd_RemoveCommand("programlist");
	Cmd_RemoveCommand("shaderlist");
	Cmd_RemoveCommand("modellist");
}

/*
 =================
 R_Init
 =================
*/
void R_Init (qboolean all){

	Com_Printf("----- R_Init -----\n");

	if (all){
		R_Register();

		GLimp_Init();
	}

	RB_InitBackend();

	R_GfxInfo_f();

	R_InitTextures();
	R_InitPrograms();
	R_InitShaders();
	R_InitModels();

	if (!r_ignoreGLErrors->integer)
		GL_CheckForErrors();

	Com_Printf("----- finished R_Init -----\n");
}

/*
 =================
 R_Shutdown
 =================
*/
void R_Shutdown (qboolean all){

	Com_Printf("R_Shutdown( %i )\n", all);

	R_ShutdownModels();
	R_ShutdownShaders();
	R_ShutdownPrograms();
	R_ShutdownTextures();

	RB_ShutdownBackend();

	if (all){
		GLimp_Shutdown();

		R_Unregister();
	}

	Hunk_ClearToLowMark();
}
