/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module libprio
%{
#include "libprio/include/mprio.h"
%}

%feature("autodoc", "3");

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


// Typemaps for dealing with the pointer to implementation idiom.
%define OPAQUE_POINTER(T)

%{
void T ## _PyCapsule_clear(PyObject *capsule) {
    T ptr = PyCapsule_GetPointer(capsule, "T");
    T ## _clear(ptr);
}
%}

// We've assigned a destructor to the capsule instance, so prevent users from
// manually destroying the pointer for safety.
%ignore T ## _clear;

// Get the pointer from the capsule
%typemap(in) T, const_ ## T {
    $1 = PyCapsule_GetPointer($input, "T");
}

// Create a new capsule for the new pointer
%typemap(out) T {
    $result = PyCapsule_New($1, "T", T ## _PyCapsule_clear);
}

// Create a temporary stack variable for allocating a new opaque pointer
%typemap(in,numinputs=0) T* (T tmp = NULL) {
    $1 = &tmp;
}

// Return the pointer to the newly allocated memory
%typemap(argout) T* {
    $result = SWIG_Python_AppendOutput($result,PyCapsule_New(*$1, "T", T ## _PyCapsule_clear));
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


// PrioPRGSeed_randomize
%typemap(in,numinputs=0) PrioPRGSeed * (PrioPRGSeed tmp) {
    $1 = &tmp;
}

%typemap(argout) PrioPRGSeed * {
    $result = SWIG_Python_AppendOutput($result,PyBytes_FromStringAndSize((const char*)*$1, PRG_SEED_LENGTH));
}

%typemap(in) const PrioPRGSeed {
    $1 = (unsigned char*)PyBytes_AsString($input);
}


// PrioConfig_new
// PublicKey_import
// PrivateKey_import
// PublicKey_import_hex
// PrivateKey_import_hex
%typemap(in) (const unsigned char *, unsigned int) {
    if (!PyBytes_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting a byte string");
        SWIG_fail;
    }
    $1 = (unsigned char*) PyBytes_AsString($input);
    $2 = (unsigned int) PyBytes_Size($input);
}

%apply (const unsigned char *, unsigned int) {
    (const unsigned char *batchId, unsigned int batchIdLen),
    (const unsigned char *data, unsigned int dataLen),
    (const unsigned char *hexData, unsigned int dataLen),
    (const unsigned char *privData, unsigned int privDataLen),
    (const unsigned char *pubData, unsigned int pubDataLen),
    (const unsigned char *privHexData, unsigned int privDataLen),
    (const unsigned char *pubHexData, unsigned int pubDataLen)
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

EXPORT_KEY(PublicKey, const_PublicKey, export, CURVE25519_KEY_LEN)
EXPORT_KEY(PublicKey, const_PublicKey, export_hex, CURVE25519_KEY_LEN_HEX+1)
EXPORT_KEY(PrivateKey, PrivateKey, export, CURVE25519_KEY_LEN)
EXPORT_KEY(PrivateKey, PrivateKey, export_hex, CURVE25519_KEY_LEN_HEX+1)


// PrioClient_encode
%typemap(in) const bool * {
    if (!PyBytes_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting a byte string");
        SWIG_fail;
    }
    $1 = (bool*) PyBytes_AsString($input);
}

%typemap(in,numinputs=0) (unsigned char **, unsigned int *) (unsigned char *data = NULL, unsigned int len = 0) {
    $1 = &data;
    $2 = &len;
}

%typemap(argout) (unsigned char **, unsigned int *) {
    $result = SWIG_Python_AppendOutput(
        $result, PyBytes_FromStringAndSize((const char *)*$1, *$2));
    // Free malloc'ed data from within PrioClient_encode
    if (*$1) free(*$1);
}

%apply (unsigned char **, unsigned int *) {
    (unsigned char **forServerA, unsigned int *aLen),
    (unsigned char **forServerB, unsigned int *bLen)
}


// PrioVerifier_set_data
%typemap(in) (unsigned char * data, unsigned int dataLen) {
    if (!PyBytes_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting a byte string");
        SWIG_fail;
    }
    $1 = (unsigned char*) PyBytes_AsString($input);
    $2 = (unsigned int) PyBytes_Size($input);
}


// PrioTotalShare_final
%typemap(in) (const_PrioConfig, unsigned long long *) {
    $1 = PyCapsule_GetPointer($input, "PrioConfig");
    $2 = malloc(sizeof(long long)*PrioConfig_numDataFields($1));
}

%typemap(argout) (const_PrioConfig, unsigned long long *) {
    $result = SWIG_Python_AppendOutput(
        $result,
        PyByteArray_FromStringAndSize((const char*)$2, sizeof(long long)*PrioConfig_numDataFields($1))
    );
    if ($2) free($2);
}

%apply (const_PrioConfig, unsigned long long *) {
    (const_PrioConfig cfg, unsigned long long *output)
}


// Define wrapper functions for msgpacker_packer due to the dynamically allocated sbuffer.
// Memory shouldn't be moving around so much, but this wrapper function needs to exist in
// order to marshal data due to typemaps not having side-effects

%define MSGPACK_WRITE(TYPE)
%ignore TYPE ## _write;
%rename(TYPE ## _write) TYPE ## _write_wrapper;
%inline %{
PyObject* TYPE ## _write_wrapper(const_ ## TYPE p) {
    PyObject* data = NULL;
    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    SECStatus rv = TYPE ## _write(p, &pk);
    if (rv == SECSuccess) {
        // move the data outside of this wrapper
        data = PyBytes_FromStringAndSize(sbuf.data, sbuf.size);
    }

    // free msgpacker data-structures
    msgpack_sbuffer_destroy(&sbuf);

    return data;
}
%}
%enddef

MSGPACK_WRITE(PrioPacketVerify1)
MSGPACK_WRITE(PrioPacketVerify2)
MSGPACK_WRITE(PrioTotalShare)


// Unfortunately, unpacking also requires some manual work. We take the binary message and
// copy the data into an unpacker

%apply (const unsigned char*, unsigned int) {
    (const unsigned char *data, unsigned int len)
}

// Add a type-casting rule to prevent swig from adding the define into the scripting interface
%inline %{
#define MSGPACK_INIT_BUFFER_SIZE (size_t)128
%}

%define MSGPACK_READ(TYPE)
%ignore TYPE ## _read;
%rename(TYPE ## _read) TYPE ## _read_wrapper;
%inline %{
SECStatus TYPE ## _read_wrapper(TYPE p, const unsigned char *data, unsigned int len, const_PrioConfig cfg) {
    SECStatus rv = SECFailure;
    msgpack_unpacker upk;
    // Initialize the unpacker with a reasonably sized initial buffer. The
    // unpacker will eat into the initially reserved space for counting and may
    // need to be reallocated.
    bool result = msgpack_unpacker_init(&upk, MSGPACK_INIT_BUFFER_SIZE);
    if (result) {
        if (msgpack_unpacker_buffer_capacity(&upk) < len) {
            result = msgpack_unpacker_reserve_buffer(&upk, len);
        }
        if (result) {
            memcpy(msgpack_unpacker_buffer(&upk), data, len);
            msgpack_unpacker_buffer_consumed(&upk, len);
            rv = TYPE ## _read(p, &upk, cfg);
        }
    }
    msgpack_unpacker_destroy(&upk);
    return rv;
}
%}
%enddef

MSGPACK_READ(PrioPacketVerify1)
MSGPACK_READ(PrioPacketVerify2)
MSGPACK_READ(PrioTotalShare)


%include "libprio/include/mprio.h"


// Helpful resources:
// * https://stackoverflow.com/a/38191420
// * http://www.swig.org/Doc3.0/SWIGDocumentation.html#Typemaps_nn2
// * https://stackoverflow.com/questions/26567457/swig-wrapping-typedef-struct
// * http://www.swig.org/Doc3.0/SWIGDocumentation.html#Typemaps_multi_argument_typemaps
// * https://github.com/msgpack/msgpack-c/wiki/v2_0_c_overview
