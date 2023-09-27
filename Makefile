PORT ?= ORIGINAL

COMMON_SRC = \
	src/wipeout/race.c \
	src/wipeout/camera.c \
	src/wipeout/object.c \
	src/wipeout/droid.c \
	src/wipeout/ui.c \
	src/wipeout/hud.c \
	src/wipeout/image.c \
	src/wipeout/game.c \
	src/wipeout/menu.c \
	src/wipeout/main_menu.c \
	src/wipeout/ingame_menus.c \
	src/wipeout/title.c \
	src/wipeout/intro.c \
	src/wipeout/scene.c \
	src/wipeout/ship.c \
	src/wipeout/ship_ai.c \
	src/wipeout/ship_player.c \
	src/wipeout/track.c \
	src/wipeout/weapon.c \
	src/wipeout/particle.c \
	src/types.c \
	src/system.c \
	src/input.c

SFX_SRC= 	src/wipeout/sfx.c

ifeq ($(PORT), Saturn)
# DEBUG=ON PORT=Saturn make -j16 -C ..
DEBUG?=OFF

NO_SFX?=OFF

DEDICATED_SRC = \
	src/saturn/math.c \
	src/saturn/mem.c \
  src/saturn/platform_yaul.c \
	src/saturn/render_saturn.c \
	src/saturn/render_vdp1.c \
	src/saturn/render_vdp2.c \
	src/saturn/utils.c \
	src/saturn/tex.c \
	src/saturn/vdp1_tex.c

ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

SH_SPECS:= yaul.specs yaul-main.specs src/saturn/wipeout.specs

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

ifeq ($(DEBUG), ON)
NO_SFX=ON
USER_CFLAGS += -DDEBUG_PRINT -DLOGD="printf"
else
USER_CFLAGS += -DLOGD=""
endif

SH_LIBRARIES:=
SH_CFLAGS+= -O3 \
						-I. \
						-I/usr/include/stb \
						-Isrc/libs/ \
						-Isrc/ \
						-Isrc/saturn \
						-std=gnu99 \
						-Wall \
						-Wno-unused-variable \
						-DFIXED_MATH \
						-DNO_QUIT \
						-DMEM_HUNK_BYTES=0x200000 \
						$(USER_CFLAGS)

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20230923
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= Wipeout Rewrite
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

SH_PROGRAM:= wipeout-rewrite
SH_SRCS:= \
	$(COMMON_SRC) \
	$(DEDICATED_SRC)

ifeq ($(NO_SFX), OFF)
SH_SRCS += $(SFX_SRC)
else
SH_SRCS += src/saturn/no_sfx.c
endif

include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk

else
CC ?= cc
EMCC ?= emcc
UNAME_S := $(shell uname -s)
RENDERER ?= GL
USE_GLX ?= false
DEBUG ?= false
USER_CFLAGS ?=

L_FLAGS ?= -lm
C_FLAGS ?= -Isrc/libs/ -std=gnu99 -Wall -Wno-unused-variable $(USER_CFLAGS)

ifeq ($(DEBUG), true)
	C_FLAGS := $(C_FLAGS) -g
else
	C_FLAGS := $(C_FLAGS) -O3
endif

# Rendeder ---------------------------------------------------------------------

ifeq ($(RENDERER), GL)
	RENDERER_SRC = src/render_gl.c
	C_FLAGS := $(C_FLAGS) -DRENDERER_GL
else ifeq ($(RENDERER), SOFTWARE)
	RENDERER_SRC = src/render_software.c
	C_FLAGS := $(C_FLAGS) -DRENDERER_SOFTWARE
else
$(error Unknown RENDERER)
endif

ifeq ($(GL_VERSION), GLES2)
	C_FLAGS := $(C_FLAGS) -DUSE_GLES2
endif



# macOS ------------------------------------------------------------------------

ifeq ($(UNAME_S), Darwin)
	BREW_HOME := $(shell brew --prefix)
	C_FLAGS := $(C_FLAGS) -x objective-c -I/opt/homebrew/include -D_THREAD_SAFE -w
	L_FLAGS := $(L_FLAGS) -L$(BREW_HOME)/lib -framework Foundation

	ifeq ($(RENDERER), GL)
		L_FLAGS := $(L_FLAGS) -lGLEW -GLU -framework OpenGL
	endif

	L_FLAGS_SDL = -lSDL2
	L_FLAGS_SOKOL = -framework Cocoa -framework QuartzCore -framework AudioToolbox


# Linux ------------------------------------------------------------------------

else ifeq ($(UNAME_S), Linux)
	ifeq ($(RENDERER), GL)
		L_FLAGS := $(L_FLAGS) -lGLEW

		# Prefer modern GLVND instead of legacy X11-only GLX
		ifeq ($(USE_GLX), true)
			L_FLAGS := $(L_FLAGS) -lGL
		else
			L_FLAGS := $(L_FLAGS) -lOpenGL
		endif
	endif

	L_FLAGS_SDL = -lSDL2
	L_FLAGS_SOKOL = -lX11 -lXcursor -pthread -lXi -ldl -lasound


# Windows MSYS ------------------------------------------------------------------
else ifeq ($(shell uname -o), Msys)
	ifeq ($(RENDERER), GL)
		L_FLAGS := $(L_FLAGS) -lglew32 -lopengl32
	endif

	C_FLAGS := $(C_FLAGS) -DSDL_MAIN_HANDLED -D__MSYS__
	L_FLAGS_SDL = -lSDL2 -lSDL2main
	L_FLAGS_SOKOL = --pthread -ldl -lasound


# Windows NON-MSYS ---------------------------------------------------------------
else ifeq ($(OS), Windows_NT)
	$(error TODO: FLAGS for windows have not been set up. Please modify this makefile and send a PR!)

else
$(error Unknown environment)
endif



# Source files -----------------------------------------------------------------

TARGET_NATIVE ?= wipegame
BUILD_DIR = build/obj/native
BUILD_DIR_WASM = build/obj/wasm

WASM_RELEASE_DIR ?= build/wasm
TARGET_WASM ?= $(WASM_RELEASE_DIR)/wipeout.js
TARGET_WASM_MINIMAL ?= $(WASM_RELEASE_DIR)/wipeout-minimal.js

DEDICATED_SRC = \
	src/mem.c \
	src/utils.c

PORT_SRC= \
	$(COMMON_SRC) \
	$(DEDICATED_SRC) \
	$(RENDERER_SRC) \
	$(SFX_SRC)


# Targets native ---------------------------------------------------------------

COMMON_OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(PORT_SRC))
COMMON_DEPS = $(patsubst %.c, $(BUILD_DIR)/%.d, $(PORT_SRC))

sdl: $(BUILD_DIR)/src/platform_sdl.o
sdl: $(COMMON_OBJ)
	$(CC) $^ -o $(TARGET_NATIVE) $(L_FLAGS) $(L_FLAGS_SDL)

sokol: $(BUILD_DIR)/src/platform_sokol.o
sokol: $(COMMON_OBJ)
	$(CC) $^ -o $(TARGET_NATIVE) $(L_FLAGS) $(L_FLAGS_SOKOL)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) -MMD -MP -c $< -o $@

-include $(COMMON_DEPS)


# Targets wasm -----------------------------------------------------------------

COMMON_OBJ_WASM = $(patsubst %.c, $(BUILD_DIR_WASM)/%.o, $(PORT_SRC))
COMMON_DEPS_WASM = $(patsubst %.c, $(BUILD_DIR_WASM)/%.d, $(PORT_SRC))

wasm: wasm_full wasm_minimal
	cp src/wasm-index.html $(WASM_RELEASE_DIR)/game.html


wasm_full: $(BUILD_DIR_WASM)/src/platform_sokol.o
wasm_full: $(COMMON_OBJ_WASM)
	mkdir -p $(WASM_RELEASE_DIR)
	$(EMCC) $^ -o $(TARGET_WASM) -lGLEW -lGL \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s ENVIRONMENT=web \
		--preload-file wipeout

wasm_minimal: $(BUILD_DIR_WASM)/src/platform_sokol.o
wasm_minimal: $(COMMON_OBJ_WASM)
	mkdir -p $(WASM_RELEASE_DIR)
	$(EMCC) $^ -o $(TARGET_WASM_MINIMAL) -lGLEW -lGL \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s ENVIRONMENT=web \
		--preload-file wipeout \
		--exclude-file wipeout/music \
		--exclude-file wipeout/intro.mpeg

$(BUILD_DIR_WASM):
	mkdir -p $(BUILD_DIR_WASM)

$(BUILD_DIR_WASM)/%.o: %.c
	mkdir -p $(dir $@)
	$(EMCC) $(C_FLAGS) -MMD -MP -c $< -o $@

-include $(COMMON_DEPS_WASM)


.PHONY: clean
clean:
	$(RM) -rf $(BUILD_DIR) $(BUILD_DIR_WASM) $(WASM_RELEASE_DIR)
endif
