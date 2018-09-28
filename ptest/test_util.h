/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __TEST_UTIL_H__
#define __TEST_UTIL_H__

#include <mpi.h>
#include <mprio.h>

// These macros behave like the macros in "prio/util.h", except that they
// also use `mu_check()` to check the return value of the argument. By
// using these macros, you get a nice human-readable error message that
// points to the line number where the test failed.

#define PT_CHECKA(s)                                                           \
  do {                                                                         \
    bool v;                                                                    \
    mu_check((v = (s)));                                                       \
    if (!v) {                                                                  \
      rv = SECFailure;                                                         \
      goto cleanup;                                                            \
    }                                                                          \
  } while (0);

#define PT_CHECKC(s)                                                           \
  do {                                                                         \
    mu_check((rv = (s)) == SECSuccess);                                        \
    if (rv != SECSuccess) {                                                    \
      goto cleanup;                                                            \
    }                                                                          \
  } while (0);

#define PT_CHECKCB(s)                                                          \
  do {                                                                         \
    bool v;                                                                    \
    mu_check((v = (s)));                                                       \
    if (!v) {                                                                  \
      goto cleanup;                                                            \
    }                                                                          \
  } while (0);

#define MPT_CHECKC(s)                                                          \
  do {                                                                         \
    bool v;                                                                    \
    mu_check((v = (s)) == MP_OKAY);                                            \
    if (v != MP_OKAY) {                                                        \
      rv = SECFailure;                                                         \
      goto cleanup;                                                            \
    }                                                                          \
  } while (0);

#endif /* __TEST_UTIL_H__ */
