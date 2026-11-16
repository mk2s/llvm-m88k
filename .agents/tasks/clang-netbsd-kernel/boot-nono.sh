#!/bin/sh
# Boot a luna88k kernel ELF headless in nono and capture the serial console.
# usage: boot-nono.sh <kernel-elf> <logfile> [seconds, default 20]
# nono's stdio console needs a tty, so script(1) provides a pty; the TCP
# console (hostcom-driver=tcp) drops output depending on when the client
# connects, so it is not used for logs.
K=$1; L=$2; T=${3:-20}
D=$(cd "$(dirname "$0")" && pwd)
W=/tmp/nb-boot; mkdir -p $W; cd $W || exit 1
# nono itself runs under timeout -s KILL so no nono process is left behind.
(sleep $((T + 1))) | timeout -s KILL $((T + 3)) script -q -f -c \
  "timeout -s KILL $T nono -c $D/nono-headless.cfg -V hostcom-driver=stdio -X $K" $L.raw >/dev/null 2>&1
# drop script(1)'s header/trailer lines and CRs
grep -a -v '^Script \(started\|done\)' $L.raw | tr -d '\r' > $L
rm -f $L.raw
