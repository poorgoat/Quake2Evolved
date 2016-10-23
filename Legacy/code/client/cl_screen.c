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


// cl_scrn.c -- master for refresh, status bar, console, etc...


#include "client.h"


/*
 =======================================================================

 STATUS BAR, LAYOUT, INVENTORY

 =======================================================================
*/


#define STAT_MINUS			10	// Num frame for '-' stats digit
#define	CHAR_WIDTH			16

#define HUD_MAX_ITEMDEFS	128
#define HUD_MAX_SKIPIFSTATS	8
#define HUD_MAX_DRAWFUNCS	16

#define HUD_STATSTRING		1
#define HUD_STATNUMBER		2
#define HUD_STATVALUE		4
#define HUD_STATSHADER		8

typedef enum {
	HUD_GREATER,
	HUD_LESS,
	HUD_GEQUAL,
	HUD_LEQUAL,
	HUD_EQUAL,
	HUD_NOTEQUAL
} cmpFunc_t;

typedef enum {
	HUD_DRAWPIC,
	HUD_DRAWSTRING,
	HUD_DRAWNUMBER,
	HUD_DRAWBAR
} drawType_t;

typedef enum {
	HUD_LEFTTORIGHT,
	HUD_RIGHTTOLEFT,
	HUD_BOTTOMTOTOP,
	HUD_TOPTOBOTTOM
} fillDir_t;

typedef struct {
	int				stat;
	cmpFunc_t		func;
	int				value;
} skipIfStat_t;

typedef struct {
	drawType_t		type;
	int				rect[4];
	int				size[2];
	int				range[2];
	int				offset[2];
	fillDir_t		fillDir;
	color_t			color;
	color_t			flashColor;
	int				flags;
	int				fromStat;
	char			string[64];
	int				number;
	int				value;
	struct shader_s	*shader;
	struct shader_s	*shaderMinus;
	int				stringStat;
	int				numberStat;
	int				valueStat;
	int				shaderStat;
} drawFunc_t;

typedef struct {
	qboolean		onlyMultiPlayer;

	skipIfStat_t	skipIfStats[HUD_MAX_SKIPIFSTATS];
	int				numSkipIfStats;

	drawFunc_t		drawFuncs[HUD_MAX_DRAWFUNCS];
	int				numDrawFuncs;
} itemDef_t;

typedef struct {
	itemDef_t		itemDefs[HUD_MAX_ITEMDEFS];
	int				numItemDefs;
} hudDef_t;

static hudDef_t		cl_hudDef;


/*
 =================
 CL_ParseHUDDrawFunc
 =================
*/
static qboolean CL_ParseHUDDrawFunc (itemDef_t *itemDef, const char *name, char **script){

	drawFunc_t	*drawFunc;
	char		*tok;
	int			i;

	if (itemDef->numDrawFuncs == HUD_MAX_DRAWFUNCS){
		Com_Printf(S_COLOR_YELLOW "WARNING: HUD_MAX_DRAWFUNCS hit in HUD script\n");
		return false;
	}
	drawFunc = &itemDef->drawFuncs[itemDef->numDrawFuncs++];

	if (!Q_stricmp(name, "drawPic"))
		drawFunc->type = HUD_DRAWPIC;
	else if (!Q_stricmp(name, "drawString"))
		drawFunc->type = HUD_DRAWSTRING;
	else if (!Q_stricmp(name, "drawNumber"))
		drawFunc->type = HUD_DRAWNUMBER;
	else if (!Q_stricmp(name, "drawBar"))
		drawFunc->type = HUD_DRAWBAR;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown draw function '%s' in HUD script\n", name);
		return false;
	}

	tok = Com_ParseExt(script, true);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
		return false;
	}

	if (!Q_stricmp(tok, "{")){
		while (1){
			tok = Com_ParseExt(script, true);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in '%s' in HUD script\n", name);
				return false;
			}

			if (!Q_stricmp(tok, "}"))
				break;

			if (!Q_stricmp(tok, "rect")){
				for (i = 0; i < 4; i++){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->rect[i] = atoi(tok);
				}
			}
			else if (!Q_stricmp(tok, "size")){
				for (i = 0; i < 2; i++){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->size[i] = atoi(tok);
				}
			}
			else if (!Q_stricmp(tok, "range")){
				for (i = 0; i < 2; i++){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->range[i] = atoi(tok);
				}
			}
			else if (!Q_stricmp(tok, "offset")){
				for (i = 0; i < 2; i++){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->offset[i] = atoi(tok);
				}
			}
			else if (!Q_stricmp(tok, "fillDir")){
				tok = Com_ParseExt(script, false);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				if (!Q_stricmp(tok, "leftToRight"))
					drawFunc->fillDir = HUD_LEFTTORIGHT;
				else if (!Q_stricmp(tok, "rightToLeft"))
					drawFunc->fillDir = HUD_RIGHTTOLEFT;
				else if (!Q_stricmp(tok, "bottomToTop"))
					drawFunc->fillDir = HUD_BOTTOMTOTOP;
				else if (!Q_stricmp(tok, "topToBottom"))
					drawFunc->fillDir = HUD_TOPTOBOTTOM;
				else {
					Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' for '%s' in HUD script\n", tok, name);
					return false;
				}
			}
			else if (!Q_stricmp(tok, "color")){
				for (i = 0; i < 4; i++){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->color[i] = drawFunc->flashColor[i] = 255 * Clamp(atof(tok), 0.0, 1.0);
				}

				tok = Com_ParseExt(script, false);
				if (tok[0]){
					if (Q_stricmp(tok, "flash")){
						Com_Printf(S_COLOR_YELLOW "WARNING: expected 'flash', found '%s' instead in '%s' in HUD script\n", tok, name);
						return false;
					}

					for (i = 0; i < 4; i++){
						tok = Com_ParseExt(script, false);
						if (!tok[0]){
							Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
							return false;
						}

						drawFunc->flashColor[i] = 255 * Clamp(atof(tok), 0.0, 1.0);
					}
				}
			}
			else if (!Q_stricmp(tok, "flags")){
				while (1){
					tok = Com_ParseExt(script, false);
					if (!tok[0])
						break;

					if (!Q_stricmp(tok, "left"))
						drawFunc->flags |= DSF_LEFT;
					else if (!Q_stricmp(tok, "center"))
						drawFunc->flags |= DSF_CENTER;
					else if (!Q_stricmp(tok, "right"))
						drawFunc->flags |= DSF_RIGHT;
					else if (!Q_stricmp(tok, "lower"))
						drawFunc->flags |= DSF_LOWERCASE;
					else if (!Q_stricmp(tok, "upper"))
						drawFunc->flags |= DSF_UPPERCASE;
					else if (!Q_stricmp(tok, "shadow"))
						drawFunc->flags |= DSF_DROPSHADOW;
					else {
						Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' for '%s' in HUD script\n", tok, name);
						return false;
					}
				}
			}
			else if (!Q_stricmp(tok, "string")){
				tok = Com_ParseExt(script, false);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				if (!Q_stricmp(tok, "fromStat")){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->stringStat = Clamp(atoi(tok), 0, 31);

					drawFunc->fromStat |= HUD_STATSTRING;
				}
				else
					Q_strncpyz(drawFunc->string, tok, sizeof(drawFunc->string));
			}
			else if (!Q_stricmp(tok, "number")){
				tok = Com_ParseExt(script, false);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				if (!Q_stricmp(tok, "fromStat")){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->numberStat = Clamp(atoi(tok), 0, 31);

					drawFunc->fromStat |= HUD_STATNUMBER;
				}
				else
					drawFunc->number = atoi(tok);
			}
			else if (!Q_stricmp(tok, "value")){
				tok = Com_ParseExt(script, false);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				if (!Q_stricmp(tok, "fromStat")){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->valueStat = Clamp(atoi(tok), 0, 31);

					drawFunc->fromStat |= HUD_STATVALUE;
				}
				else
					drawFunc->value = atoi(tok);
			}
			else if (!Q_stricmp(tok, "shader")){
				tok = Com_ParseExt(script, false);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				if (!Q_stricmp(tok, "fromStat")){
					tok = Com_ParseExt(script, false);
					if (!tok[0]){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->shaderStat = Clamp(atoi(tok), 0, 31);

					drawFunc->fromStat |= HUD_STATSHADER;
				}
				else
					drawFunc->shader = R_RegisterShaderNoMip(tok);
			}
			else if (!Q_stricmp(tok, "minus")){
				tok = Com_ParseExt(script, false);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				drawFunc->shaderMinus = R_RegisterShaderNoMip(tok);
			}
			else {
				Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' for '%s' in HUD script\n", tok, name);
				return false;
			}
		}
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in '%s' in HUD script\n", tok, name);
		return false;
	}

	return true;
}

/*
 =================
 CL_ParseHUDSkipIfStat
 =================
*/
static qboolean CL_ParseHUDSkipIfStat (itemDef_t *itemDef, char **script){

	skipIfStat_t	*skipIfStat;
	char			*tok;

	if (itemDef->numSkipIfStats == HUD_MAX_SKIPIFSTATS){
		Com_Printf(S_COLOR_YELLOW "WARNING: HUD_MAX_SKIPIFSTATS hit in HUD script");
		return false;
	}
	skipIfStat = &itemDef->skipIfStats[itemDef->numSkipIfStats++];

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skipIfStat' in HUD script\n");
		return false;
	}

	skipIfStat->stat = Clamp(atoi(tok), 0, 31);

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skipIfStat' in HUD script\n");
		return false;
	}

	if (!Q_stricmp(tok, ">"))
		skipIfStat->func = HUD_GREATER;
	else if (!Q_stricmp(tok, "<"))
		skipIfStat->func = HUD_LESS;
	else if (!Q_stricmp(tok, ">="))
		skipIfStat->func = HUD_GEQUAL;
	else if (!Q_stricmp(tok, "<="))
		skipIfStat->func = HUD_LESS;
	else if (!Q_stricmp(tok, "=="))
		skipIfStat->func = HUD_EQUAL;
	else if (!Q_stricmp(tok, "!="))
		skipIfStat->func = HUD_NOTEQUAL;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' for 'skipIfStat' in HUD script\n", tok);
		return false;
	}

	tok = Com_ParseExt(script, false);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skipIfStat' in HUD script\n");
		return false;
	}

	skipIfStat->value = atoi(tok);

	return true;
}

/*
 =================
 CL_ParseHUDItemDef
 =================
*/
static qboolean CL_ParseHUDItemDef (hudDef_t *hudDef, char **script){

	itemDef_t	*itemDef;
	char		*tok;

	if (hudDef->numItemDefs == HUD_MAX_ITEMDEFS){
		Com_Printf(S_COLOR_YELLOW "WARNING: HUD_MAX_ITEMDEFS hit in HUD script\n");
		return false;
	}
	itemDef = &hudDef->itemDefs[hudDef->numItemDefs++];

	tok = Com_ParseExt(script, true);
	if (!tok[0]){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'itemDef' in HUD script\n");
		return false;
	}

	if (!Q_stricmp(tok, "{")){
		while (1){
			tok = Com_ParseExt(script, true);
			if (!tok[0]){
				Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in 'itemDef' in HUD script\n'");
				return false;
			}

			if (!Q_stricmp(tok, "}"))
				break;

			if (!Q_stricmp(tok, "onlyMultiPlayer"))
				itemDef->onlyMultiPlayer = true;
			else if (!Q_stricmp(tok, "skipIfStat")){
				if (!CL_ParseHUDSkipIfStat(itemDef, script))
					return false;
			}
			else {
				if (!CL_ParseHUDDrawFunc(itemDef, tok, script))
					return false;
			}
		}
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in 'itemDef' in HUD script\n", tok);
		return false;
	}

	return true;
}

/*
 =================
 CL_ParseHUD
 =================
*/
static void CL_ParseHUD (char *buffer){

	char	*buf, *tok;

	buf = buffer;
	while (1){
		tok = Com_ParseExt(&buf, true);
		if (!tok[0])
			break;

		if (!Q_stricmp(tok, "hudDef")){
			tok = Com_ParseExt(&buf, true);
			if (Q_stricmp(tok, "{")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in HUD script\n", tok);
				return;
			}

			while (1){
				tok = Com_ParseExt(&buf, true);
				if (!tok[0]){
					Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in 'hudDef' in HUD script\n");
					return;
				}

				if (!Q_stricmp(tok, "}"))
					break;

				if (!Q_stricmp(tok, "itemDef")){
					if (!CL_ParseHUDItemDef(&cl_hudDef, &buf))
						return;
				}
				else {
					Com_Printf(S_COLOR_YELLOW "WARNING: unknown command '%s' in HUD script\n", tok);
					return;
				}
			}
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown command '%s' in HUD script\n", tok);
			return;
		}
	}
}

/*
 =================
 CL_LoadHUD
 =================
*/
void CL_LoadHUD (void){

	char	*buffer;

	memset(&cl_hudDef, 0, sizeof(cl_hudDef));

	// Don't use new HUD with mods
	if (cl.gameMod)
		return;

	// Load the file
	FS_LoadFile("scripts/misc/hud.txt", (void **)&buffer);
	if (!buffer)
		return;

	// Parse it
	CL_ParseHUD(buffer);

	FS_FreeFile(buffer);
}

/*
 =================
 CL_DrawHUDPic
 =================
*/
static void CL_DrawHUDPic (drawFunc_t *drawFunc){

	byte			*color;
	struct shader_s	*shader;
	int				index;

	if ((cl.frame.serverFrame>>2) & 1)
		color = drawFunc->flashColor;
	else
		color = drawFunc->color;

	if (drawFunc->fromStat & HUD_STATSHADER){
		index = cl.playerState->stats[drawFunc->shaderStat];
		if (index < 0 || index >= MAX_IMAGES)
			Com_Error(ERR_DROP, "CL_DrawHUDPic: bad pic index %i", index);

		shader = R_RegisterShaderNoMip(va("pics/newhud/%s", cl.configStrings[CS_IMAGES+index]));
	}
	else
		shader = drawFunc->shader;

	if (drawFunc->offset[0] || drawFunc->offset[1])
		CL_DrawPicOffset(drawFunc->rect[0], drawFunc->rect[1], drawFunc->rect[2], drawFunc->rect[3], drawFunc->offset[0], drawFunc->offset[1], color, shader);
	else
		CL_DrawPic(drawFunc->rect[0], drawFunc->rect[1], drawFunc->rect[2], drawFunc->rect[3], color, shader);
}

/*
 =================
 CL_DrawHUDString
 =================
*/
static void CL_DrawHUDString (drawFunc_t *drawFunc){

	byte			*color;
	char			*string;
	struct shader_s	*shader;
	int				index;

	if ((cl.frame.serverFrame>>2) & 1)
		color = drawFunc->flashColor;
	else
		color = drawFunc->color;

	if (drawFunc->fromStat & HUD_STATSTRING){
		index = cl.playerState->stats[drawFunc->stringStat];
		if (index < 0 || index >= MAX_CONFIGSTRINGS)
			Com_Error(ERR_DROP, "CL_DrawHUDString: bad string index %i", index);

		string = cl.configStrings[index];
	}
	else
		string = drawFunc->string;

	if (drawFunc->fromStat & HUD_STATSHADER){
		index = cl.playerState->stats[drawFunc->shaderStat];
		if (index < 0 || index >= MAX_IMAGES)
			Com_Error(ERR_DROP, "CL_DrawHUDString: bad pic index %i", index);

		shader = R_RegisterShaderNoMip(va("pics/newhud/%s", cl.configStrings[CS_IMAGES+index]));
	}
	else
		shader = drawFunc->shader;

	CL_DrawString(drawFunc->rect[0], drawFunc->rect[1], drawFunc->size[0], drawFunc->size[1], drawFunc->offset[0], drawFunc->offset[1], drawFunc->rect[2], string, color, shader, true, drawFunc->flags);
}

/*
 =================
 CL_DrawHUDNumber
 =================
*/
static void CL_DrawHUDNumber (drawFunc_t *drawFunc){

	byte			*color;
	int				number;
	struct shader_s	*shader;
	int				index;
	char			num[16], *ptr;
	int				width, x, y;
	int				l, frame;

	if ((cl.frame.serverFrame>>2) & 1)
		color = drawFunc->flashColor;
	else
		color = drawFunc->color;

	if (drawFunc->fromStat & HUD_STATNUMBER)
		number = cl.playerState->stats[drawFunc->numberStat];
	else
		number = drawFunc->number;

	if (drawFunc->fromStat & HUD_STATSHADER){
		index = cl.playerState->stats[drawFunc->shaderStat];
		if (index < 0 || index >= MAX_IMAGES)
			Com_Error(ERR_DROP, "CL_DrawHUDNumber: bad pic index %i", index);

		shader = R_RegisterShaderNoMip(va("pics/newhud/%s", cl.configStrings[CS_IMAGES+index]));
	}
	else
		shader = drawFunc->shader;

	if (drawFunc->size[0])
		width = drawFunc->rect[2] / drawFunc->size[0];
	else
		width = 3;

	if (width < 1)
		return;

	if (width > 5)
		width = 5;

	switch (width){
	case 1:
		number = Clamp(number, 0, 9);
		break;
	case 2:
		number = Clamp(number, -9, 99);
		break;
	case 3:
		number = Clamp(number, -99, 999);
		break;
	case 4:
		number = Clamp(number, -999, 9999);
		break;
	case 5:
		number = Clamp(number, -9999, 99999);
		break;
	}

	Q_snprintfz(num, sizeof(num), "%i", number);
	l = strlen(num);
	if (l > width)
		l = width;

	x = drawFunc->rect[0] + ((width - l) * drawFunc->size[0]);
	y = drawFunc->rect[1] + ((drawFunc->rect[3] - drawFunc->size[1]) / 2);

	ptr = num;
	while (*ptr && l){
		if (*ptr == '-'){
			if (drawFunc->offset[0] || drawFunc->offset[1])
				CL_DrawPicOffset(x, y, drawFunc->size[0], drawFunc->size[1], drawFunc->offset[0], drawFunc->offset[1], color, drawFunc->shaderMinus);
			else
				CL_DrawPic(x, y, drawFunc->size[0], drawFunc->size[1], color, drawFunc->shaderMinus);
		}
		else {
			frame = *ptr - '0';

			if (drawFunc->offset[0] || drawFunc->offset[1])
				CL_DrawPicOffsetST(x, y, drawFunc->size[0], drawFunc->size[1], frame * 0.1, 0, (frame * 0.1) + 0.1, 1, drawFunc->offset[0], drawFunc->offset[1], color, shader);
			else
				CL_DrawPicST(x, y, drawFunc->size[0], drawFunc->size[1], frame * 0.1, 0, (frame * 0.1) + 0.1, 1, color, shader);
		}

		x += drawFunc->size[0];
		ptr++;
		l--;
	}
}

/*
 =================
 CL_DrawHUDBar
 =================
*/
static void CL_DrawHUDBar (drawFunc_t *drawFunc){

	byte			*color;
	int				value;
	struct shader_s	*shader;
	int				index;
	float			percent;
	float			x, y, w, h;
	float			sl, tl, sh, th;

	if ((cl.frame.serverFrame>>2) & 1)
		color = drawFunc->flashColor;
	else
		color = drawFunc->color;

	if (drawFunc->fromStat & HUD_STATVALUE)
		value = cl.playerState->stats[drawFunc->valueStat];
	else
		value = drawFunc->value;

	if (drawFunc->fromStat & HUD_STATSHADER){
		index = cl.playerState->stats[drawFunc->shaderStat];
		if (index < 0 || index >= MAX_IMAGES)
			Com_Error(ERR_DROP, "CL_DrawHUDBar: bad pic index %i", index);

		shader = R_RegisterShaderNoMip(va("pics/newhud/%s", cl.configStrings[CS_IMAGES+index]));
	}
	else
		shader = drawFunc->shader;

	value = Clamp(value, drawFunc->range[0], drawFunc->range[1]) - drawFunc->range[0];
	if (drawFunc->range[1] - drawFunc->range[0]){
		percent = (float)value / (drawFunc->range[1] - drawFunc->range[0]);
		if (!percent)
			return;
	}
	else
		return;

	switch (drawFunc->fillDir){
	case HUD_LEFTTORIGHT:
		x = drawFunc->rect[0];
		y = drawFunc->rect[1];
		w = drawFunc->rect[2] * percent;
		h = drawFunc->rect[3];

		sl = 0;
		tl = 0;
		sh = percent;
		th = 1;

		break;
	case HUD_RIGHTTOLEFT:
		x = drawFunc->rect[0] + (drawFunc->rect[2] * (1.0 - percent));
		y = drawFunc->rect[1];
		w = drawFunc->rect[2] * percent;
		h = drawFunc->rect[3];

		sl = 1.0 - percent;
		tl = 0;
		sh = 1;
		th = 1;

		break;
	case HUD_BOTTOMTOTOP:
		x = drawFunc->rect[0];
		y = drawFunc->rect[1] + (drawFunc->rect[3] * (1.0 - percent));
		w = drawFunc->rect[2];
		h = drawFunc->rect[3] * percent;

		sl = 0;
		tl = 1.0 - percent;
		sh = 1;
		th = 1;

		break;
	case HUD_TOPTOBOTTOM:
		x = drawFunc->rect[0];
		y = drawFunc->rect[1];
		w = drawFunc->rect[2];
		h = drawFunc->rect[3] * percent;

		sl = 0;
		tl = 0;
		sh = 1;
		th = percent;

		break;
	}

	if (drawFunc->offset[0] || drawFunc->offset[1])
		CL_DrawPicOffsetST(x, y, w, h, sl, tl, sh, th, drawFunc->offset[0], drawFunc->offset[1], color, shader);
	else
		CL_DrawPicST(x, y, w, h, sl, tl, sh, th, color, shader);
}

/*
 =================
 CL_DrawLayoutString
 =================
*/
static void CL_DrawLayoutString (const char *string, int x, int y, int centerWidth, int xor){

	int		margin;
	char	line[1024];
	int		width;

	margin = x;

	while (*string){
		// Scan out one line of text from the string
		width = 0;
		while (*string && *string != '\n')
			line[width++] = *string++;
		line[width] = 0;

		if (centerWidth)
			x = margin + (centerWidth - width*8) / 2;
		else
			x = margin;

		if (xor)
			CL_DrawString(x, y, 8, 8, 0, 0, 0, line, colorGreen, cls.media.charsetShader, false, DSF_LEFT);
		else
			CL_DrawString(x, y, 8, 8, 0, 0, 0, line, colorWhite, cls.media.charsetShader, false, DSF_LEFT);

		if (*string){
			string++;	// Skip the \n
			y += 8;
		}
	}
}

/*
 =================
 CL_DrawLayoutField
 =================
*/
static void CL_DrawLayoutField (int x, int y, int color, int width, int value){

	char	num[16], *ptr;
	int		l;
	int		frame;

	if (width < 1)
		return;

	// Draw number string
	if (width > 5)
		width = 5;

	Q_snprintfz(num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;

	x += 2 + CHAR_WIDTH * (width - l);
	ptr = num;
	while (*ptr && l){
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr - '0';

		CL_DrawPicFixed(x, y, cl.media.hudNumberShaders[color][frame]);

		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}

/*
 =================
 CL_ExecuteLayoutString
 =================
*/
static void CL_ExecuteLayoutString (char *string){

	int				x, y;
	int				index, value, width, color;
	int				score, ping, time;
	char			block[80];
	char			*token;
	clientInfo_t	*ci;

	if (!string[0])
		return;

	x = 0;
	y = 0;

	while (string){
		token = Com_Parse(&string);

		if (!Q_stricmp(token, "xl")){
			token = Com_Parse(&string);
			x = atoi(token);

			continue;
		}
		if (!Q_stricmp(token, "xr")){
			token = Com_Parse(&string);
			x = cls.glConfig.videoWidth + atoi(token);

			continue;
		}
		if (!Q_stricmp(token, "xv")){
			token = Com_Parse(&string);
			x = cls.glConfig.videoWidth/2 - 160 + atoi(token);

			continue;
		}
		if (!Q_stricmp(token, "yt")){
			token = Com_Parse(&string);
			y = atoi(token);

			continue;
		}
		if (!Q_stricmp(token, "yb")){
			token = Com_Parse(&string);
			y = cls.glConfig.videoHeight + atoi(token);

			continue;
		}
		if (!Q_stricmp(token, "yv")){
			token = Com_Parse(&string);
			y = cls.glConfig.videoHeight/2 - 120 + atoi(token);

			continue;
		}
		if (!Q_stricmp(token, "client")){
			// Draw a deathmatch client block
			token = Com_Parse(&string);
			x = cls.glConfig.videoWidth/2 - 160 + atoi(token);
			token = Com_Parse(&string);
			y = cls.glConfig.videoHeight/2 - 120 + atoi(token);

			token = Com_Parse(&string);
			index = atoi(token);
			if (index < 0 || index >= MAX_CLIENTS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad client index %i", index);

			ci = &cl.clientInfo[index];
			if (!ci->valid)
				ci = &cl.baseClientInfo;

			token = Com_Parse(&string);
			score = atoi(token);
			token = Com_Parse(&string);
			ping = atoi(token);
			token = Com_Parse(&string);
			time = atoi(token);

			CL_DrawPicFixed(x, y, ci->icon);

			CL_DrawString(x+32, y, 8, 8, 0, 0, 0, ci->name, colorGreen, cls.media.charsetShader, false, DSF_LEFT);
			CL_DrawString(x+32, y+8, 8, 8, 0, 0, 0, va("Score:  %i", score), colorWhite, cls.media.charsetShader, false, DSF_LEFT);
			CL_DrawString(x+32, y+16, 8, 8, 0, 0, 0, va("Ping:  %i", ping), colorWhite, cls.media.charsetShader, false, DSF_LEFT);
			CL_DrawString(x+32, y+24, 8, 8, 0, 0, 0, va("Time:  %i", time), colorWhite, cls.media.charsetShader, false, DSF_LEFT);
			continue;
		}
		if (!Q_stricmp(token, "ctf")){
			// Draw a CTF client block
			token = Com_Parse(&string);
			x = cls.glConfig.videoWidth/2 - 160 + atoi(token);
			token = Com_Parse(&string);
			y = cls.glConfig.videoHeight/2 - 120 + atoi(token);

			token = Com_Parse(&string);
			index = atoi(token);
			if (index < 0 || index >= MAX_CLIENTS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad client index %i", index);

			ci = &cl.clientInfo[index];
			if (!ci->valid)
				ci = &cl.baseClientInfo;

			token = Com_Parse(&string);
			score = atoi(token);
			token = Com_Parse(&string);
			ping = atoi(token);

			if (ping > 999)
				ping = 999;

			Q_snprintfz(block, sizeof(block), "%3d %3d %-12.12s", score, ping, ci->name);

			if (value+1 == cl.clientNum)
				CL_DrawString(x, y, 8, 8, 0, 0, 0, block, colorGreen, cls.media.charsetShader, false, DSF_LEFT);
			else
				CL_DrawString(x, y, 8, 8, 0, 0, 0, block, colorWhite, cls.media.charsetShader, false, DSF_LEFT);

			continue;
		}
		if (!Q_stricmp(token, "pic")){
			// Draw a pic from a stat number
			token = Com_Parse(&string);
			index = atoi(token);
			if (index < 0 || index >= MAX_STATS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad pic index %i", index);

			index = cl.playerState->stats[index];
			if (index < 0 || index >= MAX_IMAGES)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad pic index %i", index);

			if (!cl.media.gameShaders[index])
				continue;

			if (cl_drawIcons->integer)
				CL_DrawPicFixed(x, y, cl.media.gameShaders[index]);

			continue;
		}
		if (!Q_stricmp(token, "picn")){
			// Draw a pic from a name
			token = Com_Parse(&string);
			if (cl_drawIcons->integer)
				CL_DrawPicFixedByName(x, y, token);

			continue;
		}
		if (!Q_stricmp(token, "num")){
			// Draw a number
			token = Com_Parse(&string);
			width = atoi(token);
			token = Com_Parse(&string);
			index = atoi(token);
			if (index < 0 || index >= MAX_STATS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad num index %i", index);

			CL_DrawLayoutField(x, y, 0, width, cl.playerState->stats[index]);
			continue;
		}
		if (!Q_stricmp(token, "hnum")){
			// Health number
			value = cl.playerState->stats[STAT_HEALTH];
			if (value > 25)
				color = 0;
			else if (value > 0)
				color = (cl.frame.serverFrame>>2) & 1;
			else
				color = 1;

			if (cl.playerState->stats[STAT_FLASHES] & 1)
				CL_DrawPicFixedByName(x, y, "field_3");

			CL_DrawLayoutField(x, y, color, 3, value);
			continue;
		}
		if (!Q_stricmp(token, "rnum")){
			// Armor number
			value = cl.playerState->stats[STAT_ARMOR];
			if (value < 1)
				continue;

			if (cl.playerState->stats[STAT_FLASHES] & 2)
				CL_DrawPicFixedByName(x, y, "field_3");

			CL_DrawLayoutField(x, y, 0, 3, value);
			continue;
		}
		if (!Q_stricmp(token, "anum")){
			// Ammo number
			value = cl.playerState->stats[STAT_AMMO];
			if (value > 5)
				color = 0;
			else if (value >= 0)
				color = (cl.frame.serverFrame>>2) & 1;
			else
				continue;

			if (cl.playerState->stats[STAT_FLASHES] & 4)
				CL_DrawPicFixedByName(x, y, "field_3");

			CL_DrawLayoutField(x, y, color, 3, value);
			continue;
		}
		if (!Q_stricmp(token, "stat_string")){
			token = Com_Parse(&string);
			index = atoi(token);
			if (index < 0 || index >= MAX_STATS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad stat_string index %i", index);

			index = cl.playerState->stats[index];
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad stat_string index %i", index);

			CL_DrawString(x, y, 8, 8, 0, 0, 0, cl.configStrings[index], colorWhite, cls.media.charsetShader, false, DSF_LEFT);
			continue;
		}
		if (!Q_stricmp(token, "string")){
			token = Com_Parse(&string);
			CL_DrawString(x, y, 8, 8, 0, 0, 0, token, colorWhite, cls.media.charsetShader, false, DSF_LEFT);
			continue;
		}
		if (!Q_stricmp(token, "string2")){
			token = Com_Parse(&string);
			CL_DrawString(x, y, 8, 8, 0, 0, 0, token, colorGreen, cls.media.charsetShader, false, DSF_LEFT);
			continue;
		}
		if (!Q_stricmp(token, "cstring")){
			token = Com_Parse(&string);
			CL_DrawLayoutString(token, x, y, 320, 0);
			continue;
		}
		if (!Q_stricmp(token, "cstring2")){
			token = Com_Parse(&string);
			CL_DrawLayoutString(token, x, y, 320, 0x80);
			continue;
		}
		if (!Q_stricmp(token, "if")){
			token = Com_Parse(&string);
			index = atoi(token);
			if (index < 0 || index >= MAX_STATS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad if index %i", index);

			if (!cl.playerState->stats[index]){
				// Skip to endif
				while (string && Q_stricmp(token, "endif"))
					token = Com_Parse(&string);
			}

			continue;
		}
	}
}

/*
 =================
 CL_DrawStatus
 =================
*/
static void CL_DrawStatus (void){

	itemDef_t		*itemDef;
	skipIfStat_t	*skipIfStat;
	drawFunc_t		*drawFunc;
	qboolean		result;
	int				i, j;

	if (cl.gameMod || !cl_newHUD->integer || !cl_hudDef.numItemDefs){
		CL_ExecuteLayoutString(cl.configStrings[CS_STATUSBAR]);
		return;
	}

	for (i = 0, itemDef = cl_hudDef.itemDefs; i < cl_hudDef.numItemDefs; i++, itemDef++){
		if (itemDef->onlyMultiPlayer){
			if (!cl.multiPlayer)
				continue;
		}

		for (j = 0, skipIfStat = itemDef->skipIfStats; j < itemDef->numSkipIfStats; j++, skipIfStat++){
			switch (skipIfStat->func){
			case HUD_GREATER:
				result = (cl.playerState->stats[skipIfStat->stat] > skipIfStat->value);
				break;
			case HUD_LESS:
				result = (cl.playerState->stats[skipIfStat->stat] < skipIfStat->value);
				break;
			case HUD_GEQUAL:
				result = (cl.playerState->stats[skipIfStat->stat] >= skipIfStat->value);
				break;
			case HUD_LEQUAL:
				result = (cl.playerState->stats[skipIfStat->stat] <= skipIfStat->value);
				break;
			case HUD_EQUAL:
				result = (cl.playerState->stats[skipIfStat->stat] == skipIfStat->value);
				break;
			case HUD_NOTEQUAL:
				result = (cl.playerState->stats[skipIfStat->stat] != skipIfStat->value);
				break;
			}

			if (result)
				break;
		}

		if (j != itemDef->numSkipIfStats)
			continue;

		for (j = 0, drawFunc = itemDef->drawFuncs; j < itemDef->numDrawFuncs; j++, drawFunc++){
			switch (drawFunc->type){
			case HUD_DRAWPIC:
				CL_DrawHUDPic(drawFunc);
				break;
			case HUD_DRAWSTRING:
				CL_DrawHUDString(drawFunc);
				break;
			case HUD_DRAWNUMBER:
				CL_DrawHUDNumber(drawFunc);
				break;
			case HUD_DRAWBAR:
				CL_DrawHUDBar(drawFunc);
				break;
			}
		}
	}
}

/*
 =================
 CL_DrawLayout
 =================
*/
static void CL_DrawLayout (void){

	if (!(cl.playerState->stats[STAT_LAYOUTS] & 1))
		return;

	CL_ExecuteLayoutString(cl.layout);
}

/*
 =================
 CL_DrawInventory
 =================
*/
static void CL_DrawInventory (void){

#define	DISPLAY_ITEMS	17

	int		i, j, x, y, top;
	int		index[MAX_ITEMS];
	char	string[128], binding[128], *bind;
	int		num, item;
	int		selected, selectedNum;

	if (!(cl.playerState->stats[STAT_LAYOUTS] & 2))
		return;

	selected = cl.playerState->stats[STAT_SELECTED_ITEM];

	num = 0;
	selectedNum = 0;
	for (i = 0; i < MAX_ITEMS; i++){
		if (i == selected)
			selectedNum = num;

		if (cl.inventory[i]){
			index[num] = i;
			num++;
		}
	}

	// Determine scroll point
	top = selectedNum - DISPLAY_ITEMS/2;
	if (num - top < DISPLAY_ITEMS)
		top = num - DISPLAY_ITEMS;
	if (top < 0)
		top = 0;

	x = (cls.glConfig.videoWidth-256)/2;
	y = (cls.glConfig.videoHeight-240)/2;

	CL_DrawPicFixedByName(x, y, "inventory");

	y += 24;
	x += 24;
	CL_DrawString(x, y, 8, 8, 0, 0, 0, "hotkey ### item", colorWhite, cls.media.charsetShader, false, DSF_FORCECOLOR|DSF_LEFT);
	CL_DrawString(x, y+8, 8, 8, 0, 0, 0, "------ --- ----", colorWhite, cls.media.charsetShader, false, DSF_FORCECOLOR|DSF_LEFT);
	y += 16;
	for (i = top; i < num && i < top+DISPLAY_ITEMS; i++){
		item = index[i];

		// Search for a binding
		Q_snprintfz(binding, sizeof(binding), "use %s", cl.configStrings[CS_ITEMS+item]);
		bind = "";
		for (j = 0; j < 256; j++){
			if (!Q_stricmp(Key_GetBinding(j), binding)){
				bind = Key_KeyNumToString(j);
				break;
			}
		}

		Q_snprintfz(string, sizeof(string), "%6s %3i %s", bind, cl.inventory[item], cl.configStrings[CS_ITEMS+item]);
		if (item != selected)
			CL_DrawString(x, y, 8, 8, 0, 0, 0, string, colorGreen, cls.media.charsetShader, false, DSF_FORCECOLOR|DSF_LEFT);
		else {	
			// Draw a blinky cursor by the selected item
			if ((cls.realTime >> 8) & 1)
				CL_DrawString(x-8, y, 8, 8, 0, 0, 0, "\15", colorWhite, cls.media.charsetShader, false, DSF_FORCECOLOR|DSF_LEFT);

			CL_DrawString(x, y, 8, 8, 0, 0, 0, string, colorWhite, cls.media.charsetShader, false, DSF_FORCECOLOR|DSF_LEFT);
		}
		
		y += 8;
	}
}


/*
 =======================================================================

 CINEMATIC FUNCTIONS

 =======================================================================
*/


/*
 =================
 CL_PlayCinematic
 =================
*/
void CL_PlayCinematic (const char *name){

	char	path[MAX_QPATH];

	// If currently playing another, stop it
	CL_StopCinematic();

	Com_DPrintf("CL_PlayCinematic( %s )\n", name);

	if (!Q_stricmp(name+strlen(name)-4, ".pcx")){
		Q_strncpyz(path, name, sizeof(path));
		Com_DefaultPath(path, sizeof(path), "pics");
	}
	else {
		Q_strncpyz(path, name, sizeof(path));
		Com_DefaultPath(path, sizeof(path), "video");
		Com_DefaultExtension(path, sizeof(path), ".cin");
	}

	cls.cinematicHandle = CIN_PlayCinematic(path, 0, 0, cls.glConfig.videoWidth, cls.glConfig.videoHeight, CIN_SYSTEM);
	if (!cls.cinematicHandle){
		Com_Printf("Cinematic %s not found\n", path);

		CL_FinishCinematic();
	}
}

/*
 =================
 CL_StopCinematic
 =================
*/
void CL_StopCinematic (void){

	if (!cls.cinematicHandle)
		return;

	Com_DPrintf("CL_StopCinematic()\n");

	CIN_StopCinematic(cls.cinematicHandle);
	cls.cinematicHandle = 0;
}

/*
 =================
 CL_FinishCinematic

 Called when either the cinematic completes, or it is aborted
 =================
*/
void CL_FinishCinematic (void){

	Com_DPrintf("CL_FinishCinematic()\n");

	if (cls.state == CA_DISCONNECTED){
		UI_SetActiveMenu(UI_MAINMENU);
		return;
	}

	// Not active anymore, but not disconnected
	cls.state = CA_CONNECTED;

	CL_Loading();

	// Tell the server to advance to the next map / cinematic
	MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print(&cls.netChan.message, va("nextserver %i\n", cl.serverCount));
}

/*
 =================
 CL_DrawCinematic
 =================
*/
void CL_DrawCinematic (void){

	if (!cls.cinematicHandle)
		return;

	if (!CIN_RunCinematic(cls.cinematicHandle)){
		CL_StopCinematic();
		CL_FinishCinematic();
		return;
	}

	CIN_SetExtents(cls.cinematicHandle, 0, 0, cls.glConfig.videoWidth, cls.glConfig.videoHeight);
	CIN_DrawCinematic(cls.cinematicHandle);
}


// =====================================================================


/*
 =================
 CL_ScanForPlayerEntity
 =================
*/
static void CL_ScanForPlayerEntity (void){

	trace_t	trace;
	vec3_t	start, end;
	int		entNumber;

	VectorCopy(cl.refDef.viewOrigin, start);
	VectorMA(cl.refDef.viewOrigin, 8192, cl.refDef.viewAxis[0], end);

	trace = CL_Trace(start, vec3_origin, vec3_origin, end, cl.clientNum, MASK_PLAYERSOLID, false, &entNumber);
	if (trace.fraction == 0.0 || trace.fraction == 1.0)
		return;

	if (entNumber < 1 || entNumber > MAX_CLIENTS)
		return;		// Not a valid entity

	if (cl.entities[entNumber].current.modelindex != 255)
		return;		// Not a player, or invisible

	cl.crosshairEntNumber = entNumber;
	cl.crosshairEntTime = cl.time;
}

/*
 =================
 CL_DrawCrosshair
 =================
*/
static void CL_DrawCrosshair (void){

	float			x, y, w, h;
	int				crosshair, health;
	color_t			color;
	byte			*fadeColor;
	clientInfo_t	*ci;

	if ((cl.refDef.rdFlags & RDF_IRGOGGLES) || cl_thirdPerson->integer)
		return;

	crosshair = (cl_drawCrosshair->integer - 1) % NUM_CROSSHAIRS;
	if (crosshair < 0)
		return;

	// Set dimensions and position
	w = cl_crosshairSize->integer * cls.screenScaleX;
	h = cl_crosshairSize->integer * cls.screenScaleY;

	x = (cl_crosshairX->integer * cls.screenScaleX) + cl.refDef.x + ((cl.refDef.width - w) * 0.5);
	y = (cl_crosshairY->integer * cls.screenScaleY) + cl.refDef.y + ((cl.refDef.height - h) * 0.5);

	// Set color and alpha
	if (cl_crosshairHealth->integer){
		health = cl.playerState->stats[STAT_HEALTH];

		color[0] = 255;

		if (health > 60)
			color[1] = 255;
		else if (health < 30)
			color[1] = 0;
		else
			color[1] = 255 * ((float)(health - 30) / 30.0);

		if (health > 99)
			color[2] = 255;
		else if (health < 66)
			color[2] = 0;
		else
			color[2] = 255 * ((float)(health - 66) / 33.0);

		color[3] = 255 * Clamp(cl_crosshairAlpha->value, 0.0, 1.0);
	}
	else {
		color[0] = colorTable[cl_crosshairColor->integer & Q_COLOR_MASK][0];
		color[1] = colorTable[cl_crosshairColor->integer & Q_COLOR_MASK][1];
		color[2] = colorTable[cl_crosshairColor->integer & Q_COLOR_MASK][2];
		color[3] = 255 * Clamp(cl_crosshairAlpha->value, 0.0, 1.0);
	}

	// Draw it
	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, color, cl.media.crosshairShaders[crosshair]);

	// Draw target name
	if (cl_crosshairNames->integer){
		if (!cl.multiPlayer)
			return;		// Don't bother in singleplayer

		// Scan for player entity
		CL_ScanForPlayerEntity();

		if (!cl.crosshairEntNumber || !cl.crosshairEntTime)
			return;

		ci = &cl.clientInfo[cl.crosshairEntNumber - 1];
		if (!ci->valid)
			return;

		// Set color and alpha
		if (cl_crosshairHealth->integer){
			color[0] = colorTable[cl_crosshairColor->integer & Q_COLOR_MASK][0];
			color[1] = colorTable[cl_crosshairColor->integer & Q_COLOR_MASK][1];
			color[2] = colorTable[cl_crosshairColor->integer & Q_COLOR_MASK][2];
		}

		fadeColor = CL_FadeAlpha(color, cl.crosshairEntTime, 1000, 250);
		if (!fadeColor){
			cl.crosshairEntNumber = 0;
			cl.crosshairEntTime = 0;
			return;
		}

		// Draw it
		CL_DrawString(0, (y+(h*2)) / cls.screenScaleY, 10, 10, 0, 0, 640, ci->name, fadeColor, cls.media.charsetShader, true, DSF_DROPSHADOW|DSF_CENTER);
	}
}

/*
 =================
 CL_DrawCenterString
 =================
*/
static void CL_DrawCenterString (void){

	byte	*fadeColor;

	fadeColor = CL_FadeAlpha(colorWhite, cl.centerPrintTime, cl_centerTime->integer, cl_centerTime->integer / 4);
	if (!fadeColor)
		return;

	CL_DrawString(0, 160, 10, 10, 0, 0, 640, cl.centerPrint, fadeColor, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
}

/*
 =================
 CL_DrawLagometer
 =================
*/
static void CL_DrawLagometer (void){

	int		i, c;
	float	x, y, w, h, v, scale;
	char	str[4];

	// Don't draw if we're also the server
	if (Com_ServerState())
		return;

	CL_DrawPic(592, 432, 48, 48, colorWhite, cl.media.lagometerShader);

	x = 592;
	y = 432;
	w = 48;
	h = 48;

	CL_ScaleCoords(&x, &y, &w, &h);

	scale = h / 500.0;

	for (i = 0; i < w; i++){
		c = (cl.lagometer.current - 1 - i) & (LAG_SAMPLES-1);

		if (cl.lagometer.dropped[c]){
			R_DrawStretchPic(x+w - i, y, 1, h, 0, 0, 1, 1, colorRed, cls.media.whiteShader);
			continue;
		}

		v = cl.lagometer.ping[c] * scale;
		if (v > h)
			v = h;

		if (cl.lagometer.suppressed[c])
			R_DrawStretchPic(x+w - i, y+h - v, 1, v, 0, 0, 1, 1, colorYellow, cls.media.whiteShader);
		else
			R_DrawStretchPic(x+w - i, y+h - v, 1, v, 0, 0, 1, 1, colorGreen, cls.media.whiteShader);
	}

	// Also draw the ping at the top
	c = cl.lagometer.ping[(cl.lagometer.current - 1) & (LAG_SAMPLES-1)];
	if (c > 999)
		c = 999;

	Q_snprintfz(str, sizeof(str), "%i", c);

	if (c < 200)
		CL_DrawString(592, 432, 10, 10, 0, 0, 48, str, colorGreen, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	else if (c < 400)
		CL_DrawString(592, 432, 10, 10, 0, 0, 48, str, colorYellow, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	else
		CL_DrawString(592, 432, 10, 10, 0, 0, 48, str, colorRed, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
}

/*
 =================
 CL_DrawDisconnected
 =================
*/
static void CL_DrawDisconnected (void){

	// Don't draw if we're also the server
	if (Com_ServerState())
		return;

	if (cls.netChan.outgoingSequence - cls.netChan.incomingAcknowledged < CMD_BACKUP-1)
		return;

	CL_DrawString(0, 100, 16, 16, 0, 0, 640, "Connection Interrupted", colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);

	// Blink the icon
	if ((cl.time >> 9) & 1)
		return;

	CL_DrawPic(592, 432, 48, 48, colorWhite, cl.media.disconnectedShader);
}

/*
 =================
 CL_DrawRecording
 =================
*/
static void CL_DrawRecording (void){

	char	str[128];

	if (!cls.demoFile)
		return;

	Q_snprintfz(str, sizeof(str), "Recording: %s (%i KB)", cls.demoName, FS_Tell(cls.demoFile) / 1024);
	CL_DrawString(0, 120, 10, 10, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
}

/*
 =================
 CL_DrawFPS
 =================
*/
static void CL_DrawFPS (void){

#define FPS_FRAMES		4

	static int	previousTimes[FPS_FRAMES];
	static int	previousTime;
	static int	index;
	char		str[8];
	int			i, time, total;
	int			fps;

	time = Sys_Milliseconds();

	previousTimes[index % FPS_FRAMES] = time - previousTime;
	previousTime = time;
	index++;

	if (index <= FPS_FRAMES)
		return;

	// Average multiple frames together to smooth changes out a bit
	total = 0;
	for (i = 0; i < FPS_FRAMES; i++)
		total += previousTimes[i];

	if (total < 1)
		total = 1;

	fps = 1000 * FPS_FRAMES / total;

	Q_snprintfz(str, sizeof(str), "%3i FPS", fps);
	CL_DrawString(0, 0, 16, 16, 0, 0, 624, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_RIGHT);
}

/*
 =================
 CL_DrawPause
 =================
*/
static void CL_DrawPause (void){

	if (!paused->integer || UI_IsVisible())
		return;

	CL_DrawPic(0, 220, 640, 40, colorWhite, cl.media.pauseShader);
}

/*
 =================
 CL_ShowShader

 Development tool
 =================
*/
static void CL_ShowShader (void){

	trace_t	trace;
	vec3_t	start, end;
	char	string[512];

	VectorCopy(cl.refDef.viewOrigin, start);
	VectorMA(cl.refDef.viewOrigin, 8192, cl.refDef.viewAxis[0], end);

	if (cl_showShader->integer == 1){
		trace = CL_Trace(start, vec3_origin, vec3_origin, end, cl.clientNum, MASK_SOLID | MASK_WATER, true, NULL);
		if (trace.fraction == 0.0 || trace.fraction == 1.0)
			return;
	}
	else {
		trace = CL_Trace(start, vec3_origin, vec3_origin, end, cl.clientNum, MASK_ALL, true, NULL);
		if (trace.fraction == 0.0 || trace.fraction == 1.0)
			return;
	}

	CL_DrawString(0, 300, 10, 10, 0, 0, 640, "SHADER", colorGreen, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	CL_DrawString(0, 322, 10, 10, 0, 0, 640, "SURFACE FLAGS", colorGreen, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	CL_DrawString(0, 344, 10, 10, 0, 0, 640, "CONTENTS FLAGS", colorGreen, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);

	// Shader
	Q_snprintfz(string, sizeof(string), "textures/%s", trace.surface->name);
	CL_DrawString(0, 310, 10, 10, 0, 0, 640, string, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);

	// Surface flags
	if (trace.surface->flags){
		string[0] = 0;

		if (trace.surface->flags & SURF_LIGHT)
			Q_strncatz(string, "LIGHT ", sizeof(string));
		if (trace.surface->flags & SURF_SLICK)
			Q_strncatz(string, "SLICK ", sizeof(string));
		if (trace.surface->flags & SURF_SKY)
			Q_strncatz(string, "SKY ", sizeof(string));
		if (trace.surface->flags & SURF_WARP)
			Q_strncatz(string, "WARP ", sizeof(string));
		if (trace.surface->flags & SURF_TRANS33)
			Q_strncatz(string, "TRANS33 ", sizeof(string));
		if (trace.surface->flags & SURF_TRANS66)
			Q_strncatz(string, "TRANS66 ", sizeof(string));
		if (trace.surface->flags & SURF_FLOWING)
			Q_strncatz(string, "FLOWING ", sizeof(string));
		if (trace.surface->flags & SURF_NODRAW)
			Q_strncatz(string, "NODRAW ", sizeof(string));

		string[strlen(string)-1] = 0;

		CL_DrawString(0, 332, 10, 10, 0, 0, 640, string, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	}

	// Contents flags
	if (trace.contents){
		string[0] = 0;

		if (trace.contents & CONTENTS_SOLID)
			Q_strncatz(string, "SOLID ", sizeof(string));
		if (trace.contents & CONTENTS_WINDOW)
			Q_strncatz(string, "WINDOW ", sizeof(string));
		if (trace.contents & CONTENTS_AUX)
			Q_strncatz(string, "AUX ", sizeof(string));
		if (trace.contents & CONTENTS_LAVA)
			Q_strncatz(string, "LAVA ", sizeof(string));
		if (trace.contents & CONTENTS_SLIME)
			Q_strncatz(string, "SLIME ", sizeof(string));
		if (trace.contents & CONTENTS_WATER)
			Q_strncatz(string, "WATER ", sizeof(string));
		if (trace.contents & CONTENTS_MIST)
			Q_strncatz(string, "MIST ", sizeof(string));

		if (!string[0])
			Q_strncatz(string, "\n", sizeof(string));
		else
			string[strlen(string)-1] = '\n';

		if (trace.contents & CONTENTS_AREAPORTAL)
			Q_strncatz(string, "AREAPORTAL ", sizeof(string));
		if (trace.contents & CONTENTS_PLAYERCLIP)
			Q_strncatz(string, "PLAYERCLIP ", sizeof(string));
		if (trace.contents & CONTENTS_MONSTERCLIP)
			Q_strncatz(string, "MONSTERCLIP ", sizeof(string));

		if (!string[0])
			Q_strncatz(string, "\n", sizeof(string));
		else
			string[strlen(string)-1] = '\n';

		if (trace.contents & CONTENTS_CURRENT_0)
			Q_strncatz(string, "CURRENT_0 ", sizeof(string));
		if (trace.contents & CONTENTS_CURRENT_90)
			Q_strncatz(string, "CURRENT_90 ", sizeof(string));
		if (trace.contents & CONTENTS_CURRENT_180)
			Q_strncatz(string, "CURRENT_180 ", sizeof(string));
		if (trace.contents & CONTENTS_CURRENT_270)
			Q_strncatz(string, "CURRENT_270 ", sizeof(string));
		if (trace.contents & CONTENTS_CURRENT_UP)
			Q_strncatz(string, "CURRENT_UP ", sizeof(string));
		if (trace.contents & CONTENTS_CURRENT_DOWN)
			Q_strncatz(string, "CURRENT_DOWN ", sizeof(string));

		if (!string[0])
			Q_strncatz(string, "\n", sizeof(string));
		else
			string[strlen(string)-1] = '\n';

		if (trace.contents & CONTENTS_ORIGIN)
			Q_strncatz(string, "ORIGIN ", sizeof(string));
		if (trace.contents & CONTENTS_MONSTER)
			Q_strncatz(string, "MONSTER ", sizeof(string));
		if (trace.contents & CONTENTS_DEADMONSTER)
			Q_strncatz(string, "DEADMONSTER ", sizeof(string));
		if (trace.contents & CONTENTS_DETAIL)
			Q_strncatz(string, "DETAIL ", sizeof(string));
		if (trace.contents & CONTENTS_TRANSLUCENT)
			Q_strncatz(string, "TRANSLUCENT ", sizeof(string));
		if (trace.contents & CONTENTS_LADDER)
			Q_strncatz(string, "LADDER ", sizeof(string));

		string[strlen(string)-1] = 0;

		CL_DrawString(0, 354, 10, 10, 0, 0, 640, string, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	}
}


// =====================================================================


/*
 =================
 CL_TileClearBox

 This repeats a 64x64 tile graphic to fill the screen around a sized
 down refresh window
 =================
*/
static void CL_TileClearBox (int x, int y, int w, int h){

	float	sl, tl, sh, th;

	sl = x / 64.0;
	tl = y / 64.0;
	sh = (x+w) / 64.0;
	th = (y+h) / 64.0;

	R_DrawStretchPic(x, y, w, h, sl, tl, sh, th, colorWhite, cl.media.backTileShader);
}

/*
 =================
 CL_TileClear

 Clear around a sized down screen
 =================
*/
void CL_TileClear (void){

	int		w, h;
	int		t, b, l, r;

	if (cl_viewSize->integer == 100)
		return;		// Full screen rendering

	w = cls.glConfig.videoWidth;
	h = cls.glConfig.videoHeight;

	t = cl.refDef.y;
	b = t + cl.refDef.height-1;
	l = cl.refDef.x;
	r = l + cl.refDef.width-1;

	// Clear above view screen
	CL_TileClearBox(0, 0, w, t);
	// Clear below view screen
	CL_TileClearBox(0, b, w, h-b);
	// Clear left of view screen
	CL_TileClearBox(0, t, l, b-t+1);
	// Clear right of view screen
	CL_TileClearBox(r, t, w-r, b-t+1);
}

/*
 =================
 CL_Draw2D

 Draw all on-screen information
 =================
*/
void CL_Draw2D (void){

	if (!cl_draw2D->integer)
		return;

	// General game information
	if (cl_drawCrosshair->integer)
		CL_DrawCrosshair();

	if (cl_drawStatus->integer)
		CL_DrawStatus();

	if (cl_drawCenterString->integer)
		CL_DrawCenterString();

	if (cl_drawInventory->integer)
		CL_DrawInventory();

	if (cl_drawLayout->integer)
		CL_DrawLayout();

	if (cl_drawLagometer->integer)
		CL_DrawLagometer();

	if (cl_drawDisconnected->integer)
		CL_DrawDisconnected();

	if (cl_drawRecording->integer)
		CL_DrawRecording();

	if (cl_drawFPS->integer)
		CL_DrawFPS();

	if (cl_drawPause->integer)
		CL_DrawPause();

	// Development tool
	if (cl_showShader->integer)
		CL_ShowShader();
}

/*
 =================
 CL_UpdateScreen

 This is called every frame, and can also be called explicitly to flush
 stuff to the screen
 =================
*/
void CL_UpdateScreen (void){

	int		i;
	int		numFrames = 1;
	float	stereoSeparation[2] = {0, 0};

	// If the screen is disabled, do nothing at all
	if (cls.screenDisabled)
		return;

	if (cls.glConfig.stereoEnabled){
		numFrames = 2;

		// Range check camera separation so we don't inadvertently fry
		// someone's brain
		if (cl_stereoSeparation->value > 1.0)
			Cvar_SetValue("cl_stereoSeparation", 1.0);
		else if (cl_stereoSeparation->value < 0.0)
			Cvar_SetValue("cl_stereoSeparation", 0.0);

		stereoSeparation[0] = -cl_stereoSeparation->value / 2;
		stereoSeparation[1] = cl_stereoSeparation->value / 2;
	}

	for (i = 0; i < numFrames; i++){
		R_BeginFrame(cls.realTime, stereoSeparation[i]);

		if (cls.loading)
			// Loading level data (or connecting)
			CL_DrawLoading();
		else if (cls.cinematicHandle)
			// Playing a cinematic
			CL_DrawCinematic();
		else {
			// Displaying game views and/or menu
			CL_RenderView(stereoSeparation[i]);

			UI_UpdateMenu(cls.realTime);
		}

		// Console can be visible at any time
		Con_DrawConsole();
	}

	R_EndFrame();
}
