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
#include "prio/circuit.c"
#include "prio/circuit.h"
#include "prio/encode.c"
#include "prio/encode.h"
#include "prio/prg.h"

#include "test_util.h"

/*
 * IntCircuit_set_fg_client and IntCircuit_set_fg_server get tested in
 * client_test and server_test respectively, like in the single bit
 * case.
 */

/*
 * Produce EInt shares where the bit shares do not implement the share of x
 */
SECStatus
forged_shared_EInt(const_PrioConfig cfg, PRG prg, int x, EInt out)
{
  SECStatus rv = SECSuccess;

  int prec = cfg->precision;

  mp_int forge;
  EInt src = NULL;

  MP_DIGITS(&forge) = NULL;
  MPT_CHECKC(mp_init(&forge));
  mp_set_int(&forge, x + 1);

  PT_CHECKA(src = EInt_new(prec));
  P_CHECKC(EInt_set(src, x)); // Not PT_CHECKC since test would fail to early
  PT_CHECKC(PRG_share_e_int(prg, out, src, cfg));

  // Write the share of forge to x of share
  PT_CHECKC(PRG_share_int(prg, &out->x, &forge, cfg));

cleanup:
  mp_clear(&forge);
  EInt_clear(src);
  return rv;
}

/*
 * Test wether IntCircuit_check_x_accum recognizes EInt with
 * non-matching x and bits
 */
void
test__bad_EInt(int tweak)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = NULL;

  int fields = 10;
  int prec = 32;
  long max = (1l << (prec)) - 1;

  long* longs = NULL;

  PrioPRGSeed key;
  PRG prg = NULL;

  EIntArray data_in = NULL;
  EIntArray shares = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(fields, prec));

  PT_CHECKC(PrioPRGSeed_randomize(&key));
  PT_CHECKA(prg = PRG_new(key));

  P_CHECKA(longs = calloc(fields, sizeof(long)));

  for (int i = 0; i < fields; i++) {
    longs[i] = max - i;
  }

  PT_CHECKA(data_in = EIntArray_new_data(fields, prec, longs));
  PT_CHECKA(shares = EIntArray_new(fields, prec));
  PT_CHECKC(PRG_share_e_array(prg, shares, data_in, cfg));

  // Introduce one EInt with bit shares not matching x
  if (tweak == 1) {
    P_CHECKC(forged_shared_EInt(cfg, prg, longs[0], shares->data[0]));
  }

  // Only EInts with bit shares not matching x
  if (tweak == 2) {
    for (int i = 0; i < fields; i++) {
      P_CHECKC(forged_shared_EInt(cfg, prg, longs[i], shares->data[i]));
    }
  }

  P_CHECKC(IntCircuit_check_x_accum(cfg, shares));

cleanup:
  if (tweak == 0) {
    mu_check(rv == SECSuccess);
  }
  if (tweak != 0) {
    mu_check(rv == SECFailure);
  }
  if (longs)
    free(longs);
  PRG_clear(prg);
  EIntArray_clear(data_in);
  EIntArray_clear(shares);
  PrioConfig_clear(cfg);
}

// No bad EInts
void
mu_test__check_bad_EInt_good(void)
{
  test__bad_EInt(0);
}

// One bad EInt
void
mu_test__check_bad_EInt_bad1(void)
{
  test__bad_EInt(1);
}

// Only bad EInts
void
mu_test__check_bad_EInt_bad2(void)
{
  test__bad_EInt(2);
}
