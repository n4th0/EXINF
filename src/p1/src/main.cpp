#include "tokenizador.h"
#include <iostream>
#include <list>
#include <string>

#include <chrono>
#include <iostream>
using namespace std;

///////// Comprobación de que vacíe la lista resultado

int main() {

  bool kCasosEspeciales = true, kpasarAminusculas = false;

  Tokenizador a("@.&", true, true);

  a.DelimitadoresPalabra("/ &_:/.?&-=#@");

  auto inicio = std::chrono::high_resolution_clock::now();

  a.TokenizarListaFicheros("temp.txt");

  auto fin = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> duracion = fin - inicio;
  std::cout << "Tiempo: " << duracion.count() << " segundos\n";
}
