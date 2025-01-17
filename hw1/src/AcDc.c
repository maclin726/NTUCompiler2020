#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "header.h"


int main( int argc, char *argv[] )
{
    FILE *source, *target;
    Program program;
    SymbolTable symtab;

    if( argc == 3){
        source = fopen(argv[1], "r");
        target = fopen(argv[2], "w");
        if( !source ){
            printf("can't open the source file\n");
            exit(2);
        }
        else if( !target ){
            printf("can't open the target file\n");
            exit(2);
        }
        else{
            program = parser(source);
            fclose(source);
            symtab = build(program);
            check(&program, &symtab);
            gencode(program, target, symtab);
        }
    }
    else{
        printf("Usage: %s source_file target_file\n", argv[0]);
        exit(2);
    }


    return 0;
}


/********************************************* 
  Scanning 
 *********************************************/
Token getNumericToken( FILE *source, char c )
{
    Token token;
    int i = 0;

    while( isdigit(c) ) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    if( c != '.' ){
        ungetc(c, source);
        token.tok[i] = '\0';
        token.type = IntValue;
        return token;
    }

    token.tok[i++] = '.';

    c = fgetc(source);
    if( !isdigit(c) ){
        ungetc(c, source);
        printf("Expect a digit : %c\n", c);
        exit(1);
    }

    while( isdigit(c) ){
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    token.type = FloatValue;
    return token;
}

Token scanner( FILE *source )
{
    char c, next_c;
    Token token;

    while( !feof(source) ){
        c = fgetc(source);

        while( isspace(c) ) c = fgetc(source);
        

        if( isdigit(c) )
            return getNumericToken(source, c);

        token.tok[0] = c;
        token.tok[1] = '\0';
        if( islower(c) ){
        	next_c = fgetc(source);
            if( c == 'f' && !islower(next_c))
            	token.type = FloatDeclaration;
            else if( c == 'i' && !islower(next_c))
                token.type = IntegerDeclaration;
            else if( c == 'p' && !islower(next_c)) {
                // because when exiting the declaration, we'll unget this token in parserDeclaration
                ungetc(next_c, source);
                token.type = PrintOp;
            }
            else {
                token.type = Alphabet;
                int i = 1;
                while(islower(next_c)){
                	token.tok[i++] = next_c;
                	next_c = fgetc(source);
                }
                token.tok[i] = '\0';
                if(strlen(token.tok) >= MAXNAMELEN) {
                	printf("Syntax Error: The length of id is too long.\n");
                	exit(1);
                }
                ungetc(next_c, source);
            }
            return token;
        }

        switch(c){
            case '=':
                token.type = AssignmentOp;
                return token;
            case '+':
                token.type = PlusOp;
                return token;
            case '-':
                token.type = MinusOp;
                return token;
            case '*':
                token.type = MulOp;
                return token;
            case '/':
                token.type = DivOp;
                return token;
            case '(':
            	token.type = LeftPar;
            	return token;
            case ')':
            	token.type = RightPar;
            	return token;
            case EOF:
                token.type = EOFsymbol;
                token.tok[0] = '\0';
                return token;
            default:
                printf("Invalid character : %c\n", c);
                exit(1);
        }
    }

    token.tok[0] = '\0';
    token.type = EOFsymbol;
    return token;
}


/********************************************************
  Parsing
 *********************************************************/
Declaration parseDeclaration( FILE *source, Token token )
{
    Token token2;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            token2 = scanner(source);
            if (strcmp(token2.tok, "f") == 0 ||
                    strcmp(token2.tok, "i") == 0 ||
                    strcmp(token2.tok, "p") == 0) {
                printf("Syntax Error: %s cannot be used as id\n", token2.tok);
                exit(1);
            }
            return makeDeclarationNode( token, token2 );
        default:
            printf("Syntax Error: Expect Declaration %s\n", token.tok);
            exit(1);
    }
}

Declarations *parseDeclarations( FILE *source )
{
    Token token = scanner(source);
    Declaration decl;
    Declarations *decls;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            decl = parseDeclaration(source, token);
            decls = parseDeclarations(source);
            return makeDeclarationTree( decl, decls );
        case PrintOp:
        case Alphabet:
        	for(int i = strlen(token.tok) - 1; i >= 0; i--)
        		ungetc(token.tok[i], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect declarations %s\n", token.tok);
            exit(1);
    }
}

Expression *parseFactor( FILE *source )
{
    Token token = scanner(source);
    Expression *factor = (Expression *)malloc( sizeof(Expression) );
    factor->leftOperand = factor->rightOperand = NULL;

    switch(token.type){
    	case LeftPar:
    		factor = parseExpression(source);
    		Token match = scanner(source);
    		if(match.type != RightPar) {
    			printf("Syntax Error: Unbalanced parenthesis.\n");
            	exit(1);
    		}
    		break;
        case Alphabet:
            (factor->v).type = Identifier;
            strcpy((factor->v).val.id, token.tok);
            break;
        case IntValue:
            (factor->v).type = IntConst;
            (factor->v).val.ivalue = atoi(token.tok);
            break;
        case FloatValue:
            (factor->v).type = FloatConst;
            (factor->v).val.fvalue = atof(token.tok);
            break;
        default:
            printf("Syntax Error: Expect Identifier or a Number %s\n", token.tok);
            exit(1);
    }

    return factor;
}

Expression *parseTermTail( FILE *source, Expression *lvalue )
{
    Expression *R2;
    Token token = scanner(source);

    switch(token.type){
        case MulOp:
            R2 = (Expression *)malloc( sizeof(Expression) );
            (R2->v).type = MulNode;
            (R2->v).val.op = Mul;
            R2->leftOperand = lvalue;
            R2->rightOperand = parseFactor(source);
            return parseTermTail(source, R2);
        case DivOp:
            R2 = (Expression *)malloc( sizeof(Expression) );
            (R2->v).type = DivNode;
            (R2->v).val.op = Div;
            R2->leftOperand = lvalue;
            R2->rightOperand = parseFactor(source);
            return parseTermTail(source, R2);
        case PlusOp:
        case MinusOp:
        case RightPar:
        case Alphabet:
        case PrintOp:
            for(int i = strlen(token.tok) - 1; i >= 0; i--)
        		ungetc(token.tok[i], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseTerm( FILE *source)
{
	Expression *factor, *R2;
	factor = parseFactor(source);
    R2 = parseTermTail(source, factor);
    return R2;
}

Expression *parseExpressionTail( FILE *source, Expression *lvalue )
{
    Expression *R1;
    Token token = scanner(source);

    switch(token.type){
        case PlusOp:
            R1 = (Expression *)malloc( sizeof(Expression) );
            (R1->v).type = PlusNode;
            (R1->v).val.op = Plus;
            R1->leftOperand = lvalue;
            R1->rightOperand = parseTerm(source);
            return parseExpressionTail(source, R1);
        case MinusOp:
            R1 = (Expression *)malloc( sizeof(Expression) );
            (R1->v).type = MinusNode;
            (R1->v).val.op = Minus;
            R1->leftOperand = lvalue;
            R1->rightOperand = parseTerm(source);
            return parseExpressionTail(source, R1);
        case RightPar:
        case Alphabet:
        case PrintOp:
            for(int i = strlen(token.tok) - 1; i >= 0; i--)
        		ungetc(token.tok[i], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpression( FILE *source)
{
    Expression *term, *R1;
    term = parseTerm(source);
    R1 = parseExpressionTail(source, term);
    return R1;
}

Statement parseStatement( FILE *source, Token token )
{
    Token next_token;
    Expression *value, *expr;

    switch(token.type){
        case Alphabet:
            next_token = scanner(source);
            if(next_token.type == AssignmentOp){
                //value = parseValue(source);
                expr = parseExpression(source);
                return makeAssignmentNode(token.tok, expr, expr);
            }
            else{
                printf("Syntax Error: Expect an assignment op %s\n", next_token.tok);
                exit(1);
            }
        case PrintOp:
            next_token = scanner(source);
            if(next_token.type == Alphabet)
                return makePrintNode(next_token.tok);
            else{
                printf("Syntax Error: Expect an identifier %s\n", next_token.tok);
                exit(1);
            }
            break;
        default:
            printf("Syntax Error: Expect a statement %s\n", token.tok);
            exit(1);
    }
}

Statements *parseStatements( FILE * source )
{

    Token token = scanner(source);
    Statement stmt;
    Statements *stmts;

    switch(token.type){
        case Alphabet:
        case PrintOp:
            stmt = parseStatement(source, token);
            stmts = parseStatements(source);
            return makeStatementTree(stmt , stmts);
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect statements %s\n", token.tok);
            exit(1);
    }
}


/*********************************************************************
  Build AST
 **********************************************************************/
Declaration makeDeclarationNode( Token declare_type, Token identifier )
{
    Declaration tree_node;

    switch(declare_type.type){
        case FloatDeclaration:
            tree_node.type = Float;
            break;
        case IntegerDeclaration:
            tree_node.type = Int;
            break;
        default:
            break;
    }

    strcpy(tree_node.name, identifier.tok);

    return tree_node;
}

Declarations *makeDeclarationTree( Declaration decl, Declarations *decls )
{
    Declarations *new_tree = (Declarations *)malloc( sizeof(Declarations) );
    new_tree->first = decl;
    new_tree->rest = decls;

    return new_tree;
}


Statement makeAssignmentNode( char *id, Expression *v, Expression *expr_tail )
{
    Statement stmt;
    AssignmentStatement assign;

    stmt.type = Assignment;
    strcpy(assign.id, id);
    if(expr_tail == NULL)
        assign.expr = v;
    else
        assign.expr = expr_tail;
    stmt.stmt.assign = assign;

    return stmt;
}

Statement makePrintNode( char *id )
{
    Statement stmt;
    stmt.type = Print;
    strcpy(stmt.stmt.variable, id);

    return stmt;
}

Statements *makeStatementTree( Statement stmt, Statements *stmts )
{
    Statements *new_tree = (Statements *)malloc( sizeof(Statements) );
    new_tree->first = stmt;
    new_tree->rest = stmts;

    return new_tree;
}

/* parser */
Program parser( FILE *source )
{
    Program program;

    program.declarations = parseDeclarations(source);
    program.statements = parseStatements(source);

    return program;
}


/********************************************************
  Build symbol table
 *********************************************************/

int hash( char *name )
{
	char c;
	int i = 0, hashValue = 0;

	while((c = name[i++]) != '\0')
		hashValue = (hashValue * 27 + c - 'a' + 1) % HASHTABLESIZE;
	return hashValue;
}

void InitializeTable( SymbolTable *table )
{
    int i;

    for(i = 0 ; i < HASHTABLESIZE; i++) {
        table->table[i] = Notype;
        table->symbolName[i] = NULL;
    }
}

void add_table( SymbolTable *table, char *name, DataType t )
{
    int index = hash(name);

    while(table->table[index] != Notype) {
    	if(strcmp(table->symbolName[index], name) == 0) {
            printf("Error : id %s has been declared\n", name);//error or collision
            exit(1);
        }
        index = (index + 1) % HASHTABLESIZE;
    }
    table->table[index] = t;
    table->symbolName[index] = (char *)malloc(sizeof(char) * MAXNAMELEN);
    strcpy(table->symbolName[index], name);
}

SymbolTable build( Program program )
{
    SymbolTable table;
    Declarations *decls = program.declarations;
    Declaration current;
    // add register count

    InitializeTable(&table);

    while(decls != NULL){
        current = decls->first;
        add_table(&table, current.name, current.type);
        decls = decls->rest;
    }

    return table;
}


/********************************************************************
  Type checking
 *********************************************************************/

void convertType( Expression * old, DataType type )
{
    if(old->type == Float && type == Int){
        printf("error : can't convert float to integer\n");
        exit(1);
    }
    if(old->type == Int && type == Float){
        Expression *tmp = (Expression *)malloc( sizeof(Expression) );
        if(old->v.type == Identifier)
            printf("convert to float %s \n",old->v.val.id);
        else if(old->v.type == IntConst)
            printf("convert to float %d \n", old->v.val.ivalue);
        tmp->v = old->v;
        tmp->leftOperand = old->leftOperand;
        tmp->rightOperand = old->rightOperand;
        tmp->type = old->type;

        Value v;
        v.type = IntToFloatConvertNode;
        v.val.op = IntToFloatConvert;
        old->v = v;
        old->type = Float;
        old->leftOperand = tmp;
        old->rightOperand = NULL;
    }
}

DataType generalize( Expression *left, Expression *right )
{
    if(left->type == Float || right->type == Float){
        printf("generalize : float\n");
        return Float;
    }
    printf("generalize : int\n");
    return Int;
}

DataType lookup_table( SymbolTable *table, char *name )
{
    int index = hash(name);
    int i = index;
    do{
    	if(table->symbolName[i] == NULL || strcmp(table->symbolName[i], name) == 0)
    		break;
    	i = (i + 1) % HASHTABLESIZE;
    }while(i != index);
	
    if(table->symbolName[i] != NULL && strcmp(table->symbolName[i], name) == 0)
    	index = i;
    else {
    	printf("Error : identifier %s is not declared\n", name);//error
    	exit(1);
    }        
    return table->table[index];
}

int constFolding( Expression * expr, DataType type )
{
	Expression *left = expr->leftOperand;
    Expression *right = expr->rightOperand;
    if((left->v.type != IntConst && left->v.type != FloatConst)
    	|| (right->v.type != IntConst && right->v.type != FloatConst))
    	return 0;

    expr->type = type;
    if(type == Int) {
    	switch(expr->v.type) {
    		case PlusNode:
    			expr->v.val.ivalue = left->v.val.ivalue + right->v.val.ivalue;
    			break;
    		case MinusNode:
    			expr->v.val.ivalue = left->v.val.ivalue - right->v.val.ivalue;
    			break;
    		case MulNode:
    			expr->v.val.ivalue = left->v.val.ivalue * right->v.val.ivalue;
    			break;
    		case DivNode:
    			expr->v.val.ivalue = left->v.val.ivalue / right->v.val.ivalue;
    			break;
    		default:
    			break;
    	}
    	expr->v.type = IntConst;
    }
    else{
    	float lvalue, rvalue;

    	lvalue = (left->v.type == IntConst)? left->v.val.ivalue : left->v.val.fvalue;
    	rvalue = (right->v.type == IntConst)? right->v.val.ivalue : right->v.val.fvalue;

    	switch(expr->v.type) {
    		case PlusNode:
    			expr->v.val.fvalue = lvalue + rvalue;
    			break;
    		case MinusNode:
    			expr->v.val.fvalue = lvalue - rvalue;
    			break;
    		case MulNode:
    			expr->v.val.fvalue = lvalue * rvalue;
    			break;
    		case DivNode:
    			expr->v.val.fvalue = lvalue / rvalue;
    			break;
    		default:
    			break;
    	}
    	expr->v.type = FloatConst;
    }
    free(left);
    free(right);
    expr->leftOperand = NULL;
    expr->rightOperand = NULL;
    return 1;
}

void checkexpression( Expression * expr, SymbolTable * table )
{
    char name[MAXNAMELEN];
    if(expr->leftOperand == NULL && expr->rightOperand == NULL){
        switch(expr->v.type){
            case Identifier:
                strcpy(name, expr->v.val.id);
                printf("identifier : %s\n",name);
                expr->type = lookup_table(table, name);
                break;
            case IntConst:
                printf("constant : int\n");
                expr->type = Int;
                break;
            case FloatConst:
                printf("constant : float\n");
                expr->type = Float;
                break;
                //case PlusNode: case MinusNode: case MulNode: case DivNode:
            default:
                break;
        }
    }
    else{
        Expression *left = expr->leftOperand;
        Expression *right = expr->rightOperand;

        checkexpression(left, table);
        checkexpression(right, table);

        DataType type = generalize(left, right);
        if(constFolding(expr, type))
        	return;
        convertType(left, type);//left->type = type;//converto
        convertType(right, type);//right->type = type;//converto
        expr->type = type;
    }
}

void checkstmt( Statement *stmt, SymbolTable * table )
{
    if(stmt->type == Assignment){
        AssignmentStatement assign = stmt->stmt.assign;
        printf("assignment : %s \n",assign.id);
        checkexpression(assign.expr, table);
        stmt->stmt.assign.type = lookup_table(table, assign.id);
        if (assign.expr->type == Float && stmt->stmt.assign.type == Int) {
            printf("error : can't convert float to integer\n");
            exit(1);
        } else {
            convertType(assign.expr, stmt->stmt.assign.type);
        }
    }
    else if (stmt->type == Print){
        printf("print : %s \n",stmt->stmt.variable);
        lookup_table(table, stmt->stmt.variable);
    }
    else{
    	printf("error : statement error\n");//error
    	exit(1);
    }
}

void check( Program *program, SymbolTable * table )
{
    Statements *stmts = program->statements;
    while(stmts != NULL){
        checkstmt(&stmts->first,table);
        stmts = stmts->rest;
    }
}


/***********************************************************************
  Code generation
 ************************************************************************/
char findRegister(char *name, SymbolTable *table)
{
	int index = hash(name);
    int i = index;
    do{
    	if(strcmp(table->symbolName[i], name) == 0)
    		break;
    	
    	i = (i + 1) % HASHTABLESIZE;
    }while(i != index);
    return (char)(i + 'a');
}

void fprint_op( FILE *target, ValueType op )
{
    switch(op){
        case MinusNode:
            fprintf(target,"-\n");
            break;
        case PlusNode:
            fprintf(target,"+\n");
            break;
        case MulNode:
        	fprintf(target,"*\n");
            break;
        case DivNode:
        	fprintf(target,"/\n");
            break;
        default:
            //fprintf(target,"Error in fprintf_op ValueType = %d\n",op);
            exit(1);
    }
}

void fprint_expr( FILE *target, Expression *expr, SymbolTable * table)
{
    if(expr->leftOperand == NULL){
        switch( (expr->v).type ){
            case Identifier:
                fprintf(target,"l%c\n", findRegister((expr->v).val.id, table));
                break;
            case IntConst:
                fprintf(target,"%d\n",(expr->v).val.ivalue);
                break;
            case FloatConst:
                fprintf(target,"%f\n", (expr->v).val.fvalue);
                break;
            default:
                //fprintf(target,"Error In fprint_left_expr. (expr->v).type=%d\n",(expr->v).type);
                exit(1);
        }
    }
    else{
        fprint_expr(target, expr->leftOperand, table);
        if(expr->rightOperand == NULL){
            fprintf(target,"5k\n");
        }
        else{
            //	fprint_right_expr(expr->rightOperand);
            fprint_expr(target, expr->rightOperand, table);
            fprint_op(target, (expr->v).type);
        }
    }
}

void gencode(Program prog, FILE * target, SymbolTable symtab)
{
    Statements *stmts = prog.statements;
    Statement stmt;

    while(stmts != NULL){
        stmt = stmts->first;
        switch(stmt.type){
            case Print:
                fprintf(target,"l%c\n", findRegister(stmt.stmt.variable, &symtab));
                fprintf(target,"p\n");
                break;
            case Assignment:
                fprint_expr(target, stmt.stmt.assign.expr, &symtab);
                /*
                   if(stmt.stmt.assign.type == Int){
                   fprintf(target,"0 k\n");
                   }
                   else if(stmt.stmt.assign.type == Float){
                   fprintf(target,"5 k\n");
                   }*/
                fprintf(target,"s%c\n", findRegister(stmt.stmt.assign.id, &symtab));
                fprintf(target,"0 k\n");
                break;
        }
        stmts=stmts->rest;
    }

}


/***************************************
  For our debug,
  you can omit them.
 ****************************************/
void print_expr(Expression *expr)
{
    if(expr == NULL)
        return;
    else{
        print_expr(expr->leftOperand);
        switch((expr->v).type){
            case Identifier:
                printf("%c ", (expr->v).val.id);
                break;
            case IntConst:
                printf("%d ", (expr->v).val.ivalue);
                break;
            case FloatConst:
                printf("%f ", (expr->v).val.fvalue);
                break;
            case PlusNode:
                printf("+ ");
                break;
            case MinusNode:
                printf("- ");
                break;
            case MulNode:
                printf("* ");
                break;
            case DivNode:
                printf("/ ");
                break;
            case IntToFloatConvertNode:
                printf("(float) ");
                break;
            default:
                printf("error ");
                break;
        }
        print_expr(expr->rightOperand);
    }
}

void test_parser( FILE *source )
{
    Declarations *decls;
    Statements *stmts;
    Declaration decl;
    Statement stmt;
    Program program = parser(source);

    decls = program.declarations;

    while(decls != NULL){
        decl = decls->first;
        if(decl.type == Int)
            printf("i ");
        if(decl.type == Float)
            printf("f ");
        printf("%c ",decl.name);
        decls = decls->rest;
    }

    stmts = program.statements;

    while(stmts != NULL){
        stmt = stmts->first;
        if(stmt.type == Print){
            printf("p %c ", stmt.stmt.variable);
        }

        if(stmt.type == Assignment){
            printf("%c = ", stmt.stmt.assign.id);
            print_expr(stmt.stmt.assign.expr);
        }
        stmts = stmts->rest;
    }

}
