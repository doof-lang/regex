#!/bin/sh
# Builds lib/libpcre2-8.a and installs the public PCRE2 header into include/.
# Runs inside a private copy of vendor/ with CC, AR, CFLAGS, and DOOF_JOBS
# supplied by the Doof compiler.
set -eu

library=libpcre2-8.a
defines="-DHAVE_CONFIG_H -DPCRE2_CODE_UNIT_WIDTH=8 -DPCRE2_STATIC -DSUPPORT_PCRE2_8 -DSUPPORT_UNICODE"
case "${DOOF_PLATFORM:-}" in macos|linux) defines="$defines -DHAVE_UNISTD_H" ;; esac
includes="-Ipcre2/src"
jobs=${DOOF_JOBS:-1}
sources=''
for name in auto_possess chkdint chartables compile compile_cgroup compile_class config context convert \
  dfa_match error extuni find_bracket jit_compile maketables match match_data match_next newline ord2utf \
  pattern_info script_run serialize string_utils study substitute substring tables ucd valid_utf xclass; do
  sources="$sources pcre2/src/pcre2_$name.c"
done

install_headers() { cp pcre2/src/pcre2.h include/; }

mkdir -p include lib obj
install_headers

pids=''
active=0
status=0
objects=''
for source in $sources; do
  object="obj/$(echo "$source" | sed 's|/|_|g; s|\.c$|.o|')"
  objects="$objects $object"
  # shellcheck disable=SC2086
  $CC $CFLAGS $defines $includes -c "$source" -o "$object" &
  pids="$pids $!"
  active=$((active + 1))
  if [ "$active" -ge "$jobs" ]; then
    for pid in $pids; do wait "$pid" || status=1; done
    pids=''
    active=0
  fi
done
for pid in $pids; do wait "$pid" || status=1; done
[ "$status" -eq 0 ]

rm -f "lib/$library"
# shellcheck disable=SC2086
$AR rcs "lib/$library" $objects
