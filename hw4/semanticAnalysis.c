#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "symbolTable.h"
// This file is for reference only, you are not required to follow the implementation. //
// You only need to check for errors stated in the hw4 document. //
int g_anyErrorOccur = 0;

DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2);
void processProgramNode(AST_NODE *programNode);
void processDeclarationNode(AST_NODE* declarationNode);
void declareIdList(AST_NODE* typeNode, int ignoreArrayFirstDimSize);
void declareFunction(AST_NODE* returnTypeNode);
void processDeclDimList(AST_NODE* variableDeclDimList, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize);
void processTypeNode(AST_NODE* typeNode);
void processBlockNode(AST_NODE* blockNode);
void processStmtNode(AST_NODE* stmtNode);
void processGeneralNode(AST_NODE *node);
void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode);
void checkWhileStmt(AST_NODE* whileNode);
void checkForStmt(AST_NODE* forNode);
void checkAssignmentStmt(AST_NODE* assignmentNode);
void checkIfStmt(AST_NODE* ifNode);
void checkWriteFunction(AST_NODE* functionCallNode);
void checkFunctionCall(AST_NODE* functionCallNode);
void processExprRelatedNode(AST_NODE* exprRelatedNode);
void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter);
void checkReturnStmt(AST_NODE* returnNode);
void processExprNode(AST_NODE* exprNode);
void processVariableLValue(AST_NODE* idNode);
void processVariableRValue(AST_NODE* idNode);
void processConstValueNode(AST_NODE* constValueNode);
void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue);
void evaluateExprValue(AST_NODE* exprNode);
void processDeclarationListNode(AST_NODE* declarationNode);

typedef enum ErrorMsgKind
{
    SYMBOL_UNDECLARED,
    SYMBOL_IS_NOT_TYPE,
    SYMBOL_REDECLARED
} ErrorMsgKind;

void printErrorMsg(AST_NODE* node, char* name, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node->linenumber);
    
    switch(errorMsgKind)
    {
        case SYMBOL_UNDECLARED:
            printf("error: \'%s\' was not declared in this scope\n", node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case SYMBOL_IS_NOT_TYPE:    // self-defined
            printf("error: \'%s\' was not declared as type\n", node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case SYMBOL_REDECLARED:
            printf("error: redeclaration of \'%s\'", node->semantic_value.identifierSemanticValue.identifierName);
            break;
        default:
            printf("Unhandled case in void printErrorMsg(AST_NODE* node, char* name, ERROR_MSG_KIND* errorMsgKind)\n");
            break;
    }
    
}

/*void printErrorMsg(AST_NODE* node, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node->linenumber);
    
    switch(errorMsgKind)
    {

        default:
            printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* errorMsgKind)\n");
            break;
    }
    
}*/

void semanticAnalysis(AST_NODE *root)
{
    processProgramNode(root);
}


DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2)
{
    if(dataType1 == FLOAT_TYPE || dataType2 == FLOAT_TYPE) {
        return FLOAT_TYPE;
    } else {
        return INT_TYPE;
    }
}

void processProgramNode(AST_NODE *programNode)
{
    // walk tree
    AST_NODE *child = programNode->child;
    while(child != NULL) {
        switch(child->nodeType) {
            case VARIABLE_DECL_LIST_NODE:
                processDeclarationListNode(child);
                break;
            case DECLARATION_NODE:  // FUNCTION_DECL
                declareFunction(child);
                break;
            default:
                break;
        }
        child = child->rightSibling;
    }
    return;
}

void processDeclarationListNode(AST_NODE* declarationNode)
{
    AST_NODE *child = declarationNode->child;
    while(child != NULL) {
        switch(child->semantic_value.declSemanticValue.kind) {
            case VARIABLE_DECL:
                declareIdList(child, 0);    // cannot ignore array first dim size
                break;
            case TYPE_DECL:
                declareIdList(child, 0);
                break;
            default:
                break;
        }
        child = child->rightSibling;
    }
    return;
}

/*void processDeclarationNode(AST_NODE* declarationNode)
{
    
}*/

void processTypeNode(AST_NODE* idNodeAsType)
{
    SymbolTableEntry *entry = retrieveSymbol(idNodeAsType->semantic_value.identifierSemanticValue.identifierName);
    if(entry == NULL) {  // e.g., ABC k; ABC was not declared as type
        printErrorMsg(idNodeAsType, NULL, SYMBOL_UNDECLARED);
        idNodeAsType->dataType = ERROR_TYPE;
    }
    else if(entry->attribute->attributeKind != TYPE_ATTRIBUTE) {
        printErrorMsg(idNodeAsType, NULL, SYMBOL_IS_NOT_TYPE);
        idNodeAsType->dataType = ERROR_TYPE;
    }
    else {
        idNodeAsType->semantic_value.identifierSemanticValue.symbolTableEntry = entry;
        if(entry->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
            idNodeAsType->dataType = entry->attribute->attr.typeDescriptor->properties.dataType;
        }
        else {
            idNodeAsType->dataType = entry->attribute->attr.typeDescriptor->properties.arrayProperties.elementType;
        }
    }
    return;
}

SymbolAttribute *makeSymbolAttribute(AST_NODE *idNode)
{
    SymbolAttribute *attribute = (SymbolAttribute *)malloc(sizeof(SymbolAttribute));
    switch(idNode->parent->semantic_value.declSemanticValue.kind) {
        case VARIABLE_DECL:
        case FUNCTION_PARAMETER_DECL:
        case TYPE_DECL:
            attribute->attributeKind = (idNode->parent->semantic_value.declSemanticValue.kind == TYPE_DECL)?TYPE_ATTRIBUTE : VARIABLE_ATTRIBUTE;
            memcpy(attribute->attr.typeDescriptor,
                idNode->leftmostSibling->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor,
                sizeof(TypeDescriptor) );
            if(idNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID) {
                /*TODO*/
            }
            break;
        case FUNCTION_DECL:
            attribute->attributeKind = FUNCTION_SIGNATURE;
            break;
        default:
            break;
    }

}

void declareIdList(AST_NODE* declarationNode, int ignoreArrayFirstDimSize)
{
    AST_NODE *typeNode = declarationNode->child;
    AST_NODE *idNode = typeNode->rightSibling;
    processTypeNode(typeNode);
    while(idNode != NULL) {
        idNode->dataType = typeNode->dataType;

        char *idName = idNode->semantic_value.identifierSemanticValue.identifierName;

        SymbolTableEntry *entry = declaredLocally(idName);
        if(entry == NULL) { // put into symbol table
            enterSymbol(idName, makeSymbolAttribute(idNode));
        }
        else{
            printErrorMsg(idNode, NULL, SYMBOL_REDECLARED);
        }
        idNode = idNode->rightSibling;
    }
}

void declareFunction(AST_NODE* declarationNode)
{
}

void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode)
{
}

void checkWhileStmt(AST_NODE* whileNode)
{
}


void checkForStmt(AST_NODE* forNode)
{
}


void checkAssignmentStmt(AST_NODE* assignmentNode)
{
}


void checkIfStmt(AST_NODE* ifNode)
{
}

void checkWriteFunction(AST_NODE* functionCallNode)
{
}

void checkFunctionCall(AST_NODE* functionCallNode)
{
}

void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter)
{
}


void processExprRelatedNode(AST_NODE* exprRelatedNode)
{
}

void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue)
{
}

void evaluateExprValue(AST_NODE* exprNode)
{
}


void processExprNode(AST_NODE* exprNode)
{
}


void processVariableLValue(AST_NODE* idNode)
{
}

void processVariableRValue(AST_NODE* idNode)
{
}


void processConstValueNode(AST_NODE* constValueNode)
{
}


void checkReturnStmt(AST_NODE* returnNode)
{
}


void processBlockNode(AST_NODE* blockNode)
{
}


void processStmtNode(AST_NODE* stmtNode)
{
}


void processGeneralNode(AST_NODE *node)
{
}

void processDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize)
{
}