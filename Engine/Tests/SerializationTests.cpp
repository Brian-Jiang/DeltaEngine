#include "Core/UUID.h"
#include "Serialization/ScriptPointer.h"
#include "Serialization/BulkDataHandle.h"
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <regex>

using namespace DeltaEngine;

static void test_uuid_roundtrip()
{
    UUID id = UUID::Generate();
    assert(!id.IsNull());
    std::string str = id.ToString();
    UUID parsed = UUID::FromString(str);
    assert(id == parsed);
}

static void test_uuid_null()
{
    UUID null = UUID::Null();
    assert(null.IsNull());
    assert(UUID::FromString("not-a-uuid").IsNull());
}

static void test_uuid_uniqueness()
{
    std::unordered_set<UUID> ids;
    for (int i = 0; i < 1000; ++i)
        ids.insert(UUID::Generate());
    assert(ids.size() == 1000);
}

static void test_uuid_format()
{
    std::regex re("^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$");
    for (int i = 0; i < 100; ++i)
    {
        UUID id = UUID::Generate();
        assert(std::regex_match(id.ToString(), re));
    }
}

static void test_script_pointer()
{
    ScriptPointer sp;
    assert(sp.IsNull());
    sp.m_assetId = UUID::Generate();
    sp.m_objectId = UUID::Generate();
    assert(!sp.IsNull());
    AssetId other = UUID::Generate();
    assert(sp.IsExternal(other));
    assert(!sp.IsExternal(sp.m_assetId));
}

static void test_bulk_data_handle()
{
    BulkDataHandle h;
    assert(!h.IsValid());
    h.m_dataSize = 1024;
    assert(h.IsValid());
}

static void test_uuid_hash_in_map()
{
    std::unordered_map<UUID, int> map;
    UUID key = UUID::Generate();
    map[key] = 42;
    assert(map[key] == 42);
}

int main()
{
    test_uuid_roundtrip();
    test_uuid_null();
    test_uuid_uniqueness();
    test_uuid_format();
    test_script_pointer();
    test_bulk_data_handle();
    test_uuid_hash_in_map();
    return 0;
}
