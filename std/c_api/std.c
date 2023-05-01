#include <stdio.h>
#include <stdarg.h>

double printd_internal(double X){
  fprintf(stderr, "%f\n", X);
  return 0;
}

void printi_internal(int x){
  fprintf(stderr, "%d\n", x);
}

double putchard_internal(double X) {
  fputc((char)X, stderr);
  return 0;
}
void printptr(void* ptr){
  fprintf(stderr, "%p\n", ptr);
}

void printfmt(char* format, ...){
	va_list va;
	va_start(va,format);
	vprintf(format,va);
  va_end(va);
}

/*void printstr(char* x){
  puts(x);
  return;
}*/