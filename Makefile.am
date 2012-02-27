ACLOCAL_AMFLAGS = -I m4
SUBDIRS = . tests

AM_CFLAGS = $(LIBSC_CFLAGS) -Wall -g -I$(srcdir)/src
AM_LDFLAGS = $(LIBSC_LIBS) -no-undefined -export-symbols-regex '^ql_'

lib_LTLIBRARIES    = libql.la
include_HEADERS    = src/libql.h

libql_la_SOURCES = src/libql.c src/libql-internal.h

if WITH_ASSEMBLY
AM_CFLAGS  += -DWITH_ASSEMBLY=1
libql_la_SOURCES += src/libql-assembly.c src/libql-assembly-@ASMARCH@.S
endif

if WITH_UCONTEXT
AM_CFLAGS  += -DWITH_UCONTEXT=1
libql_la_SOURCES += src/libql-ucontext.c
endif

if WITH_PTHREAD
AM_CFLAGS  += -DWITH_PTHREAD=1 $(PTHREAD_CFLAGS)
AM_LDFLAGS += $(PTHREAD_LIBS)
libql_la_SOURCES += src/libql-pthread.c
endif

# pkgconfig
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libql.pc
