#include "tokenizador.h"
#include <fstream>
#include <iostream>
#include <list>
#include <string>

#include <chrono>
#include <iostream>
using namespace std;

///////// Comprobación de que vacíe la lista resultado

// class FastFileReader {
//   static constexpr size_t BUFFER_SIZE = 1 << 20; // 1MB
//   std::ifstream file;
//   char buffer[BUFFER_SIZE];
//   size_t pos = 0, end = 0;
//
// public:
//   bool open(const std::string &filename) {
//     file.open(filename, std::ios::binary);
//     return file.is_open();
//   }
//
//   bool getline(std::string &line) {
//     line.clear();
//
//     while (true) {
//       if (pos >= end) {
//         file.read(buffer, BUFFER_SIZE);
//         end = file.gcount();
//         pos = 0;
//
//         if (end == 0)
//           return !line.empty();
//       }
//
//       char c = buffer[pos++];
//       if (c == '\n')
//         return true;
//
//       line += c;
//     }
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

int main() {

  // FastFileReader i;
  // FastFileWriter f;
  // i.open("./test/file.txt");
  // f.open("./test/file.txt.tk");
  // string s;
  // while (i.getline(s)) {
  // // cout << s << endl;
  // f.writeln(s);
  // }
  // i.close();
  // f.close();
  // return 0;

  {

    bool kCasosEspeciales = true, kpasarAminusculas = true;

    Tokenizador a("@.&", true, true);

    a.DelimitadoresPalabra("/ &_:/.?&-=#@");

    auto inicio = std::chrono::high_resolution_clock::now();

    // a.TokenizarListaFicheros("./temp.txt");
    a.TokenizarListaFicheros("./test/listaFicheros.txt");
    auto fin = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duracion = fin - inicio;
    std::cout << "Tiempo: " << duracion.count() << " segundos\n";
  }

  // {
  //   bool kCasosEspeciales = true, kpasarAminusculas = false;
  //
  //   Tokenizador a("@.&", true, true);
  //
  //   a.DelimitadoresPalabra("/ &_:/.?&-=#@");
  //
  //   auto inicio = std::chrono::high_resolution_clock::now();
  //
  //   a.Tokenizar("./test/file.txt", "./test/file.txt.tk");
  //   auto fin = std::chrono::high_resolution_clock::now();
  //
  //   std::chrono::duration<double> duracion = fin - inicio;
  //   std::cout << "Tiempo: " << duracion.count() << " segundos\n";
  //   // a.TokenizarListaFicheros("temp.txt");
  // }
}
