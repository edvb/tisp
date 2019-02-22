#!/bin/sh

CMD="./tisp $@ tibs/lib.tsp tibs/repl.tsp"

# if rlwrap is not installed, run without it
command -v rlwrap >/dev/null 2>&1 || {
  $CMD
  exit 0
}
rlwrap -nAcr -M ".tsp" -pWhite -q"\"" $CMD
