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


// cmodel.c -- model collision


#include "qcommon.h"


typedef struct {					// Used internally due to name len probs
	char			name[32];
	csurface_t		c;
} cmapsurface_t;

typedef struct {
	int				numClusters;
	int				bitOfs[8][2];
} cvis_t;

typedef struct {
	int				contents;
	int				cluster;
	int				area;
	unsigned short	firstLeafBrush;
	unsigned short	numLeafBrushes;
} cleaf_t;

typedef struct {
	int				contents;
	int				numSides;
	int				firstBrushSide;
	int				checkCount;		// To avoid repeated testings
} cbrush_t;

typedef struct {
	cplane_t		*plane;
	cmapsurface_t	*surface;
} cbrushside_t;

typedef struct {
	cplane_t		*plane;
	int				children[2];	// Negative numbers are leafs
} cnode_t;

typedef struct {
	int				numAreaPortals;
	int				firstAreaPortal;
	int				floodNum;		// If two areas have equal floodNums, they are connected
	int				floodValid;
} carea_t;

typedef struct {
	int				portalNum;
	int				otherArea;
} careaportal_t;

typedef struct {
	qboolean		loaded;
	char			name[MAX_OSPATH];
	unsigned		checksum;

	int				numPlanes;
	cplane_t		*planes;

	int				numSurfaces;
	cmapsurface_t	*surfaces;

	int				numVisibility;
	cvis_t			*visibility;

	int				numLeafs;
	int				numClusters;
	cleaf_t			*leafs;

	int				numLeafBrushes;
	unsigned short	*leafBrushes;

	int				numBrushes;
	cbrush_t		*brushes;

	int				numBrushSides;
	cbrushside_t	*brushSides;

	int				numNodes;
	cnode_t			*nodes;

	int				numModels;
	cmodel_t		*models;

	int				numAreas;
	carea_t			*areas;

	int				numAreaPortals;
	careaportal_t	*areaPortals;

	int				numEntityChars;
	char			*entityString;
} cm_t;

static cm_t				cm;

static cmapsurface_t	cm_nullSurface;
static cmodel_t			cm_nullModel;

// For statistics
static int				cm_traces;
static int				cm_pointContents;

cvar_t					*cm_noAreas;
cvar_t					*cm_showTrace;

static void	CM_InitBoxHull (void);
static void	CM_FloodAreaConnections (qboolean clear);


/*
 =======================================================================

 MAP LOADING

 =======================================================================
*/


/*
 =================
 CM_LoadPlanes
 =================
*/
static void CM_LoadPlanes (const byte *data, const lump_t *l){

	dplane_t	*in;
	cplane_t	*out;
	int			i;

	in = (dplane_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dplane_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size in '%s'", cm.name);

	cm.numPlanes = l->fileLen / sizeof(dplane_t);
	if (cm.numPlanes < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has no planes", cm.name);
	if (cm.numPlanes > MAX_MAP_PLANES)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too many planes", cm.name);

	// Extra for box hull
	out = cm.planes = Z_Malloc((cm.numPlanes + 12) * sizeof(cplane_t));

	for (i = 0; i < cm.numPlanes; i++, in++, out++){
		out->normal[0] = LittleFloat(in->normal[0]);
		out->normal[1] = LittleFloat(in->normal[1]);
		out->normal[2] = LittleFloat(in->normal[2]);

		out->dist = LittleFloat(in->dist);
		out->type = PlaneTypeForNormal(out->normal);
		SetPlaneSignbits(out);
	}
}

/*
 =================
 CM_LoadSurfaces
 =================
*/
static void CM_LoadSurfaces (const byte *data, const lump_t *l){

	dtexinfo_t		*in;
	cmapsurface_t	*out;
	int				i;

	in = (dtexinfo_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dtexinfo_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size in '%s'", cm.name);

	cm.numSurfaces = l->fileLen / sizeof(dtexinfo_t);
	if (cm.numSurfaces < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has no surfaces", cm.name);
	if (cm.numSurfaces > MAX_MAP_TEXINFO)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too many surfaces", cm.name);

	out = cm.surfaces = Z_Malloc(cm.numSurfaces * sizeof(cmapsurface_t));

	for (i = 0; i < cm.numSurfaces; i++, in++, out++){
		Q_strncpyz(out->name, in->texture, sizeof(out->name));
		Q_strncpyz(out->c.name, in->texture, sizeof(out->c.name));
		out->c.flags = LittleLong(in->flags);
		out->c.value = LittleLong(in->value);
	}
}

/*
 =================
 CM_LoadVisibility
 =================
*/
static void CM_LoadVisibility (const byte *data, const lump_t *l){

	int		i;
	
	cm.numVisibility = l->fileLen;
	if (cm.numVisibility < 1)
		return;
	if (cm.numVisibility > MAX_MAP_VISIBILITY)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too large visibility lump", cm.name);

	cm.visibility = Z_Malloc(cm.numVisibility);
	memcpy(cm.visibility, data + l->fileOfs, cm.numVisibility);

	cm.visibility->numClusters = LittleLong(cm.visibility->numClusters);
	for (i = 0; i < cm.visibility->numClusters; i++){
		cm.visibility->bitOfs[i][0] = LittleLong(cm.visibility->bitOfs[i][0]);
		cm.visibility->bitOfs[i][1] = LittleLong(cm.visibility->bitOfs[i][1]);
	}
}

/*
 =================
 CM_LoadLeafs
 =================
*/
static void CM_LoadLeafs (const byte *data, const lump_t *l){

	dleaf_t	*in;
	cleaf_t	*out;
	int		i;
	
	in = (dleaf_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dleaf_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size in '%s'", cm.name);
	
	cm.numLeafs = l->fileLen / sizeof(dleaf_t);
	if (cm.numLeafs < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has no leafs", cm.name);
	if (cm.numLeafs > MAX_MAP_LEAFS)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too many leafs", cm.name);

	// Extra for box hull
	out = cm.leafs = Z_Malloc((cm.numLeafs + 1) * sizeof(cleaf_t));

	for (i = 0; i < cm.numLeafs; i++, in++, out++){
		out->contents = LittleLong(in->contents);
		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);
		out->firstLeafBrush = LittleShort(in->firstLeafBrush);
		out->numLeafBrushes = LittleShort(in->numLeafBrushes);

		if (out->cluster >= cm.numClusters)
			cm.numClusters = out->cluster + 1;
	}

	if (cm.leafs[0].contents != CONTENTS_SOLID)
		Com_Error(ERR_DROP, "CM_LoadMap: leaf 0 is not CONTENTS_SOLID in '%s'", cm.name);
}

/*
 =================
 CM_LoadLeafBrushes
 =================
*/
static void CM_LoadLeafBrushes (const byte *data, const lump_t *l){

	unsigned short 	*in;
	unsigned short	*out;
	int				i;
	
	in = (unsigned short *)(data + l->fileOfs);
	if (l->fileLen % sizeof(unsigned short))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size in '%s'", cm.name);

	cm.numLeafBrushes = l->fileLen / sizeof(unsigned short);
	if (cm.numLeafBrushes < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has no leafBrushes", cm.name);
	if (cm.numLeafBrushes > MAX_MAP_LEAFBRUSHES)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too many leafBrushes", cm.name);

	// Extra for box hull
	out = cm.leafBrushes = Z_Malloc((cm.numLeafBrushes + 1) * sizeof(unsigned short));

	for (i = 0; i < cm.numLeafBrushes; i++, in++, out++)
		*out = LittleShort(*in);
}

/*
 =================
 CM_LoadBrushes
 =================
*/
static void CM_LoadBrushes (const byte *data, const lump_t *l){

	dbrush_t	*in;
	cbrush_t	*out;
	int			i;
	
	in = (dbrush_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dbrush_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size in '%s'", cm.name);
	
	cm.numBrushes = l->fileLen / sizeof(dbrush_t);
	if (cm.numBrushes < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has no brushes", cm.name);
	if (cm.numBrushes > MAX_MAP_BRUSHES)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too many brushes", cm.name);

	// Extra for box hull
	out = cm.brushes = Z_Malloc((cm.numBrushes + 1) * sizeof(cbrush_t));

	for (i = 0; i < cm.numBrushes; i++, out++, in++){
		out->contents = LittleLong(in->contents);
		out->numSides = LittleLong(in->numSides);
		out->firstBrushSide = LittleLong(in->firstSide);
		out->checkCount = 0;
	}
}

/*
 =================
 CM_LoadBrushSides
 =================
*/
static void CM_LoadBrushSides (const byte *data, const lump_t *l){

	dbrushside_t	*in;
	cbrushside_t	*out;
	int				i;

	in = (dbrushside_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dbrushside_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size in '%s'", cm.name);

	cm.numBrushSides = l->fileLen / sizeof(dbrushside_t);
	if (cm.numBrushSides < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has no brushSides", cm.name);
	if (cm.numBrushSides > MAX_MAP_BRUSHSIDES)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too many brushSides", cm.name);

	// Extra for box hull
	out = cm.brushSides = Z_Malloc((cm.numBrushSides + 6) * sizeof(cbrushside_t));

	for (i = 0; i < cm.numBrushSides; i++, in++, out++){
		out->plane = cm.planes + LittleShort(in->planeNum);
		out->surface = cm.surfaces + LittleShort(in->texInfo);
	}
}

/*
 =================
 CM_LoadNodes
 =================
*/
static void CM_LoadNodes (const byte *data, const lump_t *l){

	dnode_t	*in;
	cnode_t	*out;
	int			i;
	
	in = (dnode_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dnode_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size in '%s'", cm.name);
	
	cm.numNodes = l->fileLen / sizeof(dnode_t);
	if (cm.numNodes < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has no nodes", cm.name);
	if (cm.numNodes > MAX_MAP_NODES)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too many nodes", cm.name);

	// Extra for box hull
	out = cm.nodes = Z_Malloc((cm.numNodes + 6) * sizeof(cnode_t));

	for (i = 0; i < cm.numNodes; i++, out++, in++){
		out->plane = cm.planes + LittleLong(in->planeNum);

		out->children[0] = LittleLong(in->children[0]);
		out->children[1] = LittleLong(in->children[1]);
	}
}

/*
 =================
 CM_LoadSubmodels
 =================
*/
static void CM_LoadSubmodels (const byte *data, const lump_t *l){

	dmodel_t	*in;
	cmodel_t	*out;
	int			i, j;

	in = (dmodel_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dmodel_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size in '%s'", cm.name);

	cm.numModels = l->fileLen / sizeof(dmodel_t);
	if (cm.numModels < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has no models", cm.name);
	if (cm.numModels > MAX_MAP_MODELS)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too many models", cm.name);

	out = cm.models = Z_Malloc(cm.numModels * sizeof(cmodel_t));

	for (i = 0; i < cm.numModels; i++, in++, out++){
		for (j = 0; j < 3; j++){
			out->origin[j] = LittleFloat(in->origin[j]);

			// Spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1.0;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1.0;
		}

		out->headNode = LittleLong(in->headNode);
	}
}

/*
 =================
 CM_LoadAreas
 =================
*/
static void CM_LoadAreas (const byte *data, const lump_t *l){

	darea_t	*in;
	carea_t	*out;
	int		i;

	in = (darea_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(darea_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size in '%s'", cm.name);

	cm.numAreas = l->fileLen / sizeof(darea_t);
	if (cm.numAreas < 1)
		return;
	if (cm.numAreas > MAX_MAP_AREAS)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too many areas", cm.name);

	out = cm.areas = Z_Malloc(cm.numAreas * sizeof(carea_t));

	for (i = 0; i < cm.numAreas; i++, in++, out++){
		out->numAreaPortals = LittleLong(in->numAreaPortals);
		out->firstAreaPortal = LittleLong(in->firstAreaPortal);
		out->floodNum = 0;
		out->floodValid = 0;
	}
}

/*
 =================
 CM_LoadAreaPortals
 =================
*/
static void CM_LoadAreaPortals (const byte *data, const lump_t *l){

	dareaportal_t	*in;
	careaportal_t	*out;
	int				i;

	in = (dareaportal_t *)(data + l->fileOfs);
	if (l->fileLen % sizeof(dareaportal_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size in '%s'", cm.name);

	cm.numAreaPortals = l->fileLen / sizeof(dareaportal_t);
	if (cm.numAreaPortals < 1)
		return;
	if (cm.numAreaPortals > MAX_MAP_AREAPORTALS)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too many areaPortals", cm.name);

	out = cm.areaPortals = Z_Malloc(cm.numAreaPortals * sizeof(careaportal_t));

	for (i = 0; i < cm.numAreaPortals; i++, in++, out++){
		out->portalNum = LittleLong(in->portalNum);
		out->otherArea = LittleLong(in->otherArea);
	}
}

/*
 =================
 CM_LoadEntityString
 =================
*/
static void CM_LoadEntityString (const byte *data, const lump_t *l){

	cm.numEntityChars = l->fileLen;
	if (cm.numEntityChars < 1)
		return;
	if (cm.numEntityChars > MAX_MAP_ENTSTRING)
		Com_Error(ERR_DROP, "CM_LoadMap: map '%s' has too large entity lump", cm.name);

	cm.entityString = Z_Malloc(cm.numEntityChars + 1);

	memcpy(cm.entityString, data + l->fileOfs, cm.numEntityChars);
	cm.entityString[cm.numEntityChars] = 0;
}

/*
 =================
 CM_LoadMap

 Loads in the map and all submodels
 =================
*/
cmodel_t *CM_LoadMap (const char *map, qboolean clientLoad, unsigned *checksum){

	int			i, length;
	byte		*data;
	dheader_t	*header;

	// Cinematic servers won't have anything at all
	if (!map){
		CM_UnloadMap();

		*checksum = 0;
		return &cm_nullModel;
	}

	Com_DPrintf("CM_LoadMap( %s, %i )\n", map, clientLoad);

	if (!Q_stricmp(cm.name, map) && clientLoad && Com_ServerState()){
		// Still have the right version
		*checksum = cm.checksum;
		return &cm.models[0];
	}

	// Free old stuff
	CM_UnloadMap();

	// Load the file
	length = FS_LoadFile(map, (void **)&data);
	if (!data)
		Com_Error(ERR_DROP, "CM_LoadMap: '%s' not found", map);

	// Fill it in
	cm.loaded = true;
	Q_strncpyz(cm.name, map, sizeof(cm.name));
	cm.checksum = LittleLong(Com_BlockChecksum(data, length));

	header = (dheader_t *)data;

	// Byte swap the header fields and sanity check
	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int *)header)[i] = LittleLong(((int *)header)[i]);

	if (header->ident != BSP_IDENT)
		Com_Error(ERR_DROP, "CM_LoadMap: '%s' has wrong file id", cm.name);

	if (header->version != BSP_VERSION)
		Com_Error(ERR_DROP, "CM_LoadMap: '%s' has wrong version number (%i should be %i)", cm.name, header->version, BSP_VERSION);

	// Load into heap
	CM_LoadPlanes(data, &header->lumps[LUMP_PLANES]);
	CM_LoadSurfaces(data, &header->lumps[LUMP_TEXINFO]);
	CM_LoadVisibility(data, &header->lumps[LUMP_VISIBILITY]);
	CM_LoadLeafs(data, &header->lumps[LUMP_LEAFS]);
	CM_LoadLeafBrushes(data, &header->lumps[LUMP_LEAFBRUSHES]);
	CM_LoadBrushes(data, &header->lumps[LUMP_BRUSHES]);
	CM_LoadBrushSides(data, &header->lumps[LUMP_BRUSHSIDES]);
	CM_LoadNodes(data, &header->lumps[LUMP_NODES]);
	CM_LoadSubmodels(data, &header->lumps[LUMP_MODELS]);
	CM_LoadAreas(data, &header->lumps[LUMP_AREAS]);
	CM_LoadAreaPortals(data, &header->lumps[LUMP_AREAPORTALS]);
	CM_LoadEntityString(data, &header->lumps[LUMP_ENTITIES]);

	FS_FreeFile(data);

	// Set up some needed things
	CM_InitBoxHull();
	CM_FloodAreaConnections(true);

	*checksum = cm.checksum;
	return &cm.models[0];
}

/*
 =================
 CM_UnloadMap

 Frees the current map
 =================
*/
void CM_UnloadMap (void){

	if (cm.planes)
		Z_Free(cm.planes);
	if (cm.surfaces)
		Z_Free(cm.surfaces);
	if (cm.visibility)
		Z_Free(cm.visibility);
	if (cm.leafs)
		Z_Free(cm.leafs);
	if (cm.leafBrushes)
		Z_Free(cm.leafBrushes);
	if (cm.brushes)
		Z_Free(cm.brushes);
	if (cm.brushSides)
		Z_Free(cm.brushSides);
	if (cm.nodes)
		Z_Free(cm.nodes);
	if (cm.models)
		Z_Free(cm.models);
	if (cm.areas)
		Z_Free(cm.areas);
	if (cm.areaPortals)
		Z_Free(cm.areaPortals);
	if (cm.entityString)
		Z_Free(cm.entityString);

	memset(&cm, 0, sizeof(cm_t));
}

/*
 =================
 CM_ClearStats
 =================
*/
void CM_ClearStats (void){

	cm_traces = 0;
	cm_pointContents = 0;
}

/*
 =================
 CM_PrintStats
 =================
*/
void CM_PrintStats (void){

	if (!cm_showTrace->integerValue)
		return;

	Com_Printf("%i traces, %i points\n", cm_traces, cm_pointContents);
}

/*
 =================
 CM_Init
 =================
*/
void CM_Init (void){

	cm_noAreas = Cvar_Get("cm_noAreas", "0", CVAR_CHEAT, "Don't consider area portals");
	cm_showTrace = Cvar_Get("cm_showTrace", "0", CVAR_CHEAT, "Report trace statistics");
}


// =====================================================================


/*
 =================
 CM_NumInlineModels
 =================
*/
int	CM_NumInlineModels (void){

	if (!cm.loaded)
		return 0;

	return cm.numModels;
}

/*
 =================
 CM_InlineModel
 =================
*/
cmodel_t *CM_InlineModel (const char *name){

	int		num;

	if (!cm.loaded)
		return &cm_nullModel;

	if (!name || name[0] != '*')
		Com_Error(ERR_DROP, "CM_InlineModel: bad name");

	num = atoi(name+1);
	if (num < 1 || num >= cm.numModels)
		Com_Error(ERR_DROP, "CM_InlineModel: bad number");

	return &cm.models[num];
}

/*
 =================
 CM_EntityString
 =================
*/
char *CM_EntityString (void){

	if (!cm.loaded)
		return "";

	if (!cm.numEntityChars)
		return "";

	return cm.entityString;
}

/*
 =================
 CM_NumClusters
 =================
*/
int	CM_NumClusters (void){

	if (!cm.loaded)
		return 0;

	return cm.numClusters;
}

/*
 =================
 CM_LeafContents
 =================
*/
int	CM_LeafContents (int leafNum){

	if (!cm.loaded || !cm.numLeafs)
		return 0;

	if (leafNum < 0 || leafNum >= cm.numLeafs)
		Com_Error(ERR_DROP, "CM_LeafContents: bad number");

	return cm.leafs[leafNum].contents;
}

/*
 =================
 CM_LeafCluster
 =================
*/
int	CM_LeafCluster (int leafNum){

	if (!cm.loaded || !cm.numLeafs)
		return 0;

	if (leafNum < 0 || leafNum >= cm.numLeafs)
		Com_Error(ERR_DROP, "CM_LeafCluster: bad number");

	return cm.leafs[leafNum].cluster;
}

/*
 =================
 CM_LeafArea
 =================
*/
int	CM_LeafArea (int leafNum){

	if (!cm.loaded || !cm.numLeafs)
		return 0;

	if (leafNum < 0 || leafNum >= cm.numLeafs)
		Com_Error(ERR_DROP, "CM_LeafArea: bad number");
	
	return cm.leafs[leafNum].area;
}


// =====================================================================

static cplane_t	*cm_boxPlanes;
static int		cm_boxHeadNode;
static cbrush_t	*cm_boxBrush;
static cleaf_t	*cm_boxLeaf;


/*
 =================
 CM_InitBoxHull

 Set up the planes and nodes so that the six floats of a bounding box
 can just be stored out and get a proper clipping hull structure.
 =================
*/
static void CM_InitBoxHull (void){

	int				i;
	int				side;
	cnode_t			*n;
	cplane_t		*p;
	cbrushside_t	*s;

	cm_boxPlanes = &cm.planes[cm.numPlanes];
	cm_boxHeadNode = cm.numNodes;

	cm_boxBrush = &cm.brushes[cm.numBrushes];
	cm_boxBrush->numSides = 6;
	cm_boxBrush->firstBrushSide = cm.numBrushSides;
	cm_boxBrush->contents = CONTENTS_MONSTER;

	cm_boxLeaf = &cm.leafs[cm.numLeafs];
	cm_boxLeaf->numLeafBrushes = 1;
	cm_boxLeaf->firstLeafBrush = cm.numLeafBrushes;
	cm_boxLeaf->contents = CONTENTS_MONSTER;

	cm.leafBrushes[cm.numLeafBrushes] = cm.numBrushes;

	for (i = 0; i < 6; i++){
		side = i & 1;

		// Brush sides
		s = &cm.brushSides[cm.numBrushSides+i];
		s->plane = &cm.planes[cm.numPlanes+i*2+side];
		s->surface = &cm_nullSurface;

		// Nodes
		n = &cm.nodes[cm.numNodes+i];
		n->plane = &cm.planes[cm.numPlanes+i*2];
		n->children[side] = -1 - cm.numLeafs;
		if (i != 5)
			n->children[side^1] = cm_boxHeadNode+i + 1;
		else
			n->children[side^1] = -1 - cm.numLeafs;

		// Planes
		p = &cm_boxPlanes[i*2+0];
		VectorClear(p->normal);
		p->normal[i>>1] = 1;
		p->type = i>>1;
		p->signbits = 0;

		p = &cm_boxPlanes[i*2+1];
		VectorClear(p->normal);
		p->normal[i>>1] = -1;
		p->type = 3;
		p->signbits = 0;
	}	
}

/*
 =================
 CM_HeadNodeForBox
 
 To keep everything totally uniform, bounding boxes are turned into 
 small BSP trees instead of being compared directly
 =================
*/
int	CM_HeadNodeForBox (const vec3_t mins, const vec3_t maxs){

	cm_boxPlanes[0].dist = maxs[0];
	cm_boxPlanes[1].dist = -maxs[0];
	cm_boxPlanes[2].dist = mins[0];
	cm_boxPlanes[3].dist = -mins[0];
	cm_boxPlanes[4].dist = maxs[1];
	cm_boxPlanes[5].dist = -maxs[1];
	cm_boxPlanes[6].dist = mins[1];
	cm_boxPlanes[7].dist = -mins[1];
	cm_boxPlanes[8].dist = maxs[2];
	cm_boxPlanes[9].dist = -maxs[2];
	cm_boxPlanes[10].dist = mins[2];
	cm_boxPlanes[11].dist = -mins[2];

	return cm_boxHeadNode;
}


// =====================================================================

static int		cm_leafCount, cm_leafMaxCount;
static int		*cm_leafList;
static vec3_t	cm_leafMins, cm_leafMaxs;
static int		cm_leafTopNode;


/*
 =================
 CM_RecursiveBoxLeafNums

 Fills in a list of all the leafs touched
 =================
*/
static void CM_RecursiveBoxLeafNums (int nodeNum){

	cnode_t		*node;
	cplane_t	*plane;
	int			side;

	while (1){
		if (nodeNum < 0){
			if (cm_leafCount >= cm_leafMaxCount)
				return;
			
			cm_leafList[cm_leafCount++] = -1 - nodeNum;
			return;
		}
	
		node = &cm.nodes[nodeNum];
		plane = node->plane;

		side = BoxOnPlaneSide(cm_leafMins, cm_leafMaxs, plane);

		if (side == SIDE_FRONT)
			nodeNum = node->children[0];
		else if (side == SIDE_BACK)
			nodeNum = node->children[1];
		else {
			// Go down both
			if (cm_leafTopNode == -1)
				cm_leafTopNode = nodeNum;
		
			CM_RecursiveBoxLeafNums(node->children[0]);
			nodeNum = node->children[1];
		}
	}
}

/*
 =================
 CM_BoxLeafNumsHeadNode
 =================
*/
static int CM_BoxLeafNumsHeadNode (const vec3_t mins, const vec3_t maxs, int *list, int listSize, int headNode, int *topNode){

	cm_leafList = list;
	cm_leafCount = 0;
	cm_leafMaxCount = listSize;
	VectorCopy(mins, cm_leafMins);
	VectorCopy(maxs, cm_leafMaxs);
	cm_leafTopNode = -1;

	CM_RecursiveBoxLeafNums(headNode);

	if (topNode)
		*topNode = cm_leafTopNode;

	return cm_leafCount;
}

/*
 =================
 CM_BoxLeafNums
 =================
*/
int	CM_BoxLeafNums (const vec3_t mins, const vec3_t maxs, int *list, int listSize, int *topNode){

	if (!cm.loaded)
		return 0;

	return CM_BoxLeafNumsHeadNode(mins, maxs, list, listSize, cm.models[0].headNode, topNode);
}

/*
 =================
 CM_RecursivePointLeafNum
 =================
*/
static int CM_RecursivePointLeafNum (const vec3_t p, int nodeNum){

	cnode_t		*node;
	int			side;

	cm_pointContents++;		// Optimize counter

	while (nodeNum >= 0){
		node = &cm.nodes[nodeNum];

		side = PointOnPlaneSide(p, 0.0, node->plane);

		if (side == SIDE_BACK)
			nodeNum = node->children[1];
		else
			nodeNum = node->children[0];
	}

	return -1 - nodeNum;
}

/*
 =================
 CM_PointLeafNum
 =================
*/
int CM_PointLeafNum (const vec3_t p){

	if (!cm.loaded)
		return 0;
	
	return CM_RecursivePointLeafNum(p, 0);
}

/*
 =================
 CM_PointContents
 =================
*/
int CM_PointContents (const vec3_t p, int headNode){

	int		l;

	if (!cm.loaded)
		return 0;

	l = CM_RecursivePointLeafNum(p, headNode);

	return cm.leafs[l].contents;
}

/*
 =================
 CM_TransformedPointContents

 Handles offseting and rotation of the point for moving and rotating
 entities
 =================
*/
int	CM_TransformedPointContents (const vec3_t p, int headNode, const vec3_t origin, const vec3_t angles){

	vec3_t	p2, temp;
	vec3_t	axis[3];
	int		l;

	if (!cm.loaded)
		return 0;

	if (headNode != cm_boxHeadNode && !VectorCompare(angles, vec3_origin)){
		AnglesToAxis(angles, axis);

		VectorSubtract(p, origin, temp);
		VectorRotate(temp, axis, p2);
	}
	else
		VectorSubtract(p, origin, p2);

	l = CM_RecursivePointLeafNum(p2, headNode);

	return cm.leafs[l].contents;
}


/*
 =======================================================================

 BOX TRACING

 =======================================================================
*/

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	0.03125

static vec3_t	cm_traceStart, cm_traceEnd;
static vec3_t	cm_traceMins, cm_traceMaxs;
static vec3_t	cm_traceExtents;

static trace_t	cm_trace;
static int		cm_traceContents;
static qboolean	cm_traceIsPoint;		// Optimized case

static int		cm_traceCheckCount;

/*
 =================
 CM_ClipBoxToBrush
 =================
*/
static void CM_ClipBoxToBrush (const vec3_t mins, const vec3_t maxs, const vec3_t p1, const vec3_t p2, trace_t *trace, cbrush_t *brush){

	int				i, j;
	cbrushside_t	*side, *leadSide;
	cplane_t		*plane, *clipPlane;
	vec3_t			ofs;
	float			dist, d1, d2;
	float			enterFrac, leaveFrac;
	float			f;
	qboolean		getOut, startOut;

	enterFrac = -1;
	leaveFrac = 1;
	clipPlane = NULL;

	if (!brush->numSides)
		return;

	getOut = false;
	startOut = false;
	leadSide = NULL;

	for (i = 0; i < brush->numSides; i++){
		side = &cm.brushSides[brush->firstBrushSide+i];
		plane = side->plane;

		if (!cm_traceIsPoint){
			// General box case
			if (plane->type < PLANE_NON_AXIAL){
				// Push the plane out appropriately for mins/maxs
				if (plane->normal[plane->type] < 0)
					dist = plane->dist - maxs[plane->type];
				else
					dist = plane->dist - mins[plane->type];

				d1 = p1[plane->type] - dist;
				d2 = p2[plane->type] - dist;
			}
			else {
				// Push the plane out appropriately for mins/maxs
				for (j = 0; j < 3; j++){
					if (plane->normal[j] < 0)
						ofs[j] = maxs[j];
					else
						ofs[j] = mins[j];
				}

				dist = plane->dist - DotProduct(ofs, plane->normal);

				d1 = DotProduct(p1, plane->normal) - dist;
				d2 = DotProduct(p2, plane->normal) - dist;
			}
		}
		else {
			// Special point case
			if (plane->type < PLANE_NON_AXIAL){
				d1 = p1[plane->type] - plane->dist;
				d2 = p2[plane->type] - plane->dist;
			}
			else {
				d1 = DotProduct(p1, plane->normal) - plane->dist;
				d2 = DotProduct(p2, plane->normal) - plane->dist;
			}
		}

		if (d2 > 0)
			getOut = true;	// End point is not in solid
		if (d1 > 0)
			startOut = true;

		// If completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// Crosses face
		if (d1 > d2){
			// Enter
			f = (d1 - DIST_EPSILON) / (d1 - d2);
			if (f > enterFrac){
				enterFrac = f;
				clipPlane = plane;
				leadSide = side;
			}
		}
		else {
			// Leave
			f = (d1 + DIST_EPSILON) / (d1 - d2);
			if (f < leaveFrac)
				leaveFrac = f;
		}
	}

	if (!startOut){
		// Original point was inside brush
		trace->startsolid = true;
		if (!getOut)
			trace->allsolid = true;

		return;
	}

	if (enterFrac < leaveFrac){
		if (enterFrac > -1 && enterFrac < trace->fraction){
			if (enterFrac < 0)
				enterFrac = 0;

			trace->fraction = enterFrac;
			trace->plane = *clipPlane;
			trace->surface = &(leadSide->surface->c);
			trace->contents = brush->contents;
		}
	}
}

/*
 =================
 CM_TestBoxInBrush
 =================
*/
static void CM_TestBoxInBrush (const vec3_t mins, const vec3_t maxs, const vec3_t p, trace_t *trace, cbrush_t *brush){

	int				i, j;
	cbrushside_t	*side;
	cplane_t		*plane;
	vec3_t			ofs;
	float			dist, d;

	if (!brush->numSides)
		return;

	for (i = 0; i < brush->numSides; i++){
		side = &cm.brushSides[brush->firstBrushSide+i];
		plane = side->plane;

		// General box case
		if (plane->type < PLANE_NON_AXIAL){
			// Push the plane out appropriately for mins/maxs
			if (plane->normal[plane->type] < 0)
				dist = plane->dist - maxs[plane->type];
			else
				dist = plane->dist - mins[plane->type];

			d = p[plane->type] - dist;
		}
		else {
			// Push the plane out appropriately for mins/maxs
			for (j = 0; j < 3; j++){
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}

			dist = plane->dist - DotProduct(ofs, plane->normal);

			d = DotProduct(p, plane->normal) - dist;
		}

		// If completely in front of face, no intersection
		if (d > 0)
			return;
	}

	// Inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}

/*
 =================
 CM_TraceToLeaf
 =================
*/
static void CM_TraceToLeaf (int leafNum){

	int			i;
	int			brushNum;
	cleaf_t		*leaf;
	cbrush_t	*brush;

	leaf = &cm.leafs[leafNum];
	if (!(leaf->contents & cm_traceContents))
		return;

	// Trace line against all brushes in the leaf
	for (i = 0; i < leaf->numLeafBrushes; i++){
		brushNum = cm.leafBrushes[leaf->firstLeafBrush+i];
		brush = &cm.brushes[brushNum];

		if (brush->checkCount == cm_traceCheckCount)
			continue;		// Already checked this brush in another leaf
		brush->checkCount = cm_traceCheckCount;

		if (!(brush->contents & cm_traceContents))
			continue;

		CM_ClipBoxToBrush(cm_traceMins, cm_traceMaxs, cm_traceStart, cm_traceEnd, &cm_trace, brush);
		if (!cm_trace.fraction)
			return;
	}
}

/*
 =================
 CM_TestInLeaf
 =================
*/
static void CM_TestInLeaf (int leafNum){

	int			i;
	int			brushNum;
	cleaf_t		*leaf;
	cbrush_t	*brush;

	leaf = &cm.leafs[leafNum];
	if (!(leaf->contents & cm_traceContents))
		return;

	// Trace line against all brushes in the leaf
	for (i = 0; i < leaf->numLeafBrushes; i++){
		brushNum = cm.leafBrushes[leaf->firstLeafBrush+i];
		brush = &cm.brushes[brushNum];

		if (brush->checkCount == cm_traceCheckCount)
			continue;		// Already checked this brush in another leaf
		brush->checkCount = cm_traceCheckCount;

		if (!(brush->contents & cm_traceContents))
			continue;

		CM_TestBoxInBrush(cm_traceMins, cm_traceMaxs, cm_traceStart, &cm_trace, brush);
		if (!cm_trace.fraction)
			return;
	}
}

/*
 =================
 CM_RecursiveHullCheck
 =================
*/
static void CM_RecursiveHullCheck (int num, float pf1, float pf2, const vec3_t p1, const vec3_t p2){

	cnode_t		*node;
	cplane_t	*plane;
	float		d1, d2, offset;
	float		frac1, frac2;
	float		dist;
	vec3_t		mid;
	int			side;
	float		midf;

	if (cm_trace.fraction <= pf1)
		return;		// Already hit something nearer

	// If < 0, we are in a leaf node
	if (num < 0){
		CM_TraceToLeaf(-1-num);
		return;
	}

	// Find the point distances to the separating plane and the offset
	// for the size of the box
	node = &cm.nodes[num];
	plane = node->plane;

	if (plane->type < PLANE_NON_AXIAL){
		d1 = p1[plane->type] - plane->dist;
		d2 = p2[plane->type] - plane->dist;

		offset = cm_traceExtents[plane->type];
	}
	else {
		d1 = DotProduct(p1, plane->normal) - plane->dist;
		d2 = DotProduct(p2, plane->normal) - plane->dist;

		if (cm_traceIsPoint)
			offset = 0;
		else
			offset = fabs(cm_traceExtents[0]*plane->normal[0]) + fabs(cm_traceExtents[1]*plane->normal[1]) + fabs(cm_traceExtents[2]*plane->normal[2]);
	}

	// See which sides we need to consider
	if (d1 >= offset && d2 >= offset){
		CM_RecursiveHullCheck(node->children[0], pf1, pf2, p1, p2);
		return;
	}
	if (d1 < -offset && d2 < -offset){
		CM_RecursiveHullCheck(node->children[1], pf1, pf2, p1, p2);
		return;
	}
	
	// Put the crosspoint DIST_EPSILON pixels on the near side
	if (d1 < d2){
		dist = 1.0 / (d1 - d2);
		side = 1;
		frac1 = (d1 - offset + DIST_EPSILON) * dist;
		frac2 = (d1 + offset + DIST_EPSILON) * dist;
	}
	else if (d1 > d2){
		dist = 1.0 / (d1 - d2);
		side = 0;
		frac1 = (d1 + offset + DIST_EPSILON) * dist;
		frac2 = (d1 - offset - DIST_EPSILON) * dist;
	}
	else {
		side = 0;
		frac1 = 1;
		frac2 = 0;
	}

	// Move up to the node
	if (frac1 < 0)
		frac1 = 0;
	else if (frac1 > 1)
		frac1 = 1;

	midf = pf1 + (pf2 - pf1) * frac1;

	mid[0] = p1[0] + (p2[0] - p1[0]) * frac1;
	mid[1] = p1[1] + (p2[1] - p1[1]) * frac1;
	mid[2] = p1[2] + (p2[2] - p1[2]) * frac1;

	CM_RecursiveHullCheck(node->children[side], pf1, midf, p1, mid);

	// Go past the node
	if (frac2 < 0)
		frac2 = 0;
	else if (frac2 > 1)
		frac2 = 1;

	midf = pf1 + (pf2 - pf1) * frac2;

	mid[0] = p1[0] + (p2[0] - p1[0]) * frac2;
	mid[1] = p1[1] + (p2[1] - p1[1]) * frac2;
	mid[2] = p1[2] + (p2[2] - p1[2]) * frac2;

	CM_RecursiveHullCheck(node->children[side^1], midf, pf2, mid, p2);
}

/*
 =================
 CM_BoxTrace
 =================
*/
trace_t CM_BoxTrace (const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, int headNode, int brushMask){

	// Fill in a default trace
	memset(&cm_trace, 0, sizeof(trace_t));
	cm_trace.fraction = 1;
	cm_trace.surface = &(cm_nullSurface.c);

	if (!cm.loaded)
		return cm_trace;

	cm_traceCheckCount++;	// For multi-check avoidance
	cm_traces++;			// Optimize counter

	cm_traceContents = brushMask;
	VectorCopy(start, cm_traceStart);
	VectorCopy(end, cm_traceEnd);
	VectorCopy(mins, cm_traceMins);
	VectorCopy(maxs, cm_traceMaxs);

	// Check for position test special case
	if (VectorCompare(start, end)){
		int		i;
		vec3_t	c1, c2;
		int		leafs[1024], numLeafs;
		int		topNode;

		for (i = 0; i < 3; i++){
			c1[i] = (start[i] + mins[i]) - 1;
			c2[i] = (start[i] + maxs[i]) + 1;
		}

		numLeafs = CM_BoxLeafNumsHeadNode(c1, c2, leafs, 1024, headNode, &topNode);

		for (i = 0; i < numLeafs; i++){
			CM_TestInLeaf(leafs[i]);
			if (cm_trace.allsolid)
				break;
		}

		VectorCopy(start, cm_trace.endpos);

		return cm_trace;
	}

	// Check for point special case
	if (VectorCompare(mins, vec3_origin) && VectorCompare(maxs, vec3_origin)){
		cm_traceIsPoint = true;

		VectorClear(cm_traceExtents);
	}
	else {
		cm_traceIsPoint = false;

		cm_traceExtents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		cm_traceExtents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		cm_traceExtents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	// General sweeping through world
	CM_RecursiveHullCheck(headNode, 0, 1, start, end);

	if (cm_trace.fraction == 1.0){
		cm_trace.endpos[0] = end[0];
		cm_trace.endpos[1] = end[1];
		cm_trace.endpos[2] = end[2];
	}
	else {
		cm_trace.endpos[0] = start[0] + (end[0] - start[0]) * cm_trace.fraction;
		cm_trace.endpos[1] = start[1] + (end[1] - start[1]) * cm_trace.fraction;
		cm_trace.endpos[2] = start[2] + (end[2] - start[2]) * cm_trace.fraction;
	}

	return cm_trace;
}

/*
 =================
 CM_TransformedBoxTrace

 Handles offseting and rotation of the points for moving and rotating
 entities
 =================
*/
trace_t	CM_TransformedBoxTrace (const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, int headNode, int brushMask, const vec3_t origin, const vec3_t angles){

	trace_t		trace;
	vec3_t		start2, end2, angles2, temp;
	vec3_t		axis[3];
	qboolean	rotated;

	if (headNode != cm_boxHeadNode && !VectorCompare(angles, vec3_origin)){
		rotated = true;

		AnglesToAxis(angles, axis);

		VectorSubtract(start, origin, temp);
		VectorRotate(temp, axis, start2);

		VectorSubtract(end, origin, temp);
		VectorRotate(temp, axis, end2);
	}
	else {
		rotated = false;

		VectorSubtract(start, origin, start2);
		VectorSubtract(end, origin, end2);
	}

	// Sweep the box through the world
	trace = CM_BoxTrace(start2, end2, mins, maxs, headNode, brushMask);

	if (rotated && trace.fraction != 1.0){
		VectorNegate(angles, angles2);
		AnglesToAxis(angles2, axis);

		VectorCopy(trace.plane.normal, temp);
		VectorRotate(temp, axis, trace.plane.normal);
	}

	if (trace.fraction == 1.0){
		trace.endpos[0] = end[0];
		trace.endpos[1] = end[1];
		trace.endpos[2] = end[2];
	}
	else {
		trace.endpos[0] = start[0] + (end[0] - start[0]) * trace.fraction;
		trace.endpos[1] = start[1] + (end[1] - start[1]) * trace.fraction;
		trace.endpos[2] = start[2] + (end[2] - start[2]) * trace.fraction;
	}

	return trace;
}


/*
 =======================================================================

 PVS / PHS

 =======================================================================
*/

static byte	cm_pvsRow[MAX_MAP_LEAFS/8];
static byte	cm_phsRow[MAX_MAP_LEAFS/8];


/*
 =================
 CM_DecompressVis
 =================
*/
static void CM_DecompressVis (const byte *in, byte *out){

	byte	*vis;
	int		c, row;

	row = (cm.numClusters+7)>>3;	
	vis = out;

	if (!in){
		// No vis info, so make all visible
		while (row){
			*vis++ = 0xFF;
			row--;
		}

		return;		
	}

	do {
		if (*in){
			*vis++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((vis - out) + c > row){
			c = row - (vis - out);
			Com_DPrintf(S_COLOR_YELLOW "Vis decompression overrun\n");
		}

		while (c){
			*vis++ = 0;
			c--;
		}
	} while (vis - out < row);
}

/*
 =================
 CM_ClusterPVS
 =================
*/
byte *CM_ClusterPVS (int cluster){

	if (!cm.loaded || !cm.numClusters){
		memset(cm_pvsRow, 0xFF, 1);
		return cm_pvsRow;
	}

	if (cluster == -1 || cm.numVisibility == 0)
		memset(cm_pvsRow, 0xFF, (cm.numClusters+7)>>3);
	else
		CM_DecompressVis((byte *)cm.visibility + cm.visibility->bitOfs[cluster][VIS_PVS], cm_pvsRow);

	return cm_pvsRow;
}

/*
 =================
 CM_ClusterPHS
 =================
*/
byte *CM_ClusterPHS (int cluster){

	if (!cm.loaded || !cm.numClusters){
		memset(cm_phsRow, 0xFF, 1);
		return cm_phsRow;
	}

	if (cluster == -1 || cm.numVisibility == 0)
		memset(cm_phsRow, 0xFF, (cm.numClusters+7)>>3);
	else
		CM_DecompressVis((byte *)cm.visibility + cm.visibility->bitOfs[cluster][VIS_PHS], cm_phsRow);

	return cm_phsRow;
}


/*
 =======================================================================

 AREAPORTALS

 =======================================================================
*/

static qboolean	cm_areaPortalOpen[MAX_MAP_AREAPORTALS];
static int		cm_floodValid;


/*
 =================
 CM_RecursiveFloodArea
 =================
*/
static void CM_RecursiveFloodArea (carea_t *area, int floodNum){

	int				i;
	careaportal_t	*p;

	if (area->floodValid == cm_floodValid){
		if (area->floodNum == floodNum)
			return;

		Com_Error(ERR_DROP, "CM_RecursiveFloodArea: reflooded");
	}

	area->floodNum = floodNum;
	area->floodValid = cm_floodValid;

	p = &cm.areaPortals[area->firstAreaPortal];
	for (i = 0; i < area->numAreaPortals; i++, p++){
		if (!cm_areaPortalOpen[p->portalNum])
			continue;

		CM_RecursiveFloodArea(&cm.areas[p->otherArea], floodNum);
	}
}

/*
 =================
 CM_FloodAreaConnections
 =================
*/
static void CM_FloodAreaConnections (qboolean clear){

	int		i;
	carea_t	*area;
	int		floodNum = 0;

	if (clear)
		memset(cm_areaPortalOpen, 0, sizeof(cm_areaPortalOpen));

	// All current floods are now invalid
	cm_floodValid++;

	// Area 0 is not used
	for (i = 1; i < cm.numAreas; i++){
		area = &cm.areas[i];

		if (area->floodValid == cm_floodValid)
			continue;		// Already flooded into

		floodNum++;
		CM_RecursiveFloodArea(area, floodNum);
	}
}

/*
 =================
 CM_SetAreaPortalState
 =================
*/
void CM_SetAreaPortalState (int portalNum, qboolean open){

	if (!cm.loaded)
		return;

	if (portalNum < 0 || portalNum >= cm.numAreaPortals)
		Com_Error(ERR_DROP, "CM_SetAreaPortalState: bad area portal");

	cm_areaPortalOpen[portalNum] = open;

	CM_FloodAreaConnections(false);
}

/*
 =================
 CM_AreasConnected
 =================
*/
qboolean CM_AreasConnected (int area1, int area2){

	if (!cm.loaded || !cm.numAreas)
		return true;

	if (cm_noAreas->integerValue)
		return true;

	if ((area1 < 0 || area1 >= cm.numAreas) || (area2 < 0 || area2 >= cm.numAreas))
		Com_Error(ERR_DROP, "CM_AreasConnected: bad area");

	if (cm.areas[area1].floodNum == cm.areas[area2].floodNum)
		return true;

	return false;
}

/*
 =================
 CM_WriteAreaBits

 Writes a length byte followed by a bit vector of all the areas that 
 are in the same flood as the area parameter.

 This is used by the client refresh to cull visibility.
 =================
*/
int CM_WriteAreaBits (byte *buffer, int area){

	int		i;
	int		floodNum;
	int		bytes;

	if (!cm.loaded || !cm.numAreas){
		memset(buffer, 0xFF, 1);
		return 1;
	}

	bytes = (cm.numAreas+7)>>3;

	if (cm_noAreas->integerValue){
		// For debugging, send everything
		memset(buffer, 0xFF, bytes);
		return bytes;
	}

	memset(buffer, 0, bytes);

	floodNum = cm.areas[area].floodNum;
	for (i = 0; i < cm.numAreas; i++){
		if (cm.areas[i].floodNum == floodNum || !area)
			buffer[i>>3] |= 1<<(i&7);
	}
	
	return bytes;
}

/*
 =================
 CM_WritePortalState

 Writes the portal state to a savegame file
 =================
*/
void CM_WritePortalState (fileHandle_t f){

	FS_Write(cm_areaPortalOpen, sizeof(cm_areaPortalOpen), f);
}

/*
 =================
 CM_ReadPortalState

 Reads the portal state from a savegame file and recalculates the area 
 connections
 =================
*/
void CM_ReadPortalState (fileHandle_t f){

	FS_Read(cm_areaPortalOpen, sizeof(cm_areaPortalOpen), f);

	if (!cm.loaded)
		return;

	CM_FloodAreaConnections(false);
}

/*
 =================
 CM_HeadNodeVisible

 Returns true if any leaf under headNode has a cluster that is 
 potentially visible
 =================
*/
qboolean CM_HeadNodeVisible (int headNode, const byte *visBits){

	int		leafNum;
	int		cluster;
	cnode_t	*node;

	if (!cm.loaded)
		return false;

	if (headNode < 0){
		leafNum = -1 - headNode;
		cluster = cm.leafs[leafNum].cluster;
		if (cluster == -1)
			return false;
		
		if (visBits[cluster>>3] & (1<<(cluster&7)))
			return true;

		return false;
	}

	node = &cm.nodes[headNode];

	if (CM_HeadNodeVisible(node->children[0], visBits))
		return true;

	return CM_HeadNodeVisible(node->children[1], visBits);
}
