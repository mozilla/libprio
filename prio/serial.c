/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 */

#include <mprio.h>
#include <msgpack.h>
#include "client.h"
#include "serial.h"
#include "server.h"
#include "share.h"
#include "util.h"

#define MSGPACK_OK 0

static SECStatus 
serial_write_mp_int (msgpack_packer *pk, const mp_int *n)
{
  SECStatus rv = SECSuccess;
  unsigned int n_size = mp_unsigned_octet_size (n);  

  unsigned char data[n_size];
  MP_CHECK (mp_to_fixlen_octets (n, data, n_size));

  P_CHECK (msgpack_pack_str (pk, n_size));
  P_CHECK (msgpack_pack_str_body (pk, data, n_size));
  return rv;
}

static SECStatus
object_to_mp_int (msgpack_object *obj, mp_int *n, const mp_int *max)
{
  SECStatus rv = SECSuccess;
  P_CHECKCB (obj->type == MSGPACK_OBJECT_STR);

  msgpack_object_str s = obj->via.str;
  MP_CHECKC (mp_read_unsigned_octets (n, (unsigned char *)s.ptr, s.size));

  P_CHECKCB (mp_cmp_z (n) >= 0);
  P_CHECKCB (mp_cmp (n, max) < 0);

cleanup:
  return rv;
}

static SECStatus 
serial_read_mp_int (msgpack_unpacker *upk, mp_int *n, const mp_int *max)
{
  SECStatus rv = SECSuccess;
  msgpack_unpacked res;
  msgpack_unpacked_init (&res);
  UP_CHECK (msgpack_unpacker_next (upk, &res))

  msgpack_object obj = res.data;
  P_CHECKC (object_to_mp_int (&obj, n, max));

cleanup:
  msgpack_unpacked_destroy (&res);

  return rv;
}

static SECStatus 
serial_read_int (msgpack_unpacker *upk, int *n)
{
  SECStatus rv = SECSuccess;
  msgpack_unpacked res;
  msgpack_unpacked_init (&res);
  UP_CHECK (msgpack_unpacker_next (upk, &res))

  msgpack_object obj = res.data;
  P_CHECKCB (obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER);

  *n = obj.via.i64;

cleanup:
  msgpack_unpacked_destroy (&res);

  return rv;
}

static SECStatus 
serial_write_mp_array (msgpack_packer *pk, const_MPArray arr)
{
  SECStatus rv = SECSuccess;
  P_CHECK (msgpack_pack_array (pk, arr->len));
  for (int i = 0; i < arr->len; i++) {
    P_CHECK (serial_write_mp_int (pk, &arr->data[i]));
  }
  return rv;
}

static SECStatus 
serial_read_mp_array (msgpack_unpacker *upk, MPArray arr, size_t len, const mp_int
    *max)
{
  SECStatus rv = SECSuccess;
  msgpack_unpacked res;
  msgpack_unpacked_init (&res);
  UP_CHECK (msgpack_unpacker_next (upk, &res))

  msgpack_object obj = res.data;
  P_CHECKCB (obj.type == MSGPACK_OBJECT_ARRAY);

  msgpack_object_array objarr = obj.via.array;
  P_CHECKCB (objarr.size == len);

  P_CHECKC (MPArray_resize (arr, len));
  for (unsigned int i=0; i<len; i++) {
    P_CHECKC (object_to_mp_int (&objarr.ptr[i], &arr->data[i], max));
  }

cleanup:
  msgpack_unpacked_destroy (&res);

  return rv;
}


static SECStatus 
serial_write_beaver_triple (msgpack_packer *pk, const_BeaverTriple t)
{
  SECStatus rv = SECSuccess;
  P_CHECK (serial_write_mp_int (pk, &t->a)); 
  P_CHECK (serial_write_mp_int (pk, &t->b)); 
  P_CHECK (serial_write_mp_int (pk, &t->c)); 
  return rv;
}

static SECStatus 
serial_read_beaver_triple (msgpack_unpacker *pk, BeaverTriple t, const mp_int *max)
{
  SECStatus rv = SECSuccess;
  P_CHECK (serial_read_mp_int (pk, &t->a, max)); 
  P_CHECK (serial_read_mp_int (pk, &t->b, max)); 
  P_CHECK (serial_read_mp_int (pk, &t->c, max)); 
  return rv;
}

static SECStatus 
serial_write_server_a_data (msgpack_packer *pk, const struct server_a_data *A)
{
  SECStatus rv = SECSuccess;
  P_CHECK (serial_write_mp_array (pk, A->data_shares)); 
  P_CHECK (serial_write_mp_array (pk, A->h_points)); 
  return rv;
}

static SECStatus 
serial_read_server_a_data (msgpack_unpacker *upk, struct server_a_data *A,
    const_PrioConfig cfg)
{
  SECStatus rv = SECSuccess;
  P_CHECK (serial_read_mp_array (upk, A->data_shares, cfg->num_data_fields,
        &cfg->modulus)); 
  P_CHECK (serial_read_mp_array (upk, A->h_points, PrioConfig_hPoints (cfg),
        &cfg->modulus)); 
  return rv;
}

static SECStatus 
serial_write_prg_seed (msgpack_packer *pk, const PrioPRGSeed *seed)
{
  SECStatus rv = SECSuccess;
  P_CHECK (msgpack_pack_str (pk, PRG_SEED_LENGTH));
  P_CHECK (msgpack_pack_str_body (pk, seed, PRG_SEED_LENGTH));
  return rv;
}

static SECStatus 
serial_read_prg_seed (msgpack_unpacker *upk, PrioPRGSeed *seed)
{
  SECStatus rv = SECSuccess;
  msgpack_unpacked res;
  msgpack_unpacked_init (&res);
  UP_CHECK (msgpack_unpacker_next (upk, &res))

  msgpack_object obj = res.data;
  P_CHECKCB (obj.type == MSGPACK_OBJECT_STR);

  msgpack_object_str s = obj.via.str;
  P_CHECKCB (s.size == PRG_SEED_LENGTH);
  memcpy (seed, s.ptr, PRG_SEED_LENGTH);

cleanup:
  msgpack_unpacked_destroy (&res);

  return rv;
}

static SECStatus 
serial_write_server_b_data (msgpack_packer *pk, const struct server_b_data *B)
{
  return serial_write_prg_seed (pk, &B->seed);
}

static SECStatus 
serial_read_server_b_data (msgpack_unpacker *upk, struct server_b_data *B)
{
  return serial_read_prg_seed (upk, &B->seed);
}

SECStatus 
serial_write_packet_client (msgpack_packer *pk, const_PrioPacketClient p,
    const_PrioConfig cfg)
{
  SECStatus rv = SECSuccess;
  P_CHECK (msgpack_pack_str (pk, cfg->batch_id_len));
  P_CHECK (msgpack_pack_str_body (pk, cfg->batch_id, cfg->batch_id_len));

  P_CHECK (serial_write_beaver_triple (pk, p->triple));

  P_CHECK (serial_write_mp_int (pk, &p->f0_share));
  P_CHECK (serial_write_mp_int (pk, &p->g0_share));
  P_CHECK (serial_write_mp_int (pk, &p->h0_share));

  P_CHECK (msgpack_pack_int (pk, p->for_server));

  switch (p->for_server) {
    case PRIO_SERVER_A:
      P_CHECK (serial_write_server_a_data (pk, &p->shares.A));
      break;
    case PRIO_SERVER_B:
      P_CHECK (serial_write_server_b_data (pk, &p->shares.B));
      break;
    default:
      return SECFailure;
  }

  return rv;
}

SECStatus 
serial_read_server_id (msgpack_unpacker *upk, PrioServerId *s)
{
  SECStatus rv = SECSuccess;
  int serv;
  P_CHECK (serial_read_int (upk, &serv));
  P_CHECKCB (serv == PRIO_SERVER_A || serv == PRIO_SERVER_B);
  *s = serv;

cleanup:
  return rv;
}

SECStatus 
serial_read_packet_client (msgpack_unpacker *upk, PrioPacketClient p,
    const_PrioConfig cfg)
{
  SECStatus rv = SECSuccess;
  msgpack_unpacked res;
  msgpack_unpacked_init (&res);
  UP_CHECK (msgpack_unpacker_next (upk, &res))

  msgpack_object obj = res.data;
  P_CHECKCB (obj.type == MSGPACK_OBJECT_STR);

  msgpack_object_str s = obj.via.str;
  P_CHECKCB (s.size == cfg->batch_id_len);
  P_CHECKCB (!memcmp (s.ptr, (char *)cfg->batch_id, cfg->batch_id_len));

  P_CHECK (serial_read_beaver_triple (upk, p->triple, &cfg->modulus));

  P_CHECK (serial_read_mp_int (upk, &p->f0_share, &cfg->modulus));
  P_CHECK (serial_read_mp_int (upk, &p->g0_share, &cfg->modulus));
  P_CHECK (serial_read_mp_int (upk, &p->h0_share, &cfg->modulus));

  P_CHECK (serial_read_server_id (upk, &p->for_server));

  switch (p->for_server) {
    case PRIO_SERVER_A:
      P_CHECK (serial_read_server_a_data (upk, &p->shares.A, cfg));
      break;
    case PRIO_SERVER_B:
      P_CHECK (serial_read_server_b_data (upk, &p->shares.B));
      break;
    default:
      return SECFailure;
  }

cleanup:
  msgpack_unpacked_destroy (&res);
  return rv;
}


SECStatus 
PrioPacketVerify1_write (const_PrioPacketVerify1 p, msgpack_packer *pk)
{
  SECStatus rv = SECSuccess;
  P_CHECK (serial_write_mp_int (pk, &p->share_d));
  P_CHECK (serial_write_mp_int (pk, &p->share_e));

  return rv;
}

SECStatus 
PrioPacketVerify1_read (PrioPacketVerify1 p, msgpack_unpacker *upk,
    const_PrioConfig cfg)
{
  SECStatus rv = SECSuccess;
  P_CHECK (serial_read_mp_int (upk, &p->share_d, &cfg->modulus));
  P_CHECK (serial_read_mp_int (upk, &p->share_e, &cfg->modulus));

  return rv;
}

SECStatus 
PrioPacketVerify2_write (const_PrioPacketVerify2 p, msgpack_packer *pk)
{
  SECStatus rv = SECSuccess;
  P_CHECK (serial_write_mp_int (pk, &p->share_out));

  return rv;
}

SECStatus 
PrioPacketVerify2_read (PrioPacketVerify2 p, msgpack_unpacker *upk,
    const_PrioConfig cfg)
{
  SECStatus rv = SECSuccess;
  P_CHECK (serial_read_mp_int (upk, &p->share_out, &cfg->modulus));

  return rv;
}

SECStatus 
PrioTotalShare_write (const_PrioTotalShare t, msgpack_packer *pk)
{
  SECStatus rv = SECSuccess;
  P_CHECK (msgpack_pack_int (pk, t->idx));
  P_CHECK (serial_write_mp_array (pk, t->data_shares));

  return rv;
}

SECStatus 
PrioTotalShare_read (PrioTotalShare t, msgpack_unpacker *upk, 
    const_PrioConfig cfg)
{
  SECStatus rv = SECSuccess;
  P_CHECK (serial_read_server_id (upk, &t->idx));
  P_CHECK (serial_read_mp_array (upk, t->data_shares, cfg->num_data_fields,
        &cfg->modulus));

  return rv;
}

