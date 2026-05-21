#include "../include/indexadorInformacion.h"
#include <algorithm>
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
  return a.getYear() == b.getYear() && a.getMonth() == b.getMonth() &&
         a.getDay() == b.getDay();
}
bool operator<(const Fecha &a, const Fecha &b) {
  if (a.getYear() != b.getYear())
    return a.getYear() < b.getYear();
  if (a.getMonth() != b.getMonth())
    return a.getMonth() < b.getMonth();
  return a.getDay() < b.getDay();
}
bool operator>(const Fecha &a, const Fecha &b) { return b < a; }
bool operator<=(const Fecha &a, const Fecha &b) { return !(a > b); }
bool operator>=(const Fecha &a, const Fecha &b) { return !(a < b); }
bool operator!=(const Fecha &a, const Fecha &b) { return !(a == b); }

std::ostream &operator<<(std::ostream &s, const Fecha &f) {
  s << (f.getDay() < 10 ? "0" : "") << f.getDay() << "/"
    << (f.getMonth() < 10 ? "0" : "") << f.getMonth() << "/" << f.getYear();
  return s;
}

ostream &operator<<(ostream &s, const InfTermDoc &p) {
  s << "ft: " << p.getFt();
  for (auto it = p.posTerm.begin(); it != p.posTerm.end(); it++)
    s << "\t" << (*it);
  return s;
}

ostream &operator<<(ostream &s, const InformacionTermino &p) {
  s << "Frecuencia total: " << p.ftc << "\tfd: " << p.l_docs2.size();
  for (auto it = p.l_docs2.begin(); it != p.l_docs2.end(); it++)
    s << "\tId.Doc: " << (*it).doc_id << "\t" << (*it);
  return s;
}

ostream &operator<<(ostream &s, const InfDoc &p) {
  s << "idDoc: " << p.idDoc << "\tnumPal: " << p.numPal
    << "\tnumPalSinParada: " << p.numPalSinParada
    << "\tnumPalDiferentes: " << p.numPalDiferentes
    << "\ttamBytes: " << p.tamBytes;
  return s;
}

ostream &operator<<(ostream &s, const InfColeccionDocs &p) {
  s << "numDocs: " << p.numDocs << "\tnumTotalPal: " << p.numTotalPal
    << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada
    << "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes
    << "\ttamBytes: " << p.tamBytes;
  return s;
}

ostream &operator<<(ostream &s, const InformacionTerminoPregunta &p) {
  s << "ft: " << p.ft;
  for (auto it = p.posTerm.begin(); it != p.posTerm.end(); it++)
    s << "\t" << (*it);
  return s;
}

ostream &operator<<(ostream &s, const InformacionPregunta &p) {
  s << "numTotalPal: " << p.numTotalPal
    << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada
    << "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes;
  return s;
}

InfTermDoc::InfTermDoc() : ft(0), posTerm(), doc_id() {}
InfTermDoc::~InfTermDoc() {}

InfTermDoc::InfTermDoc(const InfTermDoc &p)
    : ft(p.ft), posTerm(p.posTerm), doc_id(p.doc_id) {}

InfTermDoc &InfTermDoc::operator=(const InfTermDoc &p) {
  if (this != &p) {
    ft = p.ft;
    doc_id = p.doc_id;
    posTerm = p.posTerm;
  }
  return *this;
}

InformacionTermino::InformacionTermino(const InformacionTermino &p)
    : ftc(p.ftc), l_docs2(p.l_docs2) {}

InformacionTermino::InformacionTermino() { this->ftc = 0; }

InformacionTermino::~InformacionTermino() {
  this->ftc = 0;
  l_docs2.clear();
}

InformacionTermino &InformacionTermino::operator=(const InformacionTermino &p) {
  if (this != &p) {
    this->l_docs2 = p.l_docs2;
    this->ftc = p.ftc;
  }
  return *this;
}

// // FIX: inserción ordenada para mantener invariante de búsqueda binaria
// void InformacionTermino::addL_docs(int a, const InfTermDoc &l) {
//   InfTermDoc newDoc = l;
//   newDoc.doc_id = a;
//   auto it = std::lower_bound(l_docs2.begin(), l_docs2.end(), newDoc);
//   l_docs2.insert(it, std::move(newDoc));
// }

// bool InformacionTermino::delete_doc(int doc_id) {
//   int left = 0;
//   int right = (int)l_docs2.size() - 1;
//   while (left <= right) {
//     int mid = left + (right - left) / 2;
//     if (l_docs2[mid].doc_id == doc_id) {
//       ftc -= l_docs2[mid].getFt();
//       l_docs2.erase(l_docs2.begin() + mid);
//       return true;
//     } else if (l_docs2[mid].doc_id < doc_id) {
//       left = mid + 1;
//     } else {
//       right = mid - 1;
//     }
//   }
//   return false;
// }

InfDoc::InfDoc(const string &file)
    : fechaModificacion(), idDoc(0), numPal(0), numPalSinParada(0),
      numPalDiferentes(0), tamBytes(0) {}

int InfDoc::getidDoc() const { return this->idDoc; }

InfDoc::InfDoc(const InfDoc &p)
    : fechaModificacion(p.fechaModificacion), idDoc(p.idDoc), numPal(p.numPal),
      numPalSinParada(p.numPalSinParada), numPalDiferentes(p.numPalDiferentes),
      tamBytes(p.tamBytes) {}

InfDoc::InfDoc()
    : fechaModificacion(), idDoc(0), numPal(0), numPalSinParada(0),
      numPalDiferentes(0), tamBytes(0) {}

InfDoc::~InfDoc() = default;

InfDoc &InfDoc::operator=(const InfDoc &p) {
  if (this != &p) {
    this->fechaModificacion = p.fechaModificacion;
    this->idDoc = p.idDoc;
    this->numPal = p.numPal;
    this->numPalSinParada = p.numPalSinParada;
    this->numPalDiferentes = p.numPalDiferentes;
    this->tamBytes = p.tamBytes;
  }
  return *this;
}

InfColeccionDocs::InfColeccionDocs(const InfColeccionDocs &p)
    : tamBytes(p.tamBytes), numDocs(p.numDocs), numTotalPal(p.numTotalPal),
      numTotalPalSinParada(p.numTotalPalSinParada),
      numTotalPalDiferentes(p.numTotalPalDiferentes) {}

InfColeccionDocs::InfColeccionDocs()
    : tamBytes(), numDocs(), numTotalPal(), numTotalPalSinParada(),
      numTotalPalDiferentes() {}

InfColeccionDocs::~InfColeccionDocs() = default;

InfColeccionDocs &InfColeccionDocs::operator=(const InfColeccionDocs &p) {
  if (this != &p) {
    this->tamBytes = p.tamBytes;
    this->numDocs = p.numDocs;
    this->numTotalPal = p.numTotalPal;
    this->numTotalPalSinParada = p.numTotalPalSinParada;
    this->numTotalPalDiferentes = p.numTotalPalDiferentes;
  }
  return *this;
}

InformacionTerminoPregunta::InformacionTerminoPregunta(
    const InformacionTerminoPregunta &p)
    : ft(p.ft), posTerm(p.posTerm) {}

InformacionTerminoPregunta::InformacionTerminoPregunta() : ft(), posTerm() {}
InformacionTerminoPregunta::~InformacionTerminoPregunta() {}

InformacionTerminoPregunta &
InformacionTerminoPregunta::operator=(const InformacionTerminoPregunta &p) {
  if (this != &p) {
    this->ft = p.ft;
    this->posTerm = p.posTerm;
  }
  return *this;
}

InformacionPregunta::InformacionPregunta(const InformacionPregunta &p)
    : numTotalPal(p.numTotalPal), numTotalPalSinParada(p.numTotalPalSinParada),
      numTotalPalDiferentes(p.numTotalPalDiferentes) {}

InformacionPregunta::InformacionPregunta()
    : numTotalPal(), numTotalPalSinParada(), numTotalPalDiferentes() {}

InformacionPregunta::~InformacionPregunta() = default;

InformacionPregunta &
InformacionPregunta::operator=(const InformacionPregunta &p) {
  if (this != &p) {
    this->numTotalPal = p.numTotalPal;
    this->numTotalPalDiferentes = p.numTotalPalDiferentes;
    this->numTotalPalSinParada = p.numTotalPalSinParada;
  }
  return *this;
}
