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


#define	MAX_PRINTMSG	8192
#define MAX_NUM_ARGVS	64

static int		com_argc;
static char		*com_argv[MAX_NUM_ARGVS];

static int		com_rdTarget;
static char		*com_rdBuffer;
static int		com_rdBufferSize;
static void		(*com_rdFlush)(int target, char *buffer);

static FILE		*com_logFileHandle;

static int		com_serverState;

static jmp_buf	com_abortFrame;

qboolean		com_configModified = false;

int				com_timeBefore, com_timeBetween, com_timeAfter;
int				com_timeBeforeGame, com_timeAfterGame;
int				com_timeBeforeRef, com_timeAfterRef;

cvar_t	*developer;
cvar_t	*dedicated;
cvar_t	*paused;
cvar_t	*timedemo;
cvar_t	*fixedtime;
cvar_t	*timescale;
cvar_t	*com_aviDemo;
cvar_t	*com_forceAviDemo;
cvar_t	*com_showTrace;
cvar_t	*com_speeds;
cvar_t	*com_debugMemory;
cvar_t	*com_zoneMegs;
cvar_t	*com_hunkMegs;
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
void Com_Redirect (const char *msg){

	if (!com_rdTarget)
		return;

	if ((strlen(msg) + strlen(com_rdBuffer)) > (com_rdBufferSize - 1)){
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
	struct tm	*ltime;
	char		str[64];

	if (com_logFileHandle)
		return;

	if (com_logFile->integer == 1 || com_logFile->integer == 2)
		com_logFileHandle = fopen("qconsole.log", "wt");
	else if (com_logFile->integer == 3 || com_logFile->integer == 4)
		com_logFileHandle = fopen("qconsole.log", "at");

	if (!com_logFileHandle)
		return;

	time(&clock);
	ltime = localtime(&clock);
	strftime(str, sizeof(str), "%a %b %d %H:%M:%S %Y", ltime);
		
	fprintf(com_logFileHandle, "\n*** Log file opened on %s ***\n\n", str);
}

/*
 =================
 Com_CloseLogFile
 =================
*/
static void Com_CloseLogFile (void){

	time_t		clock;
	struct tm	*ltime;
	char		str[64];

	if (!com_logFileHandle)
		return;

	time(&clock);
	ltime = localtime(&clock);
	strftime(str, sizeof(str), "%a %b %d %H:%M:%S %Y", ltime);

	fprintf(com_logFileHandle, "\n*** Log file closed on %s ***\n\n", str);
			
	fclose(com_logFileHandle);
	com_logFileHandle = NULL;
}

/*
 =================
 Com_LogFile
 =================
*/
static void Com_LogFile (const char *text){

	if (!com_logFileHandle)
		return;

	fprintf(com_logFileHandle, "%s", text);

	if (com_logFile->integer == 2 || com_logFile->integer == 4)
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
	vsnprintf(string, sizeof(string), fmt, argPtr);
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
		if (com_logFile->integer){
			Com_OpenLogFile();
			Com_LogFile(string);
		}
		else
			Com_CloseLogFile();
	}
}

/*
 =================
 Com_DPrintf

 A Com_Printf that only shows up if the "developer" cvar is set
 =================
*/
void Com_DPrintf (const char *fmt, ...){

	char	string[MAX_PRINTMSG];
	va_list	argPtr;

	if (!developer || !developer->integer)
		return;		// Don't confuse non-developers with techie stuff...

	va_start(argPtr, fmt);
	vsnprintf(string, sizeof(string), fmt, argPtr);
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

	static qboolean		recursive;
	static char			string[MAX_PRINTMSG];
	va_list				argPtr;

	if (recursive)
		Sys_Error("Recursive error after: %s", string);
	recursive = true;

	va_start(argPtr, fmt);
	vsnprintf(string, sizeof(string), fmt, argPtr);
	va_end(argPtr);

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
 Com_WriteConfig
 =================
*/
void Com_WriteConfig (const char *name){

	fileHandle_t	f;

	FS_FOpenFile(name, &f, FS_WRITE);
	if (!f){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}

	FS_Printf(f, "// Automatically generated by Quake II Evolved, modifications will be reset!\r\n\r\n");

	Key_WriteBindings(f);
	Cvar_WriteVariables(f);

	FS_FCloseFile(f);
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
 Com_Setenv_f
 =================
*/
void Com_Setenv_f (void){

	char	buffer[1024], *env;
	int		i;

	if (Cmd_Argc() < 2){
		Com_Printf("Usage: setenv <variable> [value]\n");
		return;
	}

	if (Cmd_Argc() == 2){
		env = getenv(Cmd_Argv(1));
		if (env)
			Com_Printf("%s=%s\n", Cmd_Argv(1), env);
		else
			Com_Printf("%s undefined\n", Cmd_Argv(1), env);
	}
	else {
		Q_strncpyz(buffer, Cmd_Argv(1), sizeof(buffer));
		Q_strncatz(buffer, "=", sizeof(buffer));

		for (i = 2; i < Cmd_Argc(); i++){
			Q_strncatz(buffer, Cmd_Argv(i), sizeof(buffer));
			Q_strncatz(buffer, " ", sizeof(buffer));
		}

		putenv(buffer);
	}
}

/*
 =================
 Com_WriteConfig_f
 =================
*/
void Com_WriteConfig_f (void){

	char	name[MAX_QPATH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: writeconfig <filename>\n");
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
	if (!com_serverState || Cvar_VariableInteger("maxclients") > 1){
		Cvar_ForceSet("paused", "0");
		return;
	}

	Cvar_SetInteger("paused", !paused->integer);
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

	Com_Printf("%s %s (%s)\n", Q2E_VERSION, BUILDSTRING, __DATE__);

	// Parse the command line
	Com_ParseCommandLine(cmdLine);

	// We need to call Com_InitMemory twice, because some strings and
	// structs need to be allocated during initialization, but we want
	// to get the amount of memory to allocate for the main zone and
	// hunk from the config files
	Com_InitMemory();

	// Prepare enough of the subsystems to handle commands and cvars
	Cmd_Init();
	Cvar_Init();
	Key_Init();

	// We need to add the early commands twice, because some file system
	// cvars need to be set before execing config files, but we want
	// other parms to override the settings of the config files
	Com_AddEarlyCommands(false);
	Cbuf_Execute();

	// Initialize file system
	FS_Init();

	Com_AddEarlyCommands(true);
	Cbuf_Execute();

	// Register cvars and commands
	Cvar_Get("version", va("%s %s (%s)", Q2E_VERSION, BUILDSTRING, __DATE__), CVAR_SERVERINFO | CVAR_ROM);
	developer = Cvar_Get("developer", "0", 0);
	dedicated = Cvar_Get("dedicated", "0", CVAR_INIT);
	paused = Cvar_Get("paused", "0", CVAR_CHEAT);
	timedemo = Cvar_Get("timedemo", "0", CVAR_CHEAT);
	fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
	timescale = Cvar_Get("timescale", "1", CVAR_CHEAT);
	com_aviDemo = Cvar_Get("com_aviDemo", "0", CVAR_CHEAT);
	com_forceAviDemo = Cvar_Get("com_forceAviDemo", "0", CVAR_CHEAT);
	com_showTrace = Cvar_Get("com_showTrace", "0", CVAR_CHEAT);
	com_speeds = Cvar_Get("com_speeds", "0", CVAR_CHEAT);
	com_debugMemory = Cvar_Get("com_debugMemory", "0", CVAR_CHEAT);
	com_zoneMegs = Cvar_Get("com_zoneMegs", "16", CVAR_ARCHIVE | CVAR_LATCH);
	com_hunkMegs = Cvar_Get("com_hunkMegs", "48", CVAR_ARCHIVE | CVAR_LATCH);
	com_logFile = Cvar_Get("com_logFile", "0", 0);

	Cmd_AddCommand("quit", Com_Quit_f);
	Cmd_AddCommand("error", Com_Error_f);
	Cmd_AddCommand("setenv", Com_Setenv_f);
	Cmd_AddCommand("writeconfig", Com_WriteConfig_f);
	Cmd_AddCommand("pause", Com_Pause_f);
	Cmd_AddCommand("meminfo", Com_MemInfo_f);

	// Initialize main zone and hunk memory
	Com_InitMemory();

	// Initialize the rest of the subsystems
	Sys_Init();

	SV_Init();
	CL_Init();

	NET_Init();
	NetChan_Init();

	Com_Printf("======= Quake II Evolved Initialized =======\n");

	// Add the late commands
	if (dedicated->integer)
		Com_AddLateCommands();
	else {
		// Hide console
		Sys_ShowConsole(false);

		// If the user didn't give any commands, play the logo cinematic
		if (!Com_AddLateCommands())
			Cbuf_AddText("cinematic idlog.cin\n");
	}

	Cbuf_Execute();
}

/*
 =================
 Com_Frame
 =================
*/
void Com_Frame (int msec){

	if (setjmp(com_abortFrame))
		return;			// An error occurred, exit the entire frame

	if (!timedemo->integer){
		if (com_aviDemo->integer > 0)
			msec = 1000 / com_aviDemo->integer;
		else {
			if (fixedtime->integer)
				msec = fixedtime->integer;
			if (timescale->value)
				msec *= timescale->value;
		}
	}

	if (msec < 1)
		msec = 1;

	// Update the config file if needed
	if (com_configModified){
		com_configModified = false;

		Com_WriteConfig("q2econfig.cfg");
	}

	// Print trace statistics
	if (com_showTrace->integer){
		extern int	cm_traces, cm_pointContents;

		Com_Printf("%4i traces %4i points\n", cm_traces, cm_pointContents);
		cm_traces = cm_pointContents = 0;
	}

	// Get input from dedicated server console
	if (dedicated->integer){
		char	*cmd;

		cmd = Sys_GetCommand();
		if (cmd){
			Cbuf_AddText(cmd);
			Cbuf_AddText("\n");
		}
	}

	Cbuf_Execute();

	if (com_speeds->integer)
		com_timeBefore = Sys_Milliseconds();

	SV_Frame(msec);

	if (com_speeds->integer)
		com_timeBetween = Sys_Milliseconds();

	CL_Frame(msec);

	if (com_speeds->integer)
		com_timeAfter = Sys_Milliseconds();

	// Print com_speeds statistics
	if (com_speeds->integer){
		int		all, sv, gm, cl, rf;

		all = com_timeAfter - com_timeBefore;
		sv = com_timeBetween - com_timeBefore;
		cl = com_timeAfter - com_timeBetween;
		gm = com_timeAfterGame - com_timeBeforeGame;
		rf = com_timeAfterRef - com_timeBeforeRef;
		sv -= gm;
		cl -= rf;

		Com_Printf("all:%3i sv:%3i gm:%3i cl:%3i rf:%3i\n", all, sv, gm, cl, rf);
	}	
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
	Key_Shutdown();
	Cvar_Shutdown();
	Cmd_Shutdown();

	Com_ShutdownMemory();

	Com_CloseLogFile();
}
