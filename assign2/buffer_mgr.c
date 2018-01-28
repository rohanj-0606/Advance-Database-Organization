#include "buffer_mgr.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/*
 * Initalize BM_BufferPool with appropriate info.
 * Allocate memory for the buffer pool and store the pointer.
 */
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages,
                  ReplacementStrategy strategy, void *stratData) {

    SM_FileHandle *fHandle = malloc(sizeof(SM_FileHandle));
    char* fileName = malloc((strlen(pageFileName)+1)* sizeof(char));
    strcpy(fileName, pageFileName);

    RC rc = openPageFile(fileName, fHandle);

    if (rc != RC_OK) {
        free(fHandle);
        return RC_FILE_NOT_FOUND;
    }

    bm->strategy = strategy;
    bm->numPages = numPages;
    bm->pageFile = fileName;
    bm->mgmtData = malloc(sizeof(BM_MgmtData));
    bm->mgmtData->fHandle = fHandle;
    bm->mgmtData->buffPoolAddr = malloc(PAGE_SIZE * numPages * sizeof(char));
    bm->mgmtData->buffPoolHeaders = malloc(numPages * sizeof(BufferHeader));
    bm->mgmtData->buffTable = createHashTable((size_t)pow(2, (double)(log(2*numPages)/ log(2))));
    bm->mgmtData->fixCount = malloc(sizeof(int)* numPages);
    bm->mgmtData->freeBuffList = createFreeList();

    if(strategy == RS_FIFO|| strategy == RS_LRU){
        bm->mgmtData->strategyData = createFreeList();
    }

    bm->mgmtData->buffStats.num_buff_hits = 0;
    bm->mgmtData->buffStats.num_reads_disk = 0;
    bm->mgmtData->buffStats.num_writes_disk = 0;

    unsigned int i;
    for (i = 0; i < numPages; ++i) {
        bm->mgmtData->buffPoolHeaders[i].buff_id = i;
        bm->mgmtData->buffPoolHeaders[i].pageNumber = NO_PAGE;
        bm->mgmtData->buffPoolHeaders[i].dirtyPage = FALSE;
        bm->mgmtData->buffPoolHeaders[i].pinned = FALSE;
        bm->mgmtData->fixCount[i] = 0;
        //insertFreeNode(bm->mgmtData->freeBuffList, i);
        insertFreeNode(bm->mgmtData->freeBuffList,i);
    }

    return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm) {
    // Flush all dirty pages to disk
    RC rc = forceFlushPool(bm);
    if (rc != RC_OK) {
        return RC_FLUSH_FAILED;
    }

    // Check for pinned pages. Throw error if there any of the pages are pinned.
    size_t i;
    for (i = 0; i < bm->numPages; i++) {
        if (bm->mgmtData->buffPoolHeaders[i].pinned) {
            return RC_BUFF_SHUT_FAILED;

        }
    }

    //close the file
    rc = closePageFile(bm->mgmtData->fHandle);
    if(rc != RC_OK){
        return rc;
    }

    // Free all allocated data
    free(bm->pageFile);
    free(bm->mgmtData->buffPoolAddr);
    free(bm->mgmtData->buffPoolHeaders);
    free(bm->mgmtData->fixCount);
    free(bm->mgmtData->fHandle);
    destroyHashTable(bm->mgmtData->buffTable);
    destroyFreeList(bm->mgmtData->freeBuffList);
    destroyFreeList(bm->mgmtData->strategyData);
    free(bm->mgmtData);
    return RC_OK;
}


// FLush the enitre buffer Pool
RC forceFlushPool(BM_BufferPool *const bm) {
    size_t i;
    BM_PageHandle *pHandle = malloc(sizeof(BM_PageHandle));
    pHandle->data = NULL;
    RC rc;
    // Iterate through all bufferHeaders, check if there is a dirty page.
    //      if it is a dirty page, flush the page
    for (i = 0; i < bm->numPages; ++i) {
            if (bm->mgmtData->buffPoolHeaders[i].dirtyPage){
                pHandle->pageNum = bm->mgmtData->buffPoolHeaders[i].pageNumber;
                assert(bm->mgmtData->buffPoolHeaders[i].pageNumber >= 0);
                rc = forcePage(bm,pHandle);

                if(rc<0){
                    free(pHandle);
                    return RC_FLUSH_FAILED;
                }
            }
    }
    free(pHandle);
    return RC_OK;
}

// Mark the page dirty
//  We expect the page to be in buffer, if not throw error.
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {

    if(page == NULL){
        return RC_DIRTY_FAILED;
    }

    HashTable* hTable = bm->mgmtData->buffTable;

    // Search page in hashTable
    int buffId = searchHashTable(hTable,page->pageNum);
    if (buffId<0){
        printf("Trying to mark page dirty, But page not in buffer.?!\n");
        return RC_DIRTY_FAILED;
    }
    bm->mgmtData->buffPoolHeaders[buffId].dirtyPage = TRUE;

    return RC_OK;
}

/* 
    Give the requested data block to the client.
    If the requested block is not in the buffer, fetch from the disk and load in buffer.
    Also pin the pageFrame  and update the stats.
*/
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {
    HashTable* hTable = bm->mgmtData->buffTable;

    // Search the page in hash table
    int buffId = searchHashTable(hTable, pageNum);
    BufferHeader* buffHead;

    char * buffPool;
    buffPool = bm->mgmtData->buffPoolAddr;
    ListNode* node;

    /*
     * If page is not in buffer:
     *      Check for empty buffer slot.
     *      if No empty slot
     *          Invoke page replacement strategy, get the buffer slot to evict.
     *      read the page in to buffer slot.
     *
    */

    if (buffId <  0){
        // Check for empty slot in buffer
        node = getFreeNode(bm->mgmtData->freeBuffList);

        if(node == NULL){
                // Buffer full, Invoke PageFrame replacement strategy
            if(bm->strategy == RS_FIFO || bm->strategy == RS_LRU){
                node = getListHead(bm->mgmtData->strategyData);
                if(node == NULL)
                    return -1;

                while (node) {
                    buffId = node->buff_id;
                    if (bm->mgmtData->buffPoolHeaders[buffId].pinned || bm->mgmtData->fixCount[buffId] > 0) {
                        node = node->next;
                    } else {
                        deleteAppendListNode(bm->mgmtData->strategyData, node);
                        break;
                    }
                }

            }

            // Strategy gave us a buffer frame, if it is dirty flush it, before replacing it.
            buffHead = &(bm->mgmtData->buffPoolHeaders[buffId]);
            BM_PageHandle *pHand = malloc(sizeof(BM_PageHandle));

            pHand->data = NULL;
            pHand->pageNum = buffHead->pageNumber;
            if(buffHead->dirtyPage)
                forcePage(bm, pHand);

            free(pHand);

        }
        else{
            if(bm->strategy == RS_FIFO|| bm->strategy == RS_LRU)
                insertListNode(bm->mgmtData->strategyData,node);

            buffId = node->buff_id;
        }


        //Read the page from disk to buffer
        RC rc = readBlock(pageNum, bm->mgmtData->fHandle, &buffPool[buffId*PAGE_SIZE]);
        if(rc !=RC_OK){
            return rc;
        }
        //printf("BuffId: %d \n", buffId);

        // Delete the old page mapping in buffTable.
        // Insert the new page mapping in buffTable. Done with single call to delsert (Both del and ins are done here)
        delsertHashNode(bm->mgmtData->buffTable,bm->mgmtData->buffPoolHeaders[buffId].pageNumber, pageNum, buffId);

        bm->mgmtData->buffStats.num_reads_disk +=1;

    }
    else{
        if(bm->strategy == RS_LRU){
            deleteAppendListData(bm->mgmtData->strategyData,buffId);
        }
        bm->mgmtData->buffStats.num_buff_hits += 1;
    }




    buffHead = &(bm->mgmtData->buffPoolHeaders[buffId]);
    // pin the buffer,update the fix count, update Statistics.
    buffHead->pinned = TRUE;
    buffHead->pageNumber = pageNum;
    bm->mgmtData->fixCount[buffId] += 1;

    assert(bm->mgmtData->buffPoolHeaders[buffId].pageNumber >= 0);
    //Fill in PageHandle and return
    page->pageNum = buffHead->pageNumber;
    page->data = &(buffPool[buffId * PAGE_SIZE]);



    return RC_OK;
}

/*
 * Unpin the buffer and reduce the Fix count
 */
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    HashTable *hTable = bm->mgmtData->buffTable;
    int buffId = searchHashTable(hTable, page->pageNum);

    if(buffId < 0){
        printf("Cannot Unpin page as it is not in buffer");
        return RC_UNPIN_FAILED;
    }

    bm->mgmtData->buffPoolHeaders[buffId].pinned = FALSE;
    assert(bm->mgmtData->buffPoolHeaders[buffId].pageNumber >= 0);
    if (bm->mgmtData->fixCount[buffId] > 0) {
        bm->mgmtData->fixCount[buffId] -= 1;
    }
    return RC_OK;
}

/*
 * Flush the page to the disk.
 * update the stats.
 * Mark the page non-dirty if Fix count is zero.
 */
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    if(page == NULL){
        return RC_FLUSH_FAILED;
    }

    int buff_id = searchHashTable(bm->mgmtData->buffTable, page->pageNum);
    if(buff_id < 0){
        printf("The page is not in buffer, Cannot flush it.!");
        return RC_FLUSH_FAILED;
    }

    SM_PageHandle data = calloc(PAGE_SIZE, sizeof(char));
    strcpy(data, &(bm->mgmtData->buffPoolAddr[buff_id*PAGE_SIZE]));
    RC rc = writeBlock(page->pageNum, bm->mgmtData->fHandle, data);

    free(data);
    if (rc != RC_OK){
        printf("Storage Manager couldn't write to disk.");
        return RC_FLUSH_FAILED;
    }

    bm->mgmtData->buffStats.num_writes_disk +=1;
    if(bm->mgmtData->fixCount[buff_id] == 0){
        bm->mgmtData->buffPoolHeaders[buff_id].dirtyPage = FALSE;
    }

    return RC_OK;
}

PageNumber *getFrameContents(BM_BufferPool *const bm) {
    PageNumber * pageNbrArr = malloc(sizeof(PageNumber)*bm->numPages);

    for(int i=0; i< bm->numPages; i++) {
       pageNbrArr[i] = bm->mgmtData->buffPoolHeaders[i].pageNumber;

    }
    return pageNbrArr;
}

bool *getDirtyFlags(BM_BufferPool *const bm) {
    bool* flagArr = malloc(sizeof(bool)* bm->numPages);

    for (int i = 0; i < bm->numPages; ++i) {
        flagArr[i] = bm->mgmtData->buffPoolHeaders[i].dirtyPage;
    }
    return flagArr;
}

int *getFixCounts(BM_BufferPool *const bm) {
    int * arr = malloc(sizeof(int)*bm->numPages);
    for (int i = 0; i < bm->numPages; ++i) {
       arr[i] = bm->mgmtData->fixCount[i];
    }
    return arr;
}

int getNumReadIO(BM_BufferPool *const bm) {
    return bm->mgmtData->buffStats.num_reads_disk;
}

int getNumWriteIO(BM_BufferPool *const bm) {
    return bm->mgmtData->buffStats.num_writes_disk;
}

