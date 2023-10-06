#include <GL/glew.h>
#include <GL/glut.h>

#include "shaders.h"

#include <stdio.h>
#include "texture.h"
#include "file.h"

static volatile int jobToProcess;
static const GLfloat g_vertex_buffer_data[8];

static struct {
    GLuint vertex_buffer, element_buffer;
    GLuint texture;
    GLuint vertex_shader, fragment_shader, program;

    struct {
        GLint textures;
    } uniforms;

    struct {
        GLint position;
    } attributes;

} g_resources;


static void show_info_log(
    GLuint object,
    PFNGLGETSHADERIVPROC glGet__iv,
    PFNGLGETSHADERINFOLOGPROC glGet__InfoLog
)
{
    GLint log_length;
    char *log;

    glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
    log = malloc(log_length);
    glGet__InfoLog(object, log_length, NULL, log);
    fprintf(stdout, "%s", log);
    free(log);
}

static GLuint make_buffer(
    GLenum target,
    const void *buffer_data,
    GLsizei buffer_size
) {
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(target, buffer);
    glBufferData(target, buffer_size, buffer_data, GL_STATIC_DRAW);
    return buffer;
}

static GLuint make_texture(void *pixels)
{
    int width, height;
    GLuint texture;

    if (!pixels)
        return 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D, 0,           /* target, level */
        GL_RGB8,                    /* internal format */
        width, height, 0,           /* width, height, border */
        GL_BGR, GL_UNSIGNED_BYTE,   /* external format, type */
        pixels                      /* pixels */
    );
    return texture;
}

static GLuint make_shader(GLenum type, const char *source)
{
    GLuint shader;
    GLint shader_ok;

    if (!source)
        return 0;

    shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
    if (!shader_ok) {
        fprintf(stdout, "Failed to compile GL program\n");
        show_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint make_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLint program_ok;

    GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
    if (!program_ok) {
        fprintf(stdout, "Failed to link shader program:\n");
        show_info_log(program, glGetProgramiv, glGetProgramInfoLog);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

static int make_resources(void)
{
  //prepare the gl resources here
  g_resources.vertex_buffer = make_buffer(
        GL_ARRAY_BUFFER,
        g_vertex_buffer_data,
        sizeof(g_vertex_buffer_data)
    );

  g_resources.vertex_shader = make_shader(
        GL_VERTEX_SHADER,
        vertex
    );
  if (g_resources.vertex_shader == 0)
        return 0;
  g_resources.fragment_shader = make_shader(
      GL_FRAGMENT_SHADER,
      fragment
  );
  if (g_resources.fragment_shader == 0)
      return 0;

  g_resources.program = make_program(g_resources.vertex_shader, g_resources.fragment_shader);
  if (g_resources.program == 0)
      return 0;
  return 1;
}

static void render(void)
{
  //Do the texture rendering here
    glutSwapBuffers();
}


void gl_generate_texture_from_tris(render_texture_t *out, tris_t *t, int16_t texture)
{
  //Get the texture.
  make_texture(NULL);
}

int gl_init(void) {
  int arg=0;
  glutInit(&arg, NULL);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowSize(240, 320);
  glutCreateWindow("convert texture");
  glutDisplayFunc(&render);

  glewExperimental = true;
  glewInit();
  if (!GLEW_VERSION_3_3) {
      fprintf(stdout, "OpenGL 3.3 not available\n");
      return 1;
  }

  if (!make_resources()) {
      fprintf(stdout, "Failed to load resources\n");
      return 1;
  }

  glutMainLoop();
  return 0;
}
