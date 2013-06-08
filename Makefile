# Compiler flags
CFLAGS = -Wall -pedantic -g -pthread

# Compiler
CC = gcc

#directory librerie
LIBDIR = ./lib

# nome libreria
LIBNAME = libdplan.a

.PHONY: all clean lib

all: dserver dplan
	@echo -e 'MAKE ALL completato\a'
clean:
	-rm -fr *.o *~  

# Lista degli object file nella libreria libdplan.a
OBJS = 	lbase.o llist.o

# Target per la creazione della libreria 
lib: $(OBJS)
	ar -r $(LIBNAME) $(OBJS)
	mkdir $(LIBDIR)
	cp $(LIBNAME) $(LIBDIR)

# Target per la creazione degli eseguibili "dserver" e "dplan"
dserver: dserver.c lcscom.o planners_table.o pthread_mem.o error_check.o 
	$(CC) $(CFLAGS) -L$(LIBDIR) $^ -o dserver -lpthread -ldplan

dplan: dplan.o lcscom.o planners_table.o pthread_mem.o error_check.o
	$(CC) $(CFLAGS) -L$(LIBDIR) $^ -o dplan -ldplan
