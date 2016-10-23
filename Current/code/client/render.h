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

#ifndef __RENDER_H__
#define __RENDER_H__

#include "../qcommon/qcommon.h"

#define MAX_RENDER_ENTITIES			2048
#define MAX_RENDER_LIGHTS			64
#define MAX_RENDER_POLYS			8192

// Material parms
#define MAX_ENTITY_MATERIAL_PARMS	8
#define MAX_GLOBAL_MATERIAL_PARMS	8

#define MATERIALPARM_RED			0		// Used by "colored" entity materials
#define MATERIALPARM_GREEN			1		// Used by "colored" entity materials
#define MATERIALPARM_BLUE			2		// Used by "colored" entity materials
#define MATERIALPARM_ALPHA			3		// Used by "colored" entity materials
#define MATERIALPARM_TIME_OFFSET	4		// Offset relative to renderView time to control effect start times
#define MATERIALPARM_DIVERSITY		5		// Random value for some effects
#define MATERIALPARM_GENERAL		6		// Used for various things throughout the code
#define MATERIALPARM_MODE			7		// For selecting which material stages to enable

typedef enum {
	RE_MODEL,
	RE_SPRITE,
	RE_BEAM
} reType_t;

typedef struct renderEntity_s {
	reType_t			reType;
	int					renderFX;			// FX flags

	int					index;				// Set by the renderer

	struct model_s		*model;				// Opaque type outside renderer

	// Transformation matrix
	vec3_t				origin;
	vec3_t				axis[3];

	// Model animation data
	int					frame;
	int					oldFrame;

	float				backLerp;			// 0.0 = current, 1.0 = old

	// Sprite specific
	float				spriteRadius;
	float				spriteRotation;
	qboolean			spriteOriented;		// Use axis for orientation

	// Beam specific
	vec3_t				beamEnd;
	float				beamWidth;
	float				beamLength;			// Segment length

	// Misc stuff
	struct renderView_s	*remoteView;		// For remote cameras

	int					skinIndex;			// Model texturing

	float				depthHack;			// Depth range hack to avoid poking into geometry

	// Material information
	struct material_s	*customMaterial;	// Can be NULL for models

	float				materialParms[MAX_ENTITY_MATERIAL_PARMS];
} renderEntity_t;

typedef struct renderLight_s {
	// Transformation matrix
	vec3_t				origin;
	vec3_t				center;				// Relative to origin
	vec3_t				axis[3];
	vec3_t				radius;

	// Light settings
	int					style;
	int					detailLevel;
	qboolean			parallel;			// Light center gives the direction to the light at infinity
	qboolean			noShadows;

	// Material information
	struct material_s	*customMaterial;	// If NULL, _defaultLight will be used

	float				materialParms[MAX_ENTITY_MATERIAL_PARMS];
} renderLight_t;

typedef struct renderPolyVertex_s {
	vec3_t				xyz;
	vec3_t				normal;
	vec2_t				st;
	color_t				color;
} renderPolyVertex_t;

typedef struct renderPoly_s {
	struct material_s	*material;

	// Geometry in triangle-fan order
	int					numVertices;
	renderPolyVertex_t	*vertices;
} renderPoly_t;

typedef struct renderView_s {
	// Viewport definition
	int					x;
	int					y;
	int					width;
	int					height;

	float				fovX;
	float				fovY;

	// Transformation matrix
	vec3_t				viewOrigin;
	vec3_t				viewAxis[3];

	// Current render time
	float				time;

	// Material parms
	float				materialParms[MAX_GLOBAL_MATERIAL_PARMS];

	// Area portal bits
	byte				*areaBits;			// If not NULL, only areas with set bits will be drawn
} renderView_t;

typedef struct {
	vec3_t				origin;
	vec3_t				axis[3];
} tag_t;

// =====================================================================

typedef struct {
	const char			*vendorString;
	const char			*rendererString;
	const char			*versionString;
	const char			*extensionsString;

#ifdef WIN32
	const char			*wglExtensionsString;
#endif

	int					colorBits;
	int					alphaBits;
	int					depthBits;
	int					stencilBits;
	int					samples;

	qboolean			deviceSupportsGamma;
	qboolean			isFullscreen;
	int					displayFrequency;
	int					videoWidth;
	int					videoHeight;

	qboolean			multitexture;
	qboolean			textureEnvAdd;
	qboolean			textureEnvCombine;
	qboolean			textureEnvDot3;
	qboolean			textureCubeMap;
	qboolean			textureNonPowerOfTwo;
	qboolean			textureCompression;
	qboolean			textureCompressionS3;
	qboolean			textureBorderClamp;
	qboolean			multisample;
	qboolean			vertexBufferObject;
	qboolean			vertexProgram;
	qboolean			fragmentProgram;
	qboolean			drawRangeElements;
	qboolean			textureFilterAnisotropic;
	qboolean			textureLodBias;
	qboolean			textureEdgeClamp;
	qboolean			bgra;
	qboolean			stencilWrap;
	qboolean			stencilTwoSide;
	qboolean			depthBoundsTest;
	qboolean			swapControl;
	qboolean			nvRegisterCombiners;
	qboolean			atiFragmentShader;
	qboolean			atiSeparateStencil;

	int					maxTextureSize;
	int					maxTextureUnits;
	int					maxCubeMapTextureSize;
	int					maxTextureCoords;
	int					maxTextureImageUnits;
	float				maxTextureMaxAnisotropy;
	float				maxTextureLodBias;

	qboolean			allowNV10Path;
	qboolean			allowNV20Path;
	qboolean			allowNV30Path;
	qboolean			allowR200Path;
	qboolean			allowARB2Path;
} glConfig_t;

// =====================================================================

void				R_LoadWorldMap (const char *mapName, const char *skyName, float skyRotate, const vec3_t skyAxis);

struct model_s		*R_RegisterModel (const char *name);
struct material_s	*R_RegisterMaterial (const char *name, qboolean lightingDefault);
struct material_s	*R_RegisterMaterialLight (const char *name);
struct material_s	*R_RegisterMaterialNoMip (const char *name);

void				R_ClearScene (void);
void				R_AddEntityToScene (const renderEntity_t *renderEntity);
void				R_AddLightToScene (const renderLight_t *renderLight);
void				R_AddPolyToScene (const renderPoly_t *renderPoly);
void				R_RenderScene (const renderView_t *renderView, qboolean primaryView);

void				R_SetLightStyle (int style, float r, float g, float b);

void				R_GetPicSize (struct material_s *material, float *w, float *h);
void				R_DrawStretchPic (float x, float y, float w, float h, float s1, float t1, float s2, float t2, const color_t color, struct material_s *material);
void				R_DrawShearedPic (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float shearX, float shearY, const color_t color, struct material_s *material);

void				R_CropRender (int width, int height, qboolean makePowerOfTwo);
void				R_UnCropRender (void);

qboolean			R_CaptureRenderToTexture (const char *name);
qboolean			R_UpdateTexture (const char *name, const byte *image, int width, int height);

void				R_ProjectDecal (const vec3_t origin, const vec3_t direction, float orientation, float radius, int time, struct material_s *material);

int					R_ModelFrames (struct model_s *model);
void				R_ModelBounds (struct model_s *model, int curFrameNum, int oldFrameNum, vec3_t mins, vec3_t maxs);
float				R_ModelRadius (struct model_s *model, int curFrameNum, int oldFrameNum);

qboolean			R_LerpTag (struct model_s *model, int curFrameNum, int oldFrameNum, float backLerp, const char *tagName, tag_t *tag);

qboolean			R_EditLight (void);

void				R_GetGLConfig (glConfig_t *config);

void				R_BeginFrame (void);
void				R_EndFrame (void);

void				R_Init (qboolean all);
void				R_Shutdown (qboolean all);


#endif	// __RENDER_H__
