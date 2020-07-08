import pytest
from prio import PrioContext
from prio.libprio import *
from array import array


def _exchange_with_serialization():
    skA, pkA = Keypair_new()
    skB, pkB = Keypair_new()
    n_data = 11
    batch_id = b"test_batch"
    cfg = PrioConfig_new(n_data, pkA, pkB, batch_id)
    server_secret = PrioPRGSeed_randomize()

    sA = PrioServer_new(cfg, PRIO_SERVER_A, skA, server_secret)
    sB = PrioServer_new(cfg, PRIO_SERVER_B, skB, server_secret)
    vA = PrioVerifier_new(sA)
    vB = PrioVerifier_new(sB)
    tA = PrioTotalShare_new()
    tB = PrioTotalShare_new()
    p1A = PrioPacketVerify1_new()
    p1B = PrioPacketVerify1_new()
    p2A = PrioPacketVerify2_new()
    p2B = PrioPacketVerify2_new()

    data = [(i % 3 == 1) or (i % 5 == 1) for i in range(n_data)]
    a, b = PrioClient_encode(cfg, bytes(data))

    # Setup verification
    PrioVerifier_set_data(vA, a)
    PrioVerifier_set_data(vB, b)

    # Produce a packet1 and send to the other party
    PrioPacketVerify1_set_data(p1A, vA)
    PrioPacketVerify1_set_data(p1B, vB)
    p1A_data = PrioPacketVerify1_write(p1A)
    p1B_data = PrioPacketVerify1_write(p1B)
    p1A = PrioPacketVerify1_new()
    p1B = PrioPacketVerify1_new()
    PrioPacketVerify1_read(p1A, p1A_data, cfg)
    PrioPacketVerify1_read(p1B, p1B_data, cfg)

    # Produce packet2 and send to the other party
    PrioPacketVerify2_set_data(p2A, vA, p1A, p1B)
    PrioPacketVerify2_set_data(p2B, vB, p1A, p1B)
    yield "verify1"
    p2A_data = PrioPacketVerify2_write(p2A)
    p2B_data = PrioPacketVerify2_write(p2B)
    p2A = PrioPacketVerify2_new()
    p2B = PrioPacketVerify2_new()
    PrioPacketVerify2_read(p2A, p2A_data, cfg)
    PrioPacketVerify2_read(p2B, p2B_data, cfg)

    # Check validity of the request
    PrioVerifier_isValid(vA, p2A, p2B)
    PrioVerifier_isValid(vB, p2A, p2B)
    yield "verify2"

    PrioServer_aggregate(sA, vA)
    PrioServer_aggregate(sB, vB)

    sA_data = PrioServer_write(sA)
    sB_data = PrioServer_write(sB)
    sA = PrioServer_new(cfg, PRIO_SERVER_A, skA, server_secret)
    sB = PrioServer_new(cfg, PRIO_SERVER_B, skB, server_secret)
    PrioServer_read(sA, sA_data, cfg)
    PrioServer_read(sB, sB_data, cfg)
    yield "aggregate"

    # Collect from many clients and share data
    PrioTotalShare_set_data(tA, sA)
    PrioTotalShare_set_data(tB, sB)
    tA_data = PrioTotalShare_write(tA)
    tB_data = PrioTotalShare_write(tB)
    tA = PrioTotalShare_new()
    tB = PrioTotalShare_new()
    PrioTotalShare_read(tA, tA_data, cfg)
    PrioTotalShare_read(tB, tB_data, cfg)

    final = PrioTotalShare_final(cfg, tA, tB)
    output = list(array("L", final))
    assert output == data

    yield "totalshare"


@PrioContext()
def test_prio_packet_verify1_serialize():
    # set/write/read data for verify1, and set data for verify2
    exchange = _exchange_with_serialization()
    assert "verify1" == next(exchange)


@PrioContext()
def test_prio_packet_verify2_serialize():
    exchange = _exchange_with_serialization()
    assert "verify1" == next(exchange)
    assert "verify2" == next(exchange)


@PrioContext()
def test_prio_aggregate_state_serialize():
    exchange = _exchange_with_serialization()
    assert "verify1" == next(exchange)
    assert "verify2" == next(exchange)
    assert "aggregate" == next(exchange)


@PrioContext()
def test_prio_packet_totalshare_serialize():
    exchange = _exchange_with_serialization()
    assert "verify1" == next(exchange)
    assert "verify2" == next(exchange)
    assert "aggregate" == next(exchange)
    assert "totalshare" == next(exchange)
