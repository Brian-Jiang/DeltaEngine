#include "Editor/EditorWindows/EditorWindow.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
struct TestEditorWindow final : EditorWindow
{
    void Render(bool& open) override { }
};
}

static_assert(IsEditorWindow<TestEditorWindow>);
static_assert(!IsEditorWindow<int>);

TEST(EditorWindowConceptTests, ConceptSatisfiedForDerivedWindow)
{
    SUCCEED();
}
