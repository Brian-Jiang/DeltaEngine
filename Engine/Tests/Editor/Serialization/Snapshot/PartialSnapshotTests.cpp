#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "Runtime/Core/UUID.h"
#include "Runtime/Serialization/ObjectSnapshot.h"
#include "Runtime/Serialization/ObjectSnapshotReader.h"
#include "Runtime/Serialization/ObjectSnapshotWriter.h"
#include "Runtime/Test/SnapshotTestTypes.h"

using namespace DeltaEngine;

namespace
{
ObjectSnapshot MakeManualSnapshot(const std::string& className, const std::string& objectId,
                                  nlohmann::json fields)
{
    fields["_class"]    = className;
    fields["_objectId"] = objectId;

    ObjectSnapshot snapshot;
    snapshot.rootClassName = className;
    snapshot.rootJson      = {{"objects", nlohmann::json::array({std::move(fields)})}};
    snapshot.capturedIds   = {ObjectId::FromString(objectId)};
    return snapshot;
}
}

class PartialSnapshotTests : public ::testing::Test
{
};

TEST_F(PartialSnapshotTests, MissingFieldsUseClassDefaults)
{
    const std::string objectId = ObjectId::Generate().ToString();

    ObjectSnapshot snapshot = MakeManualSnapshot(
        "DSnapshotTestComponentA",
        objectId,
        {{"m_value", 77.0f}});

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(snapshot, nullptr, nullptr);

    ASSERT_NE(restored, nullptr);
    auto* comp = dynamic_cast<DSnapshotTestComponentA*>(restored);
    ASSERT_NE(comp, nullptr);

    EXPECT_FLOAT_EQ(comp->m_value, 77.0f);
    EXPECT_EQ(comp->m_tag, "CompA");
    EXPECT_EQ(comp->m_sibling, nullptr);
}

TEST_F(PartialSnapshotTests, ExtraFieldsAreIgnored)
{
    const std::string objectId = ObjectId::Generate().ToString();

    ObjectSnapshot snapshot = MakeManualSnapshot(
        "DSnapshotTestComponentA",
        objectId,
        {
            {"m_value",       55.0f},
            {"m_tag",         "custom"},
            {"m_nonexistent", 999},
            {"m_mana",        "arcane"},
        });

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(snapshot, nullptr, nullptr);

    ASSERT_NE(restored, nullptr);
    auto* comp = dynamic_cast<DSnapshotTestComponentA*>(restored);
    ASSERT_NE(comp, nullptr);

    EXPECT_FLOAT_EQ(comp->m_value, 55.0f);
    EXPECT_EQ(comp->m_tag, "custom");
}

TEST_F(PartialSnapshotTests, OmittedPtrFieldRestoresAsNull)
{
    const std::string objectId = ObjectId::Generate().ToString();

    ObjectSnapshot snapshot = MakeManualSnapshot(
        "DSnapshotTestComponentB",
        objectId,
        {{"m_count", 42}});

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(snapshot, nullptr, nullptr);

    ASSERT_NE(restored, nullptr);
    auto* comp = dynamic_cast<DSnapshotTestComponentB*>(restored);
    ASSERT_NE(comp, nullptr);

    EXPECT_EQ(comp->m_count, 42);
    EXPECT_EQ(comp->m_sibling, nullptr);
}

TEST_F(PartialSnapshotTests, RoundTripNonDefaultValues)
{
    auto* comp = new DSnapshotTestComponentA();
    comp->SetObjectId(ObjectId::Generate());
    comp->m_value = 9.5f;
    comp->m_tag   = "hello";

    ObjectSnapshotWriter writer;
    ObjectSnapshot snapshot = writer.Capture(comp);
    delete comp;

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(snapshot, nullptr, nullptr);

    ASSERT_NE(restored, nullptr);
    auto* rA = dynamic_cast<DSnapshotTestComponentA*>(restored);
    ASSERT_NE(rA, nullptr);

    EXPECT_FLOAT_EQ(rA->m_value, 9.5f);
    EXPECT_EQ(rA->m_tag, "hello");
    EXPECT_EQ(rA->m_sibling, nullptr);
}

TEST_F(PartialSnapshotTests, RoundTripDefaultValues)
{
    auto* comp = new DSnapshotTestComponentA();
    comp->SetObjectId(ObjectId::Generate());

    ObjectSnapshotWriter writer;
    ObjectSnapshot snapshot = writer.Capture(comp);
    delete comp;

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(snapshot, nullptr, nullptr);

    ASSERT_NE(restored, nullptr);
    auto* rA = dynamic_cast<DSnapshotTestComponentA*>(restored);
    ASSERT_NE(rA, nullptr);

    EXPECT_FLOAT_EQ(rA->m_value, 42.0f);
    EXPECT_EQ(rA->m_tag, "CompA");
    EXPECT_EQ(rA->m_sibling, nullptr);
}

TEST_F(PartialSnapshotTests, UnknownClassEntriesInArrayAreSkipped)
{
    const std::string validId   = ObjectId::Generate().ToString();
    const std::string missingId = ObjectId::Generate().ToString();

    ObjectSnapshot snapshot;
    snapshot.rootClassName = "DSnapshotTestComponentA";
    snapshot.rootJson = {
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class",    "DUnknownDeletedClass"},
                {"_objectId", missingId},
                {"m_foo",     123}
            },
            nlohmann::json{
                {"_class",    "DSnapshotTestComponentA"},
                {"_objectId", validId},
                {"m_value",   33.0f},
                {"m_tag",     "kept"}
            }
        })}
    };
    snapshot.capturedIds = {
        ObjectId::FromString(missingId),
        ObjectId::FromString(validId)
    };

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(snapshot, nullptr, nullptr);

    ASSERT_NE(restored, nullptr);
    auto* comp = dynamic_cast<DSnapshotTestComponentA*>(restored);
    ASSERT_NE(comp, nullptr);

    EXPECT_FLOAT_EQ(comp->m_value, 33.0f);
    EXPECT_EQ(comp->m_tag, "kept");
}
