# debug-demo: source-level debugging on the NCD 19c with LLDB

`demo.c` prints Fibonacci numbers on the screen. It is linked with the RSP stub from
`/home/mk2s/Documents/ncd-88k-xterm/gdbstub`, which talks to the debugger on DUART
channel B (AUX, 9600 8N1). The program itself only writes to the screen, because the
serial port belongs to the debugger.

    python3 build.py [-O0|-O2]    # out/demo.elf (symbols + DWARF), out/demo.img (NCD COFF)

## In Kiro (lldb-dap extension)

`.vscode/launch.json` at the workspace root has "NCD 19c debug-demo (MAME)". It builds,
starts MAME through `gdbstub/tools/mame_harness.py fresh out/demo.img` (window mode),
and attaches `build-lldb/bin/lldb-dap` to 127.0.0.1:2324. Set breakpoints in `demo.c`
first, then start the configuration. The MAME instance is stopped when the session ends.

## On the real 19c

1. `python3 build.py`, then put `out/demo.img` where the unit TFTP-boots it
   (`~/Documents/mame/tftpboot/0A000207`; back up the current file first) and start
   the TFTP server as usual (`sudo ./ncd_tftpd.py --root ./tftpboot --ip 10.0.2.2`).
2. AUX port on `/dev/ttyUSB0`, 9600 8N1. Start the bridge (Kiro task
   "debug-demo: serial bridge (/dev/ttyUSB0)", or `python3 serial_bridge.py`).
   No other program may use the serial port (no m88kdbg.py, no terminal).
3. Boot the unit. The screen stays at the monitor: the stub is stopped at its entry
   breakpoint, waiting for the debugger.
4. Start "NCD 19c debug-demo (real hardware)" in Kiro (or `lldb out/demo.elf`,
   `gdb-remote 127.0.0.1:2324`). Continue, and the Fibonacci lines appear on screen.

After a detach the program keeps running, and the stub cannot be reached again until the
next breakpoint trap; reboot (power cycle) for a new session.

## From the command line

    python3 /home/mk2s/Documents/ncd-88k-xterm/gdbstub/tools/mame_harness.py fresh out/demo.img
    /home/mk2s/Documents/llvm-m88k/build-lldb/bin/lldb out/demo.elf
    (lldb) gdb-remote 127.0.0.1:2324
    (lldb) b fib
    (lldb) c

## Limits

- Pause does not work: the stub only reads the serial port while the target is stopped.
  Use breakpoints.
- Each `next` costs about 1.2 s at 9600 baud (with the 64-byte memory cache line that
  the launch configurations set; LLDB's default of 512 bytes makes it ~2.8 s).
- Breakpoints on trap instructions (tb0/tb1/tcnd) and rte are refused by the stub.
