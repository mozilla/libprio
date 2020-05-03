/*
 * Copyright (c) 2020, Daniel Reusche
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mpi.h>
#include <mprio.h>

#include "client.h"
#include "config.h"
#include "encode.h"
#include "prg.h"
#include "server.h"
#include "util.h"

SECStatus
IntCircuit_set_fg_client(const_PrioConfig cfg,
                         const_EIntArray data_in,
                         MPArray points_f,
                         MPArray points_g)
{
  SECStatus rv = SECSuccess;
  const int num_data_fields = cfg->num_data_fields;
  const int prec = cfg->precision;
  const mp_int* mod = &cfg->modulus;
  int pos;

  for (int m = 0; m < num_data_fields; m++) {
    // For each bit b_i, we compute b_i * (b_i-1)
    for (int i = 0; i < prec; i++) {
      pos = m + i;
      // f(pos) = b_i
      MP_CHECKC(
        mp_copy(&data_in->data[m]->bits->data[i], &points_f->data[pos]));
      MP_CHECKC(mp_mod(&points_f->data[pos], mod, &points_f->data[pos]));
      // g(pos) = b_i - 1
      MP_CHECKC(
        mp_sub_d(&data_in->data[m]->bits->data[i], 1, &points_g->data[pos]));
      MP_CHECKC(mp_mod(&points_g->data[pos], mod, &points_g->data[pos]));
    }
  }

cleanup:
  return rv;
}

SECStatus
IntCircuit_set_fg_server(PrioVerifier v,
                         const_EIntArray shares_in,
                         MPArray points_f,
                         MPArray points_g)
{
  SECStatus rv = SECSuccess;
  const int num_data_fields = v->s->cfg->num_data_fields;
  const int prec = v->s->cfg->precision;
  const mp_int* mod = &v->s->cfg->modulus;
  int pos;

  for (int m = 0; m < num_data_fields; m++) {
    // For each bit b_i, we compute b_i * (b_i-1)
    for (int i = 0; i < prec; i++) {
      // We offset pos by 1, since on the server f(0) and g(0) are already in
      // place
      pos = m + i + 1;
      // [f](pos) = i-th data share
      MP_CHECKC(
        mp_copy(&shares_in->data[m]->bits->data[i], &points_f->data[pos]));
      MP_CHECKC(mp_mod(&points_f->data[pos], mod, &points_f->data[pos]));
      // [g](pos) = i-th data share minus 1
      // Since subtraction is not distributive, we only do this on server 0
      if (!v->s->idx) {
        MP_CHECKC(mp_sub_d(
          &shares_in->data[m]->bits->data[i], 1, &points_g->data[pos]));
        MP_CHECKC(mp_mod(&points_g->data[pos], mod, &points_g->data[pos]));
      }
      // On server 1, we just copy the shares
      if (v->s->idx) {
        MP_CHECKC(
          mp_copy(&shares_in->data[m]->bits->data[i], &points_g->data[pos]));
      }
    }
  }

cleanup:
  return rv;
}

SECStatus
IntCircuit_check_x_accum(const_PrioConfig cfg, const_EIntArray shares_in)
{
  SECStatus rv = SECSuccess;
  const int num_data_fields = cfg->num_data_fields;
  const int prec = cfg->precision;
  const mp_int* mod = &cfg->modulus;

  mp_int tmp;
  mp_int accum;
  MP_DIGITS(&tmp) = NULL;
  MP_CHECKC(mp_init(&tmp));
  MP_DIGITS(&accum) = NULL;
  MP_CHECKC(mp_init(&accum));

  for (int m = 0; m < num_data_fields; m++) {
    mp_zero(&tmp);
    mp_zero(&accum);
    for (int i = 0; i < prec; i++) {
      // count exponents down because of big-endianness
      MP_CHECKC(mp_mul_d(
        &shares_in->data[m]->bits->data[i], (1l << (prec - i - 1)), &tmp));
      MP_CHECKC(mp_addmod(&tmp, &accum, mod, &accum));
    }
    MP_CHECKC(mp_cmp(&shares_in->data[m]->x, &accum));
  }

cleanup:
  mp_clear(&tmp);
  mp_clear(&accum);
  return rv;
}
