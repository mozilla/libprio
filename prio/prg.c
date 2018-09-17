/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mprio.h>
#include <nss/blapit.h>
#include <nss/pk11pub.h>

#include "prg.h"
#include "rand.h"
#include "share.h"
#include "util.h"

// Size (in bytes) of buffer to use for `PRG_BUFFERING` mode.
// Should be a multiple of `AES_BLOCK_SIZE`.
static const size_t PRG_BUFFER_SIZE = AES_BLOCK_SIZE * 128;
// Number of bits use for AES-CTR counter.
static const int AES_COUNTER_BITS = 128;
// Use AES-CTR mode for PRG.
static const CK_MECHANISM_TYPE PRG_CIPHER = CKM_AES_CTR;

struct prg
{
  PRGMethod meth;
  PK11SlotInfo* slot;
  PK11SymKey* key;
  CK_AES_CTR_PARAMS param;

  // For method `PRG_SIMPLE`.
  PK11Context* ctx;

  // For method `PRG_BUFFERING`.
  unsigned char* buf;
  size_t bytes_used;
};

SECStatus
PrioPRGSeed_randomize(PrioPRGSeed* key)
{
  return rand_bytes((unsigned char*)key, PRG_SEED_LENGTH);
}

static PK11Context*
create_context(PRG prg)
{
  SECItem paramItem = { siBuffer, (void*)&prg->param,
                        sizeof(CK_AES_CTR_PARAMS) };
  return PK11_CreateContextBySymKey(PRG_CIPHER, CKA_ENCRYPT, prg->key,
                                    &paramItem);
}

PRG
PRG_new(const PrioPRGSeed key_in, PRGMethod meth)
{
  PRG prg = malloc(sizeof(struct prg));
  if (!prg)
    return NULL;
  prg->meth = meth;
  prg->slot = NULL;
  prg->key = NULL;
  prg->ctx = NULL;
  prg->buf = NULL;

  SECStatus rv = SECSuccess;
  P_CHECKA(prg->slot = PK11_GetInternalSlot());

  // Create a mutable copy of the key.
  PrioPRGSeed key_mut;
  memcpy(key_mut, key_in, PRG_SEED_LENGTH);

  SECItem keyItem = { siBuffer, key_mut, PRG_SEED_LENGTH };

  // The IV can be all zeros since we only encrypt once with
  // each AES key.
  prg->param.ulCounterBits = AES_COUNTER_BITS;
  memset(prg->param.cb, 0, AES_BLOCK_SIZE);

  P_CHECKA(prg->key =
             PK11_ImportSymKey(prg->slot, PRG_CIPHER, PK11_OriginUnwrap,
                               CKA_ENCRYPT, &keyItem, NULL));

  switch (prg->meth) {
    case PRG_SIMPLE:
      P_CHECKA(prg->ctx = create_context(prg));
      break;

    case PRG_BUFFERING:
      // Set this to the buffer length to make sure that we
      // fill the buffer before giving out any output.
      prg->bytes_used = PRG_BUFFER_SIZE;
      P_CHECKA(prg->buf = calloc(PRG_BUFFER_SIZE, sizeof(unsigned char)));
      break;

    default:
      // Should never get here
      P_CHECKC(SECFailure);
      break;
  }

cleanup:
  if (rv != SECSuccess) {
    PRG_clear(prg);
    prg = NULL;
  }

  return prg;
}

void
PRG_clear(PRG prg)
{
  if (!prg)
    return;

  if (prg->key)
    PK11_FreeSymKey(prg->key);
  if (prg->slot)
    PK11_FreeSlot(prg->slot);
  if (prg->ctx)
    PK11_DestroyContext(prg->ctx, PR_TRUE);
  if (prg->buf)
    free(prg->buf);

  free(prg);
}

static SECStatus
get_bytes_internal_simple(PK11Context* ctx, unsigned char* bytes, size_t len)
{
  SECStatus rv = SECSuccess;
  unsigned char* in = NULL;
  P_CHECKA(in = calloc(len, sizeof(unsigned char)));

  int outlen;
  P_CHECKC(PK11_CipherOp(ctx, bytes, &outlen, len, in, len));
  P_CHECKCB(((size_t)outlen) == len);

cleanup:
  if (in) {
    free(in);
  }

  return rv;
}

// Code to increment the AES-CTR counter.
// Lifted from NSS:  security/nss/lib/freebl/ctr.c
static void
get_next_counter(unsigned char* counter, unsigned int counterBits,
                 unsigned int blocksize)
{
  unsigned char* counterPtr = counter + blocksize - 1;
  unsigned char mask, count;

  PORT_Assert(counterBits <= blocksize * PR_BITS_PER_BYTE);
  while (counterBits >= PR_BITS_PER_BYTE) {
    if (++(*(counterPtr--))) {
      return;
    }
    counterBits -= PR_BITS_PER_BYTE;
  }
  if (counterBits == 0) {
    return;
  }
  // increment the final partial byte
  mask = (1 << counterBits) - 1;
  count = ++(*counterPtr) & mask;
  *counterPtr = ((*counterPtr) & ~mask) | count;
  return;
}

static SECStatus
refill_buffer(PRG prg)
{
  if (!(prg && prg->meth == PRG_BUFFERING && prg->ctx == NULL &&
        prg->bytes_used == PRG_BUFFER_SIZE)) {
    return SECFailure;
  }

  PK11Context* ctx = NULL;
  const size_t blocks_in_buf = (PRG_BUFFER_SIZE / AES_BLOCK_SIZE);

  SECStatus rv = SECSuccess;
  P_CHECKA(ctx = create_context(prg));
  P_CHECKC(get_bytes_internal_simple(ctx, prg->buf, PRG_BUFFER_SIZE));
  prg->bytes_used = 0;

  // Increment counter
  for (size_t i = 0; i < blocks_in_buf; i++) {
    // TODO: If we are going to keep this for the long run, we should
    // increment the counter in a more efficient way.
    get_next_counter(prg->param.cb, prg->param.ulCounterBits, AES_BLOCK_SIZE);
  }

cleanup:
  if (ctx) {
    PK11_DestroyContext(ctx, PR_TRUE);
  }

  return rv;
}

static SECStatus
get_bytes_internal_buffering(PRG prg, unsigned char* bytes, size_t len)
{
  SECStatus rv = SECSuccess;
  unsigned char* outp = bytes;
  size_t needed = len;

  do {
    const size_t in_buf = PRG_BUFFER_SIZE - prg->bytes_used;
    const size_t to_copy = MIN(needed, in_buf);
    memcpy(outp, prg->buf + prg->bytes_used, to_copy);
    outp += to_copy;
    needed -= to_copy;
    prg->bytes_used += to_copy;

    if (prg->bytes_used == PRG_BUFFER_SIZE) {
      P_CHECKC(refill_buffer(prg));
    }
  } while (needed);

cleanup:
  return rv;
}

static SECStatus
PRG_get_bytes_internal(void* prg_vp, unsigned char* bytes, size_t len)
{
  PRG prg = prg_vp;
  switch (prg->meth) {
    case PRG_SIMPLE:
      return get_bytes_internal_simple(prg->ctx, bytes, len);
    case PRG_BUFFERING:
      return get_bytes_internal_buffering(prg, bytes, len);
    default:
      return SECFailure;
  }
}

SECStatus
PRG_get_bytes(PRG prg, unsigned char* bytes, size_t len)
{
  return PRG_get_bytes_internal(prg, bytes, len);
}

SECStatus
PRG_get_int(PRG prg, mp_int* out, const mp_int* max)
{
  return rand_int_rng(out, max, &PRG_get_bytes_internal, prg);
}

SECStatus
PRG_get_array(PRG prg, MPArray dst, const mp_int* mod)
{
  SECStatus rv;
  for (int i = 0; i < dst->len; i++) {
    P_CHECK(PRG_get_int(prg, &dst->data[i], mod));
  }

  return SECSuccess;
}

SECStatus
PRG_share_int(PRG prgB, mp_int* shareA, const mp_int* src, const_PrioConfig cfg)
{
  SECStatus rv = SECSuccess;
  mp_int tmp;
  MP_DIGITS(&tmp) = NULL;

  MP_CHECKC(mp_init(&tmp));
  P_CHECKC(PRG_get_int(prgB, &tmp, &cfg->modulus));
  MP_CHECKC(mp_submod(src, &tmp, &cfg->modulus, shareA));

cleanup:
  mp_clear(&tmp);
  return rv;
}

SECStatus
PRG_share_array(PRG prgB, MPArray arrA, const_MPArray src, const_PrioConfig cfg)
{
  SECStatus rv = SECSuccess;
  if (arrA->len != src->len)
    return SECFailure;

  const int len = src->len;

  for (int i = 0; i < len; i++) {
    P_CHECK(PRG_share_int(prgB, &arrA->data[i], &src->data[i], cfg));
  }

  return rv;
}
