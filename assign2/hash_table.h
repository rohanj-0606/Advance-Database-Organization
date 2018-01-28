
#include <stdlib.h>

#ifndef HASH_TABLE_H
#define HASH_TABLE_H
#endif

#define NO_KEY -1
#define NOT_FOUND -1
/*
  Intially a hash table is created with a given size. We don't change the size since. 
  In case of collision we simply chain it at the position determined by the hash function.
  					+---------+
  					|  key    |---> next
  					|  value  |
  					+---------+
  					|  key    |--> next
  					|  value  |
  					+---------+
  					.         .
   "size" elements  .         .
  					.         .
  					+---------+
  					|  key    |
  					|  Value  |---> next
  					+---------+
*/

// Each Node in the hash table contains a key, value and pointer to next node.
// The next node points to the head of chain (The chain is formed in case of collision).
// 
typedef struct HashNode{
    int key;
    int value;
    struct HashNode* next;
} HashNode;

// Hash table which contains a array of hash nodes as depicted above
typedef struct HashTable{
    HashNode * hashNode;
    size_t size;
} HashTable;

HashTable* createHashTable(size_t tableSize);
void insertHashNode(HashTable* hashTable, int key, int value);
int searchHashTable(HashTable* hashTable, int key);
void deleteHashNode(HashTable* hashTable, int key);
void delsertHashNode(HashTable* hashTable, int oldKey, int newKey, int newValue);
HashNode* getHashNode(HashTable* hashTable, int key);
size_t hashOfKey(HashTable *hashTable, int key);
void destroyHashTable(HashTable* hashTable);