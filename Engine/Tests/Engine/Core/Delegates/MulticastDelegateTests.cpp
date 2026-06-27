#include "Runtime/Core/Delegates/MulticastDelegate.h"

#include <gtest/gtest.h>

#include <vector>

using namespace DeltaEngine;

namespace
{

std::vector<int>* s_callOrder = nullptr;

void StaticSubscriber(int)
{
    if (s_callOrder)
        s_callOrder->push_back(2);
}

int s_staticCounter = 0;

void IncrementStaticCounter()
{
    ++s_staticCounter;
}

struct MulticastTestObject
{
    int hitCount = 0;

    void Ping() { ++hitCount; }

    void RecordAsThird(int)
    {
        if (s_callOrder)
            s_callOrder->push_back(3);
    }
};

}

TEST(MulticastDelegateTests, MultipleSubscribers_FireInRegistrationOrder)
{
    TMulticastDelegate<void(int)> multicast;
    std::vector<int> callOrder;
    s_callOrder = &callOrder;

    multicast.AddLambda([](int) { s_callOrder->push_back(1); });
    multicast.AddStatic(&StaticSubscriber);

    MulticastTestObject object;
    multicast.AddRaw(&object, &MulticastTestObject::RecordAsThird);

    callOrder.clear();
    multicast.Broadcast(0);

    ASSERT_EQ(callOrder.size(), 3u);
    EXPECT_EQ(callOrder[0], 1);
    EXPECT_EQ(callOrder[1], 2);
    EXPECT_EQ(callOrder[2], 3);

    s_callOrder = nullptr;
}

TEST(MulticastDelegateTests, Remove_ByHandle_StopsFutureBroadcasts)
{
    TMulticastDelegate<void()> multicast;
    int counter = 0;

    multicast.AddLambda([&counter]() { ++counter; });
    const FDelegateHandle middleHandle = multicast.AddLambda([&counter]() { counter += 10; });
    multicast.AddLambda([&counter]() { counter += 100; });

    multicast.Broadcast();
    EXPECT_EQ(counter, 111);

    EXPECT_TRUE(multicast.Remove(middleHandle));

    counter = 0;
    multicast.Broadcast();
    EXPECT_EQ(counter, 101);
}

TEST(MulticastDelegateTests, RemoveAll_ByObject_RemovesOnlyMatchingRawBindings)
{
    TMulticastDelegate<void()> multicast;

    MulticastTestObject objectA;
    MulticastTestObject objectB;
    int lambdaCounter = 0;
    s_staticCounter = 0;

    multicast.AddRaw(&objectA, &MulticastTestObject::Ping);
    multicast.AddRaw(&objectB, &MulticastTestObject::Ping);
    multicast.AddLambda([&lambdaCounter]() { ++lambdaCounter; });
    multicast.AddStatic(&IncrementStaticCounter);

    EXPECT_EQ(multicast.RemoveAll(&objectA), 1u);

    objectA.hitCount = 0;
    objectB.hitCount = 0;
    lambdaCounter = 0;
    s_staticCounter = 0;

    multicast.Broadcast();

    EXPECT_EQ(objectA.hitCount, 0);
    EXPECT_EQ(objectB.hitCount, 1);
    EXPECT_EQ(lambdaCounter, 1);
    EXPECT_EQ(s_staticCounter, 1);
}

TEST(MulticastDelegateTests, AddDuringBroadcast_DoesNotRunInSameBroadcast)
{
    TMulticastDelegate<void()> multicast;
    int nestedCounter = 0;
    int outerCounter = 0;

    multicast.AddLambda([&]()
    {
        ++outerCounter;
        multicast.AddLambda([&]() { ++nestedCounter; });
    });

    multicast.Broadcast();
    EXPECT_EQ(outerCounter, 1);
    EXPECT_EQ(nestedCounter, 0);

    multicast.Broadcast();
    EXPECT_EQ(outerCounter, 2);
    EXPECT_EQ(nestedCounter, 1);
}

TEST(MulticastDelegateTests, RemoveDuringBroadcast_StillRunsInCurrentBroadcast)
{
    TMulticastDelegate<void()> multicast;
    int firstCounter = 0;
    int secondCounter = 0;

    const FDelegateHandle secondHandle = multicast.AddLambda([&]() { ++secondCounter; });

    multicast.AddLambda([&]()
    {
        ++firstCounter;
        multicast.Remove(secondHandle);
    });

    multicast.Broadcast();
    EXPECT_EQ(firstCounter, 1);
    EXPECT_EQ(secondCounter, 1);

    firstCounter = 0;
    secondCounter = 0;

    multicast.Broadcast();
    EXPECT_EQ(firstCounter, 1);
    EXPECT_EQ(secondCounter, 0);
}

TEST(MulticastDelegateTests, Clear_DuringBroadcast_SnapshotStillRuns)
{
    TMulticastDelegate<void()> multicast;
    int firstCounter = 0;
    int secondCounter = 0;

    multicast.AddLambda([&]()
    {
        ++firstCounter;
        multicast.Clear();
    });
    multicast.AddLambda([&]() { ++secondCounter; });

    multicast.Broadcast();
    EXPECT_EQ(firstCounter, 1);
    EXPECT_EQ(secondCounter, 1);
    EXPECT_FALSE(multicast.IsBound());

    firstCounter = 0;
    secondCounter = 0;

    multicast.Broadcast();
    EXPECT_EQ(firstCounter, 0);
    EXPECT_EQ(secondCounter, 0);
}

TEST(MulticastDelegateTests, IsBound_ReflectsEntryCount)
{
    TMulticastDelegate<void()> multicast;

    EXPECT_FALSE(multicast.IsBound());

    multicast.AddLambda([]() {});
    EXPECT_TRUE(multicast.IsBound());

    multicast.Clear();
    EXPECT_FALSE(multicast.IsBound());
}

TEST(MulticastDelegateTests, InvalidHandle_Remove_ReturnsFalse)
{
    TMulticastDelegate<void()> multicast;
    multicast.AddLambda([]() {});

    EXPECT_FALSE(multicast.Remove(FDelegateHandle{}));
}
