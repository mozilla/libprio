/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mprio.h>

// Needed for NAN, INFINITY, nextafterf()
#include <math.h>

// Needed for FLT_MAX
#include <float.h>

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

// compute q_one and fp_eps for testing purposes
float
eps_step(int fbits, int i)
{
  long q_one = (1l << fbits);
  float fp_eps = 1 / (float)q_one;

  return fp_eps * i;
}

long
eps_num(int ibits, int fbits)
{
  return (ibits + 1) * PrioConfig_FPQOne(fbits);
}

// Test wether rounding to resolution steps works correctly and wether
// they are corretly ordered.
void
check_order_fp_encodings(int ibits, int fbits)
{
  SECStatus rv = SECSuccess;

  // Calculate the following constants explicitly for testing purposes.
  long q_one = (1l << fbits);
  PT_CHECKCB(q_one == PrioConfig_FPQOne(fbits));

  long m_bias = (1l << (ibits + fbits));
  PT_CHECKCB(m_bias == PrioConfig_FPMBias(ibits, fbits));

  // The resolution of our fixed point encoding
  float fp_eps = 1 / (float)q_one;
  PT_CHECKCB(fp_eps == PrioConfig_FPEps(fbits));

  // The maximum positive value of the integer part
  long max_int = (1l << ibits) - 1;

  // Maximum representable value of our fixed point encoding,
  // end of testing range
  float max = max_int + (1 - fp_eps);
  PT_CHECKCB(max == PrioConfig_FPMax(ibits, fbits));

  // Input floats and decoded results
  float cur, revert_p, revert_n;

  // Rounding bounds for a given range
  float lower_bound, upper_bound, next_upper_bound;

  // Buffer for fixed point encoded floats (as long)
  long dst_p = 0;
  long dst_n = 0;

  // Start test here
  cur = 0;
  long step = 0;

  /*
   *  -eps*3  -eps*2  -eps*1    0     eps*1   eps*2   eps*3
   *    |-------|-------|-------|-------|-------|-------|
   *               ^                         ^
   *             -cur                      +cur
   *
   * The fixed point encoding for any given float will always be the
   * closest resolution step towards 0, independent of where in this
   * range it lies. In the above case, +cur would be encoded as eps*1
   * and -cur as -eps*1.
   *
   * In the test below we check wether this is the case.
   */
  while (max > cur) {
    // Increment cur to the next positive representable floating point number
    cur = nextafterf(cur, max);

    // Encode float as long
    P_CHECKC(float_to_long(&dst_p, cur, ibits, fbits));
    P_CHECKC(float_to_long(&dst_n, -cur, ibits, fbits));

    // Decode long to float
    revert_p = (dst_p - (double)m_bias) / q_one;
    revert_n = (dst_n - (double)m_bias) / q_one;

    // Compute bounds
    lower_bound = eps_step(fbits, step);
    upper_bound = eps_step(fbits, step + 1);
    next_upper_bound = eps_step(fbits, step + 2);

    // Check wether bounds are in the right order.
    P_CHECKCB(lower_bound < upper_bound);
    P_CHECKCB(upper_bound < next_upper_bound);
    // NOTE: These checks fail if (ibits + bits) > 24, since the
    // mantissa of a float has 24 bit precision, which makes 16777215
    // the largest int that can be precisely represented with a float,
    // but since FPBITS_MAX is 21 this is not an issue.

    if ((cur >= lower_bound) && (cur < upper_bound)) {
      // If cur is between two resolution steps, check if it rounds
      // correctly.
      P_CHECKCB(revert_p == lower_bound);
      P_CHECKCB(revert_n == -lower_bound);
    } else if ((cur >= upper_bound) && (cur < next_upper_bound)) {
      // If cur is not in between the current two steps, but between
      // the next two, check if it rounds correctly and increment the
      // interval counter.
      P_CHECKCB(revert_p == upper_bound);
      P_CHECKCB(revert_n == -upper_bound);
      step++;
    } else {
      // Fail if cur is neither in the current, nor the next interval.
      P_CHECKCB(false);
    }

    // NOTE: We use P_CHECKCB in the loop since mu_check uses an int counter for
    // checks and would overflow.
  }

cleanup:
  mu_check(rv == SECSuccess);
}

// Test wether a sample of floats outside of the permissible range
// gets correctly rejected.
void
reject_large_small(void)
{
  SECStatus rv = SECSuccess;
  float max = FLT_MAX;
  float min = PrioConfig_FPMax(15, 6) + PrioConfig_FPEps(6);
  float cur = min;

  long dst;
  long i = 0;

  while (cur < max) {
    cur = nextafterf(cur, max);
    if (i % 10000) {
      P_CHECKCB(float_to_long(&dst, cur, 15, 6) == SECFailure);
      P_CHECKCB(float_to_long(&dst, -cur, 15, 6) == SECFailure);
    }
    i++;
  }

cleanup:
  mu_check(rv == SECSuccess);
}

void
reject_nan_inf(void)
{
#ifdef NAN
  long dst1;

  SECStatus rv1 = float_to_long(&dst1, NAN, 10, 11);

  mu_check(rv1 == SECFailure);
#endif

#ifdef INFINITY
  long dst2;

  SECStatus rv2 = float_to_long(&dst2, -INFINITY, 10, 11);
  SECStatus rv3 = float_to_long(&dst2, INFINITY, 10, 11);

  mu_check((rv2 == SECFailure) && (rv3 == SECFailure));
#endif
}

// FLT_RADIX != 2 should not happen in binary floating-point
// environments
void
float_radix(void)
{
  mu_check(FLT_RADIX == 2);
}

// Check wether a long double fits enough FPMAX_BITS submissions
void
long_double_size(void)
{
  mu_check(LDBL_MAX >= ((2 ^ BBIT_PREC_MAX) - 1) * FLT_MAX);
}

// Produces a representative sample of size (slice * 6)
void
gen_valid_fp_sample(int ibits, int fbits, int slice, float* sample)
{
  long max_i = eps_num(ibits, fbits);

  // Small positive fps, including 0
  for (int i = 0; i < slice; i++) {
    sample[i] = eps_step(fbits, i);
  }

  // Small negative fps, including -0
  for (int i = 0; i < slice; i++) {
    sample[i + slice] = -eps_step(fbits, i);
  }

  // Large positive fps (including fp_max)
  for (int i = 0; i < slice; i++) {
    sample[i + (slice * 2)] = eps_step(fbits, (max_i - i));
  }

  // Large negative fps (including -fp_max)
  for (int i = 0; i < slice; i++) {
    sample[i + (slice * 3)] = -eps_step(fbits, (max_i - i));
  }

  // Midrange positive fps
  for (int i = (slice * 4); i < (slice * 5); i++) {
    sample[i] = eps_step(fbits, ((max_i / 2) + i));
  }

  // Midrange negative fps
  for (int i = (slice * 5); i < (slice * 6); i++) {
    sample[i] = -eps_step(fbits, ((max_i / 2) + i));
  }
}

// Test wether resolution steps get properly en- and decode as longs
// tweak 0, 2, 3 succeed; 1, fails
void
test_client_encode_decode_eps_steps(int nclients,
                                    int ibits,
                                    int fbits,
                                    int slice,
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
  float* data_items = NULL;
  unsigned long long* output_uint = NULL;
  long double* output_fp = NULL;

  int num_fp_entries;

  if ((tweak == 2) || (tweak == 3) || (tweak == 5)) {
    num_fp_entries = slice;
  } else {
    num_fp_entries = 6 * slice;
  }

  PrioPRGSeed seed;
  PT_CHECKC(PrioPRGSeed_randomize(&seed));

  PT_CHECKC(Keypair_new(&skA, &pkA));
  PT_CHECKC(Keypair_new(&skB, &pkB));
  P_CHECKA(cfg = PrioConfig_new_fp(
             num_fp_entries, ibits, fbits, pkA, pkB, batch_id, batch_id_len));
  if (!config_is_okay) {
    PT_CHECKCB(
      (PrioConfig_new_fp(
         num_fp_entries, ibits, fbits, pkA, pkB, batch_id, batch_id_len) ==
       NULL));
  }
  PT_CHECKA(sA = PrioServer_new(cfg, 0, skA, seed));
  PT_CHECKA(sB = PrioServer_new(cfg, 1, skB, seed));
  PT_CHECKA(tA = PrioTotalShare_new());
  PT_CHECKA(tB = PrioTotalShare_new());
  PT_CHECKA(vA = PrioVerifier_new(sA));
  PT_CHECKA(vB = PrioVerifier_new(sB));

  PT_CHECKCB((PrioConfig_numFPEntries(cfg, ibits, fbits) >= num_fp_entries));

  PT_CHECKA(data_items = calloc(num_fp_entries, sizeof(float)));
  PT_CHECKA(output_uint = calloc(num_fp_entries, sizeof(unsigned long)));
  PT_CHECKA(output_fp = calloc(num_fp_entries, sizeof(long double)));

  if (tweak == 2) {
    data_items[0] = PrioConfig_FPMax(ibits, fbits);
  } else if (tweak == 3) {
    data_items[0] = -PrioConfig_FPMax(ibits, fbits);
  } else if (tweak == 5) {
    data_items[0] = -PrioConfig_FPMax(ibits, fbits);
    data_items[1] = PrioConfig_FPMax(ibits, fbits);
    data_items[2] = PrioConfig_FPEps(fbits);
    data_items[3] = -PrioConfig_FPEps(fbits);
  } else {
    // Data sample
    gen_valid_fp_sample(ibits, fbits, 15, data_items);
  }

  for (int c = 0; c < nclients; c++) {
    unsigned int aLen, bLen;
    // Fail on tweak 2 and 3 cause values > max and < -max get passed
    P_CHECKC(PrioClient_encode_fp(
      cfg, ibits, fbits, data_items, &for_a, &aLen, &for_b, &bLen));

    PT_CHECKC(PrioVerifier_set_data(vA, for_a, aLen));
    PT_CHECKC(PrioVerifier_set_data(vB, for_b, bLen));

    mu_check(PrioServer_aggregate(sA, vA) == SECSuccess);
    mu_check(PrioServer_aggregate(sB, vB) == SECSuccess);

    free(for_a);
    free(for_b);

    for_a = NULL;
    for_b = NULL;

    // Simulate mismatch between client and server config
    if (tweak == 1) {
      P_CHECKC(PrioTotalShare_set_data_fp(tA, sA, ibits + 1, fbits));
      P_CHECKC(PrioTotalShare_set_data_fp(tB, sB, ibits + 1, fbits));
    } else {
      mu_check(PrioTotalShare_set_data_fp(tA, sA, ibits, fbits) == SECSuccess);
      mu_check(PrioTotalShare_set_data_fp(tB, sB, ibits, fbits) == SECSuccess);
    }

    mu_check(PrioTotalShare_final_fp(
               cfg, ibits, fbits, (c + 1), output_uint, output_fp, tA, tB) ==
             SECSuccess);

    if (tweak == 2) {
      double v = PrioConfig_FPMax(ibits, fbits);
      mu_check(output_fp[0] == v * (c + 1));
    } else if (tweak == 3) {
      double v = -PrioConfig_FPMax(ibits, fbits);
      mu_check(output_fp[0] == v * (c + 1));
    } else {
      for (int i = 0; i < num_fp_entries; i++) {
        double v = data_items[i];
        mu_check(output_fp[i] == v * (c + 1));
      }
    }
  }

cleanup:
  if ((tweak == 0) || (tweak == 2) || (tweak == 3) || (tweak == 5)) {
    if (config_is_okay) {
      mu_check(rv == SECSuccess);
    } else {
      mu_check(rv == SECFailure);
    }
  } else {
    mu_check(rv == SECFailure);
  }

  if (data_items)
    free(data_items);
  if (output_uint)
    free(output_uint);
  if (output_fp)
    free(output_fp);
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

// Maximal fractional precision
void
mu_test_client__fp_round_all_floats_1(void)
{
  check_order_fp_encodings(0, 21);
}

// Maximal integer precision
void
mu_test_client__fp_round_all_floats_2(void)
{
  check_order_fp_encodings(21, 0);
}

// Mininimal fractional precision
void
mu_test_client__fp_round_all_floats_3(void)
{
  check_order_fp_encodings(0, 1);
}

// Mininimal integer precision
void
mu_test_client__fp_round_all_floats_4(void)
{
  check_order_fp_encodings(1, 0);
}

// Minimal mixed precision
void
mu_test_client__fp_round_all_floats_5(void)
{
  check_order_fp_encodings(1, 1);
}

// Balanced precision
void
mu_test_client__fp_round_all_floats_6(void)
{
  check_order_fp_encodings(10, 11);
}

void
mu_test_client__fp_reject_large_small(void)
{
  reject_large_small();
}

void
mu_test_client__fp_reject_nan_inf(void)
{
  reject_nan_inf();
}

void
mu_test_client__fp_float_radix(void)
{
  float_radix();
}

void
mu_test_client__fp_long_double_size(void)
{
  float_radix();
}

void
mu_test_client__fp_round_step_sample(void)
{
  int ibits = 4;
  int fbits = 17;
  test_client_encode_decode_eps_steps(1, ibits, fbits, 15, 1, 0);
}

void
mu_test_client__fp_round_and_aggregate_step_sample(void)
{
  int ibits = 4;
  int fbits = 17;
  test_client_encode_decode_eps_steps(10, ibits, fbits, 15, 1, 0);
}

void
mu_test_client__fp_round_and_step_min(void)
{
  int ibits = 1;
  int fbits = 1;
  test_client_encode_decode_eps_steps(1, ibits, fbits, 4, 1, 5);
}

void
mu_test_client__fp_round_and_aggregate_step_min(void)
{
  int ibits = 1;
  int fbits = 1;
  test_client_encode_decode_eps_steps(10, ibits, fbits, 4, 1, 5);
}

void
mu_test_client__fp_round_and_aggregate_max_int(void)
{
  // Aggregate single entry submissions with maximal integer part
  int ibits = 21;
  int fbits = 0;
  test_client_encode_decode_eps_steps(20, ibits, fbits, 1, 1, 2);
}

void
mu_test_client__fp_round_and_aggregate_max_frac(void)
{
  // Aggregate single entry submissions with maximal fractional part
  int ibits = 0;
  int fbits = 21;
  test_client_encode_decode_eps_steps(20, ibits, fbits, 1, 1, 2);
}

void
mu_test_client__fp_round_and_aggregate_mixed_pos(void)
{
  // Aggregate single entry submissions with mixed precision and
  // maximal positive values
  int ibits = 10;
  int fbits = 11;
  test_client_encode_decode_eps_steps(2, ibits, fbits, 1, 1, 2);
}

void
mu_test_client__fp_round_and_aggregate_mixed_neg(void)
{
  // Aggregate single entry submissions with mixed precision and
  // maximal negative values
  int ibits = 10;
  int fbits = 11;
  test_client_encode_decode_eps_steps(100, ibits, fbits, 1, 1, 3);
}

void
mu_test_client__fp_exceed_FBITS_MAX(void)
{
  // Exceed FPBITS_MAX
  int ibits = 11;
  int fbits = 11;
  test_client_encode_decode_eps_steps(1, ibits, fbits, 15, 0, 0);
}

void
mu_test_client__fp_bits_too_low(void)
{
  // Exceed FPBITS_MAX
  int ibits = 0;
  int fbits = 0;
  test_client_encode_decode_eps_steps(1, ibits, fbits, 15, 0, 0);
}

void
mu_test_client__fp_mismatch(void)
{
  // Mismatch of FP format on client (correct) and server (ibits + 1)
  int ibits = 10;
  int fbits = 11;
  test_client_encode_decode_eps_steps(1, ibits, fbits, 15, 1, 1);
}

// Move this to float_to_long
void
mu_test_client__fp_float_too_large1(void)
{
  // Pass a positive float x that is too large
  int ibits = 10;
  int fbits = 11;
  test_client_encode_decode_eps_steps(1, ibits, fbits, 15, 1, 2);
}

// Move this to float_to_long
void
mu_test_client__fp_float_too_large2(void)
{
  // Pass a negative float that is too large
  int ibits = 10;
  int fbits = 11;
  test_client_encode_decode_eps_steps(1, ibits, fbits, 15, 1, 3);
}
