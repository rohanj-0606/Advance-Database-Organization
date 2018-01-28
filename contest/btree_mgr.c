#include "btree_mgr.h"
#include "stack.h"
#include <math.h>
#include <string.h>
#include <assert.h>

#define SizeofBTHeader sizeof(BtreePageHeader)
#define SizeofBTLeaf sizeof(BtreeLeafRec)
#define SizeofBTNonLeaf sizeof(BtreeNonLeafRec)
#define RootMinKeys(n) 1
#define RootMaxKeys(n) n
#define LeafMinKeys(n) floor((n+1)/2)
#define LeafMaxKeys(n) n
#define NonLeafMinKeys(n) ceil((n+1)/2) - 1
#define NonLeafMaxKeys(n) n

#ifndef CONTEST_SETUP
int BTREE_BUFF_SIZE=20;
#else
extern int BTREE_BUFF_SIZE;
#endif
static BM_BufferPool * contestPool = NULL;

int compareValue(Value *value, char *key, DataType keyDataType);

RC _findKey(BTreeHandle *tree, Value *key, RID *result, bool getLeafNode, IntStack *pathTraversed);

RC sortAndInsert(char *data, Value *value, RID rid, PageNumber child, DataType recDataType);

RC splitNodes(PageNumber nodeToSplit, BTreeHandle *tree, Value *key, RID rid, IntStack *traversalPath);

RC initBtreePage(char *pageContent, bool isLeaf);

// Initializes Index manager
RC initIndexManager(void *mgmtData) {
    contestPool = NULL;
    return RC_OK;
}

// Shutsdown the Index manager
RC shutdownIndexManager() {
    return RC_OK;
}

/**
 * Initilizes the header of empty Btree Node
 * @param pageContent: B+Tree Node page that is to be initialized
 * @param isLeaf: Boolean to tell if the page passed in Leaf Node
 * @return RC_OK
 */
RC initBtreePage(char *pageContent, bool isLeaf) {
    BtreePageHeader *header = (BtreePageHeader *) pageContent;
    header->numRec = 0;
    if (isLeaf) {
        header->leafNode = true;
        header->nextPage = NO_PAGE;
    } else {
        header->leafNode = false;
        header->nextPage = NO_CHILD;
    }
    return RC_OK;
}

/**
 * Create a B+tree by initiliazing the metadata, and writes them to disk
 * @param idxId : File which store the indexes.
 * @param keyType : The datatype of the index
 * @param n : Maximum node size of a given B+tree
 * @return : RC_OK
 */
RC createBtree(char *idxId, DataType keyType, int n) {
    printf("\n-----BTREE_BUFF_SIZE------: %d\n", BTREE_BUFF_SIZE);
    // We assume that the metadata cannot exceed 1 page.
    // Also, we assume the node size of B+ tree doesn't exceed 1 page
    if (n > ((PAGE_SIZE - SizeofBTHeader) / SizeofBTLeaf) - 1 ||
        n > ((PAGE_SIZE - SizeofBTHeader) / SizeofBTNonLeaf) - 1) {
        return RC_IM_N_TO_LAGE;
    }

    // Create a file with name *idxId + ".bin"
    char *fileName = malloc((strlen(idxId) + 5) * sizeof(char));
    strcpy(fileName, idxId);
    strcat(fileName, ".bin");
    RC rc = createPageFile(fileName);
    if (rc != RC_OK) {
        free(fileName);
        return rc;
    }

    // Initialize buffer pool with the file created above
    BM_BufferPool *bm = MAKE_POOL();
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();

    rc = initBufferPool(bm, fileName, BTREE_BUFF_SIZE, RS_LRU, NULL);

    if (rc != RC_OK) {
        free(bm);
        free(ph);
        free(fileName);
        return rc;
    }

    int rootPage = 1;
    int numNodes = 1;
    int numRec = 0;
    // Pin the page, write the Metadata of the index to the file.
    pinPage(bm, ph, 0);
    memcpy(ph->data, &keyType, sizeof(DataType));         // Write the tree data type
    memcpy(ph->data + sizeof(DataType), &n, sizeof(int)); // Write the Node Size
    memcpy(ph->data + sizeof(DataType) + sizeof(int), &rootPage, sizeof(int)); // Write the root page number
    memcpy(ph->data + sizeof(DataType) + 2*sizeof(int), &numRec, sizeof(int)); // Write Number of records
    memcpy(ph->data + sizeof(DataType) + 3*sizeof(int), &numNodes, sizeof(int)); // Write the Number of nodes

    // Inform changes to buffer pool and shutdown
    markDirty(bm, ph);
    unpinPage(bm, ph);

    // Pin the root page, initialize the headers
    pinPage(bm, ph, rootPage);
    initBtreePage(ph->data, true);

    markDirty(bm, ph);
    unpinPage(bm, ph);
    shutdownBufferPool(bm);

    // Free the necessary data
    free(bm);
    free(ph);
    free(fileName);
    return RC_OK;
}

/**
 * Copy the metadata from file to necessary data structures.
 * If the file doesn't exist throw an error.
 * @param tree : B+tree metadata whose values are populted from the file stored in disk.
 * @param idxId : File name on which index data is stored
 * @return RC_OK if no errors else corresponding errors
 */
RC openBtree(BTreeHandle **tree, char *idxId) {

    char *fileName = malloc(sizeof(char) * (strlen(idxId) + 5));
    strcpy(fileName, idxId);
    strcat(fileName, ".bin");

    BM_BufferPool *bm = MAKE_POOL();
    contestPool = bm;
    RC rc = initBufferPool(bm, fileName, BTREE_BUFF_SIZE, RS_LRU, NULL);
    if (rc != RC_OK) {
        free(bm);
        free(fileName);
        return rc;
    }


    BM_PageHandle *ph = MAKE_PAGE_HANDLE();
    pinPage(bm, ph, 0);

    *tree = malloc(sizeof(struct BTreeHandle));
    (*tree)->idxId = idxId;
    (*tree)->mgmtData = malloc(sizeof(BtreeMgmtData));
    (*tree)->mgmtData->bm = bm;

    memcpy(&((*tree)->keyType), ph->data, sizeof(DataType));
    memcpy(&((*tree)->mgmtData->nodeSize), ph->data + sizeof(DataType), sizeof(int));
    memcpy(&((*tree)->mgmtData->rootPageNum), ph->data + sizeof(DataType) + sizeof(int), sizeof(int));
    memcpy(&((*tree)->mgmtData->numRecords), ph->data + sizeof(DataType)+ 2*sizeof(int), sizeof(int));
    memcpy(&((*tree)->mgmtData->numNodes), ph->data+sizeof(DataType)+ 3*sizeof(int), sizeof(int));

    return RC_OK;
}

/**
 * Update the statistics in the file and free all data structures used.
 *
 * @param tree: B+tree metadata
 * @return : RC_OK
 */
RC closeBtree(BTreeHandle *tree) {
    BM_BufferPool *bm = tree->mgmtData->bm;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();

    // Pin the page, write the Metadata of the index to the file.
    pinPage(bm, ph, 0);

    // Update the metadata of the tree in the file.
    memcpy(ph->data + sizeof(DataType) + sizeof(int), &(tree->mgmtData->rootPageNum), sizeof(int)); // Write the root page number
    memcpy(ph->data + sizeof(DataType) + 2*sizeof(int), &(tree->mgmtData->numRecords), sizeof(int)); // Write Number of records
    memcpy(ph->data + sizeof(DataType) + 3*sizeof(int), &(tree->mgmtData->numNodes), sizeof(int)); // Write the Number of nodes

    unpinPage(bm,ph);
    markDirty(bm,ph);
    contestPool = NULL;
    shutdownBufferPool(tree->mgmtData->bm);
    free(tree->mgmtData->bm);
    free(tree->mgmtData);
    free(tree);
    return RC_OK;
}


/**
 * Destroy the file used for index
 *
 * @param idxId: The filename that holds the index structure
 * @return : RC_OK
 */
RC deleteBtree(char *idxId) {
    char *fileName = malloc(sizeof(char) * (strlen(idxId) + 5));
    strcpy(fileName, idxId);
    strcat(fileName, ".bin");

    destroyPageFile(fileName);
    free(fileName);
    return RC_OK;
}

/**
 * Inserts a key in to the tree with given ridd
 * @param tree : B+tree metadata
 * @param key : The key to be inserted in to the tree
 * @param rid : The corresponding rid of the key
 * @return : RC_OK if no errors, corresponding RC_.. code in case of errors
 */
RC insertKey(BTreeHandle *tree, Value *key, RID rid) {
    int stackSize;
    if (tree->mgmtData->numNodes == 0) {
        stackSize = 5;
    } else {
        stackSize = (int) ceil(log(tree->mgmtData->numNodes + 1) / log(tree->mgmtData->nodeSize));
    }

    IntStack *pathTraversed = createStack(stackSize);
    RC rc;
    rc = _findKey(tree, key, NULL, true, pathTraversed);
    if (rc != RC_OK && rc != RC_IM_ROOT_EMPTY && rc != RC_IM_KEY_NOT_FOUND)
        return rc;


    PageNumber page = pop(pathTraversed);

    BtreePageHeader *pageHeader;
    BM_BufferPool *bm = tree->mgmtData->bm;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();
    bool isRoot = (page == tree->mgmtData->rootPageNum);

    rc = pinPage(bm, ph, page);
    if (rc != RC_OK) {
        free(ph);
        return rc;
    }

    pageHeader = (BtreePageHeader *) ph->data;
    assert(pageHeader->leafNode == true || isRoot);
    int maxKeys;
    if (isRoot)
        maxKeys = RootMaxKeys(tree->mgmtData->nodeSize);
    else
        maxKeys = LeafMaxKeys(tree->mgmtData->nodeSize);

    // Case 1: Leaf can accommodate the new key
    if (pageHeader->numRec < maxKeys) {
        if (!pageHeader->leafNode)
            sortAndInsert(ph->data, key, rid, NO_CHILD, tree->keyType);
        else
            sortAndInsert(ph->data, key, rid, NO_PAGE, tree->keyType);
        tree->mgmtData->numRecords += 1;
        markDirty(bm, ph);
        unpinPage(bm, ph);

    } else {
        // Case 2: Leaf full, split it.
        unpinPage(bm, ph);
        splitNodes(page, tree, key, rid, pathTraversed);
        tree->mgmtData->numRecords += 1;

    }

    free(ph);
    destroyStack(pathTraversed);
    return RC_OK;
}

/**
 * Given a node and a key to insert in to the node. It inserts key in to the node in sorted order.
 * @param data: The Node's data
 * @param value : The value to insert to the node
 * @param rid : The rid to be inserted, used only in case of leafnodes
 * @param child : In case of non-leaf node, this gives the right child for the record to be inserted.
 * @param recDataType : The Data type of tree
 * @return : RC_OK on suceess, corresponding RC_ code in case of errors
 */
RC sortAndInsert(char *data, Value *value, RID rid, PageNumber child, DataType recDataType) {
    BtreePageHeader *pageHeader = (BtreePageHeader *) data;
    BtreeLeafRec *leafRec;
    int i = 0, res;
    int totalRec = pageHeader->numRec;
    bool posFound = false;

    // If the given node is leaf node
    if (pageHeader->leafNode == true) {
        // Scan through the leaf until the record.key > value is encountered
        while (i < totalRec && !posFound) {
            leafRec = (BtreeLeafRec *) (data + SizeofBTHeader + i * SizeofBTLeaf);
            res = compareValue(value, leafRec->key, recDataType);
            switch (res) {
                case RC_IM_UNSUPPORTED_TYPE:
                case RC_IM_INCOMPATIBLE_DATA:
                    return res;
                case -1:
                    // Move all the data in the node to make space for new record to be inserted
                    memmove(data + SizeofBTHeader + (i + 1) * SizeofBTLeaf,
                            data + SizeofBTHeader + i * SizeofBTLeaf, (totalRec - i) * SizeofBTLeaf);
                    posFound = true;
                    i--;
                    break;
                default:
                    break;
            }
            i++;
        }

        // Insert the record at the desired position. Increase the number of records in the Node
        leafRec = (BtreeLeafRec *) (data + SizeofBTHeader + (i) * SizeofBTLeaf);
        leafRec->rid = rid;
        switch (recDataType) {
            case DT_INT:
                memcpy(&(leafRec->key), &(value->v.intV), sizeof(int));
                break;
            default:
                break;
        }
        pageHeader->numRec += 1;


    } else if (pageHeader->leafNode == false) {
        // If the given node is a non Leaf node

        BtreeNonLeafRec *nonLeafRec;
        i = 0;
        // Scan through all the records in the non leaf node until record.key > value.
        while (i < totalRec && !posFound) {
            nonLeafRec = (BtreeNonLeafRec *) (data + SizeofBTHeader + i * SizeofBTNonLeaf);
            res = compareValue(value, nonLeafRec->key, recDataType);
            switch (res) {
                case RC_IM_INCOMPATIBLE_DATA:
                case RC_IM_UNSUPPORTED_TYPE:
                    return res;
                case -1:
                    // Make space by moving the records at position [i..n] to pos [i+1 n+1]
                    memmove(data + SizeofBTHeader + (i + 1) * SizeofBTNonLeaf,
                            data + SizeofBTHeader + i * SizeofBTNonLeaf, (totalRec - i) * SizeofBTLeaf);
                    posFound = true;
                    i--;
                    break;
                default:
                    break;
            }
            i++;
        }
        // Insert record at position i.
        nonLeafRec = (BtreeNonLeafRec *) (data + SizeofBTHeader + i * SizeofBTNonLeaf);
        nonLeafRec->rChild = child;
        switch (recDataType) {
            case DT_INT:
                memcpy(&(nonLeafRec->key), &(value->v.intV), sizeof(int));
                break;
            default:
                break;
        }
        pageHeader->numRec += 1;
    }
    return RC_OK;
}

/**
 * Traverses the tree in search of *key, if found *result is updated. It also provides following additional:
 * If getLeafNode is set, returns after first leaf node is encountered in the search path. *result is NOT updated.!
 * If pathTraversed (A stack) is given, the page number of each node in the search path is pushed in to the stack.
 *
 * @param tree: B+tree metadata
 * @param key : The key to search for in the tree
 * @param result : If key is found, the corresponding RID is stored
 * @param getLeafNode : If set to true, returns the leaf node that might contain the key.
 * @param pathTraversed : If not null, stores the entire search path
 * @return : RC_OK if no errors, RC_IM_KEY_NOT_FOUND in case of error
 */
RC _findKey(BTreeHandle *tree, Value *key, RID *result, bool getLeafNode, IntStack *pathTraversed) {
    // IntStack stores the tree path traversed, if stack is not null.

    PageNumber currPage = tree->mgmtData->rootPageNum;
    BM_BufferPool *bm = tree->mgmtData->bm;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();

    BtreePageHeader *header;
    RC rc;
    int nRec, i, res;

    // Start from the root and traverse till we hit a leaf node
    while (true) {
        // Pin the current Page
        rc = pinPage(bm, ph, currPage);
        if (rc != RC_OK) {
            free(ph);
            return rc;
        }
        if (pathTraversed != NULL)
            push(pathTraversed, currPage);

        // If the page has no data, return no KEY_NOT_FOUND
        header = (BtreePageHeader *) ph->data;
        if (header->numRec == 0) {
            free(ph);
            return RC_IM_KEY_NOT_FOUND;
        }

        // If we hit a leaf Node, break out
        if (header->leafNode)
            break;

        nRec = header->numRec;
        i = 0;
        bool continue_ = true;
        PageNumber lchild = header->nextPage;

        // For each record.key in the node, compare it to key given to us
        while (i <= nRec - 1 && continue_ && !header->leafNode) {
            BtreeNonLeafRec *nonLeafRec = (BtreeNonLeafRec *) (ph->data + SizeofBTHeader + (i * SizeofBTNonLeaf));
            res = compareValue(key, nonLeafRec->key, tree->keyType);

            switch (res) {
                case RC_IM_UNSUPPORTED_TYPE:
                case RC_IM_INCOMPATIBLE_DATA:
                    return res;
                case 1:
                    // if key > record.key: read the next node in the page

                    // If at the end of the node and the given key is higher than the last record,
                    // traverse the right child
                    if (i == nRec - 1) {
                        continue_ = false;
                        unpinPage(bm, ph);
                        currPage = nonLeafRec->rChild;
                        if (currPage == NO_CHILD) {
                            return RC_IM_KEY_NOT_FOUND;
                        }
                        break;
                    }
                    // Right child of current record is the left child of next record.!
                    // We need this in the next iteration, as we might traverse the left child.
                    lchild = nonLeafRec->rChild;
                    break;
                case 0:
                    // if key == record.key: Read records from the right node
                    continue_ = false;
                    unpinPage(bm, ph);
                    currPage = nonLeafRec->rChild;
                    if (currPage == NO_CHILD) {
                        return RC_IM_KEY_NOT_FOUND;
                    }
                    break;

                case -1:
                    // if key < record.key: Read records from the left node
                    continue_ = false;
                    unpinPage(bm, ph);
                    currPage = lchild;
                    if (currPage == NO_CHILD) {
                        return RC_IM_KEY_NOT_FOUND;
                    }
                    break;
                default:
                    printf("Comparator returned Unknown value");
                    return RC_IM_KEY_NOT_FOUND;
            }
            i++;
        }
    }

    // If user only asks for leaf page
    if (getLeafNode) {

        unpinPage(bm, ph);
        free(ph);
        return RC_OK;
    }


    // Note the above traversal would always end with leaf Node.
    // Now traverse all the records in the leaf node.
    int numRec = header->numRec;
    BtreeLeafRec *leafRec;
    bool found = false;
    i = 0;
    while (i < numRec) {
        leafRec = (BtreeLeafRec *) (ph->data + SizeofBTHeader + (SizeofBTLeaf * i));
        res = compareValue(key, leafRec->key, tree->keyType);
        if (res == 0) {
            found = true;
            break;
        }
        i++;
    }

    unpinPage(bm, ph);
    free(ph);

    if (found) {
        result->page = leafRec->rid.page;
        result->slot = leafRec->rid.slot;
        return RC_OK;
    }

    // If the key is not in the leaf then the tree doesn't have the  key.
    return RC_IM_KEY_NOT_FOUND;

}

/**
 * Searches the B+tree for the key and stores the RID of the key in the result.
 * @param tree : B+tree meta data.
 * @param key : The key to search for in the tree.
 * @param result : The RID of the key.
 * @return : RC_OK if key found in the tree, if not RC_IM_KEY_NOT_FOUND.
 */
RC findKey(BTreeHandle *tree, Value *key, RID *result) {
    return _findKey(tree, key, result, false, NULL);

}

/**
 * Returns the number of nodes in the Btree
 * @param tree : Btree Meta data
 * @param result : Number of Nodes in the Btree
 * @return : RC_OK
 */
RC getNumNodes(BTreeHandle *tree, int *result) {
    *result = tree->mgmtData->numNodes;
    return RC_OK;
}
/**
 * Return the number of records in the tree
 * @param tree : Btree Meta data
 * @param result: Number of records in Btree
 * @return RC_OK
 */
RC getNumEntries(BTreeHandle *tree, int *result) {
    *result = tree->mgmtData->numRecords;
    return RC_OK;
}

/**
 * stores the datatype of the tree in result.
 * @param tree : The tree metadata.
 * @param result : The data type of the tree
 * @return RC_OK
 */
RC getKeyType(BTreeHandle *tree, DataType *result) {
    *result = tree->keyType;
    return RC_OK;
}

/**
 * Deletes the given key from the tree.
 * @param tree: The tree Metadata
 * @param key : Key to be deleted from the tree
 * @return : RC_OK if entry found, else RC_IM_KEY_NOT_FOUND
 */
RC deleteKey(BTreeHandle *tree, Value *key) {
    RC rc;
    IntStack *pathTraversed = createStack(10);
    rc = _findKey(tree,key,NULL,true,pathTraversed);
    if(rc != RC_OK)
        return rc;
    BM_BufferPool *bm = tree->mgmtData->bm;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();

    PageNumber leaf = pop(pathTraversed);
    pinPage(bm,ph, leaf);

    BtreePageHeader *leafHeader  = (BtreePageHeader *) ph->data;
    int numRec = leafHeader->numRec;
    int i=0; BtreeLeafRec *leafRec;
    int res; bool found=false;

    while (i < numRec && !found){
        leafRec = (BtreeLeafRec *) (ph->data + SizeofBTHeader + (i * SizeofBTLeaf));
        res = compareValue(key, leafRec->key, tree->keyType);

        switch (res){
            case 0:
                memmove(ph->data+SizeofBTHeader+i*SizeofBTLeaf, ph->data+SizeofBTHeader+(i+1)*SizeofBTLeaf, (numRec-i-1)*SizeofBTLeaf);
                leafHeader->numRec -= 1;
                tree->mgmtData->numRecords -=1;
                markDirty(bm,ph);
                found = true;
                break;
            case -1:
                unpinPage(bm,ph);
                free(ph);
                destroyStack(pathTraversed);
                return RC_IM_KEY_NOT_FOUND;
            default:
                break;
        }

        i++;
    }

    unpinPage(bm,ph);
    free(ph);
    destroyStack(pathTraversed);
    return RC_OK;
}

/**
 * Open Scan will traverse to the left most child in the tree and store the necessary metadata.
 * This metadata is later re-used by in the nextEntry function.
 *
 * @param tree: Tree Meta data
 * @param handle : Holds the necessary metadata needed for scanning. This is filled in by this function
 * @return : RC_OK
 */
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
    *handle = malloc(sizeof(BT_ScanHandle));
    (*handle)->mgmtData = malloc(sizeof(BtreeScanMgmt));
    (*handle)->tree = tree;

    PageNumber currNode;
    PageNumber lChild;
    bool isLeaf = false;
    BM_BufferPool* bm = tree->mgmtData->bm;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();
    BtreePageHeader *pageHeader;

    // Starting from the root, traverse to left most child.
    currNode = tree->mgmtData->rootPageNum;

    while (true){
        pinPage(bm,ph,currNode);
        pageHeader = (BtreePageHeader *) ph->data;
        if(pageHeader->leafNode){
            break;
        }
        currNode = pageHeader->nextPage;
        unpinPage(bm,ph);
    }
    unpinPage(bm,ph);
    // Store the left most child's page number.
    (*handle)->mgmtData->currNode = currNode;
    (*handle)->mgmtData->currRecord = 0;

    free(ph);
    return RC_OK;
}

/**
 * Get the next record from the currentNode.
 * If traversing the node is complete move on to the next Leaf and start from record 0.
 * @param handle : Metadata about the scan in progress
 * @param result : If next entry exists, the RID is stored in the result.
 * @return RC_OK if next entry exists, if not RC_IM_NO_MORE_ENTRIES
 */
RC nextEntry(BT_ScanHandle *handle, RID *result) {
    PageNumber currNode;
    int currRecord;
    currNode = handle->mgmtData->currNode;
    currRecord = handle->mgmtData->currRecord;

    BM_BufferPool *bm = handle->tree->mgmtData->bm;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();


    pinPage(bm,ph,currNode);
    BtreePageHeader *header = (BtreePageHeader *) ph->data;
    BtreeLeafRec *leafRec;
    // If current node has records, continue reading
    if (currRecord < header->numRec){
        handle->mgmtData->currRecord +=1;
    }
    else{
    // If current node is read completely, Move on to the next page.
        if(header->nextPage == NO_PAGE){
            unpinPage(bm,ph);
            free(ph);
            return RC_IM_NO_MORE_ENTRIES;
        }
        handle->mgmtData->currNode = header->nextPage;
        handle->mgmtData->currRecord = 0;
        unpinPage(bm,ph);

        currNode = handle->mgmtData->currNode;
        currRecord = handle->mgmtData->currRecord;

        handle->mgmtData->currRecord += 1;
        pinPage(bm,ph,currNode);
    }

    leafRec = (BtreeLeafRec *) (ph->data + SizeofBTHeader + currRecord * SizeofBTLeaf);
    memcpy(result, &(leafRec->rid), sizeof(RID));

    unpinPage(bm,ph);
    free(ph);
    return RC_OK;
}
/**
 * Closes the tree scan and frees allocated memory
 * @param handle : Scan Metadata to be destroyed
 * @return RC_OK after free'ing the memory
 */
RC closeTreeScan(BT_ScanHandle *handle) {
    free(handle->mgmtData);
    free(handle);
    return RC_OK;
}

char *printTree(BTreeHandle *tree) {
    return NULL;
}

long getIMNumIO() {
    if(contestPool == NULL)
        return -1;
    return getNumWriteIO(contestPool) + getNumReadIO(contestPool);
}


/* Compares Value with char* and the return values are as described below.
 *
 * if      value > key return 1
 * else if value == key return 0
 * else if value < key return -1
 */
int compareValue(Value *value, char *key, DataType keyDataType) {

    if (value->dt != keyDataType) {
        return RC_IM_INCOMPATIBLE_DATA;
    }

    char *end;
    int key_int;
    switch (value->dt) {
        case DT_INT:
            memcpy(&key_int, &key, sizeof(int));
            if (key_int > value->v.intV)
                return -1;
            else if (key_int == value->v.intV)
                return 0;
            else
                return 1;

        default:
            return RC_IM_UNSUPPORTED_TYPE;
    }

}

/**
 * When the leaf Node is full, We call this function.
 * Leaf splits and  subsequent non-Leaf are handled in this code.
 * @param nodeToSplit : Page number of the leaf node which is to be split
 * @param tree : The Data structure that holds the metadata of the Btree.
 * @param key : The key that is to be inserted into the tree.
 * @param rid : The RID of the the record
 * @param traversalPath : Stack that contains all the nodes traversed.
 * @return Appropriate RC_IM_... code for errors else RC_OK
 */
RC splitNodes(PageNumber nodeToSplit, BTreeHandle *tree, Value *key, RID rid, IntStack *traversalPath) {
    BM_BufferPool *bm = tree->mgmtData->bm;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();
    BM_PageHandle *newPh = MAKE_PAGE_HANDLE();

    bool isRoot = (nodeToSplit == tree->mgmtData->rootPageNum);
    pinPage(bm, ph, nodeToSplit);

    BtreePageHeader *header = (BtreePageHeader *) ph->data;
    // Temporarily overflow the node.
    sortAndInsert(ph->data, key, rid, NO_PAGE, tree->keyType);
    markDirty(bm, ph);

    // Determine the split point
    int splitAt = (int) ceil(header->numRec / 2.0);


    //Get a new page from the pool
    int newPageNum = getNumPagesInFile(bm);
    pinPage(bm, newPh, newPageNum);

    // Initialize the headers
    BtreePageHeader *newPageHeader = (BtreePageHeader *) newPh->data;
    initBtreePage(newPh->data, true);

    // copy the contents of the node from splitAt till the end of node to the new node
    memcpy(newPh->data + SizeofBTHeader,
           ph->data + SizeofBTHeader + splitAt * SizeofBTLeaf,
           (header->numRec - splitAt) * SizeofBTLeaf);

    // Update the headers of the new node
    newPageHeader->numRec = header->numRec - splitAt;
    newPageHeader->nextPage = header->nextPage;

    // Update the overflown node's header
    header->numRec = splitAt;
    header->nextPage = newPageNum;

    // A new node is added, update the tree Metadata
    tree->mgmtData->numNodes += 1;

    assert(header->numRec >= LeafMinKeys(tree->mgmtData->nodeSize));
    assert(newPageHeader->numRec >= LeafMinKeys(tree->mgmtData->nodeSize));

    // Determine the data to be inserted in the parent. i.e., the minimum of right node in the split
    BtreeNonLeafRec *parentData = (BtreeNonLeafRec *) (newPh->data + SizeofBTHeader);
    Value *toInsertInparent = malloc(sizeof(Value));
    toInsertInparent->dt = tree->keyType;

    switch (tree->keyType) {
        case DT_INT:
            memcpy(&(toInsertInparent->v.intV), &(parentData->key), sizeof(int));
            break;
        default:
            printf("Unknown Datatype");
            break;
    }


    markDirty(bm, ph);
    markDirty(bm, newPh);
    unpinPage(bm, ph);
    unpinPage(bm, newPh);

    BM_PageHandle *parentNode = MAKE_PAGE_HANDLE();
    PageNumber newRoot, parent;
    BtreePageHeader *parentHeader;
    while (true) {
        // Check if the node to split is root.
        // If root is to be split, create a new page, initialize it.
        if (isRoot) {
            parent = getNumPagesInFile(bm);
            tree->mgmtData->rootPageNum = parent;
            pinPage(bm, parentNode, parent);
            initBtreePage(parentNode->data, false);
            parentHeader = (BtreePageHeader *) parentNode->data;
            parentHeader->nextPage = nodeToSplit;
            tree->mgmtData->numNodes += 1;
            markDirty(bm, parentNode);
        } else {
            // Find the parent in which we need to insert. This insert is a result of split
            parent = pop(traversalPath);
            pinPage(bm, parentNode, parent);
        }

        isRoot = (parent == tree->mgmtData->rootPageNum);

        parentHeader = (BtreePageHeader *) parentNode->data;
        RID dummyRid;

        // Case 2: Parent is not full
        if (parentHeader->numRec < NonLeafMaxKeys(tree->mgmtData->nodeSize)) {
            sortAndInsert(parentNode->data, toInsertInparent, dummyRid, newPageNum, tree->keyType);
            assert(parentHeader->numRec <= NonLeafMaxKeys(tree->mgmtData->nodeSize));
            assert(parentHeader->numRec >= NonLeafMinKeys(tree->mgmtData->nodeSize)||isRoot);
            break;
        } else {
            // Case 3: Parent is full. Results in cascading splits until we find a empty node or till root is split.
            // Temporarily overflow the parent node
            sortAndInsert(parentNode->data, toInsertInparent, dummyRid, newPageNum, tree->keyType);

            // Compute the Split the parent
            splitAt = (int) floor(parentHeader->numRec / 2);

            // Create a new node, Initialize the headers
            newPageNum = getNumPagesInFile(bm);
            pinPage(bm, ph, newPageNum);
            initBtreePage(ph->data,false);
            header = (BtreePageHeader *) ph->data;

            // Copy the contents to the new Node.
            memcpy(ph->data + SizeofBTHeader, parentNode->data + SizeofBTHeader + (splitAt + 1) * SizeofBTNonLeaf,
                   (parentHeader->numRec - splitAt - 1) * SizeofBTNonLeaf);
            header->numRec = (parentHeader->numRec - splitAt - 1);

            parentData = (BtreeNonLeafRec *) (parentNode->data + SizeofBTHeader + (splitAt) * SizeofBTNonLeaf);
            header->nextPage = parentData->rChild;
            parentHeader->numRec = splitAt;

            // The necessary data for the cascading split.
            nodeToSplit =  parent;
            toInsertInparent->dt = tree->keyType;
            memcpy(&(toInsertInparent->v.intV), &(parentData->key), sizeof(int));
            tree->mgmtData->numNodes += 1;

        }
    }
    free(toInsertInparent);
    free(ph);
    free(newPh);
    free(parentNode);
    return RC_OK;
}
