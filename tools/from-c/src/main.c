#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <clang-c/Index.h>

enum CXCursorKind cursorKind;
CXTranslationUnit unit;
FILE* outf;
bool is_in_function = false;
bool is_in_struct = false;

bool is_in_binary_op = false;

bool last_cursor_is_var = false;

bool last_cursor_is_binary_op = false;

bool last_cursor_is_integer = false;

bool is_in_call = false;

void close_previous_blocks(){
    if (last_cursor_is_integer){
        fprintf(outf, "\n");
    }
	if (is_in_function){
		fprintf(outf, "}\n");
	}
	if (is_in_struct){
		fprintf(outf, "}\n");
	}
	is_in_struct = false;
	is_in_function = false;
    last_cursor_is_integer = false;
}

void close_previous_blocks_body(){
    if (is_in_call){
        fprintf(outf, ")\n");
    }
    is_in_call = false;
}

void generate_equal_if_var(){
    if (last_cursor_is_var){
        fprintf(outf, " = ");
    }
}

bool startswith(char* str, char* line){
    int similarity = 0;
    int length = strlen(str);
    int i;
    for (i = 0; i < length; i++){
        if (str[i] == line[i]){
            similarity++;
        }
    }
    return similarity >= length;
}

bool is_typedefed_type(char* type_string){
    return (!startswith("struct", type_string) && !startswith("enum", type_string));
}

const char* get_type_string_from_type_libclang(CXType type){
    printf("get_type  type enum : %d\n", type.kind);
    switch(type.kind){
        case CXType_Float:
        case CXType_Float16:
            return "float";
        case CXType_UInt:
            return "u32";
        case CXType_Int:
            return "int";
        case CXType_Char_S:
        case CXType_SChar:
        case CXType_UChar:
            return "i8";
        case CXType_Pointer: {
            CXType pointee_type = clang_getPointeeType(type);
            const char* pointee_type_str = get_type_string_from_type_libclang(pointee_type);
            char* ptr_str = " ptr";
            char* pointer_type = malloc((strlen(pointee_type_str) + strlen(ptr_str)) * sizeof(char));
            sprintf(pointer_type, "%s%s", pointee_type_str, ptr_str);
            return pointer_type;
        }
        case CXType_Void:
            return "void";
        // TODO : replace bool with int ? (because in C, int is represented by an int while in C., the bool type is a i1)
        case CXType_Bool:
            return "bool";
        case CXType_Long:
            return "i64";
        case CXType_Elaborated: {
            CXString clangstr_type_spelling = clang_getTypeSpelling(type);
            const char* type_spelling = clang_getCString(clangstr_type_spelling);
            if (is_typedefed_type((char*)type_spelling)){
                char* struct_str = "struct ";
                char* typedef_type_with_struct = malloc((strlen(struct_str) + strlen(type_spelling)+1) * sizeof(char));
                sprintf(typedef_type_with_struct, "%s%s", struct_str, clang_getCString(clang_getTypeSpelling(type)));
                return typedef_type_with_struct;
            }
            clang_disposeString(clangstr_type_spelling);
            return type_spelling;
        }
        case CXType_LongDouble:
        case CXType_Double:
        default:
            return "double";
    }
}

enum CXChildVisitResult getFirstChildVisitor(CXCursor c, CXCursor parent, CXClientData client_data){
    struct Result {
        CXCursor child;
        bool found;
    } /*result*/;
    struct Result *r = (struct Result*)client_data;
    r->found = true;
    r->child = c;
    return CXChildVisit_Break;
}

static CXCursor getFirstChild(CXCursor cxNode){
  struct Result {
    CXCursor child;
    bool found;
  } result;
  result.found = false;

  clang_visitChildren(cxNode, getFirstChildVisitor, &result);

  assert(result.found);
  return result.child;
}

enum CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data){
	CXSourceRange range;
    CXToken *tokens = 0;
    unsigned int nTokens = 0;
    cursorKind = clang_getCursorKind(cursor);
	CXString clangstr_cursor_name = clang_getCursorSpelling(cursor);
	CXString clangstr_cursor_kind = clang_getCursorKindSpelling(cursorKind);
	printf("cursor %s kind : %s\n", clang_getCString(clangstr_cursor_name), clang_getCString(clangstr_cursor_kind));
	switch(cursorKind){
	case CXCursor_FunctionDecl:
	    close_previous_blocks();
	    //fprintf(outf, "func %s(){\n", clang_getCString(clangstr_cursor_name));
	    fprintf(outf, "func %s(){", clang_getCString(clangstr_cursor_name));
        is_in_function = true;
	    break;
	case CXCursor_StructDecl:
	    close_previous_blocks();
	    fprintf(outf, "struct %s {\n",  clang_getCString(clangstr_cursor_name));
	    is_in_struct = true;
	    break;
	case CXCursor_FieldDecl:
	    fprintf(outf, "var %s \n", clang_getCString(clangstr_cursor_name));
	    break;
	case CXCursor_VarDecl:
        fprintf(outf, "\n");
        last_cursor_is_var = true;
        if (is_in_struct){
		    fprintf(outf, "}\n");
            is_in_struct = false;
	    }   
	    //fprintf(outf, "var %s \n", clang_getCString(clangstr_cursor_name));
        CXType var_type = clang_getCursorType(cursor);
        const char* var_type_str = get_type_string_from_type_libclang(var_type);
	    fprintf(outf, "var %s : %s", clang_getCString(clangstr_cursor_name), var_type_str);
        break;
	case CXCursor_BinaryOperator:
        generate_equal_if_var();
        last_cursor_is_var = false;
        last_cursor_is_binary_op = true;
        CXToken *exprTokens;
        unsigned numExprTokens;
        clang_tokenize(unit, clang_getCursorExtent(cursor), &exprTokens, &numExprTokens);

        // Get tokens in its left-hand side.
        CXCursor cxLHS = getFirstChild(cursor);
        CXToken *lhsTokens;
        unsigned numLHSTokens;
        clang_tokenize(unit, clang_getCursorExtent(cxLHS), &lhsTokens, &numLHSTokens);

        // Get the spelling of the first token not in the LHS.
        assert(numLHSTokens < numExprTokens);
        for (int i = 0; i < numLHSTokens; i++){
            CXString cxString = clang_getTokenSpelling(unit, exprTokens[i]);
            const char* ret = clang_getCString(cxString);
            fprintf(outf, "%s\n", ret);
            clang_disposeString(cxString);
        }
        CXString cxString = clang_getTokenSpelling(unit, exprTokens[numLHSTokens]);
        const char* ret = clang_getCString(cxString);
        printf("op : %s\n", ret);
        //fprintf(outf, "%s\n", ret);
        fprintf(outf, "%s", ret);
        // Clean up.
        clang_disposeString(cxString);
        clang_disposeTokens(unit, lhsTokens, numLHSTokens);
        clang_disposeTokens(unit, exprTokens, numExprTokens);
        
        /*range = clang_getCursorExtent(cursor);
        tokens = 0;
        nTokens = 0;
        clang_tokenize(unit, range, &tokens, &nTokens);
        for (unsigned int i = 0; i < nTokens-1; i++){
        CXString spelling = clang_getTokenSpelling(unit, tokens[i]);
        printf("token = %s\n", clang_getCString(spelling));
        fprintf(outf, "%s\n", clang_getCString(spelling));
        clang_disposeString(spelling);
        }
        clang_disposeTokens(unit, tokens, nTokens);*/
        /*enum CXBinaryOperatorKind operatorKind = clang_getCursorBinaryOperatorKind(cursor);;
	    CXString operator_string = clang_getBinaryOperatorKindSpelling(operatorKind);
	    fprintf(outf, "%s", clang_getCString(operator_string));
	    clang_disposeString(operator_string);*/
	    break;
    case CXCursor_IntegerLiteral:
        last_cursor_is_integer = true;
        generate_equal_if_var();
        if (last_cursor_is_binary_op){
            last_cursor_is_binary_op = false;
            break;
        }
        last_cursor_is_var = false;
        range = clang_getCursorExtent(cursor);
        tokens = 0;
        nTokens = 0;
        clang_tokenize(unit, range, &tokens, &nTokens);
        for (unsigned int i = 0; i < nTokens; i++){
        CXString spelling = clang_getTokenSpelling(unit, tokens[i]);
        printf("token = %s\n", clang_getCString(spelling));
        fprintf(outf, "%s", clang_getCString(spelling));
        clang_disposeString(spelling);
        }
        clang_disposeTokens(unit, tokens, nTokens);
        break;
    case CXCursor_ReturnStmt:
        close_previous_blocks_body();
        fprintf(outf, "return ");
        break;
    case CXCursor_StringLiteral:
        fprintf(outf, "%s", clang_getCString(clangstr_cursor_name));
        break;
    case CXCursor_CallExpr:
        close_previous_blocks_body();
        is_in_call = true;
        fprintf(outf, "%s(", clang_getCString(clangstr_cursor_name));
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
    unit = clang_parseTranslationUnit(index, path, NULL, 0, NULL, 0, CXTranslationUnit_None);
	if (unit == NULL){
        fprintf(stderr, "Unable to parse Translation Unit\n");
        exit(1);
    }
	CXCursor cursor = clang_getTranslationUnitCursor(unit);
	clang_visitChildren(cursor, cursorVisitor, NULL);
	close_previous_blocks();
	clang_disposeTranslationUnit(unit);
	clang_disposeIndex(index);
	return 0;
}

int main(int argc, char** argv){
	if (argc < 2){
	    printf("Expected at least 2 args\n");
	    printf("Usage : cpoint-from-c [filename]\n");
	    return 1;
	}
	char* outpath = "out.cpoint";
	outf = fopen(outpath, "w");
	printf("args : %d\n", argc);
	printf("%s\n", argv[1]);
	convertToC(argv[1]);
	fclose(outf);
	return 0;
}
