import pytest

from tests.clang_util import require_libclang
from parser import libclang_library_path
from type_resolver import TYPE_MAP, resolve_type_from_string

requires_libclang = pytest.mark.skipif(
    not libclang_library_path().is_file(),
    reason=f"libclang not found at {libclang_library_path()}",
)


@pytest.mark.parametrize(
    "cpp,expected_prop",
    [
        ("float", "DFloatProperty"),
        ("int", "DIntProperty"),
        ("int32_t", "DIntProperty"),
        ("unsigned int", "DIntProperty"),
        ("bool", "DBoolProperty"),
        ("double", "DDoubleProperty"),
        ("std::string", "DStringProperty"),
        ("std::filesystem::path", "DFilesystemPathProperty"),
        ("DirectX::SimpleMath::Vector3", "DVector3Property"),
        ("DirectX::SimpleMath::Quaternion", "DQuaternionProperty"),
        ("DirectX::XMMATRIX", "DFloat4x4Property"),
        ("DirectX::XMVECTOR", "DFloat4Property"),
        ("DirectX::BoundingBox", "DBoundingBoxProperty"),
        ("TBulkData", "DBulkDataProperty"),
        ("DeltaEngine::TBulkData", "DBulkDataProperty"),
    ],
)
def test_scalar_and_type_map_entries_resolve(cpp, expected_prop):
    r = resolve_type_from_string(cpp)
    assert r is not None
    assert r[0] == expected_prop
    assert r[1] is False


def test_basic_string_char_variants_resolve():
    for cpp in (
        "std::basic_string<char>",
        "std::basic_string<char, std::char_traits<char>>",
        "std::basic_string<char, std::char_traits<char>, std::allocator<char>>",
    ):
        r = resolve_type_from_string(cpp)
        assert r is not None
        assert r[0] == "DStringProperty"


def test_vector_float_inner_dfloat():
    r = resolve_type_from_string("std::vector<float>")
    assert r is not None
    assert r[0] == "DVectorProperty"
    assert r[3] == "float"
    assert r[4] == "DFloatProperty"
    assert r[5] is False


def test_vector_vector_float_nested_dvector():
    r = resolve_type_from_string("std::vector<std::vector<float>>")
    assert r is not None
    assert r[0] == "DVectorProperty"
    assert r[3] == "std::vector<float>"
    assert r[4] == "DVectorProperty"
    assert r[5] is False


def test_raw_pointer_dobject_ptr():
    r = resolve_type_from_string("DObject*")
    assert r == ("DObjectPtrProperty<DObject>", True, "DObject")


def test_unknown_type_returns_none():
    assert resolve_type_from_string("NotAReflectableType") is None


@pytest.mark.parametrize(
    "cpp",
    [
        "FDynamicMulticastDelegate",
        "FDynamicDelegate",
        "DeltaEngine::FDynamicMulticastDelegate",
        "FOnSomethingDynamicMulticastDelegate",
    ],
)
def test_delegate_property_types_resolve(cpp):
    r = resolve_type_from_string(cpp)
    assert r == ("DDelegateProperty", False, "")


def test_type_map_covers_listed_subclasses():
    expected = {
        "DFloatProperty",
        "DIntProperty",
        "DBoolProperty",
        "DDoubleProperty",
        "DStringProperty",
        "DVector3Property",
        "DQuaternionProperty",
        "DFilesystemPathProperty",
        "DFloat4Property",
        "DFloat4x4Property",
        "DBulkDataProperty",
    }
    assert set(TYPE_MAP.values()) >= expected


@requires_libclang
def test_enum_property_resolves_via_parser(fixtures_dir):
    require_libclang()
    from parser import parse_header

    result = parse_header(fixtures_dir / "denum_property.h", fixtures_dir)
    prop = next(p for p in result.classes[0].properties if p.name == "m_color")
    assert prop.property_class == "DEnumProperty<TestColor>"
    assert prop.is_enum is True
    assert prop.enum_type_name == "TestColor"


@requires_libclang
def test_vector_enum_property_resolves_via_parser(fixtures_dir):
    require_libclang()
    from parser import parse_header

    result = parse_header(fixtures_dir / "denum_property.h", fixtures_dir)
    prop = next(p for p in result.classes[0].properties if p.name == "m_colors")
    assert prop.is_vector is True
    assert prop.inner_property_class == "DEnumProperty<TestColor>"
    assert prop.inner_cpp_type == "TestColor"
