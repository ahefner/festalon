#!/bin/sh

libtoolize --force --copy
aclocal-1.6
autoconf
automake-1.6 -a -c

rm autom4te.cache/*
rmdir autom4te.cache
