#ifndef BTREE_MGR_H
#define BTREE_MGR_H

#include "dberror.h"
#include "tables.h"

#define NO_CHILD -99

// structure for accessing btrees
typedef struct BTreeHandle {
  DataType keyType;
  char *idxId;
  struct BtreeMgmtData *mgmtData;
} BTreeHandle;

// Additional Metadata needed for various operations on B+trees
typedef struct BtreeMgmtData {
    BM_BufferPool *bm;      // The buffer pool with the file holding the index loaded
    PageNumber rootPageNum; // The root page of the B+tree
    int nodeSize;           // Maximum node size of the B+tree
    int numNodes;           // Number of nodes in a B+tree
    int numRecords;         // Number of records in the B+tree
}BtreeMgmtData;

// Addtional Metadata needed for the scans
typedef struct BT_ScanMgmtData{
    PageNumber currNode; // Current node which we are scanning
    int currRecord;      // Current record which we are scanning
}BtreeScanMgmt;

typedef struct BT_ScanHandle {
  BTreeHandle *tree;
  BtreeScanMgmt *mgmtData;
} BT_ScanHandle;

// Every Page in the B+tree starts with the following header.
typedef struct BT_PageHeader{
    int numRec;          // Number of records in the B+tree
    bool leafNode;       // true if this node is a leaf Node, false other wise.
    PageNumber nextPage; // In case of Leaf Node, this would be the pointer to next page
                         // In case of non-Leaf Node, this would be the left child of the first record
}BtreePageHeader;

// After the header, every leaf node has following sequence layed out.
// This is the format of each leaf record in the node
typedef struct BT_LeafRecord{
    char *key;  // The key of the record.
    RID rid;    // The corresponding rid of the record.
}BtreeLeafRec;

// After the header, every non-leaf record is layed on disk using the following structure
typedef struct BT_NonLeafRecord{
    char* key;          // The key of the record.
    PageNumber rChild;  // The right child of the record.
}BtreeNonLeafRec;

// init and shutdown index manager
extern RC initIndexManager (void *mgmtData);
extern RC shutdownIndexManager ();

// create, destroy, open, and close an btree index
extern RC createBtree (char *idxId, DataType keyType, int n);
extern RC openBtree (BTreeHandle **tree, char *idxId);
extern RC closeBtree (BTreeHandle *tree);
extern RC deleteBtree (char *idxId);

// access information about a b-tree
extern RC getNumNodes (BTreeHandle *tree, int *result);
extern RC getNumEntries (BTreeHandle *tree, int *result);
extern RC getKeyType (BTreeHandle *tree, DataType *result);

// index access
extern RC findKey (BTreeHandle *tree, Value *key, RID *result);
extern RC insertKey (BTreeHandle *tree, Value *key, RID rid);
extern RC deleteKey (BTreeHandle *tree, Value *key);
extern RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle);
extern RC nextEntry (BT_ScanHandle *handle, RID *result);
extern RC closeTreeScan (BT_ScanHandle *handle);

// debug and test functions
extern char *printTree (BTreeHandle *tree);

#endif // BTREE_MGR_H
