CC = gcc

CFLAGS = -Wall -g

LD = gcc

all: mytar

mytar: mytar.o tar_create.o tar_list.o tar_extract.o
	$(CC) -o mytar mytar.o tar_create.o tar_list.o tar_extract.o

mytar.o: mytar.c
	$(CC) $(CFLAGS) -c -o mytar.o mytar.c

tar_create.o: tar_create.c
	$(CC) $(CFLAGS) -c -o tar_create.o tar_create.c

tar_list.o: tar_list.c
	$(CC) $(CFLAGS) -c -o tar_list.o tar_list.c

tar_extract.o: tar_extract.c tar_list.o
	$(CC) $(CFLAGS) -c -o tar_extract.o tar_extract.c

clean:
	rm *.o