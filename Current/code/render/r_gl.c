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


// r_gl.c -- GL state manager


#include "r_local.h"


static glState_t	glState;


/*
 =================
 GL_SelectTexture
 =================
*/
void GL_SelectTexture (unsigned unit){

	if (glState.activeUnit == unit)
		return;
	glState.activeUnit = unit;

	qglActiveTextureARB(GL_TEXTURE0_ARB + unit);
	qglClientActiveTextureARB(GL_TEXTURE0_ARB + unit);
}

/*
 =================
 GL_EnableTexture
 =================
*/
void GL_EnableTexture (unsigned target){

	if (glState.activeTarget[glState.activeUnit] == target)
		return;

	if (glState.activeTarget[glState.activeUnit])
		qglDisable(glState.activeTarget[glState.activeUnit]);

	qglEnable(target);

	glState.activeTarget[glState.activeUnit] = target;
}

/*
 =================
 GL_DisableTexture
 =================
*/
void GL_DisableTexture (void){

	if (glState.activeTarget[glState.activeUnit] == 0)
		return;

	qglDisable(glState.activeTarget[glState.activeUnit]);

	glState.activeTarget[glState.activeUnit] = 0;
}

/*
 =================
 GL_BindTexture
 =================
*/
void GL_BindTexture (texture_t *texture){

	if (texture->frameCount != tr.frameCount){
		texture->frameCount = tr.frameCount;

		tr.pc.textures++;
		tr.pc.textureBytes += texture->uploadSize;
	}

	if (glState.texNum[glState.activeUnit] == texture->texNum)
		return;
	glState.texNum[glState.activeUnit] = texture->texNum;

	qglBindTexture(texture->uploadTarget, texture->texNum);
}

/*
 =================
 GL_TexEnv
 =================
*/
void GL_TexEnv (int texEnv){

	if (glState.texEnv[glState.activeUnit] == texEnv)
		return;
	glState.texEnv[glState.activeUnit] = texEnv;

	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texEnv);
}

/*
 =================
 GL_TexGen
 =================
*/
void GL_TexGen (unsigned texCoord, int texGen){

	switch (texCoord){
	case GL_S:
		if (glState.texGenS[glState.activeUnit] == texGen)
			return;
		glState.texGenS[glState.activeUnit] = texGen;

		break;
	case GL_T:
		if (glState.texGenT[glState.activeUnit] == texGen)
			return;
		glState.texGenT[glState.activeUnit] = texGen;

		break;
	case GL_R:
		if (glState.texGenR[glState.activeUnit] == texGen)
			return;
		glState.texGenR[glState.activeUnit] = texGen;

		break;
	case GL_Q:
		if (glState.texGenQ[glState.activeUnit] == texGen)
			return;
		glState.texGenQ[glState.activeUnit] = texGen;

		break;
	}

	qglTexGeni(texCoord, GL_TEXTURE_GEN_MODE, texGen);
}

/*
 =================
 GL_BindProgram
 =================
*/
void GL_BindProgram (program_t *program){

	if (glState.progNum == program->progNum)
		return;
	glState.progNum = program->progNum;

	qglBindProgramARB(program->uploadTarget, program->progNum);
}

/*
 =================
 GL_BindProgramTexture
 =================
*/
void GL_BindProgramTexture (unsigned unit, texture_t *texture){

	if (texture->frameCount != tr.frameCount){
		texture->frameCount = tr.frameCount;

		tr.pc.textures++;
		tr.pc.textureBytes += texture->uploadSize;
	}

	if (glState.texNum[unit] == texture->texNum)
		return;
	glState.texNum[unit] = texture->texNum;

	if (glState.activeUnit == unit){
		qglBindTexture(texture->uploadTarget, texture->texNum);
		return;
	}
	glState.activeUnit = unit;

	qglActiveTextureARB(GL_TEXTURE0_ARB + unit);
	qglClientActiveTextureARB(GL_TEXTURE0_ARB + unit);
	qglBindTexture(texture->uploadTarget, texture->texNum);
}

/*
 =================
 GL_LoadIdentity
 =================
*/
void GL_LoadIdentity (unsigned mode){

	switch (mode){
	case GL_PROJECTION:
		if (Matrix4_Compare(glState.projectionMatrix, matrixIdentity))
			return;
		Matrix4_Identity(glState.projectionMatrix);

		break;
	case GL_MODELVIEW:
		if (Matrix4_Compare(glState.modelviewMatrix, matrixIdentity))
			return;
		Matrix4_Identity(glState.modelviewMatrix);

		break;
	case GL_TEXTURE:
		if (Matrix4_Compare(glState.textureMatrix[glState.activeUnit], matrixIdentity))
			return;
		Matrix4_Identity(glState.textureMatrix[glState.activeUnit]);

		break;
	}

	qglMatrixMode(mode);
	qglLoadIdentity();
}

/*
 =================
 GL_LoadMatrix
 =================
*/
void GL_LoadMatrix (unsigned mode, const float *m){

	switch (mode){
	case GL_PROJECTION:
		if (Matrix4_Compare(glState.projectionMatrix, m))
			return;
		Matrix4_Copy(m, glState.projectionMatrix);

		break;
	case GL_MODELVIEW:
		if (Matrix4_Compare(glState.modelviewMatrix, m))
			return;
		Matrix4_Copy(m, glState.modelviewMatrix);

		break;
	case GL_TEXTURE:
		if (Matrix4_Compare(glState.textureMatrix[glState.activeUnit], m))
			return;
		Matrix4_Copy(m, glState.textureMatrix[glState.activeUnit]);

		break;
	}

	qglMatrixMode(mode);
	qglLoadMatrixf(m);
}

/*
 =================
 GL_Enable
 =================
*/
void GL_Enable (unsigned cap){

	switch (cap){
	case GL_CULL_FACE:
		if (glState.cullFace)
			return;
		glState.cullFace = true;

		break;
	case GL_POLYGON_OFFSET_FILL:
		if (glState.polygonOffsetFill)
			return;
		glState.polygonOffsetFill = true;

		break;
	case GL_ALPHA_TEST:
		if (glState.alphaTest)
			return;
		glState.alphaTest = true;

		break;
	case GL_BLEND:
		if (glState.blend)
			return;
		glState.blend = true;

		break;
	case GL_DEPTH_TEST:
		if (glState.depthTest)
			return;
		glState.depthTest = true;

		break;
	case GL_STENCIL_TEST:
		if (glState.stencilTest)
			return;
		glState.stencilTest = true;

		break;
	case GL_TEXTURE_GEN_S:
		if (glState.textureGenS[glState.activeUnit])
			return;
		glState.textureGenS[glState.activeUnit] = true;

		break;
	case GL_TEXTURE_GEN_T:
		if (glState.textureGenT[glState.activeUnit])
			return;
		glState.textureGenT[glState.activeUnit] = true;

		break;
	case GL_TEXTURE_GEN_R:
		if (glState.textureGenR[glState.activeUnit])
			return;
		glState.textureGenR[glState.activeUnit] = true;

		break;
	case GL_TEXTURE_GEN_Q:
		if (glState.textureGenQ[glState.activeUnit])
			return;
		glState.textureGenQ[glState.activeUnit] = true;

		break;
	case GL_VERTEX_PROGRAM_ARB:
		if (!glConfig.vertexProgram || glState.vertexProgram)
			return;
		glState.vertexProgram = true;

		break;
	case GL_FRAGMENT_PROGRAM_ARB:
		if (!glConfig.fragmentProgram || glState.fragmentProgram)
			return;
		glState.fragmentProgram = true;

		break;
	}

	qglEnable(cap);
}

/*
 =================
 GL_Disable
 =================
*/
void GL_Disable (unsigned cap){

	switch (cap){
	case GL_CULL_FACE:
		if (!glState.cullFace)
			return;
		glState.cullFace = false;

		break;
	case GL_POLYGON_OFFSET_FILL:
		if (!glState.polygonOffsetFill)
			return;
		glState.polygonOffsetFill = false;

		break;
	case GL_ALPHA_TEST:
		if (!glState.alphaTest)
			return;
		glState.alphaTest = false;

		break;
	case GL_BLEND:
		if (!glState.blend)
			return;
		glState.blend = false;

		break;
	case GL_DEPTH_TEST:
		if (!glState.depthTest)
			return;
		glState.depthTest = false;

		break;
	case GL_STENCIL_TEST:
		if (!glState.stencilTest)
			return;
		glState.stencilTest = false;

		break;
	case GL_TEXTURE_GEN_S:
		if (!glState.textureGenS[glState.activeUnit])
			return;
		glState.textureGenS[glState.activeUnit] = false;

		break;
	case GL_TEXTURE_GEN_T:
		if (!glState.textureGenT[glState.activeUnit])
			return;
		glState.textureGenT[glState.activeUnit] = false;

		break;
	case GL_TEXTURE_GEN_R:
		if (!glState.textureGenR[glState.activeUnit])
			return;
		glState.textureGenR[glState.activeUnit] = false;

		break;
	case GL_TEXTURE_GEN_Q:
		if (!glState.textureGenQ[glState.activeUnit])
			return;
		glState.textureGenQ[glState.activeUnit] = false;

		break;
	case GL_VERTEX_PROGRAM_ARB:
		if (!glConfig.vertexProgram || !glState.vertexProgram)
			return;
		glState.vertexProgram = false;

		break;
	case GL_FRAGMENT_PROGRAM_ARB:
		if (!glConfig.fragmentProgram || !glState.fragmentProgram)
			return;
		glState.fragmentProgram = false;

		break;
	}

	qglDisable(cap);
}

/*
 =================
 GL_CullFace
 =================
*/
void GL_CullFace (unsigned mode){

	if (glState.cullMode == mode)
		return;
	glState.cullMode = mode;

	qglCullFace(mode);
}

/*
 =================
 GL_PolygonOffset
 =================
*/
void GL_PolygonOffset (float factor, float units){

	if (glState.offsetFactor == factor && glState.offsetUnits == units)
		return;
	glState.offsetFactor = factor;
	glState.offsetUnits = units;

	qglPolygonOffset(factor, units);
}

/*
 =================
 GL_AlphaFunc
 =================
*/
void GL_AlphaFunc (unsigned func, float ref){

	if (glState.alphaFunc == func && glState.alphaRef == ref)
		return;
	glState.alphaFunc = func;
	glState.alphaRef = ref;

	qglAlphaFunc(func, ref);
}

/*
 =================
 GL_BlendFunc
 =================
*/
void GL_BlendFunc (unsigned src, unsigned dst){

	if (glState.blendSrc == src && glState.blendDst == dst)
		return;
	glState.blendSrc = src;
	glState.blendDst = dst;

	qglBlendFunc(src, dst);
}

/*
 =================
 GL_DepthFunc
 =================
*/
void GL_DepthFunc (unsigned func){

	if (glState.depthFunc == func)
		return;
	glState.depthFunc = func;

	qglDepthFunc(func);
}

/*
 =================
 GL_StencilFunc
 =================
*/
void GL_StencilFunc (unsigned func, int ref, unsigned mask){

	if (glState.stencilFunc == func && glState.stencilRef == ref && glState.stencilRefMask == mask)
		return;
	glState.stencilFunc = func;
	glState.stencilRef = ref;
	glState.stencilRefMask = mask;

	qglStencilFunc(func, ref, mask);
}

/*
 =================
 GL_StencilOp
 =================
*/
void GL_StencilOp (unsigned fail, unsigned zFail, unsigned zPass){

	if (glState.stencilFail == fail && glState.stencilZFail == zFail && glState.stencilZPass == zPass)
		return;
	glState.stencilFail = fail;
	glState.stencilZFail = zFail;
	glState.stencilZPass = zPass;

	qglStencilOp(fail, zFail, zPass);
}

/*
 =================
 GL_ColorMask
 =================
*/
void GL_ColorMask (qboolean red, qboolean green, qboolean blue, qboolean alpha){

	if (glState.colorMask[0] == red && glState.colorMask[1] == green && glState.colorMask[2] == blue && glState.colorMask[3] == alpha)
		return;
	glState.colorMask[0] = red;
	glState.colorMask[1] = green;
	glState.colorMask[2] = blue;
	glState.colorMask[3] = alpha;

	qglColorMask(red, green, blue, alpha);
}

/*
 =================
 GL_DepthMask
 =================
*/
void GL_DepthMask (qboolean mask){

	if (glState.depthMask == mask)
		return;
	glState.depthMask = mask;

	qglDepthMask(mask);
}

/*
 =================
 GL_StencilMask
 =================
*/
void GL_StencilMask (unsigned mask){

	if (glState.stencilMask == mask)
		return;
	glState.stencilMask = mask;

	qglStencilMask(mask);
}

/*
 =================
 GL_DepthRange
 =================
*/
void GL_DepthRange (float min, float max){

	if (glState.depthMin == min && glState.depthMax == max)
		return;
	glState.depthMin = min;
	glState.depthMax = max;

	qglDepthRange(min, max);
}

/*
 =================
 GL_FrontFace
 =================
*/
void GL_FrontFace (unsigned face){

	if (glState.frontFace == face)
		return;
	glState.frontFace = face;

	qglFrontFace(face);
}

/*
 =================
 GL_Scissor
 =================
*/
void GL_Scissor (int x, int y, int width, int height){

	if (glState.scissorX == x && glState.scissorY == y && glState.scissorWidth == width && glState.scissorHeight == height)
		return;
	glState.scissorX = x;
	glState.scissorY = y;
	glState.scissorWidth = width;
	glState.scissorHeight = height;

	qglScissor(x, y, width, height);
}

/*
 =================
 GL_DepthBounds
 =================
*/
void GL_DepthBounds (float min, float max){

	if (glState.depthBoundsMin == min && glState.depthBoundsMax == max)
		return;
	glState.depthBoundsMin = min;
	glState.depthBoundsMax = max;

	qglDepthBoundsEXT(min, max);
}

/*
 =================
 GL_SetDefaultState
 =================
*/
void GL_SetDefaultState (void){

	int		i;

	// Reset the state manager
	glState.activeUnit = 0;

	for (i = 0; i < MAX_TEXTURE_UNITS; i++){
		glState.activeTarget[i] = 0;

		glState.texNum[i] = 0;
		glState.texEnv[i] = GL_MODULATE;
		glState.texGenS[i] = GL_OBJECT_LINEAR;
		glState.texGenT[i] = GL_OBJECT_LINEAR;
		glState.texGenR[i] = GL_OBJECT_LINEAR;
		glState.texGenQ[i] = GL_OBJECT_LINEAR;
	}

	glState.progNum = 0;

	Matrix4_Identity(glState.projectionMatrix);
	Matrix4_Identity(glState.modelviewMatrix);

	for (i = 0; i < MAX_TEXTURE_UNITS; i++)
		Matrix4_Identity(glState.textureMatrix[i]);

	glState.cullFace = false;
	glState.polygonOffsetFill = false;
	glState.alphaTest = false;
	glState.blend = false;
	glState.depthTest = false;
	glState.stencilTest = false;

	for (i = 0; i < MAX_TEXTURE_UNITS; i++){
		glState.textureGenS[i] = false;
		glState.textureGenT[i] = false;
		glState.textureGenR[i] = false;
		glState.textureGenQ[i] = false;
	}

	glState.vertexProgram = false;
	glState.fragmentProgram = false;

	glState.cullMode = GL_FRONT;
	glState.offsetFactor = 0.0;
	glState.offsetUnits = 0.0;
	glState.alphaFunc = GL_GREATER;
	glState.alphaRef = 0.0;
	glState.blendSrc = GL_ONE;
	glState.blendDst = GL_ZERO;
	glState.depthFunc = GL_LEQUAL;
	glState.stencilFunc = GL_ALWAYS;
	glState.stencilRef = 128;
	glState.stencilRefMask = 255;
	glState.stencilFail = GL_KEEP;
	glState.stencilZFail = GL_KEEP;
	glState.stencilZPass = GL_KEEP;

	glState.colorMask[0] = GL_TRUE;
	glState.colorMask[1] = GL_TRUE;
	glState.colorMask[2] = GL_TRUE;
	glState.colorMask[3] = GL_TRUE;
	glState.depthMask = GL_TRUE;
	glState.stencilMask = 255;

	glState.depthMin = 0.0;
	glState.depthMax = 1.0;

	glState.frontFace = GL_CCW;

	glState.scissorX = 0;
	glState.scissorY = 0;
	glState.scissorWidth = glConfig.videoWidth;
	glState.scissorHeight = glConfig.videoHeight;

	glState.depthBoundsMin = 0.0;
	glState.depthBoundsMax = 1.0;

	// Set default state
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	for (i = MAX_TEXTURE_UNITS - 1; i >= 0; i--){
		if (i >= glConfig.maxTextureUnits)
			continue;

		qglActiveTextureARB(GL_TEXTURE0_ARB + i);
		qglClientActiveTextureARB(GL_TEXTURE0_ARB + i);

		qglMatrixMode(GL_TEXTURE);
		qglLoadIdentity();

		qglDisable(GL_TEXTURE_CUBE_MAP_ARB);
		qglDisable(GL_TEXTURE_2D);

		qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		qglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		qglTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

		qglDisable(GL_TEXTURE_GEN_S);
		qglDisable(GL_TEXTURE_GEN_T);
		qglDisable(GL_TEXTURE_GEN_R);
		qglDisable(GL_TEXTURE_GEN_Q);
	}

	if (glConfig.vertexProgram)
		qglDisable(GL_VERTEX_PROGRAM_ARB);
	if (glConfig.fragmentProgram)
		qglDisable(GL_FRAGMENT_PROGRAM_ARB);

	qglDisable(GL_CULL_FACE);
	qglCullFace(GL_FRONT);

	qglDisable(GL_POLYGON_OFFSET_FILL);
	qglPolygonOffset(0.0, 0.0);

	qglDisable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.0);

	qglDisable(GL_BLEND);
	qglBlendFunc(GL_ONE, GL_ZERO);

	qglDisable(GL_DEPTH_TEST);
	qglDepthFunc(GL_LEQUAL);

	qglDisable(GL_STENCIL_TEST);
	qglStencilFunc(GL_ALWAYS, 128, 255);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	qglDepthMask(GL_TRUE);
	qglStencilMask(255);

	qglDepthRange(0.0, 1.0);

	qglFrontFace(GL_CCW);

	qglClearColor(0.0, 0.0, 0.0, 1.0);
	qglClearDepth(1.0);
	qglClearStencil(128);

	qglEnable(GL_SCISSOR_TEST);
	qglScissor(0, 0, glConfig.videoWidth, glConfig.videoHeight);

	if (glConfig.depthBoundsTest){
		qglEnable(GL_DEPTH_BOUNDS_TEST_EXT);
		qglDepthBoundsEXT(0.0, 1.0);
	}

	qglShadeModel(GL_SMOOTH);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (glConfig.multisample && glConfig.samples)
		qglEnable(GL_MULTISAMPLE_ARB);
}

/*
 =================
 GL_CheckForErrors
 =================
*/
void GL_CheckForErrors (void){

	int		err;
	char	*str;

	if ((err = qglGetError()) == GL_NO_ERROR)
		return;

	switch (err){
	case GL_INVALID_ENUM:
		str = "GL_INVALID_ENUM";
		break;
	case GL_INVALID_VALUE:
		str = "GL_INVALID_VALUE";
		break;
	case GL_INVALID_OPERATION:
		str = "GL_INVALID_OPERATION";
		break;
	case GL_STACK_OVERFLOW:
		str = "GL_STACK_OVERFLOW";
		break;
	case GL_STACK_UNDERFLOW:
		str = "GL_STACK_UNDERFLOW";
		break;
	case GL_OUT_OF_MEMORY:
		str = "GL_OUT_OF_MEMORY";
		break;
	default:
		str = "UNKNOWN ERROR";
		break;
	}

	Com_Error(ERR_DROP, "GL_CheckForErrors: %s", str);
}
