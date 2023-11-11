//#include <stdio.h>
#define hello_world "hello world"
#define a(b) b + 2
char test(char* test_str);
double test2(int number);
void test3(char c);
int test4(double arg);
struct hello {
    int a;
    double b;
};
typedef int test_t;
int testfunction(){
    return 1;
}
void function_with_struct_arg(struct hello a_arg);
enum a {
    a1,
    a2,
};
enum b {
    b1 = 1,
    b2 = 2,
};