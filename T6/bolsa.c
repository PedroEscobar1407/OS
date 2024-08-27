#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "pss.h"
#include "bolsa.h"
#include "spinlocks.h"

// Declare aca sus variables globales
typedef enum { REFUSED, WINNER,  WAIT } States;
int sl = OPEN;
int lowPrice = 999999999; //precio mas bajo
char *s = NULL; //seller
char *b = NULL; //buyer
int *sLock = NULL; //seller lock
States *mState = NULL; //main state

int vendo(int precio, char *vendedor, char *comprador) {
    spinLock(&sl);
    if (precio < lowPrice) {
        States aState; // aux state
        
        if (mState != NULL) {
            *mState = REFUSED;
            spinUnlock(sLock);
            mState = NULL;
        }

        if (mState == NULL) {
            lowPrice = precio;
            s = vendedor;
            b = comprador;
            aState = WAIT;
            mState = &aState;
        }

        int lock = CLOSED;
        sLock = &lock;
        spinUnlock(&sl);
        spinLock(&lock);
        return aState;

    }
    spinUnlock(&sl);
    return REFUSED;
}

int compro(char *comprador, char *vendedor) {
  spinLock(&sl);
  
  if(mState == NULL){
    spinUnlock(&sl);
    return REFUSED;
  }
  else{
    strcpy(vendedor, s);
    strcpy(b, comprador);
    int res = lowPrice;
    *mState = WINNER;
    lowPrice = 999999999;
    spinUnlock(sLock);
    
    s = NULL;
    b = NULL;
    mState = NULL;

    spinUnlock(&sl);
    return res;
  }
}
