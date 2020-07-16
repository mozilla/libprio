%module msgpack

// Define wrapper functions for msgpacker_packer to marshall data from the
// msgpack buffer initialized by a library function (e.g.
// PrioPacketVerify1_write) into a Python string.

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

            // free msgpacker buffer
            msgpack_sbuffer_destroy(&sbuf);

            return data;
        }
    %}
%enddef

%inline %{
    #define MSGPACK_INIT_BUFFER_SIZE (size_t)128
%}

%define MSGPACK_READ(TYPE)
    %ignore TYPE ## _read;
    %rename(TYPE ## _read) TYPE ## _read_wrapper;
    %inline %{
        SECStatus TYPE ## _read_wrapper(
            TYPE p, const unsigned char *data, unsigned int len, const_PrioConfig cfg
        ) {
            SECStatus rv = SECFailure;
            msgpack_unpacker upk;
            // Initialize the unpacker with a reasonably sized initial buffer.
            // The unpacker will eat into the initially reserved space for
            // counting and may need to be reallocated.
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

