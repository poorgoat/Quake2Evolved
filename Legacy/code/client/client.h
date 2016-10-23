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


// client.h -- primary header for client


#ifndef __CLIENT_H__
#define __CLIENT_H__


#include "../qcommon/qcommon.h"
#include "cinematic.h"
#include "console.h"
#include "refresh.h"
#include "video.h"
#include "sound.h"
#include "cdaudio.h"
#include "input.h"
#include "keys.h"
#include "ui.h"


// The cl.parseEntities array must be large enough to hold UPDATE_BACKUP
// frames of entities, so that when a delta compressed message arrives 
// from the server it can be un-deltaed from the original 
#define	MAX_PARSE_ENTITIES		1024

// Allow a lot of command backups for very fast systems
#define	CMD_BACKUP				64
#define CMD_MASK				(CMD_BACKUP-1)

#define NUM_CROSSHAIRS			20

#define LAG_SAMPLES				256

#define MAX_CLIENTWEAPONMODELS	20

typedef struct {
	entity_state_t	baseline;		// Delta from this if not from a previous frame
	entity_state_t	current;
	entity_state_t	prev;			// Will always be valid, but might just be a copy of current

	int				serverFrame;	// If not current, this ent isn't in the frame

	vec3_t			lerpOrigin;		// For trails (variable hz)

	int				flyStopTime;
} centity_t;

typedef struct {
	qboolean		valid;			// Cleared if delta parsing was invalid
	int				serverFrame;
	int				serverTime;		// Server time the message is valid for (in msec)
	int				deltaFrame;
	byte			areaBits[MAX_MAP_AREAS/8];	// Portalarea visibility bits
	player_state_t	playerState;
	int				numEntities;
	int				parseEntitiesIndex;	// Non-masked index into cl.parseEntities array
} frame_t;

// The client precaches these files during level load
typedef struct {
	// Sounds
	struct sfx_s	*sfxRichotecs[3];
	struct sfx_s	*sfxSparks[3];
	struct sfx_s	*sfxFootSteps[4];
	struct sfx_s	*sfxLaserHit;
	struct sfx_s	*sfxRailgun;
	struct sfx_s	*sfxRocketExplosion;
	struct sfx_s	*sfxGrenadeExplosion;
	struct sfx_s	*sfxWaterExplosion;
	struct sfx_s	*sfxMachinegunBrass;
	struct sfx_s	*sfxShotgunBrass;
	struct sfx_s	*sfxLightning;
	struct sfx_s	*sfxDisruptorExplosion;

	// Models
	struct model_s	*modParasiteBeam;
	struct model_s	*modPowerScreenShell;
	struct model_s	*modMachinegunBrass;
	struct model_s	*modShotgunBrass;

	// Shaders
	struct shader_s	*levelshot;
	struct shader_s	*levelshotDetail;
	struct shader_s	*loadingLogo;
	struct shader_s	*loadingDetail[2];
	struct shader_s	*loadingPercent[20];
	struct shader_s	*lagometerShader;
	struct shader_s	*disconnectedShader;
	struct shader_s	*backTileShader;
	struct shader_s	*pauseShader;
	struct shader_s	*crosshairShaders[NUM_CROSSHAIRS];
	struct shader_s	*hudNumberShaders[2][11];
	struct shader_s	*viewBloodBlend;
	struct shader_s	*viewFireBlend;
	struct shader_s	*viewIrGoggles;
	struct shader_s	*rocketExplosionShader;
	struct shader_s	*rocketExplosionWaterShader;
	struct shader_s	*grenadeExplosionShader;
	struct shader_s	*grenadeExplosionWaterShader;
	struct shader_s	*bfgExplosionShader;
	struct shader_s	*bfgBallShader;
	struct shader_s	*plasmaBallShader;
	struct shader_s	*waterPlumeShader;
	struct shader_s	*waterSprayShader;
	struct shader_s	*waterWakeShader;
	struct shader_s	*nukeShockwaveShader;
	struct shader_s	*bloodSplatShader[2];
	struct shader_s	*bloodCloudShader[2];
	struct shader_s	*powerScreenShellShader;
	struct shader_s	*invulnerabilityShellShader;
	struct shader_s	*quadDamageShellShader;
	struct shader_s	*doubleDamageShellShader;
	struct shader_s	*halfDamageShellShader;
	struct shader_s	*genericShellShader;
	struct shader_s	*laserBeamShader;
	struct shader_s	*grappleBeamShader;
	struct shader_s	*lightningBeamShader;
	struct shader_s	*heatBeamShader;
	struct shader_s	*energyParticleShader;
	struct shader_s	*glowParticleShader;
	struct shader_s	*flameParticleShader;
	struct shader_s	*smokeParticleShader;
	struct shader_s	*liteSmokeParticleShader;
	struct shader_s	*bubbleParticleShader;
	struct shader_s	*dropletParticleShader;
	struct shader_s	*steamParticleShader;
	struct shader_s	*sparkParticleShader;
	struct shader_s	*impactSparkParticleShader;
	struct shader_s	*trackerParticleShader;
	struct shader_s	*flyParticleShader;
	struct shader_s	*energyMarkShader;
	struct shader_s	*bulletMarkShader;
	struct shader_s	*burnMarkShader;
	struct shader_s	*bloodMarkShaders[2][6];

	// Files referenced by the server that the client needs
	struct sfx_s	*gameSounds[MAX_SOUNDS];
	struct model_s	*gameModels[MAX_MODELS];
	struct cmodel_s	*gameCModels[MAX_MODELS];
	struct shader_s	*gameShaders[MAX_IMAGES];
} gameMedia_t;

typedef struct {
	int				dropped[LAG_SAMPLES];
	int				suppressed[LAG_SAMPLES];
	int				ping[LAG_SAMPLES];
	int				current;
} lagometer_t;

typedef struct {
	qboolean		valid;
	char			name[MAX_QPATH];
	char			info[MAX_QPATH];
	struct model_s	*model;
	struct shader_s	*skin;
	struct shader_s	*icon;
	struct model_s	*weaponModel[MAX_CLIENTWEAPONMODELS];
} clientInfo_t;

// The clientState_t structure is wiped completely at every server map
// change
typedef struct {
	entity_state_t	parseEntities[MAX_PARSE_ENTITIES];
	int				parseEntitiesIndex;	// Index (not anded off) into parseEntities

	centity_t		entities[MAX_EDICTS];

	usercmd_t		cmds[CMD_BACKUP];	// Each mesage will send several old cmds
	int				cmdTime[CMD_BACKUP];	// Time sent, for calculating pings

	short			predictedOrigins[CMD_BACKUP][3];	// For debug comparing against server
	vec3_t			predictedOrigin;	// Generated by CL_PredictMovement
	vec3_t			predictedAngles;
	float			predictedStep;		// For stair up smoothing
	unsigned		predictedStepTime;
	vec3_t			predictedError;

	frame_t			frame;				// Received from server
	frame_t			frames[UPDATE_BACKUP];

	int				suppressCount;		// Number of messages rate suppressed
	int				timeOutCount;

	// The client maintains its own idea of view angles, which are sent
	// to the server each frame. It is cleared to 0 upon entering each 
	// level.
	// The server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes.
	vec3_t			viewAngles;

	int				time;				// This is the time value that the client is rendering at

	float			lerpFrac;			// Between oldFrame and frame

	player_state_t	*playerState;
	player_state_t	*oldPlayerState;

	// View rendering
	refDef_t		refDef;
	vec3_t			refDefViewAngles;

	int				timeDemoStart;
	int				timeDemoFrames;

	gameMedia_t		media;				// Precache

	clientInfo_t	clientInfo[MAX_CLIENTS];
	clientInfo_t	baseClientInfo;

	char			weaponModels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
	int				numWeaponModels;

	// Development tools
	qboolean		testGun;
	qboolean		testModel;
	char			testModelName[MAX_QPATH];
	entity_t		testModelEntity;

	lagometer_t		lagometer;

	// View blends
	int				damageTime;
	int				damageAngle;

	int				underwaterTime;
	int				underwaterMask;
	int				underwaterContents;

	// Crosshair names
	int				crosshairEntNumber;
	int				crosshairEntTime;

	// Zoom key
	qboolean		zooming;
	int				zoomTime;
	float			zoomSensitivity;

	// Non-gameserver information
	char			centerPrint[1024];
	int				centerPrintTime;

	// Transient data from server
	char			layout[1024];		// General 2D overlay
	int				inventory[MAX_ITEMS];

	// Server state information
	int				serverCount;		// Server identification for prespawns
	qboolean		demoPlayback;		// Running a demo, any key will disconnect
	qboolean		gameMod;
	char			gameDir[MAX_QPATH];
	int				clientNum;
	qboolean		multiPlayer;

	char			configStrings[MAX_CONFIGSTRINGS][MAX_QPATH];
} clientState_t;

typedef enum {
	CA_UNINITIALIZED,	// During initialization or dedicated servers
	CA_DISCONNECTED,	// Not talking to a server
	CA_CONNECTING,		// Sending request packets to the server
	CA_CHALLENGING,		// Sending challenge packets to the server
	CA_CONNECTED,		// netChan_t established, getting game state
	CA_LOADING,			// Only during level load, never during main loop
	CA_PRIMED,			// Got game state, waiting for first frame
	CA_ACTIVE			// Game views should be displayed
} connState_t;

// The client precaches these files during initialization
typedef struct {
	struct shader_s	*whiteShader;
	struct shader_s	*consoleShader;
	struct shader_s	*charsetShader;
} media_t;

typedef struct {
	char			map[MAX_QPATH];
	char			name[MAX_QPATH];

	char			string[128];
	int				percent;
} loadingInfo_t;

// The clientStatic_t structure is persistant through an arbitrary 
// number of server connections
typedef struct {
	connState_t		state;

	int				realTime;			// Always increasing, no clamping, etc...
	float			frameTime;			// Seconds since last frame

	// Screen rendering information
	float			screenScaleX;		// Coord scale factor
	float			screenScaleY;		// Coord scale factor
	qboolean		screenDisabled;		// Don't update screen if true

	glConfig_t		glConfig;
	alConfig_t		alConfig;

	media_t			media;				// Precache

	// Loading screen information
	qboolean		loading;
	loadingInfo_t	loadingInfo;

	// Connection information
	char			serverName[128];	// Name of server from original connect
	netAdr_t		serverAddress;		// Address of server from original connect
	int				serverChallenge;	// From the server to use for connecting
	char			serverMessage[128];	// Connection refused message from server
	int				serverProtocol;		// In case we are doing some kind of version hack
	float			connectTime;		// For connection retransmits
	int				connectCount;		// Connection retransmits count
	netChan_t		netChan;

	// File transfer from server
	fileHandle_t	downloadFile;
	int				downloadStart;
	int				downloadBytes;
	int				downloadPercent;
	char			downloadName[MAX_QPATH];
	char			downloadTempName[MAX_QPATH];

	// Demo recording information
	fileHandle_t	demoFile;
	qboolean		demoWaiting;
	char			demoName[MAX_QPATH];

	// Cinematic information
	cinHandle_t		cinematicHandle;
} clientStatic_t;

// =====================================================================

// Flags for string drawing functions
#define DSF_FORCECOLOR			1
#define DSF_DROPSHADOW			2
#define DSF_LEFT				4
#define DSF_CENTER				8
#define DSF_RIGHT				16
#define DSF_LOWERCASE			32
#define DSF_UPPERCASE			64

typedef struct {
	int				down[2];			// Key nums holding it down
	unsigned		downTime;			// Msec timestamp
	unsigned		msec;				// Msec down this frame
	int				state;
} kbutton_t;

extern clientState_t	cl;
extern clientStatic_t	cls;

extern kbutton_t		in_up;
extern kbutton_t		in_down;
extern kbutton_t		in_left;
extern kbutton_t		in_right;
extern kbutton_t		in_forward;
extern kbutton_t		in_back;
extern kbutton_t		in_lookUp;
extern kbutton_t		in_lookDown;
extern kbutton_t		in_strafe;
extern kbutton_t		in_moveLeft;
extern kbutton_t		in_moveRight;
extern kbutton_t		in_speed;
extern kbutton_t		in_attack;
extern kbutton_t		in_use;
extern kbutton_t		in_kLook;
extern int				in_impulse;

extern char				*svc_strings[256];

extern cvar_t	*cl_hand;
extern cvar_t	*cl_zoomFov;
extern cvar_t	*cl_drawGun;
extern cvar_t	*cl_drawShells;
extern cvar_t	*cl_footSteps;
extern cvar_t	*cl_noSkins;
extern cvar_t	*cl_predict;
extern cvar_t	*cl_maxFPS;
extern cvar_t	*cl_freeLook;
extern cvar_t	*cl_lookSpring;
extern cvar_t	*cl_lookStrafe;
extern cvar_t	*cl_upSpeed;
extern cvar_t	*cl_forwardSpeed;
extern cvar_t	*cl_sideSpeed;
extern cvar_t	*cl_yawSpeed;
extern cvar_t	*cl_pitchSpeed;
extern cvar_t	*cl_angleSpeedKey;
extern cvar_t	*cl_run;
extern cvar_t	*cl_noDelta;
extern cvar_t	*cl_showNet;
extern cvar_t	*cl_showMiss;
extern cvar_t	*cl_showShader;
extern cvar_t	*cl_timeOut;
extern cvar_t	*cl_visibleWeapons;
extern cvar_t	*cl_thirdPerson;
extern cvar_t	*cl_thirdPersonRange;
extern cvar_t	*cl_thirdPersonAngle;
extern cvar_t	*cl_viewBlend;
extern cvar_t	*cl_particles;
extern cvar_t	*cl_particleLOD;
extern cvar_t	*cl_particleBounce;
extern cvar_t	*cl_particleFriction;
extern cvar_t	*cl_particleVertexLight;
extern cvar_t	*cl_markTime;
extern cvar_t	*cl_brassTime;
extern cvar_t	*cl_blood;
extern cvar_t	*cl_testGunX;
extern cvar_t	*cl_testGunY;
extern cvar_t	*cl_testGunZ;
extern cvar_t	*cl_stereoSeparation;
extern cvar_t	*cl_drawCrosshair;
extern cvar_t	*cl_crosshairX;
extern cvar_t	*cl_crosshairY;
extern cvar_t	*cl_crosshairSize;
extern cvar_t	*cl_crosshairColor;
extern cvar_t	*cl_crosshairAlpha;
extern cvar_t	*cl_crosshairHealth;
extern cvar_t	*cl_crosshairNames;
extern cvar_t	*cl_viewSize;
extern cvar_t	*cl_centerTime;
extern cvar_t	*cl_drawCenterString;
extern cvar_t	*cl_drawPause;
extern cvar_t	*cl_drawFPS;
extern cvar_t	*cl_drawLagometer;
extern cvar_t	*cl_drawDisconnected;
extern cvar_t	*cl_drawRecording;
extern cvar_t	*cl_draw2D;
extern cvar_t	*cl_drawIcons;
extern cvar_t	*cl_drawStatus;
extern cvar_t	*cl_drawInventory;
extern cvar_t	*cl_drawLayout;
extern cvar_t	*cl_newHUD;
extern cvar_t	*cl_allowDownload;
extern cvar_t	*cl_rconPassword;
extern cvar_t	*cl_rconAddress;

// =====================================================================
// cl_demo.c

void		CL_Record_f (void);
void		CL_StopRecord_f (void);

void		CL_WriteDemoMessage (void);

// =====================================================================
// cl_draw.c

byte		*CL_FadeColor (const color_t color, int startTime, int totalTime, int fadeTime);
byte		*CL_FadeAlpha (const color_t color, int startTime, int totalTime, int fadeTime);
byte		*CL_FadeColorAndAlpha (const color_t color, int startTime, int totalTime, int fadeTime);
void		CL_ScaleCoords (float *x, float *y, float *w, float *h);
void		CL_DrawString (float x, float y, float w, float h, float offsetX, float offsetY, float width, const char *string, const color_t color, struct shader_s *fontShader, qboolean scale, int flags);
void		CL_FillRect (float x, float y, float w, float h, const color_t color);
void		CL_DrawPic (float x, float y, float w, float h, const color_t color, struct shader_s *shader);
void		CL_DrawPicST (float x, float y, float w, float h, float sl, float tl, float sh, float th, const color_t color, struct shader_s *shader);
void		CL_DrawPicRotated (float x, float y, float w, float h, float angle, const color_t color, struct shader_s *shader);
void		CL_DrawPicRotatedST (float x, float y, float w, float h, float sl, float tl, float sh, float th, float angle, const color_t color, struct shader_s *shader);
void		CL_DrawPicOffset (float x, float y, float w, float h, float offsetX, float offsetY, const color_t color, struct shader_s *shader);
void		CL_DrawPicOffsetST (float x, float y, float w, float h, float sl, float tl, float sh, float th, float offsetX, float offsetY, const color_t color, struct shader_s *shader);
void		CL_DrawPicByName (float x, float y, float w, float h, const color_t color, const char *pic);
void		CL_DrawPicFixed (float x, float y, struct shader_s *shader);
void		CL_DrawPicFixedByName (float x, float y, const char *pic);

// =====================================================================
// cl_effects.c

void		CL_ClearLightStyles (void);
void		CL_RunLightStyles (void);
void		CL_AddLightStyles (void);
void		CL_SetLightStyle (int style);

void		CL_ClearDynamicLights (void);
void		CL_AddDynamicLights (void);
void		CL_DynamicLight (const vec3_t org, float intensity, float r, float g, float b, qboolean fade, int duration);

void		CL_ParsePlayerMuzzleFlash (void);
void		CL_ParseMonsterMuzzleFlash (void);

// =====================================================================
// cl_ents.c

void		CL_ParseBaseLine (void);
void		CL_ParseFrame (void);

void		CL_AddPacketEntities (void);
void		CL_AddViewWeapon (void);

void		CL_GetEntitySoundSpatialization (int ent, vec3_t origin, vec3_t velocity);

// =====================================================================
// cl_input.c

void		CL_CenterView_f (void);
void		CL_ZoomDown_f (void);
void		CL_ZoomUp_f (void);
void		CL_UpDown_f (void);
void		CL_UpUp_f (void);
void		CL_DownDown_f (void);
void		CL_DownUp_f (void);
void		CL_LeftDown_f (void);
void		CL_LeftUp_f (void);
void		CL_RightDown_f (void);
void		CL_RightUp_f (void);
void		CL_ForwardDown_f (void);
void		CL_ForwardUp_f (void);
void		CL_BackDown_f (void);
void		CL_BackUp_f (void);
void		CL_LookUpDown_f (void);
void		CL_LookUpUp_f (void);
void		CL_LookDownDown_f (void);
void		CL_LookDownUp_f (void);
void		CL_StrafeDown_f (void);
void		CL_StrafeUp_f (void);
void		CL_MoveLeftDown_f (void);
void		CL_MoveLeftUp_f (void);
void		CL_MoveRightDown_f (void);
void		CL_MoveRightUp_f (void);
void		CL_SpeedDown_f (void);
void		CL_SpeedUp_f (void);
void		CL_AttackDown_f (void);
void		CL_AttackUp_f (void);
void		CL_UseDown_f (void);
void		CL_UseUp_f (void);
void		CL_KLookDown_f (void);
void		CL_KLookUp_f (void);
void		CL_Impulse_f (void);

void		CL_SendCmd (void);

// =====================================================================
// cl_load.c

void		CL_Loading (void);
void		CL_DrawLoading (void);

void		CL_LoadClientInfo (clientInfo_t *ci, const char *string);
void		CL_LoadGameMedia (void);
void		CL_LoadLocalMedia (void);

// =====================================================================
// cl_localents.c

void		CL_ClearLocalEntities (void);
void		CL_AddLocalEntities (void);

void		CL_Explosion (const vec3_t org, const vec3_t dir, float radius, float rotation, float light, float lightRed, float lightGreen, float lightBlue, struct shader_s *shader);
void		CL_WaterSplash (const vec3_t org, const vec3_t dir);
void		CL_ExplosionWaterSplash (const vec3_t org);
void		CL_Sprite (const vec3_t org, float radius, struct shader_s *shader);
void		CL_LaserBeam (const vec3_t start, const vec3_t end, int width, int color, byte alpha, int duration, struct shader_s *shader);
void		CL_MachinegunEjectBrass (const centity_t *cent, int count, float x, float y, float z);
void		CL_ShotgunEjectBrass (const centity_t *cent, int count, float x, float y, float z);
void		CL_Bleed (const vec3_t org, const vec3_t dir, int count, qboolean green);
void		CL_BloodTrail (const vec3_t start, const vec3_t end, qboolean green);
void		CL_NukeShockwave (const vec3_t org);

// =====================================================================
// cl_main.c

void		CL_ClearState (void);
void		CL_ClearMemory (void);
void		CL_Startup (void);
void		CL_Disconnect (qboolean shuttingDown);
void		CL_FixCheatVars (void);
void		CL_PlayBackgroundTrack (void);
void		CL_RequestNextDownload (void);

// =====================================================================
// cl_marks.c

void		CL_ClearMarks (void);
void		CL_AddMarks (void);
void		CL_ImpactMark (const vec3_t org, const vec3_t dir, float orientation, float radius, float r, float g, float b, float a, qboolean alphaFade, struct shader_s *shader, qboolean temporary);

// =====================================================================
// cl_parse.c

void		CL_ShowNet (int level, const char *fmt, ...);
void		CL_ParseServerMessage (void);

// =====================================================================
// cl_particles.c

void		CL_ClearParticles (void);
void		CL_AddParticles (void);

void		CL_BlasterTrail (const vec3_t start, const vec3_t end, float r, float g, float b);
void		CL_GrenadeTrail (const vec3_t start, const vec3_t end);
void		CL_RocketTrail (const vec3_t start, const vec3_t end);
void		CL_RailTrail (const vec3_t start, const vec3_t end);
void		CL_BFGTrail (const vec3_t start, const vec3_t end);
void		CL_HeatBeamTrail (const vec3_t start, const vec3_t forward);
void		CL_TrackerTrail (const vec3_t start, const vec3_t end);
void		CL_TagTrail (const vec3_t start, const vec3_t end);
void		CL_BubbleTrail (const vec3_t start, const vec3_t end, float dist, float radius);
void		CL_FlagTrail (const vec3_t start, const vec3_t end, float r, float g, float b);
void		CL_BlasterParticles (const vec3_t org, const vec3_t dir, float r, float g, float b);
void		CL_BulletParticles (const vec3_t org, const vec3_t dir);
void		CL_ExplosionParticles (const vec3_t org);
void		CL_BFGExplosionParticles (const vec3_t org);
void		CL_TrackerExplosionParticles (const vec3_t org);
void		CL_SmokePuffParticles (const vec3_t org, float radius, int count);
void		CL_BubbleParticles (const vec3_t org, int count, float magnitude);
void		CL_SparkParticles (const vec3_t org, const vec3_t dir, int count);
void		CL_DamageSparkParticles (const vec3_t org, const vec3_t dir, int count, int color);
void		CL_LaserSparkParticles (const vec3_t org, const vec3_t dir, int count, int color);
void		CL_SplashParticles (const vec3_t org, const vec3_t dir, int count, float magnitude, float spread);
void		CL_LavaSteamParticles (const vec3_t org, const vec3_t dir, int count);
void		CL_FlyParticles (const vec3_t org, int count);
void		CL_TeleportParticles (const vec3_t org);
void		CL_BigTeleportParticles (const vec3_t org);
void		CL_TeleporterParticles (const vec3_t org);
void		CL_TrapParticles (const vec3_t org);
void		CL_LogParticles (const vec3_t org, float r, float g, float b);
void		CL_ItemRespawnParticles (const vec3_t org);
void		CL_TrackerShellParticles (const vec3_t org);
void		CL_NukeSmokeParticles (const vec3_t org);
void		CL_WeldingSparkParticles (const vec3_t org, const vec3_t dir, int count, int color);
void		CL_TunnelSparkParticles (const vec3_t org, const vec3_t dir, int count, int color);
void		CL_ForceWallParticles (const vec3_t start, const vec3_t end, int color);
void		CL_SteamParticles (const vec3_t org, const vec3_t dir, int count, int color, float magnitude);

// =====================================================================
// cl_predict.c

void		CL_BuildSolidList (void);
trace_t		CL_Trace (const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int brushMask, qboolean brushOnly, int *entNumber);
int			CL_PointContents (const vec3_t point, int skipNumber);

void		CL_CheckPredictionError (void);
void		CL_PredictMovement (void);

// =====================================================================
// cl_screen.c

void		CL_LoadHUD (void);

void		CL_PlayCinematic (const char *name);
void		CL_StopCinematic (void);
void		CL_FinishCinematic (void);
void		CL_DrawCinematic (void);

void		CL_TileClear (void);
void		CL_Draw2D (void);

void		CL_UpdateScreen (void);

// =====================================================================
// cl_tempents.c

void		CL_ParseTempEntity (void);
void		CL_ClearTempEntities (void);
void		CL_AddTempEntities (void);

// =====================================================================
// cl_view.c

void		CL_TestModel_f (void);
void		CL_TestGun_f (void);
void		CL_NextFrame_f (void);
void		CL_PrevFrame_f (void);
void		CL_NextSkin_f (void);
void		CL_PrevSkin_f (void);
void		CL_Viewpos_f (void);
void		CL_TimeRefresh_f (void);
void		CL_SizeUp_f (void);
void		CL_SizeDown_f (void);

void		CL_RenderView (float stereoSeparation);


#endif	// __CLIENT_H__
