#include "hash_table.h"
#include <math.h>
#include <stdio.h>

//Create a hash table of give size
HashTable *createHashTable(size_t tableSize) {

    HashTable *hTable = malloc(sizeof(HashTable));

    hTable->hashNode = malloc(sizeof(HashNode) * tableSize);
	hTable->size = tableSize;
    hTable->p = (int) (log(hTable->size) / log(2));
    for (size_t i = 0; i < tableSize; ++i) {
        hTable->hashNode[i].key = NO_KEY;
        hTable->hashNode[i].next = NULL;
    }
    return hTable;
}

// Given a key value pair, insert in to the hash table
void insertHashNode(HashTable *hashTable, int key, int value) {
    size_t hashValue = hashOfKey(hashTable, key);
    if (hashTable->hashNode[hashValue].key == NO_KEY) {
        hashTable->hashNode[hashValue].key = key;
        hashTable->hashNode[hashValue].value = value;
        return;
    }

    HashNode *hNode = malloc(sizeof(HashNode));
    hNode->next = NULL;
    hNode->key = key;
    hNode->value = value;

    hNode->next = hashTable->hashNode[hashValue].next;
    hashTable->hashNode[hashValue].next = hNode;

}

// Compute the hash value of the given key
size_t hashOfKey(HashTable *hTable, int key) {
    // Hash function taken from Introduction to Algorithms by cormen
    double A = (sqrt(5) - 1) / 2;
    int wordSize = 32;
    unsigned int s = (unsigned) floor(A * 2 * ((unsigned int) 2 << (wordSize - 2)));
    unsigned int x = key * s;
    size_t hashValue = x >> (wordSize - hTable->p);

    // The above hash method gaurentees the size to be less than table size, but let's double check;
    if (hashValue > hTable->size) {
        printf("Waring: Problem with hash function");
        hashValue = hashValue % hTable->size;
    }
	//printf("Key: %d, HashValue: %lu\n", key, hashValue);
    return hashValue;
}

// Given a key, search for it in the hash table,
//      if found return the value in node else return NOT_FOUND
int searchHashTable(HashTable *hashTable, int key) {
    size_t hashValue = hashOfKey(hashTable, key);

    if(hashTable->hashNode[hashValue].key == key){
        return hashTable->hashNode[hashValue].value ;
    }

    HashNode * searchNode = hashTable->hashNode[hashValue].next;

    while (searchNode != NULL){
        if(searchNode->key == key){
            return searchNode->value;
        }
        searchNode = searchNode->next;
    }

    return NOT_FOUND;
}

// Given a key, delete the node from the hashtable if exists
void deleteHashNode(HashTable *hashTable, int key) {
    size_t hashValue = hashOfKey(hashTable, key);

    if(hashTable->hashNode[hashValue].key == key){
        hashTable->hashNode[hashValue].key = NO_KEY;
    }

    HashNode * currNode = hashTable->hashNode[hashValue].next;
	HashNode * prevNode = &(hashTable->hashNode[hashValue]);

    while (currNode!=NULL){
        if(currNode->key == key){
			prevNode->next = currNode->next;
            currNode->key = NO_KEY;
			free(currNode);
            return;
        }
		prevNode = currNode;
        currNode = currNode->next;
    }

}

// Delete the hash table from memory
void destroyHashTable(HashTable *hashTable) {
    HashNode* currNode,*prevNode;
    for (size_t i = 0; i < hashTable->size; ++i) {
        currNode = hashTable->hashNode[i].next;
        while ( currNode != NULL){
            prevNode = currNode;
            currNode = currNode->next;
            free(prevNode);
        }
    }

    free(hashTable->hashNode);
    free(hashTable);
}

// This function deletes the hash node with key=oldKey and Inserts a new node with (newKey, newValue) in the hash table
void delsertHashNode(HashTable *hashTable, int oldKey, int newKey, int newValue) {
    deleteHashNode(hashTable, oldKey);
    insertHashNode(hashTable,newKey,newValue);
}

// Returns the node (not value) from hashTable with the given key
HashNode *getHashNode(HashTable *hashTable, int key) {
    size_t hashValue = hashOfKey(hashTable, key);

    if(hashTable->hashNode[hashValue].key == key){
        return &(hashTable->hashNode[hashValue]);
    }

    HashNode * searchNode = hashTable->hashNode[hashValue].next;

    while (searchNode != NULL){
        if(searchNode->key == key){
            return searchNode;
        }
        searchNode = searchNode->next;
    }

    return NULL;

}

