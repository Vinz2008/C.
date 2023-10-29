#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>

#define CUSTOM_PRINTF_C 1

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

#if CUSTOM_PRINTF_C

char* internal_itoa(int val, int base){
	static char buf[32] = {0};
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base]; // TODO : make numbers uppercase
	return &buf[i+1];
}

void putstring(char* s){
    while(*s) putchar(*s++);
}

// printf reimplemented
// TODO : have a formatted  
void printfmt(char* format, ...){
    //puts("start printfmt");
    va_list va;
    char* s;
    char c;
    int i;
    double d;
    uintptr_t p;
    char* string_number;
	va_start(va,format);
    char* temp_buffer = malloc(64 * sizeof(char)); 
    char *traverse = format;
    while(*traverse != '\0'){
        if(*traverse == '%'){
            //puts("% found");
            traverse++;
            switch (*traverse){
                case 's':
                    s = va_arg(va, char*);
                    putstring(s);
                    break;
                case 'c':
                    c = va_arg(va, int);   
                    putchar(c);
                    break;
                case 'p':
                    p = (uintptr_t)va_arg(va, void*);
                    putstring("0x");
                    putstring(internal_itoa((int)p, 16));
                    break;
                // add here case 'o', case 'u', etc
                case 'i':
                case 'd':
                case 'x':
                    i = va_arg(va, int);
                    if (*traverse != 'x'){
                        if(i < 0){
                            putchar('-');
                            i = i * -1;
                        }
                    }
                    if (*traverse == 'x'){
                        string_number = internal_itoa(i, 16);
                    } else {
                        string_number = internal_itoa(i, 10);
                    }
                    putstring(string_number);
                    break;
                case 'f':
                    d = va_arg(va, double);
                    gcvt(d,10,temp_buffer);
                    putstring(temp_buffer);
                    break;
            }
        } else {
            putchar(*traverse);
        }
        traverse++;
    }
    va_end(va);
}

#else

void printfmt(char* format, ...){
	va_list va;
	va_start(va,format);
	vprintf(format,va);
    va_end(va);
}

#endif

/*void printstr(char* x){
  puts(x);
  return;
}*/