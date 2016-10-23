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


#include "client.h"


#define MAX_MARKS				2048

#define MAX_VERTS_ON_POLY		10

#define	MAX_MARK_VERTS			384
#define	MAX_MARK_FRAGMENTS		128

typedef struct cmark_s {
	struct cmark_s	*prev, *next;
	int				time;
	color_t			modulate;
	qboolean		alphaFade;
	struct shader_s	*shader;
	int				numVerts;
	polyVert_t		verts[MAX_VERTS_ON_POLY];
	vec3_t	origin;
} cmark_t;

static cmark_t	cl_activeMarks;
static cmark_t	*cl_freeMarks;
static cmark_t	cl_markList[MAX_MARKS];


/*
 =================
 CL_FreeMark
 =================
*/
static void CL_FreeMark (cmark_t *m){

	if (!m->prev)
		return;

	m->prev->next = m->next;
	m->next->prev = m->prev;

	m->next = cl_freeMarks;
	cl_freeMarks = m;
}

/*
 =================
 CL_AllocMark

 Will always succeed, even if it requires freeing an old active mark
 =================
*/
static cmark_t *CL_AllocMark (void){

	cmark_t	*m;

	if (!cl_freeMarks)
		CL_FreeMark(cl_activeMarks.prev);

	m = cl_freeMarks;
	cl_freeMarks = cl_freeMarks->next;

	memset(m, 0, sizeof(cmark_t));

	m->next = cl_activeMarks.next;
	m->prev = &cl_activeMarks;
	cl_activeMarks.next->prev = m;
	cl_activeMarks.next = m;

	return m;
}

/*
 =================
 CL_ClearMarks
 =================
*/
void CL_ClearMarks (void){

	int		i;

	memset(cl_markList, 0, sizeof(cl_markList));

	cl_activeMarks.next = &cl_activeMarks;
	cl_activeMarks.prev = &cl_activeMarks;
	cl_freeMarks = cl_markList;

	for (i = 0; i < MAX_MARKS - 1; i++)
		cl_markList[i].next = &cl_markList[i+1];
}

/*
 =================
 CL_AddMarks
 =================
*/
void CL_AddMarks (void){

	cmark_t	*m, *next;
	int		i;
	float	c;
	int		time, fadeTime;

	fadeTime = cl_markTime->integer / 10;

	for (m = cl_activeMarks.next; m != &cl_activeMarks; m = next){
		// Grab next now, so if the mark is freed we still have it
		next = m->next;

		if (cl.time >= m->time + cl_markTime->integer){
			CL_FreeMark(m);
			continue;
		}

		// Fade out glowing energy marks
		if (m->shader == cl.media.energyMarkShader){
			time = cl.time - m->time;

			if (time < fadeTime){
				c = 1.0 - ((float)time / fadeTime);

				for (i = 0; i < m->numVerts; i++){
					m->verts[i].modulate[0] = m->modulate[0] * c;
					m->verts[i].modulate[1] = m->modulate[1] * c;
					m->verts[i].modulate[2] = m->modulate[2] * c;
				}
			}
		}

		// Fade out with time
		time = m->time + cl_markTime->integer - cl.time;

		if (time < fadeTime){
			c = (float)time / fadeTime;

			if (m->alphaFade){
				for (i = 0; i < m->numVerts; i++)
					m->verts[i].modulate[3] = m->modulate[3] * c;
			}
			else {
				for (i = 0; i < m->numVerts; i++){
					m->verts[i].modulate[0] = m->modulate[0] * c;
					m->verts[i].modulate[1] = m->modulate[1] * c;
					m->verts[i].modulate[2] = m->modulate[2] * c;
				}
			}
		}

		R_AddPolyToScene(m->shader, m->numVerts, m->verts);
	}
}

/*
 =================
 CL_ImpactMark

 Temporary marks will be inmediately passed to the renderer
 =================
*/
void CL_ImpactMark (const vec3_t org, const vec3_t dir, float orientation, float radius, float r, float g, float b, float a, qboolean alphaFade, struct shader_s *shader, qboolean temporary){

	int				i, j;
	vec3_t			axis[3], delta;
	color_t			modulate;
	int				numFragments;
	markFragment_t	markFragments[MAX_MARK_FRAGMENTS], *mf;
	vec3_t			markVerts[MAX_MARK_VERTS];
	polyVert_t		verts[MAX_VERTS_ON_POLY];
	cmark_t			*m;

	if (cl_markTime->integer <= 0)
		return;

	// Find orientation vectors
	VectorNormalize2(dir, axis[0]);
	PerpendicularVector(axis[1], axis[0]);
	RotatePointAroundVector(axis[2], axis[0], axis[1], orientation);
	CrossProduct(axis[0], axis[2], axis[1]);

	// Get the clipped mark fragments
	numFragments = R_MarkFragments(org, axis, radius, MAX_MARK_VERTS, markVerts, MAX_MARK_FRAGMENTS, markFragments);
	if (!numFragments)
		return;

	VectorScale(axis[1], 0.5 / radius, axis[1]);
	VectorScale(axis[2], 0.5 / radius, axis[2]);

	*(unsigned *)modulate = ColorBytes(r, g, b, a);

	for (i = 0, mf = markFragments; i < numFragments; i++, mf++){
		if (!mf->numVerts)
			continue;

		if (mf->numVerts > MAX_VERTS_ON_POLY)
			mf->numVerts = MAX_VERTS_ON_POLY;

		// If it is a temporary mark, pass it to the renderer without
		// storing
		if (temporary){
			for (j = 0; j < mf->numVerts; j++){
				VectorCopy(markVerts[mf->firstVert + j], verts[j].xyz);

				VectorSubtract(verts[j].xyz, org, delta);
				verts[j].st[0] = 0.5 + DotProduct(delta, axis[1]);
				verts[j].st[1] = 0.5 + DotProduct(delta, axis[2]);

				*(unsigned *)verts[j].modulate = *(unsigned *)modulate;
			}

			R_AddPolyToScene(shader, mf->numVerts, verts);
			continue;
		}

		m = CL_AllocMark();
		VectorCopy(org, m->origin);
		m->time = cl.time;
		m->modulate[0] = modulate[0];
		m->modulate[1] = modulate[1];
		m->modulate[2] = modulate[2];
		m->modulate[3] = modulate[3];
		m->alphaFade = alphaFade;
		m->shader = shader;
		m->numVerts = mf->numVerts;

		for (j = 0; j < mf->numVerts; j++){
			VectorCopy(markVerts[mf->firstVert + j], m->verts[j].xyz);

			VectorSubtract(m->verts[j].xyz, org, delta);
			m->verts[j].st[0] = 0.5 + DotProduct(delta, axis[1]);
			m->verts[j].st[1] = 0.5 + DotProduct(delta, axis[2]);

			*(unsigned *)m->verts[j].modulate = *(unsigned *)modulate;
		}
	}
}
