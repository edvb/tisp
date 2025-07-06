#!/bin/sh

# if rlwrap is not installed, run without it
command -v rlwrap >/dev/null 2>&1 || {
  ./eevo $@
  exit $?
}
rlwrap -ncr -M ".evo" -pWhite -q"\"" ./eevo $@
