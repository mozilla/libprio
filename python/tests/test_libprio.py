# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import pytest
from prio.libprio import *
import array


def test_config():
    _, pkA = Keypair_new()
    _, pkB = Keypair_new()
    batch_id = b"test_batch"
    max_fields = PrioConfig_maxDataFields()

    # close over variables for convenience
    def new_config(fields, batch_id=batch_id):
        return PrioConfig_new(fields, pkA, pkB, batch_id)

    def raises_error(fields):
        # ValueError: PyCapsule_New called with null pointer
        with pytest.raises(ValueError, match=".*null pointer.*"):
            new_config(fields)

    assert new_config(max_fields)
    assert new_config(0)
    assert new_config(-1)
    raises_error(max_fields + 1)
    assert new_config(max_fields, ("?" * int(1e6)).encode())


def test_config_uint():
    _, pkA = Keypair_new()
    _, pkB = Keypair_new()
    batch_id = b"test_batch"
    max_precision = BBIT_PREC_MAX
    max_entries = PrioConfig_maxUIntEntries(max_precision)

    def new_config(entries, precision):
        return PrioConfig_new_uint(entries, precision, pkA, pkB, batch_id)

    def raises_error(entries, precision):
        with pytest.raises(ValueError, match=".*null pointer.*"):
            new_config(entries, precision)

    assert new_config(max_entries, max_precision)
    assert new_config(0, max_precision)
    assert new_config(-1, max_precision)
    raises_error(max_entries + 1, max_precision)
    raises_error(max_entries, max_precision + 1)
    raises_error(max_entries, 0)
    raises_error(max_entries, -1)


def test_publickey_export():
    raw_bytes = bytes((3 * x + 7) % 0xFF for x in range(CURVE25519_KEY_LEN))
    pubkey = PublicKey_import(raw_bytes)
    raw_bytes2 = PublicKey_export(pubkey)

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
        # fmt:off
        [
            0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0,
            0xC0, 0xD0, 0xE0, 0xF0, 0x00, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB,
            0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
        ]
        # fmt:on
    )
    pubkey = PublicKey_import_hex(hex_bytes)
    raw_bytes = PublicKey_export(pubkey)

    assert raw_bytes == expect


def test_publickey_import_hex_bad_length_raises_exception():
    hex_bytes = b"102030405060708090A"
    with pytest.raises(RuntimeError):
        PublicKey_import_hex(hex_bytes)


def test_publickey_export_hex():
    # TODO: the output includes the null-byte
    expect = b"102030405060708090A0B0C0D0E0F00000FFEEDDCCBBAA998877665544332211\0"
    raw_bytes = bytes(
        # fmt:off
        [
            0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0,
            0xC0, 0xD0, 0xE0, 0xF0, 0x00, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB,
            0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
        ]
        # fmt:on
    )
    pubkey = PublicKey_import(raw_bytes)
    hex_bytes = PublicKey_export_hex(pubkey)
    assert bytes(hex_bytes) == expect


def test_privatekey_export():
    pvtkey, pubkey = Keypair_new()
    pubdata = PublicKey_export(pubkey)
    pvtdata = PrivateKey_export(pvtkey)
    new_pvtkey = PrivateKey_import(pvtdata, pubdata)
    assert pvtdata == PrivateKey_export(new_pvtkey)


@pytest.mark.parametrize("n_clients", [1, 2, 10])
def test_client_aggregation(n_clients):
    """Run a test of the binary circuit to ensure the main path works as expected
    within the bindings."""

    seed = PrioPRGSeed_randomize()
    skA, pkA = Keypair_new()
    skB, pkB = Keypair_new()
    batch_id = b"test_batch"
    n_fields = 133

    cfg = PrioConfig_new(n_fields, pkA, pkB, batch_id)
    sA = PrioServer_new(cfg, PRIO_SERVER_A, skA, seed)
    sB = PrioServer_new(cfg, PRIO_SERVER_B, skB, seed)
    vA = PrioVerifier_new(sA)
    vB = PrioVerifier_new(sB)
    tA = PrioTotalShare_new()
    tB = PrioTotalShare_new()

    n_data = PrioConfig_numDataFields(cfg)
    data_items = bytes([(i % 3 == 1) or (i % 5 == 1) for i in range(n_data)])

    for i in range(n_clients):
        for_server_a, for_server_b = PrioClient_encode(cfg, data_items)

        PrioVerifier_set_data(vA, for_server_a)
        PrioVerifier_set_data(vB, for_server_b)

        PrioServer_aggregate(sA, vA)
        PrioServer_aggregate(sB, vB)

    PrioTotalShare_set_data(tA, sA)
    PrioTotalShare_set_data(tB, sB)

    output = array.array("L", PrioTotalShare_final(cfg, tA, tB))

    expected = [item * n_clients for item in list(data_items)]
    assert list(output) == expected
