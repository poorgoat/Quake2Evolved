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


// r_interaction.c -- interaction rendering for back-end


#include "r_local.h"


/*
 =======================================================================

 STANDARD ARB PATH (2 OR 3 PASSES, NO PROGRAMS, NO SPECULAR)

 =======================================================================
*/


/*
 =================
 RB_ARB_RenderInteraction
 =================
*/
void RB_ARB_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage){

	vec3_t		lightVectorArray[MAX_VERTICES];
	texture_t	*bumpTexture, *diffuseTexture;
	mat4_t		bumpMatrix, diffuseMatrix;
	vec3_t		diffuseColor;
	vec3_t		lightVector;
	int			i;

	if (r_skipBump->integerValue)
		bumpStage = NULL;
	if (r_skipDiffuse->integerValue)
		diffuseStage = NULL;

	if (r_fastInteractions->integerValue)
		bumpStage = NULL;

	// Set up the bump texture and matrix
	if (bumpStage){
		bumpTexture = bumpStage->textureStage.texture;

		RB_MultiplyTextureMatrix(backEnd.material, &bumpStage->textureStage, bumpMatrix);
	}
	else {
		bumpTexture = tr.flatTexture;

		Matrix4_Identity(bumpMatrix);
	}

	// Set up the diffuse texture, matrix, and color
	if (diffuseStage){
		diffuseTexture = diffuseStage->textureStage.texture;

		RB_MultiplyTextureMatrix(backEnd.material, &diffuseStage->textureStage, diffuseMatrix);

		diffuseColor[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]];
		diffuseColor[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]];
		diffuseColor[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]];

		diffuseColor[0] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[0]];
		diffuseColor[1] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[1]];
		diffuseColor[2] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[2]];
	}
	else {
		diffuseTexture = tr.whiteTexture;

		Matrix4_Identity(diffuseMatrix);

		diffuseColor[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]];
		diffuseColor[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]];
		diffuseColor[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]];
	}

	// Set up the arrays
	qglTexCoordPointer(2, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_ST);

	// Set up the light vector array
	if (backEnd.lightMaterial->ambientLight){
		for (i = 0; i < backEnd.numVertices; i++){
			lightVectorArray[i][0] = 0.0;
			lightVectorArray[i][1] = 0.0;
			lightVectorArray[i][2] = 1.0;
		}
	}
	else {
		for (i = 0; i < backEnd.numVertices; i++){
			VectorSubtract(backEnd.localParms.lightOrigin, backEnd.vertices[i].xyz, lightVector);

			lightVectorArray[i][0] = DotProduct(lightVector, backEnd.vertices[i].tangents[0]);
			lightVectorArray[i][1] = DotProduct(lightVector, backEnd.vertices[i].tangents[1]);
			lightVectorArray[i][2] = DotProduct(lightVector, backEnd.vertices[i].normal);
		}
	}

	RB_UpdateVertexBuffer(backEnd.lightVectorBuffer, lightVectorArray, backEnd.numVertices * sizeof(vec3_t));

	GL_SelectTexture(1);
	qglTexCoordPointer(3, GL_FLOAT, sizeof(vec3_t), backEnd.lightVectorBuffer->pointer);
	GL_SelectTexture(0);

	// If sourcing vertex arrays from multiple buffers, unbind them
	if (glConfig.vertexBufferObject && r_vertexBuffers->integerValue)
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	if (glConfig.maxTextureUnits > 2){
		// Write diffuse light contribution to destination alpha
		GL_BlendFunc(GL_ONE, GL_ZERO);
		GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

		qglColor3f(1, 1, 1);

		GL_SelectTexture(0);
		GL_EnableTexture(bumpTexture->uploadTarget);
		GL_BindTexture(bumpTexture);

		GL_LoadMatrix(GL_TEXTURE, bumpMatrix);
		GL_TexEnv(GL_REPLACE);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

		GL_SelectTexture(1);
		GL_EnableTexture(tr.normalCubeMapTexture->uploadTarget);
		GL_BindTexture(tr.normalCubeMapTexture);

		GL_LoadIdentity(GL_TEXTURE);
		GL_TexEnv(GL_COMBINE_ARB);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGBA_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
		qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

		RB_DrawElements();

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		GL_DisableTexture();
		GL_SelectTexture(0);

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		// Modulate diffuse map by light projection & falloff, then
		// modulate by intensity stored in destination alpha and add to
		// color buffer
		GL_BlendFunc(GL_DST_ALPHA, GL_ONE);
		GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

// TODO: vertexColor/inverseVertexColor
		qglColor3fv(diffuseColor);

		GL_SelectTexture(0);
		GL_EnableTexture(diffuseTexture->uploadTarget);
		GL_BindTexture(diffuseTexture);

		GL_LoadMatrix(GL_TEXTURE, diffuseMatrix);
		GL_TexEnv(GL_MODULATE);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

		GL_SelectTexture(1);
		GL_EnableTexture(backEnd.lightStage->textureStage.texture->uploadTarget);
		GL_BindTexture(backEnd.lightStage->textureStage.texture);

		GL_LoadIdentity(GL_TEXTURE);
		GL_TexEnv(GL_MODULATE);

		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_Q);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);
		GL_TexGen(GL_T, GL_OBJECT_LINEAR);
		GL_TexGen(GL_Q, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 0]);
		qglTexGenfv(GL_T, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 4]);
		qglTexGenfv(GL_Q, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[12]);

		GL_SelectTexture(2);
		GL_EnableTexture(backEnd.lightMaterial->lightFalloffImage->uploadTarget);
		GL_BindTexture(backEnd.lightMaterial->lightFalloffImage);

		GL_LoadIdentity(GL_TEXTURE);
		GL_TexEnv(GL_MODULATE);

		qglTexCoord2f(0.0, 0.5);

		GL_Enable(GL_TEXTURE_GEN_S);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 8]);

		RB_DrawElements();

		GL_Disable(GL_TEXTURE_GEN_S);

		GL_DisableTexture();
		GL_SelectTexture(1);

		GL_Disable(GL_TEXTURE_GEN_S);
		GL_Disable(GL_TEXTURE_GEN_T);
		GL_Disable(GL_TEXTURE_GEN_Q);

		GL_DisableTexture();
		GL_SelectTexture(0);

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		return;
	}

	// Write light falloff to destination alpha
	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	qglColor3f(1, 1, 1);

	GL_SelectTexture(0);
	GL_EnableTexture(backEnd.lightMaterial->lightFalloffImage->uploadTarget);
	GL_BindTexture(backEnd.lightMaterial->lightFalloffImage);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_MODULATE);

	qglTexCoord2f(0.0, 0.5);

	GL_Enable(GL_TEXTURE_GEN_S);

	GL_TexGen(GL_S, GL_OBJECT_LINEAR);

	qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 8]);

	RB_DrawElements();

	GL_Disable(GL_TEXTURE_GEN_S);

	// Add diffuse light contribution to destination alpha
	GL_BlendFunc(GL_DST_ALPHA, GL_ZERO);
	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	qglColor3f(1, 1, 1);

	GL_SelectTexture(0);
	GL_EnableTexture(bumpTexture->uploadTarget);
	GL_BindTexture(bumpTexture);

	GL_LoadMatrix(GL_TEXTURE, bumpMatrix);
	GL_TexEnv(GL_REPLACE);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_SelectTexture(1);
	GL_EnableTexture(tr.normalCubeMapTexture->uploadTarget);
	GL_BindTexture(tr.normalCubeMapTexture);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_COMBINE_ARB);

	qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGBA_ARB);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
	qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	RB_DrawElements();

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_DisableTexture();
	GL_SelectTexture(0);

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// Modulate diffuse map by light projection, then modulate by
	// intensity stored in destination alpha and add to color buffer
	GL_BlendFunc(GL_DST_ALPHA, GL_ONE);
	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

// TODO: vertexColor/inverseVertexColor
	qglColor3fv(diffuseColor);

	GL_SelectTexture(0);
	GL_EnableTexture(diffuseTexture->uploadTarget);
	GL_BindTexture(diffuseTexture);

	GL_LoadMatrix(GL_TEXTURE, diffuseMatrix);
	GL_TexEnv(GL_MODULATE);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_SelectTexture(1);
	GL_EnableTexture(backEnd.lightStage->textureStage.texture->uploadTarget);
	GL_BindTexture(backEnd.lightStage->textureStage.texture);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_MODULATE);

	GL_Enable(GL_TEXTURE_GEN_S);
	GL_Enable(GL_TEXTURE_GEN_T);
	GL_Enable(GL_TEXTURE_GEN_Q);

	GL_TexGen(GL_S, GL_OBJECT_LINEAR);
	GL_TexGen(GL_T, GL_OBJECT_LINEAR);
	GL_TexGen(GL_Q, GL_OBJECT_LINEAR);

	qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 0]);
	qglTexGenfv(GL_T, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 4]);
	qglTexGenfv(GL_Q, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[12]);

	RB_DrawElements();

	GL_Disable(GL_TEXTURE_GEN_S);
	GL_Disable(GL_TEXTURE_GEN_T);
	GL_Disable(GL_TEXTURE_GEN_Q);

	GL_DisableTexture();
	GL_SelectTexture(0);

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


/*
 =======================================================================

 NV10 PATH (TODO)

 =======================================================================
*/


/*
 =================
 RB_NV10_RenderInteraction
 =================
*/
void RB_NV10_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage){

}

/*
 =================
 RB_NV10_InitRenderer
 =================
*/
void RB_NV10_InitRenderer (void){

	Com_Printf("Initializing NV10 renderer\n");

	if (!glConfig.allowNV10Path){
		Com_Printf("...not available\n");
		return;
	}

	Com_Printf("...not implemented!\n");

	glConfig.allowNV10Path = false;
}


/*
 =======================================================================

 NV20 PATH (TODO)

 =======================================================================
*/


/*
 =================
 RB_NV20_RenderInteraction
 =================
*/
void RB_NV20_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage){
	vec3_t		lightVectorArray[MAX_VERTICES];
	texture_t	*bumpTexture, *diffuseTexture;
	mat4_t		bumpMatrix, diffuseMatrix;
	vec3_t		diffuseColor;
	vec3_t		lightVector;
	int			i;

	if (r_skipBump->integerValue)
		bumpStage = NULL;
	if (r_skipDiffuse->integerValue)
		diffuseStage = NULL;

	if (r_fastInteractions->integerValue)
		bumpStage = NULL;

	// Set up the bump texture and matrix
	if (bumpStage){
		bumpTexture = bumpStage->textureStage.texture;

		RB_MultiplyTextureMatrix(backEnd.material, &bumpStage->textureStage, bumpMatrix);
	}
	else {
		bumpTexture = tr.flatTexture;

		Matrix4_Identity(bumpMatrix);
	}

	// Set up the diffuse texture, matrix, and color
	if (diffuseStage){
		diffuseTexture = diffuseStage->textureStage.texture;

		RB_MultiplyTextureMatrix(backEnd.material, &diffuseStage->textureStage, diffuseMatrix);

		diffuseColor[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]];
		diffuseColor[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]];
		diffuseColor[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]];

		diffuseColor[0] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[0]];
		diffuseColor[1] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[1]];
		diffuseColor[2] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[2]];
	}
	else {
		diffuseTexture = tr.whiteTexture;

		Matrix4_Identity(diffuseMatrix);

		diffuseColor[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]];
		diffuseColor[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]];
		diffuseColor[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]];
	}

	// Set up the arrays
	qglTexCoordPointer(2, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_ST);

	// Set up the light vector array
	if (backEnd.lightMaterial->ambientLight){
		for (i = 0; i < backEnd.numVertices; i++){
			lightVectorArray[i][0] = 0.0;
			lightVectorArray[i][1] = 0.0;
			lightVectorArray[i][2] = 1.0;
		}
	}
	else {
		for (i = 0; i < backEnd.numVertices; i++){
			VectorSubtract(backEnd.localParms.lightOrigin, backEnd.vertices[i].xyz, lightVector);

			lightVectorArray[i][0] = DotProduct(lightVector, backEnd.vertices[i].tangents[0]);
			lightVectorArray[i][1] = DotProduct(lightVector, backEnd.vertices[i].tangents[1]);
			lightVectorArray[i][2] = DotProduct(lightVector, backEnd.vertices[i].normal);
		}
	}

	RB_UpdateVertexBuffer(backEnd.lightVectorBuffer, lightVectorArray, backEnd.numVertices * sizeof(vec3_t));

	GL_SelectTexture(1);
	qglTexCoordPointer(3, GL_FLOAT, sizeof(vec3_t), backEnd.lightVectorBuffer->pointer);
	GL_SelectTexture(0);

	// If sourcing vertex arrays from multiple buffers, unbind them
	if (glConfig.vertexBufferObject && r_vertexBuffers->integerValue)
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	if (glConfig.maxTextureUnits > 2){
		// Write diffuse light contribution to destination alpha
		GL_BlendFunc(GL_ONE, GL_ZERO);
		GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

		qglColor3f(1, 1, 1);

		GL_SelectTexture(0);
		GL_EnableTexture(bumpTexture->uploadTarget);
		GL_BindTexture(bumpTexture);

		GL_LoadMatrix(GL_TEXTURE, bumpMatrix);
		GL_TexEnv(GL_REPLACE);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

		GL_SelectTexture(1);
		GL_EnableTexture(tr.normalCubeMapTexture->uploadTarget);
		GL_BindTexture(tr.normalCubeMapTexture);

		GL_LoadIdentity(GL_TEXTURE);
		GL_TexEnv(GL_COMBINE_ARB);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGBA_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
		qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

		RB_DrawElements();

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		GL_DisableTexture();
		GL_SelectTexture(0);

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		// Modulate diffuse map by light projection & falloff, then
		// modulate by intensity stored in destination alpha and add to
		// color buffer
		GL_BlendFunc(GL_DST_ALPHA, GL_ONE);
		GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

// TODO: vertexColor/inverseVertexColor
		qglColor3fv(diffuseColor);

		GL_SelectTexture(0);
		GL_EnableTexture(diffuseTexture->uploadTarget);
		GL_BindTexture(diffuseTexture);

		GL_LoadMatrix(GL_TEXTURE, diffuseMatrix);
		GL_TexEnv(GL_MODULATE);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

		GL_SelectTexture(1);
		GL_EnableTexture(backEnd.lightStage->textureStage.texture->uploadTarget);
		GL_BindTexture(backEnd.lightStage->textureStage.texture);

		GL_LoadIdentity(GL_TEXTURE);
		GL_TexEnv(GL_MODULATE);

		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_Q);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);
		GL_TexGen(GL_T, GL_OBJECT_LINEAR);
		GL_TexGen(GL_Q, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 0]);
		qglTexGenfv(GL_T, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 4]);
		qglTexGenfv(GL_Q, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[12]);

		GL_SelectTexture(2);
		GL_EnableTexture(backEnd.lightMaterial->lightFalloffImage->uploadTarget);
		GL_BindTexture(backEnd.lightMaterial->lightFalloffImage);

		GL_LoadIdentity(GL_TEXTURE);
		GL_TexEnv(GL_MODULATE);

		qglTexCoord2f(0.0, 0.5);

		GL_Enable(GL_TEXTURE_GEN_S);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 8]);

		RB_DrawElements();

		GL_Disable(GL_TEXTURE_GEN_S);

		GL_DisableTexture();
		GL_SelectTexture(1);

		GL_Disable(GL_TEXTURE_GEN_S);
		GL_Disable(GL_TEXTURE_GEN_T);
		GL_Disable(GL_TEXTURE_GEN_Q);

		GL_DisableTexture();
		GL_SelectTexture(0);

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		return;
	}

	// Write light falloff to destination alpha
	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	qglColor3f(1, 1, 1);

	GL_SelectTexture(0);
	GL_EnableTexture(backEnd.lightMaterial->lightFalloffImage->uploadTarget);
	GL_BindTexture(backEnd.lightMaterial->lightFalloffImage);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_MODULATE);

	qglTexCoord2f(0.0, 0.5);

	GL_Enable(GL_TEXTURE_GEN_S);

	GL_TexGen(GL_S, GL_OBJECT_LINEAR);

	qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 8]);

	RB_DrawElements();

	GL_Disable(GL_TEXTURE_GEN_S);

	// Add diffuse light contribution to destination alpha
	GL_BlendFunc(GL_DST_ALPHA, GL_ZERO);
	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	qglColor3f(1, 1, 1);

	GL_SelectTexture(0);
	GL_EnableTexture(bumpTexture->uploadTarget);
	GL_BindTexture(bumpTexture);

	GL_LoadMatrix(GL_TEXTURE, bumpMatrix);
	GL_TexEnv(GL_REPLACE);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_SelectTexture(1);
	GL_EnableTexture(tr.normalCubeMapTexture->uploadTarget);
	GL_BindTexture(tr.normalCubeMapTexture);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_COMBINE_ARB);

	qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGBA_ARB);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
	qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	RB_DrawElements();

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_DisableTexture();
	GL_SelectTexture(0);

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// Modulate diffuse map by light projection, then modulate by
	// intensity stored in destination alpha and add to color buffer
	GL_BlendFunc(GL_DST_ALPHA, GL_ONE);
	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

// TODO: vertexColor/inverseVertexColor
	qglColor3fv(diffuseColor);

	GL_SelectTexture(0);
	GL_EnableTexture(diffuseTexture->uploadTarget);
	GL_BindTexture(diffuseTexture);

	GL_LoadMatrix(GL_TEXTURE, diffuseMatrix);
	GL_TexEnv(GL_MODULATE);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_SelectTexture(1);
	GL_EnableTexture(backEnd.lightStage->textureStage.texture->uploadTarget);
	GL_BindTexture(backEnd.lightStage->textureStage.texture);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_MODULATE);

	GL_Enable(GL_TEXTURE_GEN_S);
	GL_Enable(GL_TEXTURE_GEN_T);
	GL_Enable(GL_TEXTURE_GEN_Q);

	GL_TexGen(GL_S, GL_OBJECT_LINEAR);
	GL_TexGen(GL_T, GL_OBJECT_LINEAR);
	GL_TexGen(GL_Q, GL_OBJECT_LINEAR);

	qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 0]);
	qglTexGenfv(GL_T, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 4]);
	qglTexGenfv(GL_Q, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[12]);

	RB_DrawElements();

	GL_Disable(GL_TEXTURE_GEN_S);
	GL_Disable(GL_TEXTURE_GEN_T);
	GL_Disable(GL_TEXTURE_GEN_Q);

	GL_DisableTexture();
	GL_SelectTexture(0);

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

/*
 =================
 RB_NV20_InitRenderer
 =================
*/
void RB_NV20_InitRenderer (void){

	Com_Printf("Initializing NV20 renderer\n");

	if (!glConfig.allowNV20Path){
		Com_Printf("...not available\n");
		return;
	}


	// Load interaction.vfp
	tr.interactionVProgram = R_FindProgram("interaction.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.interactionFProgram = R_FindProgram("interaction.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.interactionVProgram || !tr.interactionFProgram){
		Com_Printf("...missing interaction.vfp\n");

		glConfig.allowNV20Path = false;
		return;
	}

	// Load interactionFast.vfp
	tr.interactionFastVProgram = R_FindProgram("interactionFast.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.interactionFastFProgram = R_FindProgram("interactionFast.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.interactionFastVProgram || !tr.interactionFastFProgram){
		Com_Printf("...missing interactionFast.vfp\n");

		glConfig.allowNV20Path = false;
		return;
	}

	// Load ambientInteraction.vfp
	tr.ambientInteractionVProgram = R_FindProgram("ambientInteraction.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.ambientInteractionFProgram = R_FindProgram("ambientInteraction.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.ambientInteractionVProgram || !tr.ambientInteractionFProgram){
		Com_Printf("...missing ambientInteraction.vfp\n");

		glConfig.allowNV20Path = false;
		return;
	}

	// Load ambientInteractionFast.vfp
	tr.ambientInteractionFastVProgram = R_FindProgram("ambientInteractionFast.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.ambientInteractionFastFProgram = R_FindProgram("ambientInteractionFast.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.ambientInteractionFastVProgram || !tr.ambientInteractionFastFProgram){
		Com_Printf("...missing ambientInteractionFast.vfp\n");

		glConfig.allowNV20Path = false;
		return;
	}

	Com_Printf("...available\n");
}


/*
 =======================================================================

 NV30 PATH (TODO)

 =======================================================================
*/


/*
 =================
 RB_NV30_RenderInteraction
 =================
*/
void RB_NV30_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage){
	vec3_t		lightVectorArray[MAX_VERTICES];
	texture_t	*bumpTexture, *diffuseTexture;
	mat4_t		bumpMatrix, diffuseMatrix;
	vec3_t		diffuseColor;
	vec3_t		lightVector;
	int			i;

	if (r_skipBump->integerValue)
		bumpStage = NULL;
	if (r_skipDiffuse->integerValue)
		diffuseStage = NULL;

	if (r_fastInteractions->integerValue)
		bumpStage = NULL;

	// Set up the bump texture and matrix
	if (bumpStage){
		bumpTexture = bumpStage->textureStage.texture;

		RB_MultiplyTextureMatrix(backEnd.material, &bumpStage->textureStage, bumpMatrix);
	}
	else {
		bumpTexture = tr.flatTexture;

		Matrix4_Identity(bumpMatrix);
	}

	// Set up the diffuse texture, matrix, and color
	if (diffuseStage){
		diffuseTexture = diffuseStage->textureStage.texture;

		RB_MultiplyTextureMatrix(backEnd.material, &diffuseStage->textureStage, diffuseMatrix);

		diffuseColor[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]];
		diffuseColor[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]];
		diffuseColor[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]];

		diffuseColor[0] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[0]];
		diffuseColor[1] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[1]];
		diffuseColor[2] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[2]];
	}
	else {
		diffuseTexture = tr.whiteTexture;

		Matrix4_Identity(diffuseMatrix);

		diffuseColor[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]];
		diffuseColor[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]];
		diffuseColor[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]];
	}

	// Set up the arrays
	qglTexCoordPointer(2, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_ST);

	// Set up the light vector array
	if (backEnd.lightMaterial->ambientLight){
		for (i = 0; i < backEnd.numVertices; i++){
			lightVectorArray[i][0] = 0.0;
			lightVectorArray[i][1] = 0.0;
			lightVectorArray[i][2] = 1.0;
		}
	}
	else {
		for (i = 0; i < backEnd.numVertices; i++){
			VectorSubtract(backEnd.localParms.lightOrigin, backEnd.vertices[i].xyz, lightVector);

			lightVectorArray[i][0] = DotProduct(lightVector, backEnd.vertices[i].tangents[0]);
			lightVectorArray[i][1] = DotProduct(lightVector, backEnd.vertices[i].tangents[1]);
			lightVectorArray[i][2] = DotProduct(lightVector, backEnd.vertices[i].normal);
		}
	}

	RB_UpdateVertexBuffer(backEnd.lightVectorBuffer, lightVectorArray, backEnd.numVertices * sizeof(vec3_t));

	GL_SelectTexture(1);
	qglTexCoordPointer(3, GL_FLOAT, sizeof(vec3_t), backEnd.lightVectorBuffer->pointer);
	GL_SelectTexture(0);

	// If sourcing vertex arrays from multiple buffers, unbind them
	if (glConfig.vertexBufferObject && r_vertexBuffers->integerValue)
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	if (glConfig.maxTextureUnits > 2){
		// Write diffuse light contribution to destination alpha
		GL_BlendFunc(GL_ONE, GL_ZERO);
		GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

		qglColor3f(1, 1, 1);

		GL_SelectTexture(0);
		GL_EnableTexture(bumpTexture->uploadTarget);
		GL_BindTexture(bumpTexture);

		GL_LoadMatrix(GL_TEXTURE, bumpMatrix);
		GL_TexEnv(GL_REPLACE);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

		GL_SelectTexture(1);
		GL_EnableTexture(tr.normalCubeMapTexture->uploadTarget);
		GL_BindTexture(tr.normalCubeMapTexture);

		GL_LoadIdentity(GL_TEXTURE);
		GL_TexEnv(GL_COMBINE_ARB);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGBA_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
		qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

		RB_DrawElements();

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		GL_DisableTexture();
		GL_SelectTexture(0);

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		// Modulate diffuse map by light projection & falloff, then
		// modulate by intensity stored in destination alpha and add to
		// color buffer
		GL_BlendFunc(GL_DST_ALPHA, GL_ONE);
		GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

// TODO: vertexColor/inverseVertexColor
		qglColor3fv(diffuseColor);

		GL_SelectTexture(0);
		GL_EnableTexture(diffuseTexture->uploadTarget);
		GL_BindTexture(diffuseTexture);

		GL_LoadMatrix(GL_TEXTURE, diffuseMatrix);
		GL_TexEnv(GL_MODULATE);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

		GL_SelectTexture(1);
		GL_EnableTexture(backEnd.lightStage->textureStage.texture->uploadTarget);
		GL_BindTexture(backEnd.lightStage->textureStage.texture);

		GL_LoadIdentity(GL_TEXTURE);
		GL_TexEnv(GL_MODULATE);

		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_Q);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);
		GL_TexGen(GL_T, GL_OBJECT_LINEAR);
		GL_TexGen(GL_Q, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 0]);
		qglTexGenfv(GL_T, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 4]);
		qglTexGenfv(GL_Q, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[12]);

		GL_SelectTexture(2);
		GL_EnableTexture(backEnd.lightMaterial->lightFalloffImage->uploadTarget);
		GL_BindTexture(backEnd.lightMaterial->lightFalloffImage);

		GL_LoadIdentity(GL_TEXTURE);
		GL_TexEnv(GL_MODULATE);

		qglTexCoord2f(0.0, 0.5);

		GL_Enable(GL_TEXTURE_GEN_S);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 8]);

		RB_DrawElements();

		GL_Disable(GL_TEXTURE_GEN_S);

		GL_DisableTexture();
		GL_SelectTexture(1);

		GL_Disable(GL_TEXTURE_GEN_S);
		GL_Disable(GL_TEXTURE_GEN_T);
		GL_Disable(GL_TEXTURE_GEN_Q);

		GL_DisableTexture();
		GL_SelectTexture(0);

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

		return;
	}

	// Write light falloff to destination alpha
	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	qglColor3f(1, 1, 1);

	GL_SelectTexture(0);
	GL_EnableTexture(backEnd.lightMaterial->lightFalloffImage->uploadTarget);
	GL_BindTexture(backEnd.lightMaterial->lightFalloffImage);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_MODULATE);

	qglTexCoord2f(0.0, 0.5);

	GL_Enable(GL_TEXTURE_GEN_S);

	GL_TexGen(GL_S, GL_OBJECT_LINEAR);

	qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 8]);

	RB_DrawElements();

	GL_Disable(GL_TEXTURE_GEN_S);

	// Add diffuse light contribution to destination alpha
	GL_BlendFunc(GL_DST_ALPHA, GL_ZERO);
	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	qglColor3f(1, 1, 1);

	GL_SelectTexture(0);
	GL_EnableTexture(bumpTexture->uploadTarget);
	GL_BindTexture(bumpTexture);

	GL_LoadMatrix(GL_TEXTURE, bumpMatrix);
	GL_TexEnv(GL_REPLACE);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_SelectTexture(1);
	GL_EnableTexture(tr.normalCubeMapTexture->uploadTarget);
	GL_BindTexture(tr.normalCubeMapTexture);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_COMBINE_ARB);

	qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGBA_ARB);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
	qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	RB_DrawElements();

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_DisableTexture();
	GL_SelectTexture(0);

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// Modulate diffuse map by light projection, then modulate by
	// intensity stored in destination alpha and add to color buffer
	GL_BlendFunc(GL_DST_ALPHA, GL_ONE);
	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

// TODO: vertexColor/inverseVertexColor
	qglColor3fv(diffuseColor);

	GL_SelectTexture(0);
	GL_EnableTexture(diffuseTexture->uploadTarget);
	GL_BindTexture(diffuseTexture);

	GL_LoadMatrix(GL_TEXTURE, diffuseMatrix);
	GL_TexEnv(GL_MODULATE);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_SelectTexture(1);
	GL_EnableTexture(backEnd.lightStage->textureStage.texture->uploadTarget);
	GL_BindTexture(backEnd.lightStage->textureStage.texture);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_MODULATE);

	GL_Enable(GL_TEXTURE_GEN_S);
	GL_Enable(GL_TEXTURE_GEN_T);
	GL_Enable(GL_TEXTURE_GEN_Q);

	GL_TexGen(GL_S, GL_OBJECT_LINEAR);
	GL_TexGen(GL_T, GL_OBJECT_LINEAR);
	GL_TexGen(GL_Q, GL_OBJECT_LINEAR);

	qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 0]);
	qglTexGenfv(GL_T, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[ 4]);
	qglTexGenfv(GL_Q, GL_OBJECT_PLANE, &backEnd.localParms.lightMatrix[12]);

	RB_DrawElements();

	GL_Disable(GL_TEXTURE_GEN_S);
	GL_Disable(GL_TEXTURE_GEN_T);
	GL_Disable(GL_TEXTURE_GEN_Q);

	GL_DisableTexture();
	GL_SelectTexture(0);

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

/*
 =================
 RB_NV30_InitRenderer
 =================
*/
void RB_NV30_InitRenderer (void){

	Com_Printf("Initializing NV30 renderer\n");

	if (!glConfig.allowNV30Path){
		Com_Printf("...not available\n");
		return;
	}


	// Load interaction.vfp
	tr.interactionVProgram = R_FindProgram("interaction.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.interactionFProgram = R_FindProgram("interaction.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.interactionVProgram || !tr.interactionFProgram){
		Com_Printf("...missing interaction.vfp\n");

		glConfig.allowNV30Path = false;
		return;
	}

	// Load interactionFast.vfp
	tr.interactionFastVProgram = R_FindProgram("interactionFast.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.interactionFastFProgram = R_FindProgram("interactionFast.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.interactionFastVProgram || !tr.interactionFastFProgram){
		Com_Printf("...missing interactionFast.vfp\n");

		glConfig.allowNV30Path = false;
		return;
	}

	// Load ambientInteraction.vfp
	tr.ambientInteractionVProgram = R_FindProgram("ambientInteraction.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.ambientInteractionFProgram = R_FindProgram("ambientInteraction.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.ambientInteractionVProgram || !tr.ambientInteractionFProgram){
		Com_Printf("...missing ambientInteraction.vfp\n");

		glConfig.allowNV30Path = false;
		return;
	}

	// Load ambientInteractionFast.vfp
	tr.ambientInteractionFastVProgram = R_FindProgram("ambientInteractionFast.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.ambientInteractionFastFProgram = R_FindProgram("ambientInteractionFast.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.ambientInteractionFastVProgram || !tr.ambientInteractionFastFProgram){
		Com_Printf("...missing ambientInteractionFast.vfp\n");

		glConfig.allowNV30Path = false;
		return;
	}

	Com_Printf("...available\n");
}


/*
 =======================================================================

 R200 PATH (TODO)

 =======================================================================
*/


/*
 =================
 RB_R200_RenderInteraction
 =================
*/
void RB_R200_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage){

}

/*
 =================
 RB_R200_InitRenderer
 =================
*/
void RB_R200_InitRenderer (void){

	Com_Printf("Initializing R200 renderer\n");

	if (!glConfig.allowR200Path){
		Com_Printf("...not available\n");
		return;
	}

	Com_Printf("...not implemented!\n");

	glConfig.allowR200Path = false;
}


/*
 =======================================================================

 HIGH-QUALITY ARB2 PATH (SINGLE-PASS, V/F PROGRAMS, FULLY FEATURED)

 =======================================================================
*/


/*
 =================
 RB_ARB2_RenderInteraction
 =================
*/
void RB_ARB2_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage){

	texture_t	*bumpTexture, *diffuseTexture, *specularTexture;
	mat4_t		bumpMatrix, diffuseMatrix, specularMatrix;
	vec3_t		diffuseColor, specularColor;
	float		lightScale, specularExponent, colorModulate = 0.0, colorAdd = 1.0;

	if (r_skipBump->integerValue)
		bumpStage = NULL;
	if (r_skipDiffuse->integerValue)
		diffuseStage = NULL;
	if (r_skipSpecular->integerValue)
		specularStage = NULL;

	// Get the light scale
	if (r_lightScale->floatValue > 1.0)
		lightScale = r_lightScale->floatValue;
	else
		lightScale = 1.0;

	// Set up color modulate/add
	if (diffuseStage && specularStage){
		if (diffuseStage->colorStage.vertexColor == VC_MODULATE && specularStage->colorStage.vertexColor == VC_MODULATE){
			colorModulate = 1.0;
			colorAdd = 0.0;
		}
		else if (diffuseStage->colorStage.vertexColor == VC_INVERSE_MODULATE && specularStage->colorStage.vertexColor == VC_INVERSE_MODULATE){
			colorModulate = -1.0;
			colorAdd = 1.0;
		}
	}
	else {
		if (diffuseStage){
			if (diffuseStage->colorStage.vertexColor == VC_MODULATE){
				colorModulate = 1.0;
				colorAdd = 0.0;
			}
			else if (diffuseStage->colorStage.vertexColor == VC_INVERSE_MODULATE){
				colorModulate = -1.0;
				colorAdd = 1.0;
			}
		}

		if (specularStage){
			if (specularStage->colorStage.vertexColor == VC_MODULATE){
				colorModulate = 1.0;
				colorAdd = 0.0;
			}
			else if (specularStage->colorStage.vertexColor == VC_INVERSE_MODULATE){
				colorModulate = -1.0;
				colorAdd = 1.0;
			}
		}
	}

	// Set up the bump texture and matrix
	if (bumpStage){
		bumpTexture = bumpStage->textureStage.texture;

		RB_MultiplyTextureMatrix(backEnd.material, &bumpStage->textureStage, bumpMatrix);
	}
	else {
		bumpTexture = tr.flatTexture;

		Matrix4_Identity(bumpMatrix);
	}

	// Set up the diffuse texture, matrix, and color
	if (diffuseStage){
		diffuseTexture = diffuseStage->textureStage.texture;

		RB_MultiplyTextureMatrix(backEnd.material, &diffuseStage->textureStage, diffuseMatrix);

		diffuseColor[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]] * lightScale;
		diffuseColor[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]] * lightScale;
		diffuseColor[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]] * lightScale;

		diffuseColor[0] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[0]];
		diffuseColor[1] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[1]];
		diffuseColor[2] *= backEnd.material->expressionRegisters[diffuseStage->colorStage.registers[2]];
	}
	else {
		diffuseTexture = tr.whiteTexture;

		Matrix4_Identity(diffuseMatrix);

		diffuseColor[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]] * lightScale;
		diffuseColor[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]] * lightScale;
		diffuseColor[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]] * lightScale;
	}

	// Set up the specular texture, matrix, color, and exponent
	if (specularStage){
		specularTexture = specularStage->textureStage.texture;

		RB_MultiplyTextureMatrix(backEnd.material, &specularStage->textureStage, specularMatrix);

		specularColor[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]] * lightScale;
		specularColor[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]] * lightScale;
		specularColor[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]] * lightScale;

		specularColor[0] *= backEnd.material->expressionRegisters[specularStage->colorStage.registers[0]] * 2.0;
		specularColor[1] *= backEnd.material->expressionRegisters[specularStage->colorStage.registers[1]] * 2.0;
		specularColor[2] *= backEnd.material->expressionRegisters[specularStage->colorStage.registers[2]] * 2.0;

		specularExponent = specularStage->specularExponent;
	}
	else {
		specularTexture = tr.blackTexture;

		Matrix4_Identity(specularMatrix);

		specularColor[0] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[0]] * lightScale;
		specularColor[1] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[1]] * lightScale;
		specularColor[2] = backEnd.lightMaterial->expressionRegisters[backEnd.lightStage->colorStage.registers[2]] * lightScale;

		specularExponent = 16.0;
	}

	// Set up the arrays
	qglEnableClientState(GL_NORMAL_ARRAY);
	qglNormalPointer(GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_NORMAL);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_ST);

	qglEnableClientState(GL_COLOR_ARRAY);
	qglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_COLOR);

	qglEnableVertexAttribArrayARB(10);
	qglVertexAttribPointerARB(10, 3, GL_FLOAT, false, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_TANGENT0);

	qglEnableVertexAttribArrayARB(11);
	qglVertexAttribPointerARB(11, 3, GL_FLOAT, false, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_TANGENT1);

	// Set up the interaction programs
	if (backEnd.lightMaterial->ambientLight){
		if (r_fastInteractions->integerValue){
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0, colorModulate, colorModulate, colorModulate, colorModulate);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 1, colorAdd, colorAdd, colorAdd, colorAdd);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 2, diffuseMatrix[ 0], diffuseMatrix[ 4], diffuseMatrix[ 8], diffuseMatrix[12]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 3, diffuseMatrix[ 1], diffuseMatrix[ 5], diffuseMatrix[ 9], diffuseMatrix[13]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 4, backEnd.localParms.lightMatrix[ 0], backEnd.localParms.lightMatrix[ 1], backEnd.localParms.lightMatrix[ 2], backEnd.localParms.lightMatrix[ 3]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 5, backEnd.localParms.lightMatrix[ 4], backEnd.localParms.lightMatrix[ 5], backEnd.localParms.lightMatrix[ 6], backEnd.localParms.lightMatrix[ 7]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 6, backEnd.localParms.lightMatrix[12], backEnd.localParms.lightMatrix[13], backEnd.localParms.lightMatrix[14], backEnd.localParms.lightMatrix[15]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 7, backEnd.localParms.lightMatrix[ 8], backEnd.localParms.lightMatrix[ 9], backEnd.localParms.lightMatrix[10], backEnd.localParms.lightMatrix[11]);

			qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, diffuseColor[0], diffuseColor[1], diffuseColor[2], 1.0);

			// Set up the textures
			GL_BindProgramTexture(0, diffuseTexture);
			GL_BindProgramTexture(1, backEnd.lightStage->textureStage.texture);
			GL_BindProgramTexture(2, backEnd.lightMaterial->lightFalloffImage);
		}
		else {
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0, colorModulate, colorModulate, colorModulate, colorModulate);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 1, colorAdd, colorAdd, colorAdd, colorAdd);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 2, bumpMatrix[ 0], bumpMatrix[ 4], bumpMatrix[ 8], bumpMatrix[12]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 3, bumpMatrix[ 1], bumpMatrix[ 5], bumpMatrix[ 9], bumpMatrix[13]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 4, diffuseMatrix[ 0], diffuseMatrix[ 4], diffuseMatrix[ 8], diffuseMatrix[12]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 5, diffuseMatrix[ 1], diffuseMatrix[ 5], diffuseMatrix[ 9], diffuseMatrix[13]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 6, backEnd.localParms.lightMatrix[ 0], backEnd.localParms.lightMatrix[ 1], backEnd.localParms.lightMatrix[ 2], backEnd.localParms.lightMatrix[ 3]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 7, backEnd.localParms.lightMatrix[ 4], backEnd.localParms.lightMatrix[ 5], backEnd.localParms.lightMatrix[ 6], backEnd.localParms.lightMatrix[ 7]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 8, backEnd.localParms.lightMatrix[12], backEnd.localParms.lightMatrix[13], backEnd.localParms.lightMatrix[14], backEnd.localParms.lightMatrix[15]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 9, backEnd.localParms.lightMatrix[ 8], backEnd.localParms.lightMatrix[ 9], backEnd.localParms.lightMatrix[10], backEnd.localParms.lightMatrix[11]);

			qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, diffuseColor[0], diffuseColor[1], diffuseColor[2], 1.0);

			// Set up the textures
			GL_BindProgramTexture(0, bumpTexture);
			GL_BindProgramTexture(1, diffuseTexture);
			GL_BindProgramTexture(2, backEnd.lightStage->textureStage.texture);
			GL_BindProgramTexture(3, backEnd.lightMaterial->lightFalloffImage);
		}
	}
	else {
		if (r_fastInteractions->integerValue){
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0, backEnd.localParms.lightOrigin[0], backEnd.localParms.lightOrigin[1], backEnd.localParms.lightOrigin[2], 1.0);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 1, colorModulate, colorModulate, colorModulate, colorModulate);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 2, colorAdd, colorAdd, colorAdd, colorAdd);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 3, diffuseMatrix[ 0], diffuseMatrix[ 4], diffuseMatrix[ 8], diffuseMatrix[12]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 4, diffuseMatrix[ 1], diffuseMatrix[ 5], diffuseMatrix[ 9], diffuseMatrix[13]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 5, backEnd.localParms.lightMatrix[ 0], backEnd.localParms.lightMatrix[ 1], backEnd.localParms.lightMatrix[ 2], backEnd.localParms.lightMatrix[ 3]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 6, backEnd.localParms.lightMatrix[ 4], backEnd.localParms.lightMatrix[ 5], backEnd.localParms.lightMatrix[ 6], backEnd.localParms.lightMatrix[ 7]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 7, backEnd.localParms.lightMatrix[12], backEnd.localParms.lightMatrix[13], backEnd.localParms.lightMatrix[14], backEnd.localParms.lightMatrix[15]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 8, backEnd.localParms.lightMatrix[ 8], backEnd.localParms.lightMatrix[ 9], backEnd.localParms.lightMatrix[10], backEnd.localParms.lightMatrix[11]);

			qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, diffuseColor[0], diffuseColor[1], diffuseColor[2], 1.0);

			// Set up the textures
			GL_BindProgramTexture(0, tr.normalCubeMapTexture);
			GL_BindProgramTexture(1, diffuseTexture);
			GL_BindProgramTexture(2, backEnd.lightStage->textureStage.texture);
			GL_BindProgramTexture(3, backEnd.lightMaterial->lightFalloffImage);
		}
		else {
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0, backEnd.localParms.viewOrigin[0], backEnd.localParms.viewOrigin[1], backEnd.localParms.viewOrigin[2], 1.0);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 1, backEnd.localParms.lightOrigin[0], backEnd.localParms.lightOrigin[1], backEnd.localParms.lightOrigin[2], 1.0);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 2, colorModulate, colorModulate, colorModulate, colorModulate);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 3, colorAdd, colorAdd, colorAdd, colorAdd);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 4, bumpMatrix[ 0], bumpMatrix[ 4], bumpMatrix[ 8], bumpMatrix[12]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 5, bumpMatrix[ 1], bumpMatrix[ 5], bumpMatrix[ 9], bumpMatrix[13]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 6, diffuseMatrix[ 0], diffuseMatrix[ 4], diffuseMatrix[ 8], diffuseMatrix[12]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 7, diffuseMatrix[ 1], diffuseMatrix[ 5], diffuseMatrix[ 9], diffuseMatrix[13]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 8, specularMatrix[ 0], specularMatrix[ 4], specularMatrix[ 8], specularMatrix[12]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 9, specularMatrix[ 1], specularMatrix[ 5], specularMatrix[ 9], specularMatrix[13]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 10, backEnd.localParms.lightMatrix[ 0], backEnd.localParms.lightMatrix[ 1], backEnd.localParms.lightMatrix[ 2], backEnd.localParms.lightMatrix[ 3]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 11, backEnd.localParms.lightMatrix[ 4], backEnd.localParms.lightMatrix[ 5], backEnd.localParms.lightMatrix[ 6], backEnd.localParms.lightMatrix[ 7]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, backEnd.localParms.lightMatrix[12], backEnd.localParms.lightMatrix[13], backEnd.localParms.lightMatrix[14], backEnd.localParms.lightMatrix[15]);
			qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, backEnd.localParms.lightMatrix[ 8], backEnd.localParms.lightMatrix[ 9], backEnd.localParms.lightMatrix[10], backEnd.localParms.lightMatrix[11]);

			qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, diffuseColor[0], diffuseColor[1], diffuseColor[2], 1.0);
			qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, specularColor[0], specularColor[1], specularColor[2], 1.0);
			qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2, specularExponent, specularExponent, specularExponent, specularExponent);

			// Set up the textures
			GL_BindProgramTexture(0, tr.normalCubeMapTexture);
			GL_BindProgramTexture(1, bumpTexture);
			GL_BindProgramTexture(2, diffuseTexture);
			GL_BindProgramTexture(3, specularTexture);
			GL_BindProgramTexture(4, backEnd.lightStage->textureStage.texture);
			GL_BindProgramTexture(5, backEnd.lightMaterial->lightFalloffImage);
		}
	}

	// Draw it
	RB_DrawElements();

	GL_SelectTexture(0);

	qglDisableVertexAttribArrayARB(11);
	qglDisableVertexAttribArrayARB(10);

	qglDisableClientState(GL_COLOR_ARRAY);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	qglDisableClientState(GL_NORMAL_ARRAY);
}

/*
 =================
 RB_ARB2_InitRenderer
 =================
*/
void RB_ARB2_InitRenderer (void){

	Com_Printf("Initializing ARB2 renderer\n");

	if (!glConfig.allowARB2Path){
		Com_Printf("...not available\n");
		return;
	}

	// Load interaction.vfp
	tr.interactionVProgram = R_FindProgram("interaction.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.interactionFProgram = R_FindProgram("interaction.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.interactionVProgram || !tr.interactionFProgram){
		Com_Printf("...missing interaction.vfp\n");

		glConfig.allowARB2Path = false;
		return;
	}

	// Load interactionFast.vfp
	tr.interactionFastVProgram = R_FindProgram("interactionFast.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.interactionFastFProgram = R_FindProgram("interactionFast.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.interactionFastVProgram || !tr.interactionFastFProgram){
		Com_Printf("...missing interactionFast.vfp\n");

		glConfig.allowARB2Path = false;
		return;
	}

	// Load ambientInteraction.vfp
	tr.ambientInteractionVProgram = R_FindProgram("ambientInteraction.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.ambientInteractionFProgram = R_FindProgram("ambientInteraction.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.ambientInteractionVProgram || !tr.ambientInteractionFProgram){
		Com_Printf("...missing ambientInteraction.vfp\n");

		glConfig.allowARB2Path = false;
		return;
	}

	// Load ambientInteractionFast.vfp
	tr.ambientInteractionFastVProgram = R_FindProgram("ambientInteractionFast.vfp", GL_VERTEX_PROGRAM_ARB);
	tr.ambientInteractionFastFProgram = R_FindProgram("ambientInteractionFast.vfp", GL_FRAGMENT_PROGRAM_ARB);

	if (!tr.ambientInteractionFastVProgram || !tr.ambientInteractionFastFProgram){
		Com_Printf("...missing ambientInteractionFast.vfp\n");

		glConfig.allowARB2Path = false;
		return;
	}

	Com_Printf("...available\n");
}
