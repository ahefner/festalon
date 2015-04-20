#!/bin/sh

libtoolize --force --copy
aclocal
autoconf
automake -a -c

rm autom4te.cache/*
rmdir autom4te.cache
