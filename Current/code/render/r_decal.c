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


// r_decal.c -- decal management and fragment clipping


#include "r_local.h"


#define MAX_FRAGMENTS			128
#define MAX_FRAGMENT_VERTICES	384

typedef struct {
	surface_t	*parent;

	int			firstVertex;
	int			numVertices;
} fragment_t;

static fragment_t	r_fragments[MAX_FRAGMENTS];
static int			r_numFragments;

static vec3_t		r_fragmentVertices[MAX_FRAGMENT_VERTICES];
static int			r_numFragmentVertices;

static cplane_t		r_fragmentClipPlanes[6];


/*
 =================
 R_ClipFragment
 =================
*/
static void R_ClipFragment (int stage, int numVertices, vec3_t vertices, surface_t *surface, fragment_t *fragment){

	int			i;
	float		*v;
	qboolean	frontSide;
	vec3_t		front[MAX_FRAGMENT_VERTICES];
	int			f;
	float		dist;
	float		dists[MAX_FRAGMENT_VERTICES];
	int			sides[MAX_FRAGMENT_VERTICES];
	cplane_t	*plane;

	if (numVertices > MAX_FRAGMENT_VERTICES-2)
		Com_Error(ERR_DROP, "R_ClipFragment: MAX_FRAGMENT_VERTICES hit");

	if (stage == 6){
		// Fully clipped
		if (numVertices > 2){
			if (r_numFragmentVertices + numVertices > MAX_FRAGMENT_VERTICES)
				return;

			fragment->parent = surface;

			fragment->firstVertex = r_numFragmentVertices;
			fragment->numVertices = numVertices;

			for (i = 0, v = vertices; i < numVertices; i++, v += 3)
				VectorCopy(v, r_fragmentVertices[r_numFragmentVertices+i]);

			r_numFragmentVertices += numVertices;
		}

		return;
	}

	frontSide = false;

	plane = &r_fragmentClipPlanes[stage];
	for (i = 0, v = vertices; i < numVertices; i++, v += 3){
		if (plane->type < PLANE_NON_AXIAL)
			dists[i] = dist = v[plane->type] - plane->dist;
		else
			dists[i] = dist = DotProduct(v, plane->normal) - plane->dist;

		if (dist > ON_EPSILON){
			frontSide = true;
			sides[i] = SIDE_FRONT;
		}
		else if (dist < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
			sides[i] = SIDE_ON;
	}

	if (!frontSide)
		return;		// Not clipped

	// Clip it
	dists[i] = dists[0];
	sides[i] = sides[0];
	VectorCopy(vertices, (vertices + (i*3)));

	f = 0;

	for (i = 0, v = vertices; i < numVertices; i++, v += 3){
		switch (sides[i]){
		case SIDE_FRONT:
			VectorCopy(v, front[f]);
			f++;

			break;
		case SIDE_BACK:

			break;
		case SIDE_ON:
			VectorCopy(v, front[f]);
			f++;

			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		dist = dists[i] / (dists[i] - dists[i+1]);

		front[f][0] = v[0] + (v[3] - v[0]) * dist;
		front[f][1] = v[1] + (v[4] - v[1]) * dist;
		front[f][2] = v[2] + (v[5] - v[2]) * dist;

		f++;
	}

	// Continue
	R_ClipFragment(stage+1, f, front[0], surface, fragment);
}

/*
 =================
 R_ClipFragmentToSurface
 =================
*/
static void R_ClipFragmentToSurface (surface_t *surface){

	fragment_t		*fragment;
	surfTriangle_t	*triangle;
	vec3_t			vertices[4];
	int				i;

	// Copy vertex data and clip to each triangle
	for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
		fragment = &r_fragments[r_numFragments];
		fragment->firstVertex = fragment->numVertices = 0;

		VectorCopy(surface->vertices[triangle->index[0]].xyz, vertices[0]);
		VectorCopy(surface->vertices[triangle->index[1]].xyz, vertices[1]);
		VectorCopy(surface->vertices[triangle->index[2]].xyz, vertices[2]);

		R_ClipFragment(0, 3, vertices[0], surface, fragment);

		if (fragment->numVertices){
			r_numFragments++;

			if (r_numFragments == MAX_FRAGMENTS || r_numFragmentVertices == MAX_FRAGMENT_VERTICES)
				return;		// Already reached the limit
		}
	}
}

/*
 =================
 R_RecursiveFragmentNode
 =================
*/
static void R_RecursiveFragmentNode (node_t *node, const vec3_t origin, const vec3_t normal, float radius){

	surface_t	*surface;
	int			i, side;

	if (node->contents != -1)
		return;

	if (r_numFragments == MAX_FRAGMENTS || r_numFragmentVertices == MAX_FRAGMENT_VERTICES)
		return;		// Already reached the limit somewhere else

	// Find which side of the node we are on
	side = SphereOnPlaneSide(origin, radius, node->plane);

	// Go down the appropriate sides
	if (side == SIDE_FRONT){
		R_RecursiveFragmentNode(node->children[0], origin, normal, radius);
		return;
	}
	if (side == SIDE_BACK){
		R_RecursiveFragmentNode(node->children[1], origin, normal, radius);
		return;
	}

	// Clip to each surface
	surface = tr.worldModel->surfaces + node->firstSurface;
	for (i = 0; i < node->numSurfaces; i++, surface++){
		if (r_numFragments == MAX_FRAGMENTS || r_numFragmentVertices == MAX_FRAGMENT_VERTICES)
			return;			// Already reached the limit

		if (surface->fragmentCount == tr.fragmentCount)
			continue;		// Already checked this surface in another node
		surface->fragmentCount = tr.fragmentCount;

		if (surface->texInfo->material->noOverlays)
			continue;		// Don't bother clipping

		if (!surface->texInfo->material->numStages)
			continue;		// Don't bother clipping

		if (!BoundsAndSphereIntersect(surface->mins, surface->maxs, origin, radius))
			continue;		// No intersection

		if (!(surface->flags & SURF_PLANEBACK)){
			if (DotProduct(surface->plane->normal, normal) < 0.5)
				continue;	// Greater than 60 degrees
		}
		else {
			if (DotProduct(surface->plane->normal, normal) > -0.5)
				continue;	// Greater than 60 degrees
		}

		// Clip to the surface
		R_ClipFragmentToSurface(surface);
	}

	// Recurse down the children
	R_RecursiveFragmentNode(node->children[0], origin, normal, radius);
	R_RecursiveFragmentNode(node->children[1], origin, normal, radius);
}

/*
 =================
 R_DecalFragments
 =================
*/
static qboolean R_DecalFragments (const vec3_t origin, const vec3_t axis[3], float radius){

	float	dot;
	int		i;

	// Bump fragment count
	tr.fragmentCount++;

	// Initialize fragments
	r_numFragments = 0;
	r_numFragmentVertices = 0;

	// Calculate clipping planes
	for (i = 0; i < 3; i++){
		dot = DotProduct(origin, axis[i]);

		VectorCopy(axis[i], r_fragmentClipPlanes[i*2+0].normal);
		r_fragmentClipPlanes[i*2+0].dist = dot - radius;
		r_fragmentClipPlanes[i*2+0].type = PlaneTypeForNormal(r_fragmentClipPlanes[i*2+0].normal);

		VectorNegate(axis[i], r_fragmentClipPlanes[i*2+1].normal);
		r_fragmentClipPlanes[i*2+1].dist = -dot - radius;
		r_fragmentClipPlanes[i*2+1].type = PlaneTypeForNormal(r_fragmentClipPlanes[i*2+1].normal);
	}

	// Clip against world geometry
	R_RecursiveFragmentNode(tr.worldModel->nodes, origin, axis[0], radius);

	return (r_numFragments > 0);
}


// =====================================================================

static decal_t		r_decalList[MAX_DECALS];
static decal_t		r_activeDecals;
static decal_t		*r_freeDecals;


/*
 =================
 R_FreeDecal
 =================
*/
static void R_FreeDecal (decal_t *decal){

	if (!decal->prev)
		return;

	decal->prev->next = decal->next;
	decal->next->prev = decal->prev;

	decal->next = r_freeDecals;
	r_freeDecals = decal;
}

/*
 =================
 R_AllocDecal

 Will always succeed, even if it requires freeing an old active decal
 =================
*/
static decal_t *R_AllocDecal (void){

	decal_t	*decal;

	if (!r_freeDecals)
		R_FreeDecal(r_activeDecals.prev);

	decal = r_freeDecals;
	r_freeDecals = r_freeDecals->next;

	memset(decal, 0, sizeof(decal_t));

	decal->next = r_activeDecals.next;
	decal->prev = &r_activeDecals;
	r_activeDecals.next->prev = decal;
	r_activeDecals.next = decal;

	return decal;
}

/*
 =================
 R_ProjectDecal
 =================
*/
void R_ProjectDecal (const vec3_t origin, const vec3_t direction, float orientation, float radius, int time, material_t *material){

	fragment_t	*fragment;
	decal_t		*decal;
	vec3_t		axis[3], delta;
	int			i, j;

	if (!tr.worldModel)
		Com_Error(ERR_DROP, "R_ProjectDecal: NULL worldModel");

	if (!material->numStages)
		return;

	if (material->decalInfo.stayTime + material->decalInfo.fadeTime == 0.0)
		return;

	// Find orientation vectors
	VectorNormalize2(direction, axis[0]);
	PerpendicularVector(axis[1], axis[0]);
	RotatePointAroundVector(axis[2], axis[0], axis[1], orientation);
	CrossProduct(axis[0], axis[2], axis[1]);

	// Get the clipped decal fragments
	if (!R_DecalFragments(origin, axis, radius))
		return;

	VectorScale(axis[1], 0.5 / radius, axis[1]);
	VectorScale(axis[2], 0.5 / radius, axis[2]);

	// Allocate and store the decal fragments
	for (i = 0, fragment = r_fragments; i < r_numFragments; i++, fragment++){
		if (!fragment->numVertices)
			continue;

		if (fragment->numVertices > MAX_DECAL_VERTICES)
			fragment->numVertices = MAX_DECAL_VERTICES;

		decal = R_AllocDecal();

		decal->time = MS2SEC(time);
		decal->duration = material->decalInfo.stayTime + material->decalInfo.fadeTime;
		decal->material = material;

		decal->parent = fragment->parent;

		decal->numVertices = fragment->numVertices;

		for (j = 0; j < fragment->numVertices; j++){
			VectorCopy(r_fragmentVertices[fragment->firstVertex+j], decal->vertices[j].xyz);

			VectorCopy(fragment->parent->vertices->normal, decal->vertices[j].normal);

			VectorSubtract(decal->vertices[j].xyz, origin, delta);
			decal->vertices[j].st[0] = DotProduct(delta, axis[1]) + 0.5;
			decal->vertices[j].st[1] = DotProduct(delta, axis[2]) + 0.5;
		}
	}
}

/*
 =================
 R_ClearDecals
 =================
*/
void R_ClearDecals (void){

	int		i;

	memset(r_decalList, 0, sizeof(r_decalList));

	r_activeDecals.next = &r_activeDecals;
	r_activeDecals.prev = &r_activeDecals;
	r_freeDecals = r_decalList;

	for (i = 0; i < MAX_DECALS - 1; i++)
		r_decalList[i].next = &r_decalList[i+1];
}

/*
 =================
 R_AddDecals
 =================
*/
void R_AddDecals (void){

	decal_t	*decal, *next;

	if (r_skipDecals->integerValue)
		return;

	if (!tr.renderViewParms.primaryView)
		return;

	for (decal = r_activeDecals.next; decal != &r_activeDecals; decal = next){
		// Grab next now, so if the decal is freed we still have it
		next = decal->next;

		// Check if completely faded out
		if (tr.renderView.time > decal->time + decal->duration){
			R_FreeDecal(decal);
			continue;
		}

		// Check if the parent surface is visible
		if (decal->parent->viewCount != tr.viewCount)
			continue;

		tr.pc.decals++;

		// Add it
		R_AddMeshToList(MESH_DECAL, decal, decal->material, tr.worldEntity);
	}
}
