#include <GL/glew.h>
#include <GL/glut.h>
#include <pthread.h>

#include <stdio.h>
#include <math.h>
#include "shaders.h"
#include "type.h"
// #include "file.h"

// #define SAVE_EXTRACT

#ifdef SAVE_EXTRACT
#include "image.h"
#endif

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
    render_texture_t *dstTexture;
    GLuint vertex_shader, fragment_shader, program;
    uint8_t volatile ready;
    step_func step;
    step_func final;
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

static rgb1555_t RGB888_RGB1555(uint8_t msb, uint8_t r, uint8_t g, uint8_t b) {
  return (rgb1555_t)(((msb&0x1)<<15) | ((r>>0x3)<<10) | ((g>>0x3)<<5) | (b>>0x3));
}

rgb1555_t convert_to_rgb(rgba_t val) {
  //RGB 16bits, MSB 1, transparent code 0
  // if (val.a == 0) return RGB888_RGB1555(0,0,0,0);
  // if ((val.b == 0) && (val.r == 0) && (val.g == 0)) {
  //   //Should be black but transparent usage makes it impossible
  //   return RGB888_RGB1555(1,0,0,1);
  // }
  return RGB888_RGB1555(1, val.b, val.g, val.r);
}

rgb1555_t rgb155_from_u32(uint32_t v) {
  rgba_t val;
	val.r = v & 0xFF;
	val.g = (v>>8) & 0xFF;
	val.b = (v>>16) & 0xFF;
	val.a = (v>24) & 0xFF;
	// LOGD("%x %x %x %x\n", val.a, val.b, val.g, val.r);
  return convert_to_rgb(val);
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

    if (!src->pixels) {
      return 0;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
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

int nbImage = 0;
static void render(void)
{
  if (g_resources.ready != 0) {
    nbImage++;
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
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
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
    GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    while (true) {
        GLint status = GL_UNSIGNALED;
        glGetSynciv(fence, GL_SYNC_STATUS, sizeof(GLint), NULL, &status);
        if (status == GL_SIGNALED) {
            break; // rendering is complete
        }
    }
    uint32_t *out=malloc(sizeof(uint32_t)*g_resources.dstTexture->width*g_resources.dstTexture->height);
    glReadPixels(0,0, g_resources.dstTexture->width, g_resources.dstTexture->height, GL_RGBA, GL_UNSIGNED_BYTE, out);
    //COLOR_BANK_RGB:
    g_resources.dstTexture->pixels = malloc(sizeof(rgb1555_t)*g_resources.dstTexture->width*g_resources.dstTexture->height);
    for (int i=0; i<g_resources.dstTexture->height*g_resources.dstTexture->width; i++) {
      g_resources.dstTexture->pixels[i] = rgb155_from_u32(out[i]);
      // printf("Convert %x to %x\n", out[i], g_resources.dstTexture->pixels[i]);
    }

#ifdef SAVE_EXTRACT
    char png_name[1024] = {0};
		sprintf(png_name, "char_%d.png", nbImage);
		stbi_write_png(png_name, g_resources.dstTexture->width, g_resources.dstTexture->height, 4, out, 0);
    if (g_resources.srcTexture->format != COLOR_BANK_RGB) {
      uint32_t *save = malloc(g_resources.dstTexture->width*g_resources.dstTexture->height*sizeof(uint32_t));
      for (int i = 0; i<g_resources.dstTexture->width*g_resources.dstTexture->height/4; i++) {
        uint16_t id = g_resources.dstTexture->pixels[i];
        for (int j=0; j<4; j++) {
          uint8_t pixel_id = (id >> (4*(3-j)))&0xF;
          save[i*4+j] = pixel_id;
          // rgb1555_t val = g_resources.srcTexture->palette.pixels[pixel_id];
          // save[i*4+j] = ((((val>>10)&0x1F)<<3)<<16) | ((((val>>5)&0x1F)<<3)<<8) | (((val&0x1F)<<3)<<0) | 0xFF000000u;
        }
      }
      sprintf(png_name, "index_%d.png", nbImage);
      stbi_write_png(png_name, g_resources.dstTexture->width, g_resources.dstTexture->height, 4, save, 0);
      for (int i = 0; i<g_resources.dstTexture->width*g_resources.dstTexture->height/4; i++) {
        uint16_t id = g_resources.dstTexture->pixels[i];
        for (int j=0; j<4; j++) {
          uint8_t pixel_id = (id >> (4*(3-j)))&0xF;
          rgb1555_t val = g_resources.srcTexture->palette.pixels[pixel_id];
          save[i*4+j] = ((((val>>10)&0x1F)<<3)<<16) | ((((val>>5)&0x1F)<<3)<<8) | (((val&0x1F)<<3)<<0) | 0xFF000000u;
        }
      }
      sprintf(png_name, "color_%d.png", nbImage);
      stbi_write_png(png_name, g_resources.dstTexture->width, g_resources.dstTexture->height, 4, save, 0);
      free(save);
    }
#endif
    free(out);
    g_resources.ready = 0;
    glutSwapBuffers();
  }
}

static void idle(void) {
    int ended = 0;
    if (g_resources.ready == 0xFF) {
      g_resources.final();
      exit(EXIT_SUCCESS);
    }
    if (g_resources.ready != 0)
      glutPostRedisplay();
}

void gl_generate_texture_from_quad(render_texture_t *out, quads_t *t, texture_t *texture)
{
  int16_t width = 0;
  for (int i = 0; i<4; i++) {
    int16_t delta_x = t->uv[i].x - t->uv[(i+2)%4].x;
    int16_t delta_y = t->uv[i].y - t->uv[(i+2)%4].y;
    int16_t new_length = sqrt(delta_x*delta_x+delta_y*delta_y);
    width = max(width, new_length);
  }

  int16_t height = 0;
  for (int i = 0; i<4; i+=2) {
    int16_t delta_x = t->uv[i].x - t->uv[(i+1)%4].x;
    int16_t delta_y = t->uv[i].y - t->uv[(i+1)%4].y;
    int16_t new_length = sqrt(delta_x*delta_x+delta_y*delta_y);
    height = max(height, new_length);
  }
  width = (width+0x7)&~0x7;

  LOGD("Was %dx%d => %d\n", width, height, width*height);
    int shift = 0;
    //limit charcters to 80x60 max
    while((width>>shift) > (320/4)) {
      shift++;
    }
    while((height>>shift) > (240/4)) {
      shift++;
    }
    if (shift != 0) {
      width = ((int)(width>>shift)+0x7)& ~0x7;
      if (width == 0) width = 8;
      height = height>>shift;
    }
    LOGD("Got %dx%d => %d\n", width, height, width*height);

  g_vertex_buffer_data[0] = -1.0f;
  g_vertex_buffer_data[1] = -1.0f;
  g_vertex_buffer_data[2] = -1.0f;
  g_vertex_buffer_data[3] = (float)height/120.0f - 1.0f;
  g_vertex_buffer_data[4] = (float)width/160.0f - 1.0f;
  g_vertex_buffer_data[5] = -1.0f;
  g_vertex_buffer_data[6] = (float)width/160.0f - 1.0f;
  g_vertex_buffer_data[7] = (float)height/120.0f - 1.0f;

  out->width = width;
  out->height = height;

  g_texcoord_buffer_data[0] = (float)t->uv[0].x / (float)texture->width;
  g_texcoord_buffer_data[1] = (float)t->uv[0].y / (float)texture->height;
  g_texcoord_buffer_data[2] = (float)t->uv[3].x / (float)texture->width;
  g_texcoord_buffer_data[3] = (float)t->uv[3].y / (float)texture->height;
  g_texcoord_buffer_data[4] = (float)t->uv[1].x / (float)texture->width;
  g_texcoord_buffer_data[5] = (float)t->uv[1].y / (float)texture->height;
  g_texcoord_buffer_data[6] = (float)t->uv[2].x / (float)texture->width;
  g_texcoord_buffer_data[7] = (float)t->uv[2].y / (float)texture->height;

  LOGD("Origin UV %dx%d %dx%d %dx%d %dx%d\n",
    t->uv[0].x,
    t->uv[0].y,
    t->uv[1].x,
    t->uv[1].y,
    t->uv[2].x,
    t->uv[2].y,
    t->uv[3].x,
    t->uv[3].y
  );
  LOGD("UV %fx%f %fx%f %fx%f %fx%f\n",
  g_texcoord_buffer_data[0],
  g_texcoord_buffer_data[1],
  g_texcoord_buffer_data[2],
  g_texcoord_buffer_data[3],
  g_texcoord_buffer_data[4],
  g_texcoord_buffer_data[5],
  g_texcoord_buffer_data[6],
  g_texcoord_buffer_data[7]);

  g_resources.srcTexture = texture;
  g_resources.dstTexture = out;
  g_resources.ready = 1;
  //wait for rendering
  while (g_resources.ready != 0);
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

int gl_init(step_func func, step_func final) {
  int arg=0;
  pthread_t conv;
  g_resources.ready = 0;
  g_resources.step = func;
  g_resources.final = final;
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
