from pathlib import Path

import pytest

from tests.clang_util import require_libclang
from generator import generate_header_file, generate_source_file
from parser import libclang_library_path, parse_header

requires_libclang = pytest.mark.skipif(
    not libclang_library_path().is_file(),
    reason=f"libclang not found at {libclang_library_path()}",
)


def _norm_newlines(s: str) -> str:
    return s.replace("\r\n", "\n")


def _read_snapshot(path: Path) -> str:
    return _norm_newlines(path.read_text(encoding="utf-8"))


def _write_snapshot(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(_norm_newlines(content), encoding="utf-8", newline="\n")


@requires_libclang
def test_generate_header_contains_expected_markers(fixtures_dir):
    require_libclang()
    path = fixtures_dir / "simple_class.h"
    result = parse_header(path, fixtures_dir)
    h = generate_header_file(result.classes, result.source_includes, result.forward_decls)
    assert "ReflectionRegisterFn_SimpleReflectClass" in h
    assert "ReflectionRegister_SimpleReflectClass" in h
    assert "CreateDObject<SimpleReflectClass>" in h
    assert "SimpleReflectClass_DoSomething_Params" in h


@requires_libclang
def test_generate_source_contains_expected_markers(fixtures_dir):
    require_libclang()
    path = fixtures_dir / "simple_class.h"
    result = parse_header(path, fixtures_dir)
    stem = path.stem
    cpp = generate_source_file(result.classes, stem, {})
    assert "DoSomething_Thunk" in cpp
    assert "static ReflectionRegistration registration_SimpleReflectClass" in cpp
    assert 'new DClass("SimpleReflectClass"' in cpp
    assert "offsetof(SimpleReflectClass, myFloat)" in cpp
    assert "offsetof(SimpleReflectClass, myInt)" in cpp


@requires_libclang
def test_generate_source_emits_show_as_button_metadata(fixtures_dir):
    require_libclang()
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


@requires_libclang
def test_generate_source_emits_hide_in_details_flags(fixtures_dir):
    require_libclang()
    path = fixtures_dir / "hide_in_details_property.h"
    result = parse_header(path, fixtures_dir)
    cpp = generate_source_file(result.classes, path.stem, {})
    assert '"m_hidden"' in cpp
    h0 = cpp.find('"m_hidden"')
    h1 = cpp.find("cls->AddProperty(_reg_prop);", h0)
    assert h1 > h0
    hidden_chunk = cpp[h0:h1]
    assert "_reg_prop->bHideInDetails = true;" in hidden_chunk
    assert "bEditorOnly" not in hidden_chunk
    assert '"m_editorHidden"' in cpp
    e0 = cpp.find('"m_editorHidden"')
    e1 = cpp.find("cls->AddProperty(_reg_prop);", e0)
    assert e1 > e0
    editor_hidden_chunk = cpp[e0:e1]
    assert "_reg_prop->bEditorOnly = true;" in editor_hidden_chunk
    assert "_reg_prop->bHideInDetails = true;" in editor_hidden_chunk


@requires_libclang
def test_generate_source_emits_class_metadata(fixtures_dir):
    require_libclang()
    path = fixtures_dir / "class_with_meta.h"
    result = parse_header(path, fixtures_dir)
    cpp = generate_source_file(result.classes, path.stem, {})
    # DCLASS metadata appears immediately after `new DClass("MetaTaggedClass"`
    cls_idx = cpp.find('new DClass("MetaTaggedClass"')
    assert cls_idx >= 0
    next_section = cpp.find("cls->AddProperty", cls_idx)
    assert next_section > cls_idx
    cls_chunk = cpp[cls_idx:next_section]
    assert 'cls->SetMetadata({' in cls_chunk
    assert '{"Category", "Gameplay"}' in cls_chunk
    assert '{"Tooltip", "A reflected thing"}' in cls_chunk

    # DSTRUCT metadata
    s_idx = cpp.find('new DStruct("MetaTaggedStruct"')
    assert s_idx >= 0
    s_end = cpp.find("RegisterDStruct", s_idx)
    s_chunk = cpp[s_idx:s_end]
    assert 'cls->SetMetadata({{"Category", "Math"}})' in s_chunk


@requires_libclang
def test_snapshots_simple_class(fixtures_dir, snapshot_dir, snapshot_update):
    require_libclang()
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


@requires_libclang
def test_generate_dynamic_delegate_markers(fixtures_dir):
    require_libclang()
    path = fixtures_dir / "dynamic_delegate.h"
    result = parse_header(path, fixtures_dir)
    h = generate_header_file(
        result.classes, result.source_includes, result.forward_decls, result.delegates,
    )
    cpp = generate_source_file(
        result.classes, path.stem, {}, result.delegates, f"{path.stem}.h", result.enums,
    )

    assert "struct FOnOneInt_Params" in h
    assert "int Value;" in h
    assert "struct FOnTwoArgs_Params" in h
    assert "int First;" in h
    assert "float Second;" in h

    assert "void FOnOneInt::Broadcast(int Value) const" in cpp
    assert "params.Value = Value;" in cpp
    assert "BroadcastWithParams(&params, 1);" in cpp
    assert "void FOnTwoArgs::Broadcast(int First, float Second) const" in cpp
    assert "BroadcastWithParams(&params, 2);" in cpp
    assert "void FOnExecutePlain::Execute() const" in cpp
    assert "ExecuteWithParams(nullptr, 0);" in cpp
    assert "void FOnExecuteInt::Execute(int Value) const" in cpp
    assert "ExecuteWithParams(&params, 1);" in cpp
    assert "new DDelegateProperty(" in cpp
    assert '"m_onOneInt"' in cpp


@requires_libclang
def test_generate_denum_property_markers(fixtures_dir):
    require_libclang()
    path = fixtures_dir / "denum_property.h"
    result = parse_header(path, fixtures_dir)
    cpp = generate_source_file(
        result.classes, path.stem, {}, result.delegates, f"{path.stem}.h", result.enums,
    )

    assert "Register_TestColor" in cpp
    assert 'e->AddEntry("Red", 0)' in cpp
    assert 'e->AddEntry("Green", 1)' in cpp
    assert "registration_TestColor" in cpp
    assert "DEnumProperty<TestColor>" in cpp
    assert '"m_color"' in cpp
    assert 'new DEnum("TestColor", "uint8_t")' in cpp
    assert '"m_colors"' in cpp
    assert 'new DEnumProperty<TestColor>("m_colors_elem", 0, "TestColor")' in cpp
    assert "DVectorProperty<TestColor>" in cpp


@requires_libclang
def test_generate_simple_enum_only(fixtures_dir):
    require_libclang()
    path = fixtures_dir / "simple_enum.h"
    result = parse_header(path, fixtures_dir)
    cpp = generate_source_file(
        result.classes, path.stem, {}, result.delegates, f"{path.stem}.h", result.enums,
    )

    assert "Register_TestOnlyEnum" in cpp
    assert "registration_TestOnlyEnum" in cpp
    assert 'new DEnum("TestOnlyEnum", "int")' in cpp
