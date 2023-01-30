
#include <stdio.h>
#include <stdarg.h>

double printd(double X){
    fprintf(stderr, "%f\n", X);
    return 0;
}

void printi(int x){
  fprintf(stderr, "%d\n", x);
}

double putchard(double X) {
  fputc((char)X, stderr);
  return 0;
}
double printptr(void* ptr){
  fprintf(stderr, "%p\n", ptr);
  return 0;
}

void printfmt(char* format, ...){
	va_list va;
	va_start(va,format);
	vprintf(format,va);
}

/*void printstr(char* x){
  puts(x);
  return;
}*/