"""Aggregate encoded shares from multiple servers.

In this case, we might think about a Prio service where disjoint sets of shares
are distributed among a set of servers sharing a common configuration. These
aggregates should have the ability to be merged.
"""
import pytest
from prio import PrioContext
from prio.libprio import *
import array


@PrioContext()
def test_server_merge():
    seed = PrioPRGSeed_randomize()
    skA, pkA = Keypair_new()
    skB, pkB = Keypair_new()
    batch_id = b"test_batch"
    n_fields = 133
    n_servers = 4
    clients_per_server = {i: 2 ** i for i in range(n_servers)}

    cfg = PrioConfig_new(n_fields, pkA, pkB, batch_id)
    n_data = PrioConfig_numDataFields(cfg)
    data_items = bytes([(i % 3 == 1) or (i % 5 == 1) for i in range(n_data)])

    sA_serialized = []
    sB_serialized = []

    for i in range(n_servers):
        sA = PrioServer_new(cfg, PRIO_SERVER_A, skA, seed)
        sB = PrioServer_new(cfg, PRIO_SERVER_B, skB, seed)
        vA = PrioVerifier_new(sA)
        vB = PrioVerifier_new(sB)
        for _ in range(clients_per_server[i]):
            for_a, for_b = PrioClient_encode(cfg, data_items)
            PrioVerifier_set_data(vA, for_a)
            PrioVerifier_set_data(vB, for_b)
            PrioServer_aggregate(sA, vA)
            PrioServer_aggregate(sB, vB)
        sA_serialized.append(PrioServer_write(sA))
        sB_serialized.append(PrioServer_write(sB))

    sA = PrioServer_new(cfg, PRIO_SERVER_A, skA, seed)
    for data in sA_serialized:
        sA_i = PrioServer_new(cfg, PRIO_SERVER_A, skA, seed)
        PrioServer_read(sA_i, data, cfg)
        PrioServer_merge(sA, sA_i)

    sB = PrioServer_new(cfg, PRIO_SERVER_B, skB, seed)
    for data in sB_serialized:
        sB_i = PrioServer_new(cfg, PRIO_SERVER_B, skB, seed)
        PrioServer_read(sB_i, data, cfg)
        PrioServer_merge(sB, sB_i)

    tA = PrioTotalShare_new()
    tB = PrioTotalShare_new()
    PrioTotalShare_set_data(tA, sA)
    PrioTotalShare_set_data(tB, sB)

    output = array.array("L", PrioTotalShare_final(cfg, tA, tB))

    expected = [
        item * sum(2 ** i for i in range(n_servers)) for item in list(data_items)
    ]
    assert list(output) == expected


@PrioContext()
def test_server_merge_uint():
    seed = PrioPRGSeed_randomize()
    skA, pkA = Keypair_new()
    skB, pkB = Keypair_new()
    batch_id = b"test_batch"
    precision = 7
    num_uint_entries = 11
    n_servers = 4
    clients_per_server = {i: 2 ** i for i in range(n_servers)}

    cfg = PrioConfig_new_uint(num_uint_entries, precision, pkA, pkB, batch_id)
    n_data = PrioConfig_numUIntEntries(cfg, precision)
    data_items = bytes(array.array("L", [i for i in range(n_data)]))

    sA_serialized = []
    sB_serialized = []

    for i in range(n_servers):
        sA = PrioServer_new(cfg, PRIO_SERVER_A, skA, seed)
        sB = PrioServer_new(cfg, PRIO_SERVER_B, skB, seed)
        vA = PrioVerifier_new(sA)
        vB = PrioVerifier_new(sB)
        for _ in range(clients_per_server[i]):
            for_a, for_b = PrioClient_encode_uint(cfg, precision, data_items)
            PrioVerifier_set_data(vA, for_a)
            PrioVerifier_set_data(vB, for_b)
            PrioServer_aggregate(sA, vA)
            PrioServer_aggregate(sB, vB)
        sA_serialized.append(PrioServer_write(sA))
        sB_serialized.append(PrioServer_write(sB))

    sA = PrioServer_new(cfg, PRIO_SERVER_A, skA, seed)
    for data in sA_serialized:
        sA_i = PrioServer_new(cfg, PRIO_SERVER_A, skA, seed)
        PrioServer_read(sA_i, data, cfg)
        PrioServer_merge(sA, sA_i)

    sB = PrioServer_new(cfg, PRIO_SERVER_B, skB, seed)
    for data in sB_serialized:
        sB_i = PrioServer_new(cfg, PRIO_SERVER_B, skB, seed)
        PrioServer_read(sB_i, data, cfg)
        PrioServer_merge(sB, sB_i)

    tA = PrioTotalShare_new()
    tB = PrioTotalShare_new()
    PrioTotalShare_set_data_uint(tA, sA, precision)
    PrioTotalShare_set_data_uint(tB, sB, precision)

    output = array.array("Q", PrioTotalShare_final_uint(cfg, precision, tA, tB))

    expected = [item * sum(2 ** i for i in range(n_servers)) for item in range(n_data)]
    assert list(output) == expected
