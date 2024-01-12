GL_INCLUDE = /usr/X11R6/include
GL_LIB = /usr/X11R6/lib

LIBXML_INCLUDE = /usr/include/libxml2
LIBXML_LIB = /usr/lib/x86_64-linux-gnu

BUILD_DIR = build

BUILD_SRC= \
	convertX3D.c \
	pngUtils.c \
	gl.c \
	# image.c \
	# object.c \
	# texture.c \
	# file.c \
	# lzss.c \
	# type.c \
	# mem.c

COMMON_OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(BUILD_SRC))
COMMON_DEPS = $(patsubst %.c, $(BUILD_DIR)/%.d, $(BUILD_SRC))


convertModel: $(COMMON_OBJ)
	gcc -Wall -g -o $(BUILD_DIR)/convertX3D $^  -L$(GL_LIB) -L$(LIBXML_LIB) -lGL -lglut -lGLEW -lxml2 -lpng -lm

$(BUILD_DIR):
		mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	gcc -c -DLOGD=printf -o $@ $< -I$(GL_INCLUDE) -I$(LIBXML_INCLUDE)

.PHONY: clean
clean:
	rm -f $(BUILD_DIR)/convertX3D $(BUILD_DIR)/*.o

-include $(COMMON_DEPS)
