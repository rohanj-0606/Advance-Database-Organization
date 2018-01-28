
#include <stdlib.h>
#include "free_list.h"
#include <stdio.h>

// Create a List data structure with empty head
FreeList *createFreeList() {
    FreeList *list;
    list = malloc(sizeof(FreeList));
    list->head = NULL;
    list->tail = NULL;
    list->listLen = 0;
    return list;
}

// Insert an element in the list at the end
void insertFreeNode(FreeList *list, int buff_id) {
    if (list == NULL) {
        printf("Create a List First");
        return;
    }

    FreeListNode *node = malloc(sizeof(FreeListNode));
    node->buff_id = buff_id;
    node->next = NULL;
    //node->prev = NULL;

    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
        list->listLen +=1;
        return;
    }

    list->tail->next = node;
    list->tail = node;
    list->listLen +=1;
    /*
    FreeListNode *currNode;
    currNode = list->head;

    while (currNode->next) {
        currNode = currNode->next;
    }

    //node->prev = currNode;
    currNode->next = node;
    */
}

// Returns node From the begining of the List
FreeListNode* getFreeNode(FreeList *list) {
    if (list == NULL) {
        printf("Create a List First");
        return NULL;
    }

    int buff_id = NO_FREE_SLOT;

    if (list->head == NULL) {

            return NULL;
    }

    FreeListNode *node;
    node = list->head;
    buff_id = node->buff_id;

    list->head = list->head->next;
    list->listLen -=1;
    //if(list->head)
      //  list->head->prev = NULL;

    //free(node);
    return node;


}

// Delete the List from memory
void destroyFreeList(FreeList *list) {

    FreeListNode *node = list->head;
    FreeListNode *prevNode;
    while (node){
        prevNode =node;
        node = node->next;
        free(prevNode);
    }
    free(list);
}

// Inserts a node (not a value) at the end of the list
void insertListNode(FreeList *list, FreeListNode *node) {


    //node->prev = NULL;

    if(list->head == NULL){
        list->head = node;
        list->tail = node;
        node->next = NULL;
        list->listLen +=1;
        return;
    }
    list->tail->next = node;
    list->tail = node;
    node->next = NULL;
    list->listLen +=1;
    /*
    FreeListNode *currNode;
    currNode = list->head;

    while (currNode->next) {
        currNode = currNode->next;
    }

    //node->prev = currNode;
    currNode->next = node;
*/
}

// Returns the head a.k.a the first node in the list.
ListNode *getListHead(List *list) {
    return list->head;
}

// Delete the given node from the list and Inserts it at the end of the list.
void deleteAppendListNode(List *list, ListNode *nodeToDel) {

    if(list == NULL)
        return;

    ListNode *node =  (list->head);
    ListNode *prevNode = NULL;

    if (node == NULL||nodeToDel == NULL)
        return;

    while (node){

        if(node ->buff_id != nodeToDel->buff_id){
            prevNode = node;
            node = node->next;
            continue;
        }
        break;
    }

    if(node == NULL){
        return;
    }

    if(node==list->head)
        list->head = node->next;
    else{
        prevNode->next = node->next;
    }
    list->listLen -=1;
    insertListNode(list, nodeToDel);

}

void deleteAppendListData(List* list, int buffId){

    if(list==NULL)
        return;

    ListNode* node = list->head;
    ListNode *prevNode = NULL;

    if(node==NULL)
        return;

    while (node){
        if(node->buff_id != buffId){
            prevNode = node;
            node = node->next;
            continue;
        }
        break;
    }
    if(node == NULL)
        return;
    if(node==list->head)
        list->head = node->next;
    else
        prevNode->next = node->next;
    list->listLen -=1;
    insertListNode(list, node);

}
