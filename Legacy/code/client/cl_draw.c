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
 CL_ScaleCoords
 =================
*/
void CL_ScaleCoords (float *x, float *y, float *w, float *h){

	*x *= cls.screenScaleX;
	*y *= cls.screenScaleY;
	*w *= cls.screenScaleX;
	*h *= cls.screenScaleY;
}

/*
 =================
 CL_DrawString
 =================
*/
void CL_DrawString (float x, float y, float w, float h, float offsetX, float offsetY, float width, const char *string, const color_t color, struct shader_s *fontShader, qboolean scale, int flags){

	color_t	modulate, shadowModulate;
	char	line[1024], *l;
	int		len, ch;
	float	xx, yy, ofsX, ofsY, col, row;

	if (scale){
		CL_ScaleCoords(&x, &y, &w, &h);

		offsetX *= cls.screenScaleX;
		offsetY *= cls.screenScaleY;

		width *= cls.screenScaleX;
	}

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
			if (*string == '\n'){
				string++;
				break;
			}

			line[len++] = *string++;

			if (len == sizeof(line)-1)
				break;
		}
		line[len] = 0;

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

				if (offsetX || offsetY){
					if (flags & DSF_DROPSHADOW)
						R_DrawOffsetPic(xx + ofsX, yy + ofsY, w, h, col, row, col + 0.0625, row + 0.0625, offsetX, offsetY, shadowModulate, fontShader);

					R_DrawOffsetPic(xx, yy, w, h, col, row, col + 0.0625, row + 0.0625, offsetX, offsetY, modulate, fontShader);
				}
				else {
					if (flags & DSF_DROPSHADOW)
						R_DrawStretchPic(xx + ofsX, yy + ofsY, w, h, col, row, col + 0.0625, row + 0.0625, shadowModulate, fontShader);

					R_DrawStretchPic(xx, yy, w, h, col, row, col + 0.0625, row + 0.0625, modulate, fontShader);
				}
			}

			xx += w;
		}

		yy += h;
	}
}

/*
 =================
 CL_FillRect
 =================
*/
void CL_FillRect (float x, float y, float w, float h, const color_t color){

	CL_ScaleCoords(&x, &y, &w, &h);
	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, color, cls.media.whiteShader);
}

/*
 =================
 CL_DrawPic
 =================
*/
void CL_DrawPic (float x, float y, float w, float h, const color_t color, struct shader_s *shader){

	CL_ScaleCoords(&x, &y, &w, &h);
	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, color, shader);
}

/*
 =================
 CL_DrawPicST
 =================
*/
void CL_DrawPicST (float x, float y, float w, float h, float sl, float tl, float sh, float th, const color_t color, struct shader_s *shader){

	CL_ScaleCoords(&x, &y, &w, &h);
	R_DrawStretchPic(x, y, w, h, sl, tl, sh, th, color, shader);
}

/*
 =================
 CL_DrawPicRotated
 =================
*/
void CL_DrawPicRotated (float x, float y, float w, float h, float angle, const color_t color, struct shader_s *shader){

	CL_ScaleCoords(&x, &y, &w, &h);
	R_DrawRotatedPic(x, y, w, h, 0, 0, 1, 1, angle, color, shader);
}

/*
 =================
 CL_DrawPicRotatedST
 =================
*/
void CL_DrawPicRotatedST (float x, float y, float w, float h, float sl, float tl, float sh, float th, float angle, const color_t color, struct shader_s *shader){

	CL_ScaleCoords(&x, &y, &w, &h);
	R_DrawRotatedPic(x, y, w, h, sl, tl, sh, th, angle, color, shader);
}

/*
 =================
 CL_DrawPicOffset
 =================
*/
void CL_DrawPicOffset (float x, float y, float w, float h, float offsetX, float offsetY, const color_t color, struct shader_s *shader){

	offsetX *= cls.screenScaleX;
	offsetY *= cls.screenScaleY;

	CL_ScaleCoords(&x, &y, &w, &h);
	R_DrawOffsetPic(x, y, w, h, 0, 0, 1, 1, offsetX, offsetY, color, shader);
}

/*
 =================
 CL_DrawPicOffsetST
 =================
*/
void CL_DrawPicOffsetST (float x, float y, float w, float h, float sl, float tl, float sh, float th, float offsetX, float offsetY, const color_t color, struct shader_s *shader){

	offsetX *= cls.screenScaleX;
	offsetY *= cls.screenScaleY;

	CL_ScaleCoords(&x, &y, &w, &h);
	R_DrawOffsetPic(x, y, w, h, sl, tl, sh, th, offsetX, offsetY, color, shader);
}

/*
 =================
 CL_DrawPicByName
 =================
*/
void CL_DrawPicByName (float x, float y, float w, float h, const color_t color, const char *pic){

	struct shader_s	*shader;
	char			name[MAX_QPATH];

	if (!strchr(pic, '/'))
		Q_snprintfz(name, sizeof(name), "pics/%s", pic);
	else
		Com_StripExtension(pic, name, sizeof(name));

	shader = R_RegisterShaderNoMip(name);

	CL_ScaleCoords(&x, &y, &w, &h);
	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, color, shader);
}

/*
 =================
 CL_DrawPicFixed
 =================
*/
void CL_DrawPicFixed (float x, float y, struct shader_s *shader){

	float	w, h;

	R_GetPicSize((const char *)shader, &w, &h);
	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, colorWhite, shader);
}

/*
 =================
 CL_DrawPicFixedByName
 =================
*/
void CL_DrawPicFixedByName (float x, float y, const char *pic){

	struct shader_s	*shader;
	char			name[MAX_QPATH];
	float			w, h;

	if (!strchr(pic, '/'))
		Q_snprintfz(name, sizeof(name), "pics/%s", pic);
	else
		Com_StripExtension(pic, name, sizeof(name));

	shader = R_RegisterShaderNoMip(name);

	R_GetPicSize(name, &w, &h);
	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, colorWhite, shader);
}
