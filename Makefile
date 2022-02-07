CC = gcc 
CFLAGS = -Wall -Wextra -pedantic -lpthread
LIBCFLAGS = -c -std=c99 -Wall -g
objects = filestorageserver.o 

LIBPATH = -I./libs/
OUTPATH = ./libs/out
SRCPATH = ./libs/src
ARTIPATH = ./libs/artifact
DEPS = ./libs/artifact/libutils.a

all: filestorageserver	client

filestorageserver: $(objects) $(DEPS)
	$(CC) $(CFLAGS) $(LIBPATH) -o $@ $^ 

$(DEPS): $(SRCPATH)/*
	@echo "[1/4] Creo le cartelle per i risultati di compilazione e i file .a delle librerie. "
	-mkdir $(OUTPATH) 
	-mkdir $(ARTIPATH)
	@echo "[2/4] Compilo le librerie $^"
	@for f in $^; do $(CC) $(LIBCFLAGS) $${f} ;  done
	@for f in $(shell ls ${SRCPATH}); do mv $${f%%.*}.o $(OUTPATH) ; done 
	@echo "[3/4] Creo l'archivio per le librerie. "
	-ar -rvs $(DEPS) $(OUTPATH)/*

client: client.o $(DEPS)
	$(CC) $(CFLAGS) $(LIBPATH) -o $@ $^ 


.PHONY : clean
clean: 
	@echo "Rimuovo tutti i risultati delle compilazioni."
	-rm *.o 
	-rm libs/src/*.o 

