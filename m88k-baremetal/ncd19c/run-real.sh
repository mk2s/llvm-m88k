#!/bin/bash
# Usage: run-real.sh IMG.coff [SECONDS]
# Runs a BSP image on the real NCD 19c: backs up the TFTP file 0A000207 (sha256 checked on restore),
# copies the image over it, captures DUART B (/dev/ttyUSB0, 9600 8N1, read-only) until EOT or timeout,
# then restores the original. The user starts the TFTP server beforehand and power-cycles the unit
# after "READY" is printed. Output: out/real/NAME.serial
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
IMG="${1:?usage: run-real.sh IMG.coff [SECONDS]}"
SECS="${2:-600}"
TFTP="${TFTP:-/home/mk2s/Documents/mame/tftpboot}"
TARGET="$TFTP/0A000207"
NAME="$(basename "$IMG" .coff)"
mkdir -p "$HERE/out/real"
BAK="$TARGET.real-$(date +%Y%m%d-%H%M%S).bak"
cp -p "$TARGET" "$BAK"
SUM="$(sha256sum "$TARGET" | cut -d' ' -f1)"
restore() {
  cp -p "$BAK" "$TARGET"
  [ "$(sha256sum "$TARGET" | cut -d' ' -f1)" = "$SUM" ] && echo "restored $TARGET (sha256 ok, backup $BAK)" || echo "RESTORE MISMATCH: $BAK"
}
trap restore EXIT
cp "$IMG" "$TARGET"
echo "READY: $TARGET = $IMG; power-cycle the unit now (capture for up to ${SECS}s)"
python3 "$HERE/out/real/capture.py" "$HERE/out/real/$NAME.serial" "$SECS"
echo "capture done: $(wc -c < "$HERE/out/real/$NAME.serial") bytes"
