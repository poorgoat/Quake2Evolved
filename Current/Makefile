#
#                Quake II Evolved Makefile
#

#### Include any local modification Makefile data ########

-include Makefile.local


#### Setup platform variable #############################

COMPILE_PLATFORM = $(shell uname|sed -e s/_.*//|tr '[:upper:]' '[:lower:]')
COMPILE_ARCH = $(shell uname -m | sed -e s/i.86/i386/)

ifeq ($(COMPILE_PLATFORM),mingw32)
	ifeq ($(COMPILE_ARCH),i386)
		COMPILE_ARCH = x86
	endif
endif


#### Filsystem variables #################################

MKDIR = mkdir


#### Additional Makefile Setup ###########################

ifndef PLATFORM
	PLATFORM = $(COMPILE_PLATFORM)
endif
export PLATFORM

ifndef ARCH
	ARCH = $(COMPILE_ARCH)
endif
export ARCH

ifneq ($(PLATFORM),$(COMPILE_PLATFORM))
	CROSS_COMPILING = 1
else
	CROSS_COMPILING = 0

	ifneq ($(ARCH),$(COMPILE_ARCH))
		CROSS_COMPILING = 1
	endif
endif
export CROSS_COMPILING


#### Local compiler and directory variables ###############

COPYDIR = "/usr/local/games/q2e"
LIBDEVDIR_LIBS = "/c/LibDev/lib"
LIBDEVDIR_INC = "/c/LibDev/include"
MOUNT_DIR = code
BUILD_DIR = build
BD = $(BUILD_DIR)/debug-$(PLATFORM)-$(ARCH)
BR = $(BUILD_DIR)/release-$(PLATFORM)-$(ARCH)
CDIR = $(MOUNT_DIR)/client
SDIR = $(MOUNT_DIR)/server
RDIR = $(MOUNT_DIR)/render
CMDIR = $(MOUNT_DIR)/qcommon
QSDIR = $(MOUNT_DIR)/qshared
W32DIR = $(MOUNT_DIR)/win32
GDIR = $(MOUNT_DIR)/game
UIDIR = $(MOUNT_DIR)/ui
LOKISETUPDIR = misc/setup
NSISDIR = misc/nsis
SDLHDIR = $(COMPAT_DIR)/SDL12
LIBSDIR = $(COMPAT_DIR)/lib
TEMPDIR = /tmp


#### MinGW build #########################################

ifeq ($(PLATFORM),mingw32)
	ifndef WINDRES
		WINDRES = windres
	endif

	ARCH = x86

	BASE_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes -I$(LIBDEVDIR_INC)

	# Require Windows XP or later
	#BASE_CFLAGS += -DWINVER=0x501

	OPTIMIZE = -O3 -march=i686 -fno-omit-frame-pointer -ffast-math \
    -falign-loops=2 -funroll-loops -falign-jumps=2 -falign-functions=2 \
    -fstrength-reduce

	SHLIBEXT = dll
	SHLIBCFLAGS = 
	SHLIBLDFLAGS = -shared $(LDFLAGS)
	BINEXT = .exe

	LDFLAGS = -lws2_32 -lwinmm -lz -lopenal32 -L$(LIBDEVDIR_LIBS)
	CLIENT_LDFLAGS = -mwindows -lgdi32 -lole32 -lopengl32 -lvorbisfile -lvorbis -logg -lopenal32 -L$(LIBDEVDIR_LIBS)

	ifeq ($(ARCH),x86)
		# build 32bit
		BASE_CFLAGS += -m32
		LDFLAGS += -m32
	endif

	DEBUG_CFLAGS = $(BASE_CFLAGS) -g -O0
	RELEASE_CFLAGS = $(BASE_CFLAGS) $(OPTIMIZE)

endif # ifeq mingw32


#### Compilation process #################################

TARGETS = $(B)/baseq2/game$(ARCH).$(SHLIBEXT) $(B)/q2e.$(ARCH)$(BINEXT)

ifeq ($(V),1)
echo_cmd=@:
Q=
else
echo_cmd=@echo
Q=@
endif

define DO_CC
$(echo_cmd) "CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) -o $@ -c $<
endef

define DO_GAME_CC
$(echo_cmd) "GAME_CC $<"
$(Q)$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<
endef

define DO_WINDRES
$(echo_cmd) "WINDRES $<"
$(Q)$(WINDRES) -J rc -O coff -i $< -o $@
endef

default: release
all: debug release


debug:
	@$(MAKE) targets B=$(BD) CFLAGS="$(CFLAGS) $(DEPEND_CFLAGS) \
		$(DEBUG_CFLAGS)" V=$(V)

release:
	@$(MAKE) targets B=$(BR) CFLAGS="$(CFLAGS) $(DEPEND_CFLAGS) \
		$(RELEASE_CFLAGS)" V=$(V)

# Create the build directories, check libraries and print out
# an informational message, then start building
targets: makedirs
	@echo ""
	@echo "Building Quake II Evolved in $(B):"
	@echo ""
	@echo "  PLATFORM: $(PLATFORM)"
	@echo "  ARCH: $(ARCH)"
	@echo "  VERSION: $(VERSION)"
	@echo "  COMPILE_PLATFORM: $(COMPILE_PLATFORM)"
	@echo "  COMPILE_ARCH: $(COMPILE_ARCH)"
	@echo "  CC: $(CC)"
	@echo ""
	@echo "  CFLAGS:"
	@for i in $(CFLAGS); \
	do \
		echo "    $$i"; \
	done
	@echo ""
	@echo "  LDFLAGS:"
	@for i in $(LDFLAGS); \
	do \
		echo "    $$i"; \
	done
	@echo ""
	@echo "  Output:"
	@for i in $(TARGETS); \
	do \
		echo "    $$i"; \
	done
	@echo ""
ifneq ($(TARGETS),)
	@$(MAKE) $(TARGETS) V=$(V)
endif

makedirs:
	@if [ ! -d $(BUILD_DIR) ];then $(MKDIR) $(BUILD_DIR);fi
	@if [ ! -d $(B) ];then $(MKDIR) $(B);fi
	@if [ ! -d $(B)/ui ];then $(MKDIR) $(B)/ui;fi
	@if [ ! -d $(B)/client ];then $(MKDIR) $(B)/client;fi
	@if [ ! -d $(B)/qcommon ];then $(MKDIR) $(B)/qcommon;fi
	@if [ ! -d $(B)/qshared ];then $(MKDIR) $(B)/qshared;fi
	@if [ ! -d $(B)/render ];then $(MKDIR) $(B)/render;fi
	@if [ ! -d $(B)/server ];then $(MKDIR) $(B)/server;fi
	@if [ ! -d $(B)/win32 ];then $(MKDIR) $(B)/win32;fi
	@if [ ! -d $(B)/baseq2 ];then $(MKDIR) $(B)/baseq2;fi
	@if [ ! -d $(B)/baseq2/game ];then $(MKDIR) $(B)/baseq2/game;fi

Q2OBJ = \
  $(B)/client/cinematic.o \
  $(B)/client/cl_demo.o \
  $(B)/client/cl_draw.o \
  $(B)/client/cl_effects.o \
  $(B)/client/cl_ents.o \
  $(B)/client/cl_input.o \
  $(B)/client/cl_keys.o \
  $(B)/client/cl_load.o \
  $(B)/client/cl_localents.o \
  $(B)/client/cl_main.o \
  $(B)/client/cl_parse.o \
  $(B)/client/cl_particles.o \
  $(B)/client/cl_predict.o \
  $(B)/client/cl_screen.o \
  $(B)/client/cl_tempents.o \
  $(B)/client/cl_view.o \
  $(B)/client/console.o \
  $(B)/client/s_dma.o \
  $(B)/client/s_sfx.o \
  $(B)/client/s_stream.o \
  $(B)/qcommon/cmd.o \
  $(B)/qcommon/cmodel.o \
  $(B)/qcommon/common.o \
  $(B)/qcommon/crc.o \
  $(B)/qcommon/cvar.o \
  $(B)/qcommon/filesystem.o \
  $(B)/qcommon/md4.o \
  $(B)/qcommon/memory.o \
  $(B)/qcommon/net_chan.o \
  $(B)/qcommon/net_msg.o \
  $(B)/qcommon/parser.o \
  $(B)/qcommon/pmove.o \
  $(B)/qshared/q_math.o \
  $(B)/qshared/q_shared.o \
  $(B)/render/r_alias.o \
  $(B)/render/r_backend.o \
  $(B)/render/r_batch.o \
  $(B)/render/r_debug.o \
  $(B)/render/r_decal.o \
  $(B)/render/r_gl.o \
  $(B)/render/r_interaction.o \
  $(B)/render/r_light.o \
  $(B)/render/r_main.o \
  $(B)/render/r_material.o \
  $(B)/render/r_mesh.o \
  $(B)/render/r_model.o \
  $(B)/render/r_program.o \
  $(B)/render/r_render.o \
  $(B)/render/r_texture.o \
  $(B)/render/r_video.o \
  $(B)/render/r_world.o \
  $(B)/server/sv_ccmds.o \
  $(B)/server/sv_ents.o \
  $(B)/server/sv_game.o \
  $(B)/server/sv_init.o \
  $(B)/server/sv_main.o \
  $(B)/server/sv_send.o \
  $(B)/server/sv_user.o \
  $(B)/server/sv_world.o \
  $(B)/ui/ui_advanced.o \
  $(B)/ui/ui_audio.o \
  $(B)/ui/ui_cinematics.o \
  $(B)/ui/ui_controls.o \
  $(B)/ui/ui_credits.o \
  $(B)/ui/ui_defaults.o \
  $(B)/ui/ui_demos.o \
  $(B)/ui/ui_gameoptions.o \
  $(B)/ui/ui_gotosite.o \
  $(B)/ui/ui_ingame.o \
  $(B)/ui/ui_loadgame.o \
  $(B)/ui/ui_main.o \
  $(B)/ui/ui_menu.o \
  $(B)/ui/ui_mods.o \
  $(B)/ui/ui_multiplayer.o \
  $(B)/ui/ui_network.o \
  $(B)/ui/ui_options.o \
  $(B)/ui/ui_performance.o \
  $(B)/ui/ui_playersetup.o \
  $(B)/ui/ui_qmenu.o \
  $(B)/ui/ui_quit.o \
  $(B)/ui/ui_savegame.o \
  $(B)/ui/ui_singleplayer.o \
  $(B)/ui/ui_video.o \
  $(B)/baseq2/game/m_flash.o \
  $(B)/win32/alw_win.o \
  $(B)/win32/cd_win.o \
  $(B)/win32/editor_win.o \
  $(B)/win32/glw_win.o \
  $(B)/win32/in_win.o \
  $(B)/win32/net_win.o \
  $(B)/win32/qal_win.o \
  $(B)/win32/qgl_win.o \
  $(B)/win32/sys_win.o \
  $(B)/win32/vid_win.o \
  $(B)/win32/q2e.res

$(B)/q2e.$(ARCH)$(BINEXT): $(Q2OBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) -o $@ $(Q2OBJ) $(CLIENT_LDFLAGS) $(LDFLAGS)


#### Game DLL/Shared Object Dependencies #####################

Q2GOBJ = \
  $(B)/baseq2/game/g_ai.o \
  $(B)/baseq2/game/g_chase.o \
  $(B)/baseq2/game/g_cmds.o \
  $(B)/baseq2/game/g_combat.o \
  $(B)/baseq2/game/g_func.o \
  $(B)/baseq2/game/g_items.o \
  $(B)/baseq2/game/g_main.o \
  $(B)/baseq2/game/g_misc.o \
  $(B)/baseq2/game/g_monster.o \
  $(B)/baseq2/game/g_phys.o \
  $(B)/baseq2/game/g_save.o \
  $(B)/baseq2/game/g_spawn.o \
  $(B)/baseq2/game/g_svcmds.o \
  $(B)/baseq2/game/g_target.o \
  $(B)/baseq2/game/g_trigger.o \
  $(B)/baseq2/game/g_turret.o \
  $(B)/baseq2/game/g_utils.o \
  $(B)/baseq2/game/g_weapon.o \
  $(B)/baseq2/game/m_actor.o \
  $(B)/baseq2/game/m_berserk.o \
  $(B)/baseq2/game/m_boss2.o \
  $(B)/baseq2/game/m_boss3.o \
  $(B)/baseq2/game/m_boss31.o \
  $(B)/baseq2/game/m_boss32.o \
  $(B)/baseq2/game/m_brain.o \
  $(B)/baseq2/game/m_chick.o \
  $(B)/baseq2/game/m_flash.o \
  $(B)/baseq2/game/m_flipper.o \
  $(B)/baseq2/game/m_float.o \
  $(B)/baseq2/game/m_flyer.o \
  $(B)/baseq2/game/m_gladiator.o \
  $(B)/baseq2/game/m_gunner.o \
  $(B)/baseq2/game/m_hover.o \
  $(B)/baseq2/game/m_infantry.o \
  $(B)/baseq2/game/m_insane.o \
  $(B)/baseq2/game/m_medic.o \
  $(B)/baseq2/game/m_move.o \
  $(B)/baseq2/game/m_mutant.o \
  $(B)/baseq2/game/m_parasite.o \
  $(B)/baseq2/game/m_soldier.o \
  $(B)/baseq2/game/m_supertank.o \
  $(B)/baseq2/game/m_tank.o \
  $(B)/baseq2/game/p_client.o \
  $(B)/baseq2/game/p_hud.o \
  $(B)/baseq2/game/p_trail.o \
  $(B)/baseq2/game/p_view.o \
  $(B)/baseq2/game/p_weapon.o \
  $(B)/qshared/q_math.o \
  $(B)/qshared/q_shared.o

$(B)/baseq2/game$(ARCH).$(SHLIBEXT): $(Q2GOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(SHLIBLDFLAGS) -o $@ $(Q2GOBJ)


#### Game DLL/Shared Object build ###########################

$(B)/baseq2/game/%.o: $(GDIR)/%.c
	$(DO_GAME_CC)


#### Client/Server build ####################################

$(B)/client/%.o: $(CDIR)/%.c
	$(DO_CC)

$(B)/qcommon/%.o: $(CMDIR)/%.c
	$(DO_CC)

$(B)/qshared/%.o: $(QSDIR)/%.c
	$(DO_CC)

$(B)/render/%.o: $(RDIR)/%.c
	$(DO_CC)

$(B)/server/%.o: $(SDIR)/%.c
	$(DO_CC)

$(B)/ui/%.o: $(UIDIR)/%.c
	$(DO_CC)

$(B)/win32/%.o: $(W32DIR)/%.c
	$(DO_CC)

$(B)/win32/q2e.res: misc/q2e.rc
	$(DO_WINDRES)

#### Misc. Build Stuff ######################################

OBJ = $(Q2OBJ) $(Q2GOBJ)

copyfiles: release
	@if [ ! -d $(COPYDIR)/baseq2 ]; then echo "You need to set COPYDIR to where your game data is!"; fi
	-$(MKDIR) -p -m 0755 $(COPYDIR)/baseq2

	$(INSTALL) -s -m 0755 $(BR)/q2e.$(ARCH)$(BINEXT) $(COPYDIR)/q2e.$(ARCH)$(BINEXT)

	$(INSTALL) -s -m 0755 $(BR)/baseq2/game$(ARCH).$(SHLIBEXT) \
					$(COPYDIR)/baseq2/.


clean: clean-debug clean-release
ifeq ($(PLATFORM),mingw32)
	@$(MAKE) -C $(NSISDIR) clean
else
	@$(MAKE) -C $(LOKISETUPDIR) clean
endif

clean-debug:
	@$(MAKE) clean2 B=$(BD)

clean-release:
	@$(MAKE) clean2 B=$(BR)

clean2:
	@echo "CLEAN $(B)"
	@rm -f $(OBJ)
	@rm -f $(TARGETS)

distclean: clean
	@rm -rf $(BUILD_DIR)

installer: release
ifeq ($(PLATFORM),mingw32)
	@$(MAKE) VERSION=$(VERSION) -C $(NSISDIR) V=$(V)
else
	@$(MAKE) VERSION=$(VERSION) -C $(LOKISETUPDIR) V=$(V)
endif

dist:
	rm -rf q2e-$(VERSION)
	svn export . q2e-$(VERSION)
	tar --owner=root --group=root --force-local -cjf q2e-$(VERSION).tar.bz2 q2e-$(VERSION)
	rm -rf q2e-$(VERSION)

.PHONY: all clean clean2 clean-debug clean-release copyfiles \
	debug default dist distclean installer makedirs \
	release targets
