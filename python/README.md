# python-libprio

This module contains a Python wrapper around libprio.

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
