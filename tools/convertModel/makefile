GL_INCLUDE = /usr/X11R6/include
GL_LIB = /usr/X11R6/lib

BUILD_DIR = build

convertModel: $(BUILD_DIR)/convertModel.o $(BUILD_DIR)/image.o $(BUILD_DIR)/object.o $(BUILD_DIR)/texture.o $(BUILD_DIR)/file.o $(BUILD_DIR)/lzss.o $(BUILD_DIR)/type.o
	gcc -o $(BUILD_DIR)/convertModel $^ -L$(GL_LIB) -lGL -lglut -lGLEW -lm

$(BUILD_DIR):
		mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	gcc -c -o $@ $< -I$(GL_INCLUDE)

clean:
	rm -f $(BUILD_DIR)/convertModel $(BUILD_DIR)/*.o


