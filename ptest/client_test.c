/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mprio.h>

#include "mutest.h"
#include "prio/client.c"
#include "prio/client.h"
#include "prio/server.h"
#include "prio/util.h"
#include "test_util.h"

void
mu_test_client__new(void)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = NULL;
  PrioPacketClient pA = NULL;
  PrioPacketClient pB = NULL;
  bool* data_items = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(23));
  PT_CHECKA(pA = PrioPacketClient_new(cfg, PRIO_SERVER_A));
  PT_CHECKA(pB = PrioPacketClient_new(cfg, PRIO_SERVER_B));

  const int ndata = PrioConfig_numDataFields(cfg);
  PT_CHECKA(data_items = calloc(ndata, sizeof(bool)));

  for (int i = 0; i < ndata; i++) {
    // Arbitrary data
    data_items[i] = (i % 3 == 1) || (i % 5 == 3);
  }

  PT_CHECKC(PrioPacketClient_set_data(cfg, data_items, pA, pB));

cleanup:
  mu_check(rv == SECSuccess);
  if (data_items)
    free(data_items);

  PrioPacketClient_clear(pA);
  PrioPacketClient_clear(pB);
  PrioConfig_clear(cfg);
}

void
test_client_agg(int nclients, int nfields, bool config_is_okay)
{
  SECStatus rv = SECSuccess;
  PublicKey pkA = NULL;
  PublicKey pkB = NULL;
  PrivateKey skA = NULL;
  PrivateKey skB = NULL;
  PrioConfig cfg = NULL;
  PrioServer sA = NULL;
  PrioServer sB = NULL;
  PrioTotalShare tA = NULL;
  PrioTotalShare tB = NULL;
  PrioVerifier vA = NULL;
  PrioVerifier vB = NULL;
  unsigned char* for_a = NULL;
  unsigned char* for_b = NULL;
  const unsigned char* batch_id = (unsigned char*)"test_batch";
  unsigned int batch_id_len = strlen((char*)batch_id);
  bool* data_items = NULL;
  unsigned long long* output = NULL;

  PrioPRGSeed seed;
  PT_CHECKC(PrioPRGSeed_randomize(&seed));

  PT_CHECKC(Keypair_new(&skA, &pkA));
  PT_CHECKC(Keypair_new(&skB, &pkB));
  P_CHECKA(cfg = PrioConfig_new(nfields, pkA, pkB, batch_id, batch_id_len));
  if (!config_is_okay) {
    PT_CHECKCB(
      (PrioConfig_new(nfields, pkA, pkB, batch_id, batch_id_len) == NULL));
  }
  PT_CHECKA(sA = PrioServer_new(cfg, 0, skA, seed));
  PT_CHECKA(sB = PrioServer_new(cfg, 1, skB, seed));
  PT_CHECKA(tA = PrioTotalShare_new());
  PT_CHECKA(tB = PrioTotalShare_new());
  PT_CHECKA(vA = PrioVerifier_new(sA));
  PT_CHECKA(vB = PrioVerifier_new(sB));

  const int ndata = PrioConfig_numDataFields(cfg);

  PT_CHECKA(data_items = calloc(ndata, sizeof(bool)));
  for (int i = 0; i < ndata; i++) {
    // Arbitrary data
    data_items[i] = (i % 3 == 1) || (i % 5 == 3);
  }

  for (int i = 0; i < nclients; i++) {
    unsigned int aLen, bLen;
    PT_CHECKC(PrioClient_encode(cfg, data_items, &for_a, &aLen, &for_b, &bLen));

    PT_CHECKC(PrioVerifier_set_data(vA, for_a, aLen));
    PT_CHECKC(PrioVerifier_set_data(vB, for_b, bLen));

    mu_check(PrioServer_aggregate(sA, vA) == SECSuccess);
    mu_check(PrioServer_aggregate(sB, vB) == SECSuccess);

    free(for_a);
    free(for_b);

    for_a = NULL;
    for_b = NULL;
  }

  mu_check(PrioTotalShare_set_data(tA, sA) == SECSuccess);
  mu_check(PrioTotalShare_set_data(tB, sB) == SECSuccess);

  PT_CHECKA(output = calloc(ndata, sizeof(unsigned long)));
  mu_check(PrioTotalShare_final(cfg, output, tA, tB) == SECSuccess);
  for (int i = 0; i < ndata; i++) {
    unsigned long v = ((i % 3 == 1) || (i % 5 == 3));
    mu_check(output[i] == v * nclients);
  }

cleanup:
  if (config_is_okay) {
    mu_check(rv == SECSuccess);
  } else {
    mu_check(rv == SECFailure);
  }
  if (data_items)
    free(data_items);
  if (output)
    free(output);
  if (for_a)
    free(for_a);
  if (for_b)
    free(for_b);

  PublicKey_clear(pkA);
  PublicKey_clear(pkB);
  PrivateKey_clear(skA);
  PrivateKey_clear(skB);

  PrioVerifier_clear(vA);
  PrioVerifier_clear(vB);

  PrioTotalShare_clear(tA);
  PrioTotalShare_clear(tB);

  PrioServer_clear(sA);
  PrioServer_clear(sB);
  PrioConfig_clear(cfg);
}

void
mu_test_client__agg_1(void)
{
  test_client_agg(1, 133, true);
}

void
mu_test_client__agg_2(void)
{
  test_client_agg(2, 133, true);
}

void
mu_test_client__agg_10(void)
{
  test_client_agg(10, 133, true);
}

void
mu_test_client__agg_max(void)
{
  int max = MIN(PrioConfig_maxDataFields(), 4000);
  test_client_agg(10, max, true);
}

void
mu_test_client__agg_max_bad(void)
{
  int max = PrioConfig_maxDataFields();
  test_client_agg(10, max + 1, false);
}

void
gen_uint_data(int nUInts, int prec, long* data, int tweak)
{
  long max = (1l << (prec)) - 1;

  // Produce valid values (nUInts < max)
  if (tweak == 0) {
    for (int i = 0; i < nUInts; i++) {
      data[i] = max - i;
    }
  }

  // Produce values which are too big
  if (tweak == 1) {
    for (int i = 0; i < nUInts; i++) {
      data[i] = max + i + 1;
    }
  }

  // Produce negative values
  if (tweak == 2) {
    for (int i = 0; i < nUInts; i++) {
      data[i] = 0 - i - 1;
    }
  }

  // Produce another set of valid values (nUInts < max - 1)
  if (tweak == 3) {
    for (int i = 0; i < nUInts; i++) {
      data[i] = max - i - 1;
    }
  }
}

// Check whether integers get correctly encoded as bits
SECStatus
check_bits(long* uint_data, bool* bool_data, int prec, int nUInts)
{
  SECStatus rv = SECSuccess;
  mp_int cur_uint;
  mp_int cur_bit;
  mp_int tmp;
  mp_int accum;
  MP_DIGITS(&cur_uint) = NULL;
  MPT_CHECKC(mp_init(&cur_uint));
  MP_DIGITS(&cur_bit) = NULL;
  MPT_CHECKC(mp_init(&cur_bit));
  MP_DIGITS(&tmp) = NULL;
  MPT_CHECKC(mp_init(&tmp));
  MP_DIGITS(&accum) = NULL;
  MPT_CHECKC(mp_init(&accum));

  int offset;

  for (int i = 0; i < nUInts; i++) {
    offset = prec * i;
    mp_set_int(&cur_uint, uint_data[i]);
    mp_zero(&accum);

    for (int b = 0; b < prec; b++) {
      // count exponents down because of big-endianness
      mp_set_int(&cur_bit, bool_data[offset + b]);
      MPT_CHECKC(mp_mul_d(&cur_bit, (1l << (prec - b - 1)), &tmp));
      MPT_CHECKC(mp_add(&tmp, &accum, &accum));
    }
    MPT_CHECKC(mp_cmp(&cur_uint, &accum)); // MP_EQ == MP_OKAY
  }

cleanup:
  mp_clear(&cur_uint);
  mp_clear(&cur_bit);
  mp_clear(&tmp);
  mp_clear(&accum);
  return rv;
}

void
test_long_to_bool(int nLongs, int prec, int tweak)
{
  SECStatus rv = SECSuccess;

  long* long_data = NULL;
  bool* bool_data = NULL;

  PT_CHECKA(long_data = calloc(nLongs, sizeof(long)));
  PT_CHECKA(bool_data = calloc(nLongs * prec, sizeof(bool)));
  gen_uint_data(nLongs, prec, long_data, tweak);

  for (int i = 0; i < nLongs; i++) {
    P_CHECKC(long_to_bool(bool_data, long_data[i], prec, i));
  }

  P_CHECKC(check_bits(long_data, bool_data, prec, nLongs));

cleanup:
  if (tweak == 0) {
    mu_check(rv == SECSuccess);
  } else if (tweak == 1) {
    mu_check(rv == SECFailure);
  } else if (tweak == 2) {
    mu_check(rv == SECFailure);
  } else if (tweak == 3) {
    mu_check(rv == SECSuccess);
  }

  if (long_data)
    free(long_data);
  if (bool_data)
    free(bool_data);
}

// Encode values of up to given precision
void
mu_test_encode__valid_1_0(void)
{
  test_long_to_bool(1, 32, 0);
}

void
mu_test_encode__valid_100_0(void)
{
  test_long_to_bool(100, 32, 0);
}

// Encode values of up to given precision - 1
void
mu_test_encode__valid_1_1(void)
{
  test_long_to_bool(1, 32, 3);
}

void
mu_test_encode__valid_100_1(void)
{
  test_long_to_bool(100, 32, 3);
}

// Fail on positive values which require more precision
void
mu_test_encode__invalid_1_0(void)
{
  test_long_to_bool(1, 32, 1);
}

void
mu_test_encode__invalid_100_0(void)
{
  test_long_to_bool(100, 32, 1);
}

// Fail on negative values
void
mu_test_encode__invalid_1_1(void)
{
  test_long_to_bool(1, 32, 2);
}

// Fail on negative values
void
mu_test_encode__invalid_100_1(void)
{
  test_long_to_bool(100, 32, 2);
}

void
test_client_agg_uint(int nclients,
                     int prec,
                     int num_uint_entries,
                     bool config_is_okay,
                     int tweak)
{
  SECStatus rv = SECSuccess;
  PublicKey pkA = NULL;
  PublicKey pkB = NULL;
  PrivateKey skA = NULL;
  PrivateKey skB = NULL;
  PrioConfig cfg = NULL;
  PrioServer sA = NULL;
  PrioServer sB = NULL;
  PrioTotalShare tA = NULL;
  PrioTotalShare tB = NULL;
  PrioVerifier vA = NULL;
  PrioVerifier vB = NULL;
  unsigned char* for_a = NULL;
  unsigned char* for_b = NULL;
  const unsigned char* batch_id = (unsigned char*)"test_batch";
  unsigned int batch_id_len = strlen((char*)batch_id);
  long* data_items = NULL;
  unsigned long long* output = NULL;

  PrioPRGSeed seed;
  PT_CHECKC(PrioPRGSeed_randomize(&seed));

  PT_CHECKC(Keypair_new(&skA, &pkA));
  PT_CHECKC(Keypair_new(&skB, &pkB));
  P_CHECKA(cfg = PrioConfig_new_uint(
             num_uint_entries, prec, pkA, pkB, batch_id, batch_id_len));
  if (!config_is_okay) {
    PT_CHECKCB(
      (PrioConfig_new_uint(
         num_uint_entries, prec, pkA, pkB, batch_id, batch_id_len) == NULL));
  }
  PT_CHECKA(sA = PrioServer_new(cfg, 0, skA, seed));
  PT_CHECKA(sB = PrioServer_new(cfg, 1, skB, seed));
  PT_CHECKA(tA = PrioTotalShare_new());
  PT_CHECKA(tB = PrioTotalShare_new());
  PT_CHECKA(vA = PrioVerifier_new(sA));
  PT_CHECKA(vB = PrioVerifier_new(sB));

  PT_CHECKCB(PrioConfig_numUIntEntries(cfg, prec) == num_uint_entries);

  long max = (1l << (prec)) - 1;

  PT_CHECKA(data_items = calloc(num_uint_entries, sizeof(long)));
  PT_CHECKA(output = calloc(num_uint_entries, sizeof(unsigned long)));

  for (int i = 0; i < num_uint_entries; i++) {
    // Arbitrary data
    data_items[i] = max - i;
  }

  for (int c = 0; c < nclients; c++) {
    unsigned int aLen, bLen;
    PT_CHECKC(PrioClient_encode_uint(
      cfg, prec, data_items, &for_a, &aLen, &for_b, &bLen));

    PT_CHECKC(PrioVerifier_set_data(vA, for_a, aLen));
    PT_CHECKC(PrioVerifier_set_data(vB, for_b, bLen));

    mu_check(PrioServer_aggregate(sA, vA) == SECSuccess);
    mu_check(PrioServer_aggregate(sB, vB) == SECSuccess);

    free(for_a);
    free(for_b);

    for_a = NULL;
    for_b = NULL;

    if (tweak == 0) {
      mu_check(PrioTotalShare_set_data_uint(tA, sA, prec) == SECSuccess);
      mu_check(PrioTotalShare_set_data_uint(tB, sB, prec) == SECSuccess);
    }

    if (tweak == 1) {
      P_CHECKC(PrioTotalShare_set_data_uint(tA, sA, prec + 1));
      P_CHECKC(PrioTotalShare_set_data_uint(tB, sB, prec + 1));
    }

    mu_check(PrioTotalShare_final_uint(cfg, prec, output, tA, tB) ==
             SECSuccess);
    for (int i = 0; i < num_uint_entries; i++) {
      unsigned long v = max - i;
      mu_check(output[i] == v * (c + 1));
    }
  }

cleanup:
  if (tweak == 0) {
    if (config_is_okay) {
      mu_check(rv == SECSuccess);
    } else {
      mu_check(rv == SECFailure);
    }
  } else if (tweak == 1) {
    mu_check(rv == SECFailure);
  }
  if (data_items)
    free(data_items);
  if (output)
    free(output);
  if (for_a)
    free(for_a);
  if (for_b)
    free(for_b);

  PublicKey_clear(pkA);
  PublicKey_clear(pkB);
  PrivateKey_clear(skA);
  PrivateKey_clear(skB);

  PrioVerifier_clear(vA);
  PrioVerifier_clear(vB);

  PrioTotalShare_clear(tA);
  PrioTotalShare_clear(tB);

  PrioServer_clear(sA);
  PrioServer_clear(sB);
  PrioConfig_clear(cfg);
}

void
mu_test_client__agg_uint_1(void)
{
  test_client_agg_uint(1, 32, 133, true, 0);
}

void
mu_test_client__agg_uint_2(void)
{
  test_client_agg_uint(2, 32, 133, true, 0);
}

void
mu_test_client__agg_uint_10(void)
{
  test_client_agg_uint(10, 32, 133, true, 0);
}

void
mu_test_client_uint__agg_max(void)
{
  int prec = 32;
  int max = MIN(PrioConfig_maxUIntEntries(prec), 500);
  test_client_agg_uint(10, prec, max, true, 0);
}

void
mu_test_client_uint__agg_max_bad(void)
{
  int prec = 32;
  int max = PrioConfig_maxUIntEntries(prec);
  test_client_agg_uint(10, prec, max + 1, false, 0);
}

void
mu_test_client__agg_uint__max_prec_0(void)
{
  test_client_agg_uint(1, BBIT_PREC_MAX, 133, true, 0);
}

void
mu_test_client__agg_uint__max_prec_1(void)
{
  test_client_agg_uint(10, BBIT_PREC_MAX, 133, true, 0);
}

void
mu_test_client__agg_uint__exceed_prec_0(void)
{
  test_client_agg_uint(1, BBIT_PREC_MAX + 1, 133, false, 0);
}

void
mu_test_client__agg_uint__exceed_prec_1(void)
{
  test_client_agg_uint(10, BBIT_PREC_MAX + 1, 133, false, 0);
}

void
mu_test_client__agg_wrong_server_prec(void)
{
  test_client_agg_uint(10, 32, 133, true, 1);
}
