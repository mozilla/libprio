import pytest
from prio import PrioContext
from prio.libprio import *
from base64 import b64encode
from multiprocessing import Pool


def _encode(internal_hex, external_hex):
    n_data = 10
    batch_id = b"test_batch"
    with PrioContext():
        pkA = PublicKey_import_hex(internal_hex)
        pkB = PublicKey_import_hex(external_hex)
        cfg = PrioConfig_new(n_data, pkA, pkB, batch_id)
        data_items = bytes([(i % 3 == 1) or (i % 5 == 1) for i in range(n_data)])
        for_server_a, for_server_b = PrioClient_encode(cfg, data_items)
    return b64encode(for_server_a), b64encode(for_server_b)


def test_multiprocessing_encoding_succeeds():
    with PrioContext():
        _, pkA = Keypair_new()
        _, pkB = Keypair_new()
        internal_hex = PublicKey_export_hex(pkA)
        external_hex = PublicKey_export_hex(pkB)

    pool_size = 2
    num_elements = 10

    p = Pool(pool_size)
    res = p.starmap(_encode, [(internal_hex, external_hex)] * num_elements)
    assert len(res) == 10
