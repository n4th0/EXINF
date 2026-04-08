#include "indexadorHash.h"
#include "indexadorInformacion.h"
#include <chrono>
// #include <chrono>
// #include <list>
// #include <string>
//
using namespace std;

int main() {
  auto start = std::chrono::high_resolution_clock::now();

  IndexadorHash a("./StopWordsEspanyol.txt", ". ,:", false, false,
                  "./indicePrueba", 0, true);

  a.IndexarDirectorio("./corpus");

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;

  std::cerr << "Tiempo: " << duration.count() << " segundos\n";

  // a.ImprimirIndexacion();

  // system("rm ./test/corpus/_.tk");
  system("rm ./corpus/*.tk");
}
