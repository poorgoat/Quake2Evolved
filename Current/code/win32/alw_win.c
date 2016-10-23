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


#include "../client/s_local.h"


alConfig_t	alConfig;
alwState_t	alwState;


/*
 =================
 ALW_GetProcAddress
 =================
*/
static void *ALW_GetProcAddress (const char *funcName){

	void	*funcAddress;

	funcAddress = qalGetProcAddress(funcName);
	if (!funcAddress){
		ALW_Shutdown();

		Com_Error(ERR_FATAL, "ALW_GetProcAddress: alGetProcAddress() failed for '%s'", funcName);
	}

	return funcAddress;
}

/*
 =================
 ALW_InitExtensions
 =================
*/
static void ALW_InitExtensions (void){

	Com_Printf("Initializing OpenAL extensions\n");
}

/*
 =================
 ALW_InitDriver
 =================
*/
static qboolean ALW_InitDriver (void){

	char	*deviceName = NULL;

	Com_Printf("Initializing OpenAL driver\n");

	// Open the device
	if (s_deviceName->value[0])
		deviceName = s_deviceName->value;

	Com_Printf("...opening device: ");
	if ((alwState.hDevice = qalcOpenDevice(deviceName)) == NULL){
		Com_Printf("failed\n");
		return true;
	}
	Com_Printf("succeeded\n");

	// Create the AL context and make it current
	Com_Printf("...creating AL context: ");
	if ((alwState.hALC = qalcCreateContext(alwState.hDevice, NULL)) == NULL){
		Com_Printf("failed\n");

		qalcCloseDevice(alwState.hDevice);
		alwState.hDevice = NULL;

		return false;
	}
	Com_Printf("succeeded\n");

	Com_Printf("...making context current: ");
	if (!qalcMakeContextCurrent(alwState.hALC)){
		Com_Printf("failed\n");

		qalcDestroyContext(alwState.hALC);
		alwState.hALC = NULL;

		qalcCloseDevice(alwState.hDevice);
		alwState.hDevice = NULL;

		return false;
	}
	Com_Printf("succeeded\n");

	return true;
}

/*
 =================
 ALW_StartOpenAL
 =================
*/
static void ALW_StartOpenAL (void){

	// Initialize our QAL dynamic bindings
	if (!QAL_Init(s_alDriver->value))
		Com_Error(ERR_FATAL, "ALW_StartOpenAL: could not load OpenAL subsystem");

	// Get device list
	if (qalcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
		alConfig.deviceList = qalcGetString(NULL, ALC_DEVICE_SPECIFIER);
	else
		alConfig.deviceList = "Generic Hardware\0Generic Software\0\0";

	// Initialize the device, context, etc...
	if (!ALW_InitDriver()){
		QAL_Shutdown();

		Com_Error(ERR_FATAL, "ALW_StartOpenAL: could not load OpenAL subsystem");
	}

	// Get AL strings
	alConfig.vendorString = qalGetString(AL_VENDOR);
	alConfig.rendererString = qalGetString(AL_RENDERER);
	alConfig.versionString = qalGetString(AL_VERSION);
	alConfig.extensionsString = qalGetString(AL_EXTENSIONS);

	// Get ALC strings
	alConfig.alcExtensionsString = qalcGetString(alwState.hDevice, ALC_EXTENSIONS);

	// Get device name
	alConfig.deviceName = qalcGetString(alwState.hDevice, ALC_DEVICE_SPECIFIER);
}

/*
 =================
 ALW_Init
 =================
*/
void ALW_Init (void){

	Com_Printf("Initializing OpenAL subsystem\n");

	// Initialize OpenAL subsystem
	ALW_StartOpenAL();

	// Initialize extensions
	ALW_InitExtensions();
}

/*
 =================
 ALW_Shutdown
 =================
*/
void ALW_Shutdown (void){

	Com_Printf("Shutting down OpenAL subsystem\n");

	if (alwState.hALC){
		if (qalcMakeContextCurrent){
			Com_Printf("...alcMakeContextCurrent( NULL ): ");
			if (!qalcMakeContextCurrent(NULL))
				Com_Printf("failed\n");
			else
				Com_Printf("succeeded\n");
		}

		if (qalcDestroyContext){
			Com_Printf("...destroying AL context\n");
			qalcDestroyContext(alwState.hALC);
		}

		alwState.hALC = NULL;
	}

	if (alwState.hDevice){
		if (qalcCloseDevice){
			Com_Printf("...closing device\n");
			qalcCloseDevice(alwState.hDevice);
		}

		alwState.hDevice = NULL;
	}

	QAL_Shutdown();

	memset(&alConfig, 0, sizeof(alConfig_t));
	memset(&alwState, 0, sizeof(alwState_t));
}
