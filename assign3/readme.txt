#   Problem Statement   #
==========================

The goal of this assignment is to create a record manager. The record manager handles tables with a fixed schema. 
Clients can insert records, delete records, update records, and scan through the records in a table. A scan is 
associated with a search condition and only returns records that match the search condition. Each table should 
be stored in a separate page file and record manager should access the pages of the file through the buffer 
manager implemented in the last assignment.

# How To Run The Script #
=========================

With default test cases:
------------------------
Compile : make
Run : ./test_assign3_1


To revert:
=========
On terminal : make clean


#         Logic         #
=========================

Data Structures and Design :
---------------------------

There are few attributes required to perform scan in order to retrieve all tuples from a table that fulfill a certain 
condition. We have implemented a structure of such attributes called RM_ScanMgmt.The attributes of this structure are :
1. cond     - condition defined by the client.
2. currentSlot  - current slot of a record.
3. currentPage  - current page of a record.
4. totalNumPages - total number of pages.
5. currPos - Current Position of scan

Table and Record Manager Functions :
----------------------------------

initRecordManager:
1. This function starts the record manager, and initialize the set up values.
2. In case of primary key constraint, value 1 is passed as argument.

shutdownRecordManager:
1. This function frees the resources assigned to record manager.

createTable:
1. This function creates the table with given name.
2. It first checks if the table file already exists. In that case, it throws error message.
3. If a new file created, it writes the tableInfo on the page 0 and schema on page 1.
4. The recordStartPage is set to page 2.

openTable:
1. This function opens the file with given table name.
2. It first checks if the file already exists. If not, it throws the error.
3. It initializes the buffer manager with given filename.
4. It then reads the page 0 and page 1, and loads the tableInfo and schema into memory.

closeTable:
1. This function closes the file and the buffermanager of a given file.
2. If the file doesn't exist, it throws error.
3. It frees all the resources assigned with table.

deleteTable:
1. This function deletes the table file.
2. If the table doesn't exist, it throws error.
3. It deletes the file, and destroys the buffer manager associated with it.

getNumTuples:
1. numTuples is stored in memory as part of tableInfo.
2. This is stored on page 0 when written to file, and loaded to memory.
3. The value is updaed every time a successful insert and delete is called.


Record Functions :
-----------------

insertRecord:
1. insertRecord first checks to see if there are any RID's in the tstone list. If there are, one of these are used 
and the record's attributes are updated accordingly.
2.Otherwise, since empty slots are unavailable, a new slot's value must be computed. If the new slot's location is equal 
to the maximum number of slots for a page, a new page is created, otherwise the current page is used. The record's values
for page and slot are updated accordingly.
3. serializeRecord is used to get the record in the proper format.
4. The functions pinPage, markDirty, unpinPage, and forcePage are then used to update the buffer pool, the number of 
tuples are increased, and the resulting table is written to file.

deleteRecord:
1. deleteRecord first checks to see if the tstone list is empty. If it is, it simply creates a new tNode, updates 
its contents with the values from RID, and adds it to the tstone list.
2. If the list is not empty, then tstone_iter is used to go to the end of the list and add a new tNode 
(with RID's contents) there.
3. Finally, the number of tuples are reduced and the resulting table is written to file.

updateRecord:
1. serializeRecord is used to get the record in the proper format.
2. The functions pinPage, markDirty, unpinPage, and forcePage are then used to update the buffer pool, and the 
resulting table is written to file.

getRecord:
1. getRecord uses the given RID value to return a record to the user.
2. It first checks to make sure that the RID is not in the tstone list. Otherwise, if it is a valid record, 
another check is used to see if the tuple number is greater than the number of tuples in the table (this is an 
error and so RC_RM_NO_MORE_TUPLES is returned).
3. Then, pinPage and unpinPage is used to update the buffer pool.
4. Finally, deserializeRecord is used to obtain a valid record from the record string which was retrieved. 
record->data is then updated accordingly.

Scan Functions:
--------------

startScan:
1. This function initializes the RM_ScanHandle data structure passed as an argument to it.
2. It also initializes a node storing the information about the record to be searched and the condition to be evaluated.
3. The node initialed in step 2 is assigned to scan->mgmtData.

next:
1. This function starts by fetching a record as per the page and slot id.
2. Next, it checks tombstone id for a deleted record.
3. If the bit is set and the record is a deleted one then it checks for the slot number of the record to see if it 
is the last record.
4. In-case of the last record, slot id is set to be 0 to start  next scan from the begining of the next page.
5. Incase the record is not the last one, the slot number is increased by one to proceed to the next record.
6. Updated record id parameters are assigned to the scan mgmtData and next function is called.
7. After verifying the tombstone parameters of the record, the given condition is evaluated to check if the record 
is the one required by the client.
8. If the record fetched is not the required one then next function is called again with the updated record id parameters.
9. Also, it returns RC_RM_NO_MORE_TUPLES once the scan is complete and RC_OK otherwise if no error occurs.
10. Function completes. Calls to this function returns the next tuple that fulfills the scan condition.


closeScan:
1. In this function we set scan handler to be free indicating the record manager that all associated resources are cleaned up.

Schema Functions:
-----------------

getRecordSize:
1. This function returns the size of the record.
2. The function counts the size of the record based on the schema. The datatype of each attribute is considered 
for this calculation.

createSchema:
1. This function creates the schema object an assigns the memory.
2. The number of attributes, their datatypes and the size (in case of DT_STRING) is stored.

freeSchema:
1. This function frees all the memory assigned to schema object :
   a. DataType
   b. AttributeNames
   c. AttributeSize

Attribute Functions
------------------

createRecord:
1. In this function memory allocation takes place for record and record data for creating a new record. Memory 
allocation happens as per the schema.

freeRecord:
1. This function free the memory space allocated to record and its data.

getAttr:
1. This function starts by allocating the space to the value data structre where the attribute values are to be fetched.
2. Next, attrOffset function is called to get the offset value of different attributes as per the attribute numbers.
3. In the final step attribute data is assigned to the value pointer as per different data types. Therefore, 
fetching the attribute values of a record.

setAttr:
1. This function starts by calling the attrOffset function to get the offset value of different attributes as 
per the attribute numbers.
2. Next, attribute values are set with the values provided by the client as per the attributes datatype.
3. Therefore, setting the attribute values of a record.
