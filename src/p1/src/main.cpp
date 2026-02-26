#include "tokenizador.h"
#include <fstream>
#include <iostream>
#include <list>
#include <string>

#include <chrono>
#include <iostream>
using namespace std;

int main() {

  {

    double avg = 0;
    for (int i = 0; i < 30; i++) {

      bool kCasosEspeciales = false, kpasarAminusculas = false;

      Tokenizador a;
      a.CasosEspeciales(kCasosEspeciales);
      a.PasarAminuscSinAcentos(kpasarAminusculas);

      a.DelimitadoresPalabra(",;:.-+*\\ '\"{}[]()<>¡!¿?&#=\t@");
      // a.DelimitadoresPalabra(",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@");
      // a.DelimitadoresPalabra("/ &_:/.?&-=#@");

      auto inicio = std::chrono::high_resolution_clock::now();

      // a.TokenizarListaFicheros("./temp.txt");
      a.TokenizarListaFicheros("./test/listaFicheros.txt");
      auto fin = std::chrono::high_resolution_clock::now();

      std::chrono::duration<double> duracion = fin - inicio;

      avg += duracion.count();
      // std::cout << "Tiempo: " << duracion.count() << " segundos\n";
    }
    cout << avg / 30 << endl;
  }
}
