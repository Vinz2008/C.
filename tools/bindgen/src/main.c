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
bool is_in_typedef = false;
CXType return_type;
const char* get_type_string_from_type_libclang(CXType type){
    printf("get_type  type enum : %d\n", type.kind);
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
    if (in_function_declaration){
        printf("return type enum : %d\n", return_type.kind);
        fprintf(outf,") %s;\n", get_type_string_from_type_libclang(return_type));
        in_function_declaration = false;
    } else if (in_struct_declaration && !pass_block){
        fprintf(outf,"}\n");
        in_struct_declaration = false;
    }
    pass_block = false;
    is_in_typedef = false;
}

enum CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data){
    cursorKind = clang_getCursorKind(cursor);
    printf("cursor %s kind : %s\n", clang_getCString(clang_getCursorSpelling(cursor)), clang_getCString(clang_getCursorKindSpelling(cursorKind)));
    switch(cursorKind){
        case CXCursor_MacroDefinition:
            printf("MACRO DEFINITION\n");
            break;
        case CXCursor_FunctionDecl:
            close_previous_blocks();
            return_type = clang_getResultType(clang_getCursorType(cursor));
            param_number = 0;
            fprintf(outf, "extern %s(", clang_getCString(clang_getCursorSpelling(cursor)));
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
            if (is_in_typedef){
                break;
            }
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
            if (!pass_block && is_in_typedef == false){
            fprintf(outf, "\tvar %s", clang_getCString(clang_getCursorSpelling(cursor)));
            fprintf(outf, " : %s \n", get_type_string_from_type_libclang(field_type));
            }
            break;
        case CXCursor_TypedefDecl:
            close_previous_blocks();
            char* new_type_name = clang_getCString(clang_getCursorSpelling(cursor));
            CXType value_type_clang = clang_getTypedefDeclUnderlyingType(cursor);
            is_in_typedef = true;
            if (value_type_clang.kind == CXType_Elaborated){
                break;
            }
            char* value_type_name = get_type_string_from_type_libclang(value_type_clang);
            printf("typedef from %s to %s\n", new_type_name, value_type_name);
            fprintf(outf, "type %s %s;\n", new_type_name, value_type_name);
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