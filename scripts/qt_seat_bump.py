#!/usr/bin/env python3
"""Patch libQt6WaylandCompositor to advertise wl_seat v5 and wl_data_device_manager v3.

Qt 6.4.2 hardcodes:
  QWaylandSeat::initialize()        -> d->init(display, 4)
  DataDeviceManager::DataDeviceManager() -> wl_data_device_manager(display, 1)

foot >= 1.16 requires wl_seat >= 5 and wl_data_device_manager >= 3, so we bump
the two immediates in the prebuilt library. Idempotent; verifies after patch.
"""
import re
import subprocess
import sys
import os

TARGETS = [
    {
        "name": "wl_seat v4 -> v5",
        "mov_bytes": ["ba 04 00 00 00", "ba 05 00 00 00"],
        "old_imm": "04",
        "new_imm": "05",
        "call_syms": ["_ZN15QtWaylandServer7wl_seat4initEP10wl_displayi"],
    },
    {
        "name": "wl_data_device_manager v1 -> v3",
        "mov_bytes": ["ba 01 00 00 00", "ba 03 00 00 00"],
        "old_imm": "01",
        "new_imm": "03",
        "call_syms": [
            "_ZN15QtWaylandServer22wl_data_device_managerC1EP10wl_displayi",
            "_ZN15QtWaylandServer22wl_data_device_managerC2EP10wl_displayi",
            "_ZN15QtWaylandServer22wl_data_device_manager4initEP10wl_displayi",
        ],
    },
]


def find_library():
    import glob
    if len(sys.argv) > 1:
        return sys.argv[1]
    candidates = []
    roots = [
        "/home/z/Qt/6.4.2/gcc_64/lib",
        os.path.expanduser("~/Qt/6.4.2/gcc_64/lib"),
        "/usr/lib/x86_64-linux-gnu",
        "/usr/lib/aarch64-linux-gnu",
    ]
    for root in roots:
        candidates.append(os.path.join(root, "libQt6WaylandCompositor.so.6.4.2"))
        candidates.extend(sorted(glob.glob(os.path.join(root, "libQt6WaylandCompositor.so.6.*.*")),
                                 reverse=True))
    for p in candidates:
        if os.path.exists(p) and not os.path.islink(p):
            return p
    raise SystemExit("libQt6WaylandCompositor.so.6.* not found; pass path as argv[1]")


def disasm(so):
    out = subprocess.run(["objdump", "-d", so],
                         capture_output=True, text=True, check=True).stdout
    insns = []
    for line in out.splitlines():
        m = re.match(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*\t(.*)$", line)
        if not m:
            continue
        insns.append((int(m.group(1), 16), m.group(2).strip(), m.group(3).strip()))
    return insns


def sections(so):
    out = subprocess.run(["objdump", "-h", so], capture_output=True, text=True, check=True).stdout
    secs = []
    for line in out.splitlines():
        m = re.match(r"^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+", line)
        if m:
            secs.append((m.group(1), int(m.group(2), 16), int(m.group(3), 16), int(m.group(5), 16)))
    return secs


def vaddr_to_off(secs, vaddr):
    for name, size, vma, off in secs:
        if vma <= vaddr < vma + size:
            return off + (vaddr - vma)
    raise SystemExit(f"vaddr {vaddr:#x} not in any section")


def main():
    so = find_library()
    print(f"[seat-bump] library: {so}")
    insns = disasm(so)
    secs = sections(so)
    patched = 0

    for t in TARGETS:
        found = False
        for i, (addr, raw, text) in enumerate(insns):
            if not any(raw.startswith(mb) for mb in t["mov_bytes"]):
                continue
            window = insns[i + 1:i + 5]
            hit = any(mnem.startswith("call") and any(s in mnem for s in t["call_syms"])
                      for _, _, mnem in window)
            if not hit:
                continue
            imm_addr = addr + 1
            off = vaddr_to_off(secs, imm_addr)
            with open(so, "r+b") as f:
                f.seek(off)
                cur = f.read(1)
                if cur == bytes([int(t["new_imm"], 16)]):
                    print(f"[seat-bump] {t['name']}: already patched at {imm_addr:#x}")
                    found = True
                    patched += 1
                    break
                if cur != bytes([int(t["old_imm"], 16)]):
                    print(f"[seat-bump] {t['name']}: unexpected byte {cur.hex()} at {imm_addr:#x}, skip")
                    continue
                f.seek(off)
                f.write(bytes([int(t["new_imm"], 16)]))
            print(f"[seat-bump] {t['name']}: patched {imm_addr:#x}: {t['old_imm']} -> {t['new_imm']}")
            found = True
            patched += 1
            break
        if not found:
            raise SystemExit(f"[seat-bump] FAILED: pattern for {t['name']} not located")

    print(f"[seat-bump] done, {patched}/{len(TARGETS)} applied")
    sys.exit(0 if patched == len(TARGETS) else 1)


if __name__ == "__main__":
    main()
