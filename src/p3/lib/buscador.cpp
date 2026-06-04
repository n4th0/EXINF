#include "../include/buscador.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>

using namespace std;

ResultadoRI::ResultadoRI() : vSimilitud(0.0), idDoc(0), numPregunta(0) {}

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
     << "\n";
  return os;
}

void ResultadoRI::clear() {
  vSimilitud = 0.0;
  idDoc = 0;
  numPregunta = 0;
}

// ---------------------------------------------------------------------------
// Buscador: Constructores y Destructor
// ---------------------------------------------------------------------------
Buscador::Buscador(const string &directorioIndexacion, const int &f)
    : IndexadorHash(directorioIndexacion), cacheConstruida(false),
      formSimilitud(f), c(2.0), k1(1.2), b(0.75) {}

Buscador::Buscador(const Buscador &other)
    : IndexadorHash(other), docsOrdenadosLista(other.docsOrdenadosLista),
      cachedIdToNombre(other.cachedIdToNombre),
      cacheConstruida(other.cacheConstruida),
      formSimilitud(other.formSimilitud), c(other.c), k1(other.k1), b(other.b),
      docsOrdenados(other.docsOrdenados) {}

Buscador::~Buscador() = default;

Buscador &Buscador::operator=(const Buscador &other) {
  if (this != &other) {
    IndexadorHash::operator=(other);
    docsOrdenadosLista = other.docsOrdenadosLista;
    cachedIdToNombre = other.cachedIdToNombre;
    cacheConstruida = other.cacheConstruida;
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

double Buscador::PuntuacionDFR(double ftd, double ld, double ft, double nt,
                               double log_lambda1, double log_ratio,
                               double avg_ld, double ftq, double k) const {
  if (k <= 0.0 || ft <= 0.0 || nt <= 0.0)
    return 0.0;
  double tf_star = ftd * log2(1.0 + c * avg_ld / ld);
  double w_id =
      (log_lambda1 + tf_star * log_ratio) * (ft + 1.0) / (nt * (tf_star + 1.0));
  return (ftq / k) * w_id;
}
double Buscador::PuntuacionBM25(const InformacionTermino &infTerm,
                                const InfTermDoc &infTermDoc, double dl,
                                double idf, double avgdl) const {
  double tf = static_cast<double>(infTermDoc.getFt());
  double num = tf * (k1 + 1.0);
  double den = tf + k1 * (1.0 - b + b * (dl / avgdl));
  return idf * (num / den);
}

// double Buscador::PuntuacionBM25(const InformacionTermino &infTerm,
//                                 const InfTermDoc &infTermDoc,
//                                 const InfDoc &infDocumento,
//                                 const InfColeccionDocs &infColeccion) const {
//   double N = static_cast<double>(infColeccion.getNumDocs());
//   double nqi = static_cast<double>(infTerm.getLdocs().size());
//   double tf = static_cast<double>(infTermDoc.getFt());
//   double dl = static_cast<double>(infDocumento.getNumPalSinParada());
//   double avgdl =
//       (N > 0) ? static_cast<double>(infColeccion.getNumTotalPalSinParada()) /
//       N
//               : 1.0;
//   double idf = log2((N - nqi + 0.5) / (nqi + 0.5));
//   double num = tf * (k1 + 1.0);
//   double den = tf + k1 * (1.0 - b + b * (dl / avgdl));
//   return idf * (num / den);
// }
// double Buscador::PuntuacionDFR(const InformacionTermino &infTerm,
//                                const InfTermDoc &infTermDoc,
//                                const InfDoc &infDocumento,
//                                const InfColeccionDocs &infColeccion,
//                                const InformacionTerminoPregunta &infTermPreg,
//                                double k) const {
//   if (k <= 0)
//     return 0.0;
//   double N = static_cast<double>(infColeccion.getNumDocs());
//   if (N <= 0.0)
//     return 0.0;
//   double ft = static_cast<double>(infTerm.getFtc());
//   double lambda = ft / N;
//   if (lambda <= 0.0)
//     return 0.0;
//   double log_lambda1 = log2(1.0 + lambda);
//   double log_ratio = log_lambda1 - log2(lambda);
//   double ftd = static_cast<double>(infTermDoc.getFt());
//   double ld = static_cast<double>(infDocumento.getNumPalSinParada());
//   if (ld == 0.0)
//     ld = 1.0;
//   double avg_ld =
//       static_cast<double>(infColeccion.getNumTotalPalSinParada()) / N;
//   double tf_star = ftd * log2(1.0 + c * avg_ld / ld);
//   double nt = static_cast<double>(infTerm.getLdocs().size());
//   double w_id =
//       (log_lambda1 + tf_star * log_ratio) * (ft + 1.0) / (nt * (tf_star
//       + 1.0));
//   double ftq = static_cast<double>(infTermPreg.getFt());
//   return (ftq / k) * w_id;
// }

// ---------------------------------------------------------------------------
// Gestión de la Caché
// ---------------------------------------------------------------------------
void Buscador::precomputarNombresDocs() const {
  if (cacheConstruida)
    return;

  const auto &indiceDocs = getIndiceDocs();
  cachedIdToNombre.clear();
  cachedIdToNombre.reserve(indiceDocs.size());

  for (const auto &[nombreCompleto, infD] : indiceDocs) {
    string n = nombreCompleto;
    size_t lastSlash = n.find_last_of("/\\");
    if (lastSlash != string::npos)
      n = n.substr(lastSlash + 1);
    size_t lastDot = n.find_last_of('.');
    if (lastDot != string::npos)
      n = n.substr(0, lastDot);

    cachedIdToNombre[infD.getidDoc()] = std::move(n);
  }
  cacheConstruida = true;
}

static unordered_map<long int, const InfDoc *>
buildIdToDoc(const unordered_map<string, InfDoc> &indiceDocs) {
  unordered_map<long int, const InfDoc *> m;
  m.reserve(indiceDocs.size());
  for (const auto &[nombre, infD] : indiceDocs)
    m[infD.getidDoc()] = &infD;
  return m;
}

// ---------------------------------------------------------------------------
// Buscar
// ---------------------------------------------------------------------------
// bool Buscador::Buscar(const int &numDocumentos) {
//   string pregActual;
//   if (!DevuelvePregunta(pregActual))
//     return false;
//
//   docsOrdenadosLista.clear();
//
//   const auto idToDoc = buildIdToDoc(getIndiceDocs());
//   const auto &infColeccion = getInformacionColeccionDocs();
//   const auto &indicePreg = getIndicePregunta();
//
//   double k = 0;
//   for (const auto &[term, info] : indicePreg)
//     k += info.getFt();
//
//   unordered_map<long int, double> acumulador;
//   acumulador.reserve(2048);
//
//   for (const auto &[termino, infTermPreg] : indicePreg) {
//     InformacionTermino infTerm;
//     if (!Devuelve(termino, infTerm))
//       continue;
//
//     for (const auto &infTermDoc : infTerm.getLdocs()) {
//       auto it = idToDoc.find(infTermDoc.doc_id);
//       if (it == idToDoc.end())
//         continue;
//
//       double score =
//           (formSimilitud == 0)
//               ? PuntuacionDFR(infTerm, infTermDoc, *it->second, infColeccion,
//                               infTermPreg, k)
//               : PuntuacionBM25(infTerm, infTermDoc, *it->second,
//               infColeccion);
//       acumulador[infTermDoc.doc_id] += score;
//     }
//   }
//
//   docsOrdenadosLista.reserve(acumulador.size());
//   for (const auto &[idDoc, puntuacion] : acumulador) {
//     docsOrdenadosLista.emplace_back(puntuacion, idDoc, 0);
//   }
//
//   std::sort(docsOrdenadosLista.begin(), docsOrdenadosLista.end(),
//             [](const ResultadoRI &a, const ResultadoRI &b) { return b < a;
//             });
//
//   return true;
// }
bool Buscador::Buscar(const int &numDocumentos) {
  string pregActual;
  if (!DevuelvePregunta(pregActual))
    return false;

  docsOrdenadosLista.clear();

  const auto idToDoc = buildIdToDoc(getIndiceDocs());
  const auto &infColeccion = getInformacionColeccionDocs();
  const auto &indicePreg = getIndicePregunta();

  // Constantes de la colección
  double N = static_cast<double>(infColeccion.getNumDocs());
  double totalPal = static_cast<double>(infColeccion.getNumTotalPalSinParada());
  double avgdl = (N > 0) ? totalPal / N : 1.0;
  double avg_ld = avgdl; // mismo valor, nombres distintos según fórmula

  // Cálculo de k para DFR
  double k = 0.0;
  for (const auto &[term, info] : indicePreg)
    k += info.getFt();

  // Precalcular constantes por término
  struct TermStats {
    double idf;                            // para BM25
    double ft, nt, log_lambda1, log_ratio; // para DFR
  };
  unordered_map<string, TermStats> termStats;
  for (const auto &[termino, infTermPreg] : indicePreg) {
    InformacionTermino infTerm;
    if (!Devuelve(termino, infTerm))
      continue;
    double nqi = static_cast<double>(infTerm.getLdocs().size());
    double ft = static_cast<double>(infTerm.getFtc());
    double lambda = (N > 0) ? ft / N : 0.0;
    double log_lambda1 = (lambda > 0) ? log2(1.0 + lambda) : 0.0;
    double log_ratio = (lambda > 0) ? log_lambda1 - log2(lambda) : 0.0;
    termStats[termino] = {log2((N - nqi + 0.5) / (nqi + 0.5)), // idf BM25
                          ft, nqi, log_lambda1, log_ratio};
  }

  unordered_map<long int, double> acumulador;
  acumulador.reserve(2048);

  for (const auto &[termino, infTermPreg] : indicePreg) {
    auto statsIt = termStats.find(termino);
    if (statsIt == termStats.end())
      continue;
    const auto &stats = statsIt->second;

    InformacionTermino infTerm;
    if (!Devuelve(termino, infTerm))
      continue;

    for (const auto &infTermDoc : infTerm.getLdocs()) {
      auto it = idToDoc.find(infTermDoc.doc_id);
      if (it == idToDoc.end())
        continue;

      double dl = static_cast<double>(it->second->getNumPalSinParada());
      double score = 0.0;
      if (formSimilitud == 1) { // BM25
        score = PuntuacionBM25(infTerm, infTermDoc, dl, stats.idf, avgdl);
      } else { // DFR
        double ftd = static_cast<double>(infTermDoc.getFt());
        double ftq = static_cast<double>(infTermPreg.getFt());
        score = PuntuacionDFR(ftd, dl, stats.ft, stats.nt, stats.log_lambda1,
                              stats.log_ratio, avg_ld, ftq, k);
      }
      acumulador[infTermDoc.doc_id] += score;
    }
  }

  // Mantener solo los numDocumentos mejores
  docsOrdenadosLista.reserve(min((size_t)numDocumentos, acumulador.size()));
  if (numDocumentos < (int)acumulador.size()) {
    // Usamos un heap de mínimos para eficiencia
    using par = pair<long int, double>;
    auto cmp = [](const par &a, const par &b) { return a.second > b.second; };
    priority_queue<par, vector<par>, decltype(cmp)> pq(cmp);
    for (const auto &[idDoc, score] : acumulador) {
      pq.push({idDoc, score});
      if ((int)pq.size() > numDocumentos)
        pq.pop();
    }
    // Pasar al vector y ordenar descendente
    docsOrdenadosLista.reserve(pq.size());
    while (!pq.empty()) {
      auto top = pq.top();
      pq.pop();
      docsOrdenadosLista.emplace_back(top.second, top.first, 0);
    }
    reverse(docsOrdenadosLista.begin(), docsOrdenadosLista.end());
  } else {
    for (const auto &[idDoc, score] : acumulador)
      docsOrdenadosLista.emplace_back(score, idDoc, 0);
    sort(docsOrdenadosLista.begin(), docsOrdenadosLista.end(),
         [](const ResultadoRI &a, const ResultadoRI &b) { return b < a; });
  }

  return true;
}
bool Buscador::Buscar(const string &dirPreguntas, const int &numDocumentos,
                      const int &numPregInicio, const int &numPregFin) {
  docsOrdenadosLista.clear();
  const auto idToDoc = buildIdToDoc(getIndiceDocs());
  const auto &infColeccion = getInformacionColeccionDocs();

  // Constantes globales de la colección
  double N = static_cast<double>(infColeccion.getNumDocs());
  double totalPal = static_cast<double>(infColeccion.getNumTotalPalSinParada());
  double avgdl = (N > 0) ? totalPal / N : 1.0;
  double avg_ld = avgdl;

  unordered_map<long int, double> acumulador;
  acumulador.reserve(2048);

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

    // k para DFR
    double k_query = 0.0;
    for (const auto &[term, info] : indicePreg)
      k_query += info.getFt();

    // Precalcular constantes por término (aprovechando la misma struct)
    struct TermStats {
      double idf;
      double ft, nt, log_lambda1, log_ratio;
    };
    unordered_map<string, TermStats> termStats;
    for (const auto &[termino, infTermPreg] : indicePreg) {
      InformacionTermino infTerm;
      if (!Devuelve(termino, infTerm))
        continue;
      double nqi = static_cast<double>(infTerm.getLdocs().size());
      double ft = static_cast<double>(infTerm.getFtc());
      double lambda = (N > 0) ? ft / N : 0.0;
      double log_lambda1 = (lambda > 0) ? log2(1.0 + lambda) : 0.0;
      double log_ratio = (lambda > 0) ? log_lambda1 - log2(lambda) : 0.0;
      termStats[termino] = {log2((N - nqi + 0.5) / (nqi + 0.5)), ft, nqi,
                            log_lambda1, log_ratio};
    }

    acumulador.clear();
    for (const auto &[termino, infTermPreg] : indicePreg) {
      auto statsIt = termStats.find(termino);
      if (statsIt == termStats.end())
        continue;
      const auto &stats = statsIt->second;
      InformacionTermino infTerm;
      if (!Devuelve(termino, infTerm))
        continue;

      for (const auto &infTermDoc : infTerm.getLdocs()) {
        auto it = idToDoc.find(infTermDoc.doc_id);
        if (it == idToDoc.end())
          continue;
        double dl = static_cast<double>(it->second->getNumPalSinParada());
        double score =
            (formSimilitud == 1)
                ? PuntuacionBM25(infTerm, infTermDoc, dl, stats.idf, avgdl)
                : PuntuacionDFR(
                      static_cast<double>(infTermDoc.getFt()), dl, stats.ft,
                      stats.nt, stats.log_lambda1, stats.log_ratio, avg_ld,
                      static_cast<double>(infTermPreg.getFt()), k_query);
        acumulador[infTermDoc.doc_id] += score;
      }
    }

    // Ordenación parcial y volcado a docsOrdenadosLista (idéntico al original)
    vector<ResultadoRI> queryResultados;
    queryResultados.reserve(acumulador.size());
    for (const auto &[idDoc, score] : acumulador)
      queryResultados.emplace_back(score, idDoc, numPreg);

    size_t limite = min((size_t)numDocumentos, queryResultados.size());
    if (limite < queryResultados.size()) {
      partial_sort(
          queryResultados.begin(), queryResultados.begin() + limite,
          queryResultados.end(),
          [](const ResultadoRI &a, const ResultadoRI &b) { return b < a; });
      queryResultados.erase(queryResultados.begin() + limite,
                            queryResultados.end());
    } else {
      sort(queryResultados.begin(), queryResultados.end(),
           [](const ResultadoRI &a, const ResultadoRI &b) { return b < a; });
    }
    docsOrdenadosLista.insert(docsOrdenadosLista.end(), queryResultados.begin(),
                              queryResultados.end());
  }
  return true;
}

// bool Buscador::Buscar(const string &dirPreguntas, const int &numDocumentos,
//                       const int &numPregInicio, const int &numPregFin) {
//   docsOrdenadosLista.clear();
//
//   const auto idToDoc = buildIdToDoc(getIndiceDocs());
//   const auto &infColeccion = getInformacionColeccionDocs();
//
//   unordered_map<long int, double> acumulador;
//   acumulador.reserve(2048);
//
//   for (int numPreg = numPregInicio; numPreg <= numPregFin; ++numPreg) {
//     string fichPreg = dirPreguntas + "/" + to_string(numPreg) + ".txt";
//     ifstream fs(fichPreg);
//     if (!fs.is_open())
//       continue;
//
//     string contenidoPreg((istreambuf_iterator<char>(fs)),
//                          istreambuf_iterator<char>());
//     fs.close();
//
//     if (!IndexarPregunta(contenidoPreg))
//       continue;
//
//     const auto &indicePreg = getIndicePregunta();
//     double k_query = 0;
//     for (const auto &[term, info] : indicePreg)
//       k_query += info.getFt();
//
//     acumulador.clear();
//
//     for (const auto &[termino, infTermPreg] : indicePreg) {
//       InformacionTermino infTerm;
//       if (!Devuelve(termino, infTerm))
//         continue;
//
//       for (const auto &infTermDoc : infTerm.getLdocs()) {
//         auto it = idToDoc.find(infTermDoc.doc_id);
//         if (it == idToDoc.end())
//           continue;
//
//         double score = (formSimilitud == 0)
//                            ? PuntuacionDFR(infTerm, infTermDoc, *it->second,
//                                            infColeccion, infTermPreg,
//                                            k_query)
//                            : PuntuacionBM25(infTerm, infTermDoc, *it->second,
//                                             infColeccion);
//         acumulador[infTermDoc.doc_id] += score;
//       }
//     }
//
//     std::vector<ResultadoRI> queryResultados;
//     queryResultados.reserve(acumulador.size());
//     for (const auto &[idDoc, puntuacion] : acumulador) {
//       queryResultados.emplace_back(puntuacion, idDoc, numPreg);
//     }
//
//     size_t limite = min((size_t)numDocumentos, queryResultados.size());
//     if (limite < queryResultados.size()) {
//       std::partial_sort(
//           queryResultados.begin(), queryResultados.begin() + limite,
//           queryResultados.end(),
//           [](const ResultadoRI &a, const ResultadoRI &b) { return b < a; });
//       // OPTIMIZACIÓN SEGURA: Eliminamos elementos sobrantes usando erase en
//       // lugar de resize por defecto
//       queryResultados.erase(queryResultados.begin() + limite,
//                             queryResultados.end());
//     } else {
//       std::sort(
//           queryResultados.begin(), queryResultados.end(),
//           [](const ResultadoRI &a, const ResultadoRI &b) { return b < a; });
//     }
//
//     docsOrdenadosLista.insert(docsOrdenadosLista.end(),
//     queryResultados.begin(),
//                               queryResultados.end());
//   }
//
//   return true;
// }

// ---------------------------------------------------------------------------
// Imprimir Resultados
// ---------------------------------------------------------------------------
void Buscador::ImprimirResultadoBusqueda(const int &numDocumentos) const {
  precomputarNombresDocs();
  const string formula = (formSimilitud == 0) ? "DFR" : "BM25";

  string pregIndex;
  DevuelvePregunta(pregIndex);

  int pregActual = -1;
  int posicion = 0;

  ios_base::sync_with_stdio(false);
  cout.tie(NULL);

  char buffer[65536];
  cout.rdbuf()->pubsetbuf(buffer, sizeof(buffer));

  for (const auto &res : docsOrdenadosLista) {
    if (res.getNumPregunta() != pregActual) {
      pregActual = res.getNumPregunta();
      posicion = 0;
    }
    if (posicion >= numDocumentos)
      continue;

    auto it = cachedIdToNombre.find(res.IdDoc());
    const string &nomDoc = (it != cachedIdToNombre.end()) ? it->second : "";
    const string &etiqPreg =
        (res.getNumPregunta() == 0) ? pregIndex : "ConjuntoDePreguntas";

    printf("%d %s %s %d %.6f %s\n", res.getNumPregunta(), formula.c_str(),
           nomDoc.c_str(), posicion, res.VSimilitud(), etiqPreg.c_str());

    ++posicion;
  }
}

bool Buscador::ImprimirResultadoBusqueda(const int &numDocumentos,
                                         const string &nombreFichero) const {
  ofstream fs(nombreFichero, ios::out | ios::binary);
  if (!fs.is_open())
    return false;

  precomputarNombresDocs();
  const string formula = (formSimilitud == 0) ? "DFR" : "BM25";

  string pregIndex;
  DevuelvePregunta(pregIndex);

  std::vector<char> buffer(131072);
  fs.rdbuf()->pubsetbuf(buffer.data(), buffer.size());

  int pregActual = -1;
  int posicion = 0;

  char linea[512];
  for (const auto &res : docsOrdenadosLista) {
    if (res.getNumPregunta() != pregActual) {
      pregActual = res.getNumPregunta();
      posicion = 0;
    }
    if (posicion >= numDocumentos)
      continue;

    auto it = cachedIdToNombre.find(res.IdDoc());
    const string &nomDoc = (it != cachedIdToNombre.end()) ? it->second : "";
    const string &etiqPreg =
        (res.getNumPregunta() == 0) ? pregIndex : "ConjuntoDePreguntas";

    int len = sprintf(linea, "%d %s %s %d %.6f %s\n", res.getNumPregunta(),
                      formula.c_str(), nomDoc.c_str(), posicion,
                      res.VSimilitud(), etiqPreg.c_str());
    fs.write(linea, len);

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
