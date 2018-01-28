#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include "dberror.h"

/************************************************************
 *                    handle data structures                *
 ************************************************************/
// This struct is used to store addition book-keeping info.
// For now this only has file handle. could be updated in future.
typedef struct SM_MgmtInfo{
    FILE* fd;

} SM_MgmtInfo;

typedef struct SM_FileHandle {
  char *fileName;
  int totalNumPages;
  int curPagePos;
  SM_MgmtInfo *mgmtInfo;
} SM_FileHandle;

typedef char* SM_PageHandle;

/************************************************************
 *                    interface                             *
 ************************************************************/
/* manipulating page files */
extern void initStorageManager (void);
extern RC createPageFile (char *fileName);
extern RC openPageFile (char *fileName, SM_FileHandle *fHandle);
extern RC closePageFile (SM_FileHandle *fHandle);
extern RC destroyPageFile (char *fileName);

/* reading blocks from disc */
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage);
extern int getBlockPos (SM_FileHandle *fHandle);
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);

/* writing blocks to a page file */
extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC appendEmptyBlock (SM_FileHandle *fHandle);
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle);

// Get total number of pages in File
extern int getNumPages (SM_FileHandle *fHandle);
#endif
