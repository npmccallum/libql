AM_CFLAGS = -Wall -g -Isrc -Iexamples

# We only actually distrubte our library...
lib_LTLIBRARIES    = libql.la
include_HEADERS    = src/libql.h

# ... but we will build our examples and our test utilities
noinst_LTLIBRARIES = libqlsp.la
noinst_HEADERS     = examples/libqlsp.h
noinst_PROGRAMS    = test benchmark

# The actual library
libql_la_SOURCES = src/libql.c src/libql.h
libql_la_LDFLAGS = -export-symbols-regex '^ql_'

# qlStatePool example library
libqlsp_la_SOURCES = examples/libqlsp.c examples/libqlsp.h  
libqlsp_la_LDFLAGS = -export-symbols-regex '^ql_'
libqlsp_la_LIBADD  = libql.la

# Our test to make sure it works
test_SOURCES = src/test.c
test_LDADD   = libql.la

# Our benchmark to make sure it performs
benchmark_SOURCES = src/benchmark.c
benchmark_LDADD   = libql.la libqlsp.la
