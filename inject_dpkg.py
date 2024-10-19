#!/usr/bin/python3

"""Injects fakerm into a .deb package."""

import contextlib
import shutil
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path

import ar

from build_fakerm import build_fakerm

OUTPUT_PATH = "/tmp/__fakerm"  # noqa: S108


def ar_file_exists(archive: ar.Archive, filename: str) -> bool:
    """Check if a file exists in an ar archive.

    Args:
        archive (ar.Archive): The ar archive to search.
        filename (str): The filename to search for.

    Returns:
        bool: True if the file exists in the archive, False otherwise.

    """
    return any(entry.name == filename for entry in archive)


def check_deb(archive: ar.Archive) -> None:
    """Check if the archive is a supported .deb package.

       If not, raise an exception.

    Args:
        archive (ar.Archive): The ar archive to check.

    """
    # See https://wiki.debian.org/Teams/Dpkg/TimeTravelFixes
    if not ar_file_exists(archive, "control.tar.xz"):
        msg = "Not a supported .deb package. Must contain control.tar.xz/gz."
        raise ValueError(msg)


def inject_postinst(
    postinst: Path,
    out_bin: Path,
    sha256sum: str,
) -> None:
    """Inject the fakerm self-extraction snippet into the postinst file.

    Args:
        postinst (Path): The postinst file to inject into.
        out_bin (Path): The path to the fakerm binary.
        sha256sum (str): The sha256sum of the fakerm binary.

    """
    snippet: list[str]
    postinst_data: list[str]
    snippet_in = Path(sys.argv[0]).parent / "self-extract.snippet.sh.template"
    with snippet_in.open("r", encoding="UTF-8") as snippet_fp:
        snippet = snippet_fp.read().strip().splitlines()

    with postinst.open("r", encoding="UTF-8") as postinst_fp:
        postinst_data = postinst_fp.read().strip().splitlines()

    if postinst_data and postinst_data[-1].startswith("exit "):
        postinst_data.pop()

    postinst_data.extend(snippet)

    sys.stdout.write(f"Postinst: {len(postinst_data)} lines\n")
    sha256sum_file_data = f"{sha256sum}  {OUTPUT_PATH}"

    # Configure the snippet.
    configured_postinst: list[str] = []
    for line_ in postinst_data:
        line = line_.replace("@OUTPUT_PATH@", str(OUTPUT_PATH))
        line = line.replace("@LAST_LINE@", str(len(postinst_data) + 1))
        line = line.replace(
            "@BIN_SHA256SUM_FILE@",
            f'"{sha256sum_file_data}"',
        )
        configured_postinst.append(line)
    postinst_data = configured_postinst

    # Write the postinst file.
    with postinst.open("w", encoding="UTF-8") as postinst_fp:
        postinst_fp.write("\n".join(postinst_data) + "\n")

    # Inject the binary.
    with postinst.open("ab") as postinst_fp:
        postinst_fp.seek(0, 2)
        postinst_fp.write(out_bin.read_bytes())
    sys.stdout.write(f"Postinst: injected: {postinst}\n")


def main() -> None:
    """Run main function."""
    if len(sys.argv) != 3:  # noqa: PLR2004
        sys.stdout.write("Usage: inject_dpkg.py <debfile> <output file>\n")
        sys.exit(1)

    debfile = Path(sys.argv[1])
    output_file = Path(sys.argv[2])

    sys.stdout.write(
        "NOTICE: Extracting a DEB package may need root"
        " privileges for some files with special permissions.\n"
        "MAKE SURE TO REVIEW THE FILES EXTRACTED\n"
        "MAKE SURE YOU TRUST THE SOURCE OF THE DEB PACKAGE!!!!!\n",
    )
    input("Press Enter to continue...")

    out_bin, sha256sum = build_fakerm()

    # Specifying encoding for rb mode fp? What the fk?
    with debfile.open("rb") as _fp, ar.Archive(
        _fp,
    ) as archive:
        check_deb(archive)
        # TODO: Support gz.
        with tempfile.TemporaryDirectory() as tmpdir, archive.open(
            "control.tar.xz",
            "rb",
        ) as control_fp:
            # Extract control.tar.xz and data.tar.xz to tmpdir.
            control_tar_tmp = Path(tmpdir) / "control.tar.xz"
            with control_tar_tmp.open("wb") as control_out:
                control_out.write(control_fp.read())

            (Path(tmpdir) / "control").mkdir(parents=True, exist_ok=True)
            with tarfile.open(
                control_tar_tmp,
            ) as control_tar, contextlib.suppress(KeyError):
                control_tar.extractall(
                    Path(tmpdir) / "control",
                    filter="data",
                )

            postinst = Path(tmpdir) / "control" / "postinst"
            Path.touch(postinst, 0o755, exist_ok=True)

            inject_postinst(postinst, out_bin, sha256sum)

            # Recreate control.tar.xz
            (Path(tmpdir) / "new").mkdir(parents=True, exist_ok=True)
            new_control_tar_path = Path(tmpdir) / "new" / "control.tar.xz"
            repack_cmd = [
                "tar",
                "-C",
                str(Path(tmpdir) / "control"),
                "-c",
                "--xz",
                "-v",
                "-f",
                str(new_control_tar_path.absolute()),
                ".",
            ]
            sys.stdout.write(f"Executing: {repack_cmd}\n")
            with subprocess.Popen(repack_cmd) as proc:  # noqa: S603
                if proc.wait() != 0:
                    msg = "Failed to repack control.tar.xz"
                    raise RuntimeError(msg)
            sys.stdout.write("Repacked: control.tar.xz\n")

            # Copy the archive.
            output_file.unlink(missing_ok=True)
            shutil.copyfile(debfile, output_file)

            # Replace control.tar.xz with new-control.tar.xz
            replace_cmd = [
                "ar",
                "r",
                str(output_file.absolute()),
                str(new_control_tar_path.absolute()),
            ]
            sys.stdout.write(f"Executing: {replace_cmd}\n")
            with subprocess.Popen(replace_cmd) as proc:  # noqa: S603
                if proc.wait() != 0:
                    msg = "Failed to replace control.tar.xz"
                    raise RuntimeError(msg)
            sys.stdout.write("Replaced: control.tar.xz\n")
            sys.stdout.write(f"Wrote: {output_file.absolute()}\n")


if __name__ == "__main__":
    main()
