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

#define	PROTOCOL_VERSION	34

#define	UPDATE_BACKUP		16
#define	UPDATE_MASK			(UPDATE_BACKUP-1)

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
#define	PS_M_TYPE			(1<<0)
#define	PS_M_ORIGIN			(1<<1)
#define	PS_M_VELOCITY		(1<<2)
#define	PS_M_TIME			(1<<3)
#define	PS_M_FLAGS			(1<<4)
#define	PS_M_GRAVITY		(1<<5)
#define	PS_M_DELTA_ANGLES	(1<<6)
#define	PS_VIEWOFFSET		(1<<7)
#define	PS_VIEWANGLES		(1<<8)
#define	PS_KICKANGLES		(1<<9)
#define	PS_BLEND			(1<<10)
#define	PS_FOV				(1<<11)
#define	PS_WEAPONINDEX		(1<<12)
#define	PS_WEAPONFRAME		(1<<13)
#define	PS_RDFLAGS			(1<<14)

// usercmd_t communication
#define	CM_ANGLE1 			(1<<0)
#define	CM_ANGLE2 			(1<<1)
#define	CM_ANGLE3 			(1<<2)
#define	CM_FORWARD			(1<<3)
#define	CM_SIDE				(1<<4)
#define	CM_UP				(1<<5)
#define	CM_BUTTONS			(1<<6)
#define	CM_IMPULSE			(1<<7)

// A sound without an ent or pos will be a local only sound
#define	SND_VOLUME			(1<<0)		// A byte
#define	SND_ATTENUATION		(1<<1)		// A byte
#define	SND_POS				(1<<2)		// Three coordinates
#define	SND_ENT				(1<<3)		// A short 0-2: channel, 3-12: entity
#define	SND_OFFSET			(1<<4)		// A byte, msec offset from frame start

#define DEFAULT_SOUND_PACKET_VOLUME			1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION	1.0

// entity_state_t communication

// First byte
#define	U_ORIGIN1			(1<<0)
#define	U_ORIGIN2			(1<<1)
#define	U_ANGLE2			(1<<2)
#define	U_ANGLE3			(1<<3)
#define	U_FRAME8			(1<<4)		// Frame is a byte
#define	U_EVENT				(1<<5)
#define	U_REMOVE			(1<<6)		// REMOVE this entity, don't add it
#define	U_MOREBITS1			(1<<7)		// Read one additional byte

// Second byte
#define	U_NUMBER16			(1<<8)		// NUMBER8 is implicit if not set
#define	U_ORIGIN3			(1<<9)
#define	U_ANGLE1			(1<<10)
#define	U_MODEL				(1<<11)
#define U_RENDERFX8			(1<<12)		// Fullbright, etc...
#define	U_EFFECTS8			(1<<14)		// Autorotate, trails, etc...
#define	U_MOREBITS2			(1<<15)		// Read one additional byte

// Third byte
#define	U_SKIN8				(1<<16)
#define	U_FRAME16			(1<<17)		// Frame is a short
#define	U_RENDERFX16 		(1<<18)		// 8 + 16 = 32
#define	U_EFFECTS16			(1<<19)		// 8 + 16 = 32
#define	U_MODEL2			(1<<20)		// Weapons, flags, etc...
#define	U_MODEL3			(1<<21)
#define	U_MODEL4			(1<<22)
#define	U_MOREBITS3			(1<<23)		// Read one additional byte

// Fourth byte
#define	U_OLDORIGIN			(1<<24)		// FIXME: get rid of this
#define	U_SKIN16			(1<<25)
#define	U_SOUND				(1<<26)
#define	U_SOLID				(1<<27)

/*
 =======================================================================

 MISC

 =======================================================================
*/

extern cvar_t	*developer;
extern cvar_t	*dedicated;
extern cvar_t	*paused;
extern cvar_t	*timedemo;
extern cvar_t	*fixedtime;
extern cvar_t	*timescale;
extern cvar_t	*com_aviDemo;
extern cvar_t	*com_forceAviDemo;
extern cvar_t	*com_showTrace;
extern cvar_t	*com_speeds;
extern cvar_t	*com_debugMemory;
extern cvar_t	*com_zoneMegs;
extern cvar_t	*com_hunkMegs;
extern cvar_t	*com_logFile;

// This is set each time a key binding or archive variable is changed so
// that the system knows to save it to the config file
extern qboolean	com_configModified;

// com_speeds times
extern int		com_timeBefore, com_timeBetween, com_timeAfter;
extern int		com_timeBeforeGame, com_timeAfterGame;
extern int		com_timeBeforeRef, com_timeAfterRef;

unsigned	Com_BlockChecksum (const void *buffer, int length);
byte		Com_BlockSequenceCRCByte (const byte *buffer, int length, int sequence);

void		Com_BeginRedirect (int target, char *buffer, int bufferSize, void (*flush));
void		Com_EndRedirect (void);
void 		Com_Printf (const char *fmt, ...);
void 		Com_DPrintf (const char *fmt, ...);
void 		Com_Error (int code, const char *fmt, ...);

int			Com_ServerState (void);
void		Com_SetServerState (int state);

void		Com_Init (char *cmdLine);
void		Com_Frame (int msec);
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

 FILESYSTEM

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

int			FS_FOpenFile (const char *name, fileHandle_t *f, fsMode_t mode);
void		FS_FCloseFile (fileHandle_t f);
int			FS_Read (void *buffer, int size, fileHandle_t f);
int			FS_Write (const void *buffer, int size, fileHandle_t f);
int			FS_Printf (fileHandle_t f, const char *fmt, ...);
void		FS_Seek (fileHandle_t f, int offset, fsOrigin_t origin);
int			FS_Tell (fileHandle_t f);
void		FS_Flush (fileHandle_t f);
void		FS_CopyFile (const char *srcName, const char *dstName);
void		FS_RenameFile (const char *oldName, const char *newName);
void		FS_DeleteFile (const char *name);
int			FS_LoadFile (const char *name, void **buffer);
void		FS_FreeFile (void *buffer);
qboolean	FS_SaveFile (const char *name, const void *buffer, int size);

int			FS_GetFileList (const char *path, const char *extension, char *buffer, int size);
int			FS_GetModList (char *buffer, int size);
void		FS_CreatePath (const char *path);
void		FS_RemovePath (const char *path);
char		*FS_NextPath (char *prevPath);

void		FS_Restart (void);
void		FS_Init (void);
void		FS_Shutdown (void);

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

// Pulls off \n terminated lines of text from the command buffer and 
// sends them through Cmd_ExecuteString. Stops when the buffer is empty.
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

// Takes a null terminated string.  Does not need to be \n terminated.
// Breaks the string up into arg tokens.
void		Cmd_TokenizeString (const char *text);

// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console
void		Cmd_ExecuteString (const char *text);

// Used by the cvar code to check for cvar / command name overlap
qboolean	Cmd_Exists (const char *name);

// Will perform callbacks when a match is found
void		Cmd_CompleteCommand (const char *partial, void (*callback)(const char *found));

// Called by the init functions of other parts of the program to 
// register commands and functions to call for them.
// If function is NULL, the command will be forwarded to the server
// as a CLC_STRINGCMD instead of executed locally.
void		Cmd_AddCommand (const char *name, void (*function)(void));

void		Cmd_RemoveCommand (const char *name);

void		Cmd_Init (void);
void		Cmd_Shutdown (void);


/*
 =======================================================================

 CVAR

 cvar_t variables are used to hold scalar or string variables that can
 be changed or displayed at the console as well as accessed directly in
 C code.

 The user can access cvars from the console in three ways:
	cvar			Prints the current value
	cvar 1			Sets the current value to 1
	set cvar 1		Same as above, but creates the cvar if not present

 Cvars are restricted from having the same names as commands to keep
 this interface from being ambiguous.
 =======================================================================
*/

extern cvar_t	*cvar_vars;

// This is set each time a CVAR_USERINFO variable is changed so that the 
// client knows to send it to the server
extern qboolean	cvar_userInfoModified;

// Appends lines containing "seta variable value" for all variables
// with the archive flag set
void 		Cvar_WriteVariables (fileHandle_t f);

// Will perform callbacks when a match is found
void		Cvar_CompleteVariable (const char *partial, void (*callback)(const char *found));

// Used by the cmd code to check for cvar / command name overlap
cvar_t		*Cvar_FindVar (const char *name);

// Returns an empty string / zero if not defined
char		*Cvar_VariableString (const char *name);
float		Cvar_VariableValue (const char *name);
int			Cvar_VariableInteger (const char *name);

// Creates the variable if it doesn't exist, or returns the existing 
// one.
// If it exists, the value will not be changed, but flags will be OR'ed
// in.
// That allows variables to be unarchived without needing bit flags.
cvar_t		*Cvar_Get (const char *name, const char *value, int flags);

// Will create the variable if it doesn't exist
cvar_t 		*Cvar_Set (const char *name, const char *value);
cvar_t		*Cvar_SetValue (const char *name, float value);
cvar_t		*Cvar_SetInteger (const char *name, int integer);

// Will set the variable even if CVAR_INIT, CVAR_LATCH or CVAR_ROM
cvar_t		*Cvar_ForceSet (const char *name, const char *value);

// If allowCheats is false, all CVAR_CHEAT cvars will be forced to their
// default values
void		Cvar_FixCheatVars (qboolean allowCheats);

// Called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command. Returns true if the command was a variable reference that
// was handled (print or change).
qboolean	Cvar_Command (void);

// Returns an info string containing all the CVAR_USERINFO cvars
char		*Cvar_UserInfo (void);

// Returns an info string containing all the CVAR_SERVERINFO cvars
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
	qboolean	allowOverflow;	// If false, do a Com_Error
	qboolean	overflowed;		// Set to true if the buffer size failed
	byte		*data;
	int			maxSize;
	int			curSize;
	int			readCount;
} msg_t;

void	MSG_Init (msg_t *msg, byte *data, int maxSize, qboolean allowOverflow);
void	MSG_Clear (msg_t *msg);
byte	*MSG_GetSpace (msg_t *msg, int length);
void	MSG_Write (msg_t *msg, const void *data, int length);
void	MSG_Print (msg_t *msg, const char *data);

void	MSG_WriteChar (msg_t *msg, int c);
void	MSG_WriteByte (msg_t *msg, int b);
void	MSG_WriteShort (msg_t *msg, int s);
void	MSG_WriteLong (msg_t *msg, int l);
void	MSG_WriteFloat (msg_t *msg, float f);
void	MSG_WriteString (msg_t *msg, const char *s);
void	MSG_WriteCoord (msg_t *msg, float c);
void	MSG_WritePos (msg_t *msg, const vec3_t pos);
void	MSG_WriteAngle (msg_t *msg, float a);
void	MSG_WriteAngle16 (msg_t *msg, float a);
void	MSG_WriteDir (msg_t *msg, const vec3_t dir);
void	MSG_WriteDeltaUserCmd (msg_t *msg, const struct usercmd_s *from, const struct usercmd_s *to);
void	MSG_WriteDeltaEntity (msg_t *msg, const struct entity_state_s *from, const struct entity_state_s *to, qboolean force, qboolean newEntity);

void	MSG_BeginReading (msg_t *msg);
int		MSG_ReadChar (msg_t *msg);
int		MSG_ReadByte (msg_t *msg);
int		MSG_ReadShort (msg_t *msg);
int		MSG_ReadLong (msg_t *msg);
float	MSG_ReadFloat (msg_t *msg);
char	*MSG_ReadString (msg_t *msg);
char	*MSG_ReadStringLine (msg_t *msg);
float	MSG_ReadCoord (msg_t *msg);
void	MSG_ReadPos (msg_t *msg, vec3_t pos);
float	MSG_ReadAngle (msg_t *msg);
float	MSG_ReadAngle16 (msg_t *msg);
void	MSG_ReadDir (msg_t *msg, vec3_t dir);
void	MSG_ReadDeltaUserCmd (msg_t *msg, const struct usercmd_s *from, struct usercmd_s *to);
void	MSG_ReadData (msg_t *msg, void *buffer, int size);

/*
 =======================================================================

 NETWORK

 =======================================================================
*/

#define	PORT_MASTER		27900
#define	PORT_CLIENT		27901
#define	PORT_SERVER		27910
#define	PORT_ANY		-1

#define	MAX_MSGLEN		1400		// Max length of a message

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
	netSrc_t	sock;

	int			dropped;						// Between last packet and previous

	int			lastReceived;					// For time-outs
	int			lastSent;						// For retransmits

	netAdr_t	remoteAddress;
	int			qport;							// qport value to write when transmitting

	// Sequencing variables
	int			incomingSequence;
	int			incomingAcknowledged;
	int			incomingReliableSequence;		// Single bit, maintained local
	int			incomingReliableAcknowledged;	// Single bit

	int			outgoingSequence;
	int			reliableSequence;				// Single bit
	int			lastReliableSequence;			// Sequence number of last send

	// Reliable staging and holding areas
	msg_t		message;						// Writing buffer to send to server
	byte		messageBuffer[MAX_MSGLEN-16];	// Leave space for header

	// Message is copied to this buffer when it is first transfered
	int			reliableLength;
	byte		reliableBuffer[MAX_MSGLEN-16];	// Unacked reliable message
} netChan_t;

extern netAdr_t		net_from;
extern msg_t		net_message;

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
	vec3_t	mins;
	vec3_t	maxs;
	vec3_t	origin;		// For sounds or lights
	int		headNode;
} cmodel_t;

cmodel_t	*CM_LoadMap (const char *name, qboolean clientLoad, unsigned *checksum);
void		CM_UnloadMap (void);

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

// Returns an ORed contents mask
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

void PMove (pmove_t *pmove);

/*
 =======================================================================

 NON-PORTABLE SYSTEM SERVICES

 =======================================================================
*/

typedef enum {
	LIB_GAME,
	LIB_CGAME,
	LIB_UI
} sysLib_t;

char	*Sys_GetCommand (void);
void	Sys_Print (const char *text);
void	Sys_Error (const char *fmt, ...);
void	Sys_ShowConsole (qboolean show);

int		Sys_FindFiles (const char *path, const char *pattern, char **fileList, int maxFiles, qboolean addFiles, qboolean addDirs);
int		Sys_RecursiveFindFiles (const char *path, char **fileList, int maxFiles, int fileCount, qboolean addFiles, qboolean addDirs);
void	Sys_CreateDirectory (const char *path);
void	Sys_RemoveDirectory (const char *path);
char	*Sys_GetCurrentDirectory (void);
char	*Sys_ScanForCD (void);

char	*Sys_GetClipboardText (void);
void	Sys_ShellExecute (const char *path, const char *parms, qboolean exit);
int		Sys_Milliseconds (void);
void	Sys_PumpMessages (void);

void	Sys_Init (void);
void	Sys_Quit (void);

void	*Sys_LoadLibrary (sysLib_t lib, void *import);
void	Sys_FreeLibrary (sysLib_t lib);

/*
 =======================================================================

 CLIENT / SERVER SYSTEMS

 =======================================================================
*/

void	Con_Print (const char *text);

void	Key_WriteBindings (fileHandle_t f);
void	Key_Init (void);
void	Key_Shutdown (void);

void	CL_Loading (void);
void	CL_UpdateScreen (void);
void	CL_ForwardToServer (void);
void	CL_ClearMemory (void);
void	CL_Drop (void);
void	CL_Frame (int msec);
void	CL_Init (void);
void	CL_Shutdown (void);

void	SV_Frame (int msec);
void	SV_Init (void);
void	SV_Shutdown (const char *message, qboolean reconnect);


#endif	// __QCOMMON_H__
