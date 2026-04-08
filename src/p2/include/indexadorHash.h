#ifndef _INDEX_HASH_
#define _INDEX_HASH_
#include "./indexadorInformacion.h"
#include "./tokenizador.h"
#include "stemmer.h"
#include <iostream>
#include <unordered_map>
#include <unordered_set>

using namespace std;

class IndexadorHash {

  inline friend ostream &operator<<(ostream &s, const IndexadorHash &p) {
    s << "Fichero con el listado de palabras de parada: " << p.ficheroStopWords
      << '\n';
    s << "Tokenizador: " << p.tok << '\n';
    s << "Directorio donde se almacenara el indice generado: "
      << p.directorioIndice << '\n';
    s << "Stemmer utilizado: " << p.tipoStemmer << '\n';
    s << "Informacion de la coleccion indexada: " << p.informacionColeccionDocs
      << '\n';
    s << "Se almacenaran las posiciones de los terminos: "
      << p.almacenarPosTerm;

    return s;
  }

public:
  IndexadorHash(const string &fichStopWords, const string &delimitadores,
                const bool &detectComp, const bool &minuscSinAcentos,
                const string &dirIndice, const int &tStemmer,
                const bool &almPosTerm);

  IndexadorHash(const string &directorioIndexacion);

  IndexadorHash(const IndexadorHash &);

  ~IndexadorHash();

  IndexadorHash &operator=(const IndexadorHash &);

  bool Indexar(const string &ficheroDocumentos);

  bool IndexarDirectorio(const string &dirAIndexar);

  bool GuardarIndexacion() const;

  bool RecuperarIndexacion(const string &directorioIndexacion);

  void ImprimirIndexacion() const {
    cout << "Terminos indexados: " << '\n';
    ListarTerminos();
    cout << "Documentos indexados: " << '\n';
    ListarDocs();
  }

  bool IndexarPregunta(const string &preg);

  bool DevuelvePregunta(string &preg) const;

  bool DevuelvePregunta(const string &word,
                        InformacionTerminoPregunta &inf) const;

  bool DevuelvePregunta(InformacionPregunta &inf) const;

  void ImprimirIndexacionPregunta() {
    cout << "Pregunta indexada: " << pregunta << '\n';
    cout << "Terminos indexados en la pregunta: " << '\n';
    for (auto it = indicePregunta.begin(); it != indicePregunta.end(); it++)
      cout << (*it).first << '\t' << (*it).second << '\n';
    cout << "Informacion de la pregunta: " << infPregunta << '\n';
  }

  void ImprimirPregunta() {
    cout << "Pregunta indexada: " << pregunta << '\n';
    cout << "Informacion de la pregunta: " << infPregunta << '\n';
  }

  bool Devuelve(const string &word, InformacionTermino &inf) const;

  bool Devuelve(const string &word, const string &nomDoc,
                InfTermDoc &InfDoc) const;

  bool Existe(const string &word) const;

  bool BorraDoc(const string &nomDoc);

  void VaciarIndiceDocs();

  void VaciarIndicePreg();

  int NumPalIndexadas() const;

  string DevolverFichPalParada() const;

  void ListarPalParada() const;

  int NumPalParada() const;

  string DevolverDelimitadores() const;

  bool DevolverCasosEspeciales() const;

  bool DevolverPasarAminuscSinAcentos() const;

  bool DevolverAlmacenarPosTerm() const;

  string DevolverDirIndice() const;

  int DevolverTipoStemming() const;

  void ListarInfColeccDocs() const;

  void ListarTerminos() const;

  bool ListarTerminos(const string &nomDoc) const;

  void ListarDocs() const;

  bool ListarDocs(const string &nomDoc) const;

private:
  IndexadorHash();
  unordered_set<string> termPool;
  // mutable unordered_map<string, string> stemmerCache;
  // string steamOptimizado(const string &token) const;
  unordered_map<string, InformacionTermino> indice;
  unordered_map<string, InfDoc> indiceDocs;
  unordered_map<string, InformacionTerminoPregunta> indicePregunta;
  unordered_set<string> stopWords;
  InfColeccionDocs informacionColeccionDocs;
  string pregunta = "";
  InformacionPregunta infPregunta;
  string ficheroStopWords = "";
  Tokenizador tok;
  string directorioIndice;
  int tipoStemmer;
  bool almacenarPosTerm;

  unordered_map<int, unordered_set<string>> docTerminos;

  string steam(const string &s) const;

  bool IndexarFichero(const string &ficheroDocumentos);
};
#endif
