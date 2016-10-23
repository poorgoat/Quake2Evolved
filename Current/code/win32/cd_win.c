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


#include "winquake.h"
#include "../client/client.h"


typedef struct {
	qboolean			initialized;
	qboolean			enabled;

	qboolean			cdValid;
	int					cdVolume;

	qboolean			isPlaying;
	qboolean			wasPlaying;
	qboolean			playLooping;
	int					loopCounter;

	int					playTrack;
	int					maxTracks;
	byte				trackMap[100];

	UINT				wDeviceID;

	HMIXER				hMixer;
	MIXERLINE			mixerLine;
	MIXERLINECONTROLS	mixerLineControl;
	MIXERCONTROL		mixerControl;
} cdAudio_t;

static cdAudio_t	cdAudio;

cvar_t	*cd_noCD;
cvar_t	*cd_volume;
cvar_t	*cd_loopCount;
cvar_t	*cd_loopTrack;


/*
 =================
 CDAudio_Eject
 =================
*/
static void CDAudio_Eject (void){

	DWORD	dwReturn;

    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, (DWORD)NULL))
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_SET_DOOR_OPEN failed (0x%X)\n", dwReturn);
}

/*
 =================
 CDAudio_CloseDoor
 =================
*/
static void CDAudio_CloseDoor (void){

	DWORD	dwReturn;

    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, (DWORD)NULL))
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_SET_DOOR_CLOSED failed (0x%X)\n", dwReturn);
}

/*
 =================
 CDAudio_GetAudioDiskInfo
 =================
*/
static void CDAudio_GetAudioDiskInfo (void){

	DWORD				dwReturn;
	MCI_STATUS_PARMS	mciStatusParms;

	cdAudio.cdValid = false;
	cdAudio.maxTracks = 0;

	mciStatusParms.dwItem = MCI_STATUS_READY;
    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms)){
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_STATUS_READY failed (0x%X)\n", dwReturn);
		return;
	}
	if (!mciStatusParms.dwReturn){
		Com_DPrintf("CDAudio: drive not ready\n");
		return;
	}

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms)){
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_STATUS_NUMBER_OF_TRACKS failed (0x%X)\n", dwReturn);
		return;
	}
	if (mciStatusParms.dwReturn < 1){
		Com_DPrintf("CDAudio: no music tracks\n");
		return;
	}

	cdAudio.cdValid = true;
	cdAudio.maxTracks = mciStatusParms.dwReturn;
}

/*
 =================
 CDAudio_Play2
 =================
*/
static void CDAudio_Play2 (int track, qboolean looping){

	DWORD				dwReturn;
    MCI_PLAY_PARMS		mciPlayParms;
	MCI_STATUS_PARMS	mciStatusParms;

	if (!cdAudio.enabled)
		return;
	
	if (!cdAudio.cdValid){
		CDAudio_GetAudioDiskInfo();
		if (!cdAudio.cdValid)
			return;
	}

	track = cdAudio.trackMap[track];

	if (track < 1 || track > cdAudio.maxTracks){
		CDAudio_Stop();
		return;
	}

	if (cdAudio.isPlaying){
		if (cdAudio.playTrack == track)
			return;

		CDAudio_Stop();
	}

	// Don't try to play a non-audio track
	mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatusParms.dwTrack = track;
    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms)){
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_STATUS failed (0x%X)\n", dwReturn);
		return;
	}
	if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO){
		Com_DPrintf("CDAudio: track %i is not audio\n", track);
		return;
	}

	// Get the length of the track to be played
	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms)){
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_STATUS failed (0x%X)\n", dwReturn);
		return;
	}

    mciPlayParms.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = (mciStatusParms.dwReturn << 8) | track;
    mciPlayParms.dwCallback = (DWORD)sys.hWndMain;
    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)(LPVOID)&mciPlayParms)){
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_PLAY failed (0x%X)\n", dwReturn);
		return;
	}

	cdAudio.playLooping = looping;
	cdAudio.playTrack = track;
	cdAudio.isPlaying = true;
	cdAudio.wasPlaying = false;
}

/*
 =================
 CDAudio_Play
 =================
*/
void CDAudio_Play (int track, qboolean looping){

	if (!cdAudio.initialized)
		return;

	// Set a loop counter so that this track will change to the 
	// loop track later
	cdAudio.loopCounter = 0;

	CDAudio_Play2(track, looping);
}

/*
 =================
 CDAudio_Stop
 =================
*/
void CDAudio_Stop (void){

	DWORD	dwReturn;

	if (!cdAudio.initialized)
		return;

	if (!cdAudio.enabled)
		return;
	
	if (!cdAudio.isPlaying)
		return;

    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_STOP, 0, (DWORD)NULL))
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_STOP failed (0x%X)", dwReturn);

	cdAudio.isPlaying = false;
	cdAudio.wasPlaying = false;
}

/*
 =================
 CDAudio_Pause
 =================
*/
static void CDAudio_Pause (void){

	DWORD				dwReturn;
	MCI_GENERIC_PARMS	mciGenericParms;

	if (!cdAudio.enabled)
		return;

	if (!cdAudio.isPlaying)
		return;

	mciGenericParms.dwCallback = (DWORD)sys.hWndMain;
    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_PAUSE, 0, (DWORD)(LPVOID)&mciGenericParms))
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_PAUSE failed (0x%X)", dwReturn);

	cdAudio.isPlaying = false;
	cdAudio.wasPlaying = true;
}

/*
 =================
 CDAudio_Resume
 =================
*/
static void CDAudio_Resume (void){

	DWORD			dwReturn;
    MCI_PLAY_PARMS	mciPlayParms;

	if (!cdAudio.enabled)
		return;
	
	if (!cdAudio.cdValid)
		return;

	if (!cdAudio.wasPlaying)
		return;
	
    mciPlayParms.dwFrom = MCI_MAKE_TMSF(cdAudio.playTrack, 0, 0, 0);
    mciPlayParms.dwTo = MCI_MAKE_TMSF(cdAudio.playTrack + 1, 0, 0, 0);
    mciPlayParms.dwCallback = (DWORD)sys.hWndMain;
    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD)(LPVOID)&mciPlayParms)){
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_PLAY failed (0x%X)\n", dwReturn);
		return;
	}

	cdAudio.isPlaying = true;
	cdAudio.wasPlaying = false;
}

/*
 =================
 CDAudio_Info
 =================
*/
static void CDAudio_Info (void){

	if (!cdAudio.initialized){
		Com_Printf("CD Audio not initialized\n");
		return;
	}

	CDAudio_GetAudioDiskInfo();

	if (!cdAudio.cdValid){
		Com_Printf("No CD in player\n");
		return;
	}
	
	Com_Printf("%u tracks\n", cdAudio.maxTracks);

	if (cdAudio.isPlaying)
		Com_Printf("Currently %s track %i\n", cdAudio.playLooping ? "looping" : "playing", cdAudio.playTrack);
	else if (cdAudio.wasPlaying)
		Com_Printf("Paused %s track %i\n", cdAudio.playLooping ? "looping" : "playing", cdAudio.playTrack);
	else {
		if (cdAudio.enabled)
			Com_Printf("Stopped\n");
		else
			Com_Printf("Disabled\n");
	}
}

/*
 =================
 CDAudio_CD_f
 =================
*/
static void CDAudio_CD_f (void){

	char	*cmd;
	int		remap;
	int		i;

	if (Cmd_Argc() < 2){
		Com_Printf("Usage: cd <command> [arguments]\n");
		Com_Printf("Commands:\n");
		Com_Printf("   info\n");
		Com_Printf("   on\n");
		Com_Printf("   off\n");
		Com_Printf("   reset\n");
		Com_Printf("   remap [tracks]\n");
		Com_Printf("   close\n");
		Com_Printf("   eject\n");
		Com_Printf("   play <track>\n");
		Com_Printf("   loop <track>\n");
		Com_Printf("   stop\n");
		Com_Printf("   pause\n");
		Com_Printf("   resume\n");
		Com_Printf("   prev\n");
		Com_Printf("   next\n");
		return;
	}

	cmd = Cmd_Argv(1);

	if (!Q_stricmp(cmd, "info")){
		CDAudio_Info();
		return;
	}

	if (!cdAudio.initialized){
		Com_Printf("CD Audio not initialized\n");
		return;
	}

	if (!Q_stricmp(cmd, "on")){
		cdAudio.enabled = true;
		return;
	}

	if (!Q_stricmp(cmd, "off")){
		if (cdAudio.isPlaying)
			CDAudio_Stop();

		cdAudio.enabled = false;
		return;
	}

	if (!Q_stricmp(cmd, "reset")){
		cdAudio.enabled = true;
		
		if (cdAudio.isPlaying)
			CDAudio_Stop();
		
		for (i = 0; i < 100; i++)
			cdAudio.trackMap[i] = i;
		
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (!Q_stricmp(cmd, "remap")){
		remap = Cmd_Argc() - 2;
		
		if (remap <= 0){
			for (i = 1; i < 100; i++)
				if (cdAudio.trackMap[i] != i)
					Com_Printf("  %i -> %i\n", i, cdAudio.trackMap[i]);
			
			return;
		}
		
		for (i = 1; i <= remap; i++)
			cdAudio.trackMap[i] = atoi(Cmd_Argv(i+1));
		
		return;
	}

	if (!Q_stricmp(cmd, "close")){
		CDAudio_CloseDoor();
		return;
	}

	if (!Q_stricmp(cmd, "eject")){
		if (cdAudio.isPlaying)
			CDAudio_Stop();

		CDAudio_Eject();
		cdAudio.cdValid = false;
		return;
	}

	if (!cdAudio.cdValid){
		CDAudio_GetAudioDiskInfo();
		
		if (!cdAudio.cdValid){
			Com_Printf("No CD in player\n");
			return;
		}
	}

	if (!Q_stricmp(cmd, "play")){
		CDAudio_Play(atoi(Cmd_Argv(2)), false);
		return;
	}

	if (!Q_stricmp(cmd, "loop")){
		CDAudio_Play(atoi(Cmd_Argv(2)), true);
		return;
	}

	if (!Q_stricmp(cmd, "stop")){
		CDAudio_Stop();
		return;
	}

	if (!Q_stricmp(cmd, "pause")){
		CDAudio_Pause();
		return;
	}

	if (!Q_stricmp(cmd, "resume")){
		CDAudio_Resume();
		return;
	}

	if (!cdAudio.playTrack)
		return;

	if (!Q_stricmp(cmd, "prev")){
		i = cdAudio.playTrack - 1;
		if (i == 0)
			i = cdAudio.maxTracks;

		CDAudio_Play(i, cdAudio.playLooping);
		return;
	}
	
	if (!Q_stricmp(cmd, "next")){
		i = cdAudio.playTrack + 1;
		if (i > cdAudio.maxTracks)
			i = 1;

		CDAudio_Play(i, cdAudio.playLooping);
		return;
	}
}

/*
 =================
 CDAudio_MessageHandler
 =================
*/
LONG CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	if (lParam != cdAudio.wDeviceID)
		return 1;

	switch (wParam){
	case MCI_NOTIFY_SUCCESSFUL:
		if (cdAudio.isPlaying){
			cdAudio.isPlaying = false;

			if (cdAudio.playLooping){
				// If the track has played the given number of times, go
				// to the ambient track
				if (++cdAudio.loopCounter >= cd_loopCount->integerValue)
					CDAudio_Play2(cd_loopTrack->integerValue, true);
				else
					CDAudio_Play2(cdAudio.playTrack, true);
			}
		}

		break;
	case MCI_NOTIFY_FAILURE:
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_NOTIFY_FAILURE\n");

		CDAudio_Stop();

		cdAudio.cdValid = false;

		break;
	case MCI_NOTIFY_ABORTED:
	case MCI_NOTIFY_SUPERSEDED:

		break;
	default:
		Com_DPrintf(S_COLOR_RED "CDAudio: unexpected MM_MCINOTIFY type (0x%X)\n", wParam);

		return 1;
	}

	return 0;
}

/*
 =================
 CDAudio_MixerClose
 =================
*/
static void CDAudio_MixerClose (void){

	DWORD	dwResult;

	if (!cdAudio.hMixer)
		return;

	if (dwResult = mixerClose(cdAudio.hMixer))
		Com_DPrintf(S_COLOR_RED "CDAudio: mixerClose() failed (0x%X)\n", dwResult);

	cdAudio.hMixer = NULL;
}

/*
 =================
 CDAudio_MixerOpen
 =================
*/
static void CDAudio_MixerOpen (void){

	DWORD	dwResult;

	if (!mixerGetNumDevs()){
		Com_DPrintf("CDAudio: no mixer devices found\n");
		return;
	}

	// Open the device
	if (dwResult = mixerOpen(&cdAudio.hMixer, 0, 0, 0, MIXER_OBJECTF_MIXER)){
		Com_DPrintf(S_COLOR_RED "CDAudio: mixerOpen() failed (0x%X)\n", dwResult);
		return;
	}

	memset(&cdAudio.mixerLine, 0, sizeof(MIXERLINE));
	cdAudio.mixerLine.cbStruct = sizeof(MIXERLINE);
	cdAudio.mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;

	if (dwResult = mixerGetLineInfo((HMIXEROBJ)cdAudio.hMixer, &cdAudio.mixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE)){
		Com_DPrintf(S_COLOR_RED "CDAudio: mixerGetLineInfo() failed (0x%X)\n", dwResult);
		CDAudio_MixerClose();
		return;
	}

	memset(&cdAudio.mixerLineControl, 0, sizeof(MIXERLINECONTROLS));
	cdAudio.mixerLineControl.cbStruct = sizeof(MIXERLINECONTROLS);
	cdAudio.mixerLineControl.dwLineID = cdAudio.mixerLine.dwLineID;
	cdAudio.mixerLineControl.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	cdAudio.mixerLineControl.cControls = 1;
	cdAudio.mixerLineControl.cbmxctrl = sizeof(cdAudio.mixerControl);
	cdAudio.mixerLineControl.pamxctrl = &cdAudio.mixerControl;

	if (dwResult = mixerGetLineControls((HMIXEROBJ)cdAudio.hMixer, &cdAudio.mixerLineControl, MIXER_GETLINECONTROLSF_ONEBYTYPE)){
		Com_DPrintf(S_COLOR_RED "CDAudio: mixerGetLineControls() failed (0x%X)\n", dwResult);
		CDAudio_MixerClose();
		return;
	}
}

/*
 =================
 CDAudio_GetMixerVolume
 =================
*/
static int CDAudio_GetMixerVolume (void){

	DWORD							dwResult;
	MIXERCONTROLDETAILS				mxControlDetails;
	MIXERCONTROLDETAILS_UNSIGNED	mxValue[2];
	MIXERCONTROL					*mxControl;
	MIXERLINE						*mxLine;
	int								value;

	if (!cdAudio.hMixer)
		return 0;

	mxLine = &cdAudio.mixerLine;
	mxControl = &cdAudio.mixerControl;

	memset(&mxControlDetails, 0, sizeof(MIXERCONTROLDETAILS));
	mxControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
	mxControlDetails.dwControlID = mxControl->dwControlID;
	mxControlDetails.cChannels = mxLine->cChannels;
	mxControlDetails.cbDetails = sizeof(mxValue);
	mxControlDetails.paDetails = &mxValue;

	if (dwResult = mixerGetControlDetails((HMIXEROBJ)cdAudio.hMixer, &mxControlDetails, MIXER_GETCONTROLDETAILSF_VALUE)){
		Com_DPrintf(S_COLOR_RED "CDAudio: mixerGetControlDetails() failed (0x%X)\n", dwResult);
		return 0;
	}

	value = mxValue[(mxValue[0].dwValue > mxValue[1].dwValue) ? 0 : 1].dwValue;
	return (255 * (value - mxControl->Bounds.dwMinimum)) / (mxControl->Bounds.dwMaximum - mxControl->Bounds.dwMinimum);
}

/*
 =================
 CDAudio_SetMixerVolume
 =================
*/
static void CDAudio_SetMixerVolume (int volume){

	DWORD							dwResult;
	MIXERCONTROLDETAILS				mxControlDetails;
	MIXERCONTROLDETAILS_UNSIGNED	mxValue[2];
	MIXERCONTROL					*mxControl;
	MIXERLINE						*mxLine;

	if (!cdAudio.hMixer)
		return;

	mxLine = &cdAudio.mixerLine;
	mxControl = &cdAudio.mixerControl;

	memset(&mxControlDetails, 0, sizeof(MIXERCONTROLDETAILS));
	mxControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
	mxControlDetails.dwControlID = mxControl->dwControlID;
	mxControlDetails.cChannels = mxLine->cChannels;
	mxControlDetails.cbDetails = sizeof(mxValue);
	mxControlDetails.paDetails = &mxValue;

	mxValue[0].dwValue = mxValue[1].dwValue = (volume * (mxControl->Bounds.dwMaximum - mxControl->Bounds.dwMinimum)) / 255 + mxControl->Bounds.dwMinimum;
	if (dwResult = mixerSetControlDetails((HMIXEROBJ)cdAudio.hMixer, &mxControlDetails, MIXER_SETCONTROLDETAILSF_VALUE))
		Com_DPrintf(S_COLOR_RED "CDAudio: mixerSetControlDetails() failed (0x%X)\n", dwResult);
}

/*
 =================
 CDAudio_Update
 =================
*/
void CDAudio_Update (void){

	if (!cdAudio.initialized)
		return;

	if (cd_noCD->integerValue != !cdAudio.enabled){
		if (cd_noCD->integerValue){
			CDAudio_Pause();
			cdAudio.enabled = false;
		}
		else {
			cdAudio.enabled = true;
			CDAudio_Resume();
		}
	}

	if (cd_volume->modified){
		if (cd_volume->floatValue > 1.0)
			Cvar_SetFloat("cd_volume", 1.0);
		else if (cd_volume->floatValue < 0.0)
			Cvar_SetFloat("cd_volume", 0.0);

		CDAudio_SetMixerVolume(cd_volume->floatValue * 255);

		cd_volume->modified = false;
	}
}

/*
 =================
 CDAudio_Activate

 Called when the main windows gains or loses focus.
 The window have been destroyed and recreated between a deactivate and
 an activate.
 =================
*/
void CDAudio_Activate (qboolean active){

	if (!cdAudio.initialized)
		return;

	if (active)
		CDAudio_Resume();
	else
		CDAudio_Pause();
}

/*
 =================
 CDAudio_Startup
 =================
*/
static qboolean CDAudio_Startup (void){

	DWORD			dwReturn;
	MCI_OPEN_PARMS	mciOpenParms;
    MCI_SET_PARMS	mciSetParms;
	int				i;

	// Open the device
	mciOpenParms.lpstrDeviceType = "cdaudio";
	if (dwReturn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD)(LPVOID)&mciOpenParms)){
		Com_DPrintf(S_COLOR_RED "CDAudio: MCI_OPEN failed (0x%X)\n", dwReturn);
		return false;
	}
	cdAudio.wDeviceID = mciOpenParms.wDeviceID;

    // Set the time format to track/minute/second/frame (TMSF)
    mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
    if (dwReturn = mciSendCommand(cdAudio.wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID)&mciSetParms)){
    	Com_DPrintf(S_COLOR_RED "CDAudio: MCI_SET_TIME_FORMAT failed (0x%X)\n", dwReturn);
        mciSendCommand(cdAudio.wDeviceID, MCI_CLOSE, 0, (DWORD)NULL);
		return false;
    }

	cdAudio.initialized = true;

	cdAudio.enabled = !cd_noCD->integerValue;

	CDAudio_GetAudioDiskInfo();

	if (!cdAudio.cdValid)
		cdAudio.enabled = false;

	for (i = 0; i < 100; i++)
		cdAudio.trackMap[i] = i;

	// Open the mixer device and save original volume
	CDAudio_MixerOpen();
	cdAudio.cdVolume = CDAudio_GetMixerVolume();

	return true;
}

/*
 =================
 CDAudio_Init
 =================
*/
void CDAudio_Init (void){

	// Register our variables and commands
	cd_noCD = Cvar_Get("cd_noCD", "0", CVAR_ARCHIVE, "Don't play CD Audio tracks");
	cd_volume = Cvar_Get("cd_volume", "1.0", CVAR_ARCHIVE, "CD Audio volume");
	cd_loopCount = Cvar_Get("cd_loopCount", "4", CVAR_ARCHIVE, "Number of times to loop the current track");
	cd_loopTrack = Cvar_Get("cd_loopTrack", "11", CVAR_ARCHIVE, "Ambient loop track");

	Cmd_AddCommand("cd", CDAudio_CD_f, "Execute a CD Audio command");

	if (!CDAudio_Startup()){
		Com_Printf(S_COLOR_YELLOW "WARNING: CD Audio initialization failed\n");
		return;
	}

	Com_Printf("CD Audio Initialized\n");

	CDAudio_Info();
}

/*
 =================
 CDAudio_Shutdown
 =================
*/
void CDAudio_Shutdown (void){

	DWORD	dwResult;

	Cmd_RemoveCommand("cd");

	if (cdAudio.initialized){
		// Stop
		CDAudio_Stop();

		// Restore original volume and close the mixer device
		CDAudio_SetMixerVolume(cdAudio.cdVolume);
		CDAudio_MixerClose();

		// Close the device
		if (dwResult = mciSendCommand(cdAudio.wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)NULL))
			Com_DPrintf(S_COLOR_RED "CDAudio: MCI_CLOSE failed (0x%X)\n", dwResult);
	}

	memset(&cdAudio, 0, sizeof(cdAudio_t));
}
