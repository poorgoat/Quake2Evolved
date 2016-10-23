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


// qcommon.h -- common definitions between client and server


#ifndef __QCOMMON_H__
#define __QCOMMON_H__


#include "../qshared/q_shared.h"
#include "../qcommon/qfiles.h"


/*
 =======================================================================

 PROTOCOL

 =======================================================================
*/

#define	PROTOCOL_VERSION		34

#define	UPDATE_BACKUP			16
#define	UPDATE_MASK				(UPDATE_BACKUP-1)

// Server to client
enum {
	SVC_BAD,

	// These ops are known to the game library
	SVC_MUZZLEFLASH,
	SVC_MUZZLEFLASH2,
	SVC_TEMP_ENTITY,
	SVC_LAYOUT,
	SVC_INVENTORY,

	// The rest are private to the client and server
	SVC_NOP,
	SVC_DISCONNECT,
	SVC_RECONNECT,
	SVC_SOUND,					// <see code>
	SVC_PRINT,					// [byte] id [string] null terminated string
	SVC_STUFFTEXT,				// [string] stuffed into client's console buffer, should be \n terminated
	SVC_SERVERDATA,				// [long] protocol ...
	SVC_CONFIGSTRING,			// [short] [string]
	SVC_SPAWNBASELINE,		
	SVC_CENTERPRINT,			// [string] to put in center of the screen
	SVC_DOWNLOAD,				// [short] size [size bytes]
	SVC_PLAYERINFO,				// variable
	SVC_PACKETENTITIES,			// [...]
	SVC_DELTAPACKETENTITIES,	// [...]
	SVC_FRAME
} svcOps_t;

// Client to server
enum {
	CLC_BAD,
	CLC_NOP, 		
	CLC_MOVE,					// [usercmd_t]
	CLC_USERINFO,				// [user info string]
	CLC_STRINGCMD				// [string] message
} clcOps_t;

// player_state_t communication
#define	PS_M_TYPE				(1<<0)
#define	PS_M_ORIGIN				(1<<1)
#define	PS_M_VELOCITY			(1<<2)
#define	PS_M_TIME				(1<<3)
#define	PS_M_FLAGS				(1<<4)
#define	PS_M_GRAVITY			(1<<5)
#define	PS_M_DELTA_ANGLES		(1<<6)
#define	PS_VIEWOFFSET			(1<<7)
#define	PS_VIEWANGLES			(1<<8)
#define	PS_KICKANGLES			(1<<9)
#define	PS_BLEND				(1<<10)
#define	PS_FOV					(1<<11)
#define	PS_WEAPONINDEX			(1<<12)
#define	PS_WEAPONFRAME			(1<<13)
#define	PS_RDFLAGS				(1<<14)

// usercmd_t communication
#define	CM_ANGLE1 				(1<<0)
#define	CM_ANGLE2 				(1<<1)
#define	CM_ANGLE3 				(1<<2)
#define	CM_FORWARD				(1<<3)
#define	CM_SIDE					(1<<4)
#define	CM_UP					(1<<5)
#define	CM_BUTTONS				(1<<6)
#define	CM_IMPULSE				(1<<7)

// A sound without an ent or pos will be a local only sound
#define	SND_VOLUME				(1<<0)		// A byte
#define	SND_ATTENUATION			(1<<1)		// A byte
#define	SND_POS					(1<<2)		// Three coordinates
#define	SND_ENT					(1<<3)		// A short 0-2: channel, 3-12: entity
#define	SND_OFFSET				(1<<4)		// A byte, msec offset from frame start

#define DEFAULT_SOUND_PACKET_VOLUME			1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION	1.0

// entity_state_t communication

// First byte
#define	U_ORIGIN1				(1<<0)
#define	U_ORIGIN2				(1<<1)
#define	U_ANGLE2				(1<<2)
#define	U_ANGLE3				(1<<3)
#define	U_FRAME8				(1<<4)		// Frame is a byte
#define	U_EVENT					(1<<5)
#define	U_REMOVE				(1<<6)		// REMOVE this entity, don't add it
#define	U_MOREBITS1				(1<<7)		// Read one additional byte

// Second byte
#define	U_NUMBER16				(1<<8)		// NUMBER8 is implicit if not set
#define	U_ORIGIN3				(1<<9)
#define	U_ANGLE1				(1<<10)
#define	U_MODEL					(1<<11)
#define U_RENDERFX8				(1<<12)		// Fullbright, etc...
#define	U_EFFECTS8				(1<<14)		// Autorotate, trails, etc...
#define	U_MOREBITS2				(1<<15)		// Read one additional byte

// Third byte
#define	U_SKIN8					(1<<16)
#define	U_FRAME16				(1<<17)		// Frame is a short
#define	U_RENDERFX16 			(1<<18)		// 8 + 16 = 32
#define	U_EFFECTS16				(1<<19)		// 8 + 16 = 32
#define	U_MODEL2				(1<<20)		// Weapons, flags, etc...
#define	U_MODEL3				(1<<21)
#define	U_MODEL4				(1<<22)
#define	U_MOREBITS3				(1<<23)		// Read one additional byte

// Fourth byte
#define	U_OLDORIGIN				(1<<24)		// FIXME: get rid of this
#define	U_SKIN16				(1<<25)
#define	U_SOUND					(1<<26)
#define	U_SOLID					(1<<27)

/*
 =======================================================================

 MISC

 =======================================================================
*/

#define	MAX_PRINTMSG			8192		// Max length of a print message

extern cvar_t	*com_developer;
extern cvar_t	*com_dedicated;
extern cvar_t	*com_paused;
extern cvar_t	*com_fixedTime;
extern cvar_t	*com_timeScale;
extern cvar_t	*com_timeDemo;
extern cvar_t	*com_aviDemo;
extern cvar_t	*com_forceAviDemo;
extern cvar_t	*com_speeds;
extern cvar_t	*com_debugMemory;
extern cvar_t	*com_zoneMegs;
extern cvar_t	*com_hunkMegs;
extern cvar_t	*com_maxFPS;
extern cvar_t	*com_logFile;

// This is set each time a key binding or archive variable is changed so
// that the system knows to save it to the config file
extern qboolean	com_configModified;

extern int		com_frameTime;
extern int		com_frameCount;

// com_speeds times
extern int		com_timeBefore, com_timeBetween, com_timeAfter;
extern int		com_timeBeforeGame, com_timeAfterGame;
extern int		com_timeBeforeRef, com_timeAfterRef;
extern int		com_timeBeforeSnd, com_timeAfterSnd;

unsigned	Com_BlockChecksum (const void *buffer, int length);
byte		Com_BlockSequenceCRCByte (const byte *buffer, int length, int sequence);

void		Com_BeginRedirect (int target, char *buffer, int bufferSize, void (*flush));
void		Com_EndRedirect (void);
void 		Com_Printf (const char *fmt, ...);
void 		Com_DPrintf (const char *fmt, ...);
void 		Com_Error (int code, const char *fmt, ...);
void		Com_ParseStartupCmdLine (char *cmdLine);

int			Com_ServerState (void);
void		Com_SetServerState (int state);

qboolean	Com_AllowCheats (void);

void		Com_Init (char *cmdLine);
void		Com_Frame (void);
void		Com_Shutdown (void);


/*
 =======================================================================

 MEMORY ALLOCATION

 =======================================================================
*/

void		*Z_Malloc (int size);
void		*Z_MallocSmall (int size);
void		*Z_TagMalloc (int size, int tag);
void		Z_Free (void *ptr);
void		Z_FreeTags (int tag);

void		*Hunk_Alloc (int size);
void		*Hunk_HighAlloc (int size);
void		Hunk_SetLowMark (void);
void		Hunk_SetHighMark (void);
void		Hunk_ClearToLowMark (void);
void		Hunk_ClearToHighMark (void);
void		Hunk_Clear (void);

char		*CopyString (const char *string);
void		FreeString (char *string);

void		Com_MemInfo_f (void);
void		Com_TouchMemory (void);
void		Com_InitMemory (void);
void		Com_ShutdownMemory (void);

/*
 =======================================================================

 FILE SYSTEM

 =======================================================================
*/

typedef int fileHandle_t;

typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND
} fsMode_t;

typedef enum {
	FS_SEEK_SET,
	FS_SEEK_CUR,
	FS_SEEK_END
} fsOrigin_t;

typedef struct {
	char			directory[MAX_OSPATH];
	char			description[MAX_OSPATH];
} modList_t;

int			FS_OpenFile (const char *name, fileHandle_t *f, fsMode_t mode);
void		FS_CloseFile (fileHandle_t f);
int			FS_Read (void *buffer, int size, fileHandle_t f);
int			FS_Write (const void *buffer, int size, fileHandle_t f);
int			FS_Printf (fileHandle_t f, const char *fmt, ...);
void		FS_Seek (fileHandle_t f, int offset, fsOrigin_t origin);
int			FS_Tell (fileHandle_t f);
void		FS_Flush (fileHandle_t f);

void		FS_CopyFile (const char *srcName, const char *dstName);
void		FS_RenameFile (const char *oldName, const char *newName);
void		FS_RemoveFile (const char *name);
int			FS_LoadFile (const char *name, void **buffer);
void		FS_FreeFile (void *buffer);
qboolean	FS_SaveFile (const char *name, const void *buffer, int size);
qboolean	FS_FileExists (const char *name);

char		**FS_ListFilteredFiles (const char *filter, qboolean sort, int *numFiles);
char		**FS_ListFiles (const char *path, const char *extension, qboolean sort, int *numFiles);
void		FS_FreeFileList (char **fileList);

modList_t	**FS_ListMods (int *numMods);
void		FS_FreeModList (modList_t **modList);

char		*FS_NextPath (char *prevPath);

void		FS_Restart (const char *game);

void		FS_Init (void);
void		FS_Shutdown (void);

/*
 =======================================================================

 SCRIPT PARSING

 =======================================================================
*/

#define MAX_TOKEN_LENGTH		1024

// Parse flags
#define PSF_ALLOW_NEWLINES		0x0001
#define PSF_ALLOW_STRINGCONCAT	0x0002
#define PSF_ALLOW_ESCAPECHARS	0x0004
#define PSF_ALLOW_PATHNAMES		0x0008
#define PSF_PARSE_GENERIC		0x0010
#define PSF_PRINT_ERRORS		0x0020
#define PSF_PRINT_WARNINGS		0x0040

// Number sub-types
#define NT_BINARY				0x0001
#define NT_OCTAL				0x0002
#define NT_DECIMAL				0x0004
#define NT_HEXADECIMAL			0x0008
#define NT_INTEGER				0x0010
#define NT_FLOAT				0x0020
#define NT_UNSIGNED				0x0040
#define NT_LONG					0x0080
#define NT_SINGLE				0x0100
#define NT_DOUBLE				0x0200
#define NT_EXTENDED				0x0400

// Punctuation sub-types
#define PT_RSHIFT_ASSIGN		1
#define PT_LSHIFT_ASSIGN		2
#define PT_PARAMETERS			3
#define PT_PRECOMPILER_MERGE	4
#define PT_LOGIC_AND			5
#define PT_LOGIC_OR				6
#define PT_LOGIC_GEQUAL			7
#define PT_LOGIC_LEQUAL			8
#define PT_LOGIC_EQUAL			9
#define PT_LOGIC_NOTEQUAL		10
#define PT_MUL_ASSIGN			11
#define PT_DIV_ASSIGN			12
#define PT_MOD_ASSIGN			13
#define PT_ADD_ASSIGN			14
#define PT_SUB_ASSIGN			15
#define PT_INCREMENT			16
#define PT_DECREMENT			17
#define PT_BINARY_AND_ASSIGN	18
#define PT_BINARY_OR_ASSIGN		19
#define PT_BINARY_XOR_ASSIGN	20
#define PT_RSHIFT				21
#define PT_LSHIFT				22
#define PT_POINTER_REFERENCE	23
#define PT_CPP_1				24
#define PT_CPP_2				25
#define PT_MUL					26
#define PT_DIV					27
#define PT_MOD					28
#define PT_ADD					29
#define PT_SUB					30
#define PT_ASSIGN				31
#define PT_BINARY_AND			32
#define PT_BINARY_OR			33
#define PT_BINARY_XOR			34
#define PT_BINARY_NOT			35
#define PT_LOGIC_NOT			36
#define PT_LOGIC_GREATER		37
#define PT_LOGIC_LESS			38
#define PT_REFERENCE			39
#define PT_COLON				40
#define PT_COMMA				41
#define PT_SEMICOLON			42
#define PT_QUESTION_MARK		43
#define PT_BRACE_OPEN			44
#define PT_BRACE_CLOSE			45
#define PT_BRACKET_OPEN			46
#define PT_BRACKET_CLOSE		47
#define PT_PARENTHESIS_OPEN		48
#define PT_PARENTHESIS_CLOSE	49
#define PT_PRECOMPILER			50
#define PT_DOLLAR				51
#define PT_BACKSLASH			52

typedef enum {
	TT_EMPTY,					// Empty (invalid or whitespace)
	TT_GENERIC,					// Generic string separated by spaces
	TT_STRING,					// String (enclosed between double quotes)
	TT_LITERAL,					// Literal (enclosed between single quotes)
	TT_NUMBER,					// Number
	TT_NAME,					// Name
	TT_PUNCTUATION				// Punctuation
} tokenType_t;

typedef struct {
	tokenType_t		type;
	unsigned		subType;

	int				line;

	char			string[MAX_TOKEN_LENGTH];
	int				length;

	double			floatValue;
	unsigned		integerValue;
} token_t;

typedef struct {
	const char		*name;
	unsigned		type;
} punctuation_t;

typedef struct {
	char			name[MAX_OSPATH];

	char			*buffer;
	int				size;
	qboolean		allocated;

	char			*text;
	int				line;

	punctuation_t	*punctuations;

	qboolean		tokenAvailable;
	token_t			token;
} script_t;

qboolean	PS_ReadToken (script_t *script, unsigned flags, token_t *token);
void		PS_UnreadToken (script_t *script, token_t *token);

qboolean	PS_ReadDouble (script_t *script, unsigned flags, double *value);
qboolean	PS_ReadFloat (script_t *script, unsigned flags, float *value);
qboolean	PS_ReadUnsigned (script_t *script, unsigned flags, unsigned *value);
qboolean	PS_ReadInteger (script_t *script, unsigned flags, int *value);

void		PS_SkipWhiteSpace (script_t *script);
void		PS_SkipRestOfLine (script_t *script);
void		PS_SkipBracedSection (script_t *script, int depth);

void		PS_ScriptError (script_t *script, unsigned flags, const char *fmt, ...);
void		PS_ScriptWarning (script_t *script, unsigned flags, const char *fmt, ...);

void		PS_SetPunctuationsTable (script_t *script, punctuation_t *punctuationsTable);
void		PS_ResetScript (script_t *script);
qboolean	PS_EndOfScript (script_t *script);

script_t	*PS_LoadScriptFile (const char *name);
script_t	*PS_LoadScriptMemory (const char *name, const char *buffer, int size);
void		PS_FreeScript (script_t *script);

/*
 =======================================================================

 CMD

 Any number of commands can be added in a frame, from several different
 sources.
 Most commands come from either key bindings or console line input, but
 remote servers can also send across commands and entire text files can
 be execed.
 The + command line options are also added to the command buffer.

 Command execution takes a null terminated string, breaks it into
 tokens, then searches for a command or variable that matches the first 
 token.
 =======================================================================
*/

typedef enum {
	EXEC_APPEND,	// Add to end of the command buffer
	EXEC_INSERT,	// Insert at current position, but don't run yet
	EXEC_NOW		// Don't return until completed
} cbufExec_t;

// As new commands are generated from the console or key bindings, the
// text is added to the end of the command buffer
void		Cbuf_AddText (const char *text);

// When a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining 
// unexecuted commands
void		Cbuf_InsertText (const char *text);

// This can be used in place of either Cbuf_AddText or Cbuf_InsertText
void		Cbuf_ExecuteText (cbufExec_t execWhen, const char *text);

// Pulls off \n or ; terminated lines of text from the command buffer
// and sends them through Cmd_ExecuteString. Stops when the buffer is
// empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!
void		Cbuf_Execute (void);

// These two functions are used to defer any pending commands while a 
// map is being loaded
void		Cbuf_CopyToDefer (void);
void		Cbuf_InsertFromDefer (void);

// The functions that execute commands get their parameters with these
// functions. Cmd_Argv() will return an empty string, not a NULL if
// arg > argc, so string operations are always safe.
int			Cmd_Argc (void);
char		*Cmd_Argv (int arg);
char		*Cmd_Args (void);

// Takes a null terminated string. Does not need to be \n terminated.
// Breaks the string up into arg tokens.
void		Cmd_TokenizeString (const char *text);

// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console
void		Cmd_ExecuteString (const char *text);

// Used by the cvar code to check for variable / command name overlap
qboolean	Cmd_Exists (const char *name);

// Will perform callbacks when a match is found
void		Cmd_CompleteCommand (const char *partial, void (*callback)(const char *));

// Called by the init functions of other parts of the program to 
// register commands and functions to call for them.
// If function is NULL, the command will be forwarded to the server
// as a CLC_STRINGCMD instead of executed locally.
void		Cmd_AddCommand (const char *name, void (*function)(void), const char *description);

// Removes a command
void		Cmd_RemoveCommand (const char *name);

void		Cmd_Init (void);
void		Cmd_Shutdown (void);


/*
 =======================================================================

 CVAR

 Console variables are used to hold scalar or string variables that can
 be changed or displayed at the console as well as accessed directly in
 C code.

 The user can access variables from the console in three ways:
	cvar			Prints the current value
	cvar X			Sets the current value to X
	set cvar X		Same as above, but creates the cvar if not present

 Variables are restricted from having the same names as commands to keep
 this interface from being ambiguous.
 =======================================================================
*/

extern cvar_t	*cvar_vars;

// Whenever a variable is modified, its flags will be OR'ed into this,
// so a single check can determine if any CVAR_USERINFO,
// CVAR_SERVERINFO, etc, variables have been modified since the last
// check. The bit can then be cleared to allow another change detection.
extern int	cvar_modifiedFlags;

// Appends lines containing "seta variable value" for all variables
// with the archive flag set
void 		Cvar_WriteVariables (fileHandle_t f);

// Will perform callbacks when a match is found
void		Cvar_CompleteVariable (const char *partial, void (*callback)(const char *));

// Used by the cmd code to check for variable / command name overlap
cvar_t		*Cvar_FindVariable (const char *name);

// Creates the variable if it doesn't exist, or returns the existing 
// one.
// If it exists, the value will only be changed if it has a latched
// value, and flags will be OR'ed in.
// That allows variables to be unarchived without needing bit flags.
cvar_t		*Cvar_Get (const char *name, const char *value, int flags, const char *description);

// Returns an empty string / zero if not defined
char		*Cvar_GetString (const char *name);
float		Cvar_GetFloat (const char *name);
int			Cvar_GetInteger (const char *name);

// Will create the variable if it doesn't exist
cvar_t 		*Cvar_SetString (const char *name, const char *value);
cvar_t		*Cvar_SetFloat (const char *name, float value);
cvar_t		*Cvar_SetInteger (const char *name, int value);

// Will set the variable even if CVAR_INIT, CVAR_LATCH or CVAR_ROM
cvar_t		*Cvar_ForceSet (const char *name, const char *value);

// If allowCheats is false, all CVAR_CHEAT variables will be forced to
// their default values
void		Cvar_FixCheatVariables (qboolean allowCheats);

// Called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command. Returns true if the command was a variable reference that
// was handled (print or change).
qboolean	Cvar_Command (void);

// Returns an info string containing all the CVAR_USERINFO variables
char		*Cvar_UserInfo (void);

// Returns an info string containing all the CVAR_SERVERINFO variables
char		*Cvar_ServerInfo (void);

void		Cvar_Init (void);
void		Cvar_Shutdown (void);

// Stupid hack to workaround our changes to cvar_t
gamecvar_t	*Cvar_GameUpdateOrCreate (cvar_t *var);
gamecvar_t	*Cvar_GameGet (char *name, char *value, int flags);
gamecvar_t	*Cvar_GameSet (char *name, char *value);
gamecvar_t	*Cvar_GameForceSet (char *name, char *value);

/*
 =======================================================================

 MESSAGE I/O FUNCTIONS

 =======================================================================
*/

typedef struct {
	qboolean		allowOverflow;	// If false, do a Com_Error
	qboolean		overflowed;		// Set to true if the buffer size failed
	byte			*data;
	int				maxSize;
	int				curSize;
	int				readCount;
} msg_t;

void		MSG_Init (msg_t *msg, byte *data, int maxSize, qboolean allowOverflow);
void		MSG_Clear (msg_t *msg);
byte		*MSG_GetSpace (msg_t *msg, int length);
void		MSG_Write (msg_t *msg, const void *data, int length);
void		MSG_Print (msg_t *msg, const char *data);

void		MSG_WriteChar (msg_t *msg, int c);
void		MSG_WriteByte (msg_t *msg, int b);
void		MSG_WriteShort (msg_t *msg, int s);
void		MSG_WriteLong (msg_t *msg, int l);
void		MSG_WriteFloat (msg_t *msg, float f);
void		MSG_WriteString (msg_t *msg, const char *s);
void		MSG_WriteCoord (msg_t *msg, float c);
void		MSG_WritePos (msg_t *msg, const vec3_t pos);
void		MSG_WriteAngle (msg_t *msg, float a);
void		MSG_WriteAngle16 (msg_t *msg, float a);
void		MSG_WriteDir (msg_t *msg, const vec3_t dir);
void		MSG_WriteDeltaUserCmd (msg_t *msg, const struct usercmd_s *from, const struct usercmd_s *to);
void		MSG_WriteDeltaEntity (msg_t *msg, const struct entity_state_s *from, const struct entity_state_s *to, qboolean force, qboolean newEntity);

void		MSG_BeginReading (msg_t *msg);
int			MSG_ReadChar (msg_t *msg);
int			MSG_ReadByte (msg_t *msg);
int			MSG_ReadShort (msg_t *msg);
int			MSG_ReadLong (msg_t *msg);
float		MSG_ReadFloat (msg_t *msg);
char		*MSG_ReadString (msg_t *msg);
char		*MSG_ReadStringLine (msg_t *msg);
float		MSG_ReadCoord (msg_t *msg);
void		MSG_ReadPos (msg_t *msg, vec3_t pos);
float		MSG_ReadAngle (msg_t *msg);
float		MSG_ReadAngle16 (msg_t *msg);
void		MSG_ReadDir (msg_t *msg, vec3_t dir);
void		MSG_ReadDeltaUserCmd (msg_t *msg, const struct usercmd_s *from, struct usercmd_s *to);
void		MSG_ReadData (msg_t *msg, void *buffer, int size);

/*
 =======================================================================

 NETWORK

 =======================================================================
*/

#define	PORT_MASTER				27900
#define	PORT_CLIENT				27901
#define	PORT_SERVER				27910
#define	PORT_ANY				-1

#define	MAX_MSGLEN				1400		// Max length of a message

typedef enum {
	NS_CLIENT, 
	NS_SERVER
} netSrc_t;

typedef enum {
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP
} netAdrType_t;

typedef struct {
	netAdrType_t	type;
	byte			ip[4];
	unsigned short	port;
} netAdr_t;

qboolean	NET_CompareAdr (const netAdr_t a, const netAdr_t b);
qboolean	NET_CompareBaseAdr (const netAdr_t a, const netAdr_t b);
qboolean	NET_IsLocalAddress (const netAdr_t adr);
char		*NET_AdrToString (const netAdr_t adr);
qboolean	NET_StringToAdr (const char *s, netAdr_t *adr);

qboolean	NET_GetPacket (netSrc_t sock, netAdr_t *from, msg_t *message);
void		NET_SendPacket (netSrc_t sock, const netAdr_t to, const void *data, int length);

void		NET_Init (void);
void		NET_Shutdown (void);

typedef struct {
	netSrc_t		sock;

	int				dropped;						// Between last packet and previous

	int				lastReceived;					// For time-outs
	int				lastSent;						// For retransmits

	netAdr_t		remoteAddress;
	int				qport;							// qport value to write when transmitting

	// Sequencing variables
	int				incomingSequence;
	int				incomingAcknowledged;
	int				incomingReliableSequence;		// Single bit, maintained local
	int				incomingReliableAcknowledged;	// Single bit

	int				outgoingSequence;
	int				reliableSequence;				// Single bit
	int				lastReliableSequence;			// Sequence number of last send

	// Reliable staging and holding areas
	msg_t			message;						// Writing buffer to send to server
	byte			messageBuffer[MAX_MSGLEN-16];	// Leave space for header

	// Message is copied to this buffer when it is first transfered
	int				reliableLength;
	byte			reliableBuffer[MAX_MSGLEN-16];	// Unacked reliable message
} netChan_t;

extern netAdr_t	net_from;
extern msg_t	net_message;

void		NetChan_Init (void);
void		NetChan_OutOfBand (netSrc_t sock, const netAdr_t adr, const void *data, int length);
void		NetChan_OutOfBandPrint (netSrc_t sock, const netAdr_t adr, const char *fmt, ...);
void		NetChan_Setup (netSrc_t sock, netChan_t *chan, const netAdr_t adr, int qport);
void		NetChan_Transmit (netChan_t *chan, const void *data, int length);
qboolean	NetChan_Process (netChan_t *chan, msg_t *msg);

/*
 =======================================================================

 CMODEL

 =======================================================================
*/

typedef struct cmodel_s {
	vec3_t			mins;
	vec3_t			maxs;
	vec3_t			origin;		// For sounds or lights
	int				headNode;
} cmodel_t;

void		CM_Init (void);

cmodel_t	*CM_LoadMap (const char *name, qboolean clientLoad, unsigned *checksum);
void		CM_UnloadMap (void);

void		CM_ClearStats (void);
void		CM_PrintStats (void);

int			CM_NumInlineModels (void);
cmodel_t	*CM_InlineModel (const char *name);

char		*CM_EntityString (void);

int			CM_NumClusters (void);
int			CM_LeafContents (int leafNum);
int			CM_LeafCluster (int leafNum);
int			CM_LeafArea (int leafNum);

// Creates a clipping hull for an arbitrary box
int			CM_HeadNodeForBox (const vec3_t mins, const vec3_t maxs);

// Call with topNode set to the headNode, returns with topNode set to
// the first node that splits the box
int			CM_BoxLeafNums (const vec3_t mins, const vec3_t maxs, int *list, int listSize, int *topNode);

int			CM_PointLeafNum (const vec3_t p);

// Returns an OR'ed contents mask
int			CM_PointContents (const vec3_t p, int headNode);
int			CM_TransformedPointContents (const vec3_t p, int headNode, const vec3_t origin, const vec3_t angles);

trace_t		CM_BoxTrace (const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, int headNode, int brushMask);
trace_t		CM_TransformedBoxTrace (const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, int headNode, int brushMask, const vec3_t origin, const vec3_t angles);

byte		*CM_ClusterPVS (int cluster);
byte		*CM_ClusterPHS (int cluster);

void		CM_SetAreaPortalState (int portalNum, qboolean open);
qboolean	CM_AreasConnected (int area1, int area2);
int			CM_WriteAreaBits (byte *buffer, int area);
void		CM_WritePortalState (fileHandle_t f);
void		CM_ReadPortalState (fileHandle_t f);
qboolean	CM_HeadNodeVisible (int headNode, const byte *visBits);

/*
 =======================================================================

 PLAYER MOVEMENT CODE

 Common between server and client so prediction matches
 =======================================================================
*/

extern float	pm_airAccelerate;

void		PMove (pmove_t *pmove);

/*
 =======================================================================

 NON-PORTABLE SYSTEM SERVICES

 =======================================================================
*/

#ifdef WIN32
	void		Sys_ShowConsole (qboolean show);
	void		Sys_CreateConsole (void);
#endif

char		**Sys_ListFilteredFiles (const char *directory, const char *filter, qboolean sort, int *numFiles);
char		**Sys_ListFiles (const char *directory, const char *extension, qboolean sort, int *numFiles);
void		Sys_FreeFileList (char **fileList);

void		Sys_CreateDirectory (const char *directory);
char		*Sys_GetCurrentDirectory (void);
char		*Sys_ScanForCD (void);

void		Sys_Print (const char *text);
void		Sys_Error (const char *fmt, ...);

char		*Sys_GetClipboardText (void);
void		Sys_ShellExecute (const char *path, const char *parms, qboolean quit);

int		Sys_Milliseconds (void);
double		Sys_GetClockTicks (void);

int		Sys_GetEvents (void);

void		Sys_Init (void);
void		Sys_Quit (void);

void		*Sys_LoadGame (void *import);
void		Sys_UnloadGame (void);

/*
 =======================================================================

 CLIENT / SERVER SYSTEMS

 =======================================================================
*/

void		Con_Print (const char *text);

void		CL_WriteKeyBindings (fileHandle_t f);
void		CL_SetKeyEditMode (qboolean editMode);
void		CL_InitKeys (void);
void		CL_ShutdownKeys (void);

void		CL_ForwardToServer (void);
void		CL_Drop (void);
void		CL_MapLoading (void);

qboolean	CL_AllowCheats (void);
void		CL_Frame (int msec);
void		CL_Init (void);
void		CL_Shutdown (void);

qboolean	SV_AllowCheats (void);
void		SV_Frame (int msec);
void		SV_Init (void);
void		SV_Shutdown (const char *message, qboolean reconnect);


#endif	// __QCOMMON_H__
