
#Specifiche per il compilatore
CC = gcc 
CFLAGS = -Wall -Wextra -pedantic -lpthread -Ilib/ 
objects = server.o 
LIBSRCPATH = ./lib/src/
LIBPATH = ./lib/
DEPS = ./lib/constvalues.h ./lib/fsqueue.h


server: $(objects) fsqueue.o 
	$(CC) $(CFLAGS) -o $@ $^  $(DEPS)

fsqueue.o: $(LIBSRCPATH)fsqueue.c $(LIBPATH)fsqueue.h
	$(CC) $(CFLAGS) -c $(LIBSRCPATH)fsqueue.c



.PHONY : clean
clean: 
	@echo "Rimuovo tutti i risultati delle compilazioni."
	-rm $(objects) 

