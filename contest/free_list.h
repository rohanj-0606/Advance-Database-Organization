
#define NO_LIST -2
#define NO_FREE_SLOT -1

// Node in the linked list
typedef struct FreeListNode{
    struct FreeListNode* next;
    int buff_id;
} FreeListNode;


//Linked List which holds the pointer to the head.
typedef struct FreeList{
    FreeListNode * head;
    FreeListNode * tail;
    int listLen;
}FreeList;

typedef struct FreeList List;
typedef struct FreeListNode ListNode;

FreeList* createFreeList();
void insertFreeNode(FreeList *list, int buff_id);
FreeListNode*  getFreeNode(FreeList* list);
void destroyFreeList(FreeList* list);


ListNode* getListHead(List* list);
void deleteAppendListNode(List* list, ListNode* nodeToDel);
void deleteAppendListData(List* list, int buffId);

void insertListNode(FreeList* list, FreeListNode* node);
