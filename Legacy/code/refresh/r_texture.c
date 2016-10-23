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


#define TEXTURES_HASHSIZE	1024

#define NUM_TEXTURE_FILTERS	(sizeof(r_textureFilters) / sizeof(textureFilter_t))

typedef struct {
	char		*name;
	int			min;
	int			mag;
} textureFilter_t;

static texture_t		*r_texturesHash[TEXTURES_HASHSIZE];
static texture_t		*r_textures[MAX_TEXTURES];
static int				r_numTextures;

static textureFilter_t	r_textureFilters[] = {
	{"GL_NEAREST",					GL_NEAREST,					GL_NEAREST},
	{"GL_LINEAR",					GL_LINEAR,					GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST",	GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST",	GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR",	GL_NEAREST_MIPMAP_LINEAR,	GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR",		GL_LINEAR_MIPMAP_LINEAR,	GL_LINEAR},
};

static int				r_textureFilterMin = GL_LINEAR_MIPMAP_LINEAR;
static int				r_textureFilterMag = GL_LINEAR;

static byte				r_intensityTable[256];
static unsigned			r_palette[256];

const char				*r_skyBoxSuffix[6] = {"rt", "lf", "bk", "ft", "up", "dn"};
vec3_t					r_skyBoxAngles[6] = {
	{   0,   0,   0},
	{   0, 180,   0},
	{   0,  90,   0},
	{   0, 270,   0},
	{ -90,   0,   0},
	{  90,   0,   0}
};
const char				*r_cubeMapSuffix[6] = {"px", "nx", "py", "ny", "pz", "nz"};
vec3_t					r_cubeMapAngles[6] = {
	{   0, 180,  90},
	{   0,   0, 270},
	{   0,  90, 180},
	{   0, 270,   0},
	{ -90, 270,   0},
	{  90,  90,   0}
};

texture_t				*r_defaultTexture;
texture_t				*r_whiteTexture;
texture_t				*r_blackTexture;
texture_t				*r_rawTexture;
texture_t				*r_dlightTexture;
texture_t				*r_lightmapTextures[MAX_LIGHTMAPS];
texture_t				*r_normalizeTexture;


/*
 =================
 R_TextureFilter
 =================
*/
void R_TextureFilter (void){

	texture_t	*texture;
	int			i;

	for (i = 0; i < NUM_TEXTURE_FILTERS; i++){
		if (!Q_stricmp(r_textureFilters[i].name, r_textureFilter->string))
			break;
	}

	if (i == NUM_TEXTURE_FILTERS){
		Com_Printf("Bad texture filter name\n");

		Cvar_Set("r_textureFilter", "GL_LINEAR_MIPMAP_LINEAR");

		r_textureFilterMin = GL_LINEAR_MIPMAP_LINEAR;
		r_textureFilterMag = GL_LINEAR;
	}
	else {
		r_textureFilterMin = r_textureFilters[i].min;
		r_textureFilterMag = r_textureFilters[i].mag;
	}

	if (glConfig.textureFilterAnisotropic){
		if (r_textureFilterAnisotropy->value > glConfig.maxTextureMaxAnisotropy)
			Cvar_SetValue("r_textureFilterAnisotropy", glConfig.maxTextureMaxAnisotropy);
		else if (r_textureFilterAnisotropy->value < 1.0)
			Cvar_SetValue("r_textureFilterAnisotropy", 1.0);
	}

	// Change all the existing texture objects
	for (i = 0; i < r_numTextures; i++){
		texture = r_textures[i];

		GL_BindTexture(texture);

		if (texture->flags & TF_MIPMAPS){
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MIN_FILTER, r_textureFilterMin);
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MAG_FILTER, r_textureFilterMag);

			if (glConfig.textureFilterAnisotropic)
				qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_textureFilterAnisotropy->value);
		}
		else {
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MIN_FILTER, r_textureFilterMag);
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MAG_FILTER, r_textureFilterMag);
		}
	}
}

/*
 =================
 R_TextureList_f
 =================
*/
void R_TextureList_f (void){

	texture_t	*texture;
	int			i;
	int			texels = 0;

	Com_Printf("\n");
	Com_Printf("      -w-- -h-- -fmt- -t-- -mm- wrap -name--------\n");

	for (i = 0; i < r_numTextures; i++){
		texture = r_textures[i];

		if (texture->uploadTarget == GL_TEXTURE_2D)
			texels += (texture->uploadWidth * texture->uploadHeight);
		else
			texels += (texture->uploadWidth * texture->uploadHeight) * 6;

		Com_Printf("%4i: ", i);

		Com_Printf("%4i %4i ", texture->uploadWidth, texture->uploadHeight);

		switch (texture->uploadFormat){
		case GL_RGBA8:
			Com_Printf("RGBA8 ");
			break;
		case GL_RGBA4:
			Com_Printf("RGBA4 ");
			break;
		case GL_RGBA:
			Com_Printf("RGBA  ");
			break;
		case GL_COMPRESSED_RGBA_ARB:
			Com_Printf("CRGBA ");
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
			Com_Printf(" CM  ");
			break;
		default:
			Com_Printf(" ??  ");
			break;
		}

		if (texture->flags & TF_MIPMAPS)
			Com_Printf(" yes ");
		else
			Com_Printf(" no  ");

		if (texture->flags & TF_CLAMP)
			Com_Printf("clmp ");
		else
			Com_Printf("rept ");
		
		Com_Printf("%s\n", texture->realName);
	}

	Com_Printf("------------------------------------------------------\n");
	Com_Printf("%i total texels (not including mipmaps)\n", texels);
	Com_Printf("%i total textures\n", r_numTextures);
	Com_Printf("\n");
}


/*
 =======================================================================

 SCREEN AND ENVIRONMENT SHOTS

 =======================================================================
*/

static int		jpegSize;
static int		jpegCompressedSize;


static void jpeg_c_error_exit (j_common_ptr cinfo){

	char	msg[1024];

	(cinfo->err->format_message)(cinfo, msg);
	Com_Error(ERR_DROP, "JPEG Lib Error: %s", msg);
}

static void jpeg_null_dest (j_compress_ptr cinfo){

}

static boolean jpeg_empty_output_buffer (j_compress_ptr cinfo){

	return TRUE;
}

static void jpeg_term_destination (j_compress_ptr cinfo){

	jpegCompressedSize = jpegSize - cinfo->dest->free_in_buffer;
}

static void jpeg_mem_dest (j_compress_ptr cinfo, byte *outdata, int size){

	cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_destination_mgr));
	cinfo->dest->init_destination = jpeg_null_dest;
	cinfo->dest->empty_output_buffer = jpeg_empty_output_buffer;
	cinfo->dest->term_destination = jpeg_term_destination;
	cinfo->dest->free_in_buffer = jpegSize = size;
	cinfo->dest->next_output_byte = outdata;
}

/*
 =================
 R_WriteJPG
 =================
*/
static void R_WriteJPG (const char *name, byte *pic, int width, int height){

	struct	jpeg_compress_struct jComp;
	struct	jpeg_error_mgr jErr;
	byte	*buffer, *scanLine;
	int		stride, ofs;

	if (r_jpegCompressionQuality->integer > 100)
		Cvar_SetInteger("r_jpegCompressionQuality", 100);
	else if (r_jpegCompressionQuality->integer < 1)
		Cvar_SetInteger("r_jpegCompressionQuality", 1);

	buffer = Z_Malloc(width * height * 3);

	// Initialize
	jComp.err = jpeg_std_error(&jErr);
	jErr.error_exit = jpeg_c_error_exit;

	jpeg_create_compress(&jComp);
	jpeg_mem_dest(&jComp, buffer, 0);

	jComp.image_width = width;
	jComp.image_height = height;
	jComp.in_color_space = JCS_RGB;
	jComp.input_components = 3;
		
	jpeg_set_defaults(&jComp);
	jpeg_set_quality(&jComp, r_jpegCompressionQuality->integer, TRUE);
	jpeg_start_compress(&jComp, true);

	// Write the JPEG
	stride = jComp.image_width * 3;
	ofs = (stride * jComp.image_height) - stride;
	while (jComp.next_scanline < jComp.image_height){
		scanLine = &pic[ofs - (jComp.next_scanline * stride)];
		jpeg_write_scanlines(&jComp, &scanLine, 1);
	}

	// Free
	jpeg_finish_compress(&jComp);
	jpeg_destroy_compress(&jComp);

	if (!FS_SaveFile(name, buffer, jpegCompressedSize))
		Com_DPrintf(S_COLOR_RED "R_WriteJPG: couldn't write %s\n", name);

	Z_Free(buffer);
}

/*
 =================
 R_WriteTGA
 =================
*/
static void R_WriteTGA (const char *name, byte *pic, int width, int height){

	byte	tmp;
	int		i, c;

	memset(pic, 0, 18);

	pic[2] = 2;				// Uncompressed type
	pic[12] = width & 255;
	pic[13] = width >> 8;
	pic[14] = height & 255;
	pic[15] = height >> 8;
	pic[16] = 24;			// Pixel size

	// Swap RGB to BGR
	c = 18 + (width * height * 3);
	for (i = 18; i < c; i += 3){
		tmp = pic[i];
		pic[i] = pic[i+2];
		pic[i+2] = tmp;
	}

	if (!FS_SaveFile(name, pic, c))
		Com_DPrintf(S_COLOR_RED "R_WriteTGA: couldn't write %s\n", name);
}

/*
 =================
 R_ScreenShot_f
 =================
*/
void R_ScreenShot_f (void){

	byte		*buffer;
	char		name[MAX_QPATH];
	char		*extension;
	int			i, size, offset;
	qboolean	silent;
	void		(*WritePic)(const char *name, byte *pic, int width, int height);

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: screenshot [name/silent]\n");
		return;
	}

	if (!Q_stricmp(Cmd_Argv(0), "screenshot")){
		extension = ".tga";
		offset = 18;
		WritePic = R_WriteTGA;
	}
	else if (!Q_stricmp(Cmd_Argv(0), "screenshotJPEG")){
		extension = ".jpg";
		offset = 0;
		WritePic = R_WriteJPG;
	}

	if (Cmd_Argc() == 2){
		if (!Q_stricmp(Cmd_Argv(1), "silent")){
			silent = true;
			name[0] = 0;
		}
		else {
			silent = false;
			Q_snprintfz(name, sizeof(name), "screenshots/%s", Cmd_Argv(1));
			Com_DefaultExtension(name, sizeof(name), extension);
		}
	}
	else {
		silent = false;
		name[0] = 0;
	}

	if (!name[0]){
		// Find a file name to save it to
		for (i = 0; i <= 9999; i++){
			Q_snprintfz(name, sizeof(name), "screenshots/q2e_shot%04i%s", i, extension);
			if (FS_LoadFile(name, NULL) == -1)
				break;	// File doesn't exist
		} 

		if (i == 10000){
			Com_Printf("Screenshots directory is full!\n");
			return;
 		}
	}

	// Write the pic
	size = offset + (glConfig.videoWidth * glConfig.videoHeight * 3);
	buffer = Z_Malloc(size);

	qglReadPixels(0, 0, glConfig.videoWidth, glConfig.videoHeight, GL_RGB, GL_UNSIGNED_BYTE, buffer + offset);

	if (glConfig.deviceSupportsGamma){
		// Apply gamma
		for (i = offset; i < size; i += 3){
			buffer[i+0] = glState.gammaRamp[buffer[i+0]] >> 8;
			buffer[i+1] = glState.gammaRamp[buffer[i+1] + 256] >> 8;
			buffer[i+2] = glState.gammaRamp[buffer[i+2] + 512] >> 8;
		}
	}

	WritePic(name, buffer, glConfig.videoWidth, glConfig.videoHeight);
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

	byte		*buffer;
	char		base[MAX_QPATH], name[MAX_QPATH];
	int			size, i;
	qboolean	isSky;

	if (Cmd_Argc() != 4){
		Com_Printf("Usage: envshot <skybox/cubemap> <basename> <size>\n");
		return;
	}

	if ((r_refDef.rdFlags & RDF_NOWORLDMODEL) || !r_worldModel){
		Com_Printf("You must be in a map to use this command\n");
		return;
	}

	if (!Q_stricmp(Cmd_Argv(1), "skybox"))
		isSky = true;
	else if (!Q_stricmp(Cmd_Argv(1), "cubemap"))
		isSky = false;
	else {
		Com_Printf("You must specify either 'skybox' or 'cubemap'\n");
		return;
	}

	Q_strncpyz(base, Cmd_Argv(2), sizeof(base));
	size = atoi(Cmd_Argv(3));

	// Make sure the specified size is valid
	i = 1;
	while (i < size)
		i <<= 1;

	if (i != size){
		Com_Printf("Specified 'size' is not a power of two value\n");
		return;
	}
	if (size > glConfig.videoWidth || size > glConfig.videoHeight){
		Com_Printf("Specified 'size' is greater than current resolution\n");
		return;
	}

	// Set up refDef
	r_refDef.x = 0;
	r_refDef.y = 0;
	r_refDef.width = size;
	r_refDef.height = size;
	r_refDef.fovX = 90;
	r_refDef.fovY = 90;

	// Render the scene for each face of the environment box
	buffer = Z_Malloc(18 + (size * size * 3));

	for (i = 0; i < 6; i++){
		if (isSky){
			Q_snprintfz(name, sizeof(name), "env/%s%s.tga", base, r_skyBoxSuffix[i]);
			AnglesToAxis(r_skyBoxAngles[i], r_refDef.viewAxis);
		}
		else {
			Q_snprintfz(name, sizeof(name), "env/%s_%s.tga", base, r_cubeMapSuffix[i]);
			AnglesToAxis(r_cubeMapAngles[i], r_refDef.viewAxis);
		}

		R_RenderView();

		// Write the texture
		qglReadPixels(0, glConfig.videoHeight - size, size, size, GL_RGB, GL_UNSIGNED_BYTE, buffer + 18);
		R_WriteTGA(name, buffer, size, size);
	}

	Z_Free(buffer);

	Com_Printf("EnvShot: wrote env/%s*.tga\n", base);
}


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
static qboolean R_LoadPCX (const char *name, byte **pic, byte **palette, int *width, int *height){

	byte		*buffer;
	byte		*in, *out;
	pcxHeader_t	*pcx;
	int			x, y, len;
	int			dataByte, runLength;

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
		
	if (pcx->xMax >= 640 || pcx->yMax >= 480 || pcx->xMax <= 0 || pcx->yMax <= 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadPCX: bad PCX file (%i x %i) (%s)\n", pcx->xMax, pcx->yMax, name);
		FS_FreeFile(buffer);
		return false;
	}

	if (palette){
		*palette = Z_Malloc(768);
		memcpy(*palette, (byte *)buffer + len - 768, 768);
	}

	if (!pic){
		FS_FreeFile(buffer);
		return true;	// Because only the palette was requested
	}

	*width = pcx->xMax+1;
	*height = pcx->yMax+1;

	*pic = out = Z_Malloc((pcx->xMax+1) * (pcx->yMax+1) * 4);

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

				out += 4;
				x++;
			}
		}
	}

	if (in - buffer > len){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadPCX: PCX file was malformed (%s)\n", name);
		FS_FreeFile(buffer);
		Z_Free(*pic);
		*pic = NULL;
		return false;
	}

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
static qboolean R_LoadWAL (const char *name, byte **pic, int *width, int *height){
	
	byte		*buffer;
	byte		*in, *out;
	mipTex_t	*mt;
	int			i, c;

	// Load the file
	FS_LoadFile(name, (void **)&buffer);
	if (!buffer)
		return false;

	// Parse the WAL file
	mt = (mipTex_t *)buffer;

	mt->width = LittleLong(mt->width);
	mt->height = LittleLong(mt->height);
	mt->offsets[0] = LittleLong(mt->offsets[0]);

	if (mt->width == 0 || mt->height == 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadWAL: bad WAL file (%i x %i) (%s)\n", mt->width, mt->height, name);
		FS_FreeFile(buffer);
		return false;
	}

	*width = mt->width;
	*height = mt->height;

	*pic = out = Z_Malloc(mt->width * mt->height * 4);

	in = buffer + mt->offsets[0];
	c = mt->width * mt->height;

	for (i = 0; i < c; i++, in++, out += 4)
		*(unsigned *)out = r_palette[*in];

	FS_FreeFile(buffer);
	return true;
}


/*
 =======================================================================

 JPEG LOADING

 =======================================================================
*/

static char		jpegName[MAX_QPATH];
static qboolean	jpegFailed;


static void jpeg_d_error_exit (j_common_ptr cinfo){

	char	msg[1024];

	(cinfo->err->format_message)(cinfo, msg);
	Com_Error(ERR_DROP, "JPEG Lib Error: %s", msg);
}

static void jpeg_null_src (j_decompress_ptr cinfo){

}

static unsigned char jpeg_fill_input_buffer (j_decompress_ptr cinfo){

	Com_DPrintf(S_COLOR_YELLOW "R_LoadJPG: premature end of JPG file (%s)\n", jpegName);
	jpegFailed = true;

    return 1;
}

static void jpeg_skip_input_data (j_decompress_ptr cinfo, long num_bytes){
        
    cinfo->src->next_input_byte += (size_t)num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t)num_bytes;

	if (cinfo->src->bytes_in_buffer < 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadJPG: premature end of JPG file (%s)\n", jpegName);
		jpegFailed = true;
	}
}

static void jpeg_mem_src (j_decompress_ptr cinfo, byte *indata, int size){

    cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
    cinfo->src->init_source = jpeg_null_src;
    cinfo->src->fill_input_buffer = jpeg_fill_input_buffer;
    cinfo->src->skip_input_data = jpeg_skip_input_data;
    cinfo->src->resync_to_restart = jpeg_resync_to_restart;
    cinfo->src->term_source = jpeg_null_src;
    cinfo->src->bytes_in_buffer = size;
    cinfo->src->next_input_byte = indata;
}

/*
 =================
 R_LoadJPG
 =================
*/
static qboolean R_LoadJPG (const char *name, byte **pic, int *width, int *height){

	struct	jpeg_decompress_struct jDec;
	struct	jpeg_error_mgr jErr;
	byte	*buffer;
	byte	*in, *out;
	byte	*scanLine;
	int		len, i;

	// Load the file
	len = FS_LoadFile(name, (void **)&buffer);
	if (!buffer)
		return false;

	// Parse the JPG file
	if (buffer[6] != 'J' || buffer[7] != 'F' || buffer[8] != 'I' || buffer[9] != 'F'){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadJPG: missing 'JFIF' signature (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	// Initialize
	Q_strncpyz(jpegName, name, sizeof(jpegName));
	jpegFailed = false;

	jDec.err = jpeg_std_error(&jErr);
	jErr.error_exit = jpeg_d_error_exit;

	jpeg_create_decompress(&jDec);

	jpeg_mem_src(&jDec, buffer, len);
	jpeg_read_header(&jDec, true);

	jpeg_start_decompress(&jDec);

	if (jDec.output_components != 3 && jDec.output_components != 1){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadJPG: bad number of output components (%i) (%s)\n", jDec.output_components, name);
		jpeg_finish_decompress(&jDec);
		jpeg_destroy_decompress(&jDec);
		FS_FreeFile(buffer);
		return false;
	}

	if (jDec.output_width <= 0 || jDec.output_height <= 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadJPG: bad JPG file (%i x %i) (%s)\n", jDec.output_width, jDec.output_height, name);
		jpeg_finish_decompress(&jDec);
		jpeg_destroy_decompress(&jDec);
		FS_FreeFile(buffer);
		return false;
	}

	*width = jDec.output_width; 
	*height = jDec.output_height;

	*pic = out = Z_Malloc(jDec.output_width * jDec.output_height * 4);

	// Read the JPEG
	if (jDec.output_components == 1){
		in = Z_Malloc(jDec.output_width);

		while (jDec.output_scanline < jDec.output_height){
			scanLine = in;
			jpeg_read_scanlines(&jDec, &scanLine, 1);

			for (i = 0; i < jDec.output_width; i++){
				out[0] = scanLine[0];
				out[1] = scanLine[0];
				out[2] = scanLine[0];
				out[3] = 255;

				scanLine += 1;
				out += 4;
			}
		}
	}
	else if (jDec.output_components == 3){
		in = Z_Malloc(jDec.output_width * 3);

		while (jDec.output_scanline < jDec.output_height){
			scanLine = in;
			jpeg_read_scanlines(&jDec, &scanLine, 1);

			for (i = 0; i < jDec.output_width; i++){
				out[0] = scanLine[0];
				out[1] = scanLine[1];
				out[2] = scanLine[2];
				out[3] = 255;

				scanLine += 3;
				out += 4;
			}
		}
	}

	// Free
	Z_Free(in);

	jpeg_finish_decompress(&jDec);
	jpeg_destroy_decompress(&jDec);

	FS_FreeFile(buffer);
	return (!jpegFailed);
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
static qboolean R_LoadTGA (const char *name, byte **pic, int *width, int *height){

	byte			*buffer;
	byte			*in, *out;
	targaHeader_t	tga;
	int				w, h, stride = 0;
	byte			r, g, b, a;
	byte			packetHeader, packetSize, i;

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

	if (tga.imageType != 2 && tga.imageType != 3 && tga.imageType != 10){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadTGA: only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	if (tga.colormapType != 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadTGA: colormaps not supported (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}
		
	if (tga.pixelSize != 32 && tga.pixelSize != 24 && tga.pixelSize != 8){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadTGA: only 32 (RGBA), 24 (RGB), and 8 (gray) bit images supported (%s)\n", name);
		FS_FreeFile(buffer);
		return false;
	}

	if (tga.width <= 0 || tga.height <= 0){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadTGA: bad TGA file (%i x %i) (%s)\n", tga.width, tga.height, name);
		FS_FreeFile(buffer);
		return false;
	}

	*width = tga.width;
	*height = tga.height;

	*pic = out = Z_Malloc(tga.width * tga.height * 4);

	if (tga.idLength != 0)
		in += tga.idLength;		// Skip TGA image comment

	if (!(tga.attributes & 0x20)){
		// Flipped image
		out += (tga.height-1) * tga.width * 4;
		stride = -tga.width * 4 * 2;
	}
	
	if (tga.imageType == 2 || tga.imageType == 3){
		// Uncompressed RGB or grayscale image
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

				out += 4;
			}

			out += stride;
		}
	}
	else if (tga.imageType == 10){   
		// Run-Length encoded RGB image
		for (h = 0; h < tga.height; h++){
			for (w = 0; w < tga.width; ){
				packetHeader = *in++;
				packetSize = 1 + (packetHeader & 0x7F);
				
				if (packetHeader & 0x80){        // Run-Length packet
					switch (tga.pixelSize){
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

	FS_FreeFile(buffer);
	return true;
}


// =====================================================================


/*
 =================
 R_ResampleTexture
 =================
*/
static void R_ResampleTexture (unsigned *in, int inWidth, int inHeight, unsigned *out, int outWidth, int outHeight){

	int			i, j;
	unsigned	*inRow1, *inRow2;
	unsigned	frac, fracStep;
	unsigned	p1[0x2000], p2[0x2000];
	byte		*pix1, *pix2, *pix3, *pix4;

	fracStep = inWidth * 0x10000 / outWidth;

	frac = fracStep>>2;
	for (i = 0; i < outWidth; i++){
		p1[i] = 4 * (frac>>16);
		frac += fracStep;
	}

	frac = (fracStep>>2) * 3;
	for (i = 0; i < outWidth; i++){
		p2[i] = 4 * (frac>>16);
		frac += fracStep;
	}

	for (i = 0; i < outHeight; i++, out += outWidth){
		inRow1 = in + inWidth * (int)((i+0.25) * inHeight/outHeight);
		inRow2 = in + inWidth * (int)((i+0.75) * inHeight/outHeight);

		for (j = 0; j < outWidth; j++){
			pix1 = (byte *)inRow1 + p1[j];
			pix2 = (byte *)inRow1 + p2[j];
			pix3 = (byte *)inRow2 + p1[j];
			pix4 = (byte *)inRow2 + p2[j];

			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
		}
	}
}

/*
 =================
 R_MipMapTexture

 Operates in place, quartering the size of the texture
 =================
*/
static void R_MipMapTexture (byte *in, int width, int height){

	int		i, j;
	byte	*out = in;

	width <<= 2;
	height >>= 1;

	for (i = 0; i < height; i++, in += width){
		for (j = 0; j < width; j += 8, in += 8, out += 4){
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4]) >> 2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5]) >> 2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6]) >> 2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7]) >> 2;
		}
	}
}

/*
 =================
 R_IntensityScaleTexture
 =================
*/
static void R_IntensityScaleTexture (byte *in, int width, int height){

	int		i, c;
	byte	*out = in;

	if (r_intensity->value == 1.0)
		return;

	c = width * height;

	for (i = 0; i < c; i++, in += 4, out += 4){
		out[0] = r_intensityTable[in[0]];
		out[1] = r_intensityTable[in[1]];
		out[2] = r_intensityTable[in[2]];
	}
}

/*
 =================
 R_UploadTexture
 =================
*/
static void R_UploadTexture (unsigned **data, int numFaces, texture_t *texture){

	unsigned	*texImage;
	unsigned	texTarget;
	int			mipWidth, mipHeight, mipLevel;
	int			i;

	// Find nearest power of two
	texture->uploadWidth = 1;
	while (texture->uploadWidth < texture->sourceWidth)
		texture->uploadWidth <<= 1;

	texture->uploadHeight = 1;
	while (texture->uploadHeight < texture->sourceHeight)
		texture->uploadHeight <<= 1;

	// Round down
	if (r_roundImagesDown->integer){
		if (texture->uploadWidth > texture->sourceWidth)
			texture->uploadWidth >>= 1;
		if (texture->uploadHeight > texture->sourceHeight)
			texture->uploadHeight >>= 1;
	}

	// Sample down and apply picmip if desired
	if (texture->flags & TF_PICMIP){
		if (r_maxTextureSize->integer > 0){
			while (texture->uploadWidth > r_maxTextureSize->integer || texture->uploadHeight > r_maxTextureSize->integer){
				texture->uploadWidth >>= 1;
				texture->uploadHeight >>= 1;
			}
		}

		if (r_picmip->integer > 0){
			texture->uploadWidth >>= r_picmip->integer;
			texture->uploadHeight >>= r_picmip->integer;
		}
	}

	// Clamp to hardware limits
	if (texture->flags & TF_CUBEMAP){
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

	// Set texture format
	if ((texture->flags & TF_COMPRESS) && glConfig.textureCompression)
		texture->uploadFormat = GL_COMPRESSED_RGBA_ARB;
	else {
		if (r_textureBits->integer == 32)
			texture->uploadFormat = GL_RGBA8;
		else if (r_textureBits->integer == 16)
			texture->uploadFormat = GL_RGBA4;
		else
			texture->uploadFormat = GL_RGBA;
	}

	// Set texture target
	if (texture->flags & TF_CUBEMAP){
		texture->uploadTarget = GL_TEXTURE_CUBE_MAP_ARB;

		texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
	}
	else {
		texture->uploadTarget = GL_TEXTURE_2D;

		texTarget = GL_TEXTURE_2D;
	}

	// Bind the texture
	GL_BindTexture(texture);

	// Upload all the faces
	for (i = 0; i < numFaces; i++){
		// Copy or resample the texture
		if (texture->uploadWidth == texture->sourceWidth && texture->uploadHeight == texture->sourceHeight)
			texImage = data[i];
		else {
			texImage = Z_Malloc(texture->uploadWidth * texture->uploadHeight * 4);

			R_ResampleTexture(data[i], texture->sourceWidth, texture->sourceHeight, texImage, texture->uploadWidth, texture->uploadHeight);
		}

		// Apply intensity if needed
		if ((texture->flags & TF_MIPMAPS) && !(texture->flags & TF_NORMALMAP))
			R_IntensityScaleTexture(texImage, texture->uploadWidth, texture->uploadHeight);

		// Upload the texture and generate mipmaps if desired
		if (!(texture->flags & TF_MIPMAPS)){
			if (glConfig.generateMipmap)
				qglTexParameterf(texture->uploadTarget, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);

			qglTexImage2D(texTarget + i, 0, texture->uploadFormat, texture->uploadWidth, texture->uploadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texImage);
		}
		else {
			if (glConfig.generateMipmap)
				qglTexParameterf(texture->uploadTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

			qglTexImage2D(texTarget + i, 0, texture->uploadFormat, texture->uploadWidth, texture->uploadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texImage);

			if (!glConfig.generateMipmap){
				mipWidth = texture->uploadWidth;
				mipHeight = texture->uploadHeight;
				mipLevel = 0;

				while (mipWidth > 1 || mipHeight > 1){
					R_MipMapTexture(texImage, mipWidth, mipHeight);

					mipWidth >>= 1;
					mipHeight >>= 1;

					if (mipWidth < 1)
						mipWidth = 1;
					if (mipHeight < 1)
						mipHeight = 1;

					mipLevel++;

					qglTexImage2D(texTarget + i, mipLevel, texture->uploadFormat, mipWidth, mipHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texImage);
				}
			}
		}

		// Free texture data
		if (texImage != data[i])
			Z_Free(texImage);
	}

	// Set texture filter
	if (texture->flags & TF_MIPMAPS){
		qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MIN_FILTER, r_textureFilterMin);
		qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MAG_FILTER, r_textureFilterMag);

		if (glConfig.textureFilterAnisotropic)
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_textureFilterAnisotropy->value);
	}
	else {
		qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MIN_FILTER, r_textureFilterMag);
		qglTexParameterf(texture->uploadTarget, GL_TEXTURE_MAG_FILTER, r_textureFilterMag);
	}

	// Set texture wrap mode
	if (texture->flags & TF_CLAMP){
		if (glConfig.textureEdgeClamp){
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		else {
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
			qglTexParameterf(texture->uploadTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
	}
	else {
		qglTexParameterf(texture->uploadTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameterf(texture->uploadTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
}

/*
 =================
 R_HeightToNormal

 Assumes the input is a grayscale image converted to RGBA
 =================
*/
static byte *R_HeightToNormal (byte *in, int width, int height, float bumpScale){

	int		i, j;
	byte	*out;
	vec3_t	normal;
	float	invLength;
	float	c, cx, cy;

	out = Z_Malloc(width * height * 4);

	for (i = 0; i < height; i++){
		for (j = 0; j < width; j++){
			c = in[4*(i*width+j)] * (1.0/255);

			cx = in[4*(i*width+(j+1)%width)] * (1.0/255);
			cy = in[4*(((i+1)%height)*width+j)] * (1.0/255);

			cx = (c - cx) * bumpScale;
			cy = (c - cy) * bumpScale;

			invLength = 1.0 / sqrt(cx*cx + cy*cy + 1.0);

			VectorSet(normal, cx * invLength, -cy * invLength, invLength);

			out[4*(i*width+j)+0] = (byte)(127.5 * (normal[0] + 1.0));
			out[4*(i*width+j)+1] = (byte)(127.5 * (normal[1] + 1.0));
			out[4*(i*width+j)+2] = (byte)(127.5 * (normal[2] + 1.0));
			out[4*(i*width+j)+3] = in[4*(i*width+j)+3];
		}
	}

	Z_Free(in);

	return out;
}

/*
 =================
 R_LoadTexture
 =================
*/
texture_t *R_LoadTexture (const char *name, byte *data, int width, int height, unsigned flags, float bumpScale){

	texture_t	*texture;
	unsigned	hashKey;

	if (r_numTextures == MAX_TEXTURES)
		Com_Error(ERR_DROP, "R_LoadTexture: MAX_TEXTURES hit");

	r_textures[r_numTextures++] = texture = Hunk_Alloc(sizeof(texture_t));

	// Fill it in
	Com_StripExtension(name, texture->name, sizeof(texture->name));
	Q_strncpyz(texture->realName, name, sizeof(texture->realName));
	texture->flags = flags;
	texture->bumpScale = bumpScale;
	texture->sourceWidth = width;
	texture->sourceHeight = height;

	qglGenTextures(1, &texture->texNum);

	R_UploadTexture(&data, 1, texture);

	// Add to hash table
	hashKey = Com_HashKey(texture->name, TEXTURES_HASHSIZE);

	texture->nextHash = r_texturesHash[hashKey];
	r_texturesHash[hashKey] = texture;

	return texture;
}

/*
 =================
 R_LoadCubeMapTexture
 =================
*/
texture_t *R_LoadCubeMapTexture (const char *name, byte *data[6], int width, int height, unsigned flags, float bumpScale){

	texture_t	*texture;
	unsigned	hashKey;

	if (r_numTextures == MAX_TEXTURES)
		Com_Error(ERR_DROP, "R_LoadCubeMapTexture: MAX_TEXTURES hit");

	r_textures[r_numTextures++] = texture = Hunk_Alloc(sizeof(texture_t));

	// Fill it in
	Com_StripExtension(name, texture->name, sizeof(texture->name));
	Q_strncpyz(texture->realName, name, sizeof(texture->realName));
	texture->flags = flags;
	texture->bumpScale = bumpScale;
	texture->sourceWidth = width;
	texture->sourceHeight = height;

	qglGenTextures(1, &texture->texNum);

	R_UploadTexture(data, 6, texture);

	// Add to hash table
	hashKey = Com_HashKey(texture->name, TEXTURES_HASHSIZE);

	texture->nextHash = r_texturesHash[hashKey];
	r_texturesHash[hashKey] = texture;

	return texture;
}

/*
 =================
 R_FindTexture
 =================
*/
texture_t *R_FindTexture (const char *name, unsigned flags, float bumpScale){

	texture_t	*texture;
	byte		*pic;
	int			width, height;
	char		checkName[MAX_QPATH], loadName[MAX_QPATH];
	unsigned	hashKey;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindTexture: NULL texture name");

	if (strlen(name) >= MAX_QPATH)
		Com_Error(ERR_DROP, "R_FindTexture: texture name exceeds MAX_QPATH");

	// Strip extension
	Com_StripExtension(name, checkName, sizeof(checkName));

	// See if already loaded
	hashKey = Com_HashKey(checkName, TEXTURES_HASHSIZE);

	for (texture = r_texturesHash[hashKey]; texture; texture = texture->nextHash){
		if (!Q_stricmp(texture->name, checkName)){
			if (texture->flags == flags && texture->bumpScale == bumpScale)
				return texture;
		}
	}

	// Load it from disk
	Q_snprintfz(loadName, sizeof(loadName), "%s.tga", checkName);
	if (R_LoadTGA(loadName, &pic, &width, &height)){
		if (flags & TF_HEIGHTMAP)
			pic = R_HeightToNormal(pic, width, height, bumpScale);

		texture = R_LoadTexture(loadName, pic, width, height, flags, bumpScale);
		Z_Free(pic);
		return texture;
	}

	Q_snprintfz(loadName, sizeof(loadName), "%s.jpg", checkName);
	if (R_LoadJPG(loadName, &pic, &width, &height)){
		if (flags & TF_HEIGHTMAP)
			pic = R_HeightToNormal(pic, width, height, bumpScale);

		texture = R_LoadTexture(loadName, pic, width, height, flags, bumpScale);
		Z_Free(pic);
		return texture;
	}

	Q_snprintfz(loadName, sizeof(loadName), "%s.pcx", checkName);
	if (R_LoadPCX(loadName, &pic, NULL, &width, &height)){
		if (flags & TF_HEIGHTMAP)
			pic = R_HeightToNormal(pic, width, height, bumpScale);

		texture = R_LoadTexture(loadName, pic, width, height, flags, bumpScale);
		Z_Free(pic);
		return texture;
	}
	
	Q_snprintfz(loadName, sizeof(loadName), "%s.wal", checkName);
	if (R_LoadWAL(loadName, &pic, &width, &height)){
		if (flags & TF_HEIGHTMAP)
			pic = R_HeightToNormal(pic, width, height, bumpScale);

		texture = R_LoadTexture(loadName, pic, width, height, flags, bumpScale);
		Z_Free(pic);
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
texture_t *R_FindCubeMapTexture (const char *name, unsigned flags, float bumpScale){

	texture_t	*texture;
	byte		*pics[6];
	int			width[6], height[6];
	char		checkName[MAX_QPATH], loadName[MAX_QPATH];
	unsigned	hashKey;
	int			i, j;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindCubeMapTexture: NULL texture name");

	if (strlen(name) >= MAX_QPATH)
		Com_Error(ERR_DROP, "R_FindCubeMapTexture: texture name exceeds MAX_QPATH");

	// Strip extension
	Com_StripExtension(name, checkName, sizeof(checkName));

	// See if already loaded
	hashKey = Com_HashKey(checkName, TEXTURES_HASHSIZE);

	for (texture = r_texturesHash[hashKey]; texture; texture = texture->nextHash){
		if (!Q_stricmp(texture->name, checkName)){
			if (texture->flags == flags && texture->bumpScale == bumpScale)
				return texture;
		}
	}

	// Load it from disk
	for (i = 0; i < 6; i++){
		Q_snprintfz(loadName, sizeof(loadName), "%s_%s.tga", checkName, r_cubeMapSuffix[i]);
		if (R_LoadTGA(loadName, &pics[i], &width[i], &height[i])){
			if (flags & TF_HEIGHTMAP)
				pics[i] = R_HeightToNormal(pics[i], width[i], height[i], bumpScale);

			continue;
		}

		Q_snprintfz(loadName, sizeof(loadName), "%s_%s.jpg", checkName, r_cubeMapSuffix[i]);
		if (R_LoadJPG(loadName, &pics[i], &width[i], &height[i])){
			if (flags & TF_HEIGHTMAP)
				pics[i] = R_HeightToNormal(pics[i], width[i], height[i], bumpScale);

			continue;
		}

		pics[i] = NULL;
	}

	// Check all faces
	for (i = 0; i < 6; i++){
		// Check if missing
		if (!pics[i]){
			if (i != 0)
				Com_Printf(S_COLOR_RED "Cube map texture face '%s_%s' is missing\n", checkName, r_cubeMapSuffix[i]);

			break;
		}

		// Check dimensions
		if (width[i] != height[i]){
			Com_Printf(S_COLOR_RED "Cube map texture face '%s_%s' is not square (%i != %i)\n", checkName, r_cubeMapSuffix[i], width[i], height[i]);
			break;
		}

		// Compare dimensions with previous faces
		for (j = 0; j < i; j++){
			if (width[i] != width[j] || height[i] != height[j]){
				Com_Printf(S_COLOR_RED "Cube map texture faces '%s_%s' and '%s_%s' have mismatched dimensions (%i x %i != %i x %i)\n", checkName, r_cubeMapSuffix[i], checkName, r_cubeMapSuffix[j], width[i], height[i], width[j], height[j]);
				break;
			}
		}
	}

	if (i == 6){
		texture = R_LoadCubeMapTexture(checkName, pics, width[0], height[0], flags, bumpScale);

		for (i = 0; i < 6; i++)
			Z_Free(pics[i]);

		return texture;
	}

	// Not found or invalid
	for (i = 0; i < 6; i++){
		if (pics[i])
			Z_Free(pics[i]);
	}

	return NULL;
}

/*
 =================
 R_CreateBuiltInTextures
 =================
*/
static void R_CreateBuiltInTextures (void){

	byte	data2D[256*256*4];
	byte	*dataCM[6];
	vec3_t	normal;
	float	s, t;
	int		i, x, y;

	// Default texture
	i = 0;
	for (x = 0; x < 16; x++){
		for (y = 0; y < 16; y++){
			if (x == 0 || x == 15 || y == 0 || y == 15)
				((unsigned *)&data2D)[i++] = LittleLong(0xffffffff);
			else
				((unsigned *)&data2D)[i++] = LittleLong(0xff000000);
		}
	}

	r_defaultTexture = R_LoadTexture("*default", data2D, 16, 16, TF_MIPMAPS, 0);

	// White texture
	for (i = 0; i < 64; i++)
		((unsigned *)&data2D)[i] = LittleLong(0xffffffff);

	r_whiteTexture = R_LoadTexture("*white", data2D, 8, 8, 0, 0);

	// Black texture
	for (i = 0; i < 64; i++)
		((unsigned *)&data2D)[i] = LittleLong(0xff000000);

	r_blackTexture = R_LoadTexture("*black", data2D, 8, 8, 0, 0);

	// Raw texture
	memset(data2D, 255, 256*256*4);
	r_rawTexture = R_LoadTexture("*raw", data2D, 256, 256, 0, 0);

	// Dynamic light texture
	memset(data2D, 255, 128*128*4);
	r_dlightTexture = R_LoadTexture("*dlight", data2D, 128, 128, TF_CLAMP, 0);

	if (glConfig.textureCubeMap){
		// Normalize texture
		for (i = 0; i < 6; i++){
			dataCM[i] = Z_Malloc(128*128*4);

			for (y = 0; y < 128; y++){
				for (x = 0; x < 128; x++){
					s = (((float)x + 0.5) / 128.0) * 2.0 - 1.0;
					t = (((float)y + 0.5) / 128.0) * 2.0 - 1.0;

					switch (i){
					case 0:
						VectorSet(normal, 1.0, -t, -s);
						break;
					case 1:
						VectorSet(normal, -1.0, -t, s);
						break;
					case 2:
						VectorSet(normal, s, 1.0, t);
						break;
					case 3:
						VectorSet(normal, s, -1.0, -t);
						break;
					case 4:
						VectorSet(normal, s, -t, 1.0);
						break;
					case 5:
						VectorSet(normal, -s, -t, -1.0);
						break;
					}

					VectorNormalize(normal);

					dataCM[i][4*(y*128+x)+0] = (byte)(127.5 * (normal[0] + 1.0));
					dataCM[i][4*(y*128+x)+1] = (byte)(127.5 * (normal[1] + 1.0));
					dataCM[i][4*(y*128+x)+2] = (byte)(127.5 * (normal[2] + 1.0));
					dataCM[i][4*(y*128+x)+3] = 255;
				}
			}
		}

		r_normalizeTexture = R_LoadCubeMapTexture("*normalize", dataCM, 128, 128, TF_CLAMP | TF_CUBEMAP, 0);

		for (i = 0; i < 6; i++)
			Z_Free(dataCM[i]);
	}

	if (glConfig.textureRectangle)
		// Screen rect texture (just reserve a slot)
		qglGenTextures(1, &glState.screenRectTexture);
}

/*
 =================
 R_InitTextures
 =================
*/
void R_InitTextures (void){

	int			i;
	unsigned	v;
	int			r, g, b;
	byte		*palette;
	byte		q2palette[] = {
#include "palette.h"
	};

	// Build intensity table
	for (i = 0; i < 256; i++){
		v = i * r_intensity->value;
		if (v > 255)
			v = 255;

		r_intensityTable[i] = v;
	}

	// Load the palette
	if (!R_LoadPCX("pics/colormap.pcx", NULL, &palette, NULL, NULL))
		palette = q2palette;

	for (i = 0; i < 256; i++){
		r = palette[i*3+0];
		g = palette[i*3+1];
		b = palette[i*3+2];

		v = (r << 0) + (g << 8) + (b << 16) + (255 << 24);
		r_palette[i] = LittleLong(v);
	}
	r_palette[255] &= LittleLong(0x00ffffff);	// 255 is transparent

	if (palette != q2palette)
		Z_Free(palette);

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

	if (glConfig.textureRectangle)
		qglDeleteTextures(1, &glState.screenRectTexture);

	for (i = 0; i < r_numTextures; i++){
		texture = r_textures[i];

		qglDeleteTextures(1, &texture->texNum);
	}

	memset(r_texturesHash, 0, sizeof(r_texturesHash));
	memset(r_textures, 0, sizeof(r_textures));

	r_numTextures = 0;
}
