#!/usr/bin/env python3
"""Run an NCD COFF image on MAME ncd19c; serial channel B is captured over TCP.

run-ncd19c.py IMG [--mode inject|netboot] [--port 2340] [--wait 20] [--timeout-emu 120]
              [--expect TEXT] [--shot PNG]
MAME always runs in a window. Only the MAME process started here is ever killed.
"""
import argparse, hashlib, os, shutil, socket, subprocess, sys, tempfile, time

MAME_DIR = "/home/mk2s/Documents/mame"
TFTP = os.path.expanduser("~/Documents/mame/tftpboot/0A000205")
HERE = os.path.dirname(os.path.abspath(__file__))
EOT = 4


def sha(p):
    return hashlib.sha256(open(p, "rb").read()).hexdigest()


def run(a):
    img = os.path.abspath(a.img)
    backup = TFTP + ".baremetal.bak"
    if a.mode == "netboot":
        if subprocess.run(["pgrep", "-x", "mame"], capture_output=True).returncode == 0:
            sys.exit("refusing netboot: another mame process is running")
        if os.path.exists(backup) and sha(backup) != sha(TFTP):
            sys.exit("refusing: %s exists and differs from %s" % (backup, TFTP))
        if not os.path.exists(backup):
            shutil.copy2(TFTP, backup)
    try:
        return _run(a, img)
    finally:
        if a.mode == "netboot":
            shutil.copy2(backup, TFTP)
            if sha(TFTP) != sha(backup):
                print("ERROR: restore of %s failed sha256 check" % TFTP, file=sys.stderr)
            else:
                os.unlink(backup)


def _run(a, img):
    if a.mode == "netboot":
        shutil.copy2(img, TFTP)
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", a.port))
    srv.listen(1)
    srv.settimeout(5)
    flag = tempfile.mktemp(prefix="ncd-eot-")
    # MAME writes cfg/ncd19c.cfg on exit. With -networkprovider none it would store the LANCE as
    # unbound (interface="-1") and break netboot for later runs, so use a private copy.
    cfgdir = tempfile.mkdtemp(prefix="ncd-cfg-")
    if os.path.exists(os.path.join(MAME_DIR, "cfg", "ncd19c.cfg")):
        shutil.copy2(os.path.join(MAME_DIR, "cfg", "ncd19c.cfg"), cfgdir)
    env = dict(os.environ, NCD_MODE=a.mode, NCD_IMAGE=img, NCD_WAIT=str(a.wait),
               NCD_TIMEOUT=str(a.timeout_emu), NCD_SHOT=os.path.abspath(a.shot) if a.shot else "",
               NCD_EOT_FLAG=flag)
    cmd = [MAME_DIR + "/mame", "ncd19c", "-window", "-nothrottle", "-sound", "none",
           "-skip_gameinfo", "-serial1", "null_modem", "-bitb", "socket.127.0.0.1:%d" % a.port,
           "-seconds_to_run", str(int(a.wait + a.timeout_emu + 60)),
           "-cfg_directory", cfgdir, "-autoboot_script", os.environ.get("NCD_LUA", os.path.join(HERE, "ncd19c-load.lua"))]
    if a.mode == "inject":
        cmd += ["-networkprovider", "none"]
    proc = subprocess.Popen(cmd, cwd=MAME_DIR, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    out, seen_eot = bytearray(), False
    try:
        conn = None
        deadline = time.time() + a.wall
        while conn is None and time.time() < deadline and proc.poll() is None:
            try:
                conn, _ = srv.accept()
            except socket.timeout:
                pass
        if conn:
            conn.settimeout(2)
            while time.time() < deadline and not seen_eot:
                try:
                    d = conn.recv(4096)
                except socket.timeout:
                    continue
                if not d:
                    break
                if EOT in d:
                    seen_eot = True
                    d = d[:d.index(EOT)]
                    open(flag, "w").close()
                d = d.replace(b"\r", b"")
                out += d
                sys.stdout.write(d.decode("latin1"))
                sys.stdout.flush()
        if seen_eot:   # let Lua take the screenshot and exit
            try:
                proc.wait(timeout=a.grace)
            except subprocess.TimeoutExpired:
                pass
    finally:
        if proc.poll() is None:
            proc.kill()
        mout = proc.communicate()[0]
        if os.path.exists(flag):
            os.unlink(flag)
        shutil.rmtree(cfgdir, ignore_errors=True)
        srv.close()
    for l in mout.splitlines():
        if l.startswith("NCD ") or not seen_eot:
            print("[mame]", l, file=sys.stderr)
    ok = seen_eot and (a.expect is None or a.expect.encode() in out)
    if not seen_eot:
        print("FAIL: no EOT seen", file=sys.stderr)
    elif not ok:
        print("FAIL: expected text %r not found" % a.expect, file=sys.stderr)
    return 0 if ok else 1


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("img")
    p.add_argument("--mode", choices=["inject", "netboot"], default="inject")
    p.add_argument("--port", type=int, default=2340)
    p.add_argument("--wait", type=float, default=20)
    p.add_argument("--timeout-emu", type=float, default=120)
    p.add_argument("--wall", type=float, default=600, help="wall-clock limit in seconds")
    p.add_argument("--grace", type=float, default=60, help="seconds to let MAME finish after EOT")
    p.add_argument("--expect")
    p.add_argument("--shot")
    sys.exit(run(p.parse_args()))
