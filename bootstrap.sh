#!/bin/sh
# Regenerate the GNU Autotools build system from configure.ac / Makefile.am.
# Usage: ./bootstrap.sh && ./configure && make check
set -e
autoreconf --install --force
