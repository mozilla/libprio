/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpi.h>

#include "mutest.h"
#include "prio/encode.h"
#include "prio/params.h"
#include "prio/prg.h"
#include "prio/util.h"
#include "test_util.h"

void
mu_test__prg_simple(void)
{
  SECStatus rv = SECSuccess;
  PrioPRGSeed key;
  PRG prg = NULL;

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(key));

cleanup:
  mu_check(rv == SECSuccess);
  PRG_clear(prg);
}

void
mu_test__prg_repeat(void)
{
  SECStatus rv = SECSuccess;
  const int buflen = 10000;
  unsigned char* buf1 = NULL;
  unsigned char* buf2 = NULL;

  PrioPRGSeed key;
  PRG prg1 = NULL;
  PRG prg2 = NULL;

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg1 = PRG_new(key));
  PT_CHECKA(prg2 = PRG_new(key));
  PT_CHECKA(buf1 = calloc(buflen, sizeof(unsigned char)));
  PT_CHECKA(buf2 = calloc(buflen, sizeof(unsigned char)));

  buf1[3] = 'a';
  buf2[3] = 'b';

  PT_CHECKC(PRG_get_bytes(prg1, buf1, buflen));
  PT_CHECKC(PRG_get_bytes(prg2, buf2, buflen));

  bool all_zero = true;
  for (int i = 0; i < buflen; i++) {
    mu_check(buf1[i] == buf2[i]);
    if (buf1[i])
      all_zero = false;
  }
  mu_check(!all_zero);

cleanup:
  mu_check(rv == SECSuccess);
  if (buf1)
    free(buf1);
  if (buf2)
    free(buf2);
  PRG_clear(prg1);
  PRG_clear(prg2);
}

void
mu_test__prg_repeat_int(void)
{
  SECStatus rv = SECSuccess;
  const int tries = 10000;
  mp_int max;
  mp_int out1;
  mp_int out2;
  MP_DIGITS(&max) = NULL;
  MP_DIGITS(&out1) = NULL;
  MP_DIGITS(&out2) = NULL;

  PrioPRGSeed key;
  PRG prg1 = NULL;
  PRG prg2 = NULL;

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg1 = PRG_new(key));
  PT_CHECKA(prg2 = PRG_new(key));

  MPT_CHECKC(mp_init(&max));
  MPT_CHECKC(mp_init(&out1));
  MPT_CHECKC(mp_init(&out2));

  for (int i = 0; i < tries; i++) {
    mp_set(&max, i + 1);
    PT_CHECKC(PRG_get_int(prg1, &out1, &max));
    PT_CHECKC(PRG_get_int(prg2, &out2, &max));
    mu_check(mp_cmp(&out1, &out2) == 0);
  }

cleanup:
  mu_check(rv == SECSuccess);
  PRG_clear(prg1);
  PRG_clear(prg2);
  mp_clear(&max);
  mp_clear(&out1);
  mp_clear(&out2);
}

void
test_prg_once(int limit)
{
  SECStatus rv = SECSuccess;
  PrioPRGSeed key;
  mp_int max;
  mp_int out;
  PRG prg = NULL;

  MP_DIGITS(&max) = NULL;
  MP_DIGITS(&out) = NULL;

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(key));

  MPT_CHECKC(mp_init(&max));
  MPT_CHECKC(mp_init(&out));

  mp_set(&max, limit);

  PT_CHECKC(PRG_get_int(prg, &out, &max));
  mu_check(mp_cmp_d(&out, limit) == -1);
  mu_check(mp_cmp_z(&out) > -1);

cleanup:
  mu_check(rv == SECSuccess);
  mp_clear(&max);
  mp_clear(&out);
  PRG_clear(prg);
}

void
mu_test_prg__multiple_of_8(void)
{
  test_prg_once(256);
  test_prg_once(256 * 256);
}

void
mu_test_prg__near_multiple_of_8(void)
{
  test_prg_once(256 + 1);
  test_prg_once(256 * 256 + 1);
}

void
mu_test_prg__odd(void)
{
  test_prg_once(39);
  test_prg_once(123);
  test_prg_once(993123);
}

void
mu_test_prg__large(void)
{
  test_prg_once(1231239933);
}

void
mu_test_prg__bit(void)
{
  test_prg_once(1);
  for (int i = 0; i < 100; i++)
    test_prg_once(2);
}

void
test_prg_distribution(int limit)
{
  int* bins = NULL;
  SECStatus rv = SECSuccess;
  PrioPRGSeed key;
  mp_int max;
  mp_int out;
  PRG prg = NULL;

  MP_DIGITS(&max) = NULL;
  MP_DIGITS(&out) = NULL;

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(key));
  PT_CHECKA(bins = calloc(limit, sizeof(int)));

  MPT_CHECKC(mp_init(&max));
  MPT_CHECKC(mp_init(&out));

  mp_set(&max, limit);

  for (int i = 0; i < limit; i++) {
    bins[i] = 0;
  }

  for (int i = 0; i < limit * limit; i++) {
    PT_CHECKC(PRG_get_int(prg, &out, &max));
    mu_check(mp_cmp_d(&out, limit) == -1);
    mu_check(mp_cmp_z(&out) > -1);

    unsigned char ival[2] = { 0x00, 0x00 };
    MPT_CHECKC(mp_to_fixlen_octets(&out, ival, 2));
    if (ival[1] + 256 * ival[0] < limit) {
      bins[ival[1] + 256 * ival[0]] += 1;
    } else {
      mu_check(false);
    }
  }

  for (int i = 0; i < limit; i++) {
    mu_check(bins[i] > limit / 2);
  }

cleanup:
  mu_check(rv == SECSuccess);
  if (bins)
    free(bins);
  mp_clear(&max);
  mp_clear(&out);
  PRG_clear(prg);
}

void
mu_test__prg_distribution123(void)
{
  test_prg_distribution(123);
}

void
mu_test__prg_distribution257(void)
{
  test_prg_distribution(257);
}

void
mu_test__prg_distribution259(void)
{
  test_prg_distribution(259);
}

void
test_prg_distribution_large(mp_int* max)
{
  const int limit = 16;
  int* bins = NULL;
  SECStatus rv = SECSuccess;
  PrioPRGSeed key;
  mp_int out;
  PRG prg = NULL;

  MP_DIGITS(&out) = NULL;

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(key));
  PT_CHECKA(bins = calloc(limit, sizeof(int)));

  MPT_CHECKC(mp_init(&out));

  for (int i = 0; i < limit; i++) {
    bins[i] = 0;
  }

  for (int i = 0; i < 100 * limit * limit; i++) {
    MPT_CHECKC(PRG_get_int(prg, &out, max));
    mu_check(mp_cmp(&out, max) == -1);
    mu_check(mp_cmp_z(&out) > -1);

    unsigned long res;
    MPT_CHECKC(mp_mod_d(&out, limit, &res));
    bins[res] += 1;
  }

  for (int i = 0; i < limit; i++) {
    mu_check(bins[i] > limit / 2);
  }

cleanup:
  mu_check(rv == SECSuccess);
  if (bins)
    free(bins);
  mp_clear(&out);
  PRG_clear(prg);
}

void
mu_test__prg_distribution_large(void)
{
  SECStatus rv = SECSuccess;
  mp_int max;
  MP_DIGITS(&max) = NULL;
  MPT_CHECKC(mp_init(&max));

  char bytes[] = "FF1230985198451798EDC8123";
  MPT_CHECKC(mp_read_radix(&max, bytes, 16));
  test_prg_distribution_large(&max);

cleanup:
  mu_check(rv == SECSuccess);
  mp_clear(&max);
}

void
mu_test__prg_share_arr(void)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = NULL;
  MPArray arr = NULL;
  MPArray arr_share = NULL;
  PRG prg = NULL;
  PrioPRGSeed seed;

  PT_CHECKA(cfg = PrioConfig_newTest(72, 1));
  PT_CHECKC(PrioPRGSeed_randomize(&seed));
  PT_CHECKA(arr = MPArray_new(10));
  PT_CHECKA(arr_share = MPArray_new(10));
  PT_CHECKA(prg = PRG_new(seed));

  for (int i = 0; i < 10; i++) {
    mp_set(&arr->data[i], i);
  }

  PT_CHECKC(PRG_share_array(prg, arr_share, arr, cfg));

  // Reset PRG
  PRG_clear(prg);
  PT_CHECKA(prg = PRG_new(seed));

  // Read pseudorandom values into arr
  PT_CHECKC(PRG_get_array(prg, arr, &cfg->modulus));

  for (int i = 0; i < 10; i++) {
    MPT_CHECKC(mp_addmod(
      &arr->data[i], &arr_share->data[i], &cfg->modulus, &arr->data[i]));
    mu_check(mp_cmp_d(&arr->data[i], i) == 0);
  }

cleanup:
  mu_check(rv == SECSuccess);

  PRG_clear(prg);
  MPArray_clear(arr);
  MPArray_clear(arr_share);
  PrioConfig_clear(cfg);
}

void
test_prg_range_once(int bot, int limit)
{
  SECStatus rv = SECSuccess;
  PrioPRGSeed key;
  mp_int lower;
  mp_int max;
  mp_int out;
  PRG prg = NULL;

  MP_DIGITS(&lower) = NULL;
  MP_DIGITS(&max) = NULL;
  MP_DIGITS(&out) = NULL;

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(key));

  MPT_CHECKC(mp_init(&max));
  MPT_CHECKC(mp_init(&out));
  MPT_CHECKC(mp_init(&lower));

  mp_set(&lower, bot);
  mp_set(&max, limit);

  for (int i = 0; i < 100; i++) {
    PT_CHECKC(PRG_get_int_range(prg, &out, &lower, &max));
    mu_check(mp_cmp_d(&out, limit) == -1);
    mu_check(mp_cmp_d(&out, bot) > -1);
    mu_check(mp_cmp_z(&out) > -1);
  }

cleanup:
  mu_check(rv == SECSuccess);
  mp_clear(&lower);
  mp_clear(&max);
  mp_clear(&out);
  PRG_clear(prg);
}

void
mu_test_prg_range__multiple_of_8(void)
{
  test_prg_range_once(128, 256);
  test_prg_range_once(256, 256 * 256);
}

void
mu_test_prg_range__near_multiple_of_8(void)
{
  test_prg_range_once(256, 256 + 1);
  test_prg_range_once(256 * 256, 256 * 256 + 1);
}

void
mu_test_prg_range__odd(void)
{
  test_prg_range_once(23, 39);
  test_prg_range_once(7, 123);
  test_prg_range_once(99000, 993123);
}

// TODO: Are the parameters for the EInt parts good like this?
void
mu_test__prg_repeat_e_int(void)
{
  SECStatus rv = SECSuccess;
  const int tries = 10;
  int precision = 32;
  mp_int max;
  MP_DIGITS(&max) = NULL;

  EInt out1 = NULL;
  EInt out2 = NULL;

  PrioPRGSeed key;
  PRG prg1 = NULL;
  PRG prg2 = NULL;

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg1 = PRG_new(key));
  PT_CHECKA(prg2 = PRG_new(key));

  MPT_CHECKC(mp_init(&max));
  PT_CHECKA(out1 = EInt_new(precision));
  PT_CHECKA(out2 = EInt_new(precision));

  for (int i = 0; i < tries; i++) {
    mp_set(&max, i + 1);
    PT_CHECKC(PRG_get_e_int(prg1, out1, &max));
    PT_CHECKC(PRG_get_e_int(prg2, out2, &max));
    mu_check(EInt_areEqual(out1, out2) == true);
  }

cleanup:
  mu_check(rv == SECSuccess);
  PRG_clear(prg1);
  PRG_clear(prg2);
  mp_clear(&max);
  EInt_clear(out1);
  EInt_clear(out2);
}

void
mu_test__prg_accum_x(void)
{
  SECStatus rv = SECSuccess;
  int precision = 32;
  PrioPRGSeed key;
  PRG prg = NULL;

  mp_int max;
  mp_int tmp;
  mp_int accum;
  EInt out = NULL;

  MP_DIGITS(&max) = NULL;
  MPT_CHECKC(mp_init(&max));
  mp_read_radix(&max, Modulus, 16);

  MP_DIGITS(&tmp) = NULL;
  MPT_CHECKC(mp_init(&tmp));

  MP_DIGITS(&accum) = NULL;
  MPT_CHECKC(mp_init(&accum));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(key));

  PT_CHECKA(out = EInt_new(precision));
  PT_CHECKC(PRG_get_e_int(prg, out, &max));

  for (int bit = 0; bit < precision; bit++) {
    // multiply share of i-th bit by 2^i
    MP_CHECKC(
      mp_mul_d(&out->bits->data[bit], (1l << (out->prec - bit - 1)), &tmp));
    // apply modulus
    MP_CHECKC(mp_mod(&tmp, &max, &tmp));
    // add result to x, to sum up to a share of x when loop is done
    MP_CHECKC(mp_addmod(&accum, &tmp, &max, &accum));
  }
  mu_check(mp_cmp(&out->x, &accum) == 0);

cleanup:
  mu_check(rv == SECSuccess);
  PRG_clear(prg);
  mp_clear(&max);
  mp_clear(&tmp);
  mp_clear(&accum);
  EInt_clear(out);
}

/*
 * Test PRG_e_int_accum_x, PRG_get_e_int, PRG_share_e_int by encoding,
 * sharing and reconstructing single longs
 */
void
mu_test__share_e_int(void)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = NULL;

  int fields = 100;
  int precision = 32;
  PrioPRGSeed key;
  PRG prg1 = NULL;
  PRG prg2 = NULL;

  long max = (1l << (precision)) - 1;
  mp_int tmp;

  EInt src = NULL;
  EInt share = NULL;
  EInt get = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(fields, precision));

  MP_DIGITS(&tmp) = NULL;
  MPT_CHECKC(mp_init(&tmp));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg1 = PRG_new(key));
  PT_CHECKA(prg2 = PRG_new(key));

  PT_CHECKA(src = EInt_new(cfg->precision));
  PT_CHECKA(share = EInt_new(cfg->precision));
  PT_CHECKA(get = EInt_new(cfg->precision));

  for (int i = 0; i < fields; i++) {
    mp_zero(&tmp);
    EInt_set(src, (max - i));
    EInt_set(share, 0);
    EInt_set(get, 0);
    PT_CHECKC(PRG_share_e_int(prg1, share, src, cfg));
    PT_CHECKC(PRG_get_e_int(prg2, get, &cfg->modulus));
    MPT_CHECKC(mp_addmod(&share->x, &get->x, &cfg->modulus, &tmp));
    MPT_CHECKC(mp_cmp(&src->x, &tmp));
  }

cleanup:
  mu_check(rv == SECSuccess);
  PRG_clear(prg1);
  PRG_clear(prg2);
  mp_clear(&tmp);
  EInt_clear(src);
  EInt_clear(share);
  EInt_clear(get);
  PrioConfig_clear(cfg);
}

/*
 * Test get_e_array, share_e_array by sharing and reconstructing arrays
 */
void
mu_test__share_e_array(void)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = NULL;

  int fields = 100;
  int precision = 32;
  PrioPRGSeed key;
  PRG prg1 = NULL;
  PRG prg2 = NULL;

  long max = (1l << (precision)) - 1;
  mp_int tmp;

  EIntArray src = NULL;
  EIntArray share = NULL;
  EIntArray get = NULL;

  long* data = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(fields, precision));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg1 = PRG_new(key));
  PT_CHECKA(prg2 = PRG_new(key));

  P_CHECKA(data = calloc(fields, sizeof(long)));
  for (int i = 0; i < fields; i++) {
    data[i] = max - i;
  }
  PT_CHECKA(src =
              EIntArray_new_data(cfg->num_data_fields, cfg->precision, data));

  PT_CHECKA(share = EIntArray_new(cfg->num_data_fields, cfg->precision));
  PT_CHECKC(PRG_share_e_array(prg1, share, src, cfg));

  PT_CHECKA(get = EIntArray_new(cfg->num_data_fields, cfg->precision));
  PT_CHECKC(PRG_get_e_array(prg2, get, &cfg->modulus));

  MP_DIGITS(&tmp) = NULL;
  MPT_CHECKC(mp_init(&tmp));

  for (int i = 0; i < fields; i++) {
    mp_zero(&tmp);
    MPT_CHECKC(
      mp_addmod(&share->data[i]->x, &get->data[i]->x, &cfg->modulus, &tmp));
    MPT_CHECKC(mp_cmp(&src->data[i]->x, &tmp));
  }

cleanup:
  mu_check(rv == SECSuccess);
  PRG_clear(prg1);
  PRG_clear(prg2);
  mp_clear(&tmp);
  EIntArray_clear(src);
  EIntArray_clear(share);
  EIntArray_clear(get);
  PrioConfig_clear(cfg);
  free(data);
}

/*
 * Same as share_e_array but reuse arrays in multiple rounds
 */
void
mu_test__share_e_arrays(void)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = NULL;

  int fields = 100;
  int precision = 32;
  int nclients = 10;

  PrioPRGSeed key;
  PRG prg1 = NULL;
  PRG prg2 = NULL;

  long max = (1l << (precision)) - 1;
  mp_int tmp;

  EIntArray src = NULL;
  EIntArray share = NULL;
  EIntArray get = NULL;

  long* data = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(fields, precision));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg1 = PRG_new(key));
  PT_CHECKA(prg2 = PRG_new(key));

  MP_DIGITS(&tmp) = NULL;
  MPT_CHECKC(mp_init(&tmp));

  P_CHECKA(data = calloc(fields, sizeof(long)));
  for (int i = 0; i < fields; i++) {
    data[i] = max - i;
  }

  PT_CHECKA(src = EIntArray_new(cfg->num_data_fields, cfg->precision));
  PT_CHECKA(share = EIntArray_new(cfg->num_data_fields, cfg->precision));
  PT_CHECKA(get = EIntArray_new(cfg->num_data_fields, cfg->precision));

  for (int client = 0; client < nclients; client++) {
    for (int i = 0; i < fields; i++) {
      PT_CHECKC(EInt_set(src->data[i], data[i]));
    }
    PT_CHECKC(PRG_share_e_array(prg1, share, src, cfg));
    PT_CHECKC(PRG_get_e_array(prg2, get, &cfg->modulus));

    for (int i = 0; i < fields; i++) {
      mp_zero(&tmp);
      MPT_CHECKC(
        mp_addmod(&share->data[i]->x, &get->data[i]->x, &cfg->modulus, &tmp));
      MPT_CHECKC(mp_cmp(&src->data[i]->x, &tmp));
    }
  }

cleanup:
  mu_check(rv == SECSuccess);
  PRG_clear(prg1);
  PRG_clear(prg2);
  mp_clear(&tmp);
  EIntArray_clear(src);
  EIntArray_clear(share);
  EIntArray_clear(get);
  PrioConfig_clear(cfg);
  free(data);
}
