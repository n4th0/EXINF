#ifndef _INFORMACION_
#define _INFORMACION_

// #include "../include/indexadorHash.h"
#include <array>
#include <iostream>
#include <list>
#include <unordered_map>

#include <iostream>
#include <stdexcept>
using namespace std;

class Fecha {
  friend bool operator==(const Fecha &a, const Fecha &b);
  friend bool operator<(const Fecha &a, const Fecha &b);
  friend bool operator>(const Fecha &a, const Fecha &b);
  friend bool operator<=(const Fecha &a, const Fecha &b);
  friend bool operator>=(const Fecha &a, const Fecha &b);
  friend bool operator!=(const Fecha &a, const Fecha &b);
  friend std::ostream &operator<<(std::ostream &s, const Fecha &f);

public:
  Fecha();
  Fecha(int d, int m, int y);
  Fecha(Fecha &&) = default;
  Fecha(const Fecha &) = default;
  Fecha &operator=(Fecha &&) = default;
  Fecha &operator=(const Fecha &) = default;
  ~Fecha();

  int getDay() const { return day; }
  int getMonth() const { return month; }
  int getYear() const { return year; }

  void setDay(int d);
  void setMonth(int m);
  void setYear(int y);

private:
  int day;
  int month;
  int year;
};

class InfTermDoc {
  friend ostream &operator<<(ostream &s, const InfTermDoc &p);

public:
  InfTermDoc(const InfTermDoc &);
  InfTermDoc();  // Inicializa ft = 0
  ~InfTermDoc(); // Pone ft = 0
  InfTermDoc &operator=(const InfTermDoc &);

  // Getters
  int getFt() const { return ft; }
  const list<int> &getPosTerm() const { return posTerm; }

  // Setters
  void setFt(int f) { ft = f; }
  void setPosTerm(const list<int> &p) { posTerm = p; }

  void incFt() { ft++; }
  void incPosTerm(int p) { posTerm.emplace_back(p); }

  // Añadir cuantos métodos se consideren necesarios para manejar la parte
  // privada de la clase
private:
  int ft; // Frecuencia del término en el documento
  list<int> posTerm;
  // Solo se almacenará esta información si el campo privado del indexador
  // almacenarPosTerm == true Lista de números de palabra en los que aparece el
  // término en el documento. Los números de palabra comenzarán desde cero (la
  // primera palabra del documento). Se numerarán las palabras de parada. Estará
  // ordenada de menor a mayor posición.
};

class InformacionTermino {
  friend ostream &operator<<(ostream &s, const InformacionTermino &p);

public:
  InformacionTermino(const InformacionTermino &);
  InformacionTermino();  // Inicializa ftc = 0
  ~InformacionTermino(); // Pone ftc = 0 y vacía l_docs
  InformacionTermino &operator=(const InformacionTermino &);
  unordered_map<int, InfTermDoc> getLdocs() const;

  // Getters
  int getFtc() const { return ftc; }
  const unordered_map<int, InfTermDoc> &getL_docs() const { return l_docs; }

  // Setters
  void setFtc(int f) { ftc = f; }
  void setL_docs(const unordered_map<int, InfTermDoc> &l) { l_docs = l; }

  void incFtc() { ftc++; }
  void addL_docs(int a, InfTermDoc l) { l_docs[a] = l; }
  // Añadir cuantos métodos se consideren necesarios para manejar la parte
  // privada de la clase
private:
  int ftc; // Frecuencia total del término en la colección
  unordered_map<int, InfTermDoc> l_docs;
  // Tabla Hash que se accederá por el id del documento, devolviendo un objeto
  // de la clase InfTermDoc que contiene toda la información de aparición del
  // término en el documento
};

class InfDoc {
  friend ostream &operator<<(ostream &s, const InfDoc &p);

public:
  InfDoc(const InfDoc &);
  InfDoc(const string &);
  InfDoc();
  ~InfDoc();
  InfDoc &operator=(const InfDoc &);

  int getidDoc() const;

  // Getters
  int getNumPal() const { return numPal; }
  int getNumPalSinParada() const { return numPalSinParada; }
  int getNumPalDiferentes() const { return numPalDiferentes; }
  int getTamBytes() const { return tamBytes; }
  const Fecha &getFechaModificacion() const { return fechaModificacion; }

  // Setters
  void setIdDoc(int id) { idDoc = id; }
  void setNumPal(int n) { numPal = n; }
  void setNumPalSinParada(int n) { numPalSinParada = n; }
  void setNumPalDiferentes(int n) { numPalDiferentes = n; }

  void incNumPal() { numPal++; }
  void incNumPalSinParada() { numPalSinParada++; }
  void incNumPalDiferentes() { numPalDiferentes++; }

  void setTamBytes(int t) { tamBytes = t; }
  void setFechaModificacion(const Fecha &f) { fechaModificacion = f; }

  // Añadir cuantos métodos se consideren necesarios para manejar la parte
  // privada de la clase
private:
  int idDoc;
  // Identificador del documento. El primer documento indexado en la colección
  // será el identificador 1
  int numPal;          // Nº total de palabras del documento
  int numPalSinParada; // Nº total de palabras sin stop-words del documento
  int numPalDiferentes;
  // Nº total de palabras diferentes que no sean stop-words (sin acumular la
  // frecuencia de cada una de ellas)
  int tamBytes; // Tamaño en bytes del documento
  Fecha fechaModificacion;
  // Atributo correspondiente a la fecha y hora (completa) de modificación del
  // documento. El tipo "Fecha/hora" lo elegirá/implementará el alumno
};

class InfColeccionDocs {
  friend ostream &operator<<(ostream &s, const InfColeccionDocs &p);

public:
  InfColeccionDocs(const InfColeccionDocs &);
  InfColeccionDocs();
  ~InfColeccionDocs();
  InfColeccionDocs &operator=(const InfColeccionDocs &);

  // Getters
  int getNumDocs() const { return numDocs; }
  int getNumTotalPal() const { return numTotalPal; }
  int getNumTotalPalSinParada() const { return numTotalPalSinParada; }
  int getNumTotalPalDiferentes() const { return numTotalPalDiferentes; }
  int getTamBytes() const { return tamBytes; }

  // Setters
  void setNumDocs(int n) { numDocs = n; }
  void setNumTotalPal(int n) { numTotalPal = n; }
  void setNumTotalPalSinParada(int n) { numTotalPalSinParada = n; }
  void setNumTotalPalDiferentes(int n) { numTotalPalDiferentes = n; }
  void setTamBytes(int t) { tamBytes = t; }

  // Añadir cuantos métodos se consideren necesarios para manejar la parte
  // privada de la clase
private:
  int numDocs; // Nº total de documentos en la colección
  int numTotalPal;
  // Nº total de palabras en la colección
  int numTotalPalSinParada;
  // Nº total de palabras sin stop-words en la colección
  int numTotalPalDiferentes;
  // Nº total de palabras diferentes en la colección que no sean stop-words (sin
  // acumular la frecuencia de cada una de ellas)
  int tamBytes; // Tamaño total en bytes de la colección
};

class InformacionTerminoPregunta {
  friend ostream &operator<<(ostream &s, const InformacionTerminoPregunta &p);

public:
  InformacionTerminoPregunta(const InformacionTerminoPregunta &);
  InformacionTerminoPregunta();
  ~InformacionTerminoPregunta();
  InformacionTerminoPregunta &operator=(const InformacionTerminoPregunta &);

  // Getters
  int getFt() const { return ft; }
  const list<int> &getPosTerm() const { return posTerm; }

  // Setters
  void setFt(int f) { ft = f; }
  void incFt() { ft++; }
  void setPosTerm(const list<int> &p) { posTerm = p; }
  void addPosTerm(int pos) { posTerm.emplace_back(pos); }

  // Añadir cuantos métodos se consideren necesarios para manejar la parte
  // privada de la clase
private:
  int ft; // Frecuencia total del término en la pregunta
  list<int> posTerm;
  // Solo se almacenará esta información si el campo privado del indexador
  // almacenarPosTerm == true Lista de números de palabra en los que aparece el
  // término en la pregunta. Los números de palabra comenzarán desde cero (la
  // primera palabra de la pregunta). Se numerarán las palabras de parada.
  // Estará ordenada de menor a mayor posición.
};

class InformacionPregunta {
  friend ostream &operator<<(ostream &s, const InformacionPregunta &p);

public:
  InformacionPregunta(const InformacionPregunta &);
  InformacionPregunta();
  ~InformacionPregunta();
  InformacionPregunta &operator=(const InformacionPregunta &);

  // Getters
  int getNumTotalPal() const { return numTotalPal; }
  int getNumTotalPalSinParada() const { return numTotalPalSinParada; }
  int getNumTotalPalDiferentes() const { return numTotalPalDiferentes; }

  // Setters
  void setNumTotalPal(int n) { numTotalPal = n; }
  void setNumTotalPalSinParada(int n) { numTotalPalSinParada = n; }
  void setNumTotalPalDiferentes(int n) { numTotalPalDiferentes = n; }

  // Añadir cuantos métodos se consideren necesarios para manejar la parte
  // privada de la clase
private:
  int numTotalPal;
  // Nº total de palabras en la pregunta
  int numTotalPalSinParada;
  // Nº total de palabras sin stop-words en la pregunta
  int numTotalPalDiferentes;
  // Nº total de palabras diferentes en la pregunta que no sean stop-words (sin
  // acumular la frecuencia de cada una de ellas)
};

#endif // !
