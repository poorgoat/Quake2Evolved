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


// r_backend.c -- main back-end interface


#include "r_local.h"


backEndState_t			backEnd;


/*
 =======================================================================

 VERTEX BUFFERS

 =======================================================================
*/

static vertexBuffer_t	rb_vertexBuffers[MAX_VERTEX_BUFFERS];
static int				rb_numVertexBuffers;


/*
 =================
 RB_UpdateVertexBuffer
 =================
*/
void RB_UpdateVertexBuffer (vertexBuffer_t *vertexBuffer, const void *data, int size){

	tr.pc.vertexBuffers++;
	tr.pc.vertexBufferBytes += size;

	if (!glConfig.vertexBufferObject){
		vertexBuffer->pointer = (char *)data;
		return;
	}

	if (!r_vertexBuffers->integerValue){
		vertexBuffer->pointer = (char *)data;

		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		return;
	}

	vertexBuffer->pointer = NULL;

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBuffer->bufNum);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, size, data, vertexBuffer->usage);
}

/*
 =================
 RB_AllocVertexBuffer
 =================
*/
static vertexBuffer_t *RB_AllocVertexBuffer (int size, unsigned usage){

	vertexBuffer_t	*vertexBuffer;

	if (rb_numVertexBuffers == MAX_VERTEX_BUFFERS)
		Com_Error(ERR_DROP, "RB_AllocVertexBuffer: MAX_VERTEX_BUFFERS hit");

	vertexBuffer = &rb_vertexBuffers[rb_numVertexBuffers++];

	vertexBuffer->pointer = NULL;
	vertexBuffer->size = size;
	vertexBuffer->usage = usage;

	if (!glConfig.vertexBufferObject)
		return vertexBuffer;

	qglGenBuffersARB(1, &vertexBuffer->bufNum);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBuffer->bufNum);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, vertexBuffer->size, NULL, vertexBuffer->usage);

	return vertexBuffer;
}

/*
 =================
 RB_InitVertexBuffers
 =================
*/
static void RB_InitVertexBuffers (void){

	backEnd.vertexBuffer = RB_AllocVertexBuffer(MAX_VERTICES * sizeof(vertexArray_t), GL_STREAM_DRAW_ARB);
	backEnd.shadowVertexBuffer = RB_AllocVertexBuffer(MAX_SHADOW_VERTICES * sizeof(shadowVertexArray_t), GL_STREAM_DRAW_ARB);

	backEnd.lightVectorBuffer = RB_AllocVertexBuffer(MAX_VERTICES * sizeof(vec3_t), GL_STREAM_DRAW_ARB);
	backEnd.halfAngleVectorBuffer = RB_AllocVertexBuffer(MAX_VERTICES * sizeof(vec3_t), GL_STREAM_DRAW_ARB);
}

/*
 =================
 RB_ShutdownVertexBuffers
 =================
*/
static void RB_ShutdownVertexBuffers (void){

	vertexBuffer_t	*vertexBuffer;
	int				i;

	if (!glConfig.vertexBufferObject){
		memset(rb_vertexBuffers, 0, sizeof(rb_vertexBuffers));

		rb_numVertexBuffers = 0;
		return;
	}

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	for (i = 0, vertexBuffer = rb_vertexBuffers; i < rb_numVertexBuffers; i++, vertexBuffer++)
		qglDeleteBuffersARB(1, &vertexBuffer->bufNum);

	memset(rb_vertexBuffers, 0, sizeof(rb_vertexBuffers));

	rb_numVertexBuffers = 0;
}


/*
 =======================================================================

 MATERIAL SETUP

 =======================================================================
*/


/*
 =================
 RB_DeformExpand
 =================
*/
static void RB_DeformExpand (void){

	vertexArray_t	*vertices = backEnd.vertices;
	float			expand;
	int				i;

	tr.pc.deforms++;
	tr.pc.deformVertices += backEnd.numVertices;

	expand = backEnd.material->expressionRegisters[backEnd.material->deformRegisters[0]];

	for (i = 0; i < backEnd.numVertices; i++, vertices++)
		VectorMA(vertices->xyz, expand, vertices->normal, vertices->xyz);
}

/*
 =================
 RB_DeformMove
 =================
*/
static void RB_DeformMove (void){

	vertexArray_t	*vertices = backEnd.vertices;
	vec3_t			move;
	int				i;

	tr.pc.deforms++;
	tr.pc.deformVertices += backEnd.numVertices;

	move[0] = backEnd.material->expressionRegisters[backEnd.material->deformRegisters[0]];
	move[1] = backEnd.material->expressionRegisters[backEnd.material->deformRegisters[1]];
	move[2] = backEnd.material->expressionRegisters[backEnd.material->deformRegisters[2]];

	for (i = 0; i < backEnd.numVertices; i++, vertices++)
		VectorAdd(vertices->xyz, move, vertices->xyz);
}

/*
 =================
 RB_DeformSprite
 =================
*/
static void RB_DeformSprite (void){

	vertexArray_t	*vertices = backEnd.vertices;
	vec3_t			viewMatrix[3];
	vec3_t			axis[3], matrix[3];
	vec3_t			center, tmp;
	int				i, j;

	if (backEnd.numIndices % 6){
		Com_Printf(S_COLOR_YELLOW "Material '%s' has 'deform sprite' with an odd index count\n", backEnd.material->name);
		return;
	}
	if (backEnd.numVertices % 4){
		Com_Printf(S_COLOR_YELLOW "Material '%s' has 'deform sprite' with an odd vertex count\n", backEnd.material->name);
		return;
	}

	tr.pc.deforms++;
	tr.pc.deformVertices += backEnd.numVertices;

	VectorCopy(&backEnd.localParms.viewMatrix[0], viewMatrix[0]);
	VectorCopy(&backEnd.localParms.viewMatrix[4], viewMatrix[1]);
	VectorCopy(&backEnd.localParms.viewMatrix[8], viewMatrix[2]);

	for (i = 0; i < backEnd.numVertices; i += 4, vertices += 4){
		center[0] = (vertices[0].xyz[0] + vertices[1].xyz[0] + vertices[2].xyz[0] + vertices[3].xyz[0]) * 0.25;
		center[1] = (vertices[0].xyz[1] + vertices[1].xyz[1] + vertices[2].xyz[1] + vertices[3].xyz[1]) * 0.25;
		center[2] = (vertices[0].xyz[2] + vertices[1].xyz[2] + vertices[2].xyz[2] + vertices[3].xyz[2]) * 0.25;

		VectorSubtract(vertices[0].xyz, vertices[1].xyz, axis[0]);
		VectorSubtract(vertices[2].xyz, vertices[1].xyz, axis[1]);
		CrossProduct(axis[0], axis[1], axis[2]);
		VectorNormalizeFast(axis[2]);

		MakeNormalVectors(axis[2], axis[1], axis[0]);

		MatrixMultiply(viewMatrix, axis, matrix);

		for (j = 0; j < 4; j++){
			VectorSubtract(vertices[j].xyz, center, tmp);
			VectorRotate(tmp, matrix, vertices[j].xyz);
			VectorAdd(vertices[j].xyz, center, vertices[j].xyz);
		}
	}
}

/*
 =================
 RB_DeformTube
 =================
*/
static void RB_DeformTube (void){

	vertexArray_t	*vertices = backEnd.vertices;
	vec3_t			matrix[3];
	vec3_t			axis[3], invAxis[3];
	vec3_t			edge[3], length;
	vec3_t			normal, dir;
	vec3_t			center, tmp;
	int				shortAxis, longAxis;
	int				i, j;

	if (backEnd.numIndices % 6){
		Com_Printf(S_COLOR_YELLOW "Material '%s' has 'deform tube' with an odd index count\n", backEnd.material->name);
		return;
	}
	if (backEnd.numVertices % 4){
		Com_Printf(S_COLOR_YELLOW "Material '%s' has 'deform tube' with an odd vertex count\n", backEnd.material->name);
		return;
	}

	tr.pc.deforms++;
	tr.pc.deformVertices += backEnd.numVertices;

	for (i = 0; i < backEnd.numVertices; i += 4, vertices += 4){
		center[0] = (vertices[0].xyz[0] + vertices[1].xyz[0] + vertices[2].xyz[0] + vertices[3].xyz[0]) * 0.25;
		center[1] = (vertices[0].xyz[1] + vertices[1].xyz[1] + vertices[2].xyz[1] + vertices[3].xyz[1]) * 0.25;
		center[2] = (vertices[0].xyz[2] + vertices[1].xyz[2] + vertices[2].xyz[2] + vertices[3].xyz[2]) * 0.25;

		if (backEnd.entity == tr.worldEntity)
			VectorSubtract(backEnd.viewParms.origin, center, dir);
		else {
			VectorAdd(center, backEnd.entity->origin, dir);
			VectorSubtract(backEnd.viewParms.origin, dir, tmp);
			VectorRotate(tmp, backEnd.entity->axis, dir);
		}

		VectorSubtract(vertices[1].xyz, vertices[0].xyz, edge[0]);
		VectorSubtract(vertices[2].xyz, vertices[0].xyz, edge[1]);
		VectorSubtract(vertices[2].xyz, vertices[1].xyz, edge[2]);

		length[0] = DotProduct(edge[0], edge[0]);
		length[1] = DotProduct(edge[1], edge[1]);
		length[2] = DotProduct(edge[2], edge[2]);

		if (length[2] > length[1] && length[2] > length[0]){
			if (length[1] > length[0]){
				shortAxis = 0;
				longAxis = 1;
			}
			else {
				shortAxis = 1;
				longAxis = 0;
			}
		}
		else if (length[1] > length[2] && length[1] > length[0]){
			if (length[2] > length[0]){
				shortAxis = 0;
				longAxis = 2;
			}
			else {
				shortAxis = 2;
				longAxis = 0;
			}
		}
		else if (length[0] > length[1] && length[0] > length[2]){
			if (length[2] > length[1]){
				shortAxis = 1;
				longAxis = 2;
			}
			else {
				shortAxis = 2;
				longAxis = 1;
			}
		}
		else {
			shortAxis = 0;
			longAxis = 0;
		}

		VectorNormalize2(edge[longAxis], normal);

		if (DotProduct(edge[shortAxis], edge[longAxis])){
			VectorCopy(normal, edge[1]);

			if (normal[0] || normal[1])
				MakeNormalVectors(edge[1], edge[0], edge[2]);
			else
				MakeNormalVectors(edge[1], edge[2], edge[0]);
		}
		else {
			VectorNormalize2(edge[shortAxis], edge[0]);

			VectorCopy(normal, edge[1]);
			CrossProduct(edge[0], edge[1], edge[2]);
		}

		VectorMA(dir, -DotProduct(dir, normal), normal, axis[2]);
		VectorNormalizeFast(axis[2]);
		VectorCopy(normal, axis[1]);
		CrossProduct(axis[1], axis[2], axis[0]);

		AxisTranspose(axis, invAxis);

		MatrixMultiply(invAxis, edge, matrix);

		for (j = 0; j < 4; j++){
			VectorSubtract(vertices[j].xyz, center, tmp);
			VectorRotate(tmp, matrix, vertices[j].xyz);
			VectorAdd(vertices[j].xyz, center, vertices[j].xyz);
		}
	}
}

/*
 =================
 RB_Deform
 =================
*/
void RB_Deform (void){

	if (r_skipDeforms->integerValue)
		return;

	switch (backEnd.material->deform){
	case DFRM_NONE:

		break;
	case DFRM_EXPAND:
		RB_DeformExpand();

		break;
	case DFRM_MOVE:
		RB_DeformMove();

		break;
	case DFRM_SPRITE:
		RB_DeformSprite();

		break;
	case DFRM_TUBE:
		RB_DeformTube();

		break;
	default:
		Com_Error(ERR_DROP, "RB_Deform: unknown deform in material '%s'", backEnd.material->name);
	}
}

/*
 =================
 RB_MultiplyTextureMatrix
 =================
*/
void RB_MultiplyTextureMatrix (material_t *material, textureStage_t *textureStage, mat4_t matrix){

	float	s, t, angle;
	int		i;

	Matrix4_Identity(matrix);

	for (i = 0; i < textureStage->numTexTransforms; i++){
		switch (textureStage->texTransform[i]){
		case TT_TRANSLATE:
			s = material->expressionRegisters[textureStage->texTransformRegisters[i][0]];
			t = material->expressionRegisters[textureStage->texTransformRegisters[i][1]];

			Matrix4_Translate(matrix, s, t, 0.0);

			break;
		case TT_SCROLL:
			s = material->expressionRegisters[textureStage->texTransformRegisters[i][0]];
			t = material->expressionRegisters[textureStage->texTransformRegisters[i][1]];

			Matrix4_Translate(matrix, s, t, 0.0);

			break;
		case TT_SCALE:
			s = material->expressionRegisters[textureStage->texTransformRegisters[i][0]];
			t = material->expressionRegisters[textureStage->texTransformRegisters[i][1]];

			Matrix4_Scale(matrix, s, t, 1.0);

			break;
		case TT_CENTERSCALE:
			s = material->expressionRegisters[textureStage->texTransformRegisters[i][0]];
			t = material->expressionRegisters[textureStage->texTransformRegisters[i][1]];

			Matrix4_Translate(matrix, 0.5, 0.5, 0.0);
			Matrix4_Scale(matrix, s, t, 1.0);
			Matrix4_Translate(matrix, -0.5, -0.5, 0.0);

			break;
		case TT_SHEAR:
			s = material->expressionRegisters[textureStage->texTransformRegisters[i][0]];
			t = material->expressionRegisters[textureStage->texTransformRegisters[i][1]];

			Matrix4_Translate(matrix, 0.5, 0.5, 0.0);
			Matrix4_Shear(matrix, s, t, 1.0);
			Matrix4_Translate(matrix, -0.5, -0.5, 0.0);

			break;
		case TT_ROTATE:
			angle = material->expressionRegisters[textureStage->texTransformRegisters[i][0]];

			Matrix4_Translate(matrix, 0.5, 0.5, 0.0);
			Matrix4_Rotate(matrix, angle, 0.0, 0.0, 1.0);
			Matrix4_Translate(matrix, -0.5, -0.5, 0.0);

			break;
		default:
			Com_Error(ERR_DROP, "RB_MultiplyTextureMatrix: unknown texTransform in material '%s'", material->name);
		}
	}
}

/*
 =================
 RB_SetupTextureStage
 =================
*/
void RB_SetupTextureStage (textureStage_t *textureStage){

	mat4_t		textureMatrix;
	qboolean	identity;

	switch (textureStage->texGen){
	case TG_EXPLICIT:
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_ST);

		identity = true;

		break;
	case TG_VECTOR:
		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);
		GL_TexGen(GL_T, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, textureStage->texGenVectors[0]);
		qglTexGenfv(GL_T, GL_OBJECT_PLANE, textureStage->texGenVectors[1]);

		identity = true;

		break;
	case TG_REFLECT:
		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_R);

		GL_TexGen(GL_S, GL_REFLECTION_MAP_ARB);
		GL_TexGen(GL_T, GL_REFLECTION_MAP_ARB);
		GL_TexGen(GL_R, GL_REFLECTION_MAP_ARB);

		qglEnableClientState(GL_NORMAL_ARRAY);
		qglNormalPointer(GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_NORMAL);

		Matrix4_Transpose(backEnd.viewParms.worldMatrix, textureMatrix);

		identity = false;

		break;
	case TG_NORMAL:
		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_R);

		GL_TexGen(GL_S, GL_NORMAL_MAP_ARB);
		GL_TexGen(GL_T, GL_NORMAL_MAP_ARB);
		GL_TexGen(GL_R, GL_NORMAL_MAP_ARB);

		qglEnableClientState(GL_NORMAL_ARRAY);
		qglNormalPointer(GL_FLOAT, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_NORMAL);

		Matrix4_Transpose(backEnd.localParms.viewMatrix, textureMatrix);

		identity = false;

		break;
	case TG_SKYBOX:
		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_R);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);
		GL_TexGen(GL_T, GL_OBJECT_LINEAR);
		GL_TexGen(GL_R, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.viewParms.skyBoxMatrix[ 0]);
		qglTexGenfv(GL_T, GL_OBJECT_PLANE, &backEnd.viewParms.skyBoxMatrix[ 4]);
		qglTexGenfv(GL_R, GL_OBJECT_PLANE, &backEnd.viewParms.skyBoxMatrix[ 8]);

		identity = true;

		break;
	case TG_SCREEN:
		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_Q);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);
		GL_TexGen(GL_T, GL_OBJECT_LINEAR);
		GL_TexGen(GL_Q, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, &backEnd.viewParms.mirrorMatrix[ 0]);
		qglTexGenfv(GL_T, GL_OBJECT_PLANE, &backEnd.viewParms.mirrorMatrix[ 4]);
		qglTexGenfv(GL_Q, GL_OBJECT_PLANE, &backEnd.viewParms.mirrorMatrix[12]);

		identity = true;

		break;
	default:
		Com_Error(ERR_DROP, "RB_SetupTextureStage: unknown texGen in material '%s'", backEnd.material->name);
	}

	if (identity){
		if (!textureStage->numTexTransforms)
			GL_LoadIdentity(GL_TEXTURE);
		else {
			RB_MultiplyTextureMatrix(backEnd.material, textureStage, textureMatrix);

			GL_LoadMatrix(GL_TEXTURE, textureMatrix);
		}
	}
	else
		GL_LoadMatrix(GL_TEXTURE, textureMatrix);

	GL_EnableTexture(textureStage->texture->uploadTarget);
	GL_BindTexture(textureStage->texture);

	if (textureStage->texture == tr.currentRenderTexture){
		if (backEnd.material->sort != SORT_POST_PROCESS)
			RB_CaptureCurrentRender();

		return;
	}

	if (textureStage->video){
		R_UpdateVideo(textureStage->video, SEC2MS(backEnd.time));
		return;
	}
}

/*
 =================
 RB_CleanupTextureStage
 =================
*/
void RB_CleanupTextureStage (textureStage_t *textureStage){

	if (textureStage->texGen == TG_EXPLICIT){
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		return;
	}

	if (textureStage->texGen == TG_REFLECT || textureStage->texGen == TG_NORMAL)
		qglDisableClientState(GL_NORMAL_ARRAY);

	GL_Disable(GL_TEXTURE_GEN_Q);
	GL_Disable(GL_TEXTURE_GEN_R);
	GL_Disable(GL_TEXTURE_GEN_T);
	GL_Disable(GL_TEXTURE_GEN_S);
}

/*
 =================
 RB_SetupColorStage
 =================
*/
void RB_SetupColorStage (colorStage_t *colorStage){

	vec4_t		color;
	qboolean	identity;

	color[0] = backEnd.material->expressionRegisters[colorStage->registers[0]];
	color[1] = backEnd.material->expressionRegisters[colorStage->registers[1]];
	color[2] = backEnd.material->expressionRegisters[colorStage->registers[2]];
	color[3] = backEnd.material->expressionRegisters[colorStage->registers[3]];

	if (color[0] >= 1.0 && color[1] >= 1.0 && color[2] >= 1.0 && color[3] >= 1.0)
		identity = true;
	else
		identity = false;

	switch (colorStage->vertexColor){
	case VC_IGNORE:
		if (identity){
			qglColor3f(1.0, 1.0, 1.0);

			GL_TexEnv(GL_REPLACE);
			break;
		}

		qglColor4fv(color);

		GL_TexEnv(GL_MODULATE);

		break;
	case VC_MODULATE:
		qglEnableClientState(GL_COLOR_ARRAY);
		qglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_COLOR);

		GL_TexEnv(GL_MODULATE);

		if (identity)
			break;

		GL_SelectTexture(1);
		GL_EnableTexture(GL_TEXTURE_2D);
		GL_BindTexture(tr.whiteTexture);

		GL_TexEnv(GL_COMBINE_ARB);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_CONSTANT_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

		qglTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

		GL_SelectTexture(0);

		break;
	case VC_INVERSE_MODULATE:
		qglEnableClientState(GL_COLOR_ARRAY);
		qglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertexArray_t), backEnd.vertexBuffer->pointer + BUFFER_OFFSET_COLOR);

		GL_TexEnv(GL_COMBINE_ARB);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_ONE_MINUS_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

		if (identity)
			break;

		GL_SelectTexture(1);
		GL_EnableTexture(GL_TEXTURE_2D);
		GL_BindTexture(tr.whiteTexture);

		GL_TexEnv(GL_COMBINE_ARB);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_CONSTANT_ARB);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

		qglTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

		GL_SelectTexture(0);

		break;
	default:
		Com_Error(ERR_DROP, "RB_SetupColorStage: unknown vertexColor in material '%s'", backEnd.material->name);
	}
}

/*
 =================
 RB_CleanupColorStage
 =================
*/
void RB_CleanupColorStage (colorStage_t *colorStage){

	if (colorStage->vertexColor == VC_IGNORE)
		return;

	GL_SelectTexture(1);
	GL_DisableTexture();
	GL_SelectTexture(0);

	qglDisableClientState(GL_COLOR_ARRAY);
}

/*
 =================
 RB_SetupProgramStage
 =================
*/
void RB_SetupProgramStage (programStage_t *programStage){

	programParm_t	*programParm;
	programMap_t	*programMap;
	vec4_t			parm;
	float			xDiv, yDiv, xScale, yScale;
	float			lightScale;
	int				i;

	// Calculate inverse and scale for render width/height
	xDiv = 1.0 / backEnd.renderWidth;
	yDiv = 1.0 / backEnd.renderHeight;

	if (glConfig.textureNonPowerOfTwo || IsPowerOfTwo(backEnd.renderWidth))
		xScale = 1.0;
	else
		xScale = (float)(backEnd.renderWidth + 1) / NearestPowerOfTwo(backEnd.renderWidth, false);

	if (glConfig.textureNonPowerOfTwo || IsPowerOfTwo(backEnd.renderHeight))
		yScale = 1.0;
	else
		yScale = (float)(backEnd.renderHeight + 1) / NearestPowerOfTwo(backEnd.renderHeight, false);

	// Get the light scale
	if (r_lightScale->floatValue > 1.0)
		lightScale = r_lightScale->floatValue;
	else
		lightScale = 1.0;

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

	// Bind the programs
	GL_Enable(GL_VERTEX_PROGRAM_ARB);
	GL_Enable(GL_FRAGMENT_PROGRAM_ARB);

	GL_BindProgram(programStage->vertexProgram);
	GL_BindProgram(programStage->fragmentProgram);

	// Set up the matrices
	qglMatrixMode(GL_MATRIX0_ARB);
	qglLoadMatrixf(backEnd.viewParms.worldMatrix);

	qglMatrixMode(GL_MATRIX1_ARB);
	qglLoadMatrixf(backEnd.viewParms.skyBoxMatrix);

	qglMatrixMode(GL_MATRIX2_ARB);
	qglLoadMatrixf(backEnd.viewParms.mirrorMatrix);

	// Set up the environment parameters
	qglProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0, backEnd.localParms.viewOrigin[0], backEnd.localParms.viewOrigin[1], backEnd.localParms.viewOrigin[2], 1.0);
	qglProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 1, backEnd.entity->origin[0], backEnd.entity->origin[1], backEnd.entity->origin[2], 1.0);
	qglProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 2, backEnd.entity->axis[0][0], backEnd.entity->axis[0][1], backEnd.entity->axis[0][2], 1.0);
	qglProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 3, backEnd.entity->axis[1][0], backEnd.entity->axis[1][1], backEnd.entity->axis[1][2], 1.0);
	qglProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 4, backEnd.entity->axis[2][0], backEnd.entity->axis[2][1], backEnd.entity->axis[2][2], 1.0);

	qglProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, xDiv, yDiv, 1.0, 1.0);
	qglProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, xScale, yScale, 1.0, 1.0);
	qglProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2, lightScale, lightScale, lightScale, 1.0);

	// Set up the local parameters
	for (i = 0, programParm = programStage->vertexParms; i < programStage->numVertexParms; i++, programParm++){
		parm[0] = backEnd.material->expressionRegisters[programParm->registers[0]];
		parm[1] = backEnd.material->expressionRegisters[programParm->registers[1]];
		parm[2] = backEnd.material->expressionRegisters[programParm->registers[2]];
		parm[3] = backEnd.material->expressionRegisters[programParm->registers[3]];

		qglProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, programParm->index, parm);
	}

	for (i = 0, programParm = programStage->fragmentParms; i < programStage->numFragmentParms; i++, programParm++){
		parm[0] = backEnd.material->expressionRegisters[programParm->registers[0]];
		parm[1] = backEnd.material->expressionRegisters[programParm->registers[1]];
		parm[2] = backEnd.material->expressionRegisters[programParm->registers[2]];
		parm[3] = backEnd.material->expressionRegisters[programParm->registers[3]];

		qglProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, programParm->index, parm);
	}

	// Set up the texture maps
	for (i = 0, programMap = programStage->fragmentMaps; i < programStage->numFragmentMaps; i++, programMap++){
		if (programMap->texture == tr.currentRenderTexture){
			GL_SelectTexture(programMap->index);
			GL_BindTexture(programMap->texture);

			if (backEnd.material->sort != SORT_POST_PROCESS)
				RB_CaptureCurrentRender();

			continue;
		}

		if (programMap->video){
			GL_SelectTexture(programMap->index);
			GL_BindTexture(programMap->texture);

			R_UpdateVideo(programMap->video, SEC2MS(backEnd.time));
			continue;
		}

		GL_BindProgramTexture(programMap->index, programMap->texture);
	}
}

/*
 =================
 RB_CleanupProgramStage
 =================
*/
void RB_CleanupProgramStage (programStage_t *programStage){

	GL_SelectTexture(0);

	GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
	GL_Disable(GL_VERTEX_PROGRAM_ARB);

	qglDisableVertexAttribArrayARB(11);
	qglDisableVertexAttribArrayARB(10);

	qglDisableClientState(GL_COLOR_ARRAY);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	qglDisableClientState(GL_NORMAL_ARRAY);
}

/*
 =================
 RB_SetMaterialState
 =================
*/
void RB_SetMaterialState (void){

	if (backEnd.material->cullType == CT_FRONT_SIDED){
		GL_Enable(GL_CULL_FACE);
		GL_CullFace(GL_FRONT);
	}
	else if (backEnd.material->cullType == CT_BACK_SIDED){
		GL_Enable(GL_CULL_FACE);
		GL_CullFace(GL_BACK);
	}
	else
		GL_Disable(GL_CULL_FACE);

	if (backEnd.material->polygonOffset){
		GL_Enable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);
	}
	else
		GL_Disable(GL_POLYGON_OFFSET_FILL);
}

/*
 =================
 RB_SetStageState
 =================
*/
void RB_SetStageState (stage_t *stage){

	if (backEnd.material->polygonOffset || stage->privatePolygonOffset){
		GL_Enable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);
	}
	else
		GL_Disable(GL_POLYGON_OFFSET_FILL);

	if (stage->alphaTest){
		GL_Enable(GL_ALPHA_TEST);
		GL_AlphaFunc(GL_GREATER, backEnd.material->expressionRegisters[stage->alphaTestRegister]);
	}
	else
		GL_Disable(GL_ALPHA_TEST);

	if (stage->blend){
		GL_Enable(GL_BLEND);
		GL_BlendFunc(stage->blendSrc, stage->blendDst);
	}
	else
		GL_Disable(GL_BLEND);

	if (stage->ignoreAlphaTest)
		GL_DepthFunc(GL_LEQUAL);
	else
		GL_DepthFunc(GL_EQUAL);

	GL_ColorMask(!stage->maskRed, !stage->maskGreen, !stage->maskBlue, !stage->maskAlpha);
}

/*
 =================
 RB_CaptureCurrentRender
 =================
*/
void RB_CaptureCurrentRender (void){

	int		w, h;

	if (r_skipCopyTexture->integerValue)
		return;

	tr.pc.captureRenders++;

	// Find the texture dimensions
	if (glConfig.textureNonPowerOfTwo){
		w = backEnd.renderWidth;
		h = backEnd.renderHeight;
	}
	else {
		w = NearestPowerOfTwo(backEnd.renderWidth, false);
		h = NearestPowerOfTwo(backEnd.renderHeight, false);
	}

	// Update the texture
	GL_BindTexture(tr.currentRenderTexture);

	if (w == tr.currentRenderTexture->uploadWidth && h == tr.currentRenderTexture->uploadHeight){
		tr.pc.captureRenderPixels += backEnd.renderWidth * backEnd.renderHeight;

		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, backEnd.renderWidth, backEnd.renderHeight);
	}
	else {
		tr.pc.captureRenderPixels += w * h;

		// Check the dimensions
		if (w > glConfig.maxTextureSize || h > glConfig.maxTextureSize)
			Com_Error(ERR_DROP, "RB_CaptureCurrentRender: size exceeds hardware limits (%i > %i or %i > %i)", w, glConfig.maxTextureSize, h, glConfig.maxTextureSize);

		// Reallocate the texture
		tr.currentRenderTexture->uploadWidth = w;
		tr.currentRenderTexture->uploadHeight = h;

		if (tr.currentRenderTexture->sourceSamples >= 3)
			tr.currentRenderTexture->uploadSize = w * h * 4;
		else
			tr.currentRenderTexture->uploadSize = w * h * tr.currentRenderTexture->sourceSamples;

		qglCopyTexImage2D(GL_TEXTURE_2D, 0, tr.currentRenderTexture->uploadFormat, 0, 0, w, h, 0);
	}

	// Avoid bilerp issues
	if (w != backEnd.renderWidth){
		tr.pc.captureRenderPixels += backEnd.renderHeight;

		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, backEnd.renderWidth, 0, backEnd.renderWidth - 1, 0, 1, backEnd.renderHeight);
	}

	if (h != glConfig.videoHeight){
		tr.pc.captureRenderPixels += backEnd.renderWidth;

		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, backEnd.renderHeight, 0, backEnd.renderHeight - 1, backEnd.renderWidth, 1);
	}
}


/*
 =======================================================================

 RENDERING UTILITIES

 =======================================================================
*/


/*
 =================
 RB_SetupView
 =================
*/
static void RB_SetupView (void){

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_SetupView ----------\n");

	// Set the state
	backEnd.overlay = false;

	if (r_finish->integerValue)
		qglFinish();

	// Set the viewport
	qglViewport(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Set the scissor rect
	GL_Scissor(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Set the depth bounds
	if (glConfig.depthBoundsTest)
		GL_DepthBounds(0.0, 1.0);

	// Set up projection
	GL_LoadMatrix(GL_PROJECTION, backEnd.viewParms.perspectiveMatrix);

	// Set up modelview
	GL_LoadMatrix(GL_MODELVIEW, backEnd.viewParms.worldMatrix);

	// Set the GL state
	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_TRUE);
	GL_StencilMask(255);

	GL_DepthRange(0.0, 1.0);

	if (backEnd.viewParms.subview != SUBVIEW_MIRROR)
		GL_FrontFace(GL_CCW);
	else
		GL_FrontFace(GL_CW);

	// Clear the buffers
	qglClearColor(0.0, 0.0, 0.0, 1.0);
	qglClearDepth(1.0);
	qglClearStencil(128);

	if (backEnd.viewParms.subview == SUBVIEW_NONE)
		qglClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	else
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 =================
 RB_SetupOverlay
 =================
*/
static void RB_SetupOverlay (void){

	mat4_t	orthoMatrix = {2.0, 0.0, 0.0, 0.0, 0.0, -2.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, -1.0, 1.0, 0.0, 1.0};

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_SetupOverlay ----------\n");

	// Set the state
	backEnd.overlay = true;
	backEnd.time = MS2SEC(Sys_Milliseconds());

	backEnd.viewport.x = 0;
	backEnd.viewport.y = 0;
	backEnd.viewport.width = backEnd.renderWidth;
	backEnd.viewport.height = backEnd.renderHeight;

	if (r_finish->integerValue)
		qglFinish();

	// Set the viewport
	qglViewport(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Set the scissor rect
	GL_Scissor(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Set the depth bounds
	if (glConfig.depthBoundsTest)
		GL_DepthBounds(0.0, 1.0);

	// Set up projection
	orthoMatrix[ 0] /= backEnd.renderWidth;
	orthoMatrix[ 5] /= backEnd.renderHeight;

	GL_LoadMatrix(GL_PROJECTION, orthoMatrix);

	// Set up modelview
	GL_LoadIdentity(GL_MODELVIEW);

	// Set the GL state
	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	GL_DepthRange(0.0, 1.0);

	GL_FrontFace(GL_CCW);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 =================
 RB_LightScale
 =================
*/
static void RB_LightScale (void){

	float	lightScale = r_lightScale->floatValue;

	if (r_skipInteractions->integerValue)
		return;

	if (backEnd.activePath == PATH_ARB2)
		return;

	if (lightScale <= 1.0)
		return;

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_LightScale ----------\n");

	// Set the GL state
	GL_LoadIdentity(GL_MODELVIEW);

	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_DST_COLOR, GL_SRC_COLOR);

	GL_Disable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	GL_DepthRange(0.0, 1.0);

	// Draw fullscreen quads
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();

	while (lightScale >= 2.0){
		lightScale *= 0.5;

		qglColor3f(1.0, 1.0, 1.0);

		qglBegin(GL_QUADS);
		qglVertex3f(-1.0, -1.0, -1.0);
		qglVertex3f( 1.0, -1.0, -1.0);
		qglVertex3f( 1.0,  1.0, -1.0);
		qglVertex3f(-1.0,  1.0, -1.0);
		qglEnd();
	}

	if (lightScale > 1.0){
		lightScale *= 0.5;

		qglColor3f(lightScale, lightScale, lightScale);

		qglBegin(GL_QUADS);
		qglVertex3f(-1.0, -1.0, -1.0);
		qglVertex3f( 1.0, -1.0, -1.0);
		qglVertex3f( 1.0,  1.0, -1.0);
		qglVertex3f(-1.0,  1.0, -1.0);
		qglEnd();
	}

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 =================
 RB_SetFogLightMatrix
 =================
*/
static void RB_SetFogLightMatrix (renderEntity_t *entity, light_t *light, material_t *lightMaterial, stage_t *lightStage){

	vec4_t	distanceVector, depthVector, viewDepthVector;
	float	distanceScale, depthScale;

	// Compute the fog distance and depth scales
	distanceScale = lightMaterial->expressionRegisters[lightStage->colorStage.registers[3]];

	if (distanceScale <= 1.0)
		distanceScale = 0.5 / 500;
	else
		distanceScale = 0.5 / distanceScale;

	depthScale = 0.5 / 500;

	// Set up the distance vector
	distanceVector[0] = backEnd.localParms.viewMatrix[ 2] * distanceScale;
	distanceVector[1] = backEnd.localParms.viewMatrix[ 6] * distanceScale;
	distanceVector[2] = backEnd.localParms.viewMatrix[10] * distanceScale;
	distanceVector[3] = backEnd.localParms.viewMatrix[14] * distanceScale + 0.5;

	// Set up the depth vector
	if (entity == tr.worldEntity){
		depthVector[0] = light->planeNormal[0] * depthScale;
		depthVector[1] = light->planeNormal[1] * depthScale;
		depthVector[2] = light->planeNormal[2] * depthScale;
		depthVector[3] = -light->planeDist * depthScale + 0.5;
	}
	else {
		depthVector[0] = DotProduct(light->planeNormal, entity->axis[0]) * depthScale;
		depthVector[1] = DotProduct(light->planeNormal, entity->axis[1]) * depthScale;
		depthVector[2] = DotProduct(light->planeNormal, entity->axis[2]) * depthScale;
		depthVector[3] = (DotProduct(entity->origin, light->planeNormal) - light->planeDist) * depthScale + 0.5;
	}

	// Set up the view-depth vector
	viewDepthVector[0] = 0.0;
	viewDepthVector[1] = 0.0;
	viewDepthVector[2] = 0.0;
	viewDepthVector[3] = (DotProduct(backEnd.viewParms.origin, light->planeNormal) - light->planeDist) * depthScale + 0.5;

	// Set up the object planes
	GL_SelectTexture(0);

	qglTexGenfv(GL_S, GL_OBJECT_PLANE, distanceVector);
	qglTexGenfv(GL_T, GL_OBJECT_PLANE, distanceVector);

	GL_SelectTexture(1);

	qglTexGenfv(GL_S, GL_OBJECT_PLANE, depthVector);
	qglTexGenfv(GL_T, GL_OBJECT_PLANE, viewDepthVector);
}

/*
 =================
 RB_SetBlendLightMatrix
 =================
*/
static void RB_SetBlendLightMatrix (renderEntity_t *entity, light_t *light, material_t *lightMaterial, stage_t *lightStage){

	mat4_t	transformMatrix, entityMatrix, textureMatrix;
	mat4_t	lightMatrix, tmpMatrix;

	// Set up the light matrix
	if (entity == tr.worldEntity){
		if (!lightStage->textureStage.numTexTransforms)
			Matrix4_Transpose(light->matrix, lightMatrix);
		else {
			RB_MultiplyTextureMatrix(lightMaterial, &lightStage->textureStage, textureMatrix);

			Matrix4_MultiplyFast(textureMatrix, light->matrix, tmpMatrix);
			Matrix4_Transpose(tmpMatrix, lightMatrix);
		}
	}
	else {
		Matrix4_Set(transformMatrix, entity->axis, entity->origin);
		Matrix4_MultiplyFast(light->matrix, transformMatrix, entityMatrix);

		if (!lightStage->textureStage.numTexTransforms)
			Matrix4_Transpose(entityMatrix, lightMatrix);
		else {
			RB_MultiplyTextureMatrix(lightMaterial, &lightStage->textureStage, textureMatrix);

			Matrix4_MultiplyFast(textureMatrix, entityMatrix, tmpMatrix);
			Matrix4_Transpose(tmpMatrix, lightMatrix);
		}
	}

	// Set up the object planes
	GL_SelectTexture(0);

	qglTexGenfv(GL_S, GL_OBJECT_PLANE, &lightMatrix[ 0]);
	qglTexGenfv(GL_T, GL_OBJECT_PLANE, &lightMatrix[ 4]);
	qglTexGenfv(GL_Q, GL_OBJECT_PLANE, &lightMatrix[12]);

	GL_SelectTexture(1);

	qglTexGenfv(GL_S, GL_OBJECT_PLANE, &lightMatrix[ 8]);
}

/*
 =================
 RB_SetLightMatrix
 =================
*/
static void RB_SetLightMatrix (renderEntity_t *entity, light_t *light, material_t *lightMaterial, stage_t *lightStage){

	mat4_t	transformMatrix, entityMatrix, textureMatrix;
	mat4_t	tmpMatrix;

	// Set up the light matrix
	if (entity == tr.worldEntity){
		if (!lightStage->textureStage.numTexTransforms)
			Matrix4_Transpose(light->matrix, backEnd.localParms.lightMatrix);
		else {
			RB_MultiplyTextureMatrix(lightMaterial, &lightStage->textureStage, textureMatrix);

			Matrix4_MultiplyFast(textureMatrix, light->matrix, tmpMatrix);
			Matrix4_Transpose(tmpMatrix, backEnd.localParms.lightMatrix);
		}
	}
	else {
		Matrix4_Set(transformMatrix, entity->axis, entity->origin);
		Matrix4_MultiplyFast(light->matrix, transformMatrix, entityMatrix);

		if (!lightStage->textureStage.numTexTransforms)
			Matrix4_Transpose(entityMatrix, backEnd.localParms.lightMatrix);
		else {
			RB_MultiplyTextureMatrix(lightMaterial, &lightStage->textureStage, textureMatrix);

			Matrix4_MultiplyFast(textureMatrix, entityMatrix, tmpMatrix);
			Matrix4_Transpose(tmpMatrix, backEnd.localParms.lightMatrix);
		}
	}
}

/*
 =================
 RB_SetLightState
 =================
*/
void RB_SetLightState (renderEntity_t *entity, light_t *light, material_t *lightMaterial, stage_t *lightStage){

	vec3_t	tmpOrigin;

	// Transform light origin to local space
	if (entity == tr.worldEntity)
		VectorCopy(light->origin, backEnd.localParms.lightOrigin);
	else {
		VectorSubtract(light->origin, entity->origin, tmpOrigin);
		VectorRotate(tmpOrigin, entity->axis, backEnd.localParms.lightOrigin);
	}

	// Set up the light matrix if needed
	if (!lightMaterial || !lightStage)
		return;

	if (lightMaterial->fogLight)
		RB_SetFogLightMatrix(entity, light, lightMaterial, lightStage);
	else if (lightMaterial->blendLight)
		RB_SetBlendLightMatrix(entity, light, lightMaterial, lightStage);
	else
		RB_SetLightMatrix(entity, light, lightMaterial, lightStage);
}

/*
 =================
 RB_SetEntityState
 =================
*/
void RB_SetEntityState (renderEntity_t *entity){

	mat4_t	transformMatrix;
	vec3_t	tmpOrigin;

	if (entity == tr.worldEntity){
		VectorCopy(backEnd.viewParms.origin, backEnd.localParms.viewOrigin);
		Matrix4_Copy(backEnd.viewParms.worldMatrix, backEnd.localParms.viewMatrix);

		GL_DepthRange(0.0, 1.0);
	}
	else {
		VectorSubtract(backEnd.viewParms.origin, entity->origin, tmpOrigin);
		VectorRotate(tmpOrigin, entity->axis, backEnd.localParms.viewOrigin);

		if (entity->reType == RE_MODEL){
			Matrix4_Set(transformMatrix, entity->axis, entity->origin);
			Matrix4_MultiplyFast(backEnd.viewParms.worldMatrix, transformMatrix, backEnd.localParms.viewMatrix);
		}
		else
			Matrix4_Copy(backEnd.viewParms.worldMatrix, backEnd.localParms.viewMatrix);

		if (entity->depthHack)
			GL_DepthRange(0.0, entity->depthHack);
		else
			GL_DepthRange(0.0, 1.0);
	}

	GL_LoadMatrix(GL_MODELVIEW, backEnd.localParms.viewMatrix);
}

/*
 =================
 RB_DrawElements
 =================
*/
void RB_DrawElements (void){

	index_t	*indices;

	tr.pc.draws++;

	if (!backEnd.stencilShadow){
		tr.pc.indices += backEnd.numIndices;
		tr.pc.vertices += backEnd.numVertices;

		indices = backEnd.indices;
	}
	else {
		tr.pc.shadowIndices += backEnd.numIndices;
		tr.pc.shadowVertices += backEnd.numVertices;

		indices = backEnd.shadowIndices;
	}

	if (glConfig.drawRangeElements)
		qglDrawRangeElementsEXT(GL_TRIANGLES, 0, backEnd.numVertices, backEnd.numIndices, GL_UNSIGNED_INT, indices);
	else
		qglDrawElements(GL_TRIANGLES, backEnd.numIndices, GL_UNSIGNED_INT, indices);
}


/*
 =======================================================================

 RENDER COMMANDS EXECUTION

 =======================================================================
*/


/*
 =================
 RB_RenderView
 =================
*/
static const void *RB_RenderView (const renderViewCommand_t *cmd){

	if (r_skipRender->integerValue)
		return (const void *)(cmd + 1);

	// Render any remaining pics
	if (backEnd.overlay)
		RB_EndBatch();

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_RenderView ----------\n");

	tr.pc.views++;

	// Set the state
	backEnd.time = cmd->time;
	memcpy(backEnd.materialParms, cmd->materialParms, MAX_GLOBAL_MATERIAL_PARMS * sizeof(float));

	backEnd.viewport = cmd->viewport;
	backEnd.viewParms = cmd->viewParms;

	// Development tool
	if (r_skipRenderContext->integerValue)
		GLimp_ActivateContext(false);

	// Switch to 3D mode
	RB_SetupView();

	// Z-fill pass
	RB_FillDepthBuffer(cmd->meshes, cmd->numMeshes);

	// Stencil shadow & light interaction pass
	RB_RenderLights(cmd->lights, cmd->numLights);

	// Scale light intensities
	RB_LightScale();

	// Ambient pass (opaque & translucent)
	RB_RenderMaterialPasses(cmd->meshes, cmd->numMeshes, false);

	// Fog & blend light interaction pass
	RB_FogAllLights(cmd->fogLights, cmd->numFogLights);

	// Ambient pass (post-process)
	RB_RenderMaterialPasses(cmd->postProcessMeshes, cmd->numPostProcessMeshes, true);

	// Render debug tools
	RB_RenderDebugTools(cmd);

	// Development tool
	if (r_skipRenderContext->integerValue){
		GLimp_ActivateContext(true);

		GL_SetDefaultState();
	}

	return (const void *)(cmd + 1);
}

/*
 =================
 RB_CaptureRender
 =================
*/
static const void *RB_CaptureRender (const captureRenderCommand_t *cmd){

	texture_t	*texture = cmd->texture;

	if (r_skipCopyTexture->integerValue)
		return (const void *)(cmd + 1);

	// Render any remaining pics
	if (backEnd.overlay)
		RB_EndBatch();

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_CaptureRender ----------\n");

	tr.pc.captureRenders++;
	tr.pc.captureRenderPixels += backEnd.renderWidth * backEnd.renderHeight;

	// Update the texture
	GL_BindTexture(texture);

	if (backEnd.renderWidth == texture->uploadWidth && backEnd.renderHeight == texture->uploadHeight)
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, backEnd.renderWidth, backEnd.renderHeight);
	else {
		// Check the dimensions
		if (!glConfig.textureNonPowerOfTwo){
			if (!IsPowerOfTwo(backEnd.renderWidth) || !IsPowerOfTwo(backEnd.renderHeight))
				Com_Error(ERR_DROP, "RB_CaptureRender: size is not a power of two (%i x %i)", backEnd.renderWidth, backEnd.renderHeight);
		}

		if (backEnd.renderWidth > glConfig.maxTextureSize || backEnd.renderHeight > glConfig.maxTextureSize)
			Com_Error(ERR_DROP, "RB_CaptureRender: size exceeds hardware limits (%i > %i or %i > %i)", backEnd.renderWidth, glConfig.maxTextureSize, backEnd.renderHeight, glConfig.maxTextureSize);

		// Reallocate the texture
		texture->uploadWidth = backEnd.renderWidth;
		texture->uploadHeight = backEnd.renderHeight;

		if (texture->sourceSamples >= 3)
			texture->uploadSize = backEnd.renderWidth * backEnd.renderHeight * 4;
		else
			texture->uploadSize = backEnd.renderWidth * backEnd.renderHeight * texture->sourceSamples;

		qglCopyTexImage2D(GL_TEXTURE_2D, 0, texture->uploadFormat, 0, 0, backEnd.renderWidth, backEnd.renderHeight, 0);
	}

	return (const void *)(cmd + 1);
}

/*
 =================
 RB_UpdateTexture
 =================
*/
static const void *RB_UpdateTexture (const updateTextureCommand_t *cmd){

	texture_t	*texture = cmd->texture;

	if (r_skipDynamicTextures->integerValue)
		return (const void *)(cmd + 1);

	// Render any remaining pics
	if (backEnd.overlay)
		RB_EndBatch();

	if (r_logFile->integerValue)
		QGL_LogPrintf("---------- RB_UpdateTexture ----------\n");

	tr.pc.updateTextures++;
	tr.pc.updateTexturePixels += cmd->width * cmd->height;

	// Update the texture
	GL_BindTexture(texture);

	if (cmd->width == texture->uploadWidth && cmd->height == texture->uploadHeight)
		qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cmd->width, cmd->height, GL_RGBA, GL_UNSIGNED_BYTE, cmd->image);
	else {
		// Check the dimensions
		if (!glConfig.textureNonPowerOfTwo){
			if (!IsPowerOfTwo(cmd->width) || !IsPowerOfTwo(cmd->height))
				Com_Error(ERR_DROP, "RB_UpdateTexture: size is not a power of two (%i x %i)", cmd->width, cmd->height);
		}

		if (cmd->width > glConfig.maxTextureSize || cmd->height > glConfig.maxTextureSize)
			Com_Error(ERR_DROP, "RB_UpdateTexture: size exceeds hardware limits (%i > %i or %i > %i)", cmd->width, glConfig.maxTextureSize, cmd->height, glConfig.maxTextureSize);

		// Reallocate the texture
		texture->uploadWidth = cmd->width;
		texture->uploadHeight = cmd->height;

		if (texture->sourceSamples >= 3)
			texture->uploadSize = cmd->width * cmd->height * 4;
		else
			texture->uploadSize = cmd->width * cmd->height * texture->sourceSamples;

		qglTexImage2D(GL_TEXTURE_2D, 0, texture->uploadFormat, cmd->width, cmd->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, cmd->image);
	}

	return (const void *)(cmd + 1);
}

/*
 =================
 RB_StretchPic
 =================
*/
static const void *RB_StretchPic (const stretchPicCommand_t *cmd){

	material_t		*material = cmd->material;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i;

	// Switch to 2D mode if needed
	if (!backEnd.overlay){
		RB_SetupOverlay();

		// Set the render function
		backEnd.renderBatch = RB_RenderOverlayMaterial;

		// Clear the batch state
		backEnd.material = NULL;
		backEnd.entity = NULL;

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;
	}

	// Check if the material changed
	if (material != backEnd.material || r_skipBatching->integerValue){
		// Render the last batch
		RB_EndBatch();

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, tr.worldEntity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			return (const void *)(cmd + 1);

		// Create a new batch
		RB_BeginBatch(material, tr.worldEntity, false, false);
	}

	// Skip if condition evaluated to false
	if (!backEnd.material->expressionRegisters[backEnd.material->conditionRegister])
		return (const void *)(cmd + 1);

	// Check for overflow
	RB_CheckOverflow(6, 4, MAX_INDICES, MAX_VERTICES);

	// Set up indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 2; i < 4; i++){
		indices[0] = backEnd.numVertices + 0;
		indices[1] = backEnd.numVertices + i-1;
		indices[2] = backEnd.numVertices + i;

		indices += 3;
	}

	backEnd.numIndices += 6;

	// Set up vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	vertices[0].xyz[0] = cmd->x;
	vertices[0].xyz[1] = cmd->y;
	vertices[0].xyz[2] = 0.0;
	vertices[1].xyz[0] = cmd->x + cmd->w;
	vertices[1].xyz[1] = cmd->y;
	vertices[1].xyz[2] = 0.0;
	vertices[2].xyz[0] = cmd->x + cmd->w;
	vertices[2].xyz[1] = cmd->y + cmd->h;
	vertices[2].xyz[2] = 0.0;
	vertices[3].xyz[0] = cmd->x;
	vertices[3].xyz[1] = cmd->y + cmd->h;
	vertices[3].xyz[2] = 0.0;

	vertices[0].st[0] = cmd->s1;
	vertices[0].st[1] = cmd->t1;
	vertices[1].st[0] = cmd->s2;
	vertices[1].st[1] = cmd->t1;
	vertices[2].st[0] = cmd->s2;
	vertices[2].st[1] = cmd->t2;
	vertices[3].st[0] = cmd->s1;
	vertices[3].st[1] = cmd->t2;

	for (i = 0; i < 4; i++){
		vertices->color[0] = cmd->color[0];
		vertices->color[1] = cmd->color[1];
		vertices->color[2] = cmd->color[2];
		vertices->color[3] = cmd->color[3];

		vertices++;
	}

	backEnd.numVertices += 4;

	return (const void *)(cmd + 1);
}

/*
 =================
 RB_ShearedPic
 =================
*/
static const void *RB_ShearedPic (const shearedPicCommand_t *cmd){

	material_t		*material = cmd->material;
	index_t			*indices;
	vertexArray_t	*vertices;
	int				i;

	// Switch to 2D mode if needed
	if (!backEnd.overlay){
		RB_SetupOverlay();

		// Set the render function
		backEnd.renderBatch = RB_RenderOverlayMaterial;

		// Clear the batch state
		backEnd.material = NULL;
		backEnd.entity = NULL;

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;
	}

	// Check if the material changed
	if (material != backEnd.material || r_skipBatching->integerValue){
		// Render the last batch
		RB_EndBatch();

		// Evaluate registers
		R_EvaluateRegisters(material, backEnd.time, tr.worldEntity->materialParms, backEnd.materialParms);

		// Skip if condition evaluated to false
		if (!material->expressionRegisters[material->conditionRegister])
			return (const void *)(cmd + 1);

		// Create a new batch
		RB_BeginBatch(material, tr.worldEntity, false, false);
	}

	// Skip if condition evaluated to false
	if (!backEnd.material->expressionRegisters[backEnd.material->conditionRegister])
		return (const void *)(cmd + 1);

	// Check for overflow
	RB_CheckOverflow(6, 4, MAX_INDICES, MAX_VERTICES);

	// Set up indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 2; i < 4; i++){
		indices[0] = backEnd.numVertices + 0;
		indices[1] = backEnd.numVertices + i-1;
		indices[2] = backEnd.numVertices + i;

		indices += 3;
	}

	backEnd.numIndices += 6;

	// Set up vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	vertices[0].xyz[0] = cmd->x + cmd->shearX;
	vertices[0].xyz[1] = cmd->y + cmd->shearY;
	vertices[0].xyz[2] = 0.0;
	vertices[1].xyz[0] = cmd->x + cmd->w + cmd->shearX;
	vertices[1].xyz[1] = cmd->y - cmd->shearY;
	vertices[1].xyz[2] = 0.0;
	vertices[2].xyz[0] = cmd->x + cmd->w - cmd->shearX;
	vertices[2].xyz[1] = cmd->y + cmd->h - cmd->shearY;
	vertices[2].xyz[2] = 0.0;
	vertices[3].xyz[0] = cmd->x - cmd->shearX;
	vertices[3].xyz[1] = cmd->y + cmd->h + cmd->shearY;
	vertices[3].xyz[2] = 0.0;

	vertices[0].st[0] = cmd->s1;
	vertices[0].st[1] = cmd->t1;
	vertices[1].st[0] = cmd->s2;
	vertices[1].st[1] = cmd->t1;
	vertices[2].st[0] = cmd->s2;
	vertices[2].st[1] = cmd->t2;
	vertices[3].st[0] = cmd->s1;
	vertices[3].st[1] = cmd->t2;

	for (i = 0; i < 4; i++){
		vertices->color[0] = cmd->color[0];
		vertices->color[1] = cmd->color[1];
		vertices->color[2] = cmd->color[2];
		vertices->color[3] = cmd->color[3];

		vertices++;
	}

	backEnd.numVertices += 4;

	return (const void *)(cmd + 1);
}

/*
 =================
 RB_RenderSize
 =================
*/
static const void *RB_RenderSize (const renderSizeCommand_t *cmd){

	// Render any remaining pics
	if (backEnd.overlay)
		RB_EndBatch();

	// Force a mode switch
	backEnd.overlay = false;

	// Set the render size
	backEnd.renderWidth = cmd->width;
	backEnd.renderHeight = cmd->height;

	return (const void *)(cmd + 1);
}

/*
 =================
 RB_DrawBuffer
 =================
*/
static const void *RB_DrawBuffer (const drawBufferCommand_t *cmd){

	float	r, g, b;

	// Set the draw buffer
	if (r_frontBuffer->integerValue)
		qglDrawBuffer(GL_FRONT);
	else
		qglDrawBuffer(GL_BACK);

	// Clear screen if desired
	if (r_clear->integerValue){
		r = colorTable[r_clearColor->integerValue & Q_COLOR_MASK][0] * (1.0/255);
		g = colorTable[r_clearColor->integerValue & Q_COLOR_MASK][1] * (1.0/255);
		b = colorTable[r_clearColor->integerValue & Q_COLOR_MASK][2] * (1.0/255);

		GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		qglClearColor(r, g, b, 1.0);
		qglClear(GL_COLOR_BUFFER_BIT);
	}

	// Set the render size
	backEnd.renderWidth = cmd->width;
	backEnd.renderHeight = cmd->height;

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	return (const void *)(cmd + 1);
}

/*
 =================
 RB_SwapBuffers
 =================
*/
static const void *RB_SwapBuffers (const swapBuffersCommand_t *cmd){

	// Render any remaining pics
	if (backEnd.overlay)
		RB_EndBatch();

	// Force a mode switch
	backEnd.overlay = false;

	// Swap the buffers
	if (r_finish->integerValue)
		qglFinish();

	GLimp_SwapBuffers();

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	// Log file
	if (r_logFile->integerValue)
		QGL_LogPrintf("\n========== RB_SwapBuffers ==========\n\n");

	return (const void *)(cmd + 1);
}

/*
 =================
 RB_ExecuteRenderCommands
 =================
*/
static void RB_ExecuteRenderCommands (const void *data){

	double	time;

	if (r_skipBackEnd->integerValue)
		return;

	if (r_showRenderTime->integerValue)
		time = Sys_GetClockTicks();

	while (1){
		switch (*(const int *)data){
		case RC_RENDER_VIEW:
			data = RB_RenderView(data);

			break;
		case RC_CAPTURE_RENDER:
			data = RB_CaptureRender(data);

			break;
		case RC_UPDATE_TEXTURE:
			data = RB_UpdateTexture(data);

			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic(data);

			break;
		case RC_SHEARED_PIC:
			data = RB_ShearedPic(data);

			break;
		case RC_RENDER_SIZE:
			data = RB_RenderSize(data);

			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer(data);

			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers(data);

			break;
		case RC_END_OF_LIST:
			if (r_showRenderTime->integerValue)
				tr.pc.timeBackEnd += (Sys_GetClockTicks() - time);

			return;
		default:
			Com_Error(ERR_FATAL, "RB_ExecuteRenderCommands: bad commandId (%i)", *(const int *)data);
		}
	}
}

/*
 =================
 RB_ClearRenderCommands
 =================
*/
void RB_ClearRenderCommands (void){

	renderCommandList_t	*commandList = &backEnd.commandList;

	// Clear it out
	commandList->used = 0;
}

/*
 =================
 RB_IssueRenderCommands
 =================
*/
void RB_IssueRenderCommands (void){

	renderCommandList_t	*commandList = &backEnd.commandList;

	if (!commandList->used)
		return;

	// Add an end of list command
	*(int *)(commandList->data + commandList->used) = RC_END_OF_LIST;

	// Execute the commands
	RB_ExecuteRenderCommands(commandList->data);
}

/*
 =================
 RB_GetCommandBuffer
 =================
*/
void *RB_GetCommandBuffer (int size){

	renderCommandList_t	*commandList = &backEnd.commandList;

	// Always leave room for the end of list command
	if (commandList->used + size + sizeof(int) > MAX_RENDER_COMMANDS){
		if (size > MAX_RENDER_COMMANDS - sizeof(int))
			Com_Error(ERR_FATAL, "RB_GetCommandBuffer: %i > MAX_RENDER_COMMANDS", size);

		// If we run out of room, just start dropping commands
		return NULL;
	}

	commandList->used += size;

	return commandList->data + commandList->used - size;
}


/*
 =======================================================================

 BACK-END INITIALIZATION

 =======================================================================
*/


/*
 =================
 RB_SetActiveRenderer
 =================
*/
void RB_SetActiveRenderer (void){

	if (!Q_stricmp(r_renderer->value, "ARB"))
		backEnd.activePath = PATH_ARB;
	else if (!Q_stricmp(r_renderer->value, "NV10")){
		if (glConfig.allowNV10Path)
			backEnd.activePath = PATH_NV10;
		else {
			Cvar_SetString("r_renderer", "ARB");

			backEnd.activePath = PATH_ARB;
		}
	}
	else if (!Q_stricmp(r_renderer->value, "NV20")){
		if (glConfig.allowNV20Path)
			backEnd.activePath = PATH_NV20;
		else {
			Cvar_SetString("r_renderer", "ARB");

			backEnd.activePath = PATH_ARB;
		}
	}
	else if (!Q_stricmp(r_renderer->value, "NV30")){
		if (glConfig.allowNV30Path)
			backEnd.activePath = PATH_NV30;
		else {
			Cvar_SetString("r_renderer", "ARB");

			backEnd.activePath = PATH_ARB;
		}
	}
	else if (!Q_stricmp(r_renderer->value, "R200")){
		if (glConfig.allowR200Path)
			backEnd.activePath = PATH_R200;
		else {
			Cvar_SetString("r_renderer", "ARB");

			backEnd.activePath = PATH_ARB;
		}
	}
	else if (!Q_stricmp(r_renderer->value, "ARB2")){
		if (glConfig.allowARB2Path)
			backEnd.activePath = PATH_ARB2;
		else {
			Cvar_SetString("r_renderer", "ARB");

			backEnd.activePath = PATH_ARB;
		}
	}
	else {
		if (!Q_stricmp(r_renderer->value, "")){
			backEnd.activePath = PATH_ARB;

			if (glConfig.allowNV10Path)
				backEnd.activePath = PATH_NV10;
			if (glConfig.allowNV20Path)
				backEnd.activePath = PATH_NV20;
			if (glConfig.allowR200Path)
				backEnd.activePath = PATH_R200;
			if (glConfig.allowARB2Path)
				backEnd.activePath = PATH_ARB2;
		}
		else {
			Cvar_SetString("r_renderer", "ARB");

			backEnd.activePath = PATH_ARB;
		}
	}

	r_renderer->modified = false;

	// Set the render function
	switch (backEnd.activePath){
	case PATH_ARB:
		Com_Printf("Using ARB path\n");

		backEnd.renderInteraction = RB_ARB_RenderInteraction;

		break;
	case PATH_NV10:
		Com_Printf("Using NV10 path\n");

		backEnd.renderInteraction = RB_NV10_RenderInteraction;

		break;
	case PATH_NV20:
		Com_Printf("Using NV20 path\n");

		backEnd.renderInteraction = RB_NV20_RenderInteraction;

		break;
	case PATH_R200:
		Com_Printf("Using R200 path\n");

		backEnd.renderInteraction = RB_R200_RenderInteraction;

		break;
	case PATH_ARB2:
		Com_Printf("Using ARB2 path\n");

		backEnd.renderInteraction = RB_ARB2_RenderInteraction;

		break;
	}
}

/*
 =================
 RB_InitBackEnd
 =================
*/
void RB_InitBackEnd (void){

	// Initialize the hardware-specific renderers
	RB_NV10_InitRenderer();
	RB_NV20_InitRenderer();
	RB_R200_InitRenderer();
	RB_ARB2_InitRenderer();

	// Set the active renderer
	RB_SetActiveRenderer();

	// Initialize vertex buffers
	RB_InitVertexBuffers();

	// Force a mode switch
	backEnd.overlay = false;

	// Clear the batch state
	backEnd.material = NULL;
	backEnd.entity = NULL;

	backEnd.stencilShadow = false;
	backEnd.shadowCaps = false;

	// Clear the arrays
	backEnd.numIndices = 0;
	backEnd.numVertices = 0;
}

/*
 =================
 RB_ShutdownBackEnd
 =================
*/
void RB_ShutdownBackEnd (void){

	int		i;

	// Disable arrays
	for (i = MAX_TEXTURE_UNITS - 1; i >= 0; i--){
		if (glConfig.fragmentProgram){
			if (i >= glConfig.maxTextureCoords || i >= glConfig.maxTextureImageUnits)
				continue;
		}
		else {
			if (i >= glConfig.maxTextureUnits)
				continue;
		}

		GL_SelectTexture(i);

		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	qglDisableClientState(GL_COLOR_ARRAY);
	qglDisableClientState(GL_NORMAL_ARRAY);
	qglDisableClientState(GL_VERTEX_ARRAY);

	// Shutdown vertex buffers
	RB_ShutdownVertexBuffers();
}
