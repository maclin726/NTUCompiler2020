#include "symbolTable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// This file is for reference only, you are not required to follow the implementation. //

int HASH(char * str) {
	int idx=0;
	while (*str){
		idx = idx << 1;
		idx+=*str;
		str++;
	}
	return (idx & (HASH_TABLE_SIZE-1));
}

SymbolTable symbolTable;

SymbolTableEntry* newSymbolTableEntry(char* symbolName, SymbolAttribute* attribute, int nestingLevel)
{
    SymbolTableEntry* symbolTableEntry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
    symbolTableEntry->nextInHashChain = NULL;
    symbolTableEntry->prevInHashChain = NULL;
    symbolTableEntry->nextInSameLevel = NULL;
    symbolTableEntry->sameNameInOuterLevel = NULL;
    symbolTableEntry->attribute = attribute;    // not sure
    symbolTableEntry->name = symbolName;        // not sure
    symbolTableEntry->nestingLevel = nestingLevel;
    return symbolTableEntry;
}

void removeFromHashChain(SymbolTableEntry* entry)    // delete
{
    if(entry->prevInHashChain != NULL) {
        entry->prevInHashChain->nextInHashChain = entry->nextInHashChain;
    }
    else {
        int hashVal = HASH(entry->name);
        symbolTable.hashTable[hashVal] = entry->nextInHashChain;
    }
    if(entry->nextInHashChain != NULL) {
        entry->nextInHashChain->prevInHashChain = entry->prevInHashChain;
    }

    entry->prevInHashChain = NULL;
    entry->nextInHashChain = NULL;
    return;
}

void enterIntoHashChain(SymbolTableEntry* entry)     // add
{
    int hashVal = HASH(entry->name);
    SymbolTableEntry *head = symbolTable.hashTable[hashVal];
    entry->nextInHashChain = head;
    symbolTable.hashTable[hashVal] = entry;
    if(head != NULL) {
        head->prevInHashChain = entry;
    }
    return;
}

void initializeSymbolTable()
{
    for(int i = 0; i < HASH_TABLE_SIZE; ++i) {
        symbolTable.hashTable[i] = NULL;
    }
    for(int i = 0; i < MAX_SCOPE_SIZE; ++i) {
        symbolTable.scopeDisplay[i] = NULL;
    }
    
    symbolTable.currentLevel = 0;
    symbolTable.scopeDisplayElementCount = 0;

    // int
    SymbolAttribute *attribute = (SymbolAttribute *)malloc(sizeof(SymbolAttribute));
    attribute->attr.typeDescriptor = (TypeDescriptor *)malloc(sizeof(TypeDescriptor));
    attribute->attributeKind = TYPE_ATTRIBUTE;
    attribute->attr.typeDescriptor->kind = SCALAR_TYPE_DESCRIPTOR;
    attribute->attr.typeDescriptor->properties.dataType = INT_TYPE;
    enterSymbol(SYMBOL_TABLE_INT_NAME, attribute);

    // float
    attribute = (SymbolAttribute *)malloc(sizeof(SymbolAttribute));
    attribute->attr.typeDescriptor = (TypeDescriptor *)malloc(sizeof(TypeDescriptor));
    attribute->attributeKind = TYPE_ATTRIBUTE;
    attribute->attr.typeDescriptor->kind = SCALAR_TYPE_DESCRIPTOR;
    attribute->attr.typeDescriptor->properties.dataType = FLOAT_TYPE;
    enterSymbol(SYMBOL_TABLE_FLOAT_NAME, attribute);
    
    // void
    attribute = (SymbolAttribute *)malloc(sizeof(SymbolAttribute));
    attribute->attr.typeDescriptor = (TypeDescriptor *)malloc(sizeof(TypeDescriptor));
    attribute->attributeKind = TYPE_ATTRIBUTE;
    attribute->attr.typeDescriptor->kind = SCALAR_TYPE_DESCRIPTOR;
    attribute->attr.typeDescriptor->properties.dataType = VOID_TYPE;
    enterSymbol(SYMBOL_TABLE_VOID_NAME, attribute);

    // read
    attribute = (SymbolAttribute *)malloc(sizeof(SymbolAttribute));
    attribute->attr.functionSignature = (FunctionSignature *)malloc(sizeof(FunctionSignature));
    attribute->attributeKind = FUNCTION_SIGNATURE;
    attribute->attr.functionSignature->parametersCount = 0;
    attribute->attr.functionSignature->parameterList = NULL;
    attribute->attr.functionSignature->returnType = INT_TYPE;
    enterSymbol(SYMBOL_TABLE_SYS_LIB_READ, attribute);

    // write
    attribute = (SymbolAttribute *)malloc(sizeof(SymbolAttribute));
    attribute->attr.functionSignature = (FunctionSignature *)malloc(sizeof(FunctionSignature));
    attribute->attributeKind = FUNCTION_SIGNATURE;
    attribute->attr.functionSignature->parametersCount = 1;
    attribute->attr.functionSignature->parameterList = NULL;
    attribute->attr.functionSignature->returnType = INT_TYPE;
    enterSymbol(SYMBOL_TABLE_SYS_LIB_WRITE, attribute);

    return;
}

void symbolTableEnd()
{
    // ...
}

SymbolTableEntry* retrieveSymbol(char* symbolName)
{
    SymbolTableEntry *sym = symbolTable.hashTable[HASH(symbolName)];
    while(sym != NULL) {
        if(strcmp(sym->name, symbolName) == 0) {
            return sym;
        }
        sym = sym->nextInHashChain;
    }
    return NULL;
}

SymbolTableEntry* enterSymbol(char* symbolName, SymbolAttribute* attribute)
{
    SymbolTableEntry *oldSym = retrieveSymbol(symbolName);
    if(oldSym != NULL && oldSym->nestingLevel == symbolTable.currentLevel) {
        return NULL;    // duplicate definition
    }
    SymbolTableEntry *newSym = newSymbolTableEntry(symbolName, attribute, symbolTable.currentLevel);
    // add to scope display
    newSym->nextInSameLevel = symbolTable.scopeDisplay[symbolTable.currentLevel];
    newSym->nestingLevel = symbolTable.currentLevel;
    symbolTable.scopeDisplay[symbolTable.currentLevel] = newSym;
    // add to hash table
    if(oldSym != NULL){
        removeFromHashChain(oldSym);
    }
    enterIntoHashChain(newSym);
    newSym->sameNameInOuterLevel = oldSym;
    return newSym;
}

//remove the symbol from the current scope
void removeSymbol(char* symbolName)
{
    // NONE
}

SymbolTableEntry* declaredLocally(char* symbolName)
{
    SymbolTableEntry *oldSym = retrieveSymbol(symbolName);
    if(oldSym != NULL && oldSym->nestingLevel == symbolTable.currentLevel) {
        return oldSym;
    }
    return NULL;
}

void openScope()
{
    ++(symbolTable.currentLevel);
    return;
}

void closeScope()
{
    SymbolTableEntry *curSym = symbolTable.scopeDisplay[symbolTable.currentLevel];
    while(curSym != NULL) {
        SymbolTableEntry *prevSym = curSym->sameNameInOuterLevel;
        SymbolTableEntry *oldSym = curSym;
        curSym = curSym->nextInSameLevel;
        removeFromHashChain(oldSym);
        if(prevSym != NULL) {
            enterIntoHashChain(prevSym);
        }
    }
    --(symbolTable.currentLevel);
    return;
}
