#include "../include/indexadorInformacion.h"
#include <ostream>

#include <ctime>
#include <stdexcept>
#include <string>
using namespace std;

static bool esBisiesto(int y) {
  return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int diasEnMes(int m, int y) {
  switch (m) {
  case 1:
  case 3:
  case 5:
  case 7:
  case 8:
  case 10:
  case 12:
    return 31;
  case 4:
  case 6:
  case 9:
  case 11:
    return 30;
  case 2:
    return esBisiesto(y) ? 29 : 28;
  default:
    return -1;
  }
}

static void validar(int d, int m, int y) {
  if (y < 1)
    throw std::invalid_argument("Año invalido: " + std::to_string(y));
  if (m < 1 || m > 12)
    throw std::invalid_argument("Mes invalido: " + std::to_string(m));
  if (d < 1 || d > diasEnMes(m, y))
    throw std::invalid_argument("Dia invalido: " + std::to_string(d));
}

Fecha::Fecha() {
  time_t t = time(nullptr);
  tm *now = localtime(&t);
  day = now->tm_mday;
  month = now->tm_mon + 1;
  year = now->tm_year + 1900;
}

Fecha::Fecha(int d, int m, int y) : day(d), month(m), year(y) {
  validar(d, m, y);
}

Fecha::~Fecha() {}

void Fecha::setDay(int d) {
  validar(d, month, year);
  day = d;
}
void Fecha::setMonth(int m) {
  validar(day, m, year);
  month = m;
}
void Fecha::setYear(int y) {
  validar(day, month, y);
  year = y;
}

bool operator==(const Fecha &a, const Fecha &b) {
  return a.year == b.year && a.month == b.month && a.day == b.day;
}
bool operator<(const Fecha &a, const Fecha &b) {
  if (a.year != b.year)
    return a.year < b.year;
  if (a.month != b.month)
    return a.month < b.month;
  return a.day < b.day;
}
bool operator>(const Fecha &a, const Fecha &b) { return b < a; }
bool operator<=(const Fecha &a, const Fecha &b) { return !(a > b); }
bool operator>=(const Fecha &a, const Fecha &b) { return !(a < b); }
bool operator!=(const Fecha &a, const Fecha &b) { return !(a == b); }

std::ostream &operator<<(std::ostream &s, const Fecha &f) {
  s << (f.day < 10 ? "0" : "") << f.day << "/" << (f.month < 10 ? "0" : "")
    << f.month << "/" << f.year;
  return s;
}

// ── InfTermDoc
// ────────────────────────────────────────────────────────────────
// copy/move/assign/dtor son =default en el .h, solo necesitamos el ctor default

InfTermDoc::InfTermDoc() : ft(0), posTerm() {}

ostream &operator<<(ostream &s, const InfTermDoc &p) {
  s << "ft: " << p.ft;
  for (int v : p.posTerm)
    s << '\t' << v;
  return s;
}

// ── InformacionTermino
// ────────────────────────────────────────────────────────

InformacionTermino::InformacionTermino() : ftc(0), l_docs() {}

InformacionTermino::~InformacionTermino() {
  ftc = 0;
  l_docs.clear();
}

ostream &operator<<(ostream &s, const InformacionTermino &p) {
  s << "Frecuencia total: " << p.ftc << "\tfd: " << p.l_docs.size();
  for (const auto &it : p.l_docs)
    s << "\tId.Doc: " << it.first << '\t' << it.second;
  return s;
}

// ── InfDoc
// ────────────────────────────────────────────────────────────────────

InfDoc::InfDoc()
    : idDoc(0), numPal(0), numPalSinParada(0), numPalDiferentes(0), tamBytes(0),
      fechaModificacion() {}

InfDoc::InfDoc(const string &)
    : idDoc(0), numPal(0), numPalSinParada(0), numPalDiferentes(0), tamBytes(0),
      fechaModificacion() {}

int InfDoc::getidDoc() const { return idDoc; }

ostream &operator<<(ostream &s, const InfDoc &p) {
  s << "idDoc: " << p.idDoc << "\tnumPal: " << p.numPal
    << "\tnumPalSinParada: " << p.numPalSinParada
    << "\tnumPalDiferentes: " << p.numPalDiferentes
    << "\ttamBytes: " << p.tamBytes;
  return s;
}

// ── InfColeccionDocs
// ──────────────────────────────────────────────────────────

InfColeccionDocs::InfColeccionDocs()
    : numDocs(0), numTotalPal(0), numTotalPalSinParada(0),
      numTotalPalDiferentes(0), tamBytes(0) {}

ostream &operator<<(ostream &s, const InfColeccionDocs &p) {
  s << "numDocs: " << p.numDocs << "\tnumTotalPal: " << p.numTotalPal
    << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada
    << "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes
    << "\ttamBytes: " << p.tamBytes;
  return s;
}

// ── InformacionTerminoPregunta
// ────────────────────────────────────────────────

InformacionTerminoPregunta::InformacionTerminoPregunta() : ft(0), posTerm() {}

ostream &operator<<(ostream &s, const InformacionTerminoPregunta &p) {
  s << "ft: " << p.ft;
  for (int v : p.posTerm)
    s << '\t' << v;
  return s;
}

// ── InformacionPregunta
// ───────────────────────────────────────────────────────

InformacionPregunta::InformacionPregunta()
    : numTotalPal(0), numTotalPalSinParada(0), numTotalPalDiferentes(0) {}

ostream &operator<<(ostream &s, const InformacionPregunta &p) {
  s << "numTotalPal: " << p.numTotalPal
    << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada
    << "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes;
  return s;
}
