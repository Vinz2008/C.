#include <stdio.h>
#include <stdarg.h>

double cprintd(double X){
    fprintf(stderr, "%f\n", X);
    return 0;
}

void cprinti(int x){
  fprintf(stderr, "%d\n", x);
}

double cputchard(double X) {
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