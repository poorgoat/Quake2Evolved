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


#ifndef __QAL_H__
#define __QAL_H__


qboolean	QAL_Init (const char *driver);
void		QAL_Shutdown (void);

typedef ALCAPI ALCboolean		(ALCAPIENTRY * ALCCAPTURECLOSEDEVICE)(ALCdevice *device);
typedef ALCAPI ALCdevice *		(ALCAPIENTRY * ALCCAPTUREOPENDEVICE)(const ALCchar *deviceName, ALCuint frequency, ALCenum format, ALCsizei bufferSize);
typedef ALCAPI ALCvoid			(ALCAPIENTRY * ALCCAPTURESAMPLES)(ALCdevice *device, ALCvoid *buffer, ALCsizei samples);
typedef ALCAPI ALCvoid			(ALCAPIENTRY * ALCCAPTURESTART)(ALCdevice *device);
typedef ALCAPI ALCvoid			(ALCAPIENTRY * ALCCAPTURESTOP)(ALCdevice *device);
typedef ALCAPI ALCboolean		(ALCAPIENTRY * ALCCLOSEDEVICE)(ALCdevice *device);
typedef ALCAPI ALCcontext *		(ALCAPIENTRY * ALCCREATECONTEXT)(ALCdevice *device, ALCint *attrList);
typedef ALCAPI ALCvoid			(ALCAPIENTRY * ALCDESTROYCONTEXT)(ALCcontext *context);
typedef ALCAPI ALCdevice *		(ALCAPIENTRY * ALCGETCONTEXTSDEVICE)(ALCcontext *context);
typedef ALCAPI ALCcontext *		(ALCAPIENTRY * ALCGETCURRENTCONTEXT)(ALCvoid);
typedef ALCAPI ALCenum			(ALCAPIENTRY * ALCGETENUMVALUE)(ALCdevice *device, const ALCchar *enumName);
typedef ALCAPI ALCenum			(ALCAPIENTRY * ALCGETERROR)(ALCdevice *device);
typedef ALCAPI ALCvoid			(ALCAPIENTRY * ALCGETINTEGERV)(ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data);
typedef ALCAPI ALCvoid *		(ALCAPIENTRY * ALCGETPROCADDRESS)(ALCdevice *device, const ALCchar *funcName);
typedef ALCAPI ALCubyte *		(ALCAPIENTRY * ALCGETSTRING)(ALCdevice *device, ALCenum param);
typedef ALCAPI ALCboolean		(ALCAPIENTRY * ALCISEXTENSIONPRESENT)(ALCdevice *device, const ALCchar *extName);
typedef ALCAPI ALCboolean		(ALCAPIENTRY * ALCMAKECONTEXTCURRENT)(ALCcontext *context);
typedef ALCAPI ALCdevice *		(ALCAPIENTRY * ALCOPENDEVICE)(const ALCchar *deviceName);
typedef ALCAPI ALCvoid			(ALCAPIENTRY * ALCPROCESSCONTEXT)(ALCcontext *context);
typedef ALCAPI ALCvoid			(ALCAPIENTRY * ALCSUSPENDCONTEXT)(ALCcontext *context);

typedef ALAPI ALvoid			(ALAPIENTRY * ALBUFFERDATA)(ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq);
typedef ALAPI ALvoid			(ALAPIENTRY * ALBUFFER3F)(ALuint buffer, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALBUFFER3I)(ALuint buffer, ALenum param, ALint value1, ALint value2, ALint value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALBUFFERF)(ALuint buffer, ALenum param, ALfloat value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALBUFFERFV)(ALuint buffer, ALenum param, const ALfloat *values);
typedef ALAPI ALvoid			(ALAPIENTRY * ALBUFFERI)(ALuint buffer, ALenum param, ALint value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALBUFFERIV)(ALuint buffer, ALenum param, const ALint *values);
typedef ALAPI ALvoid			(ALAPIENTRY * ALDELETEBUFFERS)(ALsizei n, const ALuint *buffers);
typedef ALAPI ALvoid			(ALAPIENTRY * ALDELETESOURCES)(ALsizei n, const ALuint *sources);
typedef ALAPI ALvoid			(ALAPIENTRY * ALDISABLE)(ALenum capability);
typedef ALAPI ALvoid			(ALAPIENTRY * ALDISTANCEMODEL)(ALenum distanceModel);
typedef ALAPI ALvoid			(ALAPIENTRY * ALDOPPLERFACTOR)(ALfloat value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALDOPPLERVELOCITY)(ALfloat value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALENABLE)(ALenum capability);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGENBUFFERS)(ALsizei n, ALuint *buffers);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGENSOURCES)(ALsizei n, ALuint *sources);
typedef ALAPI ALboolean			(ALAPIENTRY * ALGETBOOLEAN)(ALenum param);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETBOOLEANV)(ALenum param, ALboolean *data);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETBUFFER3F)(ALuint buffer, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETBUFFER3I)(ALuint buffer, ALenum param, ALint *value1, ALint *value2, ALint *value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETBUFFERF)(ALuint buffer, ALenum param, ALfloat *value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETBUFFERFV)(ALuint buffer, ALenum param, ALfloat *values);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETBUFFERI)(ALuint buffer, ALenum param, ALint *value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETBUFFERIV)(ALuint buffer, ALenum param, ALint *values);
typedef ALAPI ALdouble			(ALAPIENTRY * ALGETDOUBLE)(ALenum param);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETDOUBLEV)(ALenum param, ALdouble *data);
typedef ALAPI ALenum			(ALAPIENTRY * ALGETENUMVALUE)(const ALchar *enumName);
typedef ALAPI ALenum			(ALAPIENTRY * ALGETERROR)(ALvoid);
typedef ALAPI ALfloat			(ALAPIENTRY * ALGETFLOAT)(ALenum param);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETFLOATV)(ALenum param, ALfloat *data);
typedef ALAPI ALint				(ALAPIENTRY * ALGETINTEGER)(ALenum param);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETINTEGERV)(ALenum param, ALint *data);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETLISTENER3F)(ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETLISTENER3I)(ALenum param, ALint *value1, ALint *value2, ALint *value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETLISTENERF)(ALenum param, ALfloat *value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETLISTENERFV)(ALenum param, ALfloat *values);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETLISTENERI)(ALenum param, ALint *value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETLISTENERIV)(ALenum param, ALint *values);
typedef ALAPI ALvoid *			(ALAPIENTRY * ALGETPROCADDRESS)(const ALchar *funcName);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETSOURCE3F)(ALuint source, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETSOURCE3I)(ALuint source, ALenum param, ALint *value1, ALint *value2, ALint *value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETSOURCEF)(ALuint source, ALenum param, ALfloat *value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETSOURCEFV)(ALuint source, ALenum param, ALfloat *values);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETSOURCEI)(ALuint source, ALenum param, ALint *value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALGETSOURCEIV)(ALuint source, ALenum param, ALint *values);
typedef ALAPI const ALchar *	(ALAPIENTRY * ALGETSTRING)(ALenum param);
typedef ALAPI ALboolean			(ALAPIENTRY * ALISBUFFER)(ALuint buffer);
typedef ALAPI ALboolean			(ALAPIENTRY * ALISENABLED)(ALenum capability);
typedef ALAPI ALboolean			(ALAPIENTRY * ALISEXTENSIONPRESENT)(const ALchar *extName);
typedef ALAPI ALboolean			(ALAPIENTRY * ALISSOURCE)(ALuint source);
typedef ALAPI ALvoid			(ALAPIENTRY * ALLISTENER3F)(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALLISTENER3I)(ALenum param, ALint value1, ALint value2, ALint value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALLISTENERF)(ALenum param, ALfloat value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALLISTENERFV)(ALenum param, const ALfloat *values);
typedef ALAPI ALvoid			(ALAPIENTRY * ALLISTENERI)(ALenum param, ALint value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALLISTENERIV)(ALenum param, const ALint *values);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCE3F)(ALuint source, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCE3I)(ALuint source, ALenum param, ALint value1, ALint value2, ALint value3);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEF)(ALuint source, ALenum param, ALfloat value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEFV)(ALuint source, ALenum param, const ALfloat *values);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEI)(ALuint source, ALenum param, ALint value);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEIV)(ALuint source, ALenum param, const ALint *values);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEPAUSE)(ALuint source);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEPAUSEV)(ALsizei n, const ALuint *sources);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEPLAY)(ALuint source);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEPLAYV)(ALsizei n, const ALuint *sources);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEQUEUEBUFFERS)(ALuint source, ALsizei n, const ALuint *buffers);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEREWIND)(ALuint source);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEREWINDV)(ALsizei n, const ALuint *sources);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCESTOP)(ALuint source);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCESTOPV)(ALsizei n, const ALuint *sources);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSOURCEUNQUEUEBUFFERS)(ALuint source, ALsizei n, ALuint *buffers);
typedef ALAPI ALvoid			(ALAPIENTRY * ALSPEEDOFSOUND)(ALfloat value);


// =====================================================================

extern ALCCAPTURECLOSEDEVICE	qalcCaptureCloseDevice;
extern ALCCAPTUREOPENDEVICE		qalcCaptureOpenDevice;
extern ALCCAPTURESAMPLES		qalcCaptureSamples;
extern ALCCAPTURESTART			qalcCaptureStart;
extern ALCCAPTURESTOP			qalcCaptureStop;
extern ALCCLOSEDEVICE			qalcCloseDevice;
extern ALCCREATECONTEXT			qalcCreateContext;
extern ALCDESTROYCONTEXT		qalcDestroyContext;
extern ALCGETCONTEXTSDEVICE		qalcGetContextsDevice;
extern ALCGETCURRENTCONTEXT		qalcGetCurrentContext;
extern ALCGETENUMVALUE			qalcGetEnumValue;
extern ALCGETERROR				qalcGetError;
extern ALCGETINTEGERV			qalcGetIntegerv;
extern ALCGETPROCADDRESS		qalcGetProcAddress;
extern ALCGETSTRING				qalcGetString;
extern ALCISEXTENSIONPRESENT	qalcIsExtensionPresent;
extern ALCMAKECONTEXTCURRENT	qalcMakeContextCurrent;
extern ALCOPENDEVICE			qalcOpenDevice;
extern ALCPROCESSCONTEXT		qalcProcessContext;
extern ALCSUSPENDCONTEXT		qalcSuspendContext;

extern ALBUFFERDATA				qalBufferData;
extern ALBUFFER3F				qalBuffer3f;
extern ALBUFFER3I				qalBuffer3i;
extern ALBUFFERF				qalBufferf;
extern ALBUFFERFV				qalBufferfv;
extern ALBUFFERI				qalBufferi;
extern ALBUFFERIV				qalBufferiv;
extern ALDELETEBUFFERS			qalDeleteBuffers;
extern ALDELETESOURCES			qalDeleteSources;
extern ALDISABLE				qalDisable;
extern ALDISTANCEMODEL			qalDistanceModel;
extern ALDOPPLERFACTOR			qalDopplerFactor;
extern ALDOPPLERVELOCITY		qalDopplerVelocity;
extern ALENABLE					qalEnable;
extern ALGENBUFFERS				qalGenBuffers;
extern ALGENSOURCES				qalGenSources;
extern ALGETBOOLEAN				qalGetBoolean;
extern ALGETBOOLEANV			qalGetBooleanv;
extern ALGETBUFFER3F			qalGetBuffer3f;
extern ALGETBUFFER3I			qalGetBuffer3i;
extern ALGETBUFFERF				qalGetBufferf;
extern ALGETBUFFERFV			qalGetBufferfv;
extern ALGETBUFFERI				qalGetBufferi;
extern ALGETBUFFERIV			qalGetBufferiv;
extern ALGETDOUBLE				qalGetDouble;
extern ALGETDOUBLEV				qalGetDoublev;
extern ALGETENUMVALUE			qalGetEnumValue;
extern ALGETERROR				qalGetError;
extern ALGETFLOAT				qalGetFloat;
extern ALGETFLOATV				qalGetFloatv;
extern ALGETINTEGER				qalGetInteger;
extern ALGETINTEGERV			qalGetIntegerv;
extern ALGETLISTENER3F			qalGetListener3f;
extern ALGETLISTENER3I			qalGetListener3i;
extern ALGETLISTENERF			qalGetListenerf;
extern ALGETLISTENERFV			qalGetListenerfv;
extern ALGETLISTENERI			qalGetListeneri;
extern ALGETLISTENERIV			qalGetListeneriv;
extern ALGETPROCADDRESS			qalGetProcAddress;
extern ALGETSOURCE3F			qalGetSource3f;
extern ALGETSOURCE3I			qalGetSource3i;
extern ALGETSOURCEF				qalGetSourcef;
extern ALGETSOURCEFV			qalGetSourcefv;
extern ALGETSOURCEI				qalGetSourcei;
extern ALGETSOURCEIV			qalGetSourceiv;
extern ALGETSTRING				qalGetString;
extern ALISBUFFER				qalIsBuffer;
extern ALISENABLED				qalIsEnabled;
extern ALISEXTENSIONPRESENT		qalIsExtensionPresent;
extern ALISSOURCE				qalIsSource;
extern ALLISTENER3F				qalListener3f;
extern ALLISTENER3I				qalListener3i;
extern ALLISTENERF				qalListenerf;
extern ALLISTENERFV				qalListenerfv;
extern ALLISTENERI				qalListeneri;
extern ALLISTENERIV				qalListeneriv;
extern ALSOURCE3F				qalSource3f;
extern ALSOURCE3I				qalSource3i;
extern ALSOURCEF				qalSourcef;
extern ALSOURCEFV				qalSourcefv;
extern ALSOURCEI				qalSourcei;
extern ALSOURCEIV				qalSourceiv;
extern ALSOURCEPAUSE			qalSourcePause;
extern ALSOURCEPAUSEV			qalSourcePausev;
extern ALSOURCEPLAY				qalSourcePlay;
extern ALSOURCEPLAYV			qalSourcePlayv;
extern ALSOURCEQUEUEBUFFERS		qalSourceQueueBuffers;
extern ALSOURCEREWIND			qalSourceRewind;
extern ALSOURCEREWINDV			qalSourceRewindv;
extern ALSOURCESTOP				qalSourceStop;
extern ALSOURCESTOPV			qalSourceStopv;
extern ALSOURCEUNQUEUEBUFFERS	qalSourceUnqueueBuffers;
extern ALSPEEDOFSOUND			qalSpeedOfSound;


#endif	// __QAL_H__
