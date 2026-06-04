.PHONY: clean

CC = g++
OPTIONS = -g #-O3
DEBUG =
LIBDIR = lib
INCLUDEDIR = include

_OBJ = buscador.o indexadorHash.o tokenizador.o stemmer.o indexadorInformacion.o
OBJ = $(patsubst %,$(LIBDIR)/%,$(_OBJ))

all: buscador

test: all
	@cp src/main.cpp /tmp/main.cpp
	@./corrigeAlumno.sh
	@cp /tmp/main.cpp src/main.cpp
	@make run



run: all
	@./buscador 0

buscador: src/main.cpp $(OBJ)
	@$(CC) $(OPTIONS) $(DEBUG) -I$(INCLUDEDIR) src/main.cpp $(OBJ) -o buscador


$(LIBDIR)/%.o: $(LIBDIR)/%.cpp
	@$(CC) $(OPTIONS) $(DEBUG) -c -I$(INCLUDEDIR) -o $@ $<

clean:
	rm -f $(OBJ) buscador
