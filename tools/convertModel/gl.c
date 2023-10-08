#include <GL/glew.h>
#include <GL/glut.h>
#include <pthread.h>

#include "shaders.h"

#include <stdio.h>
#include "texture.h"
#include "file.h"

#define min(A,B) ((A)<(B)?(A):(B))
#define max(A,B) ((A)>(B)?(A):(B))

static volatile int jobToProcess;
static GLfloat g_vertex_buffer_data[8] =
  {
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f
  };

static GLfloat g_texcoord_buffer_data[8] =
  {
    0.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 0.0f
  };

static struct {
    GLuint vertex_buffer, texcoord_buffer;
    GLint texture;
    texture_t *srcTexture;
    GLuint vertex_shader, fragment_shader, program;
    uint8_t volatile ready;
    render_func step;
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
    glBufferData(target, buffer_size, buffer_data, GL_DYNAMIC_DRAW);
    return buffer;
}

static void update_buffer(
    GLuint buffer,
    GLenum target,
    const void *buffer_data,
    GLsizei buffer_size
) {
    glBindBuffer(target, buffer);
    glBufferData(target, buffer_size, buffer_data, GL_DYNAMIC_DRAW);
}

static GLuint make_texture(texture_t *src)
{
    GLuint texture;

    if (!src->pixels)
        return 0;

        printf("Make texture %dx%d\n", src->width, src->height);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D, 0,           /* target, level */
        GL_RGBA,                    /* internal format */
        src->width, src->height, 0,           /* width, height, border */
        GL_RGBA, GL_UNSIGNED_BYTE,   /* external format, type */
        src->pixels                      /* pixels */
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
  g_resources.texcoord_buffer = make_buffer(
        GL_ARRAY_BUFFER,
        g_texcoord_buffer_data,
        sizeof(g_texcoord_buffer_data)
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
  if (g_resources.ready != 0) {

    update_buffer(
      g_resources.vertex_buffer,
      GL_ARRAY_BUFFER,
      g_vertex_buffer_data,
      sizeof(g_vertex_buffer_data)
    );
    update_buffer(
      g_resources.texcoord_buffer,
      GL_ARRAY_BUFFER,
      g_texcoord_buffer_data,
      sizeof(g_texcoord_buffer_data)
    );
    //Get the texture.
    if (g_resources.texture != -1) glDeleteTextures(1, &g_resources.texture);
    g_resources.texture = make_texture(g_resources.srcTexture);
    //Do the texture rendering here
    glUseProgram(g_resources.program);
    glUniform1i(glGetUniformLocation(g_resources.program, "Texture"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_resources.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindBuffer(GL_ARRAY_BUFFER, g_resources.vertex_buffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, g_resources.texcoord_buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    g_resources.ready = 0;
    glutSwapBuffers();
  }
}

static void idle(void) {
    int ended = 0;
    if (g_resources.ready == 0xFF)
      exit(EXIT_SUCCESS);
    if (g_resources.ready != 0)
      glutPostRedisplay();
}

void gl_generate_texture_from_tris(render_texture_t *out, tris_t *t, texture_t *texture)
{
  printf("Rendering Tris\n");
  while (g_resources.ready != 0);
  int16_t left = min(min(t->vertices[0].uv.x, t->vertices[1].uv.x), t->vertices[2].uv.x);
  int16_t right = max(max(t->vertices[0].uv.x, t->vertices[1].uv.x), t->vertices[2].uv.x);
  int16_t bottom = min(min(t->vertices[0].uv.y, t->vertices[1].uv.y), t->vertices[2].uv.y);
  int16_t top = max(max(t->vertices[0].uv.y, t->vertices[1].uv.y), t->vertices[2].uv.y);
  int16_t width = ((right-left)+0x7)& ~0x7;
  int16_t height = (top-bottom);

  g_vertex_buffer_data[0] = -1.0f;
  g_vertex_buffer_data[1] = (float)height/240.0f - 1.0f;
  g_vertex_buffer_data[2] = (float)width/320.0f - 1.0f;
  g_vertex_buffer_data[3] = (float)height/240.0f - 1.0f;
  g_vertex_buffer_data[4] = -1.0f;
  g_vertex_buffer_data[5] = -1.0f;
  g_vertex_buffer_data[6] = (float)width/320.0f - 1.0f;
  g_vertex_buffer_data[7] = -1.0f;

  printf("Orig %d %d %d\n", t->vertices[0].uv.x, t->vertices[1].uv.x,t->vertices[2].uv.x);

  printf("tris %dx%d (%dx%d) => %f %f %f %f\n", width, height, texture->width, texture->height, g_vertex_buffer_data[1], g_vertex_buffer_data[2], g_vertex_buffer_data[3], g_vertex_buffer_data[6]);

//diviser suivant la texture
  g_texcoord_buffer_data[0] = (float)t->vertices[0].uv.x / (float)texture->width;
  g_texcoord_buffer_data[1] = (float)t->vertices[0].uv.y / (float)texture->height;
  g_texcoord_buffer_data[2] = (float)t->vertices[1].uv.x / (float)texture->width;
  g_texcoord_buffer_data[3] = (float)t->vertices[1].uv.y / (float)texture->height;
  g_texcoord_buffer_data[4] = (float)t->vertices[2].uv.x / (float)texture->width;
  g_texcoord_buffer_data[5] = (float)t->vertices[2].uv.y / (float)texture->height;
  g_texcoord_buffer_data[6] = (float)t->vertices[1].uv.x / (float)texture->width;
  g_texcoord_buffer_data[7] = (float)t->vertices[1].uv.y / (float)texture->height;
  g_resources.srcTexture = texture;
  g_resources.ready = 1;
}

void *conversion(void *arg) {
  int ended = 0;
  while ( ended == 0) {
    if (g_resources.ready == 0) {
      if (g_resources.step()) glutPostRedisplay();
      else ended = 1;
    }
  }
  g_resources.ready = 0xFF;
	// Arrêt propre du thread
	pthread_exit(EXIT_SUCCESS);
}

int gl_init(render_func func) {
  int arg=0;
  pthread_t conv;
  g_resources.ready = 0;
  g_resources.step = func;
  g_resources.texture = -1;
  glutInit(&arg, NULL);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowSize(320, 240);
  glutIdleFunc(idle);
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

pthread_create(&conv, NULL, conversion, NULL);


  glutMainLoop();
  return 0;
}
