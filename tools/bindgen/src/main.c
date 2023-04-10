#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <clang-c/Index.h>

FILE* outf;
bool in_function_declaration = false;
bool in_struct_declaration = false;
enum CXCursorKind cursorKind;
int param_number = 0;
bool pass_block = false;

const char* get_type_string_from_type_libclang(CXType type){
    switch(type.kind){
        case CXType_Int:
            return "int";
        case CXType_Char_S:
        case CXType_Char16:
        case CXType_Char32:
            return "i8";
        case CXType_Pointer:
            return "int ptr";
        case CXType_LongDouble:
        case CXType_Double:
        default:
            return "double";
    }
}

void close_previous_blocks(){
    pass_block = false;
    if (in_function_declaration){
        fprintf(outf,");\n");
        in_function_declaration = false;
    } else if (in_struct_declaration){
        fprintf(outf,"}\n");
        in_struct_declaration = false;
    }
}

enum CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data){
    cursorKind = clang_getCursorKind(cursor);
    printf("cursor %s kind : %s\n", clang_getCString(clang_getCursorSpelling(cursor)), clang_getCString(clang_getCursorKindSpelling(cursorKind)));
    switch(cursorKind){
        case CXCursor_FunctionDecl:
            close_previous_blocks();
            param_number = 0;
            fprintf(outf, "extern func %s(", clang_getCString(clang_getCursorSpelling(cursor)));
            in_function_declaration = true;
            break;
        case CXCursor_ParmDecl:
            CXType param_type = clang_getCursorType(cursor);
            printf("type param : %s\n", clang_getCString(clang_getTypeSpelling(param_type)));
            printf("type enum : %d\n", param_type.kind);
            if (param_number > 0){
                fprintf(outf, ",");
            }
            fprintf(outf, "%s", clang_getCString(clang_getCursorSpelling(cursor)));
            fprintf(outf, " : %s", get_type_string_from_type_libclang(param_type));
            param_number++;
            break;
        case CXCursor_StructDecl:
            close_previous_blocks();
            char* struct_name = clang_getCString(clang_getCursorSpelling(cursor));
            if (strlen(struct_name) != 0){
            printf("struct name : %s\n", struct_name);
            printf("struct name length : %ld\n", strlen(struct_name));
            fprintf(outf, "struct %s {\n", struct_name);
            } else {
                pass_block = true;
            }
            in_struct_declaration = true;
            break;
        case CXCursor_FieldDecl:
            CXType field_type = clang_getCursorType(cursor);
            if (!pass_block){
            fprintf(outf, "\tvar %s", clang_getCString(clang_getCursorSpelling(cursor)));
            fprintf(outf, " : %s \n", get_type_string_from_type_libclang(field_type));
            }
            break;
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
    if (cursorKind == CXCursor_ParmDecl){
        fprintf(outf, ");\n");
    } else if (cursorKind == CXCursor_FieldDecl){
        fprintf(outf, "}\n");
    }
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
    fclose(outf);
}