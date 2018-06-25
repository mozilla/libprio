"""
Copyright (c) 2012-2014 Stanford University
Copyright (c) 2018 Henry Corrigan-Gibbs

Permission to use, copy, modify, and distribute this software for any 
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR 
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
"""

import sys
import os
import multiprocessing
import SCons.Util


## Helper Functions

def CheckPkgConfig(context):
    context.Message('Checking for pkg-config... ')
    ret = context.TryAction('pkg-config --version')[0]
    context.Result(ret)
    return ret

def CheckPkg(context, name):
    context.Message('Checking for %s... ' % name)
    ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0]
    context.Result(ret)
    return ret

def CheckPkgMinVersion(context, name, version):
    context.Message('Checking %s-%s or greater... ' % (name, version))
    ret = context.TryAction('pkg-config --atleast-version \'%s\' \'%s\'' % (version, name))[0]
    context.Result(ret)
    return ret

## Configuration

opts = Variables('Local.sc')

opts.AddVariables(
    ("CC", "C Compiler"),
    ("AS", "Assembler"),
    ("LINK", "Linker"),
    ("NUMCPUS", "Number of CPUs to use for build (0 means auto)", 0, None, int),
    EnumVariable("BUILDTYPE", "Build type", "DEBUG", ["RELEASE", "DEBUG", "PERF"]),
    BoolVariable("VERBOSE", "Show full build information", 0),
    BoolVariable("WITH_GPROF", "Enable profiling", 0),
    BoolVariable("BUILD_BINARIES", "Build binaries", 1),
    PathVariable("PREFIX", "Installation target directory", "/usr/local/bin/", PathVariable.PathAccept),
    PathVariable("DESTDIR", "The root directory to install into. Useful mainly for binary package building", "", PathVariable.PathAccept),
)

env = Environment(options = opts,
                  tools = ['default'],
                  ENV = os.environ)
Help(opts.GenerateHelpText(env))

# Copy environment variables
if os.environ.has_key('CC'):
    env["CC"] = os.getenv('CC')
if os.environ.has_key('AS'):
    env["AS"] = os.getenv('AS')
if os.environ.has_key('LD'):
    env["LINK"] = os.getenv('LD')
if os.environ.has_key('CFLAGS'):
    env.Append(CCFLAGS = SCons.Util.CLVar(os.environ['CFLAGS']))
if os.environ.has_key('CPPFLAGS'):
    env.Append(CPPFLAGS = SCons.Util.CLVar(os.environ['CPPFLAGS']))
if os.environ.has_key('LDFLAGS'):
    env.Append(LINKFLAGS = SCons.Util.CLVar(os.environ['LDFLAGS']))

env.Append(CFLAGS = [ "-Wall", "-Werror", "-Wextra", "-O3", "-std=c99", "-I/usr/include/nspr"])

if "WITH_GPROF" in env and env["WITH_GPROF"]:
    print "With gprof"
    env.Append(CFLAGS = [ "-pg" ])
    env.Append(CPPFLAGS = [ "-pg" ])
    env.Append(LINKFLAGS = [ "-pg" ])

if env["BUILDTYPE"] == "DEBUG":
    print "DEBUG MODE!"
    env.Append(CPPFLAGS = [ "-g", "-DDEBUG"])
elif env["BUILDTYPE"] == "RELEASE":
    env.Append(CPPFLAGS = ["-DNDEBUG"])
else:
    print "Error BUILDTYPE must be RELEASE or DEBUG"
    sys.exit(-1)

if not env["VERBOSE"]:
    env["CCCOMSTR"] = "Compiling $SOURCE"
    env["SHCCCOMSTR"] = "Compiling $SOURCE"
    env["ARCOMSTR"] = "Creating library $TARGET"
    env["RANLIBCOMSTR"] = "Indexing library $TARGET"
    env["LINKCOMSTR"] = "Linking $TARGET"

def GetNumCPUs(env):
    if env["NUMCPUS"] > 0:
        return int(env["NUMCPUS"])
    return 2*multiprocessing.cpu_count()

env.SetOption('num_jobs', GetNumCPUs(env))

# Configuration
conf = env.Configure(custom_tests = { 'CheckPkgConfig' : CheckPkgConfig,
                                      'CheckPkg' : CheckPkg,
                                      'CheckPkgMinVersion' : CheckPkgMinVersion })

if not conf.CheckCC():
    print 'Your C compiler and/or environment is incorrectly configured.'
    CheckFailed()

conf.Finish()

Export('env')

# Set compile options for binaries
env.Append(LIBS = ["mprio", "mpi", "nss3", "nspr4"], \
  LIBPATH = ['#build/prio', "#build/mpi"])

# Add header files
env.Append(CPPPATH = ["#include", "#."])

SConscript('prio/SConscript', variant_dir='build/prio')
SConscript('mpi/SConscript', variant_dir='build/mpi')

# Utilities
if env["BUILD_BINARIES"]:
    SConscript('pclient/SConscript', variant_dir='build/pclient')
    SConscript('ptest/SConscript', variant_dir='build/ptest')

