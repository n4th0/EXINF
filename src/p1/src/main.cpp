#include "tokenizador.h"
#include <iostream>
#include <list>
#include <string>
#include <sys/resource.h>

using namespace std;

double getcputime(void) {
  struct timeval tim;
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  tim = ru.ru_utime;
  double t = (double)tim.tv_sec + (double)tim.tv_usec / 1000000.0;
  tim = ru.ru_stime;
  t += (double)tim.tv_sec + (double)tim.tv_usec / 1000000.0;
  return t;
}

int main() {

  long double aa = getcputime();
  Tokenizador a("\t ,;:.-+/*_`'{}[]()!?&#\"\\<>", true, true);
  a.TokenizarListaFicheros("./test/listaFicheros.txt"); // TODO EL CORPUS
  cout << "Ha tardado " << getcputime() - aa << " segundos" << endl;

  return 0;
}
