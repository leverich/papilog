#!/usr/bin/python
import os

env = Environment()
conf = env.Configure()

home = os.environ['HOME']

env.Append(CCFLAGS   = '-O3 -Wall -std=c++0x -g')
env.Append(CPPFLAGS  = '-D_GNU_SOURCE -D__STDC_FORMAT_MACROS')
env.Append(LINKFLAGS = '-g')

env.Command(['cmdline.cc', 'cmdline.h'], 'cmdline.ggo', 'gengetopt < $SOURCE')

libs = Split("""papi""")
src = Split("""papilog.cc cmdline.cc log.cc""")

env.Program(target='papilog', source=src, LIBS=[libs])

