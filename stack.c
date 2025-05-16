// Generic dynamic array-based stack

#include "stack.h"
#include<stdio.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdlib.h>
#include<string.h>

Stack* createStack(size_t elementSize, int capacity){
    Stack* stack = (Stack*)malloc(sizeof(Stack));
    stack->array = (void**)malloc(capacity * sizeof(void*));
    stack->top = -1;
    stack->size = 0;
    stack->capacity = capacity;
    stack->elementSize = elementSize;
    return stack;
}

void push(Stack* stack, void* element){
    if(isFull(stack)){
       printf("Stack Overflow\n");
       return; 
    }
    void* newElement = malloc(stack->elementSize);
    memcpy(newElement, element,stack->elementSize);
    stack->array[++stack->top] = newElement;
    stack->size++;    
}

void* pop(Stack *stack){
    if(isEmpty(stack)){
        printf("Stack Underflow\n");
        return NULL;
    }
    stack->size--;
    return stack->array[stack->top--];
}

void* peek(Stack *stack){
    if(isEmpty(stack)){
        printf("Stack Underflow\n");
        return NULL;
    }
    return stack->array[stack->top];
}

bool isFull(Stack *stack){
    return (stack->size == stack->capacity);
}

bool isEmpty(Stack *stack){
    return (stack->size == 0);
}

void freeStack(Stack *stack){
    while(!isEmpty(stack)){
        free(stack->array[stack->top--]);  // free individual elements
        stack->size--;
    }
    free(stack->array);  // free array
    free(stack);  // free stack
}

void printElement(void* element,void(*printFunc)(void*)){
    if(element){
        printFunc(element);
    } 
}

void printInt(void* element){
    printf("%d\n", *(int*)element);
}

void printChar(void* element){
    printf("%c\n", *(char*)element);
}

void printFloat(void* element){
    printf("%f\n", *(float*)element);
}




/*
KEY CHARACTERISTICS

Generic:
The stack can store elements of any data type (void*), making it "generic." It uses elementSize to handle arbitrary data sizes and relies on memcpy to copy elements, allowing flexibility (e.g., integers, floats, characters, or even custom structs).
The use of function pointers like printFunc in printElement further enhances its generality by allowing custom printing for different types.
Dynamic:
Memory is dynamically allocated using malloc for both the stack structure (Stack*) and the array of pointers (void**). This allows the stack to be created at runtime with a user-specified capacity and elementSize.
Array-Based:
The stack uses a contiguous block of memory (an array of void* pointers) to store elements, with top tracking the current top element. This is a classic array-based stack approach, as opposed to, say, a linked-list-based stack.
Stack Features:
It follows the Last-In-First-Out (LIFO) principle, with standard operations like push, pop, and peek.
It includes boundary checks (isFull, isEmpty) to prevent overflow and underflow, printing error messages when limits are hit.
This contrasts with other stack types, like:

Fixed-size array-based stacks: No dynamic allocation; capacity is set at compile time.
Linked-list stacks: Uses nodes with pointers instead of an array.
Typed stacks: Designed for a specific data type (e.g., int only) rather than being generic.


*/