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


#include "q_shared.h"


vec3_t	byteDirs[NUM_VERTEX_NORMALS] = {
#include "anorms.h"
};

color_t	colorBlack		= {  0,   0,   0, 255};
color_t	colorRed		= {255,   0,   0, 255};
color_t	colorGreen		= {0.0, 255,   0, 255};
color_t	colorYellow		= {255, 255,   0, 255};
color_t	colorBlue		= {  0,   0, 255, 255};
color_t	colorCyan		= {  0, 255, 255, 255};
color_t	colorMagenta	= {255,   0, 255, 255};
color_t	colorWhite		= {255, 255, 255, 255};

color_t	colorTable[32] = {
	{  0,   0,   0, 255},	// Black
	{255,   0,   0, 255},	// Red
	{  0, 255,   0, 255},	// Green
	{255, 255,   0, 255},	// Yellow
	{  0,   0, 255, 255},	// Blue
	{  0, 255, 255, 255},	// Cyan
	{255,   0, 255, 255},	// Magenta
	{255, 255, 255, 255},	// White
	{192, 192, 192, 255},	// Light grey
	{127, 127, 127, 255},	// Mid grey
	{ 64,  64,  64, 255},	// Dark grey
	{192,   0,   0, 255},	// Light red
	{127,   0,   0, 255},	// Mid red
	{ 64,   0,   0, 255},	// Dark red
	{  0, 192,   0, 255},	// Light green
	{  0, 127,   0, 255},	// Mid green
	{  0,  64,   0, 255},	// Dark green
	{192, 192,   0, 255},	// Light yellow
	{127, 127,   0, 255},	// Mid yellow
	{ 64,  64,   0, 255},	// Dark yellow
	{  0,   0, 192, 255},	// Light blue
	{  0,   0, 127, 255},	// Mid blue
	{  0,   0,  64, 255},	// Dark blue
	{  0, 192, 192, 255},	// Light cyan
	{  0, 127, 127, 255},	// Mid cyan
	{  0,  64,  64, 255},	// Dark cyan
	{192,   0, 192, 255},	// Light magenta
	{127,   0, 127, 255},	// Mid magenta
	{ 64,   0,  64, 255},	// Dark magenta
	{255, 192,   0, 255},	// Light orange
	{255, 127,   0, 255},	// Mid orange
	{255,  64,   0, 255},	// Dark orange
};

vec3_t	vec3_origin = {0, 0, 0};
vec3_t	axisDefault[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
mat4_t	matrixIdentity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};


// =====================================================================


/*
 =================
 DirToByte

 This isn't a real cheap function to call!
 =================
*/
int DirToByte (const vec3_t dir){

	int		i, best = 0;
	float	d, bestd = 0;

	if (!dir)
		return 0;

	for (i = 0; i < NUM_VERTEX_NORMALS; i++){
		d = DotProduct(dir, byteDirs[i]);
		if (d > bestd){
			bestd = d;
			best = i;
		}
	}

	return best;
}

/*
 =================
 ByteToDir
 =================
*/
void ByteToDir (int b, vec3_t dir){

	if (b < 0 || b >= NUM_VERTEX_NORMALS){
		VectorClear(dir);
		return;
	}

	VectorCopy(byteDirs[b], dir);
}


// =====================================================================


/*
 =================
 ColorBytes
 =================
*/
unsigned ColorBytes (float r, float g, float b, float a){

	unsigned	c;

	((byte *)&c)[0] = r * 255;
	((byte *)&c)[1] = g * 255;
	((byte *)&c)[2] = b * 255;
	((byte *)&c)[3] = a * 255;

	return c;
}

/*
 =================
 ColorNormalize
 =================
*/
unsigned ColorNormalize (vec3_t rgb){

	unsigned	c;
	float		max;

	// Catch negative colors
	if (rgb[0] < 0)
		rgb[0] = 0;
	if (rgb[1] < 0)
		rgb[1] = 0;
	if (rgb[2] < 0)
		rgb[2] = 0;

	// Determine the brightest of the three color components
	max = rgb[0];
	if (max < rgb[1])
		max = rgb[1];
	if (max < rgb[2])
		max = rgb[2];

	// Rescale all the color components if the intensity of the greatest
	// channel exceeds 1.0
	if (max > 1.0){
		max = 255 * (1.0 / max);

		((byte *)&c)[0] = rgb[0] * max;
		((byte *)&c)[1] = rgb[1] * max;
		((byte *)&c)[2] = rgb[2] * max;
		((byte *)&c)[3] = 255;
	}
	else {
		((byte *)&c)[0] = rgb[0] * 255;
		((byte *)&c)[1] = rgb[1] * 255;
		((byte *)&c)[2] = rgb[2] * 255;
		((byte *)&c)[3] = 255;
	}
	
	return c;
}


// =====================================================================


/*
 =================
 Q_rsqrt
 =================
*/
float Q_rsqrt (float number){

	int		i;
	float	x, y;

	x = number * 0.5f;
	i = *(int *)&number;			// Evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);		// What the fuck?
	y = *(float *)&i;
	y = y * (1.5f - (x * y * y));	// First iteration

	return y;
}

/*
 =================
 Q_log2
 =================
*/
int Q_log2 (int val){

	int	answer = 0;
	
	while (val >>= 1)
		answer++;

	return answer;
}


// =====================================================================


/*
 =================
 RotatePointAroundVector
 =================
*/
void RotatePointAroundVector (vec3_t dst, const vec3_t dir, const vec3_t point, float degrees){

	vec3_t	mat[3], tmp[3], rot[3];
	float	rad, s, c;

	VectorCopy(dir, mat[2]);
	MakeNormalVectors(mat[2], mat[0], mat[1]);

	rad = DEG2RAD(degrees);
	s = sin(rad);
	c = cos(rad);

	tmp[0][0] = mat[0][0] * c + mat[1][0] * -s;
	tmp[0][1] = mat[0][0] * s + mat[1][0] * c;
	tmp[0][2] = mat[2][0];
	tmp[1][0] = mat[0][1] * c + mat[1][1] * -s;
	tmp[1][1] = mat[0][1] * s + mat[1][1] * c;
	tmp[1][2] = mat[2][1];
	tmp[2][0] = mat[0][2] * c + mat[1][2] * -s;
	tmp[2][1] = mat[0][2] * s + mat[1][2] * c;
	tmp[2][2] = mat[2][2];

	MatrixMultiply(tmp, mat, rot);

	VectorRotate(point, rot, dst);
}

/*
 =================
 NormalToLatLong
 =================
*/
void NormalToLatLong (const vec3_t normal, byte bytes[2]){

	int		lat, lng;

	if (normal[0] == 0 && normal[1] == 0){
		if (normal[2] > 0){
			// Lattitude = 0, Longitude = 0
			bytes[0] = 0;
			bytes[1] = 0;
		}
		else {
			// Lattitude = 0, Longitude = 128
			bytes[0] = 128;
			bytes[1] = 0;
		}
	}
	else {
		lat = RAD2DEG(atan2(normal[1], normal[0])) * (255.0 / 360.0);
		lng = RAD2DEG(acos(normal[2])) * (255.0 / 360.0);

		bytes[0] = lng & 0xff;
		bytes[1] = lat & 0xff;
	}
}

/*
 =================
 VectorToAngles
 =================
*/
void VectorToAngles (const vec3_t vec, vec3_t angles){

	float	forward;
	float	pitch, yaw;

	if (vec[1] == 0 && vec[0] == 0){
		yaw = 0;

		if (vec[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else {
		if (vec[0])
			yaw = RAD2DEG(atan2(vec[1], vec[0]));
		else if (vec[1] > 0)
			yaw = 90;
		else
			yaw = 270;

		if (yaw < 0)
			yaw += 360;

		forward = sqrt(vec[0]*vec[0] + vec[1]*vec[1]);

		pitch = RAD2DEG(atan2(vec[2], forward));
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

/*
 =================
 AngleVectors
 =================
*/
void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up){

	static float	sp, sy, sr, cp, cy, cr;
	float			angle;

	angle = DEG2RAD(angles[PITCH]);
	sp = sin(angle);
	cp = cos(angle);
	angle = DEG2RAD(angles[YAW]);
	sy = sin(angle);
	cy = cos(angle);
	angle = DEG2RAD(angles[ROLL]);
	sr = sin(angle);
	cr = cos(angle);

	if (forward){
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right){
		right[0] = -1*sr*sp*cy+-1*cr*-sy;
		right[1] = -1*sr*sp*sy+-1*cr*cy;
		right[2] = -1*sr*cp;
	}
	if (up){
		up[0] = cr*sp*cy+-sr*-sy;
		up[1] = cr*sp*sy+-sr*cy;
		up[2] = cr*cp;
	}
}

/*
 =================
 AnglesToAxis
 =================
*/
void AnglesToAxis (const vec3_t angles, vec3_t axis[3]){

	static float	sp, sy, sr, cp, cy, cr;
	float			angle;

	angle = DEG2RAD(angles[PITCH]);
	sp = sin(angle);
	cp = cos(angle);
	angle = DEG2RAD(angles[YAW]);
	sy = sin(angle);
	cy = cos(angle);
	angle = DEG2RAD(angles[ROLL]);
	sr = sin(angle);
	cr = cos(angle);

	axis[0][0] = cp*cy;
	axis[0][1] = cp*sy;
	axis[0][2] = -sp;
	axis[1][0] = sr*sp*cy+cr*-sy;
	axis[1][1] = sr*sp*sy+cr*cy;
	axis[1][2] = sr*cp;
	axis[2][0] = cr*sp*cy+-sr*-sy;
	axis[2][1] = cr*sp*sy+-sr*cy;
	axis[2][2] = cr*cp;
}

/*
 =================
 AxisClear
 =================
*/
void AxisClear (vec3_t axis[3]){

	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

/*
 =================
 AxisCopy
 =================
*/
void AxisCopy (const vec3_t in[3], vec3_t out[3]){

	out[0][0] = in[0][0];
	out[0][1] = in[0][1];
	out[0][2] = in[0][2];
	out[1][0] = in[1][0];
	out[1][1] = in[1][1];
	out[1][2] = in[1][2];
	out[2][0] = in[2][0];
	out[2][1] = in[2][1];
	out[2][2] = in[2][2];
}

/*
 =================
 AxisCompare
 =================
*/
qboolean AxisCompare (const vec3_t axis1[3], const vec3_t axis2[3]){

	if (axis1[0][0] != axis2[0][0] || axis1[0][1] != axis2[0][1] || axis1[0][2] != axis2[0][2])
		return false;
	if (axis1[1][0] != axis2[1][0] || axis1[1][1] != axis2[1][1] || axis1[1][2] != axis2[1][2])
		return false;
	if (axis1[2][0] != axis2[2][0] || axis1[2][1] != axis2[2][1] || axis1[2][2] != axis2[2][2])
		return false;

	return true;
}

/*
 =================
 AngleMod
 =================
*/
float AngleMod (float angle){

	return (360.0/65536) * ((int)(angle * (65536/360.0)) & 65535);
}

/*
 =================
 LerpAngle
 =================
*/
float LerpAngle (float from, float to, float frac){

	if (to - from > 180)
		to -= 360;
	if (to - from < -180)
		to += 360;

	return from + (to - from) * frac;
}

/*
 =================
 ProjectPointOnPlane
 =================
*/
void ProjectPointOnPlane (vec3_t dst, const vec3_t p, const vec3_t normal){

	float	d, invDenom;

	invDenom = 1.0 / DotProduct(normal, normal);

	d = DotProduct(normal, p) * invDenom;

	dst[0] = p[0] - d * (normal[0] * invDenom);
	dst[1] = p[1] - d * (normal[1] * invDenom);
	dst[2] = p[2] - d * (normal[2] * invDenom);
}

/*
 =================
 PerpendicularVector

 Assumes "src" is normalized
 =================
*/
void PerpendicularVector (vec3_t dst, const vec3_t src){

	int		i, pos = 0;
	float	val, min = 1.0;
	vec3_t	tmp;

	// Find the smallest magnitude axially aligned vector
	for (i = 0; i < 3; i++){
		val = fabs(src[i]);
		if (val < min){
			min = val;
			pos = i;
		}
	}

	VectorClear(tmp);
	tmp[pos] = 1.0;

	// Project the point onto the plane defined by src
	ProjectPointOnPlane(dst, tmp, src);

	// Normalize the result
	VectorNormalize(dst);
}

/*
 =================
 MakeNormalVectors

 Given a normalized forward vector, create two other perpendicular
 vectors
 =================
*/
void MakeNormalVectors (const vec3_t forward, vec3_t right, vec3_t up){

	float	d;

	// This rotate and negate guarantees a vector not colinear with the 
	// original
	right[0] = forward[2];
	right[1] = -forward[0];
	right[2] = forward[1];

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize(right);
	CrossProduct(right, forward, up);
}

/*
 =================
 MatrixMultiply
 =================
*/
void MatrixMultiply (const vec3_t in1[3], const vec3_t in2[3], vec3_t out[3]){

	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

/*
 =================
 ClearBounds
 =================
*/
void ClearBounds (vec3_t mins, vec3_t maxs){

	mins[0] = mins[1] = mins[2] = 999999;
	maxs[0] = maxs[1] = maxs[2] = -999999;
}

/*
 =================
 AddPointToBounds
 =================
*/
void AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs){

	if (v[0] < mins[0])
		mins[0] = v[0];
	if (v[0] > maxs[0])
		maxs[0] = v[0];

	if (v[1] < mins[1])
		mins[1] = v[1];
	if (v[1] > maxs[1])
		maxs[1] = v[1];

	if (v[2] < mins[2])
		mins[2] = v[2];
	if (v[2] > maxs[2])
		maxs[2] = v[2];
}

/*
 =================
 RadiusFromBounds
 =================
*/
float RadiusFromBounds (const vec3_t mins, const vec3_t maxs){

	float	a, b;
	vec3_t	corner;

	a = fabs(mins[0]);
	b = fabs(maxs[0]);
	corner[0] = a > b ? a : b;

	a = fabs(mins[1]);
	b = fabs(maxs[1]);
	corner[1] = a > b ? a : b;

	a = fabs(mins[2]);
	b = fabs(maxs[2]);
	corner[2] = a > b ? a : b;

	return VectorLength(corner);
}

/*
 =================
 BoundsIntersect
 =================
*/
qboolean BoundsIntersect (const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2){

	if (mins1[0] > maxs2[0] || mins1[1] > maxs2[1] || mins1[2] > maxs2[2])
		return false;
	if (maxs1[0] < mins2[0] || maxs1[1] < mins2[1] || maxs1[2] < mins2[2])
		return false;

	return true;
}

/*
 =================
 BoundsAndSphereIntersect
 =================
*/
qboolean BoundsAndSphereIntersect (const vec3_t mins, const vec3_t maxs, const vec3_t origin, float radius){

	if (mins[0] > origin[0] + radius || mins[1] > origin[1] + radius || mins[2] > origin[2] + radius)
		return false;
	if (maxs[0] < origin[0] - radius || maxs[1] < origin[1] - radius || maxs[2] < origin[2] - radius)
		return false;

	return true;
}

/*
 =================
 PlaneFromPoints

 Returns false if the triangle is degenerate.
 The normal will point out of the clock for clockwise ordered points.
 =================
*/
qboolean PlaneFromPoints (cplane_t *plane, const vec3_t a, const vec3_t b, const vec3_t c){
	
	vec3_t	dir1, dir2;

	VectorSubtract(b, a, dir1);
	VectorSubtract(c, a, dir2);
	CrossProduct(dir2, dir1, plane->normal);

	if (VectorNormalize(plane->normal) == 0){
		plane->dist = 0;
		return false;
	}

	plane->dist = DotProduct(a, plane->normal);
	return true;
}

/*
 =================
 SetPlaneSignbits

 For fast box on plane side test
 =================
*/
void SetPlaneSignbits (cplane_t *plane){

	plane->signbits = 0;

	if (plane->normal[0] < 0)
		plane->signbits |= 1;
	if (plane->normal[1] < 0)
		plane->signbits |= 2;
	if (plane->normal[2] < 0)
		plane->signbits |= 4;
}

/*
 =================
 PlaneTypeForNormal
 =================
*/
int PlaneTypeForNormal (const vec3_t normal){

	if (normal[0] == 1.0)
		return PLANE_X;
	if (normal[1] == 1.0)
		return PLANE_Y;
	if (normal[2] == 1.0)
		return PLANE_Z;

	return PLANE_NON_AXIAL;
}

/*
 =================
 BoxOnPlaneSide

 Returns 1, 2, or 1 + 2
 =================
*/
int BoxOnPlaneSide (const vec3_t mins, const vec3_t maxs, cplane_t *p){

	float	dist1, dist2;
	int		sides;

	// Fast axial cases
	if (p->type < 3){
		if (p->dist <= mins[p->type])
			return 1;
		if (p->dist >= maxs[p->type])
			return 2;

		return 3;
	}
	
	// General case
	switch (p->signbits){
	case 0:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		break;
	case 1:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		break;
	case 2:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		break;
	case 3:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		break;
	case 4:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		break;
	case 5:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2];
		break;
	case 6:
		dist1 = p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		break;
	case 7:
		dist1 = p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2];
		dist2 = p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2];
		break;
	default:
		dist1 = 0;
		dist2 = 0;
		break;
	}

	sides = 0;

	if (dist1 >= p->dist)
		sides |= 1;
	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}


// =====================================================================


/*
 =================
 _DotProduct
 =================
*/
vec_t _DotProduct (const vec3_t v1, const vec3_t v2){

	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

/*
 =================
 _CrossProduct
 =================
*/
void _CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross){

	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

/*
 =================
 _Distance
 =================
*/
vec_t _Distance (const vec3_t v1, const vec3_t v2){

	vec3_t	v;

	v[0] = v2[0] - v1[0];
	v[1] = v2[1] - v1[1];
	v[2] = v2[2] - v1[2];

	return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

/*
 =================
 _DistanceSquared
 =================
*/
vec_t _DistanceSquared (const vec3_t v1, const vec3_t v2){

	vec3_t	v;

	v[0] = v2[0] - v1[0];
	v[1] = v2[1] - v1[1];
	v[2] = v2[2] - v1[2];

	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

/*
 =================
 _SnapVector
 =================
*/
void _SnapVector (vec3_t v){

	v[0] = (int)v[0];
	v[1] = (int)v[1];
	v[2] = (int)v[2];
}

/*
 =================
 _VectorCopy
 =================
*/
void _VectorCopy (const vec3_t in, vec3_t out){

	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

/*
 =================
 _VectorSet
 =================
*/
void _VectorSet (vec3_t v, float x, float y, float z){

	v[0] = x;
	v[1] = y;
	v[2] = z;
}

/*
 =================
 _VectorClear
 =================
*/
void _VectorClear (vec3_t v){

	v[0] = 0;
	v[1] = 0;
	v[2] = 0;
}

/*
 =================
 _VectorCompare
 =================
*/
qboolean _VectorCompare (const vec3_t v1, const vec3_t v2){

	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
		return false;
			
	return true;
}

/*
 =================
 _VectorAdd
 =================
*/
void _VectorAdd (const vec3_t v1, const vec3_t v2, vec3_t out){

	out[0] = v1[0] + v2[0];
	out[1] = v1[1] + v2[1];
	out[2] = v1[2] + v2[2];
}

/*
 =================
 _VectorSubtract
 =================
*/
void _VectorSubtract (const vec3_t v1, const vec3_t v2, vec3_t out){

	out[0] = v1[0] - v2[0];
	out[1] = v1[1] - v2[1];
	out[2] = v1[2] - v2[2];
}

/*
 =================
 _VectorScale
 =================
*/
void _VectorScale (const vec3_t v, float scale, vec3_t out){

	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
}

/*
 =================
 _VectorMultiply
 =================
*/
void _VectorMultiply (const vec3_t v1, const vec3_t v2, vec3_t out){

	out[0] = v1[0] * v2[0];
	out[1] = v1[1] * v2[1];
	out[2] = v1[2] * v2[2];
}

/*
 =================
 _VectorMA
 =================
*/
void _VectorMA (const vec3_t v1, float scale, const vec3_t v2, vec3_t out){

	out[0] = v1[0] + v2[0] * scale;
	out[1] = v1[1] + v2[1] * scale;
	out[2] = v1[2] + v2[2] * scale;
}

/*
 =================
 _VectorAverage
 =================
*/
void _VectorAverage (const vec3_t v1, const vec3_t v2, vec3_t out){

	out[0] = (v1[0] + v2[0]) * 0.5;
	out[1] = (v1[1] + v2[1]) * 0.5;
	out[2] = (v1[2] + v2[2]) * 0.5;
}

/*
 =================
 _VectorNegate
 =================
*/
void _VectorNegate (const vec3_t v, vec3_t out){

	out[0] = -v[0];
	out[1] = -v[1];
	out[2] = -v[2];
}

/*
 =================
 _VectorInverse
 =================
*/
void _VectorInverse (vec3_t v){

	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

/*
 =================
 _VectorLength
 =================
*/
vec_t _VectorLength (const vec3_t v){

	return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

/*
 =================
 _VectorLengthSquared
 =================
*/
vec_t _VectorLengthSquared (const vec3_t v){

	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

/*
 =================
 VectorRotate
 =================
*/
void VectorRotate (const vec3_t v, const vec3_t matrix[3], vec3_t out){

	out[0] = v[0]*matrix[0][0] + v[1]*matrix[0][1] + v[2]*matrix[0][2];
	out[1] = v[0]*matrix[1][0] + v[1]*matrix[1][1] + v[2]*matrix[1][2];
	out[2] = v[0]*matrix[2][0] + v[1]*matrix[2][1] + v[2]*matrix[2][2];
}

/*
 =================
 VectorReflect
 =================
*/
void VectorReflect (const vec3_t v, const vec3_t normal, vec3_t out){

	float	d;

	d = 2.0 * (v[0]*normal[0] + v[1]*normal[1] + v[2]*normal[2]);

	out[0] = v[0] - normal[0] * d;
	out[1] = v[1] - normal[1] * d;
	out[2] = v[2] - normal[2] * d;
}

/*
 =================
 VectorNormalize
 =================
*/
vec_t VectorNormalize (vec3_t v){

	float	length, invLength;

	length = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

	if (length){
		invLength = 1.0 / length;

		v[0] *= invLength;
		v[1] *= invLength;
		v[2] *= invLength;
	}
	
	return length;
}

/*
 =================
 VectorNormalize2
 =================
*/
vec_t VectorNormalize2 (const vec3_t v, vec3_t out){

	float	length, invLength;

	length = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

	if (length){
		invLength = 1.0 / length;

		out[0] = v[0] * invLength;
		out[1] = v[1] * invLength;
		out[2] = v[2] * invLength;
	}
	else {
		out[0] = 0;
		out[1] = 0;
		out[2] = 0;
	}

	return length;
}

/*
 =================
 VectorNormalizeFast
 
 Fast vector normalize routine that does not check to make sure that
 length != 0, nor does it return length. Uses rsqrt approximation.
 =================
*/
void VectorNormalizeFast (vec3_t v){

	float	invLength;

	invLength = Q_rsqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

	v[0] *= invLength;
	v[1] *= invLength;
	v[2] *= invLength;
}


// =====================================================================


/*
 =================
 Matrix4_Copy
 =================
*/
void Matrix4_Copy (const mat4_t in, mat4_t out){

	out[ 0] = in[ 0];
	out[ 1] = in[ 1];
	out[ 2] = in[ 2];
	out[ 3] = in[ 3];
	out[ 4] = in[ 4];
	out[ 5] = in[ 5];
	out[ 6] = in[ 6];
	out[ 7] = in[ 7];
	out[ 8] = in[ 8];
	out[ 9] = in[ 9];
	out[10] = in[10];
	out[11] = in[11];
	out[12] = in[12];
	out[13] = in[13];
	out[14] = in[14];
	out[15] = in[15];
}

/*
 =================
 Matrix4_Compare
 =================
*/
qboolean Matrix4_Compare (const mat4_t m1, const mat4_t m2){

	if (m1[ 0] != m2[ 0])
		return false;
	if (m1[ 1] != m2[ 1])
		return false;
	if (m1[ 2] != m2[ 2])
		return false;
	if (m1[ 3] != m2[ 3])
		return false;
	if (m1[ 4] != m2[ 4])
		return false;
	if (m1[ 5] != m2[ 5])
		return false;
	if (m1[ 6] != m2[ 6])
		return false;
	if (m1[ 7] != m2[ 7])
		return false;
	if (m1[ 8] != m2[ 8])
		return false;
	if (m1[ 9] != m2[ 9])
		return false;
	if (m1[10] != m2[10])
		return false;
	if (m1[11] != m2[11])
		return false;
	if (m1[12] != m2[12])
		return false;
	if (m1[13] != m2[13])
		return false;
	if (m1[14] != m2[14])
		return false;
	if (m1[15] != m2[15])
		return false;

	return true;
}

/*
 =================
 Matrix4_Transpose
 =================
*/
void Matrix4_Transpose (const mat4_t m, mat4_t out){

	out[ 0] = m[ 0];
	out[ 1] = m[ 4];
	out[ 2] = m[ 8];
	out[ 3] = m[12];
	out[ 4] = m[ 1];
	out[ 5] = m[ 5];
	out[ 6] = m[ 9];
	out[ 7] = m[13];
	out[ 8] = m[ 2];
	out[ 9] = m[ 6];
	out[10] = m[10];
	out[11] = m[14];
	out[12] = m[ 3];
	out[13] = m[ 7];
	out[14] = m[11];
	out[15] = m[15];
}

/*
 =================
 Matrix4_Multiply
 =================
*/
void Matrix4_Multiply (const mat4_t m1, const mat4_t m2, mat4_t out){

	out[ 0] = m1[ 0] * m2[ 0] + m1[ 4] * m2[ 1] + m1[ 8] * m2[ 2] + m1[12] * m2[ 3];
	out[ 1] = m1[ 1] * m2[ 0] + m1[ 5] * m2[ 1] + m1[ 9] * m2[ 2] + m1[13] * m2[ 3];
	out[ 2] = m1[ 2] * m2[ 0] + m1[ 6] * m2[ 1] + m1[10] * m2[ 2] + m1[14] * m2[ 3];
	out[ 3] = m1[ 3] * m2[ 0] + m1[ 7] * m2[ 1] + m1[11] * m2[ 2] + m1[15] * m2[ 3];
	out[ 4] = m1[ 0] * m2[ 4] + m1[ 4] * m2[ 5] + m1[ 8] * m2[ 6] + m1[12] * m2[ 7];
	out[ 5] = m1[ 1] * m2[ 4] + m1[ 5] * m2[ 5] + m1[ 9] * m2[ 6] + m1[13] * m2[ 7];
	out[ 6] = m1[ 2] * m2[ 4] + m1[ 6] * m2[ 5] + m1[10] * m2[ 6] + m1[14] * m2[ 7];
	out[ 7] = m1[ 3] * m2[ 4] + m1[ 7] * m2[ 5] + m1[11] * m2[ 6] + m1[15] * m2[ 7];
	out[ 8] = m1[ 0] * m2[ 8] + m1[ 4] * m2[ 9] + m1[ 8] * m2[10] + m1[12] * m2[11];
	out[ 9] = m1[ 1] * m2[ 8] + m1[ 5] * m2[ 9] + m1[ 9] * m2[10] + m1[13] * m2[11];
	out[10] = m1[ 2] * m2[ 8] + m1[ 6] * m2[ 9] + m1[10] * m2[10] + m1[14] * m2[11];
	out[11] = m1[ 3] * m2[ 8] + m1[ 7] * m2[ 9] + m1[11] * m2[10] + m1[15] * m2[11];
	out[12] = m1[ 0] * m2[12] + m1[ 4] * m2[13] + m1[ 8] * m2[14] + m1[12] * m2[15];
	out[13] = m1[ 1] * m2[12] + m1[ 5] * m2[13] + m1[ 9] * m2[14] + m1[13] * m2[15];
	out[14] = m1[ 2] * m2[12] + m1[ 6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15];
	out[15] = m1[ 3] * m2[12] + m1[ 7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15];
}

/*
 =================
 Matrix4_MultiplyFast
 =================
*/
void Matrix4_MultiplyFast (const mat4_t m1, const mat4_t m2, mat4_t out){

	out[ 0] = m1[ 0] * m2[ 0] + m1[ 4] * m2[ 1] + m1[ 8] * m2[ 2];
	out[ 1] = m1[ 1] * m2[ 0] + m1[ 5] * m2[ 1] + m1[ 9] * m2[ 2];
	out[ 2] = m1[ 2] * m2[ 0] + m1[ 6] * m2[ 1] + m1[10] * m2[ 2];
	out[ 3] = 0.0;
	out[ 4] = m1[ 0] * m2[ 4] + m1[ 4] * m2[ 5] + m1[ 8] * m2[ 6];
	out[ 5] = m1[ 1] * m2[ 4] + m1[ 5] * m2[ 5] + m1[ 9] * m2[ 6];
	out[ 6] = m1[ 2] * m2[ 4] + m1[ 6] * m2[ 5] + m1[10] * m2[ 6];
	out[ 7] = 0.0;
	out[ 8] = m1[ 0] * m2[ 8] + m1[ 4] * m2[ 9] + m1[ 8] * m2[10];
	out[ 9] = m1[ 1] * m2[ 8] + m1[ 5] * m2[ 9] + m1[ 9] * m2[10];
	out[10] = m1[ 2] * m2[ 8] + m1[ 6] * m2[ 9] + m1[10] * m2[10];
	out[11] = 0.0;
	out[12] = m1[ 0] * m2[12] + m1[ 4] * m2[13] + m1[ 8] * m2[14] + m1[12];
	out[13] = m1[ 1] * m2[12] + m1[ 5] * m2[13] + m1[ 9] * m2[14] + m1[13];
	out[14] = m1[ 2] * m2[12] + m1[ 6] * m2[13] + m1[10] * m2[14] + m1[14];
	out[15] = 1.0;
}

/*
 =================
 Matrix4_Identity
 =================
*/
void Matrix4_Identity (mat4_t m){

	m[ 0] = 1;
	m[ 1] = 0;
	m[ 2] = 0;
	m[ 3] = 0;
	m[ 4] = 0;
	m[ 5] = 1;
	m[ 6] = 0;
	m[ 7] = 0;
	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = 1;
	m[11] = 0;
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = 1;
}

/*
 =================
 Matrix4_Rotate
 =================
*/
void Matrix4_Rotate (mat4_t m, float angle, float x, float y, float z){

	mat4_t	rot, tmp;
	float	mag, rad, s, c, i;
	float	xx, yy, zz, xy, yz, zx, xs, ys, zs;

	mag = sqrt(x*x + y*y + z*z);
	if (mag == 0)
		return;

	mag = 1.0 / mag;

	x *= mag;
	y *= mag;
	z *= mag;

	rad = DEG2RAD(angle);
	s = sin(rad);
	c = cos(rad);

	i = 1.0 - c;

	xx = (x * x) * i;
	yy = (y * y) * i;
	zz = (z * z) * i;
	xy = (x * y) * i;
	yz = (y * z) * i;
	zx = (z * x) * i;

	xs = x * s;
	ys = y * s;
	zs = z * s;

	rot[ 0] = xx + c;
	rot[ 1] = xy + zs;
	rot[ 2] = zx - ys;
	rot[ 3] = 0.0;
	rot[ 4] = xy - zs;
	rot[ 5] = yy + c;
	rot[ 6] = yz + xs;
	rot[ 7] = 0.0;
	rot[ 8] = zx + ys;
	rot[ 9] = yz - xs;
	rot[10] = zz + c;
	rot[11] = 0.0;
	rot[12] = 0.0;
	rot[13] = 0.0;
	rot[14] = 0.0;
	rot[15] = 1.0;

	Matrix4_Copy(m, tmp);
	Matrix4_MultiplyFast(tmp, rot, m);
}

/*
 =================
 Matrix4_Scale
 =================
*/
void Matrix4_Scale (mat4_t m, float x, float y, float z){

	m[ 0] *= x;
	m[ 1] *= x;
	m[ 2] *= x;
	m[ 3] *= x;
	m[ 4] *= y;
	m[ 5] *= y;
	m[ 6] *= y;
	m[ 7] *= y;
	m[ 8] *= z;
	m[ 9] *= z;
	m[10] *= z;
	m[11] *= z;
}

/*
 =================
 Matrix4_Translate
 =================
*/
void Matrix4_Translate (mat4_t m, float x, float y, float z){

	m[12] = m[ 0] * x + m[ 4] * y + m[ 8] * z + m[12];
	m[13] = m[ 1] * x + m[ 5] * y + m[ 9] * z + m[13];
	m[14] = m[ 2] * x + m[ 6] * y + m[10] * z + m[14];
	m[15] = m[ 3] * x + m[ 7] * y + m[11] * z + m[15];
}
