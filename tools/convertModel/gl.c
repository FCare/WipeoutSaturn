#include <GL/glew.h>
#include <GL/glut.h>

#include <stdio.h>

static void render(void)
{
  //Do the texture rendering here
    glutSwapBuffers();
}

static int make_resources(void)
{
  //prepare the gl resources here
  return 1;
}

int gl_init(void) {
  int arg=0;
  glutInit(&arg, NULL);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowSize(240, 320);
  glutCreateWindow("convert texture");
  glutDisplayFunc(&render);

  glewInit();
  if (!GLEW_VERSION_2_0) {
      fprintf(stderr, "OpenGL 2.0 not available\n");
      return 1;
  }

  if (!make_resources()) {
      fprintf(stderr, "Failed to load resources\n");
      return 1;
  }

  glutMainLoop();
  return 0;
}
