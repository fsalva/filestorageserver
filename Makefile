
#Specifiche per il compilatore
CC = gcc 
CFLAGS = -Wall -Wextra -pedantic -Ilib/
objects = server.o
DEPS = ./lib/constvalues.h

%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

server: $(objects)
	$(CC) $(CFLAGS) -o $@ $^  $(DEPS)

.PHONY : clean
clean: 
	@echo "Rimuovo tutti i risultati delle compilazioni."
	-rm $(objects) 

