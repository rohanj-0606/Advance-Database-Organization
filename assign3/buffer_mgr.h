#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

// Include return codes and methods for logging errors
#include "dberror.h"
#include "storage_mgr.h"
#include "hash_table.h"
#include "free_list.h"

// Include bool DT
#include "dt.h"

// Replacement Strategies
typedef enum ReplacementStrategy {
    RS_FIFO = 0,
    RS_LRU = 1,
    RS_CLOCK = 2,
    RS_LFU = 3,
    RS_LRU_K = 4
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1
#define NOT_IN_BUF -1;


/**
 * The buffer pool has N slots in it.
 * For each slot we maintain the following data structure.
 *
 * buff_id    : Each slot in the buffer pool is uniquely identified by the buff_id.
 *              It starts from 0
 * pageNumber : The pageNumber that the slot holds.
 * dirtyPage  :  Indicates whether the page is dirty, i.e, if there are updates are not written to disk yet.
 * pinned     : Indicates if the page in this slot is pinned by user.
 */
typedef  struct  BM_BufferHeader{
    unsigned int buff_id;
    PageNumber pageNumber;
    bool dirtyPage;
    bool pinned;

}BufferHeader;

/*
 * DataStructure to hold the statistics of buffer pool.
 * All stats are maintained from the begining of buffer pool initialization
 *
 * num_reads_disk   : Number of read requests that needed to hit the disk.
 * num_writes_disk  : Number of writes made to the disk.
 * num_buff_hits    : Number of read requests that did not need disk I/O.
 */
typedef struct BM_BufferStatistics{
    unsigned int num_reads_disk;
    unsigned int num_writes_disk;
    unsigned int num_buff_hits;
}BufferStats;


/*
 * This structure holds book-keeping information for the buffer pool
 *
 * fHandle          : File handle from which buffer manager reads/Writes
 * buffPoolAddr     : Holds the pointer to the starting address in memory where the buffer pool stores the pages.
 * buffPoolHeaders  : Pointer to array of headers of length equal to number of slots in buffer pool
 * buffStats        : Holds statistics of the buffer pool
 * buffTable        : Maps PageNumber to buffer slot
 * freeBuffList     : List of empty buffers in Buffer pool
 * fixCount         : Array of size equal to number of buffer slots, when tells how many clients are using this page.
 * strategyData     : Pointer to data that would be needed by the Page replacement strategy
 */
typedef struct BM_MgmtData {
    SM_FileHandle *fHandle;
    char *buffPoolAddr;
    BufferHeader* buffPoolHeaders;
    BufferStats buffStats;
    HashTable * buffTable;
    FreeList * freeBuffList;
    int * fixCount;
    void * strategyData;
} BM_MgmtData;


typedef struct BM_BufferPool {
    char *pageFile;
    int numPages;
    ReplacementStrategy strategy;
    BM_MgmtData *mgmtData; // use this one to store the bookkeeping info your buffer
    // manager needs for a buffer pool
} BM_BufferPool;


typedef struct BM_PageHandle {
    PageNumber pageNum;
    char *data;
} BM_PageHandle;

// convenience macros
#define MAKE_POOL()                    \
  ((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()                \
  ((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy,
                  void *stratData);

RC shutdownBufferPool(BM_BufferPool *const bm);

RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page);

RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page);

RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page);

RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
           const PageNumber pageNum);

// Statistics Interface
PageNumber *getFrameContents(BM_BufferPool *const bm);

bool *getDirtyFlags(BM_BufferPool *const bm);

int *getFixCounts(BM_BufferPool *const bm);

int getNumReadIO(BM_BufferPool *const bm);

int getNumWriteIO(BM_BufferPool *const bm);

int getNumPagesInFile(BM_BufferPool *const bm);
#endif
