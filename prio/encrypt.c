/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 */

#include <nss/keyhi.h>
#include <nss/keythi.h>
#include <nss/pk11pub.h>
#include <prerror.h>

#include "encrypt.h"
#include "prio/rand.h"
#include "prio/util.h"

// Use Curve25519
#define CURVE_OID_TAG SEC_OID_CURVE25519
#define CURVE25519_KEY_LEN 32

// Use 96-bit IV
#define GCM_IV_LEN_BYTES 12
// Use 128-bit auth tag
#define GCM_TAG_LEN_BYTES 16

#define PRIO_TAG "PrioPacket"
#define AAD_LEN (strlen (PRIO_TAG) + CURVE25519_KEY_LEN + GCM_IV_LEN_BYTES)

static SECStatus 
derive_dh_secret (PK11SymKey **shared_secret, PrivateKey priv, PublicKey pub)
{
  if (priv == NULL) return SECFailure;
  if (pub == NULL) return SECFailure;
  if (shared_secret == NULL) return SECFailure;

  SECStatus rv = SECSuccess;
  *shared_secret = NULL;

  P_CHECKA (*shared_secret = PK11_PubDeriveWithKDF (priv, pub, PR_FALSE,
        NULL, NULL, CKM_ECDH1_DERIVE, CKM_AES_GCM, 
        CKA_ENCRYPT | CKA_DECRYPT, 16,
        CKD_SHA256_KDF, NULL, NULL));

cleanup:
  return rv;
}

static SECStatus
public_key_import (PublicKey *pk, const unsigned char *data, unsigned int dataLen)
{
  SECStatus rv = SECSuccess;
  PrivateKey eph_priv = NULL;
  *pk = NULL;

  if (dataLen != CURVE25519_KEY_LEN)
    return SECFailure;

  unsigned char key_bytes[dataLen];
  memcpy (key_bytes, data, dataLen);

  // TODO: Horrible hack. We should be able to import a public key
  // directly without having to generate one from scratch.
  P_CHECKC (Keypair_new (&eph_priv, pk));
  memcpy ((*pk)->u.ec.publicValue.data, data, CURVE25519_KEY_LEN);

cleanup:
  PrivateKey_clear (eph_priv);
  if (rv != SECSuccess)
    PublicKey_clear (*pk);
  return rv;
}

SECStatus 
Keypair_new (PrivateKey *pvtkey, PublicKey *pubkey)
{
  if (pvtkey == NULL) return SECFailure;
  if (pubkey == NULL) return SECFailure;

  SECStatus rv = SECSuccess;
  SECOidData *oid_data = NULL;
  *pubkey = NULL;
  *pvtkey = NULL;

  SECKEYECParams ecp;
  ecp.data = NULL;
  PK11SlotInfo *slot = NULL;

  P_CHECKA (oid_data = SECOID_FindOIDByTag (CURVE_OID_TAG));
  const int oid_struct_len = 2 + oid_data->oid.len;

  P_CHECKA (ecp.data = malloc (oid_struct_len));
  ecp.len = oid_struct_len;
  
  ecp.type = siDEROID;

  ecp.data[0] = SEC_ASN1_OBJECT_ID;
  ecp.data[1] = oid_data->oid.len;
  memcpy (&ecp.data[2], oid_data->oid.data, oid_data->oid.len);

  P_CHECKA (slot = PK11_GetInternalSlot ());
  P_CHECKA (*pvtkey = PK11_GenerateKeyPair(slot, CKM_EC_KEY_PAIR_GEN, &ecp, 
      (SECKEYPublicKey **)pubkey, PR_FALSE, PR_FALSE, NULL));

cleanup:
  if (ecp.data)
    free (ecp.data);
  if (rv != SECSuccess) {
    PublicKey_clear (*pubkey);
    PrivateKey_clear (*pvtkey);
  }
  return rv;
}

void 
PublicKey_clear (PublicKey pubkey)
{
  if (pubkey) 
    SECKEY_DestroyPublicKey(pubkey);
}

void 
PrivateKey_clear (PrivateKey pvtkey)
{
  if (pvtkey) 
    SECKEY_DestroyPrivateKey(pvtkey); 
}

const SECItem *
PublicKey_toBytes (const_PublicKey pubkey)
{
  return &pubkey->u.ec.publicValue;
}

SECStatus
PublicKey_encryptSize (unsigned int inputLen, unsigned int *outputLen)
{
  if (outputLen == NULL || inputLen >= MAX_ENCRYPT_LEN)
    return SECFailure;

  // public key, IV, tag, and input
  *outputLen = CURVE25519_KEY_LEN + GCM_IV_LEN_BYTES + GCM_TAG_LEN_BYTES + inputLen;
  return SECSuccess;
}

static void 
set_gcm_params (SECItem *paramItem, CK_GCM_PARAMS *param, unsigned char *nonce,
    const_PublicKey pubkey, unsigned char *aadBuf)
{
  int offset = 0;
  memcpy (aadBuf, PRIO_TAG, strlen (PRIO_TAG));
  offset += strlen (PRIO_TAG);
  memcpy (aadBuf + offset, PublicKey_toBytes (pubkey)->data, CURVE25519_KEY_LEN);
  offset += CURVE25519_KEY_LEN;
  memcpy (aadBuf + offset, nonce, GCM_IV_LEN_BYTES);
 
  param->pIv = nonce;
  param->ulIvLen = GCM_IV_LEN_BYTES;
  param->pAAD = aadBuf;
  param->ulAADLen = AAD_LEN;
  param->ulTagBits = GCM_TAG_LEN_BYTES * 8;

  paramItem->type = siBuffer;
  paramItem->data = (void *)param;
  paramItem->len = sizeof (*param);

}

SECStatus 
PublicKey_encrypt (PublicKey pubkey, 
    unsigned char *output, 
    unsigned int *outputLen, 
    unsigned int maxOutputLen, 
    const unsigned char *input, unsigned int inputLen)
{
  if (pubkey == NULL)
    return SECFailure;

  if (inputLen >= MAX_ENCRYPT_LEN)
    return SECFailure;

  unsigned int needLen;
  if (PublicKey_encryptSize (inputLen, &needLen) != SECSuccess)
    return SECFailure;

  if (maxOutputLen < needLen) 
    return SECFailure;

  SECStatus rv = SECSuccess;
  PublicKey eph_pub = NULL;  
  PrivateKey eph_priv = NULL;  
  PK11SymKey *aes_key = NULL;  

  unsigned char nonce[GCM_IV_LEN_BYTES];
  unsigned char aadBuf[AAD_LEN];
  P_CHECKC (rand_bytes (nonce, GCM_IV_LEN_BYTES));

  P_CHECKC (Keypair_new (&eph_priv, &eph_pub));
  P_CHECKC (derive_dh_secret (&aes_key, eph_priv, pubkey));

  CK_GCM_PARAMS param;
  SECItem paramItem;
  set_gcm_params (&paramItem, &param, nonce, eph_pub, aadBuf);

  const SECItem *pk = PublicKey_toBytes (eph_pub);
  P_CHECKCB (pk->len == CURVE25519_KEY_LEN);
  memcpy (output, pk->data, pk->len);
  memcpy (output + CURVE25519_KEY_LEN, param.pIv, param.ulIvLen);

  const int offset = CURVE25519_KEY_LEN + param.ulIvLen;
  P_CHECKC (PK11_Encrypt (aes_key, CKM_AES_GCM, &paramItem, output + offset,
        outputLen, maxOutputLen - offset, input, inputLen));
  *outputLen = *outputLen + CURVE25519_KEY_LEN + GCM_IV_LEN_BYTES;

cleanup:
  PublicKey_clear (eph_pub);
  PrivateKey_clear (eph_priv);
  if (aes_key) 
    PK11_FreeSymKey (aes_key);

  return rv;
}

SECStatus 
PrivateKey_decrypt (PrivateKey privkey, 
    unsigned char *output, 
    unsigned int *outputLen, 
    unsigned int maxOutputLen, 
    const unsigned char *input, unsigned int inputLen)
{
  PK11SymKey *aes_key = NULL;
  PublicKey eph_pub = NULL;
  unsigned char aad_buf[AAD_LEN];

  if (privkey == NULL)
    return SECFailure;

  SECStatus rv = SECSuccess;
  unsigned int headerLen;
  if (PublicKey_encryptSize (0, &headerLen) != SECSuccess)
    return SECFailure;

  if (inputLen < headerLen) 
    return SECFailure;

  const unsigned int msglen = inputLen - headerLen;
  if (maxOutputLen < msglen || msglen >= MAX_ENCRYPT_LEN)
    return SECFailure;


  P_CHECKC (public_key_import (&eph_pub, input, CURVE25519_KEY_LEN));
  unsigned char nonce[GCM_IV_LEN_BYTES];
  memcpy (nonce, input + CURVE25519_KEY_LEN, GCM_IV_LEN_BYTES);

  SECItem paramItem;
  CK_GCM_PARAMS param;
  set_gcm_params (&paramItem, &param, nonce, eph_pub, aad_buf);
 
  P_CHECKC (derive_dh_secret (&aes_key, privkey, eph_pub));

  const int offset = CURVE25519_KEY_LEN + GCM_IV_LEN_BYTES;
  P_CHECKC (PK11_Decrypt (aes_key, CKM_AES_GCM, &paramItem, output,
        outputLen, maxOutputLen, input + offset, inputLen - offset));

cleanup:
  PublicKey_clear (eph_pub);
  if (aes_key) 
    PK11_FreeSymKey (aes_key);
  return rv;
}

