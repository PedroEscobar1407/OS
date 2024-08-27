// Plantilla para maleta.c

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>
#include "maleta.h"

// Defina aca las estructuras y funciones adicionales que necesite
// ...
typedef struct{
    double* w;
    double* v;
    int* z;
    int n;
    double maxW;
    int k;
    double res;
} Args;


void *thread(void *p){
  Args *args = (Args *)p;
  int* z = malloc(args->n * sizeof(int));
  double res = llenarMaletaSec(args->w, args->v, z, args->n, args->maxW, args->k/8);
  args->res = res;
  for(int j = 0; j < args->n; j++){
    args->z[j] = z[j];
  }
  free(z);
  return NULL;
}

double llenarMaletaPar(double w[], double v[], int z[], int n,
                       double maxW, int k) {
  pthread_t pid[8];
  Args args[8];
  double best = -1;

  for (int i = 0; i < 8; i++){
    args[i].w = w;
    args[i].v = v;
    args[i].z = malloc(n * sizeof(int));
    args[i].n = n;
    args[i].maxW = maxW;
    args[i].k = k;
    pthread_create(&pid[i], NULL, thread, &args[i]);
  }

  for (int i = 0; i < 8; i++){
    pthread_join(pid[i], NULL);
    if (args[i].res > best){
      best = args[i].res;
      for (int j = 0; j < n; j++){
        z[j] = args[i].z[j];
      }
    }
    free(args[i].z);
  }

  return best;
}
