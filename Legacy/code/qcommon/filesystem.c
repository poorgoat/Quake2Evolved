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


#include "qcommon.h"
#include "unzip.h"


/*

 All of Quake's data access is through a hierchal file system, but the
 contents of the file system can be transparently merged from several
 sources.

 The "base directory" is the path to the directory holding the
 q2e.exe and all game directories. This can be overridden with the
 "fs_homePath" variable to allow code debugging in a different
 directory. The base directory is only used during filesystem
 initialization.

 The "game directory" is the first tree on the search path and directory
 that all generated files (savegames, screenshots, demos, config files)
 will be saved to. This can be overridden with the "fs_game" variable.
 The game directory can never be changed while Quake is executing. This
 is a precacution against having a malicious server instruct clients to
 write files over areas they shouldn't.

*/

#define	BASEDIRNAME			"baseq2"

#define FILES_HASHSIZE		1024

#define MAX_HANDLES			64
#define MAX_FIND_FILES		65536
#define MAX_PACK_FILES		1024

typedef struct {
	qboolean				used;
	char					name[MAX_QPATH];
	fsMode_t				mode;
	FILE					*file;				// Only one of file or
	unzFile					*zip;				// zip will be used
} fsHandle_t;

typedef struct fsPackFile_s {
	char					name[MAX_QPATH];
	int						size;
	int						offset;				// This is ignored in PK2 files

	struct fsPackFile_s		*nextHash;
} fsPackFile_t;

typedef struct {
	char					name[MAX_OSPATH];
	FILE					*pak;				// Only one of pak or
	unzFile					*pk2;				// pk2 will be used
	int						numFiles;
	fsPackFile_t			*files;

	fsPackFile_t			*filesHash[FILES_HASHSIZE];
} fsPack_t;

typedef struct fsSearchPath_s {
	char					path[MAX_OSPATH];	// Only one of path or
	fsPack_t				*pack;				// pack will be used

	struct fsSearchPath_s	*next;
} fsSearchPath_t;

static fsHandle_t		fs_handles[MAX_HANDLES];
static fsSearchPath_t	*fs_searchPaths;

static char				fs_gameDir[MAX_OSPATH];
static char				fs_curGame[MAX_QPATH];

cvar_t	*fs_homePath;
cvar_t	*fs_cdPath;
cvar_t	*fs_basePath;
cvar_t	*fs_baseGame;
cvar_t	*fs_game;
cvar_t	*fs_debug;

void CDAudio_Stop (void);

/*
 =================
 FS_HandleForFile

 Allocates a fileHandle_t
 =================
*/
static fsHandle_t *FS_HandleForFile (fileHandle_t *f){

	fsHandle_t	*handle;
	int			i;

	for (i = 0, handle = fs_handles; i < MAX_HANDLES; i++, handle++){
		if (handle->used)
			continue;

		handle->used = true;

		*f = i+1;
		return handle;
	}

	// Failed
	Com_Error(ERR_FATAL, "FS_HandleForFile: none free");
}

/*
 =================
 FS_GetFileByHandle

 Returns a fsHandle_t * for the given fileHandle_t
 =================
*/
static fsHandle_t *FS_GetFileByHandle (fileHandle_t f){

	if (f <= 0 || f > MAX_HANDLES)
		Com_Error(ERR_FATAL, "FS_GetFileByHandle: out of range");

	return &fs_handles[f-1];
}

/*
 =================
 FS_FileLength
 =================
*/
static int FS_FileLength (FILE *f){

	int cur, end;

	cur = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, cur, SEEK_SET);

	return end;
}

/*
 =================
 FS_FOpenFileAppend

 Returns file size or -1 on error
 =================
*/
static int FS_FOpenFileAppend (fsHandle_t *handle){

	char	path[MAX_OSPATH];

	FS_CreatePath(handle->name);

	Q_snprintfz(path, sizeof(path), "%s/%s", fs_gameDir, handle->name);

	handle->file = fopen(path, "ab");
	if (handle->file){
		if (fs_debug->integer)
			Com_Printf("FS_FOpenFileAppend: %s\n", path);

		return FS_FileLength(handle->file);
	}

	if (fs_debug->integer)
		Com_Printf("FS_FOpenFileAppend: couldn't open %s\n", path);

	return -1;
}

/*
 =================
 FS_FOpenFileWrite

 Always returns 0 or -1 on error
 =================
*/
static int FS_FOpenFileWrite (fsHandle_t *handle){

	char	path[MAX_OSPATH];

	FS_CreatePath(handle->name);

	Q_snprintfz(path, sizeof(path), "%s/%s", fs_gameDir, handle->name);

	handle->file = fopen(path, "wb");
	if (handle->file){
		if (fs_debug->integer)
			Com_Printf("FS_FOpenFileWrite: %s\n", path);

		return 0;
	}

	if (fs_debug->integer)
		Com_Printf("FS_FOpenFileWrite: couldn't open %s\n", path);

	return -1;
}

/*
 =================
 FS_FOpenFileRead

 Returns file size or -1 if not found.
 Can open separate files as well as files inside pack files (both PAK
 and PK2).
 =================
*/
static int FS_FOpenFileRead (fsHandle_t *handle){

	fsSearchPath_t	*search;
	fsPackFile_t	*packFile;
	fsPack_t		*pack;
	char			path[MAX_OSPATH];
	unsigned		hashKey;

	// Search through the path, one element at a time
	for (search = fs_searchPaths; search; search = search->next){
		if (search->pack){
			// Search inside a pack file
			pack = search->pack;

			hashKey = Com_HashKey(handle->name, FILES_HASHSIZE);

			for (packFile = pack->filesHash[hashKey]; packFile; packFile = packFile->nextHash){
				if (!Q_stricmp(packFile->name, handle->name)){
					// Found it!
					if (fs_debug->integer)
						Com_Printf("FS_FOpenFileRead: %s (found in %s)\n", handle->name, pack->name);

					if (pack->pak){
						// PAK
						handle->file = fopen(pack->name, "rb");
						if (handle->file){
							fseek(handle->file, packFile->offset, SEEK_SET);

							return packFile->size;
						}
					}
					else if (pack->pk2){
						// PK2
						handle->zip = unzOpen(pack->name);
						if (handle->zip){
							if (unzLocateFile(handle->zip, handle->name, 2) == UNZ_OK){
								if (unzOpenCurrentFile(handle->zip) == UNZ_OK)
									return packFile->size;
							}

							unzClose(handle->zip);
						}
					}

					Com_Error(ERR_FATAL, "Couldn't reopen %s", pack->name);
				}
			}
		}
		else {
			// Search in a directory tree
			Q_snprintfz(path, sizeof(path), "%s/%s", search->path, handle->name);

			handle->file = fopen(path, "rb");
			if (handle->file){
				// Found it!
				if (fs_debug->integer)
					Com_Printf("FS_FOpenFileRead: %s (found in %s)\n", handle->name, search->path);

				return FS_FileLength(handle->file);
			}
		}
	}

	// Not found!
	if (fs_debug->integer)
		Com_Printf("FS_FOpenFileRead: couldn't find %s\n", handle->name);

	return -1;
}

/*
 =================
 FS_FOpenFile

 Opens a file for "mode".
 Returns file size or -1 if an error occurs/not found.
 =================
*/
int FS_FOpenFile (const char *name, fileHandle_t *f, fsMode_t mode){

	fsHandle_t	*handle;
	int			size;

	handle = FS_HandleForFile(f);

	Q_strncpyz(handle->name, name, sizeof(handle->name));
	handle->mode = mode;

	switch (mode){
	case FS_READ:
		size = FS_FOpenFileRead(handle);
		break;
	case FS_WRITE:
		size = FS_FOpenFileWrite(handle);
		break;
	case FS_APPEND:
		size = FS_FOpenFileAppend(handle);
		break;
	default:
		Com_Error(ERR_FATAL, "FS_FOpenFile: bad mode (%i)", mode);
	}

	if (size != -1)
		return size;

	// Couldn't open, so free the handle
	memset(handle, 0, sizeof(*handle));

	*f = 0;
	return -1;
}

/*
 =================
 FS_FCloseFile
 =================
*/
void FS_FCloseFile (fileHandle_t f){

	fsHandle_t *handle;

	handle = FS_GetFileByHandle(f);

	if (handle->file)
		fclose(handle->file);
	else if (handle->zip){
		unzCloseCurrentFile(handle->zip);
		unzClose(handle->zip);
	}

	memset(handle, 0, sizeof(*handle));
}

/*
 =================
 FS_Read

 Properly handles partial reads
 =================
*/
int FS_Read (void *buffer, int size, fileHandle_t f){

	fsHandle_t	*handle;
	int			remaining, r;
	byte		*buf;
	qboolean	tried = false;

	handle = FS_GetFileByHandle(f);

	if (size < 0)
		Com_Error(ERR_FATAL, "FS_Read: size < 0");

	// Read
	remaining = size;
	buf = (byte *)buffer;

	while (remaining){
		if (handle->file)
			r = fread(buf, 1, remaining, handle->file);
		else if (handle->zip)
			r = unzReadCurrentFile(handle->zip, buf, remaining);
		else
			return 0;

		if (r == 0){
			if (!tried){
				// We might have been trying to read from a CD
				CDAudio_Stop();
				tried = true;
			}
			else {
				Com_DPrintf(S_COLOR_RED "FS_Read: 0 bytes read from %s\n", handle->name);
				return size - remaining;
			}
		}
		else if (r == -1)
			Com_Error(ERR_FATAL, "FS_Read: -1 bytes read from %s", handle->name);

		remaining -= r;
		buf += r;
	}

	return size;
}

/*
 =================
 FS_Write

 Properly handles partial writes
 =================
*/
int FS_Write (const void *buffer, int size, fileHandle_t f){

	fsHandle_t	*handle;
	int			remaining, w;
	byte		*buf;

	handle = FS_GetFileByHandle(f);

	if (size < 0)
		Com_Error(ERR_FATAL, "FS_Write: size < 0");

	// Write
	remaining = size;
	buf = (byte *)buffer;

	while (remaining){
		if (handle->file)
			w = fwrite(buf, 1, remaining, handle->file);
		else if (handle->zip)
			Com_Error(ERR_FATAL, "FS_Write: can't write to zip file %s", handle->name);
		else
			return 0;

		if (w == 0){
			Com_DPrintf(S_COLOR_RED "FS_Write: 0 bytes written to %s\n", handle->name);
			return size - remaining;
		}
		else if (w == -1)
			Com_Error(ERR_FATAL, "FS_Write: -1 bytes written to %s", handle->name);

		remaining -= w;
		buf += w;
	}

	return size;
}

/*
 =================
 FS_Printf
 =================
*/
int FS_Printf (fileHandle_t f, const char *fmt, ...){

	fsHandle_t	*handle;
	va_list		argPtr;
	int			w;

	handle = FS_GetFileByHandle(f);

	// Write
	if (handle->file){
		va_start(argPtr, fmt);
		w = vfprintf(handle->file, fmt, argPtr);
		va_end(argPtr);
	}
	else if (handle->zip)
		Com_Error(ERR_FATAL, "FS_Printf: can't write to zip file %s", handle->name);
	else
		return 0;

	if (w == 0)
		Com_DPrintf(S_COLOR_RED "FS_Printf: 0 chars written to %s\n", handle->name);
	else if (w == -1)
		Com_Error(ERR_FATAL, "FS_Printf: -1 chars written to %s", handle->name);

	return w;
}

/*
 =================
 FS_Seek
 =================
*/
void FS_Seek (fileHandle_t f, int offset, fsOrigin_t origin){

	fsHandle_t		*handle;
	unz_file_info	info;
	int				remaining, r, len;
	byte			dummy[0x8000];

	handle = FS_GetFileByHandle(f);

	if (handle->file){
		switch (origin){
		case FS_SEEK_SET:
			fseek(handle->file, offset, SEEK_SET);
			break;
		case FS_SEEK_CUR:
			fseek(handle->file, offset, SEEK_CUR);
			break;
		case FS_SEEK_END:
			fseek(handle->file, offset, SEEK_END);
			break;
		default:
			Com_Error(ERR_FATAL, "FS_Seek: bad origin (%i)", origin);
		}
	}
	else if (handle->zip){
		switch (origin){
		case FS_SEEK_SET:
			remaining = offset;
			break;
		case FS_SEEK_CUR:
			remaining = offset + unztell(handle->zip);
			break;
		case FS_SEEK_END:
			unzGetCurrentFileInfo(handle->zip, &info, NULL, 0, NULL, 0, NULL, 0);

			remaining = offset + info.uncompressed_size;
			break;
		default:
			Com_Error(ERR_FATAL, "FS_Seek: bad origin (%i)", origin);
		}

		// Reopen the file
		unzCloseCurrentFile(handle->zip);
		unzOpenCurrentFile(handle->zip);

		// Skip until the desired offset is reached
		while (remaining){
			len = remaining;
			if (len > sizeof(dummy))
				len = sizeof(dummy);

			r = unzReadCurrentFile(handle->zip, dummy, len);
			if (r <= 0)
				break;

			remaining -= r;
		}
	}
}

/*
 =================
 FS_Tell
 =================
*/
int FS_Tell (fileHandle_t f){

	fsHandle_t *handle;

	handle = FS_GetFileByHandle(f);

	if (handle->file)
		return ftell(handle->file);
	else if (handle->zip)
		return unztell(handle->zip);

	return 0;
}

/*
 =================
 FS_Flush
 =================
*/
void FS_Flush (fileHandle_t f){

	fsHandle_t	*handle;

	handle = FS_GetFileByHandle(f);

	if (handle->file)
		fflush(handle->file);
	else if (handle->zip)
		Com_Error(ERR_FATAL, "FS_Flush: can't flush zip file %s", handle->name);
}

/*
 =================
 FS_CopyFile
 =================
*/
void FS_CopyFile (const char *srcName, const char *dstName){

	fileHandle_t	f1, f2;
	int				size, remaining, len;
	byte			buffer[0x8000];

	if (fs_debug->integer)
		Com_Printf("FS_CopyFile( %s, %s )\n", srcName, dstName);

	size = FS_FOpenFile(srcName, &f1, FS_READ);
	if (!f1){
		Com_DPrintf(S_COLOR_RED "FS_CopyFile: couldn't open %s\n", srcName);
		return;
	}

	FS_FOpenFile(dstName, &f2, FS_WRITE);
	if (!f2){
		FS_FCloseFile(f1);
		Com_DPrintf(S_COLOR_RED "FS_CopyFile: couldn't open %s\n", dstName);
		return;
	}

	// Copy in small chunks
	remaining = size;
	while (remaining){
		len = remaining;
		if (len > sizeof(buffer))
			len = sizeof(buffer);

		len = FS_Read(buffer, len, f1);
		if (!len)
			break;

		FS_Write(buffer, len, f2);
		remaining -= len;
	}

	FS_FCloseFile(f1);
	FS_FCloseFile(f2);
}

/*
 =================
 FS_RenameFile
 =================
*/
void FS_RenameFile (const char *oldName, const char *newName){

	char	oldPath[MAX_OSPATH], newPath[MAX_OSPATH];

	if (fs_debug->integer)
		Com_Printf("FS_RenameFile( %s, %s )\n", oldName, newName);

	Q_snprintfz(oldPath, sizeof(oldPath), "%s/%s", fs_gameDir, oldName);
	Q_snprintfz(newPath, sizeof(newPath), "%s/%s", fs_gameDir, newName);

	rename(oldPath, newPath);
}

/*
 =================
 FS_DeleteFile
 =================
*/
void FS_DeleteFile (const char *name){

	char	path[MAX_OSPATH];

	if (fs_debug->integer)
		Com_Printf("FS_DeleteFile( %s )\n", name);

	Q_snprintfz(path, sizeof(path), "%s/%s", fs_gameDir, name);

	remove(path);
}

/*
 =================
 FS_LoadFile

 File name is relative to the Quake search path.
 Returns file size or -1 if not found.
 A NULL buffer will just return the file size without loading.
 Appends a trailing 0 so that text files are loaded properly.
 =================
*/
int FS_LoadFile (const char *name, void **buffer){

	fileHandle_t	f;
	int				size;

	size = FS_FOpenFile(name, &f, FS_READ);
	if (!f){
		if (buffer)
			*buffer = NULL;

		return -1;
	}

	if (!buffer){
		FS_FCloseFile(f);
		return size;
	}

	*buffer = Z_Malloc(size + 1);

	FS_Read(*buffer, size, f);
	FS_FCloseFile(f);

	return size;
}

/*
 =================
 FS_FreeFile
 =================
*/
void FS_FreeFile (void *buffer){

	if (!buffer)
		Com_Error(ERR_FATAL, "FS_FreeFile: NULL buffer");

	Z_Free(buffer);
}

/*
 =================
 FS_SaveFile

 File name is relative to the Quake search path.
 Returns true if the file was saved.
 =================
*/
qboolean FS_SaveFile (const char *name, const void *buffer, int size){

	fileHandle_t	f;

	FS_FOpenFile(name, &f, FS_WRITE);
	if (!f)
		return false;

	FS_Write(buffer, size, f);
	FS_FCloseFile(f);

	return true;
}

/*
 =================
 FS_FindFiles

 Finds files in a given path with an optional extension (if extension is
 NULL, all the files in the path are listed).
 The file list is sorted alphabetically.
 =================
*/
static int FS_FindFiles (const char *path, const char *extension, char **fileList, int maxFiles){

	fsSearchPath_t	*search;
	fsPackFile_t	*packFile;
	fsPack_t		*pack;
	int				fileCount = 0;
	const char		*name;
	char			dir[MAX_OSPATH], ext[16];
	char			*dirFiles[MAX_FIND_FILES];
	int				dirCount, i, j;

	for (search = fs_searchPaths; search; search = search->next){
		if (search->pack){
			// Search inside a pack file
			pack = search->pack;

			for (i = 0, packFile = pack->files; i < pack->numFiles; i++, packFile++){
				// Match path
				Com_FilePath(packFile->name, dir, sizeof(dir));
				if (Q_stricmp(path, dir))
					continue;

				// Match extension
				if (extension){
					Com_FileExtension(packFile->name, ext, sizeof(ext));
					if (Q_stricmp(extension, ext))
						continue;
				}

				// Found something
				name = Com_SkipPath(packFile->name);
				if (fileCount < maxFiles){
					// Ignore duplicates
					for (j = 0; j < fileCount; j++){
						if (!Q_stricmp(fileList[j], name))
							break;
					}

					if (j == fileCount)
						fileList[fileCount++] = CopyString(name);
				}
			}
		}
		else {
			// Search in a directory tree
			Q_snprintfz(dir, sizeof(dir), "%s/%s", search->path, path);

			if (extension){
				Q_snprintfz(ext, sizeof(ext), "*.%s", extension);
				dirCount = Sys_FindFiles(dir, ext, dirFiles, MAX_FIND_FILES, true, false);
			}
			else
				dirCount = Sys_FindFiles(dir, "*", dirFiles, MAX_FIND_FILES, true, true);

			for (i = 0; i < dirCount; i++){
				// Found something
				name = Com_SkipPath(dirFiles[i]);
				if (fileCount < maxFiles){
					// Ignore duplicates
					for (j = 0; j < fileCount; j++){
						if (!Q_stricmp(fileList[j], name))
							break;
					}

					if (j == fileCount)
						fileList[fileCount++] = CopyString(name);
				}

				FreeString(dirFiles[i]);
			}
		}
	}

	// Sort the list
	qsort(fileList, fileCount, sizeof(char *), Q_SortStrcmp);

	return fileCount;
}

/*
 =================
 FS_FilteredFindFiles

 Finds files with a name that matches the given pattern.
 The file list is sorted alphabetically.
 =================
*/
static int FS_FilteredFindFiles (const char *pattern, char **fileList, int maxFiles){

	fsSearchPath_t	*search;
	fsPackFile_t	*packFile;
	fsPack_t		*pack;
	int				fileCount = 0;
	const char		*name;
	char			*dirFiles[MAX_FIND_FILES];
	int				dirCount, i, j;

	for (search = fs_searchPaths; search; search = search->next){
		if (search->pack){
			// Search inside a pack file
			pack = search->pack;

			for (i = 0, packFile = pack->files; i < pack->numFiles; i++, packFile++){
				// Match pattern
				if (!Q_GlobMatch(pattern, packFile->name, false))
					continue;

				// Found something
				name = packFile->name;
				if (fileCount < maxFiles){
					// Ignore duplicates
					for (j = 0; j < fileCount; j++){
						if (!Q_stricmp(fileList[j], name))
							break;
					}

					if (j == fileCount)
						fileList[fileCount++] = CopyString(name);
				}
			}
		}
		else {
			// Search in a directory tree
			dirCount = Sys_RecursiveFindFiles(search->path, dirFiles, MAX_FIND_FILES, 0, true, true);

			for (i = 0; i < dirCount; i++){
				// Match pattern
				if (!Q_GlobMatch(pattern, dirFiles[i] + strlen(search->path) + 1, false)){
					FreeString(dirFiles[i]);
					continue;
				}

				// Found something
				name = dirFiles[i] + strlen(search->path) + 1;
				if (fileCount < maxFiles){
					// Ignore duplicates
					for (j = 0; j < fileCount; j++){
						if (!Q_stricmp(fileList[j], name))
							break;
					}

					if (j == fileCount)
						fileList[fileCount++] = CopyString(name);
				}

				FreeString(dirFiles[i]);
			}
		}
	}

	// Sort the list
	qsort(fileList, fileCount, sizeof(char *), Q_SortStrcmp);

	return fileCount;
}

/*
 =================
 FS_GetFileList

 Finds files in a given path with an optional extension (if extension is
 NULL, all the files in the path are listed) or finds files with a name
 that matches the given pattern.

 If buffer is not NULL, the file list is stored without writting past
 size, and the number of files found is returned.
 If buffer is NULL, the required storage size is returned.
 =================
*/
int FS_GetFileList (const char *path, const char *extension, char *buffer, int size){

	char	*fileList[MAX_FIND_FILES];
	int		fileCount;
	int		i, len, ret = 0;

	// If path contains special characters, then treat it as a filter
	if (strchr(path, '*') || strchr(path, '?') || strchr(path, '[') || strchr(path, ']'))
		fileCount = FS_FilteredFindFiles(path, fileList, MAX_FIND_FILES);
	else
		fileCount = FS_FindFiles(path, extension, fileList, MAX_FIND_FILES);

	for (i = 0; i < fileCount; i++){
		len = strlen(fileList[i]) + 1;

		if (buffer){	// Store in the buffer, separated by zeros
			if (len <= size){
				Q_strncpyz(buffer, fileList[i], size);
				buffer += len;
				size -= len;
				ret++;
			}
		}
		else			// Add to required size
			ret += len;

		FreeString(fileList[i]);
	}

	return ret;
}

/*
 =================
 FS_GetModList

 Finds game modifications in the current directory.

 If buffer is not NULL, the mod list is stored without writting past
 size, and the number of mods found is returned.
 If buffer is NULL, the required storage size is returned.
 =================
*/
int FS_GetModList (char *buffer, int size){

	char	*dirFiles[MAX_FIND_FILES];
	char	dirCount;
	FILE	*f;
	char	*dir, *desc;
	char	*modList[MAX_FIND_FILES];
	int		modCount = 0;
	int		i, len, ret = 0;

	// Enumerate all the directories under the current directory
	dirCount = Sys_FindFiles(fs_homePath->string, "*", dirFiles, MAX_FIND_FILES, false, true);

	for (i = 0; i < dirCount; i++){
		dir = (char *)Com_SkipPath(dirFiles[i]);

		// Ignore baseq2
		if (!Q_stricmp(dir, BASEDIRNAME)){
			FreeString(dirFiles[i]);
			continue;
		}

		// Try to load a description.txt file, otherwise use the
		// directory name.
		f = fopen(va("%s/description.txt", dirFiles[i]), "rt");
		if (f){
			len = FS_FileLength(f);
			desc = Z_Malloc(len+1);
			fread(desc, 1, len, f);
			fclose(f);

			if (modCount + 1 < MAX_FIND_FILES){
				modList[modCount++] = CopyString(dir);
				modList[modCount++] = CopyString(desc);
			}

			Z_Free(desc);
		}
		else {
			if (modCount + 1 < MAX_FIND_FILES){
				modList[modCount++] = CopyString(dir);
				modList[modCount++] = CopyString(dir);
			}
		}

		FreeString(dirFiles[i]);
	}

	for (i = 0; i < modCount; i++){
		len = strlen(modList[i]) + 1;

		if (buffer){	// Store in the buffer, separated by zeros
			if (len <= size){
				Q_strncpyz(buffer, modList[i], size);
				buffer += len;
				size -= len;
				ret++;
			}
		}
		else			// Add to required size
			ret += len;

		FreeString(modList[i]);
	}

	return ret;
}

/*
 =================
 FS_CreatePath

 Creates any directories needed to store the given filename
 =================
*/
void FS_CreatePath (const char *path){

	char	dir[MAX_OSPATH];
	char	*s, *ofs;

	if (fs_debug->integer)
		Com_Printf("FS_CreatePath( %s )\n", path);

	if (strstr(path, "..") || strstr(path, "::") || strstr(path, "\\\\") || strstr(path, "//")){
		Com_DPrintf(S_COLOR_RED "FS_CreatePath: refusing to create relative path '%s'\n", path);
		return;
	}

	Q_snprintfz(dir, sizeof(dir), "%s/%s", fs_gameDir, path);

	s = dir;
	for (ofs = s+1; *ofs; ofs++){
		if (*ofs == '/' || *ofs == '\\'){
			// Create the directory
			*ofs = 0;
			Sys_CreateDirectory(s);
			*ofs = '/';
		}
	}
}

/*
 =================
 FS_RemovePath

 Removes any files and subdirectories in the given directory tree, then
 removes the given directory
 =================
*/
void FS_RemovePath (const char *path){

	char	dir[MAX_OSPATH];
	char	*dirFiles[MAX_FIND_FILES];
	int		dirCount, i;

	if (fs_debug->integer)
		Com_Printf("FS_RemovePath( %s )\n", path);

	if (strstr(path, "..") || strstr(path, "::") || strstr(path, "\\\\") || strstr(path, "//")){
		Com_DPrintf(S_COLOR_RED "FS_RemovePath: refusing to remove relative path '%s'\n", path);
		return;
	}

	Q_snprintfz(dir, sizeof(dir), "%s/%s", fs_gameDir, path);

	// Remove any files in the directory tree
	dirCount = Sys_RecursiveFindFiles(dir, dirFiles, MAX_FIND_FILES, 0, true, false);
	for (i = 0; i < dirCount; i++){
		remove(dirFiles[i]);
		FreeString(dirFiles[i]);
	}

	// Remove any subdirectories in the directory tree.
	// We must walk this list in reverse order!!!
	dirCount = Sys_RecursiveFindFiles(dir, dirFiles, MAX_FIND_FILES, 0, false, true);
	for (i = dirCount - 1; i >= 0; i--){
		Sys_RemoveDirectory(dirFiles[i]);
		FreeString(dirFiles[i]);
	}

	// Remove the root directory
	Sys_RemoveDirectory(dir);
}

/*
 =================
 FS_NextPath

 Allows enumerating all of the directories in the search path
 =================
*/
char *FS_NextPath (char *prevPath){

	fsSearchPath_t	*search;
	char			*prev;

	if (!prevPath)
		return fs_gameDir;

	prev = fs_gameDir;
	for (search = fs_searchPaths; search; search = search->next){
		if (search->pack)
			continue;

		if (prevPath == prev)
			return search->path;

		prev = search->path;
	}

	return NULL;
}

/*
 =================
 FS_LoadPAK

 Takes an explicit (not game tree related) path to a pack file.

 Loads the header and directory, adding the files at the beginning of
 the list so they override previous pack files.
 =================
*/
static fsPack_t *FS_LoadPAK (const char *packPath){

	int				numFiles, i;
	fsPackFile_t	*packFile;
	fsPack_t		*pack;
	FILE			*handle;
	pakHeader_t		header;
	pakFile_t		info;
	unsigned		hashKey;

	handle = fopen(packPath, "rb");
	if (!handle)
		Com_Error(ERR_FATAL, "FS_LoadPAK: can't open '%s'", packPath);

	fread(&header, 1, sizeof(pakHeader_t), handle);

	if (LittleLong(header.ident) != PAK_IDENT){
		fclose(handle);
		Com_Error(ERR_FATAL, "FS_LoadPAK: '%s' is not a pack file", packPath);
	}

	header.dirOfs = LittleLong(header.dirOfs);
	header.dirLen = LittleLong(header.dirLen);

	numFiles = header.dirLen / sizeof(pakFile_t);
	if (numFiles <= 0){
		fclose(handle);
		Com_Error(ERR_FATAL, "FS_LoadPAK: '%s' is empty", packPath);
	}

	packFile = Z_Malloc(numFiles * sizeof(fsPackFile_t));
	pack = Z_Malloc(sizeof(fsPack_t));

	Q_strncpyz(pack->name, packPath, sizeof(pack->name));
	pack->pak = handle;
	pack->pk2 = NULL;
	pack->numFiles = numFiles;
	pack->files = packFile;

	// Parse the directory
	fseek(handle, header.dirOfs, SEEK_SET);

	for (i = 0; i < numFiles; i++){
		fread(&info, 1, sizeof(pakFile_t), handle);

		Q_strncpyz(packFile->name, info.name, sizeof(packFile->name));
		packFile->size = LittleLong(info.fileLen);
		packFile->offset = LittleLong(info.filePos);

		// Add to hash table
		hashKey = Com_HashKey(packFile->name, FILES_HASHSIZE);

		packFile->nextHash = pack->filesHash[hashKey];
		pack->filesHash[hashKey] = packFile;

		// Go to next file
		packFile++;
	}

	return pack;
}

/*
 =================
 FS_LoadPK2

 Takes an explicit (not game tree related) path to a pack file.

 Loads the header and directory, adding the files at the beginning of
 the list so they override previous pack files.
 =================
*/
static fsPack_t *FS_LoadPK2 (const char *packPath){

	int				numFiles;
	fsPackFile_t	*packFile;
	fsPack_t		*pack;
	unzFile			*handle;
	unz_global_info	global;
	unz_file_info	info;
	int				status;
	char			name[MAX_QPATH];
	unsigned		hashKey;

	handle = unzOpen(packPath);
	if (!handle)
		Com_Error(ERR_FATAL, "FS_LoadPK2: can't open '%s'", packPath);

	if (unzGetGlobalInfo(handle, &global) != UNZ_OK){
		unzClose(handle);
		Com_Error(ERR_FATAL, "FS_LoadPK2: '%s' is not a pack file", packPath);
	}

	numFiles = global.number_entry;
	if (numFiles <= 0){
		unzClose(handle);
		Com_Error(ERR_FATAL, "FS_LoadPK2: '%s' is empty", packPath);
	}

	packFile = Z_Malloc(numFiles * sizeof(fsPackFile_t));
	pack = Z_Malloc(sizeof(fsPack_t));

	Q_strncpyz(pack->name, packPath, sizeof(pack->name));
	pack->pak = NULL;
	pack->pk2 = handle;
	pack->numFiles = numFiles;
	pack->files = packFile;

	// Parse the directory
	status = unzGoToFirstFile(handle);

	while (status == UNZ_OK){
		unzGetCurrentFileInfo(handle, &info, name, MAX_QPATH, NULL, 0, NULL, 0);

		Q_strncpyz(packFile->name, name, sizeof(packFile->name));
		packFile->size = info.uncompressed_size;
		packFile->offset = -1;		// Not used in ZIP files

		// Add to hash table
		hashKey = Com_HashKey(packFile->name, FILES_HASHSIZE);

		packFile->nextHash = pack->filesHash[hashKey];
		pack->filesHash[hashKey] = packFile;

		// Go to next file
		packFile++;

		status = unzGoToNextFile(handle);
	}

	return pack;
}

/*
 =================
 FS_AddGameDirectory

 Sets fs_gameDir, adds the directory to the head of the path, then loads
 and adds all the pack files found (in alphabetical order).

 PK2 files are loaded later so they override PAK files.
 =================
*/
static void FS_AddGameDirectory (const char *dir){

	fsSearchPath_t	*search;
	fsPack_t		*pack;
	char			*dirFiles[MAX_PACK_FILES];
	int				dirCount, i;

	if (!dir || !dir[0])
		return;

	// Don't add the same directory twice
	for (search = fs_searchPaths; search; search = search->next){
		if (!Q_stricmp(search->path, dir))
			return;
	}

	Q_strncpyz(fs_gameDir, dir, sizeof(fs_gameDir));

	// Add the directory to the search path
	search = Z_Malloc(sizeof(fsSearchPath_t));
	Q_strncpyz(search->path, dir, sizeof(search->path));

	search->next = fs_searchPaths;
	fs_searchPaths = search;

	// Add any PAK files
	dirCount = Sys_FindFiles(dir, "*.pak", dirFiles, MAX_PACK_FILES, true, false);
	for (i = 0; i < dirCount; i++){
		pack = FS_LoadPAK(dirFiles[i]);

		search = Z_Malloc(sizeof(fsSearchPath_t));
		search->pack = pack;
		search->next = fs_searchPaths;
		fs_searchPaths = search;

		FreeString(dirFiles[i]);
	}

	// Add any PK2 files
	dirCount = Sys_FindFiles(dir, "*.pk2", dirFiles, MAX_PACK_FILES, true, false);
	for (i = 0; i < dirCount; i++){
		pack = FS_LoadPK2(dirFiles[i]);

		search = Z_Malloc(sizeof(fsSearchPath_t));
		search->pack = pack;
		search->next = fs_searchPaths;
		fs_searchPaths = search;

		FreeString(dirFiles[i]);
	}
}

/*
 =================
 FS_Dir_f
 =================
*/
void FS_Dir_f (void){

	char	*dirFiles[MAX_FIND_FILES];
	int		dirCount, i;

	if (Cmd_Argc() < 2 || Cmd_Argc() > 3){
		Com_Printf("Usage: dir <directory> [extension]\n");
		return;
	}

	Com_Printf("Directory of %s\n", Cmd_Argv(1));
	Com_Printf("----------------------\n");

	if (Cmd_Argc() == 2)
		dirCount = FS_FindFiles(Cmd_Argv(1), NULL, dirFiles, MAX_FIND_FILES);
	else
		dirCount = FS_FindFiles(Cmd_Argv(1), Cmd_Argv(2), dirFiles, MAX_FIND_FILES);

	for (i = 0; i < dirCount; i++){
		Com_Printf("%s\n", dirFiles[i]);
		FreeString(dirFiles[i]);
	}

	Com_Printf("\n");
	Com_Printf("%i files listed\n", dirCount);
}

/*
 =================
 FS_FDir_f
 =================
*/
void FS_FDir_f (void){

	char	*dirFiles[MAX_FIND_FILES];
	int		dirCount, i;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: fdir <filter>\n");
		return;
	}

	Com_Printf("Matches for %s\n", Cmd_Argv(1));
	Com_Printf("----------------------\n");

	dirCount = FS_FilteredFindFiles(Cmd_Argv(1), dirFiles, MAX_FIND_FILES);

	for (i = 0; i < dirCount; i++){
		Com_Printf("%s\n", dirFiles[i]);
		FreeString(dirFiles[i]);
	}

	Com_Printf("\n");
	Com_Printf("%i files listed\n", dirCount);
}

/*
 =================
 FS_Path_f
 =================
*/
void FS_Path_f (void){

	fsSearchPath_t	*search;
	fsHandle_t		*handle;
	int				i, totalFiles = 0;

	Com_Printf("Current search path:\n");

	for (search = fs_searchPaths; search; search = search->next){
		if (search->pack){
			Com_Printf("%s (%i files)\n", search->pack->name, search->pack->numFiles);
			totalFiles += search->pack->numFiles;
		}
		else
			Com_Printf("%s\n", search->path);
	}

	Com_Printf("\n");

	for (i = 0, handle = fs_handles; i < MAX_HANDLES; i++, handle++){
		if (!handle->used)
			continue;

		Com_Printf("Handle %2i ", i+1);

		switch (handle->mode){
		case FS_READ:
			Com_Printf("(R) ");
			break;
		case FS_WRITE:
			Com_Printf("(W) ");
			break;
		case FS_APPEND:
			Com_Printf("(A) ");
			break;
		}

		Com_Printf(": %s\n", handle->name);
	}

	Com_Printf("----------------------\n");
	Com_Printf("%i files in PAK/PK2 files\n", totalFiles);
}

/*
 =================
 FS_Startup
 =================
*/
static void FS_Startup (void){

	Com_Printf("----- FS_Startup -----\n");

	// Add the directories
	FS_AddGameDirectory(fs_cdPath->string);
	FS_AddGameDirectory(fs_basePath->string);

	if (strstr(fs_baseGame->string, "..") || strstr(fs_baseGame->string, ".") || strstr(fs_baseGame->string, "/") || strstr(fs_baseGame->string, "\\") || strstr(fs_baseGame->string, ":") || !fs_baseGame->string[0]){
		Com_Printf("Invalid game directory\n");
		Cvar_ForceSet("fs_baseGame", BASEDIRNAME);
	}

	FS_AddGameDirectory(va("%s/%s", fs_homePath->string, fs_baseGame->string));

	if (strstr(fs_game->string, "..") || strstr(fs_game->string, ".") || strstr(fs_game->string, "/") || strstr(fs_game->string, "\\") || strstr(fs_game->string, ":") || !fs_game->string[0]){
		Com_Printf("Invalid game directory\n");
		Cvar_ForceSet("fs_game", BASEDIRNAME);
	}

	FS_AddGameDirectory(va("%s/%s", fs_homePath->string, fs_game->string));

	// Set the current game
	Q_strncpyz(fs_curGame, fs_game->string, sizeof(fs_curGame));

	FS_Path_f();
}

/*
 =================
 FS_Restart
 =================
*/
void FS_Restart (void){

	if (!Q_stricmp(fs_curGame, fs_game->string)){
		// Just add the directories
		FS_Startup();
		return;
	}

	// The current game changed, so restart the file system
	FS_Shutdown();
	FS_Init();
}

/*
 =================
 FS_Init
 =================
*/
void FS_Init (void){

	// Register our cvars and commands
	fs_homePath = Cvar_Get("fs_homePath", Sys_GetCurrentDirectory(), CVAR_INIT);
	fs_cdPath = Cvar_Get("fs_cdPath", Sys_ScanForCD(), CVAR_INIT);
	fs_basePath = Cvar_Get("fs_basePath", va("%s/%s", Sys_GetCurrentDirectory(), BASEDIRNAME), CVAR_INIT);
	fs_baseGame = Cvar_Get("fs_baseGame", BASEDIRNAME, CVAR_INIT);
	fs_game = Cvar_Get("fs_game", BASEDIRNAME, CVAR_SERVERINFO | CVAR_INIT);
	fs_debug = Cvar_Get("fs_debug", "0", 0);

	Cmd_AddCommand("dir", FS_Dir_f);
	Cmd_AddCommand("fdir", FS_FDir_f);
	Cmd_AddCommand("path", FS_Path_f);

	// Add the directories
	FS_Startup();

	// Make sure default.cfg exists
	if (FS_LoadFile("default.cfg", NULL) == -1)
		Com_Error(ERR_FATAL, "Could not find default.cfg");

	// Exec config files
	Cbuf_AddText("exec default.cfg\n");
	Cbuf_AddText("exec q2econfig.cfg\n");
	Cbuf_AddText("exec autoexec.cfg\n");
	Cbuf_Execute();
}

/*
 =================
 FS_Shutdown
 =================
*/
void FS_Shutdown (void){

	fsHandle_t		*handle;
	fsSearchPath_t	*next;
	fsPack_t		*pack;
	int				i;

	Cmd_RemoveCommand("dir");
	Cmd_RemoveCommand("fdir");
	Cmd_RemoveCommand("path");

	// Close all files
	for (i = 0, handle = fs_handles; i < MAX_HANDLES; i++, handle++){
		if (!handle->used)
			continue;

		if (handle->file)
			fclose(handle->file);
		else if (handle->zip){
			unzCloseCurrentFile(handle->zip);
			unzClose(handle->zip);
		}

		memset(handle, 0, sizeof(*handle));
	}

	// Free search paths
	while (fs_searchPaths){
		if (fs_searchPaths->pack){
			pack = fs_searchPaths->pack;

			if (pack->pak)
				fclose(pack->pak);
			else if (pack->pk2)
				unzClose(pack->pk2);

			Z_Free(pack->files);
			Z_Free(pack);
		}

		next = fs_searchPaths->next;
		Z_Free(fs_searchPaths);
		fs_searchPaths = next;
	}
}
