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


/*
 =================
 CL_FadeColor
 =================
*/
byte *CL_FadeColor (const color_t color, int startTime, int totalTime, int fadeTime){

	static color_t	fadeColor;
	int				time;
	float			scale;

	time = cl.time - startTime;
	if (time >= totalTime)
		return NULL;

	if (totalTime - time < fadeTime && fadeTime != 0)
		scale = (float)(totalTime - time) * (1.0 / fadeTime);
	else
		scale = 1.0;

	fadeColor[0] = color[0] * scale;
	fadeColor[1] = color[1] * scale;
	fadeColor[2] = color[2] * scale;
	fadeColor[3] = color[3];

	return fadeColor;
}

/*
 =================
 CL_FadeAlpha
 =================
*/
byte *CL_FadeAlpha (const color_t color, int startTime, int totalTime, int fadeTime){

	static color_t	fadeColor;
	int				time;
	float			scale;

	time = cl.time - startTime;
	if (time >= totalTime)
		return NULL;

	if (totalTime - time < fadeTime && fadeTime != 0)
		scale = (float)(totalTime - time) * (1.0 / fadeTime);
	else
		scale = 1.0;

	fadeColor[0] = color[0];
	fadeColor[1] = color[1];
	fadeColor[2] = color[2];
	fadeColor[3] = color[3] * scale;

	return fadeColor;
}

/*
 =================
 CL_FadeColorAndAlpha
 =================
*/
byte *CL_FadeColorAndAlpha (const color_t color, int startTime, int totalTime, int fadeTime){

	static color_t	fadeColor;
	int				time;
	float			scale;

	time = cl.time - startTime;
	if (time >= totalTime)
		return NULL;

	if (totalTime - time < fadeTime && fadeTime != 0)
		scale = (float)(totalTime - time) * (1.0 / fadeTime);
	else
		scale = 1.0;

	fadeColor[0] = color[0] * scale;
	fadeColor[1] = color[1] * scale;
	fadeColor[2] = color[2] * scale;
	fadeColor[3] = color[3] * scale;

	return fadeColor;
}

/*
 =================
 CL_FillRect
 =================
*/
void CL_FillRect (float x, float y, float w, float h, const color_t color){

	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, color, cls.media.whiteMaterial);
}

/*
 =================
 CL_DrawString
 =================
*/
void CL_DrawString (float x, float y, float w, float h, float width, const char *string, const color_t color, struct material_s *fontMaterial, int flags){

	color_t	modulate, shadowModulate;
	char	line[1024], *l;
	int		i, len, ch;
	float	xx, yy, ofsX, ofsY, col, row;

	if (flags & DSF_DROPSHADOW){
		MakeRGBA(shadowModulate, 0, 0, 0, color[3]);

		ofsX = w * 0.125;
		ofsY = h * 0.125;
	}

	*(unsigned *)modulate = *(unsigned *)color;

	yy = y;
	while (*string){
		// Get a line of text
		len = 0;
		while (*string){
			if (*string == '\n' || *string == '\r'){
				string++;
				break;
			}
			else if (*string == '\t'){
				for (i = 0; i < 4; i++){
					line[len++] = ' ';

					if (len == sizeof(line)-1)
						break;
				}

				string++;
			}
			else
				line[len++] = *string++;

			if (len == sizeof(line)-1)
				break;
		}
		line[len] = 0;

		if (!len){
			yy += h;
			continue;
		}

		// Align the text as appropriate
		if (flags & DSF_LEFT)
			xx = x;
		if (flags & DSF_CENTER)
			xx = x + ((width - (Q_PrintStrlen(line) * w)) / 2);
		if (flags & DSF_RIGHT)
			xx = x + (width - (Q_PrintStrlen(line) * w));

		// Convert to lower/upper case if needed
		if (flags & DSF_LOWERCASE)
			Q_strlwr(line);
		if (flags & DSF_UPPERCASE)
			Q_strupr(line);

		// Draw it
		l = line;
		while (*l){
			if (Q_IsColorString(l)){
				if (!(flags & DSF_FORCECOLOR)){
					*(unsigned *)modulate = *(unsigned *)colorTable[Q_ColorIndex(*(l+1))];
					modulate[3] = color[3];
				}

				l += 2;
				continue;
			}

			ch = *l++;

			ch &= 255;
			if (ch != ' '){
				col = (ch & 15) * 0.0625;
				row = (ch >> 4) * 0.0625;

				if (flags & DSF_DROPSHADOW)
					R_DrawStretchPic(xx + ofsX, yy + ofsY, w, h, col, row, col + 0.0625, row + 0.0625, shadowModulate, fontMaterial);

				R_DrawStretchPic(xx, yy, w, h, col, row, col + 0.0625, row + 0.0625, modulate, fontMaterial);
			}

			xx += w;
		}

		yy += h;
	}
}

/*
 =================
 CL_DrawStringSheared
 =================
*/
void CL_DrawStringSheared (float x, float y, float w, float h, float shearX, float shearY, float width, const char *string, const color_t color, struct material_s *fontMaterial, int flags){

	color_t	modulate, shadowModulate;
	char	line[1024], *l;
	int		i, len, ch;
	float	xx, yy, ofsX, ofsY, col, row;

	if (flags & DSF_DROPSHADOW){
		MakeRGBA(shadowModulate, 0, 0, 0, color[3]);

		ofsX = w * 0.125;
		ofsY = h * 0.125;
	}

	*(unsigned *)modulate = *(unsigned *)color;

	yy = y;
	while (*string){
		// Get a line of text
		len = 0;
		while (*string){
			if (*string == '\n' || *string == '\r'){
				string++;
				break;
			}
			else if (*string == '\t'){
				for (i = 0; i < 4; i++){
					line[len++] = ' ';

					if (len == sizeof(line)-1)
						break;
				}

				string++;
			}
			else
				line[len++] = *string++;

			if (len == sizeof(line)-1)
				break;
		}
		line[len] = 0;

		if (!len){
			yy += h;
			continue;
		}

		// Align the text as appropriate
		if (flags & DSF_LEFT)
			xx = x;
		if (flags & DSF_CENTER)
			xx = x + ((width - (Q_PrintStrlen(line) * w)) / 2);
		if (flags & DSF_RIGHT)
			xx = x + (width - (Q_PrintStrlen(line) * w));

		// Convert to lower/upper case if needed
		if (flags & DSF_LOWERCASE)
			Q_strlwr(line);
		if (flags & DSF_UPPERCASE)
			Q_strupr(line);

		// Draw it
		l = line;
		while (*l){
			if (Q_IsColorString(l)){
				if (!(flags & DSF_FORCECOLOR)){
					*(unsigned *)modulate = *(unsigned *)colorTable[Q_ColorIndex(*(l+1))];
					modulate[3] = color[3];
				}

				l += 2;
				continue;
			}

			ch = *l++;

			ch &= 255;
			if (ch != ' '){
				col = (ch & 15) * 0.0625;
				row = (ch >> 4) * 0.0625;

				if (flags & DSF_DROPSHADOW)
					R_DrawShearedPic(xx + ofsX, yy + ofsY, w, h, col, row, col + 0.0625, row + 0.0625, shearX, shearY, shadowModulate, fontMaterial);

				R_DrawShearedPic(xx, yy, w, h, col, row, col + 0.0625, row + 0.0625, shearX, shearY, modulate, fontMaterial);
			}

			xx += w;
		}

		yy += h;
	}
}

/*
 =================
 CL_DrawStringFixed
 =================
*/
void CL_DrawStringFixed (float x, float y, float w, float h, float width, const char *string, const color_t color, struct material_s *fontMaterial, int flags){

	float	scaleX, scaleY;

	scaleX = 640.0 / cls.glConfig.videoWidth;
	scaleY = 480.0 / cls.glConfig.videoHeight;

	x *= scaleX;
	y *= scaleY;
	w *= scaleX;
	h *= scaleY;

	width *= scaleX;

	CL_DrawString(x, y, w, h, width, string, color, fontMaterial, flags);
}

/*
 =================
 CL_DrawStringShearedFixed
 =================
*/
void CL_DrawStringShearedFixed (float x, float y, float w, float h, float shearX, float shearY, float width, const char *string, const color_t color, struct material_s *fontMaterial, int flags){

	float	scaleX, scaleY;

	scaleX = 640.0 / cls.glConfig.videoWidth;
	scaleY = 480.0 / cls.glConfig.videoHeight;

	x *= scaleX;
	y *= scaleY;
	w *= scaleX;
	h *= scaleY;

	shearX *= scaleX;
	shearY *= scaleY;

	width *= scaleX;

	CL_DrawStringSheared(x, y, w, h, shearX, shearY, width, string, color, fontMaterial, flags);
}

/*
 =================
 CL_DrawPic
 =================
*/
void CL_DrawPic (float x, float y, float w, float h, const color_t color, struct material_s *material){

	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, color, material);
}

/*
 =================
 CL_DrawPicST
 =================
*/
void CL_DrawPicST (float x, float y, float w, float h, float s1, float t1, float s2, float t2, const color_t color, struct material_s *material){

	R_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, color, material);
}

/*
 =================
 CL_DrawPicSheared
 =================
*/
void CL_DrawPicSheared (float x, float y, float w, float h, float shearX, float shearY, const color_t color, struct material_s *material){

	R_DrawShearedPic(x, y, w, h, 0, 0, 1, 1, shearX, shearY, color, material);
}

/*
 =================
 CL_DrawPicShearedST
 =================
*/
void CL_DrawPicShearedST (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float shearX, float shearY, const color_t color, struct material_s *material){

	R_DrawShearedPic(x, y, w, h, s1, t1, s2, t2, shearX, shearY, color, material);
}

/*
 =================
 CL_DrawPicByName
 =================
*/
void CL_DrawPicByName (float x, float y, float w, float h, const color_t color, const char *pic){

	struct material_s	*material;
	char				name[MAX_OSPATH];

	if (!strchr(pic, '/'))
		Q_snprintfz(name, sizeof(name), "pics/%s", pic);
	else
		Com_StripExtension(pic, name, sizeof(name));

	material = R_RegisterMaterialNoMip(name);

	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, color, material);
}

/*
 =================
 CL_DrawPicFixed
 =================
*/
void CL_DrawPicFixed (float x, float y, struct material_s *material){

	float	scaleX, scaleY;
	float	w, h;

	R_GetPicSize(material, &w, &h);

	scaleX = 640.0 / cls.glConfig.videoWidth;
	scaleY = 480.0 / cls.glConfig.videoHeight;

	x *= scaleX;
	y *= scaleY;
	w *= scaleX;
	h *= scaleY;

	CL_DrawPic(x, y, w, h, colorTable[COLOR_WHITE], material);
}

/*
 =================
 CL_DrawPicFixedByName
 =================
*/
void CL_DrawPicFixedByName (float x, float y, const char *pic){

	struct material_s	*material;
	char				name[MAX_OSPATH];
	float				scaleX, scaleY;
	float				w, h;

	if (!strchr(pic, '/'))
		Q_snprintfz(name, sizeof(name), "pics/%s", pic);
	else
		Com_StripExtension(pic, name, sizeof(name));

	material = R_RegisterMaterialNoMip(name);

	R_GetPicSize(material, &w, &h);

	scaleX = 640.0 / cls.glConfig.videoWidth;
	scaleY = 480.0 / cls.glConfig.videoHeight;

	x *= scaleX;
	y *= scaleY;
	w *= scaleX;
	h *= scaleY;

	CL_DrawPic(x, y, w, h, colorTable[COLOR_WHITE], material);
}
