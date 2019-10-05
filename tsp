#!/bin/sh

# if rlwrap is not installed, run without it
command -v rlwrap >/dev/null 2>&1 || {
  ./tisp $@ -c "(repl)"
  exit 0
}
rlwrap -ncr -M ".tsp" -pWhite -q"\"" ./tisp $@ -c "(repl)"
