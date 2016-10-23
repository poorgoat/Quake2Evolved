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


// qfiles.h -- Quake file formats

// This file must be identical in the Quake and utils directories
// ZIP and JPEG files are not included here


#ifndef __QFILES_H__
#define __QFILES_H__


/*
 =======================================================================

 .PAK file format

 =======================================================================
*/

#define PAK_IDENT			(('K'<<24)+('C'<<16)+('A'<<8)+'P')

typedef struct {
	char	name[56];
	int		filePos;
	int		fileLen;
} pakFile_t;

typedef struct {
	int		ident;
	int		dirOfs;
	int		dirLen;
} pakHeader_t;

/*
 =======================================================================

 .PCX file format

 =======================================================================
*/

typedef struct {
    char			manufacturer;
    char			version;
    char			encoding;
    char			bitsPerPixel;
    unsigned short	xMin;
	unsigned short	yMin;
	unsigned short	xMax;
	unsigned short	yMax;
    unsigned short	hRes;
	unsigned short	vRes;
    unsigned char	palette[48];
    char			reserved;
    char			colorPlanes;
    unsigned short	bytesPerLine;
    unsigned short	paletteType;
    char			filler[58];
    unsigned char	data;			// Unbound
} pcxHeader_t;

/*
 =======================================================================

 .TGA file format

 =======================================================================
*/

typedef struct {
	unsigned char 	idLength;
	unsigned char 	colormapType;
	unsigned char 	imageType;
	unsigned short	colormapIndex;
	unsigned short	colormapLength;
	unsigned char	colormapSize;
	unsigned short	xOrigin;
	unsigned short	yOrigin;
	unsigned short	width;
	unsigned short	height;
	unsigned char	pixelSize;
	unsigned char	attributes;
} targaHeader_t;

/*
 =======================================================================

 .WAL texture file format

 =======================================================================
*/

#define	MIPLEVELS	4

typedef struct {
	char			name[32];
	unsigned		width;
	unsigned		height;
	unsigned		offsets[MIPLEVELS];	// Four mipmaps stored
	char			animName[32];		// Next frame in animation chain
	int				flags;
	int				contents;
	int				value;
} mipTex_t;

/*
 =======================================================================

 .SP2 sprite file format

 =======================================================================
*/

#define SP2_IDENT			(('2'<<24)+('S'<<16)+('D'<<8)+'I')
#define SP2_VERSION			2

#define SP2_MAX_FRAMES		32

typedef struct {
	int			width;
	int			height;
	int			originX;			// Raster coordinates inside pic
	int			originY;			// Raster coordinates inside pic
	char		name[MAX_QPATH];	// Name of PCX file
} sp2Frame_t;

typedef struct {
	int			ident;
	int			version;
	int			numFrames;
	sp2Frame_t	frames[1];			// Variable sized
} sp2Header_t;

/*
 =======================================================================

 .MD2 triangle model file format

 =======================================================================
*/

#define MD2_IDENT			(('2'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD2_VERSION			8

#define	MD2_MAX_TRIANGLES	4096
#define MD2_MAX_VERTS		2048
#define MD2_MAX_SKINS		32
#define MD2_MAX_FRAMES		512

typedef struct {
	short		s;
	short		t;
} md2St_t;

typedef struct {
	short		indexXyz[3];
	short		indexSt[3];
} md2Triangle_t;

typedef struct {
	byte		v[3];				// Scaled byte to fit in frame mins/maxs
	byte		lightNormalIndex;
} md2Vertex_t;

typedef struct {
	vec3_t		scale;				// Multiply byte verts by this
	vec3_t		translate;			// Then add this
	char		name[16];			// Frame name from grabbing
	md2Vertex_t	verts[1];			// Variable sized
} md2Frame_t;

typedef struct {
	int			ident;
	int			version;

	int			skinWidth;
	int			skinHeight;
	int			frameSize;			// Byte size of each frame

	int			numSkins;
	int			numXyz;
	int			numSt;				// Greater than numXyz for seams
	int			numTris;
	int			numGLCmds;			// dwords in strip/fan command list
	int			numFrames;

	int			ofsSkins;			// Each skin is a MAX_QPATH string
	int			ofsSt;				// Byte offset from start for stverts
	int			ofsTris;			// Offset for triangles
	int			ofsFrames;			// Offset for first frame
	int			ofsGLCmds;	
	int			ofsEnd;				// End of file
} md2Header_t;

/*
 =======================================================================

 .MD3 triangle model file format

 =======================================================================
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

#define MD3_MAX_TRIANGLES	8192		// Per surface
#define MD3_MAX_VERTS		4096		// Per surface
#define MD3_MAX_SHADERS		256			// Per surface
#define MD3_MAX_TAGS		16			// Per frame
#define MD3_MAX_FRAMES		1024		// Per model
#define MD3_MAX_SURFACES	32			// Per model

#define MD3_MAX_LODS		4

#define MD3_XYZ_SCALE		(1.0/64)	// Vertex scales

typedef struct {
	vec3_t		bounds[2];
	vec3_t		localOrigin;
	float		radius;
	char		name[16];
} md3Frame_t;

typedef struct {
	char		name[MAX_QPATH];
	vec3_t		origin;
	vec3_t		axis[3];
} md3Tag_t;

typedef struct {
	int			ident;

	char		name[MAX_QPATH];
	int			flags;

	int			numFrames;			// All surfaces in a model should have the same
	int			numShaders;			// All surfaces in a model should have the same
	int			numVerts;
	int			numTriangles;

	int			ofsTriangles;
	int			ofsShaders;			// Offset from start of md3Surface_t
	int			ofsSt;				// Texture coords are common for all frames
	int			ofsXyzNormals;		// numVerts * numFrames
	int			ofsEnd;				// Next surface follows
} md3Surface_t;

typedef struct {
	char		name[MAX_QPATH];
	int			shaderIndex;		// For in-game use
} md3Shader_t;

typedef struct {
	int			indexes[3];
} md3Triangle_t;

typedef struct {
	vec2_t		st;
} md3St_t;

typedef struct {
	short		xyz[3];
	short		normal;
} md3XyzNormal_t;

typedef struct {
	int			ident;
	int			version;

	char		name[MAX_QPATH];	// Model name
	int			flags;

	int			numFrames;
	int			numTags;			
	int			numSurfaces;
	int			numSkins;

	int			ofsFrames;			// Offset for first frame
	int			ofsTags;			// numTags * numFrames
	int			ofsSurfaces;		// First surface, others follow
	int			ofsEnd;				// End of file
} md3Header_t;

/*
 =======================================================================

 .BSP file format

 =======================================================================
*/

#define BSP_IDENT			(('P'<<24)+('S'<<16)+('B'<<8)+'I')
#define BSP_VERSION			38

#define	MAX_STYLES			4

#define	LIGHTMAP_WIDTH		128
#define	LIGHTMAP_HEIGHT		128

#define	VIS_PVS				0
#define	VIS_PHS				1

// Upper design bounds
#define	MAX_MAP_ENTITIES	0x000800
#define	MAX_MAP_ENTSTRING	0x040000
#define	MAX_MAP_PLANES		0x010000
#define	MAX_MAP_VERTEXES	0x010000
#define	MAX_MAP_VISIBILITY	0x100000
#define	MAX_MAP_NODES		0x010000
#define	MAX_MAP_TEXINFO		0x002000
#define	MAX_MAP_FACES		0x010000
#define	MAX_MAP_LIGHTING	0x200000
#define	MAX_MAP_LEAFS		0x010000
#define	MAX_MAP_LEAFFACES	0x010000
#define	MAX_MAP_LEAFBRUSHES 0x010000
#define	MAX_MAP_EDGES		0x01F400
#define	MAX_MAP_SURFEDGES	0x03E800
#define	MAX_MAP_MODELS		0x000400
#define	MAX_MAP_BRUSHES		0x002000
#define	MAX_MAP_BRUSHSIDES	0x010000
#define	MAX_MAP_AREAS		0x000100
#define	MAX_MAP_AREAPORTALS	0x000400
#define	MAX_MAP_PORTALS		0x010000

#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_VERTEXES		2
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES			4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES			6
#define	LUMP_LIGHTING		7
#define	LUMP_LEAFS			8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES			11
#define	LUMP_SURFEDGES		12
#define	LUMP_MODELS			13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_POP			16
#define	LUMP_AREAS			17
#define	LUMP_AREAPORTALS	18
#define	HEADER_LUMPS		19

typedef struct {
	int				fileOfs;
	int				fileLen;
} lump_t;

typedef struct {
	int				ident;
	int				version;	
	lump_t			lumps[HEADER_LUMPS];
} dheader_t;

typedef struct {
	vec3_t			mins;
	vec3_t			maxs;
	vec3_t			origin;					// For sounds or lights
	int				headNode;
	int				firstFace;				// Submodels just draw faces
	int				numFaces;				// without walking the BSP tree
} dmodel_t;

typedef struct {
	vec3_t			point;
} dvertex_t;

typedef struct {
	vec3_t			normal;
	float			dist;
	int				type;
} dplane_t;

typedef struct {
	int				planeNum;
	int				children[2];			// Negative numbers are -(leafs+1), not nodes
	
	short			mins[3];				// For frustum culling
	short			maxs[3];

	unsigned short	firstFace;
	unsigned short	numFaces;				// Counting both sides
} dnode_t;

typedef struct {
	float			vecs[2][4];				// [st][xyz offset]
	int				flags;					// Miptex flags + overrides
	int				value;					// Light emission, etc...
	char			texture[32];			// Texture name (textures/*.wal)
	int				nextTexInfo;			// For animations, -1 = end of chain
} dtexinfo_t;

// Note that edge 0 is never used, because negative edge nums are used 
// for counterclockwise use of the edge in a face
typedef struct {
	unsigned short	v[2];					// Vertex numbers
} dedge_t;

typedef struct {
	unsigned short	planeNum;
	short			side;

	int				firstEdge;				// We must support > 64k edges
	short			numEdges;	
	short			texInfo;

	// Lighting info
	byte			styles[MAX_STYLES];
	int				lightOfs;				// Start of samples
} dface_t;

typedef struct {
	int				contents;				// OR of all brushes (not needed?)

	short			cluster;
	short			area;

	short			mins[3];				// For frustum culling
	short			maxs[3];

	unsigned short	firstLeafFace;
	unsigned short	numLeafFaces;

	unsigned short	firstLeafBrush;
	unsigned short	numLeafBrushes;
} dleaf_t;

typedef struct {
	unsigned short	planeNum;				// Facing out of the leaf
	short			texInfo;
} dbrushside_t;

typedef struct {
	int				firstSide;
	int				numSides;
	int				contents;
} dbrush_t;

// The visibility lump consists of a header with a count, then byte
// offsets for the PVS and PHS of each cluster, then the raw compressed
// bit vectors
typedef struct {
	int				numClusters;
	int				bitOfs[8][2];
} dvis_t;

// Each area has a list of portals that lead into other areas when 
// portals are closed, other areas may not be visible or hearable even 
// if the vis info says that it should be
typedef struct {
	int				portalNum;
	int				otherArea;
} dareaportal_t;

typedef struct {
	int				numAreaPortals;
	int				firstAreaPortal;
} darea_t;

/*
 =======================================================================

 .lightgrid file format

 =======================================================================
*/

#define Q2EL_IDENT			(('L'<<24)+('E'<<16)+('2'<<8)+'Q')
#define Q2EL_VERSION		1

typedef struct {
	int				ident;
	int				version;
	vec3_t			gridMins;
	vec3_t			gridSize;
	int				gridBounds[4];
	int				gridPoints;
} lightGridHeader_t;


#endif	// __QFILES_H__
