/*
 * Copyright (c) 2020, Daniel Reusche
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __ENCODE_H__
#define __ENCODE_H__

#include "mparray.h"

struct encoded_integer
{
  int prec;
  mp_int x;
  MPArray bits;
};

typedef struct encoded_integer* EInt;

struct encoded_integer_array
{
  int len;
  int prec;
  EInt* data;
};

typedef struct encoded_integer_array* EIntArray;
typedef const struct encoded_integer_array* const_EIntArray;

/*
 * Initialize an integer encoding of the given precision
 */
EInt
EInt_new(int prec);
void
EInt_clear(EInt e_int);

/*
 * Write encoding for b-bit, long x to dst, with the following
 * encoding:
 * Enc(x) = (x, bit_0, bit_1, ..., bit_(b-1)), Big-endian
 * b must be at most dst->prec
 */
SECStatus
EInt_set(EInt dst, long x);

/*
 * Initialize an array of the given length, with `EInt`s of the given
 * precision.
 */
EIntArray
EIntArray_new(int len, int prec);
void
EIntArray_clear(EIntArray arr);

/*
 * Initialize array with unsigned int data given in data_in. Elements
 * of data in must be encodeable with given precision.
 */
EIntArray
EIntArray_new_data(int len, int prec, const long* data_in);

/*
 * Modular addition of x from src EInts to entries of dst.
 */
SECStatus
EIntArray_truncate_and_addmod(MPArray dst, EIntArray src, const mp_int* mod);

/*
 * Return true iff the two encodings are equal.
 * This comparison is NOT constant time.
 */
bool
EInt_areEqual(const EInt arr1, const EInt arr2);

/*
 * Return true iff the two arrays are equal in length, precision and
 * contents. This comparison is NOT constant time.
 */
bool
EIntArray_areEqual(const_EIntArray arr1, const_EIntArray arr2);

#endif /* __ENCODE_H__ */
