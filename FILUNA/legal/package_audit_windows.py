#!/usr/bin/env python3
"""Windows runner only. Relates NSIS installer (extracted with 7z, never executed) to
portable files by sha256, and reads FFmpeg license/configuration strings via ctypes
(no media, no network). Fail-closed: anything unmatched is reported, never hidden."""
from __future__ import annotations
import ctypes, hashlib, json, os, sys
from pathlib import Path


def sha(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for c in iter(lambda: f.read(1 << 20), b""):
            h.update(c)
    return h.hexdigest()


def tree(root: Path) -> dict[str, str]:
    return {p.relative_to(root).as_posix(): sha(p) for p in root.rglob("*") if p.is_file()}


def ffmpeg_info(dll_dir: Path) -> dict:
    os.add_dll_directory(str(dll_dir))
    out = {}
    for lib, prefix in (("avutil-59", "avutil"), ("swresample-5", "swresample"), ("swscale-8", "swscale"),
                        ("avcodec-61", "avcodec"), ("avformat-61", "avformat")):
        p = dll_dir / f"{lib}.dll"
        if not p.is_file():
            out[lib] = {"error": "missing"}
            continue
        d = ctypes.CDLL(str(p))
        r = {}
        for fn in ("license", "configuration"):
            f = getattr(d, f"{prefix}_{fn}")
            f.restype = ctypes.c_char_p
            r[fn] = f().decode(errors="replace")
        v = getattr(d, f"{prefix}_version"); v.restype = ctypes.c_uint
        n = v(); r["version"] = f"{n >> 16}.{(n >> 8) & 255}.{n & 255}"
        r["sha256"] = sha(p)
        out[lib] = r
    return out


def main() -> int:
    inst_dir, port_dir, out = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
    ti, tp = tree(inst_dir), tree(port_dir)
    by_hash_p = {}
    for k, v in tp.items():
        by_hash_p.setdefault(v, []).append(k)
    matched = {k: by_hash_p[v] for k, v in ti.items() if v in by_hash_p}
    only_inst = sorted(k for k in ti if k not in matched)
    inst_hashes = set(ti.values())
    only_port = sorted(k for k, v in tp.items() if v not in inst_hashes)
    exe = next((p for p in port_dir.rglob("orca-slicer.exe")), None)
    res = {"installer_files": len(ti), "portable_files": len(tp), "installer_files_matched_in_portable": len(matched),
           "installer_only": only_inst[:300], "installer_only_count": len(only_inst),
           "portable_only": only_port[:300], "portable_only_count": len(only_port),
           "ffmpeg": ffmpeg_info(exe.parent) if exe else {"error": "orca-slicer.exe not found"}}
    out.write_text(json.dumps(res, indent=1), encoding="utf-8")
    print(json.dumps({k: v for k, v in res.items() if not isinstance(v, (list, dict))}))
    print(json.dumps({k: {"license": v.get("license"), "version": v.get("version")} for k, v in res["ffmpeg"].items()}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
