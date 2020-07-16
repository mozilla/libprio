# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import pytest
from prio.libprio import *
from prio import PrioContext
import array


@PrioContext()
def test_config_new_test():
    max_fields = PrioConfig_maxDataFields()

    def new_config(fields):
        return PrioConfig_newTest(fields)

    def raises_error(fields):
        with pytest.raises(ValueError, match=".*null pointer.*"):
            new_config(fields)
        return True

    assert new_config(0)
    assert new_config(-1)
    assert new_config(max_fields)
    assert raises_error(max_fields + 1)
    assert PrioConfig_numDataFields(new_config(42)) == 42


@PrioContext()
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
        return True

    assert new_config(0)
    assert new_config(-1)
    assert new_config(max_fields)
    assert raises_error(max_fields + 1)
    assert new_config(max_fields, ("?" * int(1e6)).encode())
    assert PrioConfig_numDataFields(new_config(42)) == 42


@PrioContext()
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
        return True

    assert new_config(0, max_precision)
    assert new_config(-1, max_precision)
    assert new_config(max_entries, max_precision)
    assert raises_error(max_entries + 1, max_precision)
    assert raises_error(max_entries, max_precision + 1)
    assert raises_error(max_entries, 0)
    assert raises_error(max_entries, -1)


@PrioContext()
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
        for_a, for_b = PrioClient_encode(cfg, data_items)
        PrioVerifier_set_data(vA, for_a)
        PrioVerifier_set_data(vB, for_b)
        PrioServer_aggregate(sA, vA)
        PrioServer_aggregate(sB, vB)

    PrioTotalShare_set_data(tA, sA)
    PrioTotalShare_set_data(tB, sB)

    output = array.array("L", PrioTotalShare_final(cfg, tA, tB))

    expected = [item * n_clients for item in list(data_items)]
    assert list(output) == expected


@PrioContext()
@pytest.mark.parametrize(
    "n_clients,precision,num_uint_entries", [(1, 8, 5), (2, 32, 10)]
)
def test_client_aggregation_uint(n_clients, precision, num_uint_entries):
    """Run a test of the integer circuit to ensure the main path works as expected
    within the bindings."""

    seed = PrioPRGSeed_randomize()
    skA, pkA = Keypair_new()
    skB, pkB = Keypair_new()
    batch_id = b"test_batch"

    cfg = PrioConfig_new_uint(num_uint_entries, precision, pkA, pkB, batch_id)
    sA = PrioServer_new(cfg, PRIO_SERVER_A, skA, seed)
    sB = PrioServer_new(cfg, PRIO_SERVER_B, skB, seed)
    vA = PrioVerifier_new(sA)
    vB = PrioVerifier_new(sB)
    tA = PrioTotalShare_new()
    tB = PrioTotalShare_new()

    assert PrioConfig_numUIntEntries(cfg, precision) == num_uint_entries
    max_length = (1 << precision) - 1

    data_items = [max_length - i for i in range(num_uint_entries)]

    for _ in range(n_clients):
        for_a, for_b = PrioClient_encode_uint(
            cfg, precision, bytes(array.array("L", data_items))
        )
        PrioVerifier_set_data(vA, for_a)
        PrioVerifier_set_data(vB, for_b)
        PrioServer_aggregate(sA, vA)
        PrioServer_aggregate(sB, vB)

    PrioTotalShare_set_data_uint(tA, sA, precision)
    PrioTotalShare_set_data_uint(tB, sB, precision)

    final_bytes = PrioTotalShare_final_uint(cfg, precision, tA, tB)
    output = array.array("Q", final_bytes)

    expected = [item * n_clients for item in list(data_items)]
    assert (
        list(output) == expected
    ), f"got {len(output)} expected {len(expected)} elements"
