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


// r_shader.c -- shader script parsing and loading


#include "r_local.h"


#define SHADERS_HASHSIZE	256

typedef struct shaderScript_s {
	struct shaderScript_t	*nextHash;

	char					name[MAX_QPATH];
	shaderType_t			shaderType;
	unsigned				surfaceParm;
	char					script[1];			// Variable sized
} shaderScript_t;

static shader_t			r_parseShader;
static shaderStage_t	r_parseShaderStages[SHADER_MAX_STAGES];
static stageBundle_t	r_parseStageTMU[SHADER_MAX_STAGES][MAX_TEXTURE_UNITS];

static shaderScript_t	*r_shaderScriptsHash[SHADERS_HASHSIZE];
static shader_t			*r_shadersHash[SHADERS_HASHSIZE];

shader_t				*r_shaders[MAX_SHADERS];
int						r_numShaders = 0;

shader_t				*r_defaultShader;
shader_t				*r_lightmapShader;
shader_t				*r_waterCausticsShader;
shader_t				*r_slimeCausticsShader;
shader_t				*r_lavaCausticsShader;


/*
 =======================================================================

 SHADER PARSING

 =======================================================================
*/


/*
 =================
 R_ParseWaveFunc
 =================
*/
static qboolean R_ParseWaveFunc (shader_t *shader, waveFunc_t *func, char **script){

	char	*tok;
	int		i;

	tok = Com_ParseExt(script, false);
	if (!tok[0])
		return false;

	if (!Q_stricmp(tok, "sin"))
		func->type = WAVEFORM_SIN;
	else if (!Q_stricmp(tok, "triangle"))
		func->type = WAVEFORM_TRIANGLE;
	else if (!Q_stricmp(tok, "square"))
		func->type = WAVEFORM_SQUARE;
	else if (!Q_stricmp(tok, "sawtooth"))
		func->type = WAVEFORM_SAWTOOTH;
	else if (!Q_stricmp(tok, "inverseSawtooth"))
		func->type = WAVEFORM_INVERSESAWTOOTH;
	else if (!Q_stricmp(tok, "noise"))
		func->type = WAVEFORM_NOISE;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown waveform '%s' in shader '%s', defaulting to sin\n", tok, shader->name);
		func->type = WAVEFORM_SIN;
	}

	for (i = 0; i < 4; i++){
		tok = Com_ParseExt(script, false);
		if (!tok[0])
			return false;

		func->params[i] = atof(tok);
	}

	return true;
}

/*
 =================
 R_ParseHeightToNormal
 =================
*/
static qboolean R_ParseHeightToNormal (shader_t *shader, char *heightMap, int heightMapLen, float *bumpScale, char **script){

	char	*tok;

	tok = Com_ParseExt(script, false);
	if (Q_stricmp(tok, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing '(' for 'heightToNormal' in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'heightToNormal' in shader '%s'\n", shader->name);
		return false;
	}
	Q_strncpyz(heightMap, tok, heightMapLen);

	tok = Com_ParseExt(script, false);
	if (Q_stricmp(tok, ",")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in 'heightToNormal' in shader '%s'\n", tok, shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'heightToNormal' in shader '%s'\n", shader->name);
		return false;
	}
	*bumpScale = atof(tok);

	tok = Com_ParseExt(script, false);
	if (Q_stricmp(tok, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing ')' for 'heightToNormal' in shader '%s'\n", shader->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseGeneralSurfaceParm
 =================
*/
static qboolean R_ParseGeneralSurfaceParm (shader_t *shader, char **script){

	char	*tok;

	if (shader->shaderType != SHADER_BSP){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'surfaceParm' not allowed in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'surfaceParm' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "lightmap"))
		shader->surfaceParm |= SURFACEPARM_LIGHTMAP;
	else if (!Q_stricmp(tok, "warp"))
		shader->surfaceParm |= SURFACEPARM_WARP;
	else if (!Q_stricmp(tok, "trans33"))
		shader->surfaceParm |= SURFACEPARM_TRANS33;
	else if (!Q_stricmp(tok, "trans66"))
		shader->surfaceParm |= SURFACEPARM_TRANS66;
	else if (!Q_stricmp(tok, "flowing"))
		shader->surfaceParm |= SURFACEPARM_FLOWING;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'surfaceParm' parameter '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	shader->flags |= SHADER_SURFACEPARM;

	return true;
}

/*
 =================
 R_ParseGeneralNoMipmaps
 =================
*/
static qboolean R_ParseGeneralNoMipmaps (shader_t *shader, char **script){

	shader->flags |= (SHADER_NOMIPMAPS | SHADER_NOPICMIP);

	return true;
}

/*
 =================
 R_ParseGeneralNoPicmip
 =================
*/
static qboolean R_ParseGeneralNoPicmip (shader_t *shader, char **script){

	shader->flags |= SHADER_NOPICMIP;

	return true;
}

/*
 =================
 R_ParseGeneralNoCompress
 =================
*/
static qboolean R_ParseGeneralNoCompress (shader_t *shader, char **script){

	shader->flags |= SHADER_NOCOMPRESS;

	return true;
}

/*
 =================
 R_ParseGeneralNoShadows
 =================
*/
static qboolean R_ParseGeneralNoShadows (shader_t *shader, char **script){

	shader->flags |= SHADER_NOSHADOWS;

	return true;
}

/*
 =================
 R_ParseGeneralNoFragments
 =================
*/
static qboolean R_ParseGeneralNoFragments (shader_t *shader, char **script){

	shader->flags |= SHADER_NOFRAGMENTS;

	return true;
}

/*
 =================
 R_ParseGeneralEntityMergable
 =================
*/
static qboolean R_ParseGeneralEntityMergable (shader_t *shader, char **script){

	shader->flags |= SHADER_ENTITYMERGABLE;

	return true;
}

/*
 =================
 R_ParseGeneralPolygonOffset
 =================
*/
static qboolean R_ParseGeneralPolygonOffset (shader_t *shader, char **script){

	shader->flags |= SHADER_POLYGONOFFSET;

	return true;
}

/*
 =================
 R_ParseGeneralCull
 =================
*/
static qboolean R_ParseGeneralCull (shader_t *shader, char **script){

	char	*tok;

	tok = Com_ParseExt(script, false);
	if (!tok[0])
		shader->cull.mode = GL_FRONT;
	else {
		if (!Q_stricmp(tok, "front"))
			shader->cull.mode = GL_FRONT;
		else if (!Q_stricmp(tok, "back") || !Q_stricmp(tok, "backSide") || !Q_stricmp(tok, "backSided"))
			shader->cull.mode = GL_BACK;
		else if (!Q_stricmp(tok, "disable") || !Q_stricmp(tok, "none") || !Q_stricmp(tok, "twoSided"))
			shader->cull.mode = 0;
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'cull' parameter '%s' in shader '%s'\n", tok, shader->name);
			return false;
		}
	}

	shader->flags |= SHADER_CULL;

	return true;
}

/*
 =================
 R_ParseGeneralSort
 =================
*/
static qboolean R_ParseGeneralSort (shader_t *shader, char **script){

	char	*tok;

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'sort' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "sky"))
		shader->sort = SORT_SKY;
	else if (!Q_stricmp(tok, "opaque"))
		shader->sort = SORT_OPAQUE;
	else if (!Q_stricmp(tok, "decal"))
		shader->sort = SORT_DECAL;
	else if (!Q_stricmp(tok, "seeThrough"))
		shader->sort = SORT_SEETHROUGH;
	else if (!Q_stricmp(tok, "banner"))
		shader->sort = SORT_BANNER;
	else if (!Q_stricmp(tok, "underwater"))
		shader->sort = SORT_UNDERWATER;
	else if (!Q_stricmp(tok, "water"))
		shader->sort = SORT_WATER;
	else if (!Q_stricmp(tok, "innerBlend"))
		shader->sort = SORT_INNERBLEND;
	else if (!Q_stricmp(tok, "blend"))
		shader->sort = SORT_BLEND;
	else if (!Q_stricmp(tok, "blend2"))
		shader->sort = SORT_BLEND2;
	else if (!Q_stricmp(tok, "blend3"))
		shader->sort = SORT_BLEND3;
	else if (!Q_stricmp(tok, "blend4"))
		shader->sort = SORT_BLEND4;
	else if (!Q_stricmp(tok, "outerBlend"))
		shader->sort = SORT_OUTERBLEND;
	else if (!Q_stricmp(tok, "additive"))
		shader->sort = SORT_ADDITIVE;
	else if (!Q_stricmp(tok, "nearest"))
		shader->sort = SORT_NEAREST;
	else {
		shader->sort = atoi(tok);

		if (shader->sort < 1 || shader->sort > 15){
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'sort' parameter '%s' in shader '%s'\n", tok, shader->name);
			return false;
		}
	}

	shader->flags |= SHADER_SORT;

	return true;
}

/*
 =================
 R_ParseGeneralAmmoDisplay
 =================
*/
static qboolean R_ParseGeneralAmmoDisplay (shader_t *shader, char **script){

	char	*tok;
	int		i;

	if (shader->shaderType != SHADER_SKIN){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'ammoDisplay' not allowed in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'ammoDisplay' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "digit1"))
		shader->ammoDisplay.type = AMMODISPLAY_DIGIT1;
	else if (!Q_stricmp(tok, "digit2"))
		shader->ammoDisplay.type = AMMODISPLAY_DIGIT2;
	else if (!Q_stricmp(tok, "digit3"))
		shader->ammoDisplay.type = AMMODISPLAY_DIGIT3;
	else if (!Q_stricmp(tok, "warning"))
		shader->ammoDisplay.type = AMMODISPLAY_WARNING;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'ammoDisplay' parameter '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'ammoDisplay' in shader '%s'\n", shader->name);
		return false;
	}

	shader->ammoDisplay.lowAmmo = atoi(tok);

	for (i = 0; i < 3; i++){
		if (i == 0){
			if (shader->ammoDisplay.type == AMMODISPLAY_WARNING)
				continue;
		}

		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'ammoDisplay' in shader '%s'\n", shader->name);
			return false;
		}

		Q_strncpyz(shader->ammoDisplay.remapShaders[i], tok, sizeof(shader->ammoDisplay.remapShaders[i]));
	}

	shader->flags |= SHADER_AMMODISPLAY;

	return true;
}

/*
 =================
 R_ParseGeneralTessSize
 =================
*/
static qboolean R_ParseGeneralTessSize (shader_t *shader, char **script){

	char	*tok;
	int		i = 8;

	if (shader->shaderType != SHADER_BSP){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'tessSize' not allowed in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'tessSize' in shader '%s'\n", shader->name);
		return false;
	}

	shader->tessSize = atoi(tok);

	if (shader->tessSize < 8 || shader->tessSize > 256){
		Com_Printf(S_COLOR_YELLOW "WARNING: out of range size value of %i for 'tessSize' in shader '%s', defaulting to 64\n", shader->tessSize, shader->name);
		shader->tessSize = 64;
	}
	else {
		while (i <= shader->tessSize)
			i <<= 1;

		shader->tessSize = i >> 1;
	}

	shader->flags |= SHADER_TESSSIZE;

	return true;
}

/*
 =================
 R_ParseGeneralSkyParms
 =================
*/
static qboolean R_ParseGeneralSkyParms (shader_t *shader, char **script){

	char	name[MAX_QPATH];
	char	*tok;
	int		i;

	if (shader->shaderType != SHADER_SKY){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'skyParms' not allowed in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skyParms' in shader '%s'\n", shader->name);
		return false;
	}

	if (Q_stricmp(tok, "-")){
		for (i = 0; i < 6; i++){
			Q_snprintfz(name, sizeof(name), "%s%s.tga", tok, r_skyBoxSuffix[i]);
			shader->skyParms.farBox[i] = R_FindTexture(name, TF_CLAMP, 0);
			if (!shader->skyParms.farBox[i]){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' in shader '%s'\n", name, shader->name);
				return false;
			}
		}
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skyParms' in shader '%s'\n", shader->name);
		return false;
	}

	if (Q_stricmp(tok, "-")){
		shader->skyParms.cloudHeight = atof(tok);

		if (shader->skyParms.cloudHeight < 8.0 || shader->skyParms.cloudHeight > 1024.0){
			Com_Printf(S_COLOR_YELLOW "WARNING: out of range cloudHeight value of %f for 'skyParms' in shader '%s', defaulting to 128\n", shader->skyParms.cloudHeight, shader->name);
			shader->skyParms.cloudHeight = 128.0;
		}
	}
	else
		shader->skyParms.cloudHeight = 128.0;

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skyParms' in shader '%s'\n", shader->name);
		return false;
	}

	if (Q_stricmp(tok, "-")){
		for (i = 0; i < 6; i++){
			Q_snprintfz(name, sizeof(name), "%s%s.tga", tok, r_skyBoxSuffix[i]);
			shader->skyParms.nearBox[i] = R_FindTexture(name, TF_CLAMP, 0);
			if (!shader->skyParms.nearBox[i]){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' in shader '%s'\n", name, shader->name);
				return false;
			}
		}
	}

	shader->flags |= SHADER_SKYPARMS;

	return true;
}

/*
 =================
 R_ParseGeneralDeformVertexes
 =================
*/
static qboolean R_ParseGeneralDeformVertexes (shader_t *shader, char **script){

	deformVertexes_t	*deformVertexes;
	char				*tok;
	int					i;

	if (shader->deformVertexesNum == SHADER_MAX_DEFORMVERTEXES){
		Com_Printf(S_COLOR_YELLOW "WARNING: SHADER_MAX_DEFORMVERTEXES hit in shader '%s'\n", shader->name);
		return false;
	}
	deformVertexes = &shader->deformVertexes[shader->deformVertexesNum++];

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'deformVertexes' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "wave")){
		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'deformVertexes wave' in shader '%s'\n", shader->name);
			return false;
		}

		deformVertexes->params[0] = atof(tok);
		if (deformVertexes->params[0] == 0.0){
			Com_Printf(S_COLOR_YELLOW "WARNING: illegal div value of 0 for 'deformVertexes wave' in shader '%s', defaulting to 100\n", shader->name);
			deformVertexes->params[0] = 100.0;
		}
		deformVertexes->params[0] = 1.0 / deformVertexes->params[0];

		if (!R_ParseWaveFunc(shader, &deformVertexes->func, script)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parameters for 'deformVertexes wave' in shader '%s'\n", shader->name);
			return false;
		}

		deformVertexes->type = DEFORMVERTEXES_WAVE;
	}
	else if (!Q_stricmp(tok, "move")){
		for (i = 0; i < 3; i++){
			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'deformVertexes move' in shader '%s'\n", shader->name);
				return false;
			}

			deformVertexes->params[i] = atof(tok);
		}

		if (!R_ParseWaveFunc(shader, &deformVertexes->func, script)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parameters for 'deformVertexes move' in shader '%s'\n", shader->name);
			return false;
		}

		deformVertexes->type = DEFORMVERTEXES_MOVE;
	}
	else if (!Q_stricmp(tok, "normal")){
		for (i = 0; i < 2; i++){
			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'deformVertexes normal' in shader '%s'\n", shader->name);
				return false;
			}

			deformVertexes->params[i] = atof(tok);
		}

		deformVertexes->type = DEFORMVERTEXES_NORMAL;
	}
	else if (!Q_stricmp(tok, "autoSprite"))
		deformVertexes->type = DEFORMVERTEXES_AUTOSPRITE;
	else if (!Q_stricmp(tok, "autoSprite2"))
		deformVertexes->type = DEFORMVERTEXES_AUTOSPRITE2;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'deformVertexes' parameter '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	shader->flags |= SHADER_DEFORMVERTEXES;

	return true;
}

/*
 =================
 R_ParseStageRequires
 =================
*/
static qboolean R_ParseStageRequires (shader_t *shader, shaderStage_t *stage, char **script){

	Com_Printf(S_COLOR_YELLOW "WARNING: 'requires' is not the first command in the stage in shader '%s'\n", shader->name);

	return false;
}

/*
 =================
 R_ParseStageNoMipmaps
 =================
*/
static qboolean R_ParseStageNoMipmaps (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];

	bundle->flags |= (STAGEBUNDLE_NOMIPMAPS | STAGEBUNDLE_NOPICMIP);

	return true;
}

/*
 =================
 R_ParseStageNoPicmip
 =================
*/
static qboolean R_ParseStageNoPicmip (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];

	bundle->flags |= STAGEBUNDLE_NOPICMIP;

	return true;
}

/*
 =================
 R_ParseStageNoCompress
 =================
*/
static qboolean R_ParseStageNoCompress (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];

	bundle->flags |= STAGEBUNDLE_NOCOMPRESS;

	return true;
}

/*
 =================
 R_ParseStageClampTexCoords
 =================
*/
static qboolean R_ParseStageClampTexCoords (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];

	bundle->flags |= STAGEBUNDLE_CLAMPTEXCOORDS;

	return true;
}

/*
 =================
 R_ParseStageAnimFrequency
 =================
*/
static qboolean R_ParseStageAnimFrequency (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];
	char			*tok;

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'animFrequency' in shader '%s\n", shader->name);
		return false;
	}
	bundle->animFrequency = atof(tok);

	bundle->flags |= STAGEBUNDLE_ANIMFREQUENCY;

	return true;
}

/*
 =================
 R_ParseStageMap
 =================
*/
static qboolean R_ParseStageMap (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];
	unsigned		flags = 0;
	char			*tok;

	if (bundle->numTextures){
		if (bundle->numTextures == SHADER_MAX_TEXTURES){
			Com_Printf(S_COLOR_YELLOW "WARNING: SHADER_MAX_TEXTURES hit in shader '%s'\n", shader->name);
			return false;
		}

		if (!(bundle->flags & STAGEBUNDLE_MAP)){
			Com_Printf(S_COLOR_YELLOW "WARNING: animation with mixed texture types in shader '%s'\n", shader->name);
			return false;
		}

		if (!(bundle->flags & STAGEBUNDLE_ANIMFREQUENCY)){
			Com_Printf(S_COLOR_YELLOW "WARNING: multiple 'map' specifications without preceding 'animFrequency' in shader '%s'\n", shader->name);
			return false;
		}
	}

	if (bundle->cinematicHandle){
		Com_Printf(S_COLOR_YELLOW "WARNING: animation with mixed texture types in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'map' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "$lightmap")){
		if (shader->shaderType != SHADER_BSP){
			Com_Printf(S_COLOR_YELLOW "WARNING: 'map $lightmap' not allowed in shader '%s'\n", shader->name);
			return false;
		}

		if (bundle->flags & STAGEBUNDLE_ANIMFREQUENCY){
			Com_Printf(S_COLOR_YELLOW "WARNING: 'map $lightmap' not allowed with 'animFrequency' in shader '%s'\n", shader->name);
			return false;
		}

		bundle->texType = TEX_LIGHTMAP;

		bundle->flags |= STAGEBUNDLE_MAP;

		shader->flags |= SHADER_HASLIGHTMAP;

		return true;
	}

	if (!Q_stricmp(tok, "$whiteImage"))
		bundle->textures[bundle->numTextures++] = r_whiteTexture;
	else if (!Q_stricmp(tok, "$blackImage"))
		bundle->textures[bundle->numTextures++] = r_blackTexture;
	else {
		if (!(shader->flags & SHADER_NOMIPMAPS) && !(bundle->flags & STAGEBUNDLE_NOMIPMAPS))
			flags |= TF_MIPMAPS;
		if (!(shader->flags & SHADER_NOPICMIP) && !(bundle->flags & STAGEBUNDLE_NOPICMIP))
			flags |= TF_PICMIP;
		if (!(shader->flags & SHADER_NOCOMPRESS) && !(bundle->flags & STAGEBUNDLE_NOCOMPRESS))
			flags |= TF_COMPRESS;

		if (bundle->flags & STAGEBUNDLE_CLAMPTEXCOORDS)
			flags |= TF_CLAMP;

		bundle->textures[bundle->numTextures] = R_FindTexture(tok, flags, 0);
		if (!bundle->textures[bundle->numTextures]){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' in shader '%s'\n", tok, shader->name);
			return false;
		}
		bundle->numTextures++;
	}

	bundle->texType = TEX_GENERIC;

	bundle->flags |= STAGEBUNDLE_MAP;

	return true;
}

/*
 =================
 R_ParseStageBumpMap
 =================
*/
static qboolean R_ParseStageBumpMap (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];
	unsigned		flags = TF_NORMALMAP;
	char			heightMap[MAX_QPATH];
	float			bumpScale;
	char			*tok;

	if (bundle->numTextures){
		if (bundle->numTextures == SHADER_MAX_TEXTURES){
			Com_Printf(S_COLOR_YELLOW "WARNING: SHADER_MAX_TEXTURES hit in shader '%s'\n", shader->name);
			return false;
		}

		if (!(bundle->flags & STAGEBUNDLE_BUMPMAP)){
			Com_Printf(S_COLOR_YELLOW "WARNING: animation with mixed texture types in shader '%s'\n", shader->name);
			return false;
		}

		if (!(bundle->flags & STAGEBUNDLE_ANIMFREQUENCY)){
			Com_Printf(S_COLOR_YELLOW "WARNING: multiple 'bumpMap' specifications without preceding 'animFrequency' in shader '%s'\n", shader->name);
			return false;
		}
	}

	if (bundle->cinematicHandle){
		Com_Printf(S_COLOR_YELLOW "WARNING: animation with mixed texture types in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'bumpMap' in shader '%s'\n", shader->name);
		return false;
	}

	if (!(shader->flags & SHADER_NOMIPMAPS) && !(bundle->flags & STAGEBUNDLE_NOMIPMAPS))
		flags |= TF_MIPMAPS;
	if (!(shader->flags & SHADER_NOPICMIP) && !(bundle->flags & STAGEBUNDLE_NOPICMIP))
		flags |= TF_PICMIP;
	if (!(shader->flags & SHADER_NOCOMPRESS) && !(bundle->flags & STAGEBUNDLE_NOCOMPRESS))
		flags |= TF_COMPRESS;

	if (bundle->flags & STAGEBUNDLE_CLAMPTEXCOORDS)
		flags |= TF_CLAMP;

	if (!Q_stricmp(tok, "heightToNormal")){
		if (!R_ParseHeightToNormal(shader, heightMap, sizeof(heightMap), &bumpScale, script))
			return false;

		bundle->textures[bundle->numTextures] = R_FindTexture(heightMap, flags | TF_HEIGHTMAP, bumpScale);
		if (!bundle->textures[bundle->numTextures]){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' in shader '%s'\n", heightMap, shader->name);
			return false;
		}
		bundle->numTextures++;
	}
	else {
		bundle->textures[bundle->numTextures] = R_FindTexture(tok, flags, 0);
		if (!bundle->textures[bundle->numTextures]){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' in shader '%s'\n", tok, shader->name);
			return false;
		}
		bundle->numTextures++;
	}

	bundle->texType = TEX_GENERIC;

	bundle->flags |= STAGEBUNDLE_BUMPMAP;

	return true;
}

/*
 =================
 R_ParseStageCubeMap
 =================
*/
static qboolean R_ParseStageCubeMap (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];
	unsigned		flags = TF_CLAMP | TF_CUBEMAP;
	char			*tok;

	if (!glConfig.textureCubeMap){
		Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'cubeMap' without 'requires GL_ARB_texture_cube_map'\n", shader->name);
		return false;
	}

	if (bundle->numTextures){
		if (bundle->numTextures == SHADER_MAX_TEXTURES){
			Com_Printf(S_COLOR_YELLOW "WARNING: SHADER_MAX_TEXTURES hit in shader '%s'\n", shader->name);
			return false;
		}

		if (!(bundle->flags & STAGEBUNDLE_CUBEMAP)){
			Com_Printf(S_COLOR_YELLOW "WARNING: animation with mixed texture types in shader '%s'\n", shader->name);
			return false;
		}

		if (!(bundle->flags & STAGEBUNDLE_ANIMFREQUENCY)){
			Com_Printf(S_COLOR_YELLOW "WARNING: multiple 'cubeMap' specifications without preceding 'animFrequency' in shader '%s'\n", shader->name);
			return false;
		}
	}

	if (bundle->cinematicHandle){
		Com_Printf(S_COLOR_YELLOW "WARNING: animation with mixed texture types in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'cubeMap' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "$normalize")){
		if (bundle->flags & STAGEBUNDLE_ANIMFREQUENCY){
			Com_Printf(S_COLOR_YELLOW "WARNING: 'cubeMap $normalize' not allowed with 'animFrequency' in shader '%s'\n", shader->name);
			return false;
		}

		bundle->textures[bundle->numTextures++] = r_normalizeTexture;
	}
	else {
		if (!(shader->flags & SHADER_NOMIPMAPS) && !(bundle->flags & STAGEBUNDLE_NOMIPMAPS))
			flags |= TF_MIPMAPS;
		if (!(shader->flags & SHADER_NOPICMIP) && !(bundle->flags & STAGEBUNDLE_NOPICMIP))
			flags |= TF_PICMIP;
		if (!(shader->flags & SHADER_NOCOMPRESS) && !(bundle->flags & STAGEBUNDLE_NOCOMPRESS))
			flags |= TF_COMPRESS;

		if (bundle->flags & STAGEBUNDLE_CLAMPTEXCOORDS)
			flags |= TF_CLAMP;

		bundle->textures[bundle->numTextures] = R_FindCubeMapTexture(tok, flags, 0);
		if (!bundle->textures[bundle->numTextures]){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' in shader '%s'\n", tok, shader->name);
			return false;
		}
		bundle->numTextures++;
	}

	bundle->texType = TEX_GENERIC;

	bundle->flags |= STAGEBUNDLE_CUBEMAP;

	return true;
}

/*
 =================
 R_ParseStageVideoMap
 =================
*/
static qboolean R_ParseStageVideoMap (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];
	char			*tok;

	if (bundle->numTextures){
		Com_Printf(S_COLOR_YELLOW "WARNING: animation with mixed texture types in shader '%s\n", shader->name);
		return false;
	}

	if (bundle->cinematicHandle){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple 'videoMap' specifications in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'videoMap' in shader '%s'\n", shader->name);
		return false;
	}

	bundle->cinematicHandle = CIN_PlayCinematic(tok, 0, 0, 0, 0, CIN_LOOPED|CIN_SILENT|CIN_SHADER);
	if (!bundle->cinematicHandle){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find video '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	bundle->texType = TEX_CINEMATIC;

	bundle->flags |= STAGEBUNDLE_VIDEOMAP;

	return true;
}

/*
 =================
 R_ParseStageTexEnvCombine
 =================
*/
static qboolean R_ParseStageTexEnvCombine (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];
	int				numArgs;
	char			*tok;
	int				i;

	if (!glConfig.textureEnvCombine){
		Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'texEnvCombine' without 'requires GL_ARB_texture_env_combine'\n", shader->name);
		return false;
	}

	if (stage->flags & SHADERSTAGE_NEXTBUNDLE){
		if (!(bundle->flags & STAGEBUNDLE_TEXENVCOMBINE)){
			Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'texEnvCombine' in a bundle without 'nextBundle combine'\n", shader->name);
			return false;
		}
	}

	bundle->texEnv = GL_COMBINE_ARB;

	bundle->texEnvCombine.rgbCombine = GL_MODULATE;
	bundle->texEnvCombine.rgbSource[0] = GL_TEXTURE;
	bundle->texEnvCombine.rgbSource[1] = GL_PREVIOUS_ARB;
	bundle->texEnvCombine.rgbSource[2] = GL_CONSTANT_ARB;
	bundle->texEnvCombine.rgbOperand[0] = GL_SRC_COLOR;
	bundle->texEnvCombine.rgbOperand[1] = GL_SRC_COLOR;
	bundle->texEnvCombine.rgbOperand[2] = GL_SRC_ALPHA;
	bundle->texEnvCombine.rgbScale = 1;
	bundle->texEnvCombine.alphaCombine = GL_MODULATE;
	bundle->texEnvCombine.alphaSource[0] = GL_TEXTURE;
	bundle->texEnvCombine.alphaSource[1] = GL_PREVIOUS_ARB;
	bundle->texEnvCombine.alphaSource[2] = GL_CONSTANT_ARB;
	bundle->texEnvCombine.alphaOperand[0] = GL_SRC_ALPHA;
	bundle->texEnvCombine.alphaOperand[1] = GL_SRC_ALPHA;
	bundle->texEnvCombine.alphaOperand[2] = GL_SRC_ALPHA;
	bundle->texEnvCombine.alphaScale = 1;
	bundle->texEnvCombine.constColor[0] = 1.0;
	bundle->texEnvCombine.constColor[1] = 1.0;
	bundle->texEnvCombine.constColor[2] = 1.0;
	bundle->texEnvCombine.constColor[3] = 1.0;

	tok = Com_ParseExt(script, true);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'texEnvCombine' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "{")){
		while (1){
			tok = Com_ParseExt(script, true);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in 'texEnvCombine' in shader '%s'\n", shader->name);
				return false;
			}

			if (!Q_stricmp(tok, "}"))
				break;

			if (!Q_stricmp(tok, "rgb")){
				tok = Com_ParseExt(script, false);
				if (Q_stricmp(tok, "=")){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected '=', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				tok = Com_ParseExt(script, false);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing 'rgb' equation name for 'texEnvCombine' in shader '%s'\n", shader->name);
					return false;
				}

				if (!Q_stricmp(tok, "REPLACE")){
					bundle->texEnvCombine.rgbCombine = GL_REPLACE;
					numArgs = 1;
				}
				else if (!Q_stricmp(tok, "MODULATE")){
					bundle->texEnvCombine.rgbCombine = GL_MODULATE;
					numArgs = 2;
				}
				else if (!Q_stricmp(tok, "ADD")){
					bundle->texEnvCombine.rgbCombine = GL_ADD;
					numArgs = 2;
				}
				else if (!Q_stricmp(tok, "ADD_SIGNED")){
					bundle->texEnvCombine.rgbCombine = GL_ADD_SIGNED_ARB;
					numArgs = 2;
				}
				else if (!Q_stricmp(tok, "INTERPOLATE")){
					bundle->texEnvCombine.rgbCombine = GL_INTERPOLATE_ARB;
					numArgs = 3;
				}
				else if (!Q_stricmp(tok, "SUBTRACT")){
					bundle->texEnvCombine.rgbCombine = GL_SUBTRACT_ARB;
					numArgs = 2;
				}
				else if (!Q_stricmp(tok, "DOT3_RGB")){
					if (!glConfig.textureEnvDot3){
						Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'DOT3_RGB' in 'texEnvCombine' without 'requires GL_ARB_texture_env_dot3'\n", shader->name);
						return false;
					}

					bundle->texEnvCombine.rgbCombine = GL_DOT3_RGB_ARB;
					numArgs = 2;
				}
				else if (!Q_stricmp(tok, "DOT3_RGBA")){
					if (!glConfig.textureEnvDot3){
						Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'DOT3_RGBA' in 'texEnvCombine' without 'requires GL_ARB_texture_env_dot3'\n", shader->name);
						return false;
					}

					bundle->texEnvCombine.rgbCombine = GL_DOT3_RGBA_ARB;
					numArgs = 2;
				}
				else {
					Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'rgb' equation name '%s' for 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				tok = Com_ParseExt(script, false);
				if (Q_stricmp(tok, "(")){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				for (i = 0; i < numArgs; i++){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'rgb' equation arguments for 'texEnvCombine' in shader '%s'\n", shader->name);
						return false;
					}

					if (!Q_stricmp(tok, "Ct")){
						bundle->texEnvCombine.rgbSource[i] = GL_TEXTURE;
						bundle->texEnvCombine.rgbOperand[i] = GL_SRC_COLOR;
					}
					else if (!Q_stricmp(tok, "1-Ct")){
						bundle->texEnvCombine.rgbSource[i] = GL_TEXTURE;
						bundle->texEnvCombine.rgbOperand[i] = GL_ONE_MINUS_SRC_COLOR;
					}
					else if (!Q_stricmp(tok, "At")){
						bundle->texEnvCombine.rgbSource[i] = GL_TEXTURE;
						bundle->texEnvCombine.rgbOperand[i] = GL_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "1-At")){
						bundle->texEnvCombine.rgbSource[i] = GL_TEXTURE;
						bundle->texEnvCombine.rgbOperand[i] = GL_ONE_MINUS_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "Cc")){
						bundle->texEnvCombine.rgbSource[i] = GL_CONSTANT_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_SRC_COLOR;
					}
					else if (!Q_stricmp(tok, "1-Cc")){
						bundle->texEnvCombine.rgbSource[i] = GL_CONSTANT_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_ONE_MINUS_SRC_COLOR;
					}
					else if (!Q_stricmp(tok, "Ac")){
						bundle->texEnvCombine.rgbSource[i] = GL_CONSTANT_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "1-Ac")){
						bundle->texEnvCombine.rgbSource[i] = GL_CONSTANT_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_ONE_MINUS_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "Cf")){
						bundle->texEnvCombine.rgbSource[i] = GL_PRIMARY_COLOR_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_SRC_COLOR;
					}
					else if (!Q_stricmp(tok, "1-Cf")){
						bundle->texEnvCombine.rgbSource[i] = GL_PRIMARY_COLOR_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_ONE_MINUS_SRC_COLOR;
					}
					else if (!Q_stricmp(tok, "Af")){
						bundle->texEnvCombine.rgbSource[i] = GL_PRIMARY_COLOR_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "1-Af")){
						bundle->texEnvCombine.rgbSource[i] = GL_PRIMARY_COLOR_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_ONE_MINUS_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "Cp")){
						bundle->texEnvCombine.rgbSource[i] = GL_PREVIOUS_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_SRC_COLOR;
					}
					else if (!Q_stricmp(tok, "1-Cp")){
						bundle->texEnvCombine.rgbSource[i] = GL_PREVIOUS_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_ONE_MINUS_SRC_COLOR;
					}
					else if (!Q_stricmp(tok, "Ap")){
						bundle->texEnvCombine.rgbSource[i] = GL_PREVIOUS_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "1-Ap")){
						bundle->texEnvCombine.rgbSource[i] = GL_PREVIOUS_ARB;
						bundle->texEnvCombine.rgbOperand[i] = GL_ONE_MINUS_SRC_ALPHA;
					}
					else {
						Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'rgb' equation argument '%s' for 'texEnvCombine' in shader '%s'\n", tok, shader->name);
						return false;
					}

					if (i < numArgs - 1){
						tok = Com_ParseExt(script, false);
						if (Q_stricmp(tok, ",")){
							Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
							return false;
						}
					}
				}

				tok = Com_ParseExt(script, false);
				if (Q_stricmp(tok, ")")){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				tok = Com_ParseExt(script, false);
				if (tok[0]){
					if (Q_stricmp(tok, "*")){
						Com_Printf(S_COLOR_YELLOW "WARNING: expected '*', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
						return false;
					}

					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing scale value for 'texEnvCombine' equation in shader '%s'\n", shader->name);
						return false;
					}
					bundle->texEnvCombine.rgbScale = atoi(tok);

					if (bundle->texEnvCombine.rgbScale != 1 && bundle->texEnvCombine.rgbScale != 2 && bundle->texEnvCombine.rgbScale != 4){
						Com_Printf(S_COLOR_YELLOW "WARNING: invalid scale value of %i for 'texEnvCombine' equation in shader '%s'\n", bundle->texEnvCombine.rgbScale, shader->name);
						return false;
					}
				}
			}
			else if (!Q_stricmp(tok, "alpha")){
				tok = Com_ParseExt(script, false);
				if (Q_stricmp(tok, "=")){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected '=', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				tok = Com_ParseExt(script, false);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing 'alpha' equation name for 'texEnvCombine' in shader '%s'\n", shader->name);
					return false;
				}

				if (!Q_stricmp(tok, "REPLACE")){
					bundle->texEnvCombine.alphaCombine = GL_REPLACE;
					numArgs = 1;
				}
				else if (!Q_stricmp(tok, "MODULATE")){
					bundle->texEnvCombine.alphaCombine = GL_MODULATE;
					numArgs = 2;
				}
				else if (!Q_stricmp(tok, "ADD")){
					bundle->texEnvCombine.alphaCombine = GL_ADD;
					numArgs = 2;
				}
				else if (!Q_stricmp(tok, "ADD_SIGNED")){
					bundle->texEnvCombine.alphaCombine = GL_ADD_SIGNED_ARB;
					numArgs = 2;
				}
				else if (!Q_stricmp(tok, "INTERPOLATE")){
					bundle->texEnvCombine.alphaCombine = GL_INTERPOLATE_ARB;
					numArgs = 3;
				}
				else if (!Q_stricmp(tok, "SUBTRACT")){
					bundle->texEnvCombine.alphaCombine = GL_SUBTRACT_ARB;
					numArgs = 2;
				}
				else if (!Q_stricmp(tok, "DOT3_RGB")){
					if (!glConfig.textureEnvDot3){
						Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'DOT3_RGB' in 'texEnvCombine' without 'requires GL_ARB_texture_env_dot3'\n", shader->name);
						return false;
					}

					Com_Printf(S_COLOR_YELLOW "WARNING: 'DOT3_RGB' is not a valid 'alpha' equation for 'texEnvCombine' in shader '%s'\n", shader->name);
					return false;
				}
				else if (!Q_stricmp(tok, "DOT3_RGBA")){
					if (!glConfig.textureEnvDot3){
						Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'DOT3_RGBA' in 'texEnvCombine' without 'requires GL_ARB_texture_env_dot3'\n", shader->name);
						return false;
					}

					Com_Printf(S_COLOR_YELLOW "WARNING: 'DOT3_RGBA' is not a valid 'alpha' equation for 'texEnvCombine' in shader '%s'\n", shader->name);
					return false;
				}
				else {
					Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'alpha' equation name '%s' for 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				tok = Com_ParseExt(script, false);
				if (Q_stricmp(tok, "(")){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				for (i = 0; i < numArgs; i++){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'alpha' equation arguments for 'texEnvCombine' in shader '%s'\n", shader->name);
						return false;
					}

					if (!Q_stricmp(tok, "At")){
						bundle->texEnvCombine.alphaSource[i] = GL_TEXTURE;
						bundle->texEnvCombine.alphaOperand[i] = GL_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "1-At")){
						bundle->texEnvCombine.alphaSource[i] = GL_TEXTURE;
						bundle->texEnvCombine.alphaOperand[i] = GL_ONE_MINUS_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "Ac")){
						bundle->texEnvCombine.alphaSource[i] = GL_CONSTANT_ARB;
						bundle->texEnvCombine.alphaOperand[i] = GL_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "1-Ac")){
						bundle->texEnvCombine.alphaSource[i] = GL_CONSTANT_ARB;
						bundle->texEnvCombine.alphaOperand[i] = GL_ONE_MINUS_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "Af")){
						bundle->texEnvCombine.alphaSource[i] = GL_PRIMARY_COLOR_ARB;
						bundle->texEnvCombine.alphaOperand[i] = GL_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "1-Af")){
						bundle->texEnvCombine.alphaSource[i] = GL_PRIMARY_COLOR_ARB;
						bundle->texEnvCombine.alphaOperand[i] = GL_ONE_MINUS_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "Ap")){
						bundle->texEnvCombine.alphaSource[i] = GL_PREVIOUS_ARB;
						bundle->texEnvCombine.alphaOperand[i] = GL_SRC_ALPHA;
					}
					else if (!Q_stricmp(tok, "1-Ap")){
						bundle->texEnvCombine.alphaSource[i] = GL_PREVIOUS_ARB;
						bundle->texEnvCombine.alphaOperand[i] = GL_ONE_MINUS_SRC_ALPHA;
					}
					else {
						Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'alpha' equation argument '%s' for 'texEnvCombine' in shader '%s'\n", tok, shader->name);
						return false;
					}

					if (i < numArgs - 1){
						tok = Com_ParseExt(script, false);
						if (Q_stricmp(tok, ",")){
							Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
							return false;
						}
					}
				}

				tok = Com_ParseExt(script, false);
				if (Q_stricmp(tok, ")")){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				tok = Com_ParseExt(script, false);
				if (tok[0]){
					if (Q_stricmp(tok, "*")){
						Com_Printf(S_COLOR_YELLOW "WARNING: expected '*', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
						return false;
					}

					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing scale value for 'texEnvCombine' equation in shader '%s'\n", shader->name);
						return false;
					}
					bundle->texEnvCombine.alphaScale = atoi(tok);

					if (bundle->texEnvCombine.alphaScale != 1 && bundle->texEnvCombine.alphaScale != 2 && bundle->texEnvCombine.alphaScale != 4){
						Com_Printf(S_COLOR_YELLOW "WARNING: invalid scale value of %i for 'texEnvCombine' equation in shader '%s'\n", bundle->texEnvCombine.alphaScale, shader->name);
						bundle->texEnvCombine.alphaScale = 1;
					}
				}
			}
			else if (!Q_stricmp(tok, "const")){
				tok = Com_ParseExt(script, false);
				if (Q_stricmp(tok, "=")){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected '=', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				tok = Com_ParseExt(script, false);
				if (Q_stricmp(tok, "(")){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				for (i = 0; i < 4; i++){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing 'const' color value for 'texEnvCombine' in shader '%s'\n", shader->name);
						return false;
					}

					bundle->texEnvCombine.constColor[i] = Clamp(atof(tok), 0.0, 1.0);
				}

				tok = Com_ParseExt(script, false);
				if (Q_stricmp(tok, ")")){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
					return false;
				}

				tok = Com_ParseExt(script, false);
				if (tok[0]){
					if (Q_stricmp(tok, "*")){
						Com_Printf(S_COLOR_YELLOW "WARNING: expected '*', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
						return false;
					}

					tok = Com_ParseExt(script, false);
					if (Q_stricmp(tok, "identityLighting")){
						Com_Printf(S_COLOR_YELLOW "WARNING: 'const' color for 'texEnvCombine' can only be scaled by 'identityLighting' in shader '%s'\n", shader->name);
						return false;
					}

					if (glConfig.deviceSupportsGamma){
						for (i = 0; i < 3; i++)
							bundle->texEnvCombine.constColor[i] *= (1.0 / (float)(1 << r_overBrightBits->integer));
					}
				}
			}
			else {
				Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'texEnvCombine' parameter '%s' in shader '%s'\n", tok, shader->name);
				return false;
			}
		}
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in 'texEnvCombine' in shader '%s'\n", tok, shader->name);
		return false;
	}

	bundle->flags |= STAGEBUNDLE_TEXENVCOMBINE;

	return true;
}

/*
 =================
 R_ParseStageTcGen
 =================
*/
static qboolean R_ParseStageTcGen (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];
	char			*tok;
	int				i, j;

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'tcGen' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "base") || !Q_stricmp(tok, "texture"))
		bundle->tcGen.type = TCGEN_BASE;
	else if (!Q_stricmp(tok, "lightmap"))
		bundle->tcGen.type = TCGEN_LIGHTMAP;
	else if (!Q_stricmp(tok, "environment"))
		bundle->tcGen.type = TCGEN_ENVIRONMENT;
	else if (!Q_stricmp(tok, "vector")){
		for (i = 0; i < 2; i++){
			tok = Com_ParseExt(script, false);
			if (Q_stricmp(tok, "(")){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing '(' for 'tcGen vector' in shader '%s'\n", shader->name);
				return false;
			}

			for (j = 0; j < 3; j++){
				tok = Com_ParseExt(script, false);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'tcGen vector' in shader '%s'\n", shader->name);
					return false;
				}

				bundle->tcGen.params[i*3+j] = atof(tok);
			}

			tok = Com_ParseExt(script, false);
			if (Q_stricmp(tok, ")")){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing ')' for 'tcGen vector' in shader '%s'\n", shader->name);
				return false;
			}
		}

		bundle->tcGen.type = TCGEN_VECTOR;
	}
	else if (!Q_stricmp(tok, "warp"))
		bundle->tcGen.type = TCGEN_WARP;
	else if (!Q_stricmp(tok, "lightVector"))
		bundle->tcGen.type = TCGEN_LIGHTVECTOR;
	else if (!Q_stricmp(tok, "halfAngle"))
		bundle->tcGen.type = TCGEN_HALFANGLE;
	else if (!Q_stricmp(tok, "reflection")){
		if (!glConfig.textureCubeMap){
			Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'tcGen reflection' without 'requires GL_ARB_texture_cube_map'\n", shader->name);
			return false;
		}

		bundle->tcGen.type = TCGEN_REFLECTION;
	}
	else if (!Q_stricmp(tok, "normal")){
		if (!glConfig.textureCubeMap){
			Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'tcGen normal' without 'requires GL_ARB_texture_cube_map'\n", shader->name);
			return false;
		}

		bundle->tcGen.type = TCGEN_NORMAL;
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'tcGen' parameter '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	bundle->flags |= STAGEBUNDLE_TCGEN;

	return true;
}

/*
 =================
 R_ParseStageTcMod
 =================
*/
static qboolean R_ParseStageTcMod (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle = stage->bundles[stage->numBundles - 1];
	tcMod_t			*tcMod;
	char			*tok;
	int				i;

	if (bundle->tcModNum == SHADER_MAX_TCMOD){
		Com_Printf(S_COLOR_YELLOW "WARNING: SHADER_MAX_TCMOD hit in shader '%s'\n", shader->name);
		return false;
	}
	tcMod = &bundle->tcMod[bundle->tcModNum++];

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'tcMod' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "translate")){
		for (i = 0; i < 2; i++){
			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'tcMod translate' in shader '%s'\n", shader->name);
				return false;
			}

			tcMod->params[i] = atof(tok);
		}

		tcMod->type = TCMOD_TRANSLATE;
	}
	else if (!Q_stricmp(tok, "scale")){
		for (i = 0; i < 2; i++){
			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'tcMod scale' in shader '%s'\n", shader->name);
				return false;
			}

			tcMod->params[i] = atof(tok);
		}

		tcMod->type = TCMOD_SCALE;
	}
	else if (!Q_stricmp(tok, "scroll")){
		for (i = 0; i < 2; i++){
			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'tcMod scroll' in shader '%s'\n", shader->name);
				return false;
			}

			tcMod->params[i] = atof(tok);
		}

		tcMod->type = TCMOD_SCROLL;
	}
	else if (!Q_stricmp(tok, "rotate")){
		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'tcMod rotate' in shader '%s'\n", shader->name);
			return false;
		}

		tcMod->params[0] = atof(tok);

		tcMod->type = TCMOD_ROTATE;
	}
	else if (!Q_stricmp(tok, "stretch")){
		if (!R_ParseWaveFunc(shader, &tcMod->func, script)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parameters for 'tcMod stretch' in shader '%s'\n", shader->name);
			return false;
		}

		tcMod->type = TCMOD_STRETCH;
	}
	else if (!Q_stricmp(tok, "turb")){
		tcMod->func.type = WAVEFORM_SIN;

		for (i = 0; i < 4; i++){
			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'tcMod turb' in shader '%s'\n", shader->name);
				return false;
			}

			tcMod->func.params[i] = atof(tok);
		}

		tcMod->type = TCMOD_TURB;
	}
	else if (!Q_stricmp(tok, "transform")){
		for (i = 0; i < 6; i++){
			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'tcMod transform' in shader '%s'\n", shader->name);
				return false;
			}

			tcMod->params[i] = atof(tok);
		}

		tcMod->type = TCMOD_TRANSFORM;
	}
	else {	
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'tcMod' parameter '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	bundle->flags |= STAGEBUNDLE_TCMOD;

	return true;
}

/*
 =================
 R_ParseStageNextBundle
 =================
*/
static qboolean R_ParseStageNextBundle (shader_t *shader, shaderStage_t *stage, char **script){

	stageBundle_t	*bundle;
	char			*tok;

	if (!glConfig.multitexture){
		Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'nextBundle' without 'requires GL_ARB_multitexture'\n", shader->name);
		return false;
	}

	if (stage->flags & SHADERSTAGE_FRAGMENTPROGRAM){
		if (stage->numBundles == glConfig.maxTextureCoords){
			Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' has %i or more bundles without suitable 'requires GL_MAX_TEXTURE_COORDS_ARB'\n", shader->name, stage->numBundles + 1);
			return false;
		}

		if (stage->numBundles == glConfig.maxTextureImageUnits){
			Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' has %i or more bundles without suitable 'requires GL_MAX_TEXTURE_IMAGE_UNITS_ARB'\n", shader->name, stage->numBundles + 1);
			return false;
		}
	}
	else {
		if (stage->numBundles == glConfig.maxTextureUnits){
			Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' has %i or more bundles without suitable 'requires GL_MAX_TEXTURE_UNITS_ARB'\n", shader->name, stage->numBundles + 1);
			return false;
		}
	}

	if (stage->numBundles == MAX_TEXTURE_UNITS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXTURE_UNITS hit in shader '%s'\n", shader->name);
		return false;
	}
	bundle = stage->bundles[stage->numBundles++];

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		if (!(stage->bundles[0]->flags & STAGEBUNDLE_TEXENVCOMBINE)){
			if ((stage->flags & SHADERSTAGE_BLENDFUNC) && ((stage->blendFunc.src == GL_ONE && stage->blendFunc.dst == GL_ONE) && glConfig.textureEnvAdd))
				bundle->texEnv = GL_ADD;
			else
				bundle->texEnv = GL_MODULATE;
		}
		else {
			if ((stage->flags & SHADERSTAGE_BLENDFUNC) && (stage->blendFunc.src == GL_ONE && stage->blendFunc.dst == GL_ONE)){
				bundle->texEnv = GL_COMBINE_ARB;

				bundle->texEnvCombine.rgbCombine = GL_ADD;
				bundle->texEnvCombine.rgbSource[0] = GL_TEXTURE;
				bundle->texEnvCombine.rgbSource[1] = GL_PREVIOUS_ARB;
				bundle->texEnvCombine.rgbSource[2] = GL_CONSTANT_ARB;
				bundle->texEnvCombine.rgbOperand[0] = GL_SRC_COLOR;
				bundle->texEnvCombine.rgbOperand[1] = GL_SRC_COLOR;
				bundle->texEnvCombine.rgbOperand[2] = GL_SRC_ALPHA;
				bundle->texEnvCombine.rgbScale = 1;
				bundle->texEnvCombine.alphaCombine = GL_ADD;
				bundle->texEnvCombine.alphaSource[0] = GL_TEXTURE;
				bundle->texEnvCombine.alphaSource[1] = GL_PREVIOUS_ARB;
				bundle->texEnvCombine.alphaSource[2] = GL_CONSTANT_ARB;
				bundle->texEnvCombine.alphaOperand[0] = GL_SRC_ALPHA;
				bundle->texEnvCombine.alphaOperand[1] = GL_SRC_ALPHA;
				bundle->texEnvCombine.alphaOperand[2] = GL_SRC_ALPHA;
				bundle->texEnvCombine.alphaScale = 1;
				bundle->texEnvCombine.constColor[0] = 1.0;
				bundle->texEnvCombine.constColor[1] = 1.0;
				bundle->texEnvCombine.constColor[2] = 1.0;
				bundle->texEnvCombine.constColor[3] = 1.0;

				bundle->flags |= STAGEBUNDLE_TEXENVCOMBINE;
			}
			else {
				bundle->texEnv = GL_COMBINE_ARB;

				bundle->texEnvCombine.rgbCombine = GL_MODULATE;
				bundle->texEnvCombine.rgbSource[0] = GL_TEXTURE;
				bundle->texEnvCombine.rgbSource[1] = GL_PREVIOUS_ARB;
				bundle->texEnvCombine.rgbSource[2] = GL_CONSTANT_ARB;
				bundle->texEnvCombine.rgbOperand[0] = GL_SRC_COLOR;
				bundle->texEnvCombine.rgbOperand[1] = GL_SRC_COLOR;
				bundle->texEnvCombine.rgbOperand[2] = GL_SRC_ALPHA;
				bundle->texEnvCombine.rgbScale = 1;
				bundle->texEnvCombine.alphaCombine = GL_MODULATE;
				bundle->texEnvCombine.alphaSource[0] = GL_TEXTURE;
				bundle->texEnvCombine.alphaSource[1] = GL_PREVIOUS_ARB;
				bundle->texEnvCombine.alphaSource[2] = GL_CONSTANT_ARB;
				bundle->texEnvCombine.alphaOperand[0] = GL_SRC_ALPHA;
				bundle->texEnvCombine.alphaOperand[1] = GL_SRC_ALPHA;
				bundle->texEnvCombine.alphaOperand[2] = GL_SRC_ALPHA;
				bundle->texEnvCombine.alphaScale = 1;
				bundle->texEnvCombine.constColor[0] = 1.0;
				bundle->texEnvCombine.constColor[1] = 1.0;
				bundle->texEnvCombine.constColor[2] = 1.0;
				bundle->texEnvCombine.constColor[3] = 1.0;

				bundle->flags |= STAGEBUNDLE_TEXENVCOMBINE;
			}
		}
	}
	else {
		if (!Q_stricmp(tok, "modulate"))
			bundle->texEnv = GL_MODULATE;
		else if (!Q_stricmp(tok, "add")){
			if (!glConfig.textureEnvAdd){
				Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'nextBundle add' without 'requires GL_ARB_texture_env_add'\n", shader->name);
				return false;
			}

			bundle->texEnv = GL_ADD;
		}
		else if (!Q_stricmp(tok, "combine")){
			if (!glConfig.textureEnvCombine){
				Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'nextBundle combine' without 'requires GL_ARB_texture_env_combine'\n", shader->name);
				return false;
			}

			bundle->texEnv = GL_COMBINE_ARB;

			bundle->texEnvCombine.rgbCombine = GL_MODULATE;
			bundle->texEnvCombine.rgbSource[0] = GL_TEXTURE;
			bundle->texEnvCombine.rgbSource[1] = GL_PREVIOUS_ARB;
			bundle->texEnvCombine.rgbSource[2] = GL_CONSTANT_ARB;
			bundle->texEnvCombine.rgbOperand[0] = GL_SRC_COLOR;
			bundle->texEnvCombine.rgbOperand[1] = GL_SRC_COLOR;
			bundle->texEnvCombine.rgbOperand[2] = GL_SRC_ALPHA;
			bundle->texEnvCombine.rgbScale = 1;
			bundle->texEnvCombine.alphaCombine = GL_MODULATE;
			bundle->texEnvCombine.alphaSource[0] = GL_TEXTURE;
			bundle->texEnvCombine.alphaSource[1] = GL_PREVIOUS_ARB;
			bundle->texEnvCombine.alphaSource[2] = GL_CONSTANT_ARB;
			bundle->texEnvCombine.alphaOperand[0] = GL_SRC_ALPHA;
			bundle->texEnvCombine.alphaOperand[1] = GL_SRC_ALPHA;
			bundle->texEnvCombine.alphaOperand[2] = GL_SRC_ALPHA;
			bundle->texEnvCombine.alphaScale = 1;
			bundle->texEnvCombine.constColor[0] = 1.0;
			bundle->texEnvCombine.constColor[1] = 1.0;
			bundle->texEnvCombine.constColor[2] = 1.0;
			bundle->texEnvCombine.constColor[3] = 1.0;

			bundle->flags |= STAGEBUNDLE_TEXENVCOMBINE;
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'nextBundle' parameter '%s' in shader '%s'\n", tok, shader->name);
			return false;
		}
	}

	stage->flags |= SHADERSTAGE_NEXTBUNDLE;

	return true;
}

/*
 =================
 R_ParseStageVertexProgram
 =================
*/
static qboolean R_ParseStageVertexProgram (shader_t *shader, shaderStage_t *stage, char **script){

	char	*tok;

	if (!glConfig.vertexProgram){
		Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'vertexProgram' without 'requires GL_ARB_vertex_program'\n", shader->name);
		return false;
	}

	if (shader->shaderType == SHADER_NOMIP){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'vertexProgram' not allowed in shader '%s'\n", shader->name);
		return false;
	}

	if (stage->flags & SHADERSTAGE_NEXTBUNDLE){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'vertexProgram' not allowed in 'nextBundle' in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'vertexProgram' in shader '%s'\n", shader->name);
		return false;
	}

	stage->vertexProgram = R_FindProgram(tok, GL_VERTEX_PROGRAM_ARB);
	if (!stage->vertexProgram){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find vertex program '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	stage->flags |= SHADERSTAGE_VERTEXPROGRAM;

	return true;
}

/*
 =================
 R_ParseStageFragmentProgram
 =================
*/
static qboolean R_ParseStageFragmentProgram (shader_t *shader, shaderStage_t *stage, char **script){

	char	*tok;

	if (!glConfig.fragmentProgram){
		Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' uses 'fragmentProgram' without 'requires GL_ARB_fragment_program'\n", shader->name);
		return false;
	}

	if (shader->shaderType == SHADER_NOMIP){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'fragmentProgram' not allowed in shader '%s'\n", shader->name);
		return false;
	}

	if (stage->flags & SHADERSTAGE_NEXTBUNDLE){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'fragmentProgram' not allowed in 'nextBundle' in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'fragmentProgram' in shader '%s'\n", shader->name);
		return false;
	}

	stage->fragmentProgram = R_FindProgram(tok, GL_FRAGMENT_PROGRAM_ARB);
	if (!stage->fragmentProgram){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find fragment program '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	stage->flags |= SHADERSTAGE_FRAGMENTPROGRAM;

	return true;
}

/*
 =================
 R_ParseStageAlphaFunc
 =================
*/
static qboolean R_ParseStageAlphaFunc (shader_t *shader, shaderStage_t *stage, char **script){

	char	*tok;

	if (stage->flags & SHADERSTAGE_NEXTBUNDLE){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'alphaFunc' not allowed in 'nextBundle' in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'alphaFunc' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "GT0")){
		stage->alphaFunc.func = GL_GREATER;
		stage->alphaFunc.ref = 0.0;
	}
	else if (!Q_stricmp(tok, "LT128")){
		stage->alphaFunc.func = GL_LESS;
		stage->alphaFunc.ref = 0.5;
	}
	else if (!Q_stricmp(tok, "GE128")){
		stage->alphaFunc.func = GL_GEQUAL;
		stage->alphaFunc.ref = 0.5;
	}
	else {
		if (!Q_stricmp(tok, "GL_NEVER"))
			stage->alphaFunc.func = GL_NEVER;
		else if (!Q_stricmp(tok, "GL_ALWAYS"))
			stage->alphaFunc.func = GL_ALWAYS;
		else if (!Q_stricmp(tok, "GL_EQUAL"))
			stage->alphaFunc.func = GL_EQUAL;
		else if (!Q_stricmp(tok, "GL_NOTEQUAL"))
			stage->alphaFunc.func = GL_NOTEQUAL;
		else if (!Q_stricmp(tok, "GL_LEQUAL"))
			stage->alphaFunc.func = GL_LEQUAL;
		else if (!Q_stricmp(tok, "GL_GEQUAL"))
			stage->alphaFunc.func = GL_GEQUAL;
		else if (!Q_stricmp(tok, "GL_LESS"))
			stage->alphaFunc.func = GL_LESS;
		else if (!Q_stricmp(tok, "GL_GREATER"))
			stage->alphaFunc.func = GL_GREATER;
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'alphaFunc' parameter '%s' in shader '%s', defaulting to GL_GREATER\n", tok, shader->name);
			stage->alphaFunc.func = GL_GREATER;
		}

		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'alphaFunc' in shader '%s'\n", shader->name);
			return false;
		}

		stage->alphaFunc.ref = Clamp(atof(tok), 0.0, 1.0);
	}

	stage->flags |= SHADERSTAGE_ALPHAFUNC;

	return true;
}

/*
 =================
 R_ParseStageBlendFunc
 =================
*/
static qboolean R_ParseStageBlendFunc (shader_t *shader, shaderStage_t *stage, char **script){

	char	*tok;

	if (stage->flags & SHADERSTAGE_NEXTBUNDLE){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'blendFunc' not allowed in 'nextBundle' in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'blendFunc' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "add")){
		stage->blendFunc.src = GL_ONE;
		stage->blendFunc.dst = GL_ONE;
	}
	else if (!Q_stricmp(tok, "filter")){
		stage->blendFunc.src = GL_DST_COLOR;
		stage->blendFunc.dst = GL_ZERO;
	}
	else if (!Q_stricmp(tok, "blend")){
		stage->blendFunc.src = GL_SRC_ALPHA;
		stage->blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;
	}
	else {
		if (!Q_stricmp(tok, "GL_ZERO"))
			stage->blendFunc.src = GL_ZERO;
		else if (!Q_stricmp(tok, "GL_ONE"))
			stage->blendFunc.src = GL_ONE;
		else if (!Q_stricmp(tok, "GL_DST_COLOR"))
			stage->blendFunc.src = GL_DST_COLOR;
		else if (!Q_stricmp(tok, "GL_ONE_MINUS_DST_COLOR"))
			stage->blendFunc.src = GL_ONE_MINUS_DST_COLOR;
		else if (!Q_stricmp(tok, "GL_SRC_ALPHA"))
			stage->blendFunc.src = GL_SRC_ALPHA;
		else if (!Q_stricmp(tok, "GL_ONE_MINUS_SRC_ALPHA"))
			stage->blendFunc.src = GL_ONE_MINUS_SRC_ALPHA;
		else if (!Q_stricmp(tok, "GL_DST_ALPHA"))
			stage->blendFunc.src = GL_DST_ALPHA;
		else if (!Q_stricmp(tok, "GL_ONE_MINUS_DST_ALPHA"))
			stage->blendFunc.src = GL_ONE_MINUS_DST_ALPHA;
		else if (!Q_stricmp(tok, "GL_SRC_ALPHA_SATURATE"))
			stage->blendFunc.src = GL_SRC_ALPHA_SATURATE;
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'blendFunc' parameter '%s' in shader '%s', defaulting to GL_ONE\n", tok, shader->name);
			stage->blendFunc.src = GL_ONE;
		}

		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'blendFunc' in shader '%s'\n", shader->name);
			return false;
		}

		if (!Q_stricmp(tok, "GL_ZERO"))
			stage->blendFunc.dst = GL_ZERO;
		else if (!Q_stricmp(tok, "GL_ONE"))
			stage->blendFunc.dst = GL_ONE;
		else if (!Q_stricmp(tok, "GL_SRC_COLOR"))
			stage->blendFunc.dst = GL_SRC_COLOR;
		else if (!Q_stricmp(tok, "GL_ONE_MINUS_SRC_COLOR"))
			stage->blendFunc.dst = GL_ONE_MINUS_SRC_COLOR;
		else if (!Q_stricmp(tok, "GL_SRC_ALPHA"))
			stage->blendFunc.dst = GL_SRC_ALPHA;
		else if (!Q_stricmp(tok, "GL_ONE_MINUS_SRC_ALPHA"))
			stage->blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;
		else if (!Q_stricmp(tok, "GL_DST_ALPHA"))
			stage->blendFunc.dst = GL_DST_ALPHA;
		else if (!Q_stricmp(tok, "GL_ONE_MINUS_DST_ALPHA"))
			stage->blendFunc.dst = GL_ONE_MINUS_DST_ALPHA;
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'blendFunc' parameter '%s' in shader '%s', defaulting to GL_ONE\n", tok, shader->name);
			stage->blendFunc.dst = GL_ONE;
		}
	}

	stage->flags |= SHADERSTAGE_BLENDFUNC;

	return true;
}

/*
 =================
 R_ParseStageDepthFunc
 =================
*/
static qboolean R_ParseStageDepthFunc (shader_t *shader, shaderStage_t *stage, char **script){

	char	*tok;

	if (stage->flags & SHADERSTAGE_NEXTBUNDLE){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'depthFunc' not allowed in 'nextBundle' in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'depthFunc' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "lequal"))
		stage->depthFunc.func = GL_LEQUAL;
	else if (!Q_stricmp(tok, "equal"))
		stage->depthFunc.func = GL_EQUAL;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'depthFunc' parameter '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	stage->flags |= SHADERSTAGE_DEPTHFUNC;

	return true;
}

/*
 =================
 R_ParseStageDepthWrite
 =================
*/
static qboolean R_ParseStageDepthWrite (shader_t *shader, shaderStage_t *stage, char **script){

	if (stage->flags & SHADERSTAGE_NEXTBUNDLE){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'depthWrite' not allowed in 'nextBundle' in shader '%s'\n", shader->name);
		return false;
	}

	stage->flags |= SHADERSTAGE_DEPTHWRITE;

	return true;
}

/*
 =================
 R_ParseStageDetail
 =================
*/
static qboolean R_ParseStageDetail (shader_t *shader, shaderStage_t *stage, char **script){

	if (stage->flags & SHADERSTAGE_NEXTBUNDLE){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'detail' not allowed in 'nextBundle' in shader '%s'\n", shader->name);
		return false;
	}

	stage->flags |= SHADERSTAGE_DETAIL;

	return true;
}

/*
 =================
 R_ParseStageRgbGen
 =================
*/
static qboolean R_ParseStageRgbGen (shader_t *shader, shaderStage_t *stage, char **script){

	char	*tok;
	int		i;

	if (stage->flags & SHADERSTAGE_NEXTBUNDLE){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'rgbGen' not allowed in 'nextBundle' in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'rgbGen' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "identity"))
		stage->rgbGen.type = RGBGEN_IDENTITY;
	else if (!Q_stricmp(tok, "identityLighting"))
		stage->rgbGen.type = RGBGEN_IDENTITYLIGHTING;
	else if (!Q_stricmp(tok, "wave")){
		if (!R_ParseWaveFunc(shader, &stage->rgbGen.func, script)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parameters for 'rgbGen wave' in shader '%s'\n", shader->name);
			return false;
		}

		stage->rgbGen.type = RGBGEN_WAVE;
	}
	else if (!Q_stricmp(tok, "colorWave")){
		for (i = 0; i < 3; i++){
			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'rgbGen colorWave' in shader '%s'\n", shader->name);
				return false;
			}

			stage->rgbGen.params[i] = Clamp(atof(tok), 0.0, 1.0);
		}

		if (!R_ParseWaveFunc(shader, &stage->rgbGen.func, script)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parameters for 'rgbGen colorWave' in shader '%s'\n", shader->name);
			return false;
		}

		stage->rgbGen.type = RGBGEN_COLORWAVE;
	}
	else if (!Q_stricmp(tok, "vertex"))
		stage->rgbGen.type = RGBGEN_VERTEX;
	else if (!Q_stricmp(tok, "oneMinusVertex"))
		stage->rgbGen.type = RGBGEN_ONEMINUSVERTEX;
	else if (!Q_stricmp(tok, "entity"))
		stage->rgbGen.type = RGBGEN_ENTITY;
	else if (!Q_stricmp(tok, "oneMinusEntity"))
		stage->rgbGen.type = RGBGEN_ONEMINUSENTITY;
	else if (!Q_stricmp(tok, "lightingAmbient"))
		stage->rgbGen.type = RGBGEN_LIGHTINGAMBIENT;
	else if (!Q_stricmp(tok, "lightingDiffuse"))
		stage->rgbGen.type = RGBGEN_LIGHTINGDIFFUSE;
	else if (!Q_stricmp(tok, "const") || !Q_stricmp(tok, "constant")){
		for (i = 0; i < 3; i++){
			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'rgbGen const' in shader '%s'\n", shader->name);
				return false;
			}

			stage->rgbGen.params[i] = Clamp(atof(tok), 0.0, 1.0);
		}

		stage->rgbGen.type = RGBGEN_CONST;
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'rgbGen' parameter '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	stage->flags |= SHADERSTAGE_RGBGEN;

	return true;
}

/*
 =================
 R_ParseStageAlphaGen
 =================
*/
static qboolean R_ParseStageAlphaGen (shader_t *shader, shaderStage_t *stage, char **script){

	char	*tok;

	if (stage->flags & SHADERSTAGE_NEXTBUNDLE){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'alphaGen' not allowed in 'nextBundle' in shader '%s'\n", shader->name);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'alphaGen' in shader '%s'\n", shader->name);
		return false;
	}

	if (!Q_stricmp(tok, "identity"))
		stage->alphaGen.type = ALPHAGEN_IDENTITY;
	else if (!Q_stricmp(tok, "wave")){
		if (!R_ParseWaveFunc(shader, &stage->alphaGen.func, script)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parameters for 'alphaGen wave' in shader '%s'\n", shader->name);
			return false;
		}

		stage->alphaGen.type = ALPHAGEN_WAVE;
	}
	else if (!Q_stricmp(tok, "alphaWave")){
		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'alphaGen alphaWave' in shader '%s'\n", shader->name);
			return false;
		}

		stage->alphaGen.params[0] = Clamp(atof(tok), 0.0, 1.0);

		if (!R_ParseWaveFunc(shader, &stage->alphaGen.func, script)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parameters for 'alphaGen alphaWave' in shader '%s'\n", shader->name);
			return false;
		}

		stage->alphaGen.type = ALPHAGEN_ALPHAWAVE;
	}
	else if (!Q_stricmp(tok, "vertex"))
		stage->alphaGen.type = ALPHAGEN_VERTEX;
	else if (!Q_stricmp(tok, "oneMinusVertex"))
		stage->alphaGen.type = ALPHAGEN_ONEMINUSVERTEX;
	else if (!Q_stricmp(tok, "entity"))
		stage->alphaGen.type = ALPHAGEN_ENTITY;
	else if (!Q_stricmp(tok, "oneMinusEntity"))
		stage->alphaGen.type = ALPHAGEN_ONEMINUSENTITY;
	else if (!Q_stricmp(tok, "dot")){
		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			stage->alphaGen.params[0] = 0.0;
			stage->alphaGen.params[1] = 1.0;
		}
		else {
			stage->alphaGen.params[0] = Clamp(atof(tok), 0.0, 1.0);

			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'alphaGen dot' in shader '%s'\n", shader->name);
				return false;
			}

			stage->alphaGen.params[1] = Clamp(atof(tok), 0.0, 1.0);
		}

		stage->alphaGen.type = ALPHAGEN_DOT;
	}
	else if (!Q_stricmp(tok, "oneMinusDot")){
		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			stage->alphaGen.params[0] = 0.0;
			stage->alphaGen.params[1] = 1.0;
		}
		else {
			stage->alphaGen.params[0] = Clamp(atof(tok), 0.0, 1.0);

			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'alphaGen oneMinusDot' in shader '%s'\n", shader->name);
				return false;
			}

			stage->alphaGen.params[1] = Clamp(atof(tok), 0.0, 1.0);
		}

		stage->alphaGen.type = ALPHAGEN_ONEMINUSDOT;
	}
	else if (!Q_stricmp(tok, "fade")){
		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			stage->alphaGen.params[0] = 0.0;
			stage->alphaGen.params[1] = 256.0;
			stage->alphaGen.params[2] = 1.0 / 256.0;
		}
		else {
			stage->alphaGen.params[0] = atof(tok);

			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'alphaGen fade' in shader '%s'\n", shader->name);
				return false;
			}

			stage->alphaGen.params[1] = atof(tok);

			stage->alphaGen.params[2] = stage->alphaGen.params[1] - stage->alphaGen.params[0];
			if (stage->alphaGen.params[2])
				stage->alphaGen.params[2] = 1.0 / stage->alphaGen.params[2];
		}

		stage->alphaGen.type = ALPHAGEN_FADE;
	}
	else if (!Q_stricmp(tok, "oneMinusFade")){
		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			stage->alphaGen.params[0] = 0.0;
			stage->alphaGen.params[1] = 256.0;
			stage->alphaGen.params[2] = 1.0 / 256.0;
		}
		else {
			stage->alphaGen.params[0] = atof(tok);

			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'alphaGen oneMinusFade' in shader '%s'\n", shader->name);
				return false;
			}

			stage->alphaGen.params[1] = atof(tok);

			stage->alphaGen.params[2] = stage->alphaGen.params[1] - stage->alphaGen.params[0];
			if (stage->alphaGen.params[2])
				stage->alphaGen.params[2] = 1.0 / stage->alphaGen.params[2];
		}

		stage->alphaGen.type = ALPHAGEN_ONEMINUSFADE;
	}
	else if (!Q_stricmp(tok, "lightingSpecular")){
		tok = Com_ParseExt(script, false);
		if (!tok[0])
			stage->alphaGen.params[0] = 5.0;
		else {
			stage->alphaGen.params[0] = atof(tok);
			if (stage->alphaGen.params[0] <= 0.0){
				Com_Printf(S_COLOR_YELLOW "WARNING: invalid exponent value of %f for 'alphaGen lightingSpecular' in shader '%s', defaulting to 5\n", stage->alphaGen.params[0], shader->name);
				stage->alphaGen.params[0] = 5.0;
			}
		}

		stage->alphaGen.type = ALPHAGEN_LIGHTINGSPECULAR;
	}
	else if (!Q_stricmp(tok, "const") || !Q_stricmp(tok, "constant")){
		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'alphaGen const' in shader '%s'\n", shader->name);
			return false;
		}
		
		stage->alphaGen.params[0] = Clamp(atof(tok), 0.0, 1.0);

		stage->alphaGen.type = ALPHAGEN_CONST;
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'alphaGen' parameter '%s' in shader '%s'\n", tok, shader->name);
		return false;
	}

	stage->flags |= SHADERSTAGE_ALPHAGEN;

	return true;
}


// =====================================================================

typedef struct {
	const char	*name;
	qboolean	(*parseFunc) (shader_t *shader, char **script);
} shaderGeneralCmd_t;

typedef struct {
	const char	*name;
	qboolean	(*parseFunc) (shader_t *shader, shaderStage_t *stage, char **script);
} shaderStageCmd_t;

static shaderGeneralCmd_t	r_shaderGeneralCmds[] = {
	{"surfaceParm",				R_ParseGeneralSurfaceParm},
	{"noMipmaps",				R_ParseGeneralNoMipmaps},
	{"noPicmip",				R_ParseGeneralNoPicmip},
	{"noCompress",				R_ParseGeneralNoCompress},
	{"noShadows",				R_ParseGeneralNoShadows},
	{"noFragments",				R_ParseGeneralNoFragments},
	{"entityMergable",			R_ParseGeneralEntityMergable},
	{"polygonOffset",			R_ParseGeneralPolygonOffset},
	{"cull",					R_ParseGeneralCull},
	{"sort",					R_ParseGeneralSort},
	{"ammoDisplay",				R_ParseGeneralAmmoDisplay},
	{"tessSize",				R_ParseGeneralTessSize},
	{"skyParms",				R_ParseGeneralSkyParms},
	{"deformVertexes",			R_ParseGeneralDeformVertexes},
	{NULL,						NULL}
};

static shaderStageCmd_t		r_shaderStageCmds[] = {
	{"requires",				R_ParseStageRequires},
	{"noMipmaps",				R_ParseStageNoMipmaps},
	{"noPicmip",				R_ParseStageNoPicmip},
	{"noCompress",				R_ParseStageNoCompress},
	{"clampTexCoords",			R_ParseStageClampTexCoords},
	{"animFrequency",			R_ParseStageAnimFrequency},
	{"map",						R_ParseStageMap},
	{"bumpMap",					R_ParseStageBumpMap},
	{"cubeMap",					R_ParseStageCubeMap},
	{"videoMap",				R_ParseStageVideoMap},
	{"texEnvCombine",			R_ParseStageTexEnvCombine},
	{"tcGen",					R_ParseStageTcGen},
	{"tcMod",					R_ParseStageTcMod},
	{"nextBundle",				R_ParseStageNextBundle},
	{"vertexProgram",			R_ParseStageVertexProgram},
	{"fragmentProgram",			R_ParseStageFragmentProgram},
	{"alphaFunc",				R_ParseStageAlphaFunc},
	{"blendFunc",				R_ParseStageBlendFunc},
	{"depthFunc",				R_ParseStageDepthFunc},
	{"depthWrite",				R_ParseStageDepthWrite},
	{"detail",					R_ParseStageDetail},
	{"rgbGen",					R_ParseStageRgbGen},
	{"alphaGen",				R_ParseStageAlphaGen},
	{NULL,						NULL}
};


/*
 =================
 R_ParseShaderCommand
 =================
*/
static qboolean R_ParseShaderCommand (shader_t *shader, char **script, char *command){

	shaderGeneralCmd_t	*cmd;

	for (cmd = r_shaderGeneralCmds; cmd->name != NULL; cmd++){
		if (!Q_stricmp(cmd->name, command))
			return cmd->parseFunc(shader, script);
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown general command '%s' in shader '%s'\n", command, shader->name);
	return false;
}

/*
 =================
 R_ParseShaderStageCommand
 =================
*/
static qboolean R_ParseShaderStageCommand (shader_t *shader, shaderStage_t *stage, char **script, char *command){

	shaderStageCmd_t	*cmd;

	for (cmd = r_shaderStageCmds; cmd->name != NULL; cmd++){
		if (!Q_stricmp(cmd->name, command))
			return cmd->parseFunc(shader, stage, script);
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown stage command '%s' in shader '%s'\n", command, shader->name);
	return false;
}

/*
 =================
 R_EvaluateRequires
 =================
*/
static qboolean R_EvaluateRequires (shader_t *shader, char **script){

	qboolean	results[SHADER_MAX_EXPRESSIONS];
	qboolean	logicAnd = false, logicOr = false;
	qboolean	negate, expectingExpression = false;
	char		*tok;
	char		cmpOperator[8];
	int			cmpOperand1, cmpOperand2;
	int			i, count = 0;

	// Parse the expressions
	while (1){
		tok = Com_ParseExt(script, false);
		if (!tok[0]){
			if (count == 0 || expectingExpression){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing expression for 'requires' in shader '%s', discarded stage\n", shader->name);
				return false;
			}

			break;		// End of data
		}

		if (!Q_stricmp(tok, "&&")){
			if (count == 0 || expectingExpression){
				Com_Printf(S_COLOR_YELLOW "WARNING: 'requires' has logical operator without preceding expression in shader '%s', discarded stage\n", shader->name);
				return false;
			}

			logicAnd = true;
			expectingExpression = true;

			continue;
		}
		if (!Q_stricmp(tok, "||")){
			if (count == 0 || expectingExpression){
				Com_Printf(S_COLOR_YELLOW "WARNING: 'requires' has logical operator without preceding expression in shader '%s', discarded stage\n", shader->name);
				return false;
			}

			logicOr = true;
			expectingExpression = true;

			continue;
		}

		expectingExpression = false;

		if (count == SHADER_MAX_EXPRESSIONS){
			Com_Printf(S_COLOR_YELLOW "WARNING: SHADER_MAX_EXPRESSIONS hit in shader '%s', discarded stage\n", shader->name);
			return false;
		}

		if (!Q_stricmp(tok, "GL_MAX_TEXTURE_UNITS_ARB") || !Q_stricmp(tok, "GL_MAX_TEXTURE_COORDS_ARB") || !Q_stricmp(tok, "GL_MAX_TEXTURE_IMAGE_UNITS_ARB")){
			if (!Q_stricmp(tok, "GL_MAX_TEXTURE_UNITS_ARB"))
				cmpOperand1 = glConfig.maxTextureUnits;
			else if (!Q_stricmp(tok, "GL_MAX_TEXTURE_COORDS_ARB"))
				cmpOperand1 = glConfig.maxTextureCoords;
			else if (!Q_stricmp(tok, "GL_MAX_TEXTURE_IMAGE_UNITS_ARB"))
				cmpOperand1 = glConfig.maxTextureImageUnits;

			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing operator for 'requires' expression in shader '%s', discarded stage\n", shader->name);
				return false;
			}
			Q_strncpyz(cmpOperator, tok, sizeof(cmpOperator));

			tok = Com_ParseExt(script, false);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing operand for 'requires' expression in shader '%s', discarded stage\n", shader->name);
				return false;
			}
			cmpOperand2 = atoi(tok);

			if (!Q_stricmp(cmpOperator, "=="))
				results[count] = (cmpOperand1 == cmpOperand2);
			else if (!Q_stricmp(cmpOperator, "!="))
				results[count] = (cmpOperand1 != cmpOperand2);
			else if (!Q_stricmp(cmpOperator, ">="))
				results[count] = (cmpOperand1 >= cmpOperand2);
			else if (!Q_stricmp(cmpOperator, "<="))
				results[count] = (cmpOperand1 <= cmpOperand2);
			else if (!Q_stricmp(cmpOperator, ">"))
				results[count] = (cmpOperand1 > cmpOperand2);
			else if (!Q_stricmp(cmpOperator, "<"))
				results[count] = (cmpOperand1 < cmpOperand2);
			else {
				Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'requires' operator '%s' in shader '%s', discarded stage\n", cmpOperator, shader->name);
				return false;
			}
		}
		else {
			if (!Q_stricmp(tok, "!")){
				negate = true;

				tok = Com_ParseExt(script, false);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing expression for 'requires' in shader '%s', discarded stage\n", shader->name);
					return false;
				}
			}
			else {
				if (tok[0] == '!'){
					tok++;

					negate = true;
				}
				else
					negate = false;
			}

			if (!Q_stricmp(tok, "GL_ARB_multitexture"))
				results[count]= glConfig.multitexture;
			else if (!Q_stricmp(tok, "GL_ARB_texture_env_add"))
				results[count] = glConfig.textureEnvAdd;
			else if (!Q_stricmp(tok, "GL_ARB_texture_env_combine"))
				results[count] = glConfig.textureEnvCombine;
			else if (!Q_stricmp(tok, "GL_ARB_texture_env_dot3"))
				results[count] = glConfig.textureEnvDot3;
			else if (!Q_stricmp(tok, "GL_ARB_texture_cube_map"))
				results[count] = glConfig.textureCubeMap;
			else if (!Q_stricmp(tok, "GL_ARB_vertex_program"))
				results[count] = glConfig.vertexProgram;
			else if (!Q_stricmp(tok, "GL_ARB_fragment_program"))
				results[count] = glConfig.fragmentProgram;
			else {
				Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'requires' expression '%s' in shader '%s', discarded stage\n", tok, shader->name);
				return false;
			}

			if (negate)
				results[count] = !results[count];
		}

		count++;
	}

	// Evaluate expressions
	if (logicAnd && logicOr){
		Com_Printf(S_COLOR_YELLOW "WARNING: different logical operators used for 'requires' in shader '%s', discarded stage\n", shader->name);
		return false;
	}

	if (logicAnd){
		// All expressions must evaluate to true
		for (i = 0; i < count; i++){
			if (results[i] == false)
				return false;
		}

		return true;
	}
	else if (logicOr){
		// At least one of the expressions must evaluate to true
		for (i = 0; i < count; i++){
			if (results[i] == true)
				return true;
		}

		return false;
	}

	return results[0];
}

/*
 =================
 R_SkipShaderStage
 =================
*/
static qboolean R_SkipShaderStage (shader_t *shader, char **script){

	char	*tok;

	while (1){
		Com_BackupParseSession(script);

		tok = Com_ParseExt(script, true);
		if (!Q_stricmp(tok, "requires")){
			if (!R_EvaluateRequires(shader, script)){
				Com_SkipBracedSection(script, 1);
				return true;
			}

			continue;
		}

		Com_RestoreParseSession(script);
		break;
	}

	return false;
}

/*
 =================
 R_ParseShader
 =================
*/
static qboolean R_ParseShader (shader_t *shader, char *script){

	shaderStage_t	*stage;
	char			*tok;

	tok = Com_ParseExt(&script, true);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' has an empty script\n", shader->name);
		return false;
	}

	// Parse the shader
	if (!Q_stricmp(tok, "{")){
		while (1){
			tok = Com_ParseExt(&script, true);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in shader '%s'\n", shader->name);
				return false;	// End of data
			}

			if (!Q_stricmp(tok, "}"))
				break;			// End of shader

			// Parse a stage
			if (!Q_stricmp(tok, "{")){
				// Check if we need to skip this stage
				if (R_SkipShaderStage(shader, &script))
					continue;

				// Create a new stage
				if (shader->numStages == SHADER_MAX_STAGES){
					Com_Printf(S_COLOR_YELLOW "WARNING: SHADER_MAX_STAGES hit in shader '%s'\n", shader->name);
					return false;
				}
				stage = shader->stages[shader->numStages++];

				stage->numBundles++;

				// Parse it
				while (1){
					tok = Com_ParseExt(&script, true);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: no matching '}' in shader '%s'\n", shader->name);
						return false;	// End of data
					}

					if (!Q_stricmp(tok, "}"))
						break;			// End of stage

					// Parse the command
					if (!R_ParseShaderStageCommand(shader, stage, &script, tok))
						return false;
				}

				continue;
			}

			// Parse the command
			if (!R_ParseShaderCommand(shader, &script, tok))
				return false;
		}

		return true;
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in shader '%s'\n", shader->name);
		return false;
	}
}

/*
 =================
 R_ParseShaderFile
 =================
*/
static void R_ParseShaderFile (char *buffer, int size){

	shaderScript_t	*shaderScript;
	char			*buf, *tok;
	char			name[MAX_QPATH];
	char			*ptr1, *ptr2;
	char			*script;
	shaderType_t	shaderType;
	unsigned		surfaceParm;
	unsigned		hashKey;

	buf = buffer;
	while (1){
		// Parse the name
		tok = Com_ParseExt(&buf, true);
		if (!tok[0])
			break;		// End of data

		Q_strncpyz(name, tok, sizeof(name));

		// Parse the script
		ptr1 = buf;
		Com_SkipBracedSection(&buf, 0);
		ptr2 = buf;

		if (!ptr1)
			ptr1 = buffer;
		if (!ptr2)
			ptr2 = buffer + size;

		// We must parse surfaceParm commands here, because R_FindShader
		// needs this for correct shader loading.
		// Proper syntax checking is done when the shader is loaded.
		shaderType = -1;
		surfaceParm = 0;

		script = ptr1;
		while (script < ptr2){
			tok = Com_ParseExt(&script, true);
			if (!tok[0])
				break;		// End of data

			if (!Q_stricmp(tok, "surfaceParm")){
				tok = Com_ParseExt(&script, false);
				if (!tok[0])
					continue;

				if (!Q_stricmp(tok, "lightmap"))
					surfaceParm |= SURFACEPARM_LIGHTMAP;
				else if (!Q_stricmp(tok, "warp"))
					surfaceParm |= SURFACEPARM_WARP;
				else if (!Q_stricmp(tok, "trans33"))
					surfaceParm |= SURFACEPARM_TRANS33;
				else if (!Q_stricmp(tok, "trans66"))
					surfaceParm |= SURFACEPARM_TRANS66;
				else if (!Q_stricmp(tok, "flowing"))
					surfaceParm |= SURFACEPARM_FLOWING;
				else
					continue;

				shaderType = SHADER_BSP;
			}
		}

		// Store the script
		shaderScript = Hunk_Alloc(sizeof(shaderScript_t) + (ptr2 - ptr1) + 1);
		Q_strncpyz(shaderScript->name, name, sizeof(shaderScript->name));
		shaderScript->shaderType = shaderType;
		shaderScript->surfaceParm = surfaceParm;
		memcpy(shaderScript->script, ptr1, (ptr2 - ptr1));

		// Add to hash table
		hashKey = Com_HashKey(shaderScript->name, SHADERS_HASHSIZE);

		shaderScript->nextHash = r_shaderScriptsHash[hashKey];
		r_shaderScriptsHash[hashKey] = shaderScript;
	}
}


/*
 =======================================================================

 SHADER INITIALIZATION AND LOADING

 =======================================================================
*/


/*
 =================
 R_NewShader
 =================
*/
static shader_t *R_NewShader (void){

	shader_t	*shader;
	int			i, j;

	shader = &r_parseShader;
	memset(shader, 0, sizeof(shader_t));

	for (i = 0; i < SHADER_MAX_STAGES; i++){
		shader->stages[i] = &r_parseShaderStages[i];
		memset(shader->stages[i], 0, sizeof(shaderStage_t));

		for (j = 0; j < MAX_TEXTURE_UNITS; j++){
			shader->stages[i]->bundles[j] = &r_parseStageTMU[i][j];
			memset(shader->stages[i]->bundles[j], 0, sizeof(stageBundle_t));
		}
	}

	return shader;
}

/*
 =================
 R_CreateShader
 =================
*/
static shader_t *R_CreateShader (const char *name, shaderType_t shaderType, unsigned surfaceParm, char *script){

	shader_t		*shader;
	shaderStage_t	*stage;
	stageBundle_t	*bundle;
	int				i, j;

	// Clear static shader
	shader = R_NewShader();

	// Fill it in
	Q_strncpyz(shader->name, name, sizeof(shader->name));
	shader->shaderNum = r_numShaders;
	shader->shaderType = shaderType;
	shader->surfaceParm = surfaceParm;

	shader->flags = SHADER_EXTERNAL;

	if (shaderType == SHADER_NOMIP)
		shader->flags |= (SHADER_NOMIPMAPS | SHADER_NOPICMIP);

	if (!R_ParseShader(shader, script)){
		// Invalid script, so make sure we stop cinematics
		for (i = 0; i < shader->numStages; i++){
			stage = shader->stages[i];

			for (j = 0; j < stage->numBundles; j++){
				bundle = stage->bundles[j];

				if (bundle->flags & STAGEBUNDLE_VIDEOMAP)
					CIN_StopCinematic(bundle->cinematicHandle);
			}
		}

		// Use a default shader instead
		shader = R_NewShader();

		Q_strncpyz(shader->name, name, sizeof(shader->name));
		shader->shaderNum = r_numShaders;
		shader->shaderType = shaderType;
		shader->surfaceParm = surfaceParm;
		shader->flags = SHADER_EXTERNAL | SHADER_DEFAULTED;
		shader->stages[0]->bundles[0]->textures[0] = r_defaultTexture;
		shader->stages[0]->bundles[0]->numTextures++;
		shader->stages[0]->numBundles++;
		shader->numStages++;
	}

	return shader;
}

/*
 =================
 R_CreateDefaultShader
 =================
*/
static shader_t *R_CreateDefaultShader (const char *name, shaderType_t shaderType, unsigned surfaceParm){

	shader_t	*shader;
	int			i;

	// Clear static shader
	shader = R_NewShader();

	// Fill it in
	Q_strncpyz(shader->name, name, sizeof(shader->name));
	shader->shaderNum = r_numShaders;
	shader->shaderType = shaderType;
	shader->surfaceParm = surfaceParm;

	switch (shader->shaderType){
	case SHADER_SKY:
		shader->flags |= SHADER_SKYPARMS;

		for (i = 0; i < 6; i++){
			shader->skyParms.farBox[i] = R_FindTexture(va("env/%s%s.tga", shader->name, r_skyBoxSuffix[i]), TF_CLAMP, 0);
			if (!shader->skyParms.farBox[i]){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for shader '%s', using default...\n", shader->name);
				shader->skyParms.farBox[i] = r_defaultTexture;
			}
		}

		shader->skyParms.cloudHeight = 128.0;
			
		break;
	case SHADER_BSP:
		shader->stages[0]->bundles[0]->flags |= STAGEBUNDLE_MAP;

		shader->stages[0]->bundles[0]->textures[0] = R_FindTexture(va("%s.tga", shader->name), TF_MIPMAPS | TF_PICMIP | TF_COMPRESS, 0);
		if (!shader->stages[0]->bundles[0]->textures[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for shader '%s', using default...\n", shader->name);
			shader->stages[0]->bundles[0]->textures[0] = r_defaultTexture;
		}
		shader->stages[0]->bundles[0]->numTextures++;

		if (shader->surfaceParm & (SURFACEPARM_TRANS33 | SURFACEPARM_TRANS66)){
			shader->stages[0]->flags |= SHADERSTAGE_BLENDFUNC;
			shader->stages[0]->blendFunc.src = GL_SRC_ALPHA;
			shader->stages[0]->blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;
		}

		if (shader->surfaceParm & SURFACEPARM_WARP){
			shader->flags |= SHADER_NOFRAGMENTS;

			shader->flags |= SHADER_TESSSIZE;
			shader->tessSize = 64;

			shader->stages[0]->bundles[0]->flags |= STAGEBUNDLE_TCGEN;
			shader->stages[0]->bundles[0]->tcGen.type = TCGEN_WARP;

			if (shader->surfaceParm & SURFACEPARM_FLOWING){
				shader->stages[0]->bundles[0]->flags |= STAGEBUNDLE_TCMOD;
				shader->stages[0]->bundles[0]->tcMod[1].params[0] = -0.5;
				shader->stages[0]->bundles[0]->tcMod[1].params[1] = 0.0;
				shader->stages[0]->bundles[0]->tcMod[1].type = TCMOD_SCROLL;
				shader->stages[0]->bundles[0]->tcModNum++;
			}
		}
		else {
			if (shader->surfaceParm & SURFACEPARM_FLOWING){
				shader->stages[0]->bundles[0]->flags |= STAGEBUNDLE_TCMOD;
				shader->stages[0]->bundles[0]->tcMod[0].params[0] = -0.25;
				shader->stages[0]->bundles[0]->tcMod[0].params[1] = 0.0;
				shader->stages[0]->bundles[0]->tcMod[0].type = TCMOD_SCROLL;
				shader->stages[0]->bundles[0]->tcModNum++;
			}
		}

		shader->stages[0]->numBundles++;
		shader->numStages++;

		if (shader->surfaceParm & SURFACEPARM_LIGHTMAP){
			shader->flags |= SHADER_HASLIGHTMAP;

			shader->stages[1]->bundles[0]->flags |= STAGEBUNDLE_MAP;
			shader->stages[1]->bundles[0]->texType = TEX_LIGHTMAP;

			shader->stages[1]->flags |= SHADERSTAGE_BLENDFUNC;
			shader->stages[1]->blendFunc.src = GL_DST_COLOR;
			shader->stages[1]->blendFunc.dst = GL_ZERO;

			shader->stages[1]->numBundles++;
			shader->numStages++;
		}

		break;
	case SHADER_SKIN:
		shader->stages[0]->bundles[0]->flags |= STAGEBUNDLE_MAP;

		shader->stages[0]->bundles[0]->textures[0] = R_FindTexture(va("%s.tga", shader->name), TF_MIPMAPS | TF_PICMIP | TF_COMPRESS, 0);
		if (!shader->stages[0]->bundles[0]->textures[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for shader '%s', using default...\n", shader->name);
			shader->stages[0]->bundles[0]->textures[0] = r_defaultTexture;
		}
		shader->stages[0]->bundles[0]->numTextures++;

		shader->stages[0]->numBundles++;
		shader->numStages++;

		break;
	case SHADER_NOMIP:
		shader->stages[0]->bundles[0]->flags |= STAGEBUNDLE_MAP;

		shader->stages[0]->bundles[0]->textures[0] = R_FindTexture(va("%s.tga", shader->name), TF_COMPRESS, 0);
		if (!shader->stages[0]->bundles[0]->textures[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for shader '%s', using default...\n", shader->name);
			shader->stages[0]->bundles[0]->textures[0] = r_defaultTexture;
		}
		shader->stages[0]->bundles[0]->numTextures++;

		shader->stages[0]->flags |= SHADERSTAGE_BLENDFUNC;
		shader->stages[0]->blendFunc.src = GL_SRC_ALPHA;
		shader->stages[0]->blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;

		shader->stages[0]->numBundles++;
		shader->numStages++;

		break;
	case SHADER_GENERIC:
		shader->stages[0]->bundles[0]->flags |= STAGEBUNDLE_MAP;

		shader->stages[0]->bundles[0]->textures[0] = R_FindTexture(va("%s.tga", shader->name), TF_MIPMAPS | TF_PICMIP | TF_COMPRESS, 0);
		if (!shader->stages[0]->bundles[0]->textures[0]){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for shader '%s', using default...\n", shader->name);
			shader->stages[0]->bundles[0]->textures[0] = r_defaultTexture;
		}
		shader->stages[0]->bundles[0]->numTextures++;

		shader->stages[0]->numBundles++;
		shader->numStages++;

		break;
	}

	return shader;
}

/*
 =================
 R_FinishShader
 =================
*/
static void R_FinishShader (shader_t *shader){

	shaderStage_t	*stage;
	stageBundle_t	*bundle;
	int				i, j;

	// Remove entityMergable from 2D shaders
	if (shader->shaderType == SHADER_NOMIP)
		shader->flags &= ~SHADER_ENTITYMERGABLE;

	// Remove polygonOffset from 2D shaders
	if (shader->shaderType == SHADER_NOMIP)
		shader->flags &= ~SHADER_POLYGONOFFSET;

	// Set cull if unset
	if (!(shader->flags & SHADER_CULL)){
		shader->flags |= SHADER_CULL;
		shader->cull.mode = GL_FRONT;
	}
	else {
		// Remove if undefined (disabled)
		if (shader->cull.mode == 0)
			shader->flags &= ~SHADER_CULL;
	}

	// Remove cull from 2D shaders
	if (shader->shaderType == SHADER_NOMIP){
		shader->flags &= ~SHADER_CULL;
		shader->cull.mode = 0;
	}

	// Make sure sky shaders have a cloudHeight value
	if (shader->shaderType == SHADER_SKY){
		if (!(shader->flags & SHADER_SKYPARMS)){
			shader->flags |= SHADER_SKYPARMS;
			shader->skyParms.cloudHeight = 128.0;
		}
	}

	// Remove deformVertexes from 2D shaders
	if (shader->shaderType == SHADER_NOMIP){
		if (shader->flags & SHADER_DEFORMVERTEXES){
			shader->flags &= ~SHADER_DEFORMVERTEXES;

			for (i = 0; i < shader->deformVertexesNum; i++)
				memset(&shader->deformVertexes[i], 0, sizeof(deformVertexes_t));

			shader->deformVertexesNum = 0;
		}
	}

	// Check keywords that reference the current entity
	if (shader->flags & SHADER_DEFORMVERTEXES){
		for (i = 0; i < shader->deformVertexesNum; i++){
			switch (shader->deformVertexes[i].type){
			case DEFORMVERTEXES_AUTOSPRITE2:
				shader->flags &= ~SHADER_ENTITYMERGABLE;
				break;
			}
		}
	}

	// Lightmap but no lightmap stage?
	if (shader->shaderType == SHADER_BSP && (shader->surfaceParm & SURFACEPARM_LIGHTMAP)){
		if (!(shader->flags & SHADER_DEFAULTED) && !(shader->flags & SHADER_HASLIGHTMAP))
			Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader->name);
	}

	// Check stages
	for (i = 0; i < shader->numStages; i++){
		stage = shader->stages[i];

		// Check bundles
		for (j = 0; j < stage->numBundles; j++){
			bundle = stage->bundles[j];

			// Make sure it has a texture
			if (bundle->texType == TEX_GENERIC && !bundle->numTextures){
				if (j == 0)
					Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' has a stage with no texture!\n", shader->name);
				else
					Com_Printf(S_COLOR_YELLOW "WARNING: shader '%s' has a bundle with no texture!\n", shader->name);

				bundle->textures[bundle->numTextures++] = r_defaultTexture;
			}

			// Set tcGen if unset
			if (!(bundle->flags & STAGEBUNDLE_TCGEN)){
				if (bundle->texType == TEX_LIGHTMAP)
					bundle->tcGen.type = TCGEN_LIGHTMAP;
				else
					bundle->tcGen.type = TCGEN_BASE;

				bundle->flags |= STAGEBUNDLE_TCGEN;
			}
			else {
				// Only allow tcGen lightmap on world surfaces
				if (bundle->tcGen.type == TCGEN_LIGHTMAP){
					if (shader->shaderType != SHADER_BSP)
						bundle->tcGen.type = TCGEN_BASE;
				}
			}

			// Check keywords that reference the current entity
			if (bundle->flags & STAGEBUNDLE_TCGEN){
				switch (bundle->tcGen.type){
				case TCGEN_ENVIRONMENT:
				case TCGEN_LIGHTVECTOR:
				case TCGEN_HALFANGLE:
					if (shader->shaderType == SHADER_NOMIP)
						bundle->tcGen.type = TCGEN_BASE;

					shader->flags &= ~SHADER_ENTITYMERGABLE;
					break;
				}
			}
		}

		// blendFunc GL_ONE GL_ZERO is non-blended
		if (stage->blendFunc.src == GL_ONE && stage->blendFunc.dst == GL_ZERO){
			stage->flags &= ~SHADERSTAGE_BLENDFUNC;
			stage->blendFunc.src = 0;
			stage->blendFunc.dst = 0;
		}

		// Set depthFunc if unset
		if (!(stage->flags & SHADERSTAGE_DEPTHFUNC)){
			stage->flags |= SHADERSTAGE_DEPTHFUNC;
			stage->depthFunc.func = GL_LEQUAL;
		}

		// Remove depthFunc from 2D shaders
		if (shader->shaderType == SHADER_NOMIP){
			stage->flags &= ~SHADERSTAGE_DEPTHFUNC;
			stage->depthFunc.func = 0;
		}

		// Set depthWrite for non-blended stages
		if (!(stage->flags & SHADERSTAGE_BLENDFUNC))
			stage->flags |= SHADERSTAGE_DEPTHWRITE;

		// Remove depthWrite from sky and 2D shaders
		if (shader->shaderType == SHADER_SKY || shader->shaderType == SHADER_NOMIP)
			stage->flags &= ~SHADERSTAGE_DEPTHWRITE;

		// Ignore detail stages if detail textures are disabled
		if (stage->flags & SHADERSTAGE_DETAIL){
			if (!r_detailTextures->integer)
				stage->ignore = true;
		}

		// Set rgbGen if unset
		if (!(stage->flags & SHADERSTAGE_RGBGEN)){
			switch (shader->shaderType){
			case SHADER_SKY:
				stage->rgbGen.type = RGBGEN_IDENTITY;
				break;
			case SHADER_BSP:
				if ((stage->flags & SHADERSTAGE_BLENDFUNC) && (stage->bundles[0]->texType != TEX_LIGHTMAP))
					stage->rgbGen.type = RGBGEN_IDENTITYLIGHTING;
				else
					stage->rgbGen.type = RGBGEN_IDENTITY;

				break;
			case SHADER_SKIN:
				stage->rgbGen.type = RGBGEN_LIGHTINGDIFFUSE;
				break;
			case SHADER_NOMIP:
			case SHADER_GENERIC:
				stage->rgbGen.type = RGBGEN_VERTEX;
				break;
			}

			stage->flags |= SHADERSTAGE_RGBGEN;
		}

		// Set alphaGen if unset
		if (!(stage->flags & SHADERSTAGE_ALPHAGEN)){
			switch (shader->shaderType){
			case SHADER_SKY:
				stage->alphaGen.type = ALPHAGEN_IDENTITY;
				break;
			case SHADER_BSP:
				if ((stage->flags & SHADERSTAGE_BLENDFUNC) && (stage->bundles[0]->texType != TEX_LIGHTMAP)){
					if (shader->surfaceParm & (SURFACEPARM_TRANS33 | SURFACEPARM_TRANS66))
						stage->alphaGen.type = ALPHAGEN_VERTEX;
					else
						stage->alphaGen.type = ALPHAGEN_IDENTITY;
				}
				else
					stage->alphaGen.type = ALPHAGEN_IDENTITY;

				break;
			case SHADER_SKIN:
				stage->alphaGen.type = ALPHAGEN_IDENTITY;
				break;
			case SHADER_NOMIP:
			case SHADER_GENERIC:
				stage->alphaGen.type = ALPHAGEN_VERTEX;
				break;
			}

			stage->flags |= SHADERSTAGE_ALPHAGEN;
		}

		// Check keywords that reference the current entity
		if (stage->flags & SHADERSTAGE_RGBGEN){
			switch (stage->rgbGen.type){
			case RGBGEN_ENTITY:
			case RGBGEN_ONEMINUSENTITY:
			case RGBGEN_LIGHTINGAMBIENT:
			case RGBGEN_LIGHTINGDIFFUSE:
				if (shader->shaderType == SHADER_NOMIP)
					stage->rgbGen.type = RGBGEN_VERTEX;

				shader->flags &= ~SHADER_ENTITYMERGABLE;
				break;
			}
		}

		if (stage->flags & SHADERSTAGE_ALPHAGEN){
			switch (stage->alphaGen.type){
			case ALPHAGEN_ENTITY:
			case ALPHAGEN_ONEMINUSENTITY:
			case ALPHAGEN_DOT:
			case ALPHAGEN_ONEMINUSDOT:
			case ALPHAGEN_FADE:
			case ALPHAGEN_ONEMINUSFADE:
			case ALPHAGEN_LIGHTINGSPECULAR:
				if (shader->shaderType == SHADER_NOMIP)
					stage->alphaGen.type = ALPHAGEN_VERTEX;

				shader->flags &= ~SHADER_ENTITYMERGABLE;
				break;
			}
		}

		// Set texEnv for first bundle
		if (!(stage->bundles[0]->flags & STAGEBUNDLE_TEXENVCOMBINE)){
			if (!(stage->flags & SHADERSTAGE_BLENDFUNC)){
				if (stage->rgbGen.type == RGBGEN_IDENTITY && stage->alphaGen.type == ALPHAGEN_IDENTITY)
					stage->bundles[0]->texEnv = GL_REPLACE;
				else
					stage->bundles[0]->texEnv = GL_MODULATE;
			}
			else {
				if ((stage->blendFunc.src == GL_DST_COLOR && stage->blendFunc.dst == GL_ZERO) || (stage->blendFunc.src == GL_ZERO && stage->blendFunc.dst == GL_SRC_COLOR))
					stage->bundles[0]->texEnv = GL_MODULATE;
				else if (stage->blendFunc.src == GL_ONE && stage->blendFunc.dst == GL_ONE)
					stage->bundles[0]->texEnv = GL_ADD;
			}
		}

		// If this stage is ignored, make sure we stop cinematics
		if (stage->ignore){
			for (j = 0; j < stage->numBundles; j++){
				bundle = stage->bundles[j];

				if (bundle->flags & STAGEBUNDLE_VIDEOMAP)
					CIN_StopCinematic(bundle->cinematicHandle);
			}
		}
	}

	// Set sort if unset
	if (!(shader->flags & SHADER_SORT)){
		if (shader->shaderType == SHADER_SKY)
			shader->sort = SORT_SKY;
		else {
			stage = shader->stages[0];

			if (!(stage->flags & SHADERSTAGE_BLENDFUNC))
				shader->sort = SORT_OPAQUE;
			else {
				if (shader->flags & SHADER_POLYGONOFFSET)
					shader->sort = SORT_DECAL;
				else if (stage->flags & SHADERSTAGE_ALPHAFUNC)
					shader->sort = SORT_SEETHROUGH;
				else {
					if ((stage->blendFunc.src == GL_SRC_ALPHA && stage->blendFunc.dst == GL_ONE) || (stage->blendFunc.src == GL_ONE && stage->blendFunc.dst == GL_ONE))
						shader->sort = SORT_ADDITIVE;
					else
						shader->sort = SORT_BLEND;
				}
			}
		}
	}
}

/*
 =================
 R_MergeShaderStages
 =================
*/
static qboolean R_MergeShaderStages (shaderStage_t *stage1, shaderStage_t *stage2){

	if (!glConfig.multitexture)
		return false;

	if (stage1->numBundles == glConfig.maxTextureUnits || stage1->numBundles == MAX_TEXTURE_UNITS)
		return false;

	if (stage1->flags & SHADERSTAGE_NEXTBUNDLE || stage2->flags & SHADERSTAGE_NEXTBUNDLE)
		return false;

	if (stage1->flags & (SHADERSTAGE_VERTEXPROGRAM | SHADERSTAGE_FRAGMENTPROGRAM) || stage2->flags & (SHADERSTAGE_VERTEXPROGRAM | SHADERSTAGE_FRAGMENTPROGRAM))
		return false;

	if (stage2->flags & SHADERSTAGE_ALPHAFUNC)
		return false;

	if (stage1->flags & SHADERSTAGE_ALPHAFUNC || stage1->depthFunc.func == GL_EQUAL){
		if (stage2->depthFunc.func != GL_EQUAL)
			return false;
	}

	if (!(stage1->flags & SHADERSTAGE_DEPTHWRITE)){
		if (stage2->flags & SHADERSTAGE_DEPTHWRITE)
			return false;
	}

	if (stage2->rgbGen.type != RGBGEN_IDENTITY || stage2->alphaGen.type != ALPHAGEN_IDENTITY)
		return false;

	switch (stage1->bundles[0]->texEnv){
	case GL_REPLACE:
		if (stage2->bundles[0]->texEnv == GL_REPLACE)
			return true;
		if (stage2->bundles[0]->texEnv == GL_MODULATE)
			return true;
		if (stage2->bundles[0]->texEnv == GL_ADD && glConfig.textureEnvAdd)
			return true;

		break;
	case GL_MODULATE:
		if (stage2->bundles[0]->texEnv == GL_REPLACE)
			return true;
		if (stage2->bundles[0]->texEnv == GL_MODULATE)
			return true;

		break;
	case GL_ADD:
		if (stage2->bundles[0]->texEnv == GL_ADD && glConfig.textureEnvAdd)
			return true;

		break;
	}

	return false;
}

/*
 =================
 R_OptimizeShader
 =================
*/
static void R_OptimizeShader (shader_t *shader){

	shaderStage_t	*curStage, *prevStage = NULL;
	int				i;

	// Try to merge multiple stages for multitexturing
	for (i = 0; i < shader->numStages; i++){
		curStage = shader->stages[i];

		if (curStage->ignore)
			continue;

		if (!prevStage){
			// No previous stage to merge with
			prevStage = curStage;
			continue;
		}

		if (!R_MergeShaderStages(prevStage, curStage)){
			// Couldn't merge the stages.
			prevStage = curStage;
			continue;
		}

		// Merge with previous stage and ignore it
		memcpy(prevStage->bundles[prevStage->numBundles++], curStage->bundles[0], sizeof(stageBundle_t));

		curStage->ignore = true;
	}

	// Make sure texEnv is valid (left-over from R_FinishShader)
	for (i = 0; i < shader->numStages; i++){
		if (shader->stages[i]->ignore)
			continue;

		if (shader->stages[i]->bundles[0]->flags & STAGEBUNDLE_TEXENVCOMBINE)
			continue;

		if (shader->stages[i]->bundles[0]->texEnv != GL_REPLACE && shader->stages[i]->bundles[0]->texEnv != GL_MODULATE)
			shader->stages[i]->bundles[0]->texEnv = GL_MODULATE;
	}
}

/*
 =================
 R_LoadShader
 =================
*/
shader_t *R_LoadShader (shader_t *newShader){

	shader_t	*shader;
	unsigned	hashKey;
	int			i, j;

	if (r_numShaders == MAX_SHADERS)
		Com_Error(ERR_DROP, "R_LoadShader: MAX_SHADERS hit");

	r_shaders[r_numShaders++] = shader = Hunk_Alloc(sizeof(shader_t));

	// Make sure the shader is valid and set all the unset parameters
	R_FinishShader(newShader);

	// Try to merge multiple stages for multitexturing
	R_OptimizeShader(newShader);

	// Copy the shader
	memcpy(shader, newShader, sizeof(shader_t));
	shader->numStages = 0;

	// Allocate and copy the stages
	for (i = 0; i < newShader->numStages; i++){
		if (newShader->stages[i]->ignore)
			continue;

		shader->stages[shader->numStages] = Hunk_Alloc(sizeof(shaderStage_t));
		memcpy(shader->stages[shader->numStages], newShader->stages[i], sizeof(shaderStage_t));

		// Allocate and copy the bundles
		for (j = 0; j < shader->stages[shader->numStages]->numBundles; j++){
			shader->stages[shader->numStages]->bundles[j] = Hunk_Alloc(sizeof(stageBundle_t));
			memcpy(shader->stages[shader->numStages]->bundles[j], newShader->stages[i]->bundles[j], sizeof(stageBundle_t));
		}

		shader->numStages++;
	}

	// Add to hash table
	hashKey = Com_HashKey(shader->name, SHADERS_HASHSIZE);

	shader->nextHash = r_shadersHash[hashKey];
	r_shadersHash[hashKey] = shader;

	return shader;
}

/*
 =================
 R_FindShader
 =================
*/
shader_t *R_FindShader (const char *name, shaderType_t shaderType, unsigned surfaceParm){

	shader_t		*shader;
	shaderScript_t	*shaderScript;
	char			*script = NULL;
	unsigned		hashKey;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindShader: NULL shader name");

	if (strlen(name) >= MAX_QPATH)
		Com_Error(ERR_DROP, "R_FindShader: shader name exceeds MAX_QPATH");

	// See if already loaded
	hashKey = Com_HashKey(name, SHADERS_HASHSIZE);

	for (shader = r_shadersHash[hashKey]; shader; shader = shader->nextHash){
		if (!Q_stricmp(shader->name, name)){
			if ((shader->shaderType == shaderType) && (shader->surfaceParm == surfaceParm))
				return shader;
		}
	}

	// See if there's a script for this shader
	for (shaderScript = r_shaderScriptsHash[hashKey]; shaderScript; shaderScript = shaderScript->nextHash){
		if (!Q_stricmp(shaderScript->name, name)){
			if ((shaderScript->shaderType == shaderType && shaderScript->surfaceParm == surfaceParm) || (shaderScript->shaderType == -1)){
				script = shaderScript->script;
				break;
			}
		}
	}

	// Create the shader
	if (script)
		shader = R_CreateShader(name, shaderType, surfaceParm, script);
	else
		shader = R_CreateDefaultShader(name, shaderType, surfaceParm);

	// Load it in
	return R_LoadShader(shader);
}

/*
 =================
 R_RegisterShader
 =================
*/
shader_t *R_RegisterShader (const char *name){

	return R_FindShader(name, SHADER_GENERIC, 0);
}

/*
 =================
 R_RegisterShaderSkin
 =================
*/
shader_t *R_RegisterShaderSkin (const char *name){

	return R_FindShader(name, SHADER_SKIN, 0);
}

/*
 =================
 R_RegisterShaderNoMip
 =================
*/
shader_t *R_RegisterShaderNoMip (const char *name){

	return R_FindShader(name, SHADER_NOMIP, 0);
}

/*
 =================
 R_CreateBuiltInShaders
 =================
*/
static void R_CreateBuiltInShaders (void){

	shader_t	*shader;

	// Default shader
	shader = R_NewShader();

	Q_strncpyz(shader->name, "<default>", sizeof(shader->name));
	shader->shaderNum = r_numShaders;
	shader->shaderType = SHADER_BSP;
	shader->surfaceParm = 0;
	shader->stages[0]->bundles[0]->textures[0] = r_defaultTexture;
	shader->stages[0]->bundles[0]->numTextures++;
	shader->stages[0]->numBundles++;
	shader->numStages++;

	r_defaultShader = R_LoadShader(shader);

	// Lightmap shader
	shader = R_NewShader();

	Q_strncpyz(shader->name, "<lightmap>", sizeof(shader->name));
	shader->shaderNum = r_numShaders;
	shader->shaderType = SHADER_BSP;
	shader->surfaceParm = SURFACEPARM_LIGHTMAP;
	shader->flags = SHADER_HASLIGHTMAP;
	shader->stages[0]->bundles[0]->texType = TEX_LIGHTMAP;
	shader->stages[0]->numBundles++;
	shader->numStages++;

	r_lightmapShader = R_LoadShader(shader);
}

/*
 =================
 R_ShaderList_f
 =================
*/
void R_ShaderList_f (void){

	shader_t		*shader;
	int				i, j;
	int				passes;

	Com_Printf("\n");
	Com_Printf("-----------------------------------\n");

	for (i = 0; i < r_numShaders; i++){
		shader = r_shaders[i];

		passes = 0;
		for (j = 0; j < shader->numStages; j++)
			passes += shader->stages[j]->numBundles;

		Com_Printf("%i/%i ", passes, shader->numStages);

		if (shader->flags & SHADER_EXTERNAL)
			Com_Printf("E ");
		else
			Com_Printf("  ");

		switch (shader->shaderType){
		case SHADER_SKY:
			Com_Printf("sky ");
			break;
		case SHADER_BSP:
			Com_Printf("bsp ");
			break;
		case SHADER_SKIN:
			Com_Printf("skn ");
			break;
		case SHADER_NOMIP:
			Com_Printf("pic ");
			break;
		case SHADER_GENERIC:
			Com_Printf("gen ");
			break;
		}

		if (shader->surfaceParm)
			Com_Printf("%02X ", shader->surfaceParm);
		else
			Com_Printf("   ");

		Com_Printf("%2i ", shader->sort);

		Com_Printf(": %s%s\n", shader->name, (shader->flags & SHADER_DEFAULTED) ? " (DEFAULTED)" : "");
	}

	Com_Printf("-----------------------------------\n");
	Com_Printf("%i total shaders\n", r_numShaders);
	Com_Printf("\n");
}

/*
 =================
 R_InitShaders
 =================
*/
void R_InitShaders (void){

	char	dirFiles[0x10000], *dirPtr;
	int		dirNum, dirLen, i;
	char	name[MAX_QPATH];
	char	*buffer;
	int		size;

	Com_Printf("Initializing Shaders\n");

	// Find .shader files
	dirNum = FS_GetFileList("scripts/shaders", "shader", dirFiles, sizeof(dirFiles));
	if (!dirNum)
		Com_Printf(S_COLOR_YELLOW "WARNING: no shader files found!\n");

	// Load them
	for (i = 0, dirPtr = dirFiles; i < dirNum; i++, dirPtr += dirLen){
		dirLen = strlen(dirPtr) + 1;

		Q_snprintfz(name, sizeof(name), "scripts/shaders/%s", dirPtr);
		Com_Printf("...loading '%s'\n", name);

		size = FS_LoadFile(name, (void **)&buffer);
		if (!buffer){
			Com_Printf(S_COLOR_RED "Couldn't load '%s'\n", name);
			continue;
		}

		// Parse this file
		R_ParseShaderFile(buffer, size);

		FS_FreeFile(buffer);
	}

	// Create built-in shaders
	R_CreateBuiltInShaders();
}

/*
 =================
 R_ShutdownShaders
 =================
*/
void R_ShutdownShaders (void){

	shader_t		*shader;
	shaderStage_t	*stage;
	stageBundle_t	*bundle;
	int				i, j, k;

	for (i = 0; i < r_numShaders; i++){
		shader = r_shaders[i];

		for (j = 0; j < shader->numStages; j++){
			stage = shader->stages[j];

			for (k = 0; k < stage->numBundles; k++){
				bundle = stage->bundles[k];

				if (bundle->flags & STAGEBUNDLE_VIDEOMAP)
					CIN_StopCinematic(bundle->cinematicHandle);
			}
		}
	}

	memset(r_shaderScriptsHash, 0, sizeof(r_shaderScriptsHash));
	memset(r_shadersHash, 0, sizeof(r_shadersHash));
	memset(r_shaders, 0, sizeof(r_shaders));

	r_numShaders = 0;
}
