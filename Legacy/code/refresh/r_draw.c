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
 =================
 R_GetPicSize

 This is needed by some client drawing functions
 =================
*/
void R_GetPicSize (const char *pic, float *w, float *h){

	shader_t	*shader;

	shader = R_RegisterShaderNoMip(pic);

	*w = (float)shader->stages[0]->bundles[0]->textures[0]->sourceWidth;
	*h = (float)shader->stages[0]->bundles[0]->textures[0]->sourceHeight;
}

/*
 =================
 R_DrawStretchPic

 Batches the pic in the vertex arrays.
 Call RB_RenderMesh to flush.
 =================
*/
void R_DrawStretchPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, const color_t modulate, shader_t *shader){

	if (!shader)
		Com_Error(ERR_DROP, "R_DrawStretchPic: NULL shader");

	RB_DrawStretchPic(x, y, w, h, sl, tl, sh, th, modulate, shader);
}

/*
 =================
 R_DrawRotatedPic

 Batches the pic in the vertex arrays.
 Call RB_RenderMesh to flush.
 =================
*/
void R_DrawRotatedPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, float angle, const color_t modulate, shader_t *shader){

	if (!shader)
		Com_Error(ERR_DROP, "R_DrawRotatedPic: NULL shader");

	RB_DrawRotatedPic(x, y, w, h, sl, tl, sh, th, angle, modulate, shader);
}

/*
 =================
 R_DrawOffsetPic

 Batches the pic in the vertex arrays.
 Call RB_RenderMesh to flush.
 =================
*/
void R_DrawOffsetPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, float offsetX, float offsetY, const color_t modulate, shader_t *shader){

	if (!shader)
		Com_Error(ERR_DROP, "R_DrawOffsetPic: NULL shader");

	RB_DrawOffsetPic(x, y, w, h, sl, tl, sh, th, offsetX, offsetY, modulate, shader);
}

/*
 =================
 R_DrawStretchRaw

 Cinematic streaming
 =================
*/
void R_DrawStretchRaw (float x, float y, float w, float h, const byte *raw, int rawWidth, int rawHeight, qboolean noDraw){

	int		width = 1, height = 1;

	// Make sure everything is flushed if needed
	if (!noDraw)
		RB_RenderMesh();

	// Check the dimensions
	while (width < rawWidth)
		width <<= 1;
	while (height < rawHeight)
		height <<= 1;

	if (rawWidth != width || rawHeight != height)
		Com_Error(ERR_DROP, "R_DrawStretchRaw: size is not a power of two (%i x %i)", rawWidth, rawHeight);

	if (rawWidth > glConfig.maxTextureSize || rawHeight > glConfig.maxTextureSize)
		Com_Error(ERR_DROP, "R_DrawStretchRaw: size exceeds hardware limits (%i > %i or %i > %i)", rawWidth, glConfig.maxTextureSize, rawHeight, glConfig.maxTextureSize);

	// Update the texture as appropriate
	GL_BindTexture(r_rawTexture);

	if (rawWidth == r_rawTexture->uploadWidth && rawHeight == r_rawTexture->uploadHeight)
		qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rawWidth, rawHeight, GL_RGBA, GL_UNSIGNED_BYTE, raw);
	else {
		r_rawTexture->uploadWidth = rawWidth;
		r_rawTexture->uploadHeight = rawHeight;

		qglTexImage2D(GL_TEXTURE_2D, 0, r_rawTexture->uploadFormat, rawWidth, rawHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw);
	}

	if (noDraw)
		return;

	// Set the state
	GL_TexEnv(GL_REPLACE);

	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_VERTEX_PROGRAM_ARB);
	GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_DEPTH_TEST);

	GL_DepthMask(GL_FALSE);

	qglEnable(GL_TEXTURE_2D);

	// Draw it
	qglColor4ub(255, 255, 255, 255);

	qglBegin(GL_QUADS);
	qglTexCoord2f(0, 0);
	qglVertex2f(x, y);
	qglTexCoord2f(1, 0);
	qglVertex2f(x+w, y);
	qglTexCoord2f(1, 1);
	qglVertex2f(x+w, y+h);
	qglTexCoord2f(0, 1);
	qglVertex2f(x, y+h);
	qglEnd();

	qglDisable(GL_TEXTURE_2D);
}

/*
 =================
 R_CopyScreenRect

 Screen blur
 =================
*/
void R_CopyScreenRect (float x, float y, float w, float h){

	if (!glConfig.textureRectangle)
		return;

	if (w > glConfig.maxRectangleTextureSize || h > glConfig.maxRectangleTextureSize)
		return;

	qglBindTexture(GL_TEXTURE_RECTANGLE_NV, glState.screenRectTexture);

	qglCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, x, glConfig.videoHeight - h - y, w, h, 0);

	qglTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

/*
 =================
 R_DrawScreenRect

 Screen blur
 =================
*/
void R_DrawScreenRect (float x, float y, float w, float h, const color_t modulate){

	if (!glConfig.textureRectangle)
		return;

	if (w > glConfig.maxRectangleTextureSize || h > glConfig.maxRectangleTextureSize)
		return;

	// Set the state
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

	qglEnable(GL_TEXTURE_RECTANGLE_NV);

	// Draw it
	qglBindTexture(GL_TEXTURE_RECTANGLE_NV, glState.screenRectTexture);

	qglColor4ubv(modulate);

	qglBegin(GL_QUADS);
	qglTexCoord2f(0, h);
	qglVertex2f(x, y);
	qglTexCoord2f(w, h);
	qglVertex2f(x+w, y);
	qglTexCoord2f(w, 0);
	qglVertex2f(x+w, y+h);
	qglTexCoord2f(0, 0);
	qglVertex2f(x, y+h);
	qglEnd();

	qglDisable(GL_TEXTURE_RECTANGLE_NV);
}
