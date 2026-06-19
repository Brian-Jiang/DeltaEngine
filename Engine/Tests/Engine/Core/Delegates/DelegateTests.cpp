#include "Runtime/Core/Delegates/Delegate.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{

int StaticAdd(int a, int b)
{
    return a + b;
}

void StaticIncrementCounter()
{
    ++s_voidCounter;
}

int s_voidCounter = 0;

struct DelegateTestObject
{
    int value = 10;

    int Add(int a, int b) { return a + b + value; }

    int GetValue() const { return value; }
};

}

TEST(DelegateTests, BindStatic_Execute_ReturnsValue)
{
    TDelegate<int(int, int)> delegate;
    delegate.BindStatic(&StaticAdd);

    EXPECT_TRUE(delegate.IsBound());
    EXPECT_EQ(delegate.Execute(2, 3), 5);
}

TEST(DelegateTests, BindStatic_VoidSignature_Executes)
{
    s_voidCounter = 0;

    TDelegate<void()> delegate;
    delegate.BindStatic(&StaticIncrementCounter);

    EXPECT_TRUE(delegate.IsBound());
    delegate.Execute();
    EXPECT_EQ(s_voidCounter, 1);
}

TEST(DelegateTests, BindLambda_CapturesState)
{
    int multiplier = 3;

    TDelegate<int(int)> delegate;
    delegate.BindLambda([multiplier](int value) { return value * multiplier; });

    EXPECT_EQ(delegate.Execute(4), 12);
}

TEST(DelegateTests, BindRaw_NonConstMethod_InvokesOnObject)
{
    DelegateTestObject object;
    object.value = 5;

    TDelegate<int(int, int)> delegate;
    delegate.BindRaw(&object, &DelegateTestObject::Add);

    EXPECT_EQ(delegate.Execute(1, 2), 8);
}

TEST(DelegateTests, BindRaw_ConstMethod_InvokesOnObject)
{
    const DelegateTestObject object{42};

    TDelegate<int()> delegate;
    delegate.BindRaw(&object, &DelegateTestObject::GetValue);

    EXPECT_EQ(delegate.Execute(), 42);
}

TEST(DelegateTests, ExecuteIfBound_WhenUnbound_ReturnsDefault)
{
    TDelegate<int(int, int)> delegate;

    EXPECT_FALSE(delegate.IsBound());
    EXPECT_EQ(delegate.ExecuteIfBound(1, 2), 0);
}

TEST(DelegateTests, ExecuteIfBound_WhenUnbound_Void_NoOp)
{
    s_voidCounter = 0;

    TDelegate<void()> delegate;

    EXPECT_FALSE(delegate.IsBound());
    delegate.ExecuteIfBound();
    EXPECT_EQ(s_voidCounter, 0);
}

TEST(DelegateTests, Unbind_ClearsBinding)
{
    TDelegate<int(int, int)> delegate;
    delegate.BindStatic(&StaticAdd);

    EXPECT_TRUE(delegate.IsBound());

    delegate.Unbind();

    EXPECT_FALSE(delegate.IsBound());
    EXPECT_EQ(delegate.ExecuteIfBound(1, 2), 0);
}

TEST(DelegateTests, Rebind_ReplacesPreviousCallable)
{
    TDelegate<int(int, int)> delegate;
    delegate.BindStatic(&StaticAdd);
    EXPECT_EQ(delegate.Execute(1, 2), 3);

    delegate.BindLambda([](int a, int b) { return a * b; });
    EXPECT_EQ(delegate.Execute(4, 5), 20);
}
