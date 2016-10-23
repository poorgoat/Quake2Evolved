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


typedef struct {
	char		*suffix;
	char		*separator;
	qboolean	flipX;
	qboolean	flipY;
	qboolean	flipDiagonal;
} cubeSide_t;

typedef struct {
	char		*suffix;
	vec3_t		angles;
	qboolean	flipX;
	qboolean	flipY;
	qboolean	flipDiagonal;
} envShot_t;

static cubeSide_t	r_cubeSides[2][6] = {
	{
		{"px"	, "_"	, false	, false	, false	},
		{"nx"	, "_"	, false	, false	, false	},
		{"py"	, "_"	, false	, false	, false	},
		{"ny"	, "_"	, false	, false	, false	},
		{"pz"	, "_"	, false	, false	, false	},
		{"nz"	, "_"	, false	, false	, false	}
	},
	{
		{"rt"	, ""	, false	, false	, true	},
		{"lf"	, ""	, true	, true	, true	},
		{"bk"	, ""	, false	, true	, false	},
		{"ft"	, ""	, true	, false	, false	},
		{"up"	, ""	, false	, false	, true	},
		{"dn"	, ""	, false	, false	, true	}
	}
};

static envShot_t	r_envShot[6] = {
	{"px"	, {   0,   0,   0}	, true	, true	, true	},
	{"nx"	, {   0, 180,   0}	, false	, false	, true	},
	{"py"	, {   0,  90,   0}	, false	, true	, false	},
	{"ny"	, {   0, 270,   0}	, true	, false	, false	},
	{"pz"	, { -90, 180,   0}	, false	, false	, true	},
	{"nz"	, {  90, 180,	0}	, false	, false	, true	}
};

static int			r_textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
static int			r_textureMagFilter = GL_LINEAR;

static vec3_t		r_luminanceTable[256];
static unsigned		r_palette[256];

static texture_t	*r_texturesHashTable[TEXTURES_HASH_SIZE];
static texture_t	*r_textures[MAX_TEXTURES];
static int			r_numTextures;


/*
 =======================================================================

 PCX LOADING

 =======================================================================
*/


/*
 =================
 R_LoadPCX
 =================
*/
static qboolean R_LoadPCX (const char *name, byte **image, byte **palette, int *width, int *height, int *samples){

	byte		*buffer;
	byte		*in, *out;
	pcxHeader_t	*pcx;
	int			x, y, len;
	int			dataByte, runLength;
	qboolean	hasColor = false, hasAlpha = false;

	// Load the file
	len = FS_LoadFile(name, (void **)&buffer);
	if (!buffer)
		return false;

	// Parse the PCX file
	pcx = (pcxHeader_t *)buffer;

    pcx->xMin = LittleShort(pcx->xMin);
    pcx->yMin = LittleShort(pcx->yMin);
    pcx->xMax = LittleShort(pcx->xMax);
    pcx->yMax = LittleShort(pcx->yMax);
    pcx->hRes = LittleShort(pcx->hRes);
    pcx->vRes = LittleShort(pcx->vRes);
    pcx->bytesPerLine = LittleShort(pcx->bytesPerLine);
    pcx->paletteType = LittleShort(pcx->paletteType);

	in = &pcx->data;

	if (pcx->manufacturer != 0x0A || pcx->version != 5 || pcx->encoding != 1){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadPCX: invalid PCX header (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	if (pcx->bitsPerPixel != 8 || pcx->colorPlanes != 1){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadPCX: only 8 bit PCX images supported (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}
		
	if (pcx->xMax == 0 || pcx->yMax == 0 || pcx->xMax >= 640 || pcx->yMax >= 480){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadPCX: bad PCX file (%i x %i) (%s)\n", pcx->xMax, pcx->yMax, name);
		FS_FreeFile(buffer);
		return false;
	}

	if (palette){
		// Only the palette was requested
		*palette = Z_Malloc(768);
		memcpy(*palette, (byte *)buffer + len - 768, 768);

		FS_FreeFile(buffer);
		return true;
	}

	*width = pcx->xMax+1;
	*height = pcx->yMax+1;

	*image = out = Z_Malloc((pcx->xMax+1) * (pcx->yMax+1) * 4);

	for (y = 0; y <= pcx->yMax; y++){
		for (x = 0; x <= pcx->xMax; ){
			dataByte = *in++;

			if ((dataByte & 0xC0) == 0xC0){
				runLength = dataByte & 0x3F;
				dataByte = *in++;
			}
			else
				runLength = 1;

			while (runLength-- > 0){
				*(unsigned *)out = r_palette[dataByte];

				if (out[0] != out[1] || out[1] != out[2])
					hasColor = true;

				if (out[3] != 255)
					hasAlpha = true;

				out += 4;
				x++;
			}
		}
	}

	if (in - buffer > len){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadPCX: PCX file was malformed (%s)\n", name);
		FS_FreeFile(buffer);
		Z_Free(*image);
		*image = NULL;
		return false;
	}

	if (hasColor)
		*samples = (hasAlpha) ? 4 : 3;
	else
		*samples = (hasAlpha) ? 2 : 1;

	FS_FreeFile(buffer);
	return true;
}


/*
 =======================================================================

 WAL LOADING

 =======================================================================
*/


/*
 =================
 R_LoadWAL
 =================
*/
static qboolean R_LoadWAL (const char *name, byte **image, int *width, int *height, int *samples){
	
	byte		*buffer;
	byte		*in, *out;
	mipTex_t	*mt;
	int			i, c;
	qboolean	hasColor = false, hasAlpha = false;

	// Load the file
	FS_LoadFile(name, (void **)&buffer);
	if (!buffer)
		return false;

	// Parse the WAL file
	mt = (mipTex_t *)buffer;

	mt->width = LittleLong(mt->width);
	mt->height = LittleLong(mt->height);
	mt->offsets[0] = LittleLong(mt->offsets[0]);

	if (mt->width == 0 || mt->height == 0 || mt->width > 4096 || mt->height > 4096){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadWAL: bad WAL file (%i x %i) (%s)\n", mt->width, mt->height, name);
		FS_FreeFile(buffer);
		return false;
	}

	*width = mt->width;
	*height = mt->height;

	*image = out = Z_Malloc(mt->width * mt->height * 4);

	in = buffer + mt->offsets[0];
	c = mt->width * mt->height;

	for (i = 0; i < c; i++, in++, out += 4){
		*(unsigned *)out = r_palette[*in];

		if (out[0] != out[1] || out[1] != out[2])
			hasColor = true;

		if (out[3] != 255)
			hasAlpha = true;
	}

	if (hasColor)
		*samples = (hasAlpha) ? 4 : 3;
	else
		*samples = (hasAlpha) ? 2 : 1;

	FS_FreeFile(buffer);
	return true;
}


/*
 =======================================================================

 TARGA LOADING

 =======================================================================
*/


/*
 =================
 R_LoadTGA
 =================
*/
static qboolean R_LoadTGA (const char *name, byte **image, int *width, int *height, int *samples){

	byte			*buffer;
	byte			*in, *out;
	targaHeader_t	tga;
	int				w, h, stride = 0;
	byte			r, g, b, a;
	byte			i, packetHeader, packetSize;
	qboolean		hasColor = false, hasAlpha = false;

	// Load the file
	FS_LoadFile(name, (void **)&buffer);
	if (!buffer)
		return false;

	// Parse the TGA file
	in = buffer;

	tga.idLength = *in++;
	tga.colormapType = *in++;
	tga.imageType = *in++;
	tga.colormapIndex = LittleShort(*(short *)in);
	in += 2;
	tga.colormapLength = LittleShort(*(short *)in);
	in += 2;
	tga.colormapSize = *in++;
	tga.xOrigin = LittleShort(*(short *)in);
	in += 2;
	tga.yOrigin = LittleShort(*(short *)in);
	in += 2;
	tga.width = LittleShort(*(short *)in);
	in += 2;
	tga.height = LittleShort(*(short *)in);
	in += 2;
	tga.pixelSize = *in++;
	tga.attributes = *in++;

	if (tga.imageType != 2 && tga.imageType != 3 && tga.imageType != 10 && tga.imageType != 11){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadTGA: only type 2 (RGB), 3 (gray), 10 (RLE RGB), and 11 (RLE gray) TGA images supported (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	if (tga.colormapType != 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadTGA: colormaps not supported (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}
		
	if (tga.pixelSize != 32 && tga.pixelSize != 24 && tga.pixelSize != 8){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadTGA: only 32 (RGBA), 24 (RGB), and 8 (gray) bit TGA images supported (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	if (tga.width == 0 || tga.height == 0 || tga.width > 4096 || tga.height > 4096){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadTGA: bad TGA file (%i x %i) (%s)\n", tga.width, tga.height, name);
		FS_FreeFile(buffer);
		return false;
	}

	*width = tga.width;
	*height = tga.height;

	*image = out = Z_Malloc(tga.width * tga.height * 4);

	if (tga.idLength != 0)
		in += tga.idLength;		// Skip TGA image comment

	if (!(tga.attributes & 0x20)){
		// Flipped image
		out += (tga.height-1) * tga.width * 4;
		stride = -tga.width * 4 * 2;
	}
	
	if (tga.imageType == 2 || tga.imageType == 3){
		// Uncompressed RGBA, RGB or grayscale image
		for (h = 0; h < tga.height; h++){
			for (w = 0; w < tga.width; w++){
				switch (tga.pixelSize){
				case 8:
					out[0] = in[0];
					out[1] = in[0];
					out[2] = in[0];
					out[3] = 255;

					in += 1;
					break;
				case 24:
					out[0] = in[2];
					out[1] = in[1];
					out[2] = in[0];
					out[3] = 255;

					in += 3;
					break;
				case 32:
					out[0] = in[2];
					out[1] = in[1];
					out[2] = in[0];
					out[3] = in[3];

					in += 4;
					break;
				}

				if (out[0] != out[1] || out[1] != out[2])
					hasColor = true;

				if (out[3] != 255)
					hasAlpha = true;

				out += 4;
			}

			out += stride;
		}
	}
	else if (tga.imageType == 10 || tga.imageType == 11){
		// Run-Length encoded RGBA, RGB or grayscale image
		for (h = 0; h < tga.height; h++){
			for (w = 0; w < tga.width; ){
				packetHeader = *in++;
				packetSize = 1 + (packetHeader & 0x7F);
				
				if (packetHeader & 0x80){        // Run-Length packet
					switch (tga.pixelSize){
					case 8:
						r = in[0];
						g = in[0];
						b = in[0];
						a = 255;

						in += 1;
						break;
					case 24:
						r = in[2];
						g = in[1];
						b = in[0];
						a = 255;

						in += 3;
						break;
					case 32:
						r = in[2];
						g = in[1];
						b = in[0];
						a = in[3];

						in += 4;
						break;
					}

					if (r != g || g != b)
						hasColor = true;

					if (a != 255)
						hasAlpha = true;
	
					for (i = 0; i < packetSize; i++){
						out[0] = r;
						out[1] = g;
						out[2] = b;
						out[3] = a;

						out += 4;

						w++;
						if (w == tga.width){	// Run spans across rows
							w = 0;
							if (h < tga.height-1)
								h++;
							else
								goto breakOut;

							out += stride;
						}
					}
				}
				else {							// Non Run-Length packet
					for (i = 0; i < packetSize; i++){
						switch (tga.pixelSize){
						case 8:
							out[0] = in[0];
							out[1] = in[0];
							out[2] = in[0];
							out[3] = 255;

							in += 1;
							break;
						case 24:
							out[0] = in[2];
							out[1] = in[1];
							out[2] = in[0];
							out[3] = 255;

							in += 3;
							break;
						case 32:
							out[0] = in[2];
							out[1] = in[1];
							out[2] = in[0];
							out[3] = in[3];

							in += 4;
							break;
						}

						if (out[0] != out[1] || out[1] != out[2])
							hasColor = true;

						if (out[3] != 255)
							hasAlpha = true;

						out += 4;

						w++;
						if (w == tga.width){	// Run spans across rows
							w = 0;
							if (h < tga.height-1)
								h++;
							else
								goto breakOut;

							out += stride;
						}						
					}
				}
			}

			out += stride;

breakOut:
			;
		}
	}

	if (hasColor)
		*samples = (hasAlpha) ? 4 : 3;
	else
		*samples = (hasAlpha) ? 2 : 1;

	FS_FreeFile(buffer);
	return true;
}


/*
 =======================================================================

 IMAGE PROGRAM FUNCTIONS

 =======================================================================
*/

static byte	*R_LoadImage (script_t *script, const char *name, int *width, int *height, int *samples, textureFlags_t *flags);


/*
 =================
 R_AddImages

 Adds the given images together
 =================
*/
static byte *R_AddImages (byte *in1, byte *in2, int width, int height){

	byte	*out = in1;
	int		r, g, b, a;
	int		x, y;

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++){
			r = in1[4*(y*width+x)+0] + in2[4*(y*width+x)+0];
			g = in1[4*(y*width+x)+1] + in2[4*(y*width+x)+1];
			b = in1[4*(y*width+x)+2] + in2[4*(y*width+x)+2];
			a = in1[4*(y*width+x)+3] + in2[4*(y*width+x)+3];

			out[4*(y*width+x)+0] = Clamp(r, 0, 255);
			out[4*(y*width+x)+1] = Clamp(g, 0, 255);
			out[4*(y*width+x)+2] = Clamp(b, 0, 255);
			out[4*(y*width+x)+3] = Clamp(a, 0, 255);
		}
	}

	Z_Free(in2);

	return out;
}

/*
 =================
 R_MultiplyImages

 Multiplies the given images
 =================
*/
static byte *R_MultiplyImages (byte *in1, byte *in2, int width, int height){

	byte	*out = in1;
	int		r, g, b, a;
	int		x, y;

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++){
			r = in1[4*(y*width+x)+0] * (in2[4*(y*width+x)+0] * (1.0/255));
			g = in1[4*(y*width+x)+1] * (in2[4*(y*width+x)+1] * (1.0/255));
			b = in1[4*(y*width+x)+2] * (in2[4*(y*width+x)+2] * (1.0/255));
			a = in1[4*(y*width+x)+3] * (in2[4*(y*width+x)+3] * (1.0/255));

			out[4*(y*width+x)+0] = Clamp(r, 0, 255);
			out[4*(y*width+x)+1] = Clamp(g, 0, 255);
			out[4*(y*width+x)+2] = Clamp(b, 0, 255);
			out[4*(y*width+x)+3] = Clamp(a, 0, 255);
		}
	}

	Z_Free(in2);

	return out;
}

/*
 =================
 R_BiasImage

 Biases the given image
 =================
*/
static byte *R_BiasImage (byte *in, int width, int height, const vec4_t bias){

	int		x, y;
	byte	*out = in;
	int		r, g, b, a;

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++){
			r = in[4*(y*width+x)+0] + (255 * bias[0]);
			g = in[4*(y*width+x)+1] + (255 * bias[1]);
			b = in[4*(y*width+x)+2] + (255 * bias[2]);
			a = in[4*(y*width+x)+3] + (255 * bias[3]);

			out[4*(y*width+x)+0] = Clamp(r, 0, 255);
			out[4*(y*width+x)+1] = Clamp(g, 0, 255);
			out[4*(y*width+x)+2] = Clamp(b, 0, 255);
			out[4*(y*width+x)+3] = Clamp(a, 0, 255);
		}
	}

	return out;
}

/*
 =================
 R_ScaleImage

 Scales the given image
 =================
*/
static byte *R_ScaleImage (byte *in, int width, int height, const vec4_t scale){

	byte	*out = in;
	int		r, g, b, a;
	int		x, y;

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++){
			r = in[4*(y*width+x)+0] * scale[0];
			g = in[4*(y*width+x)+1] * scale[1];
			b = in[4*(y*width+x)+2] * scale[2];
			a = in[4*(y*width+x)+3] * scale[3];

			out[4*(y*width+x)+0] = Clamp(r, 0, 255);
			out[4*(y*width+x)+1] = Clamp(g, 0, 255);
			out[4*(y*width+x)+2] = Clamp(b, 0, 255);
			out[4*(y*width+x)+3] = Clamp(a, 0, 255);
		}
	}

	return out;
}

/*
 =================
 R_InvertColor

 Inverts the color channels of the given image
 =================
*/
static byte *R_InvertColor (byte *in, int width, int height){

	byte	*out = in;
	int		x, y;

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++){
			out[4*(y*width+x)+0] = 255 - in[4*(y*width+x)+0];
			out[4*(y*width+x)+1] = 255 - in[4*(y*width+x)+1];
			out[4*(y*width+x)+2] = 255 - in[4*(y*width+x)+2];
		}
	}

	return out;
}

/*
 =================
 R_InvertAlpha

 Inverts the alpha channel of the given image
 =================
*/
static byte *R_InvertAlpha (byte *in, int width, int height){

	byte	*out = in;
	int		x, y;

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++)
			out[4*(y*width+x)+3] = 255 - in[4*(y*width+x)+3];
	}

	return out;
}

/*
 =================
 R_MakeIntensity

 Converts the given image to intensity
 =================
*/
static byte *R_MakeIntensity (byte *in, int width, int height){

	byte	*out = in;
	byte	intensity;
	float	r, g, b;
	int		x, y;

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++){
			r = r_luminanceTable[in[4*(y*width+x)+0]][0];
			g = r_luminanceTable[in[4*(y*width+x)+1]][1];
			b = r_luminanceTable[in[4*(y*width+x)+2]][2];

			intensity = (byte)(r + g + b);

			out[4*(y*width+x)+0] = intensity;
			out[4*(y*width+x)+1] = intensity;
			out[4*(y*width+x)+2] = intensity;
			out[4*(y*width+x)+3] = intensity;
		}
	}

	return out;
}

/*
 =================
 R_MakeLuminance

 Converts the given image to luminance
 =================
*/
static byte *R_MakeLuminance (byte *in, int width, int height){

	byte	*out = in;
	byte	luminance;
	float	r, g, b;
	int		x, y;

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++){
			r = r_luminanceTable[in[4*(y*width+x)+0]][0];
			g = r_luminanceTable[in[4*(y*width+x)+1]][1];
			b = r_luminanceTable[in[4*(y*width+x)+2]][2];

			luminance = (byte)(r + g + b);

			out[4*(y*width+x)+0] = luminance;
			out[4*(y*width+x)+1] = luminance;
			out[4*(y*width+x)+2] = luminance;
			out[4*(y*width+x)+3] = 255;
		}
	}

	return out;
}

/*
 =================
 R_MakeAlpha

 Converts the given image to alpha
 =================
*/
static byte *R_MakeAlpha (byte *in, int width, int height){

	byte	*out = in;
	byte	alpha;
	float	r, g, b;
	int		x, y;

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++){
			r = r_luminanceTable[in[4*(y*width+x)+0]][0];
			g = r_luminanceTable[in[4*(y*width+x)+1]][1];
			b = r_luminanceTable[in[4*(y*width+x)+2]][2];

			alpha = (byte)(r + g + b);

			out[4*(y*width+x)+0] = 255;
			out[4*(y*width+x)+1] = 255;
			out[4*(y*width+x)+2] = 255;
			out[4*(y*width+x)+3] = alpha;
		}
	}

	return out;
}

/*
 =================
 R_HeightMap

 Converts the given height map to a normal map
 =================
*/
static byte *R_HeightMap (byte *in, int width, int height, float scale){

	byte	*out;
	vec3_t	normal;
	float	r, g, b;
	float	c, cx, cy;
	int		x, y;

	out = Z_Malloc(width * height * 4);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++){
			r = r_luminanceTable[in[4*(y*width+x)+0]][0];
			g = r_luminanceTable[in[4*(y*width+x)+1]][1];
			b = r_luminanceTable[in[4*(y*width+x)+2]][2];

			c = (r + g + b) * (1.0/255);

			r = r_luminanceTable[in[4*(y*width+((x+1)%width))+0]][0];
			g = r_luminanceTable[in[4*(y*width+((x+1)%width))+1]][1];
			b = r_luminanceTable[in[4*(y*width+((x+1)%width))+2]][2];

			cx = (r + g + b) * (1.0/255);

			r = r_luminanceTable[in[4*(((y+1)%height)*width+x)+0]][0];
			g = r_luminanceTable[in[4*(((y+1)%height)*width+x)+1]][1];
			b = r_luminanceTable[in[4*(((y+1)%height)*width+x)+2]][2];

			cy = (r + g + b) * (1.0/255);

			normal[0] = (c - cx) * scale;
			normal[1] = (c - cy) * scale;
			normal[2] = 1.0;

			if (!VectorNormalize(normal))
				VectorSet(normal, 0.0, 0.0, 1.0);

			out[4*(y*width+x)+0] = (byte)(128 + 127 * normal[0]);
			out[4*(y*width+x)+1] = (byte)(128 + 127 * normal[1]);
			out[4*(y*width+x)+2] = (byte)(128 + 127 * normal[2]);
			out[4*(y*width+x)+3] = 255;
		}
	}

	Z_Free(in);

	return out;
}

/*
 =================
 R_AddNormals

 Adds the given normal maps together
 =================
*/
static byte *R_AddNormals (byte *in1, byte *in2, int width, int height){

	byte	*out;
	vec3_t	normal;
	int		x, y;

	out = Z_Malloc(width * height * 4);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++){
			normal[0] = (in1[4*(y*width+x)+0] * (1.0/127) - 1.0) + (in2[4*(y*width+x)+0] * (1.0/127) - 1.0);
			normal[1] = (in1[4*(y*width+x)+1] * (1.0/127) - 1.0) + (in2[4*(y*width+x)+1] * (1.0/127) - 1.0);
			normal[2] = (in1[4*(y*width+x)+2] * (1.0/127) - 1.0) + (in2[4*(y*width+x)+2] * (1.0/127) - 1.0);

			if (!VectorNormalize(normal))
				VectorSet(normal, 0.0, 0.0, 1.0);

			out[4*(y*width+x)+0] = (byte)(128 + 127 * normal[0]);
			out[4*(y*width+x)+1] = (byte)(128 + 127 * normal[1]);
			out[4*(y*width+x)+2] = (byte)(128 + 127 * normal[2]);
			out[4*(y*width+x)+3] = 255;
		}
	}

	Z_Free(in1);
	Z_Free(in2);

	return out;
}

/*
 =================
 R_SmoothNormals

 Smoothes the given normal map
 =================
*/
static byte *R_SmoothNormals (byte *in, int width, int height){

	byte		*out;
	unsigned	frac, fracStep;
	unsigned	p1[0x1000], p2[0x1000];
	unsigned	*inRow1, *inRow2;
	byte		*pix1, *pix2, *pix3, *pix4;
	vec3_t		normal;
	int			i, x, y;

	out = Z_Malloc(width * height * 4);

	fracStep = 0x10000;

	frac = fracStep >> 2;
	for (i = 0; i < width; i++){
		p1[i] = 4 * (frac >> 16);
		frac += fracStep;
	}

	frac = (fracStep >> 2) * 3;
	for (i = 0; i < width; i++){
		p2[i] = 4 * (frac >> 16);
		frac += fracStep;
	}

	for (y = 0; y < height; y++){
		inRow1 = (unsigned *)in + width * (int)((float)y + 0.25);
		inRow2 = (unsigned *)in + width * (int)((float)y + 0.75);

		for (x = 0; x < width; x++){
			pix1 = (byte *)inRow1 + p1[x];
			pix2 = (byte *)inRow1 + p2[x];
			pix3 = (byte *)inRow2 + p1[x];
			pix4 = (byte *)inRow2 + p2[x];

			normal[0] = (pix1[0] * (1.0/127) - 1.0) + (pix2[0] * (1.0/127) - 1.0) + (pix3[0] * (1.0/127) - 1.0) + (pix4[0] * (1.0/127) - 1.0);
			normal[1] = (pix1[1] * (1.0/127) - 1.0) + (pix2[1] * (1.0/127) - 1.0) + (pix3[1] * (1.0/127) - 1.0) + (pix4[1] * (1.0/127) - 1.0);
			normal[2] = (pix1[2] * (1.0/127) - 1.0) + (pix2[2] * (1.0/127) - 1.0) + (pix3[2] * (1.0/127) - 1.0) + (pix4[2] * (1.0/127) - 1.0);

			if (!VectorNormalize(normal))
				VectorSet(normal, 0.0, 0.0, 1.0);

			out[4*(y*width+x)+0] = (byte)(128 + 127 * normal[0]);
			out[4*(y*width+x)+1] = (byte)(128 + 127 * normal[1]);
			out[4*(y*width+x)+2] = (byte)(128 + 127 * normal[2]);
			out[4*(y*width+x)+3] = 255;
		}
	}

	Z_Free(in);

	return out;
}

/*
 =================
 R_FlipImage

 Flips the given image
 =================
*/
static byte *R_FlipImage (byte *in, int width, int height, int samples, qboolean flipX, qboolean flipY, qboolean flipDiagonal){

	byte	*out;
	byte	*pIn, *pOut;
	byte	*line;
	int		xStride, xOffset;
	int		yStride, yOffset;
	int		i, x, y;

	out = pOut = Z_Malloc(width * height * samples);

	xStride = (flipX) ? -samples : samples;
	xOffset = (flipX) ? (width - 1) * samples : 0;

	yStride = (flipY) ? -samples * width : samples * width;
	yOffset = (flipY) ? (height - 1) * width * samples : 0;

	if (flipDiagonal){
		for (x = 0, line = in + xOffset; x < width; x++, line += xStride){
			for (y = 0, pIn = line + yOffset; y < height; y++, pIn += yStride, pOut += samples){
				for (i = 0; i < samples; i++)
					pOut[i] = pIn[i];
			}
		}
	}
	else {
		for (y = 0, line = in + yOffset; y < height; y++, line += yStride){
			for (x = 0, pIn = line + xOffset; x < width; x++, pIn += xStride, pOut += samples){
				for (i = 0; i < samples; i++)
					pOut[i] = pIn[i];
			}
		}
	}

	Z_Free(in);

	return out;
}

/*
 =================
 R_ParseAdd
 =================
*/
static byte *R_ParseAdd (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image1, *image2;
	int		width1, height1, samples1, width2, height2, samples2;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'add'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'add'\n");
		return NULL;
	}

	image1 = R_LoadImage(script, token.string, &width1, &height1, &samples1, flags);
	if (!image1)
		return NULL;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ",")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'add'\n", token.string);

		Z_Free(image1);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'add'\n");

		Z_Free(image1);
		return NULL;
	}

	image2 = R_LoadImage(script, token.string, &width2, &height2, &samples2, flags);
	if (!image2){
		Z_Free(image1);
		return NULL;
	}

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'add'\n", token.string);

		Z_Free(image1);
		Z_Free(image2);
		return NULL;
	}

	if (width1 != width2 || height1 != height2){
		Com_Printf(S_COLOR_YELLOW "WARNING: images for 'add' have mismatched dimensions (%i x %i != %i x %i)\n", width1, height1, width2, height2);

		Z_Free(image1);
		Z_Free(image2);
		return NULL;
	}

	*width = width1;
	*height = height1;

	if (samples1 == 1)
		*samples = samples2;
	else if (samples1 == 2){
		if (samples2 == 3 || samples2 == 4)
			*samples = 4;
		else
			*samples = 2;
	}
	else if (samples1 == 3){
		if (samples2 == 2 || samples2 == 4)
			*samples = 4;
		else
			*samples = 3;
	}
	else
		*samples = samples1;

	if (*samples != 1){
		*flags &= ~TF_INTENSITY;
		*flags &= ~TF_ALPHA;
	}

	return R_AddImages(image1, image2, *width, *height);
}

/*
 =================
 R_ParseMultiply
 =================
*/
static byte *R_ParseMultiply (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image1, *image2;
	int		width1, height1, samples1, width2, height2, samples2;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'multiply'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'multiply'\n");
		return NULL;
	}

	image1 = R_LoadImage(script, token.string, &width1, &height1, &samples1, flags);
	if (!image1)
		return NULL;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ",")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'multiply'\n", token.string);

		Z_Free(image1);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'multiply'\n");

		Z_Free(image1);
		return NULL;
	}

	image2 = R_LoadImage(script, token.string, &width2, &height2, &samples2, flags);
	if (!image2){
		Z_Free(image1);
		return NULL;
	}

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'multiply'\n", token.string);

		Z_Free(image1);
		Z_Free(image2);
		return NULL;
	}

	if (width1 != width2 || height1 != height2){
		Com_Printf(S_COLOR_YELLOW "WARNING: images for 'multiply' have mismatched dimensions (%i x %i != %i x %i)\n", width1, height1, width2, height2);

		Z_Free(image1);
		Z_Free(image2);
		return NULL;
	}

	*width = width1;
	*height = height1;

	if (samples1 == 1)
		*samples = samples2;
	else if (samples1 == 2){
		if (samples2 == 3 || samples2 == 4)
			*samples = 4;
		else
			*samples = 2;
	}
	else if (samples1 == 3){
		if (samples2 == 2 || samples2 == 4)
			*samples = 4;
		else
			*samples = 3;
	}
	else
		*samples = samples1;

	if (*samples != 1){
		*flags &= ~TF_INTENSITY;
		*flags &= ~TF_ALPHA;
	}

	return R_MultiplyImages(image1, image2, *width, *height);
}

/*
 =================
 R_ParseBias
 =================
*/
static byte *R_ParseBias (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image;
	vec4_t	bias;
	int		i;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'bias'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'bias'\n");
		return NULL;
	}

	image = R_LoadImage(script, token.string, width, height, samples, flags);
	if (!image)
		return NULL;

	for (i = 0; i < 4; i++){
		PS_ReadToken(script, 0, &token);
		if (Q_stricmp(token.string, ",")){
			Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'bias'\n", token.string);

			Z_Free(image);
			return NULL;
		}

		if (!PS_ReadFloat(script, 0, &bias[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'bias'\n");

			Z_Free(image);
			return NULL;
		}
	}

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'bias'\n", token.string);

		Z_Free(image);
		return NULL;
	}

	if (*samples < 3)
		*samples += 2;

	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_ALPHA;

	return R_BiasImage(image, *width, *height, bias);
}

/*
 =================
 R_ParseScale
 =================
*/
static byte *R_ParseScale (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image;
	vec4_t	scale;
	int		i;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'scale'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'scale'\n");
		return NULL;
	}

	image = R_LoadImage(script, token.string, width, height, samples, flags);
	if (!image)
		return NULL;

	for (i = 0; i < 4; i++){
		PS_ReadToken(script, 0, &token);
		if (Q_stricmp(token.string, ",")){
			Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'scale'\n", token.string);

			Z_Free(image);
			return NULL;
		}

		if (!PS_ReadFloat(script, 0, &scale[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'scale'\n");

			Z_Free(image);
			return NULL;
		}
	}

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'scale'\n", token.string);

		Z_Free(image);
		return NULL;
	}

	if (*samples < 3)
		*samples += 2;

	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_ALPHA;

	return R_ScaleImage(image, *width, *height, scale);
}

/*
 =================
 R_ParseInvertColor
 =================
*/
static byte *R_ParseInvertColor (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'invertColor'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'invertColor'\n");
		return NULL;
	}

	image = R_LoadImage(script, token.string, width, height, samples, flags);
	if (!image)
		return NULL;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'invertColor'\n", token.string);

		Z_Free(image);
		return NULL;
	}

	return R_InvertColor(image, *width, *height);
}

/*
 =================
 R_ParseInvertAlpha
 =================
*/
static byte *R_ParseInvertAlpha (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'invertAlpha'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'invertAlpha'\n");
		return NULL;
	}

	image = R_LoadImage(script, token.string, width, height, samples, flags);
	if (!image)
		return NULL;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'invertAlpha'\n", token.string);

		Z_Free(image);
		return NULL;
	}

	return R_InvertAlpha(image, *width, *height);
}

/*
 =================
 R_ParseMakeIntensity
 =================
*/
static byte *R_ParseMakeIntensity (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'makeIntensity'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'makeIntensity'\n");
		return NULL;
	}

	image = R_LoadImage(script, token.string, width, height, samples, flags);
	if (!image)
		return NULL;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'makeIntensity'\n", token.string);

		Z_Free(image);
		return NULL;
	}

	*samples = 1;

	*flags |= TF_INTENSITY;
	*flags &= ~TF_ALPHA;
	*flags &= ~TF_NORMALMAP;

	return R_MakeIntensity(image, *width, *height);
}

/*
 =================
 R_ParseMakeLuminance
 =================
*/
static byte *R_ParseMakeLuminance (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'makeLuminance'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'makeLuminance'\n");
		return NULL;
	}

	image = R_LoadImage(script, token.string, width, height, samples, flags);
	if (!image)
		return NULL;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'makeLuminance'\n", token.string);

		Z_Free(image);
		return NULL;
	}

	*samples = 1;

	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_ALPHA;
	*flags &= ~TF_NORMALMAP;

	return R_MakeIntensity(image, *width, *height);
}

/*
 =================
 R_ParseMakeAlpha
 =================
*/
static byte *R_ParseMakeAlpha (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'makeAlpha'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'makeAlpha'\n");
		return NULL;
	}

	image = R_LoadImage(script, token.string, width, height, samples, flags);
	if (!image)
		return NULL;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'makeAlpha'\n", token.string);

		Z_Free(image);
		return NULL;
	}

	*samples = 1;

	*flags &= ~TF_INTENSITY;
	*flags |= TF_ALPHA;
	*flags &= ~TF_NORMALMAP;

	return R_MakeAlpha(image, *width, *height);
}

/*
 =================
 R_ParseHeightMap
 =================
*/
static byte *R_ParseHeightMap (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image;
	float	scale;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'heightMap'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'heightMap'\n");
		return NULL;
	}

	image = R_LoadImage(script, token.string, width, height, samples, flags);
	if (!image)
		return NULL;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ",")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'heightMap'\n", token.string);

		Z_Free(image);
		return NULL;
	}

	if (!PS_ReadFloat(script, 0, &scale)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'heightMap'\n");

		Z_Free(image);
		return NULL;
	}

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'heightMap'\n", token.string);

		Z_Free(image);
		return NULL;
	}

	*samples = 3;

	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_ALPHA;
	*flags |= TF_NORMALMAP;

	return R_HeightMap(image, *width, *height, scale);
}

/*
 =================
 R_ParseAddNormals
 =================
*/
static byte *R_ParseAddNormals (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image1, *image2;
	int		width1, height1, samples1, width2, height2, samples2;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'addNormals'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'addNormals'\n");
		return NULL;
	}

	image1 = R_LoadImage(script, token.string, &width1, &height1, &samples1, flags);
	if (!image1)
		return NULL;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ",")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'addNormals'\n", token.string);

		Z_Free(image1);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'addNormals'\n");

		Z_Free(image1);
		return NULL;
	}

	image2 = R_LoadImage(script, token.string, &width2, &height2, &samples2, flags);
	if (!image2){
		Z_Free(image1);
		return NULL;
	}

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'addNormals'\n", token.string);

		Z_Free(image1);
		Z_Free(image2);
		return NULL;
	}

	if (width1 != width2 || height1 != height2){
		Com_Printf(S_COLOR_YELLOW "WARNING: images for 'addNormals' have mismatched dimensions (%i x %i != %i x %i)\n", width1, height1, width2, height2);

		Z_Free(image1);
		Z_Free(image2);
		return NULL;
	}

	*width = width1;
	*height = height1;

	*samples = 3;

	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_ALPHA;
	*flags |= TF_NORMALMAP;

	return R_AddNormals(image1, image2, *width, *height);
}

/*
 =================
 R_ParseSmoothNormals
 =================
*/
static byte *R_ParseSmoothNormals (script_t *script, int *width, int *height, int *samples, textureFlags_t *flags){

	token_t	token;
	byte	*image;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'smoothNormals'\n", token.string);
		return NULL;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'smoothNormals'\n");
		return NULL;
	}

	image = R_LoadImage(script, token.string, width, height, samples, flags);
	if (!image)
		return NULL;

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'smoothNormals'\n", token.string);

		Z_Free(image);
		return NULL;
	}

	*samples = 3;

	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_ALPHA;
	*flags |= TF_NORMALMAP;

	return R_SmoothNormals(image, *width, *height);
}

/*
 =================
 R_LoadImage
 =================
*/
static byte *R_LoadImage (script_t *script, const char *name, int *width, int *height, int *samples, textureFlags_t *flags){

	char	baseName[MAX_OSPATH], loadName[MAX_OSPATH];
	byte	*image;

	if (!Q_stricmp(name, "add"))
		return R_ParseAdd(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "multiply"))
		return R_ParseMultiply(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "bias"))
		return R_ParseBias(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "scale"))
		return R_ParseScale(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "invertColor"))
		return R_ParseInvertColor(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "invertAlpha"))
		return R_ParseInvertAlpha(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "makeIntensity"))
		return R_ParseMakeIntensity(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "makeLuminance"))
		return R_ParseMakeLuminance(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "makeAlpha"))
		return R_ParseMakeAlpha(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "heightMap"))
		return R_ParseHeightMap(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "addNormals"))
		return R_ParseAddNormals(script, width, height, samples, flags);
	else if (!Q_stricmp(name, "smoothNormals"))
		return R_ParseSmoothNormals(script, width, height, samples, flags);
	else {
		Com_StripExtension(name, baseName, sizeof(baseName));

		Q_snprintfz(loadName, sizeof(loadName), "%s.tga", baseName);
		if (R_LoadTGA(loadName, &image, width, height, samples))
			return image;

		Q_snprintfz(loadName, sizeof(loadName), "%s.pcx", baseName);
		if (R_LoadPCX(loadName, &image, NULL, width, height, samples))
			return image;

		Q_snprintfz(loadName, sizeof(loadName), "%s.wal", baseName);
		if (R_LoadWAL(loadName, &image, width, height, samples))
			return image;

		// Not found or invalid
		return NULL;
	}
}


/*
 =======================================================================

 TEXTURE INITIALIZATION AND LOADING

 =======================================================================
*/


/*
 =================
 R_ResampleTexture
 =================
*/
static void R_ResampleTexture (unsigned *in, int inWidth, int inHeight, unsigned *out, int outWidth, int outHeight, qboolean isNormalMap){

	unsigned	frac, fracStep;
	unsigned	p1[0x1000], p2[0x1000];
	unsigned	*inRow1, *inRow2;
	byte		*pix1, *pix2, *pix3, *pix4;
	vec3_t		normal;
	int			i, x, y;

	fracStep = inWidth * 0x10000 / outWidth;

	frac = fracStep >> 2;
	for (i = 0; i < outWidth; i++){
		p1[i] = 4 * (frac >> 16);
		frac += fracStep;
	}

	frac = (fracStep >> 2) * 3;
	for (i = 0; i < outWidth; i++){
		p2[i] = 4 * (frac >> 16);
		frac += fracStep;
	}

	if (isNormalMap){
		for (y = 0; y < outHeight; y++, out += outWidth){
			inRow1 = in + inWidth * (int)(((float)y + 0.25) * inHeight/outHeight);
			inRow2 = in + inWidth * (int)(((float)y + 0.75) * inHeight/outHeight);

			for (x = 0; x < outWidth; x++){
				pix1 = (byte *)inRow1 + p1[x];
				pix2 = (byte *)inRow1 + p2[x];
				pix3 = (byte *)inRow2 + p1[x];
				pix4 = (byte *)inRow2 + p2[x];

				normal[0] = (pix1[0] * (1.0/127) - 1.0) + (pix2[0] * (1.0/127) - 1.0) + (pix3[0] * (1.0/127) - 1.0) + (pix4[0] * (1.0/127) - 1.0);
				normal[1] = (pix1[1] * (1.0/127) - 1.0) + (pix2[1] * (1.0/127) - 1.0) + (pix3[1] * (1.0/127) - 1.0) + (pix4[1] * (1.0/127) - 1.0);
				normal[2] = (pix1[2] * (1.0/127) - 1.0) + (pix2[2] * (1.0/127) - 1.0) + (pix3[2] * (1.0/127) - 1.0) + (pix4[2] * (1.0/127) - 1.0);

				if (!VectorNormalize(normal))
					VectorSet(normal, 0.0, 0.0, 1.0);

				((byte *)(out+x))[0] = (byte)(128 + 127 * normal[0]);
				((byte *)(out+x))[1] = (byte)(128 + 127 * normal[1]);
				((byte *)(out+x))[2] = (byte)(128 + 127 * normal[2]);
				((byte *)(out+x))[3] = 255;
			}
		}
	}
	else {
		for (y = 0; y < outHeight; y++, out += outWidth){
			inRow1 = in + inWidth * (int)(((float)y + 0.25) * inHeight/outHeight);
			inRow2 = in + inWidth * (int)(((float)y + 0.75) * inHeight/outHeight);

			for (x = 0; x < outWidth; x++){
				pix1 = (byte *)inRow1 + p1[x];
				pix2 = (byte *)inRow1 + p2[x];
				pix3 = (byte *)inRow2 + p1[x];
				pix4 = (byte *)inRow2 + p2[x];

				((byte *)(out+x))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
				((byte *)(out+x))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
				((byte *)(out+x))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
				((byte *)(out+x))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
			}
		}
	}
}

/*
 =================
 R_BuildMipMap

 Operates in place, quartering the size of the texture
 =================
*/
static void R_BuildMipMap (byte *in, int width, int height, qboolean isNormalMap){

	byte	*out = in;
	vec3_t	normal;
	int		x, y;

	width <<= 2;
	height >>= 1;

	if (isNormalMap){
		for (y = 0; y < height; y++, in += width){
			for (x = 0; x < width; x += 8, in += 8, out += 4){
				normal[0] = (in[0] * (1.0/127) - 1.0) + (in[4] * (1.0/127) - 1.0) + (in[width+0] * (1.0/127) - 1.0) + (in[width+4] * (1.0/127) - 1.0);
				normal[1] = (in[1] * (1.0/127) - 1.0) + (in[5] * (1.0/127) - 1.0) + (in[width+1] * (1.0/127) - 1.0) + (in[width+5] * (1.0/127) - 1.0);
				normal[2] = (in[2] * (1.0/127) - 1.0) + (in[6] * (1.0/127) - 1.0) + (in[width+2] * (1.0/127) - 1.0) + (in[width+6] * (1.0/127) - 1.0);

				if (!VectorNormalize(normal))
					VectorSet(normal, 0.0, 0.0, 1.0);

				out[0] = (byte)(128 + 127 * normal[0]);
				out[1] = (byte)(128 + 127 * normal[1]);
				out[2] = (byte)(128 + 127 * normal[2]);
				out[3] = 255;
			}
		}
	}
	else {
		for (y = 0; y < height; y++, in += width){
			for (x = 0; x < width; x += 8, in += 8, out += 4){
				out[0] = (in[0] + in[4] + in[width+0] + in[width+4]) >> 2;
				out[1] = (in[1] + in[5] + in[width+1] + in[width+5]) >> 2;
				out[2] = (in[2] + in[6] + in[width+2] + in[width+6]) >> 2;
				out[3] = (in[3] + in[7] + in[width+3] + in[width+7]) >> 2;
			}
		}
	}
}

/*
 =================
 R_ForceTextureBorder
 =================
*/
static void R_ForceTextureBorder (byte *in, int width, int height, qboolean hasAlpha){

	byte		*out = in;
	unsigned	borderColor;
	int			i;

	if (hasAlpha)
		borderColor = 0x00000000;
	else
		borderColor = 0xFF000000;

	for (i = 0, out = in; i < width; i++, out += 4)
		*(unsigned *)out = borderColor;
	for (i = 0, out = in + (4 * ((height - 1) * width)); i < width; i++, out += 4)
		*(unsigned *)out = borderColor;
	for (i = 0, out = in; i < height; i++, out += 4 * width)
		*(unsigned *)out = borderColor;
	for (i = 0, out = in + (4 * (width - 1)); i < height; i++, out += 4 * width)
		*(unsigned *)out = borderColor;
}

/*
 =================
 R_CalcTextureSize
 =================
*/
static int R_CalcTextureSize (unsigned target, int level, int width, int height, int samples, qboolean useCompression){

	qboolean	isCompressed;
	int			size;

	if (useCompression){
		qglGetTexLevelParameteriv(target, level, GL_TEXTURE_COMPRESSED_ARB, &isCompressed);

		if (isCompressed){
			qglGetTexLevelParameteriv(target, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &size);
			return size;
		}
	}

	if (samples >= 3)
		size = width * height * 4;
	else
		size = width * height * samples;

	return size;
}

/*
 =================
 R_UploadTexture
 =================
*/
static void R_UploadTexture (unsigned **image, texture_t *texture){

	vec4_t		zeroBorder = {0, 0, 0, 1}, zeroAlphaBorder = {0, 0, 0, 0};
	unsigned	*texImage;
	unsigned	texTarget;
	int			texFaces;
	qboolean	useCompression, forceBorder;
	int			mipLevel, mipWidth, mipHeight;
	int			i;

	// Find nearest power of two, rounding down if desired
	texture->uploadWidth = NearestPowerOfTwo(texture->sourceWidth, r_roundTexturesDown->integerValue);
	texture->uploadHeight = NearestPowerOfTwo(texture->sourceHeight, r_roundTexturesDown->integerValue);

	// Sample down if desired
	if (texture->flags & TF_NORMALMAP){
		if (r_downSizeNormalTextures->integerValue && !(texture->flags & TF_NOPICMIP)){
			while (texture->uploadWidth > r_maxNormalTextureSize->integerValue || texture->uploadHeight > r_maxNormalTextureSize->integerValue){
				texture->uploadWidth >>= 1;
				texture->uploadHeight >>= 1;
			}
		}
	}
	else {
		if (r_downSizeTextures->integerValue && !(texture->flags & TF_NOPICMIP)){
			while (texture->uploadWidth > r_maxTextureSize->integerValue || texture->uploadHeight > r_maxTextureSize->integerValue){
				texture->uploadWidth >>= 1;
				texture->uploadHeight >>= 1;
			}
		}
	}

	// Clamp to hardware limits
	if (texture->isCubeMap){
		while (texture->uploadWidth > glConfig.maxCubeMapTextureSize || texture->uploadHeight > glConfig.maxCubeMapTextureSize){
			texture->uploadWidth >>= 1;
			texture->uploadHeight >>= 1;
		}
	}
	else {
		while (texture->uploadWidth > glConfig.maxTextureSize || texture->uploadHeight > glConfig.maxTextureSize){
			texture->uploadWidth >>= 1;
			texture->uploadHeight >>= 1;
		}
	}

	if (texture->uploadWidth < 1)
		texture->uploadWidth = 1;
	if (texture->uploadHeight < 1)
		texture->uploadHeight = 1;

	texture->uploadSize = 0;

	// Check if it should be compressed
	if (texture->flags & TF_NORMALMAP){
		if (!r_compressNormalTextures->integerValue || (texture->flags & TF_UNCOMPRESSED))
			useCompression = false;
		else
			useCompression = glConfig.textureCompression;
	}
	else {
		if (!r_compressTextures->integerValue || (texture->flags & TF_UNCOMPRESSED))
			useCompression = false;
		else
			useCompression = glConfig.textureCompression;
	}

	// Check if it needs a border
	if (texture->wrap == TW_CLAMP_TO_ZERO || texture->wrap == TW_CLAMP_TO_ZERO_ALPHA)
		forceBorder = !glConfig.textureBorderClamp;
	else
		forceBorder = false;

	// Set texture format
	if (useCompression){
		switch (texture->sourceSamples){
		case 1:		texture->uploadFormat = GL_COMPRESSED_LUMINANCE_ARB;		break;
		case 2:		texture->uploadFormat = GL_COMPRESSED_LUMINANCE_ALPHA_ARB;	break;
		case 3:		texture->uploadFormat = GL_COMPRESSED_RGB_ARB;				break;
		case 4:		texture->uploadFormat = GL_COMPRESSED_RGBA_ARB;				break;
		}

		if (texture->flags & TF_INTENSITY)
			texture->uploadFormat = GL_COMPRESSED_INTENSITY_ARB;
		if (texture->flags & TF_ALPHA)
			texture->uploadFormat = GL_COMPRESSED_ALPHA_ARB;

		texture->flags &= ~TF_INTENSITY;
		texture->flags &= ~TF_ALPHA;
	}
	else {
		switch (texture->sourceSamples){
		case 1:		texture->uploadFormat = GL_LUMINANCE8;						break;
		case 2:		texture->uploadFormat = GL_LUMINANCE8_ALPHA8;				break;
		case 3:		texture->uploadFormat = GL_RGB8;							break;
		case 4:		texture->uploadFormat = GL_RGBA8;							break;
		}

		if (texture->flags & TF_INTENSITY)
			texture->uploadFormat = GL_INTENSITY8;
		if (texture->flags & TF_ALPHA)
			texture->uploadFormat = GL_ALPHA8;

		texture->flags &= ~TF_INTENSITY;
		texture->flags &= ~TF_ALPHA;
	}

	// Set texture target
	if (texture->isCubeMap){
		texture->uploadTarget = GL_TEXTURE_CUBE_MAP_ARB;

		texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
		texFaces = 6;
	}
	else {
		texture->uploadTarget = GL_TEXTURE_2D;

		texTarget = GL_TEXTURE_2D;
		texFaces = 1;
	}

	// Bind the texture
	qglGenTextures(1, &texture->texNum);

	GL_BindTexture(texture);

	// Upload all the faces
	for (i = 0; i < texFaces; i++){
		// Copy or resample the texture
		if (texture->uploadWidth == texture->sourceWidth && texture->uploadHeight == texture->sourceHeight)
			texImage = image[i];
		else {
			texImage = Z_Malloc(texture->uploadWidth * texture->uploadHeight * 4);

			R_ResampleTexture(image[i], texture->sourceWidth, texture->sourceHeight, texImage, texture->uploadWidth, texture->uploadHeight, (texture->flags & TF_NORMALMAP));
		}

		// Make sure it has a border if needed
		if (forceBorder)
			R_ForceTextureBorder(texImage, texture->uploadWidth, texture->uploadHeight, (texture->wrap == TW_CLAMP_TO_ZERO_ALPHA));

		// Upload the base texture
		qglTexImage2D(texTarget + i, 0, texture->uploadFormat, texture->uploadWidth, texture->uploadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texImage);

		// Calculate size
		texture->uploadSize += R_CalcTextureSize(texTarget + i, 0, texture->uploadWidth, texture->uploadHeight, texture->sourceSamples, useCompression);

		// Generate mipmaps if desired
		if (texture->filter == TF_DEFAULT){
			mipLevel = 0;
			mipWidth = texture->uploadWidth;
			mipHeight = texture->uploadHeight;

			while (mipWidth > 1 || mipHeight > 1){
				// Development tool
				if (r_colorMipLevels->integerValue){
					if (!(texture->flags & (TF_INTERNAL | TF_NORMALMAP)) && texture->sourceSamples >= 3){
						qglPixelTransferf(GL_RED_BIAS, (mipLevel % 3 == 0) ? 1.0 : 0.0);
						qglPixelTransferf(GL_GREEN_BIAS, (mipLevel % 3 == 1) ? 1.0 : 0.0);
						qglPixelTransferf(GL_BLUE_BIAS, (mipLevel % 3 == 2) ? 1.0 : 0.0);
					}
				}

				// Build the mipmap
				R_BuildMipMap(texImage, mipWidth, mipHeight, (texture->flags & TF_NORMALMAP));

				mipLevel++;

				mipWidth >>= 1;
				if (mipWidth < 1)
					mipWidth = 1;

				mipHeight >>= 1;
				if (mipHeight < 1)
					mipHeight = 1;

				// Make sure it has a border if needed
				if (forceBorder)
					R_ForceTextureBorder(texImage, mipWidth, mipHeight, (texture->wrap == TW_CLAMP_TO_ZERO_ALPHA));

				// Upload the mipmap texture
				qglTexImage2D(texTarget + i, mipLevel, texture->uploadFormat, mipWidth, mipHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texImage);

				// Calculate size
				texture->uploadSize += R_CalcTextureSize(texTarget + i, mipLevel, mipWidth, mipHeight, texture->sourceSamples, useCompression);
			}

			if (r_colorMipLevels->integerValue){
				if (!(texture->flags & (TF_INTERNAL | TF_NORMALMAP)) && texture->sourceSamples >= 3){
					qglPixelTransferf(GL_RED_BIAS, 0.0);
					qglPixelTransferf(GL_GREEN_BIAS, 0.0);
					qglPixelTransferf(GL_BLUE_BIAS, 0.0);
				}
			}
		}

		// Free texture data
		if (texImage != image[i])
			Z_Free(texImage);
	}

	// Set texture filter
	switch (texture->filter){
	case TF_DEFAULT:
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_MIN_FILTER, r_textureMinFilter);
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_MAG_FILTER, r_textureMagFilter);

		// Set texture anisotropy if available
		if (glConfig.textureFilterAnisotropic)
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_textureAnisotropy->floatValue);

		// Set texture LOD bias if available
		if (glConfig.textureLodBias)
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_LOD_BIAS_EXT, r_textureLodBias->floatValue);

		break;
	case TF_NEAREST:
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		break;
	case TF_LINEAR:
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		break;
	default:
		Com_Error(ERR_DROP, "R_UploadTexture: bad texture filter (%i)", texture->filter);
	}

	// Set texture wrap
	switch (texture->wrap){
	case TW_REPEAT:
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);

		break;
	case TW_CLAMP:
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		break;
	case TW_CLAMP_TO_ZERO:
		if (glConfig.textureBorderClamp){
			qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_ARB);
			qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_ARB);
		}
		else {
			qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
			qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}

		qglTexParameterfv(texture->uploadTarget, GL_TEXTURE_BORDER_COLOR, zeroBorder);

		break;
	case TW_CLAMP_TO_ZERO_ALPHA:
		if (glConfig.textureBorderClamp){
			qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_ARB);
			qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_ARB);
		}
		else {
			qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
			qglTexParameteri(texture->uploadTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}

		qglTexParameterfv(texture->uploadTarget, GL_TEXTURE_BORDER_COLOR, zeroAlphaBorder);

		break;
	default:
		Com_Error(ERR_DROP, "R_UploadTexture: bad texture wrap (%i)", texture->wrap);
	}
}

/*
 =================
 R_LoadTexture
 =================
*/
static texture_t *R_LoadTexture (const char *name, byte *image, int width, int height, int samples, textureFlags_t flags, textureFilter_t filter, textureWrap_t wrap){

	texture_t	*texture;
	unsigned	hash;

	if (r_numTextures == MAX_TEXTURES)
		Com_Error(ERR_DROP, "R_LoadTexture: MAX_TEXTURES hit");

	r_textures[r_numTextures++] = texture = Z_Malloc(sizeof(texture_t));

	// Fill it in
	Q_strncpyz(texture->name, name, sizeof(texture->name));
	texture->flags = flags;
	texture->filter = filter;
	texture->wrap = wrap;
	texture->isCubeMap = false;
	texture->frameCount = 0;
	texture->sourceWidth = width;
	texture->sourceHeight = height;
	texture->sourceSamples = samples;

	R_UploadTexture(&image, texture);

	// Add to hash table
	hash = Com_HashKey(texture->name, TEXTURES_HASH_SIZE);

	texture->nextHash = r_texturesHashTable[hash];
	r_texturesHashTable[hash] = texture;

	return texture;
}

/*
 =================
 R_LoadCubeMapTexture
 =================
*/
static texture_t *R_LoadCubeMapTexture (const char *name, byte *images[6], int width, int height, int samples, textureFlags_t flags, textureFilter_t filter, textureWrap_t wrap){

	texture_t	*texture;
	unsigned	hash;

	if (r_numTextures == MAX_TEXTURES)
		Com_Error(ERR_DROP, "R_LoadCubeMapTexture: MAX_TEXTURES hit");

	r_textures[r_numTextures++] = texture = Z_Malloc(sizeof(texture_t));

	// Fill it in
	Q_strncpyz(texture->name, name, sizeof(texture->name));
	texture->flags = flags;
	texture->filter = filter;
	texture->wrap = wrap;
	texture->isCubeMap = true;
	texture->frameCount = 0;
	texture->sourceWidth = width;
	texture->sourceHeight = height;
	texture->sourceSamples = samples;

	R_UploadTexture(images, texture);

	// Add to hash table
	hash = Com_HashKey(texture->name, TEXTURES_HASH_SIZE);

	texture->nextHash = r_texturesHashTable[hash];
	r_texturesHashTable[hash] = texture;

	return texture;
}

/*
 =================
 R_FindTexture
 =================
*/
texture_t *R_FindTexture (const char *name, textureFlags_t flags, textureFilter_t filter, textureWrap_t wrap){

	texture_t	*texture;
	script_t	*script;
	token_t		token;
	byte		*image;
	int			width, height, samples;
	unsigned	hash;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindTexture: NULL texture name");

	if (strlen(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "R_FindTexture: texture name exceeds MAX_OSPATH");

	// See if already loaded
	hash = Com_HashKey(name, TEXTURES_HASH_SIZE);

	for (texture = r_texturesHashTable[hash]; texture; texture = texture->nextHash){
		if (texture->isCubeMap)
			continue;

		if (!Q_stricmp(texture->name, name)){
			if (texture->flags & TF_INTERNAL)
				return texture;

			if (texture->flags != flags)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed flags parameter\n", name);
			if (texture->filter != filter)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed filter parameter\n", name);
			if (texture->wrap != wrap)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed wrap parameter\n", name);

			return texture;
		}
	}

	// Load it from disk
	script = PS_LoadScriptMemory(name, name, strlen(name));
	if (!script)
		return NULL;

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		PS_FreeScript(script);
		return NULL;
	}

	image = R_LoadImage(script, token.string, &width, &height, &samples, &flags);

	PS_FreeScript(script);

	// Load the texture
	if (image){
		texture = R_LoadTexture(name, image, width, height, samples, flags, filter, wrap);
		Z_Free(image);
		return texture;
	}

	// Not found or invalid
	return NULL;
}

/*
 =================
 R_FindCubeMapTexture
 =================
*/
texture_t *R_FindCubeMapTexture (const char *name, textureFlags_t flags, textureFilter_t filter, textureWrap_t wrap, qboolean cameraSpace){

	texture_t	*texture;
	char		loadName[MAX_OSPATH];
	byte		*images[6];
	int			width, height, samples;
	int			cubeWidth, cubeHeight, cubeSamples;
	unsigned	hash;
	int			i, j;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindCubeMapTexture: NULL texture name");

	if (strlen(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "R_FindCubeMapTexture: texture name exceeds MAX_OSPATH");

	// See if already loaded
	hash = Com_HashKey(name, TEXTURES_HASH_SIZE);

	for (texture = r_texturesHashTable[hash]; texture; texture = texture->nextHash){
		if (!texture->isCubeMap)
			continue;

		if (!Q_stricmp(texture->name, name)){
			if (texture->flags & TF_INTERNAL)
				return texture;

			if (texture->flags != flags)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed flags parameter\n", name);
			if (texture->filter != filter)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed filter parameter\n", name);
			if (texture->wrap != wrap)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed wrap parameter\n", name);

			return texture;
		}
	}

	// Load it from disk
	for (i = 0; i < 6; i++){
		Q_snprintfz(loadName, sizeof(loadName), "%s%s%s.tga", name, r_cubeSides[cameraSpace][i].separator, r_cubeSides[cameraSpace][i].suffix);
		if (!R_LoadTGA(loadName, &images[i], &width, &height, &samples))
			break;

		// Check dimensions
		if (width != height){
			Com_Printf(S_COLOR_YELLOW "WARNING: cube map face '%s%s%s' is not square (%i != %i)\n", name, r_cubeSides[cameraSpace][i].separator, r_cubeSides[cameraSpace][i].suffix, width, height);
			break;
		}

		// Compare dimensions and format with previous faces
		if (i != 0){
			if (width != cubeWidth || height != cubeHeight){
				Com_Printf(S_COLOR_YELLOW "WARNING: cube map face '%s%s%s' has mismatched dimensions (%i x %i != %i x %i)\n", name, r_cubeSides[cameraSpace][i].separator, r_cubeSides[cameraSpace][i].suffix, width, height, cubeWidth, cubeHeight);
				break;
			}

			if (samples != cubeSamples){
				if (samples == 2){
					if (cubeSamples == 1)
						cubeSamples = 2;
					else if (cubeSamples == 3)
						cubeSamples = 4;
				}
				else if (samples == 3){
					if (cubeSamples == 1)
						cubeSamples = 3;
					else if (cubeSamples == 2)
						cubeSamples = 4;
				}
				else if (samples == 4)
					cubeSamples = 4;
			}

			continue;
		}

		cubeWidth = width;
		cubeHeight = height;
		cubeSamples = samples;
	}

	// Load the texture
	if (i == 6){
		for (i = 0; i < 6; i++){
			if (r_cubeSides[cameraSpace][i].flipX || r_cubeSides[cameraSpace][i].flipY || r_cubeSides[cameraSpace][i].flipDiagonal)
				images[i] = R_FlipImage(images[i], cubeWidth, cubeHeight, 4, r_cubeSides[cameraSpace][i].flipX, r_cubeSides[cameraSpace][i].flipY, r_cubeSides[cameraSpace][i].flipDiagonal);
		}

		texture = R_LoadCubeMapTexture(name, images, cubeWidth, cubeHeight, cubeSamples, flags, filter, wrap);

		for (i = 0; i < 6; i++)
			Z_Free(images[i]);

		return texture;
	}

	// Not found or invalid
	for (j = 0; j < i; j++)
		Z_Free(images[j]);

	return NULL;
}

/*
 =================
 R_SetTextureParameters
 =================
*/
void R_SetTextureParameters (void){

	texture_t	*texture;
	int			i;

	if (!Q_stricmp(r_textureFilter->value, "GL_NEAREST")){
		r_textureMinFilter = GL_NEAREST;
		r_textureMagFilter = GL_NEAREST;
	}
	else if (!Q_stricmp(r_textureFilter->value, "GL_LINEAR")){
		r_textureMinFilter = GL_LINEAR;
		r_textureMagFilter = GL_LINEAR;
	}
	else if (!Q_stricmp(r_textureFilter->value, "GL_NEAREST_MIPMAP_NEAREST")){
		r_textureMinFilter = GL_NEAREST_MIPMAP_NEAREST;
		r_textureMagFilter = GL_NEAREST;
	}
	else if (!Q_stricmp(r_textureFilter->value, "GL_LINEAR_MIPMAP_NEAREST")){
		r_textureMinFilter = GL_LINEAR_MIPMAP_NEAREST;
		r_textureMagFilter = GL_LINEAR;
	}
	else if (!Q_stricmp(r_textureFilter->value, "GL_NEAREST_MIPMAP_LINEAR")){
		r_textureMinFilter = GL_NEAREST_MIPMAP_LINEAR;
		r_textureMagFilter = GL_NEAREST;
	}
	else if (!Q_stricmp(r_textureFilter->value, "GL_LINEAR_MIPMAP_LINEAR")){
		r_textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
		r_textureMagFilter = GL_LINEAR;
	}
	else {
		Cvar_SetString("r_textureFilter", "GL_LINEAR_MIPMAP_LINEAR");

		r_textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
		r_textureMagFilter = GL_LINEAR;
	}

	r_textureFilter->modified = false;

	if (glConfig.textureFilterAnisotropic){
		if (r_textureAnisotropy->floatValue > glConfig.maxTextureMaxAnisotropy)
			Cvar_SetFloat("r_textureAnisotropy", glConfig.maxTextureMaxAnisotropy);
		else if (r_textureAnisotropy->floatValue < 1.0)
			Cvar_SetFloat("r_textureAnisotropy", 1.0);
	}

	r_textureAnisotropy->modified = false;

	if (glConfig.textureLodBias){
		if (r_textureLodBias->floatValue > glConfig.maxTextureLodBias)
			Cvar_SetFloat("r_textureLodBias", glConfig.maxTextureLodBias);
		else if (r_textureLodBias->floatValue < -glConfig.maxTextureLodBias)
			Cvar_SetFloat("r_textureLodBias", -glConfig.maxTextureLodBias);
	}

	r_textureLodBias->modified = false;

	// Change all the existing mipmapped texture objects
	for (i = 0; i < r_numTextures; i++){
		texture = r_textures[i];

		if (texture->filter != TF_DEFAULT)
			continue;

		GL_BindTexture(texture);

		// Set texture filter
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_MIN_FILTER, r_textureMinFilter);
		qglTexParameteri(texture->uploadTarget, GL_TEXTURE_MAG_FILTER, r_textureMagFilter);

		// Set texture anisotropy if available
		if (glConfig.textureFilterAnisotropic)
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_textureAnisotropy->floatValue);

		// Set texture LOD bias if available
		if (glConfig.textureLodBias)
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_LOD_BIAS_EXT, r_textureLodBias->floatValue);
	}
}

/*
 =================
 R_CaptureRenderToTexture
 =================
*/
qboolean R_CaptureRenderToTexture (const char *name){

	captureRenderCommand_t	*cmd;
	texture_t				*texture;
	unsigned				hash;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_CaptureRenderToTexture: NULL texture name");

	if (strlen(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "R_CaptureRenderToTexture: texture name exceeds MAX_OSPATH");

	// Find the texture
	hash = Com_HashKey(name, TEXTURES_HASH_SIZE);

	for (texture = r_texturesHashTable[hash]; texture; texture = texture->nextHash){
		if (texture->isCubeMap)
			continue;

		if (!Q_stricmp(texture->name, name))
			break;
	}

	if (!texture){
		Com_DPrintf(S_COLOR_YELLOW "R_CaptureRenderToTexture: couldn't find texture (%s)\n", name);
		return false;
	}

	// Make sure it may not be down-sized or compressed
	if (!(texture->flags & TF_NOPICMIP) || !(texture->flags & TF_UNCOMPRESSED)){
		Com_DPrintf(S_COLOR_YELLOW "R_CaptureRenderToTexture: texture may be down-sized or compressed (%s)\n", name);
		return false;
	}

	// Make sure it's not mipmapped
	if (texture->filter == TF_DEFAULT){
		Com_DPrintf(S_COLOR_YELLOW "R_CaptureRenderToTexture: texture has mipmaps (%s)\n", name);
		return false;
	}

	// Make sure it doesn't have a border
	if (texture->wrap != TW_REPEAT && texture->wrap != TW_CLAMP){
		Com_DPrintf(S_COLOR_YELLOW "R_CaptureRenderToTexture: texture has a border (%s)\n", name);
		return false;
	}

	// Add a capture render command
	cmd = RB_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
		return true;

	cmd->commandId = RC_CAPTURE_RENDER;

	cmd->texture = texture;

	return true;
}

/*
 =================
 R_UpdateTexture
 =================
*/
qboolean R_UpdateTexture (const char *name, const byte *image, int width, int height){

	updateTextureCommand_t	*cmd;
	texture_t				*texture;
	unsigned				hash;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_UpdateTexture: NULL texture name");

	if (strlen(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "R_UpdateTexture: texture name exceeds MAX_OSPATH");

	// Find the texture
	hash = Com_HashKey(name, TEXTURES_HASH_SIZE);

	for (texture = r_texturesHashTable[hash]; texture; texture = texture->nextHash){
		if (texture->isCubeMap)
			continue;

		if (!Q_stricmp(texture->name, name))
			break;
	}

	if (!texture){
		Com_DPrintf(S_COLOR_YELLOW "R_UpdateTexture: couldn't find texture (%s)\n", name);
		return false;
	}

	// Make sure it may not be down-sized or compressed
	if (!(texture->flags & TF_NOPICMIP) || !(texture->flags & TF_UNCOMPRESSED)){
		Com_DPrintf(S_COLOR_YELLOW "R_UpdateTexture: texture may be down-sized or compressed (%s)\n", name);
		return false;
	}

	// Make sure it's not mipmapped
	if (texture->filter == TF_DEFAULT){
		Com_DPrintf(S_COLOR_YELLOW "R_UpdateTexture: texture has mipmaps (%s)\n", name);
		return false;
	}

	// Make sure it doesn't have a border
	if (texture->wrap != TW_REPEAT && texture->wrap != TW_CLAMP){
		Com_DPrintf(S_COLOR_YELLOW "R_UpdateTexture: texture has a border (%s)\n", name);
		return false;
	}

	// Add an update texture command
	cmd = RB_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
		return true;

	cmd->commandId = RC_UPDATE_TEXTURE;

	cmd->texture = texture;

	cmd->image = image;
	cmd->width = width;
	cmd->height = height;

	return true;
}

/*
 =================
 R_ScreenShot_f
 =================
*/
void R_ScreenShot_f (void){

	byte		*buffer;
	char		name[MAX_OSPATH];
	qboolean	silent;
	int			i, size;

	if (Cmd_Argc() > 3){
		Com_Printf("Usage: screenShot [\"-silent\"] [name]\n");
		return;
	}

	if (Cmd_Argc() == 2){
		if (!Q_stricmp("-silent", Cmd_Argv(1))){
			silent = true;
			name[0] = 0;
		}
		else {
			silent = false;
			Q_snprintfz(name, sizeof(name), "screenshots/%s", Cmd_Argv(1));
			Com_DefaultExtension(name, sizeof(name), ".tga");
		}
	}
	else if (Cmd_Argc() == 3){
		if (!Q_stricmp("-silent", Cmd_Argv(1)))
			silent = true;
		else {
			Com_Printf("Usage: screenShot [\"-silent\"] [name]\n");
			return;
		}

		Q_snprintfz(name, sizeof(name), "screenshots/%s", Cmd_Argv(2));
		Com_DefaultExtension(name, sizeof(name), ".tga");
	}
	else {
		silent = false;
		name[0] = 0;
	}

	if (!name[0]){
		// Find a file name to save it to
		for (i = 0; i <= 9999; i++){
			Q_snprintfz(name, sizeof(name), "screenshots/q2e_shot%04i.tga", i);
			if (!FS_FileExists(name))
				break;	// File doesn't exist
		} 

		if (i == 10000){
			Com_Printf("Screenshots directory is full!\n");
			return;
 		}
	}

	// Allocate the buffer
	size = 18 + (glConfig.videoWidth * glConfig.videoHeight * 3);
	buffer = Z_Malloc(size);

	// Set up the header
	memset(buffer, 0, 18);

	buffer[2] = 2;
	buffer[12] = glConfig.videoWidth & 255;
	buffer[13] = glConfig.videoWidth >> 8;
	buffer[14] = glConfig.videoHeight & 255;
	buffer[15] = glConfig.videoHeight >> 8;
	buffer[16] = 24;

	// Read the pixels
	if (r_frontBuffer->integerValue)
		qglReadBuffer(GL_FRONT);
	else
		qglReadBuffer(GL_BACK);

	qglReadPixels(0, 0, glConfig.videoWidth, glConfig.videoHeight, GL_BGR, GL_UNSIGNED_BYTE, buffer + 18);

	// Apply gamma if needed
	if (glConfig.deviceSupportsGamma && (r_gamma->floatValue != 1.0 || r_brightness->floatValue != 1.0)){
		for (i = 18; i < size; i += 3){
			buffer[i+0] = tr.gammaTable[buffer[i+0]];
			buffer[i+1] = tr.gammaTable[buffer[i+1]];
			buffer[i+2] = tr.gammaTable[buffer[i+2]];
		}
	}

	// Write the image
	if (!FS_SaveFile(name, buffer, size)){
		if (!silent)
			Com_Printf("Couldn't write %s\n", name);

		Z_Free(buffer);
		return;
	}

	Z_Free(buffer);

	if (!silent)
		Com_Printf("ScreenShot: wrote %s\n", name);
}

/*
 =================
 R_EnvShot_f
 =================
*/
void R_EnvShot_f (void){

	fileHandle_t	f;
	renderView_t	renderView;
	byte			*buffer, header[18];
	char			baseName[MAX_OSPATH], realName[MAX_OSPATH];
	int				i, size;

	if (Cmd_Argc() < 2 || Cmd_Argc() > 3){
		Com_Printf("Usage: envShot <baseName> [size]\n");
		return;
	}

	if (!tr.primaryViewAvailable){
		Com_Printf("No primaryView available for taking an environment shot\n");
		return;
	}

	Q_strncpyz(baseName, Cmd_Argv(1), sizeof(baseName));

	if (Cmd_Argc() == 3){
		size = atoi(Cmd_Argv(2));
		size = NearestPowerOfTwo(size, true);
	}
	else
		size = 256;

	// Make sure the specified size is valid
	if (size > glConfig.videoWidth || size > glConfig.videoHeight){
		Com_Printf("Specified size is greater than current resolution\n");
		return;
	}

	// Allocate the buffer
	buffer = Z_Malloc(size * size * 3);

	// Set up the header
	memset(header, 0, 18);

	header[2] = 2;
	header[12] = size & 255;
	header[13] = size >> 8;
	header[14] = size & 255;
	header[15] = size >> 8;
	header[16] = 24;

	// Render a frame for each face of the environment box
	for (i = 0; i < 6; i++){
		// Set up renderView
		renderView = tr.primaryView.renderView;

		renderView.x = 0;
		renderView.y = 0;
		renderView.width = size;
		renderView.height = size;

		renderView.fovX = 90.0;
		renderView.fovY = 90.0;

		AnglesToAxis(r_envShot[i].angles, renderView.viewAxis);

		// Render the scene
		R_BeginFrame();

		tr.scene.numEntities = tr.primaryView.numEntities;
		tr.scene.firstEntity = tr.primaryView.firstEntity;

		tr.scene.numLights = tr.primaryView.numLights;
		tr.scene.firstLight = tr.primaryView.firstLight;

		tr.scene.numPolys = tr.primaryView.numPolys;
		tr.scene.firstPoly = tr.primaryView.firstPoly;

		R_RenderScene(&renderView, true);

		R_EndFrame();

		// Read the pixels
		if (r_frontBuffer->integerValue)
			qglReadBuffer(GL_FRONT);
		else
			qglReadBuffer(GL_BACK);

		qglReadPixels(0, 0, size, size, GL_BGR, GL_UNSIGNED_BYTE, buffer);

		// Flip it if needed
		if (r_envShot[i].flipX || r_envShot[i].flipY || r_envShot[i].flipDiagonal)
			buffer = R_FlipImage(buffer, size, size, 3, r_envShot[i].flipX, r_envShot[i].flipY, r_envShot[i].flipDiagonal);

		// Write the image
		Q_snprintfz(realName, sizeof(realName), "env/%s_%s.tga", baseName, r_envShot[i].suffix);

		FS_OpenFile(realName, &f, FS_WRITE);
		if (!f){
			Com_Printf("Couldn't write env/%s_*.tga\n", baseName);

			Z_Free(buffer);
			return;
		}

		FS_Write(header, sizeof(header), f);
		FS_Write(buffer, size * size * 3, f);
		FS_CloseFile(f);
	}

	Z_Free(buffer);

	Com_Printf("EnvShot: wrote env/%s_*.tga\n", baseName);
}

/*
 =================
 R_ListTextures_f
 =================
*/
void R_ListTextures_f (void){

	texture_t	*texture;
	int			i;
	int			bytes = 0;

	Com_Printf("\n");
	Com_Printf("      -w-- -h-- -size- -fmt- type filt wrap -name--------\n");

	for (i = 0; i < r_numTextures; i++){
		texture = r_textures[i];

		bytes += texture->uploadSize;

		Com_Printf("%4i: ", i);

		Com_Printf("%4i %4i ", texture->uploadWidth, texture->uploadHeight);

		Com_Printf("%5ik ", texture->uploadSize / 1024);

		switch (texture->uploadFormat){
		case GL_COMPRESSED_RGBA_ARB:
			Com_Printf("CRGBA ");
			break;
		case GL_COMPRESSED_RGB_ARB:
			Com_Printf("CRGB  ");
			break;
		case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
			Com_Printf("CLA   ");
			break;
		case GL_COMPRESSED_LUMINANCE_ARB:
			Com_Printf("CL    ");
			break;
		case GL_COMPRESSED_ALPHA_ARB:
			Com_Printf("CA    ");
			break;
		case GL_COMPRESSED_INTENSITY_ARB:
			Com_Printf("CI    ");
			break;
		case GL_RGBA8:
			Com_Printf("RGBA8 ");
			break;
		case GL_RGB8:
			Com_Printf("RGB8  ");
			break;
		case GL_LUMINANCE8_ALPHA8:
			Com_Printf("L8A8  ");
			break;
		case GL_LUMINANCE8:
			Com_Printf("L8    ");
			break;
		case GL_ALPHA8:
			Com_Printf("A8    ");
			break;
		case GL_INTENSITY8:
			Com_Printf("I8    ");
			break;
		default:
			Com_Printf("????? ");
			break;
		}

		switch (texture->uploadTarget){
		case GL_TEXTURE_2D:
			Com_Printf(" 2D  ");
			break;
		case GL_TEXTURE_CUBE_MAP_ARB:
			Com_Printf("CUBE ");
			break;
		default:
			Com_Printf("???? ");
			break;
		}

		switch (texture->filter){
		case TF_DEFAULT:
			Com_Printf("dflt ");
			break;
		case TF_NEAREST:
			Com_Printf("nrst ");
			break;
		case TF_LINEAR:
			Com_Printf("linr ");
			break;
		default:
			Com_Printf("???? ");
			break;
		}

		switch (texture->wrap){
		case TW_REPEAT:
			Com_Printf("rept ");
			break;
		case TW_CLAMP:
			Com_Printf("clmp ");
			break;
		case TW_CLAMP_TO_ZERO:
			Com_Printf("zero ");
			break;
		case TW_CLAMP_TO_ZERO_ALPHA:
			Com_Printf("azro ");
			break;
		default:
			Com_Printf("???? ");
			break;
		}

		Com_Printf("%s\n", texture->name);
	}

	Com_Printf("---------------------------------------------------------\n");
	Com_Printf("%i total textures\n", r_numTextures);
	Com_Printf("%.2f total megabytes of textures\n", bytes / 1048576.0);
	Com_Printf("\n");
}

/*
 =================
 R_CreateBuiltInTextures
 =================
*/
static void R_CreateBuiltInTextures (void){

	byte	data2D[256*256*4];
	byte	*dataCubeMap[6];
	vec3_t	normal;
	float	s, t, intensity;
	int		i, x, y;

	// Default texture
	for (y = 0; y < 16; y++){
		for (x = 0; x < 16; x++){
			if (x == 0 || x == 15 || y == 0 || y == 15)
				((unsigned *)&data2D)[y*16+x] = LittleLong(0xFFFFFFFF);
			else
				((unsigned *)&data2D)[y*16+x] = LittleLong(0xFF000000);
		}
	}

	tr.defaultTexture = R_LoadTexture("_default", data2D, 16, 16, 1, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_DEFAULT, TW_REPEAT);

	// White texture
	for (i = 0; i < 64; i++)
		((unsigned *)&data2D)[i] = LittleLong(0xFFFFFFFF);

	tr.whiteTexture = R_LoadTexture("_white", data2D, 8, 8, 1, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_DEFAULT, TW_REPEAT);

	// Black texture
	for (i = 0; i < 64; i++)
		((unsigned *)&data2D)[i] = LittleLong(0xFF000000);

	tr.blackTexture = R_LoadTexture("_black", data2D, 8, 8, 1, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_DEFAULT, TW_REPEAT);

	// Flat texture
	for (i = 0; i < 64; i++)
		((unsigned *)&data2D)[i] = LittleLong(0xFFFF8080);

	tr.flatTexture = R_LoadTexture("_flat", data2D, 8, 8, 3, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED | TF_NORMALMAP, TF_DEFAULT, TW_REPEAT);

	// Attenuation texture
	for (y = 0; y < 128; y++){
		for (x = 0; x < 128; x++){
			s = ((((float)x + 0.5) * (2.0/128)) - 1.0) * (1.0/0.9375);
			t = ((((float)y + 0.5) * (2.0/128)) - 1.0) * (1.0/0.9375);

			intensity = 1.0 - sqrt(s*s + t*t);

			intensity = Clamp(255.0 * intensity, 0.0, 255.0);

			data2D[4*(y*128+x)+0] = (byte)intensity;
			data2D[4*(y*128+x)+1] = (byte)intensity;
			data2D[4*(y*128+x)+2] = (byte)intensity;
			data2D[4*(y*128+x)+3] = 255;
		}
	}

	tr.attenuationTexture = R_LoadTexture("_attenuation", data2D, 128, 128, 1, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_DEFAULT, TW_CLAMP_TO_ZERO);

	// Falloff texture
	for (y = 0; y < 8; y++){
		for (x = 0; x < 64; x++){
			s = ((((float)x + 0.5) * (2.0/64)) - 1.0) * (1.0/0.9375);

			intensity = 1.0 - sqrt(s*s);

			intensity = Clamp(255.0 * intensity, 0.0, 255.0);

			data2D[4*(y*64+x)+0] = (byte)intensity;
			data2D[4*(y*64+x)+1] = (byte)intensity;
			data2D[4*(y*64+x)+2] = (byte)intensity;
			data2D[4*(y*64+x)+3] = (byte)intensity;
		}
	}

	tr.falloffTexture = R_LoadTexture("_falloff", data2D, 64, 8, 1, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED | TF_INTENSITY, TF_LINEAR, TW_CLAMP);

	// Fog texture
	for (y = 0; y < 128; y++){
		for (x = 0; x < 128; x++){
			s = ((((float)x + 0.5) * (2.0/128)) - 1.0) * (1.0/0.9375);
			t = ((((float)y + 0.5) * (2.0/128)) - 1.0) * (1.0/0.9375);

			intensity = pow(sqrt(s*s + t*t), 0.5);

			intensity = Clamp(255.0 * intensity, 0.0, 255.0);

			data2D[4*(y*128+x)+0] = 255;
			data2D[4*(y*128+x)+1] = 255;
			data2D[4*(y*128+x)+2] = 255;
			data2D[4*(y*128+x)+3] = (byte)intensity;
		}
	}

	tr.fogTexture = R_LoadTexture("_fog", data2D, 128, 128, 1, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED | TF_ALPHA, TF_LINEAR, TW_CLAMP);

	// Fog-enter texture
	for (y = 0; y < 64; y++){
		for (x = 0; x < 64; x++){
			s = ((((float)x + 0.5) * (2.0/64)) - 1.0) * (1.0/0.3750);
			t = ((((float)y + 0.5) * (2.0/64)) - 1.0) * (1.0/0.3750);

			s = Clamp(s + (1.0/16), -1.0, 0.0);
			t = Clamp(t + (1.0/16), -1.0, 0.0);

			intensity = pow(sqrt(s*s + t*t), 0.5 );

			intensity = Clamp(255.0 * intensity, 0.0, 255.0);

			data2D[4*(y*64+x)+0] = 255;
			data2D[4*(y*64+x)+1] = 255;
			data2D[4*(y*64+x)+2] = 255;
			data2D[4*(y*64+x)+3] = (byte)intensity;
		}
	}

	tr.fogEnterTexture = R_LoadTexture("_fogEnter", data2D, 64, 64, 1, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED | TF_ALPHA, TF_LINEAR, TW_CLAMP);

	// Cinematic texture
	memset(data2D, 255, 16*16*4);
	tr.cinematicTexture = R_LoadTexture("_cinematic", data2D, 16, 16, 4, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, TW_REPEAT);

	// Scratch texture
	memset(data2D, 255, 16*16*4);
	tr.scratchTexture = R_LoadTexture("_scratch", data2D, 16, 16, 4, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, TW_REPEAT);

	// Accum texture
	memset(data2D, 255, 16*16*4);
	tr.accumTexture = R_LoadTexture("_accum", data2D, 16, 16, 3, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, TW_CLAMP);

	// Mirror render texture
	memset(data2D, 255, 16*16*4);
	tr.mirrorRenderTexture = R_LoadTexture("_mirrorRender", data2D, 16, 16, 3, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, TW_CLAMP);

	// Remote render texture
	memset(data2D, 255, 16*16*4);
	tr.remoteRenderTexture = R_LoadTexture("_remoteRender", data2D, 16, 16, 3, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, TW_REPEAT);

	// Current render texture
	memset(data2D, 255, 16*16*4);
	tr.currentRenderTexture = R_LoadTexture("_currentRender", data2D, 16, 16, 3, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, TW_CLAMP);

	// Normal cube map texture
	for (i = 0; i < 6; i++){
		dataCubeMap[i] = Z_Malloc(32*32*4);

		for (y = 0; y < 32; y++){
			for (x = 0; x < 32; x++){
				s = (((float)x + 0.5) * (2.0/32)) - 1.0;
				t = (((float)y + 0.5) * (2.0/32)) - 1.0;

				switch (i){
				case 0:		VectorSet(normal, 1.0, -t, -s);		break;
				case 1:		VectorSet(normal, -1.0, -t, s);		break;
				case 2:		VectorSet(normal, s, 1.0, t);		break;
				case 3:		VectorSet(normal, s, -1.0, -t);		break;
				case 4:		VectorSet(normal, s, -t, 1.0);		break;
				case 5:		VectorSet(normal, -s, -t, -1.0);	break;
				}

				VectorNormalize(normal);

				dataCubeMap[i][4*(y*32+x)+0] = (byte)(128 + 127 * normal[0]);
				dataCubeMap[i][4*(y*32+x)+1] = (byte)(128 + 127 * normal[1]);
				dataCubeMap[i][4*(y*32+x)+2] = (byte)(128 + 127 * normal[2]);
				dataCubeMap[i][4*(y*32+x)+3] = 255;
			}
		}
	}

	tr.normalCubeMapTexture = R_LoadCubeMapTexture("_normalCubeMap", dataCubeMap, 32, 32, 3, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, TW_CLAMP);

	for (i = 0; i < 6; i++)
		Z_Free(dataCubeMap[i]);

}

/*
 =================
 R_InitTextures
 =================
*/
void R_InitTextures (void){

	int			i;
	int			r, g, b;
	float		f;
	byte		*palette;
	byte		defaultPalette[] = {
#include "palette.h"
	};

	// Build luminance table
	for (i = 0; i < 256; i++){
		f = (float)i;

		r_luminanceTable[i][0] = f * 0.299;
		r_luminanceTable[i][1] = f * 0.587;
		r_luminanceTable[i][2] = f * 0.114;
	}

	// Load the palette
	if (!R_LoadPCX("pics/colormap.pcx", NULL, &palette, NULL, NULL, NULL))
		palette = defaultPalette;

	for (i = 0; i < 256; i++){
		r = palette[i*3+0];
		g = palette[i*3+1];
		b = palette[i*3+2];

		r_palette[i] = (r << 0) + (g << 8) + (b << 16) + (255 << 24);
		r_palette[i] = LittleLong(r_palette[i]);
	}
	r_palette[255] &= LittleLong(0x00FFFFFF);	// 255 is transparent

	if (palette != defaultPalette)
		Z_Free(palette);

	// Set texture parameters
	R_SetTextureParameters();

	// Create built-in textures
	R_CreateBuiltInTextures();
}

/*
 =================
 R_ShutdownTextures
 =================
*/
void R_ShutdownTextures (void){

	texture_t	*texture;
	int			i;

	for (i = MAX_TEXTURE_UNITS - 1; i >= 0; i--){
		if (glConfig.fragmentProgram){
			if (i >= glConfig.maxTextureCoords || i >= glConfig.maxTextureImageUnits)
				continue;
		}
		else {
			if (i >= glConfig.maxTextureUnits)
				continue;
		}

		GL_SelectTexture(i);

		qglBindTexture(GL_TEXTURE_2D, 0);
		qglBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
	}

	for (i = 0; i < r_numTextures; i++){
		texture = r_textures[i];

		qglDeleteTextures(1, &texture->texNum);

		Z_Free(texture);
	}

	memset(r_texturesHashTable, 0, sizeof(r_texturesHashTable));
	memset(r_textures, 0, sizeof(r_textures));

	r_numTextures = 0;
}
