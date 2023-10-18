#include <GL/glew.h>
#include <GL/glut.h>
#include <pthread.h>

#include "shaders.h"

#include <stdio.h>
#include "texture.h"
#include "file.h"

#define SAVE_EXTRACT

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

    uint32_t *in = malloc(src->width*src->height*sizeof(uint32_t));

    for (int i = 0; i< src->width*src->height; i++) {
      in[i] = convert_to_rgb(src->pixels[i]);
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D, 0,           /* target, level */
        GL_RGBA,                    /* internal format */
        src->width, src->height, 0,           /* width, height, border */
        GL_RGBA, GL_UNSIGNED_BYTE,   /* external format, type */
        in                      /* pixels */
    );
    free(in);
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
    g_resources.dstTexture->id = 0;
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
    int paletteSize = 0;
    LOGD("Write format = %d\n", g_resources.srcTexture->format);
    switch (g_resources.srcTexture->format) {
      case COLOR_BANK_16_COL:
      case LOOKUP_TABLE_16_COL:
      {
        g_resources.dstTexture->length = g_resources.dstTexture->width*g_resources.dstTexture->height/4;
        g_resources.dstTexture->pixels = malloc(g_resources.dstTexture->length*sizeof(rgb1555_t));
        LOGD("Setup palette_id = %d %dx%d\n",g_resources.srcTexture->palette.index_in_file, g_resources.dstTexture->width, g_resources.dstTexture->height);
        g_resources.dstTexture->palette_id = g_resources.srcTexture->palette.index_in_file;
        LOGD("Nb pix = %d format %d\n", g_resources.dstTexture->height*g_resources.dstTexture->width, g_resources.srcTexture->format);
        for (int i=0; i<g_resources.dstTexture->height*g_resources.dstTexture->width; i+=4) {
          rgb1555_t a = out[i]&0xFFFF;
          rgb1555_t b = out[i+1]&0xFFFF;
          rgb1555_t c = out[i+2]&0xFFFF;
          rgb1555_t d = out[i+3]&0xFFFF;
          uint8_t outa = 0xFF;
          uint8_t outb = 0xFF;
          uint8_t outc = 0xFF;
          uint8_t outd = 0xFF;
          for (int j=0; j<16; j++) {
            if (a == g_resources.srcTexture->palette.pixels[j]) outa = j;
            if (b == g_resources.srcTexture->palette.pixels[j]) outb = j;
            if (c == g_resources.srcTexture->palette.pixels[j]) outc = j;
            if (d == g_resources.srcTexture->palette.pixels[j]) outd = j;
          }
          if ((outa == 0xFF)||(outb == 0xFF)||(outc == 0xFF)||(outd == 0xFF))
          {
            LOGD("Error pixel not matched on palette - should not happen - texture %d loop %d\n", nbImage, i);
            LOGD("Pix 0x%x 0x%x 0x%x 0x%x\n", a, b, c, d);
            LOGD("Palette:\n");
            for (int j=0; j<16; j++) {
              LOGD("\t 0x%x\n", g_resources.srcTexture->palette.pixels[j]);
            }
            exit(-1);
          }
          g_resources.dstTexture->pixels[i/4] = ((outa&0xF)<<12)|((outb&0xF)<<8)|((outc&0xF)<<4)|(outd&0xF);
        }
        break;
      }
      // COLOR_BANK_256_COL:
      //   if (paletteSize < 256) paletteSize = 256;
      // COLOR_BANK_128_COL:
      //   if (paletteSize < 128) paletteSize = 128;
    	// COLOR_BANK_64_COL:
      //   if (paletteSize < 64) paletteSize = 64;
      //   {
      //     g_resources.dstTexture->length = g_resources.dstTexture->width*g_resources.dstTexture->height/2;
      //     g_resources.dstTexture->pixels = malloc(g_resources.dstTexture->length);
      //     for (int i=0; i<g_resources.dstTexture->height*g_resources.dstTexture->width; i+=2) {
      //       rgb1555_t a = convert_to_rgb(out[i]);
      //       rgb1555_t b = convert_to_rgb(out[i+1]);
      //       for (int j=0; j<paletteSize; j++) {
      //         if (a == g_resources.srcTexture->palette->pixels[j]) a = j;
      //         if (b == g_resources.srcTexture->palette->pixels[j]) b = j;
      //       }
      //       g_resources.dstTexture->pixels[i] = ((a&0xFF)<<8)|(b&0xFF);
      //     }
      //   }
      //   break;
    	case COLOR_BANK_RGB:
      {
        g_resources.dstTexture->length = g_resources.dstTexture->width*g_resources.dstTexture->height;
        g_resources.dstTexture->pixels = malloc(sizeof(rgb1555_t)*g_resources.dstTexture->width*g_resources.dstTexture->height);
        for (int i=0; i<g_resources.dstTexture->height*g_resources.dstTexture->width; i++) {
          g_resources.dstTexture->pixels[i] = rgb155_from_u32(out[i]);
        }
        break;
      }
    }
#ifdef SAVE_EXTRACT
    char png_name[1024] = {0};
		sprintf(png_name, "char_%d.png", nbImage);
		stbi_write_png(png_name, g_resources.dstTexture->width, g_resources.dstTexture->height, 4, out, 0);
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

void gl_generate_texture_from_tris(render_texture_t *out, tris_t *t, texture_t *texture)
{
  int16_t length = 0;
  int16_t delta_x = t->vertices[0].uv.x - t->vertices[1].uv.x;
  int16_t delta_y = t->vertices[0].uv.y - t->vertices[1].uv.y;
  int16_t new_length = sqrt(delta_x*delta_x+delta_y*delta_y);
  length = max(length, new_length);
  delta_x = t->vertices[1].uv.x - t->vertices[2].uv.x;
  delta_y = t->vertices[1].uv.y - t->vertices[2].uv.y;
  new_length = sqrt(delta_x*delta_x+delta_y*delta_y);
  length = max(length, new_length);
  delta_x = t->vertices[1].uv.x - t->vertices[2].uv.x;
  delta_y = t->vertices[1].uv.y - t->vertices[2].uv.y;
  length = max(length, new_length);

  int16_t height = length;
  int16_t width = (length+0x7)&~0x7;
  // if (width*height > 1024) {
  //   double ratio = 1024.0/(width*height);
  //   width = ((int)(width * ratio)+0x7)& ~0x7;
  //   if (width == 0) width = 8;
  //   height = 1024/width;
  // }
  if (width == 0) width = 8;
  if (height == 0) height = 1;

  LOGD("Got %dx%d => %d\n", width, height, width*height*2);

  g_vertex_buffer_data[0] = -1.0f;
  g_vertex_buffer_data[1] = -1.0f;
  g_vertex_buffer_data[2] = (float)width/160.0f - 1.0f;
  g_vertex_buffer_data[3] = -1.0f;
  g_vertex_buffer_data[4] = -1.0f;
  g_vertex_buffer_data[5] = (float)height/120.0f - 1.0f;
  g_vertex_buffer_data[6] = (float)width/160.0f - 1.0f;
  g_vertex_buffer_data[7] = (float)height/120.0f - 1.0f;

  out->width = width;
  out->height = height;

  g_texcoord_buffer_data[0] = (float)t->vertices[0].uv.x / (float)texture->width;
  g_texcoord_buffer_data[1] = (float)t->vertices[0].uv.y / (float)texture->height;
  g_texcoord_buffer_data[2] = (float)t->vertices[1].uv.x / (float)texture->width;
  g_texcoord_buffer_data[3] = (float)t->vertices[1].uv.y / (float)texture->height;
  g_texcoord_buffer_data[4] = (float)t->vertices[2].uv.x / (float)texture->width;
  g_texcoord_buffer_data[5] = (float)t->vertices[2].uv.y / (float)texture->height;
  g_texcoord_buffer_data[6] = (float)t->vertices[2].uv.x / (float)texture->width;
  g_texcoord_buffer_data[7] = (float)t->vertices[2].uv.y / (float)texture->height;

  g_resources.srcTexture = texture;
  g_resources.dstTexture = out;
  g_resources.ready = 1;
  //wait for rendering
  while (g_resources.ready != 0);
}

void gl_generate_texture_from_quad(render_texture_t *out, quads_t *t, texture_t *texture)
{
  int16_t width = 0;
  for (int i = 0; i<4; i++) {
    int16_t delta_x = t->vertices[i].uv.x - t->vertices[(i+2)%4].uv.x;
    int16_t delta_y = t->vertices[i].uv.y - t->vertices[(i+2)%4].uv.y;
    int16_t new_length = sqrt(delta_x*delta_x+delta_y*delta_y);
    width = max(width, new_length);
  }

  int16_t height = 0;
  for (int i = 0; i<4; i+=2) {
    int16_t delta_x = t->vertices[i].uv.x - t->vertices[(i+1)%4].uv.x;
    int16_t delta_y = t->vertices[i].uv.y - t->vertices[(i+1)%4].uv.y;
    int16_t new_length = sqrt(delta_x*delta_x+delta_y*delta_y);
    height = max(height, new_length);
  }
  width = (width+0x7)&~0x7;

// LOGD("Was %dx%d => %d\n", width, height, width*height);
//     if (width*height > 4096) {
//       double ratio = 4096.0/(width*height);
//       width = ((int)(width * ratio)+0x7)& ~0x7;
//       if (width == 0) width = 8;
//       height = 4096/width;
//     }
//     LOGD("Got %dx%d => %d\n", width, height, width*height);

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

  g_texcoord_buffer_data[0] = (float)t->vertices[0].uv.x / (float)texture->width;
  g_texcoord_buffer_data[1] = (float)t->vertices[0].uv.y / (float)texture->height;
  g_texcoord_buffer_data[2] = (float)t->vertices[1].uv.x / (float)texture->width;
  g_texcoord_buffer_data[3] = (float)t->vertices[1].uv.y / (float)texture->height;
  g_texcoord_buffer_data[4] = (float)t->vertices[2].uv.x / (float)texture->width;
  g_texcoord_buffer_data[5] = (float)t->vertices[2].uv.y / (float)texture->height;
  g_texcoord_buffer_data[6] = (float)t->vertices[3].uv.x / (float)texture->width;
  g_texcoord_buffer_data[7] = (float)t->vertices[3].uv.y / (float)texture->height;

  LOGD("Origin UV %dx%d %dx%d %dx%d %dx%d\n",
    t->vertices[0].uv.x,
    t->vertices[0].uv.y,
    t->vertices[1].uv.x,
    t->vertices[1].uv.y,
    t->vertices[2].uv.x,
    t->vertices[2].uv.y,
    t->vertices[3].uv.x,
    t->vertices[3].uv.y
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
