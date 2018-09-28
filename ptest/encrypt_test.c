/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cert.h>
#include <keyhi.h>
#include <nspr.h>
#include <nss.h>
#include <pk11pub.h>
#include <secoidt.h>

#include "mutest.h"
#include "prio/encrypt.h"
#include "prio/rand.h"
#include "prio/util.h"
#include "test_util.h"

void
mu_test_keygen(void)
{
  SECStatus rv = SECSuccess;
  PublicKey pubkey = NULL;
  PrivateKey pvtkey = NULL;

  PT_CHECKC(Keypair_new(&pvtkey, &pubkey));
  mu_check(SECKEY_PublicKeyStrength(pubkey) == 32);

cleanup:
  mu_check(rv == SECSuccess);
  PublicKey_clear(pubkey);
  PrivateKey_clear(pvtkey);
  return;
}

void
test_encrypt_once(int bad, unsigned int inlen)
{
  SECStatus rv = SECSuccess;
  PublicKey pubkey = NULL;
  PrivateKey pvtkey = NULL;
  PublicKey pubkey2 = NULL;
  PrivateKey pvtkey2 = NULL;

  unsigned char* bytes_in = NULL;
  unsigned char* bytes_enc = NULL;
  unsigned char* bytes_dec = NULL;

  unsigned int enclen;
  PT_CHECKC(PublicKey_encryptSize(inlen, &enclen));
  unsigned int declen = enclen;

  P_CHECKA(bytes_in = malloc(inlen));
  P_CHECKA(bytes_enc = malloc(enclen));
  P_CHECKA(bytes_dec = malloc(enclen));
  PT_CHECKC(rand_bytes(bytes_in, inlen));

  memset(bytes_dec, 0, declen);

  unsigned int encryptedBytes;
  PT_CHECKC(Keypair_new(&pvtkey, &pubkey));
  PT_CHECKC(Keypair_new(&pvtkey2, &pubkey2));
  PT_CHECKC(PublicKey_encrypt(pubkey, bytes_enc, &encryptedBytes, enclen,
                              bytes_in, inlen));
  mu_check(encryptedBytes == enclen);

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
  P_CHECKC(PrivateKey_decrypt(key_to_use, bytes_dec, &decryptedBytes, declen,
                              bytes_enc, enclen));
  mu_check(decryptedBytes == inlen);
  mu_check(!strncmp((char*)bytes_in, (char*)bytes_dec, inlen));

cleanup:
  mu_check(bad ? (rv == SECFailure) : (rv == SECSuccess));
  if (bytes_in)
    free(bytes_in);
  if (bytes_enc)
    free(bytes_enc);
  if (bytes_dec)
    free(bytes_dec);

  PublicKey_clear(pubkey);
  PrivateKey_clear(pvtkey);
  PublicKey_clear(pubkey2);
  PrivateKey_clear(pvtkey2);
  return;
}

void
mu_test_encrypt_good(void)
{
  test_encrypt_once(0, 100);
}

void
mu_test_encrypt_good_long(void)
{
  test_encrypt_once(0, 1000000);
}

void
mu_test_encrypt_too_short(void)
{
  test_encrypt_once(1, 87);
}

void
mu_test_encrypt_garbage(void)
{
  test_encrypt_once(2, 10023);
}

void
mu_test_encrypt_garbage2(void)
{
  test_encrypt_once(3, 8123);
}

void
mu_test_decrypt_wrong_key(void)
{
  test_encrypt_once(4, 81230);
}

void
mu_test_export_pubkey(void)
{
  SECStatus rv = SECSuccess;
  PublicKey pubkey = NULL;

  unsigned char raw_bytes[CURVE25519_KEY_LEN];
  unsigned char raw_bytes2[CURVE25519_KEY_LEN];
  for (int i = 0; i < CURVE25519_KEY_LEN; i++) {
    raw_bytes[i] = (3 * i + 7) % 0xFF;
  }

  PT_CHECKC(PublicKey_import(&pubkey, raw_bytes, CURVE25519_KEY_LEN));
  PT_CHECKC(PublicKey_export(pubkey, raw_bytes2));

  for (int i = 0; i < CURVE25519_KEY_LEN; i++) {
    mu_check(raw_bytes[i] == raw_bytes2[i]);
  }

cleanup:
  mu_check(rv == SECSuccess);
  PublicKey_clear(pubkey);
  return;
}

void
mu_test_export_pubkey_zeros(void)
{
  SECStatus rv = SECSuccess;
  PublicKey pubkey = NULL;

  unsigned char raw_bytes[CURVE25519_KEY_LEN];
  unsigned char raw_bytes2[CURVE25519_KEY_LEN];
  for (int i = 0; i < CURVE25519_KEY_LEN; i++) {
    raw_bytes[i] = (3 * i + 7) % 0xFF;
  }

  for (int i = 0; i < 15; i++) {
    raw_bytes[i] = 0;
  }

  PT_CHECKC(PublicKey_import(&pubkey, raw_bytes, CURVE25519_KEY_LEN));
  PT_CHECKC(PublicKey_export(pubkey, raw_bytes2));

  for (int i = 0; i < CURVE25519_KEY_LEN; i++) {
    mu_check(raw_bytes[i] == raw_bytes2[i]);
  }

cleanup:
  mu_check(rv == SECSuccess);
  PublicKey_clear(pubkey);
  return;
}

void
test_export_privkey(int zeros)
{
  SECStatus rv = SECSuccess;
  PublicKey pubkey = NULL;
  PrivateKey pvtkey = NULL;
  PrivateKey pvtkey_imp = NULL;
  unsigned char privData[CURVE25519_KEY_LEN];
  unsigned char privData2[CURVE25519_KEY_LEN];
  unsigned char pubData[CURVE25519_KEY_LEN];
  unsigned char input[67];
  unsigned char output[256];
  unsigned char decrypt[256];

  for (unsigned int i = 0; i < sizeof(input); i++) {
    input[i] = i;
  }

  PT_CHECKC(Keypair_new(&pvtkey, &pubkey));
  PT_CHECKC(PrivateKey_export(pvtkey, privData));
  PT_CHECKC(PublicKey_export(pubkey, pubData));

  if (zeros) {
    // Zero out leading bytes of private key
    memset(privData, 0, 5);
  }

  PT_CHECKC(PrivateKey_import(&pvtkey_imp, privData, CURVE25519_KEY_LEN,
                              pubData, CURVE25519_KEY_LEN));
  PT_CHECKC(PrivateKey_export(pvtkey_imp, privData2));

  mu_check(!memcmp(privData, privData2, CURVE25519_KEY_LEN));

  if (!zeros) {
    unsigned int outputLen;
    PT_CHECKC(PublicKey_encrypt(pubkey, output, &outputLen, sizeof(output),
                                input, sizeof(input)));

    // Check that can decrypt with imported private key.
    unsigned int plainLen;
    PT_CHECKC(PrivateKey_decrypt(pvtkey_imp, decrypt, &plainLen,
                                 sizeof(decrypt), output, outputLen));

    mu_check(plainLen == sizeof(input));
    mu_check(!memcmp(input, decrypt, sizeof(input)));
  }

cleanup:
  mu_check(rv == SECSuccess);
  PublicKey_clear(pubkey);
  PrivateKey_clear(pvtkey);
  PrivateKey_clear(pvtkey_imp);
  return;
}

void
mu_test_export_privkey(void)
{
  test_export_privkey(0);
}

void
mu_test_export_privkey_zeros(void)
{
  test_export_privkey(1);
}

void
test_export_hex_privkey(int zeros)
{
  SECStatus rv = SECSuccess;
  PublicKey pubkey = NULL;
  PrivateKey pvtkey = NULL;
  PrivateKey pvtkey_imp = NULL;
  unsigned char privData[CURVE25519_KEY_LEN_HEX + 1];
  unsigned char privData2[CURVE25519_KEY_LEN_HEX + 1];
  unsigned char pubData[CURVE25519_KEY_LEN_HEX + 1];
  unsigned char input[67];
  unsigned char output[256];
  unsigned char decrypt[256];

  for (unsigned int i = 0; i < sizeof(input); i++) {
    input[i] = i;
  }

  PT_CHECKC(Keypair_new(&pvtkey, &pubkey));
  PT_CHECKC(PrivateKey_export_hex(pvtkey, privData));
  PT_CHECKC(PublicKey_export_hex(pubkey, pubData));

  if (zeros) {
    // Zero out leading bytes of private key with ASCII zero
    memset(privData, '0', 5);
  }

  PT_CHECKC(PrivateKey_import_hex(&pvtkey_imp, privData, CURVE25519_KEY_LEN_HEX,
                                  pubData, CURVE25519_KEY_LEN_HEX));
  PT_CHECKC(PrivateKey_export_hex(pvtkey_imp, privData2));

  mu_check(!memcmp(privData, privData2, CURVE25519_KEY_LEN));

  if (!zeros) {
    unsigned int outputLen;
    PT_CHECKC(PublicKey_encrypt(pubkey, output, &outputLen, sizeof(output),
                                input, sizeof(input)));

    // Check that can decrypt with imported private key.
    unsigned int plainLen;
    PT_CHECKC(PrivateKey_decrypt(pvtkey_imp, decrypt, &plainLen,
                                 sizeof(decrypt), output, outputLen));

    mu_check(plainLen == sizeof(input));
    mu_check(!memcmp(input, decrypt, sizeof(input)));
  }

cleanup:
  mu_check(rv == SECSuccess);
  PublicKey_clear(pubkey);
  PrivateKey_clear(pvtkey);
  PrivateKey_clear(pvtkey_imp);
  return;
}

void
mu_test_export_hex_privkey(void)
{
  test_export_hex_privkey(0);
}

void
mu_test_export_hex_privkey_zeros(void)
{
  test_export_hex_privkey(1);
}

void
mu_test_import_hex_wrongsize(void)
{
  PrivateKey pvtkey = NULL;
  unsigned char privData[CURVE25519_KEY_LEN + 1];

  mu_check(PrivateKey_import_hex(&pvtkey, privData, 7, privData, 8) ==
           SECFailure);
  mu_check(PrivateKey_import_hex(&pvtkey, privData, 32, privData, 1024) ==
           SECFailure);
  mu_check(PrivateKey_import_hex(&pvtkey, privData, 31, privData, 32) ==
           SECFailure);
  mu_check(PrivateKey_import_hex(&pvtkey, NULL, 32, privData, 32) ==
           SECFailure);
  mu_check(PrivateKey_import_hex(&pvtkey, privData, 32, NULL, 32) ==
           SECFailure);
  return;
}

void
mu_test_export_hex(void)
{
  SECStatus rv = SECSuccess;
  PublicKey pubkey = NULL;

  const unsigned char hex_bytes[2 * CURVE25519_KEY_LEN] =
    "102030405060708090A0B0C0D0E0F00000FFEEDDCCBBAA998877665544332211";
  const unsigned char hex_bytesl[2 * CURVE25519_KEY_LEN] =
    "102030405060708090a0B0C0D0E0F00000FfeEddcCbBaa998877665544332211";

  const unsigned char raw_bytes_should[CURVE25519_KEY_LEN] = {
    0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0,
    0xC0, 0xD0, 0xE0, 0xF0, 0x00, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB,
    0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
  };
  unsigned char raw_bytes[CURVE25519_KEY_LEN];
  unsigned char hex_bytes2[2 * CURVE25519_KEY_LEN + 1];

  // Make sure that invalid lengths are rejected.
  mu_check(PublicKey_import_hex(&pubkey, hex_bytes,
                                2 * CURVE25519_KEY_LEN - 1) == SECFailure);
  mu_check(PublicKey_import_hex(&pubkey, hex_bytes,
                                2 * CURVE25519_KEY_LEN + 1) == SECFailure);

  // Import a key in upper-case hex
  PT_CHECKC(PublicKey_import_hex(&pubkey, hex_bytes, 2 * CURVE25519_KEY_LEN));
  PT_CHECKC(PublicKey_export(pubkey, raw_bytes));
  PublicKey_clear(pubkey);
  pubkey = NULL;

  for (int i = 0; i < CURVE25519_KEY_LEN; i++) {
    mu_check(raw_bytes[i] == raw_bytes_should[i]);
  }

  // Import a key in mixed-case hex
  PT_CHECKC(PublicKey_import_hex(&pubkey, hex_bytesl, 2 * CURVE25519_KEY_LEN));
  PT_CHECKC(PublicKey_export(pubkey, raw_bytes));
  PublicKey_clear(pubkey);
  pubkey = NULL;

  for (int i = 0; i < CURVE25519_KEY_LEN; i++) {
    mu_check(raw_bytes[i] == raw_bytes_should[i]);
  }

  mu_check(PublicKey_import(&pubkey, raw_bytes_should,
                            CURVE25519_KEY_LEN - 1) == SECFailure);
  mu_check(PublicKey_import(&pubkey, raw_bytes_should,
                            CURVE25519_KEY_LEN + 1) == SECFailure);

  // Import a raw key and export as hex
  PT_CHECKC(PublicKey_import(&pubkey, raw_bytes_should, CURVE25519_KEY_LEN));
  PT_CHECKC(PublicKey_export_hex(pubkey, hex_bytes2));

  for (int i = 0; i < 2 * CURVE25519_KEY_LEN; i++) {
    mu_check(hex_bytes[i] == hex_bytes2[i]);
  }
  mu_ensure(hex_bytes2[2 * CURVE25519_KEY_LEN] == '\0');

cleanup:
  mu_check(rv == SECSuccess);
  PublicKey_clear(pubkey);
  return;
}
