import pytest

from parser import parse_header


@pytest.fixture
def parse_simple_class(fixtures_dir):
    return parse_header(fixtures_dir / "simple_class.h", fixtures_dir)


@pytest.fixture
def parse_simple_struct(fixtures_dir):
    return parse_header(fixtures_dir / "simple_struct.h", fixtures_dir)


@pytest.fixture
def parse_ptr_properties(fixtures_dir):
    return parse_header(fixtures_dir / "ptr_properties.h", fixtures_dir)


@pytest.fixture
def parse_vector_properties(fixtures_dir):
    return parse_header(fixtures_dir / "vector_properties.h", fixtures_dir)


@pytest.fixture
def parse_multi_class(fixtures_dir):
    return parse_header(fixtures_dir / "multi_class.h", fixtures_dir)


@pytest.fixture
def parse_no_annotation(fixtures_dir):
    return parse_header(fixtures_dir / "no_annotation.h", fixtures_dir)


@pytest.fixture
def parse_api_macro_class(fixtures_dir):
    return parse_header(fixtures_dir / "api_macro_class.h", fixtures_dir)


def test_simple_class_name_and_counts(parse_simple_class):
    r = parse_simple_class
    assert len(r.classes) == 1
    c = r.classes[0]
    assert c.name == "SimpleReflectClass"
    assert c.is_struct is False
    assert len(c.properties) == 2
    assert len(c.functions) == 1


def test_simple_class_properties(parse_simple_class):
    c = parse_simple_class.classes[0]
    by_name = {p.name: p for p in c.properties}
    assert "myFloat" in by_name and "myInt" in by_name
    assert by_name["myFloat"].property_class == "DFloatProperty"
    assert by_name["myInt"].property_class == "DIntProperty"


def test_simple_class_function(parse_simple_class):
    fn = parse_simple_class.classes[0].functions[0]
    assert fn.name == "DoSomething"
    assert fn.return_type == "void"
    assert len(fn.params) == 1
    assert fn.params[0].name == "x"
    assert fn.params[0].property_class == "DFloatProperty"


def test_simple_struct_is_struct(parse_simple_struct):
    assert len(parse_simple_struct.classes) == 1
    c = parse_simple_struct.classes[0]
    assert c.name == "SimpleReflectStruct"
    assert c.is_struct is True
    assert len(c.properties) == 2


def test_ptr_properties_kinds(parse_ptr_properties):
    c = parse_ptr_properties.classes[0]
    by_name = {p.name: p for p in c.properties}
    raw = by_name["rawPtr"]
    assert raw.property_class == "DObjectPtrProperty<DObject>"
    assert raw.is_object_ptr is True
    assert raw.pointee_type == "DObject"


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


def test_multi_class_two_results(parse_multi_class):
    names = {c.name for c in parse_multi_class.classes}
    assert names == {"MultiA", "MultiB"}


def test_no_annotation_empty(parse_no_annotation):
    assert parse_no_annotation.classes == []


def test_api_macro_class_base_names(parse_api_macro_class):
    by_name = {c.name: c for c in parse_api_macro_class.classes}
    assert by_name["ApiMacroBase"].base_name == "DObject"
    assert by_name["ApiMacroDerived"].base_name == "ApiMacroBase"
