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


#include "q_shared.h"


/*
 =======================================================================

 BYTE ORDER FUNCTIONS

 =======================================================================
*/


/*
 =================
 ShortSwap
 =================
*/
short ShortSwap (short s){

	union {
		byte	b[2];
		short	s;
	} in, out;

	in.s = s;

	out.b[0] = in.b[1];
	out.b[1] = in.b[0];

	return out.s;
}

/*
 =================
 LongSwap
 =================
*/
int LongSwap (int l){

	union {
		byte	b[4];
		int		l;
	} in, out;

	in.l = l;

	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];

	return out.l;
}

/*
 =================
 FloatSwap
 =================
*/
float FloatSwap (float f){

	union {
		byte	b[4];
		float	f;
	} in, out;
	
	in.f = f;

	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}


// =====================================================================


/*
 =================
 Com_HashKey

 Returns a hash value for string
 =================
*/
unsigned Com_HashKey (const char *string, unsigned hashSize){

	unsigned	hash = 0;
	int			i;

	for (i = 0; string[i]; i++)
		hash = (hash + i) * 37 + tolower(string[i]);

	return (hash % hashSize);
}


/*
 =======================================================================

 FILE / DIRECTORY PARSING

 =======================================================================
*/


/*
 =================
 Com_StripPath

 Removes the path, if any
 =================
*/
void Com_StripPath (const char *path, char *dst, int dstSize){

	const char	*last;
	
	last = path;
	while (*path){
		if (*path == '/' || *path == '\\')
			last = path+1;
		
		path++;
	}

	Q_strncpyz(dst, last, dstSize);
}

/*
 =================
 Com_StripExtension

 Removes the extension, if any
 =================
*/
void Com_StripExtension (const char *path, char *dst, int dstSize){

	const char	*s, *last;

	s = last = path + strlen(path);
	while (*s != '/' && *s != '\\' && s != path){
		if (*s == '.'){
			last = s;
			break;
		}

		s--;
	}

	Q_strncpyz(dst, path, dstSize);
	if (last-path < dstSize)
		dst[last-path] = 0;
}

/*
 =================
 Com_DefaultPath

 If path doesn't have a / or \\, insert newPath (newPath should not
 include the /)
 =================
*/
void Com_DefaultPath (char *path, int maxSize, const char *newPath){

	char	*s, oldPath[MAX_OSPATH];

	s = path;
	while (*s){
		if (*s == '/' || *s == '\\')
			return;		// It has a path
		
		s++;
	}

	Q_strncpyz(oldPath, path, sizeof(oldPath));
	Q_snprintfz(path, maxSize, "%s/%s", newPath, oldPath);
}

/*
 =================
 Com_DefaultExtension

 If path doesn't have a .EXT, append newExtension (newExtension should
 include the .)
 =================
*/
void Com_DefaultExtension (char *path, int maxSize, const char *newExtension){

	char	*s;

	s = path + strlen(path);
	while (*s != '/' && *s != '\\' && s != path){
		if (*s == '.')
			return;		// It has an extension

		s--;
	}

	Q_strncatz(path, newExtension, maxSize);
}

/*
 =================
 Com_FilePath

 Returns the path up to, but not including the last /
 =================
*/
void Com_FilePath (const char *path, char *dst, int dstSize){

	const char	*s, *last;

	s = last = path + strlen(path);
	while (*s != '/' && *s != '\\' && s != path){
		last = s-1;
		s--;
	}

	Q_strncpyz(dst, path, dstSize);
	if (last-path < dstSize)
		dst[last-path] = 0;
}

/*
 =================
 Com_FileExtension

 Returns the extension, including the .
 =================
*/
void Com_FileExtension (const char *path, char *dst, int dstSize){

	const char	*s, *last;

	s = last = path + strlen(path);
	while (*s != '/' && *s != '\\' && s != path){
		if (*s == '.'){
			last = s;
			break;
		}

		s--;
	}

	Q_strncpyz(dst, last, dstSize);
}


/*
 =======================================================================

 TEXT PARSING

 =======================================================================
*/


/*
 =================
 Com_Parse

 Parses a token out of a string
 =================
*/
char *Com_Parse (char **parseData){

	static char	token[MAX_TOKEN_CHARS];
	char		*data;
	int			c, len = 0;

	data = *parseData;
	token[0] = 0;

	// Make sure incoming data is valid
	if (!data){
		*parseData = NULL;
		return token;
	}

	while (1){
		// Skip whitespace
		while ((c = *data) <= ' '){
			if (!c){
				*parseData = NULL;
				return token;
			}

			data++;
		}
	
		// Skip // comments
		if (c == '/' && data[1] == '/'){
			while (*data && *data != '\n')
				data++;

			continue;
		}

		// Skip /* */ comments
		if (c == '/' && data[1] == '*'){
			data += 2;

			while (*data && (*data != '*' || data[1] != '/'))
				data++;

			if (*data)
				data += 2;

			continue;
		}

		// An actual token
		break;
	}

	// Handle quoted strings specially
	if (c == '\"'){
		data++;

		while (1){
			c = *data++;

			if (c == '\"' || !c){
				if (len == MAX_TOKEN_CHARS)
					len = 0;

				token[len] = 0;

				*parseData = data;
				return token;
			}

			if (len < MAX_TOKEN_CHARS)
				token[len++] = c;
		}
	}

	// Parse a regular word
	while (c > ' '){
		if (len < MAX_TOKEN_CHARS)
			token[len++] = c;

		data++;
		c = *data;
	}

	if (len == MAX_TOKEN_CHARS)
		len = 0;

	token[len] = 0;

	*parseData = data;
	return token;
}


/*
 =======================================================================

 LIBRARY REPLACEMENT FUNCTIONS

 =======================================================================
*/


/*
 =================
 Q_MatchFilterAfterStar

 Like Q_MatchFilter, but match filter against any final segment of text
 =================
*/
static qboolean Q_MatchFilterAfterStar (const char *text, const char *filter, qboolean caseSensitive){

	const char	*t = text;
	const char	*f = filter;
	char		c1, c2;

	while ((c1 = *f++) == '?' || c1 == '*'){
		if (c1 == '?' && *t++ == '\0')
			return false;
	}

	if (c1 == '\0')
		return true;

	if (c1 == '\\')
		c2 = *f;
	else
		c2 = c1;

	while (1){
		if (caseSensitive){
			if (c1 == '[' || *t == c2){
				if (Q_MatchFilter(t, f - 1, caseSensitive))
					return true;
			}
		}
		else {
			if (c1 == '[' || tolower(*t) == tolower(c2)){
				if (Q_MatchFilter(t, f - 1, caseSensitive))
					return true;
			}
		}

		if (*t++ == '\0')
			return false;
	}
}

/*
 =================
 Q_MatchFilter

 Matches the filter against text.
 Returns true if matches, false otherwise.

 A match means the entire text is used up in matching.

 In the filter string, '*' matches any sequence of characters, '?'
 matches any character, '[SET]' matches any character in the specified
 set, '[!SET]' matches any character not in the specified set.

 A set is composed of characters or ranges. A range looks like character
 hyphen character (as in 0-9 or A-Z).
 [0-9a-zA-Z_] is the set of characters allowed in C identifiers.
 Any other character in the filter must be matched exactly.

 To suppress the special syntactic significance of any of '[]*?!-\', and
 match the character exactly, precede it with a '\'.
 =================
*/
qboolean Q_MatchFilter (const char *text, const char *filter, qboolean caseSensitive){

	const char	*t = text;
	const char	*f = filter;
	char		c1, c2, start, end;
	qboolean	invert;

	while ((c1 = *f++) != '\0'){
		switch (c1){
		case '?':
			if (*t == '\0')
				return false;
			else
				++t;

			break;
		case '\\':
			if (caseSensitive){
				if (*f++ != *t++)
					return false;
			}
			else {
				if (tolower(*f++) != tolower(*t++))
					return false;
			}

			break;
		case '*':
			return Q_MatchFilterAfterStar(t, f, caseSensitive);

		case '[':
			c2 = *t++;
			if (!c2)
				return false;

			invert = (*f == '!');
			if (invert)
				f++;

			c1 = *f++;
			while (1){
				start = c1;
				end = c1;

				if (c1 == '\\'){
					start = *f++;
					end = start;
				}
				if (c1 == '\0')
					return false;

				c1 = *f++;
				if (c1 == '-' && *f != ']'){
					end = *f++;
					if (end == '\\')
						end = *f++;
					if (end == '\0')
						return false;

					c1 = *f++;
				}

				if (caseSensitive){
					if (c2 >= start && c2 <= end)
						goto match;
				}
				else {
					if (tolower(c2) >= tolower(start) && tolower(c2) <= tolower(end))
						goto match;
				}

				if (c1 == ']')
					break;
			}

			if (!invert)
				return false;

			break;

match:
			while (c1 != ']'){
				if (c1 == '\0')
					return false;

				c1 = *f++;
				if (c1 == '\0')
					return false;
				else if (c1 == '\\')
					++f;
			}

			if (invert)
				return false;

			break;

		default:
			if (caseSensitive){
				if (c1 != *t++)
					return false;
			}
			else {
				if (tolower(c1) != tolower(*t++))
					return false;
			}

			break;
		}
	}

	return (*t == '\0');
}

/*
 =================
 Q_PrintStrlen
 =================
*/
int Q_PrintStrlen (const char *string){

	int		len = 0;

	while (*string){
		if (Q_IsColorString(string)){
			string += 2;
			continue;
		}

		string++;
		len++;
	}

	return len;
}

/*
 =================
 Q_CleanStr
 =================
*/
char *Q_CleanStr (char *string){

	char	*src, *dst;
	int		c;

	src = string;
	dst = string;

	while ((c = *src) != 0){
		if (Q_IsColorString(src))
			src++;
		else if (c >= 0x20 && c <= 0x7E)
			*dst++ = c;

		src++;
	}
	*dst = 0;

	return string;
}

/*
 =================
 Q_SortStrcmp
 =================
*/
int Q_SortStrcmp (const char **string1, const char **string2){

	return Q_strcmp(*string1, *string2);
}

/*
 =================
 Q_strnicmp
 =================
*/
int Q_strnicmp (const char *string1, const char *string2, int n){

	int		c1, c2;

	if (string1 == NULL){
		if (string2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (string2 == NULL)
		return 1;

	do {
		c1 = *string1++;
		c2 = *string2++;

		if (!n--)
			return 0;	// Strings are equal until end point

		if (c1 != c2){
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');

			if (c1 != c2)
				return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0;			// Strings are equal
}

/*
 =================
 Q_stricmp
 =================
*/
int Q_stricmp (const char *string1, const char *string2){

	int		c1, c2;

	if (string1 == NULL){
		if (string2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (string2 == NULL)
		return 1;

	do {
		c1 = *string1++;
		c2 = *string2++;

		if (c1 != c2){
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');

			if (c1 != c2)
				return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0;			// Strings are equal
}

/*
 =================
 Q_strncmp
 =================
*/
int Q_strncmp (const char *string1, const char *string2, int n){

	int		c1, c2;

	if (string1 == NULL){
		if (string2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (string2 == NULL)
		return 1;

	do {
		c1 = *string1++;
		c2 = *string2++;

		if (!n--)
			return 0;	// Strings are equal until end point

		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
	} while (c1);

	return 0;			// Strings are equal
}

/*
 =================
 Q_strcmp
 =================
*/
int Q_strcmp (const char *string1, const char *string2){

	int		c1, c2;

	if (string1 == NULL){
		if (string2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (string2 == NULL)
		return 1;

	do {
		c1 = *string1++;
		c2 = *string2++;

		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
	} while (c1);

	return 0;			// Strings are equal
}

/*
 =================
 Q_strlwr
 =================
*/
char *Q_strlwr (char *string){

	char	*s = string;

	while (*s){
		*s = tolower(*s);
		s++;
	}

	return string;
}

/*
 =================
 Q_strupr
 =================
*/
char *Q_strupr (char *string){

	char	*s = string;

	while (*s){
		*s = toupper(*s);
		s++;
	}

	return string;
}

/*
 =================
 Q_strncpyz

 Safe strncpy that ensures a trailing zero
 =================
*/
void Q_strncpyz (char *dst, const char *src, int dstSize){

	if (!dst)
		Com_Error(ERR_FATAL, "Q_strncpyz: NULL dst");

	if (!src)
		Com_Error(ERR_FATAL, "Q_strncpyz: NULL src");

	if (dstSize < 1)
		Com_Error(ERR_FATAL, "Q_strncpyz: dstSize < 1");

	while (--dstSize && *src)
		*dst++ = *src++;

	*dst = 0;
}

/*
 =================
 Q_strncatz

 Safe strncat that ensures a trailing zero
 =================
*/
void Q_strncatz (char *dst, const char *src, int dstSize){

	if (!dst)
		Com_Error(ERR_FATAL, "Q_strncatz: NULL dst");

	if (!src)
		Com_Error(ERR_FATAL, "Q_strncatz: NULL src");

	if (dstSize < 1)
		Com_Error(ERR_FATAL, "Q_strncatz: dstSize < 1");

	while (--dstSize && *dst)
		dst++;

	if (dstSize > 0){
		while (--dstSize && *src)
			*dst++ = *src++;

		*dst = 0;
	}
}

/*
 =================
 Q_snprintfz

 Safe snprintf that ensures a trailing zero
 =================
*/
void Q_snprintfz (char *dst, int dstSize, const char *fmt, ...){

	va_list	argPtr;

	if (!dst)
		Com_Error(ERR_FATAL, "Q_snprintfz: NULL dst");

	if (dstSize < 1)
		Com_Error(ERR_FATAL, "Q_snprintfz: dstSize < 1");

	va_start(argPtr, fmt);
	vsnprintf(dst, dstSize, fmt, argPtr);
	va_end(argPtr);

	dst[dstSize-1] = 0;
}

/*
 =================
 va

 Does a varargs printf into a temp buffer, so I don't need to have 
 varargs versions of all text functions
 =================
*/
char *va (const char *fmt, ...){

	static char	string[8][8192];	// In case va is called by nested functions
	static int	index;
	va_list		argPtr;

	index &= 7;

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string[index], sizeof(string[index]), 8192, fmt, argPtr);
#else
	vsnprintf(string[index], sizeof(string[index]), fmt, argPtr);
#endif
	va_end(argPtr);

	string[index][sizeof(string[index])-1] = 0;

	return string[index++];
}


/*
 =======================================================================

 INFO STRINGS

 =======================================================================
*/


/*
 =================
 Info_ValueForKey

 Searches the string for the given key and returns the associated value, 
 or an empty string.
 =================
*/
char *Info_ValueForKey (char *string, char *key){

	static char	value[2][MAX_INFO_VALUE];	// Use two buffers so compares work without stomping on each other
	static int	index;
	char		pkey[MAX_INFO_KEY];
	char		*s;

	index ^= 1;
	if (*string == '\\')
		string++;

	while (1){
		s = pkey;
		while (*string != '\\'){
			if (!*string)
				return "";

			*s++ = *string++;
		}
		*s = 0;

		string++;

		s = value[index];
		while (*string != '\\' && *string){
			if (!*string)
				return "";

			*s++ = *string++;
		}
		*s = 0;

		if (!Q_strcmp(key, pkey))
			return value[index];

		if (!*string)
			return "";

		string++;
	}
}

/*
 =================
 Info_RemoveKey
 =================
*/
void Info_RemoveKey (char *string, char *key){

	char	value[MAX_INFO_VALUE], pkey[MAX_INFO_KEY];
	char	*s, *start;

	if (strchr(key, '\\'))
		return;

	while (1){
		start = string;
		if (*string == '\\')
			string++;

		s = pkey;
		while (*string != '\\'){
			if (!*string)
				return;

			*s++ = *string++;
		}
		*s = 0;

		string++;

		s = value;
		while (*string != '\\' && *string){
			if (!*string)
				return;

			*s++ = *string++;
		}
		*s = 0;

		if (!Q_strcmp(key, pkey)){
#ifdef SECURE
			strcpy_s(start, sizeof(start), string); // Remove this part
#else
			strcpy(start, string);	// Remove this part
#endif
			return;
		}

		if (!*string)
			return;
	}
}

/*
 =================
 Info_Validate

 Some characters are illegal in info strings because they can mess up 
 the server's parsing
 =================
*/
qboolean Info_Validate (char *string){

	if (strchr(string, '\"'))
		return false;
	if (strchr(string, ';'))
		return false;

	return true;
}

/*
 =================
 Info_SetValueForKey
 =================
*/
void Info_SetValueForKey (char *string, char *key, char *value){

	char	newString[MAX_INFO_STRING], *s;
	int		c;

	if (strchr(key, '\\') || strchr(value, '\\')){
		Com_Printf("Can't use keys or values with a \\\n");
		return;
	}

	if (strchr(key, ';') || strchr(value, ';')){
		Com_Printf("Can't use keys or values with a ;\n");
		return;
	}

	if (strchr(key, '\"') || strchr(value, '\"')){
		Com_Printf("Can't use keys or values with a \"\n");
		return;
	}

	if (strlen(key) >= MAX_INFO_KEY){
		Com_Printf("Keys must be < %i characters\n", MAX_INFO_KEY);
		return;
	}
	if (strlen(value) >= MAX_INFO_VALUE){
		Com_Printf("Values must be < %i characters\n", MAX_INFO_VALUE);
		return;
	}

	Info_RemoveKey(string, key);
	if (!value || !value[0])
		return;

	Q_snprintfz(newString, sizeof(newString), "\\%s\\%s", key, value);

	if (strlen(newString) + strlen(string) > MAX_INFO_STRING){
		Com_Printf("Info string length exceeded\n");
		return;
	}

	// Only copy ASCII values
	string += strlen(string);
	s = newString;
	while (*s){
		c = *s++;

		c &= 127;		// Strip high bits
		if (c >= 32 && c < 127)
			*string++ = c;
	}
	*string = 0;
}

/*
 =================
 Info_Print
 =================
*/
void Info_Print (char *string){

	char	key[MAX_INFO_KEY], value[MAX_INFO_VALUE];
	char	*s;
	int		l;

	if (*string == '\\')
		string++;

	while (*string){
		s = key;
		while (*string && *string != '\\')
			*s++ = *string++;

		l = s - key;
		if (l < 20){
			memset(s, ' ', 20-l);
			key[20] = 0;
		}
		else
			*s = 0;

		Com_Printf("%s", key);

		if (!*string){
			Com_Printf("MISSING VALUE\n");
			return;
		}

		string++;

		s = value;
		while (*string && *string != '\\')
			*s++ = *string++;
		*s = 0;

		if (*string)
			string++;

		Com_Printf("%s\n", value);
	}
}
