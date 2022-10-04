
#ifndef __cpoint_lexer__
#define __cpoint_lexer__

enum Token{
    // end of file token
    tok_eof = -1,

    // function keyword
    tok_function = -2,

    // extern keyword
    tok_extern = -3,

    // function names and variable names
    tok_identifier = -4,

    // numbers
    tok_number = -5,
    tok_next_line = -6,
};

typedef struct {
	enum Token type;
	void* data;
} token_t;

int getNextToken();
int getTok();

#endif
