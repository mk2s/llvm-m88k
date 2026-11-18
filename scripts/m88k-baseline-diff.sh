#!/bin/bash
# m88k-baseline-diff.sh: compare two results files written by m88k-baseline.sh.
#
# Usage: m88k-baseline-diff.sh BASE RESULTS
#   BASE and RESULTS are results.txt files, or baseline dirs containing one.
#   Lines look like "<suite>/<name> <STATUS>".
#
# Reports (sections are omitted when empty):
#   NEW FAILURES   passing (or ok) in BASE, failing in RESULTS
#   NEW TESTS FAILING  absent in BASE, failing in RESULTS
#   FIXED          failing in BASE, ok in RESULTS
#   STATUS CHANGED between two failing, or two ok, statuses (e.g. FAIL -> BUILD_FAIL)
#   MISSING        present in BASE, absent in RESULTS (a test that vanished is not a pass)
#   ADDED          ok in RESULTS, absent in BASE (informational)
# ok statuses: PASS XFAIL UNSUPPORTED SKIPPED. Everything else counts as failing.
# Exit status: 0 = no new failures and nothing missing, 1 = regressions or missing tests, 2 = usage.
set -uo pipefail
export LC_ALL=C

[ $# -eq 2 ] || { sed -n '2,/^set -uo/p' "$0" | sed '$d' | sed 's/^# \{0,1\}//' >&2; exit 2; }
resolve() { if [ -d "$1" ]; then echo "$1/results.txt"; else echo "$1"; fi; }
BASE="$(resolve "$1")"
NEW="$(resolve "$2")"
for f in "$BASE" "$NEW"; do [ -r "$f" ] || { echo "cannot read $f" >&2; exit 2; }; done

awk -v base="$BASE" -v new="$NEW" '
function ok(s) { return s == "PASS" || s == "XFAIL" || s == "UNSUPPORTED" || s == "SKIPPED" }
function load(file, arr,   line, k, v, i) {
  while ((getline line < file) > 0) {
    if (line == "") continue
    i = index(line, " ")
    if (i == 0) { k = line; v = "?" } else { k = substr(line, 1, i - 1); v = substr(line, i + 1) }
    arr[k] = v
  }
  close(file)
}
function emit(title, list, n,   i) {
  if (n == 0) return
  printf "%s (%d)\n", title, n
  for (i = 1; i <= n; i++) print "  " list[i]
  print ""
}
BEGIN {
  load(base, b); load(new, r)
  nbase = 0; for (k in b) nbase++
  nnew = 0; for (k in r) nnew++
  nf = nt = nx = nc = nm = na = 0
  for (k in b) {
    if (!(k in r)) { m[++nm] = k " (was " b[k] ")"; continue }
    if (ok(b[k]) && !ok(r[k])) f[++nf] = k ": " b[k] " -> " r[k]
    else if (!ok(b[k]) && ok(r[k])) x[++nx] = k ": " b[k] " -> " r[k]
    else if (b[k] != r[k]) c[++nc] = k ": " b[k] " -> " r[k]
  }
  for (k in r) if (!(k in b)) { if (ok(r[k])) a[++na] = k ": " r[k]; else t[++nt] = k ": " r[k] }
  # sort each list (insertion sort keeps this portable across awk implementations)
  sortarr(f, nf); sortarr(t, nt); sortarr(x, nx); sortarr(c, nc); sortarr(m, nm); sortarr(a, na)
  emit("NEW FAILURES", f, nf)
  emit("NEW TESTS FAILING", t, nt)
  emit("FIXED", x, nx)
  emit("STATUS CHANGED", c, nc)
  emit("MISSING", m, nm)
  emit("ADDED", a, na)
  printf "summary: base=%d new=%d new_failures=%d new_tests_failing=%d fixed=%d status_changed=%d missing=%d added=%d\n", nbase, nnew, nf, nt, nx, nc, nm, na
  exit (nf + nt + nm > 0) ? 1 : 0
}
function sortarr(arr, n,   i, j, v) {
  for (i = 2; i <= n; i++) { v = arr[i]; for (j = i - 1; j >= 1 && arr[j] > v; j--) arr[j + 1] = arr[j]; arr[j + 1] = v }
}
'
