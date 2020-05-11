# Fuzzing Support in libprio

## Overview

libprio supports fuzzing using [libFuzzer](https://llvm.org/docs/LibFuzzer.html).

To build with fuzzing targets enabled, use

```
$ scons FUZZING=1 SANITIZE=1
```

when building. Building with sanitizers is not strictly required, but highly
recommended for better stack traces and additional memory checking.

## Adding new targets

To add new targets, simply create a new `.c` file in this directory that has
the necessary `LLVMFuzzerTestOneInput` function. It will be built automatically
when building with `FUZZING=1` and create its own binary.

## Existing targets

**server_verify_fuzz**

This target takes data blobs A and B and performs a full server verification
circle. Due to the nature of this target (taking two inputs at the same time),
it uses a custom mutator making it more complex than the usual fuzzing targets.

To run this target, you should use

* the environment variable `ASAN_OPTIONS="max_allocation_size_mb=1024:allocator_may_return_null=1"`
* the argument `-max_len=258`

The `ASAN_OPTIONS` are currently required to avoid crashes due to large unchecked
allocations. This requirement will go away as soon as libprio switches to a
msgpack API that allows limiting the amount of memory allocated per unpack.

The `-max_len` argument is recommended because it limits A and B blobs to
reasonable sizes (128 byte each). Note that the custom mutator format cannot
handle individual blob sizes larger than 255, so 512 byte is the maximum
length that libFuzzer should ever be instructed to use.
