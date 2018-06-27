/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 */

#include "mutest.h"

#include <nss/nss.h>
#include <nss/secoidt.h>
#include <nspr.h>
#include <nss/keyhi.h>
#include <nss/pk11pub.h>
#include <nss/cert.h>

#include "prio/encrypt.h"
#include "prio/rand.h"
#include "prio/util.h"


void 
mu_test_keygen (void) 
{
  SECStatus rv = SECSuccess;
  PublicKey pubkey = NULL;
  PrivateKey pvtkey = NULL;

  P_CHECKC (Keypair_new (&pvtkey, &pubkey));
  mu_check (SECKEY_PublicKeyStrength (pubkey) == 32);

cleanup:
  mu_check (rv == SECSuccess);
  PublicKey_clear (pubkey);
  PrivateKey_clear (pvtkey);
  return;
}

void 
test_encrypt_once (int bad, unsigned int inlen) 
{
  SECStatus rv = SECSuccess;
  PublicKey pubkey = NULL;
  PrivateKey pvtkey = NULL;
  PublicKey pubkey2 = NULL;
  PrivateKey pvtkey2 = NULL;

  unsigned char *bytes_in = NULL;
  unsigned char *bytes_enc = NULL;
  unsigned char *bytes_dec = NULL;
  
  unsigned int enclen;
  P_CHECKC (PublicKey_encryptSize (inlen, &enclen));
  unsigned int declen = enclen;

  P_CHECKA (bytes_in = malloc (inlen));
  P_CHECKA (bytes_enc = malloc (enclen));
  P_CHECKA (bytes_dec= malloc (enclen));
  P_CHECKC (rand_bytes (bytes_in, inlen));

  memset (bytes_dec, 0, declen);

  unsigned int encryptedBytes;
  P_CHECKC (Keypair_new (&pvtkey, &pubkey));
  P_CHECKC (Keypair_new (&pvtkey2, &pubkey2));
  P_CHECKC (PublicKey_encrypt (pubkey, bytes_enc, 
        &encryptedBytes, enclen,
        bytes_in, inlen));
  mu_check (encryptedBytes == enclen);

  if (bad == 1) 
    enclen = 30; 

  if (bad == 2) {
    bytes_enc[4] = 6;
    bytes_enc[5] = 0;
  }

  if (bad == 3) {
    bytes_enc[40] = 6;
    bytes_enc[41] = 0;
  }

  unsigned int decryptedBytes;
  PrivateKey key_to_use = (bad == 4) ? pvtkey2 : pvtkey;
  P_CHECKC (PrivateKey_decrypt (key_to_use, bytes_dec, &decryptedBytes, declen,
        bytes_enc, enclen));
  mu_check (decryptedBytes == inlen);
  mu_check (!strncmp ((char *)bytes_in, (char *)bytes_dec, inlen));

cleanup:
  mu_check (bad ? (rv == SECFailure) : (rv == SECSuccess));
  if (bytes_in) free (bytes_in);
  if (bytes_enc) free (bytes_enc);
  if (bytes_dec) free (bytes_dec);

  PublicKey_clear (pubkey);
  PrivateKey_clear (pvtkey);
  PublicKey_clear (pubkey2);
  PrivateKey_clear (pvtkey2);
  return;
}

void 
mu_test_encrypt_good (void) 
{
  test_encrypt_once (0, 100);
}

void 
mu_test_encrypt_good_long (void) 
{
  test_encrypt_once (0, 1000000);
}

void 
mu_test_encrypt_too_short (void) 
{
  test_encrypt_once (1, 87);
}

void 
mu_test_encrypt_garbage (void) 
{
  test_encrypt_once (2, 10023);
}

void 
mu_test_encrypt_garbage2 (void) 
{
  test_encrypt_once (3, 8123);
}

void 
mu_test_decrypt_wrong_key (void) 
{
  test_encrypt_once (4, 81230);
}

