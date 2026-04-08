.PHONY= clean 
 
CC=g++ 
OPTIONS= -g #-O3
DEBUG= #-D DEBUG
LIBDIR=lib
INCLUDEDIR=include
_OBJ= indexadorHash.o tokenizador.o stemmer.o indexadorInformacion.o
OBJ = $(patsubst %,$(LIBDIR)/%,$(_OBJ))

all: indexador 

run: indexador
	@./indexador


a.out:
	@g++ memory.cpp

test: indexador a.out
	@./a.out ./indexador

indexador:    src/main.cpp $(OBJ)
	@$(CC) $(OPTIONS) $(DEBUG) -I$(INCLUDEDIR) src/main.cpp $(OBJ) -o indexador
 
$(LIBDIR)/%.o : $(LIBDIR)/%.cpp $(INCLUDEDIR)/%.h
	@$(CC) $(OPTIONS) $(DEBUG) -c -I$(INCLUDEDIR) -o $@ $<

clean:
	@rm -f $(OBJ)

# .PHONY= clean
# CC=g++
# OPTIONS= -g# -O3
# DEBUG= #-D DEBUG
# LIBDIR=lib
# INCLUDEDIR=include
# _OBJ= tokenizador.o
# OBJ = $(patsubst %,$(LIBDIR)/%,$(_OBJ))
# 
# all: practica1
# 
# run: practica1
# 	@./practica1
# 
# practica1: src/main.cpp $(OBJ)
# 	@$(CC) $(OPTIONS) $(DEBUG) -I$(INCLUDEDIR) src/main.cpp $(OBJ) -o practica1
# 
# $(LIBDIR)/%.o : $(LIBDIR)/%.cpp $(INCLUDEDIR)/%.h
# 	@$(CC) $(OPTIONS) $(DEBUG) -c -I$(INCLUDEDIR) -o $@ $<
# 
# clean:
# 	@rm -f $(OBJ)
