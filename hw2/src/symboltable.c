#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"header.h"

#define TABLE_SIZE	512
#define MAXIDNUM	1024

symtab * hash_table[TABLE_SIZE];
extern int linenumber;

int HASH(char * str){
	int idx=0;
	while(*str){
		idx = idx << 1;
		idx+=*str;
		str++;
	}	
	return (idx & (TABLE_SIZE-1));
}

/*returns the symbol table entry if found else NULL*/

symtab * lookup(char *name){
	int hash_key;
	symtab* symptr;
	if(!name)
		return NULL;
	hash_key=HASH(name);
	symptr=hash_table[hash_key];

	while(symptr){
		if(!(strcmp(name,symptr->lexeme)))
			return symptr;
		symptr=symptr->front;
	}
	return NULL;
}


void insertID(char *name){
	int hash_key;
	symtab* ptr;
	symtab* symptr=(symtab*)malloc(sizeof(symtab));	
	
	hash_key=HASH(name);
	ptr=hash_table[hash_key];
	
	if(ptr==NULL){
		/*first entry for this hash_key*/
		hash_table[hash_key]=symptr;
		symptr->front=NULL;
		symptr->back=symptr;
	}
	else{
		symptr->front=ptr;
		ptr->back=symptr;
		symptr->back=symptr;
		hash_table[hash_key]=symptr;	
	}
	
	strcpy(symptr->lexeme,name);
	symptr->line=linenumber;
	symptr->counter=1;
}

int compare(const void *a, const void *b)
{
	char *str1 = (*(symtab **)a)->lexeme;
	char *str2 = (*(symtab **)b)->lexeme;
	return strcmp(str1, str2);
}

void printSym(symtab* ptr) 
{
		printf("%-32s %d\n", ptr->lexeme, ptr->counter);
	    // printf(" Name = %s \n", ptr->lexeme);
	    // printf(" References = %d \n", ptr->counter);
}

void printSymTab()
{
    int i;
	symtab* IDs[MAXIDNUM];
	int IDcnt = 0;

	printf("\nFrequency of identifiers:\n");
    for (i=0; i<TABLE_SIZE; i++)
    {
        symtab* symptr;
		symptr = hash_table[i];
		while (symptr != NULL)
		{
			IDs[IDcnt++] = symptr;
	    	//printSym(symptr);
	    	symptr=symptr->front;
		}
    }
	qsort(IDs, IDcnt, sizeof(symtab *), compare);
	for (i = 0; i < IDcnt; i++)
	{
		printSym(IDs[i]);
	}
}
