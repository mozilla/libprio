/*
 * Copyright (c) 2020, Daniel Reusche
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpi.h>
#include <mprio.h>

#include "mutest.h"
#include "prio/config.h"
#include "prio/encode.h"
#include "prio/util.h"
#include "test_util.h"

void
gen_client_data(int fields, int prec, long* data, int tweak)
{
  long max = (1l << (prec)) - 1;

  // Produce valid values (fields < max)
  if (tweak == 0) {
    for (int i = 0; i < fields; i++) {
      data[i] = max - i;
    }
  }

  // Produce values which are too big
  if (tweak == 1) {
    for (int i = 0; i < fields; i++) {
      data[i] = max + i;
    }
  }

  // Produce negative values
  if (tweak == 2) {
    for (int i = 0; i < fields; i++) {
      data[i] = 0 - i;
    }
  }

  // Produce another ser of valid values (fields < max)
  if (tweak == 3) {
    for (int i = 0; i < fields; i++) {
      data[i] = max - i - 1;
    }
  }
}

// Compute x from bits
SECStatus
check_bits(EInt in)
{
  SECStatus rv = SECSuccess;
  mp_int tmp;
  mp_int accum;
  MP_DIGITS(&tmp) = NULL;
  MPT_CHECKC(mp_init(&tmp));
  MP_DIGITS(&accum) = NULL;
  MPT_CHECKC(mp_init(&accum));

  for (int b = 0; b < in->prec; b++) {
    // count exponents down because of Big-endianness
    MPT_CHECKC(mp_mul_d(&in->bits->data[b], (1l << (in->prec - b - 1)), &tmp));
    MPT_CHECKC(mp_add(&tmp, &accum, &accum));
  }
  MPT_CHECKC(mp_cmp(&in->x, &accum)); // MP_EQ == MP_OKAY

cleanup:
  mp_clear(&tmp);
  mp_clear(&accum);
  return rv;
}

SECStatus
check_encoding(EInt enc, long in)
{
  SECStatus rv = SECSuccess;
  mp_int tmp;

  MP_DIGITS(&tmp) = NULL;
  MP_CHECKC(mp_init(&tmp));
  mp_set_int(&tmp, in);

  // Check wether x is the correct integer
  MP_CHECKC(mp_cmp(&enc->x, &tmp));

  // Check wether the bits represent x correctly
  P_CHECKC(check_bits(enc));

cleanup:
  mp_clear(&tmp);
  return rv;
}

void
test_encoding(int tweak)
{
  SECStatus rv = SECSuccess;
  int fields = 10;
  int prec = 32;

  long* data = NULL;
  EInt enc = NULL;

  P_CHECKA(data = calloc(fields, sizeof(long)));
  gen_client_data(fields, prec, data, tweak);

  for (int i = 0; i < fields; i++) {
    PT_CHECKA(enc = EInt_new(prec));
    MP_CHECKC(EInt_set(enc, data[i]));
    P_CHECKC(check_encoding(enc, data[i]));
    EInt_clear(enc);
  }

cleanup:
  if (tweak == 0) {
    mu_check(rv == SECSuccess);
  } else if (tweak == 1) {
    mu_check(rv == SECFailure);
  } else if (tweak == 1) {
    mu_check(rv == SECFailure);
  }
  if (data)
    free(data);
  if (rv != SECSuccess)
    EInt_clear(enc);
}

void
test_e_int_array(int tweak)
{
  SECStatus rv = SECSuccess;
  int fields = 10;
  unsigned int prec = 32;

  long* data = NULL;
  EIntArray arr = NULL;

  P_CHECKA(data = calloc(fields, sizeof(long)));
  gen_client_data(fields, prec, data, tweak);

  P_CHECKA(arr = EIntArray_new_data(fields, prec, data));

  for (int i = 0; i < fields; i++) {
    P_CHECKC(check_encoding(arr->data[i], data[i]));
  }

cleanup:
  if (tweak == 0) {
    mu_check(rv == SECSuccess);
  } else if (tweak == 1) {
    mu_check(rv == SECFailure);
  } else if (tweak == 1) {
    mu_check(rv == SECFailure);
  }
  if (data)
    free(data);
  EIntArray_clear(arr);
}

void
test_e_int_array_eq(int tweak)
{
  SECStatus rv = SECSuccess;
  int fields = 10;
  unsigned int prec = 32;

  long* data1 = NULL;
  EIntArray arr1 = NULL;

  long* data2 = NULL;
  EIntArray arr2 = NULL;

  P_CHECKA(data1 = calloc(fields, sizeof(long)));
  gen_client_data(fields, prec, data1, 0);
  P_CHECKA(arr1 = EIntArray_new_data(fields, prec, data1));

  if (tweak == 0) {
    P_CHECKA(data2 = calloc(fields, sizeof(long)));
    gen_client_data(fields, prec, data2, 0);
    P_CHECKA(arr2 = EIntArray_new_data(fields, prec, data2));
  }
  if (tweak == 1) {
    P_CHECKA(data2 = calloc(fields, sizeof(long)));
    gen_client_data(fields, prec, data2, 3);
    P_CHECKA(arr2 = EIntArray_new_data(fields, prec, data2));
  }

  P_CHECKCB(EIntArray_areEqual(arr1, arr2));

cleanup:
  if (tweak == 0) {
    mu_check(rv == SECSuccess);
  } else if (tweak == 1) {
    mu_check(rv == SECFailure);
  }
  if (data1)
    free(data1);
  if (data2)
    free(data2);

  EIntArray_clear(arr1);
  EIntArray_clear(arr2);
}

// Encode values of up to given precision
void
mu_test_encode__valid_0(void)
{
  test_encoding(0);
}

void
mu_test_e_int_array__valid_0(void)
{
  test_e_int_array(0);
}

// Fail on positive values which require more precision
void
mu_test_encode__invalid_1(void)
{
  test_encoding(1);
}

void
mu_test_e_int_array__invalid_1(void)
{
  test_e_int_array(1);
}

// Fail on negative values
void
mu_test_encode__invalid_2(void)
{
  test_encoding(2);
}

void
mu_test_e_int_array__invalid_2(void)
{
  test_e_int_array(2);
}

// Test equal arrays
void
mu_test_e_int_array__eq(void)
{
  test_e_int_array_eq(0);
}

// Test inequal arrays
void
mu_test_e_int_array__neq(void)
{
  test_e_int_array_eq(1);
}
