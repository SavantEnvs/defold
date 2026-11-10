#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

KAT="$SRC/mayhem/build/defold_kat"
if [ ! -x "$KAT" ]; then
  echo "missing $KAT: mayhem/build.sh must build the known-answer probe" >&2
  emit_ctrf "defold-kat" 0 1 0
  exit 1
fi

DL=engine/dlib/src/test/data
MC=engine/modelc/src/test/assets/triangle/gltf
passed=0
failed=0

# expect <name> <expected stdout> <probe args...>: stdout must match exactly and rc must be 0.
expect() {
  local name="$1" want="$2" got rc
  shift 2
  got="$("$KAT" "$@" 2>/dev/null)"
  rc=$?
  got="$(printf '%s\n' "$got" | grep -v '^WARNING:' || true)"   # dlib log lines are not probe output
  if [ "$rc" -eq 0 ] && [ "$got" = "$want" ]; then
    echo "PASS $name"
    passed=$((passed + 1))
  else
    echo "FAIL $name (rc=$rc)" >&2
    printf '  want: %s\n  got:  %s\n' "$want" "$got" >&2
    failed=$((failed + 1))
  fi
}

expect json "nodes=8 list_first=7 name=defold flag=1" json mayhem/kat/sample.json
expect png "2x2 type=1 px=ff0000ff00ff00ff0000ffff0000007f" image "$DL/color_check_2x2.png"
ZIP_WANT='dir/ size=0 dir=1 data=
hello.txt size=10 dir=0 data=48656c6c6f205a69
dir/data.bin size=8 dir=0 data=0001020304050607
empty.txt size=0 dir=0 data='
expect zip_stored "$ZIP_WANT" zip "$DL/zip/archive_stored.zip"
expect zip_deflated "$ZIP_WANT" zip "$DL/zip/archive_deflated.zip"
expect config "foo=123 foo_int=123 pound=#value semi=;value bar=456 ext_string=hello ext_float=1.0" config "$DL/test.config"
expect gltf "finalize=1 validate=1 buffers=1 nodes=1 models=1 vertices=3" model "$MC/Triangle.gltf" "$MC/simpleTriangle.bin"
expect ttf "index_nonzero=1 advance=26.4 ascent_positive=1" font engine/font/src/test/data/WorkSans.ttf

emit_ctrf "defold-kat" "$passed" "$failed" 0
