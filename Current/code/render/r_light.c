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


#define MAX_STATIC_LIGHTS				4096

#define MAX_LIGHT_PLANE_VERTICES		64

typedef struct {
	qboolean	FIXME;
	qboolean	degenerate;
	cplane_t	frustum[5];
} nearClipVolume_t;

static nearClipVolume_t	r_nearClipVolume;

static mesh_t			r_shadowsList[2][MAX_MESHES];
static mesh_t			r_interactionsList[2][MAX_MESHES];

static lightSource_t	r_staticLights[MAX_STATIC_LIGHTS];
static int				r_numStaticLights;


/*
 =================
 R_ParseLights
 =================
*/
static void R_ParseLights (script_t *script){

	token_t			token;
	lightSource_t	*lightSource;
	vec3_t			origin, center, angles, radius;
	int				style, detailLevel;
	qboolean		parallel, noShadows;
	material_t		*material;
	float			materialParms[MAX_ENTITY_MATERIAL_PARMS];
	vec3_t			tmp;
	float			dot;
	int				i;

	while (1){
		if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token))
			break;		// End of data

		if (!Q_stricmp(token.string, "{")){
			// Set default values
			VectorClear(origin);
			VectorClear(center);
			VectorClear(angles);
			VectorSet(radius, 100, 100, 100);

			style = 0;
			detailLevel = 10;
			parallel = false;
			noShadows = false;

			material = tr.defaultLightMaterial;
			materialParms[MATERIALPARM_RED] = 1.0;
			materialParms[MATERIALPARM_GREEN] = 1.0;
			materialParms[MATERIALPARM_BLUE] = 1.0;
			materialParms[MATERIALPARM_ALPHA] = 1.0;
			materialParms[MATERIALPARM_TIME_OFFSET] = 0.0;
			materialParms[MATERIALPARM_DIVERSITY] = 0.0;
			materialParms[MATERIALPARM_GENERAL] = 0.0;
			materialParms[MATERIALPARM_MODE] = 0.0;

			// Parse the light
			while (1){
				if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
					Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in lights file\n");
					return;		// End of data
				}

				if (!Q_stricmp(token.string, "}"))
					break;		// End of light

				// Parse the field
				if (!Q_stricmp(token.string, "origin")){
					for (i = 0; i < 3; i++){
						if (!PS_ReadFloat(script, 0, &origin[i])){
							Com_Printf(S_COLOR_YELLOW "WARNING: missing 'origin' parameters for light\n");
							return;
						}
					}
				}
				else if (!Q_stricmp(token.string, "center")){
					for (i = 0; i < 3; i++){
						if (!PS_ReadFloat(script, 0, &center[i])){
							Com_Printf(S_COLOR_YELLOW "WARNING: missing 'center' parameters for light\n");
							return;
						}
					}
				}
				else if (!Q_stricmp(token.string, "angles")){
					for (i = 0; i < 3; i++){
						if (!PS_ReadFloat(script, 0, &angles[i])){
							Com_Printf(S_COLOR_YELLOW "WARNING: missing 'angles' parameters for light\n");
							return;
						}
					}
				}
				else if (!Q_stricmp(token.string, "radius")){
					for (i = 0; i < 3; i++){
						if (!PS_ReadFloat(script, 0, &radius[i])){
							Com_Printf(S_COLOR_YELLOW "WARNING: missing 'radius' parameters for light\n");
							return;
						}

						if (radius[i] <= 0.0){
							Com_Printf(S_COLOR_YELLOW "WARNING: invalid 'radius' value of %f for light\n", radius[i]);
							return;
						}
					}
				}
				else if (!Q_stricmp(token.string, "color")){
					for (i = 0; i < 3; i++){
						if (!PS_ReadFloat(script, 0, &materialParms[i])){
							Com_Printf(S_COLOR_YELLOW "WARNING: missing 'color' parameters for light\n");
							return;
						}

						if (materialParms[i] < 0.0){
							Com_Printf(S_COLOR_YELLOW "WARNING: invalid 'color' value of %f for light\n", materialParms[i]);
							return;
						}
					}
				}
				else if (!Q_stricmp(token.string, "style")){
					if (!PS_ReadInteger(script, 0, &style)){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'style' parameters for light\n");
						return;
					}

					if (style < 0 || style >= MAX_LIGHTSTYLES){
						Com_Printf(S_COLOR_YELLOW "WARNING: invalid 'style' value of %i for light\n", style);
						return;
					}
				}
				else if (!Q_stricmp(token.string, "detailLevel")){
					if (!PS_ReadInteger(script, 0, &detailLevel)){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'detailLevel' parameters for light\n");
						return;
					}

					if (detailLevel < 0 || detailLevel > 10){
						Com_Printf(S_COLOR_YELLOW "WARNING: invalid 'detailLevel' value of %i for light\n", detailLevel);
						return;
					}
				}
				else if (!Q_stricmp(token.string, "parallel"))
					parallel = true;
				else if (!Q_stricmp(token.string, "noShadows"))
					noShadows = true;
				else if (!Q_stricmp(token.string, "material")){
					if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'material' parameters for light\n");
						return;
					}

					material = R_FindMaterial(token.string, MT_LIGHT, 0);
				}
				else if (!Q_stricmp(token.string, "materialParm3")){
					if (!PS_ReadFloat(script, 0, &materialParms[3])){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'materialParm3' parameters for light\n");
						return;
					}
				}
				else if (!Q_stricmp(token.string, "materialParm4")){
					if (!PS_ReadFloat(script, 0, &materialParms[4])){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'materialParm4' parameters for light\n");
						return;
					}
				}
				else if (!Q_stricmp(token.string, "materialParm5")){
					if (!PS_ReadFloat(script, 0, &materialParms[5])){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'materialParm5' parameters for light\n");
						return;
					}
				}
				else if (!Q_stricmp(token.string, "materialParm6")){
					if (!PS_ReadFloat(script, 0, &materialParms[6])){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'materialParm6' parameters for light\n");
						return;
					}
				}
				else if (!Q_stricmp(token.string, "materialParm7")){
					if (!PS_ReadFloat(script, 0, &materialParms[7])){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'materialParm7' parameters for light\n");
						return;
					}
				}
				else {
					Com_Printf(S_COLOR_YELLOW "WARNING: unknown field '%s' for light\n", token.string);
					return;
				}
			}

			// Add it to the list
			if (r_numStaticLights == MAX_STATIC_LIGHTS)
				Com_Error(ERR_DROP, "R_ParseLights: MAX_STATIC_LIGHTS hit");

			lightSource = &r_staticLights[r_numStaticLights++];

			lightSource->index = r_numStaticLights - 1;
			VectorCopy(origin, lightSource->origin);
			VectorCopy(center, lightSource->center);
			VectorCopy(angles, lightSource->angles);
			VectorCopy(radius, lightSource->radius);
			lightSource->style = style;
			lightSource->detailLevel = detailLevel;
			lightSource->parallel = parallel;
			lightSource->noShadows = noShadows;
			lightSource->material = material;
			lightSource->materialParms[0] = materialParms[0];
			lightSource->materialParms[1] = materialParms[1];
			lightSource->materialParms[2] = materialParms[2];
			lightSource->materialParms[3] = materialParms[3];
			lightSource->materialParms[4] = materialParms[4];
			lightSource->materialParms[5] = materialParms[5];
			lightSource->materialParms[6] = materialParms[6];
			lightSource->materialParms[7] = materialParms[7];

			// Compute axes
			AnglesToAxis(angles, lightSource->axis);

			// Compute the corners of the bounding volume
			for (i = 0; i < 8; i++){
				tmp[0] = (i & 1) ? -lightSource->radius[0] : lightSource->radius[0];
				tmp[1] = (i & 2) ? -lightSource->radius[1] : lightSource->radius[1];
				tmp[2] = (i & 4) ? -lightSource->radius[2] : lightSource->radius[2];

				// Rotate and translate
				VectorRotate(tmp, lightSource->axis, lightSource->corners[i]);
				VectorAdd(lightSource->corners[i], lightSource->origin, lightSource->corners[i]);
			}

			// Check if it's rotated
			lightSource->rotated = !VectorCompare(angles, vec3_origin);

			// Compute mins/maxs
			VectorSubtract(lightSource->origin, lightSource->radius, lightSource->mins);
			VectorAdd(lightSource->origin, lightSource->radius, lightSource->maxs);

			// Compute frustum planes
			for (i = 0; i < 3; i++){
				dot = DotProduct(lightSource->origin, lightSource->axis[i]);

				VectorCopy(lightSource->axis[i], lightSource->frustum[i*2+0].normal);
				lightSource->frustum[i*2+0].dist = dot - lightSource->radius[i];
				lightSource->frustum[i*2+0].type = PlaneTypeForNormal(lightSource->frustum[i*2+0].normal);
				SetPlaneSignbits(&lightSource->frustum[i*2+0]);

				VectorNegate(lightSource->axis[i], lightSource->frustum[i*2+1].normal);
				lightSource->frustum[i*2+1].dist = -dot - lightSource->radius[i];
				lightSource->frustum[i*2+1].type = PlaneTypeForNormal(lightSource->frustum[i*2+1].normal);
				SetPlaneSignbits(&lightSource->frustum[i*2+1]);
			}

			// Find the leaf
			VectorAdd(lightSource->origin, lightSource->center, tmp);

			lightSource->leaf = R_PointInLeaf(tmp);
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in lights file\n", token.string);
			return;
		}
	}
}

/*
 =================
 R_LoadLights

 TODO: precompute shadow/interaction lists
 =================
*/
void R_LoadLights (void){

	script_t	*script;
	char		name[MAX_OSPATH];

	r_numStaticLights = 0;

	// Load the script file
	Q_snprintfz(name, sizeof(name), "%s.txt", tr.worldModel->name);

	script = PS_LoadScriptFile(name);
	if (!script)
		return;

	// Parse it
	R_ParseLights(script);

	PS_FreeScript(script);
}

static qboolean R_LightIntersectsBounds (lightSource_t *light, const vec3_t mins, const vec3_t maxs){

	cplane_t	*plane;
	int			i;

	if (!light->rotated){
		if (BoundsIntersect(light->mins, light->maxs, mins, maxs))
			return true;

		return false;
	}

	for (i = 0, plane = light->frustum; i < 6; i++, plane++){
		switch (plane->signbits){
		case 0:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*maxs[1] + plane->normal[2]*maxs[2] < plane->dist){
				tr.pc.boxOut++;
				return false;
			}

			break;
		case 1:
			if (plane->normal[0]*mins[0] + plane->normal[1]*maxs[1] + plane->normal[2]*maxs[2] < plane->dist){
				tr.pc.boxOut++;
				return false;
			}

			break;
		case 2:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*mins[1] + plane->normal[2]*maxs[2] < plane->dist){
				tr.pc.boxOut++;
				return false;
			}

			break;
		case 3:
			if (plane->normal[0]*mins[0] + plane->normal[1]*mins[1] + plane->normal[2]*maxs[2] < plane->dist){
				tr.pc.boxOut++;
				return false;
			}

			break;
		case 4:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*maxs[1] + plane->normal[2]*mins[2] < plane->dist){
				tr.pc.boxOut++;
				return false;
			}

			break;
		case 5:
			if (plane->normal[0]*mins[0] + plane->normal[1]*maxs[1] + plane->normal[2]*mins[2] < plane->dist){
				tr.pc.boxOut++;
				return false;
			}

			break;
		case 6:
			if (plane->normal[0]*maxs[0] + plane->normal[1]*mins[1] + plane->normal[2]*mins[2] < plane->dist){
				tr.pc.boxOut++;
				return false;
			}

			break;
		case 7:
			if (plane->normal[0]*mins[0] + plane->normal[1]*mins[1] + plane->normal[2]*mins[2] < plane->dist){
				tr.pc.boxOut++;
				return false;
			}

			break;
		default:
			tr.pc.boxIn++;
			return true;
		}
	}

	tr.pc.boxIn++;

	return true;
}

static qboolean R_LightIntersectsSphere (lightSource_t *light, const vec3_t origin, float radius){

	cplane_t	*plane;
	int			i;

	if (!light->rotated){
		if (BoundsAndSphereIntersect(light->mins, light->maxs, origin, radius))
			return true;

		return false;
	}

	for (i = 0, plane = light->frustum; i < 6; i++, plane++){
		if (DotProduct(origin, plane->normal) - plane->dist <= -radius){
			tr.pc.sphereOut++;
			return false;
		}
	}

	tr.pc.sphereIn++;

	return true;
}

static qboolean R_LightIntersectsPoint (lightSource_t *light, const vec3_t point){

	cplane_t	*plane;
	int			i;

	if (!light->rotated){
		if (BoundsAndPointIntersect(light->mins, light->maxs, point))
			return true;

		return false;
	}

	for (i = 0, plane = light->frustum; i < 6; i++, plane++){
		if (DotProduct(point, plane->normal) - plane->dist <= 0){
			return false;
		}
	}

	return true;
}

/*
 =================
 R_ClipLightPlane
 =================
*/
static void R_ClipLightPlane (int stage, int numVertices, vec3_t vertices, const mat4_t mvpMatrix, vec2_t mins, vec2_t maxs){

	int			i;
	float		*v;
	qboolean	frontSide;
	vec3_t		front[MAX_LIGHT_PLANE_VERTICES];
	int			f;
	float		dist;
	float		dists[MAX_LIGHT_PLANE_VERTICES];
	int			sides[MAX_LIGHT_PLANE_VERTICES];
	cplane_t	*plane;
	vec4_t		in, out;
	float		x, y;

	if (numVertices > MAX_LIGHT_PLANE_VERTICES-2)
		Com_Error(ERR_DROP, "R_ClipLightPlane: MAX_LIGHT_PLANE_VERTICES hit");

	if (stage == 5){
		// Fully clipped, so add screen space points
		for (i = 0, v = vertices; i < numVertices; i++, v += 3){
			in[0] = v[0];
			in[1] = v[1];
			in[2] = v[2];
			in[3] = 1.0;

			Matrix4_Transform(mvpMatrix, in, out);

			if (out[3] == 0.0)
				continue;

			out[0] /= out[3];
			out[1] /= out[3];

			x = tr.renderView.x + (0.5 + 0.5 * out[0]) * tr.renderView.width;
			y = tr.renderView.y + (0.5 + 0.5 * out[1]) * tr.renderView.height;

			mins[0] = min(mins[0], x);
			mins[1] = min(mins[1], y);
			maxs[0] = max(maxs[0], x);
			maxs[1] = max(maxs[1], y);
		}

		return;
	}

	frontSide = false;

	plane = &tr.renderViewParms.frustum[stage];
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
	R_ClipLightPlane(stage+1, f, front[0], mvpMatrix, mins, maxs);
}

/*
 =================
 R_SetScissorRect
 =================
*/
static void R_SetScissorRect (light_t *light, lightSource_t *lightSource){

	int			cornerIndices[6][4] = {{3, 2, 6, 7}, {0, 1, 5, 4}, {2, 3, 1, 0}, {4, 5, 7, 6}, {1, 3, 7, 5}, {2, 0, 4, 6}};
	mat4_t		mvpMatrix;
	vec3_t		vertices[5];
	vec2_t		mins = {999999, 999999}, maxs = {-999999, -999999};
	int			xMin, yMin, xMax, yMax;
	int			i;

	if (r_skipScissors->integerValue || R_LightIntersectsPoint(lightSource, tr.renderView.viewOrigin)){
		light->scissorX = tr.renderView.x;
		light->scissorY = tr.renderView.y;
		light->scissorWidth = tr.renderView.width;
		light->scissorHeight = tr.renderView.height;

		return;
	}

	// Compute modelview-projection matrix
	Matrix4_Multiply(tr.renderViewParms.perspectiveMatrix, tr.renderViewParms.worldMatrix, mvpMatrix);

	// Copy the corner points of each plane and clip to the frustum
	for (i = 0; i < 6; i++){
		VectorCopy(light->corners[cornerIndices[i][0]], vertices[0]);
		VectorCopy(light->corners[cornerIndices[i][1]], vertices[1]);
		VectorCopy(light->corners[cornerIndices[i][2]], vertices[2]);
		VectorCopy(light->corners[cornerIndices[i][3]], vertices[3]);

		R_ClipLightPlane(0, 4, vertices[0], mvpMatrix, mins, maxs);
	}

	// Set the scissor rectangle
	xMin = max(floor(mins[0]), tr.renderView.x);
	yMin = max(floor(mins[1]), tr.renderView.y);
	xMax = min(ceil(maxs[0]), tr.renderView.x + tr.renderView.width);
	yMax = min(ceil(maxs[1]), tr.renderView.y + tr.renderView.height);

	if (xMax <= xMin || yMax <= yMin){
		light->scissorX = tr.renderView.x;
		light->scissorY = tr.renderView.y;
		light->scissorWidth = tr.renderView.width;
		light->scissorHeight = tr.renderView.height;

		return;
	}

	light->scissorX = xMin;
	light->scissorY = yMin;
	light->scissorWidth = xMax - xMin;
	light->scissorHeight = yMax - yMin;
}

// TODO!!!
static void R_SetDepthBounds (light_t *light, lightSource_t *lightSource){

	mat4_t	mvpMatrix;
	vec4_t	in, out;
	float	depth;
	float	depthMin = 999999, depthMax = -999999;
	int		i;

//	if (r_skipDepthBounds->integerValue || !glConfig.depthBoundsTest){
		light->depthMin = 0.0;
		light->depthMax = 1.0;

		return;
//	}

	// Compute modelview-projection matrix
	Matrix4_Multiply(tr.renderViewParms.perspectiveMatrix, tr.renderViewParms.worldMatrix, mvpMatrix);

	// Add screen space points
	for (i = 0; i < 8; i++){
		in[0] = light->corners[i][0];
		in[1] = light->corners[i][1];
		in[2] = light->corners[i][2];
		in[3] = 1.0;

		Matrix4_Transform(mvpMatrix, in, out);

		if (out[3] == 0.0)
			continue;

		depth = 0.5 + 0.5 * (out[2] / out[3]);

		depthMin = min(depthMin, depth);
		depthMax = max(depthMax, depth);
	}

	// Set the depth bounds
	depthMin = max(depthMin, 0.0);
	depthMax = min(depthMax, 1.0);

	if (depthMax <= depthMin){
		light->depthMin = 0.0;
		light->depthMax = 1.0;

		return;
	}

	light->depthMin = depthMin;
	light->depthMax = depthMax;
}

/*
 =================
 R_SetupNearClipVolume
 =================
*/
static void R_SetupNearClipVolume (const vec3_t lightOrigin){

	mat4_t	invMatrix;
	vec3_t	eyeCorners[4], worldCorners[4];
	float	x, y;
	int		i, side;

	// See which side of the near plane the light is
	side = PointOnPlaneSide(lightOrigin, ON_EPSILON, &tr.renderViewParms.frustum[0]);

	if (side == SIDE_ON){
		r_nearClipVolume.degenerate = true;
		return;
	}

	r_nearClipVolume.degenerate = false;

	// Compute the eye space corners of the viewport
	x = r_zNear->floatValue * tan(tr.renderView.fovX * M_PI / 360.0);
	y = r_zNear->floatValue * tan(tr.renderView.fovY * M_PI / 360.0);

	VectorSet(eyeCorners[0], x, y, -r_zNear->floatValue);
	VectorSet(eyeCorners[1], -x, y, -r_zNear->floatValue);
	VectorSet(eyeCorners[2], -x, -y, -r_zNear->floatValue);
	VectorSet(eyeCorners[3], x, -y, -r_zNear->floatValue);

	// Transform the corners to world space
	Matrix4_AffineInverse(tr.renderViewParms.worldMatrix, invMatrix);

	Matrix4_TransformVector(invMatrix, eyeCorners[0], worldCorners[0]);
	Matrix4_TransformVector(invMatrix, eyeCorners[1], worldCorners[1]);
	Matrix4_TransformVector(invMatrix, eyeCorners[2], worldCorners[2]);
	Matrix4_TransformVector(invMatrix, eyeCorners[3], worldCorners[3]);

	// Set up the frustum planes
	if (side == SIDE_FRONT){
		VectorCopy(tr.renderViewParms.frustum[0].normal, r_nearClipVolume.frustum[0].normal);
		r_nearClipVolume.frustum[0].dist = tr.renderViewParms.frustum[0].dist;

		PlaneFromPoints(&r_nearClipVolume.frustum[1], lightOrigin, worldCorners[1], worldCorners[0]);
		PlaneFromPoints(&r_nearClipVolume.frustum[2], lightOrigin, worldCorners[2], worldCorners[1]);
		PlaneFromPoints(&r_nearClipVolume.frustum[3], lightOrigin, worldCorners[3], worldCorners[2]);
		PlaneFromPoints(&r_nearClipVolume.frustum[4], lightOrigin, worldCorners[0], worldCorners[3]);
	}
	else {
		VectorNegate(tr.renderViewParms.frustum[0].normal, r_nearClipVolume.frustum[0].normal);
		r_nearClipVolume.frustum[0].dist = -tr.renderViewParms.frustum[0].dist;

		PlaneFromPoints(&r_nearClipVolume.frustum[1], lightOrigin, worldCorners[0], worldCorners[1]);
		PlaneFromPoints(&r_nearClipVolume.frustum[2], lightOrigin, worldCorners[1], worldCorners[2]);
		PlaneFromPoints(&r_nearClipVolume.frustum[3], lightOrigin, worldCorners[2], worldCorners[3]);
		PlaneFromPoints(&r_nearClipVolume.frustum[4], lightOrigin, worldCorners[3], worldCorners[0]);
	}

	for (i = 0; i < 5; i++){
		r_nearClipVolume.frustum[i].type = PLANE_NON_AXIAL;
		SetPlaneSignbits(&r_nearClipVolume.frustum[i]);
	}
}

/*
 =================
 R_BoxInNearClipVolume
 =================
*/
static qboolean R_BoxInNearClipVolume (const vec3_t mins, const vec3_t maxs){

	int i;

	if (r_nearClipVolume.FIXME)
		return true;

	if (r_nearClipVolume.degenerate){
		if (BoxOnPlaneSide(mins, maxs, &tr.renderViewParms.frustum[0]) == SIDE_CROSS)
			return true;

		return false;
	}

	for (i = 0; i < 5; i++){
		if (BoxOnPlaneSide(mins, maxs, &r_nearClipVolume.frustum[i]) == SIDE_BACK)
			return false;
	}

	return true;
}

/*
 =================
 R_SphereInNearClipVolume
 =================
*/
static qboolean R_SphereInNearClipVolume (const vec3_t center, float radius){

	int		i;

	if (r_nearClipVolume.FIXME)
		return true;

	if (r_nearClipVolume.degenerate){
		if (SphereOnPlaneSide(center, radius, &tr.renderViewParms.frustum[0]) == SIDE_CROSS)
			return true;

		return false;
	}

	for (i = 0; i < 5; i++){
		if (SphereOnPlaneSide(center, radius, &r_nearClipVolume.frustum[i]) == SIDE_BACK)
			return false;
	}

	return true;
}

static qboolean R_CullBoxProjection (const vec3_t lightOrigin, const vec3_t mins, const vec3_t maxs){

	cplane_t	*plane;
	vec4_t		corners[16];
	vec3_t		tmp;
	int			mask, aggregateMask = ~0;
	int			i, j;

	// Compute and project the corners of the bounding volume
	for (i = 0; i < 8; i++){
		tmp[0] = (i & 1) ? mins[0] : maxs[0];
		tmp[1] = (i & 2) ? mins[1] : maxs[1];
		tmp[2] = (i & 4) ? mins[2] : maxs[2];

		corners[i+0][0] = tmp[0];
		corners[i+0][1] = tmp[1];
		corners[i+0][2] = tmp[2];
		corners[i+0][3] = 1.0;

		corners[i+8][0] = tmp[0] - lightOrigin[0];
		corners[i+8][1] = tmp[1] - lightOrigin[1];
		corners[i+8][2] = tmp[2] - lightOrigin[2];
		corners[i+8][3] = 0.0;
	}

	// Cull
	for (i = 0; i < 16; i++){
		mask = 0;

		for (j = 0, plane = tr.renderViewParms.frustum; j < 5; j++, plane++){
			if (DotProduct(corners[i], plane->normal) < corners[i][3] * plane->dist)
				mask |= (1<<j);
		}

		aggregateMask &= mask;
	}

	if (aggregateMask)
		return true;

	return false;
}

static qboolean R_CullSphereProjection (const vec3_t lightOrigin, const vec3_t center, float radius){

	cplane_t	*plane;
	vec4_t		corners[16];
	vec3_t		tmp;
	int			mask, aggregateMask = ~0;
	int			i, j;

	// Compute and project the corners of the bounding volume
	for (i = 0; i < 8; i++){
		tmp[0] = center[0] + (i & 1) ? -radius : radius;
		tmp[1] = center[1] + (i & 2) ? -radius : radius;
		tmp[2] = center[2] + (i & 4) ? -radius : radius;

		corners[i+0][0] = tmp[0];
		corners[i+0][1] = tmp[1];
		corners[i+0][2] = tmp[2];
		corners[i+0][3] = 1.0;

		corners[i+8][0] = tmp[0] - lightOrigin[0];
		corners[i+8][1] = tmp[1] - lightOrigin[1];
		corners[i+8][2] = tmp[2] - lightOrigin[2];
		corners[i+8][3] = 0.0;
	}

	// Cull
	for (i = 0; i < 16; i++){
		mask = 0;

		for (j = 0, plane = tr.renderViewParms.frustum; j < 5; j++, plane++){
			if (DotProduct(corners[i], plane->normal) < corners[i][3] * plane->dist)
				mask |= (1<<j);
		}

		aggregateMask &= mask;
	}

	if (aggregateMask)
		return true;

	return false;
}

/*
 =================
 R_AddShadowMeshToLight
 =================
*/
static void R_AddShadowMeshToLight (light_t *light, meshType_t type, meshData_t *data, material_t *material, renderEntity_t *entity, qboolean caps){

	mesh_t	*mesh;

	tr.pc.shadows++;
	tr.pc.shadowsMemory += sizeof(mesh_t);

	if (caps)
		tr.pc.shadowsZFail++;
	else
		tr.pc.shadowsZPass++;

	if (!material->noSelfShadow){
		if (light->numShadows[0] == MAX_MESHES)
			Com_Error(ERR_DROP, "R_AddShadowMeshToLight: MAX_MESHES hit");

		mesh = &r_shadowsList[0][light->numShadows[0]++];
	}
	else {
		if (light->numShadows[1] == MAX_MESHES)
			Com_Error(ERR_DROP, "R_AddShadowMeshToLight: MAX_MESHES hit");

		mesh = &r_shadowsList[1][light->numShadows[1]++];
	}

	mesh->sort = (material->index << 20) | (entity->index << 9) | (type << 5) | (caps);
	mesh->data = data;
}

/*
 =================
 R_RecursiveShadowNode
 =================
*/
static void R_RecursiveShadowNode (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin, node_t *node, int clipFlags){

	leaf_t		*leaf;
	surface_t	*surface, **mark;
	cplane_t	*plane;
	qboolean	caps;
	int			i, side;

	if (node->contents == CONTENTS_SOLID)
		return;		// Solid

	// Cull
	if (clipFlags){
		for (i = 0, plane = lightSource->frustum; i < 6; i++, plane++){
			if (!(clipFlags & (1<<i)))
				continue;

			side = BoxOnPlaneSide(node->mins, node->maxs, plane);

			if (side == SIDE_BACK)
				return;

			if (side == SIDE_FRONT)
				clipFlags &= ~(1<<i);
		}
	}

	// Recurse down the children
	if (node->contents == -1){
		R_RecursiveShadowNode(light, lightSource, lightOrigin, node->children[0], clipFlags);
		R_RecursiveShadowNode(light, lightSource, lightOrigin, node->children[1], clipFlags);
		return;
	}

	// If a leaf node, draw stuff
	leaf = (leaf_t *)node;

	if (!leaf->numMarkSurfaces)
		return;

	// Check if in light PVS
	if (!r_skipShadowCulling->integerValue){
		if (!(lightSource->vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))))
			return;
	}

	// Add all the surfaces
	for (i = 0, mark = leaf->firstMarkSurface; i < leaf->numMarkSurfaces; i++, mark++){
		surface = *mark;

		if (surface->lightCount == tr.lightCount)
			continue;		// Already added this surface from another leaf
		surface->lightCount = tr.lightCount;

		if (surface->texInfo->material->noShadows)
			continue;		// Don't bother drawing

		if (surface->texInfo->material->spectrum > lightSource->material->spectrum)
			continue;		// Not illuminated by this light

		// Cull face
		side = PointOnPlaneSide(lightOrigin, 0.0, surface->plane);

		if (!(surface->flags & SURF_PLANEBACK)) {
			if (side != SIDE_FRONT)
				continue;
		} else {
			if (side != SIDE_BACK)
				continue;
		}

		if (!r_skipShadowCulling->integerValue) {
			if (!R_LightIntersectsBounds(lightSource, surface->mins, surface->maxs))
				continue;	// Not in light volume
		}

		// Select rendering method
		caps = R_BoxInNearClipVolume(surface->mins, surface->maxs);

		// Add the surface
		R_AddShadowMeshToLight(light, MESH_SURFACE, surface, surface->texInfo->material, tr.worldEntity, caps);
	}
}

/*
 =================
 R_AddWorldShadows
 =================
*/
static void R_AddWorldShadows (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin){

	if (!tr.renderViewParms.primaryView)
		return;

	if (!r_skipShadowCulling->integerValue)
		R_RecursiveShadowNode(light, lightSource, lightOrigin, tr.worldModel->nodes, 63);
	else
		R_RecursiveShadowNode(light, lightSource, lightOrigin, tr.worldModel->nodes, 0);
}

/*
 =================
 R_AddDecorationShadows
 =================
*/
static void R_AddDecorationShadows (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin){

	decSurface_t	*surface;
	leaf_t			*leaf;
	qboolean		caps;
	int				i, j;

	if (r_skipDecorations->integerValue)
		return;

	if (!tr.renderViewParms.primaryView)
		return;

	for (i = 0, surface = tr.worldModel->decorationSurfaces; i < tr.worldModel->numDecorationSurfaces; i++, surface++){
		if (surface->material->noShadows)
			continue;		// Don't bother drawing

		if (surface->material->spectrum > lightSource->material->spectrum)
			continue;		// Not illuminated by this light

		if (!r_skipShadowCulling->integerValue){
			if (!R_LightIntersectsBounds(lightSource, surface->mins, surface->maxs))
				continue;	// Not in light volume

			// Check if in light PVS
			for (j = 0; j < surface->numLeafs; j++){
				leaf = surface->leafList[j];

				if (!(lightSource->vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))))
					continue;		// Not in PVS

				break;
			}

			if (j == surface->numLeafs)
				continue;
		}

		// Select rendering method
		caps = R_BoxInNearClipVolume(surface->mins, surface->maxs);

		// Add the surface
		R_AddShadowMeshToLight(light, MESH_DECORATION, surface, surface->material, tr.worldEntity, caps);
	}
}

/*
 =================
 R_AddBrushModelShadows
 =================
*/
static void R_AddBrushModelShadows (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin, renderEntity_t *entity){

	model_t		*model = entity->model;
	surface_t	*surface;
	vec3_t		origin, tmp;
	vec3_t		mins, maxs;
	qboolean	caps;
	int			i, side;

	if (!model->numModelSurfaces)
		return;

	// Cull
	if (!AxisCompare(entity->axis, axisDefault)){
		if (!r_skipShadowCulling->integerValue){
			if (!R_LightIntersectsSphere(lightSource, entity->origin, model->radius))
				return;		// Not in light volume
		}

		VectorSubtract(lightOrigin, entity->origin, tmp);
		VectorRotate(tmp, entity->axis, origin);

		// Select rendering method
		caps = R_SphereInNearClipVolume(entity->origin, model->radius);
	}
	else {
		VectorAdd(entity->origin, model->mins, mins);
		VectorAdd(entity->origin, model->maxs, maxs);

		if (!r_skipShadowCulling->integerValue){
			if (!R_LightIntersectsBounds(lightSource, mins, maxs))
				return;		// Not in light volume
		}

		VectorSubtract(lightOrigin, entity->origin, origin);

		// Select rendering method
		caps = R_BoxInNearClipVolume(mins, maxs);
	}

	// Add all the surfaces
	surface = model->surfaces + model->firstModelSurface;
	for (i = 0; i < model->numModelSurfaces; i++, surface++){
		if (surface->texInfo->material->noShadows)
			continue;		// Don't bother drawing

		if (surface->texInfo->material->spectrum > lightSource->material->spectrum)
			continue;		// Not illuminated by this light

		// Cull face
		side = PointOnPlaneSide(origin, 0.0, surface->plane);

		if (!(surface->flags & SURF_PLANEBACK)){
			if (side != SIDE_FRONT)
				continue;
		}
		else {
			if (side != SIDE_BACK)
				continue;
		}

		// Add the surface
		R_AddShadowMeshToLight(light, MESH_SURFACE, surface, surface->texInfo->material, entity, caps);
	}
}

/*
 =================
 R_AddAliasModelShadows
 =================
*/
static void R_AddAliasModelShadows (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin, renderEntity_t *entity){

	mdl_t			*alias = entity->model->alias;
	mdlFrame_t		*curFrame, *oldFrame;
	mdlSurface_t	*surface;
	material_t		*material;
	float			radius;
	qboolean		caps;
	int				i;

	if (entity->renderFX & RF_VIEWERMODEL){
		if (!r_playerShadow->integerValue)
			return;
	}

	// Never cast shadows from weapon model
	if (entity->renderFX & RF_WEAPONMODEL)
		return;

	// Check if this entity emits this light
	if (VectorCompare(lightOrigin, entity->origin))
		return;

	// Find model radius
	if ((entity->frame < 0 || entity->frame >= alias->numFrames) || (entity->oldFrame < 0 || entity->oldFrame >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelShadows: no such frame %i to %i (%s)\n", entity->frame, entity->oldFrame, entity->model->realName);

		entity->frame = 0;
		entity->oldFrame = 0;
	}

	curFrame = alias->frames + entity->frame;
	oldFrame = alias->frames + entity->oldFrame;

	if (curFrame == oldFrame)
		radius = curFrame->radius;
	else {
		if (curFrame->radius > oldFrame->radius)
			radius = curFrame->radius;
		else
			radius = oldFrame->radius;
	}

	if (!r_skipShadowCulling->integerValue){
		if (!R_LightIntersectsSphere(lightSource, entity->origin, radius))
			return;		// Not in light volume
	}

	// Select rendering method
	if (entity->renderFX & RF_VIEWERMODEL)
		caps = true;
	else
		caps = R_SphereInNearClipVolume(entity->origin, radius);

	// Add all the surfaces
	for (i = 0, surface = alias->surfaces; i < alias->numSurfaces; i++, surface++){
		// Select material
		if (entity->customMaterial)
			material = entity->customMaterial;
		else {
			if (surface->numMaterials){
				if (entity->skinIndex < 0 || entity->skinIndex >= surface->numMaterials){
					Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelShadows: no such material %i (%s)\n", entity->skinIndex, entity->model->realName);

					entity->skinIndex = 0;
				}

				material = surface->materials[entity->skinIndex].material;
			}
			else {
				Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelShadows: no materials for surface (%s)\n", entity->model->realName);

				material = tr.defaultMaterial;
			}
		}

		if (material->noShadows)
			continue;		// Don't bother drawing

		if (material->spectrum > lightSource->material->spectrum)
			continue;		// Not illuminated by this light

		// Add the surface
		R_AddShadowMeshToLight(light, MESH_ALIASMODEL, surface, material, entity, caps);
	}
}

/*
 =================
 R_AddEntityShadows
 =================
*/
static void R_AddEntityShadows (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin){

	renderEntity_t	*entity;
	model_t			*model;
	int				i;

	if (r_skipEntities->integerValue)
		return;

	for (i = 0, entity = tr.renderViewParms.renderEntities; i < tr.renderViewParms.numRenderEntities; i++, entity++){
		if (entity->reType != RE_MODEL)
			continue;

		model = entity->model;

		if (!model || model->modelType == MODEL_BAD)
			continue;

		switch (model->modelType){
		case MODEL_BSP:
			R_AddBrushModelShadows(light, lightSource, lightOrigin, entity);
			break;
		case MODEL_MD3:
		case MODEL_MD2:
			R_AddAliasModelShadows(light, lightSource, lightOrigin, entity);
			break;
		}
	}
}

/*
 =================
 R_CreateShadowsList
 =================
*/
static void R_CreateShadowsList (light_t *light, lightSource_t *lightSource){

	vec3_t	lightOrigin;
	double	time;

	tr.lightCount++;

	light->numShadows[0] = light->numShadows[1] = 0;

	if (!r_shadows->integerValue || lightSource->noShadows || lightSource->material->noShadows){
		light->castShadows = false;
		return;
	}

	if (r_showShadows->integerValue)
		time = Sys_GetClockTicks();

	if (lightSource->parallel)
		VectorMA(lightSource->origin, 999999, lightSource->center, lightOrigin);
	else
		VectorAdd(lightSource->origin, lightSource->center, lightOrigin);

	R_SetupNearClipVolume(lightOrigin);

	// FIXME: parallel lights and mirror views are fucked up
	if (lightSource->parallel || tr.renderViewParms.subview == SUBVIEW_MIRROR)
		r_nearClipVolume.FIXME = true;
	else
		r_nearClipVolume.FIXME = false;

	R_AddWorldShadows(light, lightSource, lightOrigin);
	R_AddDecorationShadows(light, lightSource, lightOrigin);
	R_AddEntityShadows(light, lightSource, lightOrigin);

	if (light->numShadows[0]){
		light->castShadows = true;

		light->shadows[0] = Hunk_HighAlloc(light->numShadows[0] * sizeof(mesh_t));
		memcpy(light->shadows[0], r_shadowsList[0], light->numShadows[0] * sizeof(mesh_t));
	}

	if (light->numShadows[1]){
		light->castShadows = true;

		light->shadows[1] = Hunk_HighAlloc(light->numShadows[1] * sizeof(mesh_t));
		memcpy(light->shadows[1], r_shadowsList[1], light->numShadows[1] * sizeof(mesh_t));
	}

	if (r_showShadows->integerValue)
		tr.pc.timeShadows += (Sys_GetClockTicks() - time);
}


// =====================================================================


/*
 =================
 R_AddInteractionMeshToLight
 =================
*/
static void R_AddInteractionMeshToLight (light_t *light, meshType_t type, meshData_t *data, material_t *material, renderEntity_t *entity){

	mesh_t	*mesh;

	tr.pc.interactions++;
	tr.pc.interactionsMemory += sizeof(mesh_t);

	if (light->material->fogLight || light->material->blendLight)
		tr.pc.interactionsFog++;
	else
		tr.pc.interactionsLight++;

	if (!material->noSelfShadow || (light->material->fogLight || light->material->blendLight)){
		if (light->numInteractions[0] == MAX_MESHES)
			Com_Error(ERR_DROP, "R_AddInteractionMeshToLight: MAX_MESHES hit");

		mesh = &r_interactionsList[0][light->numInteractions[0]++];
	}
	else {
		if (light->numInteractions[1] == MAX_MESHES)
			Com_Error(ERR_DROP, "R_AddInteractionMeshToLight: MAX_MESHES hit");

		mesh = &r_interactionsList[1][light->numInteractions[1]++];
	}

	mesh->sort = (material->index << 20) | (entity->index << 9) | (type << 5) | (0);
	mesh->data = data;
}

/*
 =================
 R_RecursiveInteractionNode
 =================
*/
static void R_RecursiveInteractionNode (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin, node_t *node, int clipFlags){

	leaf_t		*leaf;
	surface_t	*surface, **mark;
	cplane_t	*plane;
	int			i, side;

	if (node->contents == CONTENTS_SOLID)
		return;		// Solid

	if (node->viewCount != tr.viewCount)
		return;		// Not visible

	// Cull
	if (clipFlags){
		for (i = 0, plane = lightSource->frustum; i < 6; i++, plane++){
			if (!(clipFlags & (1<<i)))
				continue;

			side = BoxOnPlaneSide(node->mins, node->maxs, plane);

			if (side == SIDE_BACK)
				return;

			if (side == SIDE_FRONT)
				clipFlags &= ~(1<<i);
		}
	}

	// Recurse down the children
	if (node->contents == -1){
		R_RecursiveInteractionNode(light, lightSource, lightOrigin, node->children[0], clipFlags);
		R_RecursiveInteractionNode(light, lightSource, lightOrigin, node->children[1], clipFlags);
		return;
	}

	// If a leaf node, draw stuff
	leaf = (leaf_t *)node;

	if (!leaf->numMarkSurfaces)
		return;

	if (!lightSource->material->fogLight && !lightSource->material->blendLight && !lightSource->material->ambientLight){
		// Check for door connected areas
		if (!r_skipAreas->integerValue && tr.renderView.areaBits){
			if (!(tr.renderView.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
				return;
		}

		// Check if in light PVS
		if (!r_skipInteractionCulling->integerValue){
			if (!(lightSource->vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))))
				return;
		}
	}

	// Add all the surfaces
	for (i = 0, mark = leaf->firstMarkSurface; i < leaf->numMarkSurfaces; i++, mark++){
		surface = *mark;

		if (surface->viewCount != tr.viewCount)
			continue;		// Not visible

		if (surface->lightCount == tr.lightCount)
			continue;		// Already added this surface from another leaf
		surface->lightCount = tr.lightCount;

		if (lightSource->material->fogLight || lightSource->material->blendLight){
			if (surface->texInfo->material->noFog)
				continue;	// No interactions
		}
		else {
			if (surface->texInfo->material->numStages == surface->texInfo->material->numAmbientStages)
				continue;	// No interactions
		}

		if (surface->texInfo->material->spectrum > lightSource->material->spectrum)
			continue;		// Not illuminated by this light

		// Cull face
		if (!lightSource->material->fogLight && !lightSource->material->blendLight && !lightSource->material->ambientLight){
			if (surface->texInfo->material->cullType != CT_TWO_SIDED){
				side = PointOnPlaneSide(lightOrigin, 0.0, surface->plane);

				if (surface->texInfo->material->cullType == CT_BACK_SIDED){
					if (!(surface->flags & SURF_PLANEBACK)){
						if (side != SIDE_BACK)
							continue;
					}
					else {
						if (side != SIDE_FRONT)
							continue;
					}
				}
				else {
					if (!(surface->flags & SURF_PLANEBACK)){
						if (side != SIDE_FRONT)
							continue;
					}
					else {
						if (side != SIDE_BACK)
							continue;
					}
				}
			}
		}

		if (!r_skipInteractionCulling->integerValue){
			if (!R_LightIntersectsBounds(lightSource, surface->mins, surface->maxs))
				continue;	// Not in light volume
		}

		// Add the surface
		R_AddInteractionMeshToLight(light, MESH_SURFACE, surface, surface->texInfo->material, tr.worldEntity);
	}
}

/*
 =================
 R_AddWorldInteractions
 =================
*/
static void R_AddWorldInteractions (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin){

	if (!tr.renderViewParms.primaryView)
		return;

	if (!r_skipInteractionCulling->integerValue)
		R_RecursiveInteractionNode(light, lightSource, lightOrigin, tr.worldModel->nodes, 63);
	else
		R_RecursiveInteractionNode(light, lightSource, lightOrigin, tr.worldModel->nodes, 0);
}

/*
 =================
 R_AddDecorationInteractions
 =================
*/
static void R_AddDecorationInteractions (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin){

	decSurface_t	*surface;
	leaf_t			*leaf;
	int				i, j;

	if (r_skipDecorations->integerValue)
		return;

	if (!tr.renderViewParms.primaryView)
		return;

	for (i = 0, surface = tr.worldModel->decorationSurfaces; i < tr.worldModel->numDecorationSurfaces; i++, surface++){
		if (surface->viewCount != tr.viewCount)
			continue;		// Not visible

		if (lightSource->material->fogLight || lightSource->material->blendLight){
			if (surface->material->noFog)
				continue;	// No interactions
		}
		else {
			if (surface->material->numStages == surface->material->numAmbientStages)
				continue;	// No interactions
		}

		if (surface->material->spectrum > lightSource->material->spectrum)
			continue;		// Not illuminated by this light

		if (!r_skipInteractionCulling->integerValue){
			if (!R_LightIntersectsBounds(lightSource, surface->mins, surface->maxs))
				continue;	// Not in light volume

			// Check if in light PVS
			if (!lightSource->material->fogLight && !lightSource->material->blendLight && !lightSource->material->ambientLight){
				for (j = 0; j < surface->numLeafs; j++){
					leaf = surface->leafList[j];

					if (!(lightSource->vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))))
						continue;		// Not in PVS

					break;
				}

				if (j == surface->numLeafs)
					continue;
			}
		}

		// Add the surface
		R_AddInteractionMeshToLight(light, MESH_DECORATION, surface, surface->material, tr.worldEntity);
	}
}

/*
 =================
 R_AddBrushModelInteractions
 =================
*/
static void R_AddBrushModelInteractions (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin, renderEntity_t *entity){

	model_t		*model = entity->model;
	surface_t	*surface;
	vec3_t		origin, tmp;
	vec3_t		mins, maxs;
	int			i, side;

	if (!model->numModelSurfaces)
		return;		// Not visible

	// Cull
	if (!AxisCompare(entity->axis, axisDefault)){
		if (R_CullSphere(entity->origin, model->radius, 31))
			return;

		if (!r_skipInteractionCulling->integerValue){
			if (!R_LightIntersectsSphere(lightSource, entity->origin, model->radius))
				return;		// Not in light volume
		}

		VectorSubtract(lightOrigin, entity->origin, tmp);
		VectorRotate(tmp, entity->axis, origin);
	}
	else {
		VectorAdd(entity->origin, model->mins, mins);
		VectorAdd(entity->origin, model->maxs, maxs);

		if (R_CullBox(mins, maxs, 31))
			return;

		if (!r_skipInteractionCulling->integerValue){
			if (!R_LightIntersectsBounds(lightSource, mins, maxs))
				return;		// Not in light volume
		}

		VectorSubtract(lightOrigin, entity->origin, origin);
	}

	// Add all the surfaces
	surface = model->surfaces + model->firstModelSurface;
	for (i = 0; i < model->numModelSurfaces; i++, surface++){
		if (surface->viewCount != tr.viewCount)
			continue;		// Not visible

		if (lightSource->material->fogLight || lightSource->material->blendLight){
			if (surface->texInfo->material->noFog)
				continue;	// No interactions
		}
		else {
			if (surface->texInfo->material->numStages == surface->texInfo->material->numAmbientStages)
				continue;	// No interactions
		}

		if (surface->texInfo->material->spectrum > lightSource->material->spectrum)
			continue;		// Not illuminated by this light

		// Cull face
		if (!lightSource->material->fogLight && !lightSource->material->blendLight && !lightSource->material->ambientLight){
			if (surface->texInfo->material->cullType != CT_TWO_SIDED){
				side = PointOnPlaneSide(origin, 0.0, surface->plane);

				if (surface->texInfo->material->cullType == CT_BACK_SIDED){
					if (!(surface->flags & SURF_PLANEBACK)){
						if (side != SIDE_BACK)
							continue;
					}
					else {
						if (side != SIDE_FRONT)
							continue;
					}
				}
				else {
					if (!(surface->flags & SURF_PLANEBACK)){
						if (side != SIDE_FRONT)
							continue;
					}
					else {
						if (side != SIDE_BACK)
							continue;
					}
				}
			}
		}

		// Add the surface
		R_AddInteractionMeshToLight(light, MESH_SURFACE, surface, surface->texInfo->material, entity);
	}
}

/*
 =================
 R_AddAliasModelInteractions
 =================
*/
static void R_AddAliasModelInteractions (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin, renderEntity_t *entity){

	mdl_t			*alias = entity->model->alias;
	mdlFrame_t		*curFrame, *oldFrame;
	mdlSurface_t	*surface;
	material_t		*material;
	float			radius;
	int				i;

	if (entity->renderFX & RF_VIEWERMODEL){
		if (!r_skipSuppress->integerValue && tr.renderViewParms.subview == SUBVIEW_NONE)
			return;
	}
	if (entity->renderFX & RF_WEAPONMODEL){
		if (!r_skipSuppress->integerValue && tr.renderViewParms.subview != SUBVIEW_NONE)
			return;
	}

	// Check if this entity emits this light
	if (VectorCompare(lightOrigin, entity->origin))
		return;

	// Find model radius
	if ((entity->frame < 0 || entity->frame >= alias->numFrames) || (entity->oldFrame < 0 || entity->oldFrame >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelInteractions: no such frame %i to %i (%s)\n", entity->frame, entity->oldFrame, entity->model->realName);

		entity->frame = 0;
		entity->oldFrame = 0;
	}

	curFrame = alias->frames + entity->frame;
	oldFrame = alias->frames + entity->oldFrame;

	if (curFrame == oldFrame)
		radius = curFrame->radius;
	else {
		if (curFrame->radius > oldFrame->radius)
			radius = curFrame->radius;
		else
			radius = oldFrame->radius;
	}

	// Cull
	if (R_CullSphere(entity->origin, radius, 31))
		return;

	if (!r_skipInteractionCulling->integerValue){
		if (!R_LightIntersectsSphere(lightSource, entity->origin, radius))
			return;		// Not in light volume
	}

	// Add all the surfaces
	for (i = 0, surface = alias->surfaces; i < alias->numSurfaces; i++, surface++){
		// Select material
		if (entity->customMaterial)
			material = entity->customMaterial;
		else {
			if (surface->numMaterials){
				if (entity->skinIndex < 0 || entity->skinIndex >= surface->numMaterials){
					Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelInteractions: no such material %i (%s)\n", entity->skinIndex, entity->model->realName);

					entity->skinIndex = 0;
				}

				material = surface->materials[entity->skinIndex].material;
			}
			else {
				Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelInteractions: no materials for surface (%s)\n", entity->model->realName);

				material = tr.defaultMaterial;
			}
		}

		if (!material->numStages)
			continue;

		if (lightSource->material->fogLight || lightSource->material->blendLight){
			if (material->noFog)
				continue;	// No interactions
		}
		else {
			if (material->numStages == material->numAmbientStages)
				continue;	// No interactions
		}

		if (material->spectrum > lightSource->material->spectrum)
			continue;		// Not illuminated by this light

		// Add the surface
		R_AddInteractionMeshToLight(light, MESH_ALIASMODEL, surface, material, entity);
	}
}

/*
 =================
 R_AddEntityInteractions
 =================
*/
static void R_AddEntityInteractions (light_t *light, lightSource_t *lightSource, const vec3_t lightOrigin){

	renderEntity_t	*entity;
	model_t			*model;
	int				i;

	if (r_skipEntities->integerValue)
		return;

	for (i = 0, entity = tr.renderViewParms.renderEntities; i < tr.renderViewParms.numRenderEntities; i++, entity++){
		if (entity->reType != RE_MODEL)
			continue;

		model = entity->model;

		if (!model || model->modelType == MODEL_BAD)
			continue;

		switch (model->modelType){
		case MODEL_BSP:
			R_AddBrushModelInteractions(light, lightSource, lightOrigin, entity);
			break;
		case MODEL_MD3:
		case MODEL_MD2:
			R_AddAliasModelInteractions(light, lightSource, lightOrigin, entity);
			break;
		}
	}
}

/*
 =================
 R_CreateInteractionsList
 =================
*/
static void R_CreateInteractionsList (light_t *light, lightSource_t *lightSource){

	vec3_t	lightOrigin;
	double	time;

	tr.lightCount++;

	light->numInteractions[0] = light->numInteractions[1] = 0;

	if (r_showInteractions->integerValue)
		time = Sys_GetClockTicks();

	if (lightSource->parallel)
		VectorMA(lightSource->origin, 999999, lightSource->center, lightOrigin);
	else
		VectorAdd(lightSource->origin, lightSource->center, lightOrigin);

	R_AddWorldInteractions(light, lightSource, lightOrigin);
	R_AddDecorationInteractions(light, lightSource, lightOrigin);
	R_AddEntityInteractions(light, lightSource, lightOrigin);

	if (light->numInteractions[0]){
		light->interactions[0] = Hunk_HighAlloc(light->numInteractions[0] * sizeof(mesh_t));
		memcpy(light->interactions[0], r_interactionsList[0], light->numInteractions[0] * sizeof(mesh_t));
	}

	if (light->numInteractions[1]){
		light->interactions[1] = Hunk_HighAlloc(light->numInteractions[1] * sizeof(mesh_t));
		memcpy(light->interactions[1], r_interactionsList[1], light->numInteractions[1] * sizeof(mesh_t));
	}

	if (r_showInteractions->integerValue)
		tr.pc.timeInteractions += (Sys_GetClockTicks() - time);
}


// =====================================================================


/*
 =================
 R_AddLightToList
 =================
*/
static void R_AddLightToList (lightSource_t *lightSource){

	viewData_t	*viewData = &tr.viewData;
	light_t		*light;
	vec3_t		axis[3], origin;
	vec3_t		edge[2];

	if (lightSource->material->fogLight || lightSource->material->blendLight){
		if (viewData->numFogLights == MAX_FOG_LIGHTS)
			Com_Error(ERR_DROP, "R_AddLightToList: MAX_FOG_LIGHTS hit");

		light = &viewData->fogLights[viewData->numFogLights++];
	}
	else {
		if (viewData->numLights == MAX_LIGHTS)
			Com_Error(ERR_DROP, "R_AddLightToList: MAX_LIGHTS hit");

		light = &viewData->lights[viewData->numLights++];
	}

	if (lightSource->parallel)
		VectorMA(lightSource->origin, 999999, lightSource->center, light->origin);
	else
		VectorAdd(lightSource->origin, lightSource->center, light->origin);

	light->material = lightSource->material;

	light->materialParms[0] = lightSource->materialParms[0] * tr.lightStyles[lightSource->style].rgb[0];
	light->materialParms[1] = lightSource->materialParms[1] * tr.lightStyles[lightSource->style].rgb[1];
	light->materialParms[2] = lightSource->materialParms[2] * tr.lightStyles[lightSource->style].rgb[2];
	light->materialParms[3] = lightSource->materialParms[3];
	light->materialParms[4] = lightSource->materialParms[4];
	light->materialParms[5] = lightSource->materialParms[5];
	light->materialParms[6] = lightSource->materialParms[6];
	light->materialParms[7] = lightSource->materialParms[7];

	VectorCopy(lightSource->corners[0], light->corners[0]);
	VectorCopy(lightSource->corners[1], light->corners[1]);
	VectorCopy(lightSource->corners[2], light->corners[2]);
	VectorCopy(lightSource->corners[3], light->corners[3]);
	VectorCopy(lightSource->corners[4], light->corners[4]);
	VectorCopy(lightSource->corners[5], light->corners[5]);
	VectorCopy(lightSource->corners[6], light->corners[6]);
	VectorCopy(lightSource->corners[7], light->corners[7]);

	// Compute visible fog plane or transformation matrix
	if (lightSource->material->fogLight){
		VectorSubtract(lightSource->corners[3], lightSource->corners[2], edge[0]);
		VectorSubtract(lightSource->corners[1], lightSource->corners[2], edge[1]);
		CrossProduct(edge[1], edge[0], light->planeNormal);
		VectorNormalize(light->planeNormal);

		light->planeDist = DotProduct(lightSource->corners[0], light->planeNormal);
	}
	else {
		VectorScale(lightSource->axis[0], 0.5 / lightSource->radius[0], axis[0]);
		VectorScale(lightSource->axis[1], 0.5 / lightSource->radius[1], axis[1]);
		VectorScale(lightSource->axis[2], 0.5 / lightSource->radius[2], axis[2]);

		VectorNegate(lightSource->origin, origin);

		light->matrix[ 0] = axis[0][0];
		light->matrix[ 1] = axis[0][1];
		light->matrix[ 2] = axis[0][2];
		light->matrix[ 3] = 0.0;
		light->matrix[ 4] = axis[1][0];
		light->matrix[ 5] = axis[1][1];
		light->matrix[ 6] = axis[1][2];
		light->matrix[ 7] = 0.0;
		light->matrix[ 8] = axis[2][0];
		light->matrix[ 9] = axis[2][1];
		light->matrix[10] = axis[2][2];
		light->matrix[11] = 0.0;
		light->matrix[12] = (axis[0][0] * origin[0] + axis[1][0] * origin[1] + axis[2][0] * origin[2]) + 0.5;
		light->matrix[13] = (axis[0][1] * origin[0] + axis[1][1] * origin[1] + axis[2][1] * origin[2]) + 0.5;
		light->matrix[14] = (axis[0][2] * origin[0] + axis[1][2] * origin[1] + axis[2][2] * origin[2]) + 0.5;
		light->matrix[15] = 1.0;
	}

	// Compute scissor rect
	R_SetScissorRect(light, lightSource);

	// Compute depth bounds
	R_SetDepthBounds(light, lightSource);

	// Create shadows list
	R_CreateShadowsList(light, lightSource);

	// Create interactions list
	R_CreateInteractionsList(light, lightSource);
}


// =====================================================================


static void R_LightFrustum (lightSource_t *light){

	int		i;
	float	dot;
	vec3_t	tmp;

	for (i = 0; i < 3; i++){
		dot = DotProduct(light->origin, light->axis[i]);

		VectorCopy(light->axis[i], light->frustum[i*2+0].normal);
		light->frustum[i*2+0].dist = dot - light->radius[i];
		light->frustum[i*2+0].type = PlaneTypeForNormal(light->frustum[i*2+0].normal);
		SetPlaneSignbits(&light->frustum[i*2+0]);

		VectorNegate(light->axis[i], light->frustum[i*2+1].normal);
		light->frustum[i*2+1].dist = -dot - light->radius[i];
		light->frustum[i*2+1].type = PlaneTypeForNormal(light->frustum[i*2+1].normal);
		SetPlaneSignbits(&light->frustum[i*2+1]);
	}

	// Compute the corners of the bounding volume
	for (i = 0; i < 8; i++){
		tmp[0] = (i & 1) ? -light->radius[0] : light->radius[0];
		tmp[1] = (i & 2) ? -light->radius[1] : light->radius[1];
		tmp[2] = (i & 4) ? -light->radius[2] : light->radius[2];

		// Rotate and translate
		VectorRotate(tmp, light->axis, light->corners[i]);
		VectorAdd(light->corners[i], light->origin, light->corners[i]);
	}
}

static qboolean R_CullLight (lightSource_t *lightSource){

	cplane_t	*plane;
	int			mask, aggregateMask = ~0;
	int			i, j;

	// Cull
	for (i = 0; i < 8; i++){
		mask = 0;

		for (j = 0, plane = tr.renderViewParms.frustum; j < 5; j++, plane++){
			if (DotProduct(lightSource->corners[i], plane->normal) < plane->dist)
				mask |= (1<<j);
		}

		aggregateMask &= mask;
	}

	if (aggregateMask){
		tr.pc.boxOut++;
		return true;
	}

	tr.pc.boxIn++;

	return false;
}

// FIXME: fog/blend/ambient light PVS culling
// TODO: rewrite this mess!
void R_AddLights (void){

	lightSource_t	*lightSource;
	int				i, j;

	leaf_t		*leaf;
	byte		vis[MAX_MAP_LEAFS/8];
	renderLight_t	*dl;

	double	t = Sys_GetClockTicks();

	if (r_skipLights->integerValue)
		return;

	if (!tr.renderViewParms.primaryView)
		goto dynamicLights;		// TODO: get rid of this later

	if (r_showLights->integerValue > 1)
		Com_Printf("volumes:");

	for (i = 0, lightSource = r_staticLights; i < r_numStaticLights; i++, lightSource++){
		if (lightSource->detailLevel < r_lightDetailLevel->integerValue)
			continue;		// LOD'ed out

		if (!R_LightIntersectsPoint(lightSource, tr.renderView.viewOrigin)){
			if (!r_skipLightCulling->integerValue){
				if (!R_LightIntersectsBounds(lightSource, tr.renderViewParms.visMins, tr.renderViewParms.visMaxs))
					continue;		// No intersection

				// Cull
				if (R_CullLight(lightSource))
					continue;
			}

			if (!lightSource->material->fogLight && !lightSource->material->blendLight && !lightSource->material->ambientLight){
				// Check for door connected areas
				if (!r_skipAreas->integerValue && tr.renderView.areaBits && lightSource->leaf->cluster != -1){
					if (!(tr.renderView.areaBits[lightSource->leaf->area>>3] & (1<<(lightSource->leaf->area&7))))
						continue;
				}

				// Check the PVS
				R_ClusterPVS(lightSource->leaf->cluster, vis);

				if (!r_skipVisibility->integerValue && lightSource->leaf->visCount != tr.visCount){
					for (j = 0, leaf = tr.worldModel->leafs; j < tr.worldModel->numLeafs; j++, leaf++){
						if (leaf->visCount != tr.visCount)
							continue;

						if (vis[leaf->cluster>>3] & (1<<(leaf->cluster&7)))
							break;
					}

					if (j == tr.worldModel->numLeafs)
						continue;
				}
			}
		}
		else {
			if (!lightSource->material->fogLight && !lightSource->material->blendLight && !lightSource->material->ambientLight){
				// Check for door connected areas
				if (!r_skipAreas->integerValue && tr.renderView.areaBits && lightSource->leaf->cluster != -1){
					if (!(tr.renderView.areaBits[lightSource->leaf->area>>3] & (1<<(lightSource->leaf->area&7))))
						continue;
				}

				R_ClusterPVS(lightSource->leaf->cluster, vis);
			}
		}

		lightSource->vis = vis;

		// Development tool
		if (r_showLights->integerValue > 1)
			Com_Printf(" %i", lightSource->index);

		if (r_singleLight->integerValue >= 0){
			if (r_singleLight->integerValue != lightSource->index)
				continue;
		}

		// Add the light
		R_AddLightToList(lightSource);

		tr.pc.lights++;
		tr.pc.lightsStatic++;
	}

	if (r_showLights->integerValue > 1)
		Com_Printf("\n");

dynamicLights:

	if (!r_dynamicLights->integerValue){
		if (r_showLights->integerValue)
			tr.pc.timeLights += (Sys_GetClockTicks() - t);

		return;
	}

	for (i = 0, dl = tr.renderViewParms.renderLights; i < tr.renderViewParms.numRenderLights; i++, dl++){
		lightSource_t	newLight;

		lightSource = &newLight;

		VectorCopy(dl->origin, lightSource->origin);
		VectorCopy(dl->center, lightSource->center);
		VectorClear(lightSource->angles);
		VectorCopy(dl->radius, lightSource->radius);
		lightSource->style = dl->style;
		lightSource->detailLevel = dl->detailLevel;
		lightSource->parallel = dl->parallel;
		lightSource->noShadows = dl->noShadows;

		if (dl->customMaterial)
			lightSource->material = dl->customMaterial;
		else
			lightSource->material = tr.defaultLightMaterial;

		lightSource->materialParms[0] = dl->materialParms[0];
		lightSource->materialParms[1] = dl->materialParms[1];
		lightSource->materialParms[2] = dl->materialParms[2];
		lightSource->materialParms[3] = dl->materialParms[3];
		lightSource->materialParms[4] = dl->materialParms[4];
		lightSource->materialParms[5] = dl->materialParms[5];
		lightSource->materialParms[6] = dl->materialParms[6];
		lightSource->materialParms[7] = dl->materialParms[7];

		AxisCopy(dl->axis, lightSource->axis);

		VectorSubtract(dl->origin, dl->radius, lightSource->mins);
		VectorAdd(dl->origin, dl->radius, lightSource->maxs);

		lightSource->rotated = false;
		R_LightFrustum(lightSource);

		if (tr.renderViewParms.primaryView)
			lightSource->leaf = R_PointInLeaf(lightSource->origin);

		if (lightSource->detailLevel < r_lightDetailLevel->integerValue)
			continue;		// LOD'ed out

		if (!tr.renderViewParms.primaryView){
			// Add the light
			R_AddLightToList(lightSource);

			tr.pc.lights++;
			tr.pc.lightsDynamic++;
			continue;
		}

		if (!R_LightIntersectsPoint(lightSource, tr.renderView.viewOrigin)){
			if (!r_skipLightCulling->integerValue){
				if (!R_LightIntersectsBounds(lightSource, tr.renderViewParms.visMins, tr.renderViewParms.visMaxs))
					continue;		// No intersection

				// Cull
				if (R_CullLight(lightSource))
					continue;
			}

			if (!lightSource->material->fogLight && !lightSource->material->blendLight && !lightSource->material->ambientLight){
				// Check for door connected areas
				if (!r_skipAreas->integerValue && tr.renderView.areaBits && lightSource->leaf->cluster != -1){
					if (!(tr.renderView.areaBits[lightSource->leaf->area>>3] & (1<<(lightSource->leaf->area&7))))
						continue;
				}

				// Check the PVS
				R_ClusterPVS(lightSource->leaf->cluster, vis);

				if (lightSource->leaf->visCount != tr.visCount){
					for (j = 0, leaf = tr.worldModel->leafs; j < tr.worldModel->numLeafs; j++, leaf++){
						if (leaf->visCount != tr.visCount)
							continue;

						if (vis[leaf->cluster>>3] & (1<<(leaf->cluster&7)))
							break;
					}

					if (j == tr.worldModel->numLeafs)
						continue;
				}
			}
		}
		else {
			if (!lightSource->material->fogLight && !lightSource->material->blendLight && !lightSource->material->ambientLight){
				// Check for door connected areas
				if (!r_skipAreas->integerValue && tr.renderView.areaBits && lightSource->leaf->cluster != -1){
					if (!(tr.renderView.areaBits[lightSource->leaf->area>>3] & (1<<(lightSource->leaf->area&7))))
						continue;
				}

				R_ClusterPVS(lightSource->leaf->cluster, vis);
			}
		}

		lightSource->vis = vis;

		// Add the light
		R_AddLightToList(lightSource);

		tr.pc.lights++;
		tr.pc.lightsDynamic++;
	}

	if (r_showLights->integerValue)
		tr.pc.timeLights += (Sys_GetClockTicks() - t);
}


/*
 =======================================================================

 INTEGRATED LIGHT EDITOR

 =======================================================================
*/

typedef struct {
	qboolean		active;

	lightSource_t	*lights[MAX_STATIC_LIGHTS];
	int				numLights;

	lightSource_t	*focusLight;
	lightSource_t	*editLight;

	mat4_t			modelviewMatrix;
} lightEditor_t;

static lightEditor_t	r_lightEditor;


/*
 =================
 R_UpdateLight

 Called by the light properties editor to update the current light
 =================
*/
static void R_UpdateLight (lightProperties_t *lightProperties){

	lightSource_t	*lightSource = r_lightEditor.editLight;
	vec3_t			tmp;
	float			dot;
	int				i;

	if (!r_lightEditor.active)
		return;

	if (!r_lightEditor.editLight)
		return;

	// Set the properties
	VectorCopy(lightProperties->origin, lightSource->origin);
	VectorCopy(lightProperties->center, lightSource->center);
	VectorCopy(lightProperties->angles, lightSource->angles);
	VectorCopy(lightProperties->radius, lightSource->radius);
	lightSource->style = lightProperties->style;
	lightSource->detailLevel = lightProperties->detailLevel;
	lightSource->parallel = lightProperties->parallel;
	lightSource->noShadows = lightProperties->noShadows;
	lightSource->material = R_FindMaterial(lightProperties->material, MT_LIGHT, 0);
	lightSource->materialParms[0] = lightProperties->materialParms[0];
	lightSource->materialParms[1] = lightProperties->materialParms[1];
	lightSource->materialParms[2] = lightProperties->materialParms[2];
	lightSource->materialParms[3] = lightProperties->materialParms[3];
	lightSource->materialParms[4] = lightProperties->materialParms[4];
	lightSource->materialParms[5] = lightProperties->materialParms[5];
	lightSource->materialParms[6] = lightProperties->materialParms[6];
	lightSource->materialParms[7] = lightProperties->materialParms[7];

	// Compute axes
	AnglesToAxis(lightProperties->angles, lightSource->axis);

	// Compute the corners of the bounding volume
	for (i = 0; i < 8; i++){
		tmp[0] = (i & 1) ? -lightSource->radius[0] : lightSource->radius[0];
		tmp[1] = (i & 2) ? -lightSource->radius[1] : lightSource->radius[1];
		tmp[2] = (i & 4) ? -lightSource->radius[2] : lightSource->radius[2];

		// Rotate and translate
		VectorRotate(tmp, lightSource->axis, lightSource->corners[i]);
		VectorAdd(lightSource->corners[i], lightSource->origin, lightSource->corners[i]);
	}

	// Check if it's rotated
	lightSource->rotated = !VectorCompare(lightProperties->angles, vec3_origin);

	// Compute mins/maxs
	VectorSubtract(lightSource->origin, lightSource->radius, lightSource->mins);
	VectorAdd(lightSource->origin, lightSource->radius, lightSource->maxs);

	// Compute frustum planes
	for (i = 0; i < 3; i++){
		dot = DotProduct(lightSource->origin, lightSource->axis[i]);

		VectorCopy(lightSource->axis[i], lightSource->frustum[i*2+0].normal);
		lightSource->frustum[i*2+0].dist = dot - lightSource->radius[i];
		lightSource->frustum[i*2+0].type = PlaneTypeForNormal(lightSource->frustum[i*2+0].normal);
		SetPlaneSignbits(&lightSource->frustum[i*2+0]);

		VectorNegate(lightSource->axis[i], lightSource->frustum[i*2+1].normal);
		lightSource->frustum[i*2+1].dist = -dot - lightSource->radius[i];
		lightSource->frustum[i*2+1].type = PlaneTypeForNormal(lightSource->frustum[i*2+1].normal);
		SetPlaneSignbits(&lightSource->frustum[i*2+1]);
	}

	// Find the leaf
	VectorAdd(lightSource->origin, lightSource->center, tmp);

	lightSource->leaf = R_PointInLeaf(tmp);
}

/*
 =================
 R_RemoveLight

 Called by the light properties editor to remove the current light
 =================
*/
static void R_RemoveLight (void){

	lightSource_t	*lightSource;
	int				i, j;

	if (!r_lightEditor.active)
		return;

	if (!r_lightEditor.editLight)
		return;

	for (i = 0, lightSource = r_staticLights; i < r_numStaticLights; i++, lightSource++){
		if (lightSource != r_lightEditor.editLight)
			continue;

		for (j = i; j < r_numStaticLights - 1; j++)
			memcpy(&r_staticLights[j], &r_staticLights[j+1], sizeof(lightSource_t));

		r_numStaticLights--;
		break;
	}

	for (i = 0, lightSource = r_staticLights; i < r_numStaticLights; i++, lightSource++)
		lightSource->index = i;

	// No light is being edited now
	r_lightEditor.editLight = NULL;
}

/*
 =================
 R_CloseLight

 Called by the light properties editor when closed
 =================
*/
static void R_CloseLight (void){

	if (!r_lightEditor.active)
		return;

	if (!r_lightEditor.editLight)
		return;

	// No light is being edited now
	r_lightEditor.editLight = NULL;
}

/*
 =================
 R_EditLight

 Called by the client system for mouse left-button down events.
 Returns true if a light has been selected for editing, in which case
 the client should skip game commands for said event.
 =================
*/
qboolean R_EditLight (void){

	lightProperties_t	lightProperties;

	if (!r_lightEditor.active)
		return false;	// Not active

	if (!r_lightEditor.focusLight)
		return false;	// No light has focus

	// If already editing a light, apply the changes
	if (r_lightEditor.editLight)
		E_ApplyLightProperties();

	// Edit the light that has focus
	r_lightEditor.editLight = r_lightEditor.focusLight;

	VectorCopy(r_lightEditor.editLight->origin, lightProperties.origin);
	VectorCopy(r_lightEditor.editLight->center, lightProperties.center);
	VectorCopy(r_lightEditor.editLight->angles, lightProperties.angles);
	VectorCopy(r_lightEditor.editLight->radius, lightProperties.radius);
	lightProperties.style = r_lightEditor.editLight->style;
	lightProperties.detailLevel = r_lightEditor.editLight->detailLevel;
	lightProperties.parallel = r_lightEditor.editLight->parallel;
	lightProperties.noShadows = r_lightEditor.editLight->noShadows;
	Q_strncpyz(lightProperties.material, r_lightEditor.editLight->material->name, sizeof(lightProperties.material));
	lightProperties.materialParms[0] = r_lightEditor.editLight->materialParms[0];
	lightProperties.materialParms[1] = r_lightEditor.editLight->materialParms[1];
	lightProperties.materialParms[2] = r_lightEditor.editLight->materialParms[2];
	lightProperties.materialParms[3] = r_lightEditor.editLight->materialParms[3];
	lightProperties.materialParms[4] = r_lightEditor.editLight->materialParms[4];
	lightProperties.materialParms[5] = r_lightEditor.editLight->materialParms[5];
	lightProperties.materialParms[6] = r_lightEditor.editLight->materialParms[6];
	lightProperties.materialParms[7] = r_lightEditor.editLight->materialParms[7];

	E_EditLightProperties(&lightProperties);

	Com_Printf("Editing light #%i\n", r_lightEditor.editLight->index);

	return true;
}

/*
 =================
 R_AddEditorLights
 =================
*/
void R_AddEditorLights (void){

	lightSource_t	*lightSource;
	trace_t			trace;
	cplane_t		*plane;
	vec3_t			mins = {-5, -5, -5}, maxs = {5, 5, 5};
	vec3_t			start, end;
	float			fraction = 1.0;
	int				headNode;
	int				i, j;

	if (!r_lightEditor.active)
		return;		// Not active

	if (!tr.renderViewParms.primaryView)
		return;		// Ignore non-primary views

	if (tr.renderViewParms.subview != SUBVIEW_NONE)
		return;		// Ignore subviews

	Matrix4_Copy(tr.renderViewParms.worldMatrix, r_lightEditor.modelviewMatrix);

	r_lightEditor.numLights = 0;

	// Find visible lights
	for (i = 0, lightSource = r_staticLights; i < r_numStaticLights; i++, lightSource++){
		// Always add the light that is being edited
		if (lightSource == r_lightEditor.editLight){
			r_lightEditor.lights[r_lightEditor.numLights++] = lightSource;
			continue;
		}

		// Frustum cull
		for (j = 0, plane = tr.renderViewParms.frustum; j < 5; j++, plane++){
			if (SphereOnPlaneSide(lightSource->origin, 10, plane) == SIDE_BACK)
				break;
		}

		if (j != 5)
			continue;

		// Distance cull
		if (Distance(lightSource->origin, tr.renderView.viewOrigin) > 512.0)
			continue;

		// Add it to the list
		r_lightEditor.lights[r_lightEditor.numLights++] = lightSource;
	}

	// Find the light that has focus, if any
	r_lightEditor.focusLight = NULL;

	VectorCopy(tr.renderView.viewOrigin, start);
	VectorMA(tr.renderView.viewOrigin, 8192, tr.renderView.viewAxis[0], end);

	headNode = CM_HeadNodeForBox(mins, maxs);

	for (i = 0; i < r_lightEditor.numLights; i++){
		lightSource = r_lightEditor.lights[i];

		trace = CM_TransformedBoxTrace(start, end, vec3_origin, vec3_origin, headNode, MASK_ALL, lightSource->origin, lightSource->angles);
		if (trace.fraction < fraction){
			fraction = trace.fraction;

			// This light has focus
			r_lightEditor.focusLight = lightSource;
		}
	}
}

/*
 =================
 R_DrawEditorLights
 =================
*/
void R_DrawEditorLights (void){

	lightSource_t	*lightSource;
	vec3_t			corners[8], tmp;
	int				i, j;

	if (!r_lightEditor.active)
		return;		// Not active

	if (!r_lightEditor.numLights && !r_lightEditor.focusLight && !r_lightEditor.editLight)
		return;		// No lights

	// Set the GL state
	GL_LoadMatrix(GL_MODELVIEW, r_lightEditor.modelviewMatrix);
	GL_DisableTexture();
	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);
	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);
	GL_DepthRange(0.0, 1.0);

	// Draw all lights
	for (i = 0; i < r_lightEditor.numLights; i++){
		lightSource = r_lightEditor.lights[i];

		// Create a small box
		for (j = 0; j < 8; j++){
			tmp[0] = (j & 1) ? -5 : 5;
			tmp[1] = (j & 2) ? -5 : 5;
			tmp[2] = (j & 4) ? -5 : 5;

			// Rotate and translate
			VectorRotate(tmp, lightSource->axis, corners[j]);
			VectorAdd(corners[j], lightSource->origin, corners[j]);
		}

		// Set the color
		if (lightSource == r_lightEditor.focusLight || lightSource == r_lightEditor.editLight)
			qglColor3f(1.0, 0.0, 0.0);
		else
			qglColor3f(1.0, 1.0, 1.0);

		// Draw it
		qglBegin(GL_LINE_LOOP);
		qglVertex3fv(corners[0]);
		qglVertex3fv(corners[2]);
		qglVertex3fv(corners[3]);
		qglVertex3fv(corners[1]);
		qglEnd();

		qglBegin(GL_LINE_LOOP);
		qglVertex3fv(corners[4]);
		qglVertex3fv(corners[6]);
		qglVertex3fv(corners[7]);
		qglVertex3fv(corners[5]);
		qglEnd();

		qglBegin(GL_LINES);
		qglVertex3fv(corners[0]);
		qglVertex3fv(corners[4]);
		qglVertex3fv(corners[1]);
		qglVertex3fv(corners[5]);
		qglVertex3fv(corners[2]);
		qglVertex3fv(corners[6]);
		qglVertex3fv(corners[3]);
		qglVertex3fv(corners[7]);
		qglEnd();

		// If editing this light, draw the light center
		if (lightSource == r_lightEditor.editLight){
			// Create a small box
			for (j = 0; j < 8; j++){
				tmp[0] = (j & 1) ? -2 : 2;
				tmp[1] = (j & 2) ? -2 : 2;
				tmp[2] = (j & 4) ? -2 : 2;

				// Rotate and translate
				VectorRotate(tmp, lightSource->axis, corners[j]);
				VectorAdd(corners[j], lightSource->origin, corners[j]);
				VectorAdd(corners[j], lightSource->center, corners[j]);
			}

			// Set the color
			qglColor3f(1.0, 0.0, 0.0);

			// Draw it
			qglBegin(GL_LINE_LOOP);
			qglVertex3fv(corners[0]);
			qglVertex3fv(corners[2]);
			qglVertex3fv(corners[3]);
			qglVertex3fv(corners[1]);
			qglEnd();

			qglBegin(GL_LINE_LOOP);
			qglVertex3fv(corners[4]);
			qglVertex3fv(corners[6]);
			qglVertex3fv(corners[7]);
			qglVertex3fv(corners[5]);
			qglEnd();

			qglBegin(GL_LINES);
			qglVertex3fv(corners[0]);
			qglVertex3fv(corners[4]);
			qglVertex3fv(corners[1]);
			qglVertex3fv(corners[5]);
			qglVertex3fv(corners[2]);
			qglVertex3fv(corners[6]);
			qglVertex3fv(corners[3]);
			qglVertex3fv(corners[7]);
			qglEnd();
		}

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();
	}
}

/*
 =================
 R_EditLights_f
 =================
*/
static void R_EditLights_f (void){

	if (!tr.worldModel){
		Com_Printf("You must be in a map to use the light editor\n");
		return;
	}

	if (!Com_AllowCheats()){
		Com_Printf("You must enable cheats to use the light editor\n");
		return;
	}

	if (glConfig.isFullscreen){
		Com_Printf("The light editor can only be used in windowed mode\n");
		return;
	}

	if (r_lightEditor.active){
		if (r_lightEditor.editLight){
			Com_Printf("Still editing a light\n");
			return;
		}

		r_lightEditor.active = false;

		r_lightEditor.focusLight = NULL;
		r_lightEditor.editLight = NULL;

		CL_SetKeyEditMode(false);

		E_DestroyLightPropertiesWindow();

		Com_Printf("Light editor deactivated\n");
		return;
	}

	r_lightEditor.active = true;

	E_CreateLightPropertiesWindow();

	E_SetLightPropertiesCallbacks(R_UpdateLight, R_RemoveLight, R_CloseLight);

	R_EnumMaterialScripts(E_AddLightPropertiesMaterial);

	CL_SetKeyEditMode(true);

	Com_Printf("Light editor activated\n");
}

/*
 =================
 R_ClearLights_f
 =================
*/
static void R_ClearLights_f (void){

	if (!r_lightEditor.active){
		Com_Printf("The light editor is not active\n");
		return;
	}

	if (r_lightEditor.editLight){
		Com_Printf("Still editing a light\n");
		return;
	}

	// Remove all the lights
	Com_Printf("Cleared %i lights\n", r_numStaticLights);

	r_numStaticLights = 0;
}

/*
 =================
 R_SaveLights_f
 =================
*/
static void R_SaveLights_f (void){

	fileHandle_t	f;
	lightSource_t	*lightSource;
	char			name[MAX_OSPATH];
	int				i;

	if (!r_lightEditor.active){
		Com_Printf("The light editor is not active\n");
		return;
	}

	if (r_lightEditor.editLight){
		Com_Printf("Still editing a light\n");
		return;
	}

	if (!r_numStaticLights){
		Com_Printf("No lights!\n");
		return;
	}

	// Write the script file
	Q_snprintfz(name, sizeof(name), "%s.txt", tr.worldModel->name);

	FS_OpenFile(name, &f, FS_WRITE);
	if (!f){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}

	for (i = 0, lightSource = r_staticLights; i < r_numStaticLights; i++, lightSource++){
		FS_Printf(f, "{\r\n");

		FS_Printf(f, "origin %g %g %g\r\n", lightSource->origin[0], lightSource->origin[1], lightSource->origin[2]);

		if (lightSource->center[0] != 0.0 || lightSource->center[1] != 0.0 || lightSource->center[2] != 0.0)
			FS_Printf(f, "center %g %g %g\r\n", lightSource->center[0], lightSource->center[1], lightSource->center[2]);

		if (lightSource->angles[0] != 0.0 || lightSource->angles[1] != 0.0 || lightSource->angles[2] != 0.0)
			FS_Printf(f, "angles %g %g %g\r\n", lightSource->angles[0], lightSource->angles[1], lightSource->angles[2]);

		if (lightSource->radius[0] != 100.0 || lightSource->radius[1] != 100.0 || lightSource->radius[2] != 100.0)
			FS_Printf(f, "radius %g %g %g\r\n", lightSource->radius[0], lightSource->radius[1], lightSource->radius[2]);

		if (lightSource->style != 0)
			FS_Printf(f, "style %i\r\n", lightSource->style);

		if (lightSource->detailLevel != 10)
			FS_Printf(f, "detailLevel %i\r\n", lightSource->detailLevel);

		if (lightSource->parallel)
			FS_Printf(f, "parallel\r\n");

		if (lightSource->noShadows)
			FS_Printf(f, "noShadows\r\n");

		if (lightSource->material != tr.defaultLightMaterial)
			FS_Printf(f, "material %s\r\n", lightSource->material->name);

		if (lightSource->materialParms[0] != 1.0 || lightSource->materialParms[1] != 1.0 || lightSource->materialParms[2] != 1.0)
			FS_Printf(f, "color %g %g %g\r\n", lightSource->materialParms[0], lightSource->materialParms[1], lightSource->materialParms[2]);

		if (lightSource->materialParms[3] != 1.0)
			FS_Printf(f, "materialParm3 %g\r\n", lightSource->materialParms[3]);

		if (lightSource->materialParms[4] != 0.0)
			FS_Printf(f, "materialParm4 %g\r\n", lightSource->materialParms[4]);

		if (lightSource->materialParms[5] != 0.0)
			FS_Printf(f, "materialParm5 %g\r\n", lightSource->materialParms[5]);

		if (lightSource->materialParms[6] != 0.0)
			FS_Printf(f, "materialParm6 %g\r\n", lightSource->materialParms[6]);

		if (lightSource->materialParms[7] != 0.0)
			FS_Printf(f, "materialParm7 %g\r\n", lightSource->materialParms[7]);

		FS_Printf(f, "}\r\n");
	}

	FS_CloseFile(f);

	Com_Printf("Wrote %s with %i lights\n", name, r_numStaticLights);
}

/*
 =================
 R_AddLight_f
 =================
*/
static void R_AddLight_f (void){

	lightSource_t		*lightSource;
	lightProperties_t	lightProperties;
	vec3_t				tmp;
	float				dot;
	int					i;

	if (!r_lightEditor.active){
		Com_Printf("The light editor is not active\n");
		return;
	}

	// Add a new light in front of the camera
	if (r_numStaticLights == MAX_STATIC_LIGHTS){
		Com_Printf("Too many lights!\n");
		return;
	}

	lightSource = &r_staticLights[r_numStaticLights++];

	// Set the properties
	VectorMA(tr.renderView.viewOrigin, 100, tr.renderView.viewAxis[0], lightSource->origin);

	lightSource->index = r_numStaticLights - 1;
	SnapVector(lightSource->origin);
	VectorClear(lightSource->center);
	VectorClear(lightSource->angles);
	VectorSet(lightSource->radius, 100, 100, 100);
	lightSource->style = 0;
	lightSource->detailLevel = 10;
	lightSource->parallel = false;
	lightSource->noShadows = false;
	lightSource->material = tr.defaultLightMaterial;
	lightSource->materialParms[MATERIALPARM_RED] = 1.0;
	lightSource->materialParms[MATERIALPARM_GREEN] = 1.0;
	lightSource->materialParms[MATERIALPARM_BLUE] = 1.0;
	lightSource->materialParms[MATERIALPARM_ALPHA] = 1.0;
	lightSource->materialParms[MATERIALPARM_TIME_OFFSET] = 0.0;
	lightSource->materialParms[MATERIALPARM_DIVERSITY] = 0.0;
	lightSource->materialParms[MATERIALPARM_GENERAL] = 0.0;
	lightSource->materialParms[MATERIALPARM_MODE] = 0.0;

	// Compute axes
	AxisClear(lightSource->axis);

	// Compute the corners of the bounding volume
	for (i = 0; i < 8; i++){
		tmp[0] = (i & 1) ? -lightSource->radius[0] : lightSource->radius[0];
		tmp[1] = (i & 2) ? -lightSource->radius[1] : lightSource->radius[1];
		tmp[2] = (i & 4) ? -lightSource->radius[2] : lightSource->radius[2];

		// Rotate and translate
		VectorRotate(tmp, lightSource->axis, lightSource->corners[i]);
		VectorAdd(lightSource->corners[i], lightSource->origin, lightSource->corners[i]);
	}

	// Check if it's rotated
	lightSource->rotated = false;

	// Compute mins/maxs
	VectorSubtract(lightSource->origin, lightSource->radius, lightSource->mins);
	VectorAdd(lightSource->origin, lightSource->radius, lightSource->maxs);

	// Compute frustum planes
	for (i = 0; i < 3; i++){
		dot = DotProduct(lightSource->origin, lightSource->axis[i]);

		VectorCopy(lightSource->axis[i], lightSource->frustum[i*2+0].normal);
		lightSource->frustum[i*2+0].dist = dot - lightSource->radius[i];
		lightSource->frustum[i*2+0].type = PlaneTypeForNormal(lightSource->frustum[i*2+0].normal);
		SetPlaneSignbits(&lightSource->frustum[i*2+0]);

		VectorNegate(lightSource->axis[i], lightSource->frustum[i*2+1].normal);
		lightSource->frustum[i*2+1].dist = -dot - lightSource->radius[i];
		lightSource->frustum[i*2+1].type = PlaneTypeForNormal(lightSource->frustum[i*2+1].normal);
		SetPlaneSignbits(&lightSource->frustum[i*2+1]);
	}

	// Find the leaf
	VectorAdd(lightSource->origin, lightSource->center, tmp);

	lightSource->leaf = R_PointInLeaf(tmp);

	// If still editing a light, apply the changes
	if (r_lightEditor.editLight)
		E_ApplyLightProperties();

	// Edit the new light
	r_lightEditor.editLight = lightSource;

	VectorMA(tr.renderView.viewOrigin, 100, tr.renderView.viewAxis[0], lightProperties.origin);

	SnapVector(lightProperties.origin);
	VectorClear(lightProperties.center);
	VectorClear(lightProperties.angles);
	VectorSet(lightProperties.radius, 100, 100, 100);
	lightProperties.style = 0;
	lightProperties.detailLevel = 10;
	lightProperties.parallel = false;
	lightProperties.noShadows = false;
	Q_strncpyz(lightProperties.material, tr.defaultLightMaterial->name, sizeof(lightProperties.material));
	lightProperties.materialParms[0] = 1.0;
	lightProperties.materialParms[1] = 1.0;
	lightProperties.materialParms[2] = 1.0;
	lightProperties.materialParms[3] = 1.0;
	lightProperties.materialParms[4] = 0.0;
	lightProperties.materialParms[5] = 0.0;
	lightProperties.materialParms[6] = 0.0;
	lightProperties.materialParms[7] = 0.0;

	E_EditLightProperties(&lightProperties);

	Com_Printf("Added light #%i at %i, %i, %i\n", lightSource->index, (int)lightSource->origin[0], (int)lightSource->origin[1], (int)lightSource->origin[2]);
}

/*
 =================
 R_InitLightEditor
 =================
*/
void R_InitLightEditor (void){

	Cmd_AddCommand("editLights", R_EditLights_f, "Edit static lights in-game");
	Cmd_AddCommand("clearLights", R_ClearLights_f, "Clear all static lights");
	Cmd_AddCommand("saveLights", R_SaveLights_f, "Save static lights to the script file");
	Cmd_AddCommand("addLight", R_AddLight_f, "Add a new static light");
}

/*
 =================
 R_ShutdownLightEditor
 =================
*/
void R_ShutdownLightEditor (void){

	Cmd_RemoveCommand("editLights");
	Cmd_RemoveCommand("clearLights");
	Cmd_RemoveCommand("saveLights");
	Cmd_RemoveCommand("addLight");

	CL_SetKeyEditMode(false);

	E_DestroyLightPropertiesWindow();

	memset(&r_lightEditor, 0, sizeof(lightEditor_t));
}
