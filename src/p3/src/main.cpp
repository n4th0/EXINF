#include "../include/buscador.h"
#include <cstdlib>
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
  IndexadorHash b("./StopWordsEspanyol.txt", ". ,:", false, true,
                  "./indicePruebaEspanyol", 1, false);
  b.Indexar("ficherosTimes.txt");
  b.GuardarIndexacion();

  Buscador a("./indicePruebaEspanyol", 1);
  a.IndexarPregunta("KENNEDY ADMINISTRATION PRESSURE ON NGO DINH DIEM TO STOP "
                    "SUPPRESSING THE BUDDHISTS . ");
  double aa = getcputime();
  a.Buscar(423);
  a.ImprimirResultadoBusqueda(423);
  double bb = getcputime() - aa;
  cout << "\nHa tardado " << bb << " segundos\n\n";
  time_t inicioB, finB;
  time(&inicioB);
  double aaB = getcputime();
  a.Buscar("/home/n4th0/EXINF/src/p3/CorpusTime/Preguntas/", 423, 1, 83);
  a.ImprimirResultadoBusqueda(423);
  // a.ImprimirResultadoBusqueda(423, "fich_salida_buscador_alumno.txt");
  double bbB = getcputime() - aaB;
  system("rm CorpusTime/Documentos/*.tk");

  cout << "\nHa tardado " << bbB << " segundos\n\n";
}
