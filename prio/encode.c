/*
 * Copyright (c) 2020, Daniel Reusche
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mprio.h>
#include <stdlib.h>

#include "encode.h"
#include "mparray.h"
#include "util.h"

EInt
EInt_new(int prec)
{
  SECStatus rv = SECSuccess;

  EInt e_int = malloc(sizeof *e_int);
  if (!e_int)
    return NULL;

  e_int->prec = prec;

  // initialize x
  MP_DIGITS(&e_int->x) = NULL;
  MP_CHECKC(mp_init(&e_int->x));

  // create an MPArray to hold the bits
  e_int->bits = NULL;
  P_CHECKA(e_int->bits = MPArray_new(prec));

cleanup:
  if (rv != SECSuccess) {
    EInt_clear(e_int);
    return NULL;
  }

  return e_int;
}

SECStatus
EInt_set(EInt dst, long x)
{
  SECStatus rv = SECSuccess;

  int prec = dst->prec;
  // maximum for b-bit encoding (b = precision)
  long max = (1l << prec) - 1; // TODO: Compute maximum in PrioConfig?

  P_CHECKCB(max >= x); // Check wether x is can be encoded with given precision
  P_CHECKCB(0 <= x);   // Check wether x is non-negative
  // set x
  mp_set_int(&dst->x, x);

  // The following block sets b_0, ..., b_(b-1)
  // set b_0 (the first bit)
  mp_set_int(&dst->bits->data[prec - 1], (x & 1));
  // start with prec-2 to shift correctly
  for (int bit = prec - 2; bit >= 0; bit--) {
    // shift by one for subsequent bits (b_1, ..., b_(b-1))
    (x >>= 1);
    mp_set_int(&dst->bits->data[bit], (x & 1));
    // from C99 on ">>" produces leading 0s since only unsigned ints are handled
    // (see ISO/IEC 9899:1999, 6.5.7)
  }

cleanup:
  return rv;
}

void
EInt_clear(EInt e_int)
{
  if (e_int == NULL)
    return;

  if (e_int->bits != NULL)
    MPArray_clear(e_int->bits);

  mp_clear(&e_int->x);

  free(e_int);
}

EIntArray
EIntArray_new(int len, int prec)
{
  SECStatus rv = SECSuccess;
  EIntArray arr = malloc(sizeof *arr);
  if (!arr)
    return NULL;

  arr->data = NULL;
  arr->len = len;
  arr->prec = prec;

  P_CHECKA(arr->data = calloc(len, sizeof(EInt_new(prec))));
  for (int i = 0; i < len; i++) {
    P_CHECKA(arr->data[i] = EInt_new(prec));
  }

cleanup:
  if (rv != SECSuccess) {
    EIntArray_clear(arr);
    return NULL;
  }

  return arr;
};

EIntArray
EIntArray_new_data(int len, int prec, const long* data_in)
{
  EIntArray arr = EIntArray_new(len, prec);
  if (arr == NULL)
    return NULL;

  for (int i = 0; i < len; i++) {
    EInt_set(arr->data[i], data_in[i]);
  }

  return arr;
}

void
EIntArray_clear(EIntArray arr)
{
  if (arr == NULL)
    return;

  if (arr->data != NULL) {
    for (int i = 0; i < arr->len; i++) {
      EInt_clear(arr->data[i]);
    }
    free(arr->data);
  }
  free(arr);
}

SECStatus
EIntArray_truncate_and_addmod(MPArray dst, EIntArray src, const mp_int* mod)
{
  if (src->len != dst->len)
    return SECFailure;

  for (int i = 0; i < dst->len; i++) {
    MP_CHECK(mp_addmod(&dst->data[i], &src->data[i]->x, mod, &dst->data[i]));
  }

  return SECSuccess;
}

bool
EInt_areEqual(const EInt n, const EInt m)
{
  if (n->prec != m->prec)
    return false;

  if (mp_cmp(&n->x, &m->x))
    return false;

  return (MPArray_areEqual(n->bits, m->bits));
}

bool
EIntArray_areEqual(const_EIntArray arr1, const_EIntArray arr2)
{
  if (arr1->len != arr2->len)
    return false;

  for (int i = 0; i < arr1->len; i++) {
    if (!(EInt_areEqual(arr1->data[i], arr2->data[i])))
      return false;
  }

  return true;
}
