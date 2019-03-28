/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpi.h>
#include <mprio.h>
#include <stdio.h>

#include "mutest.h"
#include "prio/config.h"
#include "prio/mparray.h"
#include "prio/poly.h"
#include "prio/util.h"
#include "test_util.h"

void
mu_test__fft_one(void)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = NULL;
  MPArray points_in = NULL;
  MPArray points_out = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(123));
  PT_CHECKA(points_in = MPArray_new(1));
  PT_CHECKA(points_out = MPArray_new(1));

  mp_set(&points_in->data[0], 3);
  mu_check(poly_fft(points_out, points_in, cfg, false) == SECSuccess);

  mu_check(mp_cmp_d(&points_in->data[0], 3) == 0);
  mu_check(mp_cmp_d(&points_out->data[0], 3) == 0);

cleanup:
  mu_check(rv == SECSuccess);
  MPArray_clear(points_in);
  MPArray_clear(points_out);

  PrioConfig_clear(cfg);
}

void
mu_test__fft_roots(void)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = NULL;
  MPArray roots = NULL;
  mp_int tmp;
  MP_DIGITS(&tmp) = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(90));
  PT_CHECKA(roots = MPArray_new(4));
  MPT_CHECKC(mp_init(&tmp));

  poly_fft_get_roots(roots, 4, cfg, false);

  for (int i = 0; i < 4; i++) {
    mp_exptmod_d(&roots->data[i], 4, &cfg->modulus, &tmp);
    mu_check(mp_cmp_d(&tmp, 1) == 0);
  }

cleanup:
  mu_check(rv == SECSuccess);
  mp_clear(&tmp);
  MPArray_clear(roots);
  PrioConfig_clear(cfg);
}

void
mu_test__fft_simple(void)
{
  SECStatus rv = SECSuccess;
  const int nPoints = 4;

  PrioConfig cfg = NULL;
  MPArray points_in = NULL;
  MPArray points_out = NULL;
  MPArray roots = NULL;

  mp_int should_be, tmp;
  MP_DIGITS(&should_be) = NULL;
  MP_DIGITS(&tmp) = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(140));
  PT_CHECKA(points_in = MPArray_new(nPoints));
  PT_CHECKA(points_out = MPArray_new(nPoints));
  PT_CHECKA(roots = MPArray_new(nPoints));

  MPT_CHECKC(mp_init(&should_be));
  MPT_CHECKC(mp_init(&tmp));

  poly_fft_get_roots(roots, nPoints, cfg, false);

  mp_set(&points_in->data[0], 3);
  mp_set(&points_in->data[1], 8);
  mp_set(&points_in->data[2], 7);
  mp_set(&points_in->data[3], 9);
  mu_check(poly_fft(points_out, points_in, cfg, false) == SECSuccess);

  for (int i = 0; i < nPoints; i++) {
    mp_set(&should_be, 0);
    for (int j = 0; j < nPoints; j++) {
      mu_check(mp_exptmod_d(&roots->data[i], j, &cfg->modulus, &tmp) ==
               MP_OKAY);
      mu_check(mp_mulmod(&tmp, &points_in->data[j], &cfg->modulus, &tmp) ==
               MP_OKAY);
      mu_check(mp_addmod(&should_be, &tmp, &cfg->modulus, &should_be) ==
               MP_OKAY);
    }

    /*
    puts("Should be:");
    mp_print(&should_be, stdout);
    puts("");
    mp_print(&points_out[i], stdout);
    puts("");
    */
    mu_check(mp_cmp(&should_be, &points_out->data[i]) == 0);
  }

cleanup:
  mu_check(rv == SECSuccess);
  MPArray_clear(roots);
  mp_clear(&tmp);
  mp_clear(&should_be);
  MPArray_clear(points_in);
  MPArray_clear(points_out);
  PrioConfig_clear(cfg);
}

void
mu_test__fft_invert(void)
{
  SECStatus rv = SECSuccess;
  const int nPoints = 8;

  PrioConfig cfg = NULL;
  MPArray points_in = NULL;
  MPArray points_out = NULL;
  MPArray points_out2 = NULL;
  MPArray roots = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(91));
  PT_CHECKA(points_in = MPArray_new(nPoints));
  PT_CHECKA(points_out = MPArray_new(nPoints));
  PT_CHECKA(points_out2 = MPArray_new(nPoints));
  PT_CHECKA(roots = MPArray_new(nPoints));

  poly_fft_get_roots(roots, nPoints, cfg, false);

  mp_set(&points_in->data[0], 3);
  mp_set(&points_in->data[1], 8);
  mp_set(&points_in->data[2], 7);
  mp_set(&points_in->data[3], 9);
  mp_set(&points_in->data[4], 123);
  mp_set(&points_in->data[5], 123123987);
  mp_set(&points_in->data[6], 2);
  mp_set(&points_in->data[7], 0);
  mu_check(poly_fft(points_out, points_in, cfg, false) == SECSuccess);
  mu_check(poly_fft(points_out2, points_out, cfg, true) == SECSuccess);

  for (int i = 0; i < nPoints; i++) {
    mu_check(mp_cmp(&points_out2->data[i], &points_in->data[i]) == 0);
  }

cleanup:
  mu_check(rv == SECSuccess);

  MPArray_clear(roots);
  MPArray_clear(points_in);
  MPArray_clear(points_out);
  MPArray_clear(points_out2);
  PrioConfig_clear(cfg);
}
