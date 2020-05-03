/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <blapit.h>
#include <mprio.h>
#include <pk11pub.h>
#include <string.h>

#include "encode.h"
#include "prg.h"
#include "rand.h"
#include "share.h"
#include "util.h"

struct prg
{
  PK11SlotInfo* slot;
  PK11SymKey* key;
  PK11Context* ctx;
};

SECStatus
PrioPRGSeed_randomize(PrioPRGSeed* key)
{
  return rand_bytes((unsigned char*)key, PRG_SEED_LENGTH);
}

PRG
PRG_new(const PrioPRGSeed key_in)
{
  PRG prg = malloc(sizeof(struct prg));
  if (!prg)
    return NULL;
  prg->slot = NULL;
  prg->key = NULL;
  prg->ctx = NULL;

  SECStatus rv = SECSuccess;
  const CK_MECHANISM_TYPE cipher = CKM_AES_CTR;

  P_CHECKA(prg->slot = PK11_GetInternalSlot());

  // Create a mutable copy of the key.
  PrioPRGSeed key_mut;
  memcpy(key_mut, key_in, PRG_SEED_LENGTH);

  SECItem keyItem = { siBuffer, key_mut, PRG_SEED_LENGTH };

  // The IV can be all zeros since we only encrypt once with
  // each AES key.
  CK_AES_CTR_PARAMS param = { 128, {} };
  SECItem paramItem = { siBuffer, (void*)&param, sizeof(CK_AES_CTR_PARAMS) };

  P_CHECKA(
    prg->key = PK11_ImportSymKey(
      prg->slot, cipher, PK11_OriginUnwrap, CKA_ENCRYPT, &keyItem, NULL));

  P_CHECKA(prg->ctx = PK11_CreateContextBySymKey(
             cipher, CKA_ENCRYPT, prg->key, &paramItem));

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

  free(prg);
}

static SECStatus
PRG_get_bytes_internal(void* prg_vp, unsigned char* bytes, size_t len)
{
  SECStatus rv = SECSuccess;
  PRG prg = (PRG)prg_vp;
  unsigned char* in = NULL;

  P_CHECKA(in = calloc(len, sizeof(unsigned char)));

  int outlen;
  P_CHECKC(PK11_CipherOp(prg->ctx, bytes, &outlen, len, in, len));
  P_CHECKCB((size_t)outlen == len);

cleanup:
  if (in)
    free(in);

  return rv;
}

SECStatus
PRG_get_bytes(PRG prg, unsigned char* bytes, size_t len)
{
  return PRG_get_bytes_internal((void*)prg, bytes, len);
}

SECStatus
PRG_get_int(PRG prg, mp_int* out, const mp_int* max)
{
  return rand_int_rng(out, max, &PRG_get_bytes_internal, (void*)prg);
}

// TODO: Move tmp into EInt_x_accum -> more allocations but cleaner code
SECStatus
PRG_e_int_accum_x(EInt dst, int bit, mp_int* tmp, const mp_int* mod)
{
  SECStatus rv = SECSuccess;

  // multiply share of i-th bit by 2^(prec-bit-1) (2^i reversed)
  MP_CHECKC(
    mp_mul_d(&dst->bits->data[bit], (1l << (dst->prec - bit - 1)), tmp));
  // add result to x, to sum up to a share of x when loop is done
  MP_CHECKC(mp_addmod(&dst->x, tmp, mod, &dst->x));

cleanup:
  return rv;
}

SECStatus
PRG_get_e_int(PRG prg, EInt out, const mp_int* max)
{
  SECStatus rv = SECSuccess;

  mp_int tmp;
  MP_DIGITS(&tmp) = NULL;
  MP_CHECKC(mp_init(&tmp));

  // set x to zero for accumulation to reuse memory correctly
  mp_zero(&out->x);
  for (int bit = 0; bit < out->prec; bit++) {
    // get the i-th bit
    P_CHECKC(PRG_get_int(prg, &out->bits->data[bit], max));
    P_CHECKC(PRG_e_int_accum_x(out, bit, &tmp, max));
  }

cleanup:
  mp_clear(&tmp);
  return rv;
}

SECStatus
PRG_get_int_range(PRG prg, mp_int* out, const mp_int* lower, const mp_int* max)
{
  SECStatus rv;
  mp_int width;
  MP_DIGITS(&width) = NULL;
  MP_CHECKC(mp_init(&width));

  // Compute
  //    width = max - lower
  MP_CHECKC(mp_sub(max, lower, &width));

  // Get an integer x in the range [0, width)
  P_CHECKC(PRG_get_int(prg, out, &width));

  // Set
  //    out = lower + x
  // which is in the range [lower, width+lower),
  // which is              [lower, max).
  MP_CHECKC(mp_add(lower, out, out));

cleanup:
  mp_clear(&width);
  return rv;
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
PRG_get_e_array(PRG prg, EIntArray dst, const mp_int* mod)
{
  SECStatus rv;
  for (int i = 0; i < dst->len; i++) {
    P_CHECK(PRG_get_e_int(prg, dst->data[i], mod));
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
PRG_share_e_int(PRG prgB, EInt shareA, EInt src, const_PrioConfig cfg)
{
  SECStatus rv = SECSuccess;
  const int prec = cfg->precision;
  const mp_int* mod = &cfg->modulus;

  mp_int tmp;
  MP_DIGITS(&tmp) = NULL;
  MP_CHECKC(mp_init(&tmp));

  // set x to zero for accumulation to reuse memory correctly
  mp_zero(&shareA->x);
  for (int bit = 0; bit < prec; bit++) {
    P_CHECKC(PRG_share_int(
      prgB, &shareA->bits->data[bit], &src->bits->data[bit], cfg));
    P_CHECKC(PRG_e_int_accum_x(shareA, bit, &tmp, mod));
  }

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

SECStatus
PRG_share_e_array(PRG prgB,
                  EIntArray arrA,
                  const_EIntArray src,
                  const_PrioConfig cfg)
{
  SECStatus rv = SECSuccess;
  if (arrA->len != src->len)
    return SECFailure;

  const int len = src->len;

  for (int i = 0; i < len; i++) {
    P_CHECK(PRG_share_e_int(prgB, arrA->data[i], src->data[i], cfg));
  }

  return rv;
}
