# This file was automatically generated by SWIG (http://www.swig.org).
# Version 4.0.1
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.

from sys import version_info as _swig_python_version_info

if _swig_python_version_info < (2, 7, 0):
    raise RuntimeError("Python 2.7 or later required")

# Import the low-level C/C++ module
if __package__ or "." in __name__:
    from . import _libprio
else:
    import _libprio

try:
    import builtins as __builtin__
except ImportError:
    import __builtin__


def _swig_repr(self):
    try:
        strthis = "proxy of " + self.this.__repr__()
    except __builtin__.Exception:
        strthis = ""
    return "<%s.%s; %s >" % (
        self.__class__.__module__,
        self.__class__.__name__,
        strthis,
    )


def _swig_setattr_nondynamic_instance_variable(set):
    def set_instance_attr(self, name, value):
        if name == "thisown":
            self.this.own(value)
        elif name == "this":
            set(self, name, value)
        elif hasattr(self, name) and isinstance(getattr(type(self), name), property):
            set(self, name, value)
        else:
            raise AttributeError("You cannot add instance attributes to %s" % self)

    return set_instance_attr


def _swig_setattr_nondynamic_class_variable(set):
    def set_class_attr(cls, name, value):
        if hasattr(cls, name) and not isinstance(getattr(cls, name), property):
            set(cls, name, value)
        else:
            raise AttributeError("You cannot add class attributes to %s" % cls)

    return set_class_attr


def _swig_add_metaclass(metaclass):
    """Class decorator for adding a metaclass to a SWIG wrapped class - a slimmed down version of six.add_metaclass"""

    def wrapper(cls):
        return metaclass(cls.__name__, cls.__bases__, cls.__dict__.copy())

    return wrapper


class _SwigNonDynamicMeta(type):
    """Meta class to enforce nondynamic attributes (no new attributes) for a class"""

    __setattr__ = _swig_setattr_nondynamic_class_variable(type.__setattr__)


BBIT_PREC_MAX = _libprio.BBIT_PREC_MAX


def PrioPacketVerify1_write(p):
    r"""
    PrioPacketVerify1_write(const_PrioPacketVerify1 p) -> PyObject *

    Parameters
    ----------
    p: const_PrioPacketVerify1

    """
    return _libprio.PrioPacketVerify1_write(p)


def PrioPacketVerify2_write(p):
    r"""
    PrioPacketVerify2_write(const_PrioPacketVerify2 p) -> PyObject *

    Parameters
    ----------
    p: const_PrioPacketVerify2

    """
    return _libprio.PrioPacketVerify2_write(p)


def PrioTotalShare_write(p):
    r"""
    PrioTotalShare_write(const_PrioTotalShare p) -> PyObject *

    Parameters
    ----------
    p: const_PrioTotalShare

    """
    return _libprio.PrioTotalShare_write(p)


def PrioPacketVerify1_read(p, data, len, cfg):
    r"""
    PrioPacketVerify1_read(PrioPacketVerify1 p, unsigned char const * data, unsigned int len, const_PrioConfig cfg) -> SECStatus

    Parameters
    ----------
    p: PrioPacketVerify1
    data: unsigned char const *
    len: unsigned int
    cfg: const_PrioConfig

    """
    return _libprio.PrioPacketVerify1_read(p, data, len, cfg)


def PrioPacketVerify2_read(p, data, len, cfg):
    r"""
    PrioPacketVerify2_read(PrioPacketVerify2 p, unsigned char const * data, unsigned int len, const_PrioConfig cfg) -> SECStatus

    Parameters
    ----------
    p: PrioPacketVerify2
    data: unsigned char const *
    len: unsigned int
    cfg: const_PrioConfig

    """
    return _libprio.PrioPacketVerify2_read(p, data, len, cfg)


def PrioTotalShare_read(p, data, len, cfg):
    r"""
    PrioTotalShare_read(PrioTotalShare p, unsigned char const * data, unsigned int len, const_PrioConfig cfg) -> SECStatus

    Parameters
    ----------
    p: PrioTotalShare
    data: unsigned char const *
    len: unsigned int
    cfg: const_PrioConfig

    """
    return _libprio.PrioTotalShare_read(p, data, len, cfg)


def PublicKey_export(key):
    r"""
    PublicKey_export(const_PublicKey key) -> PyObject *

    Parameters
    ----------
    key: const_PublicKey

    """
    return _libprio.PublicKey_export(key)


def PublicKey_export_hex(key):
    r"""
    PublicKey_export_hex(const_PublicKey key) -> PyObject *

    Parameters
    ----------
    key: const_PublicKey

    """
    return _libprio.PublicKey_export_hex(key)


def PrivateKey_export(key):
    r"""
    PrivateKey_export(PrivateKey key) -> PyObject *

    Parameters
    ----------
    key: PrivateKey

    """
    return _libprio.PrivateKey_export(key)


def PrivateKey_export_hex(key):
    r"""
    PrivateKey_export_hex(PrivateKey key) -> PyObject *

    Parameters
    ----------
    key: PrivateKey

    """
    return _libprio.PrivateKey_export_hex(key)


def PrioTotalShare_final_uint(cfg, prec, tA, tB):
    r"""
    PrioTotalShare_final_uint(const_PrioConfig cfg, int const prec, const_PrioTotalShare tA, const_PrioTotalShare tB) -> SECStatus

    Parameters
    ----------
    cfg: const_PrioConfig
    prec: int const
    tA: const_PrioTotalShare
    tB: const_PrioTotalShare

    """
    return _libprio.PrioTotalShare_final_uint(cfg, prec, tA, tB)


CURVE25519_KEY_LEN = _libprio.CURVE25519_KEY_LEN

CURVE25519_KEY_LEN_HEX = _libprio.CURVE25519_KEY_LEN_HEX

PRIO_SERVER_A = _libprio.PRIO_SERVER_A

PRIO_SERVER_B = _libprio.PRIO_SERVER_B


def Prio_init():
    r"""Prio_init() -> SECStatus"""
    return _libprio.Prio_init()


def Prio_clear():
    r"""Prio_clear()"""
    return _libprio.Prio_clear()


def PrioConfig_new(nFields, serverA, serverB, batchId):
    r"""
    PrioConfig_new(int nFields, PublicKey serverA, PublicKey serverB, unsigned char const * batchId) -> PrioConfig

    Parameters
    ----------
    nFields: int
    serverA: PublicKey
    serverB: PublicKey
    batchId: unsigned char const *

    """
    return _libprio.PrioConfig_new(nFields, serverA, serverB, batchId)


def PrioConfig_new_uint(nUInts, prec, serverA, serverB, batchId):
    r"""
    PrioConfig_new_uint(int nUInts, int prec, PublicKey serverA, PublicKey serverB, unsigned char const * batchId) -> PrioConfig

    Parameters
    ----------
    nUInts: int
    prec: int
    serverA: PublicKey
    serverB: PublicKey
    batchId: unsigned char const *

    """
    return _libprio.PrioConfig_new_uint(nUInts, prec, serverA, serverB, batchId)


def PrioConfig_numDataFields(cfg):
    r"""
    PrioConfig_numDataFields(const_PrioConfig cfg) -> int

    Parameters
    ----------
    cfg: const_PrioConfig

    """
    return _libprio.PrioConfig_numDataFields(cfg)


def PrioConfig_numUIntEntries(cfg, prec):
    r"""
    PrioConfig_numUIntEntries(const_PrioConfig cfg, int prec) -> int

    Parameters
    ----------
    cfg: const_PrioConfig
    prec: int

    """
    return _libprio.PrioConfig_numUIntEntries(cfg, prec)


def PrioConfig_maxDataFields():
    r"""PrioConfig_maxDataFields() -> int"""
    return _libprio.PrioConfig_maxDataFields()


def PrioConfig_maxUIntEntries(prec):
    r"""
    PrioConfig_maxUIntEntries(int prec) -> int

    Parameters
    ----------
    prec: int

    """
    return _libprio.PrioConfig_maxUIntEntries(prec)


def PrioConfig_newTest(nFields):
    r"""
    PrioConfig_newTest(int nFields) -> PrioConfig

    Parameters
    ----------
    nFields: int

    """
    return _libprio.PrioConfig_newTest(nFields)


def Keypair_new():
    r"""Keypair_new() -> SECStatus"""
    return _libprio.Keypair_new()


def PublicKey_import(data):
    r"""
    PublicKey_import(unsigned char const * data) -> SECStatus

    Parameters
    ----------
    data: unsigned char const *

    """
    return _libprio.PublicKey_import(data)


def PrivateKey_import(privData, pubData):
    r"""
    PrivateKey_import(unsigned char const * privData, unsigned char const * pubData) -> SECStatus

    Parameters
    ----------
    privData: unsigned char const *
    pubData: unsigned char const *

    """
    return _libprio.PrivateKey_import(privData, pubData)


def PublicKey_import_hex(hexData):
    r"""
    PublicKey_import_hex(unsigned char const * hexData) -> SECStatus

    Parameters
    ----------
    hexData: unsigned char const *

    """
    return _libprio.PublicKey_import_hex(hexData)


def PrivateKey_import_hex(privHexData, pubHexData):
    r"""
    PrivateKey_import_hex(unsigned char const * privHexData, unsigned char const * pubHexData) -> SECStatus

    Parameters
    ----------
    privHexData: unsigned char const *
    pubHexData: unsigned char const *

    """
    return _libprio.PrivateKey_import_hex(privHexData, pubHexData)


def PrioClient_encode(cfg, data_in):
    r"""
    PrioClient_encode(const_PrioConfig cfg, bool const * data_in) -> SECStatus

    Parameters
    ----------
    cfg: const_PrioConfig
    data_in: bool const *

    """
    return _libprio.PrioClient_encode(cfg, data_in)


def PrioClient_encode_uint(cfg, prec, data_uint):
    r"""
    PrioClient_encode_uint(const_PrioConfig cfg, int const prec, long const * data_uint) -> SECStatus

    Parameters
    ----------
    cfg: const_PrioConfig
    prec: int const
    data_uint: long const *

    """
    return _libprio.PrioClient_encode_uint(cfg, prec, data_uint)


def PrioPRGSeed_randomize():
    r"""PrioPRGSeed_randomize() -> SECStatus"""
    return _libprio.PrioPRGSeed_randomize()


def PrioServer_new(cfg, serverIdx, serverPriv, serverSharedSecret):
    r"""
    PrioServer_new(const_PrioConfig cfg, PrioServerId serverIdx, PrivateKey serverPriv, PrioPRGSeed const serverSharedSecret) -> PrioServer

    Parameters
    ----------
    cfg: const_PrioConfig
    serverIdx: enum PrioServerId
    serverPriv: PrivateKey
    serverSharedSecret: unsigned char const [AES_128_KEY_LENGTH]

    """
    return _libprio.PrioServer_new(cfg, serverIdx, serverPriv, serverSharedSecret)


def PrioVerifier_new(s):
    r"""
    PrioVerifier_new(PrioServer s) -> PrioVerifier

    Parameters
    ----------
    s: PrioServer

    """
    return _libprio.PrioVerifier_new(s)


def PrioVerifier_set_data(v, data):
    r"""
    PrioVerifier_set_data(PrioVerifier v, unsigned char * data) -> SECStatus

    Parameters
    ----------
    v: PrioVerifier
    data: unsigned char *

    """
    return _libprio.PrioVerifier_set_data(v, data)


def PrioPacketVerify1_new():
    r"""PrioPacketVerify1_new() -> PrioPacketVerify1"""
    return _libprio.PrioPacketVerify1_new()


def PrioPacketVerify1_set_data(p1, v):
    r"""
    PrioPacketVerify1_set_data(PrioPacketVerify1 p1, const_PrioVerifier v) -> SECStatus

    Parameters
    ----------
    p1: PrioPacketVerify1
    v: const_PrioVerifier

    """
    return _libprio.PrioPacketVerify1_set_data(p1, v)


def PrioPacketVerify2_new():
    r"""PrioPacketVerify2_new() -> PrioPacketVerify2"""
    return _libprio.PrioPacketVerify2_new()


def PrioPacketVerify2_set_data(p2, v, p1A, p1B):
    r"""
    PrioPacketVerify2_set_data(PrioPacketVerify2 p2, const_PrioVerifier v, const_PrioPacketVerify1 p1A, const_PrioPacketVerify1 p1B) -> SECStatus

    Parameters
    ----------
    p2: PrioPacketVerify2
    v: const_PrioVerifier
    p1A: const_PrioPacketVerify1
    p1B: const_PrioPacketVerify1

    """
    return _libprio.PrioPacketVerify2_set_data(p2, v, p1A, p1B)


def PrioVerifier_isValid(v, pA, pB):
    r"""
    PrioVerifier_isValid(const_PrioVerifier v, const_PrioPacketVerify2 pA, const_PrioPacketVerify2 pB) -> SECStatus

    Parameters
    ----------
    v: const_PrioVerifier
    pA: const_PrioPacketVerify2
    pB: const_PrioPacketVerify2

    """
    return _libprio.PrioVerifier_isValid(v, pA, pB)


def PrioServer_aggregate(s, v):
    r"""
    PrioServer_aggregate(PrioServer s, PrioVerifier v) -> SECStatus

    Parameters
    ----------
    s: PrioServer
    v: PrioVerifier

    """
    return _libprio.PrioServer_aggregate(s, v)


def PrioTotalShare_new():
    r"""PrioTotalShare_new() -> PrioTotalShare"""
    return _libprio.PrioTotalShare_new()


def PrioTotalShare_set_data(t, s):
    r"""
    PrioTotalShare_set_data(PrioTotalShare t, const_PrioServer s) -> SECStatus

    Parameters
    ----------
    t: PrioTotalShare
    s: const_PrioServer

    """
    return _libprio.PrioTotalShare_set_data(t, s)


def PrioTotalShare_set_data_uint(t, s, prec):
    r"""
    PrioTotalShare_set_data_uint(PrioTotalShare t, const_PrioServer s, int const prec) -> SECStatus

    Parameters
    ----------
    t: PrioTotalShare
    s: const_PrioServer
    prec: int const

    """
    return _libprio.PrioTotalShare_set_data_uint(t, s, prec)


def PrioTotalShare_final(cfg, tA, tB):
    r"""
    PrioTotalShare_final(const_PrioConfig cfg, const_PrioTotalShare tA, const_PrioTotalShare tB) -> SECStatus

    Parameters
    ----------
    cfg: const_PrioConfig
    tA: const_PrioTotalShare
    tB: const_PrioTotalShare

    """
    return _libprio.PrioTotalShare_final(cfg, tA, tB)
