#include "Runtime/Reflection/ReflectionRegistry.h"

#include <gtest/gtest.h>

namespace
{
class DeltaReflectionEnvironment final : public ::testing::Environment
{
public:
    void SetUp() override
    {
        DeltaEngine::GetReflectionRegistry().FinalizeRegistration();
    }
};

[[maybe_unused]] const ::testing::Environment* const kEnvironment =
    ::testing::AddGlobalTestEnvironment(new DeltaReflectionEnvironment());
}
