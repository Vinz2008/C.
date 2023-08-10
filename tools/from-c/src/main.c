#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <clang-c/Index.h>

enum CXCursorKind cursorKind;
FILE* outf;
bool is_in_function = false;

enum CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data){
	cursorKind = clang_getCursorKind(cursor);
	CXString clangstr_cursor_name = clang_getCursorSpelling(cursor);
	CXString clangstr_cursor_kind = clang_getCursorKindSpelling(cursorKind);
	printf("cursor %s kind : %s\n", clang_getCString(clangstr_cursor_name), clang_getCString(clangstr_cursor_kind));
	switch(cursorKind){
	case CXCursor_FunctionDecl:
	    fprintf(outf, "func %s(){\n", clang_getCString(clangstr_cursor_name));
	    is_in_function = true;
	    break;
	case CXCursor_VarDecl:
	    fprintf(outf, "var %s\n", clang_getCString(clangstr_cursor_name));
	    break;
	default:
	    break;
	}
	clang_disposeString(clangstr_cursor_kind);
	clang_disposeString(clangstr_cursor_name);
	return CXChildVisit_Recurse;
}

int convertToC(char* path){
	CXIndex index = clang_createIndex(0, 0);
        CXTranslationUnit unit = clang_parseTranslationUnit(index, path, NULL, 0, NULL, 0, CXTranslationUnit_None);
	if (unit == NULL){
            fprintf(stderr, "Unable to parse Translation Unit\n");
            exit(1);
    	}
	CXCursor cursor = clang_getTranslationUnitCursor(unit);
	clang_visitChildren(cursor, cursorVisitor, NULL);
	clang_disposeTranslationUnit(unit);
	clang_disposeIndex(index);
	return 0;
}

int main(int argc, char** argv){
	char* outpath = "out.c";
	outf = fopen(outpath, "w");
	printf("%s\n", argv[1]);
	convertToC(argv[1]);
	fclose(outf);
	return 0;
}
