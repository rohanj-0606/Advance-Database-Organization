//
// Created by scl on 17/04/17.
//

#include "dt.h"

#ifndef ASSIGN4_STACK_H
#define ASSIGN4_STACK_H

#endif //ASSIGN4_STACK_H

#define EMPTY_STACK -99
#define INVALID_STACK -101
/*
 * Stack Implementation,
 * Holds only integers
 */

typedef struct IntStack{
    int * arr; // The array that holds the value of the stack
    int size; // Maximum size of the stack
    int top;  // The current position of the stack
}IntStack;

IntStack* createStack(int size);

void push(IntStack *stack, int n);
int pop(IntStack *stack);
int peek(IntStack *stack);

void destroyStack(IntStack *stack);
bool isEmpty(IntStack *stack);
