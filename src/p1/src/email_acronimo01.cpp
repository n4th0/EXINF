#include "tokenizador.h"
#include <iostream>
#include <list>
#include <string>

using namespace std;

///////// Comprobación de que vacíe la lista resultado

void imprimirListaSTL(const list<string> &cadena) {
  list<string>::const_iterator itCadena;
  for (itCadena = cadena.begin(); itCadena != cadena.end(); itCadena++) {
    cout << (*itCadena) << ", ";
  }
  cout << endl;
}

int main(void) {
  bool kCasosEspeciales = true, kpasarAminusculas = false;

  list<string> lt1, lt2;

  Tokenizador a("# ", kCasosEspeciales, kpasarAminusculas);
  list<string> tokens;

  a.DelimitadoresPalabra("@.&");
  a.Tokenizar("pal1 cat@iuii.ua.es@cd p1 p2", tokens);
  imprimirListaSTL(tokens);

  a.DelimitadoresPalabra("&.");
  a.Tokenizar("pal1 cat@iuii.ua.es@cd p1 p2", tokens);
  imprimirListaSTL(tokens);
}
