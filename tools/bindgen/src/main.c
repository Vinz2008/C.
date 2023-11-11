#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <clang-c/Index.h>
#include "macros.h"

FILE* outf;
bool in_function_declaration = false;
bool in_struct_declaration = false;
bool in_enum_declaration = false;
enum CXCursorKind cursorKind;
int param_number = 0;
bool pass_block = false;
bool is_in_typedef = false;
bool is_variable_number_args = false;
CXType return_type;

bool is_custom_enum_value = false;

const char* get_type_string_from_type_libclang(CXType type, const char* type_c_spelling){
    printf("get_type  type enum : %d\n", type.kind);
    switch(type.kind){
        case CXType_Float:
        case CXType_Float16:
            return "float";
        case CXType_Int:
            return "int";
        case CXType_Char_S:
        case CXType_SChar:
        case CXType_UChar:
            return "i8";
        case CXType_Pointer:
            return "int ptr";
        case CXType_Void:
            return "void";
        case CXType_Bool:
            return "bool";
        case CXType_Long:
            return "i64";
        case CXType_Elaborated:
            return type_c_spelling;
        case CXType_LongDouble:
        case CXType_Double:
        default:
            return "double";
    }
}

void close_previous_blocks(){
    if (in_function_declaration){
        printf("return type enum : %d\n", return_type.kind);
        if (is_variable_number_args){
            fprintf(outf, ", ...");
        }
        CXString return_type_name = clang_getTypeSpelling(return_type);
        fprintf(outf,") %s;\n", get_type_string_from_type_libclang(return_type, clang_getCString(return_type_name)));
        in_function_declaration = false;
        clang_disposeString(return_type_name);
    } else if (in_struct_declaration && !pass_block && !is_in_typedef){
        printf("closing struct so not passing block\n");
        fprintf(outf,"}\n");
        in_struct_declaration = false;
    } else if (in_enum_declaration && !pass_block && !is_in_typedef){
        fprintf(outf,"\n}\n");
        in_enum_declaration = false;
    }
    pass_block = false;
    is_in_typedef = false;
    is_variable_number_args = false;
}

// TODO : verify if function parameters or other identifier are empty name so they are given default name

enum CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data){
    cursorKind = clang_getCursorKind(cursor);
    CXString clangstr_cursor_name = clang_getCursorSpelling(cursor);
    CXString clangstr_cursor_kind = clang_getCursorKindSpelling(cursorKind);
    printf("cursor %s kind : %s\n", clang_getCString(clangstr_cursor_name), clang_getCString(clangstr_cursor_kind));
    switch(cursorKind){
        case CXCursor_MacroDefinition:
            printf("MACRO DEFINITION\n");
            break;
        case CXCursor_FunctionDecl:
            close_previous_blocks();
            if (clang_Cursor_isVariadic(cursor) != 0){
                is_variable_number_args = true;
            }
            return_type = clang_getResultType(clang_getCursorType(cursor));
            param_number = 0;
            CXString clangstr_function_name = clang_getCursorSpelling(cursor);
            fprintf(outf, "extern %s(", clang_getCString(clangstr_function_name));
            in_function_declaration = true;
            clang_disposeString(clangstr_function_name);
            break;
        case CXCursor_ParmDecl:
            if (!in_function_declaration){
                break;
            }
            CXType param_type = clang_getCursorType(cursor);
            CXString clangstr_param_type_name = clang_getTypeSpelling(param_type);
            printf("type param : %s\n", clang_getCString(clangstr_param_type_name));
            printf("type enum : %d\n", param_type.kind);
            if (param_number > 0){
                fprintf(outf, ",");
            }
            CXString clangstr_param_name = clang_getCursorSpelling(cursor);
            fprintf(outf, "%s", clang_getCString(clangstr_param_name));
            fprintf(outf, " : %s", get_type_string_from_type_libclang(param_type, clang_getCString(clangstr_param_type_name)));
            param_number++;
            clang_disposeString(clangstr_param_type_name);
            clang_disposeString(clangstr_param_name);
            break;
        case CXCursor_StructDecl:
            if (is_in_typedef){
                break;
            }
            CXString clangstr_struct_name = clang_getCursorSpelling(cursor);
            const char* struct_name = clang_getCString(clangstr_struct_name);
            if (strlen(struct_name) != 0){
            printf("struct closing previous blocks\n");
            close_previous_blocks();
            printf("struct name : %s\n", struct_name);
            printf("struct name length : %ld\n", strlen(struct_name));
            fprintf(outf, "struct %s {\n", struct_name);
            } else {
                printf("passing block\n");
                pass_block = true;
            }
            in_struct_declaration = true;
            clang_disposeString(clangstr_struct_name);
            break;
        case CXCursor_FieldDecl:
            CXType field_type = clang_getCursorType(cursor);
            if (!pass_block && !is_in_typedef){
            CXString field_type_name = clang_getTypeSpelling(field_type);
            CXString clangstr_var_name = clang_getCursorSpelling(cursor);
            fprintf(outf, "\tvar %s", clang_getCString(clangstr_var_name));
            fprintf(outf, " : %s \n", get_type_string_from_type_libclang(field_type, clang_getCString(field_type_name)));
            clang_disposeString(clangstr_var_name);
            clang_disposeString(field_type_name);
            }
            break;
        case CXCursor_TypedefDecl:
            close_previous_blocks();
            CXString clangstr_new_type_name = clang_getCursorSpelling(cursor);
            const char* new_type_name = clang_getCString(clangstr_new_type_name);
            CXType value_type_clang = clang_getTypedefDeclUnderlyingType(cursor);
            is_in_typedef = true;
            if (value_type_clang.kind == CXType_Elaborated){
                clang_disposeString(clangstr_new_type_name);
                break;
            }
            const char* value_type_name = get_type_string_from_type_libclang(value_type_clang, clang_getCString(clangstr_new_type_name));
            printf("typedef from %s to %s\n", new_type_name, value_type_name);
            fprintf(outf, "type %s %s;\n", new_type_name, value_type_name);
            clang_disposeString(clangstr_new_type_name);
            break;
        case CXCursor_EnumDecl:
            close_previous_blocks();
            CXString clangstr_enum_name = clang_getCursorSpelling(cursor);
            const char* enum_name = clang_getCString(clangstr_enum_name);
            printf("enum name : %s\n", enum_name);
            //fprintf(outf, "enum %s {\n", enum_name);
            fprintf(outf, "enum %s {", enum_name);
            in_enum_declaration = true;
            clang_disposeString(clangstr_enum_name);
            break;
        case CXCursor_EnumConstantDecl:
            is_custom_enum_value = false;
            CXString clangstr_enum_field = clang_getCursorSpelling(cursor);
            fprintf(outf, "\n\t%s", clang_getCString(clangstr_enum_field));
            /*if (!is_custom_enum_value){
                fprintf(outf, "\n");
            }*/
            clang_disposeString(clangstr_enum_field);
            break;
        case CXCursor_IntegerLiteral:
            if (in_enum_declaration){
                CXEvalResult res = clang_Cursor_Evaluate(cursor);
                fprintf(outf, " = %d", clang_EvalResult_getAsInt(res));
                clang_EvalResult_dispose(res);
                is_custom_enum_value = true;
            }
            break;
        default:
            break;
    }
    clang_disposeString(clangstr_cursor_kind);
    clang_disposeString(clangstr_cursor_name);
    return CXChildVisit_Recurse;
}

void generateBindings(char* path){
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(index, path, NULL, 0, NULL, 0, CXTranslationUnit_None);
    if (unit == NULL){
        fprintf(stderr, "Unable to parse Translation Unit\n");
        exit(1);
    }
    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(cursor, cursorVisitor, NULL);
    if (in_enum_declaration && is_custom_enum_value){
        fprintf(outf, "\n");
    }
    if (cursorKind == CXCursor_FieldDecl || cursorKind == CXCursor_EnumConstantDecl || in_enum_declaration){
        if (!pass_block){
        fprintf(outf, "}\n");
        }
        in_struct_declaration = false;
        in_enum_declaration = false;
    } else {
        CXString return_type_name = clang_getTypeSpelling(return_type);
        fprintf(outf,") %s;\n", get_type_string_from_type_libclang(return_type, clang_getCString(return_type_name)));
        in_function_declaration = false;
    }
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
}   

int main(int argc, char** argv){
    if (argc <= 1){
        fprintf(stderr, "arguments missing\n");
        exit(1);
    }
    char* outfile = "bindings.cpoint";
    char** filenames = malloc(sizeof(char*) * argc-1);
    int filenames_length = 0;
    bool macro_mode = false;
    for (int i = 1; i < argc; i++){
        printf("%d : %s\n", i, argv[i]);
        if (strcmp("-o", argv[i]) == 0){
            i++;
            outfile = argv[i];
            printf("%d : %s\n", i, argv[i]);
        } else if (strcmp("-m", argv[i]) == 0){
            macro_mode = true;
        } else {
            filenames[i-1] = argv[i];
            filenames_length++;
        }
    }
    outf = fopen(outfile, "w");

    for (int i = 0; i < filenames_length; i++){
        if (macro_mode){
            generateMacroBindings(filenames[i]);
        }
        generateBindings(filenames[i]);
    }

    fclose(outf);
    free(filenames);
}