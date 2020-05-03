/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __PRG_H__
#define __PRG_H__

#include <blapit.h>
#include <mpi.h>
#include <stdlib.h>

#include "config.h"
#include "encode.h"

typedef struct prg* PRG;
typedef const struct prg* const_PRG;

/*
 * Initialize or destroy a pseudo-random generator.
 */
PRG
PRG_new(const PrioPRGSeed key);
void
PRG_clear(PRG prg);

/*
 * Produce the next bytes of output from the PRG.
 */
SECStatus
PRG_get_bytes(PRG prg, unsigned char* bytes, size_t len);

/*
 * Use the PRG output to sample a big integer x in the range
 *    0 <= x < max.
 */
SECStatus
PRG_get_int(PRG prg, mp_int* out, const mp_int* max);

/*
 * Use the PRG output to sample a big encoded integer x in the range
 *    0 <= x < max.
 */
SECStatus
PRG_get_e_int(PRG prg, EInt out, const mp_int* max);

/*
 * Use the PRG output to sample a big integer x in the range
 *    lower <= x < max.
 */
SECStatus
PRG_get_int_range(PRG prg, mp_int* out, const mp_int* lower, const mp_int* max);

/*
 * Use secret sharing to split the int src into two shares.
 * Use PRG to generate the value `shareB`.
 * The mp_ints must be initialized.
 */
SECStatus
PRG_share_int(PRG prg, mp_int* shareA, const mp_int* src, const_PrioConfig cfg);

/*
 * Compute a share of EInt src, with the share of x being derived from
 * the shares of the individual bits. This way, checking wether a
 * share of x is correctly encoded by the the sum of (b_i * 2^i), can
 * be expressed as an affine transformation.
 *
 * With integer encoding, Enc(x) = (x, b_0, b_1, .. , b_(n-1)) the
 * derivation works as follows:
 *
 * x = \sum_(i=0)^(n-1) 2^i * b_i)
 *   => \sum_(i=0)^(n-1) ([b_i]_1 + [b_i]_2) * 2^i
 *   => [x]_1 = \sum_(i=0)^(n-1) [b_i]_1 * 2^i,
 *      [x]_2 = \sum_(i=0)^(n-1) [b_i]_2 * 2^i
 *
 * Thus: [x]_j - (\sum_(i=0)^(n-1) [b_i]_j * 2^i) = 0, with j={1,2}
 */
SECStatus
PRG_share_e_int(PRG prgB, EInt shareA, EInt src, const_PrioConfig cfg);

/*
 * Set each item in the array to a pseudorandom value in the range
 * [0, mod), where the values are generated using the PRG.
 */
SECStatus
PRG_get_array(PRG prg, MPArray arr, const mp_int* mod);

/*
 * Set each item in the array to a pseudorandom encoded value in the range
 * [0, mod), where the values are generated using the PRG.
 */
SECStatus
PRG_get_e_array(PRG prg, EIntArray dst, const mp_int* mod);

/*
 * Secret shares the array in `src` into `arrA` using randomness
 * provided by `prgB`. The arrays `src` and `arrA` must be the same
 * length.
 */
SECStatus
PRG_share_array(PRG prgB,
                MPArray arrA,
                const_MPArray src,
                const_PrioConfig cfg);

/*
 * Secret shares the encoded array in `src` into `arrA` using randomness
 * provided by `prgB`. The arrays `src` and `arrA` must be the same
 * length.
 */
SECStatus
PRG_share_e_array(PRG prgB,
                  EIntArray arrA,
                  const_EIntArray src,
                  const_PrioConfig cfg);

#endif /* __PRG_H__ */
