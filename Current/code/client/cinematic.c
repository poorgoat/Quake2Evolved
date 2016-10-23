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


typedef struct {
	char			name[MAX_OSPATH];
	unsigned		flags;

	fileHandle_t	file;
	int				size;
	int				offset;
	int				header;

	qboolean		isRoQ;
	
	int				vidWidth;
	int				vidHeight;
	byte			*vidBuffer;

	int				rawWidth;
	int				rawHeight;
	byte			*rawBuffer;

	int				sndRate;
	int				sndWidth;
	int				sndChannels;

	int				frameRate;
	int				startTime;
	int				currentFrame;

	qboolean		playing;

	// PCX stuff
	byte			*pcxBuffer;

	// Huffman stuff
	byte			*hBuffer;

	unsigned		hPalette[256];

	int				*hNodes;
	int				hNumNodes[256];

	int				hUsed[512];
	int				hCount[512];

	// RoQ stuff
	byte			*roqCache;
	byte			*roqBuffers[2];

	int				roqVrTable[256];
	int				roqUgTable[256];
	int				roqVgTable[256];
	int				roqUbTable[256];

	short			roqSndSqrTable[256];

	roqChunk_t		roqChunk;
	roqCell_t		roqCells[256];
	roqQCell_t		roqQCells[256];
} cinematic_t;

static cinematic_t	cin;


/*
 =================
 CIN_SmallestNode
 =================
*/
static int CIN_SmallestNode (int numNodes){

	int		i;
	int		best, bestNode;

	best = 99999999;
	bestNode = -1;

	for (i = 0; i < numNodes; i++){
		if (cin.hUsed[i])
			continue;

		if (!cin.hCount[i])
			continue;

		if (cin.hCount[i] < best){
			best = cin.hCount[i];
			bestNode = i;
		}
	}

	if (bestNode == -1)
		return -1;

	cin.hUsed[bestNode] = true;
	return bestNode;
}

/*
 =================
 CIN_SetupHuffTables
 =================
*/
static void CIN_SetupHuffTables (void){

	int		i, prev;
	int		*node, *nodeBase;
	byte	counts[256];
	int		numNodes;

	cin.hNodes = Z_Malloc(256 * 256 * 4 * 2);

	for (prev = 0; prev < 256; prev++){
		memset(cin.hCount, 0, sizeof(cin.hCount));
		memset(cin.hUsed, 0, sizeof(cin.hUsed));

		// Read a row of counts
		FS_Read(counts, sizeof(counts), cin.file);
		cin.offset += sizeof(counts);

		for (i = 0; i < 256; i++)
			cin.hCount[i] = counts[i];

		// Build the nodes
		numNodes = 256;
		nodeBase = cin.hNodes + prev*256*2;

		while (numNodes != 511){
			node = nodeBase + (numNodes-256)*2;

			// Pick two lowest counts
			node[0] = CIN_SmallestNode(numNodes);
			if (node[0] == -1)
				break;

			node[1] = CIN_SmallestNode(numNodes);
			if (node[1] == -1)
				break;

			cin.hCount[numNodes] = cin.hCount[node[0]] + cin.hCount[node[1]];
			numNodes++;
		}

		cin.hNumNodes[prev] = numNodes-1;
	}

	cin.header = cin.offset;
}

/*
 =================
 CIN_ReadPalette
 =================
*/
static void CIN_ReadPalette (void){

	byte	palette[768], *pal;
	int		i;

	FS_Read(palette, sizeof(palette), cin.file);
	cin.offset += sizeof(palette);

	pal = (byte *)cin.hPalette;
	for (i = 0; i < 256; i++){
		pal[i*4+0] = palette[i*3+0];
		pal[i*4+1] = palette[i*3+1];
		pal[i*4+2] = palette[i*3+2];
		pal[i*4+3] = 255;
	}
}

/*
 =================
 CIN_SetupRoQTables
 =================
*/
static void CIN_SetupRoQTables (void){

	int		i;
	float	f;

	// Build YUV tables
	for (i = 0; i < 256; i++){
		f = (float)i - 128.0;

		cin.roqVrTable[i] = (int)(f * 1.40200);
		cin.roqUgTable[i] = (int)(f * -0.34414);
		cin.roqVgTable[i] = (int)(f * 0.71414);
		cin.roqUbTable[i] = (int)(f * 1.77200);
	}

	// Build sound square table
	for (i = 0; i < 128; i++){
		cin.roqSndSqrTable[i] = i * i;
		cin.roqSndSqrTable[i+128] = -(i * i);
	}
}

/*
 =================
 CIN_ReadInfo
 =================
*/
static void CIN_ReadInfo (void){

	byte	data[8];

	FS_Read(data, sizeof(data), cin.file);
	cin.offset += sizeof(data);

	if (cin.roqCache)
		return;		// Already allocated

	// Allocate the buffers
	cin.vidWidth = data[0] + (data[1] << 8);
	cin.vidHeight = data[2] + (data[3] << 8);

	cin.roqCache = Z_Malloc(cin.vidWidth * cin.vidHeight * 4 * 2);

	cin.roqBuffers[0] = cin.roqCache;
	cin.roqBuffers[1] = cin.roqCache + (cin.vidWidth * cin.vidHeight * 4);

	// Check if we need to sample down
	if (cls.glConfig.textureNonPowerOfTwo){
		cin.rawWidth = cin.vidWidth;
		cin.rawHeight = cin.vidHeight;
	}
	else {
		cin.rawWidth = NearestPowerOfTwo(cin.vidWidth, true);
		cin.rawHeight = NearestPowerOfTwo(cin.vidHeight, true);
	}

	while (cin.rawWidth > cls.glConfig.maxTextureSize || cin.rawHeight > cls.glConfig.maxTextureSize){
		cin.rawWidth >>= 1;
		cin.rawHeight >>= 1;
	}

	if (cin.rawWidth < 1)
		cin.rawWidth = 1;
	if (cin.rawHeight < 1)
		cin.rawHeight = 1;

	if (cin.rawWidth != cin.vidWidth || cin.rawHeight != cin.vidHeight)
		cin.rawBuffer = Z_Malloc(cin.rawWidth * cin.rawHeight * 4);
}

/*
 =================
 CIN_ReadCodebook
 =================
*/
static void CIN_ReadCodebook (void){

	roqChunk_t	*chunk = &cin.roqChunk;
	int			n1, n2;

	n1 = (chunk->argument >> 8) & 0xFF;
	if (!n1)
		n1 = 256;

	n2 = chunk->argument & 0xFF;
	if (!n2 && (n1 * 6 < chunk->size))
		n2 = 256;

	FS_Read(cin.roqCells, sizeof(roqCell_t) * n1, cin.file);
	FS_Read(cin.roqQCells, sizeof(roqQCell_t) * n2, cin.file);
	cin.offset += chunk->size;
}

/*
 =================
 CIN_DecodeBlock
 =================
*/
static void CIN_DecodeBlock (byte *dst0, byte *dst1, int y1, int y2, int y3, int y4, int u, int v){

	int		r, g, b;

	// Convert YUV to RGB
	r = cin.roqVrTable[v];
	g = cin.roqUgTable[u] - cin.roqVgTable[v];
	b = cin.roqUbTable[u];

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
 CIN_ApplyVector2x2
 =================
*/
static void CIN_ApplyVector2x2 (int x, int y, const roqCell_t *cell){

	byte	*dst0, *dst1;

	dst0 = cin.roqBuffers[0] + (y * cin.vidWidth + x) * 4;
	dst1 = dst0 + cin.vidWidth * 4;

	CIN_DecodeBlock(dst0, dst1, cell->y[0], cell->y[1], cell->y[2], cell->y[3], cell->u, cell->v);
}

/*
 =================
 CIN_ApplyVector4x4
 =================
*/
static void CIN_ApplyVector4x4 (int x, int y, const roqCell_t *cell){

	byte	*dst0, *dst1;

	dst0 = cin.roqBuffers[0] + (y * cin.vidWidth + x) * 4;
	dst1 = dst0 + cin.vidWidth * 4;

	CIN_DecodeBlock(dst0, dst0+8, cell->y[0], cell->y[0], cell->y[1], cell->y[1], cell->u, cell->v);
	CIN_DecodeBlock(dst1, dst1+8, cell->y[0], cell->y[0], cell->y[1], cell->y[1], cell->u, cell->v);

	dst0 += cin.vidWidth * 4 * 2;
	dst1 += cin.vidWidth * 4 * 2;

	CIN_DecodeBlock(dst0, dst0+8, cell->y[2], cell->y[2], cell->y[3], cell->y[3], cell->u, cell->v);
	CIN_DecodeBlock(dst1, dst1+8, cell->y[2], cell->y[2], cell->y[3], cell->y[3], cell->u, cell->v);
}

/*
 =================
 CIN_ApplyMotion4x4
 =================
*/
static void CIN_ApplyMotion4x4 (int x, int y, byte mv){

	roqChunk_t	*chunk = &cin.roqChunk;
	byte		*src, *dst;
	int			xp, yp;
	int			i, stride;

	xp = x + 8 - (mv >> 4) - (char)((chunk->argument >> 8) & 0xFF);
	yp = y + 8 - (mv & 15) - (char)(chunk->argument & 0xFF);

	src = cin.roqBuffers[1] + (yp * cin.vidWidth + xp) * 4;
	dst = cin.roqBuffers[0] + (y * cin.vidWidth + x) * 4;

	stride = cin.vidWidth * 4;

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
 CIN_ApplyMotion8x8
 =================
*/
static void CIN_ApplyMotion8x8 (int x, int y, byte mv){

	roqChunk_t	*chunk = &cin.roqChunk;
	byte		*src, *dst;
	int			xp, yp;
	int			i, stride;

	xp = x + 8 - (mv >> 4) - (char)((chunk->argument >> 8) & 0xFF);
	yp = y + 8 - (mv & 15) - (char)(chunk->argument & 0xFF);

	src = cin.roqBuffers[1] + (yp * cin.vidWidth + xp) * 4;
	dst = cin.roqBuffers[0] + (y * cin.vidWidth + x) * 4;

	stride = cin.vidWidth * 4;

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
 CIN_ResampleVideoFrame
 =================
*/
static void CIN_ResampleVideoFrame (void){

	unsigned	*src, *dst;
	int			frac, fracStep;
	int			i, j;

	if (cin.rawWidth == cin.vidWidth && cin.rawHeight == cin.vidHeight)
		return;

	dst = (unsigned *)cin.rawBuffer;
	fracStep = cin.vidWidth * 0x10000 / cin.rawWidth;

	for (i = 0; i < cin.rawHeight; i++, dst += cin.rawWidth){
		src = (unsigned *)cin.vidBuffer + cin.vidWidth * (i * cin.vidHeight / cin.rawHeight);
		frac = fracStep >> 1;

		for (j = 0; j < cin.rawWidth; j++){
			dst[j] = src[frac >> 16];
			frac += fracStep;
		}
	}
}

/*
 =================
 CIN_ReadVideoFrame
 =================
*/
static void CIN_ReadVideoFrame (void){

	if (!cin.isRoQ){
		const byte	*input, *data;
		byte		compressed[0x20000];
		unsigned	*out;
		int			size, count;
		int			nodeNum;
		int			*nodes, *nodesBase;
		int			i, in;

		FS_Read(&size, sizeof(size), cin.file);
		cin.offset += sizeof(size);

		size = LittleLong(size);
		if (size < 1 || size > sizeof(compressed)){
			Com_DPrintf(S_COLOR_YELLOW, "CIN_ReadVideoFrame: bad compressed frame size (%i)\n", size);

			cin.offset = cin.size;
			return;
		}

		FS_Read(compressed, size, cin.file);
		cin.offset += size;

		// Get decompressed count
		data = compressed;
		count = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
		input = data + 4;
		out = cin.hBuffer;

		// Read bits
		nodesBase = cin.hNodes - 256*2;	// Nodes 0-255 aren't stored

		nodes = nodesBase;
		nodeNum = cin.hNumNodes[0];

		while (count){
			in = *input++;

			for (i = 0; i < 8; i++){
				if (nodeNum < 256){
					nodes = nodesBase + (nodeNum << 9);
					*out++ = cin.hPalette[nodeNum];

					if (!--count)
						break;

					nodeNum = cin.hNumNodes[nodeNum];
				}

				nodeNum = nodes[nodeNum*2 + (in & 1)];
				in >>= 1;
			}

			if (i != 8)
				break;
		}

		if (input - data != size && input - data != size+1)
			Com_DPrintf(S_COLOR_YELLOW, "CIN_ReadVideoFrame: decompression overread by %i\n", (input - data) - size);

		cin.vidBuffer = cin.hBuffer;
	}
	else {
		roqChunk_t	*chunk = &cin.roqChunk;
		byte		compressed[ROQ_MAX_SIZE];
		int			pos, xPos, yPos;
		int			i, x, y, xp, yp;
		int			vqFlgPos, vqId;
		short		vqFlg;
		byte		*tmp;

		FS_Read(compressed, chunk->size, cin.file);
		cin.offset += chunk->size;

		if (!cin.roqCache)
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
						CIN_ApplyMotion8x8(xp, yp, compressed[pos]);

						pos += 1;

						break;
					case ROQ_ID_SLD:
						CIN_ApplyVector4x4(xp, yp, cin.roqCells + cin.roqQCells[compressed[pos]].index[0]);
						CIN_ApplyVector4x4(xp+4, yp, cin.roqCells + cin.roqQCells[compressed[pos]].index[1]);
						CIN_ApplyVector4x4(xp, yp+4, cin.roqCells + cin.roqQCells[compressed[pos]].index[2]);
						CIN_ApplyVector4x4(xp+4, yp+4, cin.roqCells + cin.roqQCells[compressed[pos]].index[3]);

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
								CIN_ApplyMotion4x4(x, y, compressed[pos]);

								pos += 1;

								break;
							case ROQ_ID_SLD:
								CIN_ApplyVector2x2(x, y, cin.roqCells + cin.roqQCells[compressed[pos]].index[0]);
								CIN_ApplyVector2x2(x+2, y, cin.roqCells + cin.roqQCells[compressed[pos]].index[1]);
								CIN_ApplyVector2x2(x, y+2, cin.roqCells + cin.roqQCells[compressed[pos]].index[2]);
								CIN_ApplyVector2x2(x+2, y+2, cin.roqCells + cin.roqQCells[compressed[pos]].index[3]);

								pos += 1;

								break;
							case ROQ_ID_CCC:
								CIN_ApplyVector2x2(x, y, cin.roqCells + compressed[pos+0]);
								CIN_ApplyVector2x2(x+2, y, cin.roqCells + compressed[pos+1]);
								CIN_ApplyVector2x2(x, y+2, cin.roqCells + compressed[pos+2]);
								CIN_ApplyVector2x2(x+2, y+2, cin.roqCells + compressed[pos+3]);

								pos += 4;

								break;
							}
						}

						break;
					}
				}
			}

			xPos += 16;
			if (xPos >= cin.vidWidth){
				xPos -= cin.vidWidth;

				yPos += 16;
				if (yPos >= cin.vidHeight)
					break;
			}
		}

		// Copy or swap the buffers
		if (cin.currentFrame == 0)
			memcpy(cin.roqBuffers[1], cin.roqBuffers[0], cin.vidWidth * cin.vidHeight * 4);
		else {
			tmp = cin.roqBuffers[0];
			cin.roqBuffers[0] = cin.roqBuffers[1];
			cin.roqBuffers[1] = tmp;
		}

		cin.vidBuffer = cin.roqBuffers[1];

		cin.currentFrame++;
	}

	// Resample if needed
	CIN_ResampleVideoFrame();
}

/*
 =================
 CIN_ReadAudioFrame
 =================
*/
static void CIN_ReadAudioFrame (void){

	byte	data[0x40000];
	int		samples;

	if (!cin.isRoQ){
		int			start, end, len;

		start = cin.currentFrame * cin.sndRate/14;
		end = (cin.currentFrame+1) * cin.sndRate/14;
		samples = end - start;
		len = samples * cin.sndWidth * cin.sndChannels;

		FS_Read(data, len, cin.file);
		cin.offset += len;

		if (cin.flags & CIN_SILENT)
			return;
	}
	else {
		roqChunk_t	*chunk = &cin.roqChunk;
		byte		compressed[ROQ_MAX_SIZE];
		short		l, r;
		int			i;

		FS_Read(compressed, chunk->size, cin.file);
		cin.offset += chunk->size;

		if (cin.flags & CIN_SILENT)
			return;

		switch (chunk->id){
		case ROQ_SOUND_MONO:
			cin.sndChannels = 1;

			l = chunk->argument;

			for (i = 0; i < chunk->size; i++){
				l += cin.roqSndSqrTable[compressed[i]];

				((short *)&data)[i] = l;
			}

			samples = chunk->size;

			break;
		case ROQ_SOUND_STEREO:
			cin.sndChannels = 2;

			l = (chunk->argument & 0xFF00);
			r = (chunk->argument & 0x00FF) << 8;

			for (i = 0; i < chunk->size; i += 2){
				l += cin.roqSndSqrTable[compressed[i+0]];
				r += cin.roqSndSqrTable[compressed[i+1]];

				((short *)&data)[i+0] = l;
				((short *)&data)[i+1] = r;
			}

			samples = chunk->size / 2;

			break;
		}
	}

	// Send sound to mixer
	S_StreamRawSamples(data, samples, cin.sndRate, cin.sndWidth, cin.sndChannels);
}

/*
 =================
 CIN_ReadNextFrame
 =================
*/
static qboolean CIN_ReadNextFrame (void){

	if (!cin.isRoQ){
		int			command;

		if (cin.offset >= cin.size)
			return false;	// Already reached the end

		// Read the command
		FS_Read(&command, sizeof(command), cin.file);
		cin.offset += sizeof(command);

		command = LittleLong(command);
		if (command == 2)
			return false;	// Reached the end

		if (cin.offset >= cin.size)
			return false;	// Frame goes past the end

		if (command == 1)
			CIN_ReadPalette();

		if (cin.offset >= cin.size)
			return false;	// Invalid frame

		// Read the video frame
		CIN_ReadVideoFrame();

		if (cin.offset >= cin.size)
			return false;	// Invalid frame

		// Read the audio frame
		CIN_ReadAudioFrame();

		cin.currentFrame++;

		return true;
	}
	else {
		roqChunk_t	*chunk = &cin.roqChunk;
		byte		header[8];

		if (cin.offset >= cin.size)
			return false;	// Already reached the end

		// Read the frame header
		FS_Read(header, sizeof(header), cin.file);
		cin.offset += sizeof(header);

		chunk->id = header[0] + (header[1] << 8);
		chunk->size = header[2] + (header[3] << 8) + (header[4] << 16) + (header[5] << 24);
		chunk->argument = header[6] + (header[7] << 8);

		if (chunk->id == ROQ_IDENT || chunk->size > ROQ_MAX_SIZE)
			return false;	// Invalid frame

		if (cin.offset + chunk->size > cin.size)
			return false;	// Frame goes past the end

		if (chunk->size <= 0)
			return true;	// Frame is empty

		// Read the frame
		switch (chunk->id){
		case ROQ_QUAD_INFO:
			CIN_ReadInfo();
			break;
		case ROQ_QUAD_CODEBOOK:
			CIN_ReadCodebook();
			break;
		case ROQ_QUAD_VQ:
			CIN_ReadVideoFrame();
			break;
		case ROQ_SOUND_MONO:
		case ROQ_SOUND_STEREO:
			CIN_ReadAudioFrame();
			break;
		default:
			FS_Seek(cin.file, chunk->size, FS_SEEK_CUR);
			cin.offset += chunk->size;
			break;
		}

		return true;
	}
}

/*
 =================
 CIN_StaticCinematic
 =================
*/
static qboolean CIN_StaticCinematic (const char *name, unsigned flags){

	char		loadName[MAX_OSPATH];
	byte		*buffer, *pcxBuffer;
	byte		*in, *out;
	pcxHeader_t	*pcx;
	int			x, y, len;
	int			dataByte, runLength;
	byte		palette[768];

	// Load the file
	Q_snprintfz(loadName, sizeof(loadName), "%s.pcx", name);
	len = FS_LoadFile(loadName, (void **)&buffer);
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
		Com_DPrintf(S_COLOR_YELLOW "CIN_StaticCinematic: invalid PCX header\n");
		FS_FreeFile(buffer);
		return false;
	}

	if (pcx->bitsPerPixel != 8 || pcx->colorPlanes != 1){
		Com_DPrintf(S_COLOR_YELLOW "CIN_StaticCinematic: only 8 bit PCX images supported\n");
		FS_FreeFile(buffer);
		return false;
	}
		
	if (pcx->xMax == 0 || pcx->yMax == 0 || pcx->xMax >= 640 || pcx->yMax >= 480){
		Com_DPrintf(S_COLOR_YELLOW "CIN_StaticCinematic: bad PCX file (%i x %i)\n", pcx->xMax, pcx->yMax);
		FS_FreeFile(buffer);
		return false;
	}

	memcpy(palette, (byte *)buffer + len - 768, 768);

	pcxBuffer = out = Z_Malloc((pcx->xMax+1) * (pcx->yMax+1) * 4);

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
				out[0] = palette[dataByte*3+0];
				out[1] = palette[dataByte*3+1];
				out[2] = palette[dataByte*3+2];
				out[3] = 255;

				out += 4;
				x++;
			}
		}
	}

	if (in - buffer > len){
		Com_DPrintf(S_COLOR_YELLOW "CIN_StaticCinematic: PCX file was malformed\n");
		FS_FreeFile(buffer);
		Z_Free(pcxBuffer);
		return false;
	}

	FS_FreeFile(buffer);

	// Fill it in
	Q_strncpyz(cin.name, name, sizeof(cin.name));
	cin.flags = flags;

	cin.file = 0;
	cin.size = 0;
	cin.offset = 0;
	cin.header = 0;

	cin.isRoQ = false;

	cin.vidWidth = pcx->xMax+1;
	cin.vidHeight = pcx->yMax+1;
	cin.vidBuffer = pcxBuffer;

	if (cls.glConfig.textureNonPowerOfTwo){
		cin.rawWidth = cin.vidWidth;
		cin.rawHeight = cin.vidHeight;
	}
	else {
		cin.rawWidth = 256;
		cin.rawHeight = 256;
	}

	while (cin.rawWidth > cls.glConfig.maxTextureSize || cin.rawHeight > cls.glConfig.maxTextureSize){
		cin.rawWidth >>= 1;
		cin.rawHeight >>= 1;
	}

	if (cin.rawWidth < 1)
		cin.rawWidth = 1;
	if (cin.rawHeight < 1)
		cin.rawHeight = 1;

	if (cin.rawWidth != cin.vidWidth || cin.rawHeight != cin.vidHeight)
		cin.rawBuffer = Z_Malloc(cin.rawWidth * cin.rawHeight * 4);
	else
		cin.rawBuffer = NULL;

	cin.sndRate = 0;
	cin.sndWidth = 0;
	cin.sndChannels = 0;

	cin.frameRate = 0;
	cin.startTime = 0;
	cin.currentFrame = -1;

	cin.playing = true;

	cin.pcxBuffer = pcxBuffer;

	// Resample if needed
	CIN_ResampleVideoFrame();

	return true;
}

/*
 =================
 CIN_RunCinematic
 =================
*/
qboolean CIN_RunCinematic (void){

	int		frame;

	if (!cin.playing)
		return false;		// Not running

	if (cin.currentFrame == -1)
		return true;		// Static image

	// If this is the first run, set the start time
	if (cin.currentFrame == 0)
		cin.startTime = cls.realTime;

	// Check if a new frame is needed
	frame = (cls.realTime - cin.startTime) * cin.frameRate/1000;
	if (frame < 1)
		frame = 1;

	// Never drop too many frames in a row because it stalls
	if (frame > cin.currentFrame + 100/cin.frameRate){
		cin.startTime = cls.realTime - cin.currentFrame * 1000/cin.frameRate;

		frame = (cls.realTime - cin.startTime) * cin.frameRate/1000;
		if (frame < 1)
			frame = 1;
	}

	while (frame > cin.currentFrame){
		// Read the next frame
		if (CIN_ReadNextFrame())
			continue;

		// Make sure we don't get stuck into an infinite loop
		if (cin.currentFrame == 0)
			return false;

		// Restart the cinematic if needed
		if (cin.flags & CIN_LOOP){
			FS_Seek(cin.file, cin.header, FS_SEEK_SET);
			cin.offset = cin.header;

			cin.startTime = cls.realTime;
			cin.currentFrame = 0;

			frame = 1;
			continue;
		}

		// Hold at end if needed
		if (cin.flags & CIN_HOLD)
			return true;

		return false;	// Finished
	}

	return true;
}

/*
 =================
 CIN_DrawCinematic
 =================
*/
void CIN_DrawCinematic (int x, int y, int w, int h){

	byte	*buffer;

	if (!cin.playing)
		return;			// Not running

	// Select source buffer
	if (cin.rawWidth != cin.vidWidth || cin.rawHeight != cin.vidHeight)
		buffer = cin.rawBuffer;
	else
		buffer = cin.vidBuffer;

	// Update the cinematic texture
	if (!R_UpdateTexture("_cinematic", buffer, cin.rawWidth, cin.rawHeight))
		return;

	// Draw it
	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, colorTable[COLOR_WHITE], cls.media.cinematicMaterial);
}

/*
 =================
 CIN_PlayCinematic
 =================
*/
qboolean CIN_PlayCinematic (const char *name, unsigned flags){

	char			checkName[MAX_OSPATH], loadName[MAX_OSPATH];
	fileHandle_t	file;
	int				size;
	byte			header[8];
	qboolean		isRoQ;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "CIN_PlayCinematic: NULL cinematic name");

	if (strlen(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "CIN_PlayCinematic: cinematic name exceeds MAX_OSPATH");

	// If already playing a cinematic, stop it
	if (cin.playing)
		CIN_StopCinematic();

	if (flags & CIN_SYSTEM){
		// Make sure CD audio and sounds aren't playing
		CDAudio_Stop();
		S_StopAllSounds(false);

		// Force menu and console off
		UI_SetActiveMenu(UI_CLOSEMENU);
		Con_CloseConsole();
	}

	// Strip extension
	Com_StripExtension(name, checkName, sizeof(checkName));

	// Check for a static PCX image
	Q_snprintfz(loadName, sizeof(loadName), "%s.pcx", checkName);
	if (FS_FileExists(loadName)){
		if (!CIN_StaticCinematic(checkName, flags))
			return false;

		return true;
	}

	// Open the file
	Q_snprintfz(loadName, sizeof(loadName), "%s.RoQ", checkName);
	size = FS_OpenFile(loadName, &file, FS_READ);
	if (file)
		isRoQ = true;
	else {
		Q_snprintfz(loadName, sizeof(loadName), "%s.cin", checkName);
		size = FS_OpenFile(loadName, &file, FS_READ);
		if (file)
			isRoQ = false;
		else 
			return false;	// Not found
	}

	// Parse the header
	if (!isRoQ){
		FS_Read(&cin.vidWidth, sizeof(cin.vidWidth), file);
		FS_Read(&cin.vidHeight, sizeof(cin.vidHeight), file);
		FS_Read(&cin.sndRate, sizeof(cin.sndRate), file);
		FS_Read(&cin.sndWidth, sizeof(cin.sndWidth), file);
		FS_Read(&cin.sndChannels, sizeof(cin.sndChannels), file);

		// Fill it in
		Q_strncpyz(cin.name, checkName, sizeof(cin.name));
		cin.flags = flags;

		cin.file = file;
		cin.size = size;
		cin.offset = 20;
		cin.header = 20;

		cin.isRoQ = false;

		cin.vidWidth = LittleLong(cin.vidWidth);
		cin.vidHeight = LittleLong(cin.vidHeight);
		cin.vidBuffer = NULL;

		if (cls.glConfig.textureNonPowerOfTwo){
			cin.rawWidth = cin.vidWidth;
			cin.rawHeight = cin.vidHeight;
		}
		else {
			cin.rawWidth = 256;
			cin.rawHeight = 256;
		}

		while (cin.rawWidth > cls.glConfig.maxTextureSize || cin.rawHeight > cls.glConfig.maxTextureSize){
			cin.rawWidth >>= 1;
			cin.rawHeight >>= 1;
		}

		if (cin.rawWidth < 1)
			cin.rawWidth = 1;
		if (cin.rawHeight < 1)
			cin.rawHeight = 1;

		if (cin.rawWidth != cin.vidWidth || cin.rawHeight != cin.vidHeight)
			cin.rawBuffer = Z_Malloc(cin.rawWidth * cin.rawHeight * 4);
		else
			cin.rawBuffer = NULL;

		cin.sndRate = LittleLong(cin.sndRate);
		cin.sndWidth = LittleLong(cin.sndWidth);
		cin.sndChannels = LittleLong(cin.sndChannels);

		cin.frameRate = 14;
		cin.startTime = 0;
		cin.currentFrame = 0;

		cin.playing = true;

		cin.hBuffer = Z_Malloc(cin.vidWidth * cin.vidHeight * 4);

		CIN_SetupHuffTables();
	}
	else {
		FS_Read(header, sizeof(header), file);

		if (header[0] + (header[1] << 8) != ROQ_IDENT){
			Com_DPrintf(S_COLOR_YELLOW "CIN_PlayCinematic: invalid RoQ header\n");
			FS_CloseFile(file);
			return false;
		}

		// Fill it in
		Q_strncpyz(cin.name, checkName, sizeof(cin.name));
		cin.flags = flags;

		cin.file = file;
		cin.size = size;
		cin.offset = sizeof(header);
		cin.header = sizeof(header);

		cin.isRoQ = true;

		cin.vidWidth = 0;
		cin.vidHeight = 0;
		cin.vidBuffer = NULL;

		cin.rawWidth = 0;
		cin.rawHeight = 0;
		cin.rawBuffer = NULL;

		cin.sndRate = 22050;
		cin.sndWidth = 2;
		cin.sndChannels = 0;

		cin.frameRate = header[6] + (header[7] << 8);
		if (!cin.frameRate)
			cin.frameRate = 30;

		cin.startTime = 0;
		cin.currentFrame = 0;

		cin.playing = true;

		cin.roqCache = NULL;

		CIN_SetupRoQTables();
	}

	if (!(cin.flags & CIN_SILENT))
		S_StartStreaming();

	return true;
}

/*
 =================
 CIN_StopCinematic
 =================
*/
void CIN_StopCinematic (void){

	if (!cin.playing)
		return;			// Not running

	// Free all the resources in use
	if (!(cin.flags & CIN_SILENT))
		S_StopStreaming();

	if (cin.rawBuffer)
		Z_Free(cin.rawBuffer);

	if (cin.pcxBuffer)
		Z_Free(cin.pcxBuffer);

	if (cin.hBuffer)
		Z_Free(cin.hBuffer);
	if (cin.hNodes)
		Z_Free(cin.hNodes);

	if (cin.roqCache)
		Z_Free(cin.roqCache);

	if (cin.file)
		FS_CloseFile(cin.file);

	memset(&cin, 0, sizeof(cinematic_t));
}
