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

#include <unzip.h>

/*

 All of Quake's data access is through a hierarchical file system, but
 the contents of the file system can be transparently merged from
 several sources.

 The "base directory" is the path to the directory holding the
 executable and all game directories. This can be overridden with the
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

#define FILES_HASH_SIZE		1024

#define MAX_FILE_HANDLES	64
#define MAX_LIST_FILES		65536

#define	BASE_DIRECTORY		"baseq2"

typedef struct {
	qboolean			active;
	char				name[MAX_OSPATH];
	fsMode_t			mode;

	FILE				*realFile;				// Only one of realFile or
	unzFile				*zipFile;				// zipFile will be used
} file_t;

typedef struct packFile_s {
	char				name[MAX_OSPATH];
	int					size;
	int					offset;					// This is ignored in PK2 files
	qboolean			isDirectory;			// Always false in PAK files

	struct packFile_s	*nextHash;
} packFile_t;

typedef struct {
	char				name[MAX_OSPATH];
	FILE				*pak;					// Only one of pak or
	unzFile				*pk2;					// pk2 will be used

	int					numFiles;
	packFile_t			*files;
	packFile_t			*filesHashTable[FILES_HASH_SIZE];
} pack_t;

typedef struct searchPath_s {
	char				directory[MAX_OSPATH];	// Only one of path or
	pack_t				*pack;					// pack will be used

	struct searchPath_s	*next;
} searchPath_t;

static file_t		fs_fileHandles[MAX_FILE_HANDLES];

static searchPath_t	*fs_searchPaths;

static char			fs_gameDirectory[MAX_OSPATH];

cvar_t	*fs_homePath;
cvar_t	*fs_cdPath;
cvar_t	*fs_basePath;
cvar_t	*fs_baseGame;
cvar_t	*fs_game;
cvar_t	*fs_debug;


/*
 =================
 FS_HandleForFile

 Returns a free fileHandle_t
 =================
*/
static file_t *FS_HandleForFile (fileHandle_t *f){

	file_t	*file;
	int		i;

	for (i = 0, file = fs_fileHandles; i < MAX_FILE_HANDLES; i++, file++){
		if (file->active)
			continue;

		*f = i+1;
		return file;
	}

	// Failed
	Com_Error(ERR_FATAL, "FS_HandleForFile: none free");
}

/*
 =================
 FS_GetFileByHandle

 Returns a file_t for the given fileHandle_t
 =================
*/
static file_t *FS_GetFileByHandle (fileHandle_t f){

	if (f <= 0 || f > MAX_FILE_HANDLES)
		Com_Error(ERR_FATAL, "FS_GetFileByHandle: out of range");

	return &fs_fileHandles[f-1];
}

/*
 =================
 FS_FileLength
 =================
*/
static int FS_FileLength (FILE *f){

	int		cur, end;

	cur = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, cur, SEEK_SET);

	return end;
}

/*
 =================
 FS_CreatePath

 Creates any directories needed to store the given file
 =================
*/
static void FS_CreatePath (char *path){

	char	*p;

	if (fs_debug->integerValue)
		Com_Printf("FS_CreatePath( %s )\n", path);

	if (strstr(path, "..") || strstr(path, "::") || strstr(path, "//") || strstr(path, "\\\\")){
		Com_DPrintf(S_COLOR_RED "FS_CreatePath: refusing to create relative path '%s'\n", path);
		return;
	}

	p = path + 1;
	while (*p){
		if (*p == '/' || *p == '\\'){
			// Create the directory
			*p = 0;
			Sys_CreateDirectory(path);
			*p = '/';
		}

		p++;
	}
}

/*
 =================
 FS_OpenFileAppend

 Returns file size or -1 on error
 =================
*/
static int FS_OpenFileAppend (const char *name, FILE **realFile){

	char	path[MAX_OSPATH];

	Q_snprintfz(path, sizeof(path), "%s/%s", fs_gameDirectory, name);

	FS_CreatePath(path);

#ifdef SECURE
	*realFile = fopen_s(*realFile, path, "ab");
#else
	*realFile = fopen(path, "ab");
#endif
	if (*realFile){
		if (fs_debug->integerValue)
			Com_Printf("FS_OpenFileAppend: '%s'\n", name);

		return FS_FileLength(*realFile);
	}

	if (fs_debug->integerValue)
		Com_Printf("FS_OpenFileAppend: couldn't open '%s'\n", name);

	return -1;
}

/*
 =================
 FS_OpenFileWrite

 Always returns 0 or -1 on error
 =================
*/
static int FS_OpenFileWrite (const char *name, FILE **realFile){

	char	path[MAX_OSPATH];

	Q_snprintfz(path, sizeof(path), "%s/%s", fs_gameDirectory, name);

	FS_CreatePath(path);

#ifdef SECURE
	*realFile = fopen_s(*realFile, path, "wb");
#else
	*realFile = fopen(path, "wb");
#endif
	if (*realFile){
		if (fs_debug->integerValue)
			Com_Printf("FS_OpenFileWrite: '%s'\n", name);

		return 0;
	}

	if (fs_debug->integerValue)
		Com_Printf("FS_OpenFileWrite: couldn't open '%s'\n", name);

	return -1;
}

/*
 =================
 FS_OpenFileRead

 Returns file size or -1 if not found.
 Can open separate files as well as files inside pack files (both PAK
 and PK2).
 =================
*/
static int FS_OpenFileRead (const char *name, FILE **realFile, unzFile **zipFile){

	searchPath_t	*searchPath;
	packFile_t		*packFile;
	pack_t			*pack;
	char			path[MAX_OSPATH];
	unsigned		hash;

	// Search through the path, one element at a time
	for (searchPath = fs_searchPaths; searchPath; searchPath = searchPath->next){
		if (searchPath->pack){
			// Search inside a pack file
			pack = searchPath->pack;

			hash = Com_HashKey(name, FILES_HASH_SIZE);

			for (packFile = pack->filesHashTable[hash]; packFile; packFile = packFile->nextHash){
				if (packFile->isDirectory)
					continue;

				if (!Q_stricmp(packFile->name, name)){
					// Found it!
					if (fs_debug->integerValue)
						Com_Printf("FS_OpenFileRead: '%s' (found in '%s')\n", name, pack->name);

					if (pack->pak){
						// PAK
#ifdef SECURE
						*realFile = fopen_s(*realFile, pack->name, "rb");
#else
						*realFile = fopen(pack->name, "rb");
#endif
						if (*realFile){
							fseek(*realFile, packFile->offset, SEEK_SET);

							return packFile->size;
						}
					}
					else if (pack->pk2){
						// PK2
						*zipFile = unzOpen(pack->name);
						if (*zipFile){
							if (unzLocateFile(*zipFile, name, 2) == UNZ_OK){
								if (unzOpenCurrentFile(*zipFile) == UNZ_OK)
									return packFile->size;
							}

							unzClose(*zipFile);
						}
					}

					Com_DPrintf(S_COLOR_RED "FS_OpenFileRead: couldn't reopen '%s'\n", pack->name);

					return -1;
				}
			}
		}
		else {
			// Search in a directory tree
			Q_snprintfz(path, sizeof(path), "%s/%s", searchPath->directory, name);

#ifdef SECURE
			*realFile = fopen_s(*realFile, path, "rb");
#else
			*realFile = fopen(path, "rb");
#endif
			if (*realFile){
				// Found it!
				if (fs_debug->integerValue)
					Com_Printf("FS_OpenFileRead: '%s' (found in '%s')\n", name, searchPath->directory);

				return FS_FileLength(*realFile);
			}
		}
	}

	// Not found!
	if (fs_debug->integerValue)
		Com_Printf("FS_OpenFileRead: couldn't find '%s'\n", name);

	return -1;
}

/*
 =================
 FS_OpenFile

 Opens a file for "mode".
 Returns file size or -1 if an error occurs/not found.
 =================
*/
int FS_OpenFile (const char *name, fileHandle_t *f, fsMode_t mode){

	file_t	*file;
	FILE	*realFile = NULL;
	unzFile	*zipFile = NULL;
	int		size;

	// Try to open the file
	switch (mode){
	case FS_READ:
		size = FS_OpenFileRead(name, &realFile, &zipFile);
		break;
	case FS_WRITE:
		size = FS_OpenFileWrite(name, &realFile);
		break;
	case FS_APPEND:
		size = FS_OpenFileAppend(name, &realFile);
		break;
	default:
		Com_Error(ERR_FATAL, "FS_OpenFile: bad mode for '%s'", name);
	}

	if (size == -1){
		*f = 0;

		return -1;
	}

	// Create a new file handle
	file = FS_HandleForFile(f);

	file->active = true;
	Q_strncpyz(file->name, name, sizeof(file->name));
	file->mode = mode;
	file->realFile = realFile;
	file->zipFile = zipFile;

	return size;
}

/*
 =================
 FS_CloseFile
 =================
*/
void FS_CloseFile (fileHandle_t f){

	file_t	*file;

	file = FS_GetFileByHandle(f);

	if (file->realFile)
		fclose(file->realFile);
	else if (file->zipFile){
		unzCloseCurrentFile(file->zipFile);
		unzClose(file->zipFile);
	}

	memset(file, 0, sizeof(file_t));
}

/*
 =================
 FS_Read

 Properly handles partial reads
 =================
*/
int FS_Read (void *buffer, int size, fileHandle_t f){

	file_t	*file;
	int		remaining, r;
	byte	*buf;

	if (size < 0)
		Com_Error(ERR_FATAL, "FS_Read: size < 0");

	file = FS_GetFileByHandle(f);

	if (file->mode != FS_READ)
		Com_Error(ERR_FATAL, "FS_Read: '%s' is not opened in read mode", file->name);

	// Read
	remaining = size;
	buf = (byte *)buffer;

	while (remaining){
		if (file->realFile)
			r = fread(buf, 1, remaining, file->realFile);
		else if (file->zipFile)
			r = unzReadCurrentFile(file->zipFile, buf, remaining);
		else
			return 0;

		if (r == 0){
			Com_DPrintf(S_COLOR_RED "FS_Read: 0 bytes read from '%s'\n", file->name);
			return size - remaining;
		}
		else if (r == -1)
			Com_Error(ERR_FATAL, "FS_Read: -1 bytes read from '%s'", file->name);

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

	file_t	*file;
	int		remaining, w;
	byte	*buf;

	if (size < 0)
		Com_Error(ERR_FATAL, "FS_Write: size < 0");

	file = FS_GetFileByHandle(f);

	if (file->mode != FS_WRITE && file->mode != FS_APPEND)
		Com_Error(ERR_FATAL, "FS_Write: '%s' is not opened in write/append mode", file->mode);

	// Write
	remaining = size;
	buf = (byte *)buffer;

	while (remaining){
		if (file->realFile)
			w = fwrite(buf, 1, remaining, file->realFile);
		else if (file->zipFile)
			Com_Error(ERR_FATAL, "FS_Write: can't write to zip file '%s'", file->name);
		else
			return 0;

		if (w == 0){
			Com_DPrintf(S_COLOR_RED "FS_Write: 0 bytes written to '%s'\n", file->name);
			return size - remaining;
		}
		else if (w == -1)
			Com_Error(ERR_FATAL, "FS_Write: -1 bytes written to '%s'", file->name);

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

	file_t	*file;
	va_list	argPtr;
	int		w;

	file = FS_GetFileByHandle(f);

	if (file->mode != FS_WRITE && file->mode != FS_APPEND)
		Com_Error(ERR_FATAL, "FS_Printf: '%s' is not opened in write/append mode", file->name);

	// Write
	if (file->realFile){
		va_start(argPtr, fmt);
		w = vfprintf(file->realFile, fmt, argPtr);
		va_end(argPtr);
	}
	else if (file->zipFile)
		Com_Error(ERR_FATAL, "FS_Printf: can't write to zip file '%s'", file->name);
	else
		return 0;

	if (w == 0)
		Com_DPrintf(S_COLOR_RED "FS_Printf: 0 chars written to '%s'\n", file->name);
	else if (w == -1)
		Com_Error(ERR_FATAL, "FS_Printf: -1 chars written to '%s'", file->name);

	return w;
}

/*
 =================
 FS_Seek
 =================
*/
void FS_Seek (fileHandle_t f, int offset, fsOrigin_t origin){

	file_t			*file;
	unz_file_info	info;
	int				remaining, len;
	byte			dummy[0x8000];

	file = FS_GetFileByHandle(f);

	if (file->realFile){
		switch (origin){
		case FS_SEEK_SET:
			fseek(file->realFile, offset, SEEK_SET);
			break;
		case FS_SEEK_CUR:
			fseek(file->realFile, offset, SEEK_CUR);
			break;
		case FS_SEEK_END:
			fseek(file->realFile, offset, SEEK_END);
			break;
		default:
			Com_Error(ERR_FATAL, "FS_Seek: bad origin for '%s'", file->name);
		}
	}
	else if (file->zipFile){
		switch (origin){
		case FS_SEEK_SET:
			remaining = offset;
			break;
		case FS_SEEK_CUR:
			remaining = offset + unztell(file->zipFile);
			break;
		case FS_SEEK_END:
			unzGetCurrentFileInfo(file->zipFile, &info, NULL, 0, NULL, 0, NULL, 0);

			remaining = offset + info.uncompressed_size;
			break;
		default:
			Com_Error(ERR_FATAL, "FS_Seek: bad origin for '%s'", file->name);
		}

		// Reopen the file
		unzCloseCurrentFile(file->zipFile);
		unzOpenCurrentFile(file->zipFile);

		// Skip until the desired offset is reached
		while (remaining){
			len = remaining;
			if (len > sizeof(dummy))
				len = sizeof(dummy);

			len = unzReadCurrentFile(file->zipFile, dummy, len);
			if (len <= 0)
				break;

			remaining -= len;
		}
	}
}

/*
 =================
 FS_Tell
 =================
*/
int FS_Tell (fileHandle_t f){

	file_t	*file;

	file = FS_GetFileByHandle(f);

	if (file->realFile)
		return ftell(file->realFile);
	else if (file->zipFile)
		return unztell(file->zipFile);

	return 0;
}

/*
 =================
 FS_Flush
 =================
*/
void FS_Flush (fileHandle_t f){

	file_t	*file;

	file = FS_GetFileByHandle(f);

	if (file->realFile)
		fflush(file->realFile);
	else if (file->zipFile)
		Com_Error(ERR_FATAL, "FS_Flush: can't flush zip file '%s'", file->name);
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

	if (fs_debug->integerValue)
		Com_Printf("FS_CopyFile( %s, %s )\n", srcName, dstName);

	size = FS_OpenFile(srcName, &f1, FS_READ);
	if (!f1){
		Com_DPrintf(S_COLOR_RED "FS_CopyFile: couldn't open '%s'\n", srcName);
		return;
	}

	FS_OpenFile(dstName, &f2, FS_WRITE);
	if (!f2){
		FS_CloseFile(f1);
		Com_DPrintf(S_COLOR_RED "FS_CopyFile: couldn't open '%s'\n", dstName);
		return;
	}

	// Copy in small chunks
	remaining = size;
	while (remaining){
		len = remaining;
		if (len > sizeof(buffer))
			len = sizeof(buffer);

		len = FS_Read(buffer, len, f1);
		if (!len){
			Com_DPrintf(S_COLOR_RED "FS_CopyFile: 0 bytes copied from '%s' to '%s'\n", srcName, dstName);
			break;
		}

		FS_Write(buffer, len, f2);
		remaining -= len;
	}

	FS_CloseFile(f1);
	FS_CloseFile(f2);
}

/*
 =================
 FS_RenameFile
 =================
*/
void FS_RenameFile (const char *oldName, const char *newName){

	char	oldPath[MAX_OSPATH], newPath[MAX_OSPATH];

	if (fs_debug->integerValue)
		Com_Printf("FS_RenameFile( %s, %s )\n", oldName, newName);

	Q_snprintfz(oldPath, sizeof(oldPath), "%s/%s", fs_gameDirectory, oldName);
	Q_snprintfz(newPath, sizeof(newPath), "%s/%s", fs_gameDirectory, newName);

	if (rename(oldPath, newPath))
		Com_DPrintf(S_COLOR_RED "FS_RenameFile: couldn't rename '%s' to '%s'\n", oldName, newName);
}

/*
 =================
 FS_RemoveFile
 =================
*/
void FS_RemoveFile (const char *name){

	char	path[MAX_OSPATH];

	if (fs_debug->integerValue)
		Com_Printf("FS_RemoveFile( %s )\n", name);

	Q_snprintfz(path, sizeof(path), "%s/%s", fs_gameDirectory, name);

	if (remove(path))
		Com_DPrintf(S_COLOR_RED "FS_RemoveFile: couldn't remove '%s'\n", name);
}

/*
 =================
 FS_LoadFile

 File name is relative to the search path.
 Returns file size or -1 if not found.
 A NULL buffer will just return the file size without loading.
 Appends a trailing 0 so that text files are loaded properly.
 =================
*/
int FS_LoadFile (const char *name, void **buffer){

	fileHandle_t	f;
	byte			*buf;
	int				size;

	size = FS_OpenFile(name, &f, FS_READ);
	if (!f){
		if (buffer)
			*buffer = NULL;

		return -1;
	}

	if (!buffer){
		FS_CloseFile(f);
		return size;
	}

	buf = Z_Malloc(size + 1);

	FS_Read(buf, size, f);
	FS_CloseFile(f);

	buf[size] = 0;
	*buffer = buf;

	return size;
}

/*
 =================
 FS_FreeFile

 Frees the memory allocated by FS_LoadFile
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

 File name is relative to the search path.
 Returns true if the file was saved.
 =================
*/
qboolean FS_SaveFile (const char *name, const void *buffer, int size){

	fileHandle_t	f;

	FS_OpenFile(name, &f, FS_WRITE);
	if (!f)
		return false;

	FS_Write(buffer, size, f);
	FS_CloseFile(f);

	return true;
}

/*
 =================
 FS_FileExists

 Returns true if the file exists
 =================
*/
qboolean FS_FileExists (const char *name){

	fileHandle_t	f;

	FS_OpenFile(name, &f, FS_READ);
	if (!f)
		return false;

	FS_CloseFile(f);

	return true;
}

/*
 =================
 FS_ListFilteredFiles

 Returns a list of files and subdirectories that match the given filter.
 The returned list can optionally be sorted.
 =================
*/
char **FS_ListFilteredFiles (const char *filter, qboolean sort, int *numFiles){

	searchPath_t	*searchPath;
	packFile_t		*packFile;
	pack_t			*pack;
	char			**sysFileList;
	int				sysNumFiles;
	char			**fileList;
	char			*files[MAX_LIST_FILES];
	int				fileCount = 0;
	int				i, j;

	// Search through the path, one element at a time
	for (searchPath = fs_searchPaths; searchPath; searchPath = searchPath->next){
		if (fileCount == MAX_LIST_FILES - 1)
			break;

		if (searchPath->pack){
			// Search inside a pack file
			pack = searchPath->pack;

			for (i = 0, packFile = pack->files; i < pack->numFiles; i++, packFile++){
				if (fileCount == MAX_LIST_FILES - 1)
					break;

				// Match filter
				if (!Q_MatchFilter(packFile->name, filter, false))
					continue;

				// Ignore duplicates
				for (j = 0; j < fileCount; j++){
					if (!Q_stricmp(files[j], packFile->name))
						break;
				}

				if (j == fileCount)
					files[fileCount++] = CopyString(packFile->name);
			}
		}
		else {
			// Search in a directory tree
			sysFileList = Sys_ListFilteredFiles(searchPath->directory, filter, false, &sysNumFiles);

			for (i = 0; i < sysNumFiles; i++){
				if (fileCount == MAX_LIST_FILES - 1)
					break;

				// Ignore duplicates
				for (j = 0; j < fileCount; j++){
					if (!Q_stricmp(files[j], sysFileList[i]))
						break;
				}

				if (j == fileCount)
					files[fileCount++] = CopyString(sysFileList[i]);
			}

			Sys_FreeFileList(sysFileList);
		}
	}

	if (!fileCount){
		*numFiles = 0;
		return NULL;
	}

	// Sort the list if needed
	if (sort)
		qsort(files, fileCount, sizeof(char *), Q_SortStrcmp);

	// Copy the list
	fileList = Z_Malloc((fileCount + 1) * sizeof(char *));

	for (i = 0; i < fileCount; i++)
		fileList[i] = files[i];

	fileList[i] = NULL;

	*numFiles = fileCount;

	return fileList;
}

/*
 =================
 FS_ListFiles

 Returns a list of files and subdirectories that match the given
 extension (which must include a leading '.' and must not contain
 wildcards).
 If extension is NULL, all the files will be returned and all the
 subdirectories ignored.
 If extension is "/", all the subdirectories will be returned and all
 the files ignored.
 The returned list can optionally be sorted.
 =================
*/
char **FS_ListFiles (const char *path, const char *extension, qboolean sort, int *numFiles){

	searchPath_t	*searchPath;
	packFile_t		*packFile;
	pack_t			*pack;
	char			name[MAX_OSPATH], dir[MAX_OSPATH], ext[MAX_OSPATH];
	char			**sysFileList;
	int				sysNumFiles;
	char			**fileList;
	char			*files[MAX_LIST_FILES];
	int				fileCount = 0;
	int				i, j;

	// Search through the path, one element at a time
	for (searchPath = fs_searchPaths; searchPath; searchPath = searchPath->next){
		if (fileCount == MAX_LIST_FILES - 1)
			break;

		if (searchPath->pack){
			// Search inside a pack file
			pack = searchPath->pack;

			for (i = 0, packFile = pack->files; i < pack->numFiles; i++, packFile++){
				if (fileCount == MAX_LIST_FILES - 1)
					break;

				// Check the path
				Com_FilePath(packFile->name, dir, sizeof(dir));
				if (Q_stricmp(path, dir))
					continue;

				// Check the extension
				if (packFile->isDirectory){
					if (extension == NULL || Q_stricmp(extension, "/"))
						continue;
				}
				else {
					if (extension){
						Com_FileExtension(packFile->name, ext, sizeof(ext));
						if (Q_stricmp(extension, ext))
							continue;
					}
				}

				// Copy the name
				Com_StripPath(packFile->name, name, sizeof(name));

				// Ignore duplicates
				for (j = 0; j < fileCount; j++){
					if (!Q_stricmp(files[j], name))
						break;
				}

				if (j == fileCount)
					files[fileCount++] = CopyString(name);
			}
		}
		else {
			// Search in a directory tree
			Q_snprintfz(dir, sizeof(dir), "%s/%s", searchPath->directory, path);

			sysFileList = Sys_ListFiles(dir, extension, false, &sysNumFiles);

			for (i = 0; i < sysNumFiles; i++){
				if (fileCount == MAX_LIST_FILES - 1)
					break;

				// Ignore duplicates
				for (j = 0; j < fileCount; j++){
					if (!Q_stricmp(files[j], sysFileList[i]))
						break;
				}

				if (j == fileCount)
					files[fileCount++] = CopyString(sysFileList[i]);
			}

			Sys_FreeFileList(sysFileList);
		}
	}

	if (!fileCount){
		*numFiles = 0;
		return NULL;
	}

	// Sort the list if needed
	if (sort)
		qsort(files, fileCount, sizeof(char *), Q_SortStrcmp);

	// Copy the list
	fileList = Z_Malloc((fileCount + 1) * sizeof(char *));

	for (i = 0; i < fileCount; i++)
		fileList[i] = files[i];

	fileList[i] = NULL;

	*numFiles = fileCount;

	return fileList;
}

/*
 =================
 FS_FreeFileList

 Frees the memory allocated by FS_ListFilteredFiles and FS_ListFiles
 =================
*/
void FS_FreeFileList (char **fileList){

	int		i;

	if (!fileList)
		return;

	for (i = 0; fileList[i]; i++)
		FreeString(fileList[i]);

	Z_Free(fileList);
}

/*
 =================
 FS_ListMods

 Returns a list of game mods with all the data needed by the UI.
 The returned list is always sorted.
 =================
*/
modList_t **FS_ListMods (int *numMods){

	FILE		*f;
	int			len;
	char		description[MAX_OSPATH];
	char		**sysFileList;
	int			sysNumFiles;
	modList_t	**modList;
	modList_t	*mods[MAX_LIST_FILES];
	int			modCount = 0;
	int			i;

	// List all the directories under the current directory
	sysFileList = Sys_ListFiles(fs_homePath->value, "/", true, &sysNumFiles);

	for (i = 0; i < sysNumFiles; i++){
		if (modCount == MAX_LIST_FILES - 1)
			break;

		// Ignore BASE_DIRECTORY
		if (!Q_stricmp(sysFileList[i], BASE_DIRECTORY))
			continue;

		// Try to load a description.txt file, otherwise use the
		// directory name
#ifdef SECURE
		f = fopen_s(f, va("%s/%s/description.txt", fs_homePath->value, sysFileList[i]), "rt");
#else
		f = fopen(va("%s/%s/description.txt", fs_homePath->value, sysFileList[i]), "rt");
#endif
		if (f){
			len = FS_FileLength(f);
			if (len > sizeof(description) - 1)
				len = sizeof(description) - 1;

			fread(description, 1, len, f);
			fclose(f);

			description[len] = 0;
		}
		else
			Q_strncpyz(description, sysFileList[i], sizeof(description));

		// Add it to the list
		mods[modCount] = Z_Malloc(sizeof(modList_t));

		Q_strncpyz(mods[modCount]->directory, sysFileList[i], sizeof(mods[modCount]->directory));
		Q_strncpyz(mods[modCount]->description, description, sizeof(mods[modCount]->description));

		modCount++;
	}

	Sys_FreeFileList(sysFileList);

	if (!modCount){
		*numMods = 0;
		return NULL;
	}

	// Copy the list
	modList = Z_Malloc((modCount + 1) * sizeof(modList_t *));

	for (i = 0; i < modCount; i++)
		modList[i] = mods[i];

	modList[i] = NULL;

	*numMods = modCount;

	return modList;
}

/*
 =================
 FS_FreeModList

 Frees the memory allocated by FS_ListMods
 =================
*/
void FS_FreeModList (modList_t **modList){

	int		i;

	if (!modList)
		return;

	for (i = 0; modList[i]; i++)
		Z_Free(modList[i]);

	Z_Free(modList);
}

/*
 =================
 FS_NextPath

 Allows enumerating all of the directories in the search path
 =================
*/
char *FS_NextPath (char *prevPath){

	searchPath_t	*searchPath;
	char			*prev;

	if (!prevPath)
		return fs_gameDirectory;

	prev = fs_gameDirectory;
	for (searchPath = fs_searchPaths; searchPath; searchPath = searchPath->next){
		if (searchPath->pack)
			continue;

		if (prevPath == prev)
			return searchPath->directory;

		prev = searchPath->directory;
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
static pack_t *FS_LoadPAK (const char *packPath){

	packFile_t		*packFile;
	pack_t			*pack;
	FILE			*handle;
	pakHeader_t		header;
	pakFile_t		info;
	int				numFiles, i;
	unsigned		hash;

#ifdef SECURE
	handle = fopen_s(handle, packPath, "rb");
#else
	handle = fopen(packPath, "rb");
#endif
	if (!handle)
		return NULL;

	fread(&header, 1, sizeof(pakHeader_t), handle);

	if (LittleLong(header.ident) != PAK_IDENT){
		fclose(handle);
		return NULL;
	}

	header.dirOfs = LittleLong(header.dirOfs);
	header.dirLen = LittleLong(header.dirLen);

	numFiles = header.dirLen / sizeof(pakFile_t);
	if (numFiles <= 0){
		fclose(handle);
		return NULL;
	}

	pack = Z_Malloc(sizeof(pack_t));
	packFile = Z_Malloc(numFiles * sizeof(packFile_t));

	Q_strncpyz(pack->name, packPath, sizeof(pack->name));
	pack->pak = handle;
	pack->pk2 = NULL;
	pack->numFiles = numFiles;
	pack->files = packFile;

	// Parse the directory
	for (i = 0; i < FILES_HASH_SIZE; i++)
		pack->filesHashTable[i] = NULL;

	fseek(handle, header.dirOfs, SEEK_SET);

	for (i = 0; i < numFiles; i++){
		fread(&info, 1, sizeof(pakFile_t), handle);

		Q_strncpyz(packFile->name, info.name, sizeof(packFile->name));
		packFile->size = LittleLong(info.fileLen);
		packFile->offset = LittleLong(info.filePos);
		packFile->isDirectory = false;

		// Add to hash table
		hash = Com_HashKey(packFile->name, FILES_HASH_SIZE);

		packFile->nextHash = pack->filesHashTable[hash];
		pack->filesHashTable[hash] = packFile;

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
static pack_t *FS_LoadPK2 (const char *packPath){

	packFile_t		*packFile;
	pack_t			*pack;
	unzFile			*handle;
	unz_global_info	header;
	unz_file_info	info;
	char			name[MAX_OSPATH];
	qboolean		isDirectory;
	int				numFiles, i;
	unsigned		hash;

	handle = unzOpen(packPath);
	if (!handle)
		return NULL;

	if (unzGetGlobalInfo(handle, &header) != UNZ_OK){
		unzClose(handle);
		return NULL;
	}

	numFiles = header.number_entry;
	if (numFiles <= 0){
		unzClose(handle);
		return NULL;
	}

	pack = Z_Malloc(sizeof(pack_t));
	packFile = Z_Malloc(numFiles * sizeof(packFile_t));

	Q_strncpyz(pack->name, packPath, sizeof(pack->name));
	pack->pak = NULL;
	pack->pk2 = handle;
	pack->numFiles = numFiles;
	pack->files = packFile;

	// Parse the directory
	for (i = 0; i < FILES_HASH_SIZE; i++)
		pack->filesHashTable[i] = NULL;

	unzGoToFirstFile(handle);

	for (i = 0; i < numFiles; i++){
		if (unzGetCurrentFileInfo(handle, &info, name, MAX_OSPATH, NULL, 0, NULL, 0) != UNZ_OK)
			break;

		if (name[strlen(name)-1] == '/'){
			name[strlen(name)-1] = 0;

			isDirectory = true;
		}
		else
			isDirectory = false;

		Q_strncpyz(packFile->name, name, sizeof(packFile->name));
		packFile->size = info.uncompressed_size;
		packFile->offset = -1;
		packFile->isDirectory = isDirectory;

		// Add to hash table
		hash = Com_HashKey(packFile->name, FILES_HASH_SIZE);

		packFile->nextHash = pack->filesHashTable[hash];
		pack->filesHashTable[hash] = packFile;

		// Go to next file
		packFile++;

		unzGoToNextFile(handle);
	}

	return pack;
}

/*
 =================
 FS_AddGameDirectory

 Sets fs_gameDirectory, adds the directory to the head of the path, then
 loads and adds all the pack files found (in alphabetical order).

 PK2 files are loaded later so they override PAK files.
 =================
*/
static void FS_AddGameDirectory (const char *directory){

	searchPath_t	*searchPath;
	pack_t			*pack;
	char			**fileList;
	int				numFiles;
	int				i;
	char			name[MAX_OSPATH];

	if (!directory || !directory[0])
		return;

	// Don't add the same directory twice
	for (searchPath = fs_searchPaths; searchPath; searchPath = searchPath->next){
		if (!Q_stricmp(searchPath->directory, directory))
			return;
	}

	// Set the game directory
	Q_strncpyz(fs_gameDirectory, directory, sizeof(fs_gameDirectory));

	// Add the directory to the search path
	searchPath = Z_Malloc(sizeof(searchPath_t));
	Q_strncpyz(searchPath->directory, directory, sizeof(searchPath->directory));
	searchPath->pack = NULL;
	searchPath->next = fs_searchPaths;
	fs_searchPaths = searchPath;

	// Add any PAK files
	fileList = Sys_ListFiles(directory, ".pak", true, &numFiles);

	for (i = 0; i < numFiles; i++){
		Q_snprintfz(name, sizeof(name), "%s/%s", directory, fileList[i]);

		// Load it
		pack = FS_LoadPAK(name);
		if (!pack)
			continue;

		searchPath = Z_Malloc(sizeof(searchPath_t));
		searchPath->pack = pack;
		searchPath->next = fs_searchPaths;
		fs_searchPaths = searchPath;
	}

	Sys_FreeFileList(fileList);

	// Add any PK2 files
	fileList = Sys_ListFiles(directory, ".pk2", true, &numFiles);

	for (i = 0; i < numFiles; i++){
		Q_snprintfz(name, sizeof(name), "%s/%s", directory, fileList[i]);

		// Load it
		pack = FS_LoadPK2(name);
		if (!pack)
			continue;

		searchPath = Z_Malloc(sizeof(searchPath_t));
		searchPath->pack = pack;
		searchPath->next = fs_searchPaths;
		fs_searchPaths = searchPath;
	}

	Sys_FreeFileList(fileList);
}

/*
 =================
 FS_ListFiles_f
 =================
*/
static void FS_ListFiles_f (void){

	char	**fileList;
	int		numFiles;
	int		i;

	if (Cmd_Argc() < 2 || Cmd_Argc() > 3){
		Com_Printf("Usage: listFiles <directory> [extension]\n");
		return;
	}

	if (Cmd_Argc() == 2)
		fileList = FS_ListFiles(Cmd_Argv(1), NULL, true, &numFiles);
	else
		fileList = FS_ListFiles(Cmd_Argv(1), Cmd_Argv(2), true, &numFiles);

	for (i = 0; i < numFiles; i++)
		Com_Printf("%s\n", fileList[i]);

	FS_FreeFileList(fileList);

	Com_Printf("\n");
	Com_Printf("%i files found\n", numFiles);
}

/*
 =================
 FS_ListFilteredFiles_f
 =================
*/
static void FS_ListFilteredFiles_f (void){

	char	**fileList;
	int		numFiles;
	int		i;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: listFilteredFiles <filter>\n");
		return;
	}

	fileList = FS_ListFilteredFiles(Cmd_Argv(1), true, &numFiles);

	for (i = 0; i < numFiles; i++)
		Com_Printf("%s\n", fileList[i]);

	FS_FreeFileList(fileList);

	Com_Printf("\n");
	Com_Printf("%i files found\n", numFiles);
}

/*
 =================
 FS_ListHandles_f
 =================
*/
static void FS_ListHandles_f (void){

	file_t	*file;
	int		i;

	for (i = 0, file = fs_fileHandles; i < MAX_FILE_HANDLES; i++, file++){
		if (!file->active)
			continue;

		Com_Printf("Handle %2i ", i+1);

		switch (file->mode){
		case FS_READ:
			Com_Printf("(R) ");
			break;
		case FS_WRITE:
			Com_Printf("(W) ");
			break;
		case FS_APPEND:
			Com_Printf("(A) ");
			break;
		default:
			Com_Printf("(?) ");
			break;
		}

		Com_Printf(": %s\n", file->name);
	}
}

/*
 =================
 FS_ListPaths_f
 =================
*/
static void FS_ListPaths_f (void){

	searchPath_t	*searchPath;
	int				totalFiles = 0;

	Com_Printf("Current search path:\n");

	for (searchPath = fs_searchPaths; searchPath; searchPath = searchPath->next){
		if (searchPath->pack){
			Com_Printf("%s (%i files)\n", searchPath->pack->name, searchPath->pack->numFiles);

			totalFiles += searchPath->pack->numFiles;
		}
		else
			Com_Printf("%s\n", searchPath->directory);
	}

	Com_Printf("--------------------\n");
	Com_Printf("%i files in PAK/PK2 files\n", totalFiles);
}

/*
 =================
 FS_Restart
 =================
*/
void FS_Restart (const char *game){

	// Check if the current game changed
	if (!Q_stricmp(fs_game->value, game))
		return;

	// Update fs_game
	Cvar_ForceSet("fs_game", game);

	// Restart the file system
	FS_Shutdown();
	FS_Init();
}

/*
 =================
 FS_Init
 =================
*/
void FS_Init (void){

	Com_Printf("------- File System Initialization -------\n");

	// Register our variables and commands
	fs_homePath = Cvar_Get("fs_homePath", Sys_GetCurrentDirectory(), CVAR_INIT, "Current directory");
	fs_cdPath = Cvar_Get("fs_cdPath", Sys_ScanForCD(), CVAR_INIT, "CD directory");
	fs_basePath = Cvar_Get("fs_basePath", va("%s/%s", Sys_GetCurrentDirectory(), BASE_DIRECTORY), CVAR_INIT, "Base directory");
	fs_baseGame = Cvar_Get("fs_baseGame", BASE_DIRECTORY, CVAR_INIT, "Base game directory");
	fs_game = Cvar_Get("fs_game", BASE_DIRECTORY, CVAR_SERVERINFO | CVAR_INIT, "Game directory");
	fs_debug = Cvar_Get("fs_debug", "0", 0, "Debug filesystem operations");

	Cmd_AddCommand("listFiles", FS_ListFiles_f, "List files in a directory");
	Cmd_AddCommand("listFilteredFiles", FS_ListFilteredFiles_f, "List files with a filter");
	Cmd_AddCommand("listHandles", FS_ListHandles_f, "List active file handles");
	Cmd_AddCommand("listPaths", FS_ListPaths_f, "List current search paths");

	// Add the directories
	FS_AddGameDirectory(fs_cdPath->value);
	FS_AddGameDirectory(fs_basePath->value);

	if (strchr(fs_baseGame->value, ':') || strchr(fs_baseGame->value, '/') || strchr(fs_baseGame->value, '\\') || strchr(fs_baseGame->value, '.') || !fs_baseGame->value[0]){
		Com_Printf("Invalid game directory\n");
		Cvar_ForceSet("fs_baseGame", BASE_DIRECTORY);
	}

	FS_AddGameDirectory(va("%s/%s", fs_homePath->value, fs_baseGame->value));

	if (strchr(fs_game->value, ':') || strchr(fs_game->value, '/') || strchr(fs_game->value, '\\') || strchr(fs_game->value, '.') || !fs_game->value[0]){
		Com_Printf("Invalid game directory\n");
		Cvar_ForceSet("fs_game", BASE_DIRECTORY);
	}

	FS_AddGameDirectory(va("%s/%s", fs_homePath->value, fs_game->value));

	FS_ListPaths_f();
	FS_ListHandles_f();

	// Make sure default.cfg exists
	if (!FS_FileExists("default.cfg"))
		Com_Error(ERR_FATAL, "Could not find default.cfg - Check your Quake II Evolved installation");

	// Exec config files
	Cbuf_AddText("exec default.cfg\n");
	Cbuf_AddText("exec q2econfig.cfg\n");
	Cbuf_AddText("exec autoexec.cfg\n");
	Cbuf_Execute();

	Com_Printf("------------------------------------------\n");
}

/*
 =================
 FS_Shutdown
 =================
*/
void FS_Shutdown (void){

	file_t			*file;
	searchPath_t	*next;
	pack_t			*pack;
	int				i;

	Cmd_RemoveCommand("listFiles");
	Cmd_RemoveCommand("listFilteredFiles");
	Cmd_RemoveCommand("listHandles");
	Cmd_RemoveCommand("listPaths");

	// Close all files
	for (i = 0, file = fs_fileHandles; i < MAX_FILE_HANDLES; i++, file++){
		if (!file->active)
			continue;

		if (file->realFile)
			fclose(file->realFile);
		else if (file->zipFile){
			unzCloseCurrentFile(file->zipFile);
			unzClose(file->zipFile);
		}

		memset(file, 0, sizeof(file_t));
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
