from __future__ import annotations

import json
import subprocess
import sys
import time
from pathlib import Path

import pytest

SCRIPT = Path(__file__).resolve().parents[1] / "sync_directory.py"


def run_sync(source: Path, dest: Path, stamp: Path, *, verbose: bool = False) -> subprocess.CompletedProcess[str]:
    cmd = [
        sys.executable,
        str(SCRIPT),
        "--source",
        str(source),
        "--dest",
        str(dest),
        "--stamp",
        str(stamp),
    ]
    if verbose:
        cmd.append("--verbose")
    return subprocess.run(cmd, capture_output=True, text=True, check=False)


def read_manifest(stamp: Path) -> list[str]:
    return json.loads(stamp.read_text(encoding="utf-8"))["paths"]


def test_directory_sync_copies_new_files(tmp_path: Path) -> None:
    source = tmp_path / "src"
    dest = tmp_path / "dst"
    stamp = tmp_path / "sync.stamp"
    source.mkdir()
    (source / "a.txt").write_text("hello", encoding="utf-8")

    result = run_sync(source, dest, stamp)
    assert result.returncode == 0
    assert (dest / "a.txt").read_text(encoding="utf-8") == "hello"
    assert read_manifest(stamp) == ["a.txt"]


def test_directory_sync_skips_unchanged_files(tmp_path: Path) -> None:
    source = tmp_path / "src"
    dest = tmp_path / "dst"
    stamp = tmp_path / "sync.stamp"
    source.mkdir()
    (source / "a.txt").write_text("hello", encoding="utf-8")

    assert run_sync(source, dest, stamp).returncode == 0
    before = (dest / "a.txt").stat().st_mtime_ns
    assert run_sync(source, dest, stamp).returncode == 0
    after = (dest / "a.txt").stat().st_mtime_ns
    assert before == after


def test_directory_sync_updates_changed_files(tmp_path: Path) -> None:
    source = tmp_path / "src"
    dest = tmp_path / "dst"
    stamp = tmp_path / "sync.stamp"
    source.mkdir()
    file_path = source / "a.txt"
    file_path.write_text("v1", encoding="utf-8")

    assert run_sync(source, dest, stamp).returncode == 0
    time.sleep(0.05)
    file_path.write_text("v2", encoding="utf-8")
    assert run_sync(source, dest, stamp).returncode == 0
    assert (dest / "a.txt").read_text(encoding="utf-8") == "v2"


def test_directory_sync_removes_orphans_from_manifest(tmp_path: Path) -> None:
    source = tmp_path / "src"
    dest = tmp_path / "dst"
    stamp = tmp_path / "sync.stamp"
    source.mkdir()
    (source / "keep.txt").write_text("keep", encoding="utf-8")
    (source / "remove.txt").write_text("remove", encoding="utf-8")

    assert run_sync(source, dest, stamp).returncode == 0
    assert (dest / "remove.txt").is_file()

    (source / "remove.txt").unlink()
    assert run_sync(source, dest, stamp).returncode == 0
    assert not (dest / "remove.txt").exists()
    assert (dest / "keep.txt").is_file()
    assert read_manifest(stamp) == ["keep.txt"]


def test_shared_dest_does_not_delete_other_sync_files(tmp_path: Path) -> None:
    shared_dest = tmp_path / "bin"
    shared_dest.mkdir()

    source_a = tmp_path / "vendor_a"
    source_b = tmp_path / "vendor_b"
    source_a.mkdir()
    source_b.mkdir()
    (source_a / "a.dll").write_text("a", encoding="utf-8")
    (source_b / "b.dll").write_text("b", encoding="utf-8")

    stamp_a = tmp_path / "a.stamp"
    stamp_b = tmp_path / "b.stamp"

    assert run_sync(source_a, shared_dest, stamp_a).returncode == 0
    assert run_sync(source_b, shared_dest, stamp_b).returncode == 0
    assert (shared_dest / "a.dll").is_file()
    assert (shared_dest / "b.dll").is_file()

    (source_a / "a.dll").unlink()
    assert run_sync(source_a, shared_dest, stamp_a).returncode == 0
    assert not (shared_dest / "a.dll").exists()
    assert (shared_dest / "b.dll").is_file()


def test_file_to_file_sync(tmp_path: Path) -> None:
    source = tmp_path / "src.dll"
    dest = tmp_path / "bin" / "src.dll"
    stamp = tmp_path / "sync.stamp"
    source.write_text("dll", encoding="utf-8")

    result = run_sync(source, dest, stamp)
    assert result.returncode == 0
    assert dest.read_text(encoding="utf-8") == "dll"
    assert read_manifest(stamp) == ["src.dll"]


def test_missing_source_returns_error(tmp_path: Path) -> None:
    result = run_sync(tmp_path / "missing", tmp_path / "dest", tmp_path / "sync.stamp")
    assert result.returncode != 0
    assert "ERROR" in result.stderr
