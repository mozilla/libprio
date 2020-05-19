# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import sys
import SCons

opts = Variables()
opts.AddVariables(
    BoolVariable("DEBUG", "Make debug build", 0),
    BoolVariable("SANITIZE", "Use AddressSanitizer and UBSanitizer", 0),
    BoolVariable("FUZZING", "Build with libFuzzer instrumentation and targets", 0),
    BoolVariable("VERBOSE", "Show full build information", 0),
)

env = Environment(options=opts, ENV=os.environ)

env.Tool("clang")

# Add extra compiler flags from the environment
for flag in ["CCFLAGS", "CFLAGS", "CPPFLAGS", "CXXFLAGS", "LINKFLAGS"]:
    if flag in os.environ:
        env.Append(**{flag: SCons.Util.CLVar(os.getenv(flag))})

if env["DEBUG"]:
    print("DEBUG MODE!")
    env.Append(CCFLAGS=["-g", "-DDEBUG"])

if env["SANITIZE"]:
    sanitizers = [
        "-fsanitize=address,undefined",
        "-fno-sanitize-recover=undefined,integer,nullability",
    ]
    if not env["DEBUG"]:
        sanitizers += ["-gline-tables-only"]
    env.Append(CCFLAGS=sanitizers, LINKFLAGS=sanitizers)

if env["FUZZING"]:
    fuzzing = [
        "-fsanitize=fuzzer-no-link",
    ]
    if not env["DEBUG"]:
        fuzzing += ["-gline-tables-only"]
    env.Append(CCFLAGS=fuzzing, LINKFLAGS=fuzzing)

if sys.platform == "darwin":
    env.Append(LINKFLAGS=["-L/usr/local/opt/nss/lib"])
    include_pattern = "-I/usr/local/opt/{0}/include/{0}"
elif sys.platform.startswith("dragonfly") or \
     sys.platform.startswith("freebsd") or \
     sys.platform.startswith("openbsd"):
    env.Append(LINKFLAGS=["-L/usr/local/lib"])
    env.Append(CFLAGS=["-isystem/usr/local/include"])
    include_pattern = "-isystem/usr/local/include/{0}"
else:
    include_pattern = "-I/usr/include/{0}"

env.Append(
    LIBS=["mprio", "mpi", "nss3", "nspr4"],
    LIBPATH=["#build/prio", "#build/mpi"],
    CFLAGS=[
        "-Wall",
        "-Werror",
        "-Wextra",
        "-O3",
        "-std=c99",
        "-Wvla",
        include_pattern.format("nss"),
        include_pattern.format("nspr"),
        "-Impi",
        "-DDO_PR_CLEANUP",
        "-DNSS_PKCS11_2_0_COMPAT",
    ],
)

env.Append(CPPPATH=["#include", "#."])
Export("env")

SConscript("mpi/SConscript", variant_dir="build/mpi")
SConscript("pclient/SConscript", variant_dir="build/pclient")
SConscript("pclient_int/SConscript", variant_dir="build/pclient_int")
SConscript("prio/SConscript", variant_dir="build/prio")
SConscript("ptest/SConscript", variant_dir="build/ptest")

if env["FUZZING"]:
    SConscript("pfuzz/SConscript", variant_dir="build/pfuzz")
