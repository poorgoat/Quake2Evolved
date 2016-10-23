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


// r_render.c -- generic rendering for back-end


#include "r_local.h"


/*
 =======================================================================

 Z-FILL RENDERING

 =======================================================================
*/


/*
 =================
 RB_RenderDepth
 =================
*/
static void RB_RenderDepth (void){

	stage_t		*stage;
	int			i;
	qboolean	hasAlphaTest = false;

	RB_Deform();

	RB_UpdateVertexBuffer(backEnd.vertexBuffer, backEnd.vertices, backEnd.numVertices * sizeof(vertexArray_t));

	RB_SetMaterialState();

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_XYZ);

	if (backEnd.material->coverage == MC_PERFORATED){
		for (i = 0, stage = backEnd.material->stages; i < backEnd.material->numStages; i++, stage++){
			if (!backEnd.material->expressionRegisters[stage->conditionRegister])
				continue;

			if (stage->programs)
				continue;

			if (!stage->alphaTest)
				continue;

			hasAlphaTest = true;

			if (backEnd.material->polygonOffset || stage->privatePolygonOffset){
				GL_Enable(GL_POLYGON_OFFSET_FILL);
				GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);
			}
			else
				GL_Disable(GL_POLYGON_OFFSET_FILL);

			GL_Enable(GL_ALPHA_TEST);
			GL_AlphaFunc(GL_GREATER, backEnd.material->expressionRegisters[stage->alphaTestRegister]);

			GL_Disable(GL_BLEND);

			GL_TexEnv(GL_MODULATE);

			RB_SetupTextureStage(&stage->textureStage);

			RB_DrawElements();

			RB_CleanupTextureStage(&stage->textureStage);
		}

		if (hasAlphaTest)
			return;
	}

	GL_DisableTexture();

	GL_Disable(GL_ALPHA_TEST);

	if (backEnd.material->sort == SORT_SUBVIEW){
		GL_Enable(GL_BLEND);
		GL_BlendFunc(GL_ZERO, GL_ONE);
	}
	else
		GL_Disable(GL_BLEND);

	RB_DrawElements();
}

/*
 =================
 RB_FillDepthBuffer
 =================
*/
void RB_FillDepthBuffer (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	unsigned		sort = 0;
	material_t		*material, *oldMaterial = NULL;
	renderEntity_t	*entity, *oldEntity = NULL;
	meshType_t		type;
	qboolean		caps;
	qboolean		skip;
	int				i;

	if (!numMeshes)
		return;

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_FillDepthBuffer ----------\n");

	// Set the render function
	backEnd.renderBatch = RB_RenderDepth;

	// Set the GL state
	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 128, 255);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_TRUE);
	GL_StencilMask(255);

	qglColor3f(0.0, 0.0, 0.0);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		if (mesh->sort != sort || r_skipBatching->integerValue){
			sort = mesh->sort;

			// Render the last batch
			RB_EndBatch();

			// Decompose sort
			R_DecomposeSort(sort, &material, &entity, &type, &caps);

			// Stop if it's translucent
			if (material->coverage == MC_TRANSLUCENT)
				break;

			// Evaluate registers if needed
			if (material != oldMaterial || entity != oldEntity)
				R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

			// Skip if condition evaluated to false
			if (!material->expressionRegisters[material->conditionRegister]){
				skip = true;
				continue;
			}

			// Set the entity state if needed
			if (entity != oldEntity)
				RB_SetEntityState(entity);

			// Create a new batch
			RB_BeginBatch(material, entity, false, false);

			oldMaterial = material;
			oldEntity = entity;

			skip = false;
		}

		if (skip)
			continue;

		// Batch the mesh
		RB_BatchMesh(type, mesh->data);
	}

	// Render the last batch
	RB_EndBatch();
}


/*
 =======================================================================

 AMBIENT RENDERING

 =======================================================================
*/


/*
 =================
 RB_RenderMaterial
 =================
*/
static void RB_RenderMaterial (void){

	stage_t		*stage;
	int			i;

	if (r_logFile->integerValue)
		QGL_LogPrintf("----- RB_RenderMaterial ( %s ) -----\n", backEnd.material->name);

	RB_Deform();

	RB_UpdateVertexBuffer(backEnd.vertexBuffer, backEnd.vertices, backEnd.numVertices * sizeof(vertexArray_t));

	RB_SetMaterialState();

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_XYZ);

	for (i = 0, stage = backEnd.material->stages; i < backEnd.material->numStages; i++, stage++){
		if (!backEnd.material->expressionRegisters[stage->conditionRegister])
			continue;

		if (stage->lighting != SL_AMBIENT)
			continue;

		if (stage->blend){
			if (stage->blendSrc == GL_ZERO && stage->blendDst == GL_ONE)
				continue;
		}

		RB_SetStageState(stage);

		if (stage->programs){
			if (r_skipPrograms->integerValue)
				continue;

			RB_SetupProgramStage(&stage->programStage);

			RB_DrawElements();

			RB_CleanupProgramStage(&stage->programStage);
		}
		else {
			RB_SetupTextureStage(&stage->textureStage);
			RB_SetupColorStage(&stage->colorStage);

			RB_DrawElements();

			RB_CleanupColorStage(&stage->colorStage);
			RB_CleanupTextureStage(&stage->textureStage);
		}
	}
}

/*
 =================
 RB_RenderMaterialPasses
 =================
*/
void RB_RenderMaterialPasses (mesh_t *meshes, int numMeshes, qboolean postProcess){

	mesh_t			*mesh;
	unsigned		sort = 0;
	material_t		*material, *oldMaterial = NULL;
	renderEntity_t	*entity, *oldEntity = NULL;
	meshType_t		type;
	qboolean		caps;
	qboolean		skip;
	int				i;

	if (r_skipAmbient->integerValue)
		return;

	if (r_skipPostProcess->integerValue){
		if (postProcess)
			return;
	}

	if (!numMeshes)
		return;

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_RenderMaterialPasses ----------\n");

	// Capture current render if needed
	if (postProcess)
		RB_CaptureCurrentRender();

	// Set the render function
	backEnd.renderBatch = RB_RenderMaterial;

	// Set the GL state
	GL_Enable(GL_DEPTH_TEST);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 128, 255);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	if (postProcess)
		GL_DepthMask(GL_TRUE);
	else
		GL_DepthMask(GL_FALSE);

	GL_StencilMask(255);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		if (mesh->sort != sort || r_skipBatching->integerValue){
			sort = mesh->sort;

			// Render the last batch
			RB_EndBatch();

			// Decompose sort
			R_DecomposeSort(sort, &material, &entity, &type, &caps);

			if (r_skipTranslucent->integerValue){
				if (material->coverage == MC_TRANSLUCENT)
					break;
			}

			// Skip if it doesn't have ambient stages
			if (!material->numAmbientStages){
				skip = true;
				continue;
			}

			// Evaluate registers if needed
			if (material != oldMaterial || entity != oldEntity)
				R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

			// Skip if condition evaluated to false
			if (!material->expressionRegisters[material->conditionRegister]){
				skip = true;
				continue;
			}

			// Set the entity state if needed
			if (entity != oldEntity)
				RB_SetEntityState(entity);

			// Create a new batch
			RB_BeginBatch(material, entity, false, false);

			oldMaterial = material;
			oldEntity = entity;

			skip = false;
		}

		if (skip)
			continue;

		// Batch the mesh
		RB_BatchMesh(type, mesh->data);
	}

	// Render the last batch
	RB_EndBatch();
}


/*
 =======================================================================

 STENCIL SHADOW & LIGHT INTERACTION RENDERING

 =======================================================================
*/


/*
 =================
 RB_RenderStencilShadow
 =================
*/
static void RB_RenderStencilShadow (void){

	RB_UpdateVertexBuffer(backEnd.shadowVertexBuffer, backEnd.shadowVertices, backEnd.numVertices * sizeof(shadowVertexArray_t));

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(4, GL_FLOAT, sizeof(shadowVertexArray_t), backEnd.shadowVertexBuffer->pointer + BUFFER_OFFSET_XYZW);

	if (backEnd.shadowCaps){
		if (glConfig.stencilTwoSide){
			qglActiveStencilFaceEXT(GL_BACK);
			GL_StencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
			qglActiveStencilFaceEXT(GL_FRONT);
			GL_StencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);

			qglEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);

			RB_DrawElements();

			qglDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		}
		else if (glConfig.atiSeparateStencil){
			qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
			qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);

			RB_DrawElements();

			qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
			qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
		}
		else {
			GL_CullFace(GL_BACK);
			GL_StencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);

			RB_DrawElements();

			GL_CullFace(GL_FRONT);
			GL_StencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);

			RB_DrawElements();
		}
	}
	else {
		if (glConfig.stencilTwoSide){
			qglActiveStencilFaceEXT(GL_BACK);
			GL_StencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP_EXT);
			qglActiveStencilFaceEXT(GL_FRONT);
			GL_StencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP_EXT);

			qglEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);

			RB_DrawElements();

			qglDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		}
		else if (glConfig.atiSeparateStencil){
			qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP_EXT);
			qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP_EXT);

			RB_DrawElements();

			qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
			qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
		}
		else {
			GL_CullFace(GL_FRONT);
			GL_StencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP_EXT);

			RB_DrawElements();

			GL_CullFace(GL_BACK);
			GL_StencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP_EXT);

			RB_DrawElements();
		}
	}
}

/*
 =================
 RB_StencilShadowPass
 =================
*/
static void RB_StencilShadowPass (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	unsigned		sort = 0;
	material_t		*material, *oldMaterial = NULL;
	renderEntity_t	*entity, *oldEntity = NULL;
	meshType_t		type;
	qboolean		caps;
	qboolean		skip;
	int				i;

	if (!numMeshes)
		return;

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_StencilShadowPass ----------\n");

	// Set the render function
	backEnd.renderBatch = RB_RenderStencilShadow;

	// Set the GL state
	GL_DisableTexture();

	if (glConfig.stencilTwoSide || glConfig.atiSeparateStencil)
		GL_Disable(GL_CULL_FACE);
	else
		GL_Enable(GL_CULL_FACE);

	GL_Enable(GL_POLYGON_OFFSET_FILL);
	GL_PolygonOffset(r_shadowPolygonFactor->floatValue, r_shadowPolygonOffset->floatValue);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 128, 255);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		if (mesh->sort != sort || r_skipBatching->integerValue){
			sort = mesh->sort;

			// Render the last batch
			RB_EndBatch();

			// Decompose sort
			R_DecomposeSort(sort, &material, &entity, &type, &caps);

			// Evaluate registers if needed
			if (material != oldMaterial || entity != oldEntity)
				R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

			// Skip if condition evaluated to false
			if (!material->expressionRegisters[material->conditionRegister]){
				skip = true;
				continue;
			}

			// Set the entity state if needed
			if (entity != oldEntity){
				RB_SetEntityState(entity);

				// Set the light state for this entity
				RB_SetLightState(entity, backEnd.light, NULL, NULL);
			}

			// Create a new batch
			RB_BeginBatch(material, entity, true, caps);

			oldMaterial = material;
			oldEntity = entity;

			skip = false;
		}

		if (skip)
			continue;

		// Batch the mesh
		RB_BatchMeshShadow(type, mesh->data);
	}

	// Render the last batch
	RB_EndBatch();
}

/*
 =================
 RB_RenderInteractions
 =================
*/
static void RB_RenderInteractions (void){

	stage_t		*bumpStage = NULL, *diffuseStage = NULL, *specularStage = NULL;
	stage_t		*stage;
	int			i;

	if (r_logFile->integerValue)
		QGL_LogPrintf("----- RB_RenderInteractions ( %s on %s ) -----\n", backEnd.lightMaterial->name, backEnd.material->name);

	RB_Deform();

	RB_UpdateVertexBuffer(backEnd.vertexBuffer, backEnd.vertices, backEnd.numVertices * sizeof(vertexArray_t));

	RB_SetMaterialState();

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_XYZ);

	for (i = 0, stage = backEnd.material->stages; i < backEnd.material->numStages; i++, stage++){
		if (!backEnd.material->expressionRegisters[stage->conditionRegister])
			continue;

		if (stage->lighting == SL_AMBIENT)
			continue;

		switch (stage->lighting){
		case SL_BUMP:
			if (bumpStage)
				backEnd.renderInteraction(bumpStage, diffuseStage, specularStage);
			else {
				if (diffuseStage || specularStage)
					Com_Printf(S_COLOR_YELLOW "Material '%s' has a bump stage with a preceeding diffuse/specular stage\n", backEnd.material->name);
			}

			bumpStage = stage;
			diffuseStage = NULL;
			specularStage = NULL;

			break;
		case SL_DIFFUSE:
			if (diffuseStage){
				if (bumpStage)
					backEnd.renderInteraction(bumpStage, diffuseStage, specularStage);
				else
					Com_Printf(S_COLOR_YELLOW "Material '%s' has a diffuse stage without a preceeding bump stage\n", backEnd.material->name);
			}

			diffuseStage = stage;

			break;
		case SL_SPECULAR:
			if (specularStage){
				if (bumpStage)
					backEnd.renderInteraction(bumpStage, diffuseStage, specularStage);
				else
					Com_Printf(S_COLOR_YELLOW "Material '%s' has a specular stage without a preceeding bump stage", backEnd.material->name);
			}

			specularStage = stage;

			break;
		}
	}

	backEnd.renderInteraction(bumpStage, diffuseStage, specularStage);
}

/*
 =================
 RB_LightInteractionPass
 =================
*/
static void RB_LightInteractionPass (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	unsigned		sort = 0;
	material_t		*material, *oldMaterial = NULL;
	renderEntity_t	*entity, *oldEntity = NULL;
	meshType_t		type;
	qboolean		caps;
	qboolean		skip;
	int				i, j;

	if (!numMeshes)
		return;

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_LightInteractionPass ----------\n");

	// Set the render function
	backEnd.renderBatch = RB_RenderInteractions;

	// Set the GL state
	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_ONE, GL_ONE);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_EQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_GEQUAL, 128, 255);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	if (backEnd.activePath == PATH_ARB2){
		GL_Enable(GL_VERTEX_PROGRAM_ARB);
		GL_Enable(GL_FRAGMENT_PROGRAM_ARB);

		if (backEnd.lightMaterial->ambientLight){
			if (r_fastInteractions->integerValue){
				GL_BindProgram(tr.ambientInteractionFastVProgram);
				GL_BindProgram(tr.ambientInteractionFastFProgram);
			}
			else {
				GL_BindProgram(tr.ambientInteractionVProgram);
				GL_BindProgram(tr.ambientInteractionFProgram);
			}
		}
		else {
			if (r_fastInteractions->integerValue){
				GL_BindProgram(tr.interactionFastVProgram);
				GL_BindProgram(tr.interactionFastFProgram);
			}
			else {
				GL_BindProgram(tr.interactionVProgram);
				GL_BindProgram(tr.interactionFProgram);
			}
		}
	}

	// Run through the light stages
	for (i = 0, backEnd.lightStage = backEnd.lightMaterial->stages; i < backEnd.lightMaterial->numStages; i++, backEnd.lightStage++){
		if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->conditionRegister])
			continue;

		// Set the GL state
		if (backEnd.light->castShadows){
			if (backEnd.lightStage->shadowDraw)
				GL_StencilFunc(GL_LESS, 128, 255);
			else
				GL_StencilFunc(GL_GEQUAL, 128, 255);
		}
		else {
			if (backEnd.lightStage->shadowDraw)
				continue;

			GL_StencilFunc(GL_ALWAYS, 128, 255);
		}

		// Run through the meshes
		for (j = 0, mesh = meshes; j < numMeshes; j++, mesh++){
			if (mesh->sort != sort || r_skipBatching->integerValue){
				sort = mesh->sort;

				// Render the last batch
				RB_EndBatch();

				// Decompose sort
				R_DecomposeSort(sort, &material, &entity, &type, &caps);

				// Evaluate registers if needed
				if (material != oldMaterial || entity != oldEntity)
					R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

				// Skip if condition evaluated to false
				if (!material->expressionRegisters[material->conditionRegister]){
					skip = true;
					continue;
				}

				// Set the entity state if needed
				if (entity != oldEntity){
					RB_SetEntityState(entity);

					// Set the light state for this entity
					RB_SetLightState(entity, backEnd.light, backEnd.lightMaterial, backEnd.lightStage);
				}

				// Create a new batch
				RB_BeginBatch(material, entity, false, false);

				oldMaterial = material;
				oldEntity = entity;

				skip = false;
			}

			if (skip)
				continue;

			// Batch the mesh
			RB_BatchMesh(type, mesh->data);
		}

		// Render the last batch
		RB_EndBatch();
	}

	// Restore the GL state
	if (backEnd.activePath == PATH_ARB2){
		GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
		GL_Disable(GL_VERTEX_PROGRAM_ARB);
	}
}

/*
 =================
 RB_RenderLights
 =================
*/
void RB_RenderLights (light_t *lights, int numLights){

	int		i;

	if (r_skipInteractions->integerValue)
		return;

	if (!numLights)
		return;

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_RenderLights ----------\n");

	// Run through the lights
	for (i = 0, backEnd.light = lights; i < numLights; i++, backEnd.light++){
		// Set the light material
		backEnd.lightMaterial = backEnd.light->material;

		// Evaluate registers
		R_EvaluateRegisters(backEnd.lightMaterial, backEnd.time, backEnd.light->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightMaterial->conditionRegister])
			continue;

		// Set the scissor rect
		GL_Scissor(backEnd.light->scissorX, backEnd.light->scissorY, backEnd.light->scissorWidth, backEnd.light->scissorHeight);

		// Set the depth bounds
		if (glConfig.depthBoundsTest)
			GL_DepthBounds(backEnd.light->depthMin, backEnd.light->depthMax);

		// Clear the stencil buffer if needed
		if (backEnd.light->castShadows){
			GL_StencilMask(255);

			qglClearStencil(128);
			qglClear(GL_STENCIL_BUFFER_BIT);
		}

		// Render selfShadow meshes to the stencil buffer
		RB_StencilShadowPass(backEnd.light->shadows[0], backEnd.light->numShadows[0]);

		// Render noSelfShadow meshes with lighting
		RB_LightInteractionPass(backEnd.light->interactions[1], backEnd.light->numInteractions[1]);

		// Render noSelfShadow meshes to the stencil buffer
		RB_StencilShadowPass(backEnd.light->shadows[1], backEnd.light->numShadows[1]);

		// Render selfShadow meshes with lighting
		RB_LightInteractionPass(backEnd.light->interactions[0], backEnd.light->numInteractions[0]);
	}

	// Restore the scissor rect
	GL_Scissor(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Restore the depth bounds
	if (glConfig.depthBoundsTest)
		GL_DepthBounds(0.0, 1.0);
}


/*
 =======================================================================

 FOG & BLEND LIGHT INTERACTION RENDERING

 =======================================================================
*/


/*
 =================
 RB_RenderFogVolume
 =================
*/
static void RB_RenderFogVolume (void){

	int		cornerIndices[6][4] = {{3, 2, 6, 7}, {0, 1, 5, 4}, {2, 3, 1, 0}, {4, 5, 7, 6}, {1, 3, 7, 5}, {2, 0, 4, 6}};
	int		i;

	// Draw it
	qglBegin(GL_QUADS);
	for (i = 0; i < 6; i++){
		qglVertex3fv(backEnd.light->corners[cornerIndices[i][0]]);
		qglVertex3fv(backEnd.light->corners[cornerIndices[i][1]]);
		qglVertex3fv(backEnd.light->corners[cornerIndices[i][2]]);
		qglVertex3fv(backEnd.light->corners[cornerIndices[i][3]]);
	}
	qglEnd();

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 =================
 RB_RenderStencilFogVolume
 =================
*/
static void RB_RenderStencilFogVolume (void){

	// Clear the stencil buffer
	GL_StencilMask(255);

	qglClearStencil(128);
	qglClear(GL_STENCIL_BUFFER_BIT);

	// Set the GL state
	GL_LoadMatrix(GL_MODELVIEW, backEnd.viewParms.worldMatrix);

	GL_DisableTexture();

	GL_Enable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 128, 255);

	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	GL_DepthRange(0.0, 1.0);

	// Draw it
	GL_CullFace(GL_BACK);
	GL_StencilOp(GL_KEEP, GL_INCR, GL_KEEP);

	RB_RenderFogVolume();

	GL_CullFace(GL_FRONT);
	GL_StencilOp(GL_KEEP, GL_DECR, GL_KEEP);

	RB_RenderFogVolume();
}

/*
 =================
 RB_RenderFogPassInteraction
 =================
*/
static void RB_RenderFogPassInteraction (void){

	RB_Deform();

	RB_UpdateVertexBuffer(backEnd.vertexBuffer, backEnd.vertices, backEnd.numVertices * sizeof(vertexArray_t));

	RB_SetMaterialState();

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_XYZ);

	RB_DrawElements();
}

/*
 =================
 RB_FogPass
 =================
*/
static void RB_FogPass (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	unsigned		sort = 0;
	material_t		*material, *oldMaterial = NULL;
	renderEntity_t	*entity, *oldEntity = NULL;
	meshType_t		type;
	qboolean		caps;
	qboolean		skip;
	vec3_t			color;
	int				i, j;

	if (!numMeshes)
		return;

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_FogPass ----------\n");

	// Render the fog volume to the stencil buffer
	RB_RenderStencilFogVolume();

	// Set the render function
	backEnd.renderBatch = RB_RenderFogPassInteraction;

	// Set the GL state
	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_EQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_LESS, 128, 255);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	GL_SelectTexture(0);
	GL_TexEnv(GL_MODULATE);
	GL_LoadIdentity(GL_TEXTURE);

	GL_Enable(GL_TEXTURE_GEN_S);
	GL_Enable(GL_TEXTURE_GEN_T);

	GL_TexGen(GL_S, GL_OBJECT_LINEAR);
	GL_TexGen(GL_T, GL_OBJECT_LINEAR);

	GL_SelectTexture(1);
	GL_TexEnv(GL_MODULATE);
	GL_LoadIdentity(GL_TEXTURE);

	GL_Enable(GL_TEXTURE_GEN_S);
	GL_Enable(GL_TEXTURE_GEN_T);

	GL_TexGen(GL_S, GL_OBJECT_LINEAR);
	GL_TexGen(GL_T, GL_OBJECT_LINEAR);

	// Run through the light stages
	for (i = 0, backEnd.lightStage = backEnd.lightMaterial->stages; i < backEnd.lightMaterial->numStages; i++, backEnd.lightStage++){
		if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->conditionRegister])
			continue;

		// Set up the color
		color[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]];
		color[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]];
		color[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]];

		qglColor3fv(color);

		// Set up the textures
		GL_SelectTexture(0);
		GL_EnableTexture(backEnd.lightStage->textureStage.texture->uploadTarget);
		GL_BindTexture(backEnd.lightStage->textureStage.texture);

		GL_SelectTexture(1);
		GL_EnableTexture(backEnd.lightMaterial->lightFalloffImage->uploadTarget);
		GL_BindTexture(backEnd.lightMaterial->lightFalloffImage);

		// Run through the meshes
		for (j = 0, mesh = meshes; j < numMeshes; j++, mesh++){
			if (mesh->sort != sort || r_skipBatching->integerValue){
				sort = mesh->sort;

				// Render the last batch
				RB_EndBatch();

				// Decompose sort
				R_DecomposeSort(sort, &material, &entity, &type, &caps);

				// Evaluate registers if needed
				if (material != oldMaterial || entity != oldEntity)
					R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

				// Skip if condition evaluated to false
				if (!material->expressionRegisters[material->conditionRegister]){
					skip = true;
					continue;
				}

				// Set the entity state if needed
				if (entity != oldEntity){
					RB_SetEntityState(entity);

					// Set the light state for this entity
					RB_SetLightState(entity, backEnd.light, backEnd.lightMaterial, backEnd.lightStage);
				}

				// Create a new batch
				RB_BeginBatch(material, entity, false, false);

				oldMaterial = material;
				oldEntity = entity;

				skip = false;
			}

			if (skip)
				continue;

			// Batch the mesh
			RB_BatchMesh(type, mesh->data);
		}

		// Render the last batch
		RB_EndBatch();
	}

	// Set the GL state
	GL_Enable(GL_CULL_FACE);
	GL_CullFace(GL_BACK);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 128, 255);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// Set the entity state
	RB_SetEntityState(tr.worldEntity);

	// Run through the light stages
	for (i = 0, backEnd.lightStage = backEnd.lightMaterial->stages; i < backEnd.lightMaterial->numStages; i++, backEnd.lightStage++){
		if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->conditionRegister])
			continue;

		// Set up the color
		color[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]];
		color[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]];
		color[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]];

		qglColor3fv(color);

		// Set up the textures
		GL_SelectTexture(0);
		GL_EnableTexture(backEnd.lightStage->textureStage.texture->uploadTarget);
		GL_BindTexture(backEnd.lightStage->textureStage.texture);

		GL_SelectTexture(1);
		GL_EnableTexture(backEnd.lightMaterial->lightFalloffImage->uploadTarget);
		GL_BindTexture(backEnd.lightMaterial->lightFalloffImage);

		// Set the light state for this entity
		RB_SetLightState(tr.worldEntity, backEnd.light, backEnd.lightMaterial, backEnd.lightStage);

		// Render the fog volume
		RB_RenderFogVolume();
	}

	// Restore the GL state
	GL_SelectTexture(1);
	GL_DisableTexture();

	GL_Disable(GL_TEXTURE_GEN_S);
	GL_Disable(GL_TEXTURE_GEN_T);

	GL_SelectTexture(0);

	GL_Disable(GL_TEXTURE_GEN_S);
	GL_Disable(GL_TEXTURE_GEN_T);
}

/*
 =================
 RB_RenderBlendLightInteraction
 =================
*/
static void RB_RenderBlendLightInteraction (void){

	RB_Deform();

	RB_UpdateVertexBuffer(backEnd.vertexBuffer, backEnd.vertices, backEnd.numVertices * sizeof(vertexArray_t));

	RB_SetMaterialState();

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_XYZ);

	RB_DrawElements();
}

/*
 =================
 RB_BlendLight
 =================
*/
static void RB_BlendLight (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	unsigned		sort = 0;
	material_t		*material, *oldMaterial = NULL;
	renderEntity_t	*entity, *oldEntity = NULL;
	meshType_t		type;
	qboolean		caps;
	qboolean		skip;
	vec4_t			color;
	int				i, j;

	if (!numMeshes)
		return;

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_BlendLight ----------\n");

	// Set the render function
	backEnd.renderBatch = RB_RenderBlendLightInteraction;

	// Set the GL state
	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_EQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 128, 255);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	GL_SelectTexture(0);
	GL_TexEnv(GL_MODULATE);
	GL_LoadIdentity(GL_TEXTURE);

	GL_Enable(GL_TEXTURE_GEN_S);
	GL_Enable(GL_TEXTURE_GEN_T);
	GL_Enable(GL_TEXTURE_GEN_Q);

	GL_TexGen(GL_S, GL_OBJECT_LINEAR);
	GL_TexGen(GL_T, GL_OBJECT_LINEAR);
	GL_TexGen(GL_Q, GL_OBJECT_LINEAR);

	GL_SelectTexture(1);
	GL_TexEnv(GL_MODULATE);
	GL_LoadIdentity(GL_TEXTURE);

	qglTexCoord2f(0.0, 0.5);

	GL_Enable(GL_TEXTURE_GEN_S);

	GL_TexGen(GL_S, GL_OBJECT_LINEAR);

	// Run through the light stages
	for (i = 0, backEnd.lightStage = backEnd.lightMaterial->stages; i < backEnd.lightMaterial->numStages; i++, backEnd.lightStage++){
		if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->conditionRegister])
			continue;

		// Set the GL state
		if (backEnd.lightStage->blend){
			if (backEnd.lightStage->blendSrc == GL_ZERO && backEnd.lightStage->blendDst == GL_ONE)
				continue;

			GL_Enable(GL_BLEND);
			GL_BlendFunc(backEnd.lightStage->blendSrc, backEnd.lightStage->blendDst);
		}
		else
			GL_Disable(GL_BLEND);

		// Set up the color
		color[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]];
		color[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]];
		color[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]];
		color[3] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[3]];

		qglColor4fv(color);

		// Set up the textures
		GL_SelectTexture(0);
		GL_EnableTexture(backEnd.lightStage->textureStage.texture->uploadTarget);
		GL_BindTexture(backEnd.lightStage->textureStage.texture);

		GL_SelectTexture(1);
		GL_EnableTexture(backEnd.lightMaterial->lightFalloffImage->uploadTarget);
		GL_BindTexture(backEnd.lightMaterial->lightFalloffImage);

		// Run through the meshes
		for (j = 0, mesh = meshes; j < numMeshes; j++, mesh++){
			if (mesh->sort != sort || r_skipBatching->integerValue){
				sort = mesh->sort;

				// Render the last batch
				RB_EndBatch();

				// Decompose sort
				R_DecomposeSort(sort, &material, &entity, &type, &caps);

				// Evaluate registers if needed
				if (material != oldMaterial || entity != oldEntity)
					R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

				// Skip if condition evaluated to false
				if (!material->expressionRegisters[material->conditionRegister]){
					skip = true;
					continue;
				}

				// Set the entity state if needed
				if (entity != oldEntity){
					RB_SetEntityState(entity);

					// Set the light state for this entity
					RB_SetLightState(entity, backEnd.light, backEnd.lightMaterial, backEnd.lightStage);
				}

				// Create a new batch
				RB_BeginBatch(material, entity, false, false);

				oldMaterial = material;
				oldEntity = entity;

				skip = false;
			}

			if (skip)
				continue;

			// Batch the mesh
			RB_BatchMesh(type, mesh->data);
		}

		// Render the last batch
		RB_EndBatch();
	}

	// Restore the GL state
	GL_SelectTexture(1);
	GL_DisableTexture();

	GL_Disable(GL_TEXTURE_GEN_S);

	GL_SelectTexture(0);

	GL_Disable(GL_TEXTURE_GEN_S);
	GL_Disable(GL_TEXTURE_GEN_T);
	GL_Disable(GL_TEXTURE_GEN_Q);
}

/*
 =================
 RB_FogAllLights
 =================
*/
void RB_FogAllLights (light_t *lights, int numLights){

	int		i;

	if (r_skipFogLights->integerValue)
		return;

	if (!numLights)
		return;

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_FogAllLights ----------\n");

	// Run through the lights
	for (i = 0, backEnd.light = lights; i < numLights; i++, backEnd.light++){
		// Set the light material
		backEnd.lightMaterial = backEnd.light->material;

		// Evaluate registers
		R_EvaluateRegisters(backEnd.lightMaterial, backEnd.time, backEnd.light->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightMaterial->conditionRegister])
			continue;

		// Set the scissor rect
		GL_Scissor(backEnd.light->scissorX, backEnd.light->scissorY, backEnd.light->scissorWidth, backEnd.light->scissorHeight);

		// Set the depth bounds
		if (glConfig.depthBoundsTest)
			GL_DepthBounds(backEnd.light->depthMin, backEnd.light->depthMax);

		// Render meshes with lighting
		if (backEnd.lightMaterial->fogLight)
			RB_FogPass(backEnd.light->interactions[0], backEnd.light->numInteractions[0]);
		else {
			if (r_skipBlendLights->integerValue)
				continue;

			RB_BlendLight(backEnd.light->interactions[0], backEnd.light->numInteractions[0]);
		}
	}

	// Restore the scissor rect
	GL_Scissor(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Restore the depth bounds
	if (glConfig.depthBoundsTest)
		GL_DepthBounds(0.0, 1.0);
}


/*
 =======================================================================

 2D RENDERING

 =======================================================================
*/


/*
 =================
 RB_RenderOverlayMaterial
 =================
*/
void RB_RenderOverlayMaterial (void){

	stage_t		*stage;
	int			i;

	if (r_logFile->integerValue)
		QGL_LogPrintf("----- RB_RenderOverlayMaterial ( %s ) -----\n", backEnd.material->name);

	// Capture current render if needed
	if (backEnd.material->sort == SORT_POST_PROCESS)
		RB_CaptureCurrentRender();

	RB_Deform();

	RB_UpdateVertexBuffer(backEnd.vertexBuffer, backEnd.vertices, backEnd.numVertices * sizeof(vertexArray_t));

	RB_SetMaterialState();

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_XYZ);

	for (i = 0, stage = backEnd.material->stages; i < backEnd.material->numStages; i++, stage++){
		if (!backEnd.material->expressionRegisters[stage->conditionRegister])
			continue;

		if (stage->blend){
			if (stage->blendSrc == GL_ZERO && stage->blendDst == GL_ONE)
				continue;
		}

		RB_SetStageState(stage);

		if (stage->programs){
			if (r_skipPrograms->integerValue)
				continue;

			RB_SetupProgramStage(&stage->programStage);

			RB_DrawElements();

			RB_CleanupProgramStage(&stage->programStage);
		}
		else {
			RB_SetupTextureStage(&stage->textureStage);
			RB_SetupColorStage(&stage->colorStage);

			RB_DrawElements();

			RB_CleanupColorStage(&stage->colorStage);
			RB_CleanupTextureStage(&stage->textureStage);
		}
	}
}
