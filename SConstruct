# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import os

opts = Variables()
opts.AddVariables(
    BoolVariable("DEBUG", "Make debug build", 1),
    BoolVariable("VERBOSE", "Show full build information", 0))

env = Environment(options = opts,
                  ENV = os.environ)

if env["DEBUG"]: 
    print "DEBUG MODE!"
    env.Append(CPPFLAGS = [ "-g", "-DDEBUG"])

env.Append(LIBS = ["mprio", "mpi", "nss3", "nspr4"], \
  LIBPATH = ['#build/prio', "#build/mpi"],
  CFLAGS = [ "-Wall", "-Werror", "-Wextra", "-O3", "-std=c99", "-I/usr/include/nspr"])

env.Append(CPPPATH = ["#include", "#."])
Export('env')

SConscript('mpi/SConscript', variant_dir='build/mpi')
SConscript('pclient/SConscript', variant_dir='build/pclient')
SConscript('prio/SConscript', variant_dir='build/prio')
SConscript('ptest/SConscript', variant_dir='build/ptest')

