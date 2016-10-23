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


// r_fragment.c -- fragment clipping


#include "r_local.h"


#define MAX_FRAGMENT_VERTS		128

static int				r_numFragmentVerts;
static int				r_maxFragmentVerts;
static vec3_t			*r_fragmentVerts;

static int				r_numFragments;
static int				r_maxFragments;
static markFragment_t	*r_fragments;

static cplane_t			r_fragmentPlanes[6];

static int				r_fragmentCount;


/*
 =================
 R_ClipFragment
 =================
*/
static void R_ClipFragment (int numVerts, vec3_t verts, int stage, markFragment_t *mf){

	int			i;
	float		*v;
	qboolean	frontSide;
	vec3_t		front[MAX_FRAGMENT_VERTS];
	int			f;
	float		dist;
	float		dists[MAX_FRAGMENT_VERTS];
	int			sides[MAX_FRAGMENT_VERTS];
	cplane_t	*plane;

	if (numVerts > MAX_FRAGMENT_VERTS-2)
		Com_Error(ERR_DROP, "R_ClipFragment: MAX_FRAGMENT_VERTS hit");

	if (stage == 6){
		// Fully clipped
		if (numVerts > 2){
			if (r_numFragmentVerts + numVerts > r_maxFragmentVerts)
				return;

			mf->firstVert = r_numFragmentVerts;
			mf->numVerts = numVerts;

			for (i = 0, v = verts; i < numVerts; i++, v += 3)
				VectorCopy(v, r_fragmentVerts[r_numFragmentVerts+i]);

			r_numFragmentVerts += numVerts;
		}

		return;
	}

	frontSide = false;

	plane = &r_fragmentPlanes[stage];
	for (i = 0, v = verts; i < numVerts; i++, v += 3){
		if (plane->type < 3)
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
	VectorCopy(verts, (verts + (i*3)));

	f = 0;

	for (i = 0, v = verts; i < numVerts; i++, v += 3){
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
	R_ClipFragment(f, front[0], stage+1, mf);
}

/*
 =================
 R_ClipFragmentToSurface
 =================
*/
static void R_ClipFragmentToSurface (surface_t *surf){

	markFragment_t	*mf;
	surfPoly_t		*p;
	vec3_t			verts[MAX_FRAGMENT_VERTS];
	int				i;

	// Copy vertex data and clip to each triangle
	for (p = surf->poly; p; p = p->next){
		for (i = 0; i < p->numIndices; i += 3){
			mf = &r_fragments[r_numFragments];
			mf->firstVert = mf->numVerts = 0;

			VectorCopy(p->vertices[p->indices[i+0]].xyz, verts[0]);
			VectorCopy(p->vertices[p->indices[i+1]].xyz, verts[1]);
			VectorCopy(p->vertices[p->indices[i+2]].xyz, verts[2]);

			R_ClipFragment(3, verts[0], 0, mf);

			if (mf->numVerts){
				r_numFragments++;

				if (r_numFragmentVerts == r_maxFragmentVerts || r_numFragments == r_maxFragments)
					return;		// Already reached the limit
			}
		}
	}
}

/*
 =================
 R_RecursiveFragmentNode
 =================
*/
static void R_RecursiveFragmentNode (node_t *node, const vec3_t origin, const vec3_t normal, float radius){

	int			i;
	float		dist;
	cplane_t	*plane;
	surface_t	*surf;

	if (node->contents != -1)
		return;

	if (r_numFragmentVerts == r_maxFragmentVerts || r_numFragments == r_maxFragments)
		return;		// Already reached the limit somewhere else

	// Find which side of the node we are on
	plane = node->plane;
	if (plane->type < 3)
		dist = origin[plane->type] - plane->dist;
	else
		dist = DotProduct(origin, plane->normal) - plane->dist;

	// Go down the appropriate sides
	if (dist > radius){
		R_RecursiveFragmentNode(node->children[0], origin, normal, radius);
		return;
	}
	if (dist < -radius){
		R_RecursiveFragmentNode(node->children[1], origin, normal, radius);
		return;
	}

	// Clip to each surface
	surf = r_worldModel->surfaces + node->firstSurface;
	for (i = 0; i < node->numSurfaces; i++, surf++){
		if (r_numFragmentVerts == r_maxFragmentVerts || r_numFragments == r_maxFragments)
			break;			// Already reached the limit

		if (surf->fragmentFrame == r_fragmentCount)
			continue;		// Already checked this surface in another node
		surf->fragmentFrame = r_fragmentCount;

		if (surf->texInfo->flags & (SURF_SKY | SURF_NODRAW))
			continue;		// Don't bother clipping

		if (surf->texInfo->shader->flags & SHADER_NOFRAGMENTS)
			continue;		// Don't bother clipping

		if (!BoundsAndSphereIntersect(surf->mins, surf->maxs, origin, radius))
			continue;		// No intersection

		if (!(surf->flags & SURF_PLANEBACK)){
			if (DotProduct(normal, surf->plane->normal) < 0.5)
				continue;	// Greater than 60 degrees
		}
		else {
			if (DotProduct(normal, surf->plane->normal) > -0.5)
				continue;	// Greater than 60 degrees
		}

		// Clip to the surface
		R_ClipFragmentToSurface(surf);
	}

	// Recurse down the children
	R_RecursiveFragmentNode(node->children[0], origin, normal, radius);
	R_RecursiveFragmentNode(node->children[1], origin, normal, radius);
}

/*
 =================
 R_MarkFragments
 =================
*/
int R_MarkFragments (const vec3_t origin, const vec3_t axis[3], float radius, int maxVerts, vec3_t *verts, int maxFragments, markFragment_t *fragments){

	int		i;
	float	dot;

	if (!r_worldModel)
		return 0;			// Map not loaded

	r_fragmentCount++;		// For multi-check avoidance

	// Initialize fragments
	r_numFragmentVerts = 0;
	r_maxFragmentVerts = maxVerts;
	r_fragmentVerts = verts;

	r_numFragments = 0;
	r_maxFragments = maxFragments;
	r_fragments = fragments;

	// Calculate clipping planes
	for (i = 0; i < 3; i++){
		dot = DotProduct(origin, axis[i]);

		VectorCopy(axis[i], r_fragmentPlanes[i*2+0].normal);
		r_fragmentPlanes[i*2+0].dist = dot - radius;
		r_fragmentPlanes[i*2+0].type = PlaneTypeForNormal(r_fragmentPlanes[i*2+0].normal);

		VectorNegate(axis[i], r_fragmentPlanes[i*2+1].normal);
		r_fragmentPlanes[i*2+1].dist = -dot - radius;
		r_fragmentPlanes[i*2+1].type = PlaneTypeForNormal(r_fragmentPlanes[i*2+1].normal);
	}

	// Clip against world geometry
	R_RecursiveFragmentNode(r_worldModel->nodes, origin, axis[0], radius);

	return r_numFragments;
}
