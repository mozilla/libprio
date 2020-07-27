# python-libprio

This module contains a Python wrapper around libprio.

## Quickstart

Install the required dependencies via the system package manager and install the
package from pip. The shared libraries for msgpack and NSS must be available for
the module to initialize properly.

On an image derived from Ubuntu:

```bash
docker run -it python:3 bash
apt update && apt install libmsgpackc2 libnss3
```

On Centos:

```bash
docker run -it centos:8 bash
dnf update && dnf install epel-release && dnf install python36 msgpack nss
```

On macOS:

```bash
brew install msgpack nss
```

To test the package:

```bash
pip3 install prio
python3 -c "from prio.libprio import *; Prio_init(); print(PrioPRGSeed_randomize())"
```

## Building from source

Ensure all of the libprio build tools are available, such as `scons` and `clang`

```python
python3 -m venv venv
source venv/bin/activate
make install

# run tests
pip install pytest
python -m pytest
```

To run a coverage report:

```bash
pip install pytest-cov
python -m pytest --cov-report=html --cov-report=term --cov=prio tests
```

## Distributing

Binary eggs and wheels are built for Python 3.6, 3.7, and 3.8 for macOS 10.15
and Linux. From the project root, run the following command to distribute the
wheels for macOS.

```bash
export TWINE_REPOSITORY=testpypi  # optional: for testing
export TWINE_USERNAME=__token__
export TWINE_PASSWORD=<API_KEY>
./scripts/python-dist.sh
```

Run the docker container for distributing the linux wheels.

```bash
docker-compose build libprio-dist
export TWINE_USERNAME=__token__
export TWINE_PASSWORD=<API_KEY>
docker-compose run -e TWINE_USERNAME -e TWINE_PASSWORD libprio-dist
```

## Build Process

There are issues running an installation of the extension module inside of the
current directory (i.e. `libprio/python`). The directory is the first
item on the `PYTHONPATH`. The extension module cannot be referenced until it has
been completely packaged, due to the implicit import statement made by the swig
wrapper:

```python
from . import _libprio
```

The directory sturcture of module after running `python setup.py build` shows the
intermediate shared object stored in a directory underneath build.

```bash
# make

% tree prio build
prio
├── __init__.py
└── libprio.py
build
├── lib.macosx-10.15-x86_64-3.8
│   └── prio
│       ├── __init__.py
│       ├── _libprio.cpython-38-darwin.so
│       └── libprio.py
└── temp.macosx-10.15-x86_64-3.8
    └── libprio_wrap.o
```

The package references are valid once installed into the Python site packages.

```bash
# python3 -m venv venv
# source venv/bin/activate
# make install

% tree venv/lib/python3.8/site-packages/prio
venv/lib/python3.8/site-packages/prio
├── __init__.py
├── __pycache__
│   ├── __init__.cpython-38.pyc
│   └── libprio.cpython-38.pyc
├── _libprio.cpython-38-darwin.so
└── libprio.py

1 directory, 5 files
```

See this [StackOverflow post](https://stackoverflow.com/questions/302867/how-do-i-install-a-python-extension-module-using-distutils) for reference.

## Multiprocessing on Linux

There are certain pathological behaviors that exibit themselves when run on a
specific operating system. Due to the way that NSS is initialized, naive
multiprocessing using the `%init` block and `atexit` in the SWIG interface file
will fail on the multiprocessing test in Linux, but not on macOS. Management of
the Prio context must be done by the library user.

```bash
=================================== FAILURES ===================================
____________________ test_multiprocessing_encoding_succeeds ____________________
multiprocessing.pool.RemoteTraceback:
"""
Traceback (most recent call last):
  File "/usr/lib64/python3.6/multiprocessing/pool.py", line 119, in worker
    result = (True, func(*args, **kwds))
  File "/usr/lib64/python3.6/multiprocessing/pool.py", line 47, in starmapstar
    return list(itertools.starmap(args[0], args[1]))
  File "/app/python/tests/test_libprio_multiprocessing.py", line 14, in _encode
    for_server_a, for_server_b = PrioClient_encode(cfg, data_items)
  File "/usr/local/lib64/python3.6/site-packages/prio/libprio.py", line 380, in PrioClient_encode
    return _libprio.PrioClient_encode(cfg, data_in)
RuntimeError: PrioClient_encode was not successful.
"""

The above exception was the direct cause of the following exception:

    def test_multiprocessing_encoding_succeeds():
        _, pkA = Keypair_new()
        _, pkB = Keypair_new()
        internal_hex = PublicKey_export_hex(pkA)
        external_hex = PublicKey_export_hex(pkB)

        pool_size = 2
        num_elements = 10

        p = Pool(pool_size)
>       res = p.starmap(_encode, [(internal_hex, external_hex)] * num_elements)

python/tests/test_libprio_multiprocessing.py:28:
_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
/usr/lib64/python3.6/multiprocessing/pool.py:274: in starmap
    return self._map_async(func, iterable, starmapstar, chunksize).get()
_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

self = <multiprocessing.pool.MapResult object at 0x7f7a86db0048>, timeout = None

    def get(self, timeout=None):
        self.wait(timeout)
        if not self.ready():
            raise TimeoutError
        if self._success:
            return self._value
        else:
>           raise self._value
E           RuntimeError: PrioClient_encode was not successful.

/usr/lib64/python3.6/multiprocessing/pool.py:644: RuntimeError
=========================== short test summary info ============================
FAILED python/tests/test_libprio_multiprocessing.py::test_multiprocessing_encoding_succeeds
========================= 1 failed, 20 passed in 0.49s =========================
```
