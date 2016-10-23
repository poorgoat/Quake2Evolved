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


#ifndef __R_LOCAL_H__
#define __R_LOCAL_H__


#ifdef _WIN32
#include "../win32/winquake.h"
#endif

#include <GL/gl.h>

#include "../qcommon/qcommon.h"
#include "../client/refresh.h"
#include "../client/cinematic.h"

#include "jpeglib.h"
#include "jerror.h"

#include "glext.h"
#include "qgl.h"

#ifdef _WIN32
#include "../win32/wglext.h"
#include "../win32/glw_win.h"
#endif


// This is defined in q_shared.h
#undef MAX_MODELS


// Limits
#define MAX_TEXTURES		4096
#define MAX_PROGRAMS		512
#define MAX_SHADERS		1024
#define MAX_MODELS		512
#define MAX_LIGHTMAPS		128
#define MAX_TEXTURE_UNITS	8
#define MAX_VERTEX_BUFFERS	2048

/*
 =======================================================================

 TEXTURE MANAGER

 =======================================================================
*/

// Texture flags
#define TF_MIPMAPS			0x0001
#define TF_PICMIP			0x0002
#define TF_COMPRESS			0x0004
#define TF_CLAMP			0x0008
#define TF_CUBEMAP			0x0010
#define TF_NORMALMAP		0x0020
#define TF_HEIGHTMAP		0x0040

typedef struct texture_s {
	char				name[MAX_QPATH];
	char				realName[MAX_QPATH];

	unsigned			flags;
	float				bumpScale;

	int					sourceWidth;
	int					sourceHeight;
	int					uploadWidth;
	int					uploadHeight;
	unsigned			uploadFormat;
	unsigned			uploadTarget;
	unsigned			texNum;

	struct texture_s	*nextHash;
} texture_t;

extern const char	*r_skyBoxSuffix[6];
extern vec3_t		r_skyBoxAngles[6];
extern const char	*r_cubeMapSuffix[6];
extern vec3_t		r_cubeMapAngles[6];

extern texture_t	*r_defaultTexture;
extern texture_t	*r_whiteTexture;
extern texture_t	*r_blackTexture;
extern texture_t	*r_rawTexture;
extern texture_t	*r_dlightTexture;
extern texture_t	*r_lightmapTextures[MAX_LIGHTMAPS];
extern texture_t	*r_normalizeTexture;

void		R_TextureFilter (void);
void		R_TextureList_f (void);
void		R_ScreenShot_f (void);
void		R_EnvShot_f (void);
texture_t	*R_FindTexture (const char *name, unsigned flags, float bumpScale);
texture_t	*R_FindCubeMapTexture (const char *name, unsigned flags, float bumpScale);
void		R_InitTextures (void);
void		R_ShutdownTextures (void);

/*
 =======================================================================

 PROGRAM MANAGER

 =======================================================================
*/

typedef struct program_s {
	char				name[MAX_QPATH];

	unsigned			uploadTarget;
	unsigned			progNum;

	struct program_s	*nextHash;
} program_t;

extern program_t	*r_defaultVertexProgram;
extern program_t	*r_defaultFragmentProgram;

void		R_ProgramList_f (void);
program_t	*R_FindProgram (const char *name, unsigned target);
void		R_InitPrograms (void);
void		R_ShutdownPrograms (void);

/*
 =======================================================================

 SHADERS

 =======================================================================
*/

#define SHADER_MAX_EXPRESSIONS			16
#define SHADER_MAX_STAGES				8
#define SHADER_MAX_DEFORMVERTEXES		8
#define SHADER_MAX_TEXTURES				16
#define SHADER_MAX_TCMOD				8

// Shader types used for shader loading
typedef enum {
	SHADER_SKY,
	SHADER_BSP,
	SHADER_SKIN,
	SHADER_NOMIP,
	SHADER_GENERIC
} shaderType_t;

// surfaceParm flags used for shader loading
#define SURFACEPARM_LIGHTMAP			1
#define SURFACEPARM_WARP				2
#define SURFACEPARM_TRANS33				4
#define SURFACEPARM_TRANS66				8
#define SURFACEPARM_FLOWING				16

// Shader flags
#define SHADER_EXTERNAL					0x00000001
#define SHADER_DEFAULTED				0x00000002
#define SHADER_HASLIGHTMAP				0x00000004
#define SHADER_SURFACEPARM				0x00000008
#define SHADER_NOMIPMAPS				0x00000010
#define SHADER_NOPICMIP					0x00000020
#define SHADER_NOCOMPRESS				0x00000040
#define SHADER_NOSHADOWS				0x00000080
#define SHADER_NOFRAGMENTS				0x00000100
#define SHADER_ENTITYMERGABLE			0x00000200
#define SHADER_POLYGONOFFSET			0x00000400
#define SHADER_CULL						0x00000800
#define SHADER_SORT						0x00001000
#define SHADER_AMMODISPLAY				0x00002000
#define SHADER_TESSSIZE					0x00004000
#define SHADER_SKYPARMS					0x00008000
#define SHADER_DEFORMVERTEXES			0x00010000

// Shader stage flags
#define SHADERSTAGE_NEXTBUNDLE			0x00000001
#define SHADERSTAGE_VERTEXPROGRAM		0x00000002
#define SHADERSTAGE_FRAGMENTPROGRAM		0x00000004
#define SHADERSTAGE_ALPHAFUNC			0x00000008
#define SHADERSTAGE_BLENDFUNC			0x00000010
#define SHADERSTAGE_DEPTHFUNC			0x00000020
#define SHADERSTAGE_DEPTHWRITE			0x00000040
#define SHADERSTAGE_DETAIL				0x00000080
#define SHADERSTAGE_RGBGEN				0x00000100
#define SHADERSTAGE_ALPHAGEN			0x00000200

// Stage bundle flags
#define STAGEBUNDLE_NOMIPMAPS			0x00000001
#define STAGEBUNDLE_NOPICMIP			0x00000002
#define STAGEBUNDLE_NOCOMPRESS			0x00000004
#define STAGEBUNDLE_CLAMPTEXCOORDS		0x00000008
#define STAGEBUNDLE_ANIMFREQUENCY		0x00000010
#define STAGEBUNDLE_MAP					0x00000020
#define STAGEBUNDLE_BUMPMAP				0x00000040
#define STAGEBUNDLE_CUBEMAP				0x00000080
#define STAGEBUNDLE_VIDEOMAP			0x00000100
#define STAGEBUNDLE_TEXENVCOMBINE		0x00000200
#define STAGEBUNDLE_TCGEN				0x00000400
#define STAGEBUNDLE_TCMOD				0x00000800

typedef enum {
	WAVEFORM_SIN,
	WAVEFORM_TRIANGLE,
	WAVEFORM_SQUARE,
	WAVEFORM_SAWTOOTH,
	WAVEFORM_INVERSESAWTOOTH,
	WAVEFORM_NOISE
} waveForm_t;

typedef enum {
	SORT_SKY				= 1,
	SORT_OPAQUE				= 2,
	SORT_DECAL				= 3,
	SORT_SEETHROUGH			= 4,
	SORT_BANNER				= 5,
	SORT_UNDERWATER			= 6,
	SORT_WATER				= 7,
	SORT_INNERBLEND			= 8,
	SORT_BLEND				= 9,
	SORT_BLEND2				= 10,
	SORT_BLEND3				= 11,
	SORT_BLEND4				= 12,
	SORT_OUTERBLEND			= 13,
	SORT_ADDITIVE			= 14,
	SORT_NEAREST			= 15
} sort_t;

typedef enum {
	AMMODISPLAY_DIGIT1,
	AMMODISPLAY_DIGIT2,
	AMMODISPLAY_DIGIT3,
	AMMODISPLAY_WARNING
} ammoDisplayType_t;

typedef enum {
	DEFORMVERTEXES_WAVE,
	DEFORMVERTEXES_MOVE,
	DEFORMVERTEXES_NORMAL,
	DEFORMVERTEXES_AUTOSPRITE,
	DEFORMVERTEXES_AUTOSPRITE2
} deformVertexesType_t;

typedef enum {
	TCGEN_BASE,
	TCGEN_LIGHTMAP,
	TCGEN_ENVIRONMENT,
	TCGEN_VECTOR,
	TCGEN_WARP,
	TCGEN_LIGHTVECTOR,
	TCGEN_HALFANGLE,
	TCGEN_REFLECTION,
	TCGEN_NORMAL
} tcGenType_t;

typedef enum {
	TCMOD_TRANSLATE,
	TCMOD_SCALE,
	TCMOD_SCROLL,
	TCMOD_ROTATE,
	TCMOD_STRETCH,
	TCMOD_TURB,
	TCMOD_TRANSFORM
} tcModType_t;

typedef enum {
	RGBGEN_IDENTITY,
	RGBGEN_IDENTITYLIGHTING,
	RGBGEN_WAVE,
	RGBGEN_COLORWAVE,
	RGBGEN_VERTEX,
	RGBGEN_ONEMINUSVERTEX,
	RGBGEN_ENTITY,
	RGBGEN_ONEMINUSENTITY,
	RGBGEN_LIGHTINGAMBIENT,
	RGBGEN_LIGHTINGDIFFUSE,
	RGBGEN_CONST
} rgbGenType_t;

typedef enum {
	ALPHAGEN_IDENTITY,
	ALPHAGEN_WAVE,
	ALPHAGEN_ALPHAWAVE,
	ALPHAGEN_VERTEX,
	ALPHAGEN_ONEMINUSVERTEX,
	ALPHAGEN_ENTITY,
	ALPHAGEN_ONEMINUSENTITY,
	ALPHAGEN_DOT,
	ALPHAGEN_ONEMINUSDOT,
	ALPHAGEN_FADE,
	ALPHAGEN_ONEMINUSFADE,
	ALPHAGEN_LIGHTINGSPECULAR,
	ALPHAGEN_CONST
} alphaGenType_t;

typedef enum {
	TEX_GENERIC,
	TEX_LIGHTMAP,
	TEX_CINEMATIC
} texType_t;

typedef struct {
	waveForm_t				type;
	float					params[4];
} waveFunc_t;

typedef struct {
	GLenum					mode;
} cull_t;

typedef struct {
	ammoDisplayType_t		type;
	int						lowAmmo;
	char					remapShaders[3][MAX_QPATH];
} ammoDisplay_t;

typedef struct {
	texture_t				*farBox[6];
	float					cloudHeight;
	texture_t				*nearBox[6];
} skyParms_t;

typedef struct {
	deformVertexesType_t	type;
	waveFunc_t				func;
	float					params[3];
} deformVertexes_t;

typedef struct {
	GLint					rgbCombine;
	GLint					rgbSource[3];
	GLint					rgbOperand[3];
	GLint					rgbScale;

	GLint					alphaCombine;
	GLint					alphaSource[3];
	GLint					alphaOperand[3];
	GLint					alphaScale;

	GLfloat					constColor[4];
} texEnvCombine_t;

typedef struct {
	tcGenType_t				type;
	float					params[6];
} tcGen_t;

typedef struct {
	tcModType_t				type;
	waveFunc_t				func;
	float					params[6];
} tcMod_t;

typedef struct {
	GLenum					func;
	GLclampf				ref;
} alphaFunc_t;

typedef struct {
	GLenum					src;
	GLenum					dst;
} blendFunc_t;

typedef struct {
	GLenum					func;
} depthFunc_t;

typedef struct {
	rgbGenType_t			type;
	waveFunc_t				func;
	float					params[3];
} rgbGen_t;

typedef struct {
	alphaGenType_t			type;
	waveFunc_t				func;
	float					params[3];
} alphaGen_t;

typedef struct {
	texType_t				texType;

	unsigned				flags;

	texture_t				*textures[SHADER_MAX_TEXTURES];
	unsigned				numTextures;

	float					animFrequency;
	cinHandle_t				cinematicHandle;

	GLint					texEnv;
	texEnvCombine_t			texEnvCombine;

	tcGen_t					tcGen;
	tcMod_t					tcMod[SHADER_MAX_TCMOD];
	unsigned				tcModNum;
} stageBundle_t;

typedef struct {
	qboolean				ignore;

	unsigned				flags;

	stageBundle_t			*bundles[MAX_TEXTURE_UNITS];
	unsigned				numBundles;

	program_t				*vertexProgram;
	program_t				*fragmentProgram;

	alphaFunc_t				alphaFunc;
	blendFunc_t				blendFunc;
	depthFunc_t				depthFunc;

	rgbGen_t				rgbGen;
	alphaGen_t				alphaGen;
} shaderStage_t;

typedef struct shader_s {
	char					name[MAX_QPATH];
	int						shaderNum;
	shaderType_t			shaderType;
	unsigned				surfaceParm;

	unsigned				flags;

	cull_t					cull;
	sort_t					sort;
	ammoDisplay_t			ammoDisplay;
	unsigned				tessSize;
	skyParms_t				skyParms;
	deformVertexes_t		deformVertexes[SHADER_MAX_DEFORMVERTEXES];
	unsigned				deformVertexesNum;

	shaderStage_t			*stages[SHADER_MAX_STAGES];
	unsigned				numStages;

	struct shader_s			*nextHash;
} shader_t;

extern shader_t		*r_shaders[MAX_SHADERS];
extern int			r_numShaders;

extern shader_t		*r_defaultShader;
extern shader_t		*r_lightmapShader;
extern shader_t		*r_waterCausticsShader;
extern shader_t		*r_slimeCausticsShader;
extern shader_t		*r_lavaCausticsShader;

shader_t	*R_FindShader (const char *name, shaderType_t shaderType, unsigned surfaceParm);
void		R_ShaderList_f (void);
void		R_InitShaders (void);
void		R_ShutdownShaders (void);

/*
 =======================================================================

 MODELS

 =======================================================================
*/

#define ON_EPSILON			0.1

#define	SIDE_FRONT			0
#define	SIDE_BACK			1
#define	SIDE_ON				2

#define SKY_SIZE			8
#define SKY_INDICES			(SKY_SIZE * SKY_SIZE * 6)
#define SKY_VERTICES		((SKY_SIZE+1) * (SKY_SIZE+1))

#define	SURF_PLANEBACK		1
#define SURF_WATERCAUSTICS	2
#define SURF_SLIMECAUSTICS	4
#define SURF_LAVACAUSTICS	8

typedef struct {
	vec3_t				xyz;
	vec2_t				st;
	vec2_t				lightmap;
	color_t				color;
} surfPolyVert_t;

typedef struct surfPoly_s {
	struct surfPoly_s	*next;

	int					numIndices;
	int					numVertices;

	unsigned			*indices;
	surfPolyVert_t		*vertices;
} surfPoly_t;

typedef struct texInfo_s {
	float				vecs[2][4];
	int					width;
	int					height;
	int					flags;
	shader_t			*shader;
	int					numFrames;
	struct texInfo_s	*next;		// Animation chain
} texInfo_t;

typedef struct {
	int					flags;

	int					firstEdge;	// Look up in model->edges[]. Negative
	int					numEdges;	// numbers are backwards edges

	cplane_t			*plane;

	vec3_t				mins;
	vec3_t				maxs;

	short				textureMins[2];
	short				extents[2];

	surfPoly_t			*poly;		// Multiple if subdivided

	vec3_t				tangent;
	vec3_t				binormal;
	vec3_t				normal;

	texInfo_t			*texInfo;

	int					visFrame;
	int					fragmentFrame;

	// Lighting info
	int					dlightFrame;
	int					dlightBits;

	int					lmWidth;
	int					lmHeight;
	int					lmS;
	int					lmT;
	int					lmNum;
	byte				*lmSamples;

	int					numStyles;
	byte				styles[MAX_STYLES];
	float				cachedLight[MAX_STYLES];	// Values currently used in lightmap
} surface_t;

typedef struct node_s {
	// Common with leaf
	int					contents;	// -1, to differentiate from leafs
	int					visFrame;	// Node needs to be traversed if current

	vec3_t				mins;		// For bounding box culling
	vec3_t				maxs;		// For bounding box culling

	struct node_s		*parent;

	// Node specific
	cplane_t			*plane;
	struct node_s		*children[2];

	unsigned short		firstSurface;
	unsigned short		numSurfaces;
} node_t;

typedef struct leaf_s {
	// Common with node
	int					contents;	// Will be a negative contents number
	int					visFrame;	// Node needs to be traversed if current

	vec3_t				mins;		// For bounding box culling
	vec3_t				maxs;		// For bounding box culling

	struct node_s		*parent;

	// Leaf specific
	int					cluster;
	int					area;

	surface_t			**firstMarkSurface;
	int					numMarkSurfaces;
} leaf_t;

typedef struct {
	int					numClusters;
	int					bitOfs[8][2];
} vis_t;

typedef struct {
	vec3_t				point;
} vertex_t;

typedef struct {
	unsigned short		v[2];
} edge_t;

typedef struct {
	vec3_t				mins;
	vec3_t				maxs;
	vec3_t				origin;		// For sounds or lights
	float				radius;
	int					headNode;
	int					visLeafs;	// Not including the solid leaf 0
	int					firstFace;
	int					numFaces;
} submodel_t;

typedef struct {
	vec3_t				xyz;
	vec3_t				normal;
	vec2_t				st;
	vec2_t				sphere;
} skySideVert_t;

typedef struct {
	int					numIndices;
	int					numVertices;

	unsigned			indices[SKY_INDICES];
	skySideVert_t		vertices[SKY_VERTICES];
} skySide_t;

typedef struct {
	shader_t			*shader;

	float				rotate;
	vec3_t				axis;

	float				mins[2][6];
	float				maxs[2][6];

	unsigned			vbo[6];

	skySide_t			skySides[6];
} sky_t;

typedef struct {
	vec3_t				lightDir;
} lightGrid_t;

typedef struct {
	unsigned			index[3];
} mdlTriangle_t;

typedef struct {
	int					index[3];
} mdlNeighbor_t;

typedef struct {
	short				xyz[3];
	byte				tangent[2];
	byte				binormal[2];
	byte				normal[2];
} mdlXyzNormal_t;

typedef struct {
	vec2_t				st;
} mdlSt_t;

typedef struct {
	ammoDisplayType_t	type;
	int					lowAmmo;
	shader_t			*remapShaders[3];
} mdlAmmoDisplay_t;

typedef struct {
	shader_t			*shader;
	mdlAmmoDisplay_t	*ammoDisplay;
} mdlShader_t;

typedef struct {
	vec3_t				mins;
	vec3_t				maxs;
	vec3_t				scale;
	vec3_t				translate;
	float				radius;
} mdlFrame_t;

typedef struct {
	char				name[MAX_QPATH];
	vec3_t				origin;
	vec3_t				axis[3];
} mdlTag_t;

typedef struct {
	int					numTriangles;
	int					numVertices;
	int					numShaders;

	mdlTriangle_t		*triangles;
	mdlNeighbor_t		*neighbors;
	mdlXyzNormal_t		*xyzNormals;
	mdlSt_t				*st;
	mdlShader_t			*shaders;
} mdlSurface_t;

typedef struct {
	int					numFrames;
	int					numTags;
	int					numSurfaces;

	mdlFrame_t			*frames;
	mdlTag_t			*tags;
	mdlSurface_t		*surfaces;
} mdl_t;

typedef struct {
	shader_t			*shader;
	float				radius;
} sprFrame_t;

typedef struct {
	int					numFrames;
	sprFrame_t			*frames;
} spr_t;

typedef enum {
	MODEL_BAD,
	MODEL_BSP,
	MODEL_MD3,
	MODEL_MD2,
	MODEL_SP2
} modelType_t;

typedef struct model_s {
	char				name[MAX_QPATH];
	char				realName[MAX_QPATH];
	int					size;
	modelType_t			modelType;

	struct model_s		*nextHash;

	// Volume occupied by the model
	vec3_t				mins;
	vec3_t				maxs;
	float				radius;

	// Brush model
	int					numModelSurfaces;
	int					firstModelSurface;

	int					numSubmodels;
	submodel_t			*submodels;

	int					numVertexes;
	vertex_t			*vertexes;

	int					numSurfEdges;
	int					*surfEdges;

	int					numEdges;
	edge_t				*edges;

	int					numTexInfo;
	texInfo_t			*texInfo;

	int					numSurfaces;
	surface_t			*surfaces;

	int					numMarkSurfaces;
	surface_t			**markSurfaces;

	int					numPlanes;
	cplane_t			*planes;

	int					numNodes;
	int					firstNode;
	node_t				*nodes;

	int					numLeafs;
	leaf_t				*leafs;

	sky_t				*sky;

	vis_t				*vis;

	byte				*lightData;

	vec3_t				gridMins;
	vec3_t				gridSize;
	int					gridBounds[4];
	int					gridPoints;
	lightGrid_t			*lightGrid;

	// Alias model
	mdl_t				*alias[MD3_MAX_LODS];
	int					numAlias;

	// Sprite model
	spr_t				*sprite;
} model_t;

leaf_t		*R_PointInLeaf (const vec3_t p);
byte		*R_ClusterPVS (int cluster);
void		R_ModelList_f (void);
void		R_InitModels (void);
void		R_ShutdownModels (void);

/*
 =======================================================================

 GL STATE MANAGER

 =======================================================================
*/

typedef struct {
	qboolean		gl2D;
	float			stereoSeparation;
	unsigned short	gammaRamp[768];

	GLuint			screenRectTexture;

	unsigned		activeTMU;
	GLuint			texNum[MAX_TEXTURE_UNITS];
	GLint			texEnv[MAX_TEXTURE_UNITS];

	qboolean		cullFace;
	qboolean		polygonOffsetFill;
	qboolean		vertexProgram;
	qboolean		fragmentProgram;
	qboolean		alphaTest;
	qboolean		blend;
	qboolean		depthTest;

	GLenum			cullMode;
	GLfloat			offsetFactor;
	GLfloat			offsetUnits;
	GLenum			alphaFunc;
	GLclampf		alphaRef;
	GLenum			blendSrc;
	GLenum			blendDst;
	GLenum			depthFunc;
	GLboolean		depthMask;
} glState_t;

extern glState_t   glState;

void		GL_SelectTexture (unsigned tmu);
void		GL_BindTexture (texture_t *texture);
void		GL_TexEnv (GLint texEnv);
void		GL_Enable (GLenum cap);
void		GL_Disable (GLenum cap);
void		GL_CullFace (GLenum mode);
void		GL_PolygonOffset (GLfloat factor, GLfloat units);
void		GL_AlphaFunc (GLenum func, GLclampf ref);
void		GL_BlendFunc (GLenum src, GLenum dst);
void		GL_DepthFunc (GLenum func);
void		GL_DepthMask (GLboolean mask);
void		GL_SetDefaultState (void);
void		GL_Setup3D (void);
void		GL_Setup2D (void);
void		GL_CheckForErrors (void);

/*
 =======================================================================

 BACKEND

 =======================================================================
*/

#define VBO_OFFSET(i)		((char *)NULL + (i))

#define MAX_INDICES			8192 * 3
#define MAX_VERTICES		4096

#define MAX_MESHES			32768

typedef struct {
	unsigned		indexBuffer;
	unsigned		vertexBuffer;
	unsigned		normalBuffer;
	unsigned		colorBuffer;
	unsigned		texCoordBuffer[MAX_TEXTURE_UNITS];
} vbo_t;

typedef enum {
	MESH_SKY,
	MESH_SURFACE,
	MESH_ALIASMODEL,
	MESH_SPRITE,
	MESH_BEAM,
	MESH_PARTICLE,
	MESH_POLY
} meshType_t;

typedef struct {
	unsigned		sortKey;
	meshType_t		meshType;
	void			*mesh;
} mesh_t;

extern vbo_t		rb_vbo;

extern mesh_t		*rb_mesh;
extern shader_t		*rb_shader;
extern float		rb_shaderTime;
extern entity_t		*rb_entity;
extern int			rb_infoKey;

extern unsigned		indexArray[MAX_INDICES * 4];
extern vec3_t		vertexArray[MAX_VERTICES * 2];
extern vec3_t		tangentArray[MAX_VERTICES];
extern vec3_t		binormalArray[MAX_VERTICES];
extern vec3_t		normalArray[MAX_VERTICES];
extern color_t		colorArray[MAX_VERTICES];
extern vec3_t		texCoordArray[MAX_TEXTURE_UNITS][MAX_VERTICES];
extern color_t		inColorArray[MAX_VERTICES];
extern vec4_t		inTexCoordArray[MAX_VERTICES];

extern int			numIndex;
extern int			numVertex;

void		RB_CheckMeshOverflow (int numIndices, int numVertices);
void		RB_RenderMesh (void);
void		RB_RenderMeshes (mesh_t *meshes, int numMeshes);
void		RB_DrawStretchPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, const color_t modulate, shader_t *shader);
void		RB_DrawRotatedPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, float angle, const color_t modulate, shader_t *shader);
void		RB_DrawOffsetPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, float offsetX, float offsetY, const color_t modulate, shader_t *shader);

void		RB_VBOInfo_f (void);
unsigned	RB_AllocStaticBuffer (unsigned target, int size);
unsigned	RB_AllocStreamBuffer (unsigned target, int size);

void		RB_InitBackend (void);
void		RB_ShutdownBackend (void);

// =====================================================================

typedef struct {
	int				numShaders;
	int				numStages;
	int				numMeshes;
	int				numLeafs;
	int				numVertices;
	int				numIndices;
	int				totalIndices;

	int				numEntities;
	int				numDLights;
	int				numParticles;
	int				numPolys;
} refStats_t;

extern model_t		*r_worldModel;
extern entity_t		*r_worldEntity;

extern vec3_t		r_worldMins, r_worldMaxs;

extern int			r_frameCount;
extern int			r_visFrameCount;

extern int			r_viewCluster, r_viewCluster2;
extern int			r_oldViewCluster, r_oldViewCluster2;

extern mat4_t		r_projectionMatrix;
extern mat4_t		r_worldMatrix;
extern mat4_t		r_entityMatrix;
extern mat4_t		r_textureMatrix;

extern cplane_t		r_frustum[4];

extern float		r_frameTime;

extern mesh_t		r_solidMeshes[MAX_MESHES];
extern int			r_numSolidMeshes;

extern mesh_t		r_transMeshes[MAX_MESHES];
extern int			r_numTransMeshes;

extern entity_t		r_entities[MAX_ENTITIES];
extern int			r_numEntities;

extern dlight_t		r_dlights[MAX_DLIGHTS];
extern int			r_numDLights;

extern particle_t	r_particles[MAX_PARTICLES];
extern int			r_numParticles;

extern poly_t		r_polys[MAX_POLYS];
extern int			r_numPolys;
extern polyVert_t	r_polyVerts[MAX_POLY_VERTS];
extern int			r_numPolyVerts;

extern entity_t		*r_nullModels[MAX_ENTITIES];
extern int			r_numNullModels;

extern lightStyle_t	r_lightStyles[MAX_LIGHTSTYLES];

extern refDef_t		r_refDef;

extern refStats_t	r_stats;

extern cvar_t	*r_noRefresh;
extern cvar_t	*r_noVis;
extern cvar_t	*r_noCull;
extern cvar_t	*r_noBind;
extern cvar_t	*r_drawWorld;
extern cvar_t	*r_drawEntities;
extern cvar_t	*r_drawParticles;
extern cvar_t	*r_drawPolys;
extern cvar_t	*r_fullbright;
extern cvar_t	*r_lightmap;
extern cvar_t	*r_lockPVS;
extern cvar_t	*r_logFile;
extern cvar_t	*r_frontBuffer;
extern cvar_t	*r_showCluster;
extern cvar_t	*r_showTris;
extern cvar_t	*r_showNormals;
extern cvar_t	*r_showTangentSpace;
extern cvar_t	*r_showModelBounds;
extern cvar_t	*r_showShadowVolumes;
extern cvar_t	*r_offsetFactor;
extern cvar_t	*r_offsetUnits;
extern cvar_t	*r_debugSort;
extern cvar_t	*r_speeds;
extern cvar_t	*r_clear;
extern cvar_t	*r_singleShader;
extern cvar_t	*r_skipBackEnd;
extern cvar_t	*r_skipFrontEnd;
extern cvar_t	*r_allowSoftwareGL;
extern cvar_t	*r_maskMiniDriver;
extern cvar_t	*r_glDriver;
extern cvar_t	*r_allowExtensions;
extern cvar_t	*r_arb_multitexture;
extern cvar_t	*r_arb_texture_env_add;
extern cvar_t	*r_arb_texture_env_combine;
extern cvar_t	*r_arb_texture_env_dot3;
extern cvar_t	*r_arb_texture_cube_map;
extern cvar_t	*r_arb_texture_compression;
extern cvar_t	*r_arb_vertex_buffer_object;
extern cvar_t	*r_arb_vertex_program;
extern cvar_t	*r_arb_fragment_program;
extern cvar_t	*r_ext_draw_range_elements;
extern cvar_t	*r_ext_compiled_vertex_array;
extern cvar_t	*r_ext_texture_edge_clamp;
extern cvar_t	*r_ext_texture_filter_anisotropic;
extern cvar_t	*r_ext_texture_rectangle;
extern cvar_t	*r_ext_stencil_two_side;
extern cvar_t	*r_ext_generate_mipmap;
extern cvar_t	*r_ext_swap_control;
extern cvar_t	*r_swapInterval;
extern cvar_t	*r_finish;
extern cvar_t	*r_stereo;
extern cvar_t	*r_colorBits;
extern cvar_t	*r_depthBits;
extern cvar_t	*r_stencilBits;
extern cvar_t	*r_mode;
extern cvar_t	*r_fullscreen;
extern cvar_t	*r_customWidth;
extern cvar_t	*r_customHeight;
extern cvar_t	*r_displayRefresh;
extern cvar_t	*r_ignoreHwGamma;
extern cvar_t	*r_gamma;
extern cvar_t	*r_overBrightBits;
extern cvar_t	*r_ignoreGLErrors;
extern cvar_t	*r_shadows;
extern cvar_t	*r_caustics;
extern cvar_t	*r_lodBias;
extern cvar_t	*r_lodDistance;
extern cvar_t	*r_dynamicLights;
extern cvar_t	*r_modulate;
extern cvar_t	*r_ambientScale;
extern cvar_t	*r_directedScale;
extern cvar_t	*r_intensity;
extern cvar_t	*r_roundImagesDown;
extern cvar_t	*r_maxTextureSize;
extern cvar_t	*r_picmip;
extern cvar_t	*r_textureBits;
extern cvar_t	*r_textureFilter;
extern cvar_t	*r_textureFilterAnisotropy;
extern cvar_t	*r_jpegCompressionQuality;
extern cvar_t	*r_detailTextures;
extern cvar_t	*r_inGameVideo;

void		R_DrawAliasModel (void);
void		R_AddAliasModelToList (entity_t *entity);

void		R_MarkLights (void);
void		R_LightDir (const vec3_t origin, vec3_t lightDir);
void		R_LightingAmbient (void);
void		R_LightingDiffuse (void);
void		R_BeginBuildingLightmaps (void);
void		R_EndBuildingLightmaps (void);
void		R_BuildSurfaceLightmap (surface_t *surf);
void		R_UpdateSurfaceLightmap (surface_t *surf);

qboolean	R_CullBox (const vec3_t mins, const vec3_t maxs, int clipFlags);
qboolean	R_CullSphere (const vec3_t origin, float radius, int clipFlags);

void		R_RotateForEntity (entity_t *entity);

void		R_AddMeshToList (meshType_t meshType, void *mesh, shader_t *shader, entity_t *entity, int infoKey);

qboolean	R_GetModeInfo (int *width, int *height, int mode);

void		R_DrawSprite (void);
void		R_DrawBeam (void);
void		R_DrawParticle (void);
void		R_DrawPoly (void);

void		R_RenderView (void);

void		R_AddShadowToList (entity_t *entity, mdl_t *alias);
void		R_RenderShadows (void);

void		R_DrawSky (void);
void		R_ClearSky (void);
void		R_ClipSkySurface (surface_t *surf);
void		R_AddSkyToList (void);
void		R_SetupSky (const char *name, float rotate, const vec3_t axis);

void		R_DrawSurface (void);
void		R_AddBrushModelToList (entity_t *entity);
void		R_AddWorldToList (void);

/*
 =======================================================================

 IMPLEMENTATION SPECIFIC FUNCTIONS

 =======================================================================
*/

extern glConfig_t		glConfig;

#ifdef _WIN32

#define GL_DRIVER_OPENGL	"OpenGL32"

#define GLimp_SetDeviceGammaRamp		GLW_SetDeviceGammaRamp
#define	GLimp_SwapBuffers				GLW_SwapBuffers
#define GLimp_Activate					GLW_Activate
#define GLimp_Init						GLW_Init
#define GLimp_Shutdown					GLW_Shutdown

#else

#error "GLimp_* not available for this platform"

#endif


#endif	// __R_LOCAL_H__
