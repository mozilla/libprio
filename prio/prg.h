/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __PRG_H__
#define __PRG_H__

#include <mpi.h>
#include <nss/blapit.h>
#include <stdlib.h>

#include "config.h"

/*
 * To implement a PRG, we use AES in CTR mode. We use two different
 * implementations.
 *
 * The bottom line is:
 * - Use `PRG_SIMPLE` on Linux and Mac OS.
 * - Use `PRG_BUFFERING` on Windows.
 *
 * For more details:
 * - PRG_SIMPLE sets up a single `PK11_Context` and invokes `PK11_CipherOp`
 *   many times on this context to generate pseudorandom bytes. Because of
 *   a bug in the NSS implementation of AES-NI on Windows, calling
 * `PK11_CipherOp`
 *   many times on the same `PK11_Context` triggers an assertion failure.
 *
 * - PRG_BUFFERING maintains an internal buffer of `PRG_BUFFER_SIZE` bytes
 *   of AES-CTR output. When this buffer is empty, the PRG sets up a fresh
 *   `PK11_Context` and invokes `PK11_CipherOp` once on this context to
 *   refill the buffer. This avoids the NSS Windows AES-NI Windows bug.
 *
 * The two modes should produce byte-identical output. Once the NSS bug
 * is patched, we should get rid of `PRG_BUFFERING`.
 */

typedef enum {
  PRG_SIMPLE,
  PRG_BUFFERING // Workaround to avoid w64 AES-NI NSS bug
} PRGMethod;

#ifdef _MSC_VER
#define PRIO_DEFAULT_PRG (PRG_BUFFERING)
#else
#define PRIO_DEFAULT_PRG (PRG_SIMPLE)
#endif

typedef struct prg* PRG;
typedef const struct prg* const_PRG;

/*
 * Initialize or destroy a pseudo-random generator.
 */
PRG PRG_new(const PrioPRGSeed key, PRGMethod meth);
void PRG_clear(PRG prg);

/*
 * Produce the next bytes of output from the PRG.
 */
SECStatus PRG_get_bytes(PRG prg, unsigned char* bytes, size_t len);

/*
 * Use the PRG output to sample a big integer x in the range
 *    0 <= x < max.
 */
SECStatus PRG_get_int(PRG prg, mp_int* out, const mp_int* max);

/*
 * Use secret sharing to split the int src into two shares.
 * Use PRG to generate the value `shareB`.
 * The mp_ints must be initialized.
 */
SECStatus PRG_share_int(PRG prg, mp_int* shareA, const mp_int* src,
                        const_PrioConfig cfg);

/*
 * Set each item in the array to a pseudorandom value in the range
 * [0, mod), where the values are generated using the PRG.
 */
SECStatus PRG_get_array(PRG prg, MPArray arr, const mp_int* mod);

/*
 * Secret shares the array in `src` into `arrA` using randomness
 * provided by `prgB`. The arrays `src` and `arrA` must be the same
 * length.
 */
SECStatus PRG_share_array(PRG prgB, MPArray arrA, const_MPArray src,
                          const_PrioConfig cfg);

#endif /* __PRG_H__ */
