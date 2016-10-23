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


#define TABLE_SIZE		1024
#define TABLE_MASK		1023

static float	rb_sinTable[TABLE_SIZE];
static float	rb_triangleTable[TABLE_SIZE];
static float	rb_squareTable[TABLE_SIZE];
static float	rb_sawtoothTable[TABLE_SIZE];
static float	rb_inverseSawtoothTable[TABLE_SIZE];
static float	rb_noiseTable[TABLE_SIZE];

static float	rb_warpSinTable[256] = {
	#include "warpsin.h"
};

static unsigned	rb_vertexBuffers[MAX_VERTEX_BUFFERS];
static int		rb_numVertexBuffers;

static int		rb_staticBytes;
static int		rb_staticCount;

static int		rb_streamBytes;
static int		rb_streamCount;

vbo_t			rb_vbo;

mesh_t			*rb_mesh;
shader_t		*rb_shader;
float			rb_shaderTime;
entity_t		*rb_entity;
int				rb_infoKey;

unsigned		indexArray[MAX_INDICES * 4];
vec3_t			vertexArray[MAX_VERTICES * 2];
vec3_t			tangentArray[MAX_VERTICES];
vec3_t			binormalArray[MAX_VERTICES];
vec3_t			normalArray[MAX_VERTICES];
color_t			colorArray[MAX_VERTICES];
vec3_t			texCoordArray[MAX_TEXTURE_UNITS][MAX_VERTICES];
color_t			inColorArray[MAX_VERTICES];
vec4_t			inTexCoordArray[MAX_VERTICES];

int				numIndex;
int				numVertex;


/*
 =================
 RB_BuildTables
 =================
*/
static void RB_BuildTables (void){

	int		i;
	float	f;

	for (i = 0; i < TABLE_SIZE; i++){
		f = (float)i / (float)TABLE_SIZE;

		rb_sinTable[i] = sin(f * M_PI2);

		if (f < 0.25)
			rb_triangleTable[i] = 4.0 * f;
		else if(f < 0.75)
			rb_triangleTable[i] = 2.0 - 4.0 * f;
		else
			rb_triangleTable[i] = (f - 0.75) * 4.0 - 1.0;

		if (f < 0.5)
			rb_squareTable[i] = 1.0;
		else
			rb_squareTable[i] = -1.0;

		rb_sawtoothTable[i] = f;

		rb_inverseSawtoothTable[i] = 1.0 - f;

		rb_noiseTable[i] = crand();
	}
}

/*
 =================
 RB_TableForFunc
 =================
*/
static float *RB_TableForFunc (const waveFunc_t *func){

	switch (func->type){
	case WAVEFORM_SIN:
		return rb_sinTable;
	case WAVEFORM_TRIANGLE:
		return rb_triangleTable;
	case WAVEFORM_SQUARE:
		return rb_squareTable;
	case WAVEFORM_SAWTOOTH:
		return rb_sawtoothTable;
	case WAVEFORM_INVERSESAWTOOTH:
		return rb_inverseSawtoothTable;
	case WAVEFORM_NOISE:
		return rb_noiseTable;
	}

	Com_Error(ERR_DROP, "RB_TableForFunc: unknown waveform type %i in shader '%s'", func->type, rb_shader->name);
}

/*
 =================
 RB_DeformVertexes
 =================
*/
static void RB_DeformVertexes (void){

	deformVertexes_t	*deformVertexes = rb_shader->deformVertexes;
	unsigned			deformVertexesNum = rb_shader->deformVertexesNum;
	int					i, j;
	float				*table;
	float				now, f, t;
	vec3_t				vec, tmp, len, axis, rotCentre;
	vec3_t				mat1[3], mat2[3], mat3[3], matrix[3], invMatrix[3];
	float				*quad[4];
	int					index, longAxis, shortAxis;

	for (i = 0; i < deformVertexesNum; i++, deformVertexes++){
		switch (deformVertexes->type){
		case DEFORMVERTEXES_WAVE:
			table = RB_TableForFunc(&deformVertexes->func);
			now = deformVertexes->func.params[2] + deformVertexes->func.params[3] * rb_shaderTime;

			for (j = 0; j < numVertex; j++){
				t = (vertexArray[j][0] + vertexArray[j][1] + vertexArray[j][2]) * deformVertexes->params[0] + now;
				f = table[((int)(t * TABLE_SIZE)) & TABLE_MASK] * deformVertexes->func.params[1] + deformVertexes->func.params[0];

				VectorMA(vertexArray[j], f, normalArray[j], vertexArray[j]);
			}
			
			break;
		case DEFORMVERTEXES_MOVE:
			table = RB_TableForFunc(&deformVertexes->func);
			now = deformVertexes->func.params[2] + deformVertexes->func.params[3] * rb_shaderTime;
			f = table[((int)(now * TABLE_SIZE)) & TABLE_MASK] * deformVertexes->func.params[1] + deformVertexes->func.params[0];

			for (j = 0; j < numVertex; j++)
				VectorMA(vertexArray[j], f, deformVertexes->params, vertexArray[j]);
			
			break;
		case DEFORMVERTEXES_NORMAL:
			now = deformVertexes->params[1] * rb_shaderTime;

			for (j = 0; j < numVertex; j++){
				f = normalArray[j][2] * now;

				normalArray[j][0] *= (deformVertexes->params[0] * sin(f));
				normalArray[j][1] *= (deformVertexes->params[0] * cos(f));

				VectorNormalizeFast(normalArray[j]);
			}
			
			break;
		case DEFORMVERTEXES_AUTOSPRITE:
			if ((numIndex % 6) || (numVertex % 4)){
				Com_Printf(S_COLOR_YELLOW "Shader '%s' has autoSprite but it's not a triangle quad\n", rb_shader->name);
				break;
			}

			if (rb_entity == r_worldEntity || rb_entity->entityType != ET_MODEL){
				VectorCopy(&r_worldMatrix[0], invMatrix[0]);
				VectorCopy(&r_worldMatrix[4], invMatrix[1]);
				VectorCopy(&r_worldMatrix[8], invMatrix[2]);
			}
			else {
				VectorCopy(&r_entityMatrix[0], invMatrix[0]);
				VectorCopy(&r_entityMatrix[4], invMatrix[1]);
				VectorCopy(&r_entityMatrix[8], invMatrix[2]);
			}

			for (index = 0; index < numIndex; index += 6){
				quad[0] = (float *)(vertexArray + indexArray[index+0]);
				quad[1] = (float *)(vertexArray + indexArray[index+1]);
				quad[2] = (float *)(vertexArray + indexArray[index+2]);

				for (j = 2; j >= 0; j--){
					quad[3] = (float *)(vertexArray + indexArray[index+3+j]);
					if (!VectorCompare(quad[3], quad[0]) && !VectorCompare(quad[3], quad[1]) && !VectorCompare(quad[3], quad[2]))
						break;
				}

				VectorSubtract(quad[0], quad[1], mat1[0]);
				VectorSubtract(quad[2], quad[1], mat1[1]);
				CrossProduct(mat1[0], mat1[1], mat1[2]);
				VectorNormalizeFast(mat1[2]);
				MakeNormalVectors(mat1[2], mat1[1], mat1[0]);

				MatrixMultiply(invMatrix, mat1, matrix);

				rotCentre[0] = (quad[0][0] + quad[1][0] + quad[2][0] + quad[3][0]) * 0.25;
				rotCentre[1] = (quad[0][1] + quad[1][1] + quad[2][1] + quad[3][1]) * 0.25;
				rotCentre[2] = (quad[0][2] + quad[1][2] + quad[2][2] + quad[3][2]) * 0.25;

				for (j = 0; j < 4; j++){
					VectorSubtract(quad[j], rotCentre, vec);
					VectorRotate(vec, matrix, quad[j]);
					VectorAdd(quad[j], rotCentre, quad[j]);
				}
			}

			break;
		case DEFORMVERTEXES_AUTOSPRITE2:
			if ((numIndex % 6) || (numVertex % 4)){
				Com_Printf(S_COLOR_YELLOW "Shader '%s' has autoSprite2 but it's not a triangle quad\n", rb_shader->name);
				break;
			}

			for (index = 0; index < numIndex; index += 6){
				quad[0] = (float *)(vertexArray + indexArray[index+0]);
				quad[1] = (float *)(vertexArray + indexArray[index+1]);
				quad[2] = (float *)(vertexArray + indexArray[index+2]);

				for (j = 2; j >= 0; j--){
					quad[3] = (float *)(vertexArray + indexArray[index+3+j]);
					if (!VectorCompare(quad[3], quad[0]) && !VectorCompare(quad[3], quad[1]) && !VectorCompare(quad[3], quad[2]))
						break;
				}

				VectorSubtract(quad[1], quad[0], mat1[0]);
				VectorSubtract(quad[2], quad[0], mat1[1]);
				VectorSubtract(quad[2], quad[1], mat1[2]);

				len[0] = DotProduct(mat1[0], mat1[0]);
				len[1] = DotProduct(mat1[1], mat1[1]);
				len[2] = DotProduct(mat1[2], mat1[2]);

				if (len[2] > len[1] && len[2] > len[0]){
					if (len[1] > len[0]){
						longAxis = 1;
						shortAxis = 0;
					}
					else {
						longAxis = 0;
						shortAxis = 1;
					}
				}
				else if (len[1] > len[2] && len[1] > len[0]){
					if (len[2] > len[0]){
						longAxis = 2;
						shortAxis = 0;
					}
					else {
						longAxis = 0;
						shortAxis = 2;
					}
				}
				else if (len[0] > len[1] && len[0] > len[2]){
					if (len[2] > len[1]){
						longAxis = 2;
						shortAxis = 1;
					}
					else {
						longAxis = 1;
						shortAxis = 2;
					}
				}
				else {
					longAxis = 0;
					shortAxis = 0;
				}

				if (DotProduct(mat1[longAxis], mat1[shortAxis])){
					VectorNormalize2(mat1[longAxis], axis);
					VectorCopy(axis, mat1[1]);

					if (axis[0] || axis[1])
						MakeNormalVectors(mat1[1], mat1[0], mat1[2]);
					else
						MakeNormalVectors(mat1[1], mat1[2], mat1[0]);
				}
				else {
					VectorNormalize2(mat1[longAxis], axis);
					VectorNormalize2(mat1[shortAxis], mat1[0]);
					VectorCopy(axis, mat1[1]);
					CrossProduct(mat1[0], mat1[1], mat1[2]);
				}

				rotCentre[0] = (quad[0][0] + quad[1][0] + quad[2][0] + quad[3][0]) * 0.25;
				rotCentre[1] = (quad[0][1] + quad[1][1] + quad[2][1] + quad[3][1]) * 0.25;
				rotCentre[2] = (quad[0][2] + quad[1][2] + quad[2][2] + quad[3][2]) * 0.25;

				if (!AxisCompare(rb_entity->axis, axisDefault)){
					VectorAdd(rotCentre, rb_entity->origin, vec);
					VectorSubtract(r_refDef.viewOrigin, vec, tmp);
					VectorRotate(tmp, rb_entity->axis, vec);
				}
				else {
					VectorAdd(rotCentre, rb_entity->origin, vec);
					VectorSubtract(r_refDef.viewOrigin, vec, vec);
				}

				f = -DotProduct(vec, axis);

				VectorMA(vec, f, axis, mat2[2]);
				VectorNormalizeFast(mat2[2]);
				VectorCopy(axis, mat2[1]);
				CrossProduct(mat2[1], mat2[2], mat2[0]);

				VectorSet(mat3[0], mat2[0][0], mat2[1][0], mat2[2][0]);
				VectorSet(mat3[1], mat2[0][1], mat2[1][1], mat2[2][1]);
				VectorSet(mat3[2], mat2[0][2], mat2[1][2], mat2[2][2]);

				MatrixMultiply(mat3, mat1, matrix);

				for (j = 0; j < 4; j++){
					VectorSubtract(quad[j], rotCentre, vec);
					VectorRotate(vec, matrix, quad[j]);
					VectorAdd(quad[j], rotCentre, quad[j]);
				}
			}

			break;
		default:
			Com_Error(ERR_DROP, "RB_DeformVertexes: unknown deformVertexes type %i in shader '%s'", deformVertexes->type, rb_shader->name);
		}
	}
}

/*
 =================
 RB_CalcVertexColors
 =================
*/
static void RB_CalcVertexColors (shaderStage_t *stage){

	rgbGen_t	*rgbGen = &stage->rgbGen;
	alphaGen_t	*alphaGen = &stage->alphaGen;
	int			i;
	vec3_t		vec, dir;
	float		*table;
	float		now, f;
	byte		r, g, b, a;

	switch (rgbGen->type){
	case RGBGEN_IDENTITY:
		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = 255;
			colorArray[i][1] = 255;
			colorArray[i][2] = 255;
		}

		break;
	case RGBGEN_IDENTITYLIGHTING:
		if (glConfig.deviceSupportsGamma)
			r = g = b = 255 >> r_overBrightBits->integer;
		else
			r = g = b = 255;

		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = r;
			colorArray[i][1] = g;
			colorArray[i][2] = b;
		}
		
		break;
	case RGBGEN_WAVE:
		table = RB_TableForFunc(&rgbGen->func);
		now = rgbGen->func.params[2] + rgbGen->func.params[3] * rb_shaderTime;
		f = table[((int)(now * TABLE_SIZE)) & TABLE_MASK] * rgbGen->func.params[1] + rgbGen->func.params[0];

		f = Clamp(f, 0.0, 1.0);

		r = 255 * f;
		g = 255 * f;
		b = 255 * f;

		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = r;
			colorArray[i][1] = g;
			colorArray[i][2] = b;
		}
		
		break;
	case RGBGEN_COLORWAVE:
		table = RB_TableForFunc(&rgbGen->func);
		now = rgbGen->func.params[2] + rgbGen->func.params[3] * rb_shaderTime;
		f = table[((int)(now * TABLE_SIZE)) & TABLE_MASK] * rgbGen->func.params[1] + rgbGen->func.params[0];

		f = Clamp(f, 0.0, 1.0);
			
		r = 255 * (rgbGen->params[0] * f);
		g = 255 * (rgbGen->params[1] * f);
		b = 255 * (rgbGen->params[2] * f);

		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = r;
			colorArray[i][1] = g;
			colorArray[i][2] = b;
		}

		break;
	case RGBGEN_VERTEX:
		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = inColorArray[i][0];
			colorArray[i][1] = inColorArray[i][1];
			colorArray[i][2] = inColorArray[i][2];
		}
		
		break;
	case RGBGEN_ONEMINUSVERTEX:
		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = 255 - inColorArray[i][0];
			colorArray[i][1] = 255 - inColorArray[i][1];
			colorArray[i][2] = 255 - inColorArray[i][2];
		}
		
		break;
	case RGBGEN_ENTITY:
		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = rb_entity->shaderRGBA[0];
			colorArray[i][1] = rb_entity->shaderRGBA[1];
			colorArray[i][2] = rb_entity->shaderRGBA[2];
		}
		
		break;
	case RGBGEN_ONEMINUSENTITY:
		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = 255 - rb_entity->shaderRGBA[0];
			colorArray[i][1] = 255 - rb_entity->shaderRGBA[1];
			colorArray[i][2] = 255 - rb_entity->shaderRGBA[2];
		}
		
		break;
	case RGBGEN_LIGHTINGAMBIENT:
		R_LightingAmbient();

		break;
	case RGBGEN_LIGHTINGDIFFUSE:
		R_LightingDiffuse();

		break;
	case RGBGEN_CONST:
		r = 255 * rgbGen->params[0];
		g = 255 * rgbGen->params[1];
		b = 255 * rgbGen->params[2];

		for (i = 0; i < numVertex; i++){
			colorArray[i][0] = r;
			colorArray[i][1] = g;
			colorArray[i][2] = b;
		}
		
		break;
	default:
		Com_Error(ERR_DROP, "RB_CalcVertexColors: unknown rgbGen type %i in shader '%s'", rgbGen->type, rb_shader->name);
	}

	switch (alphaGen->type){
	case ALPHAGEN_IDENTITY:
		for (i = 0; i < numVertex; i++)
			colorArray[i][3] = 255;
		
		break;
	case ALPHAGEN_WAVE:
		table = RB_TableForFunc(&alphaGen->func);
		now = alphaGen->func.params[2] + alphaGen->func.params[3] * rb_shaderTime;
		f = table[((int)(now * TABLE_SIZE)) & TABLE_MASK] * alphaGen->func.params[1] + alphaGen->func.params[0];

		f = Clamp(f, 0.0, 1.0);

		a = 255 * f;

		for (i = 0; i < numVertex; i++)
			colorArray[i][3] = a;
		
		break;
	case ALPHAGEN_ALPHAWAVE:
		table = RB_TableForFunc(&alphaGen->func);
		now = alphaGen->func.params[2] + alphaGen->func.params[3] * rb_shaderTime;
		f = table[((int)(now * TABLE_SIZE)) & TABLE_MASK] * alphaGen->func.params[1] + alphaGen->func.params[0];

		f = Clamp(f, 0.0, 1.0);

		a = 255 * (alphaGen->params[0] * f);

		for (i = 0; i < numVertex; i++)
			colorArray[i][3] = a;

		break;
	case ALPHAGEN_VERTEX:
		for (i = 0; i < numVertex; i++)
			colorArray[i][3] = inColorArray[i][3];
		
		break;
	case ALPHAGEN_ONEMINUSVERTEX:
		for (i = 0; i < numVertex; i++)
			colorArray[i][3] = 255 - inColorArray[i][3];
		
		break;
	case ALPHAGEN_ENTITY:
		for (i = 0; i < numVertex; i++)
			colorArray[i][3] = rb_entity->shaderRGBA[3];
		
		break;
	case ALPHAGEN_ONEMINUSENTITY:
		for (i = 0; i < numVertex; i++)
			colorArray[i][3] = 255 - rb_entity->shaderRGBA[3];
		
		break;
	case ALPHAGEN_DOT:
		if (!AxisCompare(rb_entity->axis, axisDefault))
			VectorRotate(r_refDef.viewAxis[0], rb_entity->axis, vec);
		else
			VectorCopy(r_refDef.viewAxis[0], vec);

		for (i = 0; i < numVertex; i++){
			f = DotProduct(vec, normalArray[i]);
			if (f < 0)
				f = -f;

			colorArray[i][3] = 255 * Clamp(f, alphaGen->params[0], alphaGen->params[1]);
		}

		break;
	case ALPHAGEN_ONEMINUSDOT:
		if (!AxisCompare(rb_entity->axis, axisDefault))
			VectorRotate(r_refDef.viewAxis[0], rb_entity->axis, vec);
		else
			VectorCopy(r_refDef.viewAxis[0], vec);

		for (i = 0; i < numVertex; i++){
			f = DotProduct(vec, normalArray[i]);
			if (f < 0)
				f = -f;

			colorArray[i][3] = 255 * Clamp(1.0 - f, alphaGen->params[0], alphaGen->params[1]);
		}

		break;
	case ALPHAGEN_FADE:
		for (i = 0; i < numVertex; i++){
			VectorAdd(vertexArray[i], rb_entity->origin, vec);
			f = Distance(vec, r_refDef.viewOrigin);

			f = Clamp(f, alphaGen->params[0], alphaGen->params[1]) - alphaGen->params[0];
			f = f * alphaGen->params[2];

			colorArray[i][3] = 255 * Clamp(f, 0.0, 1.0);
		}

		break;
	case ALPHAGEN_ONEMINUSFADE:
		for (i = 0; i < numVertex; i++){
			VectorAdd(vertexArray[i], rb_entity->origin, vec);
			f = Distance(vec, r_refDef.viewOrigin);

			f = Clamp(f, alphaGen->params[0], alphaGen->params[1]) - alphaGen->params[0];
			f = f * alphaGen->params[2];

			colorArray[i][3] = 255 * Clamp(1.0 - f, 0.0, 1.0);
		}

		break;
	case ALPHAGEN_LIGHTINGSPECULAR:
		if (!AxisCompare(rb_entity->axis, axisDefault)){
			VectorSubtract(r_refDef.viewOrigin, rb_entity->origin, dir);
			VectorRotate(dir, rb_entity->axis, vec);
		}
		else
			VectorSubtract(r_refDef.viewOrigin, rb_entity->origin, vec);

		for (i = 0; i < numVertex; i++){
			VectorSubtract(vec, vertexArray[i], dir);
			VectorNormalizeFast(dir);

			f = DotProduct(dir, normalArray[i]);
			f = pow(f, alphaGen->params[0]);

			colorArray[i][3] = 255 * Clamp(f, 0.0, 1.0);
		}

		break;
	case ALPHAGEN_CONST:
		a = 255 * alphaGen->params[0];

		for (i = 0; i < numVertex; i++)
			colorArray[i][3] = a;
		
		break;
	default:
		Com_Error(ERR_DROP, "RB_CalcVertexColors: unknown alphaGen type %i in shader '%s'", alphaGen->type, rb_shader->name);
	}
}

/*
 =================
 RB_CalcTextureCoords
 =================
*/
static void RB_CalcTextureCoords (stageBundle_t *bundle, unsigned unit){

	tcGen_t		*tcGen = &bundle->tcGen;
	tcMod_t		*tcMod = bundle->tcMod;
	unsigned	tcModNum = bundle->tcModNum;
	int			i, j;
	vec3_t		vec, dir;
	vec3_t		lightVector, eyeVector, halfAngle;
	float		*table;
	float		now, f, t;
	float		rad, s, c;
	vec2_t		st;

	switch (tcGen->type){
	case TCGEN_BASE:
		for (i = 0; i < numVertex; i++){
			texCoordArray[unit][i][0] = inTexCoordArray[i][0];
			texCoordArray[unit][i][1] = inTexCoordArray[i][1];
		}

		break;
	case TCGEN_LIGHTMAP:
		for (i = 0; i < numVertex; i++){
			texCoordArray[unit][i][0] = inTexCoordArray[i][2];
			texCoordArray[unit][i][1] = inTexCoordArray[i][3];
		}

		break;
	case TCGEN_ENVIRONMENT:
		if (!AxisCompare(rb_entity->axis, axisDefault)){
			VectorSubtract(r_refDef.viewOrigin, rb_entity->origin, dir);
			VectorRotate(dir, rb_entity->axis, vec);
		}
		else
			VectorSubtract(r_refDef.viewOrigin, rb_entity->origin, vec);

		for (i = 0; i < numVertex; i++){
			VectorSubtract(vec, vertexArray[i], dir);
			VectorNormalizeFast(dir);

			f = 2.0 * DotProduct(dir, normalArray[i]);

			texCoordArray[unit][i][0] = dir[0] - normalArray[i][0] * f;
			texCoordArray[unit][i][1] = dir[1] - normalArray[i][1] * f;
		}

		break;
	case TCGEN_VECTOR:
		for (i = 0; i < numVertex; i++){
			texCoordArray[unit][i][0] = DotProduct(vertexArray[i], &tcGen->params[0]);
			texCoordArray[unit][i][1] = DotProduct(vertexArray[i], &tcGen->params[3]);
		}

		break;
	case TCGEN_WARP:
		for (i = 0; i < numVertex; i++){
			texCoordArray[unit][i][0] = inTexCoordArray[i][0] + rb_warpSinTable[((int)((inTexCoordArray[i][1] * 8.0 + rb_shaderTime) * (256.0/M_PI2))) & 255] * (1.0/64);
			texCoordArray[unit][i][1] = inTexCoordArray[i][1] + rb_warpSinTable[((int)((inTexCoordArray[i][0] * 8.0 + rb_shaderTime) * (256.0/M_PI2))) & 255] * (1.0/64);
		}

		break;
	case TCGEN_LIGHTVECTOR:
		if (rb_entity == r_worldEntity){
			for (i = 0; i < numVertex; i++){
				R_LightDir(vertexArray[i], lightVector);

				texCoordArray[unit][i][0] = DotProduct(lightVector, tangentArray[i]);
				texCoordArray[unit][i][1] = DotProduct(lightVector, binormalArray[i]);
				texCoordArray[unit][i][2] = DotProduct(lightVector, normalArray[i]);
			}
		}
		else {
			R_LightDir(rb_entity->origin, dir);

			if (!AxisCompare(rb_entity->axis, axisDefault))
				VectorRotate(dir, rb_entity->axis, lightVector);
			else
				VectorCopy(dir, lightVector);

			for (i = 0; i < numVertex; i++){
				texCoordArray[unit][i][0] = DotProduct(lightVector, tangentArray[i]);
				texCoordArray[unit][i][1] = DotProduct(lightVector, binormalArray[i]);
				texCoordArray[unit][i][2] = DotProduct(lightVector, normalArray[i]);
			}
		}

		break;
	case TCGEN_HALFANGLE:
		if (rb_entity == r_worldEntity){
			for (i = 0; i < numVertex; i++){
				R_LightDir(vertexArray[i], lightVector);

				VectorSubtract(r_refDef.viewOrigin, vertexArray[i], eyeVector);

				VectorNormalizeFast(lightVector);
				VectorNormalizeFast(eyeVector);

				VectorAdd(lightVector, eyeVector, halfAngle);

				texCoordArray[unit][i][0] = DotProduct(halfAngle, tangentArray[i]);
				texCoordArray[unit][i][1] = DotProduct(halfAngle, binormalArray[i]);
				texCoordArray[unit][i][2] = DotProduct(halfAngle, normalArray[i]);
			}
		}
		else {
			R_LightDir(rb_entity->origin, dir);

			if (!AxisCompare(rb_entity->axis, axisDefault)){
				VectorRotate(dir, rb_entity->axis, lightVector);

				VectorSubtract(r_refDef.viewOrigin, rb_entity->origin, dir);
				VectorRotate(dir, rb_entity->axis, eyeVector);
			}
			else {
				VectorCopy(dir, lightVector);

				VectorSubtract(r_refDef.viewOrigin, rb_entity->origin, eyeVector);
			}

			VectorNormalizeFast(lightVector);
			VectorNormalizeFast(eyeVector);

			VectorAdd(lightVector, eyeVector, halfAngle);

			for (i = 0; i < numVertex; i++){
				texCoordArray[unit][i][0] = DotProduct(halfAngle, tangentArray[i]);
				texCoordArray[unit][i][1] = DotProduct(halfAngle, binormalArray[i]);
				texCoordArray[unit][i][2] = DotProduct(halfAngle, normalArray[i]);
			}
		}

		break;
	case TCGEN_REFLECTION:
		qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
		qglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);

		break;
	case TCGEN_NORMAL:
		qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
		qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
		qglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);

		break;
	default:
		Com_Error(ERR_DROP, "RB_CalcTextureCoords: unknown tcGen type %i in shader '%s'", tcGen->type, rb_shader->name);
	}

	for (i = 0; i < tcModNum; i++, tcMod++){
		switch (tcMod->type){
		case TCMOD_TRANSLATE:
			for (j = 0; j < numVertex; j++){
				texCoordArray[unit][j][0] += tcMod->params[0];
				texCoordArray[unit][j][1] += tcMod->params[1];
			}

			break;
		case TCMOD_SCALE:
			for (j = 0; j < numVertex; j++){
				texCoordArray[unit][j][0] *= tcMod->params[0];
				texCoordArray[unit][j][1] *= tcMod->params[1];
			}

			break;
		case TCMOD_SCROLL:
			st[0] = tcMod->params[0] * rb_shaderTime;
			st[0] -= floor(st[0]);

			st[1] = tcMod->params[1] * rb_shaderTime;
			st[1] -= floor(st[1]);

			for (j = 0; j < numVertex; j++){
				texCoordArray[unit][j][0] += st[0];
				texCoordArray[unit][j][1] += st[1];
			}

			break;
		case TCMOD_ROTATE:
			rad = -DEG2RAD(tcMod->params[0] * rb_shaderTime);
			s = sin(rad);
			c = cos(rad);

			for (j = 0; j < numVertex; j++){
				st[0] = texCoordArray[unit][j][0];
				st[1] = texCoordArray[unit][j][1];
				texCoordArray[unit][j][0] = c * (st[0] - 0.5) - s * (st[1] - 0.5) + 0.5;
				texCoordArray[unit][j][1] = c * (st[1] - 0.5) + s * (st[0] - 0.5) + 0.5;
			}

			break;
		case TCMOD_STRETCH:
			table = RB_TableForFunc(&tcMod->func);
			now = tcMod->func.params[2] + tcMod->func.params[3] * rb_shaderTime;
			f = table[((int)(now * TABLE_SIZE)) & TABLE_MASK] * tcMod->func.params[1] + tcMod->func.params[0];
			
			f = (f) ? 1.0 / f : 1.0;
			t = 0.5 - 0.5 * f;

			for (j = 0; j < numVertex; j++){
				texCoordArray[unit][j][0] = texCoordArray[unit][j][0] * f + t;
				texCoordArray[unit][j][1] = texCoordArray[unit][j][1] * f + t;
			}

			break;
		case TCMOD_TURB:
			table = RB_TableForFunc(&tcMod->func);
			now = tcMod->func.params[2] + tcMod->func.params[3] * rb_shaderTime;

			for (j = 0; j < numVertex; j++){
				texCoordArray[unit][j][0] += (table[((int)(((vertexArray[j][0] + vertexArray[j][2]) * 1.0/128 * 0.125 + now) * TABLE_SIZE)) & TABLE_MASK] * tcMod->func.params[1] + tcMod->func.params[0]);
				texCoordArray[unit][j][1] += (table[((int)(((vertexArray[j][1]) * 1.0/128 * 0.125 + now) * TABLE_SIZE)) & TABLE_MASK] * tcMod->func.params[1] + tcMod->func.params[0]);
			}

			break;
		case TCMOD_TRANSFORM:
			for (j = 0; j < numVertex; j++){
				st[0] = texCoordArray[unit][j][0];
				st[1] = texCoordArray[unit][j][1];
				texCoordArray[unit][j][0] = st[0] * tcMod->params[0] + st[1] * tcMod->params[2] + tcMod->params[4];
				texCoordArray[unit][j][1] = st[1] * tcMod->params[1] + st[0] * tcMod->params[3] + tcMod->params[5];
			}

			break;
		default:
			Com_Error(ERR_DROP, "RB_CalcTextureCoords: unknown tcMod type %i in shader '%s'", tcMod->type, rb_shader->name);
		}
	}
}

/*
 =================
 RB_SetupVertexProgram
 =================
*/
static void RB_SetupVertexProgram (shaderStage_t *stage){

	program_t	*program = stage->vertexProgram;

	qglBindProgramARB(GL_VERTEX_PROGRAM_ARB, program->progNum);

	qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0, r_refDef.viewOrigin[0], r_refDef.viewOrigin[1], r_refDef.viewOrigin[2], 0);
	qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 1, r_refDef.viewAxis[0][0], r_refDef.viewAxis[0][1], r_refDef.viewAxis[0][2], 0);
	qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 2, r_refDef.viewAxis[1][0], r_refDef.viewAxis[1][1], r_refDef.viewAxis[1][2], 0);
	qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 3, r_refDef.viewAxis[2][0], r_refDef.viewAxis[2][1], r_refDef.viewAxis[2][2], 0);
	qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 4, rb_entity->origin[0], rb_entity->origin[1], rb_entity->origin[2], 0);
	qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 5, rb_entity->axis[0][0], rb_entity->axis[0][1], rb_entity->axis[0][2], 0);
	qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 6, rb_entity->axis[1][0], rb_entity->axis[1][1], rb_entity->axis[1][2], 0);
	qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 7, rb_entity->axis[2][0], rb_entity->axis[2][1], rb_entity->axis[2][2], 0);
	qglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 8, rb_shaderTime, 0, 0, 0);
}

/*
 =================
 RB_SetupFragmentProgram
 =================
*/
static void RB_SetupFragmentProgram (shaderStage_t *stage){

	program_t	*program = stage->fragmentProgram;

	qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, program->progNum);

	qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, r_refDef.viewOrigin[0], r_refDef.viewOrigin[1], r_refDef.viewOrigin[2], 0);
	qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, r_refDef.viewAxis[0][0], r_refDef.viewAxis[0][1], r_refDef.viewAxis[0][2], 0);
	qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2, r_refDef.viewAxis[1][0], r_refDef.viewAxis[1][1], r_refDef.viewAxis[1][2], 0);
	qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 3, r_refDef.viewAxis[2][0], r_refDef.viewAxis[2][1], r_refDef.viewAxis[2][2], 0);
	qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 4, rb_entity->origin[0], rb_entity->origin[1], rb_entity->origin[2], 0);
	qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 5, rb_entity->axis[0][0], rb_entity->axis[0][1], rb_entity->axis[0][2], 0);
	qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 6, rb_entity->axis[1][0], rb_entity->axis[1][1], rb_entity->axis[1][2], 0);
	qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 7, rb_entity->axis[2][0], rb_entity->axis[2][1], rb_entity->axis[2][2], 0);
	qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 8, rb_shaderTime, 0, 0, 0);
}

/*
 =================
 RB_SetupTextureCombiners
 =================
*/
static void RB_SetupTextureCombiners (stageBundle_t *bundle){

	texEnvCombine_t	*texEnvCombine = &bundle->texEnvCombine;

	qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, texEnvCombine->rgbCombine);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, texEnvCombine->rgbSource[0]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, texEnvCombine->rgbSource[1]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, texEnvCombine->rgbSource[2]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, texEnvCombine->rgbOperand[0]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, texEnvCombine->rgbOperand[1]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, texEnvCombine->rgbOperand[2]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, texEnvCombine->rgbScale);

	qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, texEnvCombine->alphaCombine);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, texEnvCombine->alphaSource[0]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, texEnvCombine->alphaSource[1]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, texEnvCombine->alphaSource[2]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, texEnvCombine->alphaOperand[0]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, texEnvCombine->alphaOperand[1]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, texEnvCombine->alphaOperand[2]);
	qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, texEnvCombine->alphaScale);

	qglTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, texEnvCombine->constColor);
}

/*
 =================
 RB_SetShaderState
 =================
*/
static void RB_SetShaderState (void){

	if (rb_shader->flags & SHADER_CULL){
		GL_Enable(GL_CULL_FACE);
		GL_CullFace(rb_shader->cull.mode);
	}
	else
		GL_Disable(GL_CULL_FACE);

	if (rb_shader->flags & SHADER_POLYGONOFFSET){
		GL_Enable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}
	else
		GL_Disable(GL_POLYGON_OFFSET_FILL);
}

/*
 =================
 RB_SetShaderStageState
 =================
*/
static void RB_SetShaderStageState (shaderStage_t *stage){

	if (stage->flags & SHADERSTAGE_VERTEXPROGRAM){
		GL_Enable(GL_VERTEX_PROGRAM_ARB);
		RB_SetupVertexProgram(stage);
	}
	else
		GL_Disable(GL_VERTEX_PROGRAM_ARB);

	if (stage->flags & SHADERSTAGE_FRAGMENTPROGRAM){
		GL_Enable(GL_FRAGMENT_PROGRAM_ARB);
		RB_SetupFragmentProgram(stage);
	}
	else
		GL_Disable(GL_FRAGMENT_PROGRAM_ARB);

	if (stage->flags & SHADERSTAGE_ALPHAFUNC){
		GL_Enable(GL_ALPHA_TEST);
		GL_AlphaFunc(stage->alphaFunc.func, stage->alphaFunc.ref);
	}
	else
		GL_Disable(GL_ALPHA_TEST);

	if (stage->flags & SHADERSTAGE_BLENDFUNC){
		GL_Enable(GL_BLEND);
		GL_BlendFunc(stage->blendFunc.src, stage->blendFunc.dst);
	}
	else
		GL_Disable(GL_BLEND);

	if (stage->flags & SHADERSTAGE_DEPTHFUNC){
		GL_Enable(GL_DEPTH_TEST);
		GL_DepthFunc(stage->depthFunc.func);
	}
	else
		GL_Disable(GL_DEPTH_TEST);

	if (stage->flags & SHADERSTAGE_DEPTHWRITE)
		GL_DepthMask(GL_TRUE);
	else
		GL_DepthMask(GL_FALSE);
}

/*
 =================
 RB_SetupTextureUnit
 =================
*/
static void RB_SetupTextureUnit (stageBundle_t *bundle, unsigned unit){

	GL_SelectTexture(unit);

	switch (bundle->texType){
	case TEX_GENERIC:
		if (bundle->numTextures == 1)
			GL_BindTexture(bundle->textures[0]);
		else
			GL_BindTexture(bundle->textures[(int)(bundle->animFrequency * rb_shaderTime) % bundle->numTextures]);

		break;
	case TEX_LIGHTMAP:
		if (rb_infoKey != 255){
			GL_BindTexture(r_lightmapTextures[rb_infoKey]);
			break;
		}

		R_UpdateSurfaceLightmap(rb_mesh->mesh);

		break;
	case TEX_CINEMATIC:
		if (!r_inGameVideo->integer){
			GL_BindTexture(r_blackTexture);
			break;
		}

		CIN_RunCinematic(bundle->cinematicHandle);
		CIN_DrawCinematic(bundle->cinematicHandle);
		
		break;
	default:
		Com_Error(ERR_DROP, "RB_SetupTextureUnit: unknown texture type %i in shader '%s'", bundle->texType, rb_shader->name);
	}

	if (unit < glConfig.maxTextureUnits){
		if (bundle->flags & STAGEBUNDLE_CUBEMAP)
			qglEnable(GL_TEXTURE_CUBE_MAP_ARB);
		else
			qglEnable(GL_TEXTURE_2D);

		GL_TexEnv(bundle->texEnv);

		if (bundle->flags & STAGEBUNDLE_TEXENVCOMBINE)
			RB_SetupTextureCombiners(bundle);
	}

	if (bundle->tcGen.type == TCGEN_REFLECTION || bundle->tcGen.type == TCGEN_NORMAL){
		qglMatrixMode(GL_TEXTURE);
		qglLoadMatrixf(r_textureMatrix);
		qglMatrixMode(GL_MODELVIEW);

		qglEnable(GL_TEXTURE_GEN_S);
		qglEnable(GL_TEXTURE_GEN_T);
		qglEnable(GL_TEXTURE_GEN_R);
	}
}

/*
 =================
 RB_CleanupTextureUnit
 =================
*/
static void RB_CleanupTextureUnit (stageBundle_t *bundle, unsigned unit){

	GL_SelectTexture(unit);

	if (bundle->tcGen.type == TCGEN_REFLECTION || bundle->tcGen.type == TCGEN_NORMAL){
		qglDisable(GL_TEXTURE_GEN_S);
		qglDisable(GL_TEXTURE_GEN_T);
		qglDisable(GL_TEXTURE_GEN_R);

		qglMatrixMode(GL_TEXTURE);
		qglLoadIdentity();
		qglMatrixMode(GL_MODELVIEW);
	}

	if (unit < glConfig.maxTextureUnits){
		if (bundle->flags & STAGEBUNDLE_CUBEMAP)
			qglDisable(GL_TEXTURE_CUBE_MAP_ARB);
		else
			qglDisable(GL_TEXTURE_2D);
	}
}

/*
 =================
 RB_RenderShaderARB
 =================
*/
static void RB_RenderShaderARB (void){

	shaderStage_t	*stage;
	stageBundle_t	*bundle;
	int				i, j;

	if (r_logFile->integer)
		QGL_LogPrintf("--- RB_RenderShaderARB( %s ) ---\n", rb_shader->name);

	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, rb_vbo.indexBuffer);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, numIndex * sizeof(unsigned), indexArray, GL_STREAM_DRAW_ARB);

	RB_SetShaderState();

	RB_DeformVertexes();

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, rb_vbo.vertexBuffer);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, numVertex * sizeof(vec3_t), vertexArray, GL_STREAM_DRAW_ARB);
	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, 0, VBO_OFFSET(0));

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, rb_vbo.normalBuffer);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, numVertex * sizeof(vec3_t), normalArray, GL_STREAM_DRAW_ARB);
	qglEnableClientState(GL_NORMAL_ARRAY);
	qglNormalPointer(GL_FLOAT, 0, VBO_OFFSET(0));

	for (i = 0; i < rb_shader->numStages; i++){
		stage = rb_shader->stages[i];

		RB_SetShaderStageState(stage);

		RB_CalcVertexColors(stage);

		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, rb_vbo.colorBuffer);
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, numVertex * sizeof(color_t), colorArray, GL_STREAM_DRAW_ARB);
		qglEnableClientState(GL_COLOR_ARRAY);
		qglColorPointer(4, GL_UNSIGNED_BYTE, 0, VBO_OFFSET(0));

		for (j = 0; j < stage->numBundles; j++){
			bundle = stage->bundles[j];

			RB_SetupTextureUnit(bundle, j);

			RB_CalcTextureCoords(bundle, j);

			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, rb_vbo.texCoordBuffer[j]);
			qglBufferDataARB(GL_ARRAY_BUFFER_ARB, numVertex * sizeof(vec3_t), texCoordArray[j], GL_STREAM_DRAW_ARB);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, 0, VBO_OFFSET(0));
		}

		if (glConfig.drawRangeElements)
			qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numVertex, numIndex, GL_UNSIGNED_INT, VBO_OFFSET(0));
		else
			qglDrawElements(GL_TRIANGLES, numIndex, GL_UNSIGNED_INT, VBO_OFFSET(0));

		for (j = stage->numBundles - 1; j >= 0; j--){
			bundle = stage->bundles[j];

			RB_CleanupTextureUnit(bundle, j);

			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}

	if (r_logFile->integer)
		QGL_LogPrintf("-----------------------------\n");
}

/*
 =================
 RB_RenderShader
 =================
*/
static void RB_RenderShader (void){

	shaderStage_t	*stage;
	stageBundle_t	*bundle;
	int				i, j;

	if (r_logFile->integer)
		QGL_LogPrintf("--- RB_RenderShader( %s ) ---\n", rb_shader->name);

	RB_SetShaderState();

	RB_DeformVertexes();

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, 0, vertexArray);

	qglEnableClientState(GL_NORMAL_ARRAY);
	qglNormalPointer(GL_FLOAT, 0, normalArray);

	if (glConfig.compiledVertexArray){
		if (rb_shader->numStages != 1){
			qglDisableClientState(GL_COLOR_ARRAY);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

			qglLockArraysEXT(0, numVertex);
		}
	}

	for (i = 0; i < rb_shader->numStages; i++){
		stage = rb_shader->stages[i];

		RB_SetShaderStageState(stage);

		RB_CalcVertexColors(stage);

		qglEnableClientState(GL_COLOR_ARRAY);
		qglColorPointer(4, GL_UNSIGNED_BYTE, 0, colorArray);

		for (j = 0; j < stage->numBundles; j++){
			bundle = stage->bundles[j];

			RB_SetupTextureUnit(bundle, j);

			RB_CalcTextureCoords(bundle, j);

			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			qglTexCoordPointer(3, GL_FLOAT, 0, texCoordArray[j]);
		}

		if (glConfig.compiledVertexArray){
			if (rb_shader->numStages == 1)
				qglLockArraysEXT(0, numVertex);
		}

		if (glConfig.drawRangeElements)
			qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numVertex, numIndex, GL_UNSIGNED_INT, indexArray);
		else
			qglDrawElements(GL_TRIANGLES, numIndex, GL_UNSIGNED_INT, indexArray);

		for (j = stage->numBundles - 1; j >= 0; j--){
			bundle = stage->bundles[j];

			RB_CleanupTextureUnit(bundle, j);

			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}

	if (glConfig.compiledVertexArray)
		qglUnlockArraysEXT();

	if (r_logFile->integer)
		QGL_LogPrintf("-----------------------------\n");
}

/*
 =================
 RB_DrawTris
 =================
*/
static void RB_DrawTris (void){

	qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	qglColor4ub(255, 255, 255, 255);

	qglDisableClientState(GL_NORMAL_ARRAY);
	qglDisableClientState(GL_COLOR_ARRAY);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (glConfig.vertexBufferObject){
		if (glConfig.drawRangeElements)
			qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numVertex, numIndex, GL_UNSIGNED_INT, VBO_OFFSET(0));
		else
			qglDrawElements(GL_TRIANGLES, numIndex, GL_UNSIGNED_INT, VBO_OFFSET(0));
	}
	else {
		if (glConfig.compiledVertexArray)
			qglLockArraysEXT(0, numVertex);

		if (glConfig.drawRangeElements)
			qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numVertex, numIndex, GL_UNSIGNED_INT, indexArray);
		else
			qglDrawElements(GL_TRIANGLES, numIndex, GL_UNSIGNED_INT, indexArray);

		if (glConfig.compiledVertexArray)
			qglUnlockArraysEXT();
	}

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/*
 =================
 RB_DrawNormals
 =================
*/
static void RB_DrawNormals (void){

	int		i;
	vec3_t	v;

	if (rb_mesh->meshType == MESH_POLY)
		return;

	qglColor4ub(255, 255, 255, 255);

	qglBegin(GL_LINES);
	for (i = 0; i < numVertex; i++){
		VectorAdd(vertexArray[i], normalArray[i], v);

		qglVertex3fv(vertexArray[i]);
		qglVertex3fv(v);
	}
	qglEnd();
}

/*
 =================
 RB_DrawTangentSpace
 =================
*/
static void RB_DrawTangentSpace (void){

	int		i;
	vec3_t	v;

	if (rb_mesh->meshType != MESH_SURFACE && rb_mesh->meshType != MESH_ALIASMODEL)
		return;

	qglColor4ub(255, 0, 0, 255);

	qglBegin(GL_LINES);
	for (i = 0; i < numVertex; i++){
		VectorAdd(vertexArray[i], tangentArray[i], v);

		qglVertex3fv(vertexArray[i]);
		qglVertex3fv(v);
	}
	qglEnd();

	qglColor4ub(0, 255, 0, 255);

	qglBegin(GL_LINES);
	for (i = 0; i < numVertex; i++){
		VectorAdd(vertexArray[i], binormalArray[i], v);

		qglVertex3fv(vertexArray[i]);
		qglVertex3fv(v);
	}
	qglEnd();

	qglColor4ub(0, 0, 255, 255);

	qglBegin(GL_LINES);
	for (i = 0; i < numVertex; i++){
		VectorAdd(vertexArray[i], normalArray[i], v);

		qglVertex3fv(vertexArray[i]);
		qglVertex3fv(v);
	}
	qglEnd();
}

/*
 =================
 RB_DrawModelBounds
 =================
*/
static void RB_DrawModelBounds (void){

	model_t		*model;
	mdl_t		*alias;
	mdlFrame_t	*curFrame, *oldFrame;
	vec3_t		mins, maxs, bbox[8];
	int			i;

	if (rb_entity == r_worldEntity)
		return;

	if (rb_mesh->meshType == MESH_SURFACE){
		model = rb_entity->model;

		// Compute a full bounding box
		for (i = 0; i < 8; i++){
			bbox[i][0] = (i & 1) ? model->mins[0] : model->maxs[0];
			bbox[i][1] = (i & 2) ? model->mins[1] : model->maxs[1];
			bbox[i][2] = (i & 4) ? model->mins[2] : model->maxs[2];
		}
	}
	else if (rb_mesh->meshType == MESH_ALIASMODEL){
		alias = rb_mesh->mesh;

		// Compute axially aligned mins and maxs
		curFrame = alias->frames + rb_entity->frame;
		oldFrame = alias->frames + rb_entity->oldFrame;

		if (curFrame == oldFrame){
			VectorCopy(curFrame->mins, mins);
			VectorCopy(curFrame->maxs, maxs);
		}
		else {
			for (i = 0; i < 3; i++){
				if (curFrame->mins[i] < oldFrame->mins[i])
					mins[i] = curFrame->mins[i];
				else
					mins[i] = oldFrame->mins[i];

				if (curFrame->maxs[i] > oldFrame->maxs[i])
					maxs[i] = curFrame->maxs[i];
				else
					maxs[i] = oldFrame->maxs[i];
			}
		}

		// Compute a full bounding box
		for (i = 0; i < 8; i++){
			bbox[i][0] = (i & 1) ? mins[0] : maxs[0];
			bbox[i][1] = (i & 2) ? mins[1] : maxs[1];
			bbox[i][2] = (i & 4) ? mins[2] : maxs[2];
		}
	}
	else
		return;

	// Draw it
	qglColor4ub(255, 255, 255, 255);

	qglBegin(GL_LINES);
	for (i = 0; i < 2; i += 1){
		qglVertex3fv(bbox[i+0]);
		qglVertex3fv(bbox[i+2]);
		qglVertex3fv(bbox[i+4]);
		qglVertex3fv(bbox[i+6]);
		qglVertex3fv(bbox[i+0]);
		qglVertex3fv(bbox[i+4]);
		qglVertex3fv(bbox[i+2]);
		qglVertex3fv(bbox[i+6]);
		qglVertex3fv(bbox[i*2+0]);
		qglVertex3fv(bbox[i*2+1]);
		qglVertex3fv(bbox[i*2+4]);
		qglVertex3fv(bbox[i*2+5]);
	}
	qglEnd();
}

/*
 =================
 RB_DrawDebugTools
 =================
*/
static void RB_DrawDebugTools (void){

	if (glState.gl2D)
		return;

	if (r_logFile->integer)
		QGL_LogPrintf("--- RB_DrawDebugTools( %s ) ---\n", rb_shader->name);

	GL_Disable(GL_VERTEX_PROGRAM_ARB);
	GL_Disable(GL_FRAGMENT_PROGRAM_ARB);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_BLEND);
	GL_DepthFunc(GL_LEQUAL);
	GL_DepthMask(GL_TRUE);

	qglDepthRange(0, 0);

	if (r_showTris->integer)
		RB_DrawTris();

	if (r_showNormals->integer)
		RB_DrawNormals();

	if (r_showTangentSpace->integer)
		RB_DrawTangentSpace();

	if (r_showModelBounds->integer)
		RB_DrawModelBounds();

	qglDepthRange(0, 1);

	if (r_logFile->integer)
		QGL_LogPrintf("-----------------------------\n");
}

/*
 =================
 RB_CheckMeshOverflow
 =================
*/
void RB_CheckMeshOverflow (int numIndices, int numVertices){

	if (numIndices > MAX_INDICES || numVertices > MAX_VERTICES)
		Com_Error(ERR_DROP, "RB_CheckMeshOverflow: %i > MAX_INDICES or %i > MAX_VERTICES", numIndices, numVertices);

	if (numIndex + numIndices <= MAX_INDICES && numVertex + numVertices <= MAX_VERTICES)
		return;

	RB_RenderMesh();
}

/*
 =================
 RB_RenderMesh
 =================
*/
void RB_RenderMesh (void){

	if (!numIndex || !numVertex)
		return;

	// Update r_speeds statistics
	r_stats.numShaders++;
	r_stats.numStages += rb_shader->numStages;
	r_stats.numVertices += numVertex;
	r_stats.numIndices += numIndex;
	r_stats.totalIndices += numIndex * rb_shader->numStages;

	// Render the shader
	if (glConfig.vertexBufferObject)
		RB_RenderShaderARB();
	else
		RB_RenderShader();

	// Draw debug tools
	if (r_showTris->integer || r_showNormals->integer || r_showTangentSpace->integer || r_showModelBounds->integer)
		RB_DrawDebugTools();

	// Check for errors
	if (!r_ignoreGLErrors->integer)
		GL_CheckForErrors();

	// Clear arrays
	numIndex = numVertex = 0;
}

/*
 =================
 RB_RenderMeshes
 =================
*/
void RB_RenderMeshes (mesh_t *meshes, int numMeshes){

	int			i;
	mesh_t		*mesh;
	shader_t	*shader;
	entity_t	*entity;
	int			infoKey;
	unsigned	sortKey = 0;

	if (r_skipBackEnd->integer || !numMeshes)
		return;

	r_stats.numMeshes += numMeshes;

	// Clear the state
	rb_mesh = NULL;
	rb_shader = NULL;
	rb_shaderTime = 0;
	rb_entity = NULL;
	rb_infoKey = -1;

	// Draw everything
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Check for changes
		if (sortKey != mesh->sortKey || (mesh->sortKey & 255) == 255){
			sortKey = mesh->sortKey;

			// Unpack sort key
			shader = r_shaders[(sortKey >> 18) & 1023];
			entity = &r_entities[(sortKey >> 8) & 1023];
			infoKey = sortKey & 255;

			// Development tool
			if (r_debugSort->integer){
				if (r_debugSort->integer != shader->sort)
					continue;
			}

			// Check if the rendering state changed
			if ((rb_shader != shader) || (rb_entity != entity && !(shader->flags & SHADER_ENTITYMERGABLE)) || (rb_infoKey != infoKey || infoKey == 255)){
				RB_RenderMesh();

				rb_shader = shader;
				rb_infoKey = infoKey;
			}

			// Check if the entity changed
			if (rb_entity != entity){
				if (entity == r_worldEntity || entity->entityType != ET_MODEL)
					qglLoadMatrixf(r_worldMatrix);
				else
					R_RotateForEntity(entity);

				rb_entity = entity;
				rb_shaderTime = r_refDef.time - entity->shaderTime;
			}
		}

		// Set the current mesh
		rb_mesh = mesh;

		// Feed arrays
		switch (rb_mesh->meshType){
		case MESH_SKY:
			R_DrawSky();
			break;
		case MESH_SURFACE:
			R_DrawSurface();
			break;
		case MESH_ALIASMODEL:
			R_DrawAliasModel();
			break;
		case MESH_SPRITE:
			R_DrawSprite();
			break;
		case MESH_BEAM:
			R_DrawBeam();
			break;
		case MESH_PARTICLE:
			R_DrawParticle();
			break;
		case MESH_POLY:
			R_DrawPoly();
			break;
		default:
			Com_Error(ERR_DROP, "RB_RenderMeshes: bad meshType (%i)", rb_mesh->meshType);
		}
	}

	// Make sure everything is flushed
	RB_RenderMesh();
}

/*
 =================
 RB_DrawStretchPic
 =================
*/
void RB_DrawStretchPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, const color_t modulate, shader_t *shader){

	int		i;

	if (r_skipBackEnd->integer)
		return;

	// Check if the rendering state changed
	if (rb_shader != shader){
		RB_RenderMesh();

		rb_shader = shader;
		rb_shaderTime = r_frameTime;
	}

	// Check if the arrays will overflow
	RB_CheckMeshOverflow(6, 4);

	// Draw it
	for (i = 2; i < 4; i++){
		indexArray[numIndex++] = numVertex + 0;
		indexArray[numIndex++] = numVertex + i-1;
		indexArray[numIndex++] = numVertex + i;
	}

	vertexArray[numVertex+0][0] = x;
	vertexArray[numVertex+0][1] = y;
	vertexArray[numVertex+0][2] = 0;
	vertexArray[numVertex+1][0] = x + w;
	vertexArray[numVertex+1][1] = y;
	vertexArray[numVertex+1][2] = 0;
	vertexArray[numVertex+2][0] = x + w;
	vertexArray[numVertex+2][1] = y + h;
	vertexArray[numVertex+2][2] = 0;
	vertexArray[numVertex+3][0] = x;
	vertexArray[numVertex+3][1] = y + h;
	vertexArray[numVertex+3][2] = 0;

	inTexCoordArray[numVertex+0][0] = sl;
	inTexCoordArray[numVertex+0][1] = tl;
	inTexCoordArray[numVertex+1][0] = sh;
	inTexCoordArray[numVertex+1][1] = tl;
	inTexCoordArray[numVertex+2][0] = sh;
	inTexCoordArray[numVertex+2][1] = th;
	inTexCoordArray[numVertex+3][0] = sl;
	inTexCoordArray[numVertex+3][1] = th;

	for (i = 0; i < 4; i++){
		inColorArray[numVertex][0] = modulate[0];
		inColorArray[numVertex][1] = modulate[1];
		inColorArray[numVertex][2] = modulate[2];
		inColorArray[numVertex][3] = modulate[3];

		numVertex++;
	}
}

/*
 =================
 RB_DrawRotatedPic
 =================
*/
void RB_DrawRotatedPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, float angle, const color_t modulate, shader_t *shader){

	float	rad, s, c;
	int		i;

	if (r_skipBackEnd->integer)
		return;

	rad = -DEG2RAD(angle);
	s = sin(rad);
	c = cos(rad);

	// Check if the rendering state changed
	if (rb_shader != shader){
		RB_RenderMesh();

		rb_shader = shader;
		rb_shaderTime = r_frameTime;
	}

	// Check if the arrays will overflow
	RB_CheckMeshOverflow(6, 4);

	// Draw it
	for (i = 2; i < 4; i++){
		indexArray[numIndex++] = numVertex + 0;
		indexArray[numIndex++] = numVertex + i-1;
		indexArray[numIndex++] = numVertex + i;
	}

	vertexArray[numVertex+0][0] = x;
	vertexArray[numVertex+0][1] = y;
	vertexArray[numVertex+0][2] = 0;
	vertexArray[numVertex+1][0] = x + w;
	vertexArray[numVertex+1][1] = y;
	vertexArray[numVertex+1][2] = 0;
	vertexArray[numVertex+2][0] = x + w;
	vertexArray[numVertex+2][1] = y + h;
	vertexArray[numVertex+2][2] = 0;
	vertexArray[numVertex+3][0] = x;
	vertexArray[numVertex+3][1] = y + h;
	vertexArray[numVertex+3][2] = 0;

	inTexCoordArray[numVertex+0][0] = c * (sl - 0.5) - s * (tl - 0.5) + 0.5;
	inTexCoordArray[numVertex+0][1] = c * (tl - 0.5) + s * (sl - 0.5) + 0.5;
	inTexCoordArray[numVertex+1][0] = c * (sh - 0.5) - s * (tl - 0.5) + 0.5;
	inTexCoordArray[numVertex+1][1] = c * (tl - 0.5) + s * (sh - 0.5) + 0.5;
	inTexCoordArray[numVertex+2][0] = c * (sh - 0.5) - s * (th - 0.5) + 0.5;
	inTexCoordArray[numVertex+2][1] = c * (th - 0.5) + s * (sh - 0.5) + 0.5;
	inTexCoordArray[numVertex+3][0] = c * (sl - 0.5) - s * (th - 0.5) + 0.5;
	inTexCoordArray[numVertex+3][1] = c * (th - 0.5) + s * (sl - 0.5) + 0.5;

	for (i = 0; i < 4; i++){
		inColorArray[numVertex][0] = modulate[0];
		inColorArray[numVertex][1] = modulate[1];
		inColorArray[numVertex][2] = modulate[2];
		inColorArray[numVertex][3] = modulate[3];

		numVertex++;
	}
}

/*
 =================
 RB_DrawOffsetPic
 =================
*/
void RB_DrawOffsetPic (float x, float y, float w, float h, float sl, float tl, float sh, float th, float offsetX, float offsetY, const color_t modulate, shader_t *shader){

	int		i;

	if (r_skipBackEnd->integer)
		return;

	// Check if the rendering state changed
	if (rb_shader != shader){
		RB_RenderMesh();

		rb_shader = shader;
		rb_shaderTime = r_frameTime;
	}

	// Check if the arrays will overflow
	RB_CheckMeshOverflow(6, 4);

	// Draw it
	for (i = 2; i < 4; i++){
		indexArray[numIndex++] = numVertex + 0;
		indexArray[numIndex++] = numVertex + i-1;
		indexArray[numIndex++] = numVertex + i;
	}

	vertexArray[numVertex+0][0] = x + offsetX;
	vertexArray[numVertex+0][1] = y + offsetY;
	vertexArray[numVertex+0][2] = 0;
	vertexArray[numVertex+1][0] = x + w + offsetX;
	vertexArray[numVertex+1][1] = y - offsetY;
	vertexArray[numVertex+1][2] = 0;
	vertexArray[numVertex+2][0] = x + w - offsetX;
	vertexArray[numVertex+2][1] = y + h - offsetY;
	vertexArray[numVertex+2][2] = 0;
	vertexArray[numVertex+3][0] = x - offsetX;
	vertexArray[numVertex+3][1] = y + h + offsetY;
	vertexArray[numVertex+3][2] = 0;

	inTexCoordArray[numVertex+0][0] = sl;
	inTexCoordArray[numVertex+0][1] = tl;
	inTexCoordArray[numVertex+1][0] = sh;
	inTexCoordArray[numVertex+1][1] = tl;
	inTexCoordArray[numVertex+2][0] = sh;
	inTexCoordArray[numVertex+2][1] = th;
	inTexCoordArray[numVertex+3][0] = sl;
	inTexCoordArray[numVertex+3][1] = th;

	for (i = 0; i < 4; i++){
		inColorArray[numVertex][0] = modulate[0];
		inColorArray[numVertex][1] = modulate[1];
		inColorArray[numVertex][2] = modulate[2];
		inColorArray[numVertex][3] = modulate[3];

		numVertex++;
	}
}

/*
 =================
 RB_VBOInfo_f
 =================
*/
void RB_VBOInfo_f (void){

	if (!glConfig.vertexBufferObject){
		Com_Printf("GL_ARB_vertex_buffer_object extension is disabled or not supported\n");
		return;
	}

	Com_Printf("%i bytes in %i static buffers\n", rb_staticBytes, rb_staticCount);
	Com_Printf("%i bytes in %i stream buffers\n", rb_streamBytes, rb_streamCount);
}

/*
 =================
 RB_AllocStaticBuffer
 =================
*/
unsigned RB_AllocStaticBuffer (unsigned target, int size){

	unsigned	buffer;

	if (rb_numVertexBuffers == MAX_VERTEX_BUFFERS)
		Com_Error(ERR_DROP, "RB_AllocStaticBuffer: MAX_VERTEX_BUFFERS hit");

	qglGenBuffersARB(1, &buffer);
	qglBindBufferARB(target, buffer);
	qglBufferDataARB(target, size, NULL, GL_STATIC_DRAW_ARB);

	rb_vertexBuffers[rb_numVertexBuffers++] = buffer;

	rb_staticBytes += size;
	rb_staticCount++;

	return buffer;
}

/*
 =================
 RB_AllocStreamBuffer
 =================
*/
unsigned RB_AllocStreamBuffer (unsigned target, int size){

	unsigned	buffer;

	if (rb_numVertexBuffers == MAX_VERTEX_BUFFERS)
		Com_Error(ERR_DROP, "RB_AllocStreamBuffer: MAX_VERTEX_BUFFERS hit");

	qglGenBuffersARB(1, &buffer);
	qglBindBufferARB(target, buffer);
	qglBufferDataARB(target, size, NULL, GL_STREAM_DRAW_ARB);

	rb_vertexBuffers[rb_numVertexBuffers++] = buffer;

	rb_streamBytes += size;
	rb_streamCount++;

	return buffer;
}

/*
 =================
 RB_InitBackend
 =================
*/
void RB_InitBackend (void){

	int		i;

	// Build waveform tables
	RB_BuildTables();

	// Clear the state
	rb_mesh = NULL;
	rb_shader = NULL;
	rb_shaderTime = 0;
	rb_entity = NULL;
	rb_infoKey = -1;

	// Clear arrays
	numIndex = numVertex = 0;

	// Set default GL state
	GL_SetDefaultState();

	// Create vertex buffers
	if (glConfig.vertexBufferObject){
		rb_vbo.indexBuffer = RB_AllocStreamBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, MAX_INDICES * 4 * sizeof(unsigned));

		rb_vbo.vertexBuffer = RB_AllocStreamBuffer(GL_ARRAY_BUFFER_ARB, MAX_VERTICES * 2 * sizeof(vec3_t));
		rb_vbo.normalBuffer = RB_AllocStreamBuffer(GL_ARRAY_BUFFER_ARB, MAX_VERTICES * sizeof(vec3_t));
		rb_vbo.colorBuffer = RB_AllocStreamBuffer(GL_ARRAY_BUFFER_ARB, MAX_VERTICES * sizeof(color_t));
		rb_vbo.texCoordBuffer[0] = RB_AllocStreamBuffer(GL_ARRAY_BUFFER_ARB, MAX_VERTICES * sizeof(vec3_t));

		if (glConfig.multitexture){
			for (i = 1; i < MAX_TEXTURE_UNITS; i++){
				if (glConfig.fragmentProgram){
					if (i >= glConfig.maxTextureUnits && (i >= glConfig.maxTextureCoords || i >= glConfig.maxTextureImageUnits))
						break;
				}
				else {
					if (i >= glConfig.maxTextureUnits)
						break;
				}

				rb_vbo.texCoordBuffer[i] = RB_AllocStreamBuffer(GL_ARRAY_BUFFER_ARB, MAX_VERTICES * sizeof(vec3_t));
			}
		}
	}
}

/*
 =================
 RB_ShutdownBackend
 =================
*/
void RB_ShutdownBackend (void){

	int		i;

	// Disable arrays
	if (glConfig.multitexture){
		for (i = MAX_TEXTURE_UNITS - 1; i > 0; i--){
			if (glConfig.fragmentProgram){
				if (i >= glConfig.maxTextureUnits && (i >= glConfig.maxTextureCoords || i >= glConfig.maxTextureImageUnits))
					continue;
			}
			else {
				if (i >= glConfig.maxTextureUnits)
					continue;
			}

			GL_SelectTexture(i);
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		GL_SelectTexture(0);
	}

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	qglDisableClientState(GL_COLOR_ARRAY);
	qglDisableClientState(GL_NORMAL_ARRAY);
	qglDisableClientState(GL_VERTEX_ARRAY);

	// Delete vertex buffers
	if (glConfig.vertexBufferObject){
		for (i = 0; i < rb_numVertexBuffers; i++)
			qglDeleteBuffersARB(1, &rb_vertexBuffers[i]);

		memset(rb_vertexBuffers, 0, sizeof(rb_vertexBuffers));

		rb_numVertexBuffers = 0;

		rb_staticBytes = 0;
		rb_staticCount = 0;

		rb_streamBytes = 0;
		rb_streamCount = 0;
	}
}
