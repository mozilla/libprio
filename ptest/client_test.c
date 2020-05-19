/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mprio.h>

#include "mutest.h"
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
test_client_agg_int(int nclients, int prec, int num_int_entries,
                    bool config_is_okay)
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
  P_CHECKA(cfg = PrioConfig_new_int(num_int_entries, prec, pkA, pkB, batch_id,
                                    batch_id_len));
  if (!config_is_okay) {
    PT_CHECKCB((PrioConfig_new_int(num_int_entries, prec, pkA, pkB, batch_id,
                                   batch_id_len) == NULL));
  }
  PT_CHECKA(sA = PrioServer_new(cfg, 0, skA, seed));
  PT_CHECKA(sB = PrioServer_new(cfg, 1, skB, seed));
  PT_CHECKA(tA = PrioTotalShare_new());
  PT_CHECKA(tB = PrioTotalShare_new());
  PT_CHECKA(vA = PrioVerifier_new(sA));
  PT_CHECKA(vB = PrioVerifier_new(sB));

  PT_CHECKCB(PrioConfig_numIntEntries(cfg, prec) == num_int_entries);

  long max = (1l << (prec)) - 1;

  PT_CHECKA(data_items = calloc(num_int_entries, sizeof(long)));
  for (int i = 0; i < num_int_entries; i++) {
    // Arbitrary data
    data_items[i] = max - i;
  }

  for (int i = 0; i < nclients; i++) {
    unsigned int aLen, bLen;
    PT_CHECKC(PrioClient_encode_int(cfg, prec, data_items, &for_a, &aLen,
                                    &for_b, &bLen));

    PT_CHECKC(PrioVerifier_set_data(vA, for_a, aLen));
    PT_CHECKC(PrioVerifier_set_data(vB, for_b, bLen));

    mu_check(PrioServer_aggregate(sA, vA) == SECSuccess);
    mu_check(PrioServer_aggregate(sB, vB) == SECSuccess);

    free(for_a);
    free(for_b);

    for_a = NULL;
    for_b = NULL;
  }

  mu_check(PrioTotalShare_set_data_int(tA, sA, prec) == SECSuccess);
  mu_check(PrioTotalShare_set_data_int(tB, sB, prec) == SECSuccess);

  PT_CHECKA(output = calloc(num_int_entries, sizeof(unsigned long)));
  mu_check(PrioTotalShare_final_int(cfg, prec, output, tA, tB) == SECSuccess);
  for (int i = 0; i < num_int_entries; i++) {
    unsigned long v = max - i;
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
mu_test_client__agg_int_1(void)
{
  test_client_agg_int(1, 32, 133, true);
}

void
mu_test_client__agg_int_2(void)
{
  test_client_agg_int(2, 32, 133, true);
}

void
mu_test_client__agg_int_10(void)
{
  test_client_agg_int(10, 32, 133, true);
}

void
mu_test_client_int__agg_max(void)
{
  int prec = 32;
  int max = MIN(PrioConfig_maxIntEntries(prec), 500);
  test_client_agg_int(10, prec, max, true);
}

void
mu_test_client_int__agg_max_bad(void)
{
  int prec = 32;
  int max = PrioConfig_maxIntEntries(prec);
  test_client_agg_int(10, prec, max + 1, false);
}
