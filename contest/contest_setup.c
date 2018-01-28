#include "contest.h"

#include "dberror.h"

#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "record_mgr.h"
#include "btree_mgr.h"

BM_BufferPool *bm;
int BTREE_BUFF_SIZE;
int RM_BUFF_SIZE;
/* set up record, buffer, pagefile, and index managers */
RC
setUpContest (int numPages)
{
  initStorageManager();
  initRecordManager(NULL);
    initIndexManager (NULL);
    BTREE_BUFF_SIZE = (int) (numPages * 0.3);
    RM_BUFF_SIZE = (int) (numPages);
    #define CONTEST_SETUP

  return RC_OK;
}

/* shutdown record, buffer, pagefile, and index managers */
RC
shutdownContest (void)
{
  shutdownRecordManager();
    shutdownIndexManager();
  return RC_OK;
}

/* return the total number of I/O operations used after setUpContest */
long
getContestIOs (void)
{
    long RM_IO = getRMNumIO();
    long IM_IO = getIMNumIO();
    if(RM_IO < 0){
        printf("\n----WARNING: RM IO not available / No tests for RM-------\n");
        RM_IO = 0;
    }
    if(IM_IO){
        printf("\n----WARNING: IM IO not available / No tests for IM-------\n");
        IM_IO = 0;
    }
  return RM_IO + IM_IO;
}
