from pathlib import Path

from generator import generate_header_file, generate_source_file
from parser import parse_header


def _norm_newlines(s: str) -> str:
    return s.replace("\r\n", "\n")


def _read_snapshot(path: Path) -> str:
    return _norm_newlines(path.read_text(encoding="utf-8"))


def _write_snapshot(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(_norm_newlines(content), encoding="utf-8", newline="\n")


def test_generate_header_contains_expected_markers(fixtures_dir):
    path = fixtures_dir / "simple_class.h"
    result = parse_header(path, fixtures_dir)
    h = generate_header_file(result.classes, result.source_includes, result.forward_decls)
    assert "ReflectionRegisterFn_SimpleReflectClass" in h
    assert "ReflectionRegister_SimpleReflectClass" in h
    assert "CreateDObject<SimpleReflectClass>" in h
    assert "SimpleReflectClass_DoSomething_Params" in h


def test_generate_source_contains_expected_markers(fixtures_dir):
    path = fixtures_dir / "simple_class.h"
    result = parse_header(path, fixtures_dir)
    stem = path.stem
    cpp = generate_source_file(result.classes, stem, {})
    assert "DoSomething_Thunk" in cpp
    assert "static ReflectionRegistration registration_SimpleReflectClass" in cpp
    assert 'new DClass("SimpleReflectClass"' in cpp
    assert "offsetof(SimpleReflectClass, myFloat)" in cpp
    assert "offsetof(SimpleReflectClass, myInt)" in cpp


def test_generate_source_emits_show_as_button_metadata(fixtures_dir):
    path = fixtures_dir / "show_as_button.h"
    result = parse_header(path, fixtures_dir)
    cpp = generate_source_file(result.classes, path.stem, {})
    assert 'DFunction("DoAction"' in cpp
    assert 'DFunction("ComputeAndReport"' in cpp
    assert 'DFunction("BadlyAnnotated"' in cpp
    assert 'DFunction("PlainFn"' in cpp
    do_action_idx = cpp.find('DFunction("DoAction"')
    do_action_add = cpp.find("cls->AddFunction", do_action_idx)
    assert 'SetMetadata({{"ShowAsButton", "true"}})' in cpp[do_action_idx:do_action_add]
    plain_idx = cpp.find('DFunction("PlainFn"')
    plain_add = cpp.find("cls->AddFunction", plain_idx)
    assert "SetMetadata" not in cpp[plain_idx:plain_add]


def test_snapshots_simple_class(fixtures_dir, snapshot_dir, snapshot_update):
    path = fixtures_dir / "simple_class.h"
    result = parse_header(path, fixtures_dir)
    stem = path.stem
    h = _norm_newlines(
        generate_header_file(result.classes, result.source_includes, result.forward_decls)
    )
    cpp = _norm_newlines(generate_source_file(result.classes, stem, {}))
    snap_h = snapshot_dir / "simple_class.generated.h"
    snap_cpp = snapshot_dir / "simple_class.generated.cpp"
    if snapshot_update:
        _write_snapshot(snap_h, h)
        _write_snapshot(snap_cpp, cpp)
    else:
        assert snap_h.is_file(), "missing snapshot; run pytest with --snapshot-update"
        assert snap_cpp.is_file(), "missing snapshot; run pytest with --snapshot-update"
        assert h == _read_snapshot(snap_h)
        assert cpp == _read_snapshot(snap_cpp)
