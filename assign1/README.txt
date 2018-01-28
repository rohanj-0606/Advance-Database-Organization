Programming Assignment 1 - Storage Manager

Team Members :Rohan Jain(A20378201)
              

Contents :
1)Running Instruction
2)Function Description


1)Running Instruction  
----------------------
a) In the linux terminal, navigate to the directory of the assignment contents
b) Type: 
   make -f makefile   
    

2)Function Description
----------------------
      
                   ** Function : CreatePageFile **

a)Creating a new file, if already exist it is discarded and new empty file is created
b)If the file is created successfully, it is filled with null bytes.


                   ** Function : OpenPageFile **

a) Check if the file with the provided file name exists.
b) If it does not exist, throw an error.
c) If it exists, check for the total number of pages that the file has.
d) After opening the file, initiate the structure elements needed.


                   ** Function : ClosePageFile **
	
a) Close the file and return a success message upon success.
b) If the file could not be located, return appropriate error message


                   ** Function : DestroyPageFile **

a)Check if the file is present, and remove the file.
b)If it is success, return a success message.
c) else, return a failure message.


                    ** Function Name: readBlock **

a)Check if the file already exists with the help of file descriptor in   file handler
b)Move the file descriptors position to the page requested in pageNum.
c)Read the content and load to the memory specified in mempage.

                    ** Function : getBlockPos **

a)Check if the file already exists with the help of file descriptor in   file handler.
b)Returns current Block position.


                    ** Function : readFirstBlock **

a)Move the file descriptor in the file handler to the first page of the  file
b)Read the content to mempage.


                    ** Function : readPreviousBlock **
         
a)Pass the previous file handle value and buffer(memPage) value to   readCurrentBlock.
b)It checks if the file exist or not.
c)If the file does not exist, it return appropriate error message.
d)If the file exist, initialize file pointer and assign page file   information values. 
e)Move the pointer to fild Handle location and read the file.

                     
                    ** Function : readCurrentBlock **
a)Check if the file already exists with the help of file descriptor in file handler
b)Read the content to mempage of the current page position in the file handler with the help of file descriptor present in the file handler.


                     ** Function : readNextBlock **

a)Read the content to mempage of the next page position in the file handler with the help of file descriptor present in the file handler.


                     ** Function : readLastBlock **

a)Move the file descriptor in the file handler to the last page of the file
b)Read the content to mempage.


                      ** Function: writeBlock **

a)Check if the file is present.
b)Get the current Position of the file.	
c)Write Contents to the file.
d)Close the file
e)If file does not exit return error message.


                     ** Function: writeCurrentBlock **

a) Write current block based on absolute position.


                     ** Function : appendEmptyBlock **

a)Check if the file is present.
b)Check for the total number of pages.
c)Add one page and print'\0' in the new empty block.


                     ** Function : ensureCapacity **

a) Try to locate the specified file, upon failure return an appropriate error message.
Upon success, continue.
b) Calculate the number of pages that the file could accomodate.
c) If the file's memory is insufficient, calculate the memory needed to make sure that 
the file has enough capacity and allocate the same.
d) If the file's memory is sufficient, provide an appropriate message.