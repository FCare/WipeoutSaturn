GL_INCLUDE = /usr/X11R6/include
GL_LIB = /usr/X11R6/lib

BUILD_DIR = build

BUILD_SRC= \
	convertModel.c \
	image.c \
	object.c \
	texture.c \
	file.c \
	lzss.c \
	type.c \
	gl.c

COMMON_OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(BUILD_SRC))
COMMON_DEPS = $(patsubst %.c, $(BUILD_DIR)/%.d, $(BUILD_SRC))

convertModel: $(COMMON_OBJ)
	gcc -o $(BUILD_DIR)/convertModel $^ -L$(GL_LIB) -lGL -lglut -lGLEW -lm

$(BUILD_DIR):
		mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	gcc -c -o $@ $< -I$(GL_INCLUDE)

clean:
	rm -f $(BUILD_DIR)/convertModel $(BUILD_DIR)/*.o


