# Make file for assign 1

CC = gcc
DEBUG = -ggdb
LFLAGS = $(DEBUG)
CFLAGS = $(LFLAGS) -c
OBJS = test_assign1_1.o storage_mgr.o dberror.o

assign1 :  $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o assign1

test_assign1_1.o : test_assign1_1.c storage_mgr.h dberror.h
	$(CC) $(CFLAGS) test_assign1_1.c

storage_mgr.o: storage_mgr.c storage_mgr.h dberror.h
	$(CC) $(CFLAGS) storage_mgr.c

dberror.o: dberror.h dberror.c
	$(CC) $(CFLAGS) dberror.c

clean:
	\rm *.o assign1

 
