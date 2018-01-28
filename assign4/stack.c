//
// Created by scl on 17/04/17.
//

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "stack.h"
/**
 * Creates a stack and returns it
 * @param size : Intial stack size
 * @return  IntStack* stack initialized to the given size
 */
IntStack *createStack(int size) {
    IntStack * stack;
    stack = malloc(sizeof(IntStack));
    stack->arr = malloc(sizeof(int)*size);
    stack->size = size;
    stack->top = EMPTY_STACK;

    return stack;
}

/**
 * Checks if the given stack is empty
 * @param stack : Stack on which emptiness is to be checked
 * @return true if empty else false
 */
bool isEmpty(IntStack *stack) {
    if(stack == NULL){
        printf("Invalid Stack passed");
        return INVALID_STACK;
    }
    if (stack->size - 1 == stack->top){
        return false;
    }
    return true;
}

/**
 * Pushes the given value in to the stack
 * @param stack : Stack in to which element would be pushed
 * @param n : Actual element to be pushed
 */
void push(IntStack *stack, int n) {
    if(stack == NULL){
        printf("Invalid Stack passed");
        return;
    }

    if (isEmpty(stack)){
        if (stack->top == EMPTY_STACK)
            stack->top = 0;
        else
            stack->top +=1;
        stack->arr[stack->top] = n;
    }
    else{
        // If stack is full, double the size, copy all old contents to the new array, then insert
        int * arr = calloc((size_t) (stack->size * 2), sizeof(int));
        memcpy(arr, stack->arr, sizeof(int)*stack->size);
        free(stack->arr);
        stack->arr = arr;
        stack->size *= 2;
        stack->top +=1;
        stack->arr[stack->top] = n;
    }
}

/**
 * Pop the top most element of the stack
 * @param stack Stack from which the top most element would be returned
 * @return top element in the stack
 */
int pop(IntStack *stack) {
    if(stack == NULL){
        return INVALID_STACK;
    }

    if (stack->top == EMPTY_STACK)
        return EMPTY_STACK;

    int top_element = stack->arr[stack->top];
    stack->top -= 1;
    return top_element;
}

/**
 * Returns the top most element of the stack with out actually popping it.
 * @param stack : Stack from which the top element is returned with out popping it
 * @return: The top element of the stack
 */
int peek(IntStack *stack) {
    if(stack == NULL){
        return INVALID_STACK;
    }

    if(stack->top == EMPTY_STACK)
        return EMPTY_STACK;

    return stack->arr[stack->top];
}

/**
 * Removes the stack from the memory and de-allocates all buffers.
 * @param stack Stack to be deleted.
 */
void destroyStack(IntStack *stack) {
    free(stack->arr);
    free(stack);
}


