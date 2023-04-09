#include <stdio.h>
#include <stdlib.h>
#include <clang-c/Index.h>

FILE* outf;

enum CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data){
    enum CXCursorKind cursorKind = clang_getCursorKind(cursor);
    printf("cursor %s kind : %s\n", clang_getCString(clang_getCursorSpelling(cursor)), clang_getCString(clang_getCursorKindSpelling(cursorKind)));
    switch(cursorKind){
        case CXCursor_FunctionDecl:
            fprintf(outf, "func %s();", clang_getCString(clang_getCursorSpelling(cursor)));
        default:
            break;
    }
    return CXChildVisit_Recurse;
}

int main(int argc, char** argv){
    if (argc <= 1){
        fprintf(stderr, "arguments missing\n");
        exit(1);
    }
    for (int i = 0; i < argc; i++){
        printf("%d : %s\n", i, argv[i]);
    }
    outf = fopen("bindings.cpoint", "w");
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(index, argv[1], NULL, 0, NULL, 0, CXTranslationUnit_None);
    if (unit == NULL){
        fprintf(stderr, "Unable to parse Translation Unit\n");
        exit(1);
    }
    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(cursor, cursorVisitor, NULL);


    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
    fclose(outf);
}