/*
 *      lock - a virtual lock that requires a key to open it
 *
 *      Copyright (C) 2025
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
#include "lock.h"
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      make.cc - make a key with an input seed
 *
 *      Copyright (C) 2025
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
void lock::make(char * bytes) {
  aRealKey[0] = bytes[0];
  aRealKey[1] = bytes[1];
  aRealKey[2] = bytes[2];
  aRealKey[3] = bytes[3];
  int seed = ((aRealKey[0] << 24) + ((aRealKey[1] << 16) & 0xff0000) +
          ((aRealKey[2] << 8) & 0xff00) + (aRealKey[3] & 0xff));
  seed = seed % modulo;
  int size = seed % KEY_MODULO + 5;
  int index = 4;
  while (index < size) {
    seed = (seed * slope + offset) % modulo;
    aRealKey[index] = seed & 0xff;
    index++;
  }
  aRealKeyLength = size;
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      check_another_key.cc - check that a key works with this lock
 *
 *      Copyright (C) 2025
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
bool lock::check_another_key(char * bytes) {
  aRealKey[0] = bytes[0];
  aRealKey[1] = bytes[1];
  aRealKey[2] = bytes[2];
  aRealKey[3] = bytes[3];
  int seed = ((aRealKey[0] << 24) + ((aRealKey[1] << 16) & 0xff0000) +
          ((aRealKey[2] << 8) & 0xff00) + (aRealKey[3] & 0xff));
  seed = seed % modulo;
  int size = seed % KEY_MODULO + 5;
  int index = 4;
  while (index < size) {
    seed = (seed * slope + offset) % modulo;
    if (aRealKey[index] != seed & 0xff) {
      locked = true;
      return false;
    }
    index++;
  }
  aRealKeyLength = size;
  locked = false;
  return true;
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      getARealKey.cc - return a real key
 *
 *      Copyright (C) 2025 
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
int lock::getARealKey(char ** bytes) {
  *bytes = aRealKey;
  return aRealKeyLength;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      lock - constructor
 *
 *      Copyright (C) 2025
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
lock::lock(int * lockParameters) {
  slope = *lockParameters++;
  offset = *lockParameters++;
  modulo = *lockParameters++;
  memset(aRealKey, 0, sizeof(aRealKey));
  aRealKeyLength = 0;
}
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
/*
 *      lock - destructor
 *
 *      Copyright (C) 2025 
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
lock::~lock() {
}
/* ---------------------------------------------------------------------- */

