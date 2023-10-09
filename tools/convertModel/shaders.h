#ifndef __SHADER_H__
#define __SHADER_H__

static const GLchar vertex[] =
"#version 330 core\n"
"layout (location = 0) in vec2 position; \n"
"layout (location = 1) in vec2 texCoord; \n"
"out vec2 texCoordV; \n"
"void main() { \n"
"    texCoordV = texCoord; \n"
"    gl_Position = vec4(position.x, position.y, 1.0, 1.0); \n"
"} \n";

static const GLchar fragment[] =
"#version 330 core\n"
"in vec2 texCoordV; \n"
"out vec4 colorOut; \n"
"uniform sampler2D Texture;\n"
"void main() { \n"
"  colorOut = texture(Texture, texCoordV); \n"
"} \n";

#endif
