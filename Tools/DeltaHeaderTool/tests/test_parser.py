import pytest

from tests.clang_util import require_libclang
from parser import libclang_library_path, parse_header

requires_libclang = pytest.mark.skipif(
    not libclang_library_path().is_file(),
    reason=f"libclang not found at {libclang_library_path()}",
)


@pytest.fixture
def parse_simple_class(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "simple_class.h", fixtures_dir)


@pytest.fixture
def parse_simple_struct(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "simple_struct.h", fixtures_dir)


@pytest.fixture
def parse_ptr_properties(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "ptr_properties.h", fixtures_dir)


@pytest.fixture
def parse_vector_properties(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "vector_properties.h", fixtures_dir)


@pytest.fixture
def parse_multi_class(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "multi_class.h", fixtures_dir)


@pytest.fixture
def parse_no_annotation(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "no_annotation.h", fixtures_dir)


@pytest.fixture
def parse_api_macro_class(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "api_macro_class.h", fixtures_dir)


@requires_libclang
def test_simple_class_name_and_counts(parse_simple_class):
    r = parse_simple_class
    assert len(r.classes) == 1
    c = r.classes[0]
    assert c.name == "SimpleReflectClass"
    assert c.is_struct is False
    assert len(c.properties) == 2
    assert len(c.functions) == 1


@requires_libclang
def test_simple_class_properties(parse_simple_class):
    c = parse_simple_class.classes[0]
    by_name = {p.name: p for p in c.properties}
    assert "myFloat" in by_name and "myInt" in by_name
    assert by_name["myFloat"].property_class == "DFloatProperty"
    assert by_name["myInt"].property_class == "DIntProperty"


@requires_libclang
def test_simple_class_function(parse_simple_class):
    fn = parse_simple_class.classes[0].functions[0]
    assert fn.name == "DoSomething"
    assert fn.return_type == "void"
    assert len(fn.params) == 1
    assert fn.params[0].name == "x"
    assert fn.params[0].property_class == "DFloatProperty"


@requires_libclang
def test_simple_struct_is_struct(parse_simple_struct):
    assert len(parse_simple_struct.classes) == 1
    c = parse_simple_struct.classes[0]
    assert c.name == "SimpleReflectStruct"
    assert c.is_struct is True
    assert len(c.properties) == 2


@requires_libclang
def test_ptr_properties_kinds(parse_ptr_properties):
    c = parse_ptr_properties.classes[0]
    by_name = {p.name: p for p in c.properties}
    raw = by_name["rawPtr"]
    assert raw.property_class == "DObjectPtrProperty<DObject>"
    assert raw.is_object_ptr is True
    assert raw.pointee_type == "DObject"


@requires_libclang
def test_vector_properties_depths(parse_vector_properties):
    c = parse_vector_properties.classes[0]
    by_name = {p.name: p for p in c.properties}
    flat = by_name["floatsFlat"]
    assert flat.is_vector is True
    assert flat.inner_property_class == "DFloatProperty"
    assert flat.inner_cpp_type == "float"
    nested = by_name["floatsNested"]
    assert nested.is_vector is True
    assert nested.inner_property_class == "DVectorProperty"
    assert nested.inner_cpp_type == "std::vector<float>"


@requires_libclang
def test_multi_class_two_results(parse_multi_class):
    names = {c.name for c in parse_multi_class.classes}
    assert names == {"MultiA", "MultiB"}


@requires_libclang
def test_no_annotation_empty(parse_no_annotation):
    assert parse_no_annotation.classes == []


@requires_libclang
def test_api_macro_class_base_names(parse_api_macro_class):
    by_name = {c.name: c for c in parse_api_macro_class.classes}
    assert by_name["ApiMacroBase"].base_name == "DObject"
    assert by_name["ApiMacroDerived"].base_name == "ApiMacroBase"


@pytest.fixture
def parse_show_as_button(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "show_as_button.h", fixtures_dir)


@requires_libclang
def test_show_as_button_metadata(parse_show_as_button):
    c = parse_show_as_button.classes[0]
    assert c.name == "ShowAsButtonClass"
    by_name = {f.name: f for f in c.functions}
    assert by_name["DoAction"].metadata == {"ShowAsButton": "true"}
    assert by_name["ComputeAndReport"].metadata == {"ShowAsButton": "true"}
    assert by_name["BadlyAnnotated"].metadata == {"ShowAsButton": "true"}
    assert by_name["PlainFn"].metadata == {}


@requires_libclang
def test_show_as_button_warns_on_nonzero_params(parse_show_as_button):
    diags = parse_show_as_button.diagnostics
    messages = [w.message for w in diags.warnings]
    assert any("ShowAsButton" in m and "BadlyAnnotated" in m for m in messages)
    assert not any("ShowAsButton" in m and "DoAction" in m for m in messages)


@pytest.fixture
def parse_hide_in_details(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "hide_in_details_property.h", fixtures_dir)


@requires_libclang
def test_hide_in_details_property_flags(parse_hide_in_details):
    c = parse_hide_in_details.classes[0]
    assert c.name == "HideDetailsTestClass"
    by_name = {p.name: p for p in c.properties}
    assert by_name["m_visible"].hide_in_details is False
    assert by_name["m_hidden"].hide_in_details is True
    assert by_name["m_editorHidden"].hide_in_details is True
    assert by_name["m_editorHidden"].editor_only is True


def test_dfunction_meta_parser_bare_identifier():
    from parser import _parse_dfunction_meta
    assert _parse_dfunction_meta("ShowAsButton") == {"ShowAsButton": "true"}
    assert _parse_dfunction_meta("") == {}
    assert _parse_dfunction_meta('Category="Debug"') == {"Category": "Debug"}
    assert _parse_dfunction_meta('ShowAsButton, Category="Debug"') == {
        "ShowAsButton": "true",
        "Category": "Debug",
    }
    assert _parse_dfunction_meta('meta=(UIType="Color")') == {"UIType": "Color"}


def test_parse_meta_kv_basic():
    from parser import _parse_meta_kv
    assert _parse_meta_kv("") == {}
    assert _parse_meta_kv("abstract") == {}
    assert _parse_meta_kv('meta=(Category="Foo")') == {"Category": "Foo"}
    assert _parse_meta_kv(
        'meta=(Category="Foo", Tooltip="Bar")'
    ) == {"Category": "Foo", "Tooltip": "Bar"}
    assert _parse_meta_kv(
        'abstract, meta=(Category="Foo")'
    ) == {"Category": "Foo"}


def test_parse_meta_kv_rejects_non_alnum_keys():
    from parser import _parse_meta_kv
    # Underscored keys must not be captured (spec: letters and digits only).
    assert _parse_meta_kv('meta=(under_score="x", Good1="y")') == {"Good1": "y"}


@pytest.fixture
def parse_class_with_meta(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "class_with_meta.h", fixtures_dir)


@requires_libclang
def test_class_with_meta_dclass_metadata(parse_class_with_meta):
    by_name = {c.name: c for c in parse_class_with_meta.classes}
    cls = by_name["MetaTaggedClass"]
    assert cls.metadata == {"Category": "Gameplay", "Tooltip": "A reflected thing"}
    assert cls.properties[0].metadata == {"UIType": "Color"}


@requires_libclang
def test_class_with_meta_dstruct_metadata(parse_class_with_meta):
    by_name = {c.name: c for c in parse_class_with_meta.classes}
    s = by_name["MetaTaggedStruct"]
    assert s.is_struct is True
    assert s.metadata == {"Category": "Math"}


@pytest.fixture
def parse_dynamic_delegate(fixtures_dir):
    require_libclang()
    return parse_header(fixtures_dir / "dynamic_delegate.h", fixtures_dir)


@requires_libclang
def test_dynamic_delegate_declarations(parse_dynamic_delegate):
    by_name = {d.name: d for d in parse_dynamic_delegate.delegates}
    assert len(parse_dynamic_delegate.delegates) == 5

    plain = by_name["FOnPlain"]
    assert plain.is_multicast is True
    assert plain.params == []
    assert plain.needs_codegen is False

    one = by_name["FOnOneInt"]
    assert one.is_multicast is True
    assert len(one.params) == 1
    assert one.params[0].cpp_type == "int"
    assert one.params[0].name == "Value"
    assert one.params[0].property_class == "DIntProperty"

    two = by_name["FOnTwoArgs"]
    assert two.is_multicast is True
    assert len(two.params) == 2
    assert two.params[0].cpp_type == "int"
    assert two.params[1].cpp_type == "float"

    execute = by_name["FOnExecuteInt"]
    assert execute.is_multicast is False
    assert len(execute.params) == 1


@requires_libclang
def test_dynamic_delegate_dproperty_resolution(parse_dynamic_delegate):
    host = next(c for c in parse_dynamic_delegate.classes if c.name == "DelegatePropertyHost")
    by_name = {p.name: p for p in host.properties}
    assert by_name["m_plainDelegate"].property_class == "DDelegateProperty"
    assert by_name["m_onOneInt"].property_class == "DDelegateProperty"
