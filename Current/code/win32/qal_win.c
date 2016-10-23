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


// qal_win.c -- binding of AL to QAL function pointers


#include "../client/s_local.h"


ALCCAPTURECLOSEDEVICE		qalcCaptureCloseDevice;
ALCCAPTUREOPENDEVICE		qalcCaptureOpenDevice;
ALCCAPTURESAMPLES			qalcCaptureSamples;
ALCCAPTURESTART				qalcCaptureStart;
ALCCAPTURESTOP				qalcCaptureStop;
ALCCLOSEDEVICE				qalcCloseDevice;
ALCCREATECONTEXT			qalcCreateContext;
ALCDESTROYCONTEXT			qalcDestroyContext;
ALCGETCONTEXTSDEVICE		qalcGetContextsDevice;
ALCGETCURRENTCONTEXT		qalcGetCurrentContext;
ALCGETENUMVALUE				qalcGetEnumValue;
ALCGETERROR					qalcGetError;
ALCGETINTEGERV				qalcGetIntegerv;
ALCGETPROCADDRESS			qalcGetProcAddress;
ALCGETSTRING				qalcGetString;
ALCISEXTENSIONPRESENT		qalcIsExtensionPresent;
ALCMAKECONTEXTCURRENT		qalcMakeContextCurrent;
ALCOPENDEVICE				qalcOpenDevice;
ALCPROCESSCONTEXT			qalcProcessContext;
ALCSUSPENDCONTEXT			qalcSuspendContext;

ALBUFFERDATA				qalBufferData;
ALBUFFER3F					qalBuffer3f;
ALBUFFER3I					qalBuffer3i;
ALBUFFERF					qalBufferf;
ALBUFFERFV					qalBufferfv;
ALBUFFERI					qalBufferi;
ALBUFFERIV					qalBufferiv;
ALDELETEBUFFERS				qalDeleteBuffers;
ALDELETESOURCES				qalDeleteSources;
ALDISABLE					qalDisable;
ALDISTANCEMODEL				qalDistanceModel;
ALDOPPLERFACTOR				qalDopplerFactor;
ALDOPPLERVELOCITY			qalDopplerVelocity;
ALENABLE					qalEnable;
ALGENBUFFERS				qalGenBuffers;
ALGENSOURCES				qalGenSources;
ALGETBOOLEAN				qalGetBoolean;
ALGETBOOLEANV				qalGetBooleanv;
ALGETBUFFER3F				qalGetBuffer3f;
ALGETBUFFER3I				qalGetBuffer3i;
ALGETBUFFERF				qalGetBufferf;
ALGETBUFFERFV				qalGetBufferfv;
ALGETBUFFERI				qalGetBufferi;
ALGETBUFFERIV				qalGetBufferiv;
ALGETDOUBLE					qalGetDouble;
ALGETDOUBLEV				qalGetDoublev;
ALGETENUMVALUE				qalGetEnumValue;
ALGETERROR					qalGetError;
ALGETFLOAT					qalGetFloat;
ALGETFLOATV					qalGetFloatv;
ALGETINTEGER				qalGetInteger;
ALGETINTEGERV				qalGetIntegerv;
ALGETLISTENER3F				qalGetListener3f;
ALGETLISTENER3I				qalGetListener3i;
ALGETLISTENERF				qalGetListenerf;
ALGETLISTENERFV				qalGetListenerfv;
ALGETLISTENERI				qalGetListeneri;
ALGETLISTENERIV				qalGetListeneriv;
ALGETPROCADDRESS			qalGetProcAddress;
ALGETSOURCE3F				qalGetSource3f;
ALGETSOURCE3I				qalGetSource3i;
ALGETSOURCEF				qalGetSourcef;
ALGETSOURCEFV				qalGetSourcefv;
ALGETSOURCEI				qalGetSourcei;
ALGETSOURCEIV				qalGetSourceiv;
ALGETSTRING					qalGetString;
ALISBUFFER					qalIsBuffer;
ALISENABLED					qalIsEnabled;
ALISEXTENSIONPRESENT		qalIsExtensionPresent;
ALISSOURCE					qalIsSource;
ALLISTENER3F				qalListener3f;
ALLISTENER3I				qalListener3i;
ALLISTENERF					qalListenerf;
ALLISTENERFV				qalListenerfv;
ALLISTENERI					qalListeneri;
ALLISTENERIV				qalListeneriv;
ALSOURCE3F					qalSource3f;
ALSOURCE3I					qalSource3i;
ALSOURCEF					qalSourcef;
ALSOURCEFV					qalSourcefv;
ALSOURCEI					qalSourcei;
ALSOURCEIV					qalSourceiv;
ALSOURCEPAUSE				qalSourcePause;
ALSOURCEPAUSEV				qalSourcePausev;
ALSOURCEPLAY				qalSourcePlay;
ALSOURCEPLAYV				qalSourcePlayv;
ALSOURCEQUEUEBUFFERS		qalSourceQueueBuffers;
ALSOURCEREWIND				qalSourceRewind;
ALSOURCEREWINDV				qalSourceRewindv;
ALSOURCESTOP				qalSourceStop;
ALSOURCESTOPV				qalSourceStopv;
ALSOURCEUNQUEUEBUFFERS		qalSourceUnqueueBuffers;
ALSPEEDOFSOUND				qalSpeedOfSound;

// =====================================================================

/*
 =================
 QAL_GetProcAddress
 =================
*/
static void *QAL_GetProcAddress (const char *funcName){

	void	*funcAddress;

	funcAddress = GetProcAddress(alwState.hInstOpenAL, funcName);
	if (!funcAddress){
		FreeLibrary(alwState.hInstOpenAL);
		alwState.hInstOpenAL = NULL;

		Com_Error(ERR_FATAL, "QAL_GetProcAddress: GetProcAddress() failed for '%s'", funcName);
	}

	return funcAddress;
}

/*
 =================
 QAL_Shutdown

 Unloads the specified DLL then nulls out all the proc pointers
 =================
*/
void QAL_Shutdown (void){

	Com_Printf("...shutting down QAL\n");

	if (alwState.hInstOpenAL){
		Com_Printf("...unloading OpenAL DLL\n");

		FreeLibrary(alwState.hInstOpenAL);
		alwState.hInstOpenAL = NULL;
	}

	qalcCaptureCloseDevice		= NULL;
	qalcCaptureOpenDevice		= NULL;
	qalcCaptureSamples			= NULL;
	qalcCaptureStart			= NULL;
	qalcCaptureStop				= NULL;
	qalcCloseDevice				= NULL;
	qalcCreateContext			= NULL;
	qalcDestroyContext			= NULL;
	qalcGetContextsDevice		= NULL;
	qalcGetCurrentContext		= NULL;
	qalcGetEnumValue			= NULL;
	qalcGetError				= NULL;
	qalcGetIntegerv				= NULL;
	qalcGetProcAddress			= NULL;
	qalcGetString				= NULL;
	qalcIsExtensionPresent		= NULL;
	qalcMakeContextCurrent		= NULL;
	qalcOpenDevice				= NULL;
	qalcProcessContext			= NULL;
	qalcSuspendContext			= NULL;

	qalBufferData				= NULL;
	qalBuffer3f					= NULL;
	qalBuffer3i					= NULL;
	qalBufferf					= NULL;
	qalBufferfv					= NULL;
	qalBufferi					= NULL;
	qalBufferiv					= NULL;
	qalDeleteBuffers			= NULL;
	qalDeleteSources			= NULL;
	qalDisable					= NULL;
	qalDistanceModel			= NULL;
	qalDopplerFactor			= NULL;
	qalDopplerVelocity			= NULL;
	qalEnable					= NULL;
	qalGenBuffers				= NULL;
	qalGenSources				= NULL;
	qalGetBoolean				= NULL;
	qalGetBooleanv				= NULL;
	qalGetBuffer3f				= NULL;
	qalGetBuffer3i				= NULL;
	qalGetBufferf				= NULL;
	qalGetBufferfv				= NULL;
	qalGetBufferi				= NULL;
	qalGetBufferiv				= NULL;
	qalGetDouble				= NULL;
	qalGetDoublev				= NULL;
	qalGetEnumValue				= NULL;
	qalGetError					= NULL;
	qalGetFloat					= NULL;
	qalGetFloatv				= NULL;
	qalGetInteger				= NULL;
	qalGetIntegerv				= NULL;
	qalGetListener3f			= NULL;
	qalGetListener3i			= NULL;
	qalGetListenerf				= NULL;
	qalGetListenerfv			= NULL;
	qalGetListeneri				= NULL;
	qalGetListeneriv			= NULL;
	qalGetProcAddress			= NULL;
	qalGetSource3f				= NULL;
	qalGetSource3i				= NULL;
	qalGetSourcef				= NULL;
	qalGetSourcefv				= NULL;
	qalGetSourcei				= NULL;
	qalGetSourceiv				= NULL;
	qalGetString				= NULL;
	qalIsBuffer					= NULL;
	qalIsEnabled				= NULL;
	qalIsExtensionPresent		= NULL;
	qalIsSource					= NULL;
	qalListener3f				= NULL;
	qalListener3i				= NULL;
	qalListenerf				= NULL;
	qalListenerfv				= NULL;
	qalListeneri				= NULL;
	qalListeneriv				= NULL;
	qalSource3f					= NULL;
	qalSource3i					= NULL;
	qalSourcef					= NULL;
	qalSourcefv					= NULL;
	qalSourcei					= NULL;
	qalSourceiv					= NULL;
	qalSourcePause				= NULL;
	qalSourcePausev				= NULL;
	qalSourcePlay				= NULL;
	qalSourcePlayv				= NULL;
	qalSourceQueueBuffers		= NULL;
	qalSourceRewind				= NULL;
	qalSourceRewindv			= NULL;
	qalSourceStop				= NULL;
	qalSourceStopv				= NULL;
	qalSourceUnqueueBuffers		= NULL;
	qalSpeedOfSound				= NULL;
}

/*
 =================
 QAL_Init

 Binds our QAL function pointers to the appropriate AL stuff
 =================
*/
qboolean QAL_Init (const char *driver){

	char	name[MAX_OSPATH], path[MAX_OSPATH];

	Com_Printf("...initializing QAL\n");

	Q_snprintfz(name, sizeof(name), "%s.dll", driver);
	if (!SearchPath(NULL, name, NULL, sizeof(path), path, NULL)){
		Com_Printf("...WARNING: couldn't find OpenAL driver '%s'\n", name);
		return false;
	}

	Com_Printf("...calling LoadLibrary( '%s' ): ", path);
	if ((alwState.hInstOpenAL = LoadLibrary(path)) == NULL){
		Com_Printf("failed\n");
		return false;
	}

	Com_Printf("succeeded\n");

	qalcCaptureCloseDevice		= (ALCCAPTURECLOSEDEVICE)QAL_GetProcAddress("alcCaptureCloseDevice");
	qalcCaptureOpenDevice		= (ALCCAPTUREOPENDEVICE)QAL_GetProcAddress("alcCaptureOpenDevice");
	qalcCaptureSamples			= (ALCCAPTURESAMPLES)QAL_GetProcAddress("alcCaptureSamples");
	qalcCaptureStart			= (ALCCAPTURESTART)QAL_GetProcAddress("alcCaptureStart");
	qalcCaptureStop				= (ALCCAPTURESTOP)QAL_GetProcAddress("alcCaptureStop");
	qalcCloseDevice				= (ALCCLOSEDEVICE)QAL_GetProcAddress("alcCloseDevice");
	qalcCreateContext			= (ALCCREATECONTEXT)QAL_GetProcAddress("alcCreateContext");
	qalcDestroyContext			= (ALCDESTROYCONTEXT)QAL_GetProcAddress("alcDestroyContext");
	qalcGetContextsDevice		= (ALCGETCONTEXTSDEVICE)QAL_GetProcAddress("alcGetContextsDevice");
	qalcGetCurrentContext		= (ALCGETCURRENTCONTEXT)QAL_GetProcAddress("alcGetCurrentContext");
	qalcGetEnumValue			= (ALCGETENUMVALUE)QAL_GetProcAddress("alcGetEnumValue");
	qalcGetError				= (ALCGETERROR)QAL_GetProcAddress("alcGetError");
	qalcGetIntegerv				= (ALCGETINTEGERV)QAL_GetProcAddress("alcGetIntegerv");
	qalcGetProcAddress			= (ALCGETPROCADDRESS)QAL_GetProcAddress("alcGetProcAddress");
	qalcGetString				= (ALCGETSTRING)QAL_GetProcAddress("alcGetString");
	qalcIsExtensionPresent		= (ALCISEXTENSIONPRESENT)QAL_GetProcAddress("alcIsExtensionPresent");
	qalcMakeContextCurrent		= (ALCMAKECONTEXTCURRENT)QAL_GetProcAddress("alcMakeContextCurrent");
	qalcOpenDevice				= (ALCOPENDEVICE)QAL_GetProcAddress("alcOpenDevice");
	qalcProcessContext			= (ALCPROCESSCONTEXT)QAL_GetProcAddress("alcProcessContext");
	qalcSuspendContext			= (ALCSUSPENDCONTEXT)QAL_GetProcAddress("alcSuspendContext");
	qalBufferData				= (ALBUFFERDATA)QAL_GetProcAddress("alBufferData");
	qalBuffer3f					= (ALBUFFER3F)QAL_GetProcAddress("alBuffer3f");
	qalBuffer3i					= (ALBUFFER3I)QAL_GetProcAddress("alBuffer3i");
	qalBufferf					= (ALBUFFERF)QAL_GetProcAddress("alBufferf");
	qalBufferfv					= (ALBUFFERFV)QAL_GetProcAddress("alBufferfv");
	qalBufferi					= (ALBUFFERI)QAL_GetProcAddress("alBufferi");
	qalBufferiv					= (ALBUFFERIV)QAL_GetProcAddress("alBufferiv");
	qalDeleteBuffers			= (ALDELETEBUFFERS)QAL_GetProcAddress("alDeleteBuffers");
	qalDeleteSources			= (ALDELETESOURCES)QAL_GetProcAddress("alDeleteSources");
	qalDisable					= (ALDISABLE)QAL_GetProcAddress("alDisable");
	qalDistanceModel			= (ALDISTANCEMODEL)QAL_GetProcAddress("alDistanceModel");
	qalDopplerFactor			= (ALDOPPLERFACTOR)QAL_GetProcAddress("alDopplerFactor");
	qalDopplerVelocity			= (ALDOPPLERVELOCITY)QAL_GetProcAddress("alDopplerVelocity");
	qalEnable					= (ALENABLE)QAL_GetProcAddress("alEnable");
	qalGenBuffers				= (ALGENBUFFERS)QAL_GetProcAddress("alGenBuffers");
	qalGenSources				= (ALGENSOURCES)QAL_GetProcAddress("alGenSources");
	qalGetBoolean				= (ALGETBOOLEAN)QAL_GetProcAddress("alGetBoolean");
	qalGetBooleanv				= (ALGETBOOLEANV)QAL_GetProcAddress("alGetBooleanv");
	qalGetBuffer3f				= (ALGETBUFFER3F)QAL_GetProcAddress("alGetBuffer3f");
	qalGetBuffer3i				= (ALGETBUFFER3I)QAL_GetProcAddress("alGetBuffer3i");
	qalGetBufferf				= (ALGETBUFFERF)QAL_GetProcAddress("alGetBufferf");
	qalGetBufferfv				= (ALGETBUFFERFV)QAL_GetProcAddress("alGetBufferfv");
	qalGetBufferi				= (ALGETBUFFERI)QAL_GetProcAddress("alGetBufferi");
	qalGetBufferiv				= (ALGETBUFFERIV)QAL_GetProcAddress("alGetBufferiv");
	qalGetDouble				= (ALGETDOUBLE)QAL_GetProcAddress("alGetDouble");
	qalGetDoublev				= (ALGETDOUBLEV)QAL_GetProcAddress("alGetDoublev");
	qalGetEnumValue				= (ALGETENUMVALUE)QAL_GetProcAddress("alGetEnumValue");
	qalGetError					= (ALGETERROR)QAL_GetProcAddress("alGetError");
	qalGetFloat					= (ALGETFLOAT)QAL_GetProcAddress("alGetFloat");
	qalGetFloatv				= (ALGETFLOATV)QAL_GetProcAddress("alGetFloatv");
	qalGetInteger				= (ALGETINTEGER)QAL_GetProcAddress("alGetInteger");
	qalGetIntegerv				= (ALGETINTEGERV)QAL_GetProcAddress("alGetIntegerv");
	qalGetListener3f			= (ALGETLISTENER3F)QAL_GetProcAddress("alGetListener3f");
	qalGetListener3i			= (ALGETLISTENER3I)QAL_GetProcAddress("alGetListener3i");
	qalGetListenerf				= (ALGETLISTENERF)QAL_GetProcAddress("alGetListenerf");
	qalGetListenerfv			= (ALGETLISTENERFV)QAL_GetProcAddress("alGetListenerfv");
	qalGetListeneri				= (ALGETLISTENERI)QAL_GetProcAddress("alGetListeneri");
	qalGetListeneriv			= (ALGETLISTENERIV)QAL_GetProcAddress("alGetListeneriv");
	qalGetProcAddress			= (ALGETPROCADDRESS)QAL_GetProcAddress("alGetProcAddress");
	qalGetSource3f				= (ALGETSOURCE3F)QAL_GetProcAddress("alGetSource3f");
	qalGetSource3i				= (ALGETSOURCE3I)QAL_GetProcAddress("alGetSource3i");
	qalGetSourcef				= (ALGETSOURCEF)QAL_GetProcAddress("alGetSourcef");
	qalGetSourcefv				= (ALGETSOURCEFV)QAL_GetProcAddress("alGetSourcefv");
	qalGetSourcei				= (ALGETSOURCEI)QAL_GetProcAddress("alGetSourcei");
	qalGetSourceiv				= (ALGETSOURCEIV)QAL_GetProcAddress("alGetSourceiv");
	qalGetString				= (ALGETSTRING)QAL_GetProcAddress("alGetString");
	qalIsBuffer					= (ALISBUFFER)QAL_GetProcAddress("alIsBuffer");
	qalIsEnabled				= (ALISENABLED)QAL_GetProcAddress("alIsEnabled");
	qalIsExtensionPresent		= (ALISEXTENSIONPRESENT)QAL_GetProcAddress("alIsExtensionPresent");
	qalIsSource					= (ALISSOURCE)QAL_GetProcAddress("alIsSource");
	qalListener3f				= (ALLISTENER3F)QAL_GetProcAddress("alListener3f");
	qalListener3i				= (ALLISTENER3I)QAL_GetProcAddress("alListener3i");
	qalListenerf				= (ALLISTENERF)QAL_GetProcAddress("alListenerf");
	qalListenerfv				= (ALLISTENERFV)QAL_GetProcAddress("alListenerfv");
	qalListeneri				= (ALLISTENERI)QAL_GetProcAddress("alListeneri");
	qalListeneriv				= (ALLISTENERIV)QAL_GetProcAddress("alListeneriv");
	qalSource3f					= (ALSOURCE3F)QAL_GetProcAddress("alSource3f");
	qalSource3i					= (ALSOURCE3I)QAL_GetProcAddress("alSource3i");
	qalSourcef					= (ALSOURCEF)QAL_GetProcAddress("alSourcef");
	qalSourcefv					= (ALSOURCEFV)QAL_GetProcAddress("alSourcefv");
	qalSourcei					= (ALSOURCEI)QAL_GetProcAddress("alSourcei");
	qalSourceiv					= (ALSOURCEIV)QAL_GetProcAddress("alSourceiv");
	qalSourcePause				= (ALSOURCEPAUSE)QAL_GetProcAddress("alSourcePause");
	qalSourcePausev				= (ALSOURCEPAUSEV)QAL_GetProcAddress("alSourcePausev");
	qalSourcePlay				= (ALSOURCEPLAY)QAL_GetProcAddress("alSourcePlay");
	qalSourcePlayv				= (ALSOURCEPLAYV)QAL_GetProcAddress("alSourcePlayv");
	qalSourceQueueBuffers		= (ALSOURCEQUEUEBUFFERS)QAL_GetProcAddress("alSourceQueueBuffers");
	qalSourceRewind				= (ALSOURCEREWIND)QAL_GetProcAddress("alSourceRewind");
	qalSourceRewindv			= (ALSOURCEREWINDV)QAL_GetProcAddress("alSourceRewindv");
	qalSourceStop				= (ALSOURCESTOP)QAL_GetProcAddress("alSourceStop");
	qalSourceStopv				= (ALSOURCESTOPV)QAL_GetProcAddress("alSourceStopv");
	qalSourceUnqueueBuffers		= (ALSOURCEUNQUEUEBUFFERS)QAL_GetProcAddress("alSourceUnqueueBuffers");
	qalSpeedOfSound				= (ALSPEEDOFSOUND)QAL_GetProcAddress("alSpeedOfSound");

	return true;
}
