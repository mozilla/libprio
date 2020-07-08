# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

from setuptools import setup, Extension
from glob import glob
from os import path
from sys import platform


def readme():
    with open("README.md") as fp:
        return fp.read()


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
    sources=["swig/libprio.i"],
    library_dirs=["libprio/build/prio", "libprio/build/mpi"] + library_dirs,
    include_dirs=include_dirs,
    libraries=["mprio", "mpi", "msgpackc", "nss3", "nspr4"],
    swig_opts=["-outdir", "src"],
)

setup(
    name="prio",
    version="1.1",
    description="An interface to libprio",
    long_description=readme(),
    long_description_content_type="text/markdown",
    author="Anthony Miyaguchi",
    author_email="amiyaguchi@mozilla.com",
    url="https://github.com/mozilla/libprio",
    packages=["prio"],
    package_dir={"prio": "src"},
    ext_modules=[extension_mod],
)
