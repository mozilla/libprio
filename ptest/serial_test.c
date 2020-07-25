/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mprio.h>
#include <msgpack.h>
#include <string.h>

#include "mutest.h"
#include "prio/client.h"
#include "prio/config.h"
#include "prio/serial.h"
#include "prio/server.h"
#include "prio/util.h"
#include "test_util.h"

SECStatus
gen_client_packets(const_PrioConfig cfg,
                   PrioPacketClient pA,
                   PrioPacketClient pB)
{
  SECStatus rv = SECSuccess;

  const int ndata = cfg->num_data_fields;
  bool* data_items = NULL;
  PT_CHECKA(data_items = calloc(ndata, sizeof(bool)));

  for (int i = 0; i < ndata; i++) {
    data_items[i] = (i % 3 == 1) || (i % 5 == 3);
  }

  PT_CHECKC(PrioPacketClient_set_data(cfg, data_items, pA, pB));

cleanup:
  if (data_items)
    free(data_items);
  return rv;
}

void
serial_client(int bad)
{
  SECStatus rv = SECSuccess;
  PrioConfig cfg = NULL;
  PrioConfig cfg2 = NULL;
  PrioPacketClient pA = NULL;
  PrioPacketClient pB = NULL;
  PrioPacketClient qA = NULL;
  PrioPacketClient qB = NULL;
  PrioPacketClient tmp = NULL;

  const unsigned char* batch_id1 = (unsigned char*)"my_test_prio_batch1";
  const unsigned char* batch_id2 = (unsigned char*)"my_test_prio_batch2";
  const unsigned int batch_id_len = strlen((char*)batch_id1);

  msgpack_sbuffer sbufA, sbufB;
  msgpack_packer pkA, pkB;
  msgpack_unpacker upkA, upkB;

  msgpack_sbuffer_init(&sbufA);
  msgpack_packer_init(&pkA, &sbufA, msgpack_sbuffer_write);

  msgpack_sbuffer_init(&sbufB);
  msgpack_packer_init(&pkB, &sbufB, msgpack_sbuffer_write);

  PT_CHECKA(cfg = PrioConfig_new(100, NULL, NULL, batch_id1, batch_id_len));
  PT_CHECKA(cfg2 = PrioConfig_new(100, NULL, NULL, batch_id2, batch_id_len));
  PT_CHECKA(pA = PrioPacketClient_new(cfg, PRIO_SERVER_A));
  PT_CHECKA(pB = PrioPacketClient_new(cfg, PRIO_SERVER_B));
  PT_CHECKA(qA = PrioPacketClient_new(cfg, PRIO_SERVER_A));
  PT_CHECKA(qB = PrioPacketClient_new(cfg, PRIO_SERVER_B));

  PT_CHECKC(gen_client_packets(cfg, pA, pB));

  PT_CHECKC(serial_write_packet_client(&pkA, pA, cfg));
  PT_CHECKC(serial_write_packet_client(&pkB, pB, cfg));

  if (bad == 1) {
    sbufA.size = 1;
  }

  if (bad == 2) {
    memset(sbufA.data, 0, sbufA.size);
  }

  const int size_a = sbufA.size;
  const int size_b = sbufB.size;

  PT_CHECKCB(msgpack_unpacker_init(&upkA, 0));
  PT_CHECKCB(msgpack_unpacker_init(&upkB, 0));

  PT_CHECKCB(msgpack_unpacker_reserve_buffer(&upkA, size_a));
  PT_CHECKCB(msgpack_unpacker_reserve_buffer(&upkB, size_b));

  memcpy(msgpack_unpacker_buffer(&upkA), sbufA.data, size_a);
  memcpy(msgpack_unpacker_buffer(&upkB), sbufB.data, size_b);

  msgpack_unpacker_buffer_consumed(&upkA, size_a);
  msgpack_unpacker_buffer_consumed(&upkB, size_b);

  if (bad == 4) {
    // Swap qA and aB
    tmp = qA;
    qA = qB;
    qB = tmp;
    tmp = NULL;
  }

  P_CHECKC(serial_read_packet_client(&upkA, qA, cfg));
  P_CHECKC(serial_read_packet_client(&upkB, qB, (bad == 3) ? cfg2 : cfg));

  if (!bad) {
    mu_check(PrioPacketClient_areEqual(pA, qA));
    mu_check(PrioPacketClient_areEqual(pB, qB));
    mu_check(!PrioPacketClient_areEqual(pB, qA));
    mu_check(!PrioPacketClient_areEqual(pA, qB));
  }

cleanup:
  PrioPacketClient_clear(pA);
  PrioPacketClient_clear(pB);
  PrioPacketClient_clear(qA);
  PrioPacketClient_clear(qB);
  PrioConfig_clear(cfg);
  PrioConfig_clear(cfg2);
  msgpack_sbuffer_destroy(&sbufA);
  msgpack_sbuffer_destroy(&sbufB);
  msgpack_unpacker_destroy(&upkA);
  msgpack_unpacker_destroy(&upkB);
  mu_check(bad ? rv == SECFailure : rv == SECSuccess);
}

void
mu_test__serial_client(void)
{
  serial_client(0);
}

void
mu_test__serial_client_bad1(void)
{
  serial_client(1);
}

void
mu_test__serial_client_bad2(void)
{
  serial_client(2);
}

void
mu_test__serial_client_bad3(void)
{
  serial_client(3);
}

void
mu_test__serial_client_bad4(void)
{
  serial_client(4);
}

void
test_server(int bad)
{
  SECStatus rv = SECSuccess;
  PublicKey pkA = NULL;
  PrivateKey skA = NULL;
  PrioServer s1 = NULL;
  PrioServer s2 = NULL;
  PrioConfig cfg = NULL;
  PrioPRGSeed seed;

  PT_CHECKC(PrioPRGSeed_randomize(&seed));
  PT_CHECKC(Keypair_new(&skA, &pkA));

  PT_CHECKA(cfg = PrioConfig_newTest(2));
  PT_CHECKA(s1 = PrioServer_new(cfg, 0, skA, seed));
  PT_CHECKA(s2 = PrioServer_new(cfg, 0, skA, seed));

  mp_set(&s1->data_shares->data[0], 4);
  mp_set(&s1->data_shares->data[1], 10);

  msgpack_sbuffer sbuf;
  msgpack_packer pk;
  msgpack_unpacker upk;

  msgpack_sbuffer_init(&sbuf);
  msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

  PT_CHECKC(PrioServer_write(s1, &pk));

  if (bad == 1) {
    mp_set(&cfg->modulus, 6);
  }

  PT_CHECKCB(msgpack_unpacker_init(&upk, 0));
  PT_CHECKCB(msgpack_unpacker_reserve_buffer(&upk, sbuf.size));
  memcpy(msgpack_unpacker_buffer(&upk), sbuf.data, sbuf.size);
  msgpack_unpacker_buffer_consumed(&upk, sbuf.size);

  P_CHECKC(PrioServer_read(s2, &upk, cfg));

  mu_check(!mp_cmp(&s2->data_shares->data[0], &s1->data_shares->data[0]));
  mu_check(!mp_cmp(&s2->data_shares->data[1], &s1->data_shares->data[1]));
  mu_check(!mp_cmp_d(&s2->data_shares->data[0], 4));
  mu_check(!mp_cmp_d(&s2->data_shares->data[1], 10));

cleanup:
  mu_check(bad ? rv == SECFailure : rv == SECSuccess);
  PublicKey_clear(pkA);
  PrivateKey_clear(skA);
  PrioConfig_clear(cfg);
  PrioServer_clear(s1);
  PrioServer_clear(s2);
  msgpack_unpacker_destroy(&upk);
  msgpack_sbuffer_destroy(&sbuf);
}

void
mu_test_server_good(void)
{
  test_server(0);
}

void
mu_test_server_bad(void)
{
  test_server(1);
}

void
test_verify1(int bad)
{
  SECStatus rv = SECSuccess;
  PrioPacketVerify1 v1 = NULL;
  PrioPacketVerify1 v2 = NULL;
  PrioConfig cfg = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(1));
  PT_CHECKA(v1 = PrioPacketVerify1_new());
  PT_CHECKA(v2 = PrioPacketVerify1_new());
  mp_set(&v1->share_d, 4);
  mp_set(&v1->share_e, 10);

  msgpack_sbuffer sbuf;
  msgpack_packer pk;
  msgpack_unpacker upk;

  msgpack_sbuffer_init(&sbuf);
  msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

  PT_CHECKC(PrioPacketVerify1_write(v1, &pk));

  if (bad == 1) {
    mp_set(&cfg->modulus, 6);
  }

  PT_CHECKCB(msgpack_unpacker_init(&upk, 0));
  PT_CHECKCB(msgpack_unpacker_reserve_buffer(&upk, sbuf.size));
  memcpy(msgpack_unpacker_buffer(&upk), sbuf.data, sbuf.size);
  msgpack_unpacker_buffer_consumed(&upk, sbuf.size);

  P_CHECKC(PrioPacketVerify1_read(v2, &upk, cfg));

  mu_check(!mp_cmp(&v1->share_d, &v2->share_d));
  mu_check(!mp_cmp(&v1->share_e, &v2->share_e));
  mu_check(!mp_cmp_d(&v2->share_d, 4));
  mu_check(!mp_cmp_d(&v2->share_e, 10));

cleanup:
  mu_check(bad ? rv == SECFailure : rv == SECSuccess);
  PrioConfig_clear(cfg);
  PrioPacketVerify1_clear(v1);
  PrioPacketVerify1_clear(v2);
  msgpack_unpacker_destroy(&upk);
  msgpack_sbuffer_destroy(&sbuf);
}

void
mu_test_verify1_good(void)
{
  test_verify1(0);
}

void
mu_test_verify1_bad(void)
{
  test_verify1(1);
}

void
test_verify2(int bad)
{
  SECStatus rv = SECSuccess;
  PrioPacketVerify2 v1 = NULL;
  PrioPacketVerify2 v2 = NULL;
  PrioConfig cfg = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest(1));
  PT_CHECKA(v1 = PrioPacketVerify2_new());
  PT_CHECKA(v2 = PrioPacketVerify2_new());
  mp_set(&v1->share_out, 4);

  msgpack_sbuffer sbuf;
  msgpack_packer pk;
  msgpack_unpacker upk;

  msgpack_sbuffer_init(&sbuf);
  msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

  PT_CHECKC(PrioPacketVerify2_write(v1, &pk));

  if (bad == 1) {
    mp_set(&cfg->modulus, 4);
  }

  PT_CHECKCB(msgpack_unpacker_init(&upk, 0));
  PT_CHECKCB(msgpack_unpacker_reserve_buffer(&upk, sbuf.size));
  memcpy(msgpack_unpacker_buffer(&upk), sbuf.data, sbuf.size);
  msgpack_unpacker_buffer_consumed(&upk, sbuf.size);

  P_CHECKC(PrioPacketVerify2_read(v2, &upk, cfg));

  mu_check(!mp_cmp(&v1->share_out, &v2->share_out));
  mu_check(!mp_cmp_d(&v2->share_out, 4));

cleanup:
  mu_check(bad ? rv == SECFailure : rv == SECSuccess);
  PrioConfig_clear(cfg);
  PrioPacketVerify2_clear(v1);
  PrioPacketVerify2_clear(v2);
  msgpack_unpacker_destroy(&upk);
  msgpack_sbuffer_destroy(&sbuf);
}

void
mu_test_verify2_good(void)
{
  test_verify2(0);
}

void
mu_test_verify2_bad(void)
{
  test_verify2(1);
}

void
test_total_share(int bad)
{
  SECStatus rv = SECSuccess;
  PrioTotalShare t1 = NULL;
  PrioTotalShare t2 = NULL;
  PrioConfig cfg = NULL;

  PT_CHECKA(cfg = PrioConfig_newTest((bad == 2 ? 4 : 3)));
  PT_CHECKA(t1 = PrioTotalShare_new());
  PT_CHECKA(t2 = PrioTotalShare_new());

  t1->idx = PRIO_SERVER_A;
  PT_CHECKC(MPArray_resize(t1->data_shares, 3));

  mp_set(&t1->data_shares->data[0], 10);
  mp_set(&t1->data_shares->data[1], 20);
  mp_set(&t1->data_shares->data[2], 30);

  msgpack_sbuffer sbuf;
  msgpack_packer pk;
  msgpack_unpacker upk;

  msgpack_sbuffer_init(&sbuf);
  msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

  PT_CHECKC(PrioTotalShare_write(t1, &pk));

  if (bad == 1) {
    mp_set(&cfg->modulus, 4);
  }

  PT_CHECKCB(msgpack_unpacker_init(&upk, 0));
  PT_CHECKCB(msgpack_unpacker_reserve_buffer(&upk, sbuf.size));
  memcpy(msgpack_unpacker_buffer(&upk), sbuf.data, sbuf.size);
  msgpack_unpacker_buffer_consumed(&upk, sbuf.size);

  P_CHECKC(PrioTotalShare_read(t2, &upk, cfg));

  mu_check(t1->idx == t2->idx);
  mu_check(MPArray_areEqual(t1->data_shares, t2->data_shares));

cleanup:
  mu_check(bad ? rv == SECFailure : rv == SECSuccess);
  PrioConfig_clear(cfg);
  PrioTotalShare_clear(t1);
  PrioTotalShare_clear(t2);
  msgpack_unpacker_destroy(&upk);
  msgpack_sbuffer_destroy(&sbuf);
}

void
mu_test_total_good(void)
{
  test_total_share(0);
}

void
mu_test_total_bad1(void)
{
  test_total_share(1);
}

void
mu_test_total_bad2(void)
{
  test_total_share(2);
}
