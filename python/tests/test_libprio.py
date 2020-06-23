# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import pytest
from prio import libprio
import array


@pytest.mark.parametrize("n_clients", [1, 2, 10])
def test_client_agg(n_clients):
    seed = libprio.PrioPRGSeed_randomize()

    skA, pkA = libprio.Keypair_new()
    skB, pkB = libprio.Keypair_new()
    cfg = libprio.PrioConfig_new(133, pkA, pkB, b"test_batch")
    sA = libprio.PrioServer_new(cfg, libprio.PRIO_SERVER_A, skA, seed)
    sB = libprio.PrioServer_new(cfg, libprio.PRIO_SERVER_B, skB, seed)
    vA = libprio.PrioVerifier_new(sA)
    vB = libprio.PrioVerifier_new(sB)
    tA = libprio.PrioTotalShare_new()
    tB = libprio.PrioTotalShare_new()

    n_data = libprio.PrioConfig_numDataFields(cfg)
    data_items = bytes([(i % 3 == 1) or (i % 5 == 1) for i in range(n_data)])

    for i in range(n_clients):
        for_server_a, for_server_b = libprio.PrioClient_encode(cfg, data_items)

        libprio.PrioVerifier_set_data(vA, for_server_a)
        libprio.PrioVerifier_set_data(vB, for_server_b)

        libprio.PrioServer_aggregate(sA, vA)
        libprio.PrioServer_aggregate(sB, vB)

    libprio.PrioTotalShare_set_data(tA, sA)
    libprio.PrioTotalShare_set_data(tB, sB)

    output = array.array("L", libprio.PrioTotalShare_final(cfg, tA, tB))

    expected = [item * n_clients for item in list(data_items)]
    assert list(output) == expected


def test_publickey_export():
    raw_bytes = bytes((3 * x + 7) % 0xFF for x in range(libprio.CURVE25519_KEY_LEN))
    pubkey = libprio.PublicKey_import(raw_bytes)
    raw_bytes2 = libprio.PublicKey_export(pubkey)

    assert raw_bytes == raw_bytes2


@pytest.mark.parametrize(
    "hex_bytes",
    [
        b"102030405060708090A0B0C0D0E0F00000FFEEDDCCBBAA998877665544332211",
        b"102030405060708090a0B0C0D0E0F00000FfeEddcCbBaa998877665544332211",
    ],
)
def test_publickey_import_hex(hex_bytes):
    expect = bytes(
        [
            0x10,
            0x20,
            0x30,
            0x40,
            0x50,
            0x60,
            0x70,
            0x80,
            0x90,
            0xA0,
            0xB0,
            0xC0,
            0xD0,
            0xE0,
            0xF0,
            0x00,
            0x00,
            0xFF,
            0xEE,
            0xDD,
            0xCC,
            0xBB,
            0xAA,
            0x99,
            0x88,
            0x77,
            0x66,
            0x55,
            0x44,
            0x33,
            0x22,
            0x11,
        ]
    )
    pubkey = libprio.PublicKey_import_hex(hex_bytes)
    raw_bytes = libprio.PublicKey_export(pubkey)

    assert raw_bytes == expect


def test_publickey_import_hex_bad_length_raises_exception():
    hex_bytes = b"102030405060708090A"
    with pytest.raises(RuntimeError):
        libprio.PublicKey_import_hex(hex_bytes)


def test_publickey_export_hex():
    # TODO: the output includes the null-byte
    expect = b"102030405060708090A0B0C0D0E0F00000FFEEDDCCBBAA998877665544332211\0"
    raw_bytes = bytes(
        [
            0x10,
            0x20,
            0x30,
            0x40,
            0x50,
            0x60,
            0x70,
            0x80,
            0x90,
            0xA0,
            0xB0,
            0xC0,
            0xD0,
            0xE0,
            0xF0,
            0x00,
            0x00,
            0xFF,
            0xEE,
            0xDD,
            0xCC,
            0xBB,
            0xAA,
            0x99,
            0x88,
            0x77,
            0x66,
            0x55,
            0x44,
            0x33,
            0x22,
            0x11,
        ]
    )
    pubkey = libprio.PublicKey_import(raw_bytes)
    hex_bytes = libprio.PublicKey_export_hex(pubkey)
    assert bytes(hex_bytes) == expect


def test_privatekey_export():
    pvtkey, pubkey = libprio.Keypair_new()
    pubdata = libprio.PublicKey_export(pubkey)
    pvtdata = libprio.PrivateKey_export(pvtkey)
    new_pvtkey = libprio.PrivateKey_import(pvtdata, pubdata)
    assert pvtdata == libprio.PrivateKey_export(new_pvtkey)
