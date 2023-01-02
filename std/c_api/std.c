#include <stdio.h>

double printd(double X){
    fprintf(stderr, "%f\n", X);
    return 0;
}

double putchard(double X) {
  fputc((char)X, stderr);
  return 0;
}

double printptr(void* ptr){
  fprintf(stderr, "%p\n", ptr);
}