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


// common.c -- misc functions used in client and server


#include "qcommon.h"
#include <setjmp.h>


#define MAX_NUM_ARGVS	64

static int		com_argc;
static char		*com_argv[MAX_NUM_ARGVS];

static int		com_rdTarget;
static char		*com_rdBuffer;
static int		com_rdBufferSize;
static void		(*com_rdFlush)(int target, char *buffer);

static FILE		*com_logFileHandle;

static int		com_serverState;

static qboolean	com_allowCheats;

static jmp_buf	com_abortFrame;

qboolean		com_configModified = false;

int				com_frameTime = 0;
int				com_frameCount = 0;

int				com_timeBefore, com_timeBetween, com_timeAfter;
int				com_timeBeforeGame, com_timeAfterGame;
int				com_timeBeforeRef, com_timeAfterRef;
int				com_timeBeforeSnd, com_timeAfterSnd;

cvar_t	*com_developer;
cvar_t	*com_dedicated;
cvar_t	*com_paused;
cvar_t	*com_fixedTime;
cvar_t	*com_timeScale;
cvar_t	*com_timeDemo;
cvar_t	*com_aviDemo;
cvar_t	*com_forceAviDemo;
cvar_t	*com_speeds;
cvar_t	*com_debugMemory;
cvar_t	*com_zoneMegs;
cvar_t	*com_hunkMegs;
cvar_t	*com_maxFPS;
cvar_t	*com_logFile;


/*
 =======================================================================

 CLIENT / SERVER INTERACTIONS

 =======================================================================
*/


/*
 =================
 Com_BeginRedirect
 =================
*/
void Com_BeginRedirect (int target, char *buffer, int bufferSize, void (*flush)){

	if (!target || !buffer || !bufferSize || !flush)
		return;

	com_rdTarget = target;
	com_rdBuffer = buffer;
	com_rdBufferSize = bufferSize;
	com_rdFlush = flush;

	*com_rdBuffer = 0;
}

/*
 =================
 Com_EndRedirect
 =================
*/
void Com_EndRedirect (void){

	com_rdFlush(com_rdTarget, com_rdBuffer);

	com_rdTarget = 0;
	com_rdBuffer = NULL;
	com_rdBufferSize = 0;
	com_rdFlush = NULL;
}

/*
 =================
 Com_Redirect
 =================
*/
static void Com_Redirect (const char *msg){

	if (!com_rdTarget)
		return;

	if ((strlen(com_rdBuffer) + strlen(msg)) > (com_rdBufferSize - 1)){
		com_rdFlush(com_rdTarget, com_rdBuffer);
		*com_rdBuffer = 0;
	}

	Q_strncatz(com_rdBuffer, msg, com_rdBufferSize);
}

/*
 =================
 Com_OpenLogFile
 =================
*/
static void Com_OpenLogFile (void){

	time_t		clock;
	struct tm	*curTime;

	if (com_logFileHandle)
		return;

	if (com_logFile->integerValue == 1 || com_logFile->integerValue == 2)
#ifdef SECURE
		com_logFileHandle = fopen_s(com_logFileHandle, "qconsole.log", "wt");
#else
		com_logFileHandle = fopen("qconsole.log", "wt");
#endif
	else if (com_logFile->integerValue == 3 || com_logFile->integerValue == 4)
#ifdef SECURE
		com_logFileHandle = fopen_s(com_logFileHandle, "qconsole.log", "at");
#else
		com_logFileHandle = fopen("qconsole.log", "at");
#endif
	if (!com_logFileHandle)
		return;

	time(&clock);
#ifdef SECURE
	curTime = localtime_s(&curTime, &clock);
#else
	curTime = localtime(&clock);
#endif
	fprintf(com_logFileHandle, "%s (%s)\n", Q2E_VERSION, __DATE__);
#ifdef SECURE
	fprintf(com_logFileHandle, "Log file opened on %s\n\n", asctime_s(curTime, sizeof(curTime), &curTime));
#else
	fprintf(com_logFileHandle, "Log file opened on %s\n\n", asctime(curTime));
#endif
}

/*
 =================
 Com_CloseLogFile
 =================
*/
static void Com_CloseLogFile (void){

	time_t		clock;
	struct tm	*curTime;

	if (!com_logFileHandle)
		return;

	time(&clock);
#ifdef SECURE
	curTime = localtime_s(&curTime, &clock);
	fprintf(com_logFileHandle, "\nLog file closed on %s\n\n", asctime_s(curTime, sizeof(curTime), &curTime));
#else
	curTime = localtime(&clock);
	fprintf(com_logFileHandle, "\nLog file closed on %s\n\n", asctime(curTime));
#endif

	fclose(com_logFileHandle);
	com_logFileHandle = NULL;
}

/*
 =================
 Com_LogPrint
 =================
*/
static void Com_LogPrint (const char *text){

	if (!com_logFileHandle)
		return;

	fprintf(com_logFileHandle, "%s", text);

	if (com_logFile->integerValue == 2 || com_logFile->integerValue == 4)
		fflush(com_logFileHandle);	// Force it to save every time
}
	
/*
 =================
 Com_Printf

 Both client and server can use this, and it will output to the 
 appropriate place
 =================
*/
void Com_Printf (const char *fmt, ...){

	char	string[MAX_PRINTMSG];
	va_list	argPtr;

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), MAX_PRINTMSG, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);

	if (com_rdTarget){
		Com_Redirect(string);
		return;
	}

	// Print to client console
	Con_Print(string);

	// Also echo to dedicated console
	Sys_Print(string);

	// Log file
	if (com_logFile){
		if (com_logFile->integerValue)
			Com_OpenLogFile();
		else
			Com_CloseLogFile();

		Com_LogPrint(string);
	}
}

/*
 =================
 Com_DPrintf

 A Com_Printf that only shows up if the "developer" variable is set
 =================
*/
void Com_DPrintf (const char *fmt, ...){

	char	string[MAX_PRINTMSG];
	va_list	argPtr;

	if (!com_developer || !com_developer->integerValue)
		return;		// Don't confuse non-developers with techie stuff...

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), MAX_PRINTMSG, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);
	
	Com_Printf("%s", string);
}

/*
 =================
 Com_Error

 Both client and server can use this, and it will do the appropriate
 things
 =================
*/
void Com_Error (int code, const char *fmt, ...){

	static qboolean	recursive;
	static char		string[MAX_PRINTMSG];
	static int		count, lastTime;
	int				time;
	va_list			argPtr;

	// If we are getting a stream of errors, do an ERR_FATAL
	time = Sys_Milliseconds();

	if (time - lastTime < 100){
		if (count >= 3)
			code = ERR_FATAL;

		count++;
	}
	else
		count = 0;

	lastTime = time;

	// Avoid recursion
	if (recursive)
		Sys_Error("Recursive error after: %s", string);
	recursive = true;

	// Get the message
	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), MAX_PRINTMSG, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);

	// Handle the error
	if (code == ERR_DISCONNECT){
		Com_Printf("*****************************\n");
		Com_Printf("ERROR: %s\n", string);
		Com_Printf("*****************************\n");

		CL_Drop();

		recursive = false;
		longjmp(com_abortFrame, -1);
	}
	else if (code == ERR_DROP){
		Com_Printf("*****************************\n");
		Com_Printf("ERROR: %s\n", string);
		Com_Printf("*****************************\n");

		SV_Shutdown(va("Server crashed: %s\n", string), false);

		CL_Drop();

		recursive = false;
		longjmp(com_abortFrame, -1);
	}

	// ERR_FATAL
	SV_Shutdown(va("Server fatal crashed: %s\n", string), false);

	Sys_Error("%s", string);
}

/*
 =================
 Com_ServerState
 =================
*/
int Com_ServerState (void){

	return com_serverState;
}

/*
 =================
 Com_SetServerState
 =================
*/
void Com_SetServerState (int state){

	com_serverState = state;
}

/*
 =================
 Com_AllowCheats
 =================
*/
qboolean Com_AllowCheats (void){

	return com_allowCheats;
}

/*
 =================
 Com_SetAllowCheats
 =================
*/
static void Com_SetAllowCheats (void){

	if (SV_AllowCheats() && CL_AllowCheats())
		com_allowCheats = true;
	else
		com_allowCheats = false;

	Cvar_FixCheatVariables(com_allowCheats);
}

/*
 =================
 Com_WriteConfig
 =================
*/
static void Com_WriteConfig (const char *name){

	fileHandle_t	f;

	FS_OpenFile(name, &f, FS_WRITE);
	if (!f){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}

	FS_Printf(f, "// Automatically generated by Quake II Evolved, modifications to this file will be discarded...\r\n\r\n");

	CL_WriteKeyBindings(f);

	Cvar_WriteVariables(f);

	FS_CloseFile(f);
}

/*
 =================
 Com_Quit_f

 Both client and server can use this, and it will do the appropriate
 things
 =================
*/
void Com_Quit_f (void){

	Sys_Quit();
}

/*
 =================
 Com_Error_f

 Just trow a fatal or drop error to test error shutdown procedures
 =================
*/
void Com_Error_f (void){

	if (Cmd_Argc() > 1)
		Com_Error(ERR_DROP, "Testing drop error...");
	else
		Com_Error(ERR_FATAL, "Testing fatal error...");
}

/*
 =================
 Com_WriteConfig_f
 =================
*/
void Com_WriteConfig_f (void){

	char	name[MAX_OSPATH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: writeConfig <fileName>\n");
		return;
	}

	Q_strncpyz(name, Cmd_Argv(1), sizeof(name));
	Com_DefaultExtension(name, sizeof(name), ".cfg");

	Com_Printf("Writing %s...\n", name);

	Com_WriteConfig(name);
}

/*
 =================
 Com_Pause_f
 =================
*/
void Com_Pause_f (void){

	// Never pause in multiplayer
	if (!com_serverState || Cvar_GetInteger("maxclients") > 1){
		Cvar_ForceSet("paused", "0");
		return;
	}

	Cvar_SetInteger("paused", !com_paused->integerValue);
}


/*
 =======================================================================

 COMMAND LINE PARSING

 =======================================================================
*/


/*
 =================
 Com_AddEarlyCommands

 Adds command line parameters as script statements.
 Commands lead with a +, and continue until another +.

 Set commands are added early, so they are guaranteed to be set before
 the client and server initialize for the first time.

 Other commands are added late, after all initialization is complete.
 =================
*/
static void Com_AddEarlyCommands (qboolean clear){

	int		i;

	for (i = 1; i < com_argc; i++){

		if (Q_stricmp(com_argv[i], "+set"))
			continue;

		Cbuf_AddText(va("set %s %s\n", com_argv[i+1], com_argv[i+2]));

		if (clear){
			com_argv[i+0] = "";
			com_argv[i+1] = "";
			com_argv[i+2] = "";
		}

		i += 2;
	}
}

/*
 =================
 Com_AddLateCommands

 Adds command line parameters as script statements.
 Commands lead with a + and continue until another +.

 Returns true if any late commands were added, which will keep the 
 logo cinematic from immediately starting.
 =================
*/
static qboolean Com_AddLateCommands (void){

	int			i, j;
	char		text[1024];
	qboolean	ret = false;

	for (i = 1; i < com_argc; ){
		if (com_argv[i][0] != '+'){
			i++;
			continue;
		}

		for (j = 1; com_argv[i][j]; j++){
			if (com_argv[i][j] != '+')
				break;
		}

		Q_strncpyz(text, com_argv[i]+j, sizeof(text));
		Q_strncatz(text, " ", sizeof(text));
		i++;

		while (i < com_argc){
			if (com_argv[i][0] == '+')
				break;

			Q_strncatz(text, com_argv[i], sizeof(text));
			Q_strncatz(text, " ", sizeof(text));
			i++;
		}

		if (text[0]){
			ret = true;

			text[strlen(text)-1] = '\n';
			Cbuf_AddText(text);
		}
	}

	return ret;
}

/*
 =================
 Com_ParseCommandLine
 =================
*/
static void Com_ParseCommandLine (char *cmdLine){

	com_argv[0] = "exe";
	com_argc = 1;

	while (*cmdLine){
		while (*cmdLine && ((*cmdLine <= 32) || (*cmdLine > 126)))
			cmdLine++;

		if (*cmdLine){
			if (com_argc == MAX_NUM_ARGVS)
				Com_Error(ERR_FATAL, "Com_ParseCommandLine: MAX_NUM_ARGVS");

			com_argv[com_argc++] = cmdLine;

			while (*cmdLine && ((*cmdLine > 32) && (*cmdLine <= 126)))
				cmdLine++;

			if (*cmdLine){
				*cmdLine = 0;
				cmdLine++;
			}
		}
	}
}


// =====================================================================


/*
 =================
 Com_Init
 =================
*/
void Com_Init (char *cmdLine){

	if (setjmp(com_abortFrame))
		Sys_Error("Error during initialization");

	Com_Printf("%s (%s)\n", Q2E_VERSION, __DATE__);

	// We need to call Com_InitMemory twice, because some strings and
	// structs need to be allocated during initialization, but we want
	// to get the amount of memory to allocate for the main zone and
	// hunk from the config files
	Com_InitMemory();

	// Parse the command line
	Com_ParseCommandLine(cmdLine);

	// Prepare enough of the subsystems to handle commands and variables
	Cmd_Init();
	Cvar_Init();

	CL_InitKeys();

	// Initialize system code
	Sys_Init();

	// We need to add the early commands twice, because some file system
	// variables need to be set before execing config files, but we want
	// other parms to override the settings of the config files
	Com_AddEarlyCommands(false);
	Cbuf_Execute();

	// Initialize file system
	FS_Init();

	Com_AddEarlyCommands(true);
	Cbuf_Execute();

	// Register variables and commands
	Cvar_Get("version", va("%s (%s)", Q2E_VERSION, __DATE__), CVAR_SERVERINFO | CVAR_ROM, "Game version");
	com_developer = Cvar_Get("developer", "0", 0, "Developer mode");
	com_dedicated = Cvar_Get("dedicated", "0", CVAR_INIT, "Dedicated server");
	com_paused = Cvar_Get("paused", "0", CVAR_CHEAT, "Paused");
	com_fixedTime = Cvar_Get("com_fixedTime", "0", CVAR_CHEAT, "Fixed time");
	com_timeScale = Cvar_Get("com_timeScale", "1.0", CVAR_CHEAT, "Time scale");
	com_timeDemo = Cvar_Get("com_timeDemo", "0", CVAR_CHEAT, "Time a demo");
	com_aviDemo = Cvar_Get("com_aviDemo", "0", CVAR_CHEAT, "Take screenshots during demo playback for AVI compression");
	com_forceAviDemo = Cvar_Get("com_forceAviDemo", "0", CVAR_CHEAT, "Take screenshots for AVI compression even if not running a demo");
	com_speeds = Cvar_Get("com_speeds", "0", CVAR_CHEAT, "Report engine speeds");
	com_debugMemory = Cvar_Get("com_debugMemory", "0", CVAR_CHEAT, "Debug memory allocations");
	com_zoneMegs = Cvar_Get("com_zoneMegs", "64", CVAR_ARCHIVE | CVAR_LATCH, "Reserved space for zone memory in megabytes");
	com_hunkMegs = Cvar_Get("com_hunkMegs", "128", CVAR_ARCHIVE | CVAR_LATCH, "Reserved space for hunk memory in megabytes");
	com_maxFPS = Cvar_Get("com_maxFPS", "0", CVAR_ARCHIVE, "Lock framerate");
	com_logFile = Cvar_Get("com_logFile", "0", 0, "Log console messages");

	Cmd_AddCommand("quit", Com_Quit_f, "Quit the game");
	Cmd_AddCommand("error", Com_Error_f, "Test an error");
	Cmd_AddCommand("writeConfig", Com_WriteConfig_f, "Write a config file");
	Cmd_AddCommand("pause", Com_Pause_f, "Pause the game");
	Cmd_AddCommand("memInfo", Com_MemInfo_f, "Show memory information");

	// Initialize main zone and hunk memory
	Com_InitMemory();

	// Initialize server and client
	SV_Init();
	CL_Init();

	// Initialize cmodel
	CM_Init();

	// Initialize network
	NET_Init();
	NetChan_Init();

	// Add the late commands
	if (com_dedicated->integerValue)
		Com_AddLateCommands();
	else
		Sys_ShowConsole(true);

		// If the user didn't give any commands, play the logo cinematic
		if (!Com_AddLateCommands())
			Cbuf_AddText("cinematic idlog.cin\n");

	Com_Printf("======= Quake II Evolved Initialized =======\n");

	// Process commands
	Cbuf_Execute();
}

/*
 =================
 Com_Frame
 =================
*/
void Com_Frame (void){

	static int	lastTime;
	int			msec, minMsec = 1;

	if (setjmp(com_abortFrame))
		return;			// An error occurred, exit the entire frame

	// Update the config file if needed
	if (com_configModified){
		com_configModified = false;

		Com_WriteConfig("q2econfig.cfg");
	}

	// Clear com_speeds statistics
	com_timeBefore = com_timeBetween = com_timeAfter = 0;
	com_timeBeforeGame = com_timeAfterGame = 0;
	com_timeBeforeRef = com_timeAfterRef = 0;
	com_timeBeforeSnd = com_timeAfterSnd = 0;

	// Clear cmodel statistics
	CM_ClearStats();

	// We may want to spin here if things are going too fast
	if (!com_dedicated->integerValue && !com_timeDemo->integerValue){
		if (com_maxFPS->integerValue > 0)
			minMsec = 1000 / com_maxFPS->integerValue;
	}

	// Main event loop
	do {
		com_frameTime = Sys_GetEvents();
		if (lastTime > com_frameTime)
			lastTime = com_frameTime;

		msec = com_frameTime - lastTime;
	} while (msec < minMsec);

	lastTime = com_frameTime;

	// Bump frame count
	com_frameCount++;

	// Modify msec
	if (!com_dedicated->integerValue && !com_timeDemo->integerValue){
		if (com_aviDemo->integerValue > 0)
			msec = 1000 / com_aviDemo->integerValue;
		else {
			if (com_fixedTime->integerValue)
				msec = com_fixedTime->integerValue;
			if (com_timeScale->floatValue)
				msec *= com_timeScale->floatValue;
		}
	}

	if (msec < 1)
		msec = 1;

	// Process commands
	Cbuf_Execute();

	// Check if cheats are allowed
	Com_SetAllowCheats();

	// Run server and client frames
	if (com_speeds->integerValue)
		com_timeBefore = Sys_Milliseconds();

	SV_Frame(msec);

	if (com_speeds->integerValue)
		com_timeBetween = Sys_Milliseconds();

	CL_Frame(msec);

	if (com_speeds->integerValue)
		com_timeAfter = Sys_Milliseconds();

	// Print com_speeds statistics
	if (com_speeds->integerValue){
		int			all, sv, gm, cl, rf, sn;

		all = com_timeAfter - com_timeBefore;
		sv = com_timeBetween - com_timeBefore;
		cl = com_timeAfter - com_timeBetween;
		gm = com_timeAfterGame - com_timeBeforeGame;
		rf = com_timeAfterRef - com_timeBeforeRef;
		sn = com_timeAfterSnd - com_timeBeforeSnd;

		sv -= gm;
		cl -= (rf + sn);

		Com_Printf("frame:%i all:%3i sv:%3i gm:%3i cl:%3i rf:%3i sn:%3i\n", com_frameCount, all, sv, gm, cl, rf, sn);
	}

	// Print cmodel statistics
	CM_PrintStats();
}

/*
 =================
 Com_Shutdown
 =================
*/
void Com_Shutdown (void){

	static qboolean	isDown;

	if (isDown)
		return;

	isDown = true;

	SV_Shutdown("Server quit\n", false);
	CL_Shutdown();
	NET_Shutdown();
	FS_Shutdown();
	CL_ShutdownKeys();
	Cvar_Shutdown();
	Cmd_Shutdown();
	Com_ShutdownMemory();
	Com_CloseLogFile();
}
