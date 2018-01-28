#ifndef RECORD_MGR_H
#define RECORD_MGR_H

#include "dberror.h"
#include "expr.h"
#include "tables.h"
#include "buffer_mgr.h"
#define NO_NEXT_PAGE -1

// Bookkeeping for scans
typedef struct RM_ScanHandle
{
  RM_TableData *rel;
  void *mgmtData;
} RM_ScanHandle;

typedef struct RM_LinePointer{
    int recOffset; // OffsetFrom start of Page
    bool isEmpty;           // Indicates if the LP is free
}RM_LinePointer;


typedef struct RM_PageHeader{
    int lowerSpace;  // Offset from page start to the begining of free space
    int upperSpace;  // Offset from page start to the end of free space
    bool pageFull;      // Indicates if page is full
    bool pageHasFreeLP; // Indicates if the page has LP that can be re-used.
    int totRecInPage;   // Indicates total records in the page
    RM_LinePointer lp[]; // line Pointers that holds offset to data
}RM_PageHeader;


/*
typedef struct RM_FreeSpaceMap{
    PageNumber page;
    size_t freeSpace;
};
*/

// table and manager
extern RC initRecordManager (void *mgmtData);
extern RC shutdownRecordManager ();
extern RC createTable (char *name, Schema *schema);
extern RC openTable (RM_TableData *rel, char *name);
extern RC closeTable (RM_TableData *rel);
extern RC deleteTable (char *name);
extern int getNumTuples (RM_TableData *rel);

// handling records in a table
extern RC insertRecord (RM_TableData *rel, Record *record);
extern RC deleteRecord (RM_TableData *rel, RID id);
extern RC updateRecord (RM_TableData *rel, Record *record);
extern RC getRecord (RM_TableData *rel, RID id, Record *record);

// scans
extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond);
extern RC next (RM_ScanHandle *scan, Record *record);
extern RC closeScan (RM_ScanHandle *scan);

// dealing with schemas
extern int getRecordSize (Schema *schema);
extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys);
extern RC freeSchema (Schema *schema);

// dealing with records and attribute values
extern RC createRecord (Record **record, Schema *schema);
extern RC freeRecord (Record *record);
extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value);
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value);

// Get the blockNumber of new empty block
//extern RC int getNewPagePos(BM_BufferPool *buff);

// Funtions for contest
long getRMNumIO();

#endif // RECORD_MGR_H