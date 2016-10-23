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


#define NUM_VIDEO_MODES	(sizeof(r_videoModes) / sizeof(videoMode_t))

typedef struct {
	const char	*description;
	int			width, height;
	int			mode;
} videoMode_t;

static videoMode_t	r_videoModes[] = {
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

trGlobals_t			tr;

cvar_t	*r_logFile;
cvar_t	*r_clear;
cvar_t	*r_clearColor;
cvar_t	*r_frontBuffer;
cvar_t	*r_screenFraction;
cvar_t	*r_subviewOnly;
cvar_t	*r_lockPVS;
cvar_t	*r_zNear;
cvar_t	*r_zFar;
cvar_t	*r_offsetFactor;
cvar_t	*r_offsetUnits;
cvar_t	*r_shadowPolygonFactor;
cvar_t	*r_shadowPolygonOffset;
cvar_t	*r_colorMipLevels;
cvar_t	*r_singleMaterial;
cvar_t	*r_singleLight;
cvar_t	*r_showCluster;
cvar_t	*r_showCull;
cvar_t	*r_showSurfaces;
cvar_t	*r_showDynamic;
cvar_t	*r_showDeforms;
cvar_t	*r_s3tc;
cvar_t	*r_showPrimitives;
cvar_t	*r_showVertexBuffers;
cvar_t	*r_showTextureUsage;
cvar_t	*r_showRenderToTexture;
cvar_t	*r_showDepth;
cvar_t	*r_showOverdraw;
cvar_t	*r_showTris;
cvar_t	*r_showNormals;
cvar_t	*r_showTextureVectors;
cvar_t	*r_showTangentSpace;
cvar_t	*r_showVertexColors;
cvar_t	*r_showModelBounds;
cvar_t	*r_showInteractions;
cvar_t	*r_showShadows;
cvar_t	*r_showShadowCount;
cvar_t	*r_showShadowTris;
cvar_t	*r_showShadowVolumes;
cvar_t	*r_showShadowSilhouettes;
cvar_t	*r_showLights;
cvar_t	*r_showLightCount;
cvar_t	*r_showLightBounds;
cvar_t	*r_showLightScissors;
cvar_t	*r_showRenderTime;
cvar_t	*r_skipVisibility;
cvar_t	*r_skipAreas;
cvar_t	*r_skipSuppress;
cvar_t	*r_skipCulling;
cvar_t	*r_skipInteractionCulling;
cvar_t	*r_skipShadowCulling;
cvar_t	*r_skipLightCulling;
cvar_t	*r_skipBatching;
cvar_t	*r_skipDecorations;
cvar_t	*r_skipEntities;
cvar_t	*r_skipDecals;
cvar_t	*r_skipPolys;
cvar_t	*r_skipLights;
cvar_t	*r_skipExpressions;
cvar_t	*r_skipConstantExpressions;
cvar_t	*r_skipDeforms;
cvar_t	*r_skipPrograms;
cvar_t	*r_skipAmbient;
cvar_t	*r_skipBump;
cvar_t	*r_skipDiffuse;
cvar_t	*r_skipSpecular;
cvar_t	*r_skipInteractions;
cvar_t	*r_skipFogLights;
cvar_t	*r_skipBlendLights;
cvar_t	*r_skipTranslucent;
cvar_t	*r_skipPostProcess;
cvar_t	*r_skipScissors;
cvar_t	*r_skipDepthBounds;
cvar_t	*r_skipSubviews;
cvar_t	*r_skipVideos;
cvar_t	*r_skipCopyTexture;
cvar_t	*r_skipDynamicTextures;
cvar_t	*r_skipRender;
cvar_t	*r_skipRenderContext;
cvar_t	*r_skipFrontEnd;
cvar_t	*r_skipBackEnd;
cvar_t	*r_glDriver;
cvar_t	*r_ignoreGLErrors;
cvar_t	*r_multiSamples;
cvar_t	*r_swapInterval;
cvar_t	*r_mode;
cvar_t	*r_fullscreen;
cvar_t	*r_customWidth;
cvar_t	*r_customHeight;
cvar_t	*r_displayRefresh;
cvar_t	*r_gamma;
cvar_t	*r_brightness;
cvar_t	*r_finish;
cvar_t	*r_renderer;
cvar_t	*r_vertexBuffers;
cvar_t	*r_shadows;
cvar_t	*r_playerShadow;
cvar_t	*r_dynamicLights;
cvar_t	*r_fastInteractions;
cvar_t	*r_caustics;
cvar_t	*r_lightScale;
cvar_t	*r_lightDetailLevel;
cvar_t	*r_shaderPrograms;
cvar_t	*r_roundTexturesDown;
cvar_t	*r_downSizeTextures;
cvar_t	*r_downSizeNormalTextures;
cvar_t	*r_maxTextureSize;
cvar_t	*r_maxNormalTextureSize;
cvar_t	*r_compressTextures;
cvar_t	*r_compressNormalTextures;
cvar_t	*r_textureFilter;
cvar_t	*r_textureAnisotropy;
cvar_t	*r_textureLodBias;


/*
 =================
 R_CullBox

 Returns true if the box is completely outside the frustum
 =================
*/
qboolean R_CullBox (const vec3_t mins, const vec3_t maxs, int clipFlags){

	cplane_t	*plane;
	int			i;

	if (r_skipCulling->integerValue)
		return false;

	for (i = 0, plane = tr.renderViewParms.frustum; i < 5; i++, plane++){
		if (!(clipFlags & (1<<i)))
			continue;

		switch (plane->signbits){
		case 0:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*maxs[1] + plane->normal[2]*maxs[2] < plane->dist){
				tr.pc.boxOut++;
				return true;
			}

			break;
		case 1:
			if (plane->normal[0]*mins[0] + plane->normal[1]*maxs[1] + plane->normal[2]*maxs[2] < plane->dist){
				tr.pc.boxOut++;
				return true;
			}

			break;
		case 2:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*mins[1] + plane->normal[2]*maxs[2] < plane->dist){
				tr.pc.boxOut++;
				return true;
			}

			break;
		case 3:
			if (plane->normal[0]*mins[0] + plane->normal[1]*mins[1] + plane->normal[2]*maxs[2] < plane->dist){
				tr.pc.boxOut++;
				return true;
			}

			break;
		case 4:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*maxs[1] + plane->normal[2]*mins[2] < plane->dist){
				tr.pc.boxOut++;
				return true;
			}

			break;
		case 5:
			if (plane->normal[0]*mins[0] + plane->normal[1]*maxs[1] + plane->normal[2]*mins[2] < plane->dist){
				tr.pc.boxOut++;
				return true;
			}

			break;
		case 6:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*mins[1] + plane->normal[2]*mins[2] < plane->dist){
				tr.pc.boxOut++;
				return true;
			}

			break;
		case 7:
			if (plane->normal[0]*mins[0] + plane->normal[1]*mins[1] + plane->normal[2]*mins[2] < plane->dist){
				tr.pc.boxOut++;
				return true;
			}

			break;
		default:
			tr.pc.boxIn++;
			return false;
		}
	}

	tr.pc.boxIn++;

	return false;
}

/*
 =================
 R_CullSphere

 Returns true if the sphere is completely outside the frustum
 =================
*/
qboolean R_CullSphere (const vec3_t center, float radius, int clipFlags){

	cplane_t	*plane;
	int			i;

	if (r_skipCulling->integerValue)
		return false;

	for (i = 0, plane = tr.renderViewParms.frustum; i < 5; i++, plane++){
		if (!(clipFlags & (1<<i)))
			continue;

		if (DotProduct(center, plane->normal) - plane->dist <= -radius){
			tr.pc.sphereOut++;
			return true;
		}
	}

	tr.pc.sphereIn++;

	return false;
}


// =====================================================================


/*
 =================
 R_AddSprite
 =================
*/
void R_AddSprite (renderEntity_t *entity){

	vec3_t	vec;

	if (!entity->customMaterial){
		Com_DPrintf(S_COLOR_YELLOW "R_AddSprite: NULL customMaterial\n");

		entity->customMaterial = tr.defaultMaterial;
	}

	if (!entity->customMaterial->numStages)
		return;

	// Cull
	if (!r_skipCulling->integerValue){
		VectorSubtract(entity->origin, tr.renderView.viewOrigin, vec);
		if (DotProduct(vec, tr.renderView.viewAxis[0]) < 0)
			return;
	}

	tr.pc.entities++;

	// Add it
	R_AddMeshToList(MESH_SPRITE, NULL, entity->customMaterial, entity);
}

/*
 =================
 R_AddBeam
 =================
*/
void R_AddBeam (renderEntity_t *entity){

	if (!entity->customMaterial){
		Com_DPrintf(S_COLOR_YELLOW "R_AddBeam: NULL customMaterial\n");

		entity->customMaterial = tr.defaultMaterial;
	}

	if (!entity->customMaterial->numStages)
		return;

	tr.pc.entities++;

	// Add it
	R_AddMeshToList(MESH_BEAM, NULL, entity->customMaterial, entity);
}

/*
 =================
 R_AddEntities
 =================
*/
void R_AddEntities (void){

	renderEntity_t	*entity;
	model_t			*model;
	int				i;

	if (r_skipEntities->integerValue)
		return;

	for (i = 0, entity = tr.renderViewParms.renderEntities; i < tr.renderViewParms.numRenderEntities; i++, entity++){
		switch (entity->reType){
		case RE_MODEL:
			model = entity->model;

			if (!model || model->modelType == MODEL_BAD){
				Com_DPrintf(S_COLOR_YELLOW "R_AddEntities: entity with no model at %i, %i, %i\n", (int)entity->origin[0], (int)entity->origin[1], (int)entity->origin[2]);
				break;
			}

			switch (model->modelType){
			case MODEL_BSP:
				R_AddBrushModel(entity);
				break;
			case MODEL_MD3:
			case MODEL_MD2:
				R_AddAliasModel(entity);
				break;
			default:
				Com_Error(ERR_DROP, "R_AddEntities: bad modelType (%i)", model->modelType);
			}

			break;
		case RE_SPRITE:
			R_AddSprite(entity);
			break;
		case RE_BEAM:
			R_AddBeam(entity);
			break;
		default:
			Com_Error(ERR_DROP, "R_AddEntities: bad reType (%i)", entity->reType);
		}
	}
}

/*
 =================
 R_AddPolys
 =================
*/
void R_AddPolys (void){

	renderPoly_t	*poly;
	int				i;

	if (r_skipPolys->integerValue)
		return;

	for (i = 0, poly = tr.renderViewParms.renderPolys; i < tr.renderViewParms.numRenderPolys; i++, poly++){
		if (!poly->material->numStages)
			continue;

		tr.pc.polys++;

		// Add it
		R_AddMeshToList(MESH_POLY, poly, poly->material, tr.worldEntity);
	}
}


// =====================================================================


/*
 =================
 R_AddSubviewSurface
 =================
*/
void R_AddSubviewSurface (material_t *material, renderEntity_t *entity, surface_t *surface){

	vec3_t	invAxis[3], normal;

	if (tr.renderViewParms.subview != SUBVIEW_NONE)
		return;

	// If we already have something, do some sanity checks
	if (tr.renderSubviewParms.material && tr.renderSubviewParms.entity){
		if (tr.renderSubviewParms.material != material){
			Com_DPrintf(S_COLOR_YELLOW "R_AddSubviewSurface: multiple subview surfaces with mismatched material\n");
			return;
		}

		if (tr.renderSubviewParms.entity != entity){
			Com_DPrintf(S_COLOR_YELLOW "R_AddSubviewSurface: multiple subview surfaces with mismatched entity\n");
			return;
		}

		// Special case for mirrors
		if (material->subview == SUBVIEW_MIRROR){
			if (!surface){
				Com_DPrintf(S_COLOR_YELLOW "R_AddSubviewSurface: NULL surface for subview\n");
				return;
			}

			if (tr.renderSubviewParms.surfaces[0]->plane != surface->plane){
				Com_DPrintf(S_COLOR_YELLOW "R_AddSubviewSurface: multiple subview surfaces with mismatched plane\n");
				return;
			}

			// Add the surface for visibility hacking
			if (tr.renderSubviewParms.numSurfaces == MAX_MIRROR_SURFACES){
				Com_DPrintf(S_COLOR_YELLOW "R_AddSubviewSurface: MAX_MIRROR_SURFACES hit\n");
				return;
			}

			tr.renderSubviewParms.surfaces[tr.renderSubviewParms.numSurfaces++] = surface;
		}

		return;
	}

	// Otherwise copy it
	switch (material->subview){
	case SUBVIEW_MIRROR:
		if (!surface){
			Com_DPrintf(S_COLOR_YELLOW "R_AddSubviewSurface: NULL surface for subview\n");
			return;
		}

		tr.renderSubviewParms.material = material;
		tr.renderSubviewParms.entity = entity;

		// Copy the surface
		tr.renderSubviewParms.surfaces[tr.renderSubviewParms.numSurfaces++] = surface;

		// Compute the mirror plane
		if (entity == tr.worldEntity){
			VectorCopy(surface->origin, tr.renderSubviewParms.planeOrigin);

			if (!(surface->flags & SURF_PLANEBACK))
				VectorCopy(surface->plane->normal, tr.renderSubviewParms.planeNormal);
			else
				VectorNegate(surface->plane->normal, tr.renderSubviewParms.planeNormal);

			tr.renderSubviewParms.planeDist = DotProduct(tr.renderSubviewParms.planeOrigin, tr.renderSubviewParms.planeNormal);
		}
		else {
			AxisTranspose(entity->axis, invAxis);

			if (!(surface->flags & SURF_PLANEBACK))
				VectorCopy(surface->plane->normal, normal);
			else
				VectorNegate(surface->plane->normal, normal);

			VectorRotate(surface->origin, invAxis, tr.renderSubviewParms.planeOrigin);
			VectorAdd(tr.renderSubviewParms.planeOrigin, entity->origin, tr.renderSubviewParms.planeOrigin);

			VectorRotate(normal, invAxis, tr.renderSubviewParms.planeNormal);
			VectorNormalize(tr.renderSubviewParms.planeNormal);

			tr.renderSubviewParms.planeDist = DotProduct(tr.renderSubviewParms.planeOrigin, tr.renderSubviewParms.planeNormal);
		}

		break;
	case SUBVIEW_REMOTE:
		if (!entity->remoteView){
			Com_DPrintf(S_COLOR_YELLOW "R_AddSubviewSurface: NULL remoteView for subview\n");
			return;
		}

		tr.renderSubviewParms.material = material;
		tr.renderSubviewParms.entity = entity;

		// Copy the remoteView
		tr.renderSubviewParms.remoteView = entity->remoteView;

		break;
	default:
		Com_Error(ERR_DROP, "R_AddSubviewSurface: bad subview (%i)", material->subview);
	}
}

/*
 =================
 R_SetGamma
 =================
*/
static void R_SetGamma (void){

	float	invGamma, div;
	int		i, v;

	if (r_gamma->floatValue > 3.0)
		Cvar_SetFloat("r_gamma", 3.0);
	else if (r_gamma->floatValue < 0.5)
		Cvar_SetFloat("r_gamma", 0.5);

	r_gamma->modified = false;

	if (r_brightness->floatValue > 2.0)
		Cvar_SetFloat("r_brightness", 2.0);
	else if (r_brightness->floatValue < 0.5)
		Cvar_SetFloat("r_brightness", 0.5);

	r_brightness->modified = false;

	// Build gamma table
	invGamma = 1.0 / r_gamma->floatValue;
	div = r_brightness->floatValue / 255.0;

	for (i = 0; i < 256; i++){
		v = 255 * pow((float)i * div, invGamma) + 0.5;

		tr.gammaTable[i] = Clamp(v, 0, 255);
	}

	// Set device gamma ramp
	GLimp_SetDeviceGammaRamp(tr.gammaTable);
}

/*
 =================
 R_ClearRender

 Called during initialization
 =================
*/
static void R_ClearRender (void){

	int		i;

	memset(&tr, 0, sizeof(trGlobals_t));

	// Clear counters
	tr.frameCount = 1;
	tr.viewCount = 1;
	tr.visCount = 1;
	tr.worldCount = 1;
	tr.lightCount = 1;
	tr.fragmentCount = 1;

	// Clear view clusters
	tr.viewCluster = tr.oldViewCluster = -1;
	tr.viewCluster2 = tr.oldViewCluster2 = -1;

	// Clear light styles
	for (i = 0; i < MAX_LIGHTSTYLES; i++){
		tr.lightStyles[i].rgb[0] = 1.0;
		tr.lightStyles[i].rgb[1] = 1.0;
		tr.lightStyles[i].rgb[2] = 1.0;
	}

	// Clear decals
	R_ClearDecals();
}

/*
 =================
 R_ResetRender

 Called for every frame
 =================
*/
void R_ResetRender (void){

	int		width, height;

	// Clear mesh lists
	tr.viewData.numMeshes = tr.viewData.firstMesh = 0;
	tr.viewData.numPostProcessMeshes = tr.viewData.firstPostProcessMesh = 0;

	// Clear light lists
	tr.viewData.numLights = tr.viewData.firstLight = 0;
	tr.viewData.numFogLights = tr.viewData.firstFogLight = 0;

	// Clear scene render lists
	tr.scene.numEntities = tr.scene.firstEntity = 1;
	tr.scene.numLights = tr.scene.firstLight = 0;
	tr.scene.numPolys = tr.scene.firstPoly = 0;

	// Clear render crops
	tr.numRenderCrops = 1;

	// Set up the current render crop
	tr.currentRenderCrop = &tr.renderCrops[tr.numRenderCrops-1];

	if (r_screenFraction->floatValue > 0.0 && r_screenFraction->floatValue < 1.0){
		width = glConfig.videoWidth * r_screenFraction->floatValue;
		height = glConfig.videoHeight * r_screenFraction->floatValue;
	}
	else {
		width = glConfig.videoWidth;
		height = glConfig.videoHeight;
	}

	tr.currentRenderCrop->width = width;
	tr.currentRenderCrop->height = height;

	tr.currentRenderCrop->scaleX = width / 640.0;
	tr.currentRenderCrop->scaleY = height / 480.0;

	// Clear primary view
	tr.primaryViewAvailable = false;
}

/*
 =================
 R_SetupFrustum
 =================
*/
static void R_SetupFrustum (void){

	int		i;

	if (r_zNear->floatValue < 0.1)
		Cvar_ForceSet("r_zNear", "3.0");
	if (r_zFar->floatValue != 0.0 && r_zFar->floatValue <= r_zNear->floatValue)
		Cvar_ForceSet("r_zFar", "0.0");

	if (tr.renderViewParms.subview != SUBVIEW_MIRROR){
		VectorCopy(tr.renderView.viewAxis[0], tr.renderViewParms.frustum[0].normal);
		tr.renderViewParms.frustum[0].dist = DotProduct(tr.renderView.viewOrigin, tr.renderViewParms.frustum[0].normal) + r_zNear->floatValue;
	}
	else {
		VectorCopy(tr.renderSubviewParms.planeNormal, tr.renderViewParms.frustum[0].normal);
		tr.renderViewParms.frustum[0].dist = tr.renderSubviewParms.planeDist;
	}

	RotatePointAroundVector(tr.renderViewParms.frustum[1].normal, tr.renderView.viewAxis[2], tr.renderView.viewAxis[0], -(90 - tr.renderView.fovX / 2));
	tr.renderViewParms.frustum[1].dist = DotProduct(tr.renderView.viewOrigin, tr.renderViewParms.frustum[1].normal);

	RotatePointAroundVector(tr.renderViewParms.frustum[2].normal, tr.renderView.viewAxis[2], tr.renderView.viewAxis[0], 90 - tr.renderView.fovX / 2);
	tr.renderViewParms.frustum[2].dist = DotProduct(tr.renderView.viewOrigin, tr.renderViewParms.frustum[2].normal);

	RotatePointAroundVector(tr.renderViewParms.frustum[3].normal, tr.renderView.viewAxis[1], tr.renderView.viewAxis[0], 90 - tr.renderView.fovY / 2);
	tr.renderViewParms.frustum[3].dist = DotProduct(tr.renderView.viewOrigin, tr.renderViewParms.frustum[3].normal);

	RotatePointAroundVector(tr.renderViewParms.frustum[4].normal, tr.renderView.viewAxis[1], tr.renderView.viewAxis[0], -(90 - tr.renderView.fovY / 2));
	tr.renderViewParms.frustum[4].dist = DotProduct(tr.renderView.viewOrigin, tr.renderViewParms.frustum[4].normal);

	for (i = 0; i < 5; i++){
		tr.renderViewParms.frustum[i].type = PLANE_NON_AXIAL;
		SetPlaneSignbits(&tr.renderViewParms.frustum[i]);
	}
}

/*
 =================
 R_SetupMatrices
 =================
*/
static void R_SetupMatrices (void){

	mat4_t	transformMatrix, entityMatrix, mvpMatrix;
	vec4_t	clipPlane, corner;
	vec3_t	planeOrigin, planeNormal;
	float	xMin, yMin, xMax, yMax;
	float	xDiv, yDiv, zDiv;
	float	scale;

	// Compute perspective projection matrix
	xMax = r_zNear->floatValue * tan(tr.renderView.fovX * M_PI / 360.0);
	xMin = -xMax;

	yMax = r_zNear->floatValue * tan(tr.renderView.fovY * M_PI / 360.0);
	yMin = -yMax;

	xDiv = 1.0 / (xMax - xMin);
	yDiv = 1.0 / (yMax - yMin);
	zDiv = 1.0 / (r_zFar->floatValue - r_zNear->floatValue);

	tr.renderViewParms.perspectiveMatrix[ 0] = 2.0 * r_zNear->floatValue * xDiv;
	tr.renderViewParms.perspectiveMatrix[ 1] = 0.0;
	tr.renderViewParms.perspectiveMatrix[ 2] = 0.0;
	tr.renderViewParms.perspectiveMatrix[ 3] = 0.0;
	tr.renderViewParms.perspectiveMatrix[ 4] = 0.0;
	tr.renderViewParms.perspectiveMatrix[ 5] = 2.0 * r_zNear->floatValue * yDiv;
	tr.renderViewParms.perspectiveMatrix[ 6] = 0.0;
	tr.renderViewParms.perspectiveMatrix[ 7] = 0.0;
	tr.renderViewParms.perspectiveMatrix[ 8] = (xMax + xMin) * xDiv;
	tr.renderViewParms.perspectiveMatrix[ 9] = (yMax + yMin) * yDiv;
	tr.renderViewParms.perspectiveMatrix[10] = -0.9;
	tr.renderViewParms.perspectiveMatrix[11] = -1.0;
	tr.renderViewParms.perspectiveMatrix[12] = 0.0;
	tr.renderViewParms.perspectiveMatrix[13] = 0.0;
	tr.renderViewParms.perspectiveMatrix[14] = -2.0 * r_zNear->floatValue * 0.9;
	tr.renderViewParms.perspectiveMatrix[15] = 0.0;

	if (r_zFar->floatValue > r_zNear->floatValue){
		tr.renderViewParms.perspectiveMatrix[10] = -(r_zNear->floatValue + r_zFar->floatValue) * zDiv;
		tr.renderViewParms.perspectiveMatrix[14] = -2.0 * r_zNear->floatValue * r_zFar->floatValue * zDiv;
	}

	// Compute world modelview matrix
	tr.renderViewParms.worldMatrix[ 0] = -tr.renderView.viewAxis[1][0];
	tr.renderViewParms.worldMatrix[ 1] = tr.renderView.viewAxis[2][0];
	tr.renderViewParms.worldMatrix[ 2] = -tr.renderView.viewAxis[0][0];
	tr.renderViewParms.worldMatrix[ 3] = 0.0;
	tr.renderViewParms.worldMatrix[ 4] = -tr.renderView.viewAxis[1][1];
	tr.renderViewParms.worldMatrix[ 5] = tr.renderView.viewAxis[2][1];
	tr.renderViewParms.worldMatrix[ 6] = -tr.renderView.viewAxis[0][1];
	tr.renderViewParms.worldMatrix[ 7] = 0.0;
	tr.renderViewParms.worldMatrix[ 8] = -tr.renderView.viewAxis[1][2];
	tr.renderViewParms.worldMatrix[ 9] = tr.renderView.viewAxis[2][2];
	tr.renderViewParms.worldMatrix[10] = -tr.renderView.viewAxis[0][2];
	tr.renderViewParms.worldMatrix[11] = 0.0;
	tr.renderViewParms.worldMatrix[12] = DotProduct(tr.renderView.viewOrigin, tr.renderView.viewAxis[1]);
	tr.renderViewParms.worldMatrix[13] = -DotProduct(tr.renderView.viewOrigin, tr.renderView.viewAxis[2]);
	tr.renderViewParms.worldMatrix[14] = DotProduct(tr.renderView.viewOrigin, tr.renderView.viewAxis[0]);
	tr.renderViewParms.worldMatrix[15] = 1.0;

	// Compute sky box texture matrix
	tr.renderViewParms.skyBoxMatrix[ 0] = 1.0;
	tr.renderViewParms.skyBoxMatrix[ 1] = 0.0;
	tr.renderViewParms.skyBoxMatrix[ 2] = 0.0;
	tr.renderViewParms.skyBoxMatrix[ 3] = -tr.renderView.viewOrigin[0];
	tr.renderViewParms.skyBoxMatrix[ 4] = 0.0;
	tr.renderViewParms.skyBoxMatrix[ 5] = 1.0;
	tr.renderViewParms.skyBoxMatrix[ 6] = 0.0;
	tr.renderViewParms.skyBoxMatrix[ 7] = -tr.renderView.viewOrigin[1];
	tr.renderViewParms.skyBoxMatrix[ 8] = 0.0;
	tr.renderViewParms.skyBoxMatrix[ 9] = 0.0;
	tr.renderViewParms.skyBoxMatrix[10] = 1.0;
	tr.renderViewParms.skyBoxMatrix[11] = -tr.renderView.viewOrigin[2];
	tr.renderViewParms.skyBoxMatrix[12] = 0.0;
	tr.renderViewParms.skyBoxMatrix[13] = 0.0;
	tr.renderViewParms.skyBoxMatrix[14] = 0.0;
	tr.renderViewParms.skyBoxMatrix[15] = 1.0;

	if (tr.worldModel && tr.worldModel->sky->rotate)
		Matrix4_Rotate(tr.renderViewParms.skyBoxMatrix, tr.renderView.time * tr.worldModel->sky->rotate, tr.worldModel->sky->axis[0], tr.worldModel->sky->axis[1], tr.worldModel->sky->axis[2]);

	// Modify the perspective projection matrix if needed
	if (tr.renderViewParms.subview != SUBVIEW_MIRROR)
		return;

	// Transform the plane origin and normal to eye space
	Matrix4_TransformVector(tr.renderViewParms.worldMatrix, tr.renderSubviewParms.planeOrigin, planeOrigin);
	Matrix4_TransformNormal(tr.renderViewParms.worldMatrix, tr.renderSubviewParms.planeNormal, planeNormal);

	// Set the clipping plane
	VectorNormalize(planeNormal);

	clipPlane[0] = planeNormal[0];
	clipPlane[1] = planeNormal[1];
	clipPlane[2] = planeNormal[2];
	clipPlane[3] = -DotProduct(planeOrigin, planeNormal);

	// Calculate the clip space corner opposite the clipping plane and
	// transform it into eye space
	corner[0] = sgn(clipPlane[0] + tr.renderViewParms.perspectiveMatrix[ 8]) / tr.renderViewParms.perspectiveMatrix[ 0];
	corner[1] = sgn(clipPlane[1] + tr.renderViewParms.perspectiveMatrix[ 9]) / tr.renderViewParms.perspectiveMatrix[ 5];
	corner[2] = -1.0;
	corner[3] = (1.0 + tr.renderViewParms.perspectiveMatrix[10]) / tr.renderViewParms.perspectiveMatrix[14];

	// Calculate the scaled clipping plane
	corner[0] *= clipPlane[0];
	corner[1] *= clipPlane[1];
	corner[2] *= clipPlane[2];
	corner[3] *= clipPlane[3];

	scale = 2.0 / (corner[0] + corner[1] + corner[2] + corner[3]);

	clipPlane[0] *= scale;
	clipPlane[1] *= scale;
	clipPlane[2] *= scale;
	clipPlane[3] *= scale;

	// Mirror the X axis and set the oblique near plane
	tr.renderViewParms.perspectiveMatrix[ 0] = -tr.renderViewParms.perspectiveMatrix[ 0];
	tr.renderViewParms.perspectiveMatrix[ 2] = clipPlane[0];
	tr.renderViewParms.perspectiveMatrix[ 6] = clipPlane[1];
	tr.renderViewParms.perspectiveMatrix[10] = clipPlane[2] + 1.0;
	tr.renderViewParms.perspectiveMatrix[14] = clipPlane[3];

	// Compute mirror texture matrix if needed
	if (tr.renderSubviewParms.material->subviewWidth && tr.renderSubviewParms.material->subviewHeight){
		if (tr.renderSubviewParms.entity == tr.worldEntity)
			Matrix4_Multiply(tr.renderViewParms.perspectiveMatrix, tr.renderViewParms.worldMatrix, mvpMatrix);
		else {
			Matrix4_Set(transformMatrix, tr.renderSubviewParms.entity->axis, tr.renderSubviewParms.entity->origin);
			Matrix4_MultiplyFast(tr.renderViewParms.worldMatrix, transformMatrix, entityMatrix);

			Matrix4_Multiply(tr.renderViewParms.perspectiveMatrix, entityMatrix, mvpMatrix);
		}

		Matrix4_Transpose(mvpMatrix, tr.renderViewParms.mirrorMatrix);
	}
}

/*
 =================
 R_AddRenderViewCommand
 =================
*/
static void R_AddRenderViewCommand (void){

	renderViewCommand_t	*cmd;

	cmd = RB_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
		return;

	cmd->commandId = RC_RENDER_VIEW;

	cmd->time = tr.renderView.time;
	memcpy(cmd->materialParms, tr.renderView.materialParms, MAX_GLOBAL_MATERIAL_PARMS * sizeof(float));

	// Viewport
	cmd->viewport.x = tr.renderView.x;
	cmd->viewport.y = tr.renderView.y;
	cmd->viewport.width = tr.renderView.width;
	cmd->viewport.height = tr.renderView.height;

	// View parms
	cmd->viewParms.primaryView = tr.renderViewParms.primaryView;

	cmd->viewParms.subview = tr.renderViewParms.subview;

	VectorCopy(tr.renderView.viewOrigin, cmd->viewParms.origin);
	AxisCopy(tr.renderView.viewAxis, cmd->viewParms.axis);

	Matrix4_Copy(tr.renderViewParms.perspectiveMatrix, cmd->viewParms.perspectiveMatrix);
	Matrix4_Copy(tr.renderViewParms.worldMatrix, cmd->viewParms.worldMatrix);
	Matrix4_Copy(tr.renderViewParms.skyBoxMatrix, cmd->viewParms.skyBoxMatrix);
	Matrix4_Copy(tr.renderViewParms.mirrorMatrix, cmd->viewParms.mirrorMatrix);

	// Mesh lists
	cmd->meshes = tr.renderViewParms.meshes;
	cmd->numMeshes = tr.renderViewParms.numMeshes;

	cmd->postProcessMeshes = tr.renderViewParms.postProcessMeshes;
	cmd->numPostProcessMeshes = tr.renderViewParms.numPostProcessMeshes;

	// Light lists
	cmd->lights = tr.renderViewParms.lights;
	cmd->numLights = tr.renderViewParms.numLights;

	cmd->fogLights = tr.renderViewParms.fogLights;
	cmd->numFogLights = tr.renderViewParms.numFogLights;
}

/*
 =================
 R_RenderSubview
 =================
*/
static qboolean R_RenderSubview (void){

	renderViewParms_t	renderViewParms;
	renderView_t		renderView;
	vec3_t				angles, axis[3];
	float				d;

	if (r_skipSubviews->integerValue)
		return false;

	if (tr.renderViewParms.subview != SUBVIEW_NONE || (!tr.renderSubviewParms.material || !tr.renderSubviewParms.entity))
		return false;

	// Backup renderViewParms
	renderViewParms = tr.renderViewParms;

	// Backup renderView
	renderView = tr.renderView;

	// Switch to subview
	tr.renderViewParms.subview = tr.renderSubviewParms.material->subview;

	// Check whether it is a mirror or a remote view
	switch (tr.renderViewParms.subview){
	case SUBVIEW_MIRROR:
		// Set viewport
		if (tr.renderSubviewParms.material->subviewWidth && tr.renderSubviewParms.material->subviewHeight){
			tr.renderView.x = 0;
			tr.renderView.y = 0;
			tr.renderView.width = 640;
			tr.renderView.height = 480;
		}
		else {
			tr.renderView.x /= tr.currentRenderCrop->scaleX;
			tr.renderView.y /= tr.currentRenderCrop->scaleY;
			tr.renderView.width /= tr.currentRenderCrop->scaleX;
			tr.renderView.height /= tr.currentRenderCrop->scaleY;
		}

		// Compute mirror view origin and axis
		d = 2.0 * (DotProduct(renderView.viewOrigin, tr.renderSubviewParms.planeNormal) - tr.renderSubviewParms.planeDist);
		VectorMA(renderView.viewOrigin, -d, tr.renderSubviewParms.planeNormal, tr.renderView.viewOrigin);

		VectorReflect(renderView.viewAxis[0], tr.renderSubviewParms.planeNormal, axis[0]);
		VectorReflect(renderView.viewAxis[1], tr.renderSubviewParms.planeNormal, axis[1]);
		VectorReflect(renderView.viewAxis[2], tr.renderSubviewParms.planeNormal, axis[2]);

		VectorNormalize(axis[0]);
		VectorNormalize(axis[1]);
		VectorNormalize(axis[2]);

		EulerAngles(axis, angles);
		AnglesToAxis(angles, tr.renderView.viewAxis);

		// Render the subview
		if (tr.renderSubviewParms.material->subviewWidth && tr.renderSubviewParms.material->subviewHeight){
			R_CropRender(tr.renderSubviewParms.material->subviewWidth, tr.renderSubviewParms.material->subviewHeight, true);
			R_RenderView();
			R_CaptureRenderToTexture("_mirrorRender");
			R_UnCropRender();
		}
		else
			R_RenderView();

		break;
	case SUBVIEW_REMOTE:
		// Copy the remote renderView
		tr.renderView = *tr.renderSubviewParms.remoteView;

		// Render the subview
		if (tr.renderSubviewParms.material->subviewWidth && tr.renderSubviewParms.material->subviewHeight){
			R_CropRender(tr.renderSubviewParms.material->subviewWidth, tr.renderSubviewParms.material->subviewHeight, true);
			R_RenderView();
			R_CaptureRenderToTexture("_remoteRender");
			R_UnCropRender();
		}
		else
			R_RenderView();

		break;
	}

	// Copy the mirror texture matrix
	Matrix4_Copy(tr.renderViewParms.mirrorMatrix, renderViewParms.mirrorMatrix);

	// Go back to main view
	memset(&tr.renderSubviewParms, 0, sizeof(renderSubviewParms_t));

	// Restore renderView
	tr.renderView = renderView;

	// Restore renderViewParms
	tr.renderViewParms = renderViewParms;

	return true;
}

/*
 =================
 R_RenderView
 =================
*/
void R_RenderView (void){

	// Bump view count
	tr.viewCount++;

	// Adjust viewport
	tr.renderView.x *= tr.currentRenderCrop->scaleX;
	tr.renderView.y *= tr.currentRenderCrop->scaleY;

	tr.renderView.width *= tr.currentRenderCrop->scaleX;
	tr.renderView.width &= ~1;

	tr.renderView.height *= tr.currentRenderCrop->scaleY;
	tr.renderView.height &= ~1;

	// Set up frustum and matrices
	R_SetupFrustum();
	R_SetupMatrices();

	// Add meshes and lights
	R_AddWorld();
	R_AddEntities();
	R_AddDecals();
	R_AddPolys();
	R_AddLights();

	// Sort meshes
	R_SortMeshes();

	// Set up renderViewParms
	tr.renderViewParms.meshes = &tr.viewData.meshes[tr.viewData.firstMesh];
	tr.renderViewParms.numMeshes = tr.viewData.numMeshes - tr.viewData.firstMesh;

	tr.renderViewParms.postProcessMeshes = &tr.viewData.postProcessMeshes[tr.viewData.firstPostProcessMesh];
	tr.renderViewParms.numPostProcessMeshes = tr.viewData.numPostProcessMeshes - tr.viewData.firstPostProcessMesh;

	tr.renderViewParms.lights = &tr.viewData.lights[tr.viewData.firstLight];
	tr.renderViewParms.numLights = tr.viewData.numLights - tr.viewData.firstLight;

	tr.renderViewParms.fogLights = &tr.viewData.fogLights[tr.viewData.firstFogLight];
	tr.renderViewParms.numFogLights = tr.viewData.numFogLights - tr.viewData.firstFogLight;

	// The next view rendered in this frame will tack on after this one
	tr.viewData.firstMesh += tr.renderViewParms.numMeshes;
	tr.viewData.firstPostProcessMesh += tr.renderViewParms.numPostProcessMeshes;

	tr.viewData.firstLight += tr.renderViewParms.numLights;
	tr.viewData.firstFogLight += tr.renderViewParms.numFogLights;

	// Render a subview if needed
	if (R_RenderSubview()){
		if (r_subviewOnly->integerValue)
			return;
	}

	// Add a render view command
	R_AddRenderViewCommand();

	// Add lights to the light editor if active
	R_AddEditorLights();
}

/*
 =================
 R_ClearScene
 =================
*/
void R_ClearScene (void){

	tr.scene.firstEntity = tr.scene.numEntities;
	tr.scene.firstLight = tr.scene.numLights;
	tr.scene.firstPoly = tr.scene.numPolys;
}

/*
 =================
 R_AddEntityToScene
 =================
*/
void R_AddEntityToScene (const renderEntity_t *renderEntity){

	renderEntity_t	*entity;

	if (tr.scene.numEntities >= MAX_RENDER_ENTITIES){
		Com_DPrintf(S_COLOR_YELLOW "R_AddEntityToScene: MAX_RENDER_ENTITIES hit\n");
		return;
	}

	entity = &tr.scene.entities[tr.scene.numEntities++];

	*entity = *renderEntity;
	entity->index = tr.scene.numEntities - 1;
}

/*
 =================
 R_AddLightToScene
 =================
*/
void R_AddLightToScene (const renderLight_t *renderLight){

	renderLight_t	*light;

	if (tr.scene.numLights >= MAX_RENDER_LIGHTS){
		Com_DPrintf(S_COLOR_YELLOW "R_AddLightToScene: MAX_RENDER_LIGHTS hit\n");
		return;
	}

	light = &tr.scene.lights[tr.scene.numLights++];

	*light = *renderLight;
}

/*
 =================
 R_AddPolyToScene
 =================
*/
void R_AddPolyToScene (const renderPoly_t *renderPoly){

	renderPoly_t	*poly;

	if (tr.scene.numPolys >= MAX_RENDER_POLYS){
		Com_DPrintf(S_COLOR_YELLOW "R_AddPolyToScene: MAX_RENDER_POLYS hit\n");
		return;
	}

	poly = &tr.scene.polys[tr.scene.numPolys++];

	*poly = *renderPoly;
}

/*
 =================
 R_RenderScene
 =================
*/
void R_RenderScene (const renderView_t *renderView, qboolean primaryView){

	double	time;

	if (r_skipFrontEnd->integerValue)
		return;

	if (r_showRenderTime->integerValue)
		time = Sys_GetClockTicks();

	if (primaryView){
		if (!tr.worldModel)
			Com_Error(ERR_DROP, "R_RenderScene: NULL worldModel");

		// Copy the primary view
		tr.primaryViewAvailable = true;

		tr.primaryView.renderView = *renderView;

		tr.primaryView.numEntities = tr.scene.numEntities;
		tr.primaryView.firstEntity = tr.scene.firstEntity;

		tr.primaryView.numLights = tr.scene.numLights;
		tr.primaryView.firstLight = tr.scene.firstLight;

		tr.primaryView.numPolys = tr.scene.numPolys;
		tr.primaryView.firstPoly = tr.scene.firstPoly;
	}

	// Copy renderView
	tr.renderView = *renderView;

	// Set up renderViewParms
	tr.renderViewParms.primaryView = primaryView;

	tr.renderViewParms.subview = SUBVIEW_NONE;

	tr.renderViewParms.renderEntities = &tr.scene.entities[tr.scene.firstEntity];
	tr.renderViewParms.numRenderEntities = tr.scene.numEntities - tr.scene.firstEntity;

	tr.renderViewParms.renderLights = &tr.scene.lights[tr.scene.firstLight];
	tr.renderViewParms.numRenderLights = tr.scene.numLights - tr.scene.firstLight;

	tr.renderViewParms.renderPolys = &tr.scene.polys[tr.scene.firstPoly];
	tr.renderViewParms.numRenderPolys = tr.scene.numPolys - tr.scene.firstPoly;

	// Render main view
	R_RenderView();

	if (r_showRenderTime->integerValue)
		tr.pc.timeFrontEnd += (Sys_GetClockTicks() - time);
}

/*
 =================
 R_SetLightStyle
 =================
*/
void R_SetLightStyle (int style, float r, float g, float b){

	lightStyle_t	*ls;

	if (style < 0 || style >= MAX_LIGHTSTYLES)
		Com_Error(ERR_DROP, "R_SetLightStyle: out of range");

	ls = &tr.lightStyles[style];

	VectorSet(ls->rgb, r, g, b);
}

/*
 =================
 R_GetPicSize

 This is needed by some client drawing functions
 =================
*/
void R_GetPicSize (material_t *material, float *w, float *h){

	*w = (float)material->stages->textureStage.texture->sourceWidth;
	*h = (float)material->stages->textureStage.texture->sourceHeight;
}

/*
 =================
 R_DrawStretchPic
 =================
*/
void R_DrawStretchPic (float x, float y, float w, float h, float s1, float t1, float s2, float t2, const color_t color, material_t *material){

	stretchPicCommand_t	*cmd;

	if (!material)
		Com_Error(ERR_DROP, "R_DrawStretchPic: NULL material");

	// Adjust coordinates
	x *= tr.currentRenderCrop->scaleX;
	y *= tr.currentRenderCrop->scaleY;
	w *= tr.currentRenderCrop->scaleX;
	h *= tr.currentRenderCrop->scaleY;

	// Add a stretch pic command
	cmd = RB_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
		return;

	cmd->commandId = RC_STRETCH_PIC;

	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;

	cmd->color[0] = color[0];
	cmd->color[1] = color[1];
	cmd->color[2] = color[2];
	cmd->color[3] = color[3];

	cmd->material = material;
}

/*
 =================
 R_DrawShearedPic
 =================
*/
void R_DrawShearedPic (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float shearX, float shearY, const color_t color, material_t *material){

	shearedPicCommand_t	*cmd;

	if (!material)
		Com_Error(ERR_DROP, "R_DrawShearedPic: NULL material");

	// Adjust coordinates
	x *= tr.currentRenderCrop->scaleX;
	y *= tr.currentRenderCrop->scaleY;
	w *= tr.currentRenderCrop->scaleX;
	h *= tr.currentRenderCrop->scaleY;

	shearX *= tr.currentRenderCrop->scaleX;
	shearY *= tr.currentRenderCrop->scaleY;

	// Add a sheared pic command
	cmd = RB_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
		return;

	cmd->commandId = RC_SHEARED_PIC;

	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
	cmd->shearX = shearX;
	cmd->shearY = shearY;

	cmd->color[0] = color[0];
	cmd->color[1] = color[1];
	cmd->color[2] = color[2];
	cmd->color[3] = color[3];

	cmd->material = material;
}

/*
 =================
 R_CropRender
 =================
*/
void R_CropRender (int width, int height, qboolean makePowerOfTwo){

	renderSizeCommand_t	*cmd;

	if (tr.numRenderCrops == MAX_RENDER_CROPS)
		Com_Error(ERR_DROP, "R_CropRender: MAX_RENDER_CROPS hit");

	tr.numRenderCrops++;

	// Check the dimensions
	if ((width < 1 || width > 640) || (height < 1 || height > 480))
		Com_Error(ERR_DROP, "R_CropRender: bad size (%i x %i)", width, height);

	// Calculate the new crop size
	width *= tr.currentRenderCrop->scaleX;
	height *= tr.currentRenderCrop->scaleY;

	if (makePowerOfTwo){
		width = NearestPowerOfTwo(width, true);
		height = NearestPowerOfTwo(height, true);
	}

	// Set up the current render crop
	tr.currentRenderCrop = &tr.renderCrops[tr.numRenderCrops-1];

	tr.currentRenderCrop->width = width;
	tr.currentRenderCrop->height = height;

	tr.currentRenderCrop->scaleX = width / 640.0;
	tr.currentRenderCrop->scaleY = height / 480.0;

	// Add a render size command
	cmd = RB_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
		return;

	cmd->commandId = RC_RENDER_SIZE;

	cmd->width = tr.currentRenderCrop->width;
	cmd->height = tr.currentRenderCrop->height;
}

/*
 =================
 R_UnCropRender
 =================
*/
void R_UnCropRender (void){

	renderSizeCommand_t	*cmd;

	if (tr.numRenderCrops < 1)
		Com_Error(ERR_DROP, "R_UnCropRender: numRenderCrops < 1");

	tr.numRenderCrops--;

	// Set up the current render crop
	tr.currentRenderCrop = &tr.renderCrops[tr.numRenderCrops-1];

	// Add a render size command
	cmd = RB_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
		return;

	cmd->commandId = RC_RENDER_SIZE;

	cmd->width = tr.currentRenderCrop->width;
	cmd->height = tr.currentRenderCrop->height;
}

/*
 =================
 R_GetGLConfig

 Used by other systems to get the GL config
 =================
*/
void R_GetGLConfig (glConfig_t *config){

	if (!config)
		Com_Error(ERR_FATAL, "R_GetGLConfig: NULL config");

	*config = glConfig;
}

/*
 =================
 R_GetModeInfo
 =================
*/
qboolean R_GetModeInfo (int mode, int *width, int *height){

	if (mode == -1){
		if (r_customWidth->integerValue < 1 || r_customHeight->integerValue < 1)
			return false;

		*width = r_customWidth->integerValue;
		*height = r_customHeight->integerValue;

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
 R_ListModes_f
 =================
*/
static void R_ListModes_f (void){

	int i;

	Com_Printf("Mode -1: %ix%i (custom)\n", r_customWidth->integerValue, r_customHeight->integerValue);

	for (i = 0; i < NUM_VIDEO_MODES; i++)
		Com_Printf("%s\n", r_videoModes[i].description);
}

/*
 =================
 R_GfxInfo_f
 =================
*/
static void R_GfxInfo_f (void){

	Com_Printf("GL_VENDOR: %s\n", glConfig.vendorString);
	Com_Printf("GL_RENDERER: %s\n", glConfig.rendererString);
	Com_Printf("GL_VERSION: %s\n", glConfig.versionString);
	Com_Printf("GL_EXTENSIONS: %s\n", glConfig.extensionsString);

#ifdef WIN32
	Com_Printf("WGL_EXTENSIONS: %s\n", glConfig.wglExtensionsString);
#endif

	Com_Printf("\n");
	Com_Printf("GL_MAX_TEXTURE_SIZE: %i\n", glConfig.maxTextureSize);
	Com_Printf("GL_MAX_CUBE_MAP_TEXTURE_SIZE: %i\n", glConfig.maxCubeMapTextureSize);
	Com_Printf("GL_MAX_TEXTURE_UNITS: %i\n", glConfig.maxTextureUnits);
	Com_Printf("GL_MAX_TEXTURE_COORDS: %i\n", glConfig.maxTextureCoords);
	Com_Printf("GL_MAX_TEXTURE_IMAGE_UNITS: %i\n", glConfig.maxTextureImageUnits);
	Com_Printf("GL_MAX_TEXTURE_MAX_ANISOTROPY: %.1f\n", glConfig.maxTextureMaxAnisotropy);
	Com_Printf("GL_MAX_TEXTURE_LOD_BIAS: %.1f\n", glConfig.maxTextureLodBias);

	Com_Printf("\n");
	Com_Printf("PIXELFORMAT: color(%i-bits) alpha(%i-bits) depth(%i-bits) stencil(%i-bits)\n", glConfig.colorBits, glConfig.alphaBits, glConfig.depthBits, glConfig.stencilBits);
	Com_Printf("SAMPLES: %s\n", (glConfig.samples) ? va("%ix", glConfig.samples) : "none");
	Com_Printf("MODE: %i, %i x %i %s%s\n", r_mode->integerValue, glConfig.videoWidth, glConfig.videoHeight, (glConfig.isFullscreen) ? "fullscreen" : "windowed", (glConfig.displayFrequency) ? va(" (%i Hz)", glConfig.displayFrequency) : "");
	Com_Printf("CPU: %s\n", Cvar_GetString("sys_cpuString"));
	Com_Printf("\n");

	Com_Printf("ARB  path available%s\n", (backEnd.activePath == PATH_ARB) ? " (ACTIVE)" : "");
	Com_Printf("NV10 path %savailable%s\n", (glConfig.allowNV10Path) ? "" : "not ", (backEnd.activePath == PATH_NV10) ? " (ACTIVE)" : "");
	Com_Printf("NV20 path %savailable%s\n", (glConfig.allowNV20Path) ? "" : "not ", (backEnd.activePath == PATH_NV20) ? " (ACTIVE)" : "");
	Com_Printf("NV30 path %savailable%s\n", (glConfig.allowNV30Path) ? "" : "not ", (backEnd.activePath == PATH_NV30) ? " (ACTIVE)" : "");
	Com_Printf("R200 path %savailable%s\n", (glConfig.allowR200Path) ? "" : "not ", (backEnd.activePath == PATH_R200) ? " (ACTIVE)" : "");
	Com_Printf("ARB2 path %savailable%s\n", (glConfig.allowARB2Path) ? "" : "not ", (backEnd.activePath == PATH_ARB2) ? " (ACTIVE)" : "");
	Com_Printf("\n");

	if (glConfig.textureFilterAnisotropic)
		Com_Printf("Using %s texture filtering w/ %.1f anisotropy\n", r_textureFilter->value, r_textureAnisotropy->floatValue);
	else
		Com_Printf("Using %s texture filtering\n", r_textureFilter->value);

	if (glConfig.textureCompression) {
		if (r_compressTextures->integerValue || r_compressNormalTextures->integerValue || r_s3tc)
			Com_Printf("Using texture compression\n");
		else
			Com_Printf("Texture compression available but disabled\n");
	} else Com_Printf("Texture compression not available\n");

	if (glConfig.vertexBufferObject) {
		if (r_vertexBuffers->integerValue)
			Com_Printf("Using vertex buffers\n");
		else
			Com_Printf("Vertex buffers available but disabled\n");
	} else Com_Printf("Vertex buffers not available\n");

	if (glConfig.vertexProgram)
		Com_Printf("Using vertex programs\n");
	else
		Com_Printf("Vertex programs not available\n");

	if (glConfig.fragmentProgram)
		Com_Printf("Using fragment programs\n");
	else
		Com_Printf("Fragment programs not available\n");

	if (glConfig.stencilTwoSide || glConfig.atiSeparateStencil)
		Com_Printf("Using two sided stencil\n");
	else
		Com_Printf("Two sided stencil not available\n");

	if (glConfig.depthBoundsTest)
		Com_Printf("Using depth bounds testing\n");
	else
		Com_Printf("Depth bounds testing not available\n");

	if (glConfig.swapControl && r_swapInterval->integerValue)
		Com_Printf("Forcing swapInterval %i\n", r_swapInterval->integerValue);
	else
		Com_Printf("swapInterval not forced\n");

	if (r_finish->integerValue)
		Com_Printf("Forcing glFinish\n");
	else
		Com_Printf("glFinish not forced\n");
}

/*
 =================
 R_Benchmark_f
 =================
*/
static void R_Benchmark_f (void){

	renderView_t	renderView;
	float			time, fraction;
	int				i, pixels;

	if (!tr.primaryViewAvailable){
		Com_Printf("No primaryView available for benchmarking\n");
		return;
	}

	renderView = tr.primaryView.renderView;

	Cvar_ForceSet("r_screenFraction", "1.0");
	Cvar_ForceSet("r_skipRenderContext", "0");
	Cvar_ForceSet("r_skipInteractions", "0");

	// Fillrate test
	for (fraction = 1.0; fraction > 0.0; fraction -= 0.1){
		pixels = (renderView.width * fraction) * (renderView.height * fraction);

		Cvar_ForceSet("r_screenFraction", va("%.1f", fraction));

		time = Sys_GetClockTicks();

		for (i = 0; i < 50; i++){
			R_BeginFrame();

			tr.scene.numEntities = tr.primaryView.numEntities;
			tr.scene.firstEntity = tr.primaryView.firstEntity;

			tr.scene.numLights = tr.primaryView.numLights;
			tr.scene.firstLight = tr.primaryView.firstLight;

			tr.scene.numPolys = tr.primaryView.numPolys;
			tr.scene.firstPoly = tr.primaryView.firstPoly;

			R_RenderScene(&renderView, true);

			R_EndFrame();
		}

		time = Sys_GetClockTicks() - time;

		Com_Printf("%-16s   msec:%8.2f   fps:%8.2f\n", va("kpix:%11i", pixels / 1000), SEC2MS(time) / 50, 50.0/time);
	}

	Cvar_ForceSet("r_screenFraction", "1.0");

	// CPU performance test
	Cvar_ForceSet("r_skipRenderContext", "1");

	time = Sys_GetClockTicks();

	for (i = 0; i < 50; i++){
		R_BeginFrame();

		tr.scene.numEntities = tr.primaryView.numEntities;
		tr.scene.firstEntity = tr.primaryView.firstEntity;

		tr.scene.numLights = tr.primaryView.numLights;
		tr.scene.firstLight = tr.primaryView.firstLight;

		tr.scene.numPolys = tr.primaryView.numPolys;
		tr.scene.firstPoly = tr.primaryView.firstPoly;

		R_RenderScene(&renderView, true);

		R_EndFrame();
	}

	time = Sys_GetClockTicks() - time;

	Com_Printf("%-16s   msec:%8.2f   fps:%8.2f\n", "no context", SEC2MS(time) / 50, 50.0/time);

	Cvar_ForceSet("r_skipRenderContext", "0");

	// Interaction overdraw test
	Cvar_ForceSet("r_skipInteractions", "1");

	time = Sys_GetClockTicks();

	for (i = 0; i < 50; i++){
		R_BeginFrame();

		tr.scene.numEntities = tr.primaryView.numEntities;
		tr.scene.firstEntity = tr.primaryView.firstEntity;

		tr.scene.numLights = tr.primaryView.numLights;
		tr.scene.firstLight = tr.primaryView.firstLight;

		tr.scene.numPolys = tr.primaryView.numPolys;
		tr.scene.firstPoly = tr.primaryView.firstPoly;

		R_RenderScene(&renderView, true);

		R_EndFrame();
	}

	time = Sys_GetClockTicks() - time;

	Com_Printf("%-16s   msec:%8.2f   fps:%8.2f\n", "no interactions", SEC2MS(time) / 50, 50.0/time);

	Cvar_ForceSet("r_skipInteractions", "0");
}

/*
 =================
 R_PerformanceCounters
 =================
*/
static void R_PerformanceCounters (void){

	if (r_showCull->integerValue)
		Com_Printf("bin: %i, bout: %i, sin: %i, sout: %i\n", tr.pc.boxIn, tr.pc.boxOut, tr.pc.sphereIn, tr.pc.sphereOut);

	if (r_showSurfaces->integerValue)
		Com_Printf("surfaces: %i, leafs: %i\n", tr.pc.surfaces, tr.pc.leafs);

	if (r_showDynamic->integerValue)
		Com_Printf("entities: %i, decals: %i, polys: %i\n", tr.pc.entities, tr.pc.decals, tr.pc.polys);

	if (r_showDeforms->integerValue)
		Com_Printf("deforms: %i (deformVertices: %i)\n", tr.pc.deforms, tr.pc.deformVertices);

	if (r_showPrimitives->integerValue)
		Com_Printf("views: %i, draws: %i, tris: %i (shdw: %i), verts: %i (shdw: %i)\n", tr.pc.views, tr.pc.draws, (tr.pc.indices + tr.pc.shadowIndices) / 3, tr.pc.shadowIndices / 3, (tr.pc.vertices + tr.pc.shadowVertices), tr.pc.shadowVertices);

	if (r_showVertexBuffers->integerValue)
		Com_Printf("vertex buffers: %i (%.2f MB)\n", tr.pc.vertexBuffers, tr.pc.vertexBufferBytes / 1048576.0);

	if (r_showTextureUsage->integerValue)
		Com_Printf("textures: %i (%.2f MB)\n", tr.pc.textures, tr.pc.textureBytes / 1048576.0);

	if (r_showRenderToTexture->integerValue)
		Com_Printf("capture renders: %i (%ik pixels), update textures: %i (%ik pixels)\n", tr.pc.captureRenders, tr.pc.captureRenderPixels / 1000, tr.pc.updateTextures, tr.pc.updateTexturePixels / 1000);

	if (r_showOverdraw->integerValue)
		Com_Printf("overdraw: %.2f\n", tr.pc.overdraw);

	if (r_showInteractions->integerValue)
		Com_Printf("interactions: %i in %.1f msec using %i KB (light: %i, fog: %i)\n", tr.pc.interactions, SEC2MS(tr.pc.timeInteractions), tr.pc.interactionsMemory / 1024, tr.pc.interactionsLight, tr.pc.interactionsFog);

	if (r_showShadows->integerValue)
		Com_Printf("shadows: %i in %.1f msec using %i KB (zpass: %i, zfail: %i)\n", tr.pc.shadows, SEC2MS(tr.pc.timeShadows), tr.pc.shadowsMemory / 1024, tr.pc.shadowsZPass, tr.pc.shadowsZFail);

	if (r_showShadowCount->integerValue)
		Com_Printf("shadow overdraw: %.2f\n", tr.pc.overdrawShadows);

	if (r_showLights->integerValue)
		Com_Printf("lights: %i in %.1f msec (static: %i, dynamic: %i)\n", tr.pc.lights, SEC2MS(tr.pc.timeLights), tr.pc.lightsStatic, tr.pc.lightsDynamic);

	if (r_showLightCount->integerValue)
		Com_Printf("light overdraw: %.2f\n", tr.pc.overdrawLights);

	if (r_showRenderTime->integerValue)
		Com_Printf("frontEnd: %.1f msec, backEnd: %.1f msec\n", SEC2MS(tr.pc.timeFrontEnd), SEC2MS(tr.pc.timeBackEnd));
}

/*
 =================
 R_BeginFrame
 =================
*/
void R_BeginFrame (void){

	drawBufferCommand_t		*cmd;

	// Bump frame count
	tr.frameCount++;

	// Clear performance counters
	memset(&tr.pc, 0, sizeof(performanceCounters_t));

	// Log file
	if (r_logFile->modified){
		QGL_EnableLogging(r_logFile->integerValue);

		r_logFile->modified = false;
	}

	// Set default GL state
	GL_SetDefaultState();

	// Reset render
	R_ResetRender();

	// Update gamma
	if (r_gamma->modified || r_brightness->modified)
		R_SetGamma();

	// Update texture parameters
	if (r_textureFilter->modified || r_textureAnisotropy->modified || r_textureLodBias->modified)
		R_SetTextureParameters();

	// Update active renderer
	if (r_renderer->modified)
		RB_SetActiveRenderer();

	// Clear all commands
	RB_ClearRenderCommands();

	// Add a draw buffer command
	cmd = RB_GetCommandBuffer(sizeof(*cmd));
	if (cmd){
		cmd->commandId = RC_DRAW_BUFFER;

		cmd->width = tr.currentRenderCrop->width;
		cmd->height = tr.currentRenderCrop->height;
	}
}

/*
 =================
 R_EndFrame
 =================
*/
void R_EndFrame (void){

	swapBuffersCommand_t	*cmd;

	// Add a swap buffers command
	cmd = RB_GetCommandBuffer(sizeof(*cmd));
	if (cmd)
		cmd->commandId = RC_SWAP_BUFFERS;

	// Issue all commands
	RB_IssueRenderCommands();

	// Performance counters
	R_PerformanceCounters();

	// Log file
	if (r_logFile->integerValue > 0)
		Cvar_SetInteger("r_logFile", r_logFile->integerValue - 1);

	// Release frame memory
	Hunk_ClearToHighMark();
}

/*
 =================
 R_Register
 =================
*/
static void R_Register (void){

	r_s3tc = Cvar_Get("r_s3tc", "1", CVAR_ARCHIVE, "S3 Texture Compression");
	r_logFile = Cvar_Get("r_logFile", "0", CVAR_CHEAT, "Number of frames to log GL calls");
	r_clear = Cvar_Get("r_clear", "0", CVAR_CHEAT, "Clear the color buffer");
	r_clearColor = Cvar_Get("r_clearColor", "0", CVAR_CHEAT, "Color index for clearing the color buffer");
	r_frontBuffer = Cvar_Get("r_frontBuffer", "0", CVAR_CHEAT, "Render to the front buffer for debugging");
	r_screenFraction = Cvar_Get("r_screenFraction", "1.0", CVAR_CHEAT, "Render to a fraction of the screen for testing fillrate");
	r_subviewOnly = Cvar_Get("r_subviewOnly", "0", CVAR_CHEAT, "Only render subviews for debugging");
	r_lockPVS = Cvar_Get("r_lockPVS", "0", CVAR_CHEAT, "Don't update the PVS when the camera moves");
	r_zNear = Cvar_Get("r_zNear", "3.0", CVAR_CHEAT, "Near clip plane distance");
	r_zFar = Cvar_Get("r_zFar", "0.0", CVAR_CHEAT, "Far clip plane distance (0 = infinite)");
	r_offsetFactor = Cvar_Get("r_offsetFactor", "0.0", CVAR_CHEAT, "Polygon offset factor");
	r_offsetUnits = Cvar_Get("r_offsetUnits", "-600.0", CVAR_CHEAT, "Polygon offset units");
	r_shadowPolygonFactor = Cvar_Get("r_shadowPolygonFactor", "0.0", CVAR_CHEAT, "Shadow polygon offset factor");
	r_shadowPolygonOffset = Cvar_Get("r_shadowPolygonOffset", "1.0", CVAR_CHEAT, "Shadow polygon offset units");
	r_colorMipLevels = Cvar_Get("r_colorMipLevels", "0", CVAR_CHEAT | CVAR_LATCH, "Color mip levels for testing mipmap usage");
	r_singleMaterial = Cvar_Get("r_singleMaterial", "0", CVAR_CHEAT | CVAR_LATCH, "Use a single default material for every surface for testing performance");
	r_singleLight = Cvar_Get("r_singleLight", "-1", CVAR_CHEAT, "Only draw the specified static light");
	r_showCluster = Cvar_Get("r_showCluster", "0", CVAR_CHEAT, "Report the current view cluster and area");
	r_showCull = Cvar_Get("r_showCull", "0", CVAR_CHEAT, "Report box and sphere culling statistics");
	r_showSurfaces = Cvar_Get("r_showSurfaces", "0", CVAR_CHEAT, "Report how many surfaces and leafs were drawn this frame");
	r_showDynamic = Cvar_Get("r_showDynamic", "0", CVAR_CHEAT, "Report how many surfaces were regenerated this frame");
	r_showDeforms = Cvar_Get("r_showDeforms", "0", CVAR_CHEAT, "Report how many surfaces and vertices were affected by material deforms");
	r_showPrimitives = Cvar_Get("r_showPrimitives", "0", CVAR_CHEAT, "Report view/draw/index/vertex counts");
	r_showVertexBuffers = Cvar_Get("r_showVertexBuffers", "0", CVAR_CHEAT, "Report vertex buffer statistics");
	r_showTextureUsage = Cvar_Get("r_showTextureUsage", "0", CVAR_CHEAT, "Report texture memory usage");
	r_showRenderToTexture = Cvar_Get("r_showRenderToTexture", "0", CVAR_CHEAT, "Report render-to-texture statistics");
	r_showDepth = Cvar_Get("r_showDepth", "0", CVAR_CHEAT, "Draw the contents of the depth buffer");
	r_showOverdraw = Cvar_Get("r_showOverdraw", "0", CVAR_CHEAT, "Draw surfaces colored by overdraw (1 = geometry, 2 = interaction, 3 = geometry and interaction)");
	r_showTris = Cvar_Get("r_showTris", "0", CVAR_CHEAT, "Draw in wireframe mode (1 = draw visible ones, 2 = draw everything through walls)");
	r_showNormals = Cvar_Get("r_showNormals", "0", CVAR_CHEAT, "Draw vertex normals");
	r_showTextureVectors = Cvar_Get("r_showTextureVectors", "0", CVAR_CHEAT, "Draw texture (tangent) vectors");
	r_showTangentSpace = Cvar_Get("r_showTangentSpace", "0", CVAR_CHEAT, "Draw surfaces colored by tangent space (1 = 1st tangent, 2 = 2nd tangent, 3 = normal)");
	r_showVertexColors = Cvar_Get("r_showVertexColors", "0", CVAR_CHEAT, "Draw vertex colors");
	r_showModelBounds = Cvar_Get("r_showModelBounds", "0", CVAR_CHEAT, "Draw model bounds (1 = draw planes, 2 = draw edges, 3 = draw both)");
	r_showInteractions = Cvar_Get("r_showInteractions", "0", CVAR_CHEAT, "Report interaction generation statistics");
	r_showShadows = Cvar_Get("r_showShadows", "0", CVAR_CHEAT, "Report shadow generation statistics");
	r_showShadowCount = Cvar_Get("r_showShadowCount", "0", CVAR_CHEAT, "Draw surfaces colored by shadow count (1 = count all, 2 = only count static shadows, 3 = only count dynamic shadows)");
	r_showShadowTris = Cvar_Get("r_showShadowTris", "0", CVAR_CHEAT, "Draw shadows in wireframe mode (1 = draw visible ones, 2 = draw everything through walls)");
	r_showShadowVolumes = Cvar_Get("r_showShadowVolumes", "0", CVAR_CHEAT, "Draw shadow planes");
	r_showShadowSilhouettes = Cvar_Get("r_showShadowSilhouettes", "0", CVAR_CHEAT, "Draw shadow silhouettes");
	r_showLights = Cvar_Get("r_showLights", "0", CVAR_CHEAT, "Report light generation statistics (2 = also print static volume numbers)");
	r_showLightCount = Cvar_Get("r_showLightCount", "0", CVAR_CHEAT, "Draw surfaces colored by light count (1 = count visible ones, 2 = count everything through walls)");
	r_showLightBounds = Cvar_Get("r_showLightBounds", "0", CVAR_CHEAT, "Draw light bounds (1 = draw planes, 2 = draw edges, 3 = draw both)");
	r_showLightScissors = Cvar_Get("r_showLightScissors", "0", CVAR_CHEAT, "Draw light scissor rectangles");
	r_showRenderTime = Cvar_Get("r_showRenderTime", "0", CVAR_CHEAT, "Report renderer timing information");
	r_skipVisibility = Cvar_Get("r_skipVisibility", "0", CVAR_CHEAT, "Skip visibility determination tests");
	r_skipAreas = Cvar_Get("r_skipAreas", "0", CVAR_CHEAT, "Skip area portals and render everything behind closed doors");
	r_skipSuppress = Cvar_Get("r_skipSuppress", "0", CVAR_CHEAT, "Skip per-view suppressions");
	r_skipCulling = Cvar_Get("r_skipCulling", "0", CVAR_CHEAT, "Skip surface culling");
	r_skipInteractionCulling = Cvar_Get("r_skipInteractionCulling", "0", CVAR_CHEAT, "Skip interaction culling");
	r_skipShadowCulling = Cvar_Get("r_skipShadowCulling", "0", CVAR_CHEAT, "Skip shadow culling");
	r_skipLightCulling = Cvar_Get("r_skipLightCulling", "0", CVAR_CHEAT, "Skip light culling");
	r_skipBatching = Cvar_Get("r_skipBatching", "0", CVAR_CHEAT, "Skip batching and issue a draw call per surface");
	r_skipDecorations = Cvar_Get("r_skipDecorations", "0", CVAR_CHEAT, "Skip rendering decorations (map models)");
	r_skipEntities = Cvar_Get("r_skipEntities", "0", CVAR_CHEAT, "Skip rendering entities");
	r_skipDecals = Cvar_Get("r_skipDecals", "0", CVAR_CHEAT, "Skip rendering decals");
	r_skipPolys = Cvar_Get("r_skipPolys", "0", CVAR_CHEAT, "Skip rendering polys");
	r_skipLights = Cvar_Get("r_skipLights", "0", CVAR_CHEAT, "Skip rendering lights");
	r_skipExpressions = Cvar_Get("r_skipExpressions", "0", CVAR_CHEAT, "Skip expression evaluation in materials");
	r_skipConstantExpressions = Cvar_Get("r_skipConstantExpressions", "0", CVAR_CHEAT, "Skip constant expressions in materials, re-evaluating everything each frame");
	r_skipDeforms = Cvar_Get("r_skipDeforms", "0", CVAR_CHEAT, "Skip surface deformations");
	r_skipPrograms = Cvar_Get("r_skipPrograms", "0", CVAR_CHEAT, "Skip rendering vertex/fragment program stages from materials");
	r_skipAmbient = Cvar_Get("r_skipAmbient", "0", CVAR_CHEAT, "Skip rendering ambient stages from materials");
	r_skipBump = Cvar_Get("r_skipBump", "0", CVAR_CHEAT, "Skip rendering bump maps");
	r_skipDiffuse = Cvar_Get("r_skipDiffuse", "0", CVAR_CHEAT, "Skip rendering diffuse maps");
	r_skipSpecular = Cvar_Get("r_skipSpecular", "0", CVAR_CHEAT, "Skip rendering specular maps");
	r_skipInteractions = Cvar_Get("r_skipInteractions", "0", CVAR_CHEAT, "Skip rendering light-surface interactions");
	r_skipFogLights = Cvar_Get("r_skipFogLights", "0", CVAR_CHEAT, "Skip rendering fog lights");
	r_skipBlendLights = Cvar_Get("r_skipBlendLights", "0", CVAR_CHEAT, "Skip rendering blend lights");
	r_skipTranslucent = Cvar_Get("r_skipTranslucent", "0", CVAR_CHEAT, "Skip rendering translucent materials");
	r_skipPostProcess = Cvar_Get("r_skipPostProcess", "0", CVAR_CHEAT, "Skip rendering post-process materials");
	r_skipScissors = Cvar_Get("r_skipScissors", "0", CVAR_CHEAT, "Skip calculating scissor rectangles for lights");
	r_skipDepthBounds = Cvar_Get("r_skipDepthBounds", "0", CVAR_CHEAT, "Skip calculating depth bounds for lights");
	r_skipSubviews = Cvar_Get("r_skipSubviews", "0", CVAR_CHEAT, "Skip rendering subviews (mirrors/cameras)");
	r_skipVideos = Cvar_Get("r_skipVideos", "0", CVAR_CHEAT, "Skip updating videos");
	r_skipCopyTexture = Cvar_Get("r_skipCopyTexture", "0", CVAR_CHEAT, "Skip copying the framebuffer to textures");
	r_skipDynamicTextures = Cvar_Get("r_skipDynamicTextures", "0", CVAR_CHEAT, "Skip updating dynamically generated textures");
	r_skipRender = Cvar_Get("r_skipRender", "0", CVAR_CHEAT, "Skip rendering 3D views");
	r_skipRenderContext = Cvar_Get("r_skipRenderContext", "0", CVAR_CHEAT, "Skip all GL calls for testing CPU performance");
	r_skipFrontEnd = Cvar_Get("r_skipFrontEnd", "0", CVAR_CHEAT, "Skip all front-end work");
	r_skipBackEnd = Cvar_Get("r_skipBackEnd", "0", CVAR_CHEAT, "Skip all back-end work");
	r_glDriver = Cvar_Get("r_glDriver", GL_DRIVER_OPENGL, CVAR_ARCHIVE | CVAR_LATCH, "GL driver");
	r_ignoreGLErrors = Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE, "Ignore GL errors");
	r_multiSamples = Cvar_Get("r_multiSamples", "0", CVAR_ARCHIVE | CVAR_LATCH, "Number of samples for antialiasing");
	r_swapInterval = Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE, "Sync to the display's refresh rate");
	r_mode = Cvar_Get("r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH, "Video mode index for setting screen resolution");
	r_fullscreen = Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH, "Switch between fullscreen and windowed mode");
	r_customWidth = Cvar_Get("r_customWidth", "1600", CVAR_ARCHIVE | CVAR_LATCH, "Screen width when r_mode is set to -1");
	r_customHeight = Cvar_Get("r_customHeight", "1024", CVAR_ARCHIVE | CVAR_LATCH, "Screen height when r_mode is set to -1");
	r_displayRefresh = Cvar_Get("r_displayRefresh", "0", CVAR_ARCHIVE | CVAR_LATCH, "Optional display refresh rate");
	r_gamma = Cvar_Get("r_gamma", "1.0", CVAR_ARCHIVE, "Set the device gamma ramp");
	r_brightness = Cvar_Get("r_brightness", "1.0", CVAR_ARCHIVE, "Set the device gamma ramp");
	r_finish = Cvar_Get("r_finish", "0", CVAR_ARCHIVE, "Sync CPU and GPU every frame");
	r_renderer = Cvar_Get("r_renderer", "", CVAR_ARCHIVE, "Renderer path to use (ARB/NV10/NV20/NV30/R200/ARB2)");
	r_vertexBuffers = Cvar_Get("r_vertexBuffers", "1", CVAR_ARCHIVE, "Store vertex data in VBOs");
	r_shadows = Cvar_Get("r_shadows", "1", CVAR_ARCHIVE, "Render stencil shadows");
	r_playerShadow = Cvar_Get("r_playerShadow", "0", CVAR_ARCHIVE, "Render stencil shadows for the player");
	r_dynamicLights = Cvar_Get("r_dynamicLights", "1", CVAR_ARCHIVE, "Render dynamic lights");
	r_fastInteractions = Cvar_Get("r_fastInteractions", "0", CVAR_ARCHIVE, "Only compute diffuse lighting with per-pixel attenuation");
	r_caustics = Cvar_Get("r_caustics", "1", CVAR_ARCHIVE, "Render underwater caustics");
	r_lightScale = Cvar_Get("r_lightScale", "2.0", CVAR_ARCHIVE, "Light intensity scale factor");
	r_lightDetailLevel = Cvar_Get("r_lightDetailLevel", "0", CVAR_ARCHIVE, "Light detail level (lights with a detail level < this setting are ignored)");
	r_shaderPrograms = Cvar_Get("r_shaderPrograms", "1", CVAR_ARCHIVE | CVAR_LATCH, "Render vertex/fragment program stages from materials");
	r_roundTexturesDown = Cvar_Get("r_roundTexturesDown", "0", CVAR_ARCHIVE | CVAR_LATCH, "Down size non-power of two textures");
	r_downSizeTextures = Cvar_Get("r_downSizeTextures", "0", CVAR_ARCHIVE | CVAR_LATCH, "Down size textures");
	r_downSizeNormalTextures = Cvar_Get("r_downSizeNormalTextures", "0", CVAR_ARCHIVE | CVAR_LATCH, "Down size normal map textures");
	r_maxTextureSize = Cvar_Get("r_maxTextureSize", "512", CVAR_ARCHIVE | CVAR_LATCH, "Maximum size for down sized textures");
	r_maxNormalTextureSize = Cvar_Get("r_maxNormalTextureSize", "256", CVAR_ARCHIVE | CVAR_LATCH, "Maximum size for down sized normal map textures");
	r_compressTextures = Cvar_Get("r_compressTextures", "0", CVAR_ARCHIVE | CVAR_LATCH, "Compress textures");
	r_compressNormalTextures = Cvar_Get("r_compressNormalTextures", "0", CVAR_ARCHIVE | CVAR_LATCH, "Compress normal map textures");
	r_textureFilter = Cvar_Get("r_textureFilter", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE, "Filtering mode for mipmapped textures");
	r_textureAnisotropy = Cvar_Get("r_textureAnisotropy", "1.0", CVAR_ARCHIVE, "Anisotropic filtering level for mipmapped textures");
	r_textureLodBias = Cvar_Get("r_textureLodBias", "0.0", CVAR_ARCHIVE, "LOD bias for mipmapped textures");

	Cmd_AddCommand("screenShot", R_ScreenShot_f, "Take a screenshot");
	Cmd_AddCommand("envShot", R_EnvShot_f, "Take an environment shot");
	Cmd_AddCommand("materialInfo", R_MaterialInfo_f, "Show material information");
	Cmd_AddCommand("gfxInfo", R_GfxInfo_f, "Show graphics information");
	Cmd_AddCommand("benchmark", R_Benchmark_f, "Benchmark");
	Cmd_AddCommand("listModes", R_ListModes_f, "List video modes");
	Cmd_AddCommand("listPrograms", R_ListPrograms_f, "List loaded programs");
	Cmd_AddCommand("listVideos", R_ListVideos_f, "List loaded videos");
	Cmd_AddCommand("listTextures", R_ListTextures_f, "List loaded textures");
	Cmd_AddCommand("listTables", R_ListTables_f, "List loaded tables");
	Cmd_AddCommand("listMaterials", R_ListMaterials_f, "List loaded materials");
	Cmd_AddCommand("listModels", R_ListModels_f, "List loaded models");
}

/*
 =================
 R_Unregister
 =================
*/
static void R_Unregister (void){

	Cmd_RemoveCommand("screenShot");
	Cmd_RemoveCommand("envShot");
	Cmd_RemoveCommand("materialInfo");
	Cmd_RemoveCommand("gfxInfo");
	Cmd_RemoveCommand("benchmark");
	Cmd_RemoveCommand("listModes");
	Cmd_RemoveCommand("listPrograms");
	Cmd_RemoveCommand("listVideos");
	Cmd_RemoveCommand("listTextures");
	Cmd_RemoveCommand("listTables");
	Cmd_RemoveCommand("listMaterials");
	Cmd_RemoveCommand("listModels");
}

/*
 =================
 R_Init
 =================
*/
void R_Init (qboolean all){

	Com_Printf("------- Renderer Initialization -------\n");

	if (all) {
		R_Register();
		GLimp_Init();
	}

	GL_SetDefaultState();

	R_ClearRender();

	R_SetGamma();

	R_InitPrograms();
	R_InitVideos();
	R_InitTextures();
	R_InitMaterials();
	R_InitModels();
	R_InitLightEditor();
	RB_InitBackEnd();

	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	Com_Printf("---------------------------------------\n");
}

/*
 =================
 R_Shutdown
 =================
*/
void R_Shutdown (qboolean all){

	RB_ShutdownBackEnd();
	R_ShutdownLightEditor();
	R_ShutdownModels();
	R_ShutdownMaterials();
	R_ShutdownTextures();
	R_ShutdownVideos();
	R_ShutdownPrograms();

	if (all) {
		GLimp_Shutdown();
		R_Unregister();
	}
}
