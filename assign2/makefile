# Make file for assign 2

CC = gcc
DEBUG = -ggdb -O0 -std=c99 -w
LFLAGS = $(DEBUG)
CFLAGS = $(LFLAGS) -c
OBJS1 = test_assign2_1.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o hash_table.o free_list.o 
OBJS2 = test_assign2_2.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o hash_table.o free_list.o

all: assign2 assign1

assign2: $(OBJS2)
	$(CC) $(LFLAGS) $(OBJS2) -lm -o test_assign2_2

assign1: $(OBJS1)
	$(CC) $(LFLAGS) $(OBJS1) -lm -o test_assign2_1

test_assign2_1.o: buffer_mgr.h buffer_mgr_stat.h storage_mgr.h test_assign2_1.c
	$(CC) $(CFLAGS) test_assign2_1.c

buffer_mgr.o: buffer_mgr.h hash_table.h free_list.h buffer_mgr.c dt.h
	$(CC) $(CFLAGS) buffer_mgr.c 

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
	\rm *.o test_assign2_1 test_assign2_2
