#!/bin/sh

# if rlwrap is not installed, run without it
command -v rlwrap >/dev/null 2>&1 || {
  ./tisp $@
  exit $?
}
rlwrap -ncr -M ".tsp" -pWhite -q"\"" ./tisp $@
