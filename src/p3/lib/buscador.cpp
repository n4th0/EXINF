#include "../include/buscador.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
using namespace std;

// ---------------------------------------------------------------------------
// ResultadoRI
// ---------------------------------------------------------------------------

ResultadoRI::ResultadoRI(const double &kvSimilitud, const long int &kidDoc,
                         const int &np)
    : vSimilitud(kvSimilitud), idDoc(kidDoc), numPregunta(np) {}

double ResultadoRI::VSimilitud() const { return vSimilitud; }
long int ResultadoRI::IdDoc() const { return idDoc; }

bool ResultadoRI::operator<(const ResultadoRI &lhs) const {
  if (numPregunta == lhs.numPregunta)
    return vSimilitud < lhs.vSimilitud;
  return numPregunta > lhs.numPregunta;
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
    : IndexadorHash(directorioIndexacion), formSimilitud(f), c(2.0), k1(1.2),
      b(0.75) {}

Buscador::Buscador(const Buscador &other)
    : IndexadorHash(other), formSimilitud(other.formSimilitud), c(other.c),
      k1(other.k1), b(other.b), docsOrdenados(other.docsOrdenados) {}

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
  if (N <= 0.0)
    return 0.0;

  double ft = static_cast<double>(infTerm.getFtc());
  double lambda = ft / N;
  if (lambda <= 0.0)
    return 0.0;

  double log_lambda1 = log2(1.0 + lambda);
  double log_ratio = log_lambda1 - log2(lambda);

  double ftd = static_cast<double>(infTermDoc.getFt());
  double ld = static_cast<double>(infDocumento.getNumPalSinParada());
  if (ld == 0.0)
    ld = 1.0;

  double avg_ld =
      static_cast<double>(infColeccion.getNumTotalPalSinParada()) / N;
  double tf_star = ftd * log2(1.0 + c * avg_ld / ld);
  double nt = static_cast<double>(infTerm.getLdocs().size());
  double w_id =
      (log_lambda1 + tf_star * log_ratio) * (ft + 1.0) / (nt * (tf_star + 1.0));

  double ftq = static_cast<double>(infTermPreg.getFt());
  return (ftq / k) * w_id;
}

// ---------------------------------------------------------------------------
// Helper: build id→InfDoc* reverse map once
// ---------------------------------------------------------------------------

static unordered_map<long int, const InfDoc *>
buildIdToDoc(const unordered_map<string, InfDoc> &indiceDocs) {
  unordered_map<long int, const InfDoc *> m;
  m.reserve(indiceDocs.size());
  for (const auto &[nombre, infD] : indiceDocs)
    m[infD.getidDoc()] = &infD;
  return m;
}

// ---------------------------------------------------------------------------
// Métodos de Búsqueda
// ---------------------------------------------------------------------------

bool Buscador::Buscar(const int &numDocumentos) {
  string pregActual;
  if (!DevuelvePregunta(pregActual))
    return false;

  docsOrdenados = priority_queue<ResultadoRI>();

  // Build reverse map once — O(N) instead of O(N) per posting
  const auto idToDoc = buildIdToDoc(getIndiceDocs());

  const auto &infColeccion = getInformacionColeccionDocs();
  const auto &indicePreg = getIndicePregunta();

  // k = total query term frequency
  double k = 0;
  for (const auto &[term, info] : indicePreg)
    k += info.getFt();

  unordered_map<long int, double> acumulador;
  acumulador.reserve(getIndiceDocs().size());

  for (const auto &[termino, infTermPreg] : indicePreg) {
    InformacionTermino infTerm;
    if (!Devuelve(termino, infTerm))
      continue;

    for (const auto &infTermDoc : infTerm.getLdocs()) {
      auto it = idToDoc.find(infTermDoc.doc_id);
      if (it == idToDoc.end())
        continue;

      double score =
          (formSimilitud == 0)
              ? PuntuacionDFR(infTerm, infTermDoc, *it->second, infColeccion,
                              infTermPreg, k)
              : PuntuacionBM25(infTerm, infTermDoc, *it->second, infColeccion);

      acumulador[infTermDoc.doc_id] += score;
    }
  }

  vector<ResultadoRI> resultados;
  resultados.reserve(acumulador.size());
  for (const auto &[idDoc, puntuacion] : acumulador)
    resultados.emplace_back(puntuacion, idDoc, 0);

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
  docsOrdenados = priority_queue<ResultadoRI>();

  // Build reverse map ONCE for all queries
  const auto idToDoc = buildIdToDoc(getIndiceDocs());
  const auto &infColeccion = getInformacionColeccionDocs();

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

    const auto &indicePreg = getIndicePregunta();

    double k_query = 0;
    for (const auto &[term, info] : indicePreg)
      k_query += info.getFt();

    unordered_map<long int, double> acumulador;
    acumulador.reserve(getIndiceDocs().size());

    for (const auto &[termino, infTermPreg] : indicePreg) {
      InformacionTermino infTerm;
      if (!Devuelve(termino, infTerm))
        continue;

      for (const auto &infTermDoc : infTerm.getLdocs()) {
        auto it = idToDoc.find(infTermDoc.doc_id);
        if (it == idToDoc.end())
          continue;

        double score = (formSimilitud == 0)
                           ? PuntuacionDFR(infTerm, infTermDoc, *it->second,
                                           infColeccion, infTermPreg, k_query)
                           : PuntuacionBM25(infTerm, infTermDoc, *it->second,
                                            infColeccion);

        acumulador[infTermDoc.doc_id] += score;
      }
    }

    vector<ResultadoRI> resultadosPreg;
    resultadosPreg.reserve(acumulador.size());
    for (const auto &[idDoc, puntuacion] : acumulador)
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
// Imprimir Resultados
// ---------------------------------------------------------------------------

// Helper: build id→filename map (strips directory and extension)
static unordered_map<long int, string>
buildIdToNombre(const unordered_map<string, InfDoc> &indiceDocs) {
  unordered_map<long int, string> m;
  m.reserve(indiceDocs.size());
  for (const auto &[nombre, infD] : indiceDocs) {
    string n = nombre;
    size_t lastSlash = n.find_last_of("/\\");
    if (lastSlash != string::npos)
      n = n.substr(lastSlash + 1);
    size_t lastDot = n.find_last_of('.');
    if (lastDot != string::npos)
      n = n.substr(0, lastDot);
    m[infD.getidDoc()] = std::move(n);
  }
  return m;
}

void Buscador::ImprimirResultadoBusqueda(const int &numDocumentos) const {
  const string formula = (formSimilitud == 0) ? "DFR" : "BM25";

  const auto idToNombre = buildIdToNombre(getIndiceDocs());

  string pregIndex;
  DevuelvePregunta(pregIndex);

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

    const string &nomDoc = [&]() -> const string & {
      auto it = idToNombre.find(res.IdDoc());
      static const string empty;
      return it != idToNombre.end() ? it->second : empty;
    }();

    const string &etiqPreg = (res.getNumPregunta() == 0)
                                 ? pregIndex
                                 : (const string &)"ConjuntoDePreguntas";

    cout << fixed << setprecision(6) << res.getNumPregunta() << " " << formula
         << " " << nomDoc << " " << posicion << " " << res.VSimilitud() << " "
         << etiqPreg << "\n";

    ++posicion;
  }
}

bool Buscador::ImprimirResultadoBusqueda(const int &numDocumentos,
                                         const string &nombreFichero) const {
  ofstream fs(nombreFichero);
  if (!fs.is_open())
    return false;

  const string formula = (formSimilitud == 0) ? "DFR" : "BM25";

  const auto idToNombre = buildIdToNombre(getIndiceDocs());

  string pregIndex;
  DevuelvePregunta(pregIndex);

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

    auto it = idToNombre.find(res.IdDoc());
    const string &nomDoc =
        (it != idToNombre.end()) ? it->second : (const string &)"";

    const string &etiqPreg = (res.getNumPregunta() == 0)
                                 ? pregIndex
                                 : (const string &)"ConjuntoDePreguntas";

    fs << res.getNumPregunta() << " " << formula << " " << nomDoc << " "
       << posicion << " " << res.VSimilitud() << " " << etiqPreg << "\n";

    ++posicion;
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
