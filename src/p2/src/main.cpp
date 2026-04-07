#include "indexadorHash.h"
#include "indexadorInformacion.h"
// #include <chrono>
#include <iostream>
// #include <list>
// #include <string>
//
using namespace std;
// #include <chrono>
// #include <iostream>

#include <chrono>
#include <iostream>

int main() {
  auto start = std::chrono::high_resolution_clock::now();

  auto end = std::chrono::high_resolution_clock::now();

  IndexadorHash a("./StopWordsEspanyol.txt", ". ,:", false, false,
                  "./indicePrueba", 0, true);

  a.IndexarDirectorio("./test");

  // a.ListarTerminos();
  std::chrono::duration<double> duration = end - start;

  std::cout << "Tiempo: " << duration.count() << " segundos\n";
}
