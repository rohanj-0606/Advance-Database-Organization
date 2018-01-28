//
// Created by scl on 12/03/17.
//
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include "record_mgr.h"

#define SizeofPageHeader offsetof(RM_PageHeader, lp)


bool initPage(char *page);

int getNumLPInPage(char *page);

PageNumber getNextFreePage(struct BM_BufferPool *bm, int recSize);

int getNumSlotsInPage(RM_TableData *rel, PageNumber pageNumber);

typedef struct RM_ScanMgmt {
    Expr *condn;                //The scan Condition associated with every Scan
    Record *currentRecord;        //helps to find the current record
    int currentPage;            //used to store current page that is scanned info
    int currentSlot;            //used to store current slot that is scanned info
    int totalNumPages;          //Total number of pages in file

} RM_ScanMgmt;

RC initRecordManager(void *mgmtData) {

    return RC_OK;
}

// Block 0 of the file always holds the Table metadata
//  We assume that metadata fits in 1 page, if not we throw an error
RC createTable(char *name, Schema *schema) {

    // Create tempBuff of size one page
    //  do all operations on the tempBuff and write it to buffer pool.
    char *tempBuff = malloc(sizeof(char) * PAGE_SIZE);
    memset(tempBuff, '\0', PAGE_SIZE);
    int offset = 0;

    //Write the table name, Number of Attributes
    offset = sprintf(tempBuff, "%s %d", name, schema->numAttr);

    // Write the attribute Names, data type, length
    for (int i = 0; i < schema->numAttr; ++i) {
        offset += sprintf((tempBuff) + offset, "%s %d %d", schema->attrNames[i],
                          (schema->dataTypes[i]), (schema->typeLength[i]));
    }

    // Write numKeyAttributes
    offset += sprintf((tempBuff) + offset, " %d ", schema->keySize);
    // Write the key attributes
    for (int j = 0; j < schema->keySize; ++j) {
        offset += sprintf((tempBuff) + offset, "%d ", schema->keyAttrs[j]);
    }


    // We assume that metadata for 1 table would not exceed 1 page.
    assert(offset <= (PAGE_SIZE));

    // Create a File with name of table, Load the file into Buffer
    char *fileName = malloc(sizeof(char) * (strlen(name) + 5));
    strcpy(fileName, name);
    strcat(fileName, ".bin");
    createPageFile(fileName);
    BM_BufferPool *buffPool = malloc(sizeof(BM_BufferPool));
    initBufferPool(buffPool, fileName, 20, RS_LRU, NULL);

    // Allocate the Page Handle
    BM_PageHandle *pHandle = malloc(sizeof(BM_PageHandle));

    //Block 0 of every file holds the metadata of the table
    RC rc = pinPage(buffPool, pHandle, 0);
    if (rc != RC_OK) {
        return rc;
    }

    sprintf(pHandle->data, "%d %s", offset, tempBuff);


    // Contents of page changed, Inform buffer manager
    rc = markDirty(buffPool, pHandle);
    if (rc != RC_OK) {
        return rc;
    }

    // Release the page
    rc = unpinPage(buffPool, pHandle);
    if (rc != RC_OK) {
        return rc;
    }

    shutdownBufferPool(buffPool);
    free(pHandle);
    free(buffPool);
    free(tempBuff);
    free(fileName);
    return RC_OK;
}

// Create a schema with given attributes
Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys) {

    Schema *schema = malloc(sizeof(Schema));

    schema->numAttr = numAttr;
    schema->attrNames = attrNames;
    schema->dataTypes = dataTypes;
    schema->typeLength = typeLength;
    schema->keySize = keySize;
    schema->keyAttrs = keys;

    return schema;
}

// Returns next empty pageNumber that can be used
int getNewPagePos(BM_BufferPool *buff) {
    int totPages = getNumPagesInFile(buff);
    return totPages;
}

// Open the file with name name + '.bin'
// Copy the table strcuture in to the schema
// Initialize management data needed.
RC openTable(RM_TableData *rel, char *name) {

    BM_BufferPool *buff = malloc(sizeof(BM_BufferPool));

    Schema *schema = malloc(sizeof(Schema));
    char *nameInFile = malloc(sizeof(char) * (strlen(name) + 1));
    char *fileName = malloc((strlen(name) + 5) * sizeof(char));
    memcpy(fileName, name, (strlen(name) + 1) * sizeof(char));
    strcat(fileName, ".bin");

    RC rc = initBufferPool(buff, fileName, 20, RS_LRU, NULL);

    if (rc != RC_OK) {
        return rc;
    }

    BM_PageHandle *pageHandle = malloc(sizeof(BM_PageHandle));

    rc = pinPage(buff, pageHandle, 0);
    if (rc != RC_OK) {
        return rc;
    }

    int offset;
    int numChars;
    int totalLen;

    sscanf(pageHandle->data, "%d %s %d%n", &totalLen, nameInFile, &(schema->numAttr), &numChars);

    schema->attrNames = malloc(sizeof(char *) * schema->numAttr);
    schema->dataTypes = malloc(sizeof(DataType) * schema->numAttr);
    schema->typeLength = malloc(sizeof(int) * schema->numAttr);

    offset = numChars;
    for (int i = 0; i < schema->numAttr; i++) {
        schema->attrNames[i] = malloc(sizeof(char) * 20);
        sscanf((pageHandle->data) + offset, "%s %d %d %n", schema->attrNames[i],
               &(schema->dataTypes[i]), &(schema->typeLength[i]), &numChars);
        offset += numChars;
    }

    sscanf((pageHandle->data) + offset, "%d%n", &(schema->keySize), &numChars);
    offset += numChars;

    schema->keyAttrs = malloc(sizeof(int) * schema->keySize);
    for (int j = 0; j < schema->keySize; ++j) {
        sscanf((pageHandle->data) + offset, " %d%n", &(schema->keyAttrs[j]), &numChars);
        offset += numChars;
    }

    rel->name = nameInFile;
    rel->schema = schema;
    rel->mgmtData = malloc(sizeof(RM_TableMgmtData));
    rel->mgmtData->buffPool = buff;
    rel->mgmtData->fileName = fileName;
    return RC_OK;

}

// Shutdown the related bufferpool and free the allocated resources for the table
RC closeTable(RM_TableData *rel) {
    forceFlushPool(rel->mgmtData->buffPool);
    shutdownBufferPool(rel->mgmtData->buffPool);
    free(rel->name);
    free(rel->mgmtData->fileName);
    free(rel->mgmtData->buffPool);
    free(rel->mgmtData);
    freeSchema(rel->schema);
    return RC_OK;
}

// Insert record in to the table 'rel'
RC insertRecord(RM_TableData *rel, Record *record) {
    BM_PageHandle *pHandle = MAKE_PAGE_HANDLE();
    int recordSize = getRecordSize(rel->schema);
    BM_BufferPool *bm = rel->mgmtData->buffPool;

    // Get page number to insert the record
    PageNumber freePage = getNextFreePage(bm, recordSize);

    // Pin the page, do the necessary initilization if page is empty
    RC rc = pinPage(bm, pHandle, freePage);
    if (rc != RC_OK) {
        free(pHandle);
        return rc;
    }

    RM_PageHeader *pageHeader = (RM_PageHeader *) pHandle->data;
    int lowerSpace = pageHeader->lowerSpace;
    int upperSpace = pageHeader->upperSpace;

    if (lowerSpace > upperSpace) {
        printf("Page doesn't have space to write: Check FSM");
        return RC_RM_NO_SPACE_PAGE;
    }
    int numLP = getNumLPInPage(pHandle->data);
    int nextSlotId = numLP;
    int slotIdToUse = -1;

    // See if any LP can be re-used
    if (pageHeader->pageHasFreeLP) {
        for (int i = 0; i < numLP; ++i) {
            if (pageHeader->lp[i].isEmpty) {
                slotIdToUse = i;
                break;
            }

        }
    } else {
        // If none can be re-used, use the next LP
        slotIdToUse = nextSlotId;
    }


    // Change the lowerSpace bound if no reuse of LP
    if (slotIdToUse == nextSlotId)
        lowerSpace += sizeof(RM_LinePointer);


    // Change the upperSpace bound
    upperSpace -= recordSize;

    if (lowerSpace > upperSpace) {
        printf("Page doesn't have space to write: Check FSM");
        return RC_RM_NO_SPACE_PAGE;
    }

    //Update the LP with new values
    pageHeader->lp[slotIdToUse].isEmpty = false;
    pageHeader->lp[slotIdToUse].recOffset = upperSpace;

    // Update the record with the new pageNumber and slotId
    record->id.slot = slotIdToUse;
    record->id.page = freePage;

    //copy the record to buffer, update the header
    memcpy(pHandle->data + upperSpace, record->data, recordSize);
    pageHeader->lowerSpace = lowerSpace;
    pageHeader->upperSpace = upperSpace;

    //Update header if page is full
    if ((upperSpace - lowerSpace) < recordSize)
        pageHeader->pageFull = true;
    pageHeader->totRecInPage += 1;


    markDirty(bm, pHandle);
    //forcePage(bm, pHandle);
    unpinPage(bm, pHandle);
    free(pHandle);
    return RC_OK;
}

RC shutdownRecordManager() {
    return RC_OK;
}

// Returns Number of records in a table;
int getNumTuples(RM_TableData *rel) {
    BM_BufferPool *bm = rel->mgmtData->buffPool;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();
    int totPages = getNumPagesInFile(bm);
    int totCount = 0;
    RM_PageHeader *pageHeader;

    // Scan through all pages and get the count from the header
    for (int i = 1; i < totPages; ++i) {
        pinPage(bm, ph, i);
        pageHeader = (RM_PageHeader *) ph->data;
        totCount += pageHeader->totRecInPage;
        unpinPage(bm, ph);
    }
    free(ph);
    return totCount;
}

RC deleteTable(char *name) {
    char *fileName = malloc(sizeof(char) * (strlen(name) + 5));
    strcpy(fileName, name);
    strcat(fileName, ".bin");
    RC rc = destroyPageFile(fileName);
    free(fileName);
    if (rc != RC_OK) {
        return rc;
    }

    return RC_OK;
}

RC deleteRecord(RM_TableData *rel, RID id) {
    PageNumber pageNumber = id.page;
    int slotNumber = id.slot;
    BM_BufferPool *bm = rel->mgmtData->buffPool;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();

    RC rc = pinPage(bm, ph, pageNumber);
    if (rc != RC_OK) {
        free(ph);
        return rc;
    }

    RM_PageHeader *pageHeader = (RM_PageHeader *) ph->data;
    int offset = pageHeader->lp[slotNumber].recOffset;

    // Mark the record with special marker indicating that record is deleted
    *(ph->data + offset) = '^';

    // Decrease the number of counter in page
    pageHeader->totRecInPage -= 1;

    markDirty(bm, ph);
    unpinPage(bm, ph);

    free(ph);
    return RC_OK;
}

// Update record with new data
RC updateRecord(RM_TableData *rel, Record *record) {
    PageNumber pageNumber = record->id.page;
    int slotNumber = record->id.slot;
    int recSize = getRecordSize(rel->schema);
    BM_BufferPool *bm = rel->mgmtData->buffPool;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();

    RC rc = pinPage(bm, ph, pageNumber);
    if (rc != RC_OK) {
        free(ph);
        return rc;
    }

    //Get the offset to the record from the slot Number
    RM_PageHeader *pageHeader = (RM_PageHeader *) ph->data;
    int offset = pageHeader->lp[slotNumber].recOffset;

    // Update with new data in buffer
    memcpy(ph->data + offset, record->data, recSize);

    markDirty(bm, ph);
    unpinPage(bm, ph);
    return RC_OK;
}

// Returns the record with a given RID
RC getRecord(RM_TableData *rel, RID id, Record *record) {
    PageNumber pageNumber = id.page;
    int slotNumber = id.slot;
    int recSize = getRecordSize(rel->schema);
    BM_BufferPool *bm = rel->mgmtData->buffPool;
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();

    RC rc = pinPage(bm, ph, pageNumber);
    if (rc != RC_OK) {
        free(ph);
        return rc;
    }

    RM_PageHeader *pageHeader = (RM_PageHeader *) ph->data;
    int offset = pageHeader->lp[slotNumber].recOffset;

    memcpy(record->data, ph->data + offset, recSize);
    return RC_OK;
}

RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {

    //using Scan Handle Structure & init its attributes
    scan->rel = rel;
    int recSize = getRecordSize(rel->schema);
    //Initialize the created Scan Management Structure
    RM_ScanMgmt *scan_mgmt = (RM_ScanMgmt *) malloc(sizeof(RM_ScanMgmt));
    scan_mgmt->condn = cond;
    scan_mgmt->currentRecord = (Record *) malloc(sizeof(Record));
    scan_mgmt->currentRecord->data = malloc(sizeof(char) * recSize);
    memset(scan_mgmt->currentRecord->data, '\0', recSize * sizeof(char));
    scan_mgmt->currentPage = 1;
    scan_mgmt->currentSlot = -1;
    scan_mgmt->totalNumPages = getNumPagesInFile(rel->mgmtData->buffPool);

    //store the managememt data
    scan->mgmtData = scan_mgmt;

    return RC_OK;
}


// For a given page, find the number of slots in it
int getNumSlotsInPage(RM_TableData *rel, PageNumber pageNumber) {
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();
    RC rc = pinPage(rel->mgmtData->buffPool, ph, pageNumber);
    if (rc != RC_OK) {
        free(ph);
        return -1;
    }

    int nSlots = getNumLPInPage(ph->data);
    unpinPage(rel->mgmtData->buffPool, ph);
    free(ph);
    return nSlots;
}


RC next(RM_ScanHandle *scan, Record *record) {
    Value *result;
    RID rid;
    int totPages = ((RM_ScanMgmt *) (scan->mgmtData))->totalNumPages;
    int recSize = getRecordSize(scan->rel->schema);

    int nSlotsCurrPage = getNumSlotsInPage(scan->rel, ((RM_ScanMgmt *) scan->mgmtData)->currentPage);
    if (nSlotsCurrPage < 0) {
        return RC_RM_SCAN_FAILED;
    }

    //initialize the page and slot number from the data Structure RM_ScanMgmt
    ((RM_ScanMgmt *) scan->mgmtData)->currentSlot += 1;
    if (((RM_ScanMgmt *) scan->mgmtData)->currentSlot > nSlotsCurrPage) {

        ((RM_ScanMgmt *) scan->mgmtData)->currentPage += 1;
        ((RM_ScanMgmt *) scan->mgmtData)->currentSlot = 0;
    }
    rid.page = ((RM_ScanMgmt *) scan->mgmtData)->currentPage;
    rid.slot = ((RM_ScanMgmt *) scan->mgmtData)->currentSlot;

    //if the scan condition is passed as NULL, return all the tuples from the table
    if (((RM_ScanMgmt *) scan->mgmtData)->condn == NULL) {

        while (true) {
            //get the record
            getRecord(scan->rel, rid, ((RM_ScanMgmt *) scan->mgmtData)->currentRecord);
            //check if the returned record is marked for deletion, if so fetch the next record
            if (((RM_ScanMgmt *) scan->mgmtData)->currentRecord->data[0] == '^') {
                ((RM_ScanMgmt *) scan->mgmtData)->currentSlot += 1;
                if (((RM_ScanMgmt *) scan->mgmtData)->currentSlot > nSlotsCurrPage) {

                    ((RM_ScanMgmt *) scan->mgmtData)->currentPage += 1;
                    ((RM_ScanMgmt *) scan->mgmtData)->currentSlot = 0;
                }
                rid.page = ((RM_ScanMgmt *) scan->mgmtData)->currentPage;
                rid.slot = ((RM_ScanMgmt *) scan->mgmtData)->currentSlot;
            } else break;
        }

        memcpy(record->data, ((RM_ScanMgmt *) scan->mgmtData)->currentRecord->data, recSize);
        memcpy(&(record->id), &(((RM_ScanMgmt *) scan->mgmtData)->currentRecord->id), sizeof(RID));

        return RC_OK;

    } else    //if specific scan condition is supplied
    {
        //loop until end of the entire table, scan for each record
        while (rid.page > 0 && rid.page < totPages) {
            //get record
            getRecord(scan->rel, rid, ((RM_ScanMgmt *) scan->mgmtData)->currentRecord);

            //check if the returned record is marked for deletion, if so fetch the next record
            if (((RM_ScanMgmt *) scan->mgmtData)->currentRecord->data[0] == '^') {
                ((RM_ScanMgmt *) scan->mgmtData)->currentSlot += 1;
                if (((RM_ScanMgmt *) scan->mgmtData)->currentSlot > nSlotsCurrPage) {

                    ((RM_ScanMgmt *) scan->mgmtData)->currentPage += 1;
                    ((RM_ScanMgmt *) scan->mgmtData)->currentSlot = 0;
                }
                rid.page = ((RM_ScanMgmt *) scan->mgmtData)->currentPage;
                rid.slot = ((RM_ScanMgmt *) scan->mgmtData)->currentSlot;
                continue;
            }


            //Evaluate the Expression
            evalExpr(((RM_ScanMgmt *) scan->mgmtData)->currentRecord, scan->rel->schema,
                     ((RM_ScanMgmt *) scan->mgmtData)->condn, &result);

            //if result is satisfied, i.e. scan returns the attributes needed i.e. Boolean Attrbiute & v.boolV as 1
            if (result->dt == DT_BOOL && result->v.boolV) {
                memcpy(record->data, ((RM_ScanMgmt *) scan->mgmtData)->currentRecord->data, recSize);
                memcpy(&(record->id), &(((RM_ScanMgmt *) scan->mgmtData)->currentRecord->id), sizeof(RID));
                return RC_OK;
            } else    //scan the next record, until the record is found
            {
                ((RM_ScanMgmt *) scan->mgmtData)->currentSlot += 1;
                //increment the page to next page
                if (((RM_ScanMgmt *) scan->mgmtData)->currentSlot > nSlotsCurrPage) {
                    ((RM_ScanMgmt *) scan->mgmtData)->currentPage += 1;
                    ((RM_ScanMgmt *) scan->mgmtData)->currentSlot = 0;
                }
                //rid as next page id
                rid.page = ((RM_ScanMgmt *) scan->mgmtData)->currentPage;
                rid.slot = ((RM_ScanMgmt *) scan->mgmtData)->currentSlot;
            }
        }
    }

    //re-init to point to first page
    ((RM_ScanMgmt *) scan->mgmtData)->currentPage = 1;

    //if all records scanned return no more tuples found, i.e. scan is completed
    return RC_RM_NO_MORE_TUPLES;
}

/*
 * This function is used to indicate the record manager
 * that all the resources can now be cleaned up
 */
RC closeScan(RM_ScanHandle *scan) {
    //free all allocations
    free(((RM_ScanMgmt *) scan->mgmtData)->currentRecord->data);
    free(((RM_ScanMgmt *) scan->mgmtData)->currentRecord);
    free(scan->mgmtData);
    return RC_OK;
}

int getRecordSize(Schema *schema) {
    int totSize = 0;
    for (int i = 0; i < schema->numAttr; ++i) {
        switch (schema->dataTypes[i]) {
            case DT_BOOL:
                totSize += sizeof(bool);
                break;
            case DT_INT:
                totSize += sizeof(int);
                break;
            case DT_FLOAT:
                totSize += sizeof(float);
                break;
            case DT_STRING:
                totSize += (sizeof(char) * (schema->typeLength[i] + 1));
                break;
            default:
                return -1;
        }
    }

    return totSize;
}

RC freeSchema(Schema *schema) {
    free(schema->typeLength);
    for (int i = 0; i < schema->numAttr; ++i) {
        free(schema->attrNames[i]);
    }
    free(schema->attrNames);
    free(schema->dataTypes);
    free(schema->keyAttrs);
    free(schema);
    return RC_OK;
}

RC createRecord(Record **record, Schema *schema) {
    int recSize = getRecordSize(schema);
    *record = (Record *) malloc(sizeof(Record));
    (*record)->data = malloc(recSize * sizeof(char));
    memset((*record)->data, '\0', recSize);
    return RC_OK;
}

RC freeRecord(Record *record) {
    free(record->data);
    free(record);
    return RC_OK;
}

RC getAttr(Record *record, Schema *schema, int attrNum, Value **value) {
    int offset = 0;
    for (int i = 0; i < attrNum; ++i) {
        switch (schema->dataTypes[i]) {
            case DT_INT :
                offset += sizeof(int);
                break;
            case DT_STRING:
                offset += sizeof(char) * (schema->typeLength[i] + 1);
                break;
            case DT_FLOAT:
                offset += sizeof(float);
                break;
            case DT_BOOL:
                offset += sizeof(bool);
        }
    }
    *value = malloc(sizeof(Value));
    (*value)->dt = schema->dataTypes[attrNum];

    switch (schema->dataTypes[attrNum]) {
        case DT_INT:
            memcpy((char *) &((*value)->v.intV), (record->data) + offset, sizeof(int));
            break;
        case DT_BOOL:
            memcpy((char *) &((*value)->v.boolV), (record->data) + offset, sizeof(bool));
            break;
        case DT_FLOAT:
            memcpy((char *) &((*value)->v.floatV), (record->data) + offset, sizeof(float));
            break;
        case DT_STRING:
            (*value)->v.stringV = malloc(sizeof(char) * (schema->typeLength[attrNum] + 1));
            // (*value)->v.stringV[schema->typeLength[attrNum]] = '\0';
            memcpy(((*value)->v.stringV), (record->data) + offset, sizeof(char) * (schema->typeLength[attrNum] + 1));
            break;
    }

    return RC_OK;
}

RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {
    int offset = 0;
    for (int i = 0; i < attrNum; ++i) {
        switch (schema->dataTypes[i]) {
            case DT_INT :
                offset += sizeof(int);
                break;
            case DT_STRING:
                offset += sizeof(char) * (schema->typeLength[i] + 1);
                break;
            case DT_FLOAT:
                offset += sizeof(float);
                break;
            case DT_BOOL:
                offset += sizeof(bool);
        }
    }

    switch (value->dt) {
        case DT_INT:
            memcpy((record->data) + offset, (char *) &((value)->v.intV), sizeof(int));
            break;
        case DT_BOOL:
            memcpy((record->data) + offset, (char *) &((value)->v.boolV), sizeof(bool));
            break;
        case DT_FLOAT:
            memcpy((record->data) + offset, (char *) &((value)->v.floatV), sizeof(float));
            break;
        case DT_STRING:
            memcpy((record->data) + offset, ((value)->v.stringV), sizeof(char) * (schema->typeLength[attrNum] + 1));
            break;
    }

    return RC_OK;
}

// Returns an page number that has emptyspace of 'recSize'
PageNumber getNextFreePage(BM_BufferPool *buff, int recSize) {
    int totPages = getNumPagesInFile(buff);
    BM_PageHandle *ph = MAKE_PAGE_HANDLE();
    PageNumber emptyPage = -1;
    RM_PageHeader *pageHeader;

    // Scan all pages in buffer
    for (int i = 1; i < totPages; ++i) {
        pinPage(buff, ph, i);
        pageHeader = (RM_PageHeader *) ph->data;
        // If header is marked full, then fetch nextPage
        if (pageHeader->pageFull)
            continue;
        if (pageHeader->upperSpace < pageHeader->lowerSpace) {
            printf("Error");
        }

        // If there is enough space in the page to fit the record we are done
        if ((pageHeader->upperSpace - pageHeader->lowerSpace) >= (recSize + SizeofPageHeader)) {
            emptyPage = i;
            break;
        } else {
            // There is not enough space, but page is marked not full
            // then mark it to full
            pageHeader->pageFull = true;
            markDirty(buff, ph);
        }
        unpinPage(buff, ph);
    }

    // If none of the pages can fit the record
    //  return a new pageNumber
    if (emptyPage == -1) {
        emptyPage = getNewPagePos(buff);
        RC rc = pinPage(buff, ph, emptyPage);
        if (rc != RC_OK) {
            return -1;
        }
        initPage(ph->data);
        markDirty(buff, ph);
        unpinPage(buff, ph);
    }

    return emptyPage;

}

// Initialize empty page with header
bool initPage(char *page) {

    RM_PageHeader *pageHeader = (RM_PageHeader *) page;

    pageHeader->pageFull = false;
    pageHeader->pageHasFreeLP = false;
    pageHeader->lowerSpace = SizeofPageHeader;
    pageHeader->upperSpace = PAGE_SIZE;
    pageHeader->totRecInPage = 0;

    return true;
}

// Get the number of Line pointers in the Page
int getNumLPInPage(char *page) {
    RM_PageHeader *pageHeader = (RM_PageHeader *) page;
    int lp_used = (pageHeader->lowerSpace
                   > SizeofPageHeader ? (pageHeader->lowerSpace - SizeofPageHeader) / sizeof(RM_LinePointer) : 0);

    return lp_used;
}