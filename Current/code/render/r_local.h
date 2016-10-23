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


#ifdef WIN32
#include "../win32/winquake.h"
#endif

#include <GL/gl.h>

#include "../qcommon/qcommon.h"
#include "../qcommon/editor.h"
#include "../client/render.h"

#include "glext.h"
#include "qgl.h"

#ifdef WIN32
#include "../win32/wglext.h"
#include "../win32/glw_win.h"
#endif


// This is defined in q_shared.h
#undef MAX_MODELS


/*
 =======================================================================

 TEXTURE MANAGER

 =======================================================================
*/

#define TEXTURES_HASH_SIZE				2048

#define MAX_TEXTURES					8192

typedef enum {
	TF_INTERNAL				= 0x0001,
	TF_NOPICMIP				= 0x0002,
	TF_UNCOMPRESSED			= 0x0004,
	TF_INTENSITY			= 0x0008,
	TF_ALPHA				= 0x0010,
	TF_NORMALMAP			= 0x0020,
} textureFlags_t;

typedef enum {
	TF_DEFAULT,
	TF_NEAREST,
	TF_LINEAR
} textureFilter_t;

typedef enum {
	TW_REPEAT,
	TW_CLAMP,
	TW_CLAMP_TO_ZERO,
	TW_CLAMP_TO_ZERO_ALPHA
} textureWrap_t;

typedef struct texture_s {
	char					name[MAX_OSPATH];

	textureFlags_t			flags;
	textureFilter_t			filter;
	textureWrap_t			wrap;

	qboolean				isCubeMap;

	int						frameCount;

	int						sourceWidth;
	int						sourceHeight;
	int						sourceSamples;

	int						uploadWidth;
	int						uploadHeight;
	int						uploadSize;
	unsigned				uploadFormat;
	unsigned				uploadTarget;
	unsigned				texNum;

	struct texture_s		*nextHash;
} texture_t;

texture_t	*R_FindTexture (const char *name, textureFlags_t flags, textureFilter_t filter, textureWrap_t wrap);
texture_t	*R_FindCubeMapTexture (const char *name, textureFlags_t flags, textureFilter_t filter, textureWrap_t wrap, qboolean cameraSpace);

void		R_SetTextureParameters (void);

qboolean	R_CaptureRenderToTexture (const char *name);
qboolean	R_UpdateTexture (const char *name, const byte *image, int width, int height);

void		R_ScreenShot_f (void);
void		R_EnvShot_f (void);
void		R_ListTextures_f (void);

void		R_InitTextures (void);
void		R_ShutdownTextures (void);

/*
 =======================================================================

 VIDEO MANAGER

 =======================================================================
*/

#define VIDEOS_HASH_SIZE				4

#define MAX_VIDEOS						16

typedef struct video_s {
	char					name[MAX_OSPATH];

	fileHandle_t			file;
	int						size;
	int						offset;
	int						header;

	int						width;
	int						height;
	byte					*buffer;

	int						frameCount;
	int						frameRate;

	int						startTime;
	int						currentFrame;

	int						roqWidth;
	int						roqHeight;
	byte					*roqCache;
	byte					*roqBuffers[2];

	roqChunk_t				roqChunk;
	roqCell_t				roqCells[256];
	roqQCell_t				roqQCells[256];

	struct video_s			*nextHash;
} video_t;

video_t		*R_PlayVideo (const char *name);
void		R_UpdateVideo (video_t *video, int time);

void		R_ListVideos_f (void);

void		R_InitVideos (void);
void		R_ShutdownVideos (void);

/*
 =======================================================================

 PROGRAM MANAGER

 =======================================================================
*/

#define PROGRAMS_HASH_SIZE				128

#define MAX_PROGRAMS					512

typedef struct program_s {
	char					name[MAX_OSPATH];

	qboolean				uploadSuccessful;
	int						uploadInstructions;
	int						uploadNative;
	unsigned				uploadTarget;
	unsigned				progNum;

	struct program_s		*nextHash;
} program_t;

program_t	*R_FindProgram (const char *name, unsigned target);

void		R_ListPrograms_f (void);

void		R_InitPrograms (void);
void		R_ShutdownPrograms (void);

/*
 =======================================================================

 MATERIAL MANAGER

 =======================================================================
*/

#define TABLES_HASH_SIZE				1024

#define MAX_TABLES						4096
#define MAX_TABLE_SIZE					256

#define MATERIALS_HASH_SIZE				1024

#define MAX_MATERIALS					4096
#define MAX_STAGES						256

#define MAX_EXPRESSION_OPS				4096
#define MAX_EXPRESSION_REGISTERS		4096

#define MAX_PROGRAM_PARMS				16
#define MAX_PROGRAM_MAPS				8

#define MAX_TEXTURE_TRANSFORMS			4

typedef struct table_s {
	char					name[MAX_OSPATH];
	int						index;

	qboolean				clamp;
	qboolean				snap;

	int						size;
	float					*values;

	struct table_s			*nextHash;
} table_t;

typedef enum {
	OP_TYPE_MULTIPLY,
	OP_TYPE_DIVIDE,
	OP_TYPE_MOD,
	OP_TYPE_ADD,
	OP_TYPE_SUBTRACT,
	OP_TYPE_GREATER,
	OP_TYPE_LESS,
	OP_TYPE_GEQUAL,
	OP_TYPE_LEQUAL,
	OP_TYPE_EQUAL,
	OP_TYPE_NOTEQUAL,
	OP_TYPE_AND,
	OP_TYPE_OR,
	OP_TYPE_TABLE
} opType_t;

typedef enum {
	EXP_REGISTER_ONE,
	EXP_REGISTER_ZERO,
	EXP_REGISTER_TIME,
	EXP_REGISTER_PARM0,
	EXP_REGISTER_PARM1,
	EXP_REGISTER_PARM2,
	EXP_REGISTER_PARM3,
	EXP_REGISTER_PARM4,
	EXP_REGISTER_PARM5,
	EXP_REGISTER_PARM6,
	EXP_REGISTER_PARM7,
	EXP_REGISTER_GLOBAL0,
	EXP_REGISTER_GLOBAL1,
	EXP_REGISTER_GLOBAL2,
	EXP_REGISTER_GLOBAL3,
	EXP_REGISTER_GLOBAL4,
	EXP_REGISTER_GLOBAL5,
	EXP_REGISTER_GLOBAL6,
	EXP_REGISTER_GLOBAL7,
	EXP_REGISTER_NUM_PREDEFINED
} expRegister_t;

typedef enum {
	MT_GENERIC,
	MT_LIGHT,
	MT_NOMIP
} materialType_t;

typedef enum {
	MC_OPAQUE,
	MC_PERFORATED,
	MC_TRANSLUCENT
} materialCoverage_t;

typedef enum {
	SURFACEPARM_LIGHTING	= 0x01,
	SURFACEPARM_SKY			= 0x02,
	SURFACEPARM_WARP		= 0x04,
	SURFACEPARM_TRANS33		= 0x08,
	SURFACEPARM_TRANS66		= 0x10,
	SURFACEPARM_FLOWING		= 0x20
} surfaceParm_t;

typedef enum {
	SUBVIEW_NONE,
	SUBVIEW_MIRROR,
	SUBVIEW_REMOTE
} subview_t;

typedef enum {
	SORT_BAD,
	SORT_SUBVIEW,
	SORT_OPAQUE,
	SORT_SKY,
	SORT_DECAL,
	SORT_SEE_THROUGH,
	SORT_BANNER,
	SORT_UNDERWATER,
	SORT_WATER,
	SORT_FARTHEST,
	SORT_FAR,
	SORT_MEDIUM,
	SORT_CLOSE,
	SORT_ADDITIVE,
	SORT_ALMOST_NEAREST,
	SORT_NEAREST,
	SORT_POST_PROCESS
} sort_t;

typedef enum {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

typedef enum {
	DFRM_NONE,
	DFRM_EXPAND,
	DFRM_MOVE,
	DFRM_SPRITE,
	DFRM_TUBE
} deform_t;

typedef enum {
	SL_AMBIENT,
	SL_BUMP,
	SL_DIFFUSE,
	SL_SPECULAR
} stageLighting_t;

typedef enum {
	TG_EXPLICIT,
	TG_VECTOR,
	TG_REFLECT,
	TG_NORMAL,
	TG_SKYBOX,
	TG_SCREEN
} texGen_t;

typedef enum {
	TT_TRANSLATE,
	TT_SCROLL,
	TT_SCALE,
	TT_CENTERSCALE,
	TT_SHEAR,
	TT_ROTATE
} texTransform_t;

typedef enum {
	VC_IGNORE,
	VC_MODULATE,
	VC_INVERSE_MODULATE
} vertexColor_t;

typedef struct {
	opType_t				opType;
	int						a;
	int						b;
	int						c;
} expOp_t;

typedef struct {
	float					stayTime;
	float					fadeTime;

	vec4_t					startRGBA;
	vec4_t					endRGBA;
} decalInfo_t;

typedef struct {
	unsigned				index;

	int						registers[4];
} programParm_t;

typedef struct {
	unsigned				index;

	texture_t				*texture;
	video_t					*video;
} programMap_t;

typedef struct {
	textureFlags_t			flags;
	textureFilter_t			filter;
	textureWrap_t			wrap;

	texture_t				*texture;
	video_t					*video;

	texGen_t				texGen;
	vec4_t					texGenVectors[2];

	texTransform_t			texTransform[MAX_TEXTURE_TRANSFORMS];
	int						texTransformRegisters[MAX_TEXTURE_TRANSFORMS][2];
	int						numTexTransforms;
} textureStage_t;

typedef struct {
	vertexColor_t			vertexColor;

	int						registers[4];
} colorStage_t;

typedef struct {
	program_t				*vertexProgram;
	program_t				*fragmentProgram;

	programParm_t			vertexParms[MAX_PROGRAM_PARMS];
	int						numVertexParms;

	programParm_t			fragmentParms[MAX_PROGRAM_PARMS];
	int						numFragmentParms;

	programMap_t			fragmentMaps[MAX_PROGRAM_MAPS];
	int						numFragmentMaps;
} programStage_t;

typedef struct {
	int						conditionRegister;

	stageLighting_t			lighting;

	textureStage_t			textureStage;
	colorStage_t			colorStage;
	programStage_t			programStage;

	qboolean				programs;

	float					specularExponent;

	qboolean				shadowDraw;

	qboolean				privatePolygonOffset;

	qboolean				alphaTest;
	int						alphaTestRegister;

	qboolean				ignoreAlphaTest;

	qboolean				blend;
	unsigned				blendSrc;
	unsigned				blendDst;

	qboolean				maskRed;
	qboolean				maskGreen;
	qboolean				maskBlue;
	qboolean				maskAlpha;
} stage_t;

typedef struct material_s {
	char					name[MAX_OSPATH];
	int						index;
	qboolean				defaulted;

	materialType_t			type;
	materialCoverage_t		coverage;

	surfaceParm_t			surfaceParm;

	int						conditionRegister;

	qboolean				noOverlays;
	qboolean				noFog;
	qboolean				noShadows;
	qboolean				noSelfShadow;

	qboolean				fogLight;
	qboolean				blendLight;
	qboolean				ambientLight;

	int						spectrum;

	texture_t				*lightFalloffImage;

	decalInfo_t				decalInfo;

	subview_t				subview;
	int						subviewWidth;
	int						subviewHeight;

	sort_t					sort;

	cullType_t				cullType;

	qboolean				polygonOffset;

	deform_t				deform;
	int						deformRegisters[3];

	stage_t					*stages;
	int						numStages;
	int						numAmbientStages;

	qboolean				constantExpressions;

	expOp_t					*ops;
	int						numOps;

	float					*expressionRegisters;
	int						numRegisters;

	struct mtrScript_s		*mtrScript;

	struct material_s		*nextHash;
} material_t;

material_t	*R_FindMaterial (const char *name, materialType_t type, surfaceParm_t surfaceParm);

void		R_EvaluateRegisters (material_t *material, float time, const float *entityParms, const float *globalParms);

void		R_EnumMaterialScripts (void (*callback)(const char *));

void		R_MaterialInfo_f (void);
void		R_ListTables_f (void);
void		R_ListMaterials_f (void);

void		R_InitMaterials (void);
void		R_ShutdownMaterials (void);

/*
 =======================================================================

 MODEL MANAGER

 =======================================================================
*/

#define MODELS_HASH_SIZE				512

#define MAX_MODELS						2048

#define	SURF_PLANEBACK					0x01
#define SURF_IN_WATER					0x02
#define SURF_IN_SLIME					0x04
#define SURF_IN_LAVA					0x08

typedef struct texInfo_s {
	int						flags;
	vec4_t					vecs[2];

	material_t				*material;
	int						width;
	int						height;
	qboolean				clamp;

	int						numFrames;
	struct texInfo_s		*next;		// Animation chain
} texInfo_t;

typedef struct {
	index_t					index[3];
	int						neighbor[3];
} surfTriangle_t;

typedef struct {
	vec3_t					normal;
	float					dist;
} surfFacePlane_t;

typedef struct {
	vec3_t					xyz;
	vec3_t					normal;
	vec3_t					tangents[2];
	vec2_t					st;
	color_t					color;
} surfVertex_t;

typedef struct {
	int						flags;

	int						firstEdge;	// Look up in model->edges[]. Negative
	int						numEdges;	// numbers are backwards edges

	cplane_t				*plane;

	vec3_t					origin;
	vec3_t					mins;
	vec3_t					maxs;

	texInfo_t				*texInfo;

	int						numTriangles;
	surfTriangle_t			*triangles;

	int						numVertices;
	surfVertex_t			*vertices;

	int						viewCount;
	int						worldCount;
	int						lightCount;
	int						fragmentCount;
} surface_t;

typedef struct {
	vec3_t					origin;
	vec3_t					mins;
	vec3_t					maxs;

	int						numLeafs;		// For visibility determination
	struct leaf_s			**leafList;		// For visibility determination

	material_t				*material;

	int						numTriangles;
	surfFacePlane_t			*facePlanes;
	surfTriangle_t			*triangles;

	int						numVertices;
	surfVertex_t			*vertices;

	int						viewCount;
} decSurface_t;

typedef struct node_s {
	// Common with leaf
	int						contents;		// -1, to differentiate from leafs
	int						viewCount;		// Node needs to be traversed if current
	int						visCount;		// Node needs to be traversed if current

	vec3_t					mins;			// For bounding box culling
	vec3_t					maxs;			// For bounding box culling

	struct node_s			*parent;

	// Node specific
	cplane_t				*plane;
	struct node_s			*children[2];	

	unsigned short			firstSurface;
	unsigned short			numSurfaces;
} node_t;

typedef struct leaf_s {
	// Common with node
	int						contents;		// Will be a negative contents number
	int						viewCount;		// Node needs to be traversed if current
	int						visCount;		// Node needs to be traversed if current

	vec3_t					mins;			// For bounding box culling
	vec3_t					maxs;			// For bounding box culling

	struct node_s			*parent;

	// Leaf specific
	int						cluster;
	int						area;

	surface_t				**firstMarkSurface;
	int						numMarkSurfaces;
} leaf_t;

typedef struct {
	int						numClusters;
	int						bitOfs[8][2];
} vis_t;

typedef struct {
	vec3_t					point;
} vertex_t;

typedef struct {
	unsigned short			v[2];
} edge_t;

typedef struct {
	vec3_t					origin;			// For sounds or lights
	vec3_t					mins;
	vec3_t					maxs;
	float					radius;

	int						firstFace;
	int						numFaces;
} submodel_t;

typedef struct {
	material_t				*material;

	float					rotate;
	vec3_t					axis;
} sky_t;

typedef struct {
	index_t					index[3];
	int						neighbor[3];
} mdlTriangle_t;

typedef struct {
	vec3_t					normal;
	float					dist;
} mdlFacePlane_t;

typedef struct {
	vec3_t					xyz;
	vec3_t					normal;
	vec3_t					tangents[2];
} mdlXyzNormal_t;

typedef struct {
	vec2_t					st;
} mdlSt_t;

typedef struct {
	material_t				*material;
} mdlMaterial_t;

typedef struct {
	vec3_t					mins;
	vec3_t					maxs;
	float					radius;
} mdlFrame_t;

typedef struct {
	char					name[MAX_QPATH];
	vec3_t					origin;
	vec3_t					axis[3];
} mdlTag_t;

typedef struct {
	int						numTriangles;
	int						numVertices;
	int						numMaterials;

	mdlTriangle_t			*triangles;
	mdlFacePlane_t			*facePlanes;
	mdlXyzNormal_t			*xyzNormals;
	mdlSt_t					*st;
	mdlMaterial_t			*materials;
} mdlSurface_t;

typedef struct {
	int						numFrames;
	int						numTags;
	int						numSurfaces;

	mdlFrame_t				*frames;
	mdlTag_t				*tags;
	mdlSurface_t			*surfaces;
} mdl_t;

typedef enum {
	MODEL_BAD,
	MODEL_BSP,
	MODEL_MD3,
	MODEL_MD2
} modelType_t;

typedef struct model_s {
	char					name[MAX_OSPATH];
	char					realName[MAX_OSPATH];
	int						size;
	modelType_t				modelType;

	struct model_s			*nextHash;

	// Brush model
	int						firstModelSurface;
	int						numModelSurfaces;

	int						numSubmodels;
	submodel_t				*submodels;

	int						numVertexes;
	vertex_t				*vertexes;

	int						numSurfEdges;
	int						*surfEdges;

	int						numEdges;
	edge_t					*edges;

	int						numTexInfo;
	texInfo_t				*texInfo;

	int						numSurfaces;
	surface_t				*surfaces;

	int						numDecorationSurfaces;
	decSurface_t			*decorationSurfaces;

	int						numMarkSurfaces;
	surface_t				**markSurfaces;

	int						numPlanes;
	cplane_t				*planes;

	int						numNodes;
	node_t					*nodes;

	int						numLeafs;
	leaf_t					*leafs;

	vis_t					*vis;

	sky_t					*sky;

	vec3_t					mins;
	vec3_t					maxs;
	float					radius;

	// Alias model
	mdl_t					*alias;
} model_t;

leaf_t		*R_PointInLeaf (const vec3_t point);
int			R_BoxInLeaves (node_t *node, const vec3_t mins, const vec3_t maxs, leaf_t **leafList, int numLeafs, int maxLeafs);
int			R_SphereInLeaves (node_t *node, const vec3_t center, float radius, leaf_t **leafList, int numLeafs, int maxLeafs);

void		R_ClusterPVS (int cluster, byte *vis);

void		R_ListModels_f (void);

void		R_InitModels (void);
void		R_ShutdownModels (void);

/*
 =======================================================================

 DECAL MANAGER

 =======================================================================
*/

#define MAX_DECALS						2048
#define MAX_DECAL_VERTICES				10

typedef struct {
	vec3_t					xyz;
	vec3_t					normal;
	vec2_t					st;
} decalVertex_t;

typedef struct decal_s {
	struct decal_s			*prev;
	struct decal_s			*next;

	float					time;
	float					duration;
	material_t				*material;

	surface_t				*parent;

	int						numVertices;
	decalVertex_t			vertices[MAX_DECAL_VERTICES];
} decal_t;

void		R_ClearDecals (void);

/*
 =======================================================================

 MESH MANAGER

 =======================================================================
*/

#define MAX_MESHES						65536
#define MAX_POST_PROCESS_MESHES			4096

typedef void				meshData_t;

typedef enum {
	MESH_SURFACE,
	MESH_DECORATION,
	MESH_ALIASMODEL,
	MESH_SPRITE,
	MESH_BEAM,
	MESH_DECAL,
	MESH_POLY
} meshType_t;

typedef struct {
	unsigned				sort;
	meshData_t				*data;
} mesh_t;

void		R_SortMeshes (void);
void		R_AddMeshToList (meshType_t type, meshData_t *data, material_t *material, renderEntity_t *entity);
void		R_DecomposeSort (unsigned sort, material_t **material, renderEntity_t **entity, meshType_t *type, qboolean *caps);

/*
 =======================================================================

 LIGHT MANAGER

 =======================================================================
*/

#define MAX_LIGHTS						4096
#define MAX_FOG_LIGHTS					1024

typedef struct {
	int						index;

	vec3_t					origin;
	vec3_t					center;
	vec3_t					angles;
	vec3_t					radius;
	int						style;
	int						detailLevel;
	qboolean				parallel;
	qboolean				noShadows;
	material_t				*material;
	float					materialParms[MAX_ENTITY_MATERIAL_PARMS];

	vec3_t					axis[3];
	vec3_t					corners[8];

	qboolean				rotated;
	vec3_t					mins;
	vec3_t					maxs;
	cplane_t				frustum[6];

	leaf_t					*leaf;
	byte					*vis;
} lightSource_t;

typedef struct {
	vec3_t					origin;

	material_t				*material;
	float					materialParms[MAX_ENTITY_MATERIAL_PARMS];

	vec3_t					corners[8];

	vec3_t					planeNormal;
	float					planeDist;

	mat4_t					matrix;

	int						scissorX;
	int						scissorY;
	int						scissorWidth;
	int						scissorHeight;

	float					depthMin;
	float					depthMax;

	qboolean				castShadows;

	mesh_t					*shadows[2];
	int						numShadows[2];

	mesh_t					*interactions[2];
	int						numInteractions[2];
} light_t;

void		R_LoadLights (void);

/*
 =======================================================================

 GL STATE MANAGER

 =======================================================================
*/

#define MAX_TEXTURE_UNITS				8

typedef struct {
	unsigned				activeUnit;
	unsigned				activeTarget[MAX_TEXTURE_UNITS];

	unsigned				texNum[MAX_TEXTURE_UNITS];
	int						texEnv[MAX_TEXTURE_UNITS];
	int						texGenS[MAX_TEXTURE_UNITS];
	int						texGenT[MAX_TEXTURE_UNITS];
	int						texGenR[MAX_TEXTURE_UNITS];
	int						texGenQ[MAX_TEXTURE_UNITS];

	unsigned				progNum;

	mat4_t					projectionMatrix;
	mat4_t					modelviewMatrix;
	mat4_t					textureMatrix[MAX_TEXTURE_UNITS];

	qboolean				cullFace;
	qboolean				polygonOffsetFill;
	qboolean				alphaTest;
	qboolean				blend;
	qboolean				depthTest;
	qboolean				stencilTest;
	qboolean				textureGenS[MAX_TEXTURE_UNITS];
	qboolean				textureGenT[MAX_TEXTURE_UNITS];
	qboolean				textureGenR[MAX_TEXTURE_UNITS];
	qboolean				textureGenQ[MAX_TEXTURE_UNITS];
	qboolean				vertexProgram;
	qboolean				fragmentProgram;

	unsigned				cullMode;
	float					offsetFactor;
	float					offsetUnits;
	unsigned				alphaFunc;
	float					alphaRef;
	unsigned				blendSrc;
	unsigned				blendDst;
	unsigned				depthFunc;
	unsigned				stencilFunc;
	int						stencilRef;
	unsigned				stencilRefMask;
	unsigned				stencilFail;
	unsigned				stencilZFail;
	unsigned				stencilZPass;

	qboolean				colorMask[4];
	qboolean				depthMask;
	unsigned				stencilMask;

	float					depthMin;
	float					depthMax;

	unsigned				frontFace;

	int						scissorX;
	int						scissorY;
	int						scissorWidth;
	int						scissorHeight;

	float					depthBoundsMin;
	float					depthBoundsMax;
} glState_t;

void		GL_SelectTexture (unsigned unit);
void		GL_EnableTexture (unsigned target);
void		GL_DisableTexture (void);
void		GL_BindTexture (texture_t *texture);
void		GL_TexEnv (int texEnv);
void		GL_TexGen (unsigned texCoord, int texGen);
void		GL_BindProgram (program_t *program);
void		GL_BindProgramTexture (unsigned unit, texture_t *texture);
void		GL_LoadIdentity (unsigned mode);
void		GL_LoadMatrix (unsigned mode, const float *m);
void		GL_Enable (unsigned cap);
void		GL_Disable (unsigned cap);
void		GL_CullFace (unsigned mode);
void		GL_PolygonOffset (float factor, float units);
void		GL_AlphaFunc (unsigned func, float ref);
void		GL_BlendFunc (unsigned src, unsigned dst);
void		GL_DepthFunc (unsigned func);
void		GL_StencilFunc (unsigned func, int ref, unsigned mask);
void		GL_StencilOp (unsigned fail, unsigned zFail, unsigned zPass);
void		GL_ColorMask (qboolean red, qboolean green, qboolean blue, qboolean alpha);
void		GL_DepthMask (qboolean mask);
void		GL_StencilMask (unsigned mask);
void		GL_DepthRange (float min, float max);
void		GL_FrontFace (unsigned face);
void		GL_Scissor (int x, int y, int width, int height);
void		GL_DepthBounds (float min, float max);

void		GL_SetDefaultState (void);
void		GL_CheckForErrors (void);

/*
 =======================================================================

 FRONT-END

 =======================================================================
*/

#define MAX_MIRROR_SURFACES				1024

#define MAX_RENDER_CROPS				16

typedef struct {
	mesh_t					meshes[MAX_MESHES];
	int						numMeshes;
	int						firstMesh;

	mesh_t					postProcessMeshes[MAX_POST_PROCESS_MESHES];
	int						numPostProcessMeshes;
	int						firstPostProcessMesh;

	light_t					lights[MAX_LIGHTS];
	int						numLights;
	int						firstLight;

	light_t					fogLights[MAX_FOG_LIGHTS];
	int						numFogLights;
	int						firstFogLight;
} viewData_t;

typedef struct {
	renderEntity_t			entities[MAX_RENDER_ENTITIES];
	int						numEntities;
	int						firstEntity;

	renderLight_t			lights[MAX_RENDER_LIGHTS];
	int						numLights;
	int						firstLight;

	renderPoly_t			polys[MAX_RENDER_POLYS];
	int						numPolys;
	int						firstPoly;
} scene_t;

typedef struct {
	vec3_t					rgb;
} lightStyle_t;

typedef struct {
	qboolean				primaryView;

	subview_t				subview;

	cplane_t				frustum[5];

	vec3_t					visMins;
	vec3_t					visMaxs;

	mat4_t					perspectiveMatrix;
	mat4_t					worldMatrix;
	mat4_t					skyBoxMatrix;
	mat4_t					mirrorMatrix;

	// Mesh lists
	mesh_t					*meshes;
	int						numMeshes;

	mesh_t					*postProcessMeshes;
	int						numPostProcessMeshes;

	// Light lists
	light_t					*lights;
	int						numLights;

	light_t					*fogLights;
	int						numFogLights;

	// Scene render lists
	renderEntity_t			*renderEntities;
	int						numRenderEntities;

	renderLight_t			*renderLights;
	int						numRenderLights;

	renderPoly_t			*renderPolys;
	int						numRenderPolys;
} renderViewParms_t;

typedef struct {
	material_t				*material;
	renderEntity_t			*entity;

	// Mirror render specific
	surface_t				*surfaces[MAX_MIRROR_SURFACES];
	int						numSurfaces;

	vec3_t					planeOrigin;
	vec3_t					planeNormal;
	float					planeDist;

	// Remote render specific
	renderView_t			*remoteView;
} renderSubviewParms_t;

typedef struct {
	int						width;
	int						height;

	float					scaleX;
	float					scaleY;
} renderCrop_t;

typedef struct {
	renderView_t			renderView;

	int						numEntities;
	int						firstEntity;

	int						numLights;
	int						firstLight;

	int						numPolys;
	int						firstPoly;
} primaryView_t;

typedef struct {
	int						boxIn;
	int						boxOut;
	int						sphereIn;
	int						sphereOut;

	int						surfaces;
	int						leafs;

	int						entities;
	int						decals;
	int						polys;

	int						deforms;
	int						deformVertices;

	int						views;
	int						draws;
	int						indices;
	int						vertices;
	int						shadowIndices;
	int						shadowVertices;

	int						vertexBuffers;
	int						vertexBufferBytes;

	int						textures;
	int						textureBytes;

	int						captureRenders;
	int						captureRenderPixels;
	int						updateTextures;
	int						updateTexturePixels;

	int						interactions;
	int						interactionsMemory;
	int						interactionsLight;
	int						interactionsFog;

	int						shadows;
	int						shadowsMemory;
	int						shadowsZPass;
	int						shadowsZFail;

	int						lights;
	int						lightsStatic;
	int						lightsDynamic;

	float					overdraw;
	float					overdrawShadows;
	float					overdrawLights;

	double					timeInteractions;
	double					timeShadows;
	double					timeLights;

	double					timeFrontEnd;
	double					timeBackEnd;
} performanceCounters_t;

typedef struct {
	int						frameCount;
	int						viewCount;
	int						visCount;
	int						worldCount;
	int						lightCount;
	int						fragmentCount;

	int						viewCluster;
	int						oldViewCluster;
	int						viewCluster2;
	int						oldViewCluster2;

	// View mesh and light lists
	viewData_t				viewData;

	// Scene render lists
	scene_t					scene;

	// Light styles
	lightStyle_t			lightStyles[MAX_LIGHTSTYLES];

	// Render view/subview parms
	renderViewParms_t		renderViewParms;
	renderSubviewParms_t	renderSubviewParms;

	// Render view
	renderView_t			renderView;

	// Render crops
	renderCrop_t			renderCrops[MAX_RENDER_CROPS];
	int						numRenderCrops;

	renderCrop_t			*currentRenderCrop;

	// Some functions need a copy of the primary view
	qboolean				primaryViewAvailable;
	primaryView_t			primaryView;

	// The following fields can be safely used by the back-end
	performanceCounters_t	pc;

	model_t					*worldModel;
	renderEntity_t			*worldEntity;

	texture_t				*defaultTexture;
	texture_t				*whiteTexture;
	texture_t				*blackTexture;
	texture_t				*flatTexture;
	texture_t				*attenuationTexture;
	texture_t				*falloffTexture;
	texture_t				*fogTexture;
	texture_t				*fogEnterTexture;
	texture_t				*cinematicTexture;
	texture_t				*scratchTexture;
	texture_t				*accumTexture;
	texture_t				*mirrorRenderTexture;
	texture_t				*remoteRenderTexture;
	texture_t				*currentRenderTexture;
	texture_t				*normalCubeMapTexture;

	program_t				*interactionVProgram;
	program_t				*interactionFProgram;
	program_t				*interactionFastVProgram;
	program_t				*interactionFastFProgram;
	program_t				*ambientInteractionVProgram;
	program_t				*ambientInteractionFProgram;
	program_t				*ambientInteractionFastVProgram;
	program_t				*ambientInteractionFastFProgram;

	material_t				*defaultMaterial;
	material_t				*defaultLightMaterial;
	material_t				*noDrawMaterial;
	material_t				*waterCausticsMaterial;
	material_t				*slimeCausticsMaterial;
	material_t				*lavaCausticsMaterial;

	material_t				*sortedMaterials[MAX_MATERIALS];

	byte					gammaTable[256];
} trGlobals_t;

extern trGlobals_t			tr;

extern cvar_t	*r_logFile;
extern cvar_t	*r_clear;
extern cvar_t	*r_clearColor;
extern cvar_t	*r_frontBuffer;
extern cvar_t	*r_screenFraction;
extern cvar_t	*r_subviewOnly;
extern cvar_t	*r_lockPVS;
extern cvar_t	*r_zNear;
extern cvar_t	*r_zFar;
extern cvar_t	*r_offsetFactor;
extern cvar_t	*r_offsetUnits;
extern cvar_t	*r_shadowPolygonFactor;
extern cvar_t	*r_shadowPolygonOffset;
extern cvar_t	*r_colorMipLevels;
extern cvar_t	*r_singleMaterial;
extern cvar_t	*r_singleLight;
extern cvar_t	*r_showCluster;
extern cvar_t	*r_showCull;
extern cvar_t	*r_showSurfaces;
extern cvar_t	*r_showDynamic;
extern cvar_t	*r_showDeforms;
extern cvar_t	*r_s3tc;
extern cvar_t	*r_showPrimitives;
extern cvar_t	*r_showVertexBuffers;
extern cvar_t	*r_showTextureUsage;
extern cvar_t	*r_showRenderToTexture;
extern cvar_t	*r_showDepth;
extern cvar_t	*r_showOverdraw;
extern cvar_t	*r_showTris;
extern cvar_t	*r_showNormals;
extern cvar_t	*r_showTextureVectors;
extern cvar_t	*r_showTangentSpace;
extern cvar_t	*r_showVertexColors;
extern cvar_t	*r_showModelBounds;
extern cvar_t	*r_showInteractions;
extern cvar_t	*r_showShadows;
extern cvar_t	*r_showShadowCount;
extern cvar_t	*r_showShadowTris;
extern cvar_t	*r_showShadowVolumes;
extern cvar_t	*r_showShadowSilhouettes;
extern cvar_t	*r_showLights;
extern cvar_t	*r_showLightCount;
extern cvar_t	*r_showLightBounds;
extern cvar_t	*r_showLightScissors;
extern cvar_t	*r_showRenderTime;
extern cvar_t	*r_skipVisibility;
extern cvar_t	*r_skipAreas;
extern cvar_t	*r_skipSuppress;
extern cvar_t	*r_skipCulling;
extern cvar_t	*r_skipInteractionCulling;
extern cvar_t	*r_skipShadowCulling;
extern cvar_t	*r_skipLightCulling;
extern cvar_t	*r_skipBatching;
extern cvar_t	*r_skipDecorations;
extern cvar_t	*r_skipEntities;
extern cvar_t	*r_skipDecals;
extern cvar_t	*r_skipPolys;
extern cvar_t	*r_skipLights;
extern cvar_t	*r_skipExpressions;
extern cvar_t	*r_skipConstantExpressions;
extern cvar_t	*r_skipDeforms;
extern cvar_t	*r_skipPrograms;
extern cvar_t	*r_skipAmbient;
extern cvar_t	*r_skipBump;
extern cvar_t	*r_skipDiffuse;
extern cvar_t	*r_skipSpecular;
extern cvar_t	*r_skipInteractions;
extern cvar_t	*r_skipFogLights;
extern cvar_t	*r_skipBlendLights;
extern cvar_t	*r_skipTranslucent;
extern cvar_t	*r_skipPostProcess;
extern cvar_t	*r_skipScissors;
extern cvar_t	*r_skipDepthBounds;
extern cvar_t	*r_skipSubviews;
extern cvar_t	*r_skipVideos;
extern cvar_t	*r_skipCopyTexture;
extern cvar_t	*r_skipDynamicTextures;
extern cvar_t	*r_skipRender;
extern cvar_t	*r_skipRenderContext;
extern cvar_t	*r_skipFrontEnd;
extern cvar_t	*r_skipBackEnd;
extern cvar_t	*r_glDriver;
extern cvar_t	*r_ignoreGLErrors;
extern cvar_t	*r_multiSamples;
extern cvar_t	*r_swapInterval;
extern cvar_t	*r_mode;
extern cvar_t	*r_fullscreen;
extern cvar_t	*r_customWidth;
extern cvar_t	*r_customHeight;
extern cvar_t	*r_displayRefresh;
extern cvar_t	*r_gamma;
extern cvar_t	*r_brightness;
extern cvar_t	*r_finish;
extern cvar_t	*r_renderer;
extern cvar_t	*r_vertexBuffers;
extern cvar_t	*r_shadows;
extern cvar_t	*r_playerShadow;
extern cvar_t	*r_dynamicLights;
extern cvar_t	*r_fastInteractions;
extern cvar_t	*r_caustics;
extern cvar_t	*r_lightScale;
extern cvar_t	*r_lightDetailLevel;
extern cvar_t	*r_shaderPrograms;
extern cvar_t	*r_roundTexturesDown;
extern cvar_t	*r_downSizeTextures;
extern cvar_t	*r_downSizeNormalTextures;
extern cvar_t	*r_maxTextureSize;
extern cvar_t	*r_maxNormalTextureSize;
extern cvar_t	*r_compressTextures;
extern cvar_t	*r_compressNormalTextures;
extern cvar_t	*r_textureFilter;
extern cvar_t	*r_textureAnisotropy;
extern cvar_t	*r_textureLodBias;

qboolean	R_CullBox (const vec3_t mins, const vec3_t maxs, int clipFlags);
qboolean	R_CullSphere (const vec3_t center, float radius, int clipFlags);

void		R_AddSubviewSurface (material_t *material, renderEntity_t *entity, surface_t *surface);

void		R_RenderView (void);

void		R_AddWorld (void);
void		R_AddDecorations (void);
void		R_AddBrushModel (renderEntity_t *entity);
void		R_AddAliasModel (renderEntity_t *entity);
void		R_AddSprite (renderEntity_t *entity);
void		R_AddBeam (renderEntity_t *entity);
void		R_AddEntities (void);
void		R_AddDecals (void);
void		R_AddPolys (void);
void		R_AddLights (void);

qboolean	R_GetModeInfo (int mode, int *width, int *height);

/*
 =======================================================================

 BACK-END

 =======================================================================
*/

#define MAX_RENDER_COMMANDS				0x100000

#define MAX_INDICES						8192 * 3
#define MAX_VERTICES					4096

#define MAX_SHADOW_INDICES				MAX_INDICES * 8
#define MAX_SHADOW_VERTICES				MAX_VERTICES * 2

#define MAX_VERTEX_BUFFERS				1024

#define BUFFER_OFFSET_XYZ				0
#define BUFFER_OFFSET_NORMAL			12
#define BUFFER_OFFSET_TANGENT0			24
#define BUFFER_OFFSET_TANGENT1			36
#define BUFFER_OFFSET_ST				48
#define BUFFER_OFFSET_COLOR				56

#define BUFFER_OFFSET_XYZW				0

typedef enum {
	PATH_ARB,
	PATH_NV10,
	PATH_NV20,
	PATH_NV30,
	PATH_R200,
	PATH_ARB2
} renderPath_t;

typedef enum {
	RC_RENDER_VIEW,
	RC_CAPTURE_RENDER,
	RC_UPDATE_TEXTURE,
	RC_STRETCH_PIC,
	RC_SHEARED_PIC,
	RC_RENDER_SIZE,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_END_OF_LIST
} renderCommand_t;

typedef struct {
	byte					data[MAX_RENDER_COMMANDS];
	int						used;
} renderCommandList_t;

typedef struct {
	int						x;
	int						y;
	int						width;
	int						height;
} viewport_t;

typedef struct {
	qboolean				primaryView;

	subview_t				subview;

	vec3_t					origin;
	vec3_t					axis[3];

	mat4_t					perspectiveMatrix;
	mat4_t					worldMatrix;
	mat4_t					skyBoxMatrix;
	mat4_t					mirrorMatrix;
} viewParms_t;

typedef struct {
	vec3_t					viewOrigin;
	vec3_t					lightOrigin;

	mat4_t					viewMatrix;
	mat4_t					lightMatrix;
} localParms_t;

typedef struct {
	vec3_t					xyz;
	vec3_t					normal;
	vec3_t					tangents[2];
	vec2_t					st;
	color_t					color;
} vertexArray_t;

typedef struct {
	vec4_t					xyzw;
} shadowVertexArray_t;

typedef struct {
	char					*pointer;
	int						size;
	unsigned				usage;
	unsigned				bufNum;
} vertexBuffer_t;

typedef struct {
	int						commandId;

	float					time;
	float					materialParms[MAX_GLOBAL_MATERIAL_PARMS];

	viewport_t				viewport;
	viewParms_t				viewParms;

	mesh_t					*meshes;
	int						numMeshes;

	mesh_t					*postProcessMeshes;
	int						numPostProcessMeshes;

	light_t					*lights;
	int						numLights;

	light_t					*fogLights;
	int						numFogLights;
} renderViewCommand_t;

typedef struct {
	int						commandId;

	texture_t				*texture;
} captureRenderCommand_t;

typedef struct {
	int						commandId;

	texture_t				*texture;

	const byte				*image;
	int						width;
	int						height;
} updateTextureCommand_t;

typedef struct {
	int						commandId;

	float					x;
	float					y;
	float					w;
	float					h;
	float					s1;
	float					t1;
	float					s2;
	float					t2;

	color_t					color;

	material_t				*material;
} stretchPicCommand_t;

typedef struct {
	int						commandId;

	float					x;
	float					y;
	float					w;
	float					h;
	float					s1;
	float					t1;
	float					s2;
	float					t2;
	float					shearX;
	float					shearY;

	color_t					color;

	material_t				*material;
} shearedPicCommand_t;

typedef struct {
	int						commandId;

	int						width;
	int						height;
} renderSizeCommand_t;

typedef struct {
	int						commandId;

	int						width;
	int						height;
} drawBufferCommand_t;

typedef struct {
	int						commandId;
} swapBuffersCommand_t;

typedef struct {
	renderPath_t			activePath;

	renderCommandList_t		commandList;

	void					(*renderBatch)(void);
	void					(*renderInteraction)(stage_t *, stage_t *, stage_t *);

	// General state
	qboolean				overlay;
	float					time;
	float					materialParms[MAX_GLOBAL_MATERIAL_PARMS];

	int						renderWidth;
	int						renderHeight;

	// Viewport definition
	viewport_t				viewport;

	// View parms
	viewParms_t				viewParms;

	// Local parms
	localParms_t			localParms;

	// Light state
	light_t					*light;
	material_t				*lightMaterial;
	stage_t					*lightStage;

	// Batch state
	material_t				*material;
	renderEntity_t			*entity;

	qboolean				stencilShadow;
	qboolean				shadowCaps;

	// Vertex arrays
	int						numIndices;
	int						numVertices;

	index_t					indices[MAX_INDICES];
	vertexArray_t			vertices[MAX_VERTICES];

	index_t					shadowIndices[MAX_SHADOW_INDICES];
	shadowVertexArray_t		shadowVertices[MAX_SHADOW_VERTICES];

	// Vertex buffers
	vertexBuffer_t			*vertexBuffer;
	vertexBuffer_t			*shadowVertexBuffer;

	vertexBuffer_t			*lightVectorBuffer;
	vertexBuffer_t			*halfAngleVectorBuffer;
} backEndState_t;

extern backEndState_t		backEnd;

// Geometry batching
void		RB_CheckOverflow (int numIndices, int numVertices, int maxIndices, int maxVertices);

void		RB_BeginBatch (material_t *material, renderEntity_t *entity, qboolean stencilShadow, qboolean shadowCaps);
void		RB_EndBatch (void);

void		RB_BatchMesh (meshType_t type, meshData_t *data);
void		RB_BatchMeshShadow (meshType_t type, meshData_t *data);

// Generic rendering
void		RB_FillDepthBuffer (mesh_t *meshes, int numMeshes);
void		RB_RenderMaterialPasses (mesh_t *meshes, int numMeshes, qboolean postProcess);
void		RB_RenderLights (light_t *lights, int numLights);
void		RB_FogAllLights (light_t *lights, int numLights);
void		RB_RenderOverlayMaterial (void);

// Interaction rendering
void		RB_ARB_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage);

void		RB_NV10_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage);
void		RB_NV10_InitRenderer (void);

void		RB_NV20_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage);
void		RB_NV20_InitRenderer (void);

void		RB_R200_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage);
void		RB_R200_InitRenderer (void);

void		RB_ARB2_RenderInteraction (stage_t *bumpStage, stage_t *diffuseStage, stage_t *specularStage);
void		RB_ARB2_InitRenderer (void);

// Debug tools rendering
void		RB_RenderDebugTools (const renderViewCommand_t *cmd);

// Vertex buffers
void		RB_UpdateVertexBuffer (vertexBuffer_t *vertexBuffer, const void *data, int size);

// Material setup
void		RB_Deform (void);

void		RB_MultiplyTextureMatrix (material_t *material, textureStage_t *textureStage, mat4_t matrix);

void		RB_SetupTextureStage (textureStage_t *textureStage);
void		RB_CleanupTextureStage (textureStage_t *textureStage);
void		RB_SetupColorStage (colorStage_t *colorStage);
void		RB_CleanupColorStage (colorStage_t *colorStage);
void		RB_SetupProgramStage (programStage_t *programStage);
void		RB_CleanupProgramStage (programStage_t *programStage);

void		RB_SetMaterialState (void);
void		RB_SetStageState (stage_t *stage);

void		RB_CaptureCurrentRender (void);

// Misc rendering utilities
void		RB_SetLightState (renderEntity_t *entity, light_t *light, material_t *lightMaterial, stage_t *lightStage);

void		RB_SetEntityState (renderEntity_t *entity);

void		RB_DrawElements (void);

// Main back-end interface.
// These should be the only functions ever called by the front-end.
void		RB_ClearRenderCommands (void);
void		RB_IssueRenderCommands (void);
void		*RB_GetCommandBuffer (int size);

void		RB_SetActiveRenderer (void);

void		RB_InitBackEnd (void);
void		RB_ShutdownBackEnd (void);

/*
 =======================================================================

 INTEGRATED LIGHT EDITOR FUNCTIONS

 =======================================================================
*/

void		R_AddEditorLights (void);
void		R_DrawEditorLights (void);

void		R_InitLightEditor (void);
void		R_ShutdownLightEditor (void);

/*
 =======================================================================

 IMPLEMENTATION SPECIFIC FUNCTIONS

 =======================================================================
*/

extern glConfig_t			glConfig;

#ifdef WIN32

#define GL_DRIVER_OPENGL				"OpenGL32"

#define GLimp_SetDeviceGammaRamp		GLW_SetDeviceGammaRamp
#define GLimp_ActivateContext			GLW_ActivateContext
#define	GLimp_SwapBuffers				GLW_SwapBuffers
#define GLimp_Init						GLW_Init
#define GLimp_Shutdown					GLW_Shutdown

#else

#error "GLimp_* not available for this platform"

#endif


#endif	// __R_LOCAL_H__
