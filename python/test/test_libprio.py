# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import pytest
from prio import libprio as prio
import array


@pytest.mark.parametrize("n_clients", [1, 2, 10])
def test_client_agg(n_clients):
    seed = prio.PrioPRGSeed_randomize()

    skA, pkA = prio.Keypair_new()
    skB, pkB = prio.Keypair_new()
    cfg = prio.PrioConfig_new(133, pkA, pkB, b"test_batch")
    sA = prio.PrioServer_new(cfg, prio.PRIO_SERVER_A, skA, seed)
    sB = prio.PrioServer_new(cfg, prio.PRIO_SERVER_B, skB, seed)
    vA = prio.PrioVerifier_new(sA)
    vB = prio.PrioVerifier_new(sB)
    tA = prio.PrioTotalShare_new()
    tB = prio.PrioTotalShare_new()

    n_data = prio.PrioConfig_numDataFields(cfg)
    data_items = bytes([(i % 3 == 1) or (i % 5 == 1) for i in range(n_data)])

    for i in range(n_clients):
        for_server_a, for_server_b = prio.PrioClient_encode(cfg, data_items)

        prio.PrioVerifier_set_data(vA, for_server_a)
        prio.PrioVerifier_set_data(vB, for_server_b)

        prio.PrioServer_aggregate(sA, vA)
        prio.PrioServer_aggregate(sB, vB)

    prio.PrioTotalShare_set_data(tA, sA)
    prio.PrioTotalShare_set_data(tB, sB)

    output = array.array('L', prio.PrioTotalShare_final(cfg, tA, tB))

    expected = [item*n_clients for item in list(data_items)]
    assert(list(output) == expected)


def test_publickey_export():
    raw_bytes = bytes((3*x + 7) % 0xFF for x in range(prio.CURVE25519_KEY_LEN))
    pubkey = prio.PublicKey_import(raw_bytes)
    raw_bytes2 = prio.PublicKey_export(pubkey)

    assert raw_bytes == raw_bytes2


@pytest.mark.parametrize("hex_bytes", [
    b"102030405060708090A0B0C0D0E0F00000FFEEDDCCBBAA998877665544332211",
    b"102030405060708090a0B0C0D0E0F00000FfeEddcCbBaa998877665544332211"
])
def test_publickey_import_hex(hex_bytes):
    expect = bytes([
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0,
        0xC0, 0xD0, 0xE0, 0xF0, 0x00, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB,
        0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
    ])
    pubkey = prio.PublicKey_import_hex(hex_bytes)
    raw_bytes = prio.PublicKey_export(pubkey)

    assert raw_bytes == expect


def test_publickey_import_hex_bad_length_raises_exception():
    hex_bytes = b"102030405060708090A"
    with pytest.raises(RuntimeError):
        prio.PublicKey_import_hex(hex_bytes)


def test_publickey_export_hex():
    # the output includes the null-byte
    expect = b"102030405060708090A0B0C0D0E0F00000FFEEDDCCBBAA998877665544332211\0"
    raw_bytes = bytes([
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0,
        0xC0, 0xD0, 0xE0, 0xF0, 0x00, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB,
        0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
    ])
    pubkey = prio.PublicKey_import(raw_bytes)
    hex_bytes = prio.PublicKey_export_hex(pubkey)
    assert bytes(hex_bytes) == expect


def test_privatekey_export():
    pvtkey, pubkey = prio.Keypair_new()
    pubdata = prio.PublicKey_export(pubkey)
    pvtdata = prio.PrivateKey_export(pvtkey)
    new_pvtkey = prio.PrivateKey_import(pvtdata, pubdata)
    assert pvtdata == prio.PrivateKey_export(new_pvtkey)
