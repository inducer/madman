#! /bin/bash

if test -x ./scons.py; then
  ./scons.py "$@"
else
  scons "$@"
fi
