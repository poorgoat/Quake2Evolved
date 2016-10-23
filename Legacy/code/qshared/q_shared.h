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
	

// q_shared.h -- included first by ALL program modules


#ifndef __Q_SHARED_H__
#define __Q_SHARED_H__


#ifdef MSCVER

#pragma warning (disable : 4005)	// macro redefinition
#pragma warning (disable : 4018)	// signed/usigned mismatch
#pragma warning (disable : 4047)	// 'x' differs in level of indirection from 'x'
#pragma warning (disable : 4113)	// 'x' differs in parameter lists from 'x'
#pragma warning (disable : 4133)	// incompatible types from 'x' to 'x'
#pragma warning (disable : 4142)	// benign redefinition of type
#pragma warning (disable : 4146)	// unary minus operator applied to unsigned type
#pragma warning (disable : 4244)	// conversion from 'x' to x'
#pragma warning (disable : 4305)	// truncation from 'x' to 'x'
#pragma warning (disable : 4700)	// local variable 'x' used without having been initialized
#pragma warning (disable : 4715)	// not all control paths return a value

#endif


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>


// This is the define for determining if we have an ASM version of a C
// function
#if (defined _M_IX86 || defined __i386__) && !defined C_ONLY
#define id386	1
#else
#define id386	0
#endif

#if (defined _M_ALPHA || defined __axp__) && !defined C_ONLY
#define idaxp	1
#else
#define idaxp	0
#endif

#if (defined __ppc__) && !defined C_ONLY
#define idppc	1
#else
#define idpcc	0
#endif

// =====================================================================

#define	Q2E_VERSION		"Q2E 0.40"

// BUILDSTRING will be incorporated into the version string
#if defined _WIN32

#ifdef NDEBUG
#if defined _M_IX86
#define BUILDSTRING		"Win32-x86"
#elif defined _M_ALPHA
#define BUILDSTRING		"Win32-AXP"
#endif
#else
#if defined _M_IX86
#define BUILDSTRING		"Win32-x86 DEBUG"
#elif defined _M_ALPHA
#define BUILDSTRING		"Win32-AXP DEBUG"
#endif
#endif

#elif defined __linux__

#if defined __i386__
#define BUILDSTRING		"Linux-i386"
#elif defined __axp__
#define BUILDSTRING		"Linux-AXP"
#else
#define BUILDSTRING		"Linux-Other"
#endif

#elif defined __MACOS__

#define BUILDSTRING		"MacOS-PPC"

#elif defined MACOS_X

#if defined __ppc__
#define BUILDSTRING		"MacOSX-PPC"
#elif defined __i386__
#define BUILDSTRING		"MacOSX-i386"
#else
#define BUILDSTRING		"MacOSX-Other"
#endif

#endif

// =====================================================================

#ifdef _WIN32

#define Q_INLINE	__inline
#define vsnprintf	_vsnprintf

#else

#define Q_INLINE	inline

#endif

// =====================================================================

typedef unsigned char 		byte;
typedef enum {false, true}	qboolean;

#ifndef NULL
#define NULL ((void *)0)
#endif

// Angle indexes
#define	PITCH				0		// Up / Down
#define	YAW					1		// Left / Right
#define	ROLL				2		// Fall over

#define	MAX_STRING_CHARS	1024	// Max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// Max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// Max length of an individual token

#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024
#define	MAX_INFO_STRING		1024

#define	MAX_QPATH			64		// Max length of a Quake game pathname
#define	MAX_OSPATH			256		// Max length of a filesystem pathname

// Per-level limits
#define	MAX_CLIENTS			256		// Absolute limit
#define	MAX_EDICTS			1024	// Must change protocol to increase more
#define	MAX_LIGHTSTYLES		256
#define	MAX_MODELS			256		// These are sent over the net as bytes
#define	MAX_SOUNDS			256		// so they cannot be blindly increased
#define	MAX_IMAGES			256
#define	MAX_ITEMS			256
#define MAX_GENERAL			(MAX_CLIENTS*2)	// General config strings

// Game print flags
#define	PRINT_LOW			0		// Pickup messages
#define	PRINT_MEDIUM		1		// Death messages
#define	PRINT_HIGH			2		// Critical messages
#define	PRINT_CHAT			3		// Chat messages

#define	ERR_FATAL			0		// Exit the entire game with a popup window
#define	ERR_DROP			1		// Print to console and disconnect from game
#define	ERR_DISCONNECT		2		// Don't kill server

// Destination class for gi.multicast()
typedef enum {
	MULTICAST_ALL,
	MULTICAST_PHS,
	MULTICAST_PVS,
	MULTICAST_ALL_R,
	MULTICAST_PHS_R,
	MULTICAST_PVS_R
} multicast_t;

/*
 =======================================================================

 MATHLIB

 =======================================================================
*/

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef float mat4_t[16];

typedef byte color_t[4];

#define NUM_VERTEX_NORMALS		162
extern vec3_t	byteDirs[NUM_VERTEX_NORMALS];

extern color_t	colorBlack;
extern color_t	colorRed;
extern color_t	colorGreen;
extern color_t	colorYellow;
extern color_t	colorBlue;
extern color_t	colorCyan;
extern color_t	colorMagenta;
extern color_t	colorWhite;

extern color_t	colorTable[32];

extern vec3_t	vec3_origin;
extern vec3_t	axisDefault[3];
extern mat4_t	matrixIdentity;

#ifndef M_PI
#define M_PI					3.14159265358979323846	// Matches value in GCC v2 math.h
#endif

#ifndef M_PI2
#define M_PI2					6.28318530717958647692	// Matches value in GCC v2 math.h
#endif

#define DEG2RAD(a)				(((a) * M_PI) / 180.0F)
#define RAD2DEG(a)				(((a) * 180.0F) / M_PI)

#define SqrtFast(x)				((x) * Q_rsqrt(x))

#define	frand()					((rand() & 0x7FFF) * (1.0/0x7FFF))		// 0 to 1
#define crand()					((rand() & 0x7FFF) * (2.0/0x7FFF) - 1)	// -1 to 1

#ifndef min
#define min(a,b)				(((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)				(((a) > (b)) ? (a) : (b))
#endif

#define Clamp(a,b,c)			(((a) < (b)) ? (b) : ((a) > (c)) ? (c) : (a))

#define MakeRGBA(c,r,g,b,a)		((c)[0]=(r),(c)[1]=(g),(c)[2]=(b),(c)[3]=(a))

#if 1

#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define CrossProduct(x,y,o)		((o)[0]=(x)[1]*(y)[2]-(x)[2]*(y)[1],(o)[1]=(x)[2]*(y)[0]-(x)[0]*(y)[2],(o)[2]=(x)[0]*(y)[1]-(x)[1]*(y)[0])
#define Distance(x,y)			(sqrt((((x)[0]-(y)[0])*((x)[0]-(y)[0])+((x)[1]-(y)[1])*((x)[1]-(y)[1])+((x)[2]-(y)[2])*((x)[2]-(y)[2]))))
#define DistanceSquared(x,y)	(((x)[0]-(y)[0])*((x)[0]-(y)[0])+((x)[1]-(y)[1])*((x)[1]-(y)[1])+((x)[2]-(y)[2])*((x)[2]-(y)[2]))
#define	SnapVector(v)			((v)[0]=((int)((v)[0])),(v)[1]=((int)((v)[1])),(v)[2]=((int)((v)[2])))
#define VectorCopy(i,o)			((o)[0]=(i)[0],(o)[1]=(i)[1],(o)[2]=(i)[2])
#define VectorSet(v,x,y,z)		((v)[0]=(x),(v)[1]=(y),(v)[2]=(z))
#define VectorClear(v)			((v)[0]=(v)[1]=(v)[2]=0)
#define VectorCompare(a,b)		((a)[0]==(b)[0]&&(a)[1]==(b)[1]&&(a)[2]==(b)[2])
#define VectorAdd(a,b,o)		((o)[0]=(a)[0]+(b)[0],(o)[1]=(a)[1]+(b)[1],(o)[2]=(a)[2]+(b)[2])
#define VectorSubtract(a,b,o)	((o)[0]=(a)[0]-(b)[0],(o)[1]=(a)[1]-(b)[1],(o)[2]=(a)[2]-(b)[2])
#define	VectorScale(v,s,o)		((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define VectorMultiply(a,b,o)	((o)[0]=(a)[0]*(b)[0],(o)[1]=(a)[1]*(b)[1],(o)[2]=(a)[2]*(b)[2])
#define	VectorMA(a,s,b,o)		((o)[0]=(a)[0]+(b)[0]*(s),(o)[1]=(a)[1]+(b)[1]*(s),(o)[2]=(a)[2]+(b)[2]*(s))
#define VectorAverage(a,b,o)	((o)[0]=((a)[0]+(b)[0])*0.5,(o)[1]=((a)[1]+(b)[1])*0.5,(o)[2]=((a)[2]+(b)[2])*0.5)
#define VectorNegate(v,o)		((o)[0]=-(v)[0],(o)[1]=-(v)[1],(o)[2]=-(v)[2])
#define VectorInverse(v)		((v)[0]=-(v)[0],(v)[1]=-(v)[1],(v)[2]=-(v)[2])
#define VectorLength(v)			(sqrt((v)[0]*(v)[0]+(v)[1]*(v)[1]+(v)[2]*(v)[2]))
#define VectorLengthSquared(v)	((v)[0]*(v)[0]+(v)[1]*(v)[1]+(v)[2]*(v)[2])

#else

#define DotProduct(x,y)			_DotProduct(x, y)
#define CrossProduct(x,y,o)		_CrossProduct(x, y, o)
#define Distance(x,y)			_Distance(x, y)
#define DistanceSquared(x,y)	_DistanceSquared(x, y)
#define	SnapVector(v)			_SnapVector(v)
#define VectorCopy(i,o)			_VectorCopy(i, o)
#define VectorSet(v,x,y,z)		_VectorSet(v, x, y, z)
#define VectorClear(v)			_VectorClear(v)
#define VectorCompare(a,b)		_VectorCompare(a, b)
#define VectorAdd(a,b,o)		_VectorAdd(a, b, o)
#define VectorSubtract(a,b,o)	_VectorSubtract(a, b, o)
#define	VectorScale(v,s,o)		_VectorScale(v, s, o)
#define VectorMultiply(a,b,o)	_VectorMultiply(a, b, o)
#define	VectorMA(a,s,b,o)		_VectorMA(a, s, b, o)
#define VectorAverage(a,b,o)	_VectorAverage(a, b, o)
#define VectorNegate(v,o)		_VectorNegate(v, o)
#define VectorInverse(v)		_VectorInverse(v)
#define VectorLength(v)			_VectorLength(v)
#define VectorLengthSquared(v)	_VectorLengthSquared(v)

#endif

// Just in case you don't want to use the macros
vec_t		_DotProduct (const vec3_t v1, const vec3_t v2);
void		_CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross);
vec_t		_Distance (const vec3_t v1, const vec3_t v2);
vec_t		_DistanceSquared (const vec3_t v1, const vec3_t v2);
void		_SnapVector (vec3_t v);
void		_VectorCopy (const vec3_t in, vec3_t out);
void		_VectorSet (vec3_t v, float x, float y, float z);
void		_VectorClear (vec3_t v);
qboolean	_VectorCompare (const vec3_t v1, const vec3_t v2);
void		_VectorAdd (const vec3_t v1, const vec3_t v2, vec3_t out);
void		_VectorSubtract (const vec3_t v1, const vec3_t v2, vec3_t out);
void		_VectorScale (const vec3_t v, float scale, vec3_t out);
void		_VectorMultiply (const vec3_t v1, const vec3_t v2, vec3_t out);
void		_VectorMA (const vec3_t v1, float scale, const vec3_t v2, vec3_t out);
void		_VectorAverage (const vec3_t v1, const vec3_t v2, vec3_t out);
void		_VectorNegate (const vec3_t v, vec3_t out);
void		_VectorInverse (vec3_t v);
vec_t		_VectorLength (const vec3_t v);
vec_t		_VectorLengthSquared (const vec3_t v);

void		VectorRotate (const vec3_t v, const vec3_t matrix[3], vec3_t out);
void		VectorReflect (const vec3_t v, const vec3_t normal, vec3_t out);
vec_t		VectorNormalize (vec3_t v);
vec_t		VectorNormalize2 (const vec3_t v, vec3_t out);
void		VectorNormalizeFast (vec3_t v);

void		Matrix4_Copy (const mat4_t in, mat4_t out);
qboolean	Matrix4_Compare (const mat4_t m1, const mat4_t m2);
void		Matrix4_Transpose (const mat4_t m, mat4_t out);
void		Matrix4_Multiply (const mat4_t m1, const mat4_t m2, mat4_t out);
void		Matrix4_MultiplyFast (const mat4_t m1, const mat4_t m2, mat4_t out);
void		Matrix4_Identity (mat4_t m);
void		Matrix4_Rotate (mat4_t m, float angle, float x, float y, float z);
void		Matrix4_Scale (mat4_t m, float x, float y, float z);
void		Matrix4_Translate (mat4_t m, float x, float y, float z);

int			DirToByte (const vec3_t dir);
void		ByteToDir (int b, vec3_t dir);

unsigned	ColorBytes (float r, float g, float b, float a);
unsigned	ColorNormalize (vec3_t rgb);

float		Q_rsqrt (float number);
int			Q_log2 (int val);

void		RotatePointAroundVector (vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);
void		NormalToLatLong (const vec3_t normal, byte bytes[2]);
void		VectorToAngles (const vec3_t vec, vec3_t angles);
void		AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void		AnglesToAxis (const vec3_t angles, vec3_t axis[3]);
void		AxisClear (vec3_t axis[3]);
void		AxisCopy (const vec3_t in[3], vec3_t out[3]);
qboolean	AxisCompare (const vec3_t axis1[3], const vec3_t axis2[3]);
float		AngleMod (float angle);
float		LerpAngle (float from, float to, float frac);
void		ProjectPointOnPlane (vec3_t dst, const vec3_t p, const vec3_t normal);
void		PerpendicularVector (vec3_t dst, const vec3_t src);
void		MakeNormalVectors (const vec3_t forward, vec3_t right, vec3_t up);
void		MatrixMultiply (const vec3_t in1[3], const vec3_t in2[3], vec3_t out[3]);
void		ClearBounds (vec3_t mins, vec3_t maxs);
void		AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs);
float		RadiusFromBounds (const vec3_t mins, const vec3_t maxs);
qboolean	BoundsIntersect (const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);
qboolean	BoundsAndSphereIntersect (const vec3_t mins, const vec3_t maxs, const vec3_t origin, float radius);
qboolean	PlaneFromPoints (struct cplane_s *plane, const vec3_t a, const vec3_t b, const vec3_t c);
void		SetPlaneSignbits (struct cplane_s *plane);
int			PlaneTypeForNormal (const vec3_t normal);
int			BoxOnPlaneSide (const vec3_t mins, const vec3_t maxs, struct cplane_s *p);

// =====================================================================

#define COLOR_BLACK			0
#define COLOR_RED			1
#define COLOR_GREEN			2
#define COLOR_YELLOW		3
#define COLOR_BLUE			4
#define COLOR_CYAN			5
#define COLOR_MAGENTA		6
#define COLOR_WHITE			7

#define S_COLOR_BLACK		"^0"
#define S_COLOR_RED			"^1"
#define	S_COLOR_GREEN		"^2"
#define S_COLOR_YELLOW		"^3"
#define S_COLOR_BLUE		"^4"
#define S_COLOR_CYAN		"^5"
#define S_COLOR_MAGENTA		"^6"
#define S_COLOR_WHITE		"^7"

#define Q_COLOR_ESCAPE		'^'
#define Q_COLOR_MASK		((sizeof(colorTable) / sizeof(color_t)) - 1)

#define Q_IsColorString(p)	(p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE)
#define Q_ColorIndex(c)		(((c) - '0') & Q_COLOR_MASK)

// =====================================================================

short	ShortSwap (short s);
int		LongSwap (int l);
float	FloatSwap (float f);

#if (defined _WIN32 || defined __linux__)

#define LittleShort
#define LittleLong
#define LittleFloat

static Q_INLINE short	BigShort (short s){ return ShortSwap(s); }
static Q_INLINE int		BigLong (int l){ return LongSwap(l); }
static Q_INLINE float	BigFloat (float f){ return FloatSwap(f); }

#elif (defined __MACOS__ || defined MACOS_X)

#define BigShort
#define BigLong
#define BigFloat

static Q_INLINE short	LittleShort (short s){ return ShortSwap(s); }
static Q_INLINE int		LittleLong (int l){ return LongSwap(l); }
static Q_INLINE float	LittleFloat (float f){ return FloatSwap(f); }

#endif

// =====================================================================

// Returns a hash key for string
unsigned	Com_HashKey (const char *string, unsigned hashSize);

// Directory parsing functions
const char	*Com_SkipPath (const char *path);
void		Com_StripExtension (const char *path, char *dst, int dstSize);
void		Com_DefaultPath (char *path, int maxSize, const char *newPath);
void		Com_DefaultExtension (char *path, int maxSize, const char *newExtension);
void		Com_FilePath (const char *path, char *dst, int dstSize);
void		Com_FileExtension (const char *path, char *dst, int dstSize);

// Text parsing functions
void		Com_BeginParseSession (const char *parseName);
void		Com_BackupParseSession (char **parseData);
void		Com_RestoreParseSession (char **parseData);
int			Com_GetCurrentParseLine (void);
void		Com_SetCurrentParseLine (int parseLine);
void		Com_ParseError (const char *fmt, ...);
void		Com_ParseWarning (const char *fmt, ...);
char		*Com_SkipWhiteSpace (char *parseData, qboolean *hasNewLines);
void		Com_SkipBracedSection (char **parseData, int depth);
void		Com_SkipRestOfLine (char **parseData);
char		*Com_Parse (char **parseData);
char		*Com_ParseExt (char **parseData, qboolean allowNewLines);

// Matches the pattern against text
qboolean	Q_GlobMatch (const char *pattern, const char *text, qboolean caseSensitive);

// String length that discounts Quake color sequences
int			Q_PrintStrlen (const char *string);

// Removes color sequences from string
char		*Q_CleanStr (char *string);

// String compare used for qsort calls
int			Q_SortStrcmp (const char **arg1, const char **arg2);

// Portable string compare
int			Q_strnicmp (const char *string1, const char *string2, int n);
int			Q_stricmp (const char *string1, const char *string2);
int			Q_strncmp (const char *string1, const char *string2, int n);
int			Q_strcmp (const char *string1, const char *strgin2);

// Portable string lower/upper
char		*Q_strlwr (char *string);
char		*Q_strupr (char *string);

// Buffer size safe library replacements
void		Q_strncpyz (char *dst, const char *src, int dstSize);
void		Q_strncatz (char *dst, const char *src, int dstSize);
void		Q_snprintfz (char *dst, int dstSize, const char *fmt, ...);

char		*va (const char *fmt, ...);

// Info strings
char		*Info_ValueForKey (char *string, char *key);
void		Info_RemoveKey (char *string, char *key);
qboolean	Info_Validate (char *string);
void		Info_SetValueForKey (char *string, char *key, char *value);
void		Info_Print (char *string);

// =====================================================================

// This is only here so the functions in q_shared.c can link
void		Com_Error (int code, const char *fmt, ...);
void		Com_Printf (const char *fmt, ...);

/*
 =======================================================================

 CVARS

 =======================================================================
*/

#ifndef CVAR
#define	CVAR

#define	CVAR_ARCHIVE		1		// Set to cause it to be saved to config file
#define	CVAR_USERINFO		2		// Added to user info when changed
#define	CVAR_SERVERINFO		4		// Added to server info when changed
#define	CVAR_INIT			8		// Don't allow change from console at all, but can be set from the command line
#define	CVAR_LATCH			16		// Save changes until restart
#define CVAR_ROM			32		// Cannot be set by user at all
#define CVAR_CHEAT			64		// Cannot be set when cheats are disabled
#define CVAR_GAMEVAR		128		// Stupid hack to workaround our changes to cvar_t

// Nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
	char			*name;
	char			*string;
	char			*resetString;	// cvar_restart and reset will set this value
	char			*latchedString;	// For CVAR_LATCH cvars
	int				flags;
	qboolean		modified;		// Set each time the cvar is changed
	float			value;
	int				integer;
	struct cvar_s	*next;
	struct cvar_s	*nextHash;
} cvar_t;

// Stupid hack to workaround our changes to cvar_t
// Used only by the game library
typedef struct gamecvar_s {
	char			*name;
	char			*string;
	char			*latched_string;
	int				flags;
	qboolean		modified;
	float			value;
	struct gamecvar_s *next;
} gamecvar_t;

#endif

/*
 =======================================================================

 COLLISION DETECTION

 =======================================================================
*/

#include "surfaceflags.h"

// Content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)

// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2

// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2
#define PLANE_NON_AXIAL	3

typedef struct cplane_s {
	vec3_t	normal;
	float	dist;
	byte	type;				// For fast side tests
	byte	signbits;			// signx + (signy<<1) + (signz<<1)
	byte	pad[2];
} cplane_t;

typedef struct csurface_s {
	char	name[16];
	int		flags;
	int		value;
} csurface_t;

// A trace is returned when a box is swept through the world
typedef struct {
	qboolean		allsolid;	// If true, plane is not valid
	qboolean		startsolid;	// If true, the initial point was in a solid area
	float			fraction;	// Time completed, 1.0 = didn't hit anything
	vec3_t			endpos;		// Final position
	cplane_t		plane;		// Surface plane at impact
	csurface_t		*surface;	// Surface hit
	int				contents;	// Contents on other side of surface hit
	struct edict_s	*ent;		// Not set by CM_*() functions
} trace_t;

// pmove_state_t is the information necessary for client side movement
// prediction
typedef enum {
	// Can accelerate and turn
	PM_NORMAL,
	PM_SPECTATOR,
	// No acceleration or turning
	PM_DEAD,
	PM_GIB,		// Different bounding box
	PM_FREEZE
} pmtype_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_ON_GROUND		4
#define	PMF_TIME_WATERJUMP	8	// pm_time is waterjump
#define	PMF_TIME_LAND		16	// pm_time is time before rejump
#define	PMF_TIME_TELEPORT	32	// pm_time is non-moving time
#define PMF_NO_PREDICTION	64	// Temporarily disables prediction (used for grappling hook)

// This structure needs to be communicated bit-accurate from the server 
// to the client to guarantee that prediction stays in sync, so no 
// floats are used.
// If any part of the game code modifies this struct, it will result in 
// a prediction error of some degree.
typedef struct {
	pmtype_t	pm_type;

	short		origin[3];		// 12.3
	short		velocity[3];	// 12.3
	byte		pm_flags;		// Ducked, jump_held, etc...
	byte		pm_time;		// Each unit = 8 ms
	short		gravity;
	short		delta_angles[3];	// Add to command angles to get view direction
									// Changed by spawns, rotating objects, and teleporters
} pmove_state_t;

// Button bits
#define	BUTTON_ATTACK		1
#define	BUTTON_USE			2
#define	BUTTON_ANY			128			// Any key whatsoever

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
	byte	msec;
	byte	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	byte	impulse;		// Remove?
	byte	lightlevel;		// Light level the player is standing on
} usercmd_t;

#define	MAXTOUCH	32
typedef struct {
	// State (in / out)
	pmove_state_t	s;

	// Command (in)
	usercmd_t		cmd;
	qboolean		snapinitial;	// If s has been changed outside pmove

	// Results (out)
	int				numtouch;
	struct edict_s	*touchents[MAXTOUCH];

	vec3_t			viewangles;		// Clamped
	float			viewheight;

	vec3_t			mins, maxs;		// Bounding box size

	struct edict_s	*groundentity;
	int				watertype;
	int				waterlevel;

	// Callbacks to test the world
	trace_t			(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int				(*pointcontents) (vec3_t point);
} pmove_t;

// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, 
// frame animations) that happen constantly on the given entity.
// An entity that has effects will be sent to the client even if it has 
// a zero index model.
#define	EF_ROTATE			0x00000001	// Rotate (bonus items)
#define	EF_GIB				0x00000002	// Leave a trail
#define	EF_BLASTER			0x00000008	// Redlight + trail
#define	EF_ROCKET			0x00000010	// Redlight + trail
#define	EF_GRENADE			0x00000020
#define	EF_HYPERBLASTER		0x00000040
#define	EF_BFG				0x00000080
#define EF_COLOR_SHELL		0x00000100
#define EF_POWERSCREEN		0x00000200
#define	EF_ANIM01			0x00000400	// Automatically cycle between frames 0 and 1 at 2 hz
#define	EF_ANIM23			0x00000800	// Automatically cycle between frames 2 and 3 at 2 hz
#define EF_ANIM_ALL			0x00001000	// Automatically cycle through all frames at 2hz
#define EF_ANIM_ALLFAST		0x00002000	// Automatically cycle through all frames at 10hz
#define	EF_FLIES			0x00004000
#define	EF_QUAD				0x00008000
#define	EF_PENT				0x00010000
#define	EF_TELEPORTER		0x00020000	// Particle fountain
#define EF_FLAG1			0x00040000
#define EF_FLAG2			0x00080000
#define EF_IONRIPPER		0x00100000
#define EF_GREENGIB			0x00200000
#define	EF_BLUEHYPERBLASTER 0x00400000
#define EF_SPINNINGLIGHTS	0x00800000
#define EF_PLASMA			0x01000000
#define EF_TRAP				0x02000000
#define EF_TRACKER			0x04000000
#define	EF_DOUBLE			0x08000000
#define	EF_SPHERETRANS		0x10000000
#define EF_TAGTRAIL			0x20000000
#define EF_HALF_DAMAGE		0x40000000
#define EF_TRACKERTRAIL		0x80000000

// entity_state_t->renderfx flags
#define	RF_MINLIGHT			0x00000001	// Always have some light (viewmodel)
#define	RF_VIEWERMODEL		0x00000002	// Don't draw through eyes, only mirrors
#define	RF_WEAPONMODEL		0x00000004	// Only draw through eyes
#define	RF_DEPTHHACK		0x00000010	// For view weapon Z crunching
#define	RF_TRANSLUCENT		0x00000020
#define	RF_FRAMELERP		0x00000040
#define RF_BEAM				0x00000080
#define	RF_CUSTOMSKIN		0x00000100	// Skin is an index in image_precache
#define	RF_GLOW				0x00000200	// Pulse lighting for bonus items
#define RF_SHELL_RED		0x00000400
#define	RF_SHELL_GREEN		0x00000800
#define RF_SHELL_BLUE		0x00001000
#define RF_IR_VISIBLE		0x00008000
#define	RF_SHELL_DOUBLE		0x00010000
#define	RF_SHELL_HALF_DAM	0x00020000
#define RF_USE_DISGUISE		0x00040000
#define RF_CULLHACK			0x00080000	// For left-handed weapons

// player_state_t->refdef flags
#define	RDF_UNDERWATER		1			// Warp the screen as appropriate
#define RDF_NOWORLDMODEL	2			// Used for player configuration screen
#define	RDF_IRGOGGLES		4			// Infrared goggles

// Muzzle flashes / player effects
#define	MZ_BLASTER			0
#define MZ_MACHINEGUN		1
#define	MZ_SHOTGUN			2
#define	MZ_CHAINGUN1		3
#define	MZ_CHAINGUN2		4
#define	MZ_CHAINGUN3		5
#define	MZ_RAILGUN			6
#define	MZ_ROCKET			7
#define	MZ_GRENADE			8
#define	MZ_LOGIN			9
#define	MZ_LOGOUT			10
#define	MZ_RESPAWN			11
#define	MZ_BFG				12
#define	MZ_SSHOTGUN			13
#define	MZ_HYPERBLASTER		14
#define	MZ_ITEMRESPAWN		15
#define MZ_IONRIPPER		16
#define MZ_BLUEHYPERBLASTER 17
#define MZ_PHALANX			18
#define MZ_SILENCED			128		// Bit flag ORed with one of the above numbers
#define MZ_ETF_RIFLE		30
#define MZ_UNUSED			31
#define MZ_SHOTGUN2			32
#define MZ_HEATBEAM			33
#define MZ_BLASTER2			34
#define	MZ_TRACKER			35
#define	MZ_NUKE1			36
#define	MZ_NUKE2			37
#define	MZ_NUKE4			38
#define	MZ_NUKE8			39

// Monster muzzle flashes
#define MZ2_TANK_BLASTER_1				1
#define MZ2_TANK_BLASTER_2				2
#define MZ2_TANK_BLASTER_3				3
#define MZ2_TANK_MACHINEGUN_1			4
#define MZ2_TANK_MACHINEGUN_2			5
#define MZ2_TANK_MACHINEGUN_3			6
#define MZ2_TANK_MACHINEGUN_4			7
#define MZ2_TANK_MACHINEGUN_5			8
#define MZ2_TANK_MACHINEGUN_6			9
#define MZ2_TANK_MACHINEGUN_7			10
#define MZ2_TANK_MACHINEGUN_8			11
#define MZ2_TANK_MACHINEGUN_9			12
#define MZ2_TANK_MACHINEGUN_10			13
#define MZ2_TANK_MACHINEGUN_11			14
#define MZ2_TANK_MACHINEGUN_12			15
#define MZ2_TANK_MACHINEGUN_13			16
#define MZ2_TANK_MACHINEGUN_14			17
#define MZ2_TANK_MACHINEGUN_15			18
#define MZ2_TANK_MACHINEGUN_16			19
#define MZ2_TANK_MACHINEGUN_17			20
#define MZ2_TANK_MACHINEGUN_18			21
#define MZ2_TANK_MACHINEGUN_19			22
#define MZ2_TANK_ROCKET_1				23
#define MZ2_TANK_ROCKET_2				24
#define MZ2_TANK_ROCKET_3				25

#define MZ2_INFANTRY_MACHINEGUN_1		26
#define MZ2_INFANTRY_MACHINEGUN_2		27
#define MZ2_INFANTRY_MACHINEGUN_3		28
#define MZ2_INFANTRY_MACHINEGUN_4		29
#define MZ2_INFANTRY_MACHINEGUN_5		30
#define MZ2_INFANTRY_MACHINEGUN_6		31
#define MZ2_INFANTRY_MACHINEGUN_7		32
#define MZ2_INFANTRY_MACHINEGUN_8		33
#define MZ2_INFANTRY_MACHINEGUN_9		34
#define MZ2_INFANTRY_MACHINEGUN_10		35
#define MZ2_INFANTRY_MACHINEGUN_11		36
#define MZ2_INFANTRY_MACHINEGUN_12		37
#define MZ2_INFANTRY_MACHINEGUN_13		38

#define MZ2_SOLDIER_BLASTER_1			39
#define MZ2_SOLDIER_BLASTER_2			40
#define MZ2_SOLDIER_SHOTGUN_1			41
#define MZ2_SOLDIER_SHOTGUN_2			42
#define MZ2_SOLDIER_MACHINEGUN_1		43
#define MZ2_SOLDIER_MACHINEGUN_2		44

#define MZ2_GUNNER_MACHINEGUN_1			45
#define MZ2_GUNNER_MACHINEGUN_2			46
#define MZ2_GUNNER_MACHINEGUN_3			47
#define MZ2_GUNNER_MACHINEGUN_4			48
#define MZ2_GUNNER_MACHINEGUN_5			49
#define MZ2_GUNNER_MACHINEGUN_6			50
#define MZ2_GUNNER_MACHINEGUN_7			51
#define MZ2_GUNNER_MACHINEGUN_8			52
#define MZ2_GUNNER_GRENADE_1			53
#define MZ2_GUNNER_GRENADE_2			54
#define MZ2_GUNNER_GRENADE_3			55
#define MZ2_GUNNER_GRENADE_4			56

#define MZ2_CHICK_ROCKET_1				57

#define MZ2_FLYER_BLASTER_1				58
#define MZ2_FLYER_BLASTER_2				59

#define MZ2_MEDIC_BLASTER_1				60

#define MZ2_GLADIATOR_RAILGUN_1			61

#define MZ2_HOVER_BLASTER_1				62

#define MZ2_ACTOR_MACHINEGUN_1			63

#define MZ2_SUPERTANK_MACHINEGUN_1		64
#define MZ2_SUPERTANK_MACHINEGUN_2		65
#define MZ2_SUPERTANK_MACHINEGUN_3		66
#define MZ2_SUPERTANK_MACHINEGUN_4		67
#define MZ2_SUPERTANK_MACHINEGUN_5		68
#define MZ2_SUPERTANK_MACHINEGUN_6		69
#define MZ2_SUPERTANK_ROCKET_1			70
#define MZ2_SUPERTANK_ROCKET_2			71
#define MZ2_SUPERTANK_ROCKET_3			72

#define MZ2_BOSS2_MACHINEGUN_L1			73
#define MZ2_BOSS2_MACHINEGUN_L2			74
#define MZ2_BOSS2_MACHINEGUN_L3			75
#define MZ2_BOSS2_MACHINEGUN_L4			76
#define MZ2_BOSS2_MACHINEGUN_L5			77
#define MZ2_BOSS2_ROCKET_1				78
#define MZ2_BOSS2_ROCKET_2				79
#define MZ2_BOSS2_ROCKET_3				80
#define MZ2_BOSS2_ROCKET_4				81

#define MZ2_FLOAT_BLASTER_1				82

#define MZ2_SOLDIER_BLASTER_3			83
#define MZ2_SOLDIER_SHOTGUN_3			84
#define MZ2_SOLDIER_MACHINEGUN_3		85
#define MZ2_SOLDIER_BLASTER_4			86
#define MZ2_SOLDIER_SHOTGUN_4			87
#define MZ2_SOLDIER_MACHINEGUN_4		88
#define MZ2_SOLDIER_BLASTER_5			89
#define MZ2_SOLDIER_SHOTGUN_5			90
#define MZ2_SOLDIER_MACHINEGUN_5		91
#define MZ2_SOLDIER_BLASTER_6			92
#define MZ2_SOLDIER_SHOTGUN_6			93
#define MZ2_SOLDIER_MACHINEGUN_6		94
#define MZ2_SOLDIER_BLASTER_7			95
#define MZ2_SOLDIER_SHOTGUN_7			96
#define MZ2_SOLDIER_MACHINEGUN_7		97
#define MZ2_SOLDIER_BLASTER_8			98
#define MZ2_SOLDIER_SHOTGUN_8			99
#define MZ2_SOLDIER_MACHINEGUN_8		100

#define	MZ2_MAKRON_BFG					101
#define MZ2_MAKRON_BLASTER_1			102
#define MZ2_MAKRON_BLASTER_2			103
#define MZ2_MAKRON_BLASTER_3			104
#define MZ2_MAKRON_BLASTER_4			105
#define MZ2_MAKRON_BLASTER_5			106
#define MZ2_MAKRON_BLASTER_6			107
#define MZ2_MAKRON_BLASTER_7			108
#define MZ2_MAKRON_BLASTER_8			109
#define MZ2_MAKRON_BLASTER_9			110
#define MZ2_MAKRON_BLASTER_10			111
#define MZ2_MAKRON_BLASTER_11			112
#define MZ2_MAKRON_BLASTER_12			113
#define MZ2_MAKRON_BLASTER_13			114
#define MZ2_MAKRON_BLASTER_14			115
#define MZ2_MAKRON_BLASTER_15			116
#define MZ2_MAKRON_BLASTER_16			117
#define MZ2_MAKRON_BLASTER_17			118
#define MZ2_MAKRON_RAILGUN_1			119
#define	MZ2_JORG_MACHINEGUN_L1			120
#define	MZ2_JORG_MACHINEGUN_L2			121
#define	MZ2_JORG_MACHINEGUN_L3			122
#define	MZ2_JORG_MACHINEGUN_L4			123
#define	MZ2_JORG_MACHINEGUN_L5			124
#define	MZ2_JORG_MACHINEGUN_L6			125
#define	MZ2_JORG_MACHINEGUN_R1			126
#define	MZ2_JORG_MACHINEGUN_R2			127
#define	MZ2_JORG_MACHINEGUN_R3			128
#define	MZ2_JORG_MACHINEGUN_R4			129
#define MZ2_JORG_MACHINEGUN_R5			130
#define	MZ2_JORG_MACHINEGUN_R6			131
#define MZ2_JORG_BFG_1					132
#define MZ2_BOSS2_MACHINEGUN_R1			133
#define MZ2_BOSS2_MACHINEGUN_R2			134
#define MZ2_BOSS2_MACHINEGUN_R3			135
#define MZ2_BOSS2_MACHINEGUN_R4			136
#define MZ2_BOSS2_MACHINEGUN_R5			137

#define	MZ2_CARRIER_MACHINEGUN_L1		138
#define	MZ2_CARRIER_MACHINEGUN_R1		139
#define	MZ2_CARRIER_GRENADE				140
#define MZ2_TURRET_MACHINEGUN			141
#define MZ2_TURRET_ROCKET				142
#define MZ2_TURRET_BLASTER				143
#define MZ2_STALKER_BLASTER				144
#define MZ2_DAEDALUS_BLASTER			145
#define MZ2_MEDIC_BLASTER_2				146
#define	MZ2_CARRIER_RAILGUN				147
#define	MZ2_WIDOW_DISRUPTOR				148
#define	MZ2_WIDOW_BLASTER				149
#define	MZ2_WIDOW_RAIL					150
#define	MZ2_WIDOW_PLASMABEAM			151
#define	MZ2_CARRIER_MACHINEGUN_L2		152
#define	MZ2_CARRIER_MACHINEGUN_R2		153
#define	MZ2_WIDOW_RAIL_LEFT				154
#define	MZ2_WIDOW_RAIL_RIGHT			155
#define	MZ2_WIDOW_BLASTER_SWEEP1		156
#define	MZ2_WIDOW_BLASTER_SWEEP2		157
#define	MZ2_WIDOW_BLASTER_SWEEP3		158
#define	MZ2_WIDOW_BLASTER_SWEEP4		159
#define	MZ2_WIDOW_BLASTER_SWEEP5		160
#define	MZ2_WIDOW_BLASTER_SWEEP6		161
#define	MZ2_WIDOW_BLASTER_SWEEP7		162
#define	MZ2_WIDOW_BLASTER_SWEEP8		163
#define	MZ2_WIDOW_BLASTER_SWEEP9		164
#define	MZ2_WIDOW_BLASTER_100			165
#define	MZ2_WIDOW_BLASTER_90			166
#define	MZ2_WIDOW_BLASTER_80			167
#define	MZ2_WIDOW_BLASTER_70			168
#define	MZ2_WIDOW_BLASTER_60			169
#define	MZ2_WIDOW_BLASTER_50			170
#define	MZ2_WIDOW_BLASTER_40			171
#define	MZ2_WIDOW_BLASTER_30			172
#define	MZ2_WIDOW_BLASTER_20			173
#define	MZ2_WIDOW_BLASTER_10			174
#define	MZ2_WIDOW_BLASTER_0				175
#define	MZ2_WIDOW_BLASTER_10L			176
#define	MZ2_WIDOW_BLASTER_20L			177
#define	MZ2_WIDOW_BLASTER_30L			178
#define	MZ2_WIDOW_BLASTER_40L			179
#define	MZ2_WIDOW_BLASTER_50L			180
#define	MZ2_WIDOW_BLASTER_60L			181
#define	MZ2_WIDOW_BLASTER_70L			182
#define	MZ2_WIDOW_RUN_1					183
#define	MZ2_WIDOW_RUN_2					184
#define	MZ2_WIDOW_RUN_3					185
#define	MZ2_WIDOW_RUN_4					186
#define	MZ2_WIDOW_RUN_5					187
#define	MZ2_WIDOW_RUN_6					188
#define	MZ2_WIDOW_RUN_7					189
#define	MZ2_WIDOW_RUN_8					190
#define	MZ2_CARRIER_ROCKET_1			191
#define	MZ2_CARRIER_ROCKET_2			192
#define	MZ2_CARRIER_ROCKET_3			193
#define	MZ2_CARRIER_ROCKET_4			194
#define	MZ2_WIDOW2_BEAMER_1				195
#define	MZ2_WIDOW2_BEAMER_2				196
#define	MZ2_WIDOW2_BEAMER_3				197
#define	MZ2_WIDOW2_BEAMER_4				198
#define	MZ2_WIDOW2_BEAMER_5				199
#define	MZ2_WIDOW2_BEAM_SWEEP_1			200
#define	MZ2_WIDOW2_BEAM_SWEEP_2			201
#define	MZ2_WIDOW2_BEAM_SWEEP_3			202
#define	MZ2_WIDOW2_BEAM_SWEEP_4			203
#define	MZ2_WIDOW2_BEAM_SWEEP_5			204
#define	MZ2_WIDOW2_BEAM_SWEEP_6			205
#define	MZ2_WIDOW2_BEAM_SWEEP_7			206
#define	MZ2_WIDOW2_BEAM_SWEEP_8			207
#define	MZ2_WIDOW2_BEAM_SWEEP_9			208
#define	MZ2_WIDOW2_BEAM_SWEEP_10		209
#define	MZ2_WIDOW2_BEAM_SWEEP_11		210

extern vec3_t	monster_flash_offset[];

// Temp entity events
//
// Temp entity events are for things that happen at a location separate 
// from any existing entity.
// Temporary entity messages are explicitly constructed and broadcast.
typedef enum {
	TE_GUNSHOT,
	TE_BLOOD,
	TE_BLASTER,
	TE_RAILTRAIL,
	TE_SHOTGUN,
	TE_EXPLOSION1,
	TE_EXPLOSION2,
	TE_ROCKET_EXPLOSION,
	TE_GRENADE_EXPLOSION,
	TE_SPARKS,
	TE_SPLASH,
	TE_BUBBLETRAIL,
	TE_SCREEN_SPARKS,
	TE_SHIELD_SPARKS,
	TE_BULLET_SPARKS,
	TE_LASER_SPARKS,
	TE_PARASITE_ATTACK,
	TE_ROCKET_EXPLOSION_WATER,
	TE_GRENADE_EXPLOSION_WATER,
	TE_MEDIC_CABLE_ATTACK,
	TE_BFG_EXPLOSION,
	TE_BFG_BIGEXPLOSION,
	TE_BOSSTPORT,			// Used as '22' in a map, so DON'T RENUMBER!!!
	TE_BFG_LASER,
	TE_GRAPPLE_CABLE,
	TE_WELDING_SPARKS,
	TE_GREENBLOOD,
	TE_BLUEHYPERBLASTER,
	TE_PLASMA_EXPLOSION,
	TE_TUNNEL_SPARKS,
	TE_BLASTER2,
	TE_RAILTRAIL2,
	TE_FLAME,
	TE_LIGHTNING,
	TE_DEBUGTRAIL,
	TE_PLAIN_EXPLOSION,
	TE_FLASHLIGHT,
	TE_FORCEWALL,
	TE_HEATBEAM,
	TE_MONSTER_HEATBEAM,
	TE_STEAM,
	TE_BUBBLETRAIL2,
	TE_MOREBLOOD,
	TE_HEATBEAM_SPARKS,
	TE_HEATBEAM_STEAM,
	TE_CHAINFIST_SMOKE,
	TE_ELECTRIC_SPARKS,
	TE_TRACKER_EXPLOSION,
	TE_TELEPORT_EFFECT,
	TE_DBALL_GOAL,
	TE_WIDOWBEAMOUT,
	TE_NUKEBLAST,
	TE_WIDOWSPLASH,
	TE_EXPLOSION1_BIG,
	TE_EXPLOSION1_NP,
	TE_FLECHETTE
} temp_event_t;

#define SPLASH_UNKNOWN			0
#define SPLASH_SPARKS			1
#define SPLASH_BLUE_WATER		2
#define SPLASH_BROWN_WATER		3
#define SPLASH_SLIME			4
#define	SPLASH_LAVA				5
#define SPLASH_BLOOD			6

// Sound channels
// Channel 0 never willingly overrides
// Other channels (1-7) always override a playing sound on that channel
#define	CHAN_AUTO               0
#define	CHAN_WEAPON             1
#define	CHAN_VOICE              2
#define	CHAN_ITEM               3
#define	CHAN_BODY               4
// Modifier flags
#define	CHAN_NO_PHS_ADD			8	// Send to all clients, not just ones in PHS (ATTN 0 will also do this)
#define	CHAN_RELIABLE			16	// Send by reliable message, not datagram

// Sound attenuation values
#define	ATTN_NONE               0	// Full volume the entire level
#define	ATTN_NORM               1
#define	ATTN_IDLE               2
#define	ATTN_STATIC             3	// Diminish very rapidly with distance

// player_state->stats[] indexes
#define STAT_HEALTH_ICON		0
#define	STAT_HEALTH				1
#define	STAT_AMMO_ICON			2
#define	STAT_AMMO				3
#define	STAT_ARMOR_ICON			4
#define	STAT_ARMOR				5
#define	STAT_SELECTED_ICON		6
#define	STAT_PICKUP_ICON		7
#define	STAT_PICKUP_STRING		8
#define	STAT_TIMER_ICON			9
#define	STAT_TIMER				10
#define	STAT_HELPICON			11
#define	STAT_SELECTED_ITEM		12
#define	STAT_LAYOUTS			13
#define	STAT_FRAGS				14
#define	STAT_FLASHES			15		// Cleared each frame, 1 = health, 2 = armor
#define STAT_CHASE				16
#define STAT_SPECTATOR			17

#define	MAX_STATS				32

// dmflags->value flags
#define	DF_NO_HEALTH			0x00000001
#define	DF_NO_ITEMS				0x00000002
#define	DF_WEAPONS_STAY			0x00000004
#define	DF_NO_FALLING			0x00000008
#define	DF_INSTANT_ITEMS		0x00000010
#define	DF_SAME_LEVEL			0x00000020
#define DF_SKINTEAMS			0x00000040
#define DF_MODELTEAMS			0x00000080
#define DF_NO_FRIENDLY_FIRE		0x00000100
#define	DF_SPAWN_FARTHEST		0x00000200
#define DF_FORCE_RESPAWN		0x00000400
#define DF_NO_ARMOR				0x00000800
#define DF_ALLOW_EXIT			0x00001000
#define DF_INFINITE_AMMO		0x00002000
#define DF_QUAD_DROP			0x00004000
#define DF_FIXED_FOV			0x00008000
#define	DF_QUADFIRE_DROP		0x00010000
#define DF_NO_MINES				0x00020000
#define DF_NO_STACK_DOUBLE		0x00040000
#define DF_NO_NUKES				0x00080000
#define DF_NO_SPHERES			0x00100000

/*
 =======================================================================

 ELEMENTS COMMUNICATED ACROSS THE NET

 =======================================================================
*/

#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))

// Config strings are a general means of communication from the server 
// to all connected clients.
// Each config string can be at most MAX_QPATH characters.
#define	CS_NAME				0
#define	CS_CDTRACK			1
#define	CS_SKY				2
#define	CS_SKYAXIS			3		// %f %f %f format
#define	CS_SKYROTATE		4
#define	CS_STATUSBAR		5		// Display program string

#define CS_AIRACCEL			29		// Air acceleration control
#define	CS_MAXCLIENTS		30
#define	CS_MAPCHECKSUM		31		// For catching cheater maps

#define	CS_MODELS			32
#define	CS_SOUNDS			(CS_MODELS+MAX_MODELS)
#define	CS_IMAGES			(CS_SOUNDS+MAX_SOUNDS)
#define	CS_LIGHTS			(CS_IMAGES+MAX_IMAGES)
#define	CS_ITEMS			(CS_LIGHTS+MAX_LIGHTSTYLES)
#define	CS_PLAYERSKINS		(CS_ITEMS+MAX_ITEMS)
#define CS_GENERAL			(CS_PLAYERSKINS+MAX_CLIENTS)
#define	MAX_CONFIGSTRINGS	(CS_GENERAL+MAX_GENERAL)

// entity_state_t->event values
// Entity events are for effects that take place relative to an existing 
// entities origin. Very network efficient.
// All muzzle flashes really should be converted to events...
typedef enum {
	EV_NONE,
	EV_ITEM_RESPAWN,
	EV_FOOTSTEP,
	EV_FALLSHORT,
	EV_FALL,
	EV_FALLFAR,
	EV_PLAYER_TELEPORT,
	EV_OTHER_TELEPORT
} entity_event_t;

// entity_state_t is the information conveyed from the server in an
// update message about entities that the client will need to render in 
// some way
typedef struct entity_state_s {
	int				number;			// Edict index

	vec3_t			origin;
	vec3_t			angles;
	vec3_t			old_origin;		// For lerping
	int				modelindex;
	int				modelindex2, modelindex3, modelindex4;	// Weapons, CTF flags, etc...
	int				frame;
	int				skinnum;
	unsigned int	effects;		// We're filling it, so it needs to be unsigned
	int				renderfx;
	int				solid;			// For client side prediction, 8*(bits 0-4) is x/y radius
									// 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
									// gi.linkentity sets this properly
	int				sound;			// For looping sounds, to guarantee shutoff
	int				event;			// Impulse events -- muzzle flashes, footsteps, etc...
									// events only go out for a single frame, they
									// are automatically cleared each frame
} entity_state_t;

// player_state_t is the information needed in addition to pmove_state_t
// to render a view. There will only be 10 player_state_t sent each 
// second, but the number of pmove_state_t changes will be relative to 
// client frame rates
typedef struct {
	pmove_state_t	pmove;			// For prediction

	// These fields do not need to be communicated bit-precise

	vec3_t			viewangles;		// For fixed views
	vec3_t			viewoffset;		// Add to pmovestate->origin
	vec3_t			kick_angles;	// Add to view direction to get render angles
									// Set by weapon kicks, pain effects, etc...

	vec3_t			gunangles;
	vec3_t			gunoffset;
	int				gunindex;
	int				gunframe;

	float			blend[4];		// RGBA full screen effect
	
	float			fov;			// Horizontal field of view

	int				rdflags;		// refdef flags

	short			stats[MAX_STATS];	// Fast status bar updates
} player_state_t;


#endif	// __Q_SHARED_H__
