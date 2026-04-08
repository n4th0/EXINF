#ifndef _INFORMACION_
#define _INFORMACION_

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
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
  InfTermDoc(const InfTermDoc &) = default;
  InfTermDoc(InfTermDoc &&) = default; // ── OPTIMIZACIÓN: move ctor
  InfTermDoc();
  ~InfTermDoc() = default;
  InfTermDoc &operator=(const InfTermDoc &) = default;
  InfTermDoc &
  operator=(InfTermDoc &&) = default; // ── OPTIMIZACIÓN: move assign

  int getFt() const { return ft; }
  const vector<int> &getPosTerm() const { return posTerm; }

  void setFt(int f) { ft = f; }
  // ── OPTIMIZACIÓN: move overload para evitar copia del vector ─────────────
  void setPosTerm(const vector<int> &p) { posTerm = p; }
  void setPosTerm(vector<int> &&p) { posTerm = std::move(p); }

  void incFt() { ft++; }
  void incPosTerm(int p) { posTerm.push_back(p); }

private:
  int ft;
  vector<int> posTerm;
};

class InformacionTermino {
  friend ostream &operator<<(ostream &s, const InformacionTermino &p);

public:
  InformacionTermino(const InformacionTermino &) = default;
  InformacionTermino(InformacionTermino &&) = default;
  InformacionTermino();
  ~InformacionTermino();
  InformacionTermino &operator=(const InformacionTermino &) = default;
  InformacionTermino &operator=(InformacionTermino &&) = default;

  unordered_map<uint16_t, InfTermDoc> getLdocs() const { return l_docs; }

  int getFtc() const { return ftc; }
  const unordered_map<uint16_t, InfTermDoc> &getL_docs() const {
    return l_docs;
  }
  unordered_map<uint16_t, InfTermDoc> &getL_docs_mut() { return l_docs; }

  inline void setFtc(int f) { ftc = f; }
  inline void setL_docs(const unordered_map<uint16_t, InfTermDoc> &l) {
    l_docs = l;
  }

  inline void incFtc() { ftc++; }
  // ── OPTIMIZACIÓN: move overload ──────────────────────────────────────────
  void addL_docs(uint16_t a, InfTermDoc &&l) { l_docs[a] = std::move(l); }
  void addL_docs(int a, const InfTermDoc &l) { l_docs[a] = l; }

private:
  int ftc;
  unordered_map<uint16_t, InfTermDoc> l_docs;
};

class InfDoc {
  friend ostream &operator<<(ostream &s, const InfDoc &p);

public:
  InfDoc(const InfDoc &) = default;
  InfDoc(InfDoc &&) = default; // ── OPTIMIZACIÓN
  InfDoc(const string &);
  InfDoc();
  ~InfDoc() = default;
  InfDoc &operator=(const InfDoc &) = default;
  InfDoc &operator=(InfDoc &&) = default; // ── OPTIMIZACIÓN

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
  uint16_t idDoc;
  int numPal;
  int numPalSinParada;
  int numPalDiferentes;
  int tamBytes;
  Fecha fechaModificacion;
};

class InfColeccionDocs {
  friend ostream &operator<<(ostream &s, const InfColeccionDocs &p);

public:
  InfColeccionDocs(const InfColeccionDocs &) = default;
  InfColeccionDocs(InfColeccionDocs &&) = default;
  InfColeccionDocs();
  ~InfColeccionDocs() = default;
  InfColeccionDocs &operator=(const InfColeccionDocs &) = default;
  InfColeccionDocs &operator=(InfColeccionDocs &&) = default;

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
  uint16_t numDocs;
  int numTotalPal;
  int numTotalPalSinParada;
  int numTotalPalDiferentes;
  int tamBytes;
};

class InformacionTerminoPregunta {
  friend ostream &operator<<(ostream &s, const InformacionTerminoPregunta &p);

public:
  InformacionTerminoPregunta(const InformacionTerminoPregunta &) = default;
  InformacionTerminoPregunta(InformacionTerminoPregunta &&) = default;
  InformacionTerminoPregunta();
  ~InformacionTerminoPregunta() = default;
  InformacionTerminoPregunta &
  operator=(const InformacionTerminoPregunta &) = default;
  InformacionTerminoPregunta &
  operator=(InformacionTerminoPregunta &&) = default;

  int getFt() const { return ft; }
  const vector<uint16_t> &getPosTerm() const { return posTerm; }

  void setFt(int f) { ft = f; }
  void incFt() { ft++; }
  void setPosTerm(const vector<uint16_t> &p) { posTerm = p; }
  void addPosTerm(int pos) { posTerm.push_back(pos); }

private:
  int ft;
  vector<uint16_t> posTerm;
};

class InformacionPregunta {
  friend ostream &operator<<(ostream &s, const InformacionPregunta &p);

public:
  InformacionPregunta(const InformacionPregunta &) = default;
  InformacionPregunta(InformacionPregunta &&) = default;
  InformacionPregunta();
  ~InformacionPregunta() = default;
  InformacionPregunta &operator=(const InformacionPregunta &) = default;
  InformacionPregunta &operator=(InformacionPregunta &&) = default;

  uint16_t getNumTotalPal() const { return numTotalPal; }
  uint16_t getNumTotalPalSinParada() const { return numTotalPalSinParada; }
  uint16_t getNumTotalPalDiferentes() const { return numTotalPalDiferentes; }

  void setNumTotalPal(int n) { numTotalPal = n; }
  void setNumTotalPalSinParada(int n) { numTotalPalSinParada = n; }
  void setNumTotalPalDiferentes(int n) { numTotalPalDiferentes = n; }

private:
  uint16_t numTotalPal;
  uint16_t numTotalPalSinParada;
  uint16_t numTotalPalDiferentes;
};

#endif
