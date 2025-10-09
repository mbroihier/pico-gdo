#ifndef INCLUDE_LOCK_H_
#define INCLUDE_LOCK_H_
/*
 *      lock.h - header for lock object
 *
 *      Copyright (C) 2020
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <cstdio>
/* ---------------------------------------------------------------------- */
class lock {
 public:
  static const int MAX_BYTES = 12;   // space + 1 for a simple key
  static const int KEY_MODULO = 0x7;  // only allow of keys of 6+5 bytes
 private:
  int slope;
  int offset;
  int modulo;
  bool locked;
  char aRealKey[MAX_BYTES];
  int aRealKeyLength;

 public:
  void make(char * bytes);
  bool check_another_key(char * bytes);
  int getARealKey(char ** bytes);
  lock(int * lockParameters);
  bool isLocked() { return locked; };
  ~lock();
};

#endif  // INCLUDE_LOCK_H_
