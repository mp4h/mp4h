#! /bin/sh

#   Run this script when reconfiguring

libtoolize --copy --force --ltdl
autoheader
autoconf

