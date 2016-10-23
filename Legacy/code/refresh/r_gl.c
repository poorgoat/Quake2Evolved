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


glState_t	glState;


/*
 =================
 GL_SelectTexture
 =================
*/
void GL_SelectTexture (unsigned tmu){

	if (!glConfig.multitexture)
		return;

	if (glState.activeTMU == tmu)
		return;
	glState.activeTMU = tmu;

	tmu += GL_TEXTURE0_ARB;

	qglActiveTextureARB(tmu);
	qglClientActiveTextureARB(tmu);
}

/*
 =================
 GL_BindTexture
 =================
*/
void GL_BindTexture (texture_t *texture){

	// Performance evaluation option
	if (r_noBind->integer)
		texture = r_defaultTexture;

	if (glState.texNum[glState.activeTMU] == texture->texNum)
		return;
	glState.texNum[glState.activeTMU] = texture->texNum;

	qglBindTexture(texture->uploadTarget, texture->texNum);
}

/*
 =================
 GL_TexEnv
 =================
*/
void GL_TexEnv (GLint texEnv){

	if (glState.texEnv[glState.activeTMU] == texEnv)
		return;
	glState.texEnv[glState.activeTMU] = texEnv;

	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texEnv);
}

/*
 =================
 GL_Enable
 =================
*/
void GL_Enable (GLenum cap){

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
	}

	qglEnable(cap);
}

/*
 =================
 GL_Disable
 =================
*/
void GL_Disable (GLenum cap){

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
	}

	qglDisable(cap);
}

/*
 =================
 GL_CullFace
 =================
*/
void GL_CullFace (GLenum mode){

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
void GL_PolygonOffset (GLfloat factor, GLfloat units){

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
void GL_AlphaFunc (GLenum func, GLclampf ref){

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
void GL_BlendFunc (GLenum src, GLenum dst){

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
void GL_DepthFunc (GLenum func){

	if (glState.depthFunc == func)
		return;
	glState.depthFunc = func;

	qglDepthFunc(func);
}

/*
 =================
 GL_DepthMask
 =================
*/
void GL_DepthMask (GLboolean mask){

	if (glState.depthMask == mask)
		return;
	glState.depthMask = mask;

	qglDepthMask(mask);
}

/*
 =================
 GL_SetDefaultState
 =================
*/
void GL_SetDefaultState (void){

	int		i;

	// Reset the state manager
	glState.gl2D = false;
	glState.stereoSeparation = 0;

	glState.activeTMU = 0;

	for (i = 0; i < MAX_TEXTURE_UNITS; i++){
		glState.texNum[i] = 0;
		glState.texEnv[i] = GL_MODULATE;
	}

	glState.cullFace = true;
	glState.polygonOffsetFill = false;
	glState.vertexProgram = false;
	glState.fragmentProgram = false;
	glState.alphaTest = false;
	glState.blend = false;
	glState.depthTest = true;

	glState.cullMode = GL_FRONT;
	glState.offsetFactor = -1;
	glState.offsetUnits = -2;
	glState.alphaFunc = GL_GREATER;
	glState.alphaRef = 0.0;
	glState.blendSrc = GL_SRC_ALPHA;
	glState.blendDst = GL_ONE_MINUS_SRC_ALPHA;
	glState.depthFunc = GL_LEQUAL;
	glState.depthMask = GL_TRUE;

	// Set default state
	qglEnable(GL_CULL_FACE);
	qglDisable(GL_POLYGON_OFFSET_FILL);
	qglDisable(GL_ALPHA_TEST);
	qglDisable(GL_BLEND);
	qglEnable(GL_DEPTH_TEST);

	if (glConfig.vertexProgram)
		qglDisable(GL_VERTEX_PROGRAM_ARB);
	if (glConfig.fragmentProgram)
		qglDisable(GL_FRAGMENT_PROGRAM_ARB);

	qglCullFace(GL_FRONT);
	qglPolygonOffset(-1, -2);
	qglAlphaFunc(GL_GREATER, 0.0);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthFunc(GL_LEQUAL);
	qglDepthMask(GL_TRUE);

	qglClearColor(1.0, 0.0, 0.5, 0.5);
	qglClearDepth(1.0);
	qglClearStencil(128);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	qglShadeModel(GL_SMOOTH);

	qglEnable(GL_SCISSOR_TEST);
	qglDisable(GL_STENCIL_TEST);

	if (glConfig.multitexture){
		for (i = MAX_TEXTURE_UNITS - 1; i > 0; i--){
			if (i >= glConfig.maxTextureUnits)
				continue;

			qglActiveTextureARB(GL_TEXTURE0_ARB + i);

			qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			if (glConfig.textureCubeMap)
				qglDisable(GL_TEXTURE_CUBE_MAP_ARB);

			qglDisable(GL_TEXTURE_2D);
		}

		qglActiveTextureARB(GL_TEXTURE0_ARB);
	}

	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (glConfig.textureCubeMap)
		qglDisable(GL_TEXTURE_CUBE_MAP_ARB);

	qglDisable(GL_TEXTURE_2D);
}

/*
 =================
 GL_Setup3D
 =================
*/
void GL_Setup3D (void){

	int		bits;

	if (r_finish->integer)
		qglFinish();

	// Set up viewport
	qglViewport(r_refDef.x, glConfig.videoHeight - r_refDef.height - r_refDef.y, r_refDef.width, r_refDef.height);
	qglScissor(r_refDef.x, glConfig.videoHeight - r_refDef.height - r_refDef.y, r_refDef.width, r_refDef.height);

	// Set up projection
	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf(r_projectionMatrix);
	qglMatrixMode(GL_MODELVIEW);

	// Set state
	glState.gl2D = false;

	GL_TexEnv(GL_MODULATE);

	GL_Enable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_VERTEX_PROGRAM_ARB);
	GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_BLEND);
	GL_Enable(GL_DEPTH_TEST);

	GL_CullFace(GL_FRONT);
	GL_DepthFunc(GL_LEQUAL);
	GL_DepthMask(GL_TRUE);

	// Clear depth buffer, and optionally stencil buffer
	bits = GL_DEPTH_BUFFER_BIT;

	if (glConfig.stencilBits && r_shadows->integer){
		qglClearStencil(128);

		bits |= GL_STENCIL_BUFFER_BIT;
	}

	qglClear(bits);
}

/*
 =================
 GL_Setup2D
 =================
*/
void GL_Setup2D (void){

	if (r_finish->integer)
		qglFinish();

	// Set 2D virtual screen size
	qglViewport(0, 0, glConfig.videoWidth, glConfig.videoHeight);
	qglScissor(0, 0, glConfig.videoWidth, glConfig.videoHeight);

	// Set up projection
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, glConfig.videoWidth, glConfig.videoHeight, 0, -1, 1);

	// Set up modelview
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity();

	// Set state
	glState.gl2D = true;

	GL_TexEnv(GL_MODULATE);

	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_VERTEX_PROGRAM_ARB);
	GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
	GL_Disable(GL_ALPHA_TEST);
	GL_Enable(GL_BLEND);
	GL_Disable(GL_DEPTH_TEST);

	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_DepthMask(GL_FALSE);
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
