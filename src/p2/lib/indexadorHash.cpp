#include "../include/indexadorHash.h"
#include "../include/stemmer.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
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
  // ERR: el constructor original no cargaba las stopWords del fichero

  ifstream f(fichStopWords);
  string word;
  while (getline(f, word)) {
    if (!word.empty()) {
      // ERR: hay que aplicar el mismo tratamiento que a los términos indexados
      if (minuscSinAcentos) {
        list<string> tmp;
        tok.Tokenizar(word, tmp);
        if (!tmp.empty())
          stopWords.insert(tmp.front());
        // stopWords.insert(steam(tmp.front()));
      } else {
        // stopWords.insert(steam(word));
        stopWords.insert(word);
      }
    }
  }
}
// "fichStopWords" será el nombre del archivo que contendrá todas las palabras
// de parada (una palabra por cada línea del fichero) y se almacenará en el
// campo privado "ficheroStopWords". Asimismo, almacenará todas las palabras
// de parada que contenga el archivo en el campo privado "stopWords", el
// índice de palabras de parada. "delimitadores" será el string que contiene
// todos los delimitadores utilizados por el tokenizador (campo privado "tok")
// detectComp y minuscSinAcentos serán los parámetros que se pasarán al
// tokenizador "dirIndice" será el directorio del disco duro donde se
// almacenará el índice (campo privado "directorioIndice"). Si dirIndice=""
// entonces se almacenará en el directorio donde se ejecute el programa
// "tStemmer" inicializará la variable privada "tipoStemmer":
// 0 = no se aplica stemmer: se indexa el término tal y como aparece
// tokenizado 1 = stemmer de Porter para español 2 = stemmer de Porter para
// inglés "almPosTerm" inicializará la variable privada "almacenarPosTerm" Los
// índices (p.ej. índice, indiceDocs e informacionColeccionDocs) quedarán
// vacíos

IndexadorHash::IndexadorHash(const string &directorioIndexacion) {
  if (!RecuperarIndexacion(directorioIndexacion))
    throw runtime_error("No se pudo recuperar la indexacion de: " +
                        directorioIndexacion);
}
// Constructor para inicializar IndexadorHash a partir de una indexación
// previamente realizada que habrá sido almacenada en "directorioIndexacion"
// mediante el método "bool GuardarIndexacion()". Con ello toda la parte
// privada se inicializará convenientemente, igual que si se acabase de
// indexar la colección de documentos. En caso que no exista el directorio o
// que no contenga los datos de la indexación se tratará la excepción
// correspondiente

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
  fstream f2(fichero + ".tk");

  string line;
  int posGlobal = 0;
  unordered_set<string> terminos_este_doc;

  while (getline(f2, line)) {
    string term = steam(line);
    indiceDocs[fichero].incNumPal();
    if (stopWords.count(term)) {
      posGlobal++;
      continue;
    }
    auto &infTerm = indice[term];
    terminos_este_doc.insert(term);
    infTerm.incFtc();

    // // DEBUG
    // auto ldocsAntes = infTerm.getL_docs();
    // cerr << "  TERM=[" << term << "] id_doc=" << id_doc
    //      << " ftc_antes=" << infTerm.getFtc() - 1;
    // if (ldocsAntes.count(id_doc))
    //   cerr << " ft_antes=" << ldocsAntes.at(id_doc).getFt();
    // else
    //   cerr << " ft_antes=NUEVO";
    // cerr << endl;

    {
      auto ldocsCopy = infTerm.getL_docs();
      InfTermDoc &itd = ldocsCopy[id_doc];
      itd.incFt();
      if (almacenarPosTerm)
        itd.incPosTerm(posGlobal);
      infTerm.setL_docs(ldocsCopy);
    }

    // // DEBUG
    // cerr << "  TERM=[" << term << "] id_doc=" << id_doc
    //      << " ft_despues=" << infTerm.getL_docs().at(id_doc).getFt() <<
    //      endl;

    indiceDocs[fichero].incNumPalSinParada();
    posGlobal++;
  }

  indiceDocs[fichero].setNumPalDiferentes(terminos_este_doc.size());
  informacionColeccionDocs.setNumDocs(indiceDocs.size());
  informacionColeccionDocs.setNumTotalPal(
      informacionColeccionDocs.getNumTotalPal() +
      indiceDocs[fichero].getNumPal());
  informacionColeccionDocs.setNumTotalPalSinParada(
      informacionColeccionDocs.getNumTotalPalSinParada() +
      indiceDocs[fichero].getNumPalSinParada());
  informacionColeccionDocs.setNumTotalPalDiferentes(indice.size());
  informacionColeccionDocs.setTamBytes(informacionColeccionDocs.getTamBytes() +
                                       indiceDocs[fichero].getTamBytes());
  return true;
}

bool IndexadorHash::Indexar(const string &ficheroDocumentos) {
  string line;
  // ERR: fstream en modo lectura; usar ifstream
  ifstream f(ficheroDocumentos);
  if (!f.is_open())
    return false;

  bool result = true;
  int count = 0;
  while (getline(f, line) && result) {
    cout << count << '\n';
    count++;
    if (!line.empty()) // ERR: ignorar líneas vacías
      result = result && IndexarFichero(line);
  }
  return result;
}
// Devuelve true si consigue crear el índice para la colección de documentos
// detallada en ficheroDocumentos, el cual contendrá un nombre de documento
// por línea. Los añadirá a los ya existentes anteriormente en el índice.
// Devuelve falso si no finaliza la indexación (p.ej. por falta de memoria),
// mostrando el mensaje de error correspondiente, indicando el documento y
// término en el que se ha quedado, dejando en memoria lo que se haya indexado
// hasta ese momento. En el caso que aparezcan documentos repetidos,
// documentos que no existen o que ya estuviesen previamente indexados (ha de
// coincidir el nombre del documento y el directorio en que se encuentre), se
// devolverá true, mostrando el mensaje de excepción correspondiente, y se
// re-indexarán (borrar el documento previamente indexado e indexar el nuevo)
// en caso que la fecha de modificación del documento sea más reciente que la
// almacenada previamente (class "InfDoc" campo "fechaModificacion"). Los
// casos de reindexación mantendrán el mismo idDoc.

bool IndexadorHash::IndexarDirectorio(const string &dirAIndexar) {
  struct stat dir;
  int err = stat(dirAIndexar.c_str(), &dir);
  if (err == -1 || !S_ISDIR(dir.st_mode))
    return false;
  else {
    // ERR: faltaba -type f para no intentar indexar directorios como ficheros
    string cmd = "find -L \"" + dirAIndexar + "\" -type f | sort > .lista_fich";
    int ret = system(cmd.c_str());
    if (ret != 0)
      return false;
    return Indexar(".lista_fich");
  }
}
// Devuelve true si consigue crear el índice para la colección de documentos
// que se encuentra en el directorio (y subdirectorios que contenga)
// dirAIndexar (independientemente de la extensión de los mismos). Se
// considerará que todos los documentos del directorio serán ficheros de
// texto. Los añadirá a los ya existentes anteriormente en el índice. Devuelve
// falso si no finaliza la indexación (p.ej. por falta de memoria o porque no
// exista "dirAIndexar"), mostrando el mensaje de error correspondiente,
// indicando el documento y término en el que se ha quedado, dejando en
// memoria lo que se haya indexado hasta ese momento. En el caso que aparezcan
// documentos repetidos o que ya estuviesen previamente indexados (ha de
// coincidir el nombre del documento y el directorio en que se encuentre), se
// mostrará el mensaje de excepción correspondiente, y se re-indexarán (borrar
// el documento previamente indexado e indexar el nuevo) en caso que la fecha
// de modificación del documento sea más reciente que la almacenada
// previamente (class "InfDoc" campo "fechaModificacion"). Los casos de
// reindexación mantendrán el mismo idDoc.

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

  ofstream fDocs(dir + "/docs.idx");
  if (!fDocs.is_open())
    return false;
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
  for (const auto &par : indice) {
    const InformacionTermino &inf = par.second;
    fIdx << "TERM " << par.first << '\n' << "ftc " << inf.getFtc() << '\n';
    for (const auto &docPar : inf.getL_docs()) {
      const InfTermDoc &itd = docPar.second;
      const list<int> &pos = itd.getPosTerm();
      fIdx << "DOC " << docPar.first << ' ' << itd.getFt() << ' ' << pos.size();
      for (int p : pos)
        fIdx << ' ' << p;
      fIdx << '\n';
    }
    fIdx << "END\n";
  }
  fIdx.close();

  ofstream fPreg(dir + "/pregunta.idx");
  if (!fPreg.is_open())
    return false;
  fPreg << infPregunta.getNumTotalPal() << '\n'
        << infPregunta.getNumTotalPalSinParada() << '\n'
        << infPregunta.getNumTotalPalDiferentes() << '\n';
  for (const auto &par : indicePregunta) {
    const InformacionTerminoPregunta &itp = par.second;
    const list<int> &pos = itp.getPosTerm();
    fPreg << "TERM " << par.first << ' ' << itp.getFt() << ' ' << pos.size();
    for (int p : pos)
      fPreg << ' ' << p;
    fPreg << '\n';
  }
  fPreg.close();

  return true;
}
// Se guardará en disco duro (directorio contenido en la variable privada
// "directorioIndice") la indexación actualmente en memoria (incluidos todos
// los parámetros de la parte privada, incluida la indexación de la pregunta).
// La forma de almacenamiento la determinará el alumno. El objetivo es que
// esta indexación se pueda recuperar posteriormente mediante el constructor
// "IndexadorHash(const string& directorioIndexacion)". Por ejemplo,
// supongamos que se ejecuta esta secuencia de comandos: "IndexadorHash
// a("./fichStopWords.txt", "[ ,.", "./dirIndexPrueba", 0, false);
// a.Indexar("./fichConDocsAIndexar.txt"); a.GuardarIndexacion();", entonces
// mediante el comando: "IndexadorHash b("./dirIndexPrueba");" se recuperará
// la indexación realizada en la secuencia anterior, cargándola en "b"
// Devuelve falso si no finaliza la operación (p.ej. por falta de memoria, o
// el nombre del directorio contenido en "directorioIndice" no es correcto),
// mostrando el mensaje de error correspondiente, vaciando los ficheros
// generados. En caso que no existiese el directorio directorioIndice, habría
// que crearlo previamente

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
      stopWords.insert(w);
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
    indiceDocs[nomDoc] = d;
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
      itd.setFt(ft);
      list<int> pos;
      for (int i = 0; i < nPos; i++) {
        int p;
        fIdx >> p;
        pos.push_back(p);
      }
      itd.setPosTerm(pos);
      inf.addL_docs(idDoc, itd);
    }
    indice[term] = inf;
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
    indicePregunta[termP] = itp;
  }
  fPreg.close();

  return true;
}
// Vacía la indexación que tuviese en ese momento e inicializa IndexadorHash a
// partir de una indexación previamente realizada que habrá sido almacenada en
// "directorioIndexacion" mediante el método "bool GuardarIndexacion()". Con
// ello toda la parte privada se inicializará convenientemente, igual que si
// se acabase de indexar la colección de documentos. En caso que no exista el
// directorio o que no contenga los datos de la indexación se tratará la
// excepción correspondiente, y se devolverá false, dejando la indexación
// vacía.

bool IndexadorHash::IndexarPregunta(const string &preg) {
  list<string> tokens;
  // ERR: se tokenizaba preg sin steamear; hay que steamear cada token igual que
  // en IndexarFichero
  tok.Tokenizar(preg, tokens);

  // ERR: se comprobaba size()==0 antes de limpiar; si falla no debe modificar
  // nada
  bool hayTerminos = false;
  for (auto &t : tokens)
    if (!stopWords.count(steam(t))) {
      hayTerminos = true;
      break;
    }

  if (!hayTerminos)
    return false;

  indicePregunta.clear();
  infPregunta = InformacionPregunta();
  pregunta = preg;

  int count = 0; // ERR: comenzaba en -1, la primera posición debe ser 0
  int numPal = 0, numPalSinParada = 0;
  for (auto &token : tokens) {
    string term = steam(token); // ERR: steamear el token
    numPal++;

    if (stopWords.count(term)) {
      count++;
      continue;
    }
    numPalSinParada++;

    auto &itp = indicePregunta[term];
    itp.incFt();
    if (almacenarPosTerm) // ERR: respetar flag almacenarPosTerm
      itp.addPosTerm(count);
    count++;
  }

  // ERR: infPregunta nunca se rellenaba
  infPregunta.setNumTotalPal(numPal);
  infPregunta.setNumTotalPalSinParada(numPalSinParada);
  infPregunta.setNumTotalPalDiferentes(indicePregunta.size());

  return true;
}
// Devuelve true si consigue crear el índice para la pregunta "preg". Antes de
// realizar la indexación vaciará los campos privados indicePregunta e
// infPregunta Generará la misma información que en la indexación de
// documentos, pero dejándola toda accesible en memoria principal (mediante
// las variables privadas "pregunta, indicePregunta, infPregunta") Devuelve
// falso si no finaliza la operación (p.ej. por falta de memoria o bien si la
// pregunta no contiene ningún término con contenido), mostrando el mensaje de
// error correspondiente

bool IndexadorHash::DevuelvePregunta(string &preg) const {
  // ERR: la lógica original tokenizaba "preg" (el parámetro de salida, vacío)
  // en lugar de comprobar simplemente si hay términos en indicePregunta
  if (indicePregunta.empty())
    return false;
  preg = pregunta;
  return true;
}
// Devuelve true si hay una pregunta indexada (con al menos un término que no
// sea palabra de parada, o sea, que haya algún término indexado en
// indicePregunta), devolviéndo "pregunta" en "preg"

bool IndexadorHash::DevuelvePregunta(const string &word,
                                     InformacionTerminoPregunta &inf) const {
  string term = steam(word); // ERR: faltaba aplicar steam antes de buscar
  if (indicePregunta.count(term)) {
    inf = indicePregunta.at(term);
    return true;
  }
  inf = InformacionTerminoPregunta();
  return false;
}
// Devuelve true si word (aplicándole el tratamiento de stemming y mayúsculas
// correspondiente) está indexado en la pregunta, devolviendo su información
// almacenada "inf". En caso que no esté, devolvería "inf" vacío

bool IndexadorHash::DevuelvePregunta(InformacionPregunta &inf) const {
  if (indicePregunta
          .empty()) { // ERR: comprobar indicePregunta, no pregunta.empty()
    inf = InformacionPregunta();
    return false;
  }
  inf = infPregunta;
  return true;
}
// Devuelve true si hay una pregunta indexada, devolviendo su información
// almacenada (campo privado "infPregunta") en "inf". En caso que no esté,
// devolvería "inf" vacío

bool IndexadorHash::Devuelve(const string &word,
                             InformacionTermino &inf) const {
  string term = steam(word); // ERR: faltaba aplicar steam antes de buscar
  if (indice.count(term)) {
    inf = indice.at(term);
    return true;
  }
  inf = InformacionTermino();
  return false;
}
// Devuelve true si word (aplicándole el tratamiento de stemming y mayúsculas
// correspondiente) está indexado, devolviendo su información almacenada
// "inf". En caso que no esté, devolvería "inf" vacío

bool IndexadorHash::Devuelve(const string &word, const string &nomDoc,
                             InfTermDoc &infDoc) const {
  string term = steam(word); // ERR: faltaba aplicar steam antes de buscar
  if (!indice.count(term) || !indiceDocs.count(nomDoc)) {
    infDoc = InfTermDoc();
    return false;
  }
  int doc = indiceDocs.at(nomDoc).getidDoc();
  const auto &ldocs = indice.at(term).getL_docs();
  if (ldocs.count(doc)) {
    infDoc = ldocs.at(doc); // hay segfault
    return true;
  }
  infDoc = InfTermDoc();
  return false;
}
// Devuelve true si word (aplicándole el tratamiento de stemming y mayúsculas
// correspondiente) está indexado y aparece en el documento de nombre nomDoc,
// en cuyo caso devuelve la información almacenada para word en el documento.
// En caso que no esté, devolvería "InfDoc" vacío

bool IndexadorHash::Existe(const string &word) const {
  return indice.count(steam(word));
}
// Devuelve true si word (aplicándole el tratamiento de stemming y mayúsculas
// correspondiente) aparece como término indexado

bool IndexadorHash::BorraDoc(const string &nomDoc) {
  if (!indiceDocs.count(nomDoc))
    return false;

  int doc = indiceDocs.at(nomDoc).getidDoc();

  for (auto &par : indice) {
    // Usar getOrInsertDoc no aplica aquí; acceder con setL_docs tras copia
    // es correcto porque setL_docs reemplaza el mapa entero.
    const auto &ldocsRef = par.second.getL_docs();
    if (ldocsRef.count(doc)) {
      int ft = ldocsRef.at(doc).getFt();
      auto ldocs = ldocsRef; // copia para modificar
      ldocs.erase(doc);
      par.second.setFtc(par.second.getFtc() - ft);
      par.second.setL_docs(ldocs); // persistir los cambios
    }
  }

  for (auto it = indice.begin(); it != indice.end();) {
    if (it->second.getL_docs().empty())
      it = indice.erase(it);
    else
      ++it;
  }

  // ERR: faltaba borrar el documento de indiceDocs
  informacionColeccionDocs.setNumDocs(informacionColeccionDocs.getNumDocs() -
                                      1);
  informacionColeccionDocs.setNumTotalPal(
      informacionColeccionDocs.getNumTotalPal() -
      indiceDocs.at(nomDoc).getNumPal());
  informacionColeccionDocs.setNumTotalPalSinParada(
      informacionColeccionDocs.getNumTotalPalSinParada() -
      indiceDocs.at(nomDoc).getNumPalSinParada());
  informacionColeccionDocs.setNumTotalPalDiferentes(indice.size());
  informacionColeccionDocs.setTamBytes(informacionColeccionDocs.getTamBytes() -
                                       indiceDocs.at(nomDoc).getTamBytes());
  indiceDocs.erase(nomDoc);
  return true;
}
// Devuelve true si nomDoc está indexado y se realiza el borrado de todos los
// términos del documento y del documento en los campos privados "indiceDocs"
// e "informacionColeccionDocs"

void IndexadorHash::VaciarIndiceDocs() {
  indice.clear(); // ERR: también hay que vaciar el índice de términos
  indiceDocs.clear();
  informacionColeccionDocs = InfColeccionDocs();
}
// Borra todos los términos del índice de documentos: toda la indexación de
// documentos.

void IndexadorHash::VaciarIndicePreg() {
  pregunta = ""; // ERR: faltaba limpiar la cadena pregunta
  indicePregunta.clear();
  infPregunta = InformacionPregunta();
}
// Borra todos los términos del índice de la pregunta: toda la indexación de
// la pregunta actual.

int IndexadorHash::NumPalIndexadas() const { return indice.size(); }
// Devolverá el número de términos diferentes indexados (cardinalidad de campo
// privado "índice")

string IndexadorHash::DevolverFichPalParada() const { return ficheroStopWords; }
// Devuelve el contenido del campo privado "ficheroStopWords"

void IndexadorHash::ListarPalParada() const {
  for (const auto &w : stopWords)
    cout << w << '\n';
}
// Mostrará por pantalla las palabras de parada almacenadas (originales, sin
// aplicar stemming): una palabra por línea (salto de línea al final de cada
// palabra)

int IndexadorHash::NumPalParada() const { return stopWords.size(); }
// Devolverá el número de palabras de parada almacenadas

string IndexadorHash::DevolverDelimitadores() const {
  return tok.DelimitadoresPalabra();
}
// Devuelve los delimitadores utilizados por el tokenizador

bool IndexadorHash::DevolverCasosEspeciales() const {
  return tok.CasosEspeciales();
}
// Devuelve si el tokenizador analiza los casos especiales

bool IndexadorHash::DevolverPasarAminuscSinAcentos() const {
  return tok.PasarAminuscSinAcentos();
}
// Devuelve si el tokenizador pasa a minúsculas y sin acentos

bool IndexadorHash::DevolverAlmacenarPosTerm() const {
  return almacenarPosTerm;
}
// Devuelve el valor de almacenarPosTerm

string IndexadorHash::DevolverDirIndice() const { return directorioIndice; }
// Devuelve "directorioIndice" (el directorio del disco duro donde se
// almacenará el índice)

int IndexadorHash::DevolverTipoStemming() const { return tipoStemmer; }
// Devolverá el tipo de stemming realizado en la indexación de acuerdo con el
// valor indicado en la variable privada "tipoStemmer"
// 0 = no se aplica stemmer: se indexa el término tal y como aparece
// tokenizado Los siguientes valores harán que los términos a indexar se les
// aplique el stemmer y se almacene solo dicho stem. 1 = stemmer de Porter
// para español 2 = stemmer de Porter para inglés Para el stemmer de Porter se
// utilizarán los archivos stemmer.cpp y stemmer.h, concretamente las
// funciones de nombre "stemmer"

void IndexadorHash::ListarInfColeccDocs() const {
  cout << informacionColeccionDocs << '\n';
}
// Mostrar por pantalla: cout << informacionColeccionDocs << endl;

void IndexadorHash::ListarTerminos() const {
  for (const auto &par : indice)
    cout << par.first << '\t' << par.second << '\n';
}
// Mostrar por pantalla el contenido el contenido del campo privado "índice":
// cout << termino << '\t' << InformacionTermino << endl;

bool IndexadorHash::ListarTerminos(const string &nomDoc) const {
  if (!indiceDocs.count(nomDoc))
    return false;
  int doc = indiceDocs.at(nomDoc).getidDoc();
  for (const auto &par : indice) {
    if (par.second.getL_docs().count(doc))
      cout << par.first << '\t' << par.second << '\n';
  }
  return true;
}
// Devuelve true si nomDoc existe en la colección y muestra por pantalla todos
// los términos indexados del documento con nombre "nomDoc": cout << termino
// '\t' << InformacionTermino << endl; . Si no existe no se muestra nada

void IndexadorHash::ListarDocs() const {
  for (const auto &par : indiceDocs)
    cout << par.first << '\t' << par.second << '\n';
}
// Mostrar por pantalla el contenido el contenido del campo privado
// "indiceDocs": cout << nomDoc << '\t' << InfDoc << endl;

bool IndexadorHash::ListarDocs(const string &nomDoc) const {
  if (!indiceDocs.count(nomDoc))
    return false;
  cout << nomDoc << '\t' << indiceDocs.at(nomDoc) << '\n';
  return true;
}
// Devuelve true si nomDoc existe en la colección y muestra por pantalla el
// contenido del campo privado "indiceDocs" para el documento con nombre
// "nomDoc": cout << nomDoc << '\t' << InfDoc << endl; . Si no existe no se
// muestra nada
