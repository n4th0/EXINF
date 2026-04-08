#include "../include/indexadorHash.h"
#include "../include/stemmer.h"
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

static constexpr size_t IO_BUF = 1 << 20;

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
          stopWords.insert(std::move(tmp.front()));
      } else {
        stopWords.insert(word);
      }
    }
  }
  // Pre-reservar stopWords para evitar rehashing
  stopWords.reserve(stopWords.size() * 2);
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
      almacenarPosTerm(other.almacenarPosTerm), docTerminos(other.docTerminos) {
}

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
    docTerminos = other.docTerminos;
  }
  return *this;
}

IndexadorHash::~IndexadorHash() {}

bool IndexadorHash::IndexarFichero(const string &fichero) {
  int id_doc;
  {
    ifstream f(fichero);
    if (!f.is_open())
      return false;
  }

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

  // ── OPTIMIZACIÓN: leer el fichero .tk entero en memoria de una vez ────────
  const string tkFile = fichero + ".tk";
  ifstream f2(tkFile, ios::binary | ios::ate);
  if (!f2.is_open())
    return false;

  const streamsize fsize = f2.tellg();
  f2.seekg(0);
  string contenido(fsize, '\0');
  f2.read(&contenido[0], fsize);
  f2.close();

  // ── OPTIMIZACIÓN: reserva adaptativa del índice ───────────────────────────
  // Estimamos tokens como bytes/6 (longitud media de palabra)
  const size_t estimTokens = static_cast<size_t>(fsize) / 6 + 1;
  indice.reserve(indice.size() + estimTokens / 4); // solo nuevos únicos

  int numPal = 0, numPalSinParada = 0;
  int posGlobal = 0;

  // ── OPTIMIZACIÓN: índice inverso para este documento ─────────────────────
  auto &termsDeEsteDoc = docTerminos[id_doc];

  // ── OPTIMIZACIÓN: cache local term→InfTermDoc* para evitar búsquedas
  //    repetidas en el hash global cuando el mismo término aparece mucho ─────
  unordered_map<string, InfTermDoc *> cacheLocal;
  cacheLocal.reserve(estimTokens / 8);

  // Parsear contenido línea a línea sin allocaciones extra
  const char *ptr = contenido.data();
  const char *end = ptr + contenido.size();

  while (ptr < end) {
    // Encontrar fin de línea
    const char *nl = static_cast<const char *>(memchr(ptr, '\n', end - ptr));
    const char *lineEnd = nl ? nl : end;
    const size_t len = lineEnd - ptr;

    if (len == 0) {
      // Línea vacía: token vacío, solo avanzar posición
      posGlobal++;
      ptr = nl ? nl + 1 : end;
      continue;
    }

    // ── OPTIMIZACIÓN: steam directamente sobre string_view-like sin copiar ──
    // Construimos el string solo una vez para steam
    string token(ptr, len);
    ptr = nl ? nl + 1 : end;

    numPal++;

    // ── OPTIMIZACIÓN: steam una sola vez, resultado reutilizado ──────────────
    const string term = steam(token);
    // const string &term = steamOptimizado(token);

    if (stopWords.count(term)) {
      posGlobal++;
      continue;
    }

    numPalSinParada++;

    // Acceso al índice global con emplace para evitar construcción doble
    InformacionTermino &infTerm = indice[term];

    infTerm.incFtc();
    termsDeEsteDoc.insert(term);

    // ── OPTIMIZACIÓN: cache local evita segunda búsqueda en
    // indice[term].l_docs
    InfTermDoc *itdPtr;
    auto cit = cacheLocal.find(term);
    if (cit != cacheLocal.end()) {
      itdPtr = cit->second;
    } else {
      itdPtr = &infTerm.getL_docs_mut()[id_doc];
      cacheLocal[term] = itdPtr;
    }

    itdPtr->incFt();
    if (almacenarPosTerm)
      itdPtr->incPosTerm(posGlobal);

    posGlobal++;
  }

  // Número de términos diferentes = entradas únicas en el cache local
  const int numPalDif = static_cast<int>(cacheLocal.size());

  InfDoc &doc = indiceDocs[fichero];
  doc.setNumPal(numPal);
  doc.setNumPalSinParada(numPalSinParada);
  doc.setNumPalDiferentes(numPalDif);

  // Actualizar estadísticas de colección
  informacionColeccionDocs.setNumDocs(indiceDocs.size());
  informacionColeccionDocs.setNumTotalPal(
      informacionColeccionDocs.getNumTotalPal() + numPal);
  informacionColeccionDocs.setNumTotalPalSinParada(
      informacionColeccionDocs.getNumTotalPalSinParada() + numPalSinParada);
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
  while (getline(f, line) && result) {
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

bool IndexadorHash::GuardarIndexacion() const {
  string dir = directorioIndice.empty() ? "." : directorioIndice;
  mkdir(dir.c_str(), 0755);

  // ── OPTIMIZACIÓN: buffers de 512 KB para todos los ficheros ──────────────
  static char bufConfig[IO_BUF], bufStop[IO_BUF], bufCol[IO_BUF],
      bufDocs[IO_BUF], bufIdx[IO_BUF], bufPreg[IO_BUF];

  ofstream fConfig(dir + "/config.idx");
  if (!fConfig.is_open())
    return false;
  fConfig.rdbuf()->pubsetbuf(bufConfig, IO_BUF);
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
  fStop.rdbuf()->pubsetbuf(bufStop, IO_BUF);
  for (const auto &w : stopWords)
    fStop << w << '\n';
  fStop.close();

  ofstream fCol(dir + "/coleccion.idx");
  if (!fCol.is_open())
    return false;
  fCol.rdbuf()->pubsetbuf(bufCol, IO_BUF);
  fCol << informacionColeccionDocs.getNumDocs() << '\n'
       << informacionColeccionDocs.getNumTotalPal() << '\n'
       << informacionColeccionDocs.getNumTotalPalSinParada() << '\n'
       << informacionColeccionDocs.getNumTotalPalDiferentes() << '\n'
       << informacionColeccionDocs.getTamBytes() << '\n';
  fCol.close();

  ofstream fDocs(dir + "/docs.idx");
  if (!fDocs.is_open())
    return false;
  fDocs.rdbuf()->pubsetbuf(bufDocs, IO_BUF);
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

  // ── OPTIMIZACIÓN: usar ostringstream con reserve para índice grande ───────
  ofstream fIdx(dir + "/indice.idx");
  if (!fIdx.is_open())
    return false;
  fIdx.rdbuf()->pubsetbuf(bufIdx, IO_BUF);

  // ── OPTIMIZACIÓN: usar char[] + snprintf para enteros en lugar de << ─────
  char numBuf[32];
  for (const auto &par : indice) {
    const InformacionTermino &inf = par.second;
    fIdx << "TERM " << par.first << '\n' << "ftc " << inf.getFtc() << '\n';
    for (const auto &docPar : inf.getL_docs()) {
      const InfTermDoc &itd = docPar.second;
      const auto &pos = itd.getPosTerm();
      fIdx << "DOC " << docPar.first << ' ' << itd.getFt() << ' ' << pos.size();
      for (int p : pos) {
        fIdx << ' ' << p;
      }
      fIdx << '\n';
    }
    fIdx << "END\n";
  }
  fIdx.close();

  // ── OPTIMIZACIÓN: guardar también el índice inverso docTerminos ───────────
  ofstream fDocTerm(dir + "/docTerminos.idx");
  if (!fDocTerm.is_open())
    return false;
  fDocTerm.rdbuf()->pubsetbuf(nullptr, IO_BUF);
  for (const auto &par : docTerminos) {
    fDocTerm << par.first << ' ' << par.second.size() << '\n';
    for (const auto &t : par.second)
      fDocTerm << t << '\n';
  }
  fDocTerm.close();

  ofstream fPreg(dir + "/pregunta.idx");
  if (!fPreg.is_open())
    return false;
  fPreg.rdbuf()->pubsetbuf(bufPreg, IO_BUF);
  fPreg << infPregunta.getNumTotalPal() << '\n'
        << infPregunta.getNumTotalPalSinParada() << '\n'
        << infPregunta.getNumTotalPalDiferentes() << '\n';
  for (const auto &par : indicePregunta) {
    const InformacionTerminoPregunta &itp = par.second;
    const auto &pos = itp.getPosTerm();
    fPreg << "TERM " << par.first << ' ' << itp.getFt() << ' ' << pos.size();
    for (int p : pos)
      fPreg << ' ' << p;
    fPreg << '\n';
  }
  fPreg.close();

  return true;
}

bool IndexadorHash::RecuperarIndexacion(const string &directorioIndexacion) {
  indice.clear();
  indiceDocs.clear();
  indicePregunta.clear();
  stopWords.clear();
  docTerminos.clear();
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
  stopWords.reserve(stopWords.size() * 2);
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

  // ── OPTIMIZACIÓN: pre-reservar indice según numTotalPalDiferentes ─────────
  indice.reserve(informacionColeccionDocs.getNumTotalPalDiferentes() * 2);
  indiceDocs.reserve(informacionColeccionDocs.getNumDocs() * 2);

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

  // ── OPTIMIZACIÓN: leer indice.idx entero en memoria ──────────────────────
  {
    fIdx.seekg(0, ios::end);
    const streamsize fsz = fIdx.tellg();
    fIdx.seekg(0);
    string buf(fsz, '\0');
    fIdx.read(&buf[0], fsz);
    fIdx.close();

    istringstream ss(std::move(buf));
    string token;
    while (ss >> token) {
      if (token != "TERM")
        return false;
      string term;
      ss >> term;
      InformacionTermino inf;
      string tag;
      int ftc;
      ss >> tag >> ftc;
      inf.setFtc(ftc);
      while (ss >> tag && tag != "END") {
        if (tag != "DOC")
          return false;
        uint16_t idDoc, ft, nPos;
        ss >> idDoc >> ft >> nPos;
        InfTermDoc itd;
        itd.setFt(ft);
        vector<int> pos;
        pos.reserve(nPos);
        for (int i = 0; i < nPos; i++) {
          int p;
          ss >> p;
          pos.push_back(p);
        }
        itd.setPosTerm(std::move(pos));
        inf.addL_docs(idDoc, std::move(itd));
      }
      indice[std::move(term)] = std::move(inf);
    }
  }

  // ── OPTIMIZACIÓN: recuperar índice inverso si existe ─────────────────────
  {
    ifstream fDocTerm(dir + "/docTerminos.idx");
    if (fDocTerm.is_open()) {
      string line2;
      while (getline(fDocTerm, line2)) {
        if (line2.empty())
          continue;
        istringstream iss(line2);
        int docId, nTerms;
        iss >> docId >> nTerms;
        auto &tset = docTerminos[docId];
        tset.reserve(nTerms * 2);
        for (int i = 0; i < nTerms; i++) {
          string t;
          getline(fDocTerm, t);
          if (!t.empty())
            tset.insert(std::move(t));
        }
      }
      fDocTerm.close();
    } else {
      // Reconstruir desde el índice si el fichero no existe (compatibilidad)
      for (const auto &par : indice) {
        for (const auto &dp : par.second.getL_docs()) {
          docTerminos[dp.first].insert(par.first);
        }
      }
    }
  }

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
  indicePregunta.reserve(ntpd * 2);
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

  // ── OPTIMIZACIÓN: verificar existencia de términos válidos en un solo paso
  bool hayTerminos = false;
  for (const auto &t : tokens) {
    if (!stopWords.count(steam(t))) {
      hayTerminos = true;
      break;
    }
  }

  if (!hayTerminos)
    return false;

  indicePregunta.clear();
  indicePregunta.reserve(tokens.size() * 2);
  infPregunta = InformacionPregunta();
  pregunta = preg;

  int count = 0;
  int numPal = 0, numPalSinParada = 0;
  for (const auto &token : tokens) {
    // ── OPTIMIZACIÓN: steam una sola vez por token ────────────────────────
    const string term = steam(token);
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
  const string term = steam(word);
  auto it = indicePregunta.find(term);
  if (it != indicePregunta.end()) {
    inf = it->second;
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
  const string term = steam(word);
  auto it = indice.find(term);
  if (it != indice.end()) {
    inf = it->second;
    return true;
  }
  inf = InformacionTermino();
  return false;
}

bool IndexadorHash::Devuelve(const string &word, const string &nomDoc,
                             InfTermDoc &infDoc) const {
  const string term = steam(word);
  auto idxIt = indice.find(term);
  auto docIt = indiceDocs.find(nomDoc);
  if (idxIt == indice.end() || docIt == indiceDocs.end()) {
    infDoc = InfTermDoc();
    return false;
  }
  const int doc = docIt->second.getidDoc();
  const auto &ldocs = idxIt->second.getL_docs();
  auto it = ldocs.find(doc);
  if (it != ldocs.end()) {
    infDoc = it->second;
    return true;
  }
  infDoc = InfTermDoc();
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

  // ── OPTIMIZACIÓN CLAVE: usar índice inverso docTerminos ──────────────────
  // Antes: O(todos_los_términos_del_índice) - iteraba todo el índice.
  // Ahora: O(términos_del_documento) - solo toca los términos del doc.
  auto dtIt = docTerminos.find(doc);
  if (dtIt != docTerminos.end()) {
    for (const auto &term : dtIt->second) {
      auto idxIt = indice.find(term);
      if (idxIt == indice.end())
        continue;

      auto &ldocs = idxIt->second.getL_docs_mut();
      auto found = ldocs.find(doc);
      if (found != ldocs.end()) {
        idxIt->second.setFtc(idxIt->second.getFtc() - found->second.getFt());
        ldocs.erase(found);
      }
      if (ldocs.empty())
        indice.erase(idxIt);
    }
    docTerminos.erase(dtIt);
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
  docTerminos.clear();
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
  const int doc = it->second.getidDoc();

  // ── OPTIMIZACIÓN: usar docTerminos en lugar de escanear todo el índice ────
  auto dtIt = docTerminos.find(doc);
  if (dtIt == docTerminos.end())
    return true; // doc existe pero sin términos
  for (const auto &term : dtIt->second) {
    auto idxIt = indice.find(term);
    if (idxIt != indice.end())
      cout << idxIt->first << '\t' << idxIt->second << '\n';
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
