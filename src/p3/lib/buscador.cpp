#include "../include/buscador.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip> // Para manipulación de flujo
#include <iostream>

using namespace std;

// ---------------------------------------------------------------------------
// ResultadoRI
// ---------------------------------------------------------------------------

ResultadoRI::ResultadoRI(const double &kvSimilitud, const long int &kidDoc,
                         const int &np) {
  vSimilitud = kvSimilitud;
  idDoc = kidDoc;
  numPregunta = np;
}

double ResultadoRI::VSimilitud() const { return vSimilitud; }
long int ResultadoRI::IdDoc() const { return idDoc; }

bool ResultadoRI::operator<(const ResultadoRI &lhs) const {
  if (numPregunta == lhs.numPregunta)
    return (vSimilitud < lhs.vSimilitud);
  else
    return (numPregunta > lhs.numPregunta);
}

ostream &operator<<(ostream &os, const ResultadoRI &res) {
  os << res.vSimilitud << "\t\t" << res.idDoc << "\t" << res.numPregunta
     << endl;
  return os;
}

// ---------------------------------------------------------------------------
// Buscador: Constructores y Destructor
// ---------------------------------------------------------------------------

Buscador::Buscador(const string &directorioIndexacion, const int &f)
    : IndexadorHash(directorioIndexacion) {
  formSimilitud = f;
  c = 2.0;
  k1 = 1.2;
  b = 0.75;
}

Buscador::Buscador(const Buscador &b) : IndexadorHash(b) {
  formSimilitud = b.formSimilitud;
  c = b.c;
  k1 = b.k1;
  this->b = b.b;
  docsOrdenados = b.docsOrdenados;
}

Buscador::~Buscador() = default;

Buscador &Buscador::operator=(const Buscador &other) {
  if (this != &other) {
    IndexadorHash::operator=(other);
    formSimilitud = other.formSimilitud;
    c = other.c;
    k1 = other.k1;
    b = other.b;
    docsOrdenados = other.docsOrdenados;
  }
  return *this;
}

// ---------------------------------------------------------------------------
// Métodos de Similitud
// ---------------------------------------------------------------------------

double Buscador::PuntuacionBM25(const InformacionTermino &infTerm,
                                const InfTermDoc &infTermDoc,
                                const InfDoc &infDocumento,
                                const InfColeccionDocs &infColeccion) const {

  double N = static_cast<double>(infColeccion.getNumDocs());
  double nqi = static_cast<double>(infTerm.getLdocs().size());
  double tf = static_cast<double>(infTermDoc.getFt());
  double dl = static_cast<double>(infDocumento.getNumPalSinParada());
  double avgdl =
      (N > 0) ? static_cast<double>(infColeccion.getNumTotalPalSinParada()) / N
              : 1.0;

  // IDF con logaritmo en base 2
  double idf = log2((N - nqi + 0.5) / (nqi + 0.5));

  double num = tf * (k1 + 1.0);
  double den = tf + k1 * (1.0 - b + b * (dl / avgdl));

  return idf * (num / den);
}

double Buscador::PuntuacionDFR(const InformacionTermino &infTerm,
                               const InfTermDoc &infTermDoc,
                               const InfDoc &infDocumento,
                               const InfColeccionDocs &infColeccion,
                               const InformacionTerminoPregunta &infTermPreg,
                               double k) const {

  if (k <= 0)
    return 0.0;

  double N = static_cast<double>(infColeccion.getNumDocs());
  double ft = static_cast<double>(infTerm.getFtc());
  double ftd = static_cast<double>(infTermDoc.getFt());
  double ftq = static_cast<double>(infTermPreg.getFt());
  double ld = static_cast<double>(infDocumento.getNumPalSinParada());
  double avg_ld =
      (N > 0) ? static_cast<double>(infColeccion.getNumTotalPalSinParada()) / N
              : 1.0;

  if (ld == 0.0)
    ld = 1.0;

  double tf_star = ftd * log2(1.0 + c * avg_ld / ld);
  double lambda = ft / N;

  if (lambda <= 0.0)
    return 0.0;

  double nt = static_cast<double>(infTerm.getLdocs().size());

  // Fórmula w_id
  double w_id = (log2(1.0 + lambda) + tf_star * log2((1.0 + lambda) / lambda)) *
                (ft + 1.0) / (nt * (tf_star + 1.0));

  // Peso de la query
  double w_iq = ftq / k;

  return w_iq * w_id;
}

// ---------------------------------------------------------------------------
// Métodos de Búsqueda
// ---------------------------------------------------------------------------

bool Buscador::Buscar(const int &numDocumentos) {
  string pregActual;
  if (!DevuelvePregunta(pregActual))
    return false;

  docsOrdenados = std::priority_queue<ResultadoRI>();
  unordered_map<long int, double> acumulador;

  double k = 0;
  for (auto &[term, info] : getIndicePregunta())
    k += info.getFt();

  for (auto &[termino, infTermPreg] : getIndicePregunta()) {
    InformacionTermino infTerm;
    if (!Devuelve(termino, infTerm))
      continue;

    for (auto &infTermDoc : infTerm.getLdocs()) {
      // Optimizamos: Buscar el InfDoc una sola vez
      const auto &docs = getIndiceDocs();
      for (auto it = docs.begin(); it != docs.end(); ++it) {
        if (it->second.getidDoc() == infTermDoc.doc_id) {
          double score =
              (formSimilitud == 0)
                  ? PuntuacionDFR(infTerm, infTermDoc, it->second,
                                  getInformacionColeccionDocs(), infTermPreg, k)
                  : PuntuacionBM25(infTerm, infTermDoc, it->second,
                                   getInformacionColeccionDocs());

          acumulador[infTermDoc.doc_id] += score;
          break;
        }
      }
    }
  }

  vector<ResultadoRI> resultados;
  for (auto &[idDoc, puntuacion] : acumulador) {
    resultados.emplace_back(puntuacion, idDoc, 0);
  }

  sort(resultados.begin(), resultados.end(),
       [](const ResultadoRI &a, const ResultadoRI &b) {
         return a.VSimilitud() > b.VSimilitud();
       });

  int limite = min(static_cast<int>(resultados.size()), numDocumentos);
  for (int i = 0; i < limite; ++i)
    docsOrdenados.push(resultados[i]);

  return true;
}

bool Buscador::Buscar(const string &dirPreguntas, const int &numDocumentos,
                      const int &numPregInicio, const int &numPregFin) {
  docsOrdenados = std::priority_queue<ResultadoRI>();

  for (int numPreg = numPregInicio; numPreg <= numPregFin; ++numPreg) {
    string fichPreg = dirPreguntas + "/" + to_string(numPreg) + ".txt";
    ifstream fs(fichPreg);
    if (!fs.is_open())
      continue;

    string contenidoPreg((istreambuf_iterator<char>(fs)),
                         istreambuf_iterator<char>());
    fs.close();

    if (!IndexarPregunta(contenidoPreg))
      continue;

    unordered_map<long int, double> acumulador;
    double k_query = 0;
    for (auto &[term, info] : getIndicePregunta())
      k_query += info.getFt();

    for (auto &[termino, infTermPreg] : getIndicePregunta()) {
      InformacionTermino infTerm;
      if (!Devuelve(termino, infTerm))
        continue;

      for (auto &infTermDoc : infTerm.getLdocs()) {
        for (auto &[nombre, infD] : getIndiceDocs()) {
          if (infD.getidDoc() == infTermDoc.doc_id) {
            double score = (formSimilitud == 0)
                               ? PuntuacionDFR(infTerm, infTermDoc, infD,
                                               getInformacionColeccionDocs(),
                                               infTermPreg, k_query)
                               : PuntuacionBM25(infTerm, infTermDoc, infD,
                                                getInformacionColeccionDocs());
            acumulador[infTermDoc.doc_id] += score;
            break;
          }
        }
      }
    }

    vector<ResultadoRI> resultadosPreg;
    for (auto &[idDoc, puntuacion] : acumulador)
      resultadosPreg.emplace_back(puntuacion, idDoc, numPreg);

    sort(resultadosPreg.begin(), resultadosPreg.end(),
         [](const ResultadoRI &a, const ResultadoRI &b) {
           return a.VSimilitud() > b.VSimilitud();
         });

    int limite = min(static_cast<int>(resultadosPreg.size()), numDocumentos);
    for (int i = 0; i < limite; ++i)
      docsOrdenados.push(resultadosPreg[i]);
  }
  return true;
}

// ---------------------------------------------------------------------------
// Métodos de Salida
// ---------------------------------------------------------------------------

void Buscador::ImprimirResultadoBusqueda(const int &numDocumentos) const {
  // CRITICO: Configurar el flujo para 6 cifras significativas (formato por
  // defecto) No usar fixed ni showpoint para que el redondeo y los ceros sean
  // como espera el test
  cout << std::defaultfloat << std::setprecision(6);

  string formula = (formSimilitud == 0) ? "DFR" : "BM25";
  int pregActual = -1;
  int posicion = 0;

  auto copia = docsOrdenados;
  while (!copia.empty()) {
    auto res = copia.top();
    copia.pop();

    if (res.getNumPregunta() != pregActual) {
      pregActual = res.getNumPregunta();
      posicion = 0;
    }

    if (posicion >= numDocumentos)
      continue;

    string nomDoc = "";
    for (auto &[nombre, infD] : getIndiceDocs()) {
      if (infD.getidDoc() == res.IdDoc()) {
        nomDoc = nombre;
        break;
      }
    }

    size_t lastSlash = nomDoc.find_last_of("/\\");
    if (lastSlash != string::npos)
      nomDoc = nomDoc.substr(lastSlash + 1);
    size_t lastDot = nomDoc.find_last_of('.');
    if (lastDot != string::npos)
      nomDoc = nomDoc.substr(0, lastDot);

    string pregIndex;
    DevuelvePregunta(pregIndex);
    string etiqPreg =
        (res.getNumPregunta() == 0) ? pregIndex : "ConjuntoDePreguntas";

    cout << res.getNumPregunta() << " " << formula << " " << nomDoc << " "
         << posicion << " " << res.VSimilitud() << " " << etiqPreg << "\n";

    posicion++;
  }
}

bool Buscador::ImprimirResultadoBusqueda(const int &numDocumentos,
                                         const string &nombreFichero) const {
  ofstream fs(nombreFichero);
  if (!fs.is_open())
    return false;

  fs << std::defaultfloat << std::setprecision(6);

  string formula = (formSimilitud == 0) ? "DFR" : "BM25";
  int pregActual = -1;
  int posicion = 0;

  auto copia = docsOrdenados;
  while (!copia.empty()) {
    auto res = copia.top();
    copia.pop();

    if (res.getNumPregunta() != pregActual) {
      pregActual = res.getNumPregunta();
      posicion = 0;
    }

    if (posicion >= numDocumentos)
      continue;

    string nomDoc = "";
    for (auto &[nombre, infD] : getIndiceDocs()) {
      if (infD.getidDoc() == res.IdDoc()) {
        nomDoc = nombre;
        break;
      }
    }

    size_t lastSlash = nomDoc.find_last_of("/\\");
    if (lastSlash != string::npos)
      nomDoc = nomDoc.substr(lastSlash + 1);
    size_t lastDot = nomDoc.find_last_of('.');
    if (lastDot != string::npos)
      nomDoc = nomDoc.substr(0, lastDot);

    string pregIndex;
    DevuelvePregunta(pregIndex);
    string etiqPreg =
        (res.getNumPregunta() == 0) ? pregIndex : "ConjuntoDePreguntas";

    fs << res.getNumPregunta() << " " << formula << " " << nomDoc << " "
       << posicion << " " << res.VSimilitud() << " " << etiqPreg << "\n";

    posicion++;
  }
  fs.close();
  return true;
}

// ---------------------------------------------------------------------------
// Getters y Setters
// ---------------------------------------------------------------------------

int Buscador::DevolverFormulaSimilitud() const { return formSimilitud; }

bool Buscador::CambiarFormulaSimilitud(const int &f) {
  if (f == 0 || f == 1) {
    formSimilitud = f;
    return true;
  }
  return false;
}

void Buscador::CambiarParametrosDFR(const double &kc) { c = kc; }
double Buscador::DevolverParametrosDFR() const { return c; }
void Buscador::CambiarParametrosBM25(const double &kk1, const double &kb) {
  k1 = kk1;
  b = kb;
}
void Buscador::DevolverParametrosBM25(double &kk1, double &kb) const {
  kk1 = k1;
  kb = b;
}
