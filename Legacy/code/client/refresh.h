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


#ifndef __REFRESH_H__
#define __REFRESH_H__


#define	MAX_ENTITIES		1024
#define	MAX_DLIGHTS			32
#define MAX_PARTICLES		8192
#define MAX_POLYS			4096
#define MAX_POLY_VERTS		16384

typedef enum {
	ET_MODEL,
	ET_SPRITE,
	ET_BEAM
} entityType_t;

typedef struct {
	entityType_t	entityType;
	int				renderFX;

	struct model_s	*model;			// Opaque type outside refresh

	// Most recent data
	vec3_t			axis[3];		// Rotation vectors

	vec3_t			origin;			// Also used as beam's "from"
	int				frame;			// Also used as beam's diameter

	// Previous data for lerping
	vec3_t			oldOrigin;		// Also used as beam's "to"
	int				oldFrame;		// Also used as beam's segment length

	float			backLerp;		// 0.0 = current, 1.0 = old

	// Ammo display data
	int				ammoValue;

	// Sprite data
	float			radius;
	float			rotation;

	// Skin information
	int				skinNum;		// Skin index
	struct shader_s	*customShader;	// Custom player skin

	// Shader information
	color_t			shaderRGBA;		// Used by rgbGen/alphaGen entity shaders
	float			shaderTime;		// Subtracted from refDef time to control effect start times
} entity_t;

typedef struct {
	vec3_t			origin;
	vec3_t			color;
	float			intensity;
} dlight_t;

typedef struct {
	struct shader_s	*shader;
	vec3_t			origin;
	vec3_t			oldOrigin;
	float			radius;
	float			length;
	float			rotation;
	color_t			modulate;
} particle_t;

typedef struct {
	vec3_t			xyz;
	vec2_t			st;
	color_t			modulate;
} polyVert_t;

typedef struct {
	struct shader_s	*shader;
	int				numVerts;
	polyVert_t		*verts;
} poly_t;

typedef struct {
	vec3_t			rgb;			// 0.0 - 2.0
	float			white;			// Highest of RGB
} lightStyle_t;

typedef struct {
	vec3_t			origin;
	vec3_t			axis[3];
} tag_t;

typedef struct {
	int				firstVert;
	int				numVerts;
} markFragment_t;

typedef struct {
	int				x;
	int				y;
	int				width;
	int				height;

	float			fovX;
	float			fovY;

	vec3_t			viewOrigin;
	vec3_t			viewAxis[3];	// Transformation matrix

	float			time;			// Time for shader effects
	int				rdFlags;		// RDF_NOWORLDMODEL, etc...

	byte			*areaBits;		// If not NULL, only areas with set bits will be drawn
} refDef_t;

// =====================================================================

typedef struct {
	const char			*vendorString;
	const char			*rendererString;
	const char			*versionString;
	const char			*extensionsString;

	int					colorBits;
	int					depthBits;
	int					stencilBits;

	qboolean			miniDriver;
	qboolean			stereoEnabled;

	qboolean			deviceSupportsGamma;
	int					displayDepth;
	int					displayFrequency;
	qboolean			isFullscreen;
	int					videoWidth;
	int					videoHeight;

	qboolean			multitexture;
	qboolean			textureEnvAdd;
	qboolean			textureEnvCombine;
	qboolean			textureEnvDot3;
	qboolean			textureCubeMap;
	qboolean			textureCompression;
	qboolean			vertexBufferObject;
	qboolean			vertexProgram;
	qboolean			fragmentProgram;
	qboolean			drawRangeElements;
	qboolean			compiledVertexArray;
	qboolean			textureEdgeClamp;
	qboolean			textureFilterAnisotropic;
	qboolean			textureRectangle;
	qboolean			stencilTwoSide;
	qboolean			separateStencil;
	qboolean			generateMipmap;
	qboolean			swapControl;

	int					maxTextureSize;
	int					maxTextureUnits;
	int					maxCubeMapTextureSize;
	int					maxTextureCoords;
	int					maxTextureImageUnits;
	float				maxTextureMaxAnisotropy;
	int					maxRectangleTextureSize;
} glConfig_t;

// =====================================================================

void			R_LoadWorldMap (const char *mapName, const char *skyName, float skyRotate, const vec3_t skyAxis);

struct model_s	*R_RegisterModel (const char *name);
struct shader_s	*R_RegisterShader (const char *name);
struct shader_s	*R_RegisterShaderSkin (const char *name);
struct shader_s	*R_RegisterShaderNoMip (const char *name);

void			R_GetPicSize (const char *pic, float *w, float *h);
void			R_DrawStretchPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, const color_t modulate, struct shader_s *shader);
void			R_DrawRotatedPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, float angle, const color_t modulate, struct shader_s *shader);
void			R_DrawOffsetPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, float offsetX, float offsetY, const color_t modulate, struct shader_s *shader);
void			R_DrawStretchRaw (float x, float y, float w, float h, const byte *raw, int rawWidth, int rawHeight, qboolean noDraw);
void			R_CopyScreenRect (float x, float y, float w, float h);
void			R_DrawScreenRect (float x, float y, float w, float h, const color_t modulate);

void			R_ClearScene (void);
void			R_AddEntityToScene (const entity_t *entity);
void			R_AddLightToScene (const vec3_t origin, float intensity, float r, float g, float b);
void			R_AddParticleToScene (struct shader_s *shader, const vec3_t origin, const vec3_t oldOrigin, float radius, float length, float rotation, const color_t modulate);
void			R_AddPolyToScene (struct shader_s *shader, int numVerts, const polyVert_t *verts);
void			R_RenderScene (const refDef_t *rd);

void			R_SetLightStyle (int style, float r, float g, float b);

void			R_LightForPoint (const vec3_t point, vec3_t ambientLight);

void			R_ModelBounds (struct model_s *model, vec3_t mins, vec3_t maxs);

qboolean		R_LerpTag (tag_t *tag, struct model_s *model, int curFrame, int oldFrame, float backLerp, const char *tagName);

int				R_MarkFragments (const vec3_t origin, const vec3_t axis[3], float radius, int maxVerts, vec3_t *verts, int maxFragments, markFragment_t *fragments);

void			R_GetGLConfig (glConfig_t *config);

void			R_BeginFrame (int realTime, float stereoSeparation);
void			R_EndFrame (void);

void			R_Init (qboolean all);
void			R_Shutdown (qboolean all);


#endif // __REFRESH_H__
