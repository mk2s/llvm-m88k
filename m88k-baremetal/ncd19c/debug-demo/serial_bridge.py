#!/usr/bin/env python3
"""serial_bridge.py - expose the NCD 19c AUX serial port as a TCP port for LLDB.

  serial_bridge.py [--dev /dev/ttyUSB0] [--baud 9600] [--port 2324]

LLDB (gdb-remote) only talks TCP. This listens on 127.0.0.1:PORT and relays bytes to and
from the serial device (8N1, raw). One client at a time; the serial port stays open when
a client disconnects, so debug sessions can come and go while the target keeps its state.
Stop with Ctrl-C. Do not run another client (m88kdbg.py, a terminal) on the device.

Baud switching: the stub's `baud <csr>` monitor command (from LLDB:
`process plugin packet monitor baud cc`) replies at the old rate, waits for the host's
'+' ack, then switches the DUART.  The bridge watches for that command and switches the
serial port right after forwarding the ack.  CSR codes: bb = 9600, cc = 38400 (DUART baud
rate set 1; it would be 19200 in set 2, see --cc-rate).  If the stub hears no good packet at
the new rate it falls back to 9600 on its own; restart the bridge then.
"""
import argparse
import re
import selectors
import socket
import time

import serial


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dev", default="/dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=9600)
    ap.add_argument("--port", type=int, default=2324)
    ap.add_argument("--cc-rate", type=int, default=38400,
                    help="host rate for the stub's CSR code cc (38400 in set 1, 19200 in set 2)")
    a = ap.parse_args()
    rates = {"bb": 9600, "99": 4800, "cc": a.cc_rate}

    ser = serial.Serial(a.dev, a.baud, bytesize=8, parity="N", stopbits=1, timeout=0,
                        xonxoff=False, rtscts=False, dsrdtr=False)
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", a.port))
    srv.listen(1)
    print("bridge: %s @ %d <-> tcp 127.0.0.1:%d" % (a.dev, a.baud, a.port), flush=True)

    sel = selectors.DefaultSelector()
    sel.register(srv, selectors.EVENT_READ, "srv")
    sel.register(ser.fileno(), selectors.EVENT_READ, "ser")
    client = None
    up = b""            # client -> target bytes, scanned for the baud command
    down = b""          # target -> client bytes, scanned for its reply
    want = None         # rate requested by a baud command in flight
    switch = False      # reply seen: switch after the client's next '+'
    try:
        while True:
            for key, _ in sel.select():
                if key.data == "srv":
                    c, addr = srv.accept()
                    if client:
                        c.close()              # only one debugger at a time
                        continue
                    c.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                    c.setblocking(False)
                    client = c
                    sel.register(c, selectors.EVENT_READ, "cli")
                    print("bridge: client connected", flush=True)
                elif key.data == "ser":
                    data = ser.read(4096)
                    if data and client:
                        client.sendall(data)
                    if data and want:
                        down = (down + data)[-64:]
                        if re.search(rb"\$[^#$]*#[0-9a-fA-F]{2}", down):
                            switch = True
                    # bytes that arrive with no client attached are dropped
                elif key.data == "cli":
                    try:
                        data = client.recv(4096)
                    except ConnectionError:
                        data = b""
                    if data:
                        up = (up + data)[-512:]
                        m = re.search(rb"\$qRcmd,([0-9a-fA-F]+)#", up)
                        if m:
                            try:
                                cmd = bytes.fromhex(m.group(1).decode()).decode().split()
                            except ValueError:
                                cmd = []
                            if len(cmd) == 2 and cmd[0] == "baud" and cmd[1].lower() in rates:
                                want, switch, down = rates[cmd[1].lower()], False, b""
                            up = up[m.end():]
                        if switch and b"+" in data:
                            # Send the ack alone at the old rate, wait until it has left
                            # the USB adapter (tcdrain alone returns too early) and the stub
                            # has switched, then send whatever followed it at the new rate.
                            k = data.index(b"+") + 1
                            ser.write(data[:k])
                            ser.flush()
                            time.sleep(0.05)
                            ser.baudrate = want
                            time.sleep(0.01)
                            if data[k:]:
                                ser.write(data[k:])
                            print("bridge: switched to %d baud" % want, flush=True)
                            want, switch, up = None, False, b""
                        else:
                            ser.write(data)
                    else:
                        sel.unregister(client)
                        client.close()
                        client = None
                        print("bridge: client disconnected", flush=True)
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()


if __name__ == "__main__":
    main()
