#!/usr/bin/python3

"""Build fakerm."""
from __future__ import annotations

import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

configure_cmd = [
    "cmake",
    "-S",
    ".",
    "-B",
    "build",
    "-DCMAKE_BUILD_TYPE=Release",
    f"-DCMAKE_INSTALL_PREFIX={Path.cwd() / 'out'}",
]

build_cmd = [
    "cmake",
    "--build",
    "build",
    "--config",
    "Release",
]

install_cmd = [
    "cmake",
    "--install",
    "build",
    "--config",
    "Release",
]


def build_fakerm() -> tuple[Path, str]:
    """Build fakerm."""
    if not shutil.which("cmake"):
        msg = "CMake is required to build fakerm."
        raise RuntimeError(msg)

    if Path("out").exists():
        sys.stdout.write("Removing out directory...\n")
        shutil.rmtree("out", ignore_errors=True)
    sys.stdout.write("Building fakerm...\n")
    Path("build").mkdir(exist_ok=True)
    Path("out").mkdir(exist_ok=True)
    with subprocess.Popen(configure_cmd) as proc:  # noqa: S603
        if proc.wait() != 0:
            msg = "CMake configure failed."
            raise RuntimeError(msg)

    with subprocess.Popen(build_cmd) as proc:  # noqa: S603
        if proc.wait() != 0:
            msg = "CMake build failed."
            raise RuntimeError(msg)

    with subprocess.Popen(install_cmd) as proc:  # noqa: S603
        if proc.wait() != 0:
            msg = "CMake install failed."
            raise RuntimeError(msg)

    out_binary = Path("out") / "bin" / "fakerm"
    sha256_hash = hashlib.sha256(out_binary.read_bytes()).hexdigest()

    return out_binary, sha256_hash


if __name__ == "__main__":
    out_bin, sha256 = build_fakerm()
    sys.stdout.write(f"{out_bin} {sha256}\n")
