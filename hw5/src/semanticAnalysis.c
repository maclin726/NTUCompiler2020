#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "symbolTable.h"
#include "errorMsg.h"

// This file is for reference only, you are not required to follow the implementation. //
// You only need to check for errors stated in the hw4 document. //

DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2);
void processProgramNode(AST_NODE* programNode);
void processDeclarationListNode(AST_NODE* declarationNode);
void processTypeNode(AST_NODE* typeNode);
SymbolAttribute *makeSymbolAttribute(AST_NODE* idNode);
int processConstExprNode(AST_NODE* constExprNode);
void makeItConstNode(AST_NODE* constExprNode, int inum, float fnum, int isInt);
void declareIdList(AST_NODE* declarationNode);
SymbolAttribute *makeFunctionAttribute(AST_NODE *idNode);
void declareFunction(AST_NODE* declarationNode);
void processBlockNode(AST_NODE* blockNode);
void processStmtNode(AST_NODE* stmtNode);
void processWhileStmt(AST_NODE* whileNode);
void processAssignmentList(AST_NODE* assignmentListNode);
void processRelopList(AST_NODE* relopListNode) ;
void processForStmt(AST_NODE* forNode);
void processAssignmentStmt(AST_NODE* assignmentNode);
void processIfStmt(AST_NODE* ifNode);
void processReadFunction(AST_NODE* functioncallNode);
void processFreadFunction(AST_NODE* functioncallNode);
void processWriteFunction(AST_NODE* functionCallNode);
void processFunctionCall(AST_NODE* functionCallNode);
void checkParamNodeType(TypeDescriptor* curFormalType, AST_NODE* curActual, int useTypeDescriptor);
void processParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter);
void processRelopNode(AST_NODE* relopNode);
void updateConstNodeType(AST_NODE* constNode);
void checkIdDimension(AST_NODE* idNode, SymbolTableEntry* entry);
void processVariableLValue(AST_NODE* idNode);
int isRelOp(EXPRSemanticValue exprSemanticValue);
void processVariableRValue(AST_NODE* idNode);
void processReturnStmt(AST_NODE* returnNode);


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
SymbolAttribute *makeSymbolAttribute(AST_NODE* idNode)
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
            makeItConstNode(dimNode, 0, 0, 0);
            IDdim++;
            dimNode = dimNode->rightSibling;
        }
        while( dimNode != NULL ){
            int isInt = processConstExprNode(dimNode);
            if( isInt == 0 ){
                printErrorMsg(idNode, NULL, ARRAY_SIZE_NOT_INT);
            }
            else if( dimNode->semantic_value.const1->const_u.intval < 0 ){
                printErrorMsg(idNode, NULL, ARRAY_SIZE_NEGATIVE);
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

int processConstExprNode(AST_NODE* constExprNode)   //return whether the expression is INT
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
            makeItConstNode(constExprNode, 0, 0, 0);  // error, but let the node to be 0 => arr[0]
            return 0;
        }
        int lVal, rVal;
        lVal = constExprNode->child->semantic_value.const1->const_u.intval;
        if( isBinaryOp ) {
            rVal = constExprNode->child->rightSibling->semantic_value.const1->const_u.intval;
            switch (constExprNode->semantic_value.exprSemanticValue.op.binaryOp){
                case BINARY_OP_ADD: curNodeConst = lVal + rVal;       break;
                case BINARY_OP_SUB: curNodeConst = lVal - rVal;       break;
                case BINARY_OP_MUL: curNodeConst = lVal * rVal;       break;
                case BINARY_OP_DIV: curNodeConst = lVal / rVal;       break;
                case BINARY_OP_EQ:  curNodeConst = (lVal == rVal);    break;
                case BINARY_OP_GE:  curNodeConst = (lVal >= rVal);    break;
                case BINARY_OP_GT:  curNodeConst = (lVal > rVal);     break;
                case BINARY_OP_LE:  curNodeConst = (lVal <= rVal);    break;
                case BINARY_OP_LT:  curNodeConst = (lVal < rVal);     break;
                case BINARY_OP_OR:  curNodeConst = (lVal || rVal);    break;
                case BINARY_OP_AND: curNodeConst = (lVal && rVal);    break;
                default:                                              break;
            }
        }
        else {
            switch (constExprNode->semantic_value.exprSemanticValue.op.unaryOp){
                case UNARY_OP_NEGATIVE: curNodeConst = -lVal;                   break;
                case UNARY_OP_LOGICAL_NEGATION: curNodeConst = (!lVal);         break;
                case UNARY_OP_POSITIVE: curNodeConst = (lVal);                  break;
                default:                                                        break;
            }
        }
        makeItConstNode(constExprNode, curNodeConst, 0, 1);
    }
    if( constExprNode->nodeType == CONST_VALUE_NODE && constExprNode->semantic_value.const1->const_type == INTEGERC ){
        return 1;
    }
    return 0;
}

void makeItConstNode(AST_NODE* constExprNode, int inum, float fnum, int isInt)
{
    if( isInt ){
        constExprNode->nodeType = CONST_VALUE_NODE;
        constExprNode->dataType = INT_TYPE;
        constExprNode->semantic_value.const1 = (CON_Type *)malloc(sizeof(CON_Type));
        constExprNode->semantic_value.const1->const_type = INTEGERC;
        constExprNode->semantic_value.const1->const_u.intval = inum;
    }
    else{
        constExprNode->nodeType = CONST_VALUE_NODE;
        constExprNode->dataType = FLOAT_TYPE;
        constExprNode->semantic_value.const1 = (CON_Type *)malloc(sizeof(CON_Type));
        constExprNode->semantic_value.const1->const_type = FLOATC;
        constExprNode->semantic_value.const1->const_u.fval = fnum;
    }
    return;
}

float floatArithmeticCalc(float lVal, float rVal, BINARY_OPERATOR op)
{
    switch(op){
        case BINARY_OP_ADD: return lVal + rVal;
        case BINARY_OP_SUB: return lVal - rVal;
        case BINARY_OP_MUL: return lVal * rVal;
        case BINARY_OP_DIV: return lVal / rVal;
        default: break;
    }
    return 0;
}

int floatLogicalRelationCalc(float lVal, float rVal, BINARY_OPERATOR op)
{
    switch(op){
        case BINARY_OP_EQ:  return (lVal == rVal);
        case BINARY_OP_GE:  return (lVal >= rVal);
        case BINARY_OP_GT:  return (lVal > rVal); 
        case BINARY_OP_LE:  return (lVal <= rVal);
        case BINARY_OP_LT:  return (lVal < rVal); 
        case BINARY_OP_OR:  return (lVal || rVal);
        case BINARY_OP_AND: return (lVal && rVal);
        default: break;
    }
    return 0;
}

//return 0 if not const, 1 if const int, 2 if const float
int processGlobalInitValue(AST_NODE *exprNode){
    if( exprNode->nodeType == CONST_VALUE_NODE ){
        if( exprNode->semantic_value.const1->const_type == INTEGERC ){
            exprNode->dataType = INT_TYPE;
            return 1;
        }
        else{
            exprNode->dataType = FLOAT_TYPE;
            return 2;
        }
    }
    else if( exprNode->nodeType == EXPR_NODE ){
        int leftIsConst, rightIsConst = 1;
        int isBinaryOp = ( exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION )? 1 : 0;
        leftIsConst = processGlobalInitValue(exprNode->child);
        if( isBinaryOp ) {
            rightIsConst = processGlobalInitValue(exprNode->child->rightSibling);
        }
        if( leftIsConst == 0 || rightIsConst == 0 ){    //error, not a const node
            return 0;
        }

        if( leftIsConst == 1 && rightIsConst == 1 ){
            int lVal, rVal, curNodeConst;
            lVal = exprNode->child->semantic_value.const1->const_u.intval;
            if( isBinaryOp ) {      // int + int
                rVal = exprNode->child->rightSibling->semantic_value.const1->const_u.intval;
                switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp){
                    case BINARY_OP_ADD: curNodeConst = lVal + rVal;       break;
                    case BINARY_OP_SUB: curNodeConst = lVal - rVal;       break;
                    case BINARY_OP_MUL: curNodeConst = lVal * rVal;       break;
                    case BINARY_OP_DIV: curNodeConst = lVal / rVal;       break;
                    case BINARY_OP_EQ:  curNodeConst = (lVal == rVal);    break;
                    case BINARY_OP_GE:  curNodeConst = (lVal >= rVal);    break;
                    case BINARY_OP_GT:  curNodeConst = (lVal > rVal);     break;
                    case BINARY_OP_LE:  curNodeConst = (lVal <= rVal);    break;
                    case BINARY_OP_LT:  curNodeConst = (lVal < rVal);     break;
                    case BINARY_OP_OR:  curNodeConst = (lVal || rVal);    break;
                    case BINARY_OP_AND: curNodeConst = (lVal && rVal);    break;
                    default:                                              break;
                }
            }
            else {                  // !(int)
                switch (exprNode->semantic_value.exprSemanticValue.op.unaryOp){
                    case UNARY_OP_NEGATIVE: curNodeConst = -lVal;                   break;
                    case UNARY_OP_LOGICAL_NEGATION: curNodeConst = (!lVal);         break;
                    case UNARY_OP_POSITIVE: curNodeConst = (lVal);                  break;
                    default:                                                        break;
                }
            }
            makeItConstNode(exprNode, curNodeConst, 0, 1);
            return 1;
        }
        else{       //at least one is float
            float lVal, rVal, curNodeConstf;
            int curNodeConsti;
            lVal = leftIsConst == 1 ? exprNode->child->semantic_value.const1->const_u.intval:
                                      exprNode->child->semantic_value.const1->const_u.fval;
            if( isBinaryOp ) {
                rVal = rightIsConst == 1 ? exprNode->child->rightSibling->semantic_value.const1->const_u.intval:
                                            exprNode->child->rightSibling->semantic_value.const1->const_u.fval;
                switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp){
                    case BINARY_OP_ADD: case BINARY_OP_SUB: case BINARY_OP_MUL: case BINARY_OP_DIV:
                        curNodeConstf = floatArithmeticCalc(lVal, rVal, exprNode->semantic_value.exprSemanticValue.op.binaryOp);
                        makeItConstNode(exprNode, 0, curNodeConstf, 0);
                        return 2;
                    default:
                        curNodeConsti = floatLogicalRelationCalc(lVal, rVal, exprNode->semantic_value.exprSemanticValue.op.binaryOp);
                        makeItConstNode(exprNode, curNodeConsti, 0, 1);
                        return 1;
                }
            }
            else {
                switch (exprNode->semantic_value.exprSemanticValue.op.unaryOp){
                    case UNARY_OP_NEGATIVE:
                        curNodeConstf = -lVal;
                        makeItConstNode(exprNode, 0, curNodeConstf, 0);
                        return 2;
                        break;
                    case UNARY_OP_POSITIVE:
                        curNodeConstf = lVal;
                        makeItConstNode(exprNode, 0, curNodeConstf, 0);
                        return 2;
                        break;
                    case UNARY_OP_LOGICAL_NEGATION:
                        curNodeConsti = !lVal;
                        makeItConstNode(exprNode, curNodeConsti, 0, 1);
                        return 1;
                        break;
                    default:
                        break;
                }
            }
        }
    }
    return 0;
}

//typedef, var decl, and param decl
void declareIdList(AST_NODE* declarationNode)
{
    AST_NODE *typeNode = declarationNode->child;
    AST_NODE *idNode = typeNode->rightSibling;
    processTypeNode(typeNode);
    if(typeNode->dataType == ERROR_TYPE)
        return;
    while(idNode != NULL) {
        idNode->dataType = typeNode->dataType;

        char *idName = idNode->semantic_value.identifierSemanticValue.identifierName;

        SymbolTableEntry *entry = declaredLocally(idName);
        if(entry == NULL) { // put into symbol table
            idNode->semantic_value.identifierSemanticValue.symbolTableEntry = enterSymbol(idName, makeSymbolAttribute(idNode));
        }
        else{
            printErrorMsg(idNode, NULL, SYMBOL_REDECLARED);
            idNode = idNode->rightSibling;
            continue;
        }
        
        if(idNode->semantic_value.identifierSemanticValue.kind == WITH_INIT_ID){
            if( idNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR ){
                printErrorMsg(idNode, NULL, TRY_TO_INIT_ARRAY);
            }
            else if( idNode->semantic_value.identifierSemanticValue.symbolTableEntry->nestingLevel == 0 ){
                processGlobalInitValue(idNode->child);
                if( idNode->child->nodeType != CONST_VALUE_NODE )
                    printErrorMsg(idNode, NULL, ASSIGN_NON_CONST_TO_GLOBAL);
            }
            else{
                processVariableRValue(idNode->child);
            }
        }

        idNode = idNode->rightSibling;
    }
    return;
}

SymbolAttribute* makeFunctionAttribute(AST_NODE* idNode){
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
    if( stmtNode->nodeType == BLOCK_NODE ){
        openScope();
        processBlockNode(stmtNode);
        closeScope();
    }
    else if(stmtNode->nodeType != NUL_NODE){
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

void processAssignmentList(AST_NODE* assignmentListNode)   //process assignment list in for-loop
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

void processRelopList(AST_NODE* relopListNode)         //process relop list in for-loop
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
        case STMT_NODE:             
            if( assignmentNode->semantic_value.stmtSemanticValue.kind == FUNCTION_CALL_STMT ){  //e.g. a = func();
                processFunctionCall(assignmentNode);
            }
            else{       // e.g. a = b;
                processVariableLValue(assignmentNode->child);
                processVariableRValue(assignmentNode->child->rightSibling);
                assignmentNode->dataType = assignmentNode->child->dataType;
            }
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
        case IDENTIFIER_NODE:       //e.g. while(a)
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

void processReadFunction(AST_NODE* functioncallNode)
{
    if( functioncallNode->child->rightSibling->nodeType != NUL_NODE ){
        printErrorMsg(functioncallNode->child, NULL, TOO_MANY_ARGUMENT);
    }
    return;
}

void processFreadFunction(AST_NODE* functioncallNode)
{
    if( functioncallNode->child->rightSibling->nodeType != NUL_NODE ){
        printErrorMsg(functioncallNode->child, NULL, TOO_MANY_ARGUMENT);
    }
    return;
}

void processWriteFunction(AST_NODE* functionCallNode)
{
    if( functionCallNode->child->rightSibling->nodeType == NUL_NODE ){                  // < 1 argument
        printErrorMsg(functionCallNode->child, NULL, TOO_FEW_ARGUMENT);
    }
    else if( functionCallNode->child->rightSibling->child->rightSibling != NULL ){      // >= 2 argument
        printErrorMsg(functionCallNode->child, NULL, TOO_MANY_ARGUMENT);
    }
    else{
        AST_NODE *paramNode = functionCallNode->child->rightSibling->child;
        processVariableRValue(paramNode);
    }
    return;
}

void processFunctionCall(AST_NODE* functionCallNode)
{
    if( strcmp("write", functionCallNode->child->semantic_value.identifierSemanticValue.identifierName) == 0 ){
        processWriteFunction(functionCallNode);
        functionCallNode->dataType = INT_TYPE;
    }
    else if( strcmp("read", functionCallNode->child->semantic_value.identifierSemanticValue.identifierName) == 0 ){
        processReadFunction(functionCallNode);
        functionCallNode->dataType = INT_TYPE;
    }
    else if( strcmp("fread", functionCallNode->child->semantic_value.identifierSemanticValue.identifierName) == 0 ){
        processFreadFunction(functionCallNode);
        functionCallNode->dataType = FLOAT_TYPE;
    }
    else{
        SymbolTableEntry *funcEntry = retrieveSymbol(functionCallNode->child->semantic_value.identifierSemanticValue.identifierName);
        if(funcEntry == NULL){
            printErrorMsg(functionCallNode->child, NULL, SYMBOL_UNDECLARED);
            functionCallNode->dataType = ERROR_TYPE;
        }
        else if( funcEntry->attribute->attributeKind != FUNCTION_SIGNATURE ){
            printErrorMsg(functionCallNode->child, NULL, NOT_FUNCTION_NAME);
            functionCallNode->dataType = ERROR_TYPE;
        }
        else{
            processParameterPassing(funcEntry->attribute->attr.functionSignature->parameterList, 
                                    functionCallNode->child->rightSibling);
            functionCallNode->dataType = funcEntry->attribute->attr.functionSignature->returnType;
        }
    }
    return;
}

/* useTypeDescriptor = 1, then it's the case of single identifier(maybe with subscript); 
   Otherwise, it must be a scalar */
void checkParamNodeType(TypeDescriptor* curFormalType, AST_NODE* curActual, int useTypeDescriptor)
{
    if( useTypeDescriptor ){
        SymbolTableEntry *actualEntry = retrieveSymbol(curActual->semantic_value.identifierSemanticValue.identifierName);
        if( actualEntry == NULL ){
            printErrorMsg(curActual, NULL, SYMBOL_UNDECLARED);
            curActual->dataType = ERROR_TYPE;
        }
        else if( actualEntry->attribute->attributeKind != VARIABLE_ATTRIBUTE ){
            printErrorMsg(curActual, NULL, NOT_REFERABLE);
            curActual->dataType = ERROR_TYPE;
        }
        else{
            int actualDim = 0, formalDim = 0;
            if( curFormalType->kind == ARRAY_TYPE_DESCRIPTOR )
                formalDim = curFormalType->properties.arrayProperties.dimension;

            //compute actual dimension 
            if( actualEntry->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR ){
                actualDim = actualEntry->attribute->attr.typeDescriptor->properties.arrayProperties.dimension;
                curActual->dataType = actualEntry->attribute->attr.typeDescriptor->properties.arrayProperties.elementType;
            }
            else{
                curActual->dataType = actualEntry->attribute->attr.typeDescriptor->properties.dataType;
            }
            AST_NODE *dimNode = curActual->child;
            while( dimNode != NULL ){
                actualDim--;
                processVariableRValue(dimNode);
                if( dimNode->dataType != INT_TYPE && dimNode->dataType != ERROR_TYPE)
                    printErrorMsg(curActual, NULL, ARRAY_SUBSCRIPT_NOT_INT);
                dimNode = dimNode->rightSibling;
            }

            if( actualDim < 0 ){    //e.g. declare A[10] but use A[1][1] as param
                printErrorMsg(curActual, NULL, INCOMPATIBLE_ARRAY_DIMENSION_DECL_LT_REF);
            }
            else if( formalDim == 0 && actualDim > 0 ){
                char str[16] = {0};
                if( curActual->dataType == INT_TYPE )
                    strncpy(str, "int", 3);
                else if( curActual->dataType == FLOAT_TYPE )
                    strncpy(str, "float", 5);
                if( curFormalType->properties.dataType == INT_TYPE )
                    strncpy(&str[6], "int", 3);
                else if( curFormalType->properties.dataType == FLOAT_TYPE )
                    strncpy(&str[6], "float", 5);
                printErrorMsg(curActual, str, PASS_ARRAY_TO_SCALAR);
            }
            else if ( formalDim > 0 && actualDim == 0){
                char str[16] = {0};
                if( curActual->dataType == INT_TYPE )
                    strncpy(str, "int", 3);
                else if( curActual->dataType == FLOAT_TYPE )
                    strncpy(str, "float", 5);
                if( curFormalType->properties.arrayProperties.elementType == INT_TYPE )
                    strncpy(&str[6], "int", 3);
                else if( curFormalType->properties.arrayProperties.elementType == FLOAT_TYPE )
                    strncpy(&str[6], "float", 5);
                printErrorMsg(curActual, str, PASS_SCALAR_TO_ARRAY);
            }
            else{       //convert type to the same as formal param
                if( curFormalType->kind == SCALAR_TYPE_DESCRIPTOR ){
                    curActual->dataType = curFormalType->properties.dataType;
                }
                else{
                    curActual->dataType = curFormalType->properties.arrayProperties.elementType;
                }
            }
        }
    }
    else{   //actual param must be scalar
        if( curFormalType->kind == ARRAY_TYPE_DESCRIPTOR ){
            char str[16] = {0};
            if( curActual->dataType == INT_TYPE )
                strncpy(str, "int", 3);
            else if( curActual->dataType == FLOAT_TYPE )
                strncpy(str, "float", 5);
            if( curFormalType->properties.arrayProperties.elementType == INT_TYPE )
                strncpy(&str[6], "int", 3);
            else if( curFormalType->properties.arrayProperties.elementType == FLOAT_TYPE )
                strncpy(&str[6], "float", 5);
            printErrorMsg(curActual, str, PASS_SCALAR_TO_ARRAY);
        }
        else{
            curActual->dataType = curFormalType->properties.dataType;   //convert to type as formal param
        }
    }
    return;
}

void processParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter)
{
    if( actualParameter->nodeType == NUL_NODE && formalParameter != NULL ){
        printErrorMsg(actualParameter->leftmostSibling, NULL, TOO_FEW_ARGUMENT);
        return;
    }

    Parameter *curFormal = formalParameter;
    AST_NODE *curActual = actualParameter->child;
    while( curFormal != NULL && curActual != NULL ){
        switch(curActual->nodeType){
            case CONST_VALUE_NODE:      //func(1)
                updateConstNodeType(curActual);
                checkParamNodeType(curFormal->type, curActual, 0);
                break;
            case EXPR_NODE:
                if( isRelOp(curActual->semantic_value.exprSemanticValue) ){     //e.g. func( a>b )
                    processRelopNode(curActual->child);
                    if( curActual->child->rightSibling ){       //binary op, process right child
                        processRelopNode(curActual->child->rightSibling);
                    }
                    curActual->dataType = INT_TYPE;
                }
                else{                   //e.g. func(a+b);
                    processVariableRValue(curActual);
                }
                checkParamNodeType(curFormal->type, curActual, 0);
                break;
            case IDENTIFIER_NODE:       //e.g. func(a), might pass an array or a scalar
                checkParamNodeType(curFormal->type, curActual, 1);
                break;
            case STMT_NODE:             //e.g. func(func(1))
                processFunctionCall(curActual);
                checkParamNodeType(curFormal->type, curActual, 0);
                break;
            default:
                break;
        }
        curFormal = curFormal->next;
        curActual = curActual->rightSibling;
    }
    if( curActual == NULL && curFormal != NULL ){
        printErrorMsg(actualParameter->leftmostSibling, NULL, TOO_FEW_ARGUMENT);
    }
    else if( curActual != NULL && curFormal == NULL ){
        printErrorMsg(actualParameter->leftmostSibling, NULL, TOO_MANY_ARGUMENT);
    }
    return;
}

void processRelopNode(AST_NODE* relopNode)
{
    switch(relopNode->nodeType){
        case CONST_VALUE_NODE:      //e.g. for(; 1; )
            updateConstNodeType(relopNode);
            break;
        case EXPR_NODE:
            if( isRelOp(relopNode->semantic_value.exprSemanticValue) ){     //e.g. for(; a > b;)
                processRelopNode(relopNode->child);
                if( relopNode->child->rightSibling ){       //binary op, process right child
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

void checkIdDimension(AST_NODE* idNode, SymbolTableEntry* entry)    //check if a subscripted id is a legel rvalue
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
        printErrorMsg(idNode, NULL, INCOMPATIBLE_ARRAY_DIMENSION_DECL_GT_REF);
    }
    else if( dimInEntry < dimInASTNode ){
        printErrorMsg(idNode, NULL, INCOMPATIBLE_ARRAY_DIMENSION_DECL_LT_REF);
    }

    AST_NODE *dimNode = idNode->child;
    while( dimNode != NULL ){
        processVariableRValue(dimNode);
        if( dimNode->dataType != INT_TYPE && dimNode->dataType != ERROR_TYPE)
            printErrorMsg(idNode, NULL, ARRAY_SUBSCRIPT_NOT_INT);
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
        idNode->dataType = ERROR_TYPE;
        return;
    }
    else if( entry->attribute->attributeKind != VARIABLE_ATTRIBUTE ){
        printErrorMsg(idNode, NULL, NOT_ASSIGNABLE);
        idNode->dataType = ERROR_TYPE;
        return;
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
                idNode->dataType = ERROR_TYPE;
            }
            else if( entry->attribute->attributeKind != VARIABLE_ATTRIBUTE ){
                printErrorMsg(idNode, NULL, NOT_REFERABLE);
                idNode->dataType = ERROR_TYPE;
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
    AST_NODE *ancestor = returnNode->parent;
    while( ! (ancestor->nodeType == DECLARATION_NODE && ancestor->semantic_value.declSemanticValue.kind == FUNCTION_DECL ) ){
        ancestor = ancestor->parent;
    }
    if( returnNode->child == NULL ){
        if( ancestor->child->dataType != VOID_TYPE )
            printErrorMsg(returnNode, NULL, RETURN_TYPE_UNMATCH);
        else{
            returnNode->dataType = VOID_TYPE;
        }
    }
    else if( ancestor->child->dataType == VOID_TYPE ){
        printErrorMsg(returnNode, NULL, RETURN_TYPE_UNMATCH);
    }
    else{       
        /*  we don't call processVariableRValue here immediately, 
            since the error message in return is different from that in processVariableRvalue.  */
        if( returnNode->child->nodeType == IDENTIFIER_NODE ){
            SymbolTableEntry *returnIDEntry = retrieveSymbol(returnNode->child->semantic_value.identifierSemanticValue.identifierName);
            if( returnIDEntry == NULL || returnIDEntry->attribute->attributeKind == TYPE_ATTRIBUTE){
                processVariableRValue(returnNode->child);
                return;
            }

            int idDim = 0, nodeDim = 0;
            if( returnIDEntry->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR )
                idDim = returnIDEntry->attribute->attr.typeDescriptor->properties.arrayProperties.dimension;
            
            AST_NODE *dimNode = returnNode->child->child;
            while( dimNode != NULL ){
                nodeDim++;
                dimNode = dimNode->rightSibling;
            }
            if( nodeDim < idDim ){
                printErrorMsg(returnNode, NULL, RETURN_ARRAY);
                return;
            }
            else{
                processVariableRValue(returnNode->child);
                returnNode->dataType = ancestor->child->dataType;   //we store the return type in return->dataType
                return;
            }
        }
        processVariableRValue(returnNode->child);
        returnNode->dataType = ancestor->child->dataType;   //we store the return type in return->dataType
    }
    return;
}