#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <clang-c/Index.h>

extern FILE* outf;

enum CXChildVisitResult cursorMacroVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data){
    enum CXCursorKind cursorKind = clang_getCursorKind(cursor);
    CXString clangstr_cursor_name = clang_getCursorSpelling(cursor);
    CXString clangstr_cursor_kind = clang_getCursorKindSpelling(cursorKind);
    const char* cursor_name = clang_getCString(clangstr_cursor_name);
    if (cursorKind != CXCursor_MacroDefinition || (cursor_name[0] == '_' && cursor_name[1] == '_') || strcmp(cursor_name, "linux") == 0 || strcmp(cursor_name, "unix") == 0 || strcmp(cursor_name, "_LP64") == 0){
        return CXChildVisit_Recurse;
    }
    printf("cursor %s kind : %s\n", clang_getCString(clangstr_cursor_name), clang_getCString(clangstr_cursor_kind));
    bool is_function = clang_Cursor_isMacroFunctionLike(cursor);
    printf("is function %d\n", is_function);
    if (is_function){
        printf("function macros are not supported for the moment\n");
    } else {
        printf("value : %s\n", "a");
        fprintf(outf, "?[define %s, %s]\n", cursor_name, "\"temporary value because of libclang parsing\"");
    }
    return CXChildVisit_Recurse;
}

void generateMacroBindings(char* path){
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(index, path, NULL, 0, NULL, 0, CXTranslationUnit_DetailedPreprocessingRecord);
    if (unit == NULL){
        fprintf(stderr, "Unable to parse Translation Unit\n");
        exit(1);
    }
    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(cursor, cursorMacroVisitor, NULL);
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
}