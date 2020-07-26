/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpi.h>

#include "mutest.h"
#include "prio/prg.h"
#include "prio/util.h"
#include "test_util.h"

void
mu_test__prg_simple(void)
{
  SECStatus rv = SECSuccess;
  PrioPRGSeed key;
  PRG prg = NULL;
  PrioConfig cfg = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(1));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(cfg, key));

cleanup:
  mu_check(rv == SECSuccess);
  PRG_clear(prg);
  PrioConfig_clear(cfg);
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
  PrioConfig cfg = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(1));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg1 = PRG_new(cfg, key));
  PT_CHECKA(prg2 = PRG_new(cfg, key));
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
  PrioConfig_clear(cfg);
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
  PrioConfig cfg = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(1));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg1 = PRG_new(cfg, key));
  PT_CHECKA(prg2 = PRG_new(cfg, key));

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
  PrioConfig_clear(cfg);
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
  PrioConfig cfg = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(1));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(cfg, key));

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
  PrioConfig_clear(cfg);
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
  PrioConfig cfg = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(1));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(cfg, key));
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
  PrioConfig_clear(cfg);
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
  PrioConfig cfg = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(1));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(cfg, key));
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
  PrioConfig_clear(cfg);
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

  PT_CHECKA(cfg = PrioConfig_newTest(72));
  PT_CHECKC(PrioPRGSeed_randomize(&seed));
  PT_CHECKA(arr = MPArray_new(10));
  PT_CHECKA(arr_share = MPArray_new(10));
  PT_CHECKA(prg = PRG_new(cfg, seed));

  for (int i = 0; i < 10; i++) {
    mp_set(&arr->data[i], i);
  }

  PT_CHECKC(PRG_share_array(prg, arr_share, arr, cfg));

  // Reset PRG
  PRG_clear(prg);
  PT_CHECKA(prg = PRG_new(cfg, seed));

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
  PrioConfig cfg = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(1));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(cfg, key));

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
  PrioConfig_clear(cfg);
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

void
mu_test_prg_uses_batch_id(void)
{
  SECStatus rv = SECSuccess;
  PrioPRGSeed key;
  mp_int out1;
  mp_int out2;
  PRG prg1 = NULL;
  PRG prg2 = NULL;

  MP_DIGITS(&out1) = NULL;
  MP_DIGITS(&out2) = NULL;
  PrioConfig cfg1 = NULL;
  PrioConfig cfg2 = NULL;

  PT_CHECKA(cfg1 = PrioConfig_newTest(1));
  PT_CHECKA(cfg2 = PrioConfig_newTest(1));

  // Two batch IDs are different. Check that
  // PRG outputs are actually different.
  cfg2->batch_id[0] = '\0';

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg1 = PRG_new(cfg1, key));
  PT_CHECKA(prg2 = PRG_new(cfg2, key));

  MPT_CHECKC(mp_init(&out1));
  MPT_CHECKC(mp_init(&out2));

  PT_CHECKC(PRG_get_int(prg1, &out1, &cfg1->modulus));
  PT_CHECKC(PRG_get_int(prg2, &out2, &cfg1->modulus));
  mu_check(mp_cmp(&out1, &out2) != 0);

cleanup:
  mu_check(rv == SECSuccess);
  mp_clear(&out1);
  mp_clear(&out2);
  PRG_clear(prg1);
  PRG_clear(prg2);
  PrioConfig_clear(cfg1);
  PrioConfig_clear(cfg2);
}
