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
#include "render.h"
#include "video.h"
#include "cdaudio.h"
#include "sound.h"
#include "input.h"
#include "ui.h"


// The cl.parseEntities array must be large enough to hold UPDATE_BACKUP
// frames of entities, so that when a delta compressed message arrives 
// from the server it can be un-deltaed from the original 
#define	MAX_PARSE_ENTITIES		1024

// Allow a lot of command backups for very fast systems
#define	CMD_BACKUP				64
#define CMD_MASK				(CMD_BACKUP-1)

#define NUM_CROSSHAIRS			20

#define LAG_SAMPLES				128
#define LAG_WIDTH				64
#define LAG_HEIGHT				64

#define MAX_CLIENTWEAPONMODELS	20

typedef struct {
	entity_state_t		baseline;		// Delta from this if not from a previous frame
	entity_state_t		current;
	entity_state_t		prev;			// Will always be valid, but might just be a copy of current

	int					serverFrame;	// If not current, this ent isn't in the frame

	vec3_t				lerpOrigin;		// For trails (variable hz)

	float				flashStartTime;	// Muzzle flash effect start time in seconds
	float				flashRotation;	// Muzzle flash effect rotation

	int					flyStopTime;	// Fly particle effect stop time
} entity_t;

typedef struct {
	qboolean			valid;			// Cleared if delta parsing was invalid
	int					serverFrame;
	int					serverTime;		// Server time the message is valid for (in msec)
	int					deltaFrame;
	byte				areaBits[MAX_MAP_AREAS/8];	// Portalarea visibility bits
	player_state_t		playerState;
	int					numEntities;
	int					parseEntitiesIndex;	// Non-masked index into cl.parseEntities array
} frame_t;

// The client precaches these files during level load
typedef struct {
	// Sounds
	struct sfx_s		*sfxRichotecs[3];
	struct sfx_s		*sfxSparks[3];
	struct sfx_s		*sfxFootSteps[4];
	struct sfx_s		*sfxLaserHit;
	struct sfx_s		*sfxRailgun;
	struct sfx_s		*sfxRocketExplosion;
	struct sfx_s		*sfxGrenadeExplosion;
	struct sfx_s		*sfxWaterExplosion;
	struct sfx_s		*sfxMachinegunBrass;
	struct sfx_s		*sfxShotgunBrass;
	struct sfx_s		*sfxLightning;
	struct sfx_s		*sfxDisruptorExplosion;

	// Models
	struct model_s		*modParasiteBeam;
	struct model_s		*modPowerScreenShell;
	struct model_s		*modMachinegunBrass;
	struct model_s		*modShotgunBrass;

	// Materials
	struct material_s	*levelshot;
	struct material_s	*levelshotDetail;
	struct material_s	*loadingLogo;
	struct material_s	*loadingDetail[2];
	struct material_s	*loadingPercent[20];
	struct material_s	*lagometerMaterial;
	struct material_s	*disconnectedMaterial;
	struct material_s	*pauseMaterial;
	struct material_s	*crosshairMaterials[NUM_CROSSHAIRS];
	struct material_s	*hudNumberMaterials[2][11];
	struct material_s	*fireScreenMaterial;
	struct material_s	*waterBlurMaterial;
	struct material_s	*doubleVisionMaterial;
	struct material_s	*underWaterVisionMaterial;
	struct material_s	*irGogglesMaterial;
	struct material_s	*rocketExplosionMaterial;
	struct material_s	*rocketExplosionWaterMaterial;
	struct material_s	*grenadeExplosionMaterial;
	struct material_s	*grenadeExplosionWaterMaterial;
	struct material_s	*bfgExplosionMaterial;
	struct material_s	*bfgBallMaterial;
	struct material_s	*plasmaBallMaterial;
	struct material_s	*waterPlumeMaterial;
	struct material_s	*waterSprayMaterial;
	struct material_s	*waterWakeMaterial;
	struct material_s	*nukeShockwaveMaterial;
	struct material_s	*bloodBlendMaterial;
	struct material_s	*bloodSplatMaterial[2];
	struct material_s	*bloodCloudMaterial[2];
	struct material_s	*powerScreenShellMaterial;
	struct material_s	*invulnerabilityShellMaterial;
	struct material_s	*quadDamageShellMaterial;
	struct material_s	*doubleDamageShellMaterial;
	struct material_s	*halfDamageShellMaterial;
	struct material_s	*genericShellMaterial;
	struct material_s	*laserBeamMaterial;
	struct material_s	*grappleBeamMaterial;
	struct material_s	*lightningBeamMaterial;
	struct material_s	*heatBeamMaterial;
	struct material_s	*energyParticleMaterial;
	struct material_s	*glowParticleMaterial;
	struct material_s	*flameParticleMaterial;
	struct material_s	*smokeParticleMaterial;
	struct material_s	*liteSmokeParticleMaterial;
	struct material_s	*bubbleParticleMaterial;
	struct material_s	*dropletParticleMaterial;
	struct material_s	*steamParticleMaterial;
	struct material_s	*sparkParticleMaterial;
	struct material_s	*impactSparkParticleMaterial;
	struct material_s	*trackerParticleMaterial;
	struct material_s	*flyParticleMaterial;
	struct material_s	*energyMarkMaterial;
	struct material_s	*bulletMarkMaterial;
	struct material_s	*burnMarkMaterial;
	struct material_s	*bloodMarkMaterials[2][6];

	// Files referenced by the server that the client needs
	struct sfx_s		*gameSounds[MAX_SOUNDS];
	struct model_s		*gameModels[MAX_MODELS];
	struct cmodel_s		*gameCModels[MAX_MODELS];
	struct material_s	*gameMaterials[MAX_IMAGES];
} gameMedia_t;

typedef struct {
	int					dropped;
	int					suppressed;
	int					ping;
} lagSamples_t;

typedef struct {
	lagSamples_t		samples[LAG_SAMPLES];
	int					current;

	byte				image[LAG_HEIGHT][LAG_WIDTH][4];
} lagometer_t;

typedef struct {
	qboolean			valid;
	char				name[MAX_QPATH];
	char				info[MAX_QPATH];
	struct model_s		*model;
	struct material_s	*skin;
	struct material_s	*icon;
	struct model_s		*weaponModel[MAX_CLIENTWEAPONMODELS];
} clientInfo_t;

// The clientState_t structure is wiped completely at every server map
// change
typedef struct {
	entity_state_t		parseEntities[MAX_PARSE_ENTITIES];
	int					parseEntitiesIndex;	// Index (not anded off) into parseEntities

	entity_t			entities[MAX_EDICTS];

	usercmd_t			cmds[CMD_BACKUP];	// Each mesage will send several old commands
	int					cmdTime[CMD_BACKUP];	// Time sent, for calculating pings

	short				predictedOrigins[CMD_BACKUP][3];	// For debug comparing against server
	vec3_t				predictedOrigin;	// Generated by CL_PredictMovement
	vec3_t				predictedAngles;
	float				predictedStep;		// For stair up smoothing
	unsigned			predictedStepTime;
	vec3_t				predictedError;

	frame_t				frame;				// Received from server
	frame_t				frames[UPDATE_BACKUP];

	int					suppressCount;		// Number of messages rate suppressed
	int					timeOutCount;

	// The client maintains its own idea of view angles, which are sent
	// to the server each frame. It is cleared to 0 upon entering each 
	// level.
	// The server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes.
	vec3_t				viewAngles;

	int					time;				// This is the time value that the client is rendering at

	float				lerpFrac;			// Between oldFrame and frame

	player_state_t		*playerState;
	player_state_t		*oldPlayerState;

	// View rendering
	renderView_t		renderView;
	vec3_t				renderViewAngles;

	int					captureFrame;

	qboolean			underWater;

	int					timeDemoStart;
	int					timeDemoFrames;

	gameMedia_t			media;				// Precache

	clientInfo_t		clientInfo[MAX_CLIENTS];
	clientInfo_t		baseClientInfo;

	char				weaponModels[MAX_CLIENTWEAPONMODELS][MAX_OSPATH];
	int					numWeaponModels;

	// Development tools
	qboolean			testGun;
	qboolean			testModel;
	int					testModelTime;
	int					testModelFrames;
	renderEntity_t		testModelEntity;

	qboolean			testPostProcess;
	struct material_s	*testPostProcessMaterial;

	lagometer_t			lagometer;

	// View blends
	int					doubleVisionEndTime;
	int					underWaterVisionEndTime;
	int					fireScreenEndTime;

	// Crosshair names
	int					crosshairEntTime;
	int					crosshairEntNumber;

	// Zoom key
	qboolean			zooming;
	int					zoomTime;
	float				zoomSensitivity;

	// Mouse movement
	int					mouseX;
	int					mouseY;

	int					oldMouseX;
	int					oldMouseY;

	// Non-server information
	char				centerPrint[1024];
	int					centerPrintTime;

	// Transient data from server
	char				layout[1024];		// General 2D overlay
	int					inventory[MAX_ITEMS];

	// Server state information
	int					serverProtocol;		// In case we are doing some kind of version hack
	int					serverCount;		// Server identification for respawns
	qboolean			demoPlaying;		// Running a demo, any key will disconnect
	qboolean			gameMod;			// True if playing a game modification
	char				gameDir[MAX_QPATH];	// Game directory
	int					clientNum;
	qboolean			multiPlayer;

	char				configStrings[MAX_CONFIGSTRINGS][MAX_QPATH];
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
	struct material_s	*cinematicMaterial;
	struct material_s	*whiteMaterial;
	struct material_s	*consoleMaterial;
	struct material_s	*charsetMaterial;
} media_t;

typedef struct {
	char				map[MAX_QPATH];
	char				name[MAX_QPATH];

	char				string[128];
	int					percent;
} loadingInfo_t;

// The clientStatic_t structure is persistant through an arbitrary 
// number of server connections
typedef struct {
	connState_t			state;

	int					realTime;			// Always increasing, no clamping, etc...

	int					frameCount;			// Bumped for every frame
	float				frameTime;			// Seconds since last frame

	int					aviFrame;			// For AVI demos

	glConfig_t			glConfig;
	alConfig_t			alConfig;

	media_t				media;				// Precache

	// Loading screen information
	qboolean			loading;
	loadingInfo_t		loadingInfo;

	// Connection information
	char				serverName[128];	// Name of server from original connect
	char				serverMessage[512];	// Connection refused message from server
	netAdr_t			serverAddress;		// Address of server from original connect
	int					serverChallenge;	// From the server to use for connecting
	float				connectTime;		// For connection retransmits
	int					connectCount;		// Connection retransmits count
	netChan_t			netChan;

	// File download information
	fileHandle_t		downloadFile;
	int					downloadStart;
	int					downloadBytes;
	int					downloadPercent;
	char				downloadName[MAX_OSPATH];
	char				downloadTempName[MAX_OSPATH];

	// Demo recording information
	fileHandle_t		demoFile;
	qboolean			demoWaiting;
	char				demoName[MAX_OSPATH];

	// Cinematic information
	qboolean			cinematicPlaying;
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

// These are the key numbers that should be passed to CL_KeyEvent
// Normal keys should be passed as lowercased ASCII
enum {
	K_TAB = 9,
	K_ENTER	= 13,
	K_ESCAPE = 27,
	K_SPACE = 32,

	K_BACKSPACE	= 127,

	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,
	K_SHIFT,
	
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	
	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_STAR,
	K_KP_MINUS,
	K_KP_PLUS,
	K_KP_ENTER,
	K_KP_NUMLOCK,

	K_PAUSE,
	K_CAPSLOCK,

	// Mouse buttons generate virtual keys
	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,

	K_MWHEELDOWN,
	K_MWHEELUP
};

typedef enum {
	KEY_GAME,
	KEY_CONSOLE,
	KEY_MESSAGE,
	KEY_MENU
} keyDest_t;

extern clientState_t	cl;
extern clientStatic_t	cls;

extern const char		*svc_strings[256];

extern cvar_t	*cl_hand;
extern cvar_t	*cl_zoomFov;
extern cvar_t	*cl_footSteps;
extern cvar_t	*cl_noSkins;
extern cvar_t	*cl_noRender;
extern cvar_t	*cl_showNet;
extern cvar_t	*cl_showMiss;
extern cvar_t	*cl_showMaterial;
extern cvar_t	*cl_predict;
extern cvar_t	*cl_timeOut;
extern cvar_t	*cl_thirdPerson;
extern cvar_t	*cl_thirdPersonRange;
extern cvar_t	*cl_thirdPersonAngle;
extern cvar_t	*cl_viewBlend;
extern cvar_t	*cl_particles;
extern cvar_t	*cl_muzzleFlashes;
extern cvar_t	*cl_decals;
extern cvar_t	*cl_ejectBrass;
extern cvar_t	*cl_blood;
extern cvar_t	*cl_shells;
extern cvar_t	*cl_drawGun;
extern cvar_t	*cl_testGunX;
extern cvar_t	*cl_testGunY;
extern cvar_t	*cl_testGunZ;
extern cvar_t	*cl_testModelAnimate;
extern cvar_t	*cl_testModelRotate;
extern cvar_t	*cl_crosshairX;
extern cvar_t	*cl_crosshairY;
extern cvar_t	*cl_crosshairSize;
extern cvar_t	*cl_crosshairColor;
extern cvar_t	*cl_crosshairAlpha;
extern cvar_t	*cl_crosshairHealth;
extern cvar_t	*cl_crosshairNames;
extern cvar_t	*cl_centerTime;
extern cvar_t	*cl_draw2D;
extern cvar_t	*cl_drawCrosshair;
extern cvar_t	*cl_drawStatus;
extern cvar_t	*cl_drawIcons;
extern cvar_t	*cl_drawCenterString;
extern cvar_t	*cl_drawInventory;
extern cvar_t	*cl_drawLayout;
extern cvar_t	*cl_drawLagometer;
extern cvar_t	*cl_drawDisconnected;
extern cvar_t	*cl_drawRecording;
extern cvar_t	*cl_drawFPS;
extern cvar_t	*cl_drawPause;
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
void		CL_FillRect (float x, float y, float w, float h, const color_t color);
void		CL_DrawString (float x, float y, float w, float h, float width, const char *string, const color_t color, struct material_s *fontMaterial, int flags);
void		CL_DrawStringSheared (float x, float y, float w, float h, float shearX, float shearY, float width, const char *string, const color_t color, struct material_s *fontMaterial, int flags);
void		CL_DrawStringFixed (float x, float y, float w, float h, float width, const char *string, const color_t color, struct material_s *fontMaterial, int flags);
void		CL_DrawStringShearedFixed (float x, float y, float w, float h, float shearX, float shearY, float width, const char *string, const color_t color, struct material_s *fontMaterial, int flags);
void		CL_DrawPic (float x, float y, float w, float h, const color_t color, struct material_s *material);
void		CL_DrawPicST (float x, float y, float w, float h, float s1, float t1, float s2, float t2, const color_t color, struct material_s *material);
void		CL_DrawPicSheared (float x, float y, float w, float h, float offsetX, float offsetY, const color_t color, struct material_s *material);
void		CL_DrawPicShearedST (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float offsetX, float offsetY, const color_t color, struct material_s *material);
void		CL_DrawPicByName (float x, float y, float w, float h, const color_t color, const char *pic);
void		CL_DrawPicFixed (float x, float y, struct material_s *material);
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

void		CL_ProjectDecal (const vec3_t org, const vec3_t dir, float radius, struct material_s *material);

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

void		CL_SendCmd (void);

void		CL_MouseMoveEvent (int x, int y);

void		CL_InitInput (void);
void		CL_ShutdownInput (void);

// =====================================================================
// cl_keys.c

int			CL_StringToKeyNum (const char *string);
char		*CL_KeyNumToString (int keyNum);
void		CL_SetKeyBinding (int keyNum, const char *binding);
char		*CL_GetKeyBinding (int keyNum);
void		CL_WriteKeyBindings (fileHandle_t f);

keyDest_t	CL_GetKeyDest (void);
void		CL_SetKeyDest (keyDest_t dest);
void		CL_SetKeyEditMode (qboolean editMode);
qboolean	CL_KeyIsDown (int key);
qboolean	CL_AnyKeyIsDown (void);
void		CL_ClearKeyStates (void);

void		CL_KeyEvent (int key, qboolean down, unsigned time);
void		CL_CharEvent (int ch);

void		CL_InitKeys (void);
void		CL_ShutdownKeys (void);

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

void		CL_Explosion (const vec3_t org, const vec3_t dir, float radius, float rotation, float light, float lightRed, float lightGreen, float lightBlue, struct material_s *material);
void		CL_WaterSplash (const vec3_t org, const vec3_t dir);
void		CL_ExplosionWaterSplash (const vec3_t org);
void		CL_Sprite (const vec3_t org, float radius, struct material_s *material);
void		CL_LaserBeam (const vec3_t start, const vec3_t end, int width, int color, byte alpha, int duration, struct material_s *material);
void		CL_MachinegunEjectBrass (const entity_t *cent, int count, float x, float y, float z);
void		CL_ShotgunEjectBrass (const entity_t *cent, int count, float x, float y, float z);
void		CL_Bleed (const vec3_t org, const vec3_t dir, int count, qboolean green);
void		CL_BloodTrail (const vec3_t start, const vec3_t end, qboolean green);
void		CL_NukeShockwave (const vec3_t org);

// =====================================================================
// cl_main.c

void		CL_ClearState (void);
void		CL_Restart (void);
void		CL_Disconnect (qboolean shuttingDown);
void		CL_PlayBackgroundTrack (void);
void		CL_RequestNextDownload (void);

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
void		CL_RunCinematic (void);
void		CL_DrawCinematic (void);
void		CL_StopCinematic (void);
void		CL_FinishCinematic (void);

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
void		CL_TestMaterial_f (void);
void		CL_TestMaterialParm_f (void);
void		CL_NextFrame_f (void);
void		CL_PrevFrame_f (void);
void		CL_NextSkin_f (void);
void		CL_PrevSkin_f (void);
void		CL_TestPostProcess_f (void);
void		CL_ViewPos_f (void);

void		CL_ClearBloodBlends (void);
void		CL_AddBloodBlends (void);
void		CL_DamageFeedback (void);

void		CL_RenderActiveFrame (void);


#endif	// __CLIENT_H__
