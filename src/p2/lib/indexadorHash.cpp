#include "../include/indexadorHash.h"

using namespace std;

IndexadorHash::IndexadorHash() {}
IndexadorHash::IndexadorHash(const string &fichStopWords,
                             const string &delimitadores,
                             const bool &detectComp,
                             const bool &minuscSinAcentos,
                             const string &dirIndice, const int &tStemmer,
                             const bool &almPosTerm)

    : ficheroStopWords(fichStopWords),
      tok(delimitadores, detectComp, minuscSinAcentos), tipoStemmer(tStemmer),
      directorioIndice(dirIndice), almacenarPosTerm(almPosTerm) {}
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

IndexadorHash::IndexadorHash(const string &directorioIndexacion) {}
// Constructor para inicializar IndexadorHash a partir de una indexación
// previamente realizada que habrá sido almacenada en "directorioIndexacion"
// mediante el método "bool GuardarIndexacion()". Con ello toda la parte
// privada se inicializará convenientemente, igual que si se acabase de
// indexar la colección de documentos. En caso que no exista el directorio o
// que no contenga los datos de la indexación se tratará la excepción
// correspondiente

IndexadorHash::IndexadorHash(const IndexadorHash &) {}

IndexadorHash::~IndexadorHash() {}

IndexadorHash &IndexadorHash::operator=(const IndexadorHash &) {}

bool IndexadorHash::Indexar(const string &ficheroDocumentos) {}
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

bool IndexadorHash::IndexarDirectorio(const string &dirAIndexar) {}
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

bool IndexadorHash::GuardarIndexacion() const {}
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

bool IndexadorHash::RecuperarIndexacion(const string &directorioIndexacion) {}
// Vacía la indexación que tuviese en ese momento e inicializa IndexadorHash a
// partir de una indexación previamente realizada que habrá sido almacenada en
// "directorioIndexacion" mediante el método "bool GuardarIndexacion()". Con
// ello toda la parte privada se inicializará convenientemente, igual que si
// se acabase de indexar la colección de documentos. En caso que no exista el
// directorio o que no contenga los datos de la indexación se tratará la
// excepción correspondiente, y se devolverá false, dejando la indexación
// vacía.

// // TODO: hay que hacer esot
// void IndexadorHash::ImprimirIndexacion() const {
//   cout << "Terminos indexados: " << endl;
//   // A continuación aparecerá un listado del contenido del campo privado
//   // "índice" donde para cada término indexado se imprimirá: cout << termino
//   // << '\t' << InformacionTermino << endl;
//   cout << "Documentos indexados: " << endl;
//   // A continuación aparecerá un listado del contenido del campo privado
//   // "indiceDocs" donde para cada documento indexado se imprimirá: cout <<
//   // nomDoc << '\t' << InfDoc << endl;
// }

bool IndexadorHash::IndexarPregunta(const string &preg) {}
// Devuelve true si consigue crear el índice para la pregunta "preg". Antes de
// realizar la indexación vaciará los campos privados indicePregunta e
// infPregunta Generará la misma información que en la indexación de
// documentos, pero dejándola toda accesible en memoria principal (mediante
// las variables privadas "pregunta, indicePregunta, infPregunta") Devuelve
// falso si no finaliza la operación (p.ej. por falta de memoria o bien si la
// pregunta no contiene ningún término con contenido), mostrando el mensaje de
// error correspondiente

bool IndexadorHash::DevuelvePregunta(string &preg) const {}
// Devuelve true si hay una pregunta indexada (con al menos un término que no
// sea palabra de parada, o sea, que haya algún término indexado en
// indicePregunta), devolviéndo "pregunta" en "preg"

bool IndexadorHash::DevuelvePregunta(const string &word,
                                     InformacionTerminoPregunta &inf) const {}
// Devuelve true si word (aplicándole el tratamiento de stemming y mayúsculas
// correspondiente) está indexado en la pregunta, devolviendo su información
// almacenada "inf". En caso que no esté, devolvería "inf" vacío

bool IndexadorHash::DevuelvePregunta(InformacionPregunta &inf) const {}
// Devuelve true si hay una pregunta indexada, devolviendo su información
// almacenada (campo privado "infPregunta") en "inf". En caso que no esté,
// devolvería "inf" vacío

// // TODO:
// void IndexadorHash::ImprimirIndexacionPregunta() {
//   cout << "Pregunta indexada: " << pregunta << endl;
//   cout << "Terminos indexados en la pregunta: " << endl;
//   // A continuación aparecerá un listado del contenido de "indicePregunta"
//   // donde para cada término indexado se imprimirá: cout << termino << '\t'
//   <<
//   // InformacionTerminoPregunta << endl;
//   cout << "Informacion de la pregunta: " << infPregunta << endl;
// }

// // TODO:
// void IndexadorHash::ImprimirPregunta() {
//   cout << "Pregunta indexada: " << pregunta << endl;
//   cout << "Informacion de la pregunta: " << infPregunta << endl;
// }

bool IndexadorHash::Devuelve(const string &word,
                             InformacionTermino &inf) const {}
// Devuelve true si word (aplicándole el tratamiento de stemming y mayúsculas
// correspondiente) está indexado, devolviendo su información almacenada
// "inf". En caso que no esté, devolvería "inf" vacío

bool IndexadorHash::Devuelve(const string &word, const string &nomDoc,
                             InfTermDoc &InfDoc) const {}
// Devuelve true si word (aplicándole el tratamiento de stemming y mayúsculas
// correspondiente) está indexado y aparece en el documento de nombre nomDoc,
// en cuyo caso devuelve la información almacenada para word en el documento.
// En caso que no esté, devolvería "InfDoc" vacío

bool IndexadorHash::Existe(const string &word) const {}
// Devuelve true si word (aplicándole el tratamiento de stemming y mayúsculas
// correspondiente) aparece como término indexado

bool IndexadorHash::BorraDoc(const string &nomDoc) {}
// Devuelve true si nomDoc está indexado y se realiza el borrado de todos los
// términos del documento y del documento en los campos privados "indiceDocs"
// e "informacionColeccionDocs"

void IndexadorHash::VaciarIndiceDocs() {}
// Borra todos los términos del índice de documentos: toda la indexación de
// documentos.

void IndexadorHash::VaciarIndicePreg() {
  // pregunta.clear();
  indicePregunta.clear();
}
// Borra todos los términos del índice de la pregunta: toda la indexación de
// la pregunta actual.

int IndexadorHash::NumPalIndexadas() const { return indice.size(); }
// Devolverá el número de términos diferentes indexados (cardinalidad de campo
// privado "índice")

string IndexadorHash::DevolverFichPalParada() const { return ficheroStopWords; }
// Devuelve el contenido del campo privado "ficheroStopWords"

void IndexadorHash::ListarPalParada() const {}
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

// 0 = no se aplica stemmer: se indexa el término tal y como aparece
// tokenizado Los siguientes valores harán que los términos a indexar se les
// aplique el stemmer y se almacene solo dicho stem. 1 = stemmer de Porter
// para español 2 = stemmer de Porter para inglés Para el stemmer de Porter se
// utilizarán los archivos stemmer.cpp y stemmer.h, concretamente las
// funciones de nombre "stemmer"
int IndexadorHash::DevolverTipoStemming() const { return tipoStemmer; }
// Devolverá el tipo de stemming realizado en la indexación de acuerdo con el
// valor indicado en la variable privada "tipoStemmer"

void IndexadorHash::ListarInfColeccDocs() const {}
// Mostrar por pantalla: cout << informacionColeccionDocs << endl;

void IndexadorHash::ListarTerminos() const {}
// Mostrar por pantalla el contenido el contenido del campo privado "índice":
// cout << termino << '\t' << InformacionTermino << endl;

bool IndexadorHash::ListarTerminos(const string &nomDoc) const {}
// Devuelve true si nomDoc existe en la colección y muestra por pantalla todos
// los términos indexados del documento con nombre "nomDoc": cout << termino
// << '\t' << InformacionTermino << endl; . Si no existe no se muestra nada

void IndexadorHash::ListarDocs() const {}
// Mostrar por pantalla el contenido el contenido del campo privado
// "indiceDocs": cout << nomDoc << '\t' << InfDoc << endl;

bool IndexadorHash::ListarDocs(const string &nomDoc) const {}
// Devuelve true si nomDoc existe en la colección y muestra por pantalla el
// contenido del campo privado "indiceDocs" para el documento con nombre
// "nomDoc": cout << nomDoc << '\t' << InfDoc << endl; . Si no existe no se
// muestra nada
