"""DeltaHeaderTool — C++ reflection code generator for DeltaEngine.

Pass 1: fast text pre-scan (no libclang) to find headers containing DCLASS() or DSTRUCT().
Pass 2: parallel libclang AST parse + code generation via multiprocessing.Pool.
"""

from __future__ import annotations

import argparse
import os
import sys
import time
from multiprocessing import Pool
from pathlib import Path


# ── fast pre-scan (no libclang) ──────────────────────────────


def _contains_reflected_macro(path: Path) -> bool:
    """Return True if the file uses reflection or dynamic delegate macros."""
    try:
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            stripped = line.lstrip()
            if stripped.startswith("#"):
                continue
            if "DCLASS(" in stripped or "DSTRUCT(" in stripped:
                return True
            if "DECLARE_DYNAMIC_DELEGATE" in stripped or "DECLARE_DYNAMIC_MULTICAST_DELEGATE" in stripped:
                return True
        return False
    except OSError:
        return False


def _build_type_to_header_map(input_dir: Path, engine_include_root: Path) -> dict[str, str]:
    """Map type name (e.g. 'GameObject') -> include path (e.g. 'Runtime/Core/GameObject.h').
    Paths are relative to engine_include_root (Engine/) to match parser's include_path format."""
    type_to_header: dict[str, str] = {}
    for path in input_dir.rglob("*.h"):
        rel = path.relative_to(engine_include_root)
        include_path = rel.as_posix()
        type_name = path.stem
        type_to_header[type_name] = include_path
    return type_to_header


def _is_up_to_date(source: Path, out_h: Path, out_cpp: Path) -> bool:
    if not out_h.exists() or not out_cpp.exists():
        return False
    src_mtime = source.stat().st_mtime
    return (
        out_h.stat().st_mtime >= src_mtime
        and out_cpp.stat().st_mtime >= src_mtime
    )


# ── multiprocessing worker ───────────────────────────────────


def _pool_init():
    """Called once per worker process to eagerly load libclang."""
    from parser import _ensure_configured

    try:
        _ensure_configured()
    except RuntimeError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)


def _process_one(job):
    """Parse one header and generate reflection code.

    Must live at module level so multiprocessing can pickle it.
    Returns (stem, header_text | None, source_text | None, warning | None,
             class_super_pairs, diagnostics_list).
    """
    header_path, input_dir, include_dirs, type_to_header = job
    stem = header_path.stem

    from parser import parse_header
    from generator import generate_header_file, generate_source_file

    try:
        result = parse_header(header_path, input_dir, include_dirs)
        diag_list = result.diagnostics.warnings

        class_super_pairs = [
            (cls.name, cls.base_name, str(cls.source_file))
            for cls in result.classes
        ]

        engine_include_root = input_dir.parent
        rel = header_path.relative_to(engine_include_root)
        parent_part = rel.parent.as_posix()
        header_include_path = (
            f"{parent_part}/{stem}.h" if parent_part != "." else f"{stem}.h"
        )

        if not result.classes and not result.delegates:
            return (
                stem, None, None,
                f"WARNING: {header_path.name} contains reflected macros but no "
                "classes or dynamic delegates were found",
                class_super_pairs, diag_list,
            )

        header_text = generate_header_file(
            result.classes,
            result.source_includes,
            result.forward_decls,
            result.delegates,
        )
        source_text = generate_source_file(
            result.classes, stem, type_to_header, result.delegates, header_include_path,
        )

        return (stem, header_text, source_text, None,
                class_super_pairs, diag_list)
    except Exception as e:
        return (stem, None, None, f"ERROR processing {header_path.name}: {e}",
                [], [])


# ── main ─────────────────────────────────────────────────────


def main() -> int:
    ap = argparse.ArgumentParser(description="DeltaHeaderTool")
    ap.add_argument("--input-dir", required=True, type=Path)
    ap.add_argument("--output-dir", required=True, type=Path)
    ap.add_argument("--manifest", required=True, type=Path)
    ap.add_argument("--engine-root", required=True, type=Path)
    ap.add_argument("--include-dir", action="append", default=[], type=Path)
    ap.add_argument("--force", action="store_true")
    ap.add_argument(
        "-j", "--jobs", type=int, default=0,
        help="Max parallel workers (0 = cpu count)",
    )
    args = ap.parse_args()

    input_dir: Path = args.engine_root / args.input_dir
    output_dir: Path = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)

    t0 = time.perf_counter()

    # ── pass 1: fast text pre-scan (no libclang) ──────────────
    all_headers = sorted(input_dir.rglob("*.h"))
    candidates: list[Path] = [h for h in all_headers if _contains_reflected_macro(h)]

    t_scan = time.perf_counter()

    # ── filter by timestamp ───────────────────────────────────
    reflected_stems: set[str] = set()
    generated_cpps: list[Path] = []
    type_to_header = _build_type_to_header_map(input_dir, input_dir.parent)
    jobs: list[tuple[Path, Path, list[Path], dict[str, str]]] = []

    for header in candidates:
        stem = header.stem
        reflected_stems.add(stem)

        out_h = output_dir / f"{stem}.generated.h"
        out_cpp = output_dir / f"{stem}.generated.cpp"

        if not args.force and _is_up_to_date(header, out_h, out_cpp):
            generated_cpps.append(out_cpp)
            print(f"  up-to-date: {stem}")
            continue

        jobs.append((header, input_dir, args.include_dir, type_to_header))

    # ── pass 2: parallel libclang parse + codegen ─────────────
    from diagnostics import DiagnosticCollector

    all_diagnostics = DiagnosticCollector()
    all_class_super_pairs: list[tuple[str, str, str]] = []

    if jobs:
        max_workers = args.jobs if args.jobs > 0 else (os.cpu_count() or 1)
        num_workers = min(max_workers, len(jobs))

        if num_workers <= 1:
            _pool_init()
            results = [_process_one(j) for j in jobs]
        else:
            with Pool(processes=num_workers, initializer=_pool_init) as pool:
                results = pool.map(_process_one, jobs)

        for stem, header_text, source_text, warning, class_super_pairs, diag_list in results:
            if warning:
                print(f"  {warning}", file=sys.stderr)

            all_class_super_pairs.extend(class_super_pairs)
            all_diagnostics.warnings.extend(diag_list)

            if header_text is None:
                continue

            out_h = output_dir / f"{stem}.generated.h"
            out_cpp = output_dir / f"{stem}.generated.cpp"
            out_h.write_text(header_text, encoding="utf-8", newline="\n")
            out_cpp.write_text(source_text, encoding="utf-8", newline="\n")
            generated_cpps.append(out_cpp)
            print(f"  generated:  {stem}")

    # ── post-pass: check super class references ───────────────
    all_reflected_names = {name for name, _, _ in all_class_super_pairs}
    all_reflected_names.add("DObject")
    for class_name, super_name, source_file in all_class_super_pairs:
        if super_name and super_name not in all_reflected_names:
            all_diagnostics.warn(source_file, 0,
                                 f"DCLASS '{class_name}' super '{super_name}' "
                                 f"was not found among reflected classes")

    # ── emit all diagnostics ──────────────────────────────────
    all_diagnostics.emit_all()

    # ── stale file cleanup ────────────────────────────────────
    for f in sorted(output_dir.iterdir()):
        if f.suffix not in (".h", ".cpp"):
            continue
        raw_stem = f.stem
        if raw_stem.endswith(".generated"):
            raw_stem = raw_stem[: -len(".generated")]
        if raw_stem not in reflected_stems:
            f.unlink()
            print(f"  removed stale: {f.name}")

    # ── write manifest ────────────────────────────────────────
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    lines = ["# AUTO-GENERATED by DeltaHeaderTool - DO NOT EDIT\n"]
    lines.append("set(DELTA_GENERATED_SOURCES\n")
    for cpp in sorted(generated_cpps):
        lines.append(f'    "{cpp.resolve().as_posix()}"\n')
    lines.append(")\n")
    args.manifest.write_text("".join(lines), encoding="utf-8", newline="\n")

    elapsed = time.perf_counter() - t0
    scan_ms = (t_scan - t0) * 1000
    print(
        f"  DeltaHeaderTool: scanned {len(all_headers)} headers in {scan_ms:.0f}ms, "
        f"{len(candidates)} reflected, {len(jobs)} regenerated  "
        f"({elapsed:.2f}s total)"
    )

    return 0


if __name__ == "__main__":
    sys.exit(main())
