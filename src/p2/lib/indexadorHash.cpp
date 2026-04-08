#include "../include/indexadorHash.h"
#include "../include/stemmer.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
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
        if (!tmp.empty())
          stopWords.insert(tmp.front());
      } else {
        stopWords.insert(word);
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
  int id_doc;
  ifstream f(fichero);
  if (!f.is_open())
    return false;

  if (indiceDocs.count(fichero)) {
    struct stat st;
    if (stat(fichero.c_str(), &st) == -1)
      return true;
    tm *modTm = localtime(&st.st_mtime);
    Fecha fechaDisco(modTm->tm_mday, modTm->tm_mon + 1, modTm->tm_year + 1900);
    const Fecha &fechaIdx = indiceDocs.at(fichero).getFechaModificacion();
    if (fechaDisco <= fechaIdx)
      return true;
    int oldId = indiceDocs.at(fichero).getidDoc();
    BorraDoc(fichero);
    indiceDocs[fichero] = InfDoc();
    indiceDocs[fichero].setIdDoc(oldId);
    id_doc = oldId;
  } else {
    indiceDocs[fichero] = InfDoc();
    indiceDocs[fichero].setIdDoc(indiceDocs.size());
    id_doc = indiceDocs[fichero].getidDoc();
  }

  struct stat st;
  if (stat(fichero.c_str(), &st) == 0) {
    indiceDocs[fichero].setTamBytes(st.st_size);
    tm *modTm = localtime(&st.st_mtime);
    indiceDocs[fichero].setFechaModificacion(
        Fecha(modTm->tm_mday, modTm->tm_mon + 1, modTm->tm_year + 1900));
  }

  tok.Tokenizar(fichero);
  ifstream f2(fichero + ".tk");
  if (!f2.is_open())
    return false;

  // Pre-reservamos para reducir rehashing
  indice.reserve(indice.size() + 1024);

  string line;
  int posGlobal = 0;
  unordered_set<string> terminos_este_doc;

  while (getline(f2, line)) {
    if (line.empty()) {
      posGlobal++;
      continue;
    }

    const string term = steam(line);
    indiceDocs[fichero].incNumPal();

    if (stopWords.count(term)) {
      posGlobal++;
      continue;
    }

    // ── OPTIMIZACIÓN CLAVE ──────────────────────────────────────────────────
    // Antes: getL_docs() devolvía copia del mapa → setL_docs() lo volvía a
    // copiar entero. Ahora: acceso directo por referencia mutable, sin copias.
    // ────────────────────────────────────────────────────────────────────────
    InformacionTermino &infTerm = indice[term];
    infTerm.incFtc(); // dudo incluso de que haga falta
    terminos_este_doc.insert(term);

    InfTermDoc *itd = infTerm.find(id_doc);
    if (itd == nullptr) {
      InfTermDoc newDoc;
      newDoc.doc_id = id_doc;
      if (almacenarPosTerm) {
        newDoc.incPosTerm(posGlobal);
      }
      infTerm.addL_docs(id_doc, std::move(newDoc));
    } else {
      // Document found - update existing entry
      // itd->incFt();
      if (almacenarPosTerm) {
        itd->incPosTerm(posGlobal);
        // cout << term << " llego: " << *itd << endl;
      }
    }

    indiceDocs[fichero].incNumPalSinParada();
    posGlobal++;
  }

  indiceDocs[fichero].setNumPalDiferentes(terminos_este_doc.size());

  // Actualizar estadísticas de colección
  const InfDoc &doc = indiceDocs[fichero];
  informacionColeccionDocs.setNumDocs(indiceDocs.size());
  informacionColeccionDocs.setNumTotalPal(
      informacionColeccionDocs.getNumTotalPal() + doc.getNumPal());
  informacionColeccionDocs.setNumTotalPalSinParada(
      informacionColeccionDocs.getNumTotalPalSinParada() +
      doc.getNumPalSinParada());
  informacionColeccionDocs.setNumTotalPalDiferentes(indice.size());
  informacionColeccionDocs.setTamBytes(informacionColeccionDocs.getTamBytes() +
                                       doc.getTamBytes());
  return true;
}

bool IndexadorHash::Indexar(const string &ficheroDocumentos) {
  ifstream f(ficheroDocumentos);
  if (!f.is_open())
    return false;

  bool result = true;
  string line;
  // int count = 0;
  while (getline(f, line) && result) {
    // cout << count << '\n';
    // count++;
    if (!line.empty())
      result = result && IndexarFichero(line);
  }
  return result;
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

// TODO: rehacer esto entero (la parte de obtener los InformacionTermino)
bool IndexadorHash::GuardarIndexacion() const {
  string dir = directorioIndice.empty() ? "." : directorioIndice;
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
  fConfig.close();

  ofstream fStop(dir + "/stopwords.idx");
  if (!fStop.is_open())
    return false;
  for (const auto &w : stopWords)
    fStop << w << '\n';
  fStop.close();

  ofstream fCol(dir + "/coleccion.idx");
  if (!fCol.is_open())
    return false;
  fCol << informacionColeccionDocs.getNumDocs() << '\n'
       << informacionColeccionDocs.getNumTotalPal() << '\n'
       << informacionColeccionDocs.getNumTotalPalSinParada() << '\n'
       << informacionColeccionDocs.getNumTotalPalDiferentes() << '\n'
       << informacionColeccionDocs.getTamBytes() << '\n';
  fCol.close();

  // Usar buffer grande para escritura — reduce syscalls
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
  fDocs.close();

  ofstream fIdx(dir + "/indice.idx");
  if (!fIdx.is_open())
    return false;
  fIdx.rdbuf()->pubsetbuf(nullptr, 1 << 16);

  // for (const auto &par : indice) {
  //   const InformacionTermino &inf = par.second;
  //   fIdx << "TERM " << par.first << '\n' << "ftc " << inf.getFtc() << '\n';
  //   for (const auto &docPar : inf.getLdocs()) {
  //     const InfTermDoc &itd = docPar.second;
  //     const auto &pos = itd.getPosTerm(); // ahora es vector<int>
  //     fIdx << "DOC " << docPar.first << ' ' << itd.getFt() << ' ' <<
  //     pos.size(); for (int p : pos)
  //       fIdx << ' ' << p;
  //     fIdx << '\n';
  //   }
  //   fIdx << "END\n";
  // }

  fIdx.close();

  ofstream fPreg(dir + "/pregunta.idx");
  if (!fPreg.is_open())
    return false;
  fPreg << infPregunta.getNumTotalPal() << '\n'
        << infPregunta.getNumTotalPalSinParada() << '\n'
        << infPregunta.getNumTotalPalDiferentes() << '\n';
  for (const auto &par : indicePregunta) {
    const InformacionTerminoPregunta &itp = par.second;
    const auto &pos = itp.getPosTerm(); // ahora es vector<int>
    fPreg << "TERM " << par.first << ' ' << itp.getFt() << ' ' << pos.size();
    for (int p : pos)
      fPreg << ' ' << p;
    fPreg << '\n';
  }
  fPreg.close();

  return true;
}

// TODO: rehacer esto entero (la parte de obtener los InformacionTermino)
bool IndexadorHash::RecuperarIndexacion(const string &directorioIndexacion) {
  indice.clear();
  indiceDocs.clear();
  indicePregunta.clear();
  stopWords.clear();
  pregunta = "";
  infPregunta = InformacionPregunta();
  informacionColeccionDocs = InfColeccionDocs();

  string dir = directorioIndexacion.empty() ? "." : directorioIndexacion;

  ifstream fConfig(dir + "/config.idx");
  if (!fConfig.is_open())
    return false;
  string delims;
  bool detectComp, minusc;
  getline(fConfig, ficheroStopWords);
  getline(fConfig, delims);
  fConfig >> detectComp >> minusc >> tipoStemmer >> almacenarPosTerm;
  fConfig.ignore();
  getline(fConfig, directorioIndice);
  getline(fConfig, pregunta);
  fConfig.close();

  tok = Tokenizador(delims, detectComp, minusc);

  ifstream fStop(dir + "/stopwords.idx");
  if (!fStop.is_open())
    return false;
  string w;
  while (getline(fStop, w))
    if (!w.empty())
      stopWords.insert(std::move(w));
  fStop.close();

  ifstream fCol(dir + "/coleccion.idx");
  if (!fCol.is_open())
    return false;
  int v;
  fCol >> v;
  informacionColeccionDocs.setNumDocs(v);
  fCol >> v;
  informacionColeccionDocs.setNumTotalPal(v);
  fCol >> v;
  informacionColeccionDocs.setNumTotalPalSinParada(v);
  fCol >> v;
  informacionColeccionDocs.setNumTotalPalDiferentes(v);
  fCol >> v;
  informacionColeccionDocs.setTamBytes(v);
  fCol.close();

  ifstream fDocs(dir + "/docs.idx");
  if (!fDocs.is_open())
    return false;
  string nomDoc;
  while (getline(fDocs, nomDoc)) {
    if (nomDoc.empty())
      continue;
    InfDoc d;
    int id, np, nps, npd, tb, day, month, year;
    fDocs >> id >> np >> nps >> npd >> tb >> day >> month >> year;
    fDocs.ignore();
    d.setIdDoc(id);
    d.setNumPal(np);
    d.setNumPalSinParada(nps);
    d.setNumPalDiferentes(npd);
    d.setTamBytes(tb);
    d.setFechaModificacion(Fecha(day, month, year));
    indiceDocs[std::move(nomDoc)] = std::move(d);
  }
  fDocs.close();

  ifstream fIdx(dir + "/indice.idx");
  if (!fIdx.is_open())
    return false;

  string token;
  while (fIdx >> token) {
    if (token != "TERM")
      return false;
    string term;
    fIdx >> term;
    fIdx.ignore();
    InformacionTermino inf;
    string tag;
    int ftc;
    fIdx >> tag >> ftc;
    inf.setFtc(ftc);
    while (fIdx >> tag && tag != "END") {
      if (tag != "DOC")
        return false;
      int idDoc, ft, nPos;
      fIdx >> idDoc >> ft >> nPos;
      InfTermDoc itd;
      // itd.setFt(ft);
      // OPTIMIZACIÓN: reservar capacidad del vector antes de leer
      vector<int> pos;
      pos.reserve(nPos);
      for (int i = 0; i < nPos; i++) {
        int p;
        fIdx >> p;
        pos.push_back(p);
      }
      itd.setPosTerm(std::move(pos));
      inf.addL_docs(idDoc, std::move(itd));
    }
    indice[std::move(term)] = std::move(inf);
  }
  fIdx.close();

  ifstream fPreg(dir + "/pregunta.idx");
  if (!fPreg.is_open())
    return false;
  int ntp, ntps, ntpd;
  fPreg >> ntp >> ntps >> ntpd;
  infPregunta.setNumTotalPal(ntp);
  infPregunta.setNumTotalPalSinParada(ntps);
  infPregunta.setNumTotalPalDiferentes(ntpd);
  fPreg.ignore();
  string linea;
  while (getline(fPreg, linea)) {
    if (linea.empty())
      continue;
    istringstream iss(linea);
    string tag, termP;
    iss >> tag >> termP;
    int ft, nPos;
    iss >> ft >> nPos;
    InformacionTerminoPregunta itp;
    itp.setFt(ft);
    for (int i = 0; i < nPos; i++) {
      int p;
      iss >> p;
      itp.addPosTerm(p);
    }
    indicePregunta[std::move(termP)] = std::move(itp);
  }
  fPreg.close();

  return true;
}

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
  string term = steam(word);
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
  string term = steam(word);
  if (indice.count(term)) {
    inf = indice.at(term);
    return true;
  }
  inf = InformacionTermino();
  return false;
}

// dado una palabra si está en un documento
bool IndexadorHash::Devuelve(const string &word, const string &nomDoc,
                             InfTermDoc &infDoc) const {
  string term = steam(word);
  if (!indice.count(term) || !indiceDocs.count(nomDoc)) {
    infDoc = InfTermDoc();
    return false;
  }
  int doc = indiceDocs.at(nomDoc).getidDoc();
  auto ldocs = indice.at(term);

  infDoc = *ldocs.find(doc);

  if (!infDoc.empty()) {
    return true;
  }
  // infDoc = InfTermDoc();
  return false;
}

bool IndexadorHash::Existe(const string &word) const {
  return indice.count(steam(word));
}

bool IndexadorHash::BorraDoc(const string &nomDoc) {
  auto docIt = indiceDocs.find(nomDoc);
  if (docIt == indiceDocs.end())
    return false;

  const int doc = docIt->second.getidDoc();

  // ── OPTIMIZACIÓN ────────────────────────────────────────────────────────
  // Antes: getL_docs() (copia) + setL_docs() (copia) por cada término.
  // Ahora: getL_docs_mut() da referencia directa, erase in-place.
  // ────────────────────────────────────────────────────────────────────────
  for (auto it = indice.begin(); it != indice.end();) {

    if (it->second.delete_doc(doc)) {
      // cout << "elimino " << (it->first) << endl;
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
  for (const auto &w : stopWords)
    cout << w << '\n';
}

int IndexadorHash::NumPalParada() const { return stopWords.size(); }

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
