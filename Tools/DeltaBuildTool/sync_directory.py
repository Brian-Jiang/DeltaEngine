"""Incremental directory/file sync for DeltaEngine build pipeline."""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path


def _read_manifest(stamp_path: Path) -> list[str]:
    if not stamp_path.is_file():
        return []
    try:
        data = json.loads(stamp_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return []
    paths = data.get("paths")
    if not isinstance(paths, list):
        return []
    return [str(p) for p in paths]


def _write_manifest(stamp_path: Path, paths: list[str]) -> None:
    stamp_path.parent.mkdir(parents=True, exist_ok=True)
    payload = {"paths": sorted(paths)}
    stamp_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def _needs_copy(source: Path, dest: Path) -> bool:
    if not dest.is_file():
        return True
    try:
        source_stat = source.stat()
        dest_stat = dest.stat()
    except OSError:
        return True
    return source_stat.st_size != dest_stat.st_size or source_stat.st_mtime_ns != dest_stat.st_mtime_ns


def _collect_source_files(source_dir: Path) -> dict[str, Path]:
    files: dict[str, Path] = {}
    for path in source_dir.rglob("*"):
        if path.is_file():
            rel = path.relative_to(source_dir).as_posix()
            files[rel] = path
    return files


def _sync_file(source: Path, dest: Path, verbose: bool) -> bool:
    if not _needs_copy(source, dest):
        return False
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, dest)
    if verbose:
        print(f"copied: {source} -> {dest}")
    return True


def sync_directory(source_dir: Path, dest_dir: Path, stamp_path: Path, verbose: bool) -> int:
    if not source_dir.is_dir():
        print(f"ERROR: source directory not found: {source_dir}", file=sys.stderr)
        return 1

    dest_dir.mkdir(parents=True, exist_ok=True)

    current_files = _collect_source_files(source_dir)
    previous_paths = set(_read_manifest(stamp_path))
    current_paths = set(current_files.keys())

    copied = 0
    for rel, src in sorted(current_files.items()):
        dst = dest_dir / rel
        if _sync_file(src, dst, verbose):
            copied += 1

    deleted = 0
    for rel in sorted(previous_paths - current_paths):
        orphan = dest_dir / rel
        if orphan.exists():
            orphan.unlink()
            deleted += 1
            if verbose:
                print(f"deleted orphan: {orphan}")

    _write_manifest(stamp_path, list(current_paths))

    if verbose:
        print(f"sync complete: copied={copied} deleted={deleted} paths={len(current_paths)}")
    return 0


def sync_file(source_file: Path, dest_file: Path, stamp_path: Path, verbose: bool) -> int:
    if not source_file.is_file():
        print(f"ERROR: source file not found: {source_file}", file=sys.stderr)
        return 1

    rel = dest_file.name
    previous_paths = set(_read_manifest(stamp_path))

    copied = _sync_file(source_file, dest_file, verbose)

    deleted = 0
    for old_rel in sorted(previous_paths - {rel}):
        orphan = dest_file.parent / old_rel
        if orphan.exists():
            orphan.unlink()
            deleted += 1
            if verbose:
                print(f"deleted orphan: {orphan}")

    _write_manifest(stamp_path, [rel])

    if verbose:
        print(f"sync complete: copied={1 if copied else 0} deleted={deleted} paths=1")
    return 0


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Incrementally sync a file or directory tree.")
    parser.add_argument("--source", "-s", required=True, type=Path, help="Source file or directory")
    parser.add_argument("--dest", "-d", required=True, type=Path, help="Destination file or directory")
    parser.add_argument("--stamp", required=True, type=Path, help="Stamp manifest path")
    parser.add_argument("--verbose", action="store_true", help="Print copied/deleted paths")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    source = args.source.resolve()
    dest = args.dest.resolve()
    stamp = args.stamp.resolve()

    if source.is_dir():
        if dest.suffix:
            print("ERROR: destination must be a directory when source is a directory", file=sys.stderr)
            return 1
        return sync_directory(source, dest, stamp, args.verbose)

    if source.is_file():
        return sync_file(source, dest, stamp, args.verbose)

    print(f"ERROR: source path does not exist: {source}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
