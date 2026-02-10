
#include "../include/tokenizador.h"
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>

#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <type_traits>
#include <unordered_map>

#include <string_view>
using namespace std;

// Inicializa delimiters a delimitadoresPalabra filtrando que no se
// introduzcan delimitadores repetidos (de izquierda a derecha, en cuyo caso
// se eliminarían los que hayan sido repetidos por la derecha);
// casosEspeciales a kcasosEspeciales; pasarAminuscSinAcentos a
// minuscSinAcentos
Tokenizador::Tokenizador(const string &delimitadoresPalabra,
                         const bool &kcasosEspeciales,
                         const bool &minuscSinAcentos) {

  // this->delimiters = this->strtohash(delimitadoresPalabra); // TODO: CHANGE

  this->casosEspeciales = kcasosEspeciales;
  this->pasarAminuscSinAcentos = minuscSinAcentos;

  DelimitadoresPalabra(delimitadoresPalabra);
}

// constructor copia
Tokenizador::Tokenizador(const Tokenizador &t) {

  (*this) =
      Tokenizador(t.delimiters, t.casosEspeciales, t.pasarAminuscSinAcentos);
  // (*this) = Tokenizador(delimitersToString(), this->casosEspeciales,
  // this->pasarAminuscSinAcentos);
}

// Inicializa delimiters=",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@"; casosEspeciales a
// true; pasarAminuscSinAcentos a false
Tokenizador::Tokenizador() {

  (*this) = Tokenizador(",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@", true, false);
}

Tokenizador &Tokenizador::operator=(const Tokenizador &t) {

  // esto tiene que ser muy lento
  // (*this) = Tokenizador(t.delimitersToString(), t.casosEspeciales,
  // t.pasarAminuscSinAcentos);
  this->casosEspeciales = t.casosEspeciales;
  this->delimiters = t.delimiters;
  this->pasarAminuscSinAcentos = t.pasarAminuscSinAcentos;

  for (unsigned i = 0; i < MAX_DELIM; i++) {
    this->delim[i] = t.delim[i];
  }

  return *this;
}

// Destructor
// Pone delimiters=""
Tokenizador::~Tokenizador() { this->delimiters.clear(); }

char normalize(unsigned char c) {

  // if (c == 0xF1) {
  //   return 'n';
  // }

  /* a: à á â ã ä  AND  À Á Â Ã Ä */
  if ((c >= 0xE0 && c <= 0xE4) || (c >= 0xC0 && c <= 0xC5))
    return 'a';

  /* e: è é ê ë  AND  È É Ê Ë */
  else if ((c >= 0xE8 && c <= 0xEB) || (c >= 0xC8 && c <= 0xCB))
    return 'e';

  /* i: ì í î ï  AND  Ì Í Î Ï */
  else if ((c >= 0xEC && c <= 0xEF) || (c >= 0xCC && c <= 0xCF))
    return 'i';

  /* o: ò ó ô õ ö  AND  Ò Ó Ô Õ Ö */
  else if ((c >= 0xF2 && c <= 0xF6) || (c >= 0xD2 && c <= 0xD6))
    return 'o';

  /* u: ù ú û ü  AND  Ù Ú Û Ü */
  else if ((c >= 0xF9 && c <= 0xFC) || (c >= 0xD9 && c <= 0xDC))
    return 'u';

  /* Ñ */
  else if (c == 0xD1)
    return 0xF1;

  /* ç Ç */
  else if (c == 0xE7 || c == 0xC7)
    return 'c';

  /* ASCII */
  else if (c >= 'A' && c <= 'Z')
    return c + ('a' - 'A');

  return c;
}

bool isNum(char c) { return (c >= '0' && c <= '9'); }

bool Tokenizador::isUrl(const string &str, unsigned &posDel,
                        unsigned inicio) const {

  string aux = str.substr(inicio, posDel - inicio);

  bool premature =
      str[posDel] == ':' && (aux == "http" || aux == "https" || aux == "ftp");

  if (!premature) {
    return false;
  }

  unsigned k = posDel;

  // buscar excluyendo x delimitadores
  for (; k < str.size(); k++) {
    if ((!delim[static_cast<unsigned char>(str[k])] || str[k] == '_' ||
         str[k] == ':' || str[k] == '/' || str[k] == '.' || str[k] == '-' ||
         str[k] == '?' || str[k] == '=' || str[k] == '#' || str[k] == '&' ||
         str[k] == '@') &&
        str[k] != ' ') {
    } else {
      break;
    }
  }

  if (posDel == k - 1) {
    return false;
  }

  posDel = k;

  return true;
}

bool Tokenizador::isDec(const string &str, unsigned &posDel, unsigned &inicio,
                        bool lookingToken, bool &spezial) const {

  // comprobar que por detrás es todo número
  if (lookingToken) {
    if ((posDel - inicio) != 0) { // hay cosas
      bool todoNum = true;
      for (unsigned i = inicio; i < posDel && todoNum; i++) {
        if (!isNum(str[i]) && str[i] != '.' && str[i] != ',') {
          todoNum = false;
        }
      }

      if (!todoNum) {
        return false;
      }
    }
  } else {
    inicio = posDel;
  }

  unsigned i = posDel + 1;
  bool hay_delimitador_antes = true; // punto/coma
  for (; i < str.size(); i++) {
    if (hay_delimitador_antes && (str[i] == '.' || str[i] == ',')) {
      return false;
    }

    hay_delimitador_antes = (str[i] == '.' || str[i] == ',');

    if (hay_delimitador_antes) {
      continue;
    }

    if (str[i] == ' ') {
      // he llegado al espacio (delimita el numero)

      if (i - posDel == 1) { // no hay más allá del ',' o '.'
        return false;
      }

      posDel = i;
      return true;
    }

    // caso ".4043%" o ".4043& "
    if ((str[i] == '%' || str[i] == '$') &&
        // hay un espacio despues      || termina justo
        // despues
        ((i + 1 < str.size() && str[i + 1] == ' ') || i + 1 == str.size())) {

      if ((i + 1) - posDel == 1) { // no hay antes del '%' o '$'
        return false;
      }

      posDel = i;
      spezial = true;
      return true;
    }

    if (delim[(unsigned char)str[i]]) {
      posDel = i;
      return true;
    }

    if (!isNum(str[i])) {
      return false;
    }
  }

  if (isNum(str[i - 1])) {
    posDel = i;
  } else {
    posDel = i - 1;
  }

  return true;
}

bool Tokenizador::isMail(const string &str, unsigned &posDel, unsigned &inicio,
                         bool lookingToken) const {

  if (!lookingToken && str[posDel] != '@') {
    return false;
  }
  if ((posDel + 1 < str.size() && str[posDel + 1] == ' ') ||
      posDel + 1 == str.size()) {
    return false;
  }

  bool habia_antes_un_del_esp = false;
  // TODO: "something@.ua.es" es mail?
  unsigned i = posDel + 1;
  for (; i < str.size(); i++) {

    if (str[i] == '@') {
      return false;
    }

    if (str[i] == ' ') {
      posDel = i;
      return true;
    }

    if (habia_antes_un_del_esp &&
        (str[i] == '_' || str[i] == '.' || str[i] == '-')) {
      // algo@--
      if (posDel == i - 2) {
        return false;
      }
      posDel = i - 1;
      return true;
    }

    // si llega al final con un . no es un mail. lo es si hay
    // main antes
    // "algo@b-" -> algo@b
    if ((str[i] == '_' || str[i] == '.' || str[i] == '-') &&
        ((i == str.size() - 1) || (i + 1 < str.size() && str[i + 1] == ' '))) {

      if (posDel == i - 1) {
        return false;
      }
      posDel = i;
      return true;
    }

    if ((str[i] == '_' || str[i] == '.' || str[i] == '-')) {
      habia_antes_un_del_esp = true;
      continue;
    }

    habia_antes_un_del_esp = false;

    if (delim[(unsigned char)str[i]]) {
      posDel = i;
      return true;
    }

    if (i + 1 == str.size()) {
      posDel = i + 1;
      return true;
    }
  }

  // cerr << "unespected behaviour of func isMail" << endl;
  posDel = i;
  return true;
}

bool Tokenizador::isAcron(const string &str, unsigned &posDel, unsigned &inicio,
                          bool lookingToken) const {

  if (!lookingToken && str[posDel] != '.') {
    return false;
  }

  unsigned i = posDel + 1;

  if (i < str.size() && str[i] == ' ') {
    return false;
  }

  // busco el siguiente delimitador

  bool hay_punto_antes = true;
  bool hay_acronimo = false;
  for (; i < str.size(); i++) {

    // hay 2 puntos seguidos
    if (str[i] == '.' && hay_punto_antes) {

      if (hay_acronimo) {
        posDel = i - 1;
        return true;
      }

      return false;
    }

    if (hay_punto_antes && !delim[(unsigned char)str[i]]) {
      hay_acronimo = true;
    }

    if (delim[(unsigned char)str[i]] && str[i] != '.') {
      if (hay_acronimo) {

        if (hay_punto_antes) {

          posDel = i - 1;
        } else {
          posDel = i;
        }
        return true;
      }
      return false;
    }

    hay_punto_antes = str[i] == '.';
  }

  // cerr << "Unespected behaviour of func isAcro" << endl;

  posDel = i;
  return true;
}

bool Tokenizador::isMultip(const string &str, unsigned &posDel,
                           unsigned &inicio, bool lookingToken) const {

  if (!lookingToken && str[posDel] != '-') {
    return false;
  }

  unsigned i = posDel + 1;

  if (i < str.size() && str[i] == ' ') {
    return false;
  }

  // busco el siguiente delimitador

  bool hay_punto_antes = true;
  bool hay_acronimo = false;
  for (; i < str.size(); i++) {

    // hay 2 puntos seguidos
    if (str[i] == '-' && hay_punto_antes) {

      if (hay_acronimo) {
        posDel = i - 1;
        return true;
      }

      return false;
    }

    if (hay_punto_antes && !delim[(unsigned char)str[i]]) {
      hay_acronimo = true;
    }

    if (delim[(unsigned char)str[i]] && str[i] != '-') {
      if (hay_acronimo) {

        if (hay_punto_antes) {

          posDel = i - 1;
        } else {
          posDel = i;
        }
        return true;
      }
      return false;
    }

    hay_punto_antes = str[i] == '-';
  }

  posDel = i;
  // cerr << "Unespected behaviour of func isMultip" << endl;

  return true;
}

// string::size_type Tokenizador::buscar(const string &str, unsigned pos,
//                                       bool busca_delimitador) const {
//   for (size_t i = pos; i < str.size(); i++) {
//     if (busca_delimitador) {
//       if (delim[static_cast<unsigned char>(str[i])] ||
//           (casosEspeciales && str[i] == ':')) {
//         return i;
//       }
//     } else {
//       if (!delim[static_cast<unsigned char>(str[i])] &&
//           !(casosEspeciales && str[i] == ':')) {
//         return i;
//       }
//     }
//   }
//   return string::npos;
// }

// Tokeniza str devolviendo el resultado en tokens. La lista tokens se vaciará
// antes de almacenar el resultado de la tokenización.
void Tokenizador::Tokenizar(const string &str, list<string> &tokens) const {

  tokens.clear();

  string s = str;

  if (pasarAminuscSinAcentos) {
    for (unsigned i = 0; i < s.size(); i++) {
      s[i] = normalize(s[i]);
    }
  }

  if (casosEspeciales) {
    bool lookingToken = false;
    unsigned inicio = 0;
    bool he_comprobado_que_antes_habia_un_mail = false;
    bool spezial;

    for (unsigned pos = 0; pos < str.size(); pos++) {
      spezial = false;
      unsigned char c = s[pos];

      // is url
      if (lookingToken && s[pos] == ':' && isUrl(s, pos, inicio)) {
        string aux = s.substr(inicio, pos - inicio);

        if (pasarAminuscSinAcentos) {
          for (unsigned i = 0; i < aux.size(); i++) {
            aux[i] = normalize(aux[i]);
          }
        }
        he_comprobado_que_antes_habia_un_mail = false;
        tokens.emplace_back(aux);
        lookingToken = false;
        continue;
      }

      // is Num
      if ((s[pos] == '.' || s[pos] == ',') && delim[c] &&
          isDec(s, pos, inicio, lookingToken, spezial)) {

        string aux = s.substr(inicio, pos - inicio);
        if (s[inicio] == '.' || s[inicio] == ',') {
          aux = "0" + aux;
        }

        // if (pos + 1 < s.size() && s[pos + 1] != ' ') {
        //   tokens.emplace_back(aux);
        //   ++pos;
        // } else {

        tokens.emplace_back(aux);

        if (spezial) {
          tokens.emplace_back(s.substr(pos, 1));
          pos++;
        }

        //   pos++;
        // }

        // if (s[pos] == ' ') {
        //   tokens.emplace_back(s.substr(inicio, pos - inicio));
        //   lookingToken = false;
        //   continue;
        // }
        he_comprobado_que_antes_habia_un_mail = false;
        // tokens.emplace_back(s.substr(inicio, pos - inicio));
        lookingToken = false;
        continue;
      }

      // is Mail
      if (lookingToken && s[pos] == '@' &&
          !he_comprobado_que_antes_habia_un_mail &&
          isMail(s, pos, inicio, lookingToken)) {

        tokens.emplace_back(s.substr(inicio, pos - inicio));
        // cout << "lo detecto como mail" << endl;
        lookingToken = false;
        continue;
      }

      if (lookingToken && s[pos] == '@') {
        he_comprobado_que_antes_habia_un_mail = true;
      }

      // is isAcron
      if (lookingToken && s[pos] == '.' && delim[(unsigned char)'.'] &&
          isAcron(s, pos, inicio, lookingToken)) {
        tokens.emplace_back(s.substr(inicio, pos - inicio));
        lookingToken = false;
        he_comprobado_que_antes_habia_un_mail = false;
        continue;
      }

      // is Multi
      if (lookingToken && s[pos] == '-' && delim[(unsigned char)'-'] &&
          isMultip(s, pos, inicio, lookingToken)) {
        tokens.emplace_back(s.substr(inicio, pos - inicio));
        lookingToken = false;
        he_comprobado_que_antes_habia_un_mail = false;
        continue;
      }

      // cout << "llego " << pos << endl;

      if (delim[c]) {
        if (lookingToken) {
          tokens.emplace_back(s.substr(inicio, pos - inicio));
          lookingToken = false;
          he_comprobado_que_antes_habia_un_mail = false;
        }
      } else {
        if (!lookingToken) {
          inicio = pos;
          lookingToken = true;
        }
      }
    }

    if (lookingToken) {
      tokens.emplace_back(s.substr(inicio));
    }

  } else {
    bool lookingToken = false;
    int inicio = 0;
    for (unsigned pos = 0; pos < str.size(); pos++) {
      unsigned char c = s[pos];
      if (delim[c]) {
        if (lookingToken) {
          tokens.emplace_back(s.substr(inicio, pos - inicio));
          lookingToken = false;
        }
      } else {
        if (!lookingToken) {
          inicio = pos;
          lookingToken = true;
        }
      }
    }
    if (lookingToken) {
      tokens.emplace_back(s.substr(inicio));
    }
  }
}

// Tokeniza el fichero i guardando la salida en el fichero f (una palabra en
// cada línea del fichero). Devolverá true si se realiza la tokenización de
// forma correcta; false en caso contrario enviando a cerr el mensaje
// correspondiente (p.ej. que no exista el archivo i)
bool Tokenizador::Tokenizar(const string &NomFichEntr,
                            const string &NomFichSal) const {
  ifstream i;
  ofstream f2;

  string str;

  i.open(NomFichEntr.c_str());
  f2.open(NomFichSal.c_str());

  // i2.open(NomFichSal.c_str());

  if (!i) {
    cerr << "ERROR : No existe el archivo : " << NomFichEntr << endl;
    return false;
  } else if (!f2) {
    cerr << "ERROR : No se ha podido abrir el archivo : " << NomFichSal << endl;
    return false;
  } else {

    while (!i.eof()) {
      str = "";
      getline(i, str);
      if (str.length() != 0) {

        /// inicio Tokenizar
        // tokens.clear();

        // string s = str;

        if (pasarAminuscSinAcentos) {
          for (unsigned i = 0; i < str.size(); i++) {
            str[i] = normalize(str[i]);
          }
        }

        if (casosEspeciales) {
          bool lookingToken = false;
          unsigned inicio = 0;
          bool he_comprobado_que_antes_habia_un_mail = false;
          bool spezial;

          for (unsigned pos = 0; pos < str.size(); pos++) {
            unsigned char c = str[pos];

            spezial = false;
            // is url
            if (lookingToken && str[pos] == ':' && isUrl(str, pos, inicio)) {
              string aux = str.substr(inicio, pos - inicio);

              // creo que esto es innecesario
              if (pasarAminuscSinAcentos) {
                for (unsigned i = 0; i < aux.size(); i++) {
                  aux[i] = normalize(aux[i]);
                }
              }

              he_comprobado_que_antes_habia_un_mail = false;
              f2 << aux << endl;
              // tokens.emplace_back(aux);
              lookingToken = false;
              continue;
            }

            // is Num
            if ((str[pos] == '.' || str[pos] == ',') && delim[c] &&
                isDec(str, pos, inicio, lookingToken, spezial)) {

              string aux = str.substr(inicio, pos - inicio);

              if (str[inicio] == '.' || str[inicio] == ',') {
                aux = "0" + aux;
              }

              f2 << aux << endl;
              // tokens.emplace_back(aux);

              if (spezial) {
                pos++;
                f2 << str[pos] << endl;
              }

              he_comprobado_que_antes_habia_un_mail = false;
              // tokens.emplace_back(s.substr(inicio, pos - inicio));
              lookingToken = false;
              continue;
            }

            // is Mail
            if (lookingToken && str[pos] == '@' &&
                !he_comprobado_que_antes_habia_un_mail &&
                isMail(str, pos, inicio, lookingToken)) {

              f2 << str.substr(inicio, pos - inicio) << endl;
              // tokens.emplace_back(s.substr(inicio, pos - inicio));
              lookingToken = false;
              continue;
            }

            if (lookingToken && str[pos] == '@') {
              he_comprobado_que_antes_habia_un_mail = true;
            }

            // is isAcron
            if (lookingToken && str[pos] == '.' && delim[(unsigned char)'.'] &&
                isAcron(str, pos, inicio, lookingToken)) {
              // tokens.emplace_back(s.substr(inicio, pos - inicio));
              f2 << str.substr(inicio, pos - inicio) << endl;
              lookingToken = false;
              he_comprobado_que_antes_habia_un_mail = false;
              continue;
            }

            // is Multi
            if (lookingToken && str[pos] == '-' && delim[(unsigned char)'-'] &&
                isMultip(str, pos, inicio, lookingToken)) {
              f2 << str.substr(inicio, pos - inicio) << endl;
              // tokens.emplace_back(s.substr(inicio, pos - inicio));
              lookingToken = false;
              he_comprobado_que_antes_habia_un_mail = false;
              continue;
            }

            if (delim[c]) {
              if (lookingToken) {
                f2 << str.substr(inicio, pos - inicio) << endl;
                // tokens.emplace_back(s.substr(inicio, pos - inicio));
                lookingToken = false;
                he_comprobado_que_antes_habia_un_mail = false;
              }
            } else {
              if (!lookingToken) {
                inicio = pos;
                lookingToken = true;
              }
            }
          }
          if (lookingToken) {

            f2 << str.substr(inicio) << endl;
            // tokens.emplace_back(s.substr(inicio));
          }

        } else {
          bool lookingToken = false;
          int inicio = 0;
          for (unsigned pos = 0; pos < str.size(); pos++) {
            unsigned char c = str[pos];
            if (delim[c]) {
              if (lookingToken) {
                f2 << str.substr(inicio, pos - inicio) << endl;
                // tokens.emplace_back(s.substr(inicio, pos - inicio));
                lookingToken = false;
              }
            } else {
              if (!lookingToken) {
                inicio = pos;
                lookingToken = true;
              }
            }
          }
          if (lookingToken) {
            f2 << str.substr(inicio) << endl;
            // tokens.emplace_back(s.substr(inicio));
          }
        }
      }
    }
  }

  i.close();
  // f2.open(NomFichSal.c_str());
  // list<string>::iterator itS;
  // for (itS = tokens.begin(); itS != tokens.end(); itS++) {
  //   f2 << (*itS) << endl;
  // }
  f2.close();
  return true;
}

// Tokeniza el fichero i guardando la salida en un fichero de nombre i
// añadiéndole extensión .tk (sin eliminar previamente la extensión de i por
// ejemplo, del archivo pp.txt se generaría el resultado en pp.txt.tk), y que
// contendrá una palabra en cada línea del fichero. Devolverá true si se
// realiza la tokenización de forma correcta; false en caso contrario enviando
// a cerr el mensaje correspondiente (p.ej. que no exista el archivo i)
bool Tokenizador::Tokenizar(const string &i) const {
  return this->Tokenizar(i, i + ".tk");
}

// TODO: Optimizar al fallo
//
//
// Tokeniza el fichero i que contiene un nombre de fichero por línea guardando
// la salida en ficheros (uno por cada línea de i) cuyo nombre será el leído
// en i añadiéndole extensión .tk, y que contendrá una palabra en cada línea
// del fichero leído en i. Devolverá true si se realiza la tokenización de
// caso contrario enviando a cerr el mensaje correspondiente (p.ej. que no
// exista el archivo i, o que se trate de un directorio, enviando a "cerr" los
// archivos de i que no existan o que sean directorios; luego no se ha de
// interrumpir la ejecución si hay algún archivo en i que no exista)
bool Tokenizador::TokenizarListaFicheros(const string &NomFich) const {

  ifstream i;
  string cadena;

  i.open(NomFich.c_str());

  if (!i) {
    cerr << "ERROR : No existe el archivo : " << NomFich << endl;
    return false;
  } else {

    while (!i.eof()) {
      // cadena = "";

      // merece la pena comprobar que no lea archivos duplicados
      getline(i, cadena);

      // cout << "llego " << cadena << endl;
      if (cadena.length() != 0) {

        if (!Tokenizar(cadena, cadena + ".tk"))
          return false;
      }
    }
  }

  return true;
}

// Tokeniza todos los archivos que contenga el directorio i, incluyendo los de
// los subdirectorios, guardando la salida en ficheros cuyo nombre será el de
// entrada añadiéndole extensión .tk, y que contendrá una palabra en cada
// línea del fichero. Devolverá true si se realiza la tokenización de forma
// correcta de todos los archivos; devolverá false en caso contrario enviando
// a cerr el mensaje correspondiente (p.ej. que no exista el directorio i, o
// los ficheros que no se hayan podido tokenizar)
bool Tokenizador::TokenizarDirectorio(const string &dirAIndexar) const {
  struct stat dir;
  // Compruebo la existencia del directorio
  int err = stat(dirAIndexar.c_str(), &dir);
  if (err == -1 || !S_ISDIR(dir.st_mode))
    return false;
  else {
    // Hago una lista en un fichero con find>fich
    string cmd = "find " + dirAIndexar + " -follow |sort > .lista_fich";
    system(cmd.c_str());
    return TokenizarListaFicheros(".lista_fich");
  }
}

// Inicializa delimiters a nuevoDelimiters, filtrando que no se introduzcan
// delimitadores repetidos (de izquierda a derecha, en cuyo caso se
// eliminarían los que hayan sido repetidos por la derecha)
void Tokenizador::DelimitadoresPalabra(const string &nuevoDelimiters) {

  delimiters.clear();

  for (int i = 0; i < MAX_DELIM; i++) {
    delim[i] = false;
  }

  if (this->CasosEspeciales()) {
    delim[static_cast<unsigned char>(' ')] = true;
  }

  delimiters = nuevoDelimiters;
  delimiters = uniq();

  for (unsigned i = 0; i < delimiters.size(); i++) {
    delim[static_cast<unsigned char>(delimiters[i])] = true;
    // cout << "llego con " << delimiters[i] << endl;
    // delim[(unsigned)delimiters[i]] = true;
  }
}

// Añade al final de "delimiters" los nuevos delimitadores que aparezcan en
// "nuevoDelimiters" (no se almacenarán caracteres repetidos)
void Tokenizador::AnyadirDelimitadoresPalabra(const string &nuevoDelimiters) {

  string s = delimiters;
  s += nuevoDelimiters;

  DelimitadoresPalabra(s);
}

// Devuelve "delimiters"
string Tokenizador::DelimitadoresPalabra() const { return delimiters; }

// Cambia la variable privada "casosEspeciales"
void Tokenizador::CasosEspeciales(const bool &nuevoCasosEspeciales) {

  if (casosEspeciales)
    delim[(unsigned char)' '] = nuevoCasosEspeciales;

  casosEspeciales = nuevoCasosEspeciales;
}

// Devuelve el contenido de la variable privada "casosEspeciales"
bool Tokenizador::CasosEspeciales() const { return casosEspeciales; }

// Cambia la variable privada "pasarAminuscSinAcentos". Atención al formato de
// codificación del corpus (comando "file" de Linux). Para la corrección de la
// práctica se utilizará el formato actual (ISO-8859).
void Tokenizador::PasarAminuscSinAcentos(
    const bool &nuevoPasarAminuscSinAcentos) {
  this->pasarAminuscSinAcentos = nuevoPasarAminuscSinAcentos;
}

// Devuelve el contenido de la variable privada "pasarAminuscSinAcentos"
bool Tokenizador::PasarAminuscSinAcentos() const {
  return pasarAminuscSinAcentos;
}

/////////////////////////////////
//        MIAS                ///
/////////////////////////////////

string Tokenizador::uniq() const {
  string result = "";
  for (unsigned i = 0; i < delimiters.size(); i++) {
    char c = delimiters[i];
    bool found = false;

    for (unsigned j = 0; j < result.size() && !found; j++) {
      if (result[j] == c) {
        found = true;
      }
    }

    if (!found) {
      result += c;
    }
  }

  return result;
}

// cout << "DELIMITADORES: " << delimiters << " TRATA CASOS ESPECIALES: " <<
// casosEspeciales << " PASAR A MINUSCULAS Y SIN ACENTOS: " <<
// pasarAminuscSinAcentos; Aunque se modifique el almacenamiento de los
// delimitadores por temas de eficiencia, el campo delimiters se imprimirá con
// el string leído en el tokenizador (tras las modificaciones y eliminación de
// los caracteres repetidos correspondientes)
ostream &operator<<(ostream &os, const Tokenizador &t) {

  os << "DELIMITADORES: " << t.DelimitadoresPalabra()
     << " TRATA CASOS ESPECIALES: " << t.casosEspeciales
     << " PASAR A MINUSCULAS Y SIN ACENTOS: " << t.pasarAminuscSinAcentos;

  return os;
}
