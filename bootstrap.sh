#! /bin/sh

#   Run this script when reconfiguring

aclocal
libtoolize --copy --force --ltdl
autoheader
autoconf

