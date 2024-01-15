#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define FIX16(A) (int)(((A)*65536.0)+0.5)

/* tweak TABLE_BITS if you need a smaller table */
//M_PI_2 divided by (1<<PRECISION_BITS)
#define PRECISION_BITS 9
#define TABLE_SIZE (FIX16(M_PI_2)/(1<<PRECISION_BITS))

int *lut;

void
lut_init(void)
{
	int i;
  lut = (int*) malloc((TABLE_SIZE+1) * sizeof(int));
  int mPI2 = FIX16(M_PI_2);
	for (i = 0; i < TABLE_SIZE + 1; i++) {
    float val = (1<<PRECISION_BITS)*i / 65536.0;
    lut[i] = FIX16(sin(val));
  }
}

int
lut_sinf(int phase)
{
  int quadrant = (phase/FIX16(M_PI_2))%4;
  int index = 0;
  switch(quadrant){
    case 0:
      index = (phase%FIX16(M_PI_2))>>PRECISION_BITS;
      return lut[index];
    case 1:
      index = TABLE_SIZE-((phase%FIX16(M_PI_2))>>PRECISION_BITS);
      return lut[index];
    case 2:
      index = (phase%FIX16(M_PI_2))>>PRECISION_BITS;
      return -lut[index];
      break;
    case 3:
      index = TABLE_SIZE-((phase%FIX16(M_PI_2))>>PRECISION_BITS);
      return -lut[index];
      break;
    default:
      break;
  }
  return 0;
}

void main(void) {
  lut_init();
#ifdef EXPORT_GNUPLOT
//Export to a file showable in gnuplot
// gcc -o makeSin makeSin.c -lm
// ./makeSin  > tableTest
// gnuplot -p -e "plot 'tableTest'"
  int nbPoint = 22512;
  for (int i=0; i<nbPoint; i++) {
    //Try to check if it computes for every 90/1024 degrees
    double degree = (1080.0/(nbPoint-1)) * i;
    int val =  FIX16(degree * (M_PI/180.0));
    int result = lut_sinf(val);
    printf("%f %f\n", degree, result/65536.0);
  }
#else
//export the LUT to copy to C code
  printf("#define TABLE_SIZE %d\n", TABLE_SIZE);
  printf("#define PRECISION_BITS %d\n", PRECISION_BITS);
  printf("static const fix16_t sin_lut[TABLE_SIZE+1]={");
  for (int i = 0; i<TABLE_SIZE; i++) {
    if ((i%8)==0)printf("\n");
    printf("0x%08x,", lut[i]);
  }
  printf("0x%08x\n};\n", lut[TABLE_SIZE]);
  printf("fix16_t sin(fix16_t phase)\n");
  printf("{\n");
  printf("  while(phase < FIX16_ZERO) {\n");
  printf("    phase += PLATFORM_2PI;\n");
  printf("  \n");
  printf("  int quadrant = (phase/PLATFORM_PI_2)%%4;\n");
  printf("  int index = 0;\n");
  printf("  switch(quadrant){\n");
  printf("    case 0:\n");
  printf("      index = (phase%%PLATFORM_PI_2)>>PRECISION_BITS;\n");
  printf("      return sin_lut[index];\n");
  printf("    case 1:\n");
  printf("      index = TABLE_SIZE-((phase%%PLATFORM_PI_2)>>PRECISION_BITS);\n");
  printf("      return sin_lut[index];\n");
  printf("    case 2:\n");
  printf("      index = (phase%%PLATFORM_PI_2)>>PRECISION_BITS;\n");
  printf("      return -sin_lut[index];\n");
  printf("      break;\n");
  printf("    case 3:\n");
  printf("      index = TABLE_SIZE-((phase%%PLATFORM_PI_2)>>PRECISION_BITS);\n");
  printf("      return -sin_lut[index];\n");
  printf("      break;\n");
  printf("    default:\n");
  printf("      break;\n");
  printf("  }\n");
  printf("  return 0;\n");
  printf("}\n");
#endif
}