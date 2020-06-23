# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

from distutils.core import setup, Extension
from glob import glob
from os import path
from sys import platform


# Add platform specific settings for building the extension module
if platform == "darwin":
    # macOS
    library_dirs = [
        "/usr/local/opt/nss/lib",
        "/usr/local/opt/nspr/lib",
        "/usr/local/opt/msgpack/lib",
    ]
    include_dirs = [
        "/usr/local/opt/nss/include/nss",
        "/usr/local/opt/nspr/include/nspr",
        "/usr/local/opt/msgpack/include",
    ]
else:
    # Fedora
    library_dirs = []
    include_dirs = ["/usr/include/nspr", "/usr/include/nss"]


extension_mod = Extension(
    "prio._libprio",
    sources=["libprio.i"],
    library_dirs=["libprio/build/prio", "libprio/build/mpi"] + library_dirs,
    include_dirs=include_dirs,
    libraries=["mprio", "mpi", "msgpackc", "nss3", "nspr4"],
    swig_opts=["-outdir", "prio"]
)

setup(
    name="prio",
    version="1.0",
    description="An interface to libprio",
    # long_description=long_description,
    # long_description_content_type="text/markdown",
    author="Anthony Miyaguchi",
    author_email="amiyaguchi@mozilla.com",
    url="https://github.com/mozilla/libprio",
    packages=["prio"],
    ext_modules=[extension_mod],
)
