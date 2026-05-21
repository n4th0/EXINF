#include "../include/indexadorHash.h"
#include "../include/stemmer.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
using namespace std;

string IndexadorHash::steam(const string &s) const {
  string final;
  stemmerPorter stemmer = stemmerPorter();
  stemmer.stemmer(s, tipoStemmer, final);
  return final;
}

// Normaliza una palabra igual que al indexar: tokeniza + stemmer
string IndexadorHash::normalizar(const string &word) const {
  list<string> tmp;
  tok.Tokenizar(word, tmp);
  if (tmp.empty())
    return "";
  return steam(tmp.front());
}

IndexadorHash::IndexadorHash() {}

IndexadorHash::IndexadorHash(const string &fichStopWords,
                             const string &delimitadores,
                             const bool &detectComp,
                             const bool &minuscSinAcentos,
                             const string &dirIndice, const int &tStemmer,
                             const bool &almPosTerm)
    : ficheroStopWords(fichStopWords),
      tok(delimitadores, detectComp, minuscSinAcentos), tipoStemmer(tStemmer),
      directorioIndice(dirIndice), almacenarPosTerm(almPosTerm) {

  ifstream f(fichStopWords);
  string word;
  while (getline(f, word)) {
    if (!word.empty()) {
      if (minuscSinAcentos) {
        list<string> tmp;
        tok.Tokenizar(word, tmp);
        if (!tmp.empty()) {

          stopWordsOriginales.insert(tmp.front()); // original tokenizada
          stopWords.insert(steam(tmp.front()));    // con stem para filtrar
        }
        // stopWords.insert(steam(tmp.front()));
      } else {
        stopWordsOriginales.insert(word); // original tokenizada
        stopWords.insert(steam(word));    // con stem para filtrar
      }
    }
  }
}

IndexadorHash::IndexadorHash(const string &directorioIndexacion) {
  if (!RecuperarIndexacion(directorioIndexacion))
    throw runtime_error("No se pudo recuperar la indexacion de: " +
                        directorioIndexacion);
}

IndexadorHash::IndexadorHash(const IndexadorHash &other)
    : indice(other.indice), indiceDocs(other.indiceDocs),
      informacionColeccionDocs(other.informacionColeccionDocs),
      pregunta(other.pregunta), indicePregunta(other.indicePregunta),
      infPregunta(other.infPregunta), stopWords(other.stopWords),
      ficheroStopWords(other.ficheroStopWords), tok(other.tok),
      directorioIndice(other.directorioIndice), tipoStemmer(other.tipoStemmer),
      almacenarPosTerm(other.almacenarPosTerm) {}

IndexadorHash &IndexadorHash::operator=(const IndexadorHash &other) {
  if (this != &other) {
    indice = other.indice;
    indiceDocs = other.indiceDocs;
    informacionColeccionDocs = other.informacionColeccionDocs;
    pregunta = other.pregunta;
    indicePregunta = other.indicePregunta;
    infPregunta = other.infPregunta;
    stopWords = other.stopWords;
    ficheroStopWords = other.ficheroStopWords;
    tok = other.tok;
    directorioIndice = other.directorioIndice;
    tipoStemmer = other.tipoStemmer;
    almacenarPosTerm = other.almacenarPosTerm;
  }
  return *this;
}

IndexadorHash::~IndexadorHash() {}

bool IndexadorHash::IndexarFichero(const string &fichero) {
  struct stat st;
  if (stat(fichero.c_str(), &st) == -1)
    return false;

  tm *modTm = localtime(&st.st_mtime);
  Fecha fechaDisco(modTm->tm_mday, modTm->tm_mon + 1, modTm->tm_year + 1900);

  int id_doc;
  auto it = indiceDocs.find(fichero);

  if (it != indiceDocs.end()) {
    if (fechaDisco <= it->second.getFechaModificacion())
      return true;
    id_doc = it->second.getidDoc();
    BorraDoc(fichero);
    it = indiceDocs.emplace(fichero, InfDoc()).first;
    it->second.setIdDoc(id_doc);
  } else {
    auto [ins, _] = indiceDocs.emplace(fichero, InfDoc());
    it = ins;
    id_doc = indiceDocs.size();
    it->second.setIdDoc(id_doc);
  }

  InfDoc &doc = it->second;
  doc.setTamBytes(st.st_size);
  doc.setFechaModificacion(fechaDisco);

  tok.Tokenizar(fichero);

  ifstream f(fichero + ".tk");
  if (!f.is_open())
    return false;

  indice.reserve(indice.size() + 1024);

  string line;
  int posGlobal = 0;
  int numTerminos_este_doc = 0;

  while (getline(f, line)) {
    if (line.empty()) {
      ++posGlobal;
      continue;
    }

    const string term = steam(line);
    doc.incNumPal();

    // Skip blank/whitespace-only terms after stemming
    if (term.find_first_not_of(" \t\r\n") == string::npos) {
      ++posGlobal;
      continue;
    }

    if (stopWords.count(term)) {
      ++posGlobal;
      continue;
    }

    InformacionTermino &infTerm = indice[term];
    infTerm.incFtc();

    InfTermDoc *itd = infTerm.find(id_doc);
    if (!itd) {
      InfTermDoc newDoc;
      newDoc.doc_id = id_doc;
      newDoc.incFt();
      if (almacenarPosTerm)
        newDoc.incPosTerm(posGlobal);
      infTerm.addL_docs(id_doc, std::move(newDoc));
      ++numTerminos_este_doc;
    } else {
      itd->incFt();
      if (almacenarPosTerm)
        itd->incPosTerm(posGlobal);
    }

    doc.incNumPalSinParada();
    ++posGlobal;
  }

  doc.setNumPalDiferentes(numTerminos_este_doc);

  auto &col = informacionColeccionDocs;
  col.setNumDocs(indiceDocs.size());
  col.setNumTotalPal(col.getNumTotalPal() + doc.getNumPal());
  col.setNumTotalPalSinParada(col.getNumTotalPalSinParada() +
                              doc.getNumPalSinParada());
  col.setNumTotalPalDiferentes(indice.size());
  col.setTamBytes(col.getTamBytes() + doc.getTamBytes());

  return true;
}

bool IndexadorHash::Indexar(const string &ficheroDocumentos) {
  ifstream f(ficheroDocumentos);
  if (!f.is_open())
    return false;

  string line;
  while (getline(f, line))
    if (!line.empty() && !IndexarFichero(line))
      return false;

  return true;
}

bool IndexadorHash::IndexarDirectorio(const string &dirAIndexar) {
  struct stat dir;
  int err = stat(dirAIndexar.c_str(), &dir);
  if (err == -1 || !S_ISDIR(dir.st_mode))
    return false;

  string cmd = "find -L \"" + dirAIndexar + "\" -type f | sort > .lista_fich";
  int ret = system(cmd.c_str());
  if (ret != 0)
    return false;
  return Indexar(".lista_fich");
}

// bool IndexadorHash::GuardarIndexacion() const {
//   const string dir = directorioIndice.empty() ? "." : directorioIndice;
//   mkdir(dir.c_str(), 0755);
//
//   ofstream fConfig(dir + "/config.idx");
//   if (!fConfig.is_open())
//     return false;
//   fConfig << ficheroStopWords << '\n'
//           << tok.DelimitadoresPalabra() << '\n'
//           << tok.CasosEspeciales() << '\n'
//           << tok.PasarAminuscSinAcentos() << '\n'
//           << tipoStemmer << '\n'
//           << almacenarPosTerm << '\n'
//           << directorioIndice << '\n'
//           << pregunta << '\n';
//
//   ofstream fStop(dir + "/stopwords.idx");
//   if (!fStop.is_open())
//     return false;
//   for (const auto &w : stopWords)
//     fStop << w << '\n';
//
//   ofstream fCol(dir + "/coleccion.idx");
//   if (!fCol.is_open())
//     return false;
//   fCol << informacionColeccionDocs.getNumDocs() << '\n'
//        << informacionColeccionDocs.getNumTotalPal() << '\n'
//        << informacionColeccionDocs.getNumTotalPalSinParada() << '\n'
//        << informacionColeccionDocs.getNumTotalPalDiferentes() << '\n'
//        << informacionColeccionDocs.getTamBytes() << '\n';
//
//   ofstream fDocs(dir + "/docs.idx");
//   if (!fDocs.is_open())
//     return false;
//   fDocs.rdbuf()->pubsetbuf(nullptr, 1 << 16);
//   for (const auto &par : indiceDocs) {
//     const InfDoc &d = par.second;
//     fDocs << par.first << '\n'
//           << d.getidDoc() << '\n'
//           << d.getNumPal() << '\n'
//           << d.getNumPalSinParada() << '\n'
//           << d.getNumPalDiferentes() << '\n'
//           << d.getTamBytes() << '\n'
//           << d.getFechaModificacion().getDay() << '\n'
//           << d.getFechaModificacion().getMonth() << '\n'
//           << d.getFechaModificacion().getYear() << '\n';
//   }
//
//   ofstream fIdx(dir + "/indice.idx");
//   if (!fIdx.is_open())
//     return false;
//   fIdx.rdbuf()->pubsetbuf(nullptr, 1 << 16);
//
//   for (const auto &par : indice) {
//     const InformacionTermino &inf = par.second;
//     const vector<InfTermDoc> &docs = inf.getLdocs();
//     if (docs.empty())
//       continue; // ← add this guard
//     fIdx << "TERM " << par.first << ' ' << inf.getFtc() << ' ' << docs.size()
//          << '\n';
//     for (const InfTermDoc &itd : docs) {
//       const vector<int> &pos = itd.getPosTerm();
//       fIdx << "DOC " << itd.doc_id << ' ' << itd.getFt() << ' ' <<
//       pos.size(); for (int p : pos)
//         fIdx << ' ' << p;
//       fIdx << '\n';
//     }
//   }
//
//   // for (const auto &par : indice) {
//   //   const InformacionTermino &inf = par.second;
//   //   const vector<InfTermDoc> &docs = inf.getLdocs();
//   //   fIdx << "TERM " << par.first << ' ' << inf.getFtc() << ' ' <<
//   docs.size()
//   //        << '\n';
//   // }
//
//   ofstream fPreg(dir + "/pregunta.idx");
//   if (!fPreg.is_open())
//     return false;
//   fPreg << infPregunta.getNumTotalPal() << '\n'
//         << infPregunta.getNumTotalPalSinParada() << '\n'
//         << infPregunta.getNumTotalPalDiferentes() << '\n';
//   for (const auto &par : indicePregunta) {
//     const InformacionTerminoPregunta &itp = par.second;
//     const vector<int> &pos = itp.getPosTerm();
//     fPreg << par.first << ' ' << itp.getFt() << ' ' << pos.size();
//     for (int p : pos)
//       fPreg << ' ' << p;
//     fPreg << '\n';
//   }
//
//   return true;
// }

bool IndexadorHash::GuardarIndexacion() const {
  const string dir = directorioIndice.empty() ? "." : directorioIndice;
  mkdir(dir.c_str(), 0755);

  ofstream fConfig(dir + "/config.idx");
  if (!fConfig.is_open())
    return false;
  fConfig << ficheroStopWords << '\n'
          << tok.DelimitadoresPalabra() << '\n'
          << tok.CasosEspeciales() << '\n'
          << tok.PasarAminuscSinAcentos() << '\n'
          << tipoStemmer << '\n'
          << almacenarPosTerm << '\n'
          << directorioIndice << '\n'
          << pregunta << '\n';

  ofstream fStop(dir + "/stopwords.idx");
  if (!fStop.is_open())
    return false;
  for (const auto &w : stopWords)
    fStop << w << '\n';

  ofstream fCol(dir + "/coleccion.idx");
  if (!fCol.is_open())
    return false;
  fCol << informacionColeccionDocs.getNumDocs() << '\n'
       << informacionColeccionDocs.getNumTotalPal() << '\n'
       << informacionColeccionDocs.getNumTotalPalSinParada() << '\n'
       << informacionColeccionDocs.getNumTotalPalDiferentes() << '\n'
       << informacionColeccionDocs.getTamBytes() << '\n';

  ofstream fDocs(dir + "/docs.idx");
  if (!fDocs.is_open())
    return false;
  fDocs.rdbuf()->pubsetbuf(nullptr, 1 << 16);
  for (const auto &par : indiceDocs) {
    const InfDoc &d = par.second;
    fDocs << par.first << '\n'
          << d.getidDoc() << '\n'
          << d.getNumPal() << '\n'
          << d.getNumPalSinParada() << '\n'
          << d.getNumPalDiferentes() << '\n'
          << d.getTamBytes() << '\n'
          << d.getFechaModificacion().getDay() << '\n'
          << d.getFechaModificacion().getMonth() << '\n'
          << d.getFechaModificacion().getYear() << '\n';
  }

  ofstream fIdx(dir + "/indice.idx");
  if (!fIdx.is_open())
    return false;
  fIdx.rdbuf()->pubsetbuf(nullptr, 1 << 16);
  for (const auto &par : indice) {
    const InformacionTermino &inf = par.second;
    const vector<InfTermDoc> &docs = inf.getLdocs();
    if (docs.empty())
      continue;
    // Term on its own line to handle multi-word terms with spaces

    if (par.first.find_first_not_of(" \t\r\n") == string::npos)
      continue;
    fIdx << "TERM\n"
         << par.first << '\n'
         << inf.getFtc() << ' ' << docs.size() << '\n';
    for (const InfTermDoc &itd : docs) {
      const vector<int> &pos = itd.getPosTerm();
      fIdx << "DOC " << itd.doc_id << ' ' << itd.getFt() << ' ' << pos.size();
      for (int p : pos)
        fIdx << ' ' << p;
      fIdx << '\n';
    }
  }

  ofstream fPreg(dir + "/pregunta.idx");
  if (!fPreg.is_open())
    return false;
  fPreg << infPregunta.getNumTotalPal() << '\n'
        << infPregunta.getNumTotalPalSinParada() << '\n'
        << infPregunta.getNumTotalPalDiferentes() << '\n';
  for (const auto &par : indicePregunta) {
    const InformacionTerminoPregunta &itp = par.second;
    const vector<int> &pos = itp.getPosTerm();
    fPreg << par.first << ' ' << itp.getFt() << ' ' << pos.size();
    for (int p : pos)
      fPreg << ' ' << p;
    fPreg << '\n';
  }

  return true;
}

bool IndexadorHash::RecuperarIndexacion(const string &directorioIndexacion) {
  indice.clear();
  indiceDocs.clear();
  indicePregunta.clear();
  stopWords.clear();
  pregunta = "";
  infPregunta = InformacionPregunta();
  informacionColeccionDocs = InfColeccionDocs();

  const string dir = directorioIndexacion.empty() ? "." : directorioIndexacion;

  // ── config ──────────────────────────────────────────────────────────────
  {
    ifstream f(dir + "/config.idx");
    if (!f.is_open()) {
      cerr << "ERROR: no se puede abrir config.idx\n";
      return false;
    }

    string detectCompStr, minuscStr, tipoStemmerStr, almacenarPosTermStr;
    string delims;
    getline(f, ficheroStopWords);
    getline(f, delims);
    getline(f, detectCompStr);
    getline(f, minuscStr);
    getline(f, tipoStemmerStr);
    getline(f, almacenarPosTermStr);
    getline(f, directorioIndice);
    getline(f, pregunta);

    bool detectComp = (detectCompStr == "1");
    bool minusc = (minuscStr == "1");
    tipoStemmer = stoi(tipoStemmerStr);
    almacenarPosTerm = (almacenarPosTermStr == "1");
    tok = Tokenizador(delims, detectComp, minusc);
  }

  // ── stopwords ───────────────────────────────────────────────────────────
  {
    ifstream f(dir + "/stopwords.idx");
    if (!f.is_open()) {
      cerr << "ERROR: no se puede abrir stopwords.idx\n";
      return false;
    }
    string w;
    while (getline(f, w)) {
      if (!w.empty()) {
        stopWordsOriginales.insert(w);
        stopWords.insert(steam(w));
      }
    }
  }

  // ── colección ────────────────────────────────────────────────────────────
  {
    ifstream f(dir + "/coleccion.idx");
    if (!f.is_open()) {
      cerr << "ERROR: no se puede abrir coleccion.idx\n";
      return false;
    }
    int nd, ntp, ntps, ntpd, tb;
    f >> nd >> ntp >> ntps >> ntpd >> tb;
    informacionColeccionDocs.setNumDocs(nd);
    informacionColeccionDocs.setNumTotalPal(ntp);
    informacionColeccionDocs.setNumTotalPalSinParada(ntps);
    informacionColeccionDocs.setNumTotalPalDiferentes(ntpd);
    informacionColeccionDocs.setTamBytes(tb);
  }

  // ── documentos ──────────────────────────────────────────────────────────
  {
    ifstream f(dir + "/docs.idx");
    if (!f.is_open()) {
      cerr << "ERROR: no se puede abrir docs.idx\n";
      return false;
    }
    indiceDocs.reserve(informacionColeccionDocs.getNumDocs());
    string nomDoc;
    while (getline(f, nomDoc)) {
      if (nomDoc.empty())
        continue;
      int id, np, nps, npd, tb, day, month, year;
      f >> id >> np >> nps >> npd >> tb >> day >> month >> year;
      f.ignore();
      InfDoc d;
      d.setIdDoc(id);
      d.setNumPal(np);
      d.setNumPalSinParada(nps);
      d.setNumPalDiferentes(npd);
      d.setTamBytes(tb);
      d.setFechaModificacion(Fecha(day, month, year));
      indiceDocs.emplace(std::move(nomDoc), std::move(d));
    }
  }

  // ── índice de términos ───────────────────────────────────────────────────
  {
    ifstream f(dir + "/indice.idx");
    if (!f.is_open()) {
      cerr << "ERROR: no se puede abrir indice.idx\n";
      return false;
    }
    indice.reserve(informacionColeccionDocs.getNumTotalPalDiferentes());

    string tag;
    while (f >> tag) {
      if (tag != "TERM") {
        cerr << "ERROR indice.idx: esperaba TERM, encontre '" << tag << "'\n";
        return false;
      }

      string term;
      getline(f >> std::ws, term); // reads "UNDERSTANDING WITH GERMANY "

      int ftc, numDocs;
      f >> ftc >> numDocs;
      f.ignore(numeric_limits<streamsize>::max(),
               '\n'); // ← consume rest of "1 1\n"

      InformacionTermino inf;
      inf.setFtc(ftc);

      for (int i = 0; i < numDocs; ++i) {
        f >> tag;
        if (tag != "DOC") {
          cerr << "ERROR indice.idx: esperaba DOC, encontre '" << tag
               << "' en termino '" << term << "'\n";
          return false;
        }

        int docId, ft_val, nPos;
        f >> docId >> ft_val >> nPos;

        InfTermDoc itd;
        itd.doc_id = docId;
        itd.setFt(ft_val);
        vector<int> pos;
        pos.reserve(nPos);
        for (int j = 0; j < nPos; ++j) {
          int p;
          f >> p;
          pos.push_back(p);
        }
        itd.setPosTerm(std::move(pos));
        inf.addL_docs(docId, itd);
      }
      indice.emplace(std::move(term), std::move(inf));
    }
  }

  // ── pregunta ─────────────────────────────────────────────────────────────
  {
    ifstream f(dir + "/pregunta.idx");
    if (!f.is_open()) {
      cerr << "ERROR: no se puede abrir pregunta.idx\n";
      return false;
    }
    int ntp, ntps, ntpd;
    f >> ntp >> ntps >> ntpd;
    f.ignore();
    infPregunta.setNumTotalPal(ntp);
    infPregunta.setNumTotalPalSinParada(ntps);
    infPregunta.setNumTotalPalDiferentes(ntpd);

    string termP;
    int ft, nPos;
    while (f >> termP >> ft >> nPos) {
      InformacionTerminoPregunta itp;
      itp.setFt(ft);
      for (int i = 0; i < nPos; ++i) {
        int p;
        f >> p;
        itp.addPosTerm(p);
      }
      indicePregunta.emplace(std::move(termP), std::move(itp));
    }
  }

  return true;
}
// bool IndexadorHash::RecuperarIndexacion(const string &directorioIndexacion) {
//   indice.clear();
//   indiceDocs.clear();
//   indicePregunta.clear();
//   stopWords.clear();
//   pregunta = "";
//   infPregunta = InformacionPregunta();
//   informacionColeccionDocs = InfColeccionDocs();
//
//   const string dir = directorioIndexacion.empty() ? "." :
//   directorioIndexacion;
//
//   // ── config ──────────────────────────────────────────────────────────────
//   {
//     ifstream f(dir + "/config.idx");
//     if (!f.is_open()) {
//       cerr << "ERROR: no se puede abrir config.idx\n";
//       return false;
//     }
//
//     string detectCompStr, minuscStr, tipoStemmerStr, almacenarPosTermStr;
//     string delims;
//     getline(f, ficheroStopWords);
//     getline(f, delims);
//     getline(f, detectCompStr);
//     getline(f, minuscStr);
//     getline(f, tipoStemmerStr);
//     getline(f, almacenarPosTermStr);
//     getline(f, directorioIndice);
//     getline(f, pregunta);
//
//     bool detectComp = (detectCompStr == "1");
//     bool minusc = (minuscStr == "1");
//     tipoStemmer = stoi(tipoStemmerStr);
//     almacenarPosTerm = (almacenarPosTermStr == "1");
//     tok = Tokenizador(delims, detectComp, minusc);
//   }
//
//   // ── stopwords ───────────────────────────────────────────────────────────
//   // Las stopwords se guardan ya normalizadas (tokenizadas + stemmer
//   aplicado)
//   {
//     ifstream f(dir + "/stopwords.idx");
//     if (!f.is_open()) {
//       cerr << "ERROR: no se puede abrir stopwords.idx\n";
//       return false;
//     }
//     string w;
//     while (getline(f, w)) {
//       if (!w.empty()) {
//         stopWordsOriginales.insert(w);
//         stopWords.insert(steam(w));
//       }
//     }
//   }
//
//   // ── colección
//   ────────────────────────────────────────────────────────────
//   {
//     ifstream f(dir + "/coleccion.idx");
//     if (!f.is_open()) {
//       cerr << "ERROR: no se puede abrir coleccion.idx\n";
//       return false;
//     }
//     int nd, ntp, ntps, ntpd, tb;
//     f >> nd >> ntp >> ntps >> ntpd >> tb;
//     informacionColeccionDocs.setNumDocs(nd);
//     informacionColeccionDocs.setNumTotalPal(ntp);
//     informacionColeccionDocs.setNumTotalPalSinParada(ntps);
//     informacionColeccionDocs.setNumTotalPalDiferentes(ntpd);
//     informacionColeccionDocs.setTamBytes(tb);
//   }
//
//   // ── documentos ──────────────────────────────────────────────────────────
//   {
//     ifstream f(dir + "/docs.idx");
//     if (!f.is_open()) {
//       cerr << "ERROR: no se puede abrir docs.idx\n";
//       return false;
//     }
//     indiceDocs.reserve(informacionColeccionDocs.getNumDocs());
//     string nomDoc;
//     while (getline(f, nomDoc)) {
//       if (nomDoc.empty())
//         continue;
//       int id, np, nps, npd, tb, day, month, year;
//       f >> id >> np >> nps >> npd >> tb >> day >> month >> year;
//       f.ignore();
//       InfDoc d;
//       d.setIdDoc(id);
//       d.setNumPal(np);
//       d.setNumPalSinParada(nps);
//       d.setNumPalDiferentes(npd);
//       d.setTamBytes(tb);
//       d.setFechaModificacion(Fecha(day, month, year));
//       indiceDocs.emplace(std::move(nomDoc), std::move(d));
//     }
//   }
//
//   // ── índice de términos
//   ───────────────────────────────────────────────────
//   {
//     ifstream f(dir + "/indice.idx");
//     if (!f.is_open()) {
//       cerr << "ERROR: no se puede abrir indice.idx\n";
//       return false;
//     }
//     indice.reserve(informacionColeccionDocs.getNumTotalPalDiferentes());
//
//     string tag, term;
//     while (f >> tag) {
//       if (tag != "TERM") {
//         cerr << "ERROR indice.idx: esperaba TERM, encontre '" << tag <<
//         "'\n"; return false;
//       }
//
//       int ftc, numDocs;
//       f >> term >> ftc >> numDocs;
//
//       InformacionTermino inf;
//       inf.setFtc(ftc);
//
//       for (int i = 0; i < numDocs; ++i) {
//         f >> tag;
//         if (tag != "DOC") {
//           cerr << "ERROR indice.idx: esperaba DOC, encontre '" << tag
//                << "' en termino '" << term << "'\n";
//           return false;
//         }
//
//         int docId, ft_val, nPos;
//         f >> docId >> ft_val >> nPos;
//
//         InfTermDoc itd;
//         itd.doc_id = docId;
//         itd.setFt(ft_val);
//         vector<int> pos;
//         pos.reserve(nPos);
//         for (int j = 0; j < nPos; ++j) {
//           int p;
//           f >> p;
//           pos.push_back(p);
//         }
//         itd.setPosTerm(std::move(pos));
//         inf.addL_docs(docId, itd);
//       }
//       indice.emplace(std::move(term), std::move(inf));
//     }
//   }
//
//   // ── pregunta
//   ─────────────────────────────────────────────────────────────
//   {
//     ifstream f(dir + "/pregunta.idx");
//     if (!f.is_open()) {
//       cerr << "ERROR: no se puede abrir pregunta.idx\n";
//       return false;
//     }
//     int ntp, ntps, ntpd;
//     f >> ntp >> ntps >> ntpd;
//     f.ignore();
//     infPregunta.setNumTotalPal(ntp);
//     infPregunta.setNumTotalPalSinParada(ntps);
//     infPregunta.setNumTotalPalDiferentes(ntpd);
//
//     string termP;
//     int ft, nPos;
//     while (f >> termP >> ft >> nPos) {
//       InformacionTerminoPregunta itp;
//       itp.setFt(ft);
//       for (int i = 0; i < nPos; ++i) {
//         int p;
//         f >> p;
//         itp.addPosTerm(p);
//       }
//       indicePregunta.emplace(std::move(termP), std::move(itp));
//     }
//   }
//
//   return true;
// }

bool IndexadorHash::IndexarPregunta(const string &preg) {
  list<string> tokens;
  tok.Tokenizar(preg, tokens);

  bool hayTerminos = false;
  for (const auto &t : tokens)
    if (!stopWords.count(steam(t))) {
      hayTerminos = true;
      break;
    }

  if (!hayTerminos)
    return false;

  indicePregunta.clear();
  infPregunta = InformacionPregunta();
  pregunta = preg;

  int count = 0;
  int numPal = 0, numPalSinParada = 0;
  for (const auto &token : tokens) {
    string term = steam(token);
    numPal++;
    if (stopWords.count(term)) {
      count++;
      continue;
    }
    numPalSinParada++;

    auto &itp = indicePregunta[term];
    itp.incFt();
    if (almacenarPosTerm)
      itp.addPosTerm(count);
    count++;
  }

  infPregunta.setNumTotalPal(numPal);
  infPregunta.setNumTotalPalSinParada(numPalSinParada);
  infPregunta.setNumTotalPalDiferentes(indicePregunta.size());
  return true;
}

bool IndexadorHash::DevuelvePregunta(string &preg) const {
  if (indicePregunta.empty())
    return false;
  preg = pregunta;
  return true;
}

bool IndexadorHash::DevuelvePregunta(const string &word,
                                     InformacionTerminoPregunta &inf) const {
  // FIX: normalizar igual que al indexar (tokenizar + stemmer)
  string term = normalizar(word);
  if (term.empty()) {
    inf = InformacionTerminoPregunta();
    return false;
  }

  if (indicePregunta.count(term)) {
    inf = indicePregunta.at(term);
    return true;
  }
  inf = InformacionTerminoPregunta();
  return false;
}

bool IndexadorHash::DevuelvePregunta(InformacionPregunta &inf) const {
  if (indicePregunta.empty()) {
    inf = InformacionPregunta();
    return false;
  }
  inf = infPregunta;
  return true;
}

bool IndexadorHash::Devuelve(const string &word,
                             InformacionTermino &inf) const {
  // FIX: normalizar igual que al indexar
  string term = normalizar(word);
  if (term.empty()) {
    inf = InformacionTermino();
    return false;
  }

  if (indice.count(term)) {
    inf = indice.at(term);
    return true;
  }
  inf = InformacionTermino();
  return false;
}

bool IndexadorHash::Devuelve(const string &word, const string &nomDoc,
                             InfTermDoc &infDoc) const {
  // FIX: normalizar igual que al indexar
  string term = normalizar(word);
  if (term.empty() || !indice.count(term) || !indiceDocs.count(nomDoc)) {
    infDoc = InfTermDoc();
    return false;
  }
  int doc = indiceDocs.at(nomDoc).getidDoc();
  auto ldocs = indice.at(term);

  infDoc = *ldocs.find(doc);

  if (!infDoc.empty()) {
    return true;
  }
  return false;
}

bool IndexadorHash::Existe(const string &word) const {
  // FIX: normalizar igual que al indexar
  string term = normalizar(word);
  if (term.empty())
    return false;
  return indice.count(term);
}

bool IndexadorHash::BorraDoc(const string &nomDoc) {
  auto docIt = indiceDocs.find(nomDoc);
  if (docIt == indiceDocs.end())
    return false;

  const int doc = docIt->second.getidDoc();

  for (auto it = indice.begin(); it != indice.end();) {
    if (it->second.delete_doc(doc)) {
      // eliminado
    }
    if (it->second.getFtc() == 0)
      it = indice.erase(it);
    else
      ++it;
  }

  informacionColeccionDocs.setNumDocs(informacionColeccionDocs.getNumDocs() -
                                      1);
  informacionColeccionDocs.setNumTotalPal(
      informacionColeccionDocs.getNumTotalPal() - docIt->second.getNumPal());
  informacionColeccionDocs.setNumTotalPalSinParada(
      informacionColeccionDocs.getNumTotalPalSinParada() -
      docIt->second.getNumPalSinParada());
  informacionColeccionDocs.setNumTotalPalDiferentes(indice.size());
  informacionColeccionDocs.setTamBytes(informacionColeccionDocs.getTamBytes() -
                                       docIt->second.getTamBytes());
  indiceDocs.erase(docIt);
  return true;
}

void IndexadorHash::VaciarIndiceDocs() {
  indice.clear();
  indiceDocs.clear();
  informacionColeccionDocs = InfColeccionDocs();
}

void IndexadorHash::VaciarIndicePreg() {
  pregunta = "";
  indicePregunta.clear();
  infPregunta = InformacionPregunta();
}

int IndexadorHash::NumPalIndexadas() const { return indice.size(); }

string IndexadorHash::DevolverFichPalParada() const { return ficheroStopWords; }

void IndexadorHash::ListarPalParada() const {
  for (const auto &w : stopWordsOriginales)
    cout << w << '\n';
}

int IndexadorHash::NumPalParada() const { return stopWordsOriginales.size(); }

string IndexadorHash::DevolverDelimitadores() const {
  return tok.DelimitadoresPalabra();
}

bool IndexadorHash::DevolverCasosEspeciales() const {
  return tok.CasosEspeciales();
}

bool IndexadorHash::DevolverPasarAminuscSinAcentos() const {
  return tok.PasarAminuscSinAcentos();
}

bool IndexadorHash::DevolverAlmacenarPosTerm() const {
  return almacenarPosTerm;
}

string IndexadorHash::DevolverDirIndice() const { return directorioIndice; }

int IndexadorHash::DevolverTipoStemming() const { return tipoStemmer; }

void IndexadorHash::ListarInfColeccDocs() const {
  cout << informacionColeccionDocs << '\n';
}

void IndexadorHash::ListarTerminos() const {
  for (const auto &par : indice)
    cout << par.first << '\t' << par.second << '\n';
}

bool IndexadorHash::ListarTerminos(const string &nomDoc) const {
  auto it = indiceDocs.find(nomDoc);
  if (it == indiceDocs.end())
    return false;
  int doc = it->second.getidDoc();
  for (const auto &par : indice) {
    if (!par.second.find(doc).empty())
      cout << par.first << '\t' << par.second << '\n';
  }
  return true;
}

void IndexadorHash::ListarDocs() const {
  for (const auto &par : indiceDocs)
    cout << par.first << '\t' << par.second << '\n';
}

bool IndexadorHash::ListarDocs(const string &nomDoc) const {
  auto it = indiceDocs.find(nomDoc);
  if (it == indiceDocs.end())
    return false;
  cout << nomDoc << '\t' << it->second << '\n';
  return true;
}
