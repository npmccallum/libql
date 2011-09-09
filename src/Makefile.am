AM_CFLAGS = -Wall -g

lib_LTLIBRARIES = libql.la
noinst_PROGRAMS = test

libql_la_SOURCES = libql.c libql.h
libql_la_LDFLAGS = -export-symbols-regex '^ql_'

test_SOURCES = test.c
test_LDADD   = libql.la


