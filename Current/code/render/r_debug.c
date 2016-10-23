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


// r_debug.c -- debug tools rendering for back-end


#include "r_local.h"


/*
 =================
 RB_ShowDepth
 =================
*/
static void RB_ShowDepth (void){

	float	*buffer;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	qglColor3f(1.0, 1.0, 1.0);

	// Read the depth buffer
	buffer = Z_Malloc(backEnd.viewport.width * backEnd.viewport.height * sizeof(float));

	if (r_frontBuffer->integerValue)
		qglReadBuffer(GL_FRONT);
	else
		qglReadBuffer(GL_BACK);

	qglReadPixels(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height, GL_DEPTH_COMPONENT, GL_FLOAT, buffer);

	// Set the raster position
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity();

	qglRasterPos2i(0, 0);

	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();

	// Draw the depth buffer
	qglDrawPixels(backEnd.viewport.width, backEnd.viewport.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

	Z_Free(buffer);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 =================
 RB_ShowOverdraw
 =================
*/
static void RB_ShowOverdraw (mesh_t *meshes, int numMeshes, qboolean ambient){

	mesh_t			*mesh;
	material_t		*material;
	renderEntity_t	*entity;
	meshType_t		type;
	qboolean		caps;
	index_t			*indices;
	vertexArray_t	*vertices;
	stage_t			*stage, *bumpStage, *diffuseStage, *specularStage;
	int				passes;
	int				i, j, n;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 0, 255);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_INCR);

	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

		// Batch the mesh
		RB_BatchMesh(type, mesh->data);

		// Count number of passes
		bumpStage = diffuseStage = specularStage = NULL;

		passes = 0;

		for (n = 0, stage = backEnd.material->stages; n < backEnd.material->numStages; n++, stage++){
			if (!backEnd.material->expressionRegisters[stage->conditionRegister])
				continue;

			switch (stage->lighting){
			case SL_AMBIENT:
				if (ambient)
					passes++;

				break;
			case SL_BUMP:
				if (bumpStage){
					if (!ambient)
						passes++;
				}

				bumpStage = stage;
				diffuseStage = NULL;
				specularStage = NULL;

				break;
			case SL_DIFFUSE:
				if (bumpStage && diffuseStage){
					if (!ambient)
						passes++;
				}

				diffuseStage = stage;

				break;
			case SL_SPECULAR:
				if (bumpStage && specularStage){
					if (!ambient)
						passes++;
				}

				specularStage = stage;

				break;
			}
		}

		if (bumpStage || diffuseStage || specularStage){
			if (!ambient)
				passes++;
		}

		// Add to overdraw
		vertices = backEnd.vertices;

		RB_Deform();

		for (n = 0; n < passes; n++){
			indices = backEnd.indices;

			qglBegin(GL_TRIANGLES);
			for (j = 0; j < backEnd.numIndices; j += 3){
				qglVertex3fv(vertices[indices[0]].xyz);
				qglVertex3fv(vertices[indices[1]].xyz);
				qglVertex3fv(vertices[indices[2]].xyz);

				indices += 3;
			}
			qglEnd();
		}

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 =================
 RB_ShowShadowCount
 =================
*/
static void RB_ShowShadowCount (mesh_t *meshes, int numMeshes){

	mesh_t				*mesh;
	material_t			*material;
	renderEntity_t		*entity;
	meshType_t			type;
	qboolean			caps;
	index_t				*indices;
	shadowVertexArray_t	*vertices;
	int					i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Enable(GL_POLYGON_OFFSET_FILL);
	GL_PolygonOffset(r_shadowPolygonFactor->floatValue, r_shadowPolygonOffset->floatValue);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 0, 255);
	GL_StencilOp(GL_KEEP, GL_INCR, GL_INCR);

	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Skip static or dynamic shadows as needed
		if (entity == tr.worldEntity){
			if (r_showShadowCount->integerValue == 3)
				continue;
		}
		else {
			if (r_showShadowCount->integerValue == 2)
				continue;
		}

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the light state for this entity
		RB_SetLightState(entity, backEnd.light, NULL, NULL);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = true;
		backEnd.shadowCaps = caps;

		// Batch the mesh
		RB_BatchMeshShadow(type, mesh->data);

		// Render the shadow volumes
		indices = backEnd.shadowIndices;
		vertices = backEnd.shadowVertices;

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			qglVertex4fv(vertices[indices[0]].xyzw);
			qglVertex4fv(vertices[indices[1]].xyzw);
			qglVertex4fv(vertices[indices[2]].xyzw);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 =================
 RB_ShowLightCount
 =================
*/
static void RB_ShowLightCount (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	material_t		*material;
	renderEntity_t	*entity;
	meshType_t		type;
	qboolean		caps;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_EQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 0, 255);

	if (r_showLightCount->integerValue == 1)
		GL_StencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	else
		GL_StencilOp(GL_KEEP, GL_INCR, GL_INCR);

	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

		// Batch the mesh
		RB_BatchMesh(type, mesh->data);

		// Add to light count
		indices = backEnd.indices;
		vertices = backEnd.vertices;

		RB_Deform();

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			qglVertex3fv(vertices[indices[0]].xyz);
			qglVertex3fv(vertices[indices[1]].xyz);
			qglVertex3fv(vertices[indices[2]].xyz);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 =================
 RB_ShowLightBounds
 =================
*/
static void RB_ShowLightBounds (void){

	int		cornerIndices[6][4] = {{3, 2, 6, 7}, {0, 1, 5, 4}, {2, 3, 1, 0}, {4, 5, 7, 6}, {1, 3, 7, 5}, {2, 0, 4, 6}};
	int		i;

	// Set the GL state
	GL_LoadMatrix(GL_MODELVIEW, backEnd.viewParms.worldMatrix);

	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	GL_DepthRange(0.0, 1.0);

	if (r_showLightBounds->integerValue != 2){
		// Set the GL state
		GL_Enable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);

		GL_Enable(GL_BLEND);
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		GL_Enable(GL_DEPTH_TEST);
		GL_DepthFunc(GL_LEQUAL);

		qglColor4f(backEnd.light->materialParms[0], backEnd.light->materialParms[1], backEnd.light->materialParms[2], 0.5);

		// Draw it
		qglBegin(GL_QUADS);
		for (i = 0; i < 6; i++){
			qglVertex3fv(backEnd.light->corners[cornerIndices[i][0]]);
			qglVertex3fv(backEnd.light->corners[cornerIndices[i][1]]);
			qglVertex3fv(backEnd.light->corners[cornerIndices[i][2]]);
			qglVertex3fv(backEnd.light->corners[cornerIndices[i][3]]);
		}
		qglEnd();
	}

	if (r_showLightBounds->integerValue != 1){
		// Set the GL state
		GL_Disable(GL_POLYGON_OFFSET_FILL);

		GL_Disable(GL_BLEND);

		GL_Disable(GL_DEPTH_TEST);

		qglColor3f(backEnd.light->materialParms[0], backEnd.light->materialParms[1], backEnd.light->materialParms[2]);

		// Draw it
		qglBegin(GL_LINE_LOOP);
		qglVertex3fv(backEnd.light->corners[0]);
		qglVertex3fv(backEnd.light->corners[2]);
		qglVertex3fv(backEnd.light->corners[3]);
		qglVertex3fv(backEnd.light->corners[1]);
		qglEnd();

		qglBegin(GL_LINE_LOOP);
		qglVertex3fv(backEnd.light->corners[4]);
		qglVertex3fv(backEnd.light->corners[6]);
		qglVertex3fv(backEnd.light->corners[7]);
		qglVertex3fv(backEnd.light->corners[5]);
		qglEnd();

		qglBegin(GL_LINES);
		qglVertex3fv(backEnd.light->corners[0]);
		qglVertex3fv(backEnd.light->corners[4]);
		qglVertex3fv(backEnd.light->corners[1]);
		qglVertex3fv(backEnd.light->corners[5]);
		qglVertex3fv(backEnd.light->corners[2]);
		qglVertex3fv(backEnd.light->corners[6]);
		qglVertex3fv(backEnd.light->corners[3]);
		qglVertex3fv(backEnd.light->corners[7]);
		qglEnd();
	}

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 =================
 RB_ShowLightScissors
 =================
*/
static void RB_ShowLightScissors (void){

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	qglColor3f(backEnd.light->materialParms[0], backEnd.light->materialParms[1], backEnd.light->materialParms[2]);

	// Draw it
	qglBegin(GL_LINE_LOOP);
	qglVertex2f(backEnd.light->scissorX, backEnd.light->scissorY);
	qglVertex2f(backEnd.light->scissorX + backEnd.light->scissorWidth, backEnd.light->scissorY);
	qglVertex2f(backEnd.light->scissorX + backEnd.light->scissorWidth, backEnd.light->scissorY + backEnd.light->scissorHeight);
	qglVertex2f(backEnd.light->scissorX, backEnd.light->scissorY + backEnd.light->scissorHeight);
	qglEnd();

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 =================
 RB_ShowShadowTris
 =================
*/
static void RB_ShowShadowTris (mesh_t *meshes, int numMeshes){

	mesh_t				*mesh;
	material_t			*material;
	renderEntity_t		*entity;
	meshType_t			type;
	qboolean			caps;
	index_t				*indices;
	shadowVertexArray_t	*vertices;
	int					i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	if (r_showShadowTris->integerValue == 1){
		GL_Enable(GL_DEPTH_TEST);
		GL_DepthFunc(GL_LEQUAL);
	}
	else
		GL_Disable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	qglColor3f(1.0, 0.0, 1.0);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (r_showShadowTris->integerValue == 1){
		GL_Enable(GL_POLYGON_OFFSET_LINE);
		GL_PolygonOffset(-1.0, -2.0);
	}

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the light state for this entity
		RB_SetLightState(entity, backEnd.light, NULL, NULL);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = true;
		backEnd.shadowCaps = caps;

		// Batch the mesh
		RB_BatchMeshShadow(type, mesh->data);

		// Render the shadow tris
		indices = backEnd.shadowIndices;
		vertices = backEnd.shadowVertices;

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			qglVertex4fv(vertices[indices[0]].xyzw);
			qglVertex4fv(vertices[indices[1]].xyzw);
			qglVertex4fv(vertices[indices[2]].xyzw);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}

	// Restore the GL state
	if (r_showShadowTris->integerValue == 1)
		GL_Disable(GL_POLYGON_OFFSET_LINE);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/*
 =================
 RB_ShowShadowVolumes
 =================
*/
static void RB_ShowShadowVolumes (mesh_t *meshes, int numMeshes){

	mesh_t				*mesh;
	material_t			*material;
	renderEntity_t		*entity;
	meshType_t			type;
	qboolean			caps;
	index_t				*indices;
	shadowVertexArray_t	*vertices;
	int					i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Enable(GL_POLYGON_OFFSET_FILL);
	GL_PolygonOffset(r_shadowPolygonFactor->floatValue, r_shadowPolygonOffset->floatValue);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	qglColor4f(0.0, 1.0, 1.0, 0.5);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the light state for this entity
		RB_SetLightState(entity, backEnd.light, NULL, NULL);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = true;
		backEnd.shadowCaps = caps;

		// Batch the mesh
		RB_BatchMeshShadow(type, mesh->data);

		// Render the shadow volumes
		indices = backEnd.shadowIndices;
		vertices = backEnd.shadowVertices;

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			qglVertex4fv(vertices[indices[0]].xyzw);
			qglVertex4fv(vertices[indices[1]].xyzw);
			qglVertex4fv(vertices[indices[2]].xyzw);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 =================
 RB_ShowShadowSilhouettes
 =================
*/
static void RB_ShowShadowSilhouettes (mesh_t *meshes, int numMeshes){

	mesh_t				*mesh;
	material_t			*material;
	renderEntity_t		*entity;
	meshType_t			type;
	qboolean			caps;
	index_t				*indices;
	shadowVertexArray_t	*vertices;
	int					i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	qglColor3f(1.0, 1.0, 0.0);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the light state for this entity
		RB_SetLightState(entity, backEnd.light, NULL, NULL);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = true;
		backEnd.shadowCaps = false;

		// Batch the mesh
		RB_BatchMeshShadow(type, mesh->data);

		// Render the shadow silhouettes
		indices = backEnd.shadowIndices;
		vertices = backEnd.shadowVertices;

		qglBegin(GL_LINES);
		for (j = 0; j < backEnd.numIndices; j += 6){
			qglVertex4fv(vertices[indices[0]].xyzw);
			qglVertex4fv(vertices[indices[1]].xyzw);

			indices += 6;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 =================
 RB_ShowVertexColors
 =================
*/
static void RB_ShowVertexColors (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	material_t		*material;
	renderEntity_t	*entity;
	meshType_t		type;
	qboolean		caps;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_Enable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

		// Batch the mesh
		RB_BatchMesh(type, mesh->data);

		// Render the vertex colors
		indices = backEnd.indices;
		vertices = backEnd.vertices;

		RB_Deform();

		if (backEnd.material->polygonOffset){
			GL_Enable(GL_POLYGON_OFFSET_FILL);
			GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);
		}
		else
			GL_Disable(GL_POLYGON_OFFSET_FILL);

		if (backEnd.material->coverage == MC_TRANSLUCENT)
			GL_DepthFunc(GL_LEQUAL);
		else
			GL_DepthFunc(GL_EQUAL);

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			qglColor4ubv(vertices[indices[0]].color);
			qglVertex3fv(vertices[indices[0]].xyz);

			qglColor4ubv(vertices[indices[1]].color);
			qglVertex3fv(vertices[indices[1]].xyz);

			qglColor4ubv(vertices[indices[2]].color);
			qglVertex3fv(vertices[indices[2]].xyz);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 =================
 RB_ShowTangentSpace
 =================
*/
static void RB_ShowTangentSpace (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	material_t		*material;
	renderEntity_t	*entity;
	meshType_t		type;
	qboolean		caps;
	index_t			*indices;
	vertexArray_t	*vertices;
	vec3_t			color[3];
	int				i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_Enable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Skip if it doesn't have tangent vectors
		if (type != MESH_SURFACE && type != MESH_DECORATION && type != MESH_ALIASMODEL)
			continue;

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

		// Batch the mesh
		RB_BatchMesh(type, mesh->data);

		// Render the tangent space
		indices = backEnd.indices;
		vertices = backEnd.vertices;

		RB_Deform();

		if (backEnd.material->polygonOffset){
			GL_Enable(GL_POLYGON_OFFSET_FILL);
			GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);
		}
		else
			GL_Disable(GL_POLYGON_OFFSET_FILL);

		if (backEnd.material->coverage == MC_TRANSLUCENT)
			GL_DepthFunc(GL_LEQUAL);
		else
			GL_DepthFunc(GL_EQUAL);

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			// Select color from tangent vector
			switch (r_showTangentSpace->integerValue){
			case 1:
				VectorScale(vertices[indices[0]].tangents[0], 0.5, color[0]);
				VectorScale(vertices[indices[1]].tangents[0], 0.5, color[1]);
				VectorScale(vertices[indices[2]].tangents[0], 0.5, color[2]);

				break;
			case 2:
				VectorScale(vertices[indices[0]].tangents[1], 0.5, color[0]);
				VectorScale(vertices[indices[1]].tangents[1], 0.5, color[1]);
				VectorScale(vertices[indices[2]].tangents[1], 0.5, color[2]);

				break;
			default:
				VectorScale(vertices[indices[0]].normal, 0.5, color[0]);
				VectorScale(vertices[indices[1]].normal, 0.5, color[1]);
				VectorScale(vertices[indices[2]].normal, 0.5, color[2]);

				break;
			}

			qglColor4f(0.5 + color[0][0], 0.5 + color[0][1], 0.5 + color[0][2], 0.5);
			qglVertex3fv(vertices[indices[0]].xyz);
			qglColor4f(0.5 + color[1][0], 0.5 + color[1][1], 0.5 + color[1][2], 0.5);
			qglVertex3fv(vertices[indices[1]].xyz);
			qglColor4f(0.5 + color[2][0], 0.5 + color[2][1], 0.5 + color[2][2], 0.5);
			qglVertex3fv(vertices[indices[2]].xyz);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 =================
 RB_ShowTris
 =================
*/
static void RB_ShowTris (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	material_t		*material;
	renderEntity_t	*entity;
	meshType_t		type;
	qboolean		caps;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	if (r_showTris->integerValue == 1){
		GL_Enable(GL_DEPTH_TEST);
		GL_DepthFunc(GL_LEQUAL);
	}
	else
		GL_Disable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	qglColor3f(1.0, 1.0, 1.0);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (r_showTris->integerValue == 1){
		GL_Enable(GL_POLYGON_OFFSET_LINE);
		GL_PolygonOffset(-1.0, -2.0);
	}

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

		// Batch the mesh
		RB_BatchMesh(type, mesh->data);

		// Render the tris
		indices = backEnd.indices;
		vertices = backEnd.vertices;

		RB_Deform();

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			qglVertex3fv(vertices[indices[0]].xyz);
			qglVertex3fv(vertices[indices[1]].xyz);
			qglVertex3fv(vertices[indices[2]].xyz);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}

	// Restore the GL state
	if (r_showTris->integerValue == 1)
		GL_Disable(GL_POLYGON_OFFSET_LINE);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/*
 =================
 RB_ShowNormals
 =================
*/
static void RB_ShowNormals (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	material_t		*material;
	renderEntity_t	*entity;
	meshType_t		type;
	qboolean		caps;
	vertexArray_t	*vertices;
	vec3_t			point;
	int				i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

		// Batch the mesh
		RB_BatchMesh(type, mesh->data);

		// Render the normals
		vertices = backEnd.vertices;

		RB_Deform();

		qglBegin(GL_LINES);
		for (j = 0; j < backEnd.numVertices; j++){
			if (type == MESH_SURFACE || type == MESH_DECORATION || type == MESH_ALIASMODEL){
				// First tangent
				VectorMA(vertices->xyz, r_showNormals->floatValue, vertices->tangents[0], point);

				qglColor3f(1.0, 0.0, 0.0);
				qglVertex3fv(vertices->xyz);
				qglVertex3fv(point);

				// Second tangent
				VectorMA(vertices->xyz, r_showNormals->floatValue, vertices->tangents[1], point);

				qglColor3f(0.0, 1.0, 0.0);
				qglVertex3fv(vertices->xyz);
				qglVertex3fv(point);
			}

			// Normal
			VectorMA(vertices->xyz, r_showNormals->floatValue, vertices->normal, point);

			qglColor3f(0.0, 0.0, 1.0);
			qglVertex3fv(vertices->xyz);
			qglVertex3fv(point);

			vertices++;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 =================
 RB_ShowTextureVectors
 =================
*/
static void RB_ShowTextureVectors (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	material_t		*material;
	renderEntity_t	*entity;
	meshType_t		type;
	qboolean		caps;
	index_t			*indices;
	vertexArray_t	*vertices;
	vec3_t			center, tangents[2], point;
	int				i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Skip if it doesn't have tangent vectors
		if (type != MESH_SURFACE && type != MESH_DECORATION && type != MESH_ALIASMODEL)
			continue;

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

		// Batch the mesh
		RB_BatchMesh(type, mesh->data);

		// Render the texture vectors
		indices = backEnd.indices;
		vertices = backEnd.vertices;

		RB_Deform();

		qglBegin(GL_LINES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			// Compute center of triangle
			center[0] = (vertices[indices[0]].xyz[0] + vertices[indices[1]].xyz[0] + vertices[indices[2]].xyz[0]) * (1.0/3);
			center[1] = (vertices[indices[0]].xyz[1] + vertices[indices[1]].xyz[1] + vertices[indices[2]].xyz[1]) * (1.0/3);
			center[2] = (vertices[indices[0]].xyz[2] + vertices[indices[1]].xyz[2] + vertices[indices[2]].xyz[2]) * (1.0/3);

			// First tangent
			tangents[0][0] = (vertices[indices[0]].tangents[0][0] + vertices[indices[1]].tangents[0][0] + vertices[indices[2]].tangents[0][0]);
			tangents[0][1] = (vertices[indices[0]].tangents[0][1] + vertices[indices[1]].tangents[0][1] + vertices[indices[2]].tangents[0][1]);
			tangents[0][2] = (vertices[indices[0]].tangents[0][2] + vertices[indices[1]].tangents[0][2] + vertices[indices[2]].tangents[0][2]);

			VectorNormalizeFast(tangents[0]);
			VectorMA(center, r_showTextureVectors->floatValue, tangents[0], point);

			qglColor3f(1.0, 0.0, 0.0);
			qglVertex3fv(center);
			qglVertex3fv(point);

			// Second tangent
			tangents[1][0] = (vertices[indices[0]].tangents[1][0] + vertices[indices[1]].tangents[1][0] + vertices[indices[2]].tangents[1][0]);
			tangents[1][1] = (vertices[indices[0]].tangents[1][1] + vertices[indices[1]].tangents[1][1] + vertices[indices[2]].tangents[1][1]);
			tangents[1][2] = (vertices[indices[0]].tangents[1][2] + vertices[indices[1]].tangents[1][2] + vertices[indices[2]].tangents[1][2]);

			VectorNormalizeFast(tangents[1]);
			VectorMA(center, r_showTextureVectors->floatValue, tangents[1], point);

			qglColor3f(0.0, 1.0, 0.0);
			qglVertex3fv(center);
			qglVertex3fv(point);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 =================
 RB_ShowModelBounds
 =================
*/
static void RB_ShowModelBounds (mesh_t *meshes, int numMeshes){

	mesh_t			*mesh;
	material_t		*material;
	renderEntity_t	*entity;
	meshType_t		type;
	qboolean		caps;
	int				cornerIndices[6][4] = {{3, 2, 6, 7}, {0, 1, 5, 4}, {2, 3, 1, 0}, {4, 5, 7, 6}, {1, 3, 7, 5}, {2, 0, 4, 6}};
	model_t			*model;
	mdl_t			*alias;
	mdlFrame_t		*curFrame, *oldFrame;
	vec3_t			mins, maxs, corners[8];
	int				i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Decompose sort
		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		// Skip if world entity
		if (entity == tr.worldEntity)
			continue;

		// Skip if not surface or alias model
		if (type != MESH_SURFACE && type != MESH_ALIASMODEL)
			continue;

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, entity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			continue;

		// Set the entity state
		RB_SetEntityState(entity);

		// Set the batch state
		backEnd.material = material;
		backEnd.entity = entity;

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

		// Render the model bounds
		model = backEnd.entity->model;

		if (model->modelType == MODEL_BSP){
			// Compute the corners of the bounding volume
			for (j = 0; j < 8; j++){
				corners[j][0] = (j & 1) ? model->mins[0] : model->maxs[0];
				corners[j][1] = (j & 2) ? model->mins[1] : model->maxs[1];
				corners[j][2] = (j & 4) ? model->mins[2] : model->maxs[2];
			}
		}
		else {
			alias = model->alias;

			// Compute axially aligned mins and maxs
			curFrame = alias->frames + backEnd.entity->frame;
			oldFrame = alias->frames + backEnd.entity->oldFrame;

			if (curFrame == oldFrame){
				VectorCopy(curFrame->mins, mins);
				VectorCopy(curFrame->maxs, maxs);
			}
			else {
				VectorMin(curFrame->mins, oldFrame->mins, mins);
				VectorMax(curFrame->maxs, oldFrame->maxs, maxs);
			}

			// Compute the corners of the bounding volume
			for (j = 0; j < 8; j++){
				corners[j][0] = (j & 1) ? mins[0] : maxs[0];
				corners[j][1] = (j & 2) ? mins[1] : maxs[1];
				corners[j][2] = (j & 4) ? mins[2] : maxs[2];
			}
		}

		if (r_showModelBounds->integerValue != 2){
			// Set the GL state
			GL_Enable(GL_POLYGON_OFFSET_FILL);
			GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);

			GL_Enable(GL_BLEND);
			GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			GL_Enable(GL_DEPTH_TEST);
			GL_DepthFunc(GL_LEQUAL);

			qglColor4f(1.0, 1.0, 1.0, 0.5);

			// Draw it
			qglBegin(GL_QUADS);
			for (j = 0; j < 6; j++){
				qglVertex3fv(corners[cornerIndices[j][0]]);
				qglVertex3fv(corners[cornerIndices[j][1]]);
				qglVertex3fv(corners[cornerIndices[j][2]]);
				qglVertex3fv(corners[cornerIndices[j][3]]);
			}
			qglEnd();
		}

		if (r_showModelBounds->integerValue != 1){
			// Set the GL state
			GL_Disable(GL_POLYGON_OFFSET_FILL);

			GL_Disable(GL_BLEND);

			GL_Disable(GL_DEPTH_TEST);

			qglColor3f(1.0, 1.0, 1.0);

			// Draw it
			qglBegin(GL_LINE_LOOP);
			qglVertex3fv(corners[0]);
			qglVertex3fv(corners[2]);
			qglVertex3fv(corners[3]);
			qglVertex3fv(corners[1]);
			qglEnd();

			qglBegin(GL_LINE_LOOP);
			qglVertex3fv(corners[4]);
			qglVertex3fv(corners[6]);
			qglVertex3fv(corners[7]);
			qglVertex3fv(corners[5]);
			qglEnd();

			qglBegin(GL_LINES);
			qglVertex3fv(corners[0]);
			qglVertex3fv(corners[4]);
			qglVertex3fv(corners[1]);
			qglVertex3fv(corners[5]);
			qglVertex3fv(corners[2]);
			qglVertex3fv(corners[6]);
			qglVertex3fv(corners[3]);
			qglVertex3fv(corners[7]);
			qglEnd();
		}

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();
	}
}

/*
 =================
 RB_ShowStencil
 =================
*/
static float RB_ShowStencil (void){

	byte	*buffer;
	int		overdraw = 0;
	int		i;

	// Set the GL state
	GL_LoadIdentity(GL_MODELVIEW);

	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_DEPTH_TEST);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	GL_DepthRange(0.0, 1.0);

	// Draw fullscreen quads
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();

	for (i = 0; i <= 7; i++){
		if (i == 7)
			GL_StencilFunc(GL_LEQUAL, i, 255);
		else
			GL_StencilFunc(GL_EQUAL, i, 255);

		qglColor4ubv(colorTable[i]);

		qglBegin(GL_QUADS);
		qglVertex3f(-1.0, -1.0, -1.0);
		qglVertex3f( 1.0, -1.0, -1.0);
		qglVertex3f( 1.0,  1.0, -1.0);
		qglVertex3f(-1.0,  1.0, -1.0);
		qglEnd();
	}

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();

	// Read the stencil buffer
	buffer = Z_Malloc(backEnd.viewport.width * backEnd.viewport.height);

	if (r_frontBuffer->integerValue)
		qglReadBuffer(GL_FRONT);
	else
		qglReadBuffer(GL_BACK);

	qglReadPixels(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, buffer);

	// Measure overdraw
	for (i = 0; i < backEnd.viewport.width * backEnd.viewport.height; i++)
		overdraw += buffer[i];

	Z_Free(buffer);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	return overdraw / (float)(backEnd.viewport.width * backEnd.viewport.height);
}

/*
 =================
 RB_RenderDebugTools
 =================
*/
void RB_RenderDebugTools (const renderViewCommand_t *cmd){

	int		i, j;

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_RenderDebugTools ----------\n");

	if (r_showDepth->integerValue)
		RB_ShowDepth();

	if (r_showOverdraw->integerValue){
		// Clear the stencil buffer
		GL_StencilMask(255);

		qglClearStencil(0);
		qglClear(GL_STENCIL_BUFFER_BIT);

		if (r_showOverdraw->integerValue != 2){
			RB_ShowOverdraw(cmd->meshes, cmd->numMeshes, true);
			RB_ShowOverdraw(cmd->postProcessMeshes, cmd->numPostProcessMeshes, true);
		}

		if (r_showOverdraw->integerValue != 1){
			// Run through the lights
			for (i = 0, backEnd.light = cmd->lights; i < cmd->numLights; i++, backEnd.light++){
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

				// Run through the light stages
				for (j = 0, backEnd.lightStage = backEnd.lightMaterial->stages; j < backEnd.lightMaterial->numStages; j++, backEnd.lightStage++){
					if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->conditionRegister])
						continue;

					// Run through the meshes
					RB_ShowOverdraw(backEnd.light->interactions[0], backEnd.light->numInteractions[0], false);
					RB_ShowOverdraw(backEnd.light->interactions[1], backEnd.light->numInteractions[1], false);
				}
			}

			// Run through the lights
			for (i = 0, backEnd.light = cmd->fogLights; i < cmd->numFogLights; i++, backEnd.light++){
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

				// Run through the light stages
				for (j = 0, backEnd.lightStage = backEnd.lightMaterial->stages; j < backEnd.lightMaterial->numStages; j++, backEnd.lightStage++){
					if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->conditionRegister])
						continue;

					// Run through the meshes
					RB_ShowOverdraw(backEnd.light->interactions[0], backEnd.light->numInteractions[0], false);
				}
			}

			// Restore the scissor rect
			GL_Scissor(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

			// Restore the depth bounds
			if (glConfig.depthBoundsTest)
				GL_DepthBounds(0.0, 1.0);
		}

		// Draw the stencil buffer contents
		tr.pc.overdraw += RB_ShowStencil();
	}

	if (r_showShadowCount->integerValue){
		// Clear the stencil buffer
		GL_StencilMask(255);

		qglClearStencil(0);
		qglClear(GL_STENCIL_BUFFER_BIT);

		// Run through the lights
		for (i = 0, backEnd.light = cmd->lights; i < cmd->numLights; i++, backEnd.light++){
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

			// Run through the meshes
			RB_ShowShadowCount(backEnd.light->shadows[0], backEnd.light->numShadows[0]);
			RB_ShowShadowCount(backEnd.light->shadows[1], backEnd.light->numShadows[1]);
		}

		// Restore the scissor rect
		GL_Scissor(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

		// Restore the depth bounds
		if (glConfig.depthBoundsTest)
			GL_DepthBounds(0.0, 1.0);

		// Draw the stencil buffer contents
		tr.pc.overdrawShadows += RB_ShowStencil();
	}

	if (r_showLightCount->integerValue){
		// Clear the stencil buffer
		GL_StencilMask(255);

		qglClearStencil(0);
		qglClear(GL_STENCIL_BUFFER_BIT);

		// Run through the lights
		for (i = 0, backEnd.light = cmd->lights; i < cmd->numLights; i++, backEnd.light++){
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

			// Run through the meshes
			RB_ShowLightCount(backEnd.light->interactions[0], backEnd.light->numInteractions[0]);
			RB_ShowLightCount(backEnd.light->interactions[1], backEnd.light->numInteractions[1]);
		}

		// Run through the lights
		for (i = 0, backEnd.light = cmd->fogLights; i < cmd->numFogLights; i++, backEnd.light++){
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

			// Run through the meshes
			RB_ShowLightCount(backEnd.light->interactions[0], backEnd.light->numInteractions[0]);
		}

		// Restore the scissor rect
		GL_Scissor(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

		// Restore the depth bounds
		if (glConfig.depthBoundsTest)
			GL_DepthBounds(0.0, 1.0);

		// Draw the stencil buffer contents
		tr.pc.overdrawLights += RB_ShowStencil();
	}

	if (r_showLightBounds->integerValue){
		// Run through the lights
		for (i = 0, backEnd.light = cmd->lights; i < cmd->numLights; i++, backEnd.light++){
			// Set the light material
			backEnd.lightMaterial = backEnd.light->material;

			// Evaluate registers
			R_EvaluateRegisters(backEnd.lightMaterial, backEnd.time, backEnd.light->materialParms, backEnd.materialParms);

			// Skip if condition evaluated to false
			if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightMaterial->conditionRegister])
				continue;

			// Draw the light bounds
			RB_ShowLightBounds();
		}

		// Run through the lights
		for (i = 0, backEnd.light = cmd->fogLights; i < cmd->numFogLights; i++, backEnd.light++){
			// Set the light material
			backEnd.lightMaterial = backEnd.light->material;

			// Evaluate registers
			R_EvaluateRegisters(backEnd.lightMaterial, backEnd.time, backEnd.light->materialParms, backEnd.materialParms);

			// Skip if condition evaluated to false
			if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightMaterial->conditionRegister])
				continue;

			// Draw the light bounds
			RB_ShowLightBounds();
		}
	}

	if (r_showLightScissors->integerValue){
		// Set the GL state
		GL_LoadIdentity(GL_MODELVIEW);

		GL_DepthRange(0.0, 1.0);

		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();
		qglOrtho(backEnd.viewport.x, backEnd.viewport.x + backEnd.viewport.width, backEnd.viewport.y, backEnd.viewport.y + backEnd.viewport.height, -1.0, 1.0);

		// Run through the lights
		for (i = 0, backEnd.light = cmd->lights; i < cmd->numLights; i++, backEnd.light++){
			// Set the light material
			backEnd.lightMaterial = backEnd.light->material;

			// Evaluate registers
			R_EvaluateRegisters(backEnd.lightMaterial, backEnd.time, backEnd.light->materialParms, backEnd.materialParms);

			// Skip if condition evaluated to false
			if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightMaterial->conditionRegister])
				continue;

			// Draw the light scissors
			RB_ShowLightScissors();
		}

		// Run through the lights
		for (i = 0, backEnd.light = cmd->fogLights; i < cmd->numFogLights; i++, backEnd.light++){
			// Set the light material
			backEnd.lightMaterial = backEnd.light->material;

			// Evaluate registers
			R_EvaluateRegisters(backEnd.lightMaterial, backEnd.time, backEnd.light->materialParms, backEnd.materialParms);

			// Skip if condition evaluated to false
			if (!backEnd.lightMaterial->expressionRegisters[backEnd.lightMaterial->conditionRegister])
				continue;

			// Draw the light scissors
			RB_ShowLightScissors();
		}

		// Restore the GL state
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
	}

	if (r_showShadowTris->integerValue || r_showShadowVolumes->integerValue || r_showShadowSilhouettes->integerValue){
		// Run through the lights
		for (i = 0, backEnd.light = cmd->lights; i < cmd->numLights; i++, backEnd.light++){
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

			// Run through the meshes
			if (r_showShadowTris->integerValue){
				RB_ShowShadowTris(backEnd.light->shadows[0], backEnd.light->numShadows[0]);
				RB_ShowShadowTris(backEnd.light->shadows[1], backEnd.light->numShadows[1]);
			}

			if (r_showShadowVolumes->integerValue){
				RB_ShowShadowVolumes(backEnd.light->shadows[0], backEnd.light->numShadows[0]);
				RB_ShowShadowVolumes(backEnd.light->shadows[1], backEnd.light->numShadows[1]);
			}

			if (r_showShadowSilhouettes->integerValue){
				RB_ShowShadowSilhouettes(backEnd.light->shadows[0], backEnd.light->numShadows[0]);
				RB_ShowShadowSilhouettes(backEnd.light->shadows[1], backEnd.light->numShadows[1]);
			}
		}

		// Restore the scissor rect
		GL_Scissor(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

		// Restore the depth bounds
		if (glConfig.depthBoundsTest)
			GL_DepthBounds(0.0, 1.0);
	}

	if (r_showVertexColors->integerValue){
		RB_ShowVertexColors(cmd->meshes, cmd->numMeshes);
		RB_ShowVertexColors(cmd->postProcessMeshes, cmd->numPostProcessMeshes);
	}

	if (r_showTangentSpace->integerValue){
		RB_ShowTangentSpace(cmd->meshes, cmd->numMeshes);
		RB_ShowTangentSpace(cmd->postProcessMeshes, cmd->numPostProcessMeshes);
	}

	if (r_showTris->integerValue){
		RB_ShowTris(cmd->meshes, cmd->numMeshes);
		RB_ShowTris(cmd->postProcessMeshes, cmd->numPostProcessMeshes);
	}

	if (r_showNormals->floatValue){
		RB_ShowNormals(cmd->meshes, cmd->numMeshes);
		RB_ShowNormals(cmd->postProcessMeshes, cmd->numPostProcessMeshes);
	}

	if (r_showTextureVectors->floatValue){
		RB_ShowTextureVectors(cmd->meshes, cmd->numMeshes);
		RB_ShowTextureVectors(cmd->postProcessMeshes, cmd->numPostProcessMeshes);
	}

	if (r_showModelBounds->integerValue){
		RB_ShowModelBounds(cmd->meshes, cmd->numMeshes);
		RB_ShowModelBounds(cmd->postProcessMeshes, cmd->numPostProcessMeshes);
	}

	// Draw lights for the light editor if active
	if (backEnd.viewParms.primaryView){
		if (backEnd.viewParms.subview == SUBVIEW_NONE)
			R_DrawEditorLights();
	}
}
