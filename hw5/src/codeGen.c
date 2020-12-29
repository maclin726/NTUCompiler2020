#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbolTable.h"

void gen_function_call(AST_NODE *stmtNode, int *ARoffset);
void gen_block(AST_NODE *blockNode, int *ARoffset);
void gen_localVar(AST_NODE *blockNode, int *ARoffset);

FILE *output;

const char int_reg[18][4] = {"x5", "x6", "x7", "x9", "x18", "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};
const char float_reg[24][4] = {"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f18", "f19", "f20", "f21", "f22", "f23", "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"};

int cur_int_reg = 0, cur_float_reg = 0;
int cur_const = 0;
int total_if = 0, total_for = 0, total_while = 0;

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

void gen_expr(AST_NODE *exprNode, int *ARoffset)
{
    int reg;
    int offset;
    SymbolTableEntry *entry;

    switch ( exprNode->nodeType ){
        case CONST_VALUE_NODE:
            if( exprNode->dataType == INT_TYPE ){
                reg = get_reg(0);
                fprintf(output, "\taddi %s, x0, %d\n", int_reg[reg], exprNode->semantic_value.const1->const_u.intval);
            }
            else{
                reg = get_reg(1);
                // fprintf(output, "\tli %s, %u\n", float_reg[reg-18], get_float_bit(exprNode->semantic_value.const1->const_u.fval));
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
                    fprintf(output, "\tla %s, _g_%s\n", int_reg[reg], entry->name);
                    fprintf(output, "\tlw %s, 0(%s)\n", int_reg[reg], int_reg[reg]);
                }
                else
                    fprintf(output, "\tlw %s, -%d(fp)\n", int_reg[reg], entry->offset);
            }
            else{
                reg = get_reg(1);
                if( entry->nestingLevel == 0 ){
                    int reg_float_addr = get_reg(0);
                    fprintf(output, "\tla %s, _g_%s\n", int_reg[reg_float_addr], entry->name);
                    fprintf(output, "\tflw %s, 0(%s)\n", float_reg[reg-18], int_reg[reg_float_addr]);
                    free_reg(0);
                }
                else{
                    fprintf(output, "\tflw %s, -%d(fp)\n", float_reg[reg-18], entry->offset);
                }
            }
            exprNode->place = reg;
            break;
        case EXPR_NODE:
            if( exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION ){
                gen_expr(exprNode->child, ARoffset);
                gen_expr(exprNode->child->rightSibling, ARoffset);
                if( exprNode->child->place < 18 && exprNode->child->place < 18 ){
                    free_reg(0);
                    free_reg(0);
                    exprNode->place = get_reg(0);
                    switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp){
                        case BINARY_OP_ADD:
                            fprintf(output, "\tadd %s, %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->child->place], int_reg[exprNode->child->rightSibling->place]);
                            break;
                        case BINARY_OP_SUB:
                            fprintf(output, "\tsub %s, %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->child->place], int_reg[exprNode->child->rightSibling->place]);
                            break;
                        case BINARY_OP_MUL:
                            fprintf(output, "\tmul %s, %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->child->place], int_reg[exprNode->child->rightSibling->place]);
                            break;
                        case BINARY_OP_DIV:
                            fprintf(output, "\tdiv %s, %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->child->place], int_reg[exprNode->child->rightSibling->place]);
                            break;
                        // case BINARY_OP_AND:
                        //     fprintf(output, "\tand %s, %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->child->place], int_reg[exprNode->child->rightSibling->place]);
                        //     break;
                        // case BINARY_OP_OR:
                        //     fprintf(output, "\tor %s, %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->child->place], int_reg[exprNode->child->rightSibling->place]);
                        //     break;
                        // case BINARY_OP_EQ:
                        //     fprintf(output, "\t %s, %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->child->place], int_reg[exprNode->child->rightSibling->place]);
                        //     break;
                        // case BINARY_OP_ADD:
                        //     fprintf(output, "\tadd %s, %s, %s\n", int_reg[exprNode->place], int_reg[exprNode->child->place], int_reg[exprNode->child->rightSibling->place]);
                        //     break;

                    }
                }
                else{

                }
            }
            else{
                gen_expr(exprNode->child, ARoffset);
            }
            break;
        case STMT_NODE:
            gen_function_call(exprNode, ARoffset);
            break;
    }
}

void gen_while(AST_NODE *stmtNode, int *ARoffset)
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
    gen_block(stmtNode->child->rightSibling, ARoffset);
    fprintf(output, "\tj WHILE_test%d\n", cur_while);
    fprintf(output, "WHILE_exit%d:\n", cur_while);
    return;
}

void gen_for(AST_NODE *stmtNode, int *ARoffset)
{

}

void gen_if(AST_NODE *stmtNode, int *ARoffset)
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
    gen_block(stmtNode->child->rightSibling, ARoffset);
    fprintf(output, "\tj IF_exit%d\n", cur_if);
    fprintf(output, "IF_else%d:\n", cur_if);
    gen_block(stmtNode->child->rightSibling->rightSibling, ARoffset);
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
        
    }
}

void gen_assign(AST_NODE *left, AST_NODE *right, int *ARoffset)
{
    gen_expr(right, ARoffset);
    SymbolTableEntry *leftentry = left->semantic_value.identifierSemanticValue.symbolTableEntry;
    if( leftentry->nestingLevel == 0 ){
    }
    else{   //local variable assignment
        if( left->dataType == INT_TYPE ){
            if( right->place < 18 ){                //e.g. int a = 1;
                fprintf(output, "\tsw %s, -%d(fp)\n", int_reg[right->place], leftentry->offset);
                free_reg(0);
            }
            else{                                   //e.g. int a = 1.0;
                int reg_convert = get_reg(0);
                fprintf(output, "\tfcvt.w.s %s, %s, rtz\n", int_reg[reg_convert], float_reg[(right->place)-18]);
                fprintf(output, "\tsw %s, -%d(fp)\n", int_reg[reg_convert], leftentry->offset);
                free_reg(0);
                free_reg(1);
            }
        }
        else{
            if( right->place < 18 ){                //e.g. float a = 1;
                int reg_convert = get_reg(1);
                fprintf(output, "\tfcvt.s.w %s, %s\n", float_reg[reg_convert-18], int_reg[right->place]);
                fprintf(output, "\tfsw %s, -%d(fp)\n", float_reg[reg_convert-18], leftentry->offset);
                free_reg(0);
                free_reg(1);
            }
            else{                                   //e.g. float a = 1.0;
                fprintf(output, "\tfsw %s, -%d(fp)\n", float_reg[(right->place)-18], leftentry->offset);
                free_reg(1);
            }
        }
    }
}

void gen_return(AST_NODE *stmtNode)
{

}

void gen_block(AST_NODE *blockNode, int *ARoffset)
{
    if( blockNode->child == NULL )
        return;
    gen_localVar(blockNode, ARoffset);

    AST_NODE *stmtListNode;
    if( blockNode->child->nodeType == STMT_LIST_NODE ){
        stmtListNode = blockNode->child;
    }
    else if( blockNode->child->rightSibling->nodeType == STMT_LIST_NODE ){
        stmtListNode = blockNode->child->rightSibling;
    }
    else{
        return;
    }
    AST_NODE *stmtNode = stmtListNode->child;
    while( stmtNode != NULL ){
        if( stmtNode->nodeType == BLOCK_NODE ){
            gen_block(stmtNode, ARoffset);
        }
        else{
            switch(stmtNode->semantic_value.stmtSemanticValue.kind){
                case WHILE_STMT:
                    gen_while(stmtNode, ARoffset);
                    break;
                case FOR_STMT:
                    gen_for(stmtNode, ARoffset);
                    break;
                case IF_STMT:
                    gen_if(stmtNode, ARoffset);
                    break;
                case FUNCTION_CALL_STMT:
                    gen_function_call(stmtNode, ARoffset);
                    break;
                case ASSIGN_STMT:
                    gen_assign(stmtNode->child,stmtNode->child->rightSibling, ARoffset);
                    break;
                case RETURN_STMT:
                    gen_return(stmtNode);
                    break;
            }
        }
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
    fprintf(output, "\t_frameSize_%s: .word %d", funcName, size);
    return;
}

void gen_funcDecl(AST_NODE* funcDeclNode)
{
    int ARoffset = 0;
    char *functionName = funcDeclNode->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
    AST_NODE *blockNode = funcDeclNode->child->rightSibling->rightSibling->rightSibling;
    gen_head(functionName);
    gen_prologue(functionName);
    gen_block(blockNode, &ARoffset);
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
            var_decl_node = var_decl_node->rightSibling;
        }
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