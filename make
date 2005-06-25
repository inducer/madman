#! /bin/bash

if -x ./scons.py; then
  ./scons.py "$@"
else
  scons "$@"
fi
