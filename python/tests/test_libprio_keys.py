import pytest
from prio import PrioContext
from prio.libprio import *


@PrioContext()
def test_publickey_serialize_binary():
    private, public = Keypair_new()
    data = PublicKey_export(public)
    assert PublicKey_import(data)

    with pytest.raises(RuntimeError):
        PublicKey_export(private)


@PrioContext()
def test_publickey_serialize_hex():
    private, public = Keypair_new()
    data = PublicKey_export_hex(public)
    assert PublicKey_import_hex(data)

    with pytest.raises(RuntimeError):
        PublicKey_export_hex(private)


@PrioContext()
def test_private_serialize_binary():
    private, public = Keypair_new()
    private_data = PrivateKey_export(private)
    public_data = PublicKey_export(public)
    assert PrivateKey_import(private_data, public_data)


@PrioContext()
def test_private_serialize_hex():
    private, public = Keypair_new()
    private_data = PrivateKey_export_hex(private)
    public_data = PublicKey_export_hex(public)
    assert PrivateKey_import_hex(private_data, public_data)


@PrioContext()
def test_publickey_export():
    raw_bytes = bytes((3 * x + 7) % 0xFF for x in range(CURVE25519_KEY_LEN))
    pubkey = PublicKey_import(raw_bytes)
    raw_bytes2 = PublicKey_export(pubkey)
    assert raw_bytes == raw_bytes2


@PrioContext()
def test_publickey_export_hex():
    expect = b"102030405060708090A0B0C0D0E0F00000FFEEDDCCBBAA998877665544332211"
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


@PrioContext()
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


@PrioContext()
def test_publickey_import_hex_bad_length_raises_exception():
    hex_bytes = b"102030405060708090A"
    with pytest.raises(RuntimeError):
        PublicKey_import_hex(hex_bytes)
