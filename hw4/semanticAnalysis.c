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
void declareIdList(AST_NODE* typeNode);
void declareFunction(AST_NODE* returnTypeNode);
void processDeclDimList(AST_NODE* variableDeclDimList, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize);
void processTypeNode(AST_NODE* typeNode);
void processBlockNode(AST_NODE* blockNode);
void processStmtNode(AST_NODE* stmtNode);
void processGeneralNode(AST_NODE *node);
void processAssignOrExpr(AST_NODE* assignOrExprRelatedNode);
void processWhileStmt(AST_NODE* whileNode);
void processForStmt(AST_NODE* forNode);
void processAssignmentStmt(AST_NODE* assignmentNode);
void processIfStmt(AST_NODE* ifNode);
void processWriteFunction(AST_NODE* functionCallNode);
void processFunctionCall(AST_NODE* functionCallNode);
void processRelopNode(AST_NODE* relopNode);
void processParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter);
void processReturnStmt(AST_NODE* returnNode);
void processExprNode(AST_NODE* exprNode);
void processVariableLValue(AST_NODE* idNode);
void processVariableRValue(AST_NODE* idNode);
void processConstValueNode(AST_NODE* constValueNode);
void updateConstNodeType(AST_NODE* constNode);
void evaluateExprValue(AST_NODE* exprNode);
void processDeclarationListNode(AST_NODE* declarationNode);
SymbolAttribute *makeFunctionAttribute(AST_NODE *idNode);
int processConstExprNode(AST_NODE *constExprNode);
void makeItConstNode(AST_NODE *constExprNode, int num);
int isRelOp(EXPRSemanticValue exprSemanticValue);

typedef enum ErrorMsgKind
{
    SYMBOL_UNDECLARED,
    SYMBOL_IS_NOT_TYPE,
    SYMBOL_REDECLARED,
    ARRAY_SIZE_NOT_INT,
    ARRAY_SIZE_NEGATIVE,
    NOT_ASSIGNABLE,
    NOT_REFERABLE,
    IMCOMPATIBLE_ARRAY_DIMENSION_DECL_GT_REF,
    IMCOMPATIBLE_ARRAY_DIMENSION_DECL_LT_REF,
    ARRAY_SUBSCRIPT_NOT_INT
} ErrorMsgKind;

void printErrorMsg(AST_NODE* node, char* name, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d: ", node->linenumber);
    
    switch(errorMsgKind)
    {
        case SYMBOL_UNDECLARED:
            printf("\'%s\' was not declared in this scope\n", node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case SYMBOL_IS_NOT_TYPE:    // self-defined
            printf("\'%s\' was not declared as type\n", node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case SYMBOL_REDECLARED:
            printf("redeclaration of \'%s\'\n", node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case ARRAY_SIZE_NOT_INT:
            printf("The size of array should be an integer.\n");
            break;
        case ARRAY_SIZE_NEGATIVE:
            printf("The size of array should be positive.\n");
            break;
        case ARRAY_SUBSCRIPT_NOT_INT:
            printf("Array subscript is not integer.\n");
            break;
        case NOT_ASSIGNABLE:
            printf("\'%s\' is not assignable.\n", node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case NOT_REFERABLE:
            printf("\'%s\' is not referable.\n", node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case IMCOMPATIBLE_ARRAY_DIMENSION_DECL_GT_REF:
            printf("assignment to expression with array type.\n");
            break;
        case IMCOMPATIBLE_ARRAY_DIMENSION_DECL_LT_REF:
            printf("subscripted value is neither array nor pointer.\n");
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
    return;
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
                declareIdList(child);    // cannot ignore array first dim size
                break;
            case TYPE_DECL:
                declareIdList(child);
                break;
            default:
                break;
        }
        child = child->rightSibling;
    }
    return;
}

void processTypeNode(AST_NODE* idNodeAsType)
{
    SymbolTableEntry *entry = retrieveSymbol(idNodeAsType->semantic_value.identifierSemanticValue.identifierName);
    if(entry == NULL) {  // e.g., ABC k; ABC was not declared
        printErrorMsg(idNodeAsType, NULL, SYMBOL_UNDECLARED);
        idNodeAsType->dataType = ERROR_TYPE;
    }
    else if(entry->attribute->attributeKind != TYPE_ATTRIBUTE) {  //e.g. int ABC = 1; ABC k;
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

//return the attribute of a VAR_DECL, FUNC_PARA_DECL or TYPE_DECL node
SymbolAttribute *makeSymbolAttribute(AST_NODE *idNode)
{
    SymbolAttribute *attribute = (SymbolAttribute *)malloc(sizeof(SymbolAttribute));
    attribute->attributeKind = (idNode->parent->semantic_value.declSemanticValue.kind == TYPE_DECL)?TYPE_ATTRIBUTE : VARIABLE_ATTRIBUTE;
    attribute->attr.typeDescriptor = (TypeDescriptor *)malloc(sizeof(TypeDescriptor));
    memcpy(attribute->attr.typeDescriptor,
        idNode->leftmostSibling->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor,
        sizeof(TypeDescriptor) );    
    if(idNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID) {
        if(attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
            DATA_TYPE dataType = attribute->attr.typeDescriptor->properties.dataType;
            attribute->attr.typeDescriptor->kind = ARRAY_TYPE_DESCRIPTOR;
            attribute->attr.typeDescriptor->properties.arrayProperties.dimension = 0;
            attribute->attr.typeDescriptor->properties.arrayProperties.elementType = dataType;   
        }

        int IDdim = 0;          //e.g. int A[3][4], IDdim = 2
        AST_NODE *dimNode = idNode->child;
        
        if( dimNode != NULL && dimNode->nodeType == NUL_NODE ){
            makeItConstNode(dimNode, 0);
            IDdim++;
            dimNode = dimNode->rightSibling;
        }
        while( dimNode != NULL ){
            int isInt = processConstExprNode(dimNode);
            if( isInt == 0 ){
                printErrorMsg(dimNode, NULL, ARRAY_SIZE_NOT_INT);
            }
            else if( dimNode->semantic_value.const1->const_u.intval <= 0 ){
                printErrorMsg(dimNode, NULL, ARRAY_SIZE_NEGATIVE);
            }
            IDdim++;
            dimNode = dimNode->rightSibling;
        }
        
        int *sizeInEachDimension = attribute->attr.typeDescriptor->properties.arrayProperties.sizeInEachDimension;  // new
        int typedim = 0; // typedef int INT2[2][3]; INT2[2][3] arr[1]; typedim = 2 
        if(attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR) {
            typedim = attribute->attr.typeDescriptor->properties.arrayProperties.dimension;
        }
        for( int i =  typedim - 1; i >= 0; i-- ){
            sizeInEachDimension[i+IDdim] = sizeInEachDimension[i];
        }
        dimNode = idNode->child;
        for( int i = 0; i < IDdim; i++ ){
            sizeInEachDimension[i] = dimNode->semantic_value.const1->const_u.intval;
            dimNode = dimNode->rightSibling;
        }
        attribute->attr.typeDescriptor->properties.arrayProperties.dimension += IDdim;
    }

    return attribute;
}

int processConstExprNode(AST_NODE *constExprNode)   //return whether the expression is INT
{
    if( constExprNode->nodeType == EXPR_NODE ){
        int curNodeConst = 0;
        int leftIsInt, rightIsInt = 1;
        int isBinaryOp = ( constExprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION )? 1 : 0;
        leftIsInt = processConstExprNode(constExprNode->child);
        if( isBinaryOp ) {
            rightIsInt = processConstExprNode(constExprNode->child->rightSibling);
        }
        if( leftIsInt == 0 || rightIsInt == 0 ){
            makeItConstNode(constExprNode, 0);  // error, but let the node to be 0 => arr[0]
            return 0;
        }
        int lVal, rVal;
        lVal = constExprNode->child->semantic_value.const1->const_u.intval;
        if( isBinaryOp ) {
            rVal = constExprNode->child->rightSibling->semantic_value.const1->const_u.intval;
            switch (constExprNode->semantic_value.exprSemanticValue.op.binaryOp){
                case BINARY_OP_ADD:
                    curNodeConst = lVal + rVal;
                    break;
                case BINARY_OP_SUB:
                    curNodeConst = lVal - rVal;
                    break;
                case BINARY_OP_MUL:
                    curNodeConst = lVal * rVal;
                    break;
                case BINARY_OP_DIV:
                    curNodeConst = lVal / rVal;
                    break;
                default:
                    break;
            }
        }
        else {
            switch (constExprNode->semantic_value.exprSemanticValue.op.unaryOp){
                case UNARY_OP_NEGATIVE:
                    curNodeConst = -lVal;
                    break;
                default:
                    break;
            }
        }
        makeItConstNode(constExprNode, curNodeConst);
    }
    if( constExprNode->nodeType == CONST_VALUE_NODE && constExprNode->semantic_value.const1->const_type == INTEGERC ){
        return 1;
    }
    return 0;
}

void makeItConstNode(AST_NODE *constExprNode, int num)
{
    constExprNode->nodeType = CONST_VALUE_NODE;
    constExprNode->dataType = INT_TYPE;
    constExprNode->semantic_value.const1 = (CON_Type *)malloc(sizeof(CON_Type));
    constExprNode->semantic_value.const1->const_type = INTEGERC;
    constExprNode->semantic_value.const1->const_u.intval = num;
    return;
}

//typedef, var decl, and param decl
void declareIdList(AST_NODE* declarationNode)
{
    AST_NODE *typeNode = declarationNode->child;
    AST_NODE *idNode = typeNode->rightSibling;
    processTypeNode(typeNode);
    while(idNode != NULL) {
        idNode->dataType = typeNode->dataType;

        char *idName = idNode->semantic_value.identifierSemanticValue.identifierName;

        SymbolTableEntry *entry = declaredLocally(idName);
        if(entry == NULL) { // put into symbol table
            idNode->semantic_value.identifierSemanticValue.symbolTableEntry = enterSymbol(idName, makeSymbolAttribute(idNode));
        }
        else{
            printErrorMsg(idNode, NULL, SYMBOL_REDECLARED);
        }
        idNode = idNode->rightSibling;
    }
    return;
}

SymbolAttribute *makeFunctionAttribute(AST_NODE *idNode){
    SymbolAttribute *attribute = (SymbolAttribute *)malloc(sizeof(SymbolAttribute));
    attribute->attributeKind = FUNCTION_SIGNATURE;
    attribute->attr.functionSignature = (FunctionSignature *)malloc(sizeof(FunctionSignature));
    attribute->attr.functionSignature->returnType = idNode->leftmostSibling->dataType;
    attribute->attr.functionSignature->parametersCount = 0;
    Parameter *tail = NULL;
    AST_NODE *parameterDeclNode = idNode->rightSibling->child;
    while( parameterDeclNode != NULL ){
        declareIdList(parameterDeclNode);
        if( tail == NULL ){
            attribute->attr.functionSignature->parameterList = (Parameter *)malloc(sizeof(Parameter));
            tail = attribute->attr.functionSignature->parameterList;
        }
        else{
            tail->next = (Parameter *)malloc(sizeof(Parameter));
            tail = tail->next;
        }
        tail->type = (TypeDescriptor *)malloc(sizeof(TypeDescriptor));
        memcpy(tail->type, parameterDeclNode->child->rightSibling->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor, sizeof(TypeDescriptor));
        tail->parameterName = parameterDeclNode->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
        attribute->attr.functionSignature->parametersCount += 1;

        parameterDeclNode = parameterDeclNode->rightSibling;
    }
    return attribute;
}

void declareFunction(AST_NODE* declarationNode)
{
    AST_NODE *typeNode = declarationNode->child;
    AST_NODE *idNode = typeNode->rightSibling;
    processTypeNode(typeNode);
    char *idName = idNode->semantic_value.identifierSemanticValue.identifierName;
    if( declaredLocally(idName) != NULL ){
        printErrorMsg(idNode, NULL, SYMBOL_REDECLARED);
    }
    enterSymbol(idName, NULL);
    
    openScope();
    SymbolTableEntry *functionEntry = retrieveSymbol(idName);
    functionEntry->attribute = makeFunctionAttribute(idNode);
    AST_NODE *blockNode = idNode->rightSibling->rightSibling;
    processBlockNode(blockNode);
    closeScope();
    return;
}

void processBlockNode(AST_NODE* blockNode)
{   //If it's a NUL_NULL or an empty blockNode, return immediately.
    if( blockNode->nodeType == NUL_NODE || blockNode->child == NULL )
        return;
    
    AST_NODE *listNode = blockNode->child;
    while( listNode != NULL ){
        AST_NODE *child;
        switch(listNode->nodeType){
            case VARIABLE_DECL_LIST_NODE:
                processDeclarationListNode(listNode);
                break;
            case STMT_LIST_NODE:
                child = listNode->child;
                while( child != NULL ){
                    processStmtNode(child);
                    child = child->rightSibling;
                }
                break;
            default:
                break;
        }
        listNode = listNode->rightSibling;
    }
    return;
}

void processStmtNode(AST_NODE* stmtNode)
{
    switch(stmtNode->semantic_value.stmtSemanticValue.kind){
        case WHILE_STMT:
            processWhileStmt(stmtNode);
            break;
        case FOR_STMT:
            processForStmt(stmtNode);
            break;
        case ASSIGN_STMT:
            processAssignmentStmt(stmtNode);
            break;
        case IF_STMT:
            processIfStmt(stmtNode);
            break;
        case FUNCTION_CALL_STMT:
            processFunctionCall(stmtNode);
            break;
        case RETURN_STMT:
            processReturnStmt(stmtNode);
            break;
    }
    return;
}

void processWhileStmt(AST_NODE* whileNode)      //process assignment + block
{
    processAssignmentStmt(whileNode->child);
    openScope();
    processBlockNode(whileNode->child->rightSibling);
    closeScope();
    return;
}

void processAssignmentList(AST_NODE *assignmentListNode)   //process assignment list in for-loop
{
    if( assignmentListNode->nodeType == NUL_NODE )
        return;
    AST_NODE *assignStmtNode = assignmentListNode->child;
    while( assignStmtNode != NULL ){
        processAssignmentStmt(assignStmtNode);
        assignStmtNode = assignStmtNode->rightSibling;
    }
    return;
}

void processRelopList(AST_NODE *relopListNode)         //process relop list in for-loop
{
    if( relopListNode->nodeType == NUL_NODE )
        return;
    AST_NODE *relopNode = relopListNode->child;
    while( relopNode != NULL ){
        processRelopNode(relopNode);
        relopNode = relopNode->rightSibling;
    }
    return;
}

void processForStmt(AST_NODE* forNode)      //for = assignment list + relop list + assignment list + block
{   
    processAssignmentList(forNode->child);
    processRelopList(forNode->child->rightSibling);
    processAssignmentList(forNode->child->rightSibling->rightSibling);
    openScope();
    processBlockNode(forNode->child->rightSibling->rightSibling->rightSibling);
    closeScope();
    return;
}

void processAssignmentStmt(AST_NODE* assignmentNode)
{
    switch(assignmentNode->nodeType){
        case STMT_NODE:             // e.g. a = b
            processVariableLValue(assignmentNode->child);
            processVariableRValue(assignmentNode->child->rightSibling);
            if( assignmentNode->child->dataType == FLOAT_TYPE ){
                assignmentNode->child->rightSibling->dataType = FLOAT_TYPE;
            }
            assignmentNode->dataType = assignmentNode->child->dataType;
            break;
        case EXPR_NODE:
            if( isRelOp(assignmentNode->semantic_value.exprSemanticValue) ){        //while(a < b)
                processRelopNode(assignmentNode->child);
                if( assignmentNode->child->rightSibling ){
                    processRelopNode(assignmentNode->child->rightSibling);
                }
                assignmentNode->dataType = INT_TYPE;
            }
            else{                   //e.g. while(a+b)
                processVariableRValue(assignmentNode);
            }
            break;
        case CONST_VALUE_NODE:      //e.g. while(1)
            updateConstNodeType(assignmentNode);
            break;
        case IDENTIFIER_NODE:
            processVariableRValue(assignmentNode);
            break;
        default:
            break;
    }
    return;   
}

void processIfStmt(AST_NODE* ifNode)
{
    processAssignmentStmt(ifNode->child);                            //assignment(condition)
    openScope();
    processBlockNode(ifNode->child->rightSibling);                 //"then" block
    closeScope();
    openScope();
    processBlockNode(ifNode->child->rightSibling->rightSibling);   //"else" block
    closeScope();
    return;
}

void processReadFunction(AST_NODE *functioncallNode)
{

}

void processWriteFunction(AST_NODE* functionCallNode)
{
}

void processFunctionCall(AST_NODE* functionCallNode)
{

}

void processParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter)
{

}

void processRelopNode(AST_NODE* relopNode)
{
    switch(relopNode->nodeType){
        case CONST_VALUE_NODE:      //e.g. for(; 1; )
            updateConstNodeType(relopNode);
            break;
        case EXPR_NODE:             //e.g. for(; a > b;)
            if( isRelOp(relopNode->semantic_value.exprSemanticValue) ){
                processRelopNode(relopNode->child);
                if( relopNode->child->rightSibling ){
                    processRelopNode(relopNode->child->rightSibling);
                }
                relopNode->dataType = INT_TYPE;
            }
            else{                   //e.g. for(; a+b;)
                processVariableRValue(relopNode);
            }
            break;
        case IDENTIFIER_NODE:       //e.g. for(; a; )
            processVariableRValue(relopNode);
            break;
        default:
            break;
    }
}

//update the node type of const node
void updateConstNodeType(AST_NODE* constNode)
{
    switch(constNode->semantic_value.const1->const_type){
        case INTEGERC:
            constNode->dataType = INT_TYPE;
            break;
        case FLOATC:
            constNode->dataType = FLOAT_TYPE;
            break;
        case STRINGC:
            constNode->dataType = CONST_STRING_TYPE;
            break;
    }
    return;
}

void checkIdDimension(AST_NODE *idNode, SymbolTableEntry *entry)
{
    int dimInEntry = 0, dimInASTNode = 0;
    if( entry->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR ){
        dimInEntry = entry->attribute->attr.typeDescriptor->properties.arrayProperties.dimension;
    }
    if( idNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID ){
        AST_NODE *dimNode = idNode->child;
        while( dimNode != NULL ){
            dimInASTNode += 1;
            dimNode = dimNode->rightSibling;
        }
    }
    if( dimInEntry > dimInASTNode ){
        printErrorMsg(idNode, NULL, IMCOMPATIBLE_ARRAY_DIMENSION_DECL_GT_REF);
    }
    else if( dimInEntry < dimInASTNode ){
        printErrorMsg(idNode, NULL, IMCOMPATIBLE_ARRAY_DIMENSION_DECL_LT_REF);
    }

    AST_NODE *dimNode = idNode->child;
    while( dimNode != NULL ){
        processVariableRValue(dimNode);
        if( dimNode->dataType != INT_TYPE )
            printErrorMsg(dimNode, NULL, ARRAY_SUBSCRIPT_NOT_INT);
        dimNode = dimNode->rightSibling;
    }
    
    idNode->semantic_value.identifierSemanticValue.symbolTableEntry = entry;
    if( entry->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR ){
        idNode->dataType = entry->attribute->attr.typeDescriptor->properties.dataType;
    }
    else{
        idNode->dataType = entry->attribute->attr.typeDescriptor->properties.arrayProperties.elementType;
    }
    return;
}

void processVariableLValue(AST_NODE* idNode)
{
    SymbolTableEntry *entry = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
    if( entry == NULL ){
        printErrorMsg(idNode, NULL, SYMBOL_UNDECLARED);
    }
    else if( entry->attribute->attributeKind != VARIABLE_ATTRIBUTE ){
        printErrorMsg(idNode, NULL, NOT_ASSIGNABLE);
    }
    checkIdDimension(idNode, entry);
    return;
}

int isRelOp(EXPRSemanticValue exprSemanticValue)
{
    if( exprSemanticValue.kind == BINARY_OPERATION ){
        switch(exprSemanticValue.op.binaryOp){
            case BINARY_OP_EQ:
            case BINARY_OP_GE:
            case BINARY_OP_LE:
            case BINARY_OP_NE:
            case BINARY_OP_GT:
            case BINARY_OP_LT:
            case BINARY_OP_AND:
            case BINARY_OP_OR:
                return 1;
            default:
                return 0;
        }
    }
    else if( exprSemanticValue.op.unaryOp == UNARY_OP_LOGICAL_NEGATION ){
        return 1;
    }
    return 0;
}

void processVariableRValue(AST_NODE* idNode)
{
    int isBinaryOp;
    SymbolTableEntry *entry;
    switch(idNode->nodeType){
        case CONST_VALUE_NODE:
            updateConstNodeType(idNode);
            break;
        case EXPR_NODE:
            if( isRelOp(idNode->semantic_value.exprSemanticValue) ){
                processRelopNode(idNode);
            }
            else{
                isBinaryOp = (idNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION);
                processVariableRValue(idNode->child);
                idNode->dataType = idNode->child->dataType;
                if( isBinaryOp ){
                    processVariableRValue(idNode->child->rightSibling);
                    idNode->dataType = getBiggerType(idNode->dataType, idNode->child->rightSibling->dataType);
                }
            }
            break;
        case IDENTIFIER_NODE:
            entry = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
            if( entry == NULL ){
                printErrorMsg(idNode, NULL, SYMBOL_UNDECLARED);
            }
            else if( entry->attribute->attributeKind != VARIABLE_ATTRIBUTE ){
                printErrorMsg(idNode, NULL, NOT_REFERABLE);
            }
            else{
                checkIdDimension(idNode, entry);
            }
            break;
        case STMT_NODE:         //function call stmt
            processFunctionCall(idNode);
            break;
    }
    return;
}

void processReturnStmt(AST_NODE* returnNode)
{
}




void processGeneralNode(AST_NODE *node)
{
}

void processDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize)
{
}

void processExprNode(AST_NODE* exprNode)
{
}

void processConstValueNode(AST_NODE* constValueNode)
{
}

void evaluateExprValue(AST_NODE* exprNode)
{
}

void processAssignOrExpr(AST_NODE* assignOrExprRelatedNode)
{

}