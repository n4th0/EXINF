
#include "../include/tokenizador.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>

#include <cstdlib>
#include <filesystem>
#include <numeric>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <type_traits>
#include <unordered_map>
namespace fs = std::filesystem;
#include <string_view>
using namespace std;

/*
class FastFileReader {
  char buffer[BUFFER_SIZE];
  FILE *file;

public:
  bool open(const string &filename) {
    file = fopen(filename.c_str(), "r");
    // file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);
    // file.open(filename);
    return file != NULL;
  }

  char *getline(bool &gets_a_line, size_t &str_len) {

    size_t count = 0;
    int c;
    while ((c = fgetc(file)) != EOF && (c != (int)'\n')) {
      buffer[count] = (char)c;
      ++count;
    }

    str_len = count;

    gets_a_line = c != EOF;

    return buffer;
  }

  void close() { fclose(file); }
};
*/
#include <cstdio>
#include <string>

class FastFileReader {
  // static constexpr size_t BUFFER_SIZE = 1 << 16; // 64KB

  FILE *file = nullptr;

  char buffer[BUFFER_SIZE];
  size_t pos = 0;
  size_t len = 0;

  inline bool refill() noexcept {
    len = fread(buffer, 1, BUFFER_SIZE, file);
    pos = 0;
    return len > 0;
  }

public:
  bool open(const std::string &filename) noexcept {
    file = fopen(filename.c_str(), "rb");
    pos = len = 0;
    return file != nullptr;
  }

  char inline getc() { return (char)fgetc(file); }

  // devuelve puntero interno (válido hasta siguiente llamada)
  char *getline(bool &ok, size_t &out_len) noexcept {
    if (!file) {
      ok = false;
      out_len = 0;
      return nullptr;
    }

    static thread_local char line[1 << 20]; // 1MB line buffer
    size_t count = 0;

    while (true) {
      if (pos >= len) {
        if (!refill()) {
          ok = count > 0;
          out_len = count;
          if (count)
            line[count] = '\0';
          return line;
        }
      }

      char c = buffer[pos++];

      if (c == '\n') {
        ok = true;
        out_len = count;
        line[count] = '\0';
        return line;
      }

      line[count++] = c;
    }
  }

  void close() noexcept {
    if (file) {
      fclose(file);
      file = nullptr;
    }
  }

  ~FastFileReader() { close(); }
};

class FastFileWriter {
  char buffer[BUFFER_SIZE];
  FILE *file;

public:
  void inline write(char c) const { fputc(c, file); }
  bool open(const string &filename) {
    file = fopen(filename.c_str(), "w");
    return file != NULL;
  }

  void inline write(const char *str, size_t str_len) const {
    fwrite(str, str_len, 1, file);
  }

  void writeln(const char *str, size_t str_len) const {

    // for (size_t s = 0; s < str_len; s++) {
    //   fputc(str[s], file);
    // }
    // fputc('\n', file);

    fwrite(str, str_len, 1, file);
    fwrite("\n", 1, 1, file);
  }

  void close() { fclose(file); }
};

static uint8_t NORMALIZE_TABLE[256] = {
    // 0x00-0x1F: control characters - keep as is
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    // 0x20-0x3F: punctuation and numbers
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    // 0x40-0x5F: uppercase, @, etc.
    64, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 91, 92, 93, 94, 95,
    // 0x60-0x7F: lowercase, etc.
    96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
    127,
    // 0x80-0x9F: extended ASCII
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
    143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157,
    158, 159,
    // 0xA0-0xBF
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
    175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
    190, 191,
    // 0xC0-0xDF: À-ß -> à-ß (mostly)
    97, 97, 97, 97, 97, 97, 97, 99, 101, 101, 101, 101, 105, 105, 105, 105, 209,
    0xF1, 111, 111, 111, 111, 111, 111, 117, 117, 117, 117, 117, 89, 223,
    // 0xE0-0xFF: à-ÿ
    223, 97, 97, 97, 97, 97, 97, 99, 231, 101, 101, 101, 105, 105, 105, 105,
    241, 111, 241, 111, 111, 111, 111, 111, 117, 117, 117, 117, 117, 117, 255};

// class FastFileReader {
//   static constexpr size_t BUFFER_SIZE = 8192;
//   char buffer[BUFFER_SIZE];
//   ifstream file;
//
// public:
//   bool open(const string &filename) {
//     file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);
//     file.open(filename);
//     return file.is_open();
//   }
//
//   bool getline(string &line) {
//     line.clear();
//     while (file.getline(buffer, BUFFER_SIZE)) {
//       line = buffer;
//       return true;
//     }
//     return false;
//   }
//
//   void close() { file.close(); }
// };
//
// class FastFileWriter {
//   static constexpr size_t BUFFER_SIZE = 8192;
//   char buffer[BUFFER_SIZE];
//   ofstream file;
//
// public:
//   bool open(const string &filename) {
//     file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);
//     file.open(filename);
//     return file.is_open();
//   }
//
//   void writeln(const string_view &str) {
//     file.write(str.data(), str.size());
//     file.put('\n');
//   }
//
//   void close() { file.close(); }
// };

// Inicializa delimiters a delimitadoresPalabra filtrando que no se
// introduzcan delimitadores repetidos (de izquierda a derecha, en cuyo caso
// se eliminarían los que hayan sido repetidos por la derecha);
// casosEspeciales a kcasosEspeciales; pasarAminuscSinAcentos a
// minuscSinAcentos
Tokenizador::Tokenizador(const string &delimitadoresPalabra,
                         const bool &kcasosEspeciales,
                         const bool &minuscSinAcentos) {

  casosEspeciales = kcasosEspeciales;
  pasarAminuscSinAcentos = minuscSinAcentos;
  DelimitadoresPalabra(delimitadoresPalabra);
}

// constructor copia
Tokenizador::Tokenizador(const Tokenizador &t)
    : casosEspeciales(t.casosEspeciales),
      pasarAminuscSinAcentos(t.pasarAminuscSinAcentos),
      delimiters(t.delimiters) {

  copy(t.delim.begin(), t.delim.end(), delim.begin());

  // (*this) =
  //     Tokenizador(t.delimiters, t.casosEspeciales, t.pasarAminuscSinAcentos);
  // (*this) = Tokenizador(delimitersToString(), this->casosEspeciales,
  // this->pasarAminuscSinAcentos);
}

// Inicializa delimiters=",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@"; casosEspeciales a
// true; pasarAminuscSinAcentos a false
Tokenizador::Tokenizador()
    : casosEspeciales(true), pasarAminuscSinAcentos(false) {
  DelimitadoresPalabra(",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@");
  // delimiters=",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
}

Tokenizador &Tokenizador::operator=(const Tokenizador &t) {
  if (this != &t) {
    casosEspeciales = t.casosEspeciales;
    pasarAminuscSinAcentos = t.pasarAminuscSinAcentos;
    delimiters = t.delimiters;
    std::copy(t.delim.begin(), t.delim.end(), delim.begin());
  }
  return *this;
}

// Destructor
// Pone delimiters=""
Tokenizador::~Tokenizador() = default;

static inline char normalize(unsigned char c) {
  return static_cast<char>(NORMALIZE_TABLE[c]);

  // // if (c == 0xF1) {
  // //   return 'n';
  // // }
  //
  // /* a: à á â ã ä  AND  À Á Â Ã Ä */
  // if ((c >= 0xE0 && c <= 0xE4) || (c >= 0xC0 && c <= 0xC5))
  //   return 'a';
  //
  // /* e: è é ê ë  AND  È É Ê Ë */
  // else if ((c >= 0xE8 && c <= 0xEB) || (c >= 0xC8 && c <= 0xCB))
  //   return 'e';
  //
  // /* i: ì í î ï  AND  Ì Í Î Ï */
  // else if ((c >= 0xEC && c <= 0xEF) || (c >= 0xCC && c <= 0xCF))
  //   return 'i';
  //
  // /* o: ò ó ô õ ö  AND  Ò Ó Ô Õ Ö */
  // else if ((c >= 0xF2 && c <= 0xF6) || (c >= 0xD2 && c <= 0xD6))
  //   return 'o';
  //
  // /* u: ù ú û ü  AND  Ù Ú Û Ü */
  // else if ((c >= 0xF9 && c <= 0xFC) || (c >= 0xD9 && c <= 0xDC))
  //   return 'u';
  //
  // /* Ñ */
  // else if (c == 0xD1)
  //   return 0xF1;
  //
  // /* ç Ç */
  // else if (c == 0xE7 || c == 0xC7)
  //   return 'c';
  //
  // /* ASCII */
  // else if (c >= 'A' && c <= 'Z')
  //   return c + ('a' - 'A');
  //
  // return c;
}

bool static inline isNum(char c) { return (c >= '0' && c <= '9'); }

bool Tokenizador::isUrl_pasar(char *str, size_t str_len, unsigned &posDel,
                              unsigned inicio) const {

  const char *aux = str + inicio;

  if (!(str[posDel] == ':' &&
        (!strncmp(aux, "http", 4) || !strncmp(aux, "https", 5) ||
         !strncmp(aux, "ftp", 3)))) {
    return false;
  }

  unsigned k = posDel;

  // buscar excluyendo x delimitadores
  for (; k < str_len; k++) {

    str[k] = normalize(str[k]);

    if ((!delim[(unsigned char)(str[k])] || str[k] == '_' || str[k] == ':' ||
         str[k] == '/' || str[k] == '.' || str[k] == '-' || str[k] == '?' ||
         str[k] == '=' || str[k] == '#' || str[k] == '&' || str[k] == '@') &&
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

bool Tokenizador::isUrl(char *str, size_t str_len, unsigned &posDel,
                        unsigned inicio) const {

  const char *aux = str + inicio;

  if (!(str[posDel] == ':' &&
        (!strncmp(aux, "http", 4) || !strncmp(aux, "https", 5) ||
         !strncmp(aux, "ftp", 3)))) {
    return false;
  }

  unsigned k = posDel;

  // buscar excluyendo x delimitadores
  for (; k < str_len; k++) {

    if (pasarAminuscSinAcentos) {
      str[k] = normalize(str[k]);
    }

    if ((!delim[(unsigned char)(str[k])] || str[k] == '_' || str[k] == ':' ||
         str[k] == '/' || str[k] == '.' || str[k] == '-' || str[k] == '?' ||
         str[k] == '=' || str[k] == '#' || str[k] == '&' || str[k] == '@') &&
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

bool Tokenizador::isDec_pasar(char *str, size_t str_len, unsigned &posDel,
                              unsigned &inicio, bool lookingToken,
                              bool &spezial) const {
  int SIZE = str_len;

  if ((posDel + 1) >= SIZE) {
    return false;
  }

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

  if (delim[(unsigned char)str[i]]) {
    return false;
  }

  bool hay_punto_coma_antes = true; // punto/coma
  for (; i < SIZE; i++) {

    str[i] = normalize(str[i]);

    if (hay_punto_coma_antes && (str[i] == '.' || str[i] == ',')) {
      return false;
    }

    // if (hay_punto_coma_antes && delim[(unsigned char)str[i]]) {
    //   posDel = i;
    //   return true;
    // }

    hay_punto_coma_antes = (str[i] == '.' || str[i] == ',');

    if (hay_punto_coma_antes) {
      continue;
    }

    //|| (delim[(unsigned char)str[i]] && str[i] != '%' && str[i] != '$')
    //
    if (str[i] == ' ' ||
        (delim[(unsigned char)str[i]] && str[i] != '%' && str[i] != '$')) {
      // he llegado al espacio (delimita el numero)

      if ((i - posDel) == 1) { // no hay más allá del ',' o '.'
        return false;
      }

      posDel = i;
      if (str[i - 1] == '.' || str[i - 1] == ',') {
        posDel -= 1;
      }

      return true;
    }

    // caso ".4043%" o ".4043$ "
    //|| (i + 1 < SIZE && delim[(unsigned char)str[i + 1]])
    if ((str[i] == '%' || str[i] == '$') &&
        // hay un espacio despues      || termina justo
        // despues
        ((i + 1 < SIZE && str[i + 1] == ' ') || (i + 1 == SIZE))) {

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
bool Tokenizador::isDec(char *str, size_t str_len, unsigned &posDel,
                        unsigned &inicio, bool lookingToken,
                        bool &spezial) const {
  int SIZE = str_len;

  if ((posDel + 1) >= SIZE) {
    return false;
  }

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

  if (delim[(unsigned char)str[i]]) {
    return false;
  }

  bool hay_punto_coma_antes = true; // punto/coma
  for (; i < SIZE; i++) {

    if (hay_punto_coma_antes && (str[i] == '.' || str[i] == ',')) {
      return false;
    }

    // if (hay_punto_coma_antes && delim[(unsigned char)str[i]]) {
    //   posDel = i;
    //   return true;
    // }

    hay_punto_coma_antes = (str[i] == '.' || str[i] == ',');

    if (hay_punto_coma_antes) {
      continue;
    }

    //|| (delim[(unsigned char)str[i]] && str[i] != '%' && str[i] != '$')
    //
    if (str[i] == ' ' ||
        (delim[(unsigned char)str[i]] && str[i] != '%' && str[i] != '$')) {
      // he llegado al espacio (delimita el numero)

      if ((i - posDel) == 1) { // no hay más allá del ',' o '.'
        return false;
      }

      posDel = i;
      if (str[i - 1] == '.' || str[i - 1] == ',') {
        posDel -= 1;
      }

      return true;
    }

    // caso ".4043%" o ".4043$ "
    //|| (i + 1 < SIZE && delim[(unsigned char)str[i + 1]])
    if ((str[i] == '%' || str[i] == '$') &&
        // hay un espacio despues      || termina justo
        // despues
        ((i + 1 < SIZE && str[i + 1] == ' ') || (i + 1 == SIZE))) {

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

bool Tokenizador::isMail_pasar(char *str, size_t str_len, unsigned &posDel,
                               unsigned &inicio, bool lookingToken) const {

  int SIZE = str_len;
  if (!lookingToken && str[posDel] != '@') {
    return false;
  }
  if ((posDel + 1 < SIZE && str[posDel + 1] == ' ') || posDel + 1 == SIZE) {
    return false;
  }

  bool habia_antes_un_del_esp = true;
  unsigned i = posDel + 1;
  for (; i < SIZE; i++) {

    str[i] = normalize(str[i]);

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
        ((i == SIZE - 1) || (i + 1 < SIZE && str[i + 1] == ' '))) {

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

    if (i + 1 == SIZE) {
      posDel = i + 1;
      return true;
    }
  }

  // cerr << "unespected behaviour of func isMail" << endl;
  posDel = i;
  return true;
}
bool Tokenizador::isMail(char *str, size_t str_len, unsigned &posDel,
                         unsigned &inicio, bool lookingToken) const {

  int SIZE = str_len;
  if (!lookingToken && str[posDel] != '@') {
    return false;
  }
  if ((posDel + 1 < SIZE && str[posDel + 1] == ' ') || posDel + 1 == SIZE) {
    return false;
  }

  bool habia_antes_un_del_esp = true;
  unsigned i = posDel + 1;
  for (; i < SIZE; i++) {

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
        ((i == SIZE - 1) || (i + 1 < SIZE && str[i + 1] == ' '))) {

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

    if (i + 1 == SIZE) {
      posDel = i + 1;
      return true;
    }
  }

  // cerr << "unespected behaviour of func isMail" << endl;
  posDel = i;
  return true;
}

bool Tokenizador::isAcron_pasar(char *str, size_t str_len, unsigned &posDel,
                                unsigned &inicio, bool lookingToken) const {

  int SIZE = str_len;
  if (!lookingToken && str[posDel] != '.') {
    return false;
  }

  unsigned i = posDel + 1;

  if (i < SIZE && str[i] == ' ') {
    return false;
  }

  if (i == SIZE) {
    return false;
  }

  // busco el siguiente delimitador

  bool hay_punto_antes = true;
  bool hay_acronimo = false;
  for (; i < SIZE; i++) {

    str[i] = normalize(str[i]);
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

  if (hay_acronimo) {
    posDel = i;
    if (str[posDel - 1] == '.') {
      posDel = posDel - 1;
    }
    return true;
  } else {
    return false;
  }
}

bool Tokenizador::isAcron(char *str, size_t str_len, unsigned &posDel,
                          unsigned &inicio, bool lookingToken) const {

  int SIZE = str_len;
  if (!lookingToken && str[posDel] != '.') {
    return false;
  }

  unsigned i = posDel + 1;

  if (i < SIZE && str[i] == ' ') {
    return false;
  }

  if (i == SIZE) {
    return false;
  }

  // busco el siguiente delimitador

  bool hay_punto_antes = true;
  bool hay_acronimo = false;
  for (; i < SIZE; i++) {

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

  if (hay_acronimo) {
    posDel = i;
    if (str[posDel - 1] == '.') {
      posDel = posDel - 1;
    }
    return true;
  } else {
    return false;
  }
}

bool Tokenizador::isMultip_pasar(char *str, size_t str_len, unsigned &posDel,
                                 unsigned &inicio, bool lookingToken) const {

  int SIZE = str_len;

  if (!lookingToken || str[posDel] != '-') {
    return false;
  }

  unsigned i = posDel + 1;

  if (i < SIZE && str[i] == ' ') {
    return false;
  }

  // busco el siguiente delimitador

  bool hay_punto_antes = true;
  bool hay_acronimo = false;
  for (; i < SIZE; i++) {

    str[i] = normalize(str[i]);
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

  if (hay_acronimo) {
    posDel = i;
    if (hay_punto_antes) {
      posDel = posDel - 1;
    }
    return true;
  } else {
    return false;
  }
}
bool Tokenizador::isMultip(char *str, size_t str_len, unsigned &posDel,
                           unsigned &inicio, bool lookingToken) const {

  int SIZE = str_len;

  if (!lookingToken || str[posDel] != '-') {
    return false;
  }

  unsigned i = posDel + 1;

  if (i < SIZE && str[i] == ' ') {
    return false;
  }

  // busco el siguiente delimitador

  bool hay_punto_antes = true;
  bool hay_acronimo = false;
  for (; i < SIZE; i++) {

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

  if (hay_acronimo) {
    posDel = i;
    if (hay_punto_antes) {
      posDel = posDel - 1;
    }
    return true;
  } else {
    return false;
  }
}

// Tokeniza str devolviendo el resultado en tokens. La lista tokens se vaciará
// antes de almacenar el resultado de la tokenización.
void Tokenizador::Tokenizar(const string &str, list<string> &tokens) const {

  tokens.clear();

  string s = str;

  s.reserve(str.size());
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
      if (lookingToken && s[pos] == ':' &&
          isUrl(s.data(), s.size(), pos, inicio)) {

        // string aux = s.substr(inicio, pos - inicio);

        // if (pasarAminuscSinAcentos) {
        //   for (unsigned i = 0; i < aux.size(); i++) {
        //     aux[i] = normalize(aux[i]);
        //   }
        // }

        he_comprobado_que_antes_habia_un_mail = false;
        tokens.emplace_back(s, inicio, pos - inicio);
        lookingToken = false;
        continue;
      }

      // is Num
      if ((s[pos] == '.' || s[pos] == ',') && delim[c] &&
          isDec(s.data(), s.size(), pos, inicio, lookingToken, spezial)) {

        string aux = s.substr(inicio, pos - inicio);

        if (s[inicio] == '.' || s[inicio] == ',') {
          aux = "0" + aux;
        }

        tokens.emplace_back(aux);

        if (spezial) {
          tokens.emplace_back(s, pos, 1);
          pos++;
        }

        he_comprobado_que_antes_habia_un_mail = false;
        lookingToken = false;
        continue;
      }

      // is Mail
      if (lookingToken && s[pos] == '@' &&
          !he_comprobado_que_antes_habia_un_mail &&
          isMail(s.data(), s.size(), pos, inicio, lookingToken)) {

        tokens.emplace_back(s, inicio, pos - inicio);
        // cout << "lo detecto como mail" << endl;
        lookingToken = false;
        continue;
      }

      if (lookingToken && s[pos] == '@') {
        he_comprobado_que_antes_habia_un_mail = true;
      }

      // is isAcron
      if (lookingToken && s[pos] == '.' && delim[(unsigned char)'.'] &&
          isAcron(s.data(), s.size(), pos, inicio, lookingToken)) {
        tokens.emplace_back(s, inicio, pos - inicio);
        lookingToken = false;
        he_comprobado_que_antes_habia_un_mail = false;
        continue;
      }

      // is Multi
      if (lookingToken && s[pos] == '-' && delim[(unsigned char)'-'] &&
          isMultip(s.data(), s.size(), pos, inicio, lookingToken)) {
        tokens.emplace_back(s, inicio, pos - inicio);
        lookingToken = false;
        he_comprobado_que_antes_habia_un_mail = false;
        continue;
      }

      if (delim[c]) {
        if (lookingToken) {
          tokens.emplace_back(s, inicio, pos - inicio);
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
      tokens.emplace_back(s, inicio);
    }

  } else {
    bool lookingToken = false;
    int inicio = 0;
    for (unsigned pos = 0; pos < str.size(); pos++) {
      unsigned char c = s[pos];
      if (delim[c]) {
        if (lookingToken) {
          tokens.emplace_back(s, inicio, pos - inicio);
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
      tokens.emplace_back(s, inicio);
    }
  }
}

// Tokeniza el fichero i guardando la salida en el fichero f (una palabra en
// cada línea del fichero). Devolverá true si se realiza la tokenización de
// forma correcta; false en caso contrario enviando a cerr el mensaje
// correspondiente (p.ej. que no exista el archivo i)
bool Tokenizador::Tokenizar(const string &NomFichEntr,
                            const string &NomFichSal) const {

  size_t str_len;
  FastFileReader i;
  FastFileWriter f;

  if (!i.open(NomFichEntr.c_str())) {
    cerr << "ERROR: No existe el archivo: " << NomFichEntr << endl;
    return false;
  }

  if (!f.open(NomFichSal.c_str())) {
    cerr << "ERROR: No existe el archivo: " << NomFichSal << endl;
    return false;
  }

  char *s;
  bool getline = false;
  while ((s = i.getline(getline, str_len)) && getline) {
    if (str_len == 0) {
      continue;
    }

    if (casosEspeciales) {
      if (pasarAminuscSinAcentos) {

        bool lookingToken = false;
        unsigned inicio = 0;
        bool he_comprobado_que_antes_habia_un_mail = false;
        bool spezial;

        for (unsigned pos = 0; pos < str_len; pos++) {
          spezial = false;

          s[pos] = normalize(s[pos]);

          unsigned char c = s[pos];

          // is url
          if (lookingToken && s[pos] == ':' &&
              isUrl_pasar(s, str_len, pos, inicio)) {
            he_comprobado_que_antes_habia_un_mail = false;
            f.writeln(s + inicio, pos - inicio);
            lookingToken = false;
            continue;
          }

          if ((s[pos] == '.' || s[pos] == ',') && delim[c] &&
              isDec_pasar(s, str_len, pos, inicio, lookingToken, spezial)) {

            if (s[inicio] == '.' || s[inicio] == ',') {
              f.write("0", 1);
            }

            f.writeln(s + inicio, pos - inicio);

            if (spezial) {
              f.writeln(s + pos, 1);
              pos++;
            }

            he_comprobado_que_antes_habia_un_mail = false;
            lookingToken = false;
            continue;
          }

          // is Mail
          if (lookingToken && s[pos] == '@' &&
              !he_comprobado_que_antes_habia_un_mail &&
              isMail_pasar(s, str_len, pos, inicio, lookingToken)) {

            f.writeln(s + inicio, pos - inicio);
            lookingToken = false;
            continue;
          }

          if (lookingToken && s[pos] == '@') {
            he_comprobado_que_antes_habia_un_mail = true;
          }

          // is isAcron
          if (lookingToken && s[pos] == '.' && delim[(unsigned char)'.'] &&
              isAcron_pasar(s, str_len, pos, inicio, lookingToken)) {

            f.writeln(s + inicio, pos - inicio);

            lookingToken = false;
            he_comprobado_que_antes_habia_un_mail = false;
            continue;
          }

          // is Multi
          if (lookingToken && s[pos] == '-' && delim[(unsigned char)'-'] &&
              isMultip_pasar(s, str_len, pos, inicio, lookingToken)) {

            f.writeln(s + inicio, pos - inicio);

            lookingToken = false;
            he_comprobado_que_antes_habia_un_mail = false;
            continue;
          }

          if (delim[c]) {
            if (lookingToken) {

              f.writeln(s + inicio, pos - inicio);

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
          f.writeln(s + inicio, str_len - inicio);
        }
      } else {

        bool lookingToken = false;
        unsigned inicio = 0;
        bool he_comprobado_que_antes_habia_un_mail = false;
        bool spezial;

        for (unsigned pos = 0; pos < str_len; pos++) {
          spezial = false;

          unsigned char c = s[pos];

          // is url
          if (lookingToken && s[pos] == ':' && isUrl(s, str_len, pos, inicio)) {
            he_comprobado_que_antes_habia_un_mail = false;
            f.writeln(s + inicio, pos - inicio);
            lookingToken = false;
            continue;
          }

          // is Num
          if ((s[pos] == '.' || s[pos] == ',') && delim[c] &&
              isDec(s, str_len, pos, inicio, lookingToken, spezial)) {

            if (s[inicio] == '.' || s[inicio] == ',') {
              f.write("0", 1);
            }

            f.writeln(s + inicio, pos - inicio);

            if (spezial) {

              f.writeln(s + pos, 1);
              pos++;
            }

            he_comprobado_que_antes_habia_un_mail = false;
            lookingToken = false;
            continue;
          }

          // is Mail
          if (lookingToken && s[pos] == '@' &&
              !he_comprobado_que_antes_habia_un_mail &&
              isMail(s, str_len, pos, inicio, lookingToken)) {

            f.writeln(s + inicio, pos - inicio);
            lookingToken = false;
            continue;
          }

          if (lookingToken && s[pos] == '@') {
            he_comprobado_que_antes_habia_un_mail = true;
          }

          // is isAcron
          if (lookingToken && s[pos] == '.' && delim[(unsigned char)'.'] &&
              isAcron(s, str_len, pos, inicio, lookingToken)) {

            f.writeln(s + inicio, pos - inicio);

            lookingToken = false;
            he_comprobado_que_antes_habia_un_mail = false;
            continue;
          }

          // is Multi
          if (lookingToken && s[pos] == '-' && delim[(unsigned char)'-'] &&
              isMultip(s, str_len, pos, inicio, lookingToken)) {

            f.writeln(s + inicio, pos - inicio);

            lookingToken = false;
            he_comprobado_que_antes_habia_un_mail = false;
            continue;
          }

          if (delim[c]) {
            if (lookingToken) {

              f.writeln(s + inicio, pos - inicio);

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
          f.writeln(s + inicio, str_len - inicio);
        }
      }

    } else {

      if (pasarAminuscSinAcentos) {

        bool lookingToken = false;
        char s1;
        while ((s1 = i.getc()) && s1 != (char)EOF) {
          s1 = normalize(s1);
          unsigned char c = s1;
          if (delim[c]) {
            if (lookingToken) {
              f.write('\n');
              lookingToken = false;
            }
          } else {
            if (!lookingToken) {
              lookingToken = true;
            }
            f.write(s1);
            if (s1 == '\n') {
              lookingToken = false;
            }
          }
        }

        // bool lookingToken = false;
        // int inicio = 0;
        // for (unsigned pos = 0; pos < str_len; pos++) {
        //   s[pos] = normalize(s[pos]);
        //   unsigned char c = s[pos];
        //   if (delim[c]) {
        //     if (lookingToken) {
        //       f.writeln(s + inicio, pos - inicio);
        //       lookingToken = false;
        //     }
        //   } else {
        //     if (!lookingToken) {
        //       inicio = pos;
        //       lookingToken = true;
        //     }
        //   }
        // }
        // if (lookingToken) {
        //   f.writeln(s + inicio, str_len - inicio);
        // }

      } else {

        bool lookingToken = false;
        char s1;
        while ((s1 = i.getc()) && s1 != (char)EOF) {
          unsigned char c = s1;
          if (delim[c]) {
            if (lookingToken) {
              f.write('\n');
              lookingToken = false;
            }
          } else {
            if (!lookingToken) {
              lookingToken = true;
            }
            f.write(s1);
            if (s1 == '\n') {
              lookingToken = false;
            }
          }
        }
        // bool lookingToken = false;
        // int inicio = 0;
        // for (unsigned pos = 0; pos < str_len; pos++) {
        //   unsigned char c = s[pos];
        //   if (delim[c]) {
        //     if (lookingToken) {
        //       f.writeln(s + inicio, pos - inicio);
        //       lookingToken = false;
        //     }
        //   } else {
        //     if (!lookingToken) {
        //       inicio = pos;
        //       lookingToken = true;
        //     }
        //   }
        // }
        // if (lookingToken) {
        //   f.writeln(s + inicio, str_len - inicio);
        // }
      }
    }
  }

  // fclose(i);
  i.close();
  f.close();
  // fclose(f);

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

  // return Tokenizar(NomFich, NomFich + ".tk");
  fstream f;
  string aux;
  f.open(NomFich);
  bool todoCorrecto = true;

  while (getline(f, aux)) {
    // cout << aux << endl;
    if (!Tokenizar(aux, aux + ".tk")) {
      todoCorrecto = false;
    }
  }

  f.close();

  return todoCorrecto;
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
  delim.fill(false);

  if (casosEspeciales) {
    delim[static_cast<unsigned char>(' ')] = true;
  }

  bool seen[256] = {false};
  for (unsigned char c : nuevoDelimiters) {
    if (!seen[c]) {
      seen[c] = true;
      delimiters += static_cast<char>(c);
    }
  }

  for (unsigned char c : delimiters) {
    delim[c] = true;
  }
}

// Añade al final de "delimiters" los nuevos delimitadores que aparezcan en
// "nuevoDelimiters" (no se almacenarán caracteres repetidos)
void Tokenizador::AnyadirDelimitadoresPalabra(const string &nuevoDelimiters) {

  DelimitadoresPalabra(delimiters + nuevoDelimiters);
}

// Devuelve "delimiters"
string Tokenizador::DelimitadoresPalabra() const { return delimiters; }

// Cambia la variable privada "casosEspeciales"
void Tokenizador::CasosEspeciales(const bool &nuevoCasosEspeciales) {

  /// TODO: mirar esto
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
