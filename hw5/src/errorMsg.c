#include "errorMsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int g_anyErrorOccur = 0;

#define NONE "\033[m"
#define RED "\033[0;32;31m"
void printErrorMsg(AST_NODE* node, char* str, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d: ", node->linenumber);
    
    switch(errorMsgKind)
    {
        case SYMBOL_UNDECLARED:
            printf(RED "\'%s\' was not declared in this scope\n" NONE, node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case SYMBOL_IS_NOT_TYPE:    // self-defined
            printf(RED "\'%s\' was not declared as type\n" NONE, node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case SYMBOL_REDECLARED:
            printf(RED "redeclaration of \'%s\'\n" NONE, node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case ARRAY_SIZE_NOT_INT:
            printf(RED "the size of array should be an integer.\n" NONE);
            break;
        case ARRAY_SIZE_NEGATIVE:
            printf(RED "size of array \'%s\' is negative.\n" NONE, node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case ARRAY_SUBSCRIPT_NOT_INT:
            printf(RED "array subscript is not an integer.\n" NONE);
            break;
        case NOT_ASSIGNABLE:
            printf(RED "\'%s\' is not assignable.\n" NONE, node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case NOT_REFERABLE:
            printf(RED "\'%s\' is not referable.\n" NONE, node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case INCOMPATIBLE_ARRAY_DIMENSION_DECL_GT_REF:
            printf(RED "assignment to expression with array type.\n" NONE);
            break;
        case INCOMPATIBLE_ARRAY_DIMENSION_DECL_LT_REF:
            printf(RED "subscripted value is neither array nor pointer nor vector.\n" NONE);
            break;
        case TRY_TO_INIT_ARRAY:
            printf(RED "array can't be initialized.\n" NONE);
            break;
        case ASSIGN_NON_CONST_TO_GLOBAL:
            printf(RED "initializer element is not constant.\n" NONE);
            break;
        case NOT_FUNCTION_NAME:
            printf(RED "called object \'%s\' is not a function or function pointer.\n" NONE, node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case TOO_FEW_ARGUMENT:
            printf(RED "too few argument to function \'%s\'.\n" NONE, node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case TOO_MANY_ARGUMENT:
            printf(RED "too many argument to function \'%s\'.\n" NONE, node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case PASS_SCALAR_TO_ARRAY:
            printf(RED "invalid conversion from \'%s\' to \'%s\' array.\n" NONE, str, &str[6]);
            break;
        case PASS_ARRAY_TO_SCALAR:
            printf(RED "invalid conversion from \'%s\' array to \'%s\'.\n" NONE, str, &str[6]);
            break;
        case PARAMETER_TYPE_UNMATCH:
            printf(RED "unmatch parameter type.\n" NONE);
            break;
        case RETURN_TYPE_UNMATCH:
            printf(RED "return type unmatch.\n" NONE);
            break;
        case RETURN_ARRAY:
            printf(RED "return array.\n" NONE);
            break;
        default:
            printf(RED "unhandled case in void printErrorMsg(AST_NODE* node, char* name, ERROR_MSG_KIND* errorMsgKind)\n" NONE);
            break;
    }
    
}