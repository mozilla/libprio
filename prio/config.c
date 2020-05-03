/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mprio.h>
#include <stdlib.h>

#include "config.h"
#include "mparray.h"
#include "params.h"
#include "rand.h"
#include "util.h"

int
PrioConfig_maxDataFields(int precision)
{
  const int n_roots = 1 << Generator2Order;
  // TODO: Is it correct to just divide, since we use b fields per
  // b-bit integer? For this case of non-negative, non-zero, integer
  // values a and b, truncation toward zero C99 (see: ISO/IEC 9899:1999,
  // 6.5.5) discards the fractional remainder and is equivalent to
  // floor(a/b).
  return ((n_roots >> 1) - 1) / precision;
}

PrioConfig
PrioConfig_new(int n_fields,
               int prec,
               PublicKey server_a,
               PublicKey server_b,
               const unsigned char* batch_id,
               unsigned int batch_id_len)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = malloc(sizeof(*cfg));
  if (!cfg)
    return NULL;

  cfg->batch_id = NULL;
  cfg->batch_id_len = batch_id_len;
  cfg->server_a_pub = server_a;
  cfg->server_b_pub = server_b;
  cfg->num_data_fields = n_fields;
  cfg->precision = prec;
  cfg->n_roots = 1 << Generator2Order;
  MP_DIGITS(&cfg->modulus) = NULL;
  MP_DIGITS(&cfg->inv2) = NULL;
  MP_DIGITS(&cfg->generator) = NULL;

  P_CHECKCB(cfg->n_roots > 1);
  P_CHECKCB(cfg->num_data_fields <= PrioConfig_maxDataFields(prec));
  // TODO: Up to which precision is reasonable (including for fixed
  // point representation)?
  P_CHECKCB(cfg->precision <= 32);
  P_CHECKCB(cfg->precision >= 1);

  P_CHECKA(cfg->batch_id = malloc(batch_id_len));
  strncpy((char*)cfg->batch_id, (char*)batch_id, batch_id_len);

  MP_CHECKC(mp_init(&cfg->modulus));
  MP_CHECKC(mp_read_radix(&cfg->modulus, Modulus, 16));

  MP_CHECKC(mp_init(&cfg->generator));
  MP_CHECKC(mp_read_radix(&cfg->generator, Generator, 16));

  // Compute  2^{-1} modulo M
  MP_CHECKC(mp_init(&cfg->inv2));
  mp_set(&cfg->inv2, 2);
  MP_CHECKC(mp_invmod(&cfg->inv2, &cfg->modulus, &cfg->inv2));

cleanup:
  if (rv != SECSuccess) {
    PrioConfig_clear(cfg);
    return NULL;
  }

  return cfg;
}

PrioConfig
PrioConfig_newTest(int nFields, int precision)
{
  return PrioConfig_new(
    nFields, precision, NULL, NULL, (unsigned char*)"testBatch", 9);
}

void
PrioConfig_clear(PrioConfig cfg)
{
  if (!cfg)
    return;
  if (cfg->batch_id)
    free(cfg->batch_id);
  mp_clear(&cfg->modulus);
  mp_clear(&cfg->inv2);
  mp_clear(&cfg->generator);
  free(cfg);
}

int
PrioConfig_numDataFields(const_PrioConfig cfg)
{
  return cfg->num_data_fields;
}

int
PrioConfig_precDataFields(const_PrioConfig cfg)
{
  return cfg->precision;
}

SECStatus
Prio_init(void)
{
  return rand_init();
}

void
Prio_clear(void)
{
  rand_clear();
}

int
PrioConfig_hPoints(const_PrioConfig cfg)
{
  const int mul_gates = cfg->precision * cfg->num_data_fields;
  const int N = next_power_of_two(mul_gates + 1);
  return N;
}
