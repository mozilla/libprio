# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import SCons

opts = Variables()
opts.AddVariables(
    BoolVariable("DEBUG", "Make debug build", 0),
    BoolVariable("SANITIZE", "Use AddressSanitizer and UBSanitizer", 0),
    BoolVariable("VERBOSE", "Show full build information", 0))

env = Environment(options = opts,
                  ENV = os.environ)

env.Tool('clang')

# Add extra compiler flags from the environment
for flag in ["CCFLAGS", "CFLAGS", "CPPFLAGS", "CXXFLAGS", "LINKFLAGS"]:
  if flag in os.environ:
    env.Append(**{flag: SCons.Util.CLVar(os.getenv(flag))})

if env["DEBUG"]: 
    print("DEBUG MODE!")
    env.Append(CCFLAGS = ["-g", "-DDEBUG"])

if env["SANITIZE"]:
    sanitizers = ['-fsanitize=address,undefined', \
                  '-fno-sanitize-recover=undefined,integer,nullability']
    env.Append(CCFLAGS = sanitizers, LINKFLAGS = sanitizers)

env.Append(
  LIBS = ["mprio", "mpi", "nss3", "nspr4"],
  LIBPATH = ['#build/prio', "#build/mpi"],
  CFLAGS = [ "-Wall", "-Werror", "-Wextra", "-O3", "-std=c99", "-Wvla",
    "-I/usr/include/nspr", "-I/usr/include/nss", "-Impi", "-DDO_PR_CLEANUP"])

env.Append(CPPPATH = ["#include", "#."])
Export('env')

SConscript('mpi/SConscript', variant_dir='build/mpi')
SConscript('pclient/SConscript', variant_dir='build/pclient')
SConscript('prio/SConscript', variant_dir='build/prio')
SConscript('ptest/SConscript', variant_dir='build/ptest')

