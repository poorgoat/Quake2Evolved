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


// cl_screen.c -- master for refresh, status bar, console, etc...


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
#define HUD_STATMATERIAL	8

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
	int					stat;
	cmpFunc_t			func;
	int					value;
} skipIfStat_t;

typedef struct {
	drawType_t			type;
	int					rect[4];
	int					size[2];
	int					range[2];
	int					shear[2];
	fillDir_t			fillDir;
	color_t				color;
	color_t				flashColor;
	int					flags;
	int					fromStat;
	char				string[64];
	int					number;
	int					value;
	struct material_s	*material;
	struct material_s	*materialMinus;
	int					stringStat;
	int					numberStat;
	int					valueStat;
	int					materialStat;
} drawFunc_t;

typedef struct {
	qboolean			onlyMultiPlayer;

	skipIfStat_t		skipIfStats[HUD_MAX_SKIPIFSTATS];
	int					numSkipIfStats;

	drawFunc_t			drawFuncs[HUD_MAX_DRAWFUNCS];
	int					numDrawFuncs;
} itemDef_t;

typedef struct {
	itemDef_t			itemDefs[HUD_MAX_ITEMDEFS];
	int					numItemDefs;
} hudDef_t;

static hudDef_t		cl_hudDef;


/*
 =================
 CL_ParseHUDDrawFunc
 =================
*/
static qboolean CL_ParseHUDDrawFunc (itemDef_t *itemDef, const char *name, script_t *script){

	token_t		token;
	drawFunc_t	*drawFunc;
	float		color;
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

	if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
		return false;
	}

	if (!Q_stricmp(token.string, "{")){
		while (1){
			if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
				Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in '%s' in HUD script\n", name);
				return false;
			}

			if (!Q_stricmp(token.string, "}"))
				break;

			if (!Q_stricmp(token.string, "rect")){
				for (i = 0; i < 4; i++){
					if (!PS_ReadInteger(script, 0, &drawFunc->rect[i])){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}
				}
			}
			else if (!Q_stricmp(token.string, "size")){
				for (i = 0; i < 2; i++){
					if (!PS_ReadInteger(script, 0, &drawFunc->size[i])){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}
				}
			}
			else if (!Q_stricmp(token.string, "range")){
				for (i = 0; i < 2; i++){
					if (!PS_ReadInteger(script, 0, &drawFunc->range[i])){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}
				}
			}
			else if (!Q_stricmp(token.string, "shear")){
				for (i = 0; i < 2; i++){
					if (!PS_ReadInteger(script, 0, &drawFunc->shear[i])){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}
				}
			}
			else if (!Q_stricmp(token.string, "fillDir")){
				if (!PS_ReadToken(script, 0, &token)){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				if (!Q_stricmp(token.string, "leftToRight"))
					drawFunc->fillDir = HUD_LEFTTORIGHT;
				else if (!Q_stricmp(token.string, "rightToLeft"))
					drawFunc->fillDir = HUD_RIGHTTOLEFT;
				else if (!Q_stricmp(token.string, "bottomToTop"))
					drawFunc->fillDir = HUD_BOTTOMTOTOP;
				else if (!Q_stricmp(token.string, "topToBottom"))
					drawFunc->fillDir = HUD_TOPTOBOTTOM;
				else {
					Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' for '%s' in HUD script\n", token.string, name);
					return false;
				}
			}
			else if (!Q_stricmp(token.string, "color")){
				for (i = 0; i < 4; i++){
					if (!PS_ReadFloat(script, 0, &color)){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->color[i] = drawFunc->flashColor[i] = 255 * Clamp(color, 0.0, 1.0);
				}

				if (PS_ReadToken(script, 0, &token)){
					if (Q_stricmp(token.string, "flash")){
						Com_Printf(S_COLOR_YELLOW "WARNING: expected 'flash', found '%s' instead in '%s' in HUD script\n", token.string, name);
						return false;
					}

					for (i = 0; i < 4; i++){
						if (!PS_ReadFloat(script, 0, &color)){
							Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
							return false;
						}

						drawFunc->flashColor[i] = 255 * Clamp(color, 0.0, 1.0);
					}
				}
			}
			else if (!Q_stricmp(token.string, "flags")){
				while (1){
					if (!PS_ReadToken(script, 0, &token))
						break;

					if (!Q_stricmp(token.string, "left"))
						drawFunc->flags |= DSF_LEFT;
					else if (!Q_stricmp(token.string, "center"))
						drawFunc->flags |= DSF_CENTER;
					else if (!Q_stricmp(token.string, "right"))
						drawFunc->flags |= DSF_RIGHT;
					else if (!Q_stricmp(token.string, "lower"))
						drawFunc->flags |= DSF_LOWERCASE;
					else if (!Q_stricmp(token.string, "upper"))
						drawFunc->flags |= DSF_UPPERCASE;
					else if (!Q_stricmp(token.string, "shadow"))
						drawFunc->flags |= DSF_DROPSHADOW;
					else {
						Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' for '%s' in HUD script\n", token.string, name);
						return false;
					}
				}
			}
			else if (!Q_stricmp(token.string, "string")){
				if (!PS_ReadToken(script, 0, &token)){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				if (!Q_stricmp(token.string, "fromStat")){
					if (!PS_ReadInteger(script, 0, &drawFunc->stringStat)){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->stringStat = Clamp(drawFunc->stringStat, 0, 31);

					drawFunc->fromStat |= HUD_STATSTRING;
				}
				else
					Q_strncpyz(drawFunc->string, token.string, sizeof(drawFunc->string));
			}
			else if (!Q_stricmp(token.string, "number")){
				if (!PS_ReadToken(script, 0, &token)){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				if (!Q_stricmp(token.string, "fromStat")){
					if (!PS_ReadInteger(script, 0, &drawFunc->numberStat)){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->numberStat = Clamp(drawFunc->numberStat, 0, 31);

					drawFunc->fromStat |= HUD_STATNUMBER;
				}
				else
					drawFunc->number = token.integerValue;
			}
			else if (!Q_stricmp(token.string, "value")){
				if (!PS_ReadToken(script, 0, &token)){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				if (!Q_stricmp(token.string, "fromStat")){
					if (!PS_ReadInteger(script, 0, &drawFunc->valueStat)){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->valueStat = Clamp(drawFunc->valueStat, 0, 31);

					drawFunc->fromStat |= HUD_STATVALUE;
				}
				else
					drawFunc->value = token.integerValue;
			}
			else if (!Q_stricmp(token.string, "material")){
				if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				if (!Q_stricmp(token.string, "fromStat")){
					if (!PS_ReadInteger(script, 0, &drawFunc->materialStat)){
						Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
						return false;
					}

					drawFunc->materialStat = Clamp(drawFunc->materialStat, 0, 31);

					drawFunc->fromStat |= HUD_STATMATERIAL;
				}
				else
					drawFunc->material = R_RegisterMaterialNoMip(token.string);
			}
			else if (!Q_stricmp(token.string, "minus")){
				if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for '%s' in HUD script\n", name);
					return false;
				}

				drawFunc->materialMinus = R_RegisterMaterialNoMip(token.string);
			}
			else {
				Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' for '%s' in HUD script\n", token.string, name);
				return false;
			}
		}
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in '%s' in HUD script\n", token.string, name);
		return false;
	}

	return true;
}

/*
 =================
 CL_ParseHUDSkipIfStat
 =================
*/
static qboolean CL_ParseHUDSkipIfStat (itemDef_t *itemDef, script_t *script){

	token_t			token;
	skipIfStat_t	*skipIfStat;

	if (itemDef->numSkipIfStats == HUD_MAX_SKIPIFSTATS){
		Com_Printf(S_COLOR_YELLOW "WARNING: HUD_MAX_SKIPIFSTATS hit in HUD script");
		return false;
	}
	skipIfStat = &itemDef->skipIfStats[itemDef->numSkipIfStats++];

	if (!PS_ReadInteger(script, 0, &skipIfStat->stat)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skipIfStat' in HUD script\n");
		return false;
	}

	skipIfStat->stat = Clamp(skipIfStat->stat, 0, 31);

	if (!PS_ReadToken(script, 0, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skipIfStat' in HUD script\n");
		return false;
	}

	if (!Q_stricmp(token.string, ">"))
		skipIfStat->func = HUD_GREATER;
	else if (!Q_stricmp(token.string, "<"))
		skipIfStat->func = HUD_LESS;
	else if (!Q_stricmp(token.string, ">="))
		skipIfStat->func = HUD_GEQUAL;
	else if (!Q_stricmp(token.string, "<="))
		skipIfStat->func = HUD_LESS;
	else if (!Q_stricmp(token.string, "=="))
		skipIfStat->func = HUD_EQUAL;
	else if (!Q_stricmp(token.string, "!="))
		skipIfStat->func = HUD_NOTEQUAL;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' for 'skipIfStat' in HUD script\n", token.string);
		return false;
	}

	if (!PS_ReadInteger(script, 0, &skipIfStat->value)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skipIfStat' in HUD script\n");
		return false;
	}

	return true;
}

/*
 =================
 CL_ParseHUDItemDef
 =================
*/
static qboolean CL_ParseHUDItemDef (hudDef_t *hudDef, script_t *script){

	token_t		token;
	itemDef_t	*itemDef;

	if (hudDef->numItemDefs == HUD_MAX_ITEMDEFS){
		Com_Printf(S_COLOR_YELLOW "WARNING: HUD_MAX_ITEMDEFS hit in HUD script\n");
		return false;
	}
	itemDef = &hudDef->itemDefs[hudDef->numItemDefs++];

	if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'itemDef' in HUD script\n");
		return false;
	}

	if (!Q_stricmp(token.string, "{")){
		while (1){
			if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
				Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in 'itemDef' in HUD script\n'");
				return false;
			}

			if (!Q_stricmp(token.string, "}"))
				break;

			if (!Q_stricmp(token.string, "onlyMultiPlayer"))
				itemDef->onlyMultiPlayer = true;
			else if (!Q_stricmp(token.string, "skipIfStat")){
				if (!CL_ParseHUDSkipIfStat(itemDef, script))
					return false;
			}
			else {
				if (!CL_ParseHUDDrawFunc(itemDef, token.string, script))
					return false;
			}
		}
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in 'itemDef' in HUD script\n", token.string);
		return false;
	}

	return true;
}

/*
 =================
 CL_ParseHUD
 =================
*/
static void CL_ParseHUD (script_t *script){

	token_t	token;

	while (1){
		if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token))
			break;

		if (!Q_stricmp(token.string, "hudDef")){
			PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token);
			if (Q_stricmp(token.string, "{")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in HUD script\n", token.string);
				return;
			}

			while (1){
				if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
					Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in 'hudDef' in HUD script\n");
					return;
				}

				if (!Q_stricmp(token.string, "}"))
					break;

				if (!Q_stricmp(token.string, "itemDef")){
					if (!CL_ParseHUDItemDef(&cl_hudDef, script))
						return;
				}
				else {
					Com_Printf(S_COLOR_YELLOW "WARNING: unknown command '%s' in HUD script\n", token.string);
					return;
				}
			}
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown command '%s' in HUD script\n", token.string);
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

	script_t	*script;

	memset(&cl_hudDef, 0, sizeof(cl_hudDef));

	// Don't use new HUD with mods
	if (cl.gameMod)
		return;

	// Load the script file
	script = PS_LoadScriptFile("scripts/misc/hud.txt");
	if (!script)
		return;

	// Parse it
	CL_ParseHUD(script);

	PS_FreeScript(script);
}

/*
 =================
 CL_DrawHUDPic
 =================
*/
static void CL_DrawHUDPic (drawFunc_t *drawFunc){

	byte				*color;
	struct material_s	*material;
	int					index;

	if ((cl.frame.serverFrame>>2) & 1)
		color = drawFunc->flashColor;
	else
		color = drawFunc->color;

	if (drawFunc->fromStat & HUD_STATMATERIAL){
		index = cl.playerState->stats[drawFunc->materialStat];
		if (index < 0 || index >= MAX_IMAGES)
			Com_Error(ERR_DROP, "CL_DrawHUDPic: bad pic index %i", index);

		material = R_RegisterMaterialNoMip(va("pics/newhud/%s", cl.configStrings[CS_IMAGES+index]));
	}
	else
		material = drawFunc->material;

	if (drawFunc->shear[0] || drawFunc->shear[1])
		CL_DrawPicSheared(drawFunc->rect[0], drawFunc->rect[1], drawFunc->rect[2], drawFunc->rect[3], drawFunc->shear[0], drawFunc->shear[1], color, material);
	else
		CL_DrawPic(drawFunc->rect[0], drawFunc->rect[1], drawFunc->rect[2], drawFunc->rect[3], color, material);
}

/*
 =================
 CL_DrawHUDString
 =================
*/
static void CL_DrawHUDString (drawFunc_t *drawFunc){

	byte				*color;
	char				*string;
	struct material_s	*material;
	int					index;

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

	if (drawFunc->fromStat & HUD_STATMATERIAL){
		index = cl.playerState->stats[drawFunc->materialStat];
		if (index < 0 || index >= MAX_IMAGES)
			Com_Error(ERR_DROP, "CL_DrawHUDString: bad pic index %i", index);

		material = R_RegisterMaterialNoMip(va("pics/newhud/%s", cl.configStrings[CS_IMAGES+index]));
	}
	else
		material = drawFunc->material;

	if (drawFunc->shear[0] || drawFunc->shear[1])
		CL_DrawStringSheared(drawFunc->rect[0], drawFunc->rect[1], drawFunc->size[0], drawFunc->size[1], drawFunc->shear[0], drawFunc->shear[1], drawFunc->rect[2], string, color, material, drawFunc->flags);
	else
		CL_DrawString(drawFunc->rect[0], drawFunc->rect[1], drawFunc->size[0], drawFunc->size[1], drawFunc->rect[2], string, color, material, drawFunc->flags);
}

/*
 =================
 CL_DrawHUDNumber
 =================
*/
static void CL_DrawHUDNumber (drawFunc_t *drawFunc){

	byte				*color;
	int					number;
	struct material_s	*material;
	int					index;
	char				num[16], *ptr;
	int					width, x, y;
	int					l, frame;

	if ((cl.frame.serverFrame>>2) & 1)
		color = drawFunc->flashColor;
	else
		color = drawFunc->color;

	if (drawFunc->fromStat & HUD_STATNUMBER)
		number = cl.playerState->stats[drawFunc->numberStat];
	else
		number = drawFunc->number;

	if (drawFunc->fromStat & HUD_STATMATERIAL){
		index = cl.playerState->stats[drawFunc->materialStat];
		if (index < 0 || index >= MAX_IMAGES)
			Com_Error(ERR_DROP, "CL_DrawHUDNumber: bad pic index %i", index);

		material = R_RegisterMaterialNoMip(va("pics/newhud/%s", cl.configStrings[CS_IMAGES+index]));
	}
	else
		material = drawFunc->material;

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
			if (drawFunc->shear[0] || drawFunc->shear[1])
				CL_DrawPicSheared(x, y, drawFunc->size[0], drawFunc->size[1], drawFunc->shear[0], drawFunc->shear[1], color, drawFunc->materialMinus);
			else
				CL_DrawPic(x, y, drawFunc->size[0], drawFunc->size[1], color, drawFunc->materialMinus);
		}
		else {
			frame = *ptr - '0';

			if (drawFunc->shear[0] || drawFunc->shear[1])
				CL_DrawPicShearedST(x, y, drawFunc->size[0], drawFunc->size[1], frame * 0.1, 0, (frame * 0.1) + 0.1, 1, drawFunc->shear[0], drawFunc->shear[1], color, material);
			else
				CL_DrawPicST(x, y, drawFunc->size[0], drawFunc->size[1], frame * 0.1, 0, (frame * 0.1) + 0.1, 1, color, material);
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

	byte				*color;
	int					value;
	struct material_s	*material;
	int					index;
	float				percent;
	float				x, y, w, h;
	float				s1, t1, s2, t2;

	if ((cl.frame.serverFrame>>2) & 1)
		color = drawFunc->flashColor;
	else
		color = drawFunc->color;

	if (drawFunc->fromStat & HUD_STATVALUE)
		value = cl.playerState->stats[drawFunc->valueStat];
	else
		value = drawFunc->value;

	if (drawFunc->fromStat & HUD_STATMATERIAL){
		index = cl.playerState->stats[drawFunc->materialStat];
		if (index < 0 || index >= MAX_IMAGES)
			Com_Error(ERR_DROP, "CL_DrawHUDBar: bad pic index %i", index);

		material = R_RegisterMaterialNoMip(va("pics/newhud/%s", cl.configStrings[CS_IMAGES+index]));
	}
	else
		material = drawFunc->material;

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

		s1 = 0;
		t1 = 0;
		s2 = percent;
		t2 = 1;

		break;
	case HUD_RIGHTTOLEFT:
		x = drawFunc->rect[0] + (drawFunc->rect[2] * (1.0 - percent));
		y = drawFunc->rect[1];
		w = drawFunc->rect[2] * percent;
		h = drawFunc->rect[3];

		s1 = 1.0 - percent;
		t1 = 0;
		s2 = 1;
		t2 = 1;

		break;
	case HUD_BOTTOMTOTOP:
		x = drawFunc->rect[0];
		y = drawFunc->rect[1] + (drawFunc->rect[3] * (1.0 - percent));
		w = drawFunc->rect[2];
		h = drawFunc->rect[3] * percent;

		s1 = 0;
		t1 = 1.0 - percent;
		s2 = 1;
		t2 = 1;

		break;
	case HUD_TOPTOBOTTOM:
		x = drawFunc->rect[0];
		y = drawFunc->rect[1];
		w = drawFunc->rect[2];
		h = drawFunc->rect[3] * percent;

		s1 = 0;
		t1 = 0;
		s2 = 1;
		t2 = percent;

		break;
	}

	if (drawFunc->shear[0] || drawFunc->shear[1])
		CL_DrawPicShearedST(x, y, w, h, s1, t1, s2, t2, drawFunc->shear[0], drawFunc->shear[1], color, material);
	else
		CL_DrawPicST(x, y, w, h, s1, t1, s2, t2, color, material);
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
			CL_DrawStringFixed(x, y, 8, 8, 0, line, colorTable[COLOR_GREEN], cls.media.charsetMaterial, DSF_LEFT);
		else
			CL_DrawStringFixed(x, y, 8, 8, 0, line, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_LEFT);

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

		CL_DrawPicFixed(x, y, cl.media.hudNumberMaterials[color][frame]);

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

			CL_DrawStringFixed(x+32, y, 8, 8, 0, ci->name, colorTable[COLOR_GREEN], cls.media.charsetMaterial, DSF_LEFT);
			CL_DrawStringFixed(x+32, y+8, 8, 8, 0, va("Score:  %i", score), colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_LEFT);
			CL_DrawStringFixed(x+32, y+16, 8, 8, 0, va("Ping:  %i", ping), colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_LEFT);
			CL_DrawStringFixed(x+32, y+24, 8, 8, 0, va("Time:  %i", time), colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_LEFT);
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
				CL_DrawStringFixed(x, y, 8, 8, 0, block, colorTable[COLOR_GREEN], cls.media.charsetMaterial, DSF_LEFT);
			else
				CL_DrawStringFixed(x, y, 8, 8, 0, block, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_LEFT);

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

			if (!cl.media.gameMaterials[index])
				continue;

			if (cl_drawIcons->integerValue)
				CL_DrawPicFixed(x, y, cl.media.gameMaterials[index]);

			continue;
		}
		if (!Q_stricmp(token, "picn")){
			// Draw a pic from a name
			token = Com_Parse(&string);
			if (cl_drawIcons->integerValue)
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

			CL_DrawStringFixed(x, y, 8, 8, 0, cl.configStrings[index], colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_LEFT);
			continue;
		}
		if (!Q_stricmp(token, "string")){
			token = Com_Parse(&string);
			CL_DrawStringFixed(x, y, 8, 8, 0, token, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_LEFT);
			continue;
		}
		if (!Q_stricmp(token, "string2")){
			token = Com_Parse(&string);
			CL_DrawStringFixed(x, y, 8, 8, 0, token, colorTable[COLOR_GREEN], cls.media.charsetMaterial, DSF_LEFT);
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

	if (cl.gameMod || !cl_newHUD->integerValue || !cl_hudDef.numItemDefs){
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
	CL_DrawStringFixed(x, y, 8, 8, 0, "hotkey ### item", colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_LEFT);
	CL_DrawStringFixed(x, y+8, 8, 8, 0, "------ --- ----", colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_LEFT);
	y += 16;
	for (i = top; i < num && i < top+DISPLAY_ITEMS; i++){
		item = index[i];

		// Search for a binding
		Q_snprintfz(binding, sizeof(binding), "use %s", cl.configStrings[CS_ITEMS+item]);
		bind = "";
		for (j = 0; j < 256; j++){
			if (!Q_stricmp(CL_GetKeyBinding(j), binding)){
				bind = CL_KeyNumToString(j);
				break;
			}
		}

		Q_snprintfz(string, sizeof(string), "%6s %3i %s", bind, cl.inventory[item], cl.configStrings[CS_ITEMS+item]);
		if (item != selected)
			CL_DrawStringFixed(x, y, 8, 8, 0, string, colorTable[COLOR_GREEN], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_LEFT);
		else {	
			// Draw a blinky cursor by the selected item
			if ((cls.realTime >> 8) & 1)
				CL_DrawStringFixed(x-8, y, 8, 8, 0, "\15", colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_LEFT);

			CL_DrawStringFixed(x, y, 8, 8, 0, string, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_LEFT);
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

	char	path[MAX_OSPATH], extension[8];

	// If currently playing another, stop it
	CL_StopCinematic();

	Com_DPrintf("CL_PlayCinematic( %s )\n", name);

	Com_FileExtension(name, extension, sizeof(extension));

	if (!Q_stricmp(extension, ".pcx")){
		Q_strncpyz(path, name, sizeof(path));
		Com_DefaultPath(path, sizeof(path), "pics");
	}
	else {
		Q_strncpyz(path, name, sizeof(path));
		Com_DefaultPath(path, sizeof(path), "video");
		Com_DefaultExtension(path, sizeof(path), ".cin");
	}

	// Play the cinematic
	cls.cinematicPlaying = CIN_PlayCinematic(path, CIN_SYSTEM);
	if (!cls.cinematicPlaying){
		Com_Printf("Cinematic %s not found\n", path);

		CL_FinishCinematic();
		return;
	}

	// Run the first frame
	if (CIN_RunCinematic())
		return;

	// Something went wrong
	CL_StopCinematic();
	CL_FinishCinematic();
}

/*
 =================
 CL_RunCinematic
 =================
*/
void CL_RunCinematic (void){

	if (!cls.cinematicPlaying)
		return;

	if (CIN_RunCinematic())
		return;

	CL_StopCinematic();
	CL_FinishCinematic();
}

/*
 =================
 CL_DrawCinematic
 =================
*/
void CL_DrawCinematic (void){

	if (!cls.cinematicPlaying)
		return;

	CIN_DrawCinematic(0, 0, 640, 480);
}

/*
 =================
 CL_StopCinematic
 =================
*/
void CL_StopCinematic (void){

	if (!cls.cinematicPlaying)
		return;

	Com_DPrintf("CL_StopCinematic()\n");

	CIN_StopCinematic();

	cls.cinematicPlaying = false;
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

	// Draw the loading screen
	CL_Loading();

	// Tell the server to advance to the next map / cinematic
	MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print(&cls.netChan.message, va("nextserver %i\n", cl.serverCount));
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

	VectorCopy(cl.renderView.viewOrigin, start);
	VectorMA(cl.renderView.viewOrigin, 8192, cl.renderView.viewAxis[0], end);

	trace = CL_Trace(start, vec3_origin, vec3_origin, end, cl.clientNum, MASK_PLAYERSOLID, false, &entNumber);
	if (trace.fraction == 0.0 || trace.fraction == 1.0)
		return;

	if (entNumber < 1 || entNumber > MAX_CLIENTS)
		return;		// Not a valid entity

	if (cl.entities[entNumber].current.modelindex != 255)
		return;		// Not a player, or invisible

	cl.crosshairEntTime = cl.time;
	cl.crosshairEntNumber = entNumber;
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

	if ((cl.playerState->rdflags & RDF_IRGOGGLES) || cl_thirdPerson->integerValue)
		return;

	// Select crosshair
	crosshair = (cl_drawCrosshair->integerValue - 1) % NUM_CROSSHAIRS;
	if (crosshair < 0)
		return;

	// Set dimensions and position
	w = cl_crosshairSize->integerValue;
	h = cl_crosshairSize->integerValue;

	x = cl_crosshairX->integerValue + ((640.0 - w) * 0.5);
	y = cl_crosshairY->integerValue + ((480.0 - h) * 0.5);

	// Set color and alpha
	if (cl_crosshairHealth->integerValue){
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

		color[3] = 255 * Clamp(cl_crosshairAlpha->floatValue, 0.0, 1.0);
	}
	else {
		color[0] = colorTable[cl_crosshairColor->integerValue & Q_COLOR_MASK][0];
		color[1] = colorTable[cl_crosshairColor->integerValue & Q_COLOR_MASK][1];
		color[2] = colorTable[cl_crosshairColor->integerValue & Q_COLOR_MASK][2];
		color[3] = 255 * Clamp(cl_crosshairAlpha->floatValue, 0.0, 1.0);
	}

	// Draw it
	CL_DrawPic(x, y, w, h, color, cl.media.crosshairMaterials[crosshair]);

	// Draw target name
	if (cl_crosshairNames->integerValue){
		if (!cl.multiPlayer)
			return;		// Don't bother in singleplayer

		// Scan for player entity
		CL_ScanForPlayerEntity();

		if (!cl.crosshairEntTime || !cl.crosshairEntNumber)
			return;

		ci = &cl.clientInfo[cl.crosshairEntNumber - 1];
		if (!ci->valid)
			return;

		// Set color and alpha
		if (cl_crosshairHealth->integerValue){
			color[0] = colorTable[cl_crosshairColor->integerValue & Q_COLOR_MASK][0];
			color[1] = colorTable[cl_crosshairColor->integerValue & Q_COLOR_MASK][1];
			color[2] = colorTable[cl_crosshairColor->integerValue & Q_COLOR_MASK][2];
		}

		fadeColor = CL_FadeAlpha(color, cl.crosshairEntTime, 1000, 250);
		if (!fadeColor){
			cl.crosshairEntTime = 0;
			cl.crosshairEntNumber = 0;
			return;
		}

		// Draw it
		CL_DrawString(0, y + (h*2), 10, 10, 640, ci->name, fadeColor, cls.media.charsetMaterial, DSF_DROPSHADOW|DSF_CENTER);
	}
}

/*
 =================
 CL_DrawCenterString
 =================
*/
static void CL_DrawCenterString (void){

	byte	*fadeColor;

	fadeColor = CL_FadeAlpha(colorTable[COLOR_WHITE], cl.centerPrintTime, cl_centerTime->integerValue, cl_centerTime->integerValue / 4);
	if (!fadeColor)
		return;

	CL_DrawString(0, 160, 10, 10, 640, cl.centerPrint, fadeColor, cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
}

/*
 =================
 CL_DrawLagometer
 =================
*/
static void CL_DrawLagometer (void){

	lagometer_t	*lagometer = &cl.lagometer;
	char		str[4];
	int			current, ping;
	int			i, j;

	// Don't draw if we're also the server
	if (Com_ServerState())
		return;

	current = (lagometer->current - 1) & (LAG_SAMPLES-1);

	// Update the lagometer image
	ping = lagometer->samples[current].ping * (float)(LAG_HEIGHT / 500.0);
	if (ping > LAG_HEIGHT)
		ping = LAG_HEIGHT;

	ping = LAG_HEIGHT - ping;

	for (i = 0; i < LAG_HEIGHT; i++)
		memmove((byte *)lagometer->image + LAG_WIDTH * 4 * i, (byte *)lagometer->image + LAG_WIDTH * 4 * i + 4, (LAG_WIDTH-1) * 4);

	j = LAG_WIDTH-1;
	for (i = 0; i < LAG_HEIGHT; i++){
		lagometer->image[i][j][0] = 0;
		lagometer->image[i][j][1] = 0;
		lagometer->image[i][j][2] = 0;
		lagometer->image[i][j][3] = 0;
	}

	if (lagometer->samples[current].dropped){
		for (i = ping; i < LAG_HEIGHT; i++){
			lagometer->image[i][j][0] = 255;
			lagometer->image[i][j][3] = 255;
		}
	}
	else if (lagometer->samples[current].suppressed){
		for (i = ping; i < LAG_HEIGHT; i++){
			lagometer->image[i][j][0] = 255;
			lagometer->image[i][j][1] = 255;
			lagometer->image[i][j][3] = 255;
		}
	}
	else {
		for (i = ping; i < LAG_HEIGHT; i++){
			lagometer->image[i][j][1] = 255;
			lagometer->image[i][j][3] = 255;
		}
	}

	if (!R_UpdateTexture("_scratch", (byte *)lagometer->image, LAG_WIDTH, LAG_HEIGHT))
		return;

	// Draw it
	CL_DrawPic(576, 416, 64, 64, colorTable[COLOR_WHITE], cl.media.lagometerMaterial);

	// Also draw the ping at the top
	ping = lagometer->samples[current].ping;
	if (ping > 999)
		ping = 999;

	Q_snprintfz(str, sizeof(str), "%i", ping);

	if (ping < 200)
		CL_DrawString(592, 432, 10, 10, 48, str, colorTable[COLOR_GREEN], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	else if (ping < 400)
		CL_DrawString(592, 432, 10, 10, 48, str, colorTable[COLOR_YELLOW], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	else
		CL_DrawString(592, 432, 10, 10, 48, str, colorTable[COLOR_RED], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
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

	CL_DrawString(0, 100, 16, 16, 640, "Connection Interrupted", colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);

	// Blink the icon
	if ((cl.time >> 9) & 1)
		return;

	CL_DrawPic(576, 416, 64, 64, colorTable[COLOR_WHITE], cl.media.disconnectedMaterial);
}

/*
 =================
 CL_DrawRecording
 =================
*/
static void CL_DrawRecording (void){

	char	str[MAX_OSPATH];

	if (!cls.demoFile)
		return;

	Q_snprintfz(str, sizeof(str), "Recording: %s (%i KB)", cls.demoName, FS_Tell(cls.demoFile) / 1024);
	CL_DrawString(0, 120, 10, 10, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
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
	int			i, total;
	int			fps;

	previousTimes[index % FPS_FRAMES] = cls.realTime - previousTime;
	previousTime = cls.realTime;
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
	if (fps > 999)
		fps = 999;

	Q_snprintfz(str, sizeof(str), "%3i FPS", fps);
	CL_DrawString(0, 0, 16, 16, 624, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_RIGHT);
}

/*
 =================
 CL_DrawPause
 =================
*/
static void CL_DrawPause (void){

	if (!com_paused->integerValue || UI_IsVisible())
		return;

	CL_DrawPic(0, 220, 640, 40, colorTable[COLOR_WHITE], cl.media.pauseMaterial);
}

/*
 =================
 CL_ShowMaterial

 Development tool
 =================
*/
static void CL_ShowMaterial (void){

	trace_t	trace;
	vec3_t	start, end;
	char	string[512];

	VectorCopy(cl.renderView.viewOrigin, start);
	VectorMA(cl.renderView.viewOrigin, 8192, cl.renderView.viewAxis[0], end);

	if (cl_showMaterial->integerValue == 1){
		trace = CL_Trace(start, vec3_origin, vec3_origin, end, cl.clientNum, MASK_SOLID | MASK_WATER, true, NULL);
		if (trace.fraction == 0.0 || trace.fraction == 1.0)
			return;
	}
	else {
		trace = CL_Trace(start, vec3_origin, vec3_origin, end, cl.clientNum, MASK_ALL, true, NULL);
		if (trace.fraction == 0.0 || trace.fraction == 1.0)
			return;
	}

	CL_DrawString(0, 300, 10, 10, 640, "MATERIAL", colorTable[COLOR_GREEN], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	CL_DrawString(0, 322, 10, 10, 640, "SURFACE FLAGS", colorTable[COLOR_GREEN], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	CL_DrawString(0, 344, 10, 10, 640, "CONTENTS FLAGS", colorTable[COLOR_GREEN], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);

	// Material
	Q_snprintfz(string, sizeof(string), "textures/%s", trace.surface->name);
	CL_DrawString(0, 310, 10, 10, 640, string, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);

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

		CL_DrawString(0, 332, 10, 10, 640, string, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
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

		CL_DrawString(0, 354, 10, 10, 640, string, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER);
	}
}


// =====================================================================


/*
 =================
 CL_Draw2D

 Draw all on-screen information
 =================
*/
void CL_Draw2D (void){

	if (!cl_draw2D->integerValue)
		return;

	// General game information
	if (cl_drawCrosshair->integerValue)
		CL_DrawCrosshair();

	if (cl_drawStatus->integerValue)
		CL_DrawStatus();

	if (cl_drawCenterString->integerValue)
		CL_DrawCenterString();

	if (cl_drawInventory->integerValue)
		CL_DrawInventory();

	if (cl_drawLayout->integerValue)
		CL_DrawLayout();

	if (cl_drawLagometer->integerValue)
		CL_DrawLagometer();

	if (cl_drawDisconnected->integerValue)
		CL_DrawDisconnected();

	if (cl_drawRecording->integerValue)
		CL_DrawRecording();

	if (cl_drawFPS->integerValue)
		CL_DrawFPS();

	if (cl_drawPause->integerValue)
		CL_DrawPause();

	// Development tool
	if (cl_showMaterial->integerValue)
		CL_ShowMaterial();
}

/*
 =================
 CL_UpdateScreen

 This is called every frame, and can also be called explicitly to flush
 stuff to the screen
 =================
*/
void CL_UpdateScreen (void){

	// Begin frame
	R_BeginFrame();

	switch (cls.state){
	case CA_DISCONNECTED:
		// Draw a cinematic or the menu
		if (cls.cinematicPlaying)
			CL_DrawCinematic();
		else
			UI_UpdateMenu(cls.realTime);

		break;
	case CA_CONNECTING:
	case CA_CHALLENGING:
	case CA_CONNECTED:
	case CA_LOADING:
	case CA_PRIMED:
		// Draw the loading screen and connection information
		CL_DrawLoading();

		break;
	case CA_ACTIVE:
		// If the menu covers the entire screen, draw it and forget
		// about the rest
		if (UI_IsFullscreen()){
			UI_UpdateMenu(cls.realTime);
			break;
		}

		// Draw a cinematic or game view
		if (cls.cinematicPlaying)
			CL_DrawCinematic();
		else
			CL_RenderActiveFrame();

		// Draw the menu, if visible
		UI_UpdateMenu(cls.realTime);

		break;
	}

	// The console will be drawn on top of anything
	Con_DrawConsole();

	// End frame
	R_EndFrame();
}
