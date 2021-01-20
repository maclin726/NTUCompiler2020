#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbolTable.h"

void gen_function_call(AST_NODE *stmtNode, int *ARoffset);
void gen_block(AST_NODE *blockNode, int *ARoffset, char *funcName);
void gen_localVar(AST_NODE *blockNode, int *ARoffset);
int isRelOp(EXPRSemanticValue exprSemanticValue);
void gen_expr(AST_NODE *exprNode, int *ARoffset);
void gen_stmt(AST_NODE *stmtNode, int *ARoffset, char *funcName);
void gen_assign(AST_NODE *left, AST_NODE *right, int *ARoffset);

    FILE *output;

const char int_reg[18][4] = {"x5", "x6", "x7", "x9", "x18", "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};
const char float_reg[24][4] = {"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f18", "f19", "f20", "f21", "f22", "f23", "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"};

int cur_int_reg = 0, cur_float_reg = 0;
int cur_const = 0;
int total_if = 0, total_for = 0, total_while = 0;
int total_and = 0, total_or = 0;

unsigned int get_float_bit(float f){
    union ufloat{
        unsigned u;
        float f;
    };
    union ufloat u1;
    u1.f = f;
    return u1.u;
}

int get_reg(int type)   //0 for int_reg, 1 for float_reg
{
    if( type == 0 ){
        cur_int_reg = (cur_int_reg + 1) % 18;
        return cur_int_reg;
    }
    else{
        cur_float_reg = (cur_float_reg + 1) % 24;
        return cur_float_reg + 18;
    }
}

void free_reg(int type)
{
    if( type == 0 )
        cur_int_reg = (cur_int_reg - 1 + 18) % 18;
    else
        cur_float_reg = (cur_float_reg - 1 + 24) % 24;
    return;
}

int get_local_addr_register(AST_NODE *node, int *ARoffset)
{
    SymbolTableEntry *entry = node->semantic_value.identifierSemanticValue.symbolTableEntry;
    int base_offset = entry->offset;
    int target_addr_reg = get_reg(0);

    if( entry->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR ){
        fprintf(output, "\tli %s, %d\n", int_reg[target_addr_reg], -base_offset);
        fprintf(output, "\tadd %s, fp, %s\n", int_reg[target_addr_reg], int_reg[target_addr_reg]);
    }
    else{
        gen_expr(node->child, ARoffset);
        fprintf(output, "\tli %s, 4\n", int_reg[target_addr_reg]);
        fprintf(output, "\tmul %s, %s, %s\n", int_reg[target_addr_reg], int_reg[node->child->place], int_reg[target_addr_reg]);
        fprintf(output, "\tli %s, %d\n", int_reg[node->child->place], -base_offset);
        fprintf(output, "\tadd %s, %s, %s\n", int_reg[target_addr_reg], int_reg[target_addr_reg], int_reg[node->child->place]);
        
        fprintf(output, "\tadd %s, fp, %s\n", int_reg[target_addr_reg], int_reg[target_addr_reg]);
        free_reg(0);    //free node->child->place
    }
    return target_addr_reg;
}

int get_global_addr_register(AST_NODE *node, int *ARoffset){
    SymbolTableEntry *entry = node->semantic_value.identifierSemanticValue.symbolTableEntry;

    int target_addr_reg = get_reg(0);
    int base_addr_reg = get_reg(0);
    fprintf(output, "\tla %s, _g_%s\n", int_reg[base_addr_reg], node->semantic_value.identifierSemanticValue.identifierName);

    if( entry->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR ){
        fprintf(output, "\tmv %s, %s\n", int_reg[target_addr_reg], int_reg[base_addr_reg]);
        free_reg(0);    //free base_addr_reg
    }
    else{
        gen_expr(node->child, ARoffset);
        fprintf(output, "\tli %s, 4\n", int_reg[target_addr_reg]);
        fprintf(output, "\tmul %s, %s, %s\n", int_reg[target_addr_reg], int_reg[node->child->place], int_reg[target_addr_reg]);
        fprintf(output, "\tadd %s, %s, %s\n", int_reg[target_addr_reg], int_reg[target_addr_reg], int_reg[base_addr_reg]);
        free_reg(0);    //free node->child->place
        free_reg(0);    //free base_addr_reg
    }
    return target_addr_reg;
}

void gen_and_expr(AST_NODE *exprNode, int *ARoffset)
{
    int cur_and = total_and++;
    gen_expr(exprNode->child, ARoffset);
    int left_child_reg = exprNode->child->place;
    if( left_child_reg < 18 ) {
        fprintf(output, "\tbeqz %s, AND_false%d\n", int_reg[left_child_reg], cur_and);
        free_reg(0);    // free left_child_reg
    }
    else {
        int zero_float_reg = get_reg(1);
        fprintf(output, "\tfcvt.s.w %s, x0\n", float_reg[zero_float_reg-18]);
        free_reg(1);    // free zero_float_reg
        free_reg(1);    // free left_child_reg
        int tmp_reg = get_reg(0);
        fprintf(output, "\tfeq.s %s, %s, %s\n", int_reg[tmp_reg], float_reg[left_child_reg-18], float_reg[zero_float_reg-18]);
        fprintf(output, "\tseqz %s, %s\n", int_reg[tmp_reg], int_reg[tmp_reg]);
        fprintf(output, "\tbeqz %s, AND_false%d\n", int_reg[tmp_reg], cur_and);
        free_reg(0);    // free tmp_reg
    }

    gen_expr(exprNode->child->rightSibling, ARoffset);
    int right_child_reg = exprNode->child->rightSibling->place;
    if( right_child_reg < 18 ) {
        fprintf(output, "\tbeqz %s, AND_false%d\n", int_reg[right_child_reg], cur_and);
        free_reg(0);    // free right_child_reg
    }
    else {
        int zero_float_reg = get_reg(1);
        fprintf(output, "\tfcvt.s.w %s, x0\n", float_reg[zero_float_reg-18]);
        free_reg(1);    // free zero_float_reg
        free_reg(1);    // free right_child_reg
        int tmp_reg = get_reg(0);
        fprintf(output, "\tfeq.s %s, %s, %s\n", int_reg[tmp_reg], float_reg[right_child_reg-18], float_reg[zero_float_reg-18]);
        fprintf(output, "\tseqz %s, %s\n", int_reg[tmp_reg], int_reg[tmp_reg]);
        fprintf(output, "\tbeqz %s, AND_false%d\n", int_reg[tmp_reg], cur_and);
        free_reg(0);    // free tmp_reg
    }
    exprNode->place = get_reg(0);
    fprintf(output, "\taddi %s, x0, 1\n", int_reg[exprNode->place]);
    fprintf(output, "\tj AND_exit%d\n", cur_and);
    fprintf(output, "\tAND_false%d:\n", cur_and);
    fprintf(output, "\taddi %s, x0, 0\n", int_reg[exprNode->place]);
    fprintf(output, "\tAND_exit%d:\n", cur_and);
    return;
}

void gen_or_expr(AST_NODE *exprNode, int *ARoffset)
{
    int cur_or = total_or++;
    gen_expr(exprNode->child, ARoffset);
    int left_child_reg = exprNode->child->place;
    if( left_child_reg < 18 ) {
        fprintf(output, "\tbnez %s, OR_true%d\n", int_reg[left_child_reg], cur_or);
        free_reg(0);    // free left_child_reg
    }
    else {
        int zero_float_reg = get_reg(1);
        fprintf(output, "\tfcvt.s.w %s, x0\n", float_reg[zero_float_reg-18]);
        free_reg(1);    // free zero_float_reg
        free_reg(1);    // free left_child_reg
        int tmp_reg = get_reg(0);
        fprintf(output, "\tfeq.s %s, %s, %s\n", int_reg[tmp_reg], float_reg[left_child_reg-18], float_reg[zero_float_reg-18]);
        fprintf(output, "\tbeqz %s, OR_true%d\n", int_reg[tmp_reg], cur_or);
        free_reg(0);    // free tmp_reg
    }

    gen_expr(exprNode->child->rightSibling, ARoffset);
    int right_child_reg = exprNode->child->rightSibling->place;
    if( right_child_reg < 18 ) {
        fprintf(output, "\tbnez %s, OR_true%d\n", int_reg[right_child_reg], cur_or);
        free_reg(0);    // free right_child_reg
    }
    else {
        int zero_float_reg = get_reg(1);
        fprintf(output, "\tfcvt.s.w %s, x0\n", float_reg[zero_float_reg-18]);
        free_reg(1);    // free zero_float_reg
        free_reg(1);    // free right_child_reg
        int tmp_reg = get_reg(0);
        fprintf(output, "\tfeq.s %s, %s, %s\n", int_reg[tmp_reg], float_reg[right_child_reg-18], float_reg[zero_float_reg-18]);
        fprintf(output, "\tbeqz %s, OR_true%d\n", int_reg[tmp_reg], cur_or);
        free_reg(0);    // free tmp_reg
    }
    exprNode->place = get_reg(0);
    fprintf(output, "\taddi %s, x0, 0\n", int_reg[exprNode->place]);
    fprintf(output, "\tj OR_exit%d\n", cur_or);
    fprintf(output, "\tOR_true%d:\n", cur_or);
    fprintf(output, "\taddi %s, x0, 1\n", int_reg[exprNode->place]);
    fprintf(output, "\tOR_exit%d:\n", cur_or);
    return;
}

int gen_int_expr(int left_reg, int right_reg, AST_NODE *exprNode)
{
    //left_reg and right_reg are two int register, generate the code and return the int_reg index that containing the result
    if( exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION ){
        free_reg(0);
        free_reg(0);
        exprNode->place = get_reg(0);
        switch( exprNode->semantic_value.exprSemanticValue.op.binaryOp ){
            case BINARY_OP_ADD:
                fprintf(output, "\tadd %s, %s, %s\n", int_reg[exprNode->place], int_reg[left_reg], int_reg[right_reg]);
                break;
            case BINARY_OP_SUB:
                fprintf(output, "\tsub %s, %s, %s\n", int_reg[exprNode->place], int_reg[left_reg], int_reg[right_reg]);
                break;
            case BINARY_OP_MUL:
                fprintf(output, "\tmul %s, %s, %s\n", int_reg[exprNode->place], int_reg[left_reg], int_reg[right_reg]);
                break;
            case BINARY_OP_DIV:
                fprintf(output, "\tdiv %s, %s, %s\n", int_reg[exprNode->place], int_reg[left_reg], int_reg[right_reg]);
                break;
            case BINARY_OP_AND:
                // nothing
                break;
            case BINARY_OP_OR:
                // nothing
                break;
            case BINARY_OP_EQ:
                fprintf(output, "\tsub %s, %s, %s\n", int_reg[exprNode->place], int_reg[left_reg], int_reg[right_reg]);
                fprintf(output, "\tseqz %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->place]);
                break;
            case BINARY_OP_GE:
                fprintf(output, "\tslt %s, %s, %s\n", int_reg[exprNode->place], int_reg[left_reg], int_reg[right_reg]);
                fprintf(output, "\tseqz %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->place]);
                break;
            case BINARY_OP_NE:
                fprintf(output, "\tsub %s, %s, %s\n", int_reg[exprNode->place], int_reg[left_reg], int_reg[right_reg]);
                fprintf(output, "\tsnez %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->place]);
                break;
            case BINARY_OP_GT:
                fprintf(output, "\tsgt %s, %s, %s\n", int_reg[exprNode->place], int_reg[left_reg], int_reg[right_reg]);
                break;
            case BINARY_OP_LE:
                fprintf(output, "\tsgt %s, %s, %s\n", int_reg[exprNode->place], int_reg[left_reg], int_reg[right_reg]);
                fprintf(output, "\tseqz %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->place]);
                break;
            case BINARY_OP_LT:
                fprintf(output, "\tslt %s, %s, %s\n", int_reg[exprNode->place], int_reg[left_reg], int_reg[right_reg]);
                break;
            default:
                break;
        }
    }
    else{
        free_reg(0);
        exprNode->place = get_reg(0);
        switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp){
            case UNARY_OP_LOGICAL_NEGATION:
                fprintf(output, "\tseqz %s, %s\n", int_reg[exprNode->place], int_reg[left_reg]);
                break;
            case UNARY_OP_NEGATIVE:
                fprintf(output, "\tsub %s, x0, %s\n", int_reg[exprNode->place], int_reg[left_reg]);
                break;
            case UNARY_OP_POSITIVE:
                fprintf(output, "\tmv %s, %s\n", int_reg[exprNode->place], int_reg[left_reg]);
                break;
            default:
                break;
        }
    }
}

int gen_float_expr(int left_reg, int right_reg, AST_NODE *exprNode)
{
    /*left_reg and right_reg are two float register, generate the code and return int_reg(if it's arithmetic expr)
       or float_reg(if it's relation/logic expr)  */
    if( exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION ){
        free_reg(1);
        free_reg(1);
        if( isRelOp(exprNode->semantic_value.exprSemanticValue) ){
            exprNode->place = get_reg(0);
            switch( exprNode->semantic_value.exprSemanticValue.op.binaryOp ){
                case BINARY_OP_AND:
                    // nothing
                    break;
                case BINARY_OP_OR:
                    // nothing
                    break;
                case BINARY_OP_EQ:
                    fprintf(output, "\tfeq.s %s, %s, %s\n", int_reg[exprNode->place], float_reg[left_reg-18], float_reg[right_reg-18]);
                    break;
                case BINARY_OP_GE:
                    fprintf(output, "\tfge.s %s, %s, %s\n", int_reg[exprNode->place], float_reg[left_reg-18], float_reg[right_reg-18]);
                    break;
                case BINARY_OP_NE:
                    fprintf(output, "\tfeq.s %s, %s, %s\n", int_reg[exprNode->place], float_reg[left_reg-18], float_reg[right_reg-18]);
                    fprintf(output, "\tseqz %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->place]);
                    break;
                case BINARY_OP_GT:
                    fprintf(output, "\tfgt.s %s, %s, %s\n", int_reg[exprNode->place], float_reg[left_reg-18], float_reg[right_reg-18]);
                    break;
                case BINARY_OP_LE:
                    fprintf(output, "\tfle.s %s, %s, %s\n", int_reg[exprNode->place], float_reg[left_reg-18], float_reg[right_reg-18]);
                    break;
                case BINARY_OP_LT:
                    fprintf(output, "\tflt.s %s, %s, %s\n", int_reg[exprNode->place], float_reg[left_reg-18], float_reg[right_reg-18]);
                    break;
                default:
                    break;
            }
        }
        else{
            exprNode->place = get_reg(1);
            switch( exprNode->semantic_value.exprSemanticValue.op.binaryOp ){
                case BINARY_OP_ADD:
                    fprintf(output, "\tfadd.s %s, %s, %s\n", float_reg[exprNode->place-18], float_reg[left_reg-18], float_reg[right_reg-18]);
                    break;
                case BINARY_OP_SUB:
                    fprintf(output, "\tfsub.s %s, %s, %s\n", float_reg[exprNode->place-18], float_reg[left_reg-18], float_reg[right_reg-18]);
                    break;
                case BINARY_OP_MUL:
                    fprintf(output, "\tfmul.s %s, %s, %s\n", float_reg[exprNode->place-18], float_reg[left_reg-18], float_reg[right_reg-18]);
                    break;
                case BINARY_OP_DIV:
                    fprintf(output, "\tfdiv.s %s, %s, %s\n", float_reg[exprNode->place-18], float_reg[left_reg-18], float_reg[right_reg-18]);
                    break;
            }
        }
    }
    else{
        int zero_float_reg = get_reg(1);
        if( isRelOp(exprNode->semantic_value.exprSemanticValue) ){
            switch( exprNode->semantic_value.exprSemanticValue.op.unaryOp ){
                case UNARY_OP_LOGICAL_NEGATION:
                    fprintf(output, "\tfcvt.s.w %s, x0\n", float_reg[zero_float_reg-18]);
                    free_reg(1);    // free zero_reg
                    free_reg(1);    // free left_reg
                    exprNode->place = get_reg(0);
                    fprintf(output, "\tfeq.s %s, %s, %s\n", int_reg[exprNode->place], float_reg[left_reg-18], float_reg[zero_float_reg-18]);
                    break;
            }
        }
        else{
            switch( exprNode->semantic_value.exprSemanticValue.op.unaryOp ){
                case UNARY_OP_NEGATIVE:
                    free_reg(1);    // free zero_reg
                    free_reg(1);    // free left_reg
                    exprNode->place = get_reg(1);
                    fprintf(output, "\tfneg.s %s, %s\n", float_reg[(exprNode->place)-18], float_reg[left_reg-18]);
                    break;
                case UNARY_OP_POSITIVE:
                    fprintf(output, "\tfcvt.s.w %s, x0\n", float_reg[zero_float_reg-18]);
                    free_reg(1);    // free zero_reg
                    free_reg(1);    // free left_reg
                    exprNode->place = get_reg(1);
                    fprintf(output, "\tadd.s %s, %s, %s\n", float_reg[(exprNode->place)-18], float_reg[zero_float_reg-18], float_reg[left_reg-18]);
                    break;
                default:
                    break;
            }
        }
    }
}

void gen_expr(AST_NODE *exprNode, int *ARoffset)
{
    int reg;
    int offset;
    SymbolTableEntry *entry;

    switch ( exprNode->nodeType ){
        case CONST_VALUE_NODE:
            if( exprNode->dataType == INT_TYPE ){
                reg = get_reg(0);
                fprintf(output, "\tli %s, %d\n", int_reg[reg], exprNode->semantic_value.const1->const_u.intval);
            }
            else{
                reg = get_reg(1);
                fprintf(output, "\t.data\n");
                fprintf(output, "\t_CONSTANT_%d: .word %u\n", cur_const, get_float_bit(exprNode->semantic_value.const1->const_u.fval));
                fprintf(output, "\t.align 3\n");
                fprintf(output, "\t.text\n");

                int reg_float_addr = get_reg(0);
                fprintf(output, "\tla %s, _CONSTANT_%d\n", int_reg[reg_float_addr], cur_const);
                fprintf(output, "\tflw %s, 0(%s)\n", float_reg[reg-18], int_reg[reg_float_addr]);
                cur_const++;
                free_reg(0);
            }
            exprNode->place = reg;
            break;
        case IDENTIFIER_NODE:
            entry = exprNode->semantic_value.identifierSemanticValue.symbolTableEntry;
            if( exprNode->dataType == INT_TYPE ){
                reg = get_reg(0);
                if( entry->nestingLevel == 0 ){
                    int target_addr_reg = get_global_addr_register(exprNode, ARoffset);
                    fprintf(output, "\tlw %s, 0(%s)\n", int_reg[reg], int_reg[target_addr_reg]);
                    free_reg(0);    //free target_addr_reg
                }
                else{
                    int target_addr_reg = get_local_addr_register(exprNode, ARoffset);
                    fprintf(output, "\tlw %s, 0(%s)\n", int_reg[reg], int_reg[target_addr_reg]);
                    free_reg(0);    //free target_addr_reg
                }
            }
            else{
                reg = get_reg(1);
                if( entry->nestingLevel == 0 ){
                    int target_addr_reg = get_global_addr_register(exprNode, ARoffset);
                    fprintf(output, "\tflw %s, 0(%s)\n", float_reg[reg-18], int_reg[target_addr_reg]);
                    free_reg(0);    //free target_addr_reg
                }
                else{
                    int target_addr_reg = get_local_addr_register(exprNode, ARoffset);
                    fprintf(output, "\tflw %s, 0(%s)\n", float_reg[reg-18], int_reg[target_addr_reg]);
                    free_reg(0);    //free target_addr_reg
                }
            }
            exprNode->place = reg;
            break;
        case EXPR_NODE:
            if( exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION ){
                if(exprNode->semantic_value.exprSemanticValue.op.binaryOp == BINARY_OP_AND) {
                    gen_and_expr(exprNode, ARoffset);
                    break;
                }
                else if(exprNode->semantic_value.exprSemanticValue.op.binaryOp == BINARY_OP_OR) {
                    gen_or_expr(exprNode, ARoffset);
                    break;
                }
                gen_expr(exprNode->child, ARoffset);
                gen_expr(exprNode->child->rightSibling, ARoffset);
                if( exprNode->child->place < 18 && exprNode->child->rightSibling->place < 18 ){
                    gen_int_expr(exprNode->child->place, exprNode->child->rightSibling->place, exprNode);
                }
                else{
                    int convert_reg;
                    if( exprNode->child->place < 18 ){
                        convert_reg = get_reg(1);
                        fprintf(output, "\tfcvt.s.w %s, %s\n", float_reg[convert_reg-18], int_reg[exprNode->child->place]);
                        free_reg(0);
                        exprNode->child->place = convert_reg;
                    }
                    else if( exprNode->child->rightSibling->place < 18 ){
                        convert_reg = get_reg(1);
                        fprintf(output, "\tfcvt.s.w %s, %s\n", float_reg[convert_reg-18], int_reg[exprNode->child->rightSibling->place]);
                        free_reg(0);
                        exprNode->child->rightSibling->place = convert_reg;   
                    }
                    gen_float_expr(exprNode->child->place, exprNode->child->rightSibling->place, exprNode);
                }
            }
            else{
                gen_expr(exprNode->child, ARoffset);
                if( exprNode->child->place < 18 ){
                    gen_int_expr(exprNode->child->place, -1, exprNode);
                }
                else{
                    gen_float_expr(exprNode->child->place, -1, exprNode);
                }
            }
            break;
        case STMT_NODE:
            if( exprNode->semantic_value.exprSemanticValue.kind == ASSIGN_STMT ){
                gen_assign(exprNode->child, exprNode->child->rightSibling, ARoffset);
                if( exprNode->child->dataType == INT_TYPE ){
                    exprNode->place = get_reg(0);
                    fprintf(output, "\tmv %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->child->place]);
                }
                else{
                    exprNode->place = get_reg(1);
                    fprintf(output, "\tfmv.s %s, %s\n", float_reg[exprNode->place-18], float_reg[exprNode->child->place-18]);
                }
            }
            else{
                gen_function_call(exprNode, ARoffset);
            }
            break;
    }
}

void gen_while(AST_NODE *stmtNode, int *ARoffset, char *funcName)
{
    int cur_while = total_while++;
    fprintf(output, "WHILE_test%d:\n", cur_while);
    gen_expr(stmtNode->child, ARoffset);
    if( stmtNode->child->dataType == INT_TYPE ){
        fprintf(output, "\tbeqz  %s, WHILE_exit%d\n", int_reg[stmtNode->child->place], cur_while);
        free_reg(0);
    }
    else{
        int reg_conv = get_reg(0);
        fprintf(output, "\tfcvt.w.s %s, %s\n", int_reg[reg_conv], float_reg[stmtNode->child->place-18]);
        fprintf(output, "\tbeqz %s, WHILE_exit%d\n", int_reg[reg_conv], cur_while);
        free_reg(0);
        free_reg(1);
    }
    gen_stmt(stmtNode->child->rightSibling, ARoffset, funcName);
    fprintf(output, "\tj WHILE_test%d\n", cur_while);
    fprintf(output, "WHILE_exit%d:\n", cur_while);
    return;
}

void gen_for(AST_NODE *stmtNode, int *ARoffset, char *funcName)
{

}

void gen_if(AST_NODE *stmtNode, int *ARoffset, char *funcName)
{
    int cur_if = total_if++;
    gen_expr(stmtNode->child, ARoffset);
    if( stmtNode->child->dataType == INT_TYPE ){
        fprintf(output, "\tbeqz  %s, IF_else%d\n", int_reg[stmtNode->child->place], cur_if);
        free_reg(0);
    }
    else{
        int reg_conv = get_reg(0);
        fprintf(output, "\tfcvt.w.s %s, %s\n", int_reg[reg_conv], float_reg[stmtNode->child->place-18]);
        fprintf(output, "\tbeqz %s, IF_else%d\n", int_reg[reg_conv], cur_if);
        free_reg(0);
        free_reg(1);
    }
    gen_stmt(stmtNode->child->rightSibling, ARoffset, funcName);
    fprintf(output, "\tj IF_exit%d\n", cur_if);
    fprintf(output, "IF_else%d:\n", cur_if);
    gen_stmt(stmtNode->child->rightSibling->rightSibling, ARoffset, funcName);
    fprintf(output, "IF_exit%d:\n", cur_if);
    return;
}

void gen_write_call(AST_NODE *writefunc, int *ARoffset)
{
    if( writefunc->child->rightSibling->child->dataType == CONST_STRING_TYPE ){
        fprintf(output, "\t.data\n");
        fprintf(output, "_CONSTANT_%d: .string %s\n", cur_const, writefunc->child->rightSibling->child->semantic_value.const1->const_u.sc);
        fprintf(output, "\t.align 3\n");
        fprintf(output, "\t.text\n");
        fprintf(output, "\tla   a0, _CONSTANT_%d\n", cur_const);
        cur_const++;
        fprintf(output, "\tcall _write_str\n");
    }
    else{
        AST_NODE *paramNode = writefunc->child->rightSibling->child;
        gen_expr(paramNode, ARoffset);
        if( paramNode->place < 18 ){
            fprintf(output, "\tmv a0, %s\n", int_reg[paramNode->place]);
            fprintf(output, "\tjal _write_int\n");
            free_reg(0);
        }
        else{
            fprintf(output, "\tfmv.s fa0, %s\n", float_reg[paramNode->place-18]);
            fprintf(output, "\tjal _write_float\n");
            free_reg(1);
        }
    }
}

void gen_read_call(AST_NODE *readfunc)
{
    fprintf(output, "\tcall _read_int\n");
    readfunc->place = get_reg(0);
    fprintf(output, "\tmv %s, a0\n", int_reg[readfunc->place]);
    return;
}

void gen_fread_call(AST_NODE *freadfunc)
{
    fprintf(output, "\tcall _read_float\n");
    freadfunc->place = get_reg(1);
    fprintf(output, "\tfmv.s %s, fa0\n", float_reg[freadfunc->place-18]);
    return;
}

void save_registers()
{
    for(int i = 0; i <= 6; ++i) {
        fprintf(output, "\tsw   t%d, %d(sp)\n", i, -(i + 1) * 8);
    }
    for(int i = 0; i <= 7; ++i) {
        fprintf(output, "\tsw   a%d, %d(sp)\n", i, -i * 8 - 64);
    }
    for(int i = 0; i <= 11; ++i) {
        fprintf(output, "\tfsw   ft%d, %d(sp)\n", i, -i * 8 - 128);
    }
    for(int i = 0; i <= 7; ++i) {
        fprintf(output, "\tfsw  fa%d, %d(sp)\n", i, -i * 8 - 224);
    }

    fprintf(output, "\taddi sp, sp, -280\n");
    return;
}

void load_registers(int exclude_reg)        //ignore the exclude_reg storing return value
{
    fprintf(output, "\taddi sp, sp, 280\n");
    for(int i = 0; i <= 6; ++i) {
        if( exclude_reg != i && exclude_reg - 11 != i )
            fprintf(output, "\tlw   t%d, %d(sp)\n", i, -(i + 1) * 8);
    }
    for(int i = 0; i <= 7; ++i) {
        fprintf(output, "\tlw   a%d, %d(sp)\n", i, -i * 8 - 64);
    }
    for(int i = 0; i <= 11; ++i) {
        if( exclude_reg - 18 != i && exclude_reg - 30 != i )
        fprintf(output, "\tflw   ft%d, %d(sp)\n", i, -i * 8 - 128);
    }
    for(int i = 0; i <= 7; ++i) {
        fprintf(output, "\tflw  fa%d, %d(sp)\n", i, -i * 8 - 224);
    }
    return;
}

void gen_function_call(AST_NODE *stmtNode, int *ARoffset)
{
    if( strcmp(stmtNode->child->semantic_value.identifierSemanticValue.identifierName, "write") == 0 ){
        gen_write_call(stmtNode, ARoffset);
    }
    else if( strcmp(stmtNode->child->semantic_value.identifierSemanticValue.identifierName, "read") == 0 ){
        gen_read_call(stmtNode);
    }
    else if( strcmp(stmtNode->child->semantic_value.identifierSemanticValue.identifierName, "fread") == 0 ){
        gen_fread_call(stmtNode);
    }
    else{
        save_registers();
        fprintf(output, "\tcall _start_%s\n", stmtNode->child->semantic_value.identifierSemanticValue.identifierName);
        if( stmtNode->dataType == INT_TYPE ){
            stmtNode->place = get_reg(0);
            fprintf(output, "\tmv %s, a0\n", int_reg[stmtNode->place]);
        }
        else if( stmtNode->dataType == FLOAT_TYPE ){
            stmtNode->place = get_reg(1);
            fprintf(output, "\tfmv.s %s, fa0\n", float_reg[(stmtNode->place)-18]);
        }
        load_registers(stmtNode->place);
    }
}

void gen_assign(AST_NODE *left, AST_NODE *right, int *ARoffset)
{
    gen_expr(right, ARoffset);
    SymbolTableEntry *leftentry = left->semantic_value.identifierSemanticValue.symbolTableEntry;
    if( leftentry->nestingLevel == 0 ){     //global variable assignment
        int left_addr_reg = get_global_addr_register(left, ARoffset);
        
        if( left->dataType == INT_TYPE ){
            if( right->place < 18 ){                //e.g. int a = 1;
                fprintf(output, "\tsw %s, 0(%s)\n", int_reg[right->place], int_reg[left_addr_reg]);
                left->place = right->place;
                free_reg(0);    //free left_addr_reg
                free_reg(0);    //free right expr
            }
            else{                                   //e.g. int a = 1.0;
                int reg_convert = get_reg(0);
                fprintf(output, "\tfcvt.w.s %s, %s, rtz\n", int_reg[reg_convert], float_reg[(right->place)-18]);
                fprintf(output, "\tsw %s, 0(%s)\n", int_reg[reg_convert], int_reg[left_addr_reg]);
                left->place = reg_convert;
                free_reg(0);    //free convert
                free_reg(0);    //free left_addr_reg
                free_reg(1);    //free right expr
            }
        }
        else{   //left->dataType == FLOAT_TYPE
            if( right->place < 18 ){                //e.g. float a = 1;
                int reg_convert = get_reg(1);
                fprintf(output, "\tfcvt.s.w %s, %s\n", float_reg[reg_convert-18], int_reg[right->place]);
                fprintf(output, "\tfsw %s, 0(%s)\n", float_reg[reg_convert-18], int_reg[left_addr_reg]);
                left->place = reg_convert;
                free_reg(1);    //free convert
                free_reg(0);    //free left addr
                free_reg(0);    //free right expr
            }
            else{                                   //e.g. float a = 1.0;
                fprintf(output, "\tfsw %s, 0(%s)\n", float_reg[(right->place)-18], int_reg[left_addr_reg]);
                left->place = right->place;
                free_reg(0);    //free left addr
                free_reg(1);    //free right expr
            }
        }
    }
    else{   //local variable assignment
        int left_addr_reg = get_local_addr_register(left, ARoffset);
        if( left->dataType == INT_TYPE ){
            if( right->place < 18 ){                //e.g. int a = 1;
                fprintf(output, "\tsw %s, 0(%s)\n", int_reg[right->place], int_reg[left_addr_reg]);
                left->place = right->place;
                free_reg(0);        //free left addr
                free_reg(0);        //free right int expr
            }
            else{                                   //e.g. int a = 1.0;
                int reg_convert = get_reg(0);
                fprintf(output, "\tfcvt.w.s %s, %s, rtz\n", int_reg[reg_convert], float_reg[(right->place)-18]);
                fprintf(output, "\tsw %s, 0(%s)\n", int_reg[reg_convert], int_reg[left_addr_reg]);
                left->place = reg_convert;
                free_reg(0);        //free right int expr
                free_reg(0);        //free left addr
                free_reg(1);        //free right float expr
            }
        }
        else{
            if( right->place < 18 ){                //e.g. float a = 1;
                int reg_convert = get_reg(1);
                fprintf(output, "\tfcvt.s.w %s, %s\n", float_reg[reg_convert-18], int_reg[right->place]);
                fprintf(output, "\tfsw %s, 0(%s)\n", float_reg[reg_convert-18], int_reg[left_addr_reg]);
                left->place = reg_convert;
                free_reg(1);        //free right float expr
                free_reg(0);        //free left addr
                free_reg(0);        //free right int expr
            }
            else{                                   //e.g. float a = 1.0;
                fprintf(output, "\tfsw %s, 0(%s)\n", float_reg[(right->place)-18], int_reg[left_addr_reg]);
                left->place = right->place;
                free_reg(0);        //free left addr
                free_reg(1);        //free right float expr
            }
        }
    }
}

void gen_return(AST_NODE *returnNode, int *ARoffset, char *funcName)
{
    if( returnNode->dataType == INT_TYPE ){
        gen_expr(returnNode->child, ARoffset);
        if( returnNode->child->dataType == INT_TYPE ){
            fprintf(output, "\tmv a0, %s\n", int_reg[returnNode->child->place]);
            free_reg(0);
        }
        else{
            fprintf(output, "\tfcvt.w.s a0, %s\n", float_reg[(returnNode->child->place)-18]);
            free_reg(1);
        }
    }
    else if( returnNode->dataType == FLOAT_TYPE ){
        gen_expr(returnNode->child, ARoffset);
        if( returnNode->child->dataType == INT_TYPE ){
            fprintf(output, "\tfcvt.s.w fa0, %s\n", int_reg[returnNode->child->place]);
            free_reg(0);
        }
        else{
            fprintf(output, "\tfmv.s fa0, %s\n", float_reg[(returnNode->child->place)-18]);
            free_reg(1);
        }
    }
    fprintf(output, "\tj _end_%s\n", funcName);
    return;
}

void gen_stmt(AST_NODE *stmtNode, int *ARoffset, char *funcName)
{
    if( stmtNode->nodeType == BLOCK_NODE ){
        gen_block(stmtNode, ARoffset, funcName);
    }
    else if (stmtNode->nodeType != NUL_NODE){
        switch(stmtNode->semantic_value.stmtSemanticValue.kind){
            case WHILE_STMT:
                gen_while(stmtNode, ARoffset, funcName);
                break;
            case FOR_STMT:
                gen_for(stmtNode, ARoffset, funcName);
                break;
            case IF_STMT:
                gen_if(stmtNode, ARoffset, funcName);
                break;
            case FUNCTION_CALL_STMT:
                gen_function_call(stmtNode, ARoffset);
                if( stmtNode->dataType == INT_TYPE )
                    free_reg(0);
                else if( stmtNode->dataType == FLOAT_TYPE )
                    free_reg(1);
                break;
            case ASSIGN_STMT:
                gen_assign(stmtNode->child,stmtNode->child->rightSibling, ARoffset);
                break;
            case RETURN_STMT:
                gen_return(stmtNode, ARoffset, funcName);
                break;
        }
    }
    return;
}

void gen_block(AST_NODE *blockNode, int *ARoffset, char *funcName)
{
    if( blockNode->child == NULL )
        return;
    gen_localVar(blockNode, ARoffset);

    AST_NODE *stmtListNode;
    if( blockNode->child->nodeType == STMT_LIST_NODE ){
        stmtListNode = blockNode->child;
    }
    else if( blockNode->child->rightSibling != NULL && blockNode->child->rightSibling->nodeType == STMT_LIST_NODE ){
        stmtListNode = blockNode->child->rightSibling;
    }
    else{
        return;
    }
    AST_NODE *stmtNode = stmtListNode->child;
    while( stmtNode != NULL ){
        gen_stmt(stmtNode, ARoffset, funcName);
        stmtNode = stmtNode->rightSibling;
    }
    return;
}

void gen_head(char *funcName)
{
    fprintf(output, ".text\n");
    fprintf(output, "_start_%s:\n", funcName);
    return;
}

void gen_localVar(AST_NODE *blockNode, int *ARoffset)
{
    if( blockNode->child->nodeType == VARIABLE_DECL_LIST_NODE ){
        AST_NODE *var_decl_node = blockNode->child->child;
        while( var_decl_node != NULL ){
            if( var_decl_node->semantic_value.declSemanticValue.kind == VARIABLE_DECL ){
                AST_NODE *variable = var_decl_node->child->rightSibling;
                while( variable != NULL ){
                    //compute space in AR
                    SymbolTableEntry *entry = variable->semantic_value.identifierSemanticValue.symbolTableEntry;
                    int unit = 1;
                    if( entry->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR ){
                        for( int i = 0; i < entry->attribute->attr.typeDescriptor->properties.arrayProperties.dimension; i++ ){
                            unit *= entry->attribute->attr.typeDescriptor->properties.arrayProperties.sizeInEachDimension[i];
                        }
                    }
                    (*ARoffset) += (4 * unit);
                    entry->offset = (*ARoffset);
                    
                    //initialize if a variable has init value
                    if( variable->semantic_value.identifierSemanticValue.kind == WITH_INIT_ID ){
                        gen_assign(variable, variable->child, ARoffset);
                    }
                    variable = variable->rightSibling;
                }
                var_decl_node = var_decl_node->rightSibling;
            }
        }
    }
    return;
}

void gen_prologue(char *funcName)
{
    fprintf(output, "\tsd   ra, 0(sp)\n");
    fprintf(output, "\tsd   fp, -8(sp)\n");
    fprintf(output, "\taddi fp, sp, -8\n");
    fprintf(output, "\taddi sp, sp, -16\n");        // 預留 ra 跟 old fp 的位置
    fprintf(output, "\tla   ra, _frameSize_%s\n", funcName);
    fprintf(output, "\tlw   ra, 0(ra)\n");
    fprintf(output, "\tsub  sp, sp, ra\n");

    for(int i = 1; i <= 11; ++i) {      //save callee-saved reg
        fprintf(output, "\tsd   s%d, %d(sp)\n", i, (i-1) * 8);
    }
    for(int i = 0; i <= 11; ++i) {      //save callee-saved reg
        fprintf(output, "\tfsw   fs%d, %d(sp)\n", i, 88 + i * 4);
    }
    return;
}

void gen_epilogue(char *funcName, int size)
{
    fprintf(output, "_end_%s:\n", funcName);
    for(int i = 1; i <= 11; ++i) {      //restore callee-saved reg
        fprintf(output, "\tld   s%d, %d(sp)\n", i, (i-1) * 8);
    }
    for(int i = 0; i <= 11; ++i) {      //save callee-saved reg
        fprintf(output, "\tflw   fs%d, %d(sp)\n", i, 88 + i * 4);
    }
    fprintf(output, "\tld   ra, 8(fp)\n");
    fprintf(output, "\taddi sp, fp, 8\n");
    fprintf(output, "\tld   fp, 0(fp)\n");
    fprintf(output, "\tjr   ra\n");
    fprintf(output, ".data\n");
    fprintf(output, "\t_frameSize_%s: .word %d\n", funcName, size);
    return;
}

void gen_funcDecl(AST_NODE* funcDeclNode)
{
    int ARoffset = 0;
    char *functionName = funcDeclNode->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
    AST_NODE *blockNode = funcDeclNode->child->rightSibling->rightSibling->rightSibling;
    gen_head(functionName);
    gen_prologue(functionName);
    gen_block(blockNode, &ARoffset, functionName);
    gen_epilogue(functionName, ARoffset + 136);
    return;
}

void gen_global_var(AST_NODE* var_decl_list_node)
{
    fprintf(output, ".data\n");
    AST_NODE *var_decl_node = var_decl_list_node->child;
    while( var_decl_node != NULL ){

        if( var_decl_node->semantic_value.declSemanticValue.kind == VARIABLE_DECL ) {
            AST_NODE *variable = var_decl_node->child->rightSibling;
            while( variable != NULL ){
                SymbolTableEntry *entry = variable->semantic_value.identifierSemanticValue.symbolTableEntry;
                int unit = 1;
                if( entry->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR ){
                    for( int i = 0; i < entry->attribute->attr.typeDescriptor->properties.arrayProperties.dimension; i++ ){
                        unit *= entry->attribute->attr.typeDescriptor->properties.arrayProperties.sizeInEachDimension[i];
                    }
                }
                
                if( variable->semantic_value.identifierSemanticValue.kind == WITH_INIT_ID ){    
                    fprintf(output, "_g_%s:\n", variable->semantic_value.identifierSemanticValue.identifierName);
                    if( variable->dataType == INT_TYPE && variable->child->dataType == INT_TYPE )
                        fprintf(output, "\t.word %d\n", variable->child->semantic_value.const1->const_u.intval);
                    else if( variable->dataType == INT_TYPE && variable->child->dataType == FLOAT_TYPE )
                        fprintf(output, "\t.word %d\n", (int) variable->child->semantic_value.const1->const_u.fval);
                    else if( variable->dataType == FLOAT_TYPE && variable->child->dataType == INT_TYPE )
                        fprintf(output, "\t.word %u\n", get_float_bit((float) variable->child->semantic_value.const1->const_u.intval));
                    else
                        fprintf(output, "\t.word %u\n", get_float_bit(variable->child->semantic_value.const1->const_u.fval));
                }
                else{
                    fprintf(output, "_g_%s: .space %d\n", variable->semantic_value.identifierSemanticValue.identifierName, unit * 4);
                }

                variable = variable->rightSibling;
            }
        }
        var_decl_node = var_decl_node->rightSibling;
    }
    return;
}

void gen_code(AST_NODE *root)
{
    output = fopen("output.s", "w");
    AST_NODE *child = root->child;
    while( child != NULL ){
        if( child->nodeType == VARIABLE_DECL_LIST_NODE ){    //for global decl or typedef
            gen_global_var(child);
        }
        else{   //for function declaration
            gen_funcDecl(child);
        }
        child = child->rightSibling;
    }
    fclose(output);
    return;
}