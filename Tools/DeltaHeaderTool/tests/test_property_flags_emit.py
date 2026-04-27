from generator import _apply_property_flags


def test_apply_property_flags_hide_only_wraps_new():
    code = '    cls->AddProperty(new DFloatProperty("x", offsetof(C, x)));\n'
    out = _apply_property_flags(code, editor_only=False, hide_in_details=True)
    assert "_reg_prop->bHideInDetails = true;" in out
    assert "cls->AddProperty(_reg_prop);" in out
    assert "bEditorOnly" not in out
    assert "new DFloatProperty" in out


def test_apply_property_flags_editor_and_hide_wraps_new():
    code = '    cls->AddProperty(new DFloatProperty("x", offsetof(C, x)));\n'
    out = _apply_property_flags(code, editor_only=True, hide_in_details=True)
    assert "_reg_prop->bEditorOnly = true;" in out
    assert "_reg_prop->bHideInDetails = true;" in out
    assert "cls->AddProperty(_reg_prop);" in out
    eo = out.find("_reg_prop->bEditorOnly = true;")
    hd = out.find("_reg_prop->bHideInDetails = true;")
    assert 0 <= eo < hd


def test_apply_property_flags_inserts_before_addproperty_identifier():
    code = (
        "    {\n"
        '        auto* _prop = new DFloatProperty("x", offsetof(C, x));\n'
        "        cls->AddProperty(_prop);\n"
        "    }\n"
    )
    out = _apply_property_flags(code, editor_only=True, hide_in_details=True)
    assert "    _prop->bEditorOnly = true;\n" in out
    assert "    _prop->bHideInDetails = true;\n" in out
    add = out.find("cls->AddProperty(_prop);")
    eo = out.find("_prop->bEditorOnly = true;")
    assert 0 <= eo < add


def test_apply_property_flags_neither():
    code = '    cls->AddProperty(new DFloatProperty("x", offsetof(C, x)));\n'
    assert _apply_property_flags(code, editor_only=False, hide_in_details=False) == code
