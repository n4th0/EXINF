

#include "../include/indexadorInformacion.h"

using namespace std;

Fecha::Fecha() {}

// Fecha::Fecha(Fecha &&) = default;
// Fecha::Fecha(const Fecha &) = default;
// Fecha &operator=(Fecha &&) = default;
// Fecha &operator=(const Fecha &) = default;

Fecha::~Fecha() {}

// class InfTermDoc {
InfTermDoc::InfTermDoc(const InfTermDoc &) {}
InfTermDoc::InfTermDoc() {}
InfTermDoc::~InfTermDoc() {}
InfTermDoc &InfTermDoc::operator=(const InfTermDoc &p) {}

// class InformacionTermino {
InformacionTermino::InformacionTermino(const InformacionTermino &) {}
InformacionTermino::InformacionTermino() {
  this->ftc = 0;
} // Inicializa ftc = 0
InformacionTermino::~InformacionTermino() {
  this->ftc = 0;
  l_docs.clear();
} // Pone ftc = 0 y vacía l_docs
InformacionTermino &InformacionTermino::operator=(const InformacionTermino &p) {

  if (this != &p) {
    this->l_docs = p.l_docs;
    this->ftc = p.ftc;
  }
  return *this;
}

// class InfDoc {
InfDoc::InfDoc(const InfDoc &p)
    : fechaModificacion(p.fechaModificacion), idDoc(p.idDoc), numPal(p.numPal),
      numPalDiferentes(p.numPalDiferentes), tamBytes(p.tamBytes) {}

InfDoc::InfDoc()
    : fechaModificacion(), idDoc(), numPal(), numPalDiferentes(), tamBytes() {}
InfDoc::~InfDoc() = default;

InfDoc &InfDoc::operator=(const InfDoc &p) {

  if (this != &p) {
    this->fechaModificacion = p.fechaModificacion;
    this->idDoc = p.idDoc;
    this->numPal = p.numPal;
    this->numPalDiferentes = p.numPalDiferentes;
    this->tamBytes = p.tamBytes;
  }
  return *this;
}

// class InfColeccionDocs {
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

// class InformacionTerminoPregunta {
//
// int ft; // Frecuencia total del término en la pregunta
// list<int> posTerm;
//

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

// class InformacionPregunta {

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
