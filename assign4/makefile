# Make file for assign 4

CC = gcc
DEBUG = -ggdb -O0 -std=c99 -w
LFLAGS = $(DEBUG)
CFLAGS = $(LFLAGS) -c
OBJS = test_assign4_1.o btree_mgr.o stack.o record_mgr.o expr.o rm_serializer.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o hash_table.o free_list.o 


all: assign4

assign4: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -lm -o test_assign4_1

test_assign4_1.o: dberror.h stack.o expr.h btree_mgr.h record_mgr.h test_helper.h tables.h test_assign4_1.c
	$(CC) $(CFLAGS) test_assign4_1.c

btree_mgr.o: btree_mgr.h btree_mgr.c stack.h
	$(CC) $(CFLAGS) btree_mgr.c

stack.o: stack.h
	$(CC) $(CFLAGS) stack.c

record_mgr.o: record_mgr.h record_mgr.c
	$(CC) $(CFLAGS) record_mgr.c

expr.o: dberror.h record_mgr.h expr.h tables.h expr.c
	$(CC) $(CFLAGS) expr.c

buffer_mgr.o: buffer_mgr.h hash_table.h free_list.h buffer_mgr.c dt.h buffer_mgr.c
	$(CC) $(CFLAGS) buffer_mgr.c 

rm_serializer.o: dberror.h tables.h record_mgr.h rm_serializer.c
	$(CC) $(CFLAGS) rm_serializer.c

buffer_mgr_stat.o: buffer_mgr_stat.h buffer_mgr.h buffer_mgr_stat.c
	$(CC) $(CFLAGS) buffer_mgr_stat.c

dberror.o: dberror.h dberror.c
	$(CC) $(CFLAGS) dberror.c 

hash_table.o: hash_table.h hash_table.c
	$(CC) $(CFLAGS) hash_table.c 

free_list.o: free_list.h free_list.c
	$(CC) $(CFLAGS) free_list.c 

storage_mgr.o: storage_mgr.h storage_mgr.c
	$(CC) $(CFLAGS) storage_mgr.c

clean: 
	\rm *.o test_assign4_1
