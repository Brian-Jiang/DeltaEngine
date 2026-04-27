from generator import _apply_property_flags


def test_apply_property_flags_hide_only():
    code = '    cls->AddProperty(new DFloatProperty("x", offsetof(C, x)));\n'
    out = _apply_property_flags(code, editor_only=False, hide_in_details=True)
    assert "->bHideInDetails = true" in out
    assert "bEditorOnly" not in out


def test_apply_property_flags_editor_and_hide():
    code = '    cls->AddProperty(new DFloatProperty("x", offsetof(C, x)));\n'
    out = _apply_property_flags(code, editor_only=True, hide_in_details=True)
    assert "->bEditorOnly = true" in out
    assert "->bHideInDetails = true" in out
    assert out.count("->") == 3


def test_apply_property_flags_neither():
    code = '    cls->AddProperty(new DFloatProperty("x", offsetof(C, x)));\n'
    assert _apply_property_flags(code, editor_only=False, hide_in_details=False) == code
