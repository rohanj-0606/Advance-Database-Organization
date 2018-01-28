#include "storage_mgr.h"
#include <stdlib.h>
#include <errno.h>
#include <error.h>

// Storage manager Initialization
//  Reserved for future use
void initStorageManager(void) {
    printf("\nStorage Manager");
}

// Creates a pagefile with name given in *filename.
//  Add an empty page of size PAGE_SIZE to the file.
RC createPageFile(char *filename) {
    FILE *fp;
    fp = fopen(filename, "w");
    if (fp == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    size_t i;
    for (i = 0; i < PAGE_SIZE; i++) {
        fprintf(fp, "%c", '\0');
    }

    fclose(fp);
    return RC_OK;
}

// Open pageFile with name *fileName and store book-keeping details in *fHandle
RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    FILE *fp;

    fp = fopen(fileName, "r+");
    if (fp == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    //struct stat st;
    //int rc = stat(fileName, &st);
    int rc = fseek(fp, 0, SEEK_END);
    if (rc < 0) {
        return RC_OPEN_FAILED;
    }

    //fHandle->totalNumPages = (int) (st.st_size / PAGE_SIZE);
    fHandle->totalNumPages = (int) (ftell(fp) / PAGE_SIZE);

    rc = fseek(fp, 0, SEEK_SET);
    if (rc < 0) {
        return RC_OPEN_FAILED;
    }

    fHandle->curPagePos = 0;
    fHandle->fileName = fileName;
    fHandle->mgmtInfo = malloc(sizeof(SM_MgmtInfo));
    fHandle->mgmtInfo->fd = fp;

    return RC_OK;

}

// closePage file with file handle pointed by the fHandle.
RC closePageFile(SM_FileHandle *fHandle) {

    if (fHandle->mgmtInfo->fd == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    fflush(fHandle->mgmtInfo->fd);
    int rc = fclose(fHandle->mgmtInfo->fd);

    if (rc == 0) {
        free(fHandle->mgmtInfo);
        return RC_OK;
    } else {
        return RC_FILE_NOT_FOUND;
    }
}

// Delete the pageFile with name fileName
RC destroyPageFile(char *fileName) {
    if (remove(fileName) != 0) {
        switch (errno){

            case EACCES:
                printf("Write permission is denied for the directory from which the file is to be removed, or the directory has the sticky bit set and you do not own the file.");
                break;
            case EBUSY:
                printf("This error indicates that the file is being used by the system in such a way that it can’t be unlinked. For example, you might see this error if the file name specifies the root directory or a mount point for a file system.");
                break;
            case ENOENT:
                printf("The file name to be deleted doesn’t exist.");
                break;
            case EPERM:
                printf("On some systems unlink cannot be used to delete the name of a directory, or at least can only be used this way by a privileged user. To avoid such problems, use rmdir to delete directories. (On GNU/Linux and GNU/Hurd systems unlink can never delete the name of a directory.)");
                break;
            case EROFS:
                printf("The directory containing the file name to be deleted is on a read-only file system and can’t be modified.");
                break;
            default:
                printf("Unknown error deleting file");

        }
        return RC_FILE_NOT_FOUND;
    } else {
        return RC_OK;
    }
}


// Read a page with number pageNum, from file pointed by fHandle.
// Store the page data in the buffer memPage.
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (pageNum < 0 || pageNum > ((fHandle->totalNumPages)) - 1) {
        return RC_READ_FAILED;
    }

    if (fHandle->curPagePos != pageNum) {
        long offSet = pageNum * PAGE_SIZE;
        int rc = fseek(fHandle->mgmtInfo->fd, offSet, SEEK_SET);

        if (rc != 0) {
            return RC_READ_FAILED;
        }
        fHandle->curPagePos = pageNum;
    }

    size_t num_items = fread(memPage, PAGE_SIZE, 1, fHandle->mgmtInfo->fd);
    if (num_items != 1) {
        return RC_READ_FAILED;
    }
    fHandle->curPagePos = pageNum + 1;
    return RC_OK;
}

// Get the page number to which the fHandle is currently pointing.
int getBlockPos(SM_FileHandle *fHandle) {
    if (fHandle == NULL) {
        return -1;
    }
    return fHandle->curPagePos;
}

// Read the page 0 from the file pointed by fHandle and store the data in memPage
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    RC rc = readBlock(0, fHandle, memPage);
    return rc;
}

// Read page previous to current page (i.e., page the fHandle is pointing to) and store it in memPage
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int curr_page = fHandle->curPagePos;
    if (curr_page - 1 < 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }
    RC rc = readBlock(curr_page - 1, fHandle, memPage);
    return rc;
}

// Read the current page pointed to by the fHandle and store it in memPage
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    RC rc = readBlock(fHandle->curPagePos, fHandle, memPage);
    return rc;
}

// Read next page to current and store it in memPage
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int curr_page = fHandle->curPagePos;
    if (curr_page + 1 > (fHandle->totalNumPages) - 1) {
        return RC_READ_NON_EXISTING_PAGE;
    }
    RC rc = readBlock(curr_page + 1, fHandle, memPage);
    return rc;
}

// Read the Last block in the file
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int last_page = (fHandle->totalNumPages) - 1;
    RC rc = readBlock(last_page, fHandle, memPage);
    return rc;

}

// Write to the page of number 'pageNum' with data in memPage.
// The file to write is given by fHandle
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {

    if (fHandle->mgmtInfo->fd == NULL) {
        return RC_WRITE_FAILED;
    } else if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }


    if (fHandle->curPagePos != pageNum) {

        int offset = pageNum * PAGE_SIZE;
        int rc = fseek(fHandle->mgmtInfo->fd, offset, SEEK_SET);
        if (rc < 0) {
            return RC_WRITE_FAILED;
        }
    }

    size_t num_items = fwrite(memPage, PAGE_SIZE, 1, fHandle->mgmtInfo->fd);
    if (num_items != 1) {
        return RC_WRITE_FAILED;
    }
    fHandle->curPagePos = pageNum + 1;
    return RC_OK;

}

// Write to the currentPage pointed by fHandle with data from memPage
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return writeBlock(getBlockPos(fHandle), fHandle, memPage);
}

// Add an empty page to the end of the file pointed by fHandle.
RC appendEmptyBlock(SM_FileHandle *fHandle) {

    size_t i;

    FILE *fp = fHandle->mgmtInfo->fd;

    if (fp == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    int rc = fseek(fp, 0, SEEK_END);
    if (rc < 0) {
        return RC_ALLOCATION_FAILED;
    }

    for (i = 0; i < PAGE_SIZE; i++) {
        fprintf(fp, "%c", '\0');
    }
    fflush(fHandle->mgmtInfo->fd);
    fHandle->totalNumPages += 1;
    fHandle->curPagePos = (fHandle->totalNumPages + 1);
    return RC_OK;

}

// If there are less than numberOfPages, increase the number of pages to numberOfPages by appending empty pages.
// If there are more (or equal) pages than numberOfPages, we are good.!!, do nothing.
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {

    if (fHandle->totalNumPages < numberOfPages) {
        int numPages = numberOfPages - fHandle->totalNumPages;
        size_t i;
        RC rc;
        for (i = 0; i < numPages; i++) {
            rc = appendEmptyBlock(fHandle);
            if (rc != RC_OK) {
                return RC_ALLOCATION_FAILED;
            }
        }
    }
    return RC_OK;
}

int getNumPages(SM_FileHandle *fHandle) {
    return fHandle->totalNumPages;
}
