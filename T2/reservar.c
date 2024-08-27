#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "reservar.h"

// Defina aca las variables globales y funciones auxiliares que necesite
pthread_mutex_t mutex;
pthread_cond_t cond;
int estacionamientos[N_EST];
int disponibles = N_EST;
int ticket = 0, display = 0;

void initReservar() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    for (int i = 0; i < N_EST; i++) {
        estacionamientos[i] = 1; // Inicialmente todos los estacionamientos están disponibles
    }
}

void cleanReservar() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

int reservar(int k) {
    pthread_mutex_lock(&mutex);
    int my_ticket = ticket++;
    while (disponibles < k || my_ticket != display) {
        pthread_cond_wait(&cond, &mutex);
    }
    int i=0;
    while (i < N_EST) {
        if (estacionamientos[i]) {
            int j;
            for (j = 1; j < k && i + j < N_EST; j++) {
                if (!estacionamientos[i+j]) {  
                    i += j;
                    break;
                }
            }
            if (j == k) {
                for (int l = 0; l < k; l++) {
                    estacionamientos[i+l] = 0;
                }
                disponibles -= k;
                display++;
                pthread_cond_broadcast(&cond);
                break;
            } 
            else if (i + j == N_EST){
                pthread_cond_wait(&cond, &mutex);
                i = 0;
            } 
        } else {
            i++;
        }
    }          
    pthread_mutex_unlock(&mutex);
    return i;
}

void liberar(int e, int k) {
    pthread_mutex_lock(&mutex);
    for (int i = e; i < e + k; i++) {
        estacionamientos[i] = 1;
    }
    disponibles += k;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
  
} 
