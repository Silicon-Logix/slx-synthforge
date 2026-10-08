# Silicon LogiX / SLX SynthForge
# Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
# Author: Marco Pezzullo
# License: Silicon LogiX Evaluation License 1.0. See LICENSE.

"""Fetch and verify the pinned corresponding sources for the approved Windows kit."""
import argparse
import concurrent.futures
import hashlib
import json
from pathlib import Path
import re
import shutil
import tarfile
import urllib.request

# Origins and checksums of the source materials corresponding to the supplied kit.
SOURCES = [
    {
        "file": "qtbase-everywhere-src-6.12.0.tar.xz",
        "url": "https://download.qt.io/archive/qt/6.12/6.12.0/submodules/qtbase-everywhere-src-6.12.0.tar.xz",
        "bytes": 51220268,
        "sha256": "a951bd163c7b80fc6b8c88d7668fb56abf91c152373e13c10666763238131307"
    },
    {
        "file": "qtdeclarative-everywhere-src-6.12.0.tar.xz",
        "url": "https://download.qt.io/archive/qt/6.12/6.12.0/submodules/qtdeclarative-everywhere-src-6.12.0.tar.xz",
        "bytes": 39221652,
        "sha256": "311f3a2603e1973bb59baef9dfa740a376de713157d4782ee043681e889c9260"
    },
    {
        "file": "qtsvg-everywhere-src-6.12.0.tar.xz",
        "url": "https://download.qt.io/archive/qt/6.12/6.12.0/submodules/qtsvg-everywhere-src-6.12.0.tar.xz",
        "bytes": 2341876,
        "sha256": "e4ab39534ec97987b1b9b60ef7f4d3253d5912a69f8c713a01753174a4029331"
    },
    {
        "file": "qtshadertools-everywhere-src-6.12.0.tar.xz",
        "url": "https://download.qt.io/archive/qt/6.12/6.12.0/submodules/qtshadertools-everywhere-src-6.12.0.tar.xz",
        "bytes": 1244684,
        "sha256": "c7d84f436e1aaef39fdcadebf2bd71bdf24dc69497e13f2e0198873cbbfea2ab"
    },
    {
        "file": "gcc-13.1.0.tar.xz",
        "url": "https://ftp.gnu.org/gnu/gcc/gcc-13.1.0/gcc-13.1.0.tar.xz",
        "bytes": 87451196,
        "sha256": "61d684f0aa5e76ac6585ad8898a2427aade8979ed5e7f85492286c4dfc13ee86"
    },
    {
        "file": "mingw-w64-v11.0.0.tar.gz",
        "url": "https://codeload.github.com/mingw-w64/mingw-w64/tar.gz/refs/tags/v11.0.0",
        "bytes": 13920612,
        "sha256": "760fc2b908418060f5ccfdf9740ea9e6560f60b155d4ebb1608348fc56abf386"
    },
    {
        "file": "mingw-builds-7ff96692a8032d893781523b311ac9b6cbbda0a1.tar.gz",
        "url": "https://codeload.github.com/niXman/mingw-builds/tar.gz/7ff96692a8032d893781523b311ac9b6cbbda0a1",
        "bytes": 578807,
        "sha256": "cb92e4c9df87cd3536a4f9318620a3153bae3de813207aded08a6747ba2d0fc1"
    }
]
COMMIT = "7ff96692a8032d893781523b311ac9b6cbbda0a1"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1] / "dist/runtime-sources-0.4")
    parser.add_argument("--qt-root", type=Path, default=Path("C:/Qt/6.12.0/mingw_64"))
    parser.add_argument("--toolchain-root", type=Path, default=Path("C:/Qt/Tools/mingw1310_64"))
    args = parser.parse_args()
    output, qt, kit = args.output.resolve(), args.qt_root, args.toolchain_root
    output.mkdir(parents=True, exist_ok=True)

    def fetch(record):
        path = output / record["file"]
        if not path.exists():
            temporary = path.with_suffix(path.suffix + ".part")
            with urllib.request.urlopen(record["url"], timeout=30) as response, temporary.open("wb") as target:
                shutil.copyfileobj(response, target)
            temporary.replace(path)
        data = path.read_bytes()
        if len(data) != record["bytes"] or hashlib.sha256(data).hexdigest() != record["sha256"]:
            raise ValueError("Source archive checksum mismatch: " + record["file"])
        with tarfile.open(path) as archive:
            if not archive.getnames():
                raise ValueError("Empty source archive")
        print("Verified " + record["file"], flush=True)
        return record

    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
        records = list(executor.map(fetch, SOURCES))
    build_info = (kit / "build-info.txt").read_text(encoding="utf-8")
    if "gcc-13.1.0" not in build_info or "mingw-w64" not in build_info:
        raise ValueError("Toolchain build information does not match the approved kit")
    patches = sorted(set(re.findall(r"gcc/[^,\s]+\.patch", build_info)))
    if not patches:
        raise ValueError("No GCC runtime patch provenance found")
    with tarfile.open(output / f"mingw-builds-{COMMIT}.tar.gz") as archive:
        members = archive.getnames()
        if any(not any(name.endswith("/patches/" + patch) for name in members) for patch in patches):
            raise ValueError("Build-script snapshot lacks a required GCC runtime patch")
    runtimes = {}
    for name in ("libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll"):
        data = (kit / "bin" / name).read_bytes()
        if data != (qt / "bin" / name).read_bytes():
            raise ValueError("Qt and toolchain runtime DLLs differ: " + name)
        runtimes[name] = hashlib.sha256(data).hexdigest()
    shutil.copy2(kit / "build-info.txt", output / "mingw-build-info.txt")
    for source in (qt / "sbom").glob("*.spdx"):
        if source.name.startswith(("qtbase-", "qtdeclarative-", "qtsvg-", "qtshadertools-")):
            shutil.copy2(source, output / source.name)
    for source in (qt / "mkspecs/qconfig.pri", qt / "mkspecs/qmodule.pri"):
        shutil.copy2(source, output / source.name)
    manifest = {"schema": "siliconlogix.runtime-sources.v1", "qt": "6.12.0", "gcc": "13.1.0", "mingw_w64": "11.0.0",
                "mingw_build_scripts_commit": COMMIT, "source_archives": records,
                "build_script_selection": "Pinned develop snapshot at or before the installed runtime build timestamp; every GCC runtime patch listed by build-info is verified in the archive.",
                "runtime_files": runtimes, "gcc_runtime_patches_verified": patches}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("Corresponding sources verified: " + str(output), flush=True)


if __name__ == "__main__":
    main()
