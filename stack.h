#ifndef STACK_H
#define STACK_H
#include<stdio.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdlib.h>

typedef struct {
void** array;  // Array to store elements
int top;  // Index of top element
int size; // Current size of stack
int capacity; // Maximum number of elements
size_t elementSize;   // Size of each element
} Stack;


Stack* createStack(size_t elementSize, int capacity);
void freeStack(Stack* stack);
void push(Stack* stack, void* element);
void* pop(Stack* stack);
bool isEmpty(Stack* stack);
bool isFull(Stack* stack);
void* peek(Stack* stack);
void printElement(void* element, void(*printFunc) (void*));
void printInt(void* element);
void printFloat(void* element);
void printChar(void* element);
#endif