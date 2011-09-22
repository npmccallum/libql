AM_CFLAGS = -Wall -g -Isrc -Iexamples

# We only actually distrubte our library...
lib_LTLIBRARIES    = libql.la
include_HEADERS    = src/libql.h

# ... but we will build our examples and our test utilities
noinst_LTLIBRARIES = libqlsp.la
noinst_HEADERS     = examples/libqlsp.h
noinst_PROGRAMS    = test benchmark

# The actual library
libql_la_SOURCES = src/libql.c
libql_la_CFLAGS  = $(AM_CFLAGS) 
libql_la_LDFLAGS = -no-undefined -export-symbols-regex '^ql_'
if WITH_ASSEMBLY
libql_la_SOURCES += src/libql-assembly.c src/libql-assembly-@ASMARCH@.S 
libql_la_CFLAGS  += -DWITH_ASSEMBLY=1
endif
if WITH_UCONTEXT
libql_la_SOURCES += src/libql-ucontext.c
libql_la_CFLAGS  += -DWITH_UCONTEXT=1
endif

# qlStatePool example library
libqlsp_la_SOURCES = examples/libqlsp.c examples/libqlsp.h
libqlsp_la_LIBADD  = libql.la

# Our test to make sure it works
test_SOURCES = tests/test.c
test_LDADD   = libql.la

# Our benchmark to make sure it performs
benchmark_SOURCES = tests/benchmark.c
benchmark_LDADD   = libql.la libqlsp.la

# Run our tests during make check
TESTS = test benchmark
