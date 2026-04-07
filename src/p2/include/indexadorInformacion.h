#ifndef _INFORMACION_
#define _INFORMACION_

#include <iostream>
#include <unordered_map>
#include <vector>

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
  InfTermDoc();
  ~InfTermDoc();
  InfTermDoc &operator=(const InfTermDoc &);

  int getFt() const { return ft; }
  const vector<int> &getPosTerm() const { return posTerm; }

  void setFt(int f) { ft = f; }
  void setPosTerm(const vector<int> &p) { posTerm = p; }

  void incFt() { ft++; }
  void incPosTerm(int p) { posTerm.push_back(p); }

private:
  int ft;
  vector<int> posTerm;
};

class InformacionTermino {
  friend ostream &operator<<(ostream &s, const InformacionTermino &p);

public:
  InformacionTermino(const InformacionTermino &);
  InformacionTermino();
  ~InformacionTermino();
  InformacionTermino &operator=(const InformacionTermino &);

  // Mantiene getLdocs() por compatibilidad (devuelve copia)
  unordered_map<int, InfTermDoc> getLdocs() const { return l_docs; }

  int getFtc() const { return ftc; }
  const unordered_map<int, InfTermDoc> &getL_docs() const { return l_docs; }

  unordered_map<int, InfTermDoc> &getL_docs_mut() { return l_docs; }

  void setFtc(int f) { ftc = f; }
  void setL_docs(const unordered_map<int, InfTermDoc> &l) { l_docs = l; }

  void incFtc() { ftc++; }
  void addL_docs(int a, InfTermDoc l) { l_docs[a] = std::move(l); }

private:
  int ftc;
  unordered_map<int, InfTermDoc> l_docs;
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

  int getNumPal() const { return numPal; }
  int getNumPalSinParada() const { return numPalSinParada; }
  int getNumPalDiferentes() const { return numPalDiferentes; }
  int getTamBytes() const { return tamBytes; }
  const Fecha &getFechaModificacion() const { return fechaModificacion; }

  void setIdDoc(int id) { idDoc = id; }
  void setNumPal(int n) { numPal = n; }
  void setNumPalSinParada(int n) { numPalSinParada = n; }
  void setNumPalDiferentes(int n) { numPalDiferentes = n; }

  void incNumPal() { numPal++; }
  void incNumPalSinParada() { numPalSinParada++; }
  void incNumPalDiferentes() { numPalDiferentes++; }

  void setTamBytes(int t) { tamBytes = t; }
  void setFechaModificacion(const Fecha &f) { fechaModificacion = f; }

private:
  int idDoc;
  int numPal;
  int numPalSinParada;
  int numPalDiferentes;
  int tamBytes;
  Fecha fechaModificacion;
};

class InfColeccionDocs {
  friend ostream &operator<<(ostream &s, const InfColeccionDocs &p);

public:
  InfColeccionDocs(const InfColeccionDocs &);
  InfColeccionDocs();
  ~InfColeccionDocs();
  InfColeccionDocs &operator=(const InfColeccionDocs &);

  int getNumDocs() const { return numDocs; }
  int getNumTotalPal() const { return numTotalPal; }
  int getNumTotalPalSinParada() const { return numTotalPalSinParada; }
  int getNumTotalPalDiferentes() const { return numTotalPalDiferentes; }
  int getTamBytes() const { return tamBytes; }

  void setNumDocs(int n) { numDocs = n; }
  void setNumTotalPal(int n) { numTotalPal = n; }
  void setNumTotalPalSinParada(int n) { numTotalPalSinParada = n; }
  void setNumTotalPalDiferentes(int n) { numTotalPalDiferentes = n; }
  void setTamBytes(int t) { tamBytes = t; }

private:
  int numDocs;
  int numTotalPal;
  int numTotalPalSinParada;
  int numTotalPalDiferentes;
  int tamBytes;
};

class InformacionTerminoPregunta {
  friend ostream &operator<<(ostream &s, const InformacionTerminoPregunta &p);

public:
  InformacionTerminoPregunta(const InformacionTerminoPregunta &);
  InformacionTerminoPregunta();
  ~InformacionTerminoPregunta();
  InformacionTerminoPregunta &operator=(const InformacionTerminoPregunta &);

  int getFt() const { return ft; }
  // CAMBIO: vector en lugar de list
  const vector<int> &getPosTerm() const { return posTerm; }

  void setFt(int f) { ft = f; }
  void incFt() { ft++; }
  void setPosTerm(const vector<int> &p) { posTerm = p; }
  void addPosTerm(int pos) { posTerm.push_back(pos); }

private:
  int ft;
  vector<int> posTerm;
};

class InformacionPregunta {
  friend ostream &operator<<(ostream &s, const InformacionPregunta &p);

public:
  InformacionPregunta(const InformacionPregunta &);
  InformacionPregunta();
  ~InformacionPregunta();
  InformacionPregunta &operator=(const InformacionPregunta &);

  int getNumTotalPal() const { return numTotalPal; }
  int getNumTotalPalSinParada() const { return numTotalPalSinParada; }
  int getNumTotalPalDiferentes() const { return numTotalPalDiferentes; }

  void setNumTotalPal(int n) { numTotalPal = n; }
  void setNumTotalPalSinParada(int n) { numTotalPalSinParada = n; }
  void setNumTotalPalDiferentes(int n) { numTotalPalDiferentes = n; }

private:
  int numTotalPal;
  int numTotalPalSinParada;
  int numTotalPalDiferentes;
};

#endif
