import pytest

from type_resolver import TYPE_MAP, resolve_type_from_string


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
        ("std::wstring", "DWStringProperty"),
        ("std::filesystem::path", "DFilesystemPathProperty"),
        ("DirectX::SimpleMath::Vector3", "DVector3Property"),
        ("DirectX::SimpleMath::Quaternion", "DQuaternionProperty"),
        ("DirectX::XMMATRIX", "DFloat4x4Property"),
        ("DirectX::XMVECTOR", "DFloat4Property"),
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


def test_type_map_covers_listed_subclasses():
    expected = {
        "DFloatProperty",
        "DIntProperty",
        "DBoolProperty",
        "DDoubleProperty",
        "DStringProperty",
        "DVector3Property",
        "DQuaternionProperty",
        "DWStringProperty",
        "DFilesystemPathProperty",
        "DFloat4Property",
        "DFloat4x4Property",
        "DBulkDataProperty",
    }
    assert set(TYPE_MAP.values()) >= expected
