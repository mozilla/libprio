/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module libprio
%{
    #include "../libprio/include/mprio.h"
%}

%feature("autodoc", "3");
%include "pointer.i"
%include "msgpack.i"

%init %{
    Prio_init();
    atexit(Prio_clear);
%}

// Handle SECStatus.
%typemap(out) SECStatus {
   if ($1 != SECSuccess) {
       PyErr_SetString(PyExc_RuntimeError, "$symname was not successful.");
       SWIG_fail;
   }
   $result = Py_BuildValue("");
}


// We need to redefine the function so we can fix the buffer size at
// compile time, using the Python object
%define EXPORT_KEY(TYPE, ARGTYPE, FUNC, BUFSIZE)

    // Ignore the mprio.h functions
    %ignore TYPE ## _ ## FUNC;

    // Rename the wrapper function
    %rename(TYPE ## _ ## FUNC) TYPE ## _ ## FUNC ## _wrapper;

    // Define the wrapper function
    %inline {
        PyObject* TYPE ## _ ## FUNC ## _wrapper(ARGTYPE key) {
            SECStatus rv = SECFailure;
            unsigned char data[BUFSIZE];

            rv = TYPE ## _ ## FUNC(key, data, BUFSIZE);
            if (rv != SECSuccess) {
                PyErr_SetString(PyExc_RuntimeError, "Error exporting TYPE");
                return NULL;
            }
            return PyBytes_FromStringAndSize((char *)data, BUFSIZE);
        }
    }
%enddef


%define PRG_SEED()
    // PrioPRGSeed_randomize
    %typemap(in,numinputs=0) PrioPRGSeed * (PrioPRGSeed tmp) {
        $1 = &tmp;
    }

    %typemap(argout) PrioPRGSeed * {
        $result = SWIG_Python_AppendOutput(
            $result, PyBytes_FromStringAndSize((const char*)*$1, PRG_SEED_LENGTH)
        );
    }

    %typemap(in) const PrioPRGSeed {
        $1 = (unsigned char*)PyBytes_AsString($input);
    }
%enddef


%define EXPORT_PYTHON_DATA()
    /// Typemaps for converting Python bytes into a buffer for libray calls
    %typemap(in) (const unsigned char *, unsigned int) {
        if (!PyBytes_Check($input)) {
            PyErr_SetString(PyExc_ValueError, "Expecting a byte string");
            SWIG_fail;
        }
        $1 = (unsigned char*) PyBytes_AsString($input);
        $2 = (unsigned int) PyBytes_Size($input);
    }

    %apply (const unsigned char *, unsigned int) {
        // PrioConfig_new
        // PrioConfig_new_uint
        (const unsigned char *batchId, unsigned int batchIdLen),
        // PublicKey_import
        // PrivateKey_import
        (const unsigned char *data, unsigned int dataLen),
        (const unsigned char *privData, unsigned int privDataLen),
        (const unsigned char *pubData, unsigned int pubDataLen),
        // PublicKey_import_hex
        // PrivateKey_import_hex
        (const unsigned char *hexData, unsigned int dataLen),
        (const unsigned char *privHexData, unsigned int privDataLen),
        (const unsigned char *pubHexData, unsigned int pubDataLen),
        // MSGPACK_READ: *_read_wrapper
        (const unsigned char *data, unsigned int len)
    }
%enddef


%define CLIENT_ENCODE()
    // PrioClient_encode
    // PrioClient_encode_uint
    %typemap(in) const bool * {
        if (!PyBytes_Check($input)) {
            PyErr_SetString(PyExc_ValueError, "Expecting a byte string");
            SWIG_fail;
        }
        $1 = (bool*) PyBytes_AsString($input);
    }

    %typemap(in,numinputs=0)
        (unsigned char **, unsigned int *)
        (unsigned char *data = NULL, unsigned int len = 0) {
        $1 = &data;
        $2 = &len;
    }

    %typemap(argout) (unsigned char **, unsigned int *) {
        $result = SWIG_Python_AppendOutput(
            $result, PyBytes_FromStringAndSize((const char *)*$1, *$2));
        // Free malloc'ed data from within PrioClient_encode
        if (*$1) {
            free(*$1);
        }
    }

    %apply (unsigned char **, unsigned int *) {
        (unsigned char **forServerA, unsigned int *aLen),
        (unsigned char **forServerB, unsigned int *bLen)
    }
%enddef


%define VERIFIER_SET_DATA()
    // PrioVerifier_set_data
    %typemap(in) (unsigned char * data, unsigned int dataLen) {
        if (!PyBytes_Check($input)) {
            PyErr_SetString(PyExc_ValueError, "Expecting a byte string");
            SWIG_fail;
        }
        $1 = (unsigned char*) PyBytes_AsString($input);
        $2 = (unsigned int) PyBytes_Size($input);
    }
%enddef


%define TOTAL_SHARE_FINAL()
    // PrioTotalShare_final
    %typemap(in) (const_PrioConfig, unsigned long long *) {
        $1 = PyCapsule_GetPointer($input, "PrioConfig");
        $2 = malloc(sizeof(long long)*PrioConfig_numDataFields($1));
    }

    %typemap(argout) (const_PrioConfig, unsigned long long *) {
        $result = SWIG_Python_AppendOutput(
            $result,
            PyByteArray_FromStringAndSize(
                (const char*)$2, sizeof(long long)*PrioConfig_numDataFields($1)
            )
        );
        if ($2) {
            free($2);
        }
    }

    %apply (const_PrioConfig, unsigned long long *) {
        (const_PrioConfig cfg, unsigned long long *output)
    }
%enddef


OPAQUE_POINTER(PrioConfig)
OPAQUE_POINTER(PrioServer)
OPAQUE_POINTER(PrioVerifier)
OPAQUE_POINTER(PrioPacketVerify1)
OPAQUE_POINTER(PrioPacketVerify2)
OPAQUE_POINTER(PrioTotalShare)
OPAQUE_POINTER(PublicKey)
OPAQUE_POINTER(PrivateKey)

MSGPACK_WRITE(PrioPacketVerify1)
MSGPACK_WRITE(PrioPacketVerify2)
MSGPACK_WRITE(PrioTotalShare)
MSGPACK_READ(PrioPacketVerify1)
MSGPACK_READ(PrioPacketVerify2)
MSGPACK_READ(PrioTotalShare)

EXPORT_KEY(PublicKey, const_PublicKey, export, CURVE25519_KEY_LEN)
EXPORT_KEY(PublicKey, const_PublicKey, export_hex, CURVE25519_KEY_LEN_HEX+1)
EXPORT_KEY(PrivateKey, PrivateKey, export, CURVE25519_KEY_LEN)
EXPORT_KEY(PrivateKey, PrivateKey, export_hex, CURVE25519_KEY_LEN_HEX+1)

PRG_SEED()
EXPORT_PYTHON_DATA()
CLIENT_ENCODE()
VERIFIER_SET_DATA()
TOTAL_SHARE_FINAL()


%include "../libprio/include/mprio.h"


// Helpful resources:
// * https://stackoverflow.com/a/38191420
// * http://www.swig.org/Doc3.0/SWIGDocumentation.html#Typemaps_nn2
// * https://stackoverflow.com/questions/26567457/swig-wrapping-typedef-struct
// * http://www.swig.org/Doc3.0/SWIGDocumentation.html#Typemaps_multi_argument_typemaps
// * https://github.com/msgpack/msgpack-c/wiki/v2_0_c_overview
