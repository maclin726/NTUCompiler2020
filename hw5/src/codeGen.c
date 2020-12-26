#include <stdio.h>
#include <stdlib.h>
#include "symbolTable.h"

FILE *output;

const char int_reg[18][4] = {"x5", "x6", "x7", "x9", "x18", "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};
const char float_reg[24][4] = {"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f18", "f19", "f20", "f21", "f22", "f23", "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"};

int cur_int_reg = 0, cur_float_reg = 0;
int get_reg(int type)   //0 for int_reg, 1 for float_reg
{
    if( type == 0 ){
        cur_int_reg = (cur_int_reg + 1) % 18;
        return cur_int_reg;
    }
    else{
        cur_float_reg = (cur_int_reg + 1) % 18;
        return cur_float_reg;
    }
}

void gen_while(AST_NODE *stmtNode, int *ARoffset)
{

}

void gen_for(AST_NODE *stmtNode, int *ARoffset)
{

}

void gen_if(AST_NODE *stmtNode, int *ARoffset)
{

}

void gen_assign(AST_NODE *stmtNode)
{

}

void gen_function_call(AST_NODE *stmtNode)
{

}

void gen_return(AST_NODE *stmtNode)
{

}

void gen_block(AST_NODE *blockNode, int *ARoffset)
{
    if( blockNode->child == NULL )
        return;
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
            case ASSIGN_STMT:
                gen_assign(stmtNode);
                break;
            case FUNCTION_CALL_STMT:
                gen_function_call(stmtNode);
                break;
            case RETURN_STMT:
                gen_return(stmtNode);
                break;
        }
        stmtNode = stmtNode->rightSibling;
    }
    return;
}

void gen_head(char *funcName)
{
    fprintf(output, ".test\n");
    fprintf(output, "\t_start_%s:\n", funcName);
    return;
}

void gen_localVar(AST_NODE *blockNode, int *ARoffset)
{
    if( blockNode->child->nodeType == VARIABLE_DECL_LIST_NODE ){
        AST_NODE *var_decl_node = blockNode->child->child;
        while( var_decl_node != NULL ){
            if( var_decl_node->semantic_value.declSemanticValue.kind == VARIABLE_DECL ){
                AST_NODE *variable = var_decl_node->rightSibling;
                while( variable != NULL ){
                    SymbolTableEntry *entry = variable->semantic_value.identifierSemanticValue.symbolTableEntry;
                    int unit = 1;
                    if( entry->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR ){
                        for( int i = 0; i < entry->attribute->attr.typeDescriptor->properties.arrayProperties.dimension; i++ ){
                            unit *= entry->attribute->attr.typeDescriptor->properties.arrayProperties.sizeInEachDimension[i];
                        }
                    }
                    (*ARoffset) += (4 * unit);
                    entry->offset = (*ARoffset);

                    variable = variable->rightSibling;
                }
                var_decl_node = var_decl_node->rightSibling;
            }
        }
    }
    return;
}

void gen_prologue(char *funcName, AST_NODE *blockNode, int *ARoffset)
{
    fprintf(output, "\tsd   ra, 8(sp)\n");
    fprintf(output, "\tsd   fp, 0(sp)\n");
    fprintf(output, "\tsd   ra, 0(sp)\n");
    fprintf(output, "\tsd   fp, -8(sp)\n");
    fprintf(output, "\taddi fp, sp, -8\n");
    fprintf(output, "\taddi sp, sp, -16\n");
    fprintf(output, "\tla   ra, _frameSize_%s\n", funcName);
    fprintf(output, "\tlw   ra, 0(ra)\n");
    fprintf(output, "\tsub  sp, sp, ra\n");

    for(int i = 1; i <= 11; ++i) {      //save callee-saved reg
        fprintf(output, "\tsd   s%d, %d(sp)\n", i, i * 8);
    }
    for(int i = 0; i <= 11; ++i) {      //save callee-saved reg
        fprintf(output, "\tfsw   s%d, %d(sp)\n", i, 96 + i * 4);
    }
    gen_localVar(blockNode, ARoffset);
    return;
}

void gen_epilogue(char *funcName, int size)
{
    for(int i = 1; i <= 11; ++i) {      //restore callee-saved reg
        fprintf(output, "\tld   s%d, %d(sp)\n", i, i * 8);
    }
    for(int i = 0; i <= 11; ++i) {      //save callee-saved reg
        fprintf(output, "\tflw   s%d, %d(sp)\n", i, 96 + i * 4);
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
    gen_prologue(functionName, blockNode, &ARoffset);
    gen_block(blockNode, &ARoffset);
    gen_epilogue(functionName, ARoffset + 144);
    return;
}

void gen_code(AST_NODE *root)
{
    output = fopen("output.s", "w");
    AST_NODE *child = root->child;
    while( child != NULL ){
        if( child->nodeType == VARIABLE_DECL_LIST_NODE ){    //for global decl or typedef
            
        }
        else{   //for function declaration
            gen_funcDecl(child);
        }
        child = child->rightSibling;
    }
    fclose(output);
    return;
}