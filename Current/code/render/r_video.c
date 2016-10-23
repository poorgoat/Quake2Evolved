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


static int			r_vrTable[256];
static int			r_ugTable[256];
static int			r_vgTable[256];
static int			r_ubTable[256];

static video_t		*r_videosHashTable[VIDEOS_HASH_SIZE];
static video_t		*r_videos[MAX_VIDEOS];
static int			r_numVideos;


/*
 =================
 R_ReadInfo
 =================
*/
static void R_ReadInfo (video_t *video){

	byte	data[8];

	FS_Read(data, sizeof(data), video->file);
	video->offset += sizeof(data);

	if (video->roqCache)
		return;		// Already allocated

	// Allocate the buffers
	video->roqWidth = data[0] + (data[1] << 8);
	video->roqHeight = data[2] + (data[3] << 8);

	video->roqCache = Hunk_Alloc(video->roqWidth * video->roqHeight * 4 * 2);

	video->roqBuffers[0] = video->roqCache;
	video->roqBuffers[1] = video->roqCache + (video->roqWidth * video->roqHeight * 4);

	// Check if we need to sample down
	if (glConfig.textureNonPowerOfTwo){
		video->width = video->roqWidth;
		video->height = video->roqHeight;
	}
	else {
		video->width = NearestPowerOfTwo(video->roqWidth, true);
		video->height = NearestPowerOfTwo(video->roqHeight, true);
	}

	while (video->width > glConfig.maxTextureSize || video->height > glConfig.maxTextureSize){
		video->width >>= 1;
		video->height >>= 1;
	}

	if (video->width < 1)
		video->width = 1;
	if (video->height < 1)
		video->height = 1;

	if (video->width != video->roqWidth || video->height != video->roqHeight)
		video->buffer = Hunk_Alloc(video->width * video->height * 4);
}

/*
 =================
 R_ReadCodebook
 =================
*/
static void R_ReadCodebook (video_t *video){

	roqChunk_t	*chunk = &video->roqChunk;
	int			n1, n2;

	n1 = (chunk->argument >> 8) & 0xFF;
	if (!n1)
		n1 = 256;

	n2 = chunk->argument & 0xFF;
	if (!n2 && (n1 * 6 < chunk->size))
		n2 = 256;

	FS_Read(video->roqCells, sizeof(roqCell_t) * n1, video->file);
	FS_Read(video->roqQCells, sizeof(roqQCell_t) * n2, video->file);
	video->offset += chunk->size;
}

/*
 =================
 R_DecodeBlock
 =================
*/
static void R_DecodeBlock (byte *dst0, byte *dst1, int y1, int y2, int y3, int y4, int u, int v){

	int		r, g, b;

	// Convert YUV to RGB
	r = r_vrTable[v];
	g = r_ugTable[u] - r_vgTable[v];
	b = r_ubTable[u];

	// 1st pixel
	dst0[0] = Clamp(r + y1, 0, 255);
	dst0[1] = Clamp(g + y1, 0, 255);
	dst0[2] = Clamp(b + y1, 0, 255);
	dst0[3] = 255;

	// 2nd pixel
	dst0[4] = Clamp(r + y2, 0, 255);
	dst0[5] = Clamp(g + y2, 0, 255);
	dst0[6] = Clamp(b + y2, 0, 255);
	dst0[7] = 255;

	// 3rd pixel
	dst1[0] = Clamp(r + y3, 0, 255);
	dst1[1] = Clamp(g + y3, 0, 255);
	dst1[2] = Clamp(b + y3, 0, 255);
	dst1[3] = 255;

	// 4th pixel
	dst1[4] = Clamp(r + y4, 0, 255);
	dst1[5] = Clamp(g + y4, 0, 255);
	dst1[6] = Clamp(b + y4, 0, 255);
	dst1[7] = 255;
}

/*
 =================
 R_ApplyVector2x2
 =================
*/
static void R_ApplyVector2x2 (video_t *video, int x, int y, const roqCell_t *cell){

	byte	*dst0, *dst1;

	dst0 = video->roqBuffers[0] + (y * video->roqWidth + x) * 4;
	dst1 = dst0 + video->roqWidth * 4;

	R_DecodeBlock(dst0, dst1, cell->y[0], cell->y[1], cell->y[2], cell->y[3], cell->u, cell->v);
}

/*
 =================
 R_ApplyVector4x4
 =================
*/
static void R_ApplyVector4x4 (video_t *video, int x, int y, const roqCell_t *cell){

	byte	*dst0, *dst1;

	dst0 = video->roqBuffers[0] + (y * video->roqWidth + x) * 4;
	dst1 = dst0 + video->roqWidth * 4;

	R_DecodeBlock(dst0, dst0+8, cell->y[0], cell->y[0], cell->y[1], cell->y[1], cell->u, cell->v);
	R_DecodeBlock(dst1, dst1+8, cell->y[0], cell->y[0], cell->y[1], cell->y[1], cell->u, cell->v);

	dst0 += video->roqWidth * 4 * 2;
	dst1 += video->roqWidth * 4 * 2;

	R_DecodeBlock(dst0, dst0+8, cell->y[2], cell->y[2], cell->y[3], cell->y[3], cell->u, cell->v);
	R_DecodeBlock(dst1, dst1+8, cell->y[2], cell->y[2], cell->y[3], cell->y[3], cell->u, cell->v);
}

/*
 =================
 R_ApplyMotion4x4
 =================
*/
static void R_ApplyMotion4x4 (video_t *video, int x, int y, byte mv){

	roqChunk_t	*chunk = &video->roqChunk;
	byte		*src, *dst;
	int			xp, yp;
	int			i, stride;

	xp = x + 8 - (mv >> 4) - (char)((chunk->argument >> 8) & 0xFF);
	yp = y + 8 - (mv & 15) - (char)(chunk->argument & 0xFF);

	src = video->roqBuffers[1] + (yp * video->roqWidth + xp) * 4;
	dst = video->roqBuffers[0] + (y * video->roqWidth + x) * 4;

	stride = video->roqWidth * 4;

	for (i = 0; i < 4; i++){
		((unsigned *)dst)[0] = ((unsigned *)src)[0];
		((unsigned *)dst)[1] = ((unsigned *)src)[1];
		((unsigned *)dst)[2] = ((unsigned *)src)[2];
		((unsigned *)dst)[3] = ((unsigned *)src)[3];

		src += stride;
		dst += stride;
	}
}

/*
 =================
 R_ApplyMotion8x8
 =================
*/
static void R_ApplyMotion8x8 (video_t *video, int x, int y, byte mv){

	roqChunk_t	*chunk = &video->roqChunk;
	byte		*src, *dst;
	int			xp, yp;
	int			i, stride;

	xp = x + 8 - (mv >> 4) - (char)((chunk->argument >> 8) & 0xFF);
	yp = y + 8 - (mv & 15) - (char)(chunk->argument & 0xFF);

	src = video->roqBuffers[1] + (yp * video->roqWidth + xp) * 4;
	dst = video->roqBuffers[0] + (y * video->roqWidth + x) * 4;

	stride = video->roqWidth * 4;

	for (i = 0; i < 8; i++){
		((unsigned *)dst)[0] = ((unsigned *)src)[0];
		((unsigned *)dst)[1] = ((unsigned *)src)[1];
		((unsigned *)dst)[2] = ((unsigned *)src)[2];
		((unsigned *)dst)[3] = ((unsigned *)src)[3];
		((unsigned *)dst)[4] = ((unsigned *)src)[4];
		((unsigned *)dst)[5] = ((unsigned *)src)[5];
		((unsigned *)dst)[6] = ((unsigned *)src)[6];
		((unsigned *)dst)[7] = ((unsigned *)src)[7];

		src += stride;
		dst += stride;
	}
}

/*
 =================
 R_ResampleVideoFrame
 =================
*/
static void R_ResampleVideoFrame (video_t *video){

	unsigned	*src, *dst;
	int			frac, fracStep;
	int			i, j;

	if (video->width == video->roqWidth && video->height == video->roqHeight)
		return;

	dst = (unsigned *)video->buffer;
	fracStep = video->roqWidth * 0x10000 / video->width;

	for (i = 0; i < video->height; i++, dst += video->width){
		src = (unsigned *)video->roqBuffers[1] + video->roqWidth * (i * video->roqHeight / video->height);
		frac = fracStep >> 1;

		for (j = 0; j < video->width; j++){
			dst[j] = src[frac >> 16];
			frac += fracStep;
		}
	}
}

/*
 =================
 R_ReadVideoFrame
 =================
*/
static void R_ReadVideoFrame (video_t *video){

	roqChunk_t	*chunk = &video->roqChunk;
	byte		compressed[ROQ_MAX_SIZE];
	int			pos, xPos, yPos;
	int			i, x, y, xp, yp;
	int			vqFlgPos, vqId;
	short		vqFlg;
	byte		*tmp;

	FS_Read(compressed, chunk->size, video->file);
	video->offset += chunk->size;

	if (!video->roqCache)
		return;		// No buffer cache

	pos = 0;
	xPos = yPos = 0;

	vqFlg = 0;
	vqFlgPos = -1;

	while (pos < chunk->size){
		for (yp = yPos; yp < yPos + 16; yp += 8){
			for (xp = xPos; xp < xPos + 16; xp += 8){
				if (vqFlgPos < 0){
					vqFlgPos = 7;

					vqFlg = compressed[pos] + (compressed[pos+1] << 8);
					pos += 2;
				}

				vqId = (vqFlg >> (vqFlgPos * 2)) & 3;
				vqFlgPos--;
				
				switch (vqId){
				case ROQ_ID_FCC:
					R_ApplyMotion8x8(video, xp, yp, compressed[pos]);

					pos += 1;

					break;
				case ROQ_ID_SLD:
					R_ApplyVector4x4(video, xp, yp, video->roqCells + video->roqQCells[compressed[pos]].index[0]);
					R_ApplyVector4x4(video, xp+4, yp, video->roqCells + video->roqQCells[compressed[pos]].index[1]);
					R_ApplyVector4x4(video, xp, yp+4, video->roqCells + video->roqQCells[compressed[pos]].index[2]);
					R_ApplyVector4x4(video, xp+4, yp+4, video->roqCells + video->roqQCells[compressed[pos]].index[3]);

					pos += 1;

					break;
				case ROQ_ID_CCC:
					for (i = 0; i < 4; i++){
						x = xp;
						y = yp;

						if (i & 1)
							x += 4;
						if (i & 2)
							y += 4;

						if (vqFlgPos < 0){
							vqFlgPos = 7;

							vqFlg = compressed[pos] + (compressed[pos+1] << 8);
							pos += 2;
						}

						vqId = (vqFlg >> (vqFlgPos * 2)) & 3;
						vqFlgPos--;

						switch (vqId){
						case ROQ_ID_FCC:
							R_ApplyMotion4x4(video, x, y, compressed[pos]);

							pos += 1;

							break;
						case ROQ_ID_SLD:
							R_ApplyVector2x2(video, x, y, video->roqCells + video->roqQCells[compressed[pos]].index[0]);
							R_ApplyVector2x2(video, x+2, y, video->roqCells + video->roqQCells[compressed[pos]].index[1]);
							R_ApplyVector2x2(video, x, y+2, video->roqCells + video->roqQCells[compressed[pos]].index[2]);
							R_ApplyVector2x2(video, x+2, y+2, video->roqCells + video->roqQCells[compressed[pos]].index[3]);

							pos += 1;

							break;
						case ROQ_ID_CCC:
							R_ApplyVector2x2(video, x, y, video->roqCells + compressed[pos+0]);
							R_ApplyVector2x2(video, x+2, y, video->roqCells + compressed[pos+1]);
							R_ApplyVector2x2(video, x, y+2, video->roqCells + compressed[pos+2]);
							R_ApplyVector2x2(video, x+2, y+2, video->roqCells + compressed[pos+3]);

							pos += 4;

							break;
						}
					}

					break;
				}
			}
		}

		xPos += 16;
		if (xPos >= video->roqWidth){
			xPos -= video->roqWidth;

			yPos += 16;
			if (yPos >= video->roqHeight)
				break;
		}
	}

	// Copy or swap the buffers
	if (video->currentFrame == 0)
		memcpy(video->roqBuffers[1], video->roqBuffers[0], video->roqWidth * video->roqHeight * 4);
	else {
		tmp = video->roqBuffers[0];
		video->roqBuffers[0] = video->roqBuffers[1];
		video->roqBuffers[1] = tmp;
	}

	video->currentFrame++;

	// Resample if needed
	R_ResampleVideoFrame(video);
}

/*
 =================
 R_ReadNextFrame
 =================
*/
static qboolean R_ReadNextFrame (video_t *video){

	roqChunk_t	*chunk = &video->roqChunk;
	byte		header[8];

	if (video->offset >= video->size)
		return false;	// Already reached the end

	// Read the frame header
	FS_Read(header, sizeof(header), video->file);
	video->offset += sizeof(header);

	chunk->id = header[0] + (header[1] << 8);
	chunk->size = header[2] + (header[3] << 8) + (header[4] << 16) + (header[5] << 24);
	chunk->argument = header[6] + (header[7] << 8);

	if (chunk->id == ROQ_IDENT || chunk->size > ROQ_MAX_SIZE)
		return false;	// Invalid frame

	if (video->offset + chunk->size > video->size)
		return false;	// Frame goes past the end

	if (chunk->size <= 0)
		return true;	// Frame is empty

	// Read the frame
	switch (chunk->id){
	case ROQ_QUAD_INFO:
		R_ReadInfo(video);
		break;
	case ROQ_QUAD_CODEBOOK:
		R_ReadCodebook(video);
		break;
	case ROQ_QUAD_VQ:
		R_ReadVideoFrame(video);
		break;
	default:
		FS_Seek(video->file, chunk->size, FS_SEEK_CUR);
		video->offset += chunk->size;
		break;
	}

	return true;
}

/*
 =================
 R_SetupVideo
 =================
*/
static void R_SetupVideo (video_t *video){

	roqChunk_t	*chunk = &video->roqChunk;
	byte		header[8];

	while (video->offset < video->size){
		// Read the frame header
		FS_Read(header, sizeof(header), video->file);
		video->offset += sizeof(header);

		chunk->id = header[0] + (header[1] << 8);
		chunk->size = header[2] + (header[3] << 8) + (header[4] << 16) + (header[5] << 24);
		chunk->argument = header[6] + (header[7] << 8);

		if (chunk->id == ROQ_IDENT || chunk->size > ROQ_MAX_SIZE)
			break;			// Invalid frame

		if (video->offset + chunk->size > video->size)
			break;			// Frame goes past the end

		if (chunk->size <= 0)
			continue;		// Frame is empty

		// Allocate the buffers now to avoid stalling gameplay
		if (chunk->id == ROQ_QUAD_INFO){
			R_ReadInfo(video);
			continue;
		}

		// Count frames
		if (chunk->id == ROQ_QUAD_VQ)
			video->frameCount++;

		// Skip the frame data
		FS_Seek(video->file, chunk->size, FS_SEEK_CUR);
		video->offset += chunk->size;
	}

	// Go back to the start
	FS_Seek(video->file, video->header, FS_SEEK_SET);
	video->offset = video->header;
}

/*
 =================
 R_UploadVideo
 =================
*/
static void R_UploadVideo (video_t *video){

	byte	*buffer;

	if (r_skipDynamicTextures->integerValue)
		return;

	if (!video->roqCache)
		return;		// No buffer cache

	tr.pc.updateTextures++;
	tr.pc.updateTexturePixels += video->width * video->header;

	// Select source buffer
	if (video->width != video->roqWidth || video->height != video->roqHeight)
		buffer = video->buffer;
	else
		buffer = video->roqBuffers[1];

	// Update the texture
	GL_BindTexture(tr.cinematicTexture);

	if (video->width == tr.cinematicTexture->uploadWidth && video->height == tr.cinematicTexture->uploadHeight)
		qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video->width, video->height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	else {
		// Reallocate the texture
		tr.cinematicTexture->uploadWidth = video->width;
		tr.cinematicTexture->uploadHeight = video->height;

		if (tr.cinematicTexture->sourceSamples >= 3)
			tr.cinematicTexture->uploadSize = video->width * video->height * 4;
		else
			tr.cinematicTexture->uploadSize = video->width * video->height * tr.cinematicTexture->sourceSamples;

		qglTexImage2D(GL_TEXTURE_2D, 0, tr.cinematicTexture->uploadFormat, video->width, video->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	}
}

/*
 =================
 R_PlayVideo
 =================
*/
video_t *R_PlayVideo (const char *name){

	video_t			*video;
	roqChunk_t		chunk;
	fileHandle_t	file;
	int				size;
	byte			header[8];
	unsigned		hash;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_PlayVideo: NULL video name");

	if (strlen(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "R_PlayVideo: video name exceeds MAX_OSPATH");

	// See if already playing
	hash = Com_HashKey(name, VIDEOS_HASH_SIZE);

	for (video = r_videosHashTable[hash]; video; video = video->nextHash){
		if (!Q_stricmp(video->name, name))
			return video;
	}

	// Open the file
	size = FS_OpenFile(name, &file, FS_READ);
	if (!file)
		return NULL;	// Not found

	// Parse the header
	FS_Read(header, sizeof(header), file);

	chunk.id = header[0] + (header[1] << 8);
	chunk.size = header[2] + (header[3] << 8) + (header[4] << 16) + (header[5] << 24);
	chunk.argument = header[6] + (header[7] << 8);

	if (chunk.id != ROQ_IDENT){
		Com_DPrintf(S_COLOR_YELLOW "R_PlayVideo: invalid RoQ header (%s)\n", name);
		FS_CloseFile(file);
		return NULL;
	}

	// Play the video
	if (r_numVideos == MAX_VIDEOS)
		Com_Error(ERR_DROP, "R_PlayVideo: MAX_VIDEOS hit");

	r_videos[r_numVideos++] = video = Z_Malloc(sizeof(video_t));

	// Fill it in
	Q_strncpyz(video->name, name, sizeof(video->name));
	video->file = file;
	video->size = size;
	video->offset = sizeof(header);
	video->header = sizeof(header);

	video->width = 0;
	video->height = 0;
	video->buffer = NULL;

	video->frameCount = 0;
	video->frameRate = (chunk.argument != 0) ? chunk.argument : 30;

	video->startTime = 0;
	video->currentFrame = 0;

	video->roqWidth = 0;
	video->roqHeight = 0;
	video->roqCache = NULL;

	// Set some things up
	R_SetupVideo(video);

	// Add to hash table
	video->nextHash = r_videosHashTable[hash];
	r_videosHashTable[hash] = video;

	return video;
}

/*
 =================
 R_UpdateVideo
 =================
*/
void R_UpdateVideo (video_t *video, int time){

	int		frame;

	if (r_skipVideos->integerValue)
		return;

	// If this is the first update, set the start time
	if (video->currentFrame == 0)
		video->startTime = time;

	// Check if a new frame is needed
	frame = (time - video->startTime) * video->frameRate/1000;
	if (frame < 1)
		frame = 1;

	// Never drop too many frames in a row because it stalls
	if (frame > video->currentFrame + 100/video->frameRate){
		video->startTime = time - video->currentFrame * 1000/video->frameRate;

		frame = (time - video->startTime) * video->frameRate/1000;
		if (frame < 1)
			frame = 1;
	}

	while (frame > video->currentFrame){
		// Read the next frame
		if (R_ReadNextFrame(video))
			continue;

		// Make sure we don't get stuck into an infinite loop
		if (video->currentFrame == 0)
			break;

		// Restart the video
		FS_Seek(video->file, video->header, FS_SEEK_SET);
		video->offset = video->header;

		video->startTime = time;
		video->currentFrame = 0;

		frame = 1;
	}

	// Upload the current frame
	R_UploadVideo(video);
}

/*
 =================
 R_ListVideos_f
 =================
*/
void R_ListVideos_f (void){

	video_t	*video;
	int		i;

	Com_Printf("\n");
	Com_Printf("      -w-- -h-- frms fps -name--------\n");

	for (i = 0; i < r_numVideos; i++){
		video = r_videos[i];

		Com_Printf("%4i: %4i %4i %4i %3i %s\n", i, video->width, video->height, video->frameCount, video->frameRate, video->name);
	}

	Com_Printf("--------------------------------------\n");
	Com_Printf("%i total videos\n", r_numVideos);
	Com_Printf("\n");
}

/*
 =================
 R_InitVideos
 =================
*/
void R_InitVideos (void){

	int		i;
	float	f;

	// Build YUV tables
	for (i = 0; i < 256; i++){
		f = (float)i - 128.0;

		r_vrTable[i] = (int)(f * 1.40200);
		r_ugTable[i] = (int)(f * -0.34414);
		r_vgTable[i] = (int)(f * 0.71414);
		r_ubTable[i] = (int)(f * 1.77200);
	}
}

/*
 =================
 R_ShutdownVideos
 =================
*/
void R_ShutdownVideos (void){

	video_t	*video;
	int		i;

	for (i = 0; i < r_numVideos; i++){
		video = r_videos[i];

		if (video->file)
			FS_CloseFile(video->file);

		Z_Free(video);
	}

	memset(r_videosHashTable, 0, sizeof(r_videosHashTable));
	memset(r_videos, 0, sizeof(r_videos));

	r_numVideos = 0;
}
